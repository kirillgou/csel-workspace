// my_module.c
/*
Exercice #5: À l’aide d’un module noyau, 
afficher le Chip-ID du processeur, 
la température du CPU et 
la MAC adresse du contrôleur Ethernet.

Les 4 registres de 32 bits du Chip-ID sont aux adresses 0x01c1'4200 à 0x01c1'420c
Le registre de 32 bits du senseur de température du CPU est à l’adresse 0x01c2'5080
Les 2 registres de 32 bits de la MAC adresse sont aux adresses 0x01c3'0050 et 0x01c3'0054
*/

#include <linux/module.h>  // needed by all modules
#include <linux/init.h>    // needed for macros
#include <linux/kernel.h>  // needed for debugging

#include <linux/moduleparam.h>  /* needed for module parameters */
#include <linux/slab.h> /* need for using kmalloc */
#include <linux/string.h> /* need for managing strings */
#include <linux/kernel.h> /* need for converting strings to int or others */
#include <linux/list.h> /* need for using lists */
#include <linux/ioport.h> /* need for request and release mem_region */
#include <linux/io.h> /* need for ioremap */

// #define MAX_STRING_SIZE 30

static char* text = "Module ex 4";
module_param(text, charp, 0);

static int elements = 1;
module_param(elements, int, S_IRUSR | S_IRGRP);

#define ADDR_MAC_REG 0x01c30050
//TODO comment bien choisir la taille ???
#define ADDR_MAC_LEN 0x2 * 4
// #define ADDR_MAC_REG 0x01c30000
// //TODO comment bien choisir la taille ???
// #define ADDR_MAC_LEN 0x1000
#define ADDR_TEMP_REG 0x01c25080
#define ADDR_TEMP_LEN 1 * 4
#define ADDR_CHIP_ID_REG 0x01c14200
#define ADDR_CHIP_ID_LEN 0x4 * 4


static struct resource *mem_reg;
void __iomem *regs_mac;
void __iomem *regs_temp;
void __iomem *regs_chip_id;


static int __init my_module_init(void){
    pr_info ("My_module say : 'hello'\n");

    // ul strat, ul len, char* name
    mem_reg = request_mem_region (ADDR_MAC_REG, ADDR_MAC_LEN, "Ex5_MAC_ADDR");
    //check error on mem_reg
    if(mem_reg == 0){
        pr_err("Module Ex5: request_mem_region fails");
    }

    // void* ioremap (unsigned long phys_addr, unsigned long size);
    regs_mac = ioremap(ADDR_MAC_REG, ADDR_MAC_LEN);
    if(regs_mac == 0){
        pr_err("Module Ex5: ioremap MAC_REG fails");
    }else{
        int mac = 0;
        int mac1 = 0;
        mac = ioread32(regs_mac);
        // *4 car sur 32 bits
        mac1 = ioread32(regs_mac + (1*4));
        // pr_info("mac-addr=%012x%012x\n",mac1, mac);
        // int64_t m = ioread64(regs_mac);
        pr_info("MAC : %02x:%02x:%02x:%02x:%02x:%02x\n",(mac1 >> 0) & 0xff, 
                (mac1 >> 8) & 0xff, (mac1 >> 16) & 0xff, (mac1 >> 24) & 0xff, 
                (mac >> 0) & 0xff, (mac >> 8) & 0xff);
        // pr_info("mac-addr.=%02x:%02x:%02x:%02x:%02x:%02x\n", (m >> 0) & 0xff, 
        //         (m >> 8) & 0xff, (m >> 16) & 0xff, (m >> 24) & 0xff, 
        //         (m >> 32) & 0xff, (m >> 40) & 0xff);
    }

    regs_temp = ioremap(ADDR_TEMP_REG, ADDR_TEMP_LEN);
    regs_chip_id = ioremap(ADDR_CHIP_ID_REG, ADDR_CHIP_ID_LEN);
    if(regs_temp == 0){
        pr_err("Module Ex5: ioremap TEMP_REG fails");
    }else{
        int temp = ioread32(regs_temp);
        int tempc = -1191 * (int)temp / 10 +223000;
        pr_info("TEMP : %d, TEMPC : %d", temp, tempc);
    }
    if(regs_chip_id == 0){
        pr_err("Module Ex5: ioremap CHIP_ID_REG fails");
    }else{
        int cid_0 = ioread32(regs_chip_id + (0*4));
        int cid_1 = ioread32(regs_chip_id + (1*4));
        int cid_2 = ioread32(regs_chip_id + (2*4));
        int cid_3 = ioread32(regs_chip_id + (3*4));
        pr_info("CHIP_ID : %08x%08x%08x%08x", cid_0, cid_1, cid_2, cid_3);
    }
    pr_info("Text: %s\telement: %d\n", text, elements);
    return 0;
}

static void __exit my_module_exit(void){
    
    if(mem_reg){
        // ul strat, ul len
        release_mem_region (ADDR_MAC_REG,  ADDR_MAC_LEN);
    }
    if (regs_mac){
        iounmap(regs_mac);
    }
    if (regs_temp){
        iounmap(regs_temp);
    }
    if (regs_chip_id){
        iounmap(regs_chip_id);
    }
    pr_info ("My_module say : 'bye'\n");
}


module_init (my_module_init);
module_exit (my_module_exit);

MODULE_AUTHOR ("T.Dietrich & K.Goundiaev");
MODULE_DESCRIPTION ("My module test");
MODULE_LICENSE ("GPL");
