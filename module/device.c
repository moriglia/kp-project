#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/stat.h>
#include "device.h"
#include "periodic.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif



static struct miscdevice kp_device;

/* kp_dev_open() - opens misc device
 * 
 * Allocates a new periodic_conf structure (see "periodic.c")
 * Starts a new callback helper thread 
 */
static int kp_dev_open(struct inode* in, struct file* f){
#if VERBOSE
  printk("Opening device");
#endif
  f->private_data = create_periodic_conf();
  if (!f->private_data) {
    return -1;
  }
  if (create_thread(f->private_data)) {
    delete_periodic_conf(f->private_data);
    return -1;
  }
  return 0;
}

/* kp_dev_release() - closes misc device
 * 
 * Deallocates the periodic_conf structure and stops callback helper thread.
 */
static int kp_dev_release(struct inode* in, struct file* f){
#if VERBOSE
  printk("Closing device");
#endif
  delete_periodic_conf(f->private_data);
  f->private_data = NULL;
  return 0;
}

/* kp_dev_ioctl() - handler for ioctl() calls
 * 
 * The definition if the cmd possible values are 
 * in the header file "device.h"
 */
static long kp_dev_ioctl(struct file* f, unsigned int cmd, unsigned long arg){
  int resi;
  long resl;

  if (! f)
    return -ERR_GENERIC;
  
  switch (cmd){

  case TIMEOUT_SET:
    resi = periodic_conf_set_timeout_ms(f->private_data, arg);
    switch ( resi ){
    case -1:
      BUG();
      return -ERR_GENERIC;
    case -2:
      return -ERR_INVALID_TIMEOUT;
    case -3:
      return -ERR_TIMEOUT_NOT_SET;
    default:
      return 0;
    } 

  case TIMEOUT_GET:
    resl = periodic_conf_get_timeout_ms(f->private_data);
    if (resl == 0)
      return -ERR_TIMEOUT_NOT_SET;
    return resl;
    
  case BLOCK:
    if (!f->private_data) {
      return -1 ; // handle error
    }
    resi = wait_for_timeout(f->private_data);
    if (resi == -1){
      BUG();
    }
    return 0;

  case START:
    resi = start_cycling(f->private_data);
    switch (resi){
    case -1:
      BUG();
      return -ERR_GENERIC;
    case -2:
      // already running
      return -ERR_RUNNING_STATE;
    case -3:
      return -ERR_TIMEOUT_NOT_SET;
    default:
      if (resi != 0){
	BUG();
	return -ERR_GENERIC;
      }
    }
    
    return 0;
    
  case STOP:
    resi = stop_cycling(f->private_data);
    if (resi == -1) {
      BUG();
      return -ERR_GENERIC;
    } else if (resi == -2) {
      // not running
      return -ERR_RUNNING_STATE;
    }
    return 0;
    
  default:
    return -ERR_UNKNOWN_IOCTL_COMMAND;
  }
}

static struct file_operations kp_dev_file_operations = {
  .owner            = THIS_MODULE,
  .unlocked_ioctl   = kp_dev_ioctl,
  .open             = kp_dev_open,
  .release          = kp_dev_release
};

static struct miscdevice kp_device = {
  .minor            = MISC_DYNAMIC_MINOR,
  .name             = "kpdev",
  .fops             = &kp_dev_file_operations,
  .mode             = S_IRWXUGO
};


/* Registration/deregistration functions */

int kp_device_create(void){
  return misc_register(&kp_device);
}

void kp_device_destroy(void){
  misc_deregister(&kp_device);
  return ;
}
