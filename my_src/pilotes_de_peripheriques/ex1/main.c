/*
Exercice 1: 
Réaliser un pilote orienté mémoire permettant de mapper 
en espace utilisateur les registres du microprocesseur 
en utilisant le fichier virtuel /dev/mem. 
Ce pilote permettra de lire l’identification du microprocesseur 
(Chip-ID aux adresses 0x01c1'4200 à 0x01c1'420c) 
décrit dans l’exercice “Accès aux entrées/sorties” 
du cours sur la programmation de modules noyau.
*/

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <unistd.h>

#define ADDR_CHIP_ID_REG 0x01c14200
#define ADDR_CHIP_ID_LEN 0x4 * 4

int main()
{
    // Ouverture du fichier correspondant au pilote
    int fd = open("/dev/mem", O_RDWR);
    if (fd < 0) {
        printf("Could not open /dev/mem: error=%i \n", fd);
        return -1;
    }

    // Appel de l’opération mmap afin de placer 
    // dans la mémoire virtuelle du processus 
    // les registres du périphérique
    // void* mmap (
    //     void* addr,    // généralement NULL, adresse de départ en mémoire virtuelle
    //     size_t length, // taille de la zone à placer en mémoire virtuelle
    //     int prot,      // droits d'accès à la mémoire: read, write, execute
    //     int flags,     // visibilité de la page pour d'autres processus: shared, private
    //     int fd,        // descripteur du fichier correspondant au pilote
    //     off_t offset); // offset des registres en mémoire
    
    // page size
    size_t page_size = getpagesize();
    // offset
    off_t offset = ADDR_CHIP_ID_REG % page_size;
    // adresse de départ de la page
    off_t page_addr = ADDR_CHIP_ID_REG - offset;
    volatile uint32_t* regs = mmap(
        0, 
        page_size, 
        PROT_READ | PROT_WRITE, 
        MAP_SHARED, 
        fd, 
        page_addr);
    
    if (regs == MAP_FAILED){
        printf("mmap failed, error: %i:%s \n", errno, strerror(errno));
        return -1;
    }
    
    // Affichage des registres
    // uint32_t cid_0 = *(regs + (offset + (0*4)) / sizeof(uint32_t));
    // uint32_t cid_1 = *(regs + (offset + (1*4)) / sizeof(uint32_t));
    // uint32_t cid_2 = *(regs + (offset + (2*4)) / sizeof(uint32_t));
    // uint32_t cid_3 = *(regs + (offset + (3*4)) / sizeof(uint32_t));
    uint32_t cid_0 = *(regs + offset / sizeof(uint32_t) + (0));
    uint32_t cid_1 = *(regs + offset / sizeof(uint32_t) + (1));
    uint32_t cid_2 = *(regs + offset / sizeof(uint32_t) + (2));
    uint32_t cid_3 = *(regs + offset / sizeof(uint32_t) + (3));
    printf("CHIP_ID : %08x%08x%08x%08x\n", cid_0, cid_1, cid_2, cid_3);

    // while (1)
    // {
    //     sleep(20);
    //     printf("sleep done\n");
    // }

    // Après utilisation, appel de l’opération munmap pour libérer l’espace mémoire
    munmap((void*)regs, page_size);
    // Fermeture du fichier
    close(fd);

    return 0;
}
