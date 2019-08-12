#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "../module/device.h"


int main(void){
  int fd;
  unsigned long timeout;
  long retval;

  struct timeval before, after;

  fd = open("/dev/kpdev", O_RDWR);
  if (fd < 0){
    retval = errno;
    perror("Error opening device");
    return 0;
  }

  timeout = 10UL;
  retval = ioctl(fd, WR_VALUE, timeout);
  printf("Return value of ioctl() call with set is %d\n", retval);

  retval = ioctl(fd, RD_VALUE, NULL);
  printf("Return value of ioctl() call with get is %d\n", retval);

  retval = ioctl(fd, START);
  printf("Started\n");

  gettimeofday( &before, NULL);
  retval = ioctl( fd, BLOCK);
  gettimeofday( &after, NULL);
  printf( "Delay 0: %ld us\n", (long int)(after.tv_usec - before.tv_usec) + (long int)(after.tv_sec - before.tv_sec)*1000000);
  printf( "Return value of ioctl() call with block is %d\n", retval);
  printf( "Doing some elaboration: 5+3 is %d\n", 5+3 );

  before = after;
  retval = ioctl( fd, BLOCK);
  gettimeofday( &after, NULL);
  printf( "Delay 1: %ld us\n", (long int)(after.tv_usec - before.tv_usec) + (long int)(after.tv_sec - before.tv_sec)*1000000);
  printf("More elaboration...\n");

  before = after;
  retval = ioctl( fd, BLOCK);
  gettimeofday( &after, NULL);
  printf( "Delay 2: %ld us\n", (long int)(after.tv_usec - before.tv_usec) + (long int)(after.tv_sec - before.tv_sec)*1000000);
  printf("More elaboration...\n");

  before = after;
  retval = ioctl( fd, BLOCK);
  gettimeofday( &after, NULL);
  printf( "Delay 3: %ld us\n", (long int)(after.tv_usec - before.tv_usec) + (long int)(after.tv_sec - before.tv_sec)*1000000);
  printf("More elaboration...\n");

  before = after;
  retval = ioctl( fd, BLOCK);
  gettimeofday( &after, NULL);
  printf( "Delay 4: %ld us\n", (long int)(after.tv_usec - before.tv_usec) + (long int)(after.tv_sec - before.tv_sec)*1000000);
  printf("More elaboration...\n");

  before = after;
  retval = ioctl( fd, BLOCK);
  gettimeofday( &after, NULL);
  printf( "Delay 5: %ld us\n", (long int)(after.tv_usec - before.tv_usec) + (long int)(after.tv_sec - before.tv_sec)*1000000);

  retval = ioctl( fd, STOP );
  printf("Retval of STOP is %d\n", retval);
  
  

  if ( close(fd) == -1 ) {
    printf("Error while closing file. Error code is %d\n", errno);
  } else {
    printf("Device successfully closed\n");
  }
  
  return 0;
}

