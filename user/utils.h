#ifndef __TEST_UTILS__
#define __TEST_UTILS__
#include <sys/time.h>

/* check_print_usage(condition, usage_message)
 * if condition is true,  then continue
 * if condition is false, then print usage and return;
 *
 * usage_message must contain exactly one "%s"
 */
#define check_print_usage(condition, usage_message)	\
  do {							\
    if (!(condition)) {					\
      printf(usage_message, argv[0]); 			\
      return 0;						\
    }							\
  } while (0)

#define evaluate_delta( time2 , time1 )			\
  (long int)(time2.tv_usec - time1.tv_usec) +		\
  1000000 * (long int)(time2.tv_sec - time1.tv_sec)

#define do_something_msg( n, tmp, time2, time1, msg )	\
  do {							\
    gettimeofday(&time2 , NULL);			\
    tmp = n;						\
    printf("[%s] %dmore iterations\tDelay = %ldus\n",	\
	   msg, n-1, evaluate_delta( time2 , time1 ) );	\
    for ( ; tmp > 0 ; ) printf( "[%d] " , tmp-- );	\
    printf("\n");					\
    time1 = time2 ;					\
  } while (0)

#define do_something(n, tmp, time2, time1)	\
  do_something_msg( n, tmp, time2, time1, "")	

#endif /*__TEST_UTILS__*/
