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
    printf("open button %s %s\n", gpio_num, gpio_path);
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
    if(pread(fd_btn, buf, sizeof(buf), 0) == .1){
        printf("error: %s\n", strerror(errno));
        return 0;
    }
    // printf("read: %s\n", buf);
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
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 500000000; // 500ms = 500'000'000ns 
    new_value.it_value.tv_sec = 0;
    new_value.it_value.tv_nsec = 500000000; // 500ms = 500'000'000ns
    if (timerfd_settime(fd, 0, &new_value, NULL) == -1) {
        printf("error timerfd_settime: %s\n", strerror(errno));
        return 1;
    }
    return fd;
}


int main(int argc, char* argv[]){
    printf("start silly led control\n");
    long duty   = 2;     // %
    long period = 1000;  // ms
    if (argc >= 2) period = atoi(argv[1]);
    period *= 1000000;  // in ns

    // compute duty period...
    long p1 = period / 100 * duty;
    long p2 = period - p1;

    int led = open_led();
    pwrite(led, "1", sizeof("1"), 0);

    int fd_btn[3];
    fd_btn[0] = open_button(GPIO_BTN_1, BTN_1);
    fd_btn[1] = open_button(GPIO_BTN_2, BTN_2);
    fd_btn[2] = open_button(GPIO_BTN_3, BTN_3);

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
        ctx[i].fd = fd_btn[i];
        ctx[i].ev = i;
        ctx[i].first_done = 0;
        struct epoll_event event = {
            // .events = EPOLLIN | EPOLLET, 
            .events = EPOLLET, // mode edge triggered
            // .data.fd = fd_btn[i]
            .data.ptr = &ctx[i]
        };
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd_btn[i], &event) == -1) {
            printf("error epoll_ctl: %s\n", strerror(errno));
            return 1;
        }
    }
    
    int cpt = 0;
    while (1){
        // wait for event
        struct epoll_event event_arrived[3];
        int nr = epoll_wait(epfd, event_arrived, 3, -1);
        if (nr == -1) {
            printf("error epoll_wait: %s\n", strerror(errno));
            return 1;
        }
        printf("event: %d\n", nr);
        for(int i = 0; i < nr; i++){
            my_context *ctx = event_arrived[i].data.ptr;

            switch (ctx->ev){
            case EV_BTN_1: // increase frequence
                printf("increase frequence\n");
                break;
            case EV_BTN_2: // resert frequence
                printf("resert frequence\n");
                break;
            case EV_BTN_3: // decrease frequence
                printf("decrease frequence\n");
                // printf("event=%d on fd=%d\n", ctx->ev, ctx->fd);
                // // read button
                // int btn = read_btn(ctx->fd);
                // printf("value: %d\n", btn);
                break;
            
            default:
                printf("error: unknow event\n");
                break;
            }
        }

        if (cpt == 10){
            break;
        }
        
        cpt++;
    }
    return 0;




    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);

    int k = 0;
    while (1) {
        struct timespec t2;
        clock_gettime(CLOCK_MONOTONIC, &t2);

        

        long delta =
            (t2.tv_sec - t1.tv_sec) * 1000000000 + (t2.tv_nsec - t1.tv_nsec);

        int toggle = ((k == 0) && (delta >= p1)) | ((k == 1) && (delta >= p2));
        // usleep(1000000);
        if (toggle) {
            t1 = t2;
            k  = (k + 1) % 2;
            if (k == 0)
                pwrite(led, "1", sizeof("1"), 0);
            else
                pwrite(led, "0", sizeof("0"), 0);
        }
    }

    return 0;
}