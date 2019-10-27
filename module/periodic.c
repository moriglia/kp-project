#include "periodic.h"
#include <linux/hrtimer.h>
#include <linux/time.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/spinlock_types.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#define PRINT_STAT 1

struct periodic_conf {
  ktime_t timeout;
  struct hrtimer hrt;
  struct completion unblock_ready;
  bool blocked_user;
  spinlock_t blocked_user_lock;
  bool running;
  struct mutex running_mutex;
};

/* hrtimer_hard_isr() - callback handler for timer
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
  
  pc->timeout = 0;
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

// stop_cycling() - stop the timer
int stop_cycling(struct periodic_conf * pc){
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

  pc->timeout = ktime_set(tms/1000, (tms % 1000)*1000000);

  mutex_unlock(&pc->running_mutex);
  return 0;
}

/* periodic_conf_get_timeout_ms() - gets the timeout value */
unsigned long periodic_conf_get_timeout_ms(struct periodic_conf * pc){
  if (! pc) return 0;

  return pc->timeout/1000000;
}



