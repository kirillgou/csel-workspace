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

static void catch_signal(int signal);
static void *threadSocket(void *arg);
static void initSocket();
static void initButtons();
static void initLeds();
static void initScreen();
static void daemonFunc();
static void fork_process();
static void writeMode(int mode);
static void writeFreq(int freq);


// GLOBAL VARIABLES
static int g_signal_catched = 0;
static int g_server_fd = 0;
static int g_client_fd = 0;
static int g_led_fd = 0;
static int g_mode = 0;
static int g_freq = 0;
static struct sockaddr_in g_address;
static struct sockaddr_in g_clientaddress;
static pthread_t g_thread_id;
static my_context g_ctx[3];


static void catch_signal(int signal)
{
    syslog(LOG_INFO, "signal=%d catched\n", signal);
    g_signal_catched++;
}

static void *threadSocket(void *arg)
{
    // TODO : ADD A SIGNAL TO STOP THE THREAD
    int addresslen = sizeof(g_clientaddress);
    //listen on the socket
    while(1) {
        char buffer[1024] = {0};
        int valread = read(g_client_fd, buffer, 1024);
        if (valread == 0) {
            syslog(LOG_INFO, "client disconnected\n");
            close(g_client_fd);
            g_client_fd = accept(g_server_fd, (struct sockaddr*)&g_clientaddress, ((socklen_t*) &addresslen));
        } else {
            syslog(LOG_INFO, "received: %s\n", buffer);
        }
    }
    return NULL;
}

static void writeMode(int mode)
{
    char str[32] = {0};
    // open MODULE_FILE_MODE in write mode
    int fd = open(MODULE_FILE_MODE, O_WRONLY);
    if (fd < 0) {
        syslog(LOG_ERR, "open %s failed\n", MODULE_FILE_MODE);
        exit(EXIT_FAILURE);
    }
    // write mode in the file
    char buffer[2] = {0};
    sprintf(buffer, "%d", mode);
    write(fd, buffer, strlen(buffer));
    close(fd);
    // add a line for the mode
    sprintf(str, "Mode: %d", mode);
    ssd1306_set_position (0,6);
    ssd1306_puts(str);
}

static void writeFreq(int freq)
{
    char str[32] = {0};
    int fd = open(MODULE_FILE_MODE, O_WRONLY);
    if (fd < 0) {
        syslog(LOG_ERR, "open %s failed\n", MODULE_FILE_MODE);
        exit(EXIT_FAILURE);
    }
    // write mode in the file
    char buffer[2] = {0};
    sprintf(buffer, "%d", freq);
    // TODO : check if write is ok
    write(fd, buffer, strlen(buffer));
    close(fd);
    // then write new data to the screen
    sprintf(str, "Freq: %dHz", freq);
    ssd1306_set_position (0 ,4);
    ssd1306_puts(str);
}

static void initSocket()
{
    int opt = 1;
    int adddresslen = sizeof(g_address);
    
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
    if (listen(g_server_fd, 1) < 0) {
        syslog(LOG_ERR, "listen");
        exit(EXIT_FAILURE);
    }

    if((g_client_fd = accept(g_server_fd, (struct sockaddr*)&g_clientaddress, ((socklen_t*) &adddresslen))) < 0) {
        syslog(LOG_ERR, "accept");
        exit(EXIT_FAILURE);
    }

    // TODO : make a join on the thread
    pthread_create(&g_thread_id, NULL, threadSocket, NULL);
    syslog(LOG_INFO, "socket initialized\n");
}

int open_button(const char *gpio_path, const char *gpio_num){
    // printf("open button %s %s\n", gpio_num, gpio_path);
    // unexport pin out of sysfs (reinitialization)
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, gpio_num, strlen(gpio_num));
    close(f);

    // export pin to sysfs
    f = open(GPIO_EXPORT, O_WRONLY);
    write(f, gpio_num, strlen(gpio_num));
    close(f);

    // config pin
    char *path = malloc(strlen(gpio_path) + strlen("/direction") + 1);
    strcpy(path, gpio_path);
    strcat(path, "/direction");
    f = open(path, O_WRONLY);
    write(f, "in", 2);
    close(f);
    free(path);

    // config edge
    path = malloc(strlen(gpio_path) + strlen("/edge") + 1);
    strcpy(path, gpio_path);
    strcat(path, "/edge");
    f = open(path, O_WRONLY);
    write(f, "rising", 6);
    // write(f, "falling", 7);
    // write(f, "both", 4);
    close(f);
    free(path);

    // open gpio value attribute
    path = malloc(strlen(gpio_path) + strlen("/value") + 1);
    strcpy(path, gpio_path);
    strcat(path, "/value");
    f = open(path, O_RDWR);
    free(path);
    return f;
}

