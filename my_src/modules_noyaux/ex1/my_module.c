// my_module.c
#include <linux/module.h>  // needed by all modules
#include <linux/init.h>    // needed for macros
#include <linux/kernel.h>  // needed for debugging

static int __init my_module_init(void)
{
    pr_info ("My_module say : 'hello'\n");
    return 0;
}

static void __exit my_module_exit(void)
{
    pr_info ("My_module say : 'bye'\n");
}

module_init (my_module_init);
module_exit (my_module_exit);

MODULE_AUTHOR ("T.Dietrich & K.Goundiaev");
MODULE_DESCRIPTION ("My module test");
MODULE_LICENSE ("GPL");
