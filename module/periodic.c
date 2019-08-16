#include "periodic.h"
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
//#include <linux/sched/signal.h>

#define PRINT_STAT 1

struct periodic_conf {
  unsigned long timeout_ms;
  struct timer_list tl;
  struct completion timer_elapsed;
  struct completion unblock_ready;
  bool blocked_user;
  struct mutex blocked_user_mutex;
  struct task_struct * callback_helper_thread_id;
  bool running;
  struct mutex running_mutex;
  bool should_stop;
};

/* quick_periodic_callback() - callback handler for timer
 *
 * Restarts (postpones) the timer and unblocks the 
 * callback helper thread
 *
 * Note: the operations done in `callback_helper_thread()`
 * could not be done in this callback handler because 
 * they involved acquiring a mutex, which is not allowed 
 * in this context
 */
void quick_periodic_callback(struct timer_list * pc_ptr){
  struct periodic_conf * pc;
  pc = container_of(pc_ptr, struct periodic_conf, tl);

  mod_timer(pc_ptr, jiffies + msecs_to_jiffies(pc->timeout_ms));
  
  complete(&pc->timer_elapsed);
}

/* callback_helper_thread() - code for the thread that wakes up the user process
 * 
 * This thread checks whether there is a user process blocked every time it 
 * gets woken up itself, then possibly unblocks it.
 * 
 * See note on `quick_periodic_callback()` for the reason for not including
 * the code below in the callback itself.
 */
int callback_helper_thread(void * pc_ptr){
  struct periodic_conf * pc;
  int res;
  pc = (struct periodic_conf*) pc_ptr;

  while(!kthread_should_stop()){
    res = wait_for_completion_interruptible(&pc->timer_elapsed);
    if (res!=0 || pc->should_stop){
      return 0;
    }

    mutex_lock(&pc->blocked_user_mutex);
    if (pc->blocked_user){
      // complete only if blocked user thread is present
      complete(&pc->unblock_ready);
      pc->blocked_user = false;
      mutex_unlock(&pc->blocked_user_mutex);
    } else {
      mutex_unlock(&pc->blocked_user_mutex);
      printk("Missed timeout!");
    }
  }

  return 0;

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
  
  mutex_lock(&pc->blocked_user_mutex);
  pc->blocked_user = true;
  mutex_unlock(&pc->blocked_user_mutex);

  wait_for_completion_interruptible(&pc->unblock_ready);

  return 0;
}


int create_thread(struct periodic_conf * pc){
  if (pc->callback_helper_thread_id > 0){
    return 1;
  }

  pc->callback_helper_thread_id = kthread_run(callback_helper_thread,
					      pc, "callback_helper");
  if (IS_ERR(pc->callback_helper_thread_id)) {
    printk("Impossible to create thread");
    return 1;
  }

  return 0;
}

void delete_helper_thread(struct periodic_conf * pc){
  pc->should_stop = true;
  complete(&pc->timer_elapsed);
  kthread_stop(pc->callback_helper_thread_id);
}

void init_periodic_conf(struct periodic_conf * pc){

  if (!pc) return ;
  
  pc->timeout_ms = 0;
  mutex_init(&pc->blocked_user_mutex);
  init_completion(&pc->unblock_ready);
  init_completion(&pc->timer_elapsed);
  pc->blocked_user = false;
  pc->callback_helper_thread_id = NULL;
  pc->running = false;
  mutex_init(&pc->running_mutex);
  pc->should_stop = false;
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
  delete_helper_thread(pc);
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

  timer_setup(&pc->tl, quick_periodic_callback, 0);
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