static void initButtons()
{
    g_ctx[EV_BTN_1].ev = EV_BTN_1;
    g_ctx[EV_BTN_2].ev = EV_BTN_2;
    g_ctx[EV_BTN_3].ev = EV_BTN_3;
    g_ctx[EV_BTN_1].fd = open_button(GPIO_BTN_1, BTN_1);
    g_ctx[EV_BTN_2].fd = open_button(GPIO_BTN_2, BTN_2);
    g_ctx[EV_BTN_3].fd = open_button(GPIO_BTN_3, BTN_3);
    syslog(LOG_INFO, "buttons initialized\n");

}

static void initLeds()
{
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // export pin to sysfs
    f = open(GPIO_EXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // config pin
    f = open(GPIO_LED "/direction", O_WRONLY);
    write(f, "out", 3);
    close(f);
    g_led_fd = open(GPIO_LED "/value", O_WRONLY);
    syslog(LOG_INFO, "leds initialized\n");
}


static void initScreen()
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
    syslog(LOG_INFO, "screen initialized\n");
}

// this will be called when in daemon mode
static void daemonFunc()
{
    syslog(LOG_INFO, "daemon function\n");
    // start by initialising the ssd1306 display
    initScreen();

    // init a socket for communication with the application
    // initSocket();

    // init the buttons S1, S2 and S3
    initButtons();

    int epfd = epoll_create1(0);// parametre size isn't used anymore by Linux

    for(uint32_t i = 0; i < 3; i++){
        g_ctx[i].first_done = 0;
        struct epoll_event event = {
            // .events = EPOLLIN | EPOLLET, 
            .events = EPOLLET, // mode edge triggered
            .data.ptr = &g_ctx[i]
        };
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, g_ctx[i].fd, &event) == -1) {
            printf("error epoll_ctl: %s\n", strerror(errno));
            // return 1;
            exit(EXIT_FAILURE);
        }
    }
    // creation contexte epoll
    if (epfd == -1) {
        // printf("error epoll_create1: %s\n", strerror(errno));
        syslog(LOG_ERR, "epoll_create1");
        exit(EXIT_FAILURE);
    }

    // init the leds
    initLeds();

    

    while (1) {
        struct epoll_event event_arrived[3];
        syslog(LOG_INFO, "waiting for event epoll\n");
        int nr = epoll_wait(epfd, event_arrived, 3, -1);
        syslog(LOG_INFO, "event arrived\n");
        if (nr == -1) {
            // printf("error epoll_wait: %s\n", strerror(errno));
            syslog(LOG_ERR, "epoll_wait");
            exit(EXIT_FAILURE);
        }
        for(int i = 0; i < nr; i++){
            my_context *ctx = event_arrived[i].data.ptr;

            switch (ctx->ev){
            case EV_BTN_1: // increase frequence
            syslog(LOG_INFO, "button 1 pressed\n");
                if(ctx->first_done == 0){
                    ctx->first_done = 1;
                    break;
                }
                g_freq++;
                writeFreq(g_freq);
            break;
            case EV_BTN_2: // decrease frequence
            syslog(LOG_INFO, "button 2 pressed\n");
            if(ctx->first_done == 0){
                    ctx->first_done = 1;
                    break;
                }
                g_freq--;
                writeFreq(g_freq);
            break;
            case EV_BTN_3: // change auto/manual mode
            syslog(LOG_INFO, "button 3 pressed\n");
                if(ctx->first_done == 0){
                    ctx->first_done = 1;
                    break;
                }
                // button_action(ctx->ev, 0);
                g_mode = !g_mode;
                //TODO : write it to the the module
                writeMode(g_mode);
                break;
            
            default:
                syslog(LOG_ERR, "unknow event");
                // printf("error: unknow event\n");
                break;
            }
        }
    }
    // TODO : SEND A SIGNAL TO KILL THE LISTENING THREAD
    kill(g_thread_id, SIGKILL);
    pthread_join(g_thread_id, NULL);
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
    syslog(LOG_INFO, "Daemon has started 7...");

    // // 10. option: get effective user and group id for appropriate's one
    // struct passwd* pwd = getpwnam("daemon");
    // if (pwd == 0) {
    //     syslog(LOG_ERR, "ERROR while reading daemon password file entry");
    //     exit(1);
    // }

    // // 11. option: change root directory
    // if (chroot(".") == -1) {
    //     syslog(LOG_ERR, "ERROR while changing to new root directory");
    //     exit(1);
    // }

    // // 12. option: change effective user and group id for appropriate's one
    // if (setegid(pwd->pw_gid) == -1) {
    //     syslog(LOG_ERR, "ERROR while setting new effective group id");
    //     exit(1);
    // }
    // if (seteuid(pwd->pw_uid) == -1) {
    //     syslog(LOG_ERR, "ERROR while setting new effective user id");
    //     exit(1);
    // }

    // 13. implement daemon body...
    syslog(LOG_INFO, "running daemon");
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