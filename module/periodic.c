#include "periodic.h"
#include <linux/hrtimer.h>
#include <linux/time.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/spinlock_types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/list_sort.h>
#include <linux/pid.h>

#define PRINT_STAT 1

struct periodic_conf {
  ktime_t timeout;
  struct hrtimer hrt;
  
  struct completion unblock_ready;
  bool blocked_user;
  spinlock_t blocked_user_lock;
  
  bool running;
  struct mutex running_mutex;
  
  struct list_head pconf_list;
  struct pid * thread_pid;
};

struct periodic_conf_list {
  struct mutex pconf_mutex;
  struct list_head h;
};


/* compare_timeout()
 * Used to establish which list element should 
 * precede and which should follow out of 
 * the parameters
 *
 * @priv:	not used, please set to NULL
 * @a:		first element to compare
 * @b:		second element to compare
 *
 * @a and @b should be the pconf_list element of 
 * a struct periodic_conf, namely *pca and *pcb.
 * Return:
 * 	case: pca->timeout < pcb->timeout : -1
 * 	case: pca->timeout > pcb->timeout :  1
 * 	case: pca->timeout ==pcb->timeout :  0
 */
int compare_timeout(
    void * priv __attribute__((unused)), 
    struct list_head * a, 
    struct list_head * b
    ){

  struct periodic_conf * pca, * pcb;

  pca = list_entry(a, struct periodic_conf, pconf_list);
  pcb = list_entry(b, struct periodic_conf, pconf_list);

  return ktime_compare(pca->timeout, pcb->timeout);
}

/* periodic_conf_list_insert()
 * insert a new timer comfiguration in the list
 * keep it ordered so that shorter timeouts have 
 * shorter distance from the list head
 *
 * @pcl:	configuration list pointer
 * @pc:		pointer to element to add
 */
void periodic_conf_list_insert(struct periodic_conf_list * pcl, struct periodic_conf * pc) {
  if (!pcl || !pc) return ;

  mutex_lock(&pcl->pconf_mutex);
  list_add(&pc->pconf_list, &pcl->h);
  list_sort(NULL, &pcl->h, compare_timeout);
  mutex_unlock(&pcl->pconf_mutex);
}

/* get_periodic_conf()
 * get periodic_conf pointer for the current task
 *
 * @pcl:	configuration list pointer
 *
 * Return: pointer to periodic_conf  of the current task
 * if it has set a timeout, NULL othrewise
 */
struct periodic_conf * get_periodic_conf(struct periodic_conf_list * pcl){
  struct pid * thread_pid;
  struct periodic_conf * pc;
  struct list_head * pos;

  thread_pid = get_task_pid(current, PIDTYPE_PID);

  mutex_lock(&pcl->pconf_mutex);
  list_for_each(pos, &pcl->h){
    pc = list_entry(pos, struct periodic_conf, pconf_list);
    if (pc->thread_pid == thread_pid){
      mutex_unlock(&pcl->pconf_mutex);
      return pc;
    }
  }

  mutex_unlock(&pcl->pconf_mutex);
  return NULL;
}

/* hrtimer_hard_isr() - callback handler for timer
 * Postpone high resolution timer and unblock user
 */
enum hrtimer_restart timer_hard_isr(struct hrtimer * hrtimer_ptr){
  struct periodic_conf * pc;
  pc = container_of(hrtimer_ptr, struct periodic_conf, hrt);

  hrtimer_forward_now(hrtimer_ptr, pc->timeout);

  spin_lock_irq(&pc->blocked_user_lock);
  if (pc->blocked_user){
    // complete only if blocked user thread is present
    complete(&pc->unblock_ready);
    pc->blocked_user = false;
    spin_unlock_irq(&pc->blocked_user_lock);
  } else {
    spin_unlock_irq(&pc->blocked_user_lock);
    printk("Missed timeout!");
  }

  return HRTIMER_RESTART;
}

/* __wait_for_timeout() - blocks until timer elapses
 *
 * Signals that the user application is blocked and blocks
 *
 * The periodic_conf of the current thread must be known
 */
int __wait_for_timeout(struct periodic_conf * pc){
  if (!pc)
    return -1;

  if (!pc->running)
    return -2;
  
  spin_lock(&pc->blocked_user_lock);
  pc->blocked_user = true;
  spin_unlock(&pc->blocked_user_lock);

  wait_for_completion(&pc->unblock_ready);

  return 0;
}

/* wait_for_timeout()
 * extracts periodic_conf of current task
 * and wraps the previous function
 */
int wait_for_timeout(struct periodic_conf_list * pcl){
  return __wait_for_timeout(get_periodic_conf(pcl));
}

