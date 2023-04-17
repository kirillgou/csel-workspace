/*
Exercice 7
*/

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/poll.h>

#define DEVICE_NAME_0 "/dev/my_btns_poll_device"
#define DEVICE_NAME_1 "/dev/my_mod_ex3_1"
#define DEVICE_NAME_2 "/dev/my_mod_ex3_2"
#define DEVICE_NAME_3 "/dev/my_mod_ex3_3"
#define DEVICE_NAME_4 "/dev/my_mod_ex3_4"


int main(){
    // Ouverture du fichier correspondant au pilote
    int fd = open(DEVICE_NAME_0, O_RDONLY);
    if (fd < 0) {
        printf("Could not open /dev/mem: error=%i \n", fd);
        return -1;
    }
    struct pollfd *fds;
    fds = malloc(sizeof(struct pollfd));
    fds->fd = fd;
    fds->events = POLLIN;
    fds->revents = 0;

    printf("polling...\n");
    poll(fds, 1, -1);
    printf("polling done\n");
    
    char* str_rd = malloc(3 * 2);
    read(fd, str_rd, 3 * 2);
    printf("lecture faite: %s\n", str_rd);
    //change char to uint16_t
    uint16_t* data = (uint16_t*)str_rd;
    printf("data[0]: %d\n", data[0]);
    printf("data[1]: %d\n", data[1]);
    printf("data[2]: %d\n", data[2]);

    close(fd);
    return 0;

}