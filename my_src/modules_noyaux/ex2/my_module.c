// my_module.c
#include <linux/module.h>  // needed by all modules
#include <linux/init.h>    // needed for macros
#include <linux/kernel.h>  // needed for debugging

#include <linux/moduleparam.h>  /* needed for module parameters */

static char* name = "Module ex 2";
module_param(name, charp, 0);

static int elements = 1;
module_param(elements, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

static int __init my_module_init(void)
{
    pr_info ("My_module say : 'hello'\n");
    pr_info("Name: %s\telement: %d\n", name, elements);
    printk(KERN_EMERG "KERN_EMERG\n");
    printk(KERN_ALERT "KERN_ALERT\n");
    printk(KERN_CRIT "KERN_CRIT\n");
    printk(KERN_ERR "KERN_ERR\n");
    printk(KERN_WARNING "KERN_WARNING\n");
    printk(KERN_NOTICE "KERN_NOTICE\n");
    printk(KERN_INFO "KERN_INFO\n");
    printk(KERN_DEBUG "KERN_DEBUG\n");
    printk(KERN_DEFAULT "KERN_DEFAULT\n");
    printk(KERN_CONT "KERN_CONT\n");
    return 0;
}

static void __exit my_module_exit(void)
{
    pr_info ("My_module say : 'bye'\n");
    pr_info("Name: %s\telement: %d\n", name, elements);
}

module_init (my_module_init);
module_exit (my_module_exit);

MODULE_AUTHOR ("T.Dietrich & K.Goundiaev");
MODULE_DESCRIPTION ("My module test");
MODULE_LICENSE ("GPL");