void init_periodic_conf(struct periodic_conf * pc){

  if (!pc) return ;
  
  pc->timeout = 0;
  spin_lock_init(&pc->blocked_user_lock);
  init_completion(&pc->unblock_ready);
  pc->blocked_user = false;
  pc->running = false;
  mutex_init(&pc->running_mutex);
  pc->thread_pid = get_task_pid(current, PIDTYPE_PID);
  INIT_LIST_HEAD(&pc->pconf_list);
}

void * create_periodic_conf(void){
  void * ptr;
  ptr = kmalloc(sizeof(struct periodic_conf), GFP_KERNEL);
  
  init_periodic_conf(ptr);

  return ptr;
}

void * create_periodic_conf_list(void){
  struct periodic_conf_list * ptr;
  ptr = kmalloc(sizeof(struct periodic_conf_list), GFP_KERNEL);

  INIT_LIST_HEAD(&ptr->h);
  mutex_init(&ptr->pconf_mutex);

  return ptr;
}

void delete_periodic_conf(struct periodic_conf * pc){
  if (!pc) {
    return;
  }

  hrtimer_cancel(&pc->hrt);
  list_del(&pc->pconf_list);

  kfree(pc);
}

void delete_periodic_conf_list(struct periodic_conf_list * pcl) {
  struct list_head * tmp, *node;
  struct periodic_conf * pc;

  if (!pcl)
    return;

  mutex_lock(&pcl->pconf_mutex);
  list_for_each_safe(node, tmp, &pcl->h) {
    pc = list_entry(node, struct periodic_conf, pconf_list);
    delete_periodic_conf(pc);
  }

  mutex_unlock(&pcl->pconf_mutex);

  kfree(pcl);
}


// start_cycling() - activate the timer
int __start_cycling(struct periodic_conf * pc){
  if (!pc) return -1;
  
  mutex_lock(&pc->running_mutex);
  if (pc->running){
    mutex_unlock(&pc->running_mutex);
    return -2;
  }

  if (pc->timeout == 0){
    mutex_unlock(&pc->running_mutex);
    return -3;
  }

  hrtimer_init(&pc->hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  pc->hrt.function = timer_hard_isr ;
  hrtimer_start(&pc->hrt, pc->timeout, HRTIMER_MODE_REL);

  pc->running  = true;
  mutex_unlock(&pc->running_mutex);

  return 0;
}

int start_cycling(struct periodic_conf_list * pcl){
  return __start_cycling(get_periodic_conf(pcl));
}

// stop_cycling() - stop the timer
int __stop_cycling(struct periodic_conf * pc){
  if (! pc) return -1;
  
  mutex_lock(&pc->running_mutex);
  if (!pc->running){
    mutex_unlock(&pc->running_mutex);
    return -2;
  }

  hrtimer_cancel(&pc->hrt);
  pc->running = false;
  mutex_unlock(&pc->running_mutex);
  
  return 0;
}

int stop_cycling(struct periodic_conf_list * pcl){
  return __stop_cycling(get_periodic_conf(pcl));
}

/* periodic_conf_set_timeout_ms() - sets the timeout value */
int __periodic_conf_set_timeout_ms(struct periodic_conf * pc,
				 unsigned long tms){
  if (!pc)
    return -1;

  if (tms == 0)
    return -2;
  
  mutex_lock(&pc->running_mutex);

  if (pc->running) {
    mutex_unlock(&pc->running_mutex);
    return -3;
  }

  pc->timeout = ktime_set(tms/1000, (tms % 1000)*1000000);

  mutex_unlock(&pc->running_mutex);
  return 0;
}

int periodic_conf_set_timeout_ms(
    struct periodic_conf_list * pcl,
    unsigned long tms ) {

  struct periodic_conf * pc;
  int res;

  res = __periodic_conf_set_timeout_ms(get_periodic_conf(pcl), tms);

  switch (res) {
    case -1:
      
      pc = create_periodic_conf();
      if (! pc) 
	return -1;
      
      periodic_conf_list_insert(pcl, pc);
      return __periodic_conf_set_timeout_ms(pc, tms);

    default:
      return res;
  }
}

/* __periodic_conf_get_timeout_ms() - gets the timeout value */
unsigned long __periodic_conf_get_timeout_ms(struct periodic_conf * pc){
  if (! pc) return 0;

  return pc->timeout/1000000;
}

unsigned long periodic_conf_get_timeout_ms(struct periodic_conf_list * pcl){
  return __periodic_conf_get_timeout_ms(get_periodic_conf(pcl));
}



