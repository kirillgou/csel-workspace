#include "command.h"

int main()
{
    // try to pass to manual mode
    init_socket();
    // set mode to manual
    send_mode(0);
    // set freq to 100Hz
    send_freq(100);
    usleep(1000000); // wait 1s
    // set freq to 2Hz
    send_freq(2);
    // wait 2s
    usleep(2000000);
    // set mode to auto
    send_mode(1);
    close_socket();
    return 0;
}

