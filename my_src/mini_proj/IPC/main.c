#include "command.h"

int main()
{
    // try to pass to manual mode
    init_socket();
    send_mode(1);
    send_freq(100);
    usleep(100000);
    send_freq(2);
    close_socket();
    return 0;
}

