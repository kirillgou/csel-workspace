// fan_ctl.c
/*

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
#include <linux/gpio.h> /* need for gpio functions */
#include <linux/fs.h> /* need for file operations inode & file struct */
#include <linux/uaccess.h> /* need for copy_to_user & copy_from_user */
#include <linux/kdev_t.h> /* need for MAJOR & MINOR */
#include <linux/cdev.h> /* need for cdev functions */
#include <linux/thermal.h> /* need for thermal zone */
#include <linux/timer.h> /* need for timer */
// timer example: https://yannik520.github.io/linux_driver_code/timer/timer_example.html
//In operating systems, especially Unix, a jiffy is the time between two successive clock ticks. 
// thermal example: https://github.com/torvalds/linux/blob/master/drivers/thermal/thermal_core.c

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

#define CLASS_NAME  "fan_ctl"
#define DEVICE_NAME "fan_ctl"

#define S_IN_US     1000000
#define MS_IN_US    1000

#define TEMP_PERIOD_MS  500
#define TEMP_PERIOD_US  (TEMP_PERIOD_MS * MS_IN_US)

#define GPIO_LED_GREEN 10
#define GPIO_FAN       GPIO_LED_GREEN

#define TEMP_THR_1  35000
#define TEMP_THR_2  40000
#define TEMP_THR_3  45000
#define TEMP_FRQ_1  2
#define TEMP_FRQ_2  5
#define TEMP_FRQ_3  10
#define TEMP_FRQ_4  20



//variable globale
static dev_t my_dev_buff = 0;
static struct cdev my_cdev;
static struct class *my_class = NULL;
struct device *my_device = NULL;

// structure du device attributs
typedef struct my_device_attribute_struct {
    int frequency_Hz;
    int period_us_d2;
    int auto_config;
    int temperature_mC;
}my_device_attribute_struct;

my_device_attribute_struct my_device_attribute; 

static struct timer_list timer_temprature;
static struct timer_list timer_fan;

// int get_temperature(void){

//     return temp;
// }

void run_timer(struct timer_list *timer, unsigned long period_us){
    int ret;
    ret = mod_timer(timer, jiffies + usecs_to_jiffies(period_us));
    if(ret){
        pr_err("Pilotes Fan_ctl: Error in mod_timer\n");
    }
}

void timer_temprature_callback(struct timer_list *timer){
    struct thermal_zone_device *tzd;
    int temperature, ret, last_f;
    pr_debug("Pilotes Fan_ctl: timer_temprature_callback called\n");
    run_timer(&timer_temprature, TEMP_PERIOD_US);

    tzd = thermal_zone_get_zone_by_name("cpu-thermal");
    ret = thermal_zone_get_temp(tzd, &temperature);
    if(ret){
        pr_err("Pilotes Fan_ctl: Error in thermal_zone_get_temp\n");
        return;
    }
    my_device_attribute.temperature_mC = temperature;
    // can't print float
    pr_info("Pilotes Fan_ctl: temperature: %d.%d\n", temperature / 1000, temperature % 1000);
    if(my_device_attribute.auto_config){
        last_f = my_device_attribute.frequency_Hz;
        if(temperature < TEMP_THR_1){
            my_device_attribute.frequency_Hz = TEMP_FRQ_1;
        }else if(temperature < TEMP_THR_2){
            my_device_attribute.frequency_Hz = TEMP_FRQ_2;
        }else if(temperature < TEMP_THR_3){
            my_device_attribute.frequency_Hz = TEMP_FRQ_3;
        }else{
            my_device_attribute.frequency_Hz = TEMP_FRQ_4;
        }
        my_device_attribute.period_us_d2 = S_IN_US / (2 * my_device_attribute.frequency_Hz);
        if(last_f == 0){
            run_timer(&timer_fan, my_device_attribute.period_us_d2);
        }
    }
    
}

void timer_fan_callback(struct timer_list *timer){
    static int state = 0;
    pr_debug("Pilotes Fan_ctl: timer_fan_callback called\n");
    if(my_device_attribute.period_us_d2 == 0){
        state = 0;
    }else{
        run_timer(&timer_fan, my_device_attribute.period_us_d2);
        state = (state + 1) % 2;
    }
        gpio_set_value(GPIO_FAN, state);
}






ssize_t show_frequency_Hz(struct device *dev, struct device_attribute *attr, char *buf){
    pr_info("Pilotes Fan_ctl: show_frequency_Hz called\n");
    sprintf(buf, "%d\n", my_device_attribute.frequency_Hz);
    return strlen(buf);
}

ssize_t store_frequency_Hz(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
    int frequency_Hz, last_f;
    if(my_device_attribute.auto_config){
        // pr_info("Pilotes Fan_ctl: auto_config is enabled, frequency_Hz can't be changed\n");
        return -1;
    }
    // pr_debug("Pilotes Fan_ctl: store_frequency_Hz called\n");
    last_f = my_device_attribute.frequency_Hz;
    sscanf(buf, "%d", &frequency_Hz);
    if(frequency_Hz < 0){
        // pr_info("Pilotes Fan_ctl: frequency_Hz must be positive\n");
        return -1;
    }
    my_device_attribute.frequency_Hz = frequency_Hz;
    if(my_device_attribute.frequency_Hz == 0){
        my_device_attribute.period_us_d2 = 0;
    }else{
        my_device_attribute.period_us_d2 = S_IN_US / (2 * my_device_attribute.frequency_Hz);
    }
    if(last_f == 0 && my_device_attribute.frequency_Hz != 0){
        run_timer(&timer_fan, my_device_attribute.period_us_d2);
    }
    return count;
}
ssize_t show_auto_config(struct device *dev, struct device_attribute *attr, char *buf){
    pr_info("Pilotes Fan_ctl: show_auto_config called\n");
    sprintf(buf, "%d\n", my_device_attribute.auto_config);
    return strlen(buf);
}
ssize_t store_auto_config(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
    int auto_config;
    pr_info("Pilotes Fan_ctl: store_auto_config called\n");
    sscanf(buf, "%d", &auto_config);
    my_device_attribute.auto_config = auto_config ? 1 : 0;
    return count;
}

//polling function
// static ssize_t store(struct kobject *kobj, struct attribute *attr, 
//                      const char *buf, size_t len) { 
//     struct d_attr *da = container_of(attr, struct d_attr, attr); 
 
//     sscanf(buf, "%d", &da->value); 
//     printk("sysfs_foo store %s = %d\n", a->attr.name, a->value); 
 
//     if (strcmp(a->attr.name, "foo") == 0){ 
//         foo.value = a->value; 
//         sysfs_notify(mykobj, NULL, "foo"); 
//     } 
//     else if(strcmp(a->attr.name, "bar") == 0){ 
//         bar.value = a->value; 
//         sysfs_notify(mykobj, NULL, "bar"); 
//     } 
//     return sizeof(int); 
// } 

ssize_t show_temperature_mC(struct device *dev, struct device_attribute *attr, char *buf){
    pr_debug("Pilotes Fan_ctl: show_temperature_mC called\n");
    sprintf(buf, "%d\n", my_device_attribute.temperature_mC);
    return strlen(buf);
}

// déclaration de l'attribut
// my_drive_attr_name est le nom de l'attribut qui sera utiliser par la suite et vu dans sysfs
// 0664 est le mode d'accès, 666 pas possible 
DEVICE_ATTR(frequency_Hz, 0664, show_frequency_Hz, store_frequency_Hz);
DEVICE_ATTR(auto_config, 0664, show_auto_config, store_auto_config);
DEVICE_ATTR(temperature_mC, 0444, show_temperature_mC, NULL);

// 1. Implémentation des opérations sur les fichiers (handler) 
// pour l'opération open
static int my_open(struct inode *i, struct file *f){
    pr_info("Pilotes Fan_ctl: my_open called\n");
    // inode: représente de façon unique un fichier dans le noyau Linux
    // file: représente un fichier ouvert par un processus, il peut exister 
    //      plusieurs structures de fichiers attachées au même inode.
    // file contient position actuelle, mode d'ouverture, un pointeur void * private_data
    return 0;
}

