#define _XOPEN_SOURCE
#include "init.h"

#define LOAD_HIGH 1

// TODO: there is a lot of global data being thrown around. Separate out Firestarter's.
int IDLE = 0;
int (*workload)(threaddata_t *);
int (*sampling)(int) = NULL;
mydata_t *mdp_global;
int NUMTHREADS = 0;
unsigned long long *thread_table;
void *sample_arg;
pthread_t *threads;
int LOADVAR = 0;

// get this threads TID
fs_error_t me()
{
	if (thread_table == NULL)
	{
		return FS_THREAD_INIT;
	}
	// thread_table stores the thread descriptor for each thread. A thread can get it's own thread descriptor with
	// pthread_self. Essentially, this works like MPI_COMM_RANK.
	int i;
	for (i = 0; i < NUMTHREADS; i++)
	{
		if (thread_table[i] == pthread_self())
		{
			return i;
		}
	}
	return -1;
}

// TODO: this does not really do much right now but eventually it should log a thread's error for reporting.
static void log_error(fs_error_t error)
{
	fprintf(stderr, "LIBFS ERROR: %d\n", error);
	int i;
	for (i = 0; i < NUMTHREADS; i++)
	{
	}
}

// This sets the workload function pointer. This will be called from the user application.
int set_workload(int (*load)(threaddata_t *))
{
	if (load != NULL)
	{
		workload = load;
		return FS_OK;
	}
	return FS_WORK;
}

// This sets the sampling function pointer. This will be called from the user application.
// The idea is that the user can write their own data collection function and it will
// automatically be called by the API to collect data.
int set_sampling(int (*sampler)(int))
{
	if (sampler != NULL)
	{
		sampling = sampler;
		return FS_OK;
	}
	return FS_SAMPLE;
}

// When this function is called the program will be placed in an idle state.
static void thread_idle()
{
	while (IDLE)
	{
		sleep(0.1);
	}
}

// This executes the work payload currently defined in the workload function pointer.
static void thread_work()
{
	if (workload == NULL)
	{
		fprintf(stderr, "LIBFS ERROR: no workload specified\n");
		log_error(FS_WORK);
	}
	int tid = me();
	while (!IDLE)
	{
		if (workload(&mdp_global->threaddata[tid]) != EXIT_SUCCESS)
		{
			log_error(FS_WORK);
		}
	}
}

// Send an approved signal to each of the threads.
// SIGUSR1: switch to work
// SIGUSR2: switch to idle
// SIGTERM: terminate thread execution
void send_signal(int signum)
{
	if (signum == SIGUSR1 || signum == SIGUSR2 || signum == SIGTERM || signum == SIGPOLL)
	{
		int i;
		for (i = 0; i < NUMTHREADS; i++)
		{
			pthread_kill(thread_table[i], signum);
		}
		return;
	}
	printf("bad signal %d\n", signum);
}

// Signal handler for the threads.
void signal_workload(int signum)
{
	// These threads are all writing LOADVAR and IDLE but there are no race conditions since
	// every thread is writing the same value.
	// Still, this should be made less hackey eventually.
	switch (signum)
	{
		case SIGUSR1:
			printf("thread %d switching to work\n", me());
			LOADVAR = 1;
			IDLE = 0;
			return;
		case SIGUSR2:
			printf("thread %d switching to idle\n", me());
			LOADVAR = 1;
			IDLE = 1;
			return;
		case SIGTERM:
			printf("thread %d terminating\n", me());
			IDLE = -1;
			return;
		case SIGPOLL:
			printf("thread %d sampling\n", me());
			if (sampling != NULL)
			{
				sampling(me());
			}
			return;
	}
	fprintf(stderr, "bad signal\n");
}

// TODO: this is dead code
void signal_idle(int signum)
{
	if (signum != SIGUSR2)
	{
		fprintf(stderr, "child received wrong signal!\n");
		return;
	}
	IDLE =  1;
	//return thread_idle();
}

// Allocate the memory buffer used by firestarter for each thread.
static int thread_mem_alloc(mydata_t *mdp, int threadno)
{
	/* allocate memory */
	int t = threadno;
	if(mdp->threaddata[t].buffersizeMem){
		mdp->threaddata[t].bufferMem = _mm_malloc(mdp->threaddata[t].buffersizeMem, 
			mdp->threaddata[t].alignment);
		mdp->threaddata[t].addrMem = (unsigned long long)(mdp->threaddata[t].bufferMem);
		if (mdp->threaddata[t].bufferMem == NULL)
		{
			mdp->ack = FS_THREAD_INIT;
			return FS_THREAD_INIT;
		}
		mdp->ack = 1;
	}
	IDLE = 1;
	//thread_idle();
	return FS_OK;
}

// The main work loop of the threads.
static void *work(void *data)
{
	thread_table[((threaddata_t *) data)->thread_id] = pthread_self();
	thread_mem_alloc(mdp_global, me());
	while(IDLE >= 0)
	{
		if (IDLE)
		{
			thread_idle();
		}
		else
		{
			thread_work();
		}
	}
	return NULL;
}

// Initialization for this API and firestarter
int init_threads(mydata_t *mdp, int function, int numthreads)
{
	thread_table = (unsigned long long *) malloc(numthreads * sizeof(unsigned long long));
	NUMTHREADS = numthreads;
	memset(thread_table, -1, sizeof(unsigned long long) * numthreads);
	mdp_global = mdp;
	// TODO this is dead code
    //mdp->cpuinfo = cpuinfo;

    //if((numthreads > mdp->cpuinfo->num_cpus) || (numthreads == 0)){
    //   numthreads = mdp->cpuinfo->num_cpus;
    //}
	LOADVAR = LOAD_HIGH;

    threads = _mm_malloc(numthreads * sizeof(pthread_t), ALIGNMENT);
    mdp->thread_comm = _mm_malloc(numthreads * sizeof(int), ALIGNMENT);
    mdp->threaddata = _mm_malloc(numthreads * sizeof(threaddata_t), ALIGNMENT);
    mdp->num_threads = numthreads;
    if((threads == NULL) || (mdp->thread_comm == NULL) || (mdp->threaddata == NULL)){
        fprintf(stderr, "Error: Allocation of structure mydata_t failed\n");
        fflush(stderr);
		return FS_NO_MEM;
    }

	struct sigaction workload_action;
	memset(&workload_action, 0, sizeof(struct sigaction));
	workload_action.sa_handler = signal_workload;
	sigaction(SIGUSR1, &workload_action, NULL);
	sigaction(SIGUSR2, &workload_action, NULL);
	sigaction(SIGTERM, &workload_action, NULL);

	// TODO: this is dead code
/*
	struct sigaction idle_action;
	memset(&idle_action, 0, sizeof(struct sigaction));
	workload_action.sa_handler = signal_idle;	
	sigaction(SIGUSR2, &idle_action, NULL);
*/

	sigset_t sset;
	sigemptyset(&sset);
	sigaddset(&sset, SIGUSR1);
	sigaddset(&sset, SIGUSR2);
	sigaddset(&sset, SIGTERM);
	pthread_sigmask(SIG_UNBLOCK, &sset, NULL);

	// Firestarter buffer size calculation
	unsigned int BUFFERSIZE[3];
	unsigned long long RAMBUFFERSIZE;
	int verbose = 0;
	int i;

    switch (function) {
    case FUNC_KNL_XEONPHI_AVX512_4T:
        BUFFERSIZE[0] = 8192;
        BUFFERSIZE[1] = 131072;
        BUFFERSIZE[2] = 59069781;
        RAMBUFFERSIZE = 6553600;
        if (verbose) {
            printf("\n  Taking AVX512 path optimized for Knights_Landing - 4 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_SKL_COREI_FMA_1T:
        BUFFERSIZE[0] = 32768;
        BUFFERSIZE[1] = 262144;
        BUFFERSIZE[2] = 1572864;
        RAMBUFFERSIZE = 104857600;
        if (verbose) {
            printf("\n  Taking FMA path optimized for Skylake - 1 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_SKL_COREI_FMA_2T:
        BUFFERSIZE[0] = 16384;
        BUFFERSIZE[1] = 131072;
        BUFFERSIZE[2] = 786432;
        RAMBUFFERSIZE = 52428800;
        if (verbose) {
            printf("\n  Taking FMA path optimized for Skylake - 2 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_HSW_COREI_FMA_1T:
        BUFFERSIZE[0] = 32768;
        BUFFERSIZE[1] = 262144;
        BUFFERSIZE[2] = 1572864;
        RAMBUFFERSIZE = 104857600;
        if (verbose) {
            printf("\n  Taking FMA path optimized for Haswell - 1 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_HSW_COREI_FMA_2T:
        BUFFERSIZE[0] = 16384;
        BUFFERSIZE[1] = 131072;
        BUFFERSIZE[2] = 786432;
        RAMBUFFERSIZE = 52428800;
        if (verbose) {
            printf("\n  Taking FMA path optimized for Haswell - 2 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_HSW_XEONEP_FMA_1T:
        BUFFERSIZE[0] = 32768;
        BUFFERSIZE[1] = 262144;
        BUFFERSIZE[2] = 2621440;
        RAMBUFFERSIZE = 104857600;
        if (verbose) {
            printf("\n  Taking FMA path optimized for Haswell-EP - 1 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_HSW_XEONEP_FMA_2T:
        BUFFERSIZE[0] = 16384;
        BUFFERSIZE[1] = 131072;
        BUFFERSIZE[2] = 1310720;
        RAMBUFFERSIZE = 52428800;
        if (verbose) {
            printf("\n  Taking FMA path optimized for Haswell-EP - 2 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_SNB_COREI_AVX_1T:
        BUFFERSIZE[0] = 32768;
        BUFFERSIZE[1] = 262144;
        BUFFERSIZE[2] = 1572864;
        RAMBUFFERSIZE = 104857600;
        if (verbose) {
            printf("\n  Taking AVX path optimized for Sandy Bridge - 1 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_SNB_COREI_AVX_2T:
        BUFFERSIZE[0] = 16384;
        BUFFERSIZE[1] = 131072;
        BUFFERSIZE[2] = 786432;
        RAMBUFFERSIZE = 52428800;
        if (verbose) {
            printf("\n  Taking AVX path optimized for Sandy Bridge - 2 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_SNB_XEONEP_AVX_1T:
        BUFFERSIZE[0] = 32768;
        BUFFERSIZE[1] = 262144;
        BUFFERSIZE[2] = 2621440;
        RAMBUFFERSIZE = 104857600;
        if (verbose) {
            printf("\n  Taking AVX path optimized for Sandy Bridge-EP - 1 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_SNB_XEONEP_AVX_2T:
        BUFFERSIZE[0] = 16384;
        BUFFERSIZE[1] = 131072;
        BUFFERSIZE[2] = 1310720;
        RAMBUFFERSIZE = 52428800;
        if (verbose) {
            printf("\n  Taking AVX path optimized for Sandy Bridge-EP - 2 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_NHM_COREI_SSE2_1T:
        BUFFERSIZE[0] = 32768;
        BUFFERSIZE[1] = 262144;
        BUFFERSIZE[2] = 1572864;
        RAMBUFFERSIZE = 104857600;
        if (verbose) {
            printf("\n  Taking SSE2 path optimized for Nehalem - 1 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_NHM_COREI_SSE2_2T:
        BUFFERSIZE[0] = 16384;
        BUFFERSIZE[1] = 131072;
        BUFFERSIZE[2] = 786432;
        RAMBUFFERSIZE = 52428800;
        if (verbose) {
            printf("\n  Taking SSE2 path optimized for Nehalem - 2 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_NHM_XEONEP_SSE2_1T:
        BUFFERSIZE[0] = 32768;
        BUFFERSIZE[1] = 262144;
        BUFFERSIZE[2] = 2097152;
        RAMBUFFERSIZE = 104857600;
        if (verbose) {
            printf("\n  Taking SSE2 path optimized for Nehalem-EP - 1 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_NHM_XEONEP_SSE2_2T:
        BUFFERSIZE[0] = 16384;
        BUFFERSIZE[1] = 131072;
        BUFFERSIZE[2] = 1048576;
        RAMBUFFERSIZE = 52428800;
        if (verbose) {
            printf("\n  Taking SSE2 path optimized for Nehalem-EP - 2 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
    case FUNC_BLD_OPTERON_FMA4_1T:
        BUFFERSIZE[0] = 16384;
        BUFFERSIZE[1] = 1048576;
        BUFFERSIZE[2] = 786432;
        RAMBUFFERSIZE = 104857600;
        if (verbose) {
            printf("\n  Taking FMA4 path optimized for Bulldozer - 1 thread(s) per core");
            printf("\n  Used buffersizes per thread:\n");
            for (i = 0; i < MAX_CACHELEVELS; i++) if (BUFFERSIZE[i] > 0) printf("    - L%d-Cache: %d Bytes\n", i + 1, BUFFERSIZE[i]);
            printf("    - Memory: %llu Bytes\n\n", RAMBUFFERSIZE);
        }
        break;
      default:
        fprintf(stderr, "Internal Error: missing code-path %i!\n",function);
		return FS_BAD_ARCH;
    }

	unsigned long long BUFFERSIZEMEM = sizeof(char) * (2 * BUFFERSIZE[0] + BUFFERSIZE[1] + BUFFERSIZE[2] + RAMBUFFERSIZE + ALIGNMENT + 2 * sizeof(unsigned long long));

    if(BUFFERSIZEMEM <= 0){
        fprintf(stderr, "Error: Determine BUFFERSIZEMEM failed\n");
        fflush(stderr);
		return FS_THREAD_INIT;
    }
	printf("buffersizemem %lld\n", BUFFERSIZEMEM);

	// initialize each of the threads to the idle state
	int t;
    for (t = 0; t < numthreads; t++) {
        mdp->ack = 0;
        mdp->threaddata[t].thread_id = t;
        mdp->threaddata[t].cpu_id = t; //cpu_bind[t];
        mdp->threaddata[t].data = mdp;
        mdp->threaddata[t].buffersizeMem = BUFFERSIZEMEM;
        mdp->threaddata[t].iterations = 0;
        mdp->threaddata[t].flops = 0;
        mdp->threaddata[t].bytes = 0;
        mdp->threaddata[t].alignment = ALIGNMENT;
        mdp->threaddata[t].FUNCTION = function;
        mdp->threaddata[t].period = PERIOD;
        mdp->threaddata[t].iter = 0;
        mdp->threaddata[t].msrdata = NULL;
        mdp->threaddata[t].addrHigh = (unsigned long long) &LOADVAR;
        mdp->thread_comm[t] = THREAD_INIT;

		threads[t] = pthread_create(&(threads[t]), NULL, work,(void *) (&(mdp->threaddata[t])));
		//thread_table[t] = threads[t];
        while (!mdp->ack); // wait for this thread's memory allocation
        if (mdp->ack == FS_THREAD_INIT) {
            fprintf(stderr,"Error: Initialization of threads failed\n");
            fflush(stderr);
			return FS_THREAD_INIT;
        }
    }
    mdp->ack = 0;

	return FS_OK;
}
