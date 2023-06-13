/*
Fichier : lib.c
Auteur : Tanguy Dietrich
Description :
*/
#include "gpio_utility.h"
#include <stdio.h>
#include <stdlib.h>

static my_context g_ctx[NUM_EVENTS];
static int g_led_fd = 0;
pthread_mutex_t g_mutex_lcd;

void writeMode(int mode)
{
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
    writeLCDMode(mode);
}

void writeLCDMode(int mode)
{
    char str[32] = {0};
    sprintf(str, "Mode: %d", mode);
    pthread_mutex_lock(&g_mutex_lcd);
    ssd1306_set_position (0,6);
    ssd1306_puts(str);
    pthread_mutex_unlock(&g_mutex_lcd);
}

void writeLCDFreq(int freq)
{
    char str[32] = {0};
    sprintf(str, "Freq: %03d Hz", freq);
    pthread_mutex_lock(&g_mutex_lcd);
    ssd1306_set_position (0 ,4);
    ssd1306_puts(str);
    pthread_mutex_unlock(&g_mutex_lcd);
}

int readMode()
{
    int fd = open(MODULE_FILE_MODE, O_RDONLY);
    if (fd < 0) {
        syslog(LOG_ERR, "open %s failed\n", MODULE_FILE_MODE);
        exit(EXIT_FAILURE);
    }
    char buffer[2] = {0};
    read(fd, buffer, 2);
    close(fd);
    return atoi(buffer);
}

float readTempCPU()
{
    // data are in format 37204 -> 37.204°C
    int fd = open(MODULE_FILE_TEMP, O_RDONLY);
    if (fd < 0) {
        syslog(LOG_ERR, "open %s failed\n", MODULE_FILE_TEMP);
        exit(EXIT_FAILURE);
    }
    char buffer[5] = {0};
    read(fd, buffer, 5);
    close(fd);
    return atof(buffer) / 1000;
}

void updateTempCPU()
{
    float temp = readTempCPU();
    char str[32] = {0};
    sprintf(str, "Temp: %2.1f'C", temp);
    pthread_mutex_lock(&g_mutex_lcd);
    ssd1306_set_position (0,3);
    ssd1306_puts(str);
    pthread_mutex_unlock(&g_mutex_lcd);
}

int readFreq()
{
    int fd = open(MODULE_FILE_SPEED, O_RDONLY);
    if (fd < 0) {
        syslog(LOG_ERR, "open %s failed\n", MODULE_FILE_SPEED);
        exit(EXIT_FAILURE);
    }
    char buffer[4] = {0};
    read(fd, buffer, 4);
    close(fd);
    return atoi(buffer);
}

void writeFreq(int freq)
{
    int fd = open(MODULE_FILE_SPEED, O_WRONLY);
    int ret = 0;
    if (fd < 0) {
        syslog(LOG_ERR, "open %s failed\n", MODULE_FILE_SPEED);
        exit(EXIT_FAILURE);
    }
    // write mode in the file
    char buffer[2] = {0};
    sprintf(buffer, "%d", freq);
    ret = write(fd, buffer, strlen(buffer));
    close(fd);
    if (ret >= 0) {
        writeLCDFreq(freq);
    }
    
    // return ret;
}

int open_button(const char *gpio_path, const char *gpio_num)
{
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

int open_timer()
{ // see https://man7.org/linux/man-pages/man2/timerfd_create.2.html
    int fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (fd == -1) {
        syslog(LOG_ERR, "error timerfd_create: %s\n", strerror(errno));
        return 1;
    }
    // default 2Hz => 500ms
    struct itimerspec new_value;
    // interval for periodic timer
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = DEFAULT_PERIOD; // 500ms = 500'000'000ns 

    // initial expiration (timer before first expiration)
    new_value.it_value.tv_sec = 10;
    // new_value.it_value.tv_sec = 1;
    new_value.it_value.tv_nsec = 0; // 500ms = 500'000'000ns
    // new_value.it_value.tv_nsec = DEFAULT_PERIOD; // 500ms = 500'000'000ns
    if (timerfd_settime(fd, 0, &new_value, NULL) == -1) {
        syslog(LOG_ERR, "error timerfd_settime: %s\n", strerror(errno));
        return 1;
    }
    syslog(LOG_INFO, "frequence: %.5fHz", S_IN_NSEC / (double)DEFAULT_PERIOD);
    return fd;
}

int initButtonsAndTimer()
{
    g_ctx[EV_BTN_1].ev = EV_BTN_1;
    g_ctx[EV_BTN_2].ev = EV_BTN_2;
    g_ctx[EV_BTN_3].ev = EV_BTN_3;
    g_ctx[EV_BTN_1].fd = open_button(GPIO_BTN_1, BTN_1);
    g_ctx[EV_BTN_2].fd = open_button(GPIO_BTN_2, BTN_2);
    g_ctx[EV_BTN_3].fd = open_button(GPIO_BTN_3, BTN_3);

    g_ctx[EV_TIMER].ev = EV_TIMER;
    g_ctx[EV_TIMER].fd = open_timer();

    int epfd = epoll_create1(0);// parametre size isn't used anymore by Linux
    // creation contexte epoll
    if (epfd == -1) {
        syslog(LOG_ERR, "epoll_create1");
        exit(EXIT_FAILURE);
    }

    for(uint32_t i = 0; i < 3; i++){
        g_ctx[i].first_done = 0;
        struct epoll_event event = {
            // .events = EPOLLIN | EPOLLET, 
            .events = EPOLLET, // mode edge triggered
            .data.ptr = &g_ctx[i]
        };
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, g_ctx[i].fd, &event) == -1) {
            syslog(LOG_ERR, "error epoll_ctl: %s\n", strerror(errno));
            // return 1;
            exit(EXIT_FAILURE);
        }
    }

    struct epoll_event event_timer = {
        .events = EPOLLIN, // read available
        .data.ptr = &g_ctx[EV_TIMER]
    };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, g_ctx[EV_TIMER].fd, &event_timer) == -1) {
        syslog(LOG_ERR, "error epoll_ctl_timer: %s\n", strerror(errno));
        return 1;
    }
    
    syslog(LOG_INFO, "buttons initialized\n");
    return epfd;
}

void initLeds()
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

void writeLed(int value)
{
    char buffer[2] = {0};
    sprintf(buffer, "%d", value);
    write(g_led_fd, buffer, strlen(buffer));
}


void initScreen(int mode, int freq)
{
    syslog(LOG_INFO, "initializing screen\n");
    ssd1306_init();

    // init the mutex
    syslog(LOG_INFO, "initializing mutex\n");
    pthread_mutex_init(&g_mutex_lcd, NULL);

    syslog(LOG_INFO, "getting mutex screen\n");
    pthread_mutex_lock(&g_mutex_lcd);
    syslog(LOG_INFO, "writing on screen\n");
    ssd1306_set_position (0,0);
    ssd1306_puts("CSEL1a - SP.07");
    ssd1306_set_position (0,1);
    ssd1306_puts("  Demo - SW");
    ssd1306_set_position (0,2);
    ssd1306_puts("--------------");

    ssd1306_set_position (0,3);
    ssd1306_puts("Temp: 35'C");
    writeFreq(freq);
    ssd1306_set_position (0,5);
    ssd1306_puts("Duty: 50%");
    pthread_mutex_unlock(&g_mutex_lcd);
    writeMode(mode);
    syslog(LOG_INFO, "screen initialized\n");
}

