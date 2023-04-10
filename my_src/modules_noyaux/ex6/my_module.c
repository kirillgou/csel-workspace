// my_module.c
/*
Exercice #6: Développez un petit module permettant d’instancier 
un thread dans le noyau. Ce thread affichera un message toutes 
les 5 secondes. Il pourra être mis en sommeil durant ces 5 secondes 
à l’aide de la fonction ssleep(5) provenant de l’interface 
<linux/delay.h>.
*/

#include <linux/module.h>  // needed by all modules
#include <linux/init.h>    // needed for macros
#include <linux/kernel.h>  // needed for debugging

#include <linux/moduleparam.h>  /* needed for module parameters */
#include <linux/slab.h> /* need for using kmalloc */
#include <linux/string.h> /* need for managing strings */
#include <linux/kernel.h> /* need for converting strings to int or others */
#include <linux/kthread.h> /* need for threads */
#include <linux/delay.h> /* need for ssleep */

// thread function
int th_count_5s(void* data){
    pr_info("Module Ex6_th: Start Thread");
    int i  = 0;
    while(!kthread_should_stop()){
        pr_info("Module Ex6_th: Count %d", i++);
        ssleep(5);
    }
    pr_info("Module Ex6: End Thread");
    return 0;
}

struct task_struct *th;

static int __init my_module_init(void){
    pr_info ("Module Ex6: 'hello'\n");

    th = kthread_run(&th_count_5s, NULL, "ex6_th_name");
    if(!th){
        pr_err("Module Ex6: ERR on kthread_run");
    }
    pr_info("Module Ex6: Init done");
    return 0;
}

static void __exit my_module_exit(void){
    if(kthread_stop(th)){
        pr_err("Module Ex6: ERR on kthread_stop");
    }
    pr_info ("My_module say : 'bye'\n");
}


module_init (my_module_init);
module_exit (my_module_exit);

MODULE_AUTHOR ("T.Dietrich & K.Goundiaev");
MODULE_DESCRIPTION ("My module test");
MODULE_LICENSE ("GPL");
