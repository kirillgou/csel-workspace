#include "ssd1306.h"
#include "daemonfanlib.h"
#include "gpio_utility.h"

static void *threadSocket(void *arg);
void catch_signal(int signal);

void catch_signal(int signal)
{
    syslog(LOG_INFO, "signal=%d catched\n", signal);
    // g_signal_catched++;
}

// Example of packet :
// set mode to 1 : "M1"
// set mode to 0 : "M0"
// set Freq to 1Hz : "F1"
// set Freq to 200 : "F200"
static void *threadSocket(void *arg)
{
    int client_fd = 0;
    char buffer[SOCKET_BUFFER_SIZE] = {0};
    // get the parameters
    socketParamThread *param = (socketParamThread*) arg;
    // TODO : ADD A SIGNAL TO STOP THE THREAD
    int addresslen = sizeof(param->address);
    //listen on the socket
    if((client_fd = accept(param->server_fd, (struct sockaddr*)&param->address, ((socklen_t*) &addresslen))) < 0) {
        syslog(LOG_ERR, "accept");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_INFO, "threadSocket started\n");
    while(1) {
        int valread = read(client_fd, buffer, SOCKET_BUFFER_SIZE);
        if (valread == 0) {
            syslog(LOG_INFO, "client disconnected\n");
            close(client_fd);
            client_fd = accept(param->server_fd, (struct sockaddr*)&param->address, ((socklen_t*) &addresslen));
        } else {
            if (buffer[0] == 'M') {
                *param->mode = buffer[1] - '0';
                writeMode(*param->mode);
                if (*param->mode == 0) {
                    writeFreq(*param->freq);
                }
            } else if (buffer[0] == 'F') {
                *param->freq = atoi(&buffer[1]);
                writeFreq(*param->freq);
            }
            syslog(LOG_INFO, "received: %s\n", buffer);
        }
    }
    free(param);
    return NULL;
}

int main()
{
    // go to the daemon
    generateDaemon(catch_signal);
    // now we are in the daemon
    pthread_t thread_id;
    int freq = 0;
    int mode = 0;
    int epfd = 0;
    // start by initialising the ssd1306 display
    initScreen(mode, freq);

    // init the buttons S1, S2 and S3
    epfd = initButtonsAndTimer();

    // init the leds
    initLeds();

    // init a socket for communication with the application
    initSocket(&mode, &freq, &thread_id, threadSocket);

    while (1) {
        struct epoll_event event_arrived[NUM_EVENTS];
        syslog(LOG_INFO, "waiting for event epoll\n");
        int nr = epoll_wait(epfd, event_arrived, NUM_EVENTS, -1);
        syslog(LOG_INFO, "event arrived\n");
        if (nr == -1) {
            // printf("error epoll_wait: %s\n", strerror(errno));
            syslog(LOG_ERR, "epoll_wait");
            exit(EXIT_FAILURE);
        }
        writeLed(LED_OFF);
        for(int i = 0; i < nr; i++){
            my_context *ctx = event_arrived[i].data.ptr;

            switch (ctx->ev){
            case EV_BTN_1: // increase frequence
                syslog(LOG_INFO, "button 1 pressed\n");
                if(ctx->first_done == 0){
                    ctx->first_done = 1;
                    break;
                }
                freq++;
                writeFreq(freq);
                writeLed(LED_ON);
            break;
            case EV_BTN_2: // decrease frequence
            syslog(LOG_INFO, "button 2 pressed\n");
                if(ctx->first_done == 0){
                    ctx->first_done = 1;
                    break;
                }
                if(freq > 0)
                {
                    freq--;
                }
                writeFreq(freq);
                writeLed(LED_ON);
            break;
            case EV_BTN_3: // change auto/manual mode
            syslog(LOG_INFO, "button 3 pressed\n");
                if(ctx->first_done == 0){
                    ctx->first_done = 1;
                    break;
                }
                // button_action(ctx->ev, 0);
                mode = !mode;
                //TODO : write it to the the module
                writeMode(mode);
                if(mode == 0){
                    // need to rewrite the actual freq
                    writeFreq(freq);
                }
                writeLed(LED_ON);
                break;
            case EV_TIMER:
                // read the actual mode
                syslog(LOG_INFO, "timer expired\n");
                mode = readMode(); // maybe and IPC changed it ?
                if(mode == 1) // if in auto mode
                {
                    syslog(LOG_INFO, "auto mode\n");
                    writeLCDMode(mode);
                    // read the actual freq
                    freq = readFreq();
                    // show it on the screen
                    writeLCDFreq(freq);
                }
                else // if in manual mode
                {
                    syslog(LOG_INFO, "manual mode\n");
                    writeLCDMode(mode);
                }
                break;
            
            default:
                syslog(LOG_ERR, "unknow event");
                // printf("error: unknow event\n");
                break;
            }
        }
    }
    // TODO : SEND A SIGNAL TO KILL THE LISTENING THREAD
    kill(thread_id, SIGKILL);
    pthread_join(thread_id, NULL);
    return 0;
}

