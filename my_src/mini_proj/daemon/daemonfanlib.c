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

Remarque :
nous avons decider d'utiliser un soket unix pour la communication entre le daemon et l'application.
Nous assumons qu'il n'y auras qu'une seul connection sur le socket.
*/

#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE

#include "daemonfanlib.h"
#include "gpio_utility.h"

static void fork_process();

void initSocket(int *mode, int *freq, pthread_t *thread_id, void* (*threadFunc)(void*))
{
    int opt = 1;
    socketParamThread *param = malloc(sizeof(socketParamThread));
        
    // init a socket for communication with the application
    syslog(LOG_INFO, "init socket\n");
    if ((param->server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        syslog(LOG_ERR, "socket creation failed");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "socket created\n");
    if (setsockopt(param->server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        syslog(LOG_ERR, "setsockopt");
        exit(EXIT_FAILURE);
    }
    param->address.sin_family = AF_INET;
    param->address.sin_addr.s_addr = INADDR_ANY;
    param->address.sin_port = htons(DAEMON_PORT);
    syslog(LOG_INFO, "bind socket\n");
    if (bind(param->server_fd, (struct sockaddr*)&param->address, sizeof(param->address)) < 0) {
        syslog(LOG_ERR, "bind failed");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "socket binded\n");
    if (listen(param->server_fd, 1) < 0) {
        syslog(LOG_ERR, "listen");
        exit(EXIT_FAILURE);
    }

    // TODO : make a join on the thread
    syslog(LOG_INFO, "running thread socket");
    param->mode = mode;
    param->freq = freq;
    pthread_create(thread_id, NULL, threadFunc, param);
    syslog(LOG_INFO, "socket initialized");
}


void generateDaemon(void (*catchSignalFunc)(int))
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
        .sa_handler = catchSignalFunc,
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
    syslog(LOG_INFO, "Daemon has started 8...");

    // 13. implement daemon body...
    syslog(LOG_INFO, "running daemon");

    // syslog(LOG_INFO, "daemon stopped. Number of signals catched=%d\n", g_signal_catched);
    // closelog();
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