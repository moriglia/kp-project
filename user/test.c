#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "../module/device.h"


int main(void){
  int fd;
  unsigned long timeout;
  long retval;

  fd = open("/dev/kpdev", O_RDWR);
  if (fd < 0){
    retval = errno;
    perror("Error opening device");
    return 0;
  }

  timeout = 5000UL;
  retval = ioctl(fd, WR_VALUE, timeout);
  printf("Return value of ioctl() call with set is %d\n", retval);

  retval = ioctl(fd, RD_VALUE, NULL);
  printf("Return value of ioctl() call with get is %d\n", retval);

  if ( close(fd) == -1 ) {
    printf("Error while closing file. Error code is %d\n", errno);
  } else {
    printf("Device successfully closed\n");
  }
  
  return 0;
}
  
