#include "periodic.h"
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/spinlock_types.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
//#include <linux/sched/signal.h>

#define PRINT_STAT 1

struct periodic_conf {
  unsigned long timeout_ms;
  struct timer_list tl;
  //struct completion timer_elapsed;
  struct completion unblock_ready;
  bool blocked_user;
  spinlock_t blocked_user_lock;
  //struct task_struct * callback_helper_thread_id;
  bool running;
  struct mutex running_mutex;
};

/* timer_hard_isr() - callback handler for timer
 */
void timer_hard_isr(struct timer_list * pc_ptr){
  struct periodic_conf * pc;
  pc = container_of(pc_ptr, struct periodic_conf, tl);

  mod_timer(pc_ptr, jiffies + msecs_to_jiffies(pc->timeout_ms));

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
}

/* wait_for_timeout() - blocks until timer elapses
 *
 * Sets signals that the user application is blocked and blocks
 */
int wait_for_timeout(struct periodic_conf * pc){
  if (!pc)
    return -1;

  if (!pc->running)
    return -2;
  
  spin_lock(&pc->blocked_user_lock);
  pc->blocked_user = true;
  spin_unlock(&pc->blocked_user_lock);

  wait_for_completion_interruptible(&pc->unblock_ready);

  return 0;
}

void init_periodic_conf(struct periodic_conf * pc){

  if (!pc) return ;
  
  pc->timeout_ms = 0;
  spin_lock_init(&pc->blocked_user_lock);
  init_completion(&pc->unblock_ready);
  pc->blocked_user = false;
  pc->running = false;
  mutex_init(&pc->running_mutex);
}

void * create_periodic_conf(){
  void * ptr;
  ptr = kmalloc(sizeof(struct periodic_conf), GFP_KERNEL);
  
  init_periodic_conf(ptr);

  return ptr;
}

void delete_periodic_conf(struct periodic_conf * pc){
  if (!pc) {
    return;
  }
  kfree(pc);
}

// start_cycling() - activate the timer
int start_cycling(struct periodic_conf * pc){
  if (!pc) return -1;
  
  mutex_lock(&pc->running_mutex);
  if (pc->running){
    mutex_unlock(&pc->running_mutex);
    return -2;
  }

  if (pc->timeout_ms == 0){
    mutex_unlock(&pc->running_mutex);
    return -3;
  }

  timer_setup(&pc->tl, timer_hard_isr, 0);
  mod_timer(&pc->tl, jiffies + msecs_to_jiffies(pc->timeout_ms));

  pc->running  = true;
  mutex_unlock(&pc->running_mutex);

  return 0;
}

// stop_cycling() - stop the timer
int stop_cycling(struct periodic_conf * pc){
  if (! pc) return -1;
  
  mutex_lock(&pc->running_mutex);
  if (!pc->running){
    mutex_unlock(&pc->running_mutex);
    return -2;
  }

  del_timer(&pc->tl);
  pc->running = false;
  mutex_unlock(&pc->running_mutex);
  
  return 0;
}

/* periodic_conf_set_timeout_ms() - sets the timeout value */
int periodic_conf_set_timeout_ms(struct periodic_conf * pc,
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

  pc->timeout_ms = tms;

  mutex_unlock(&pc->running_mutex);
  return 0;
}

/* periodic_conf_get_timeout_ms() - gets the timeout value */
unsigned long periodic_conf_get_timeout_ms(struct periodic_conf * pc){
  if (! pc) return 0;

  return pc->timeout_ms;
}



