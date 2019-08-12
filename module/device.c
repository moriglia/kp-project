#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/stat.h>
#include "device.h"
#include "periodic.h"

//#ifndef VERBOSE
#define VERBOSE 1
//#endif



static struct miscdevice kp_device;

static int kp_dev_open(struct inode* in, struct file* f){
#if VERBOSE
  printk("Opening device");
#endif
  f->private_data = create_periodic_conf();
  if (create_thread(f->private_data)) {
    delete_periodic_conf(f->private_data);
    return -1;
  }
  return 0;
}

static int kp_dev_release(struct inode* in, struct file* f){
#if VERBOSE
  printk("Closing device");
#endif
  delete_periodic_conf(f->private_data);
  f->private_data = NULL;
  printk("Almost closed!");
  return 0;
}

static long kp_dev_ioctl(struct file* f, unsigned int cmd, unsigned long arg){
  //void *timeout;

  switch (cmd){

  case WR_VALUE:
    return periodic_conf_set_timeout_ms(f->private_data, arg);
    /*
    if (f->private_data) {
      timeout = f->private_data;
    } else {
      timeout = kmalloc(sizeof(unsigned int), GFP_USER);
      if (!timeout) {
	return -ERR_NO_KMALLOC;
      }
      f->private_data = timeout;
    }
    *((unsigned long *)timeout) = arg;
    return 0;
    */

  case RD_VALUE:
    return periodic_conf_get_timeout_ms(f->private_data);
    /*
    if (!f->private_data) {
      return -ERR_TIMEOUT_NOT_SET;
    }
    timeout = f->private_data;
    return *((unsigned long*)timeout);
    */
  case BLOCK:
    if (!f->private_data) {
      return -1 ; // handle error
    }
    wait_for_timeout(f->private_data);
    return 0;

  case START:
    start_cycling(f->private_data);
    return 0;
    
  case STOP:
    stop_cycling(f->private_data);
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
