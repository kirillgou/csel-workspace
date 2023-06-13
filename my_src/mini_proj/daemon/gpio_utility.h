#ifndef GPIO_UTILITY_H
#define GPIO_UTILITY_H

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
#include <sys/timerfd.h> // timerfd_create, timerfd_settime
#include <pthread.h>


#include "ssd1306.h"
#include "daemonfanlib.h"

#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"

#define GPIO_LED_VERTE      "/sys/class/gpio/gpio10"
#define LED_VERTE           "10"

#define GPIO_LED_ROUGE      "/sys/class/gpio/gpio362"
#define LED_ROUGE           "362"

#define LED_ON  1
#define LED_OFF 0

// LED USED ON THE PROGRAM
#define GPIO_LED GPIO_LED_ROUGE
#define LED LED_ROUGE

#define GPIO_BTN_1   "/sys/class/gpio/gpio0"
#define BTN_1        "0"
#define GPIO_BTN_2   "/sys/class/gpio/gpio2"
#define BTN_2        "2"
#define GPIO_BTN_3   "/sys/class/gpio/gpio3"
#define BTN_3        "3"

#define S_IN_NSEC 1000000000 // 1s = 1'000'000'000ns
#define DEFAULT_PERIOD 200000000 // 200ms = 200'000'000ns

#define NUM_EVENTS 4

enum my_event {
    EV_BTN_1 = 0,
    EV_BTN_2 = 1,
    EV_BTN_3 = 2,
    EV_TIMER = 3
};

typedef struct {
    int fd;
    enum my_event ev;
    int first_done;
} my_context;

int initButtonsAndTimer();
int open_timer();
void initLeds();
void writeLed(int value);
void initScreen(int mode, int freq);
void writeMode(int mode);
int readMode();
void writeFreq(int freq);
int readFreq();
float readTempCPU();
void updateTempCPU();
void writeLCDMode(int mode);
void writeLCDFreq(int freq);
int open_button(const char *gpio_path, const char *gpio_num);

#endif
