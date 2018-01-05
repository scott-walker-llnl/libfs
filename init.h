#include "work.h"
#include <signal.h>

#define ALIGNMENT 64
#define PERIOD 0
#define MAX_CACHELEVELS 3


fs_error_t me();
int set_workload(int (*load)(threaddata_t *));
int set_sampling(int (*sampler)(int));
void signal_workload(int signum);
void signal_idle(int signum);
void send_signal(int signum);
int init_threads(mydata_t *mdp, int function, int numthreads);
