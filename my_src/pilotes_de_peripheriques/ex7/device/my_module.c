// my_module.c
/*
Exercice 7: 
Développer un pilote et une application utilisant les entrées/sorties 
bloquantes pour signaler une interruption matérielle provenant de 
l’un des switches de la carte d’extension du NanoPI. 
L’application utilisera le service select pour compter le nombre d’interruptions.

*/

#include <linux/module.h>  // needed by all modules
#include <linux/init.h>    // needed for macros
#include <linux/kernel.h>  // needed for debugging

#include <linux/moduleparam.h>  /* needed for module parameters */
#include <linux/slab.h> /* need for using kmalloc */
#include <linux/string.h> /* need for managing strings */
#include <linux/kernel.h> /* need for converting strings to int or others */
// #include <linux/kthread.h> /* need for threads */
// #include <linux/delay.h> /* need for ssleep */
// #include <linux/wait.h> /* need for waits */
// #include <linux/atomic.h> /* need for atomic finctions */
#include <linux/interrupt.h> /* need for interrupts */
#include <linux/gpio.h> /* need for gpio functions */

#define P_COLOR_RST "\033[1;0m"
#define P_COLOR_RED "\033[1;31m"
#define P_COLOR_GREEN "\033[1;32m"
#define P_COLOR_YELLOW "\033[1;33m"
#define P_COLOR_BLUE "\033[1;34m"
#define P_COLOR_MAGENTA "\033[1;35m"
#define P_COLOR_CYAN "\033[1;36m"
#define P_COLOR_WHITE "\033[1;37m"

#define P_C P_COLOR_RST

int irq_k1 = 0;
int irq_k2 = 0;
int irq_k3 = 0;

// variable pour la communication avec user
static int my_var = 0;
static dev_t my_dev;
static struct cdev my_cdev;

// routine d'interruption
//     irqreturn_t my_irq_handler (int irq, void *dev_id);
irqreturn_t my_irq_handler(int irq, void *dev_id){
    pr_info("Pilotes Ex5: my_irq_handler(%d, %p)\n", irq, dev_id);
    // si traité avec succès, retourner IRQ_HANDLED
    // sinon, retourner IRQ_NONE
    return IRQ_HANDLED;
}

static int my_open(struct inode *inode, struct file *file){
    return 0;
}

static int my_release(struct inode *inode, struct file *file){
    return 0;
}
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *off){
    char c = '0' + my_var;
    copy_to_user(buf, &c, 1);
    return 1;
}
static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *off){
    return 1;
}
static unsigned int my_poll(struct file *file, struct poll_table_struct *wait){
    unsigned int mask = 0;
    
    return 0;
}

static struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    // .write = my_write,
    .poll = my_poll,
};

static int __init my_module_init(void){
    pr_info("Pilotes Ex5: 'hello'\n");
    // vérification de la validité des numéros de port GPIO
    //     int gpio_is_valid (int number);
    if (!gpio_is_valid(0) || !gpio_is_valid(2) || !gpio_is_valid(3)){
        pr_info("Pilotes Ex5: gpio_is_valid(0) failed\n");
        return -1;
    }

    // acquision de la porte GPIO
    //     int gpio_request (unsigned gpio, const char *label);
    if (gpio_request(0, "k1") != 0){
        pr_info("Pilotes Ex5: gpio_request(0, 'k1') failed\n");
        return -1;
    }
    if (gpio_request(2, "k2") != 0){
        pr_info("Pilotes Ex5: gpio_request(2, 'k2') failed\n");
        gpio_free(0);
        return -1;
    }
    if (gpio_request(3, "k3") != 0){
        pr_info("Pilotes Ex5: gpio_request(3, 'k3') failed\n");
        gpio_free(0);
        gpio_free(2);
        return 1;
    }

    // configuration de la porte GPIO
    //      int gpio_direction_output (unsigned gpio, int value);
    //      int gpio_direction_input (unsigned gpio);
    gpio_direction_input(0);
    gpio_direction_input(2);
    gpio_direction_input(3);

    // obtention du vecteur d'interruption
    //     int gpio_to_irq (unsigned gpio);
    irq_k1 = gpio_to_irq(0);
    irq_k2 = gpio_to_irq(2);
    irq_k3 = gpio_to_irq(3);
    pr_info("Pilotes Ex5: irq_k1=%d, irq_k2=%d, irq_k3=%d\n", irq_k1, irq_k2, irq_k3);
    

    // enregistrement de la routinne d'interruption
    if (request_irq(irq_k1, my_irq_handler, IRQF_TRIGGER_RISING, "irq_k1", NULL) != 0){
        pr_info("Pilotes Ex5: request_irq(irq_k1, ...) failed\n");
        gpio_free(0);
        gpio_free(2);
        gpio_free(3);
        return -1;
    }
    if (request_irq(irq_k2, my_irq_handler, IRQF_TRIGGER_RISING, "irq_k2", NULL) != 0){
        pr_info("Pilotes Ex5: request_irq(irq_k2, ...) failed\n");
        free_irq(irq_k1, NULL);
        gpio_free(0);
        gpio_free(2);
        gpio_free(3);
        return -1;
    }
    if (request_irq(irq_k3, my_irq_handler, IRQF_TRIGGER_RISING, "irq_k3", NULL) != 0){
        pr_info(P_C "Pilotes Ex5: request_irq(irq_k3, ...) failed\n");
        free_irq(irq_k1, NULL);
        free_irq(irq_k2, NULL);
        gpio_free(0);
        gpio_free(2);
        gpio_free(3);
        return -1;
    }

    
    pr_info("Pilotes Ex5: Init done\n");
    return 0;
}

static void __exit my_module_exit(void){

    // effacement de la routinne d'interruption
    //     void free_irq (unsigned int irq, void *dev_id);
    free_irq(irq_k1, NULL);
    free_irq(irq_k2, NULL);
    free_irq(irq_k3, NULL);
    
    // libération de la porte GPIO
    //     void gpio_free (unsigned gpio);
    gpio_free(0);
    gpio_free(2);
    gpio_free(3);

    //libération du vecteur d'interruption
    //     void irq_to_gpio (unsigned irq);
    // pas besion


    pr_info ("Pilotes Ex5: 'bye'\n");
}


module_init (my_module_init);
module_exit (my_module_exit);

MODULE_AUTHOR ("T.Dietrich & K.Goundiaev");
MODULE_DESCRIPTION ("My module test");
MODULE_LICENSE ("GPL");
