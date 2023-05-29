/*
    Exercice 3
    Afin de valider la capacité des groupes de contrôle de limiter l’utilisation des CPU,
    concevez une petite application composée au minimum de 2 processus utilisant le 100% des ressources du processeur.
*/
#define _GNU_SOURCE
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sched.h>
#include <string.h>

void process1(void) {
    int a = 0;
    while (1) {
        // do some random computation
        a = (a + 1) % 1000000;
    }
}

void process2(void) {
    int a = 0;
    while (1) {
        // do some random computation
        a = (a + 1) % 1000000;
    }
}


int main(void)
{
    pid_t pid;
    pid = fork();
    if (pid == 0) // child
    { 
        // // set thread affinity
        // cpu_set_t cpuset;
        // CPU_ZERO(&cpuset);
        // // set this process to run on core 1
        // CPU_SET(1, &cpuset);
        // // here 0 mean use the calling process
        // if(sched_setaffinity(0, sizeof(cpuset), &cpuset) == -1) {
        //     perror("sched_setaffinity");
        //     // exit(1);
        // }
        process1();
        exit(0);
    }
    else // parent
    { 
        // // set thread affinity
        // cpu_set_t cpuset;
        // CPU_ZERO(&cpuset);
        // // set this process to run on core 0
        // CPU_SET(0, &cpuset);
        // // here 0 mean use the calling process
        // if(sched_setaffinity(0, sizeof(cpuset), &cpuset) == -1) {
        //     perror("sched_setaffinity");
        //     // exit(1);
        // }
        process2();
    }
    // must wait for child to exit
    // waitpid(pid, NULL, 0);
    wait(NULL);
    exit(0); /* do everything in the parent and child functions */
    return 0;
}