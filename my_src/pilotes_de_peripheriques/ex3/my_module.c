// my_module.c
/*
Exercice 3
Etendre la fonctionnalité du pilote de l’exercice précédent afin 
que l’on puisse à l’aide d’un paramètre module spécifier le nombre 
d’instances. Pour chaque instance, on créera une variable 
unique permettant de stocker les données échangées avec l’application 
en espace utilisateur.
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

static int  instances = 1;
module_param(instances, int, 0);

//variable globale
#define MAX_SIZE 1000
// change to dynamic allocation
// char module_data[MAX_SIZE];
static char **module_data_tab;
// size_t data_saved = 0;
static size_t *data_saved_tab;

static dev_t my_dev_buff = 0;
static struct cdev my_cdev;

// 1. Implémentation des opérations sur les fichiers (handler) 

// pour l'opération open
static int my_open(struct inode *i, struct file *f){
    pr_info("Pilotes Ex3: my_open called\n");
    // inode: représente de façon unique un fichier dans le noyau Linux
    // file: représente un fichier ouvert par un processus, il peut exister 
    //      plusieurs structures de fichiers attachées au même inode.
    // file contient position actuelle, mode d'ouverture, un pointeur void * private_data
    return 0;
}

// pour l'opération release
static int my_release(struct inode *i, struct file *f){
    pr_info("Pilotes Ex3: my_release called\n");
    return 0;
}

// pour l'opération read
static ssize_t my_read(struct file *f, char __user *buf, size_t count, loff_t *off){
    unsigned minor = iminor(f->f_inode);
    unsigned major = imajor(f->f_inode);
    char *module_data = module_data_tab[minor];
    // cpoie au maximum count octets vers le buffer utilisateur buf
    // met à jour la position off dans le fichier
    // retourne le nombre d'octets copiés
    size_t max_read = data_saved_tab[minor] - *off;
    char *read_from = module_data + *off;

    pr_info("Pilotes Ex3: my_read called\n");
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
        pr_info("Pilotes Ex3: copy_to_user failed\n");
        return -EFAULT;
    }
    return count;
}

// pour l'opération write
static ssize_t my_write(struct file *f, const char __user *buf, size_t count, loff_t *off){
    pr_info("Pilotes Ex3: __%s__\n", buf);
    unsigned minor = iminor(f->f_inode);
    unsigned major = imajor(f->f_inode);
    size_t max_write = MAX_SIZE - *off;
    char *module_data = module_data_tab[minor];
    // size_t *data_saved = data_saved_tab[minor];
    char *write_to = module_data + *off;

    pr_info("Pilotes Ex3: my_write called\n");
    if(count > max_write || count < 0){
        pr_err("Pilotes Ex3: my_write failed max=%d, count=%d\n", max_write, count);
        return -EFAULT;
    }
    // copie au maximum count octets depuis le buffer utilisateur buf
    // met à jour la position off dans le fichier
    // retourne le nombre d'octets copiés
    // copie dans espace kernel
    // copy_from_user (void * to, const void __user * from, unsigned long n)
    if(copy_from_user(write_to, buf, count)){
        pr_info("Pilotes Ex3: copy_from_user failed\n");
        return -EFAULT;
    }
    pr_info("Pilotes Ex3: __%s__\n", write_to);
    *off += count;
    // mise à jour de la taille des données sauvegardées
    // data_saved = *off;
    data_saved_tab[minor] = *off;
       
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
    int i=0;
    pr_info("Pilotes Ex3: 'hello'\n");
    pr_info("Pilotes Ex3: instances = %d\n", instances);

    if(instances <= 0){
        pr_err("Pilotes Ex3: instances must be > 0\n");
        return -EFAULT;
    }

    // 3. Réservation dynamique du numéro de pilote
    // alloc_chrdev_region (dev_t *dev, unsigned baseminor, unsigned count, const char *name)
    if(alloc_chrdev_region(&my_dev_buff, 0, instances, "my_dev_buffer")){
        pr_info("Pilotes Ex3: alloc_chrdev_region failed\n");
        return -1;
    }

    // 4. Association des opérations sur le fichier au numéro de pilote
    // cdev_init (struct cdev *cdev, const struct file_operations *fops)
    cdev_init(&my_cdev, &my_fops);
    //owner assigné à THIS_MODULE
    my_cdev.owner = THIS_MODULE;
    // enregisrement du pilote dans le noyau
    // cdev_add (struct cdev *p, dev_t dev, unsigned count)
    if(cdev_add(&my_cdev, my_dev_buff, instances)){
        pr_info("Pilotes Ex3: cdev_add failed\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(my_dev_buff, instances);
        return -1;
    }

    // alloc buffer
    module_data_tab = kmalloc(sizeof(char *) * instances, GFP_KERNEL);
    if(module_data_tab == NULL){
        pr_info("Pilotes Ex3: kmalloc failed\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(my_dev_buff, instances);
        return -1;
    }
    for(i = 0; i < instances; i++){
        module_data_tab[i] = kmalloc(sizeof(char) * MAX_SIZE, GFP_KERNEL);
        if(module_data_tab[i] == NULL){
            pr_info("Pilotes Ex3: kmalloc failed\n");
            cdev_del(&my_cdev);
            unregister_chrdev_region(my_dev_buff, instances);
            return -1;
        }
    }
    data_saved_tab = kmalloc(sizeof(size_t) * instances, GFP_KERNEL);
    if(data_saved_tab == NULL){
        pr_info("Pilotes Ex3: kmalloc failed\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(my_dev_buff, instances);
        return -1;
    }
    for(i = 0; i < instances; i++){
        data_saved_tab[i] = 0;
    }
        
    pr_info("Pilotes Ex3: Init done\n");
    // Affichage des informations
    pr_info("Pilotes Ex3: Major number: %d Minor number: %d\n", MAJOR(my_dev_buff), MINOR(my_dev_buff));
    return 0;
}

static void __exit my_module_exit(void){
    // Libération des ressources
    // cdev_del (struct cdev *p)
    cdev_del(&my_cdev);
    // unregister_chrdev_region (dev_t from, unsigned count)
    unregister_chrdev_region(my_dev_buff, instances);
    pr_info ("Pilotes Ex3: 'bye'\n");
}


module_init (my_module_init);
module_exit (my_module_exit);

MODULE_AUTHOR ("T.Dietrich & K.Goundiaev");
MODULE_DESCRIPTION ("My module test");
MODULE_LICENSE ("GPL");
