#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "../module/device.h"
#include "utils.h"

#define USAGE_MESSAGE "Usage: %s [millis [iterations]]\nmillis\t\tbase ten number of milliseconds (unsigned long)\niterations\tbase ten number of iterations\n"

int main(int argc, char ** argv) {
  int fd, iterations, tmp;
  unsigned long timeout;
  long res;

  struct timeval t1, t2;

  // check for timeout param
  check_print_usage( argc > 1 , USAGE_MESSAGE);
  timeout = strtoul(argv[1], NULL, 10);
  check_print_usage( timeout!=0 , USAGE_MESSAGE);

  // check for iteration number
  if ( argc > 2 ) {
    iterations = strtol( argv[2], NULL, 10 );
    check_print_usage(iterations!=0, USAGE_MESSAGE);
  } else {
    iterations = 10;
  }
  printf("Iteration count: %d\n", iterations);

  // open device
  fd = open("/dev/kpdev", O_RDWR);
  if (fd <= 0) {
    perror("Error opening device");
    return 1;
  }

  // setting timer:
  printf("Setting timer to %ld\n", timeout);
  res = ioctl(fd, TIMEOUT_SET, timeout);
  if ( res == -1 ) {
    printf("Timer not set!\n");
    return 0;
  }

  ioctl(fd, START);

  gettimeofday( &t1 , NULL );

  for (; iterations > 0; --iterations) {
    ioctl(fd, BLOCK);

    /* do some operatios */
    do_something(iterations, tmp, t2, t1);
  }

  ioctl(fd, STOP);
  
  if ( close(fd) == -1 ) {
    printf("Error closing file. Error code: %d \n", errno);
  } else {
    printf("Device successfully closed\n");
  }
  
  return 0;
}
