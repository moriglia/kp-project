#ifndef   __KP_DEVICE__
#define   __KP_DEVICE__

#include <linux/ioctl.h>

#define WR_VALUE _IOW('k', 's', unsigned long )
#define RD_VALUE _IOR('k', 'g', void* )
#define BLOCK    _IO ('k', 'b')

/* Error codes */
#define ERR_NO_KMALLOC                     1
#define ERR_TIMEOUT_NOT_SET                2
#define ERR_UNKNOWN_IOCTL_COMMAND          3

/* Registration/deregistration functions */
int  kp_device_create  (void);
void kp_device_destroy (void);


#endif /* __KP_DEVICE__*/
