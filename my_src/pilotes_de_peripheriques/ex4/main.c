/*
Exercice 4: 
Développer une petite application en espace utilisateur 
permettant d’accéder à ces pilotes orientés caractère (fait dans ex3). 
L’application devra écrire un texte dans le pilote et le relire.
*/

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEVICE_NAME_0 "/dev/my_mod_ex3_0"
#define DEVICE_NAME_1 "/dev/my_mod_ex3_1"
#define DEVICE_NAME_2 "/dev/my_mod_ex3_2"
#define DEVICE_NAME_3 "/dev/my_mod_ex3_3"
#define DEVICE_NAME_4 "/dev/my_mod_ex3_4"

// #define ADDR_CHIP_ID_REG 0x01c14200
// #define ADDR_CHIP_ID_LEN 0x4 * 4

int main()
{
    // Ouverture du fichier correspondant au pilote
    int fd = open(DEVICE_NAME_0, O_WRONLY);
    if (fd < 0) {
        printf("Could not open /dev/mem: error=%i \n", fd);
        return -1;
    }
    // écriture dans le pilote
    static char* str = "Hello World!\n";
    write(fd, str, strlen(str));
    printf("écriture faite\n");
    close(fd);
    fd = open(DEVICE_NAME_0, O_RDONLY);
    // lecture dans le pilote
    char* str_rd = malloc(100);
    read(fd, str_rd, 100);
    printf("lecture faite: %s\n", str_rd);
    close(fd);
    fd = open(DEVICE_NAME_0, O_WRONLY);
    // nouvelle écriture dans le pilote
    char* str3 = "Hi!\n";
    write(fd, str3, strlen(str3));
    printf("nouvelle écriture faite\n");
    close(fd);
    fd = open(DEVICE_NAME_0, O_RDWR);
    // nouvelle lecture dans le pilote
    read(fd, str_rd, 100);
    printf("nouvelle lecture faite: %s\n", str_rd);
    // fermeture du fichier
    close(fd);
    printf("programme terminé\n");

    return 0;
}

/*
   FILE* fd;
    // Ouverture du fichier correspondant au pilote
    fd = fopen(DEVICE_NAME_0, "rw");
    if (fd == 0) {
        printf("Could not open /dev/...\n");
        return -1;
    }
    // écriture dans le pilote
    char* str = "Hello World!";
    fprintf(fd, str);
    printf("écriture faite\n");
    // lecture dans le pilote
    char* str_rd = malloc(100);
    fgets(str_rd, 100, fd);
    printf("lecture faite: %s\n", str_rd);
    // nouvelle écriture dans le pilote
    char* str3 = "Hi!";
    fprintf(fd, str3);
    printf("nouvelle écriture faite\n");
    // nouvelle lecture dans le pilote
    fgets(str_rd, 100, fd);
    printf("nouvelle lecture faite: %s\n", str_rd);
    // fermeture du fichier
    fclose(fd);
    printf("programme terminé\n");
*/