/*
Fichier : lib.c
Auteur : Tanguy Dietrich
Description :
*/
#include "gpio_utility.h"
#include <stdio.h>
#include <stdlib.h>

static my_context g_ctx[3];
static int g_led_fd = 0;

int open_button(const char *gpio_path, const char *gpio_num)
{
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

int initButtons()
{
    g_ctx[EV_BTN_1].ev = EV_BTN_1;
    g_ctx[EV_BTN_2].ev = EV_BTN_2;
    g_ctx[EV_BTN_3].ev = EV_BTN_3;
    g_ctx[EV_BTN_1].fd = open_button(GPIO_BTN_1, BTN_1);
    g_ctx[EV_BTN_2].fd = open_button(GPIO_BTN_2, BTN_2);
    g_ctx[EV_BTN_3].fd = open_button(GPIO_BTN_3, BTN_3);
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
    ssd1306_init();

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
    writeMode(mode);
    syslog(LOG_INFO, "screen initialized\n");
}

void writeMode(int mode)
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

void writeFreq(int freq)
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