// my_module.c
/*
Exercice 7: Mise en sommeil
Développez un petit module permettant d’instancier deux threads 
dans le noyau. Le premier thread attendra une notification 
de réveil du deuxième thread et se remettra en sommeil. 
Le 2ème thread enverra cette notification toutes les 5 secondes 
et se rendormira. On utilisera 

les waitqueues

 pour les mises 
en sommeil. Afin de permettre le debugging du module, 
chaque thread affichera un petit message à chaque réveil.
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
#include <linux/wait.h> /* need for waits */
#include <linux/atomic.h> /* need for atomic finctions */

#define P_COLOR_RST "\033[1;0m"
#define P_COLOR_RED "\033[1;31m"
#define P_COLOR_GREEN "\033[1;32m"
#define P_COLOR_YELLOW "\033[1;33m"
#define P_COLOR_BLUE "\033[1;34m"
#define P_COLOR_MAGENTA "\033[1;35m"
#define P_COLOR_CYAN "\033[1;36m"
#define P_COLOR_WHITE "\033[1;37m"

#define TH1_C P_COLOR_BLUE
#define TH2_C P_COLOR_GREEN
#define P_C P_COLOR_RST


typedef struct thread_data{
    int thread_id;
    wait_queue_head_t *queue;
    atomic_t *v;
}thread_data;

struct task_struct *th1;
struct task_struct *th2;

thread_data th_data[2];

// init queue static
// DECLARE_WAIT_QUEUE_HEAD(queue);


// thread notify function
int th_notify_5s(void* data){
    pr_info("%sModule Ex7_notify_th: Start Thread%s\n", TH1_C, P_C);
    thread_data *p = (thread_data *)data;
    ssleep(5);
    int i  = 0;

    while(!kthread_should_stop()){
        ssleep(5);
        //notify
        atomic_inc(p->v);
        pr_info("%sModule Ex7_notify_th: %d th_wait notified%s\n", TH1_C, i++, P_C);
        // pr_info("Module Ex7_notify_th: %d value atomic", atomic_read(p->v));
        wake_up(p->queue);
    }
    return 0;
}

// thread waiting function
int th_wait_sig(void* data){
    pr_info("%sModule Ex7_wait_th: Start Thread%s\n", TH2_C, P_C);
    thread_data *p = (thread_data *)data;
    int i  = 0;
    
    while(!kthread_should_stop()){
        //wait notification
        wait_event_killable(*(p->queue), atomic_read(p->v)>=1);
        atomic_dec(p->v);
        pr_info("%sModule Ex7_wait_th: %d recive sign%s\n", TH2_C, i++, P_C);
    }
    return 0;
}


static int __init my_module_init(void){
    pr_info ("Module Ex7: 'hello'\n");
    thread_data param;    
    wait_queue_head_t *queue = kmalloc(sizeof(wait_queue_head_t), GFP_KERNEL);
    init_waitqueue_head (queue);    
    param.queue = queue;
    atomic_t *pval = kmalloc(sizeof(atomic_t), GFP_KERNEL);
    atomic_set(pval, 0);
    param.v = pval;

    param.thread_id = 0;
    th_data[0] = param;
    param.thread_id = 1;
    th_data[1] = param;

    th2 = kthread_run(&th_wait_sig, &th_data[1], "ex7_th_waiting");
    th1 = kthread_run(&th_notify_5s, &th_data[0], "ex7_th_notify");
    if(!th1){
        pr_err("Module Ex7: ERR on kthread_run\n");
    }
    if(!th2){
        pr_err("Module Ex7: ERR on kthread_run\n");
    }
    
    pr_info("Module Ex7: Init done\n");
    return 0;
}

static void __exit my_module_exit(void){
    if(kthread_stop(th1)){
        pr_err("Module Ex7: ERR on kthread_stop th1\n");
    }
    if(kthread_stop(th2)){
        pr_err("Module Ex7: ERR on kthread_stop th2\n");
    }

    // free memory
    kfree(th_data[0].v);
    kfree(th_data[0].queue);
    pr_info ("My_module say : 'bye'\n");
}


module_init (my_module_init);
module_exit (my_module_exit);

MODULE_AUTHOR ("T.Dietrich & K.Goundiaev");
MODULE_DESCRIPTION ("My module test");
MODULE_LICENSE ("GPL");
