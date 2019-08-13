#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "../module/device.h"

/* check_print_usage(condition)
 * if condition is true,  then continue
 * if condition is false, then print usage and return;
 */
#define check_print_usage(condition)					\
  do {									\
    if (!(condition)) {							\
      printf("Usage: %s [millis [iterations]]\nmillis\t\tbase ten number of milliseconds (unsigned long)\niterations\tbase ten number of iterations\n", argv[0]); \
      return 0;								\
    }									\
  } while (0)

#define evaluate_delta( time1 , time2 )			\
  (long int)(time1.tv_usec - time2.tv_usec) +		\
  1000000 * (long int)(time1.tv_sec - time2.tv_sec)

#define scream_like_a_barbagianni( n , tmp , time1 , time2 )	\
  do {								\
    gettimeofday(&time1 , NULL);				\
    tmp = n;							\
    printf("Iterations remaining %d\tDelay = %ldus\n",		\
	   n - 1, evaluate_delta( time1 , time2 ) );		\
    for ( ; tmp > 0 ; ) printf( "[%d] " , tmp-- );			\
    printf("\n");						\
    time2 = time1 ;						\
  } while (0)

int main(int argc, char ** argv) {
  int fd, iterations;
  unsigned long timeout;
  long res;

  struct timeval t1, t2;

  // check for timeout param
  check_print_usage( argc > 1 );
  timeout = strtoul(argv[1], NULL, 10);
  check_print_usage( timeout!=0 );

  // check for iteration number
  if ( argc > 2 ) {
    iterations = strtol( argv[2], NULL, 10 );
    check_print_usage(iterations!=0);
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
  printf("Setting timer to %d\n", timeout);
  res = ioctl(fd, WR_VALUE, timeout);
  if ( res == -1 ) {
    printf("Timer not set!\n");
    return 0;
  }

  ioctl(fd, START);

  gettimeofday( &t2 , NULL );

  for (; iterations > 0; --iterations) {
    ioctl(fd, BLOCK);

    /* do some operatios */
    scream_like_a_barbagianni(iterations, res, t1, t2);
  }

  ioctl(fd, STOP);
  
  if ( close(fd) == -1 ) {
    printf("Error closing file. Error code: %d \n", errno);
  } else {
    printf("Device successfully closed\n");
  }
  
  return 0;
}
