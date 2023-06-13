#ifndef DAEMONFANLIB_H
#define DAEMONFANLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>

#include "ssd1306.h"

#define DAEMON_PORT 8080

#define SOCKET_BUFFER_SIZE 8

typedef struct _socketParamThread {
    int *mode;
    int *freq;
    int server_fd;
    struct sockaddr_in address;
} socketParamThread;

void generateDaemon(void (*catchSignalFunc)(int));
void initSocket(int *mode, int *freq, pthread_t *thread_id, void* (*threadFunc)(void*));


#endif
