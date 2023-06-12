/*
Fichier : lib.c
Auteur : Tanguy Dietrich / Kirill Goundiaev
Description : send command with a socket
Ex : send_mode(1) -> send command to change mode to 1 : "M1" / "M0"
     send_freq(4) -> send command to change frequency to 4 : "F4" / "F0" / "F100"
*/

#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE

#include "command.h"

static struct sockaddr_in g_addr;
static int g_sock;

void init_socket()
{
    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    g_addr.sin_family = AF_INET;
    g_addr.sin_port = htons(DAEMON_PORT);
    inet_aton(DAEMON_ADDR, &g_addr.sin_addr);

    if (connect(g_sock, (struct sockaddr *)&g_addr, sizeof(g_addr)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

}

void send_mode(int mode)
{
    char buf[3];
    sprintf(buf, "M%d", mode);
    send(g_sock, buf, 3, 0);
}

void send_freq(int freq)
{
    char buf[5]; // F100\0
    sprintf(buf, "F%d", freq);
    send(g_sock, buf, 4, 0);
}

void close_socket()
{
    close(g_sock);
}
