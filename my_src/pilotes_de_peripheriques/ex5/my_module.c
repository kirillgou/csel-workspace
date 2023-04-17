// my_module.c
/*
Exercice 5
Développer un pilote de périphérique orienté caractère permettant 
de valider la fonctionnalité du sysfs. Le pilote offrira quelques attributs 
pouvant être lus et écrites avec les commandes echo et cat. 
Ces attributs seront disponibles sous l’arborescence /sys/class/....

Dans un premier temps, implémentez juste ce qu’il faut pour 
créer une nouvelle classe (par exemple : my_sysfs_class)

Exercice 5.1
Ajoutez maintenant les opérations sur les fichiers définies à l’exercice #3.
Vous pouvez définir une classe comme dans l’exercice précédent, 
ou vous pouvez utiliser un platform_device, ou encore un miscdevice.
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
// #include <linux/interrupt.h> /* need for interrupts */
// #include <linux/gpio.h> /* need for gpio functions */
#include <linux/fs.h> /* need for file operations inode & file struct */
#include <linux/uaccess.h> /* need for copy_to_user & copy_from_user */
#include <linux/kdev_t.h> /* need for MAJOR & MINOR */
#include <linux/cdev.h> /* need for cdev functions */

#define P_COLOR_RST "\033[1;0m"
#define P_COLOR_RED "\033[1;31m"
#define P_COLOR_GREEN "\033[1;32m"
#define P_COLOR_YELLOW "\033[1;33m"
#define P_COLOR_BLUE "\033[1;34m"
#define P_COLOR_MAGENTA "\033[1;35m"
#define P_COLOR_CYAN "\033[1;36m"
#define P_COLOR_WHITE "\033[1;37m"
#define P_C P_COLOR_RST

#define NUMBER_OF_MINORS 1

#define CLASS_NAME "my_per_ex5_class"
#define DEVICE_NAME "my_per_ex5_device"

//variable globale
#define MAX_SIZE 1000
char module_data[MAX_SIZE];
size_t data_saved = 0;
static dev_t my_dev_buff = 0;
static struct cdev my_cdev;
static struct class *my_class = NULL;
struct device *my_device = NULL;

// structure du device attributs
struct my_device_attribute_struct {
    int id;
    int max_value;
    char name[100];
};

static struct my_device_attribute_struct my_device_attribute; 

ssize_t show_device_attribute(struct device *dev, struct device_attribute *attr, char *buf){
    pr_info("Pilotes Ex5: show_driver_attribute called\n");
    sprintf(buf, "%d %d %s\n", my_device_attribute.id, 
                                my_device_attribute.max_value, 
                                my_device_attribute.name);
    // pourquoi strlen(buf)?
    return strlen(buf);
}

ssize_t store_device_attribute(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
    pr_info("Pilotes Ex5: store_driver_attribute called\n");
    sscanf(buf, "%d %d %s", &my_device_attribute.id, 
                            &my_device_attribute.max_value, 
                            my_device_attribute.name);
    return count;
}

// déclaration de l'attribut
// my_drive_attr_name est le nom de l'attribut qui sera utiliser par la suite et vu dans sysfs
// 0664 est le mode d'accès, 666 pas possible 
DEVICE_ATTR(my_device_attr_name, 0664, show_device_attribute, store_device_attribute);

// 1. Implémentation des opérations sur les fichiers (handler) 

// pour l'opération open
static int my_open(struct inode *i, struct file *f){
    pr_info("Pilotes Ex5: my_open called\n");
    // inode: représente de façon unique un fichier dans le noyau Linux
    // file: représente un fichier ouvert par un processus, il peut exister 
    //      plusieurs structures de fichiers attachées au même inode.
    // file contient position actuelle, mode d'ouverture, un pointeur void * private_data
    return 0;
}

// pour l'opération release
static int my_release(struct inode *i, struct file *f){
    pr_info("Pilotes Ex5: my_release called\n");
    return 0;
}

// pour l'opération read
static ssize_t my_read(struct file *f, char __user *buf, size_t count, loff_t *off){
    // cpoie au maximum count octets vers le buffer utilisateur buf
    // met à jour la position off dans le fichier
    // retourne le nombre d'octets copiés
    size_t max_read = data_saved - *off;
    char *read_from = module_data + *off;

    pr_info("Pilotes Ex5: my_read called\n");
    if(count > max_read){
        count = max_read;
    }
    if(count < 0){
        return -EFAULT;
    }
    *off += count;

    // copie dans espace utilisateur
    // copy_to_user (void __user * to, const void * from, unsigned long n)
    if(copy_to_user(buf, read_from, count)){
        pr_info("Pilotes Ex5: copy_to_user failed\n");
        return -EFAULT;
    }
    return count;
}

