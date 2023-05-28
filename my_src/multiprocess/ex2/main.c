/*
Exercice #2: Concevez une petite application permettant de valider la capacité des groupes de contrôle à limiter l’utilisation de la mémoire.

Quelques indications pour la création du programme :
Allouer un nombre défini de blocs de mémoire de 2^20 Bytes, par exemple 50
Tester si le pointeur est non nul
Remplir le bloc avec des 0

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

#define NUM_BLOCKS 50
#define MEGABYTE 1024 * 1024
#define BLOCK_SIZE (2 * MEGABYTE)
#define WAIT_TIME_US 1000000

int main(void)
{
    int i;
    char *ptr[NUM_BLOCKS];
    // show the pid of the process
    printf("PID: %d\n", getpid());
    printf("Press any key to allocate memory.\n");
    getchar();
    printf("Allocating memory...\n");
    for (i = 0; i < NUM_BLOCKS; i++)
    {
        printf("Allocating block %d\n", i);
        ptr[i] = malloc(BLOCK_SIZE * sizeof(char));
        if (ptr[i] == NULL){exit(EXIT_FAILURE);}
        usleep(100000);
        // set the memory to 0 using memset
        memset(ptr[i], 0, BLOCK_SIZE);
    }
    printf("Press any key to free memory.\n");
    getchar();
    for (i = 0; i < NUM_BLOCKS; i++)
    {
        free(ptr[i]);
    }
    printf("Memory freed.\n");
    return 0;
}