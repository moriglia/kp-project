#ifndef   __PERIODIC__
#define   __PERIODIC__

struct periodic_conf_list;

int wait_for_timeout(struct periodic_conf_list *);
void * create_periodic_conf_list(void);
void delete_periodic_conf_list(struct periodic_conf_list *);
int start_cycling(struct periodic_conf_list *);
int stop_cycling(struct periodic_conf_list *);
int periodic_conf_set_timeout_ms(struct periodic_conf_list *,
				 unsigned long);
unsigned long periodic_conf_get_timeout_ms(struct periodic_conf_list *);

#endif /* __PERIODIC__ */
