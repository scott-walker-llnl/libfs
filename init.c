#include "work.h"

int IDLE 0;

mydata_t *init_globals()
{
    mydata_t *globals = (mydata_t *) _mm_malloc(sizeof(mydata_t), ALIGNMENT);
    if (mdp == 0) {
        fprintf(stderr,"Error: Allocation of structure mydata_t failed (1)\n");
        fflush(stderr);
		return NULL;
    }
    memset(mdp, 0, sizeof(mydata_t));
	return globals;
}

void thread_idle()
{
	while (IDLE)
	{
		sleep(0.1);
	}
}

void signal_workload(int signum)
{
	if (signum != SIGUSR1)
	{
		fprintf(stderr, "child received wrong signal!\n");
		return;
	}
	IDLE = 0;

}

void signal_idle(int signum)
{
	if (signum != SIGUSR2)
	{
		fprintf(stderr, "child received wrong signal!\n");
		return;
	}
	IDLE =  1;
	thread_idle();
}

int thread_mem_alloc(mydata_t *mdp, int threadno)
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
	thread_idle();
}

int init_threads(mydata_t *mdp, int function, int numthreads)
{
    mdp->cpuinfo = cpuinfo;

    if((numthreads > mdp->cpuinfo->num_cpus) || (numthreads == 0)){
        numthreads = mdp->cpuinfo->num_cpus;
    }

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
	memset(&worload_action, 0, sizeof(struct sigaction));
	workload_action.sa_handler = signal_workload;
	sigaction(SIGUSR1, &workload_action, NULL);

	struct sigaction idle_action;
	memset(&idle_action, 0, sizeof(struct sigaction));
	workload_action.sa_handler = signal_idle;	
	sigaction(SIGUSR2, &idle_action, NULL);

	sigset_t sset;
	sigemptyset(&sset);
	sigaddset(&sset, SIGUSR1);
	sigaddset(&sset, SIGUSR2);
	int hstat = pthread_sigmask(SIG_UNBLOCK, &sset, NULL);

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

	BUFFERSIZEMEM = sizeof(char) * (2 * BUFFERSIZE[0] + BUFFERSIZE[1] + BUFFERSIZE[2] + RAMBUFFERSIZE + ALIGNMENT + 2 * sizeof(unsigned long long));

    if(BUFFERSIZEMEM <= 0){
        fprintf(stderr, "Error: Determine BUFFERSIZEMEM failed\n");
        fflush(stderr);
		return FS_THREAD_INIT;
    }

    for (t = 0; t < numthreads; t++) {
        mdp->ack = 0;
        mdp->threaddata[t].thread_id = t;
        mdp->threaddata[t].cpu_id = cpu_bind[t];
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
        mdp->thread_comm[t] = THREAD_INIT;

		threads[t] = pthread_create(&(threads[t]), NULL, thread_mem_alloc,(void *) (&(mdp->threaddata[t])));
        while (!mdp->ack); // wait for this thread's memory allocation
        if (mdp->ack == FS_THREAD_INIT) {
            fprintf(stderr,"Error: Initialization of threads failed\n");
            fflush(stderr);
			return FS_THREAD_INIT;
        }
    }
    mdp->ack = 0;

	// 0 returns OK
	return FS_OK;
}
