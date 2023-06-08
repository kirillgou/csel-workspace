#ifndef DAEMONFANLIB_H
#define DAEMONFANLIB_H

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

#define DAEMON_PORT 8080

static void fork_process();
static void catch_signal(int signal);


#endif
