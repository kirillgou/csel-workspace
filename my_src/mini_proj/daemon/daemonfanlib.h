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


#define MODULE_FILE_MODE "/sys/class/fan_ctl/fan_ctl/auto_config"
#define MODULE_FILE_SPEED "/sys/class/fan_ctl/fan_ctl/frequency_Hz"
#define MODULE_FILE_TEMP "/sys/class/fan_ctl/fan_ctl/temperature_mC"
#define DAEMON_PORT 8080

#define SOCKET_BUFFER_SIZE 8

#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"

#define GPIO_LED      "/sys/class/gpio/gpio10"
#define LED           "10"

#define GPIO_BTN_1   "/sys/class/gpio/gpio0"
#define BTN_1        "0"
#define GPIO_BTN_2   "/sys/class/gpio/gpio2"
#define BTN_2        "2"
#define GPIO_BTN_3   "/sys/class/gpio/gpio3"
#define BTN_3        "3"

enum my_event {
    EV_BTN_1 = 0,
    EV_BTN_2 = 1,
    EV_BTN_3 = 2
};

typedef struct {
    int fd;
    enum my_event ev;
    int first_done;
} my_context;

void generateDaemon();


#endif
