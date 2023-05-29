/*
Exercice #1: Concevez et développez une petite application mettant en œuvre un des services de communication proposés par Linux (par exemple socketpair) entre un processus parent et un processus enfant.
Le processus enfant devra émettre quelques messages sous forme de texte vers le processus parent, lequel les affichera sur la console. 
Le message exit permettra de terminer l’application.
Cette application devra impérativement capturer les signaux SIGHUP, SIGINT, SIGQUIT, SIGABRT et SIGTERM et les ignorer. 
Seul un message d’information sera affiché sur la console. Chacun des processus devra utiliser son propre cœur, par exemple core 0 pour le parent, et core 1 pour l’enfant.

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

int setAffinity(int core);

#define PARENTSOCKET 0
#define CHILDSOCKET 1

#define EXIT_MESSAGE "exit"

// List of message to send :
#define NUM_MESSAGE 5
char *messages[NUM_MESSAGE] = {
    "I'll be back",
    "You talkin' to me?",
    "Hello. My name is Inigo Montoya. You killed my father. Prepare to die.",
    "There is no spoon.",
    EXIT_MESSAGE,
};

void child(int socket) {
    // get the child socket
    int cpt = 0;
    int messageLength;
    bool exitProcess = false;
    while (!exitProcess) {
        messageLength = strlen(messages[cpt]) + 1;
        write(socket, messages[cpt], messageLength);
        if (strcmp(messages[cpt], EXIT_MESSAGE) == 0) {
            exitProcess = true;
        }
        // juste to give the time to the parent to read the message
        // because the parent could read two message at once and not see the exit message
        usleep(1000000); 
        cpt = (cpt + 1) % NUM_MESSAGE;
    }
    printf("Child exit\r\n");
}

void parent(int socket) {
    // get the parent socket
    char buffer[512];
    bool exitProcess = false;
    while (!exitProcess) {
        if(read(socket, buffer, sizeof(buffer)) <= 0) {
            perror("read");
            exit(1);
        }
        printf("Parent received: %s\r\n", buffer);
        if (strcmp(buffer, EXIT_MESSAGE) == 0) {
            exitProcess = true;
        }
    }
    printf("Parent exit\r\n");
}


int setAffinity(int core) {
    // set thread affinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    // set this process to run on core 0
    CPU_SET(core, &cpuset);
    // here 0 mean use the calling process
    if(sched_setaffinity(0, sizeof(cpuset), &cpuset) == -1) {

        return -1;
    }
    return 0;
}

int main(void)
{
    int fd[2];
    pid_t pid;

    if(socketpair(PF_LOCAL, SOCK_STREAM, 0, fd) < 0) {
        perror("socketpair");
        exit(1);
    }

    // equivalent using sigaction
    struct sigaction act;
    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGABRT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    pid = fork();
    if (pid == 0) { // child 
        close(fd[PARENTSOCKET]); 
        // set thread affinity
        if(setAffinity(1) == -1) { perror("sched_setaffinity");}
        child(fd[CHILDSOCKET]);
        close(fd[CHILDSOCKET]);
        exit(0);
    }
    else { // parent 
        close(fd[CHILDSOCKET]);
        // set thread affinity
        if(setAffinity(0) == -1) { perror("sched_setaffinity");}
        parent(fd[PARENTSOCKET]);
        close(fd[PARENTSOCKET]);
    }
    // must wait for child to exit
    // waitpid(pid, NULL, 0);
    wait(NULL);
    exit(0); /* do everything in the parent and child functions */

    return 0;
}