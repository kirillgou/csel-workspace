/*
Fichier : lib.c
Auteur : Tanguy Dietrich / Kirill Goundiaev
Description :
Un daemon en espace utilisateur offrira les services pour une gestion manuelle. Ce daemon proposera deux interfaces de gestion distinctes, soit :

Interface physique via les boutons poussoir et LED Power de la carte d’extension
S1 pour augmenter la vitesse de rotation du ventilateur, la pression du S1 devra être signalisée sur la LED Power
S2 pour diminuer la vitesse de rotation du ventilateur, la pression du S2 devra être signalisée sur la LED Power
S3 pour changer du mode automatique au mode manuel et vice versa.
Interface IPC, au choix du développeur, permettant de dialoguer avec une application pour choisir le mode de fonctionnement et spécifier la fréquence de clignotement
Le daemon utilisera l’écran OLED pour indiquer le mode de fonctionnement, la température actuelle du microprocesseur ainsi que la fréquence de clignotement de la LED Status.

Une application fournira une interface utilisateur, une ligne de commande, pour piloter le système via l’interface IPC choisie.

nous avons decider d'utiliser un soket unix pour la communication entre le daemon et l'application
*/

*/
#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "daemonfanlib.h"


// GLOBAL VARIABLES
static int g_signal_catched = 0;
static int g_server_fd = 0;
struct sockaddr_un g_address;

static void catch_signal(int signal)
{
    syslog(LOG_INFO, "signal=%d catched\n", signal);
    g_signal_catched++;
}

void *threadWaitConnection(void *arg)
{
    // wait for a new connection on the socket
    int new_socket;
    struct sockaddr clientaddress;
    while(1) {
        if((new_socket = accept(g_server_fd, (struct sockaddr*)&clientaddress, sizeof(clientaddress))) < 0) {
            syslog(LOG_ERR, "accept");
            exit(EXIT_FAILURE);
        }
        syslog(LOG_INFO, "new connection accepted\n");
        // launch a thread to process the communication with the application
        pthread_t threadProcessMessage;
        pthread_create(&threadProcessMessage, NULL, processMessage, NULL);
        // TODO : GET RID OF THE join
        pthread_join(threadProcessMessage, NULL);
    }
}

void *processMessage(void *arg)
{
    while
}

void initsocket()
{
    int opt = 1;
    
    // init a socket for communication with the application
    if ((g_server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        syslog(LOG_ERR, "socket creation failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(g_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        syslog(LOG_ERR, "setsockopt");
        exit(EXIT_FAILURE);
    }
    g_address.sin_family = AF_INET;
    g_address.sin_addr.s_addr = INADDR_ANY;
    g_address.sin_port = htons(DAEMON_PORT);
    if (bind(g_server_fd, (struct sockaddr*)&g_address, sizeof(g_address)) < 0) {
        syslog(LOG_ERR, "bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(g_server_fd, 3) < 0) {
        syslog(LOG_ERR, "listen");
        exit(EXIT_FAILURE);
    }

    // launch a thread to wait for a new connection
    pthread_t threadWaitMessage;
    pthread_create(&threadWaitMessage, NULL, threadWaitMessage, NULL);


}

void initScreen()
{
    ssd1306_init();

    ssd1306_set_position (0,0);
    ssd1306_puts("CSEL1a - SP.07");
    ssd1306_set_position (0,1);
    ssd1306_puts("  Demo - SW");
    ssd1306_set_position (0,2);
    ssd1306_puts("--------------");

    ssd1306_set_position (0,3);
    ssd1306_puts("Temp: 35'C");
    ssd1306_set_position (0,4);
    ssd1306_puts("Freq: 1Hz");
    ssd1306_set_position (0,5);
    ssd1306_puts("Duty: 50%");
}

// this will be called when in daemon mode
void daemonFunc()
{
    int server_fd, new_socket, valread;
    int opt = 1;
    struct sockaddr_un address;
    // start by initialising the ssd1306 display
   initScreen();

    // init a socket for communication with the application
    
    // init the buttons S1, S2 and S3


    // main loop
    // while (signal_catched < 3) {
    // }
    // passive wait for signals
    // temporary solution
    while (1) {
        sleep(1);
    }
}

void generateDaemon()
{
    // 1. fork off the parent process
    fork_process();

    // 2. create new session
    if (setsid() == -1) {
        syslog(LOG_ERR, "ERROR while creating new session");
        exit(1);
    }

    // 3. fork again to get rid of session leading process
    fork_process();

    // 4. capture all required signals
    struct sigaction act = {
        .sa_handler = catch_signal,
    };
    sigaction(SIGHUP, &act, NULL);   //  1 - hangup
    sigaction(SIGINT, &act, NULL);   //  2 - terminal interrupt
    sigaction(SIGQUIT, &act, NULL);  //  3 - terminal quit
    sigaction(SIGABRT, &act, NULL);  //  6 - abort
    sigaction(SIGTERM, &act, NULL);  // 15 - termination
    sigaction(SIGTSTP, &act, NULL);  // 19 - terminal stop signal

    // 5. update file mode creation mask
    umask(0027);

    // 6. change working directory to appropriate place
    if (chdir("/opt") == -1) {
        syslog(LOG_ERR, "ERROR while changing to working directory");
        exit(1);
    }

    // 7. close all open file descriptors
    for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--) {
        close(fd);
    }

    // 8. redirect stdin, stdout and stderr to /dev/null
    if (open("/dev/null", O_RDWR) != STDIN_FILENO) {
        syslog(LOG_ERR, "ERROR while opening '/dev/null' for stdin");
        exit(1);
    }
    if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO) {
        syslog(LOG_ERR, "ERROR while opening '/dev/null' for stdout");
        exit(1);
    }
    if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO) {
        syslog(LOG_ERR, "ERROR while opening '/dev/null' for stderr");
        exit(1);
    }

    // 9. option: open syslog for message logging
    openlog(NULL, LOG_NDELAY | LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "Daemon has started...");

    // 10. option: get effective user and group id for appropriate's one
    struct passwd* pwd = getpwnam("daemon");
    if (pwd == 0) {
        syslog(LOG_ERR, "ERROR while reading daemon password file entry");
        exit(1);
    }

    // 11. option: change root directory
    if (chroot(".") == -1) {
        syslog(LOG_ERR, "ERROR while changing to new root directory");
        exit(1);
    }

    // 12. option: change effective user and group id for appropriate's one
    if (setegid(pwd->pw_gid) == -1) {
        syslog(LOG_ERR, "ERROR while setting new effective group id");
        exit(1);
    }
    if (seteuid(pwd->pw_uid) == -1) {
        syslog(LOG_ERR, "ERROR while setting new effective user id");
        exit(1);
    }

    // 13. implement daemon body...
    daemonFunc();

    syslog(LOG_INFO, "daemon stopped. Number of signals catched=%d\n", g_signal_catched);
    closelog();
}


static void fork_process()
{
    pid_t pid = fork();
    switch (pid) {
        case 0:
            break;  // child process has been created
        case -1:
            syslog(LOG_ERR, "ERROR while forking");
            exit(1);
            break;
        default:
            exit(0);  // exit parent process with success
    }
}