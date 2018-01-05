#include "work.h"
#include "init.h"
#include "firestarter_global.h"
#include <stdlib.h>

int main()
{
	int function = FUNC_SNB_XEONEP_AVX_1T;
	int numthreads = 8;
	struct mydata *dp = (struct mydata *) calloc(numthreads, sizeof(struct mydata));
	init_threads(dp, function, numthreads);
	set_workload(asm_work_snb_xeonep_avx_1t);
	send_signal(SIGUSR1);
	sleep(5);
	send_signal(SIGUSR2);
	sleep(5);
	send_signal(SIGTERM);
	return 0;
};
