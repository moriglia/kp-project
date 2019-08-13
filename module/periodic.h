#ifndef   __PERIODIC__
#define   __PERIODIC__

struct periodic_conf;

void wait_for_timeout(struct periodic_conf *);
int create_thread(struct periodic_conf *);
void delete_helper_thread(struct periodic_conf *);
void * create_periodic_conf(void);
void delete_periodic_conf(struct periodic_conf *);
int start_cycling(struct periodic_conf *);
int stop_cycling(struct periodic_conf *);
int periodic_conf_set_timeout_ms(struct periodic_conf *,
				 unsigned long);
unsigned long periodic_conf_get_timeout_ms(struct periodic_conf *);

#endif /* __PERIODIC__ */
