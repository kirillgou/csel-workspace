// my_module.c
/*
Exercice #4: Créez dynamiquement des éléments dans le noyau. 
Adaptez un module noyau, afin que l’on puisse lors de 
son installation spécifier un nombre d’éléments à créer ainsi 
qu’un texte initial à stocker dans les éléments précédemment 
alloués. Chaque élément contiendra également un numéro unique. 
Les éléments seront créés lors de l’installation du module et 
chaînés dans une liste. Ces éléments seront détruits lors 
de la désinstallation du module. Des messages d’information 
seront émis afin de permettre le debugging du module.
*/

#include <linux/module.h>  // needed by all modules
#include <linux/init.h>    // needed for macros
#include <linux/kernel.h>  // needed for debugging

#include <linux/moduleparam.h>  /* needed for module parameters */
#include <linux/slab.h> /* need for using kmalloc */
#include <linux/string.h> /* need for managing strings */
#include <linux/kernel.h> /* need for converting strings to int or others */
#include <linux/list.h> /* need for using lists */

#define MAX_STRING_SIZE 30

static char* text = "Module ex 4";
module_param(text, charp, 0);

static int elements = 1;
module_param(elements, int, S_IRUSR | S_IRGRP);

// definition of a list element with struct list_head as member
struct element {
    int value;
    char text[MAX_STRING_SIZE];
    // some members
    struct list_head list;
};

// definition of the global list
static LIST_HEAD (my_list);

static int __init my_module_init(void){
    pr_info ("My_module say : 'hello'\n");
    pr_info("Text: %s\telement: %d\n", text, elements);
    int i;
    for(i = 0; i < elements; i++){
        // create a new element
        struct element* ele;
        ele = kzalloc(sizeof(*ele), GFP_KERNEL); 
        if (ele != NULL){
            ele->value = i;
            if(strlen(text) < MAX_STRING_SIZE){
                strcpy(ele->text, text);
            }else{
                strcpy(ele->text, "NOP");
            }
            // add element at the end of the list 
            list_add_tail(&ele->list, &my_list); 
        }else{
            printk(KERN_ERR "allocation fails !\n");
        }
    }

    //print the list:
    struct element* np;
    list_for_each_entry(np, &my_list, list){
        printk("%i : %s\n", np->value, np->text);
    }

    return 0;
}

static void __exit my_module_exit(void){
    pr_info ("My_module say : 'bye'\n");
    pr_info("Name: %s\telement: %d\n", text, elements);
    // kfree (buffer);
    // buffer = 0;
}


module_init (my_module_init);
module_exit (my_module_exit);

MODULE_AUTHOR ("T.Dietrich & K.Goundiaev");
MODULE_DESCRIPTION ("My module test");
MODULE_LICENSE ("GPL");
