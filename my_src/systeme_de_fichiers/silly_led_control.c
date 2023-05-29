/**
 * Copyright 2018 University of Applied Sciences Western Switzerland / Fribourg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Project: HEIA-FR / HES-SO MSE - MA-CSEL1 Laboratory
 *
 * Abstract: System programming -  file system
 *
 * Purpose: NanoPi silly status led control system
 *
 * Autĥor:  Daniel Gachet
 * Date:    07.11.2018
 */
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h> // epoll_create1, epoll_ctl, epoll_wait
#include <sys/timerfd.h> // timerfd_create, timerfd_settime
#include <syslog.h> // syslog see https://www.gnu.org/software/libc/manual/html_node/Syslog-Example.html

/*
 * status led - gpioa.10 --> gpio10
 * power led  - gpiol.10 --> gpio362
 * 
 * button k1 - gpio0
 * button k2 - gpio2
 * button k3 - gpio3
 */
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

#define S_IN_NSEC 1000000000
#define DEFAULT_PERIOD 500000000 // 500ms = 500'000'000ns

enum my_event {
    EV_BTN_1 = 0,
    EV_BTN_2 = 1,
    EV_BTN_3 = 2,
    EV_TIMER = 3,
    FD_LED   = 4
};

typedef struct {
    int fd;
    enum my_event ev;
    int first_done;
} my_context;

my_context ctx[5];


static int open_led(){
    // unexport pin out of sysfs (reinitialization)
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

    // open gpio value attribute
    f = open(GPIO_LED "/value", O_RDWR);
    return f;
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

int read_btn(int fd_btn){
    char buf[1];
    if(pread(fd_btn, buf, sizeof(buf), 0) == -1){
        printf("error: %s\n", strerror(errno));
        return 0;
    }
    return atoi(buf)==0 ? 0 : 1;
}

int open_timer(){ // see https://man7.org/linux/man-pages/man2/timerfd_create.2.html
    int fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (fd == -1) {
        printf("error timerfd_create: %s\n", strerror(errno));
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
        printf("error timerfd_settime: %s\n", strerror(errno));
        return 1;
    }
    return fd;
}

void change_led(){
    static int cpt = DEFAULT_PERIOD;
    cpt = (cpt + 1) % 2;
        pwrite(ctx[FD_LED].fd, cpt ? "1" : "0", sizeof("0"), 0);
}

void button_action(enum my_event ev, int wait_for_first_event){
    static uint64_t period = 0;
    struct itimerspec current_value;
    struct itimerspec new_value;
    uint64_t last_period;

    switch (ev){
    case EV_BTN_1: // increase frequence
        period /= 2;
        if(period == 0){
            period = 1;
            // printf("error: period == 0\n");
        }
        break;
    case EV_BTN_2: // resert frequence
        period = DEFAULT_PERIOD;
        break;
    case EV_BTN_3: // decrease frequence
        last_period = period;
        period *= 2;
        if(period < last_period){
            period = last_period;
            // printf("error: period overflow\n");
        }
        break;
    default:
        printf("error: unused event\n");
        break;
    }
    

    new_value.it_interval.tv_sec = period / S_IN_NSEC;
    new_value.it_interval.tv_nsec = period % S_IN_NSEC;

    if(wait_for_first_event){
        // time until next expiration
        if(timerfd_gettime(ctx[EV_TIMER].fd, &current_value) == -1){
            printf("error timerfd_gettime: %s\n", strerror(errno));
            return;
        }
        new_value.it_value.tv_sec = current_value.it_value.tv_sec;
        new_value.it_value.tv_nsec = current_value.it_value.tv_nsec+1;// +1 to force to start timer, can be 0
        // printf("old sec: %ld\n", current_value.it_value.tv_sec);
        // printf("old nsec: %ld\n", current_value.it_value.tv_nsec);
    }else{// direct reset timer
        new_value.it_value.tv_sec = 0;
        new_value.it_value.tv_nsec = 1;// +1 to force to start timer
    }
    if (timerfd_settime(ctx[EV_TIMER].fd, 0, &new_value, NULL) == -1) {
        printf("error timerfd_settime: %s\n", strerror(errno));
        return;
    }
}

int main(){

    ctx[FD_LED].ev = FD_LED;
    ctx[FD_LED].fd = open_led();

    ctx[EV_BTN_1].ev = EV_BTN_1;
    ctx[EV_BTN_2].ev = EV_BTN_2;
    ctx[EV_BTN_3].ev = EV_BTN_3;
    ctx[EV_BTN_1].fd = open_button(GPIO_BTN_1, BTN_1);
    ctx[EV_BTN_2].fd = open_button(GPIO_BTN_2, BTN_2);
    ctx[EV_BTN_3].fd = open_button(GPIO_BTN_3, BTN_3);

    ctx[EV_TIMER].ev = EV_TIMER;
    ctx[EV_TIMER].fd = open_timer();

    // creation contexte epoll
    int epfd = epoll_create1(0);// parametre size isn't used anymore by Linux
    if (epfd == -1) {
        printf("error epoll_create1: %s\n", strerror(errno));
        return 1;
    }

    // add contexte controller to epoll
    // inspire from https://github.com/eklitzke/epollet
    // all events https://man7.org/linux/man-pages/man2/epoll_ctl.2.html
    // EPOLLERR une condition d’erreur a été levée
    // EPOLLIN un fichier virtuel est disponible lecture
    // EPOLLOUT un fchier virtuel est disponible en écriture
    // EPOLLPRI des données prioritaires out-of-band sont diponibles
    for(uint32_t i = 0; i < 3; i++){
        ctx[i].first_done = 0;
        struct epoll_event event = {
            // .events = EPOLLIN | EPOLLET, 
            .events = EPOLLET, // mode edge triggered
            .data.ptr = &ctx[i]
        };
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, ctx[i].fd, &event) == -1) {
            printf("error epoll_ctl: %s\n", strerror(errno));
            return 1;
        }
    }
    
    // add contexte timer to epoll
    struct epoll_event event_timer = {
        .events = EPOLLIN, // read available
        .data.ptr = &ctx[EV_TIMER]
    };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, ctx[EV_TIMER].fd, &event_timer) == -1) {
        printf("error epoll_ctl_timer: %s\n", strerror(errno));
        return 1;
    }

    uint64_t time;
    while (1){
        // wait for event
        struct epoll_event event_arrived[5];
        int nr = epoll_wait(epfd, event_arrived, 5, -1);
        if (nr == -1) {
            printf("error epoll_wait: %s\n", strerror(errno));
            return 1;
        }
        for(int i = 0; i < nr; i++){
            my_context *ctx = event_arrived[i].data.ptr;

            switch (ctx->ev){
            case EV_BTN_1: // increase frequence
            case EV_BTN_2: // resert frequence
            case EV_BTN_3: // decrease frequence
                if(ctx->first_done == 0){
                    ctx->first_done = 1;
                    break;
                }
                button_action(ctx->ev, 0);
                break;
            
            case EV_TIMER:
                read(ctx->fd, &time, sizeof(time));
                change_led();
                break;
            
            default:
                printf("error: unknow event\n");
                break;
            }
        }
    }
    return 0;
}