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
#include <linux/wait.h> /* need for waits */
// #include <linux/atomic.h> /* need for atomic finctions */
#include <linux/interrupt.h> /* need for interrupts */
#include <linux/gpio.h> /* need for gpio functions */
#include <linux/fs.h> /* need for file operations inode & file struct */
#include <linux/uaccess.h> /* need for copy_to_user & copy_from_user */
#include <linux/kdev_t.h> /* need for MAJOR & MINOR */
#include <linux/cdev.h> /* need for cdev functions */
#include <linux/poll.h> /* need for poll functions */

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
static uint16_t my_var_k1 = 0;
static uint16_t my_var_k2 = 0;
static uint16_t my_var_k3 = 0;

static dev_t my_dev;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

wait_queue_head_t my_queue;
static atomic_t request_can_be_processed = ATOMIC_INIT(0);

// clearring function
void clear_module(int stage){
    switch (stage){
        case -1:
        case 14:
        case 13:
        case 12:
        case 11:
        case 10:
        case 9:
            device_destroy(my_class, my_dev);
        case 8:
            class_destroy(my_class);
        case 7:
            cdev_del(&my_cdev);
        case 6:
            unregister_chrdev_region(my_dev, 1);
        case 5:
            free_irq(irq_k3, NULL);
        case 4:
            free_irq(irq_k2, NULL);
        case 3:
            free_irq(irq_k1, NULL);
        case 2:
            gpio_free(3);
        case 1:
            gpio_free(2);
        case 0:
            gpio_free(0);
            break;
        default:
            break;
    }
}

// routine d'interruption
irqreturn_t my_irq_handler(int irq, void *dev_id){
    pr_info("Pilotes Ex7: my_irq_handler(%d, %p)\n", irq, dev_id);
    // si traité avec succès, retourner IRQ_HANDLED
    // sinon, retourner IRQ_NONE
    if(irq == irq_k1){
        my_var_k1++;
    }else if(irq == irq_k2){
        my_var_k2++;
    }else if(irq == irq_k3){
        my_var_k3++;
    }
    atomic_inc(&request_can_be_processed);
    wake_up_interruptible(&my_queue);
    return IRQ_HANDLED;
}

static int my_open(struct inode *inode, struct file *file){
    return 0;
}

static int my_release(struct inode *inode, struct file *file){
    return 0;
}
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *off){
    // pour avoit un wait lors de la lecture, sinon fait un poll
    // wait_event_interruptible(my_queue, atomic_read(&request_can_be_processed)>0);
    // atomic_dec(&request_can_be_processed);

    char my_int16_to_char[3 * 2]; // char = 8bits, uint16_t = 16bits
    my_int16_to_char[0 * 2 + 1] = (my_var_k1 >> 8) & 0xFF;
    my_int16_to_char[0 * 2] =my_var_k1 & 0xFF;
    my_int16_to_char[1 * 2 + 1] = (my_var_k2 >> 8) & 0xFF;
    my_int16_to_char[1 * 2] =my_var_k2 & 0xFF;
    my_int16_to_char[2 * 2 + 1] = (my_var_k3 >> 8) & 0xFF;
    my_int16_to_char[2 * 2] =my_var_k3 & 0xFF;
    int data_saved = 3 * 2;
    // cpoie au maximum count octets vers le buffer utilisateur buf
    // met à jour la position off dans le fichier
    // retourne le nombre d'octets copiés
    size_t max_read = data_saved - *off;
    char *read_from = my_int16_to_char + *off;

    pr_info("Pilotes Ex7: my_read called\n");
    if(count > max_read){
        count = max_read;
    }
    if(count < 0){
        return -EFAULT;
    }
    *off += count;
    // copie dans espace utilisateur
    if(copy_to_user(buf, read_from, count)){
        pr_info("Pilotes Ex7: copy_to_user failed\n");
        return -EFAULT;
    }
    return count;

}
static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *off){
    pr_err("Pilotes Ex7: my_write called\n");
    pr_err("Pilotes Ex7: write not supported\n");
    return -EFAULT;
}
static unsigned int my_poll(struct file *file, struct poll_table_struct *wait){
    unsigned int mask = 0;
    poll_wait(file, &my_queue, wait);
    if(atomic_read(&request_can_be_processed) > 0){
        atomic_dec(&request_can_be_processed);
        mask |= POLLIN | POLLRDNORM; // read operation
        // mask |= POLLOUT | POLLWRNORM; // write operation
    }    
    return mask;
}

static struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .poll = my_poll,
};

static int __init my_module_init(void){
    int stage_setup = 0;
    pr_info("Pilotes Ex7: 'hello'\n");
    // vérification de la validité des numéros de port GPIO
    if (!gpio_is_valid(0) || !gpio_is_valid(2) || !gpio_is_valid(3)){
        pr_info("Pilotes Ex7: gpio_is_valid(0) failed\n");
        return -1;
    }

    // acquision de la porte GPIO
    if (gpio_request(0, "k1")){
        pr_info("Pilotes Ex7: gpio_request(0, 'k1') failed\n");
        return -1;
    }
    if (gpio_request(2, "k2")){
        pr_info("Pilotes Ex7: gpio_request(2, 'k2') failed\n");
        clear_module(stage_setup);
        return -1;
    }
    stage_setup++;
    if (gpio_request(3, "k3")){
        pr_info("Pilotes Ex7: gpio_request(3, 'k3') failed\n");
        clear_module(stage_setup);
        return 1;
    }
    stage_setup++;
    // configuration de la porte GPIO
    gpio_direction_input(0);
    gpio_direction_input(2);
    gpio_direction_input(3);

    // obtention du vecteur d'interruption
    irq_k1 = gpio_to_irq(0);
    irq_k2 = gpio_to_irq(2);
    irq_k3 = gpio_to_irq(3);
    pr_info("Pilotes Ex7: irq_k1=%d, irq_k2=%d, irq_k3=%d\n", irq_k1, irq_k2, irq_k3);
    

    // enregistrement de la routinne d'interruption
    if (request_irq(irq_k1, my_irq_handler, IRQF_TRIGGER_RISING, "irq_k1", NULL)){
        pr_info("Pilotes Ex7: request_irq(irq_k1, ...) failed\n");
        clear_module(stage_setup);
        return -1;
    }
    stage_setup++;
    if (request_irq(irq_k2, my_irq_handler, IRQF_TRIGGER_RISING, "irq_k2", NULL)){
        pr_info("Pilotes Ex7: request_irq(irq_k2, ...) failed\n");
        clear_module(stage_setup);
        return -1;
    }
    stage_setup++;
    if (request_irq(irq_k3, my_irq_handler, IRQF_TRIGGER_RISING, "irq_k3", NULL)){
        pr_info(P_C "Pilotes Ex7: request_irq(irq_k3, ...) failed\n");
        clear_module(stage_setup);
        return -1;
    }
    stage_setup++;
    
    //allocation de numéro majeur et mineur
    if (alloc_chrdev_region(&my_dev, 0, 1, "my_btns_poll")){
        pr_info("Pilotes Ex7: alloc_chrdev_region(&my_dev, 0, 1, 'my_btns_poll') failed\n");
        clear_module(stage_setup);
        return -1;
    }
    stage_setup++;

    // init de la structure cdev
    cdev_init(&my_cdev, &my_fops);
    // enregistrement de la structure cdev
    if (cdev_add(&my_cdev, my_dev, 1)){
        pr_info("Pilotes Ex7: cdev_add(&my_cdev, my_dev, 1) failed\n");
        clear_module(stage_setup);
        return -1;
    }
    stage_setup++;

    // creation de class
    my_class = class_create(THIS_MODULE, "my_btns_poll_class");
    if (my_class == NULL){
        pr_info("Pilotes Ex7: class_create(THIS_MODULE, 'my_btns_poll_class') failed\n");
        clear_module(stage_setup);
        return -1;
    }
    stage_setup++;

    // creation de device
    my_device = device_create(my_class, NULL, my_dev, NULL, "my_btns_poll_device");
    if (my_device == NULL){
        pr_info("Pilotes Ex7: device_create(my_class, NULL, my_dev, NULL, 'my_btns_poll_device') failed\n");
        clear_module(stage_setup);
        return -1;
    }  
    stage_setup++;
    init_waitqueue_head(&my_queue);
    
    
    pr_info("Pilotes Ex7: Init done\n");
    return 0;
}

static void __exit my_module_exit(void){
    clear_module(-1);
    pr_info ("Pilotes Ex7: 'bye'\n");
}


module_init (my_module_init);
module_exit (my_module_exit);

MODULE_AUTHOR ("T.Dietrich & K.Goundiaev");
MODULE_DESCRIPTION ("My module test");
MODULE_LICENSE ("GPL");