// pour l'opération release
static int my_release(struct inode *i, struct file *f){
    pr_info("Pilotes Fan_ctl: my_release called\n");
    return 0;
}

// 2. Définition de la structure struct file_operations
static struct file_operations my_fops = {
    .owner  = THIS_MODULE,
    .open   = my_open,
    // read and write are forbidden
    // .read   = my_read,
    // .write  = my_write,
    .release= my_release,
};

static int __init my_module_init(void){
    // 3. Réservation dynamique du numéro de pilote
    // alloc_chrdev_region (dev_t *dev, unsigned baseminor, unsigned count, const char *name)
    if(alloc_chrdev_region(&my_dev_buff, 0, NUMBER_OF_MINORS, "my_dev_buffer")){
        pr_info("Pilotes Fan_ctl: alloc_chrdev_region failed\n");
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
        pr_info("Pilotes Fan_ctl: cdev_add failed\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(my_dev_buff, NUMBER_OF_MINORS);
        return -1;
    }
    
    // creation de class
    my_class = class_create(THIS_MODULE, CLASS_NAME);
    if(IS_ERR(my_class)){
        pr_info("Pilotes Fan_ctl: class_create failed\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(my_dev_buff, NUMBER_OF_MINORS);
        return -1;
    }

    // creation de device
    my_device = device_create(my_class, NULL, my_dev_buff, NULL, DEVICE_NAME);
    if(IS_ERR(my_device)){
        pr_info("Pilotes Fan_ctl: device_create failed\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(my_dev_buff, NUMBER_OF_MINORS);
        return -1;
    }

    my_device_attribute.frequency_Hz = 2;
    my_device_attribute.period_us_d2 = S_IN_US / (2 * my_device_attribute.frequency_Hz);
    my_device_attribute.auto_config = 1;
    // creation de sysfs attribute file
    device_create_file(my_device, &dev_attr_frequency_Hz);
    device_create_file(my_device, &dev_attr_auto_config);
    device_create_file(my_device, &dev_attr_temperature_mC);

    // init gpio
    if (gpio_request(GPIO_FAN, "Fan_gpio") != 0){
        pr_info("Module Ex8: gpio_request(GPIO_FAN, 'Fan_gpio') failed\n");
        gpio_free(0);
        return -1;
    }
    gpio_direction_output(GPIO_FAN, 0);

    // init timer
    timer_setup(&timer_temprature, timer_temprature_callback, 0);
    run_timer(&timer_temprature, TEMP_PERIOD_US);
    timer_setup(&timer_fan, timer_fan_callback, 0);
    run_timer(&timer_fan, my_device_attribute.period_us_d2);

    pr_info("Pilotes Fan_ctl: Init done\n");
    pr_info("Pilotes Fan_ctl: Major number: %d Minor number: %d\n", MAJOR(my_dev_buff), MINOR(my_dev_buff));
    return 0;
}

static void __exit my_module_exit(void){
    // delete timer
    del_timer(&timer_temprature);
    del_timer(&timer_fan);

    // free gpio
    gpio_free(GPIO_FAN);

    // Libération des ressources
    // destroy sysfs attribute file
    device_remove_file(my_device, &dev_attr_frequency_Hz);
    device_remove_file(my_device, &dev_attr_auto_config);
    // destroy device
    device_destroy(my_class, my_dev_buff);
    // destroy class
    class_destroy(my_class);
    // cdev_del (struct cdev *p)
    cdev_del(&my_cdev);
    // unregister_chrdev_region (dev_t from, unsigned count)
    unregister_chrdev_region(my_dev_buff, NUMBER_OF_MINORS);
    pr_info ("Pilotes Fan_ctl: 'bye'\n");
}

module_init (my_module_init);
module_exit (my_module_exit);

MODULE_AUTHOR ("T.Dietrich & K.Goundiaev");
MODULE_DESCRIPTION ("Fan control driver");
MODULE_LICENSE ("GPL");