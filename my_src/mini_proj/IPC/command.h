#ifndef COMMANDLIB_H
#define COMMANDLIB_H

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
#include <arpa/inet.h>

#define DAEMON_ADDR "127.0.0.1"
#define DAEMON_PORT 8080

void init_socket();
void send_mode(int mode);
void send_freq(int freq);
void close_socket();
#endif
