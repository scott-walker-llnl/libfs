/******************************************************************************
 * FIRESTARTER - A Processor Stress Test Utility
 * Copyright (C) 2017 TU Dresden, Center for Information Services and High
 * Performance Computing
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: daniel.hackenberg@tu-dresden.de
 *****************************************************************************/

#ifndef __FIRESTARTER__GLOBAL__H
#define __FIRESTARTER__GLOBAL__H

/* current version */
#define VERSION_MAJOR  1
#define VERSION_MINOR  6
#define VERSION_INFO   "LIBFS" //additional information, e.g. "BETA"
#define BUILDDATE      "2017-12-14"
#define COPYRIGHT_YEAR 2017

#if (defined(linux) || defined(__linux__)) && defined (AFFINITY)
#define _GNU_SOURCE
#include <sched.h>
#endif

#ifdef ENABLE_VTRACING
#include <vt_user.h>
#endif
#ifdef ENABLE_SCOREP
#include <SCOREP_User.h>
#endif

#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
//#include "cpu.h"

#define THREAD_WAIT        1
#define THREAD_WORK        2
#define THREAD_INIT        3
#define THREAD_STOP        4
#define THREAD_INIT_FAILURE 0xffffffff

#define FUNC_NOT_DEFINED   0
#define FUNC_UNKNOWN      -1

/* load levels */
#define LOAD_LOW           0
#define LOAD_HIGH          1 /* DO NOT CHANGE! the asm load-loop continues until the load-variable is != 1 */
#define LOAD_STOP          2

#define INIT_BLOCKSIZE  8192

typedef enum fs_error_t
{
	FS_OK = 0,
	FS_WORK = -1,
	FS_SAMPLE = -2,
	FS_PLACEHOLDER3 = -3,
	FS_PLACEHOLDER4 = -4,
	FS_PLACEHOLDER5 = -5,
	FS_PLACEHOLDER6 = -6,
	FS_PLACEHOLDER7 = -7,
	FS_PLACEHOLDER8 = -8,
	FS_PLACEHOLDER9 = -9,
	FS_PLACEHOLDER10 = -10,
	FS_PLACEHOLDER11 = -11,
	FS_NO_MEM = -ENOMEM,
	FS_BAD_ARCH = -125,
	FS_THREAD_INIT = -126
} fs_error_t;

/*
 * watchdog timer
 */
/*
typedef struct watchdog_args {
    unsigned long long *loadvar;
    pid_t pid;
    useconds_t period;
    useconds_t load;
    unsigned int timeout;
} watchdog_arg_t;
watchdog_arg_t watchdog_arg;
*/

/** The data structure that holds all the global data.
 */
typedef struct mydata
{                                   
   struct threaddata *threaddata;           
   //cpu_info_t *cpuinfo;
   int *thread_comm;
   volatile unsigned int ack;   
   unsigned int num_threads;
} mydata_t;

typedef struct msrdata
{
	uint64_t tsc;
	uint64_t aperf;
	uint64_t mperf;
	uint64_t retired;
	uint64_t pmc0;
	uint64_t pmc1;
	uint64_t pmc2;
	uint64_t pmc3;
	uint64_t log;
	double   watts;
} msrdata_t;

/* data needed by each thread */
typedef struct threaddata
{
   volatile mydata_t *data;                                 
   char* bufferMem;                 
   unsigned long long addrMem;      
   unsigned long long addrHigh;             
   unsigned long long buffersizeMem;
   unsigned long long iterations;
   unsigned long long flops;
   unsigned long long bytes;
   unsigned long long start_tsc;
   unsigned long long stop_tsc;
   unsigned int alignment;      
   unsigned int cpu_id;
   unsigned int thread_id;
   unsigned int package;
   unsigned int period;                     
   unsigned char FUNCTION;
   unsigned long iter;
   msrdata_t *msrdata;
} threaddata_t;

#endif