// pour l'opération write
static ssize_t my_write(struct file *f, const char __user *buf, size_t count, loff_t *off){
    size_t max_write = MAX_SIZE - *off;
    char *write_to = module_data + *off;

    pr_info("Pilotes Ex5: my_write called\n");
    if(count > max_write || count < 0){
        return -EFAULT;
    }
    // copie au maximum count octets depuis le buffer utilisateur buf
    // met à jour la position off dans le fichier
    // retourne le nombre d'octets copiés
    // copie dans espace kernel
    // copy_from_user (void * to, const void __user * from, unsigned long n)
    if(copy_from_user(write_to, buf, count)){
        pr_info("Pilotes Ex5: copy_from_user failed\n");
        return -EFAULT;
    }
    *off += count;
    // mise à jour de la taille des données sauvegardées
    data_saved = *off;
    
    return count;
}

// 2. Définition de la structure struct file_operations
static struct file_operations my_fops = {
    .owner  = THIS_MODULE,
    .open   = my_open,
    .read   = my_read,
    .write  = my_write,
    .release= my_release,
};

static int __init my_module_init(void){
    pr_info("Pilotes Ex5: 'hello'\n");
    module_data[0] = 'H';
    module_data[1] = 'e';
    module_data[2] = 'l';
    module_data[3] = 'l';
    module_data[4] = 'o';
    module_data[5] = '\n';
    module_data[6] = '\r';
    data_saved = 7;


    // 3. Réservation dynamique du numéro de pilote
    // alloc_chrdev_region (dev_t *dev, unsigned baseminor, unsigned count, const char *name)
    if(alloc_chrdev_region(&my_dev_buff, 0, NUMBER_OF_MINORS, "my_dev_buffer")){
        pr_info("Pilotes Ex5: alloc_chrdev_region failed\n");
        return -1;
    }

    // 4. Association des opérations sur le fichier au numéro de pilote
    // cdev_init (struct cdev *cdev, const struct file_operations *fops)
    cdev_init(&my_cdev, &my_fops);
    //owner assigné à THIS_MODULE
    my_cdev.owner = THIS_MODULE;
    // enregisrement du pilote dans le noyau
    // cdev_add (struct cdev *p, dev_t dev, unsigned count)
    if(cdev_add(&my_cdev, my_dev_buff, NUMBER_OF_MINORS)){
        pr_info("Pilotes Ex5: cdev_add failed\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(my_dev_buff, NUMBER_OF_MINORS);
        return -1;
    }
    
    // creation de class
    my_class = class_create(THIS_MODULE, CLASS_NAME);
    if(IS_ERR(my_class)){
        pr_info("Pilotes Ex5: class_create failed\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(my_dev_buff, NUMBER_OF_MINORS);
        return -1;
    }

    // creation de device
    my_device = device_create(my_class, NULL, my_dev_buff, NULL, DEVICE_NAME);
    if(IS_ERR(my_device)){
        pr_info("Pilotes Ex5: device_create failed\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(my_dev_buff, NUMBER_OF_MINORS);
        return -1;
    }

    my_device_attribute.id = 1;
    my_device_attribute.max_value = 100;
    strcpy(my_device_attribute.name, "my_device_attribute");
    // creation de sysfs attribute file
    device_create_file(my_device, &dev_attr_my_device_attr_name);

    pr_info("Pilotes Ex5: Init done\n");
    // Affichage des informations
    pr_info("Pilotes Ex5: Major number: %d Minor number: %d\n", MAJOR(my_dev_buff), MINOR(my_dev_buff));
    return 0;
}

static void __exit my_module_exit(void){
    // Libération des ressources
    // destroy sysfs attribute file
    device_create_file(my_device, &dev_attr_my_device_attr_name);
    // destroy device
    device_destroy(my_class, my_dev_buff);
    // destroy class
    class_destroy(my_class);
    // cdev_del (struct cdev *p)
    cdev_del(&my_cdev);
    // unregister_chrdev_region (dev_t from, unsigned count)
    unregister_chrdev_region(my_dev_buff, NUMBER_OF_MINORS);
    pr_info ("Pilotes Ex5: 'bye'\n");
}


module_init (my_module_init);
module_exit (my_module_exit);

MODULE_AUTHOR ("T.Dietrich & K.Goundiaev");
MODULE_DESCRIPTION ("My module test");
MODULE_LICENSE ("GPL");