#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include "utils.h"
#include "../module/device.h"

#define NUM_THREADS 15

struct args {
  int id;
  unsigned long timeout_ms;
};

void * work(void * arguments){
  struct args * args_ptr;
  int res, fd, iterations;
  struct timeval t1, t2;
  char msg[20];

  args_ptr = (struct args*)arguments;

  fd = open("/dev/kpdev", O_RDWR);
  if ( fd < 0 ){
    printf("[%d] Cannot open device\n", args_ptr->id);
    return NULL;
  }

  res = ioctl(fd, TIMEOUT_SET, args_ptr->timeout_ms);
  if (res != 0) {
    close(fd);
    printf("[%d] Cannot set timeout\n", args_ptr->id);
    return NULL;
  }

  sprintf(msg, "Thread\t%d", args_ptr->id);

  ioctl(fd, START);

  gettimeofday( &t1 , NULL );

  for (iterations = args_ptr->id; iterations > 0; --iterations) {
    ioctl(fd, BLOCK);

    /* do some operatios */
    do_something_msg(iterations, res, t2, t1, msg);
  }

  ioctl(fd, STOP);
  
  if ( close(fd) == -1 ) {
    printf("Error closing file. Error code: %d \n", errno);
  } else {
    printf("Device successfully closed\n");
  }

  return NULL;
}

int main(){
  pthread_t threads[NUM_THREADS];
  struct args thread_args[NUM_THREADS];
  int i;

  for (i = 0 ; i<NUM_THREADS ; ++i){
    thread_args[i].id = i;
    thread_args[i].timeout_ms = (i+1)*20;
    pthread_create(&threads[i], NULL, work, &thread_args[i]);
  }

  for (i = 0 ; i<NUM_THREADS ; ++i){
   pthread_join(threads[i], NULL); 
  }

  return 0;

}
