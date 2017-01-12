/*
 * process_server_macro.h
 *
 *  Created on: Jun 13, 2014
 *      Author: marina
 */

#ifndef PROCESS_SERVER_MACRO_H_
#define PROCESS_SERVER_MACRO_H_

enum {
	PROCESS_SERVER_CLONE_SUCCESS = 0,
	PROCESS_SERVER_CLONE_FAIL,
};

enum {
	REPLICATION_STATUS_NOT_REPLICATED = 0,
	REPLICATION_STATUS_WRITTEN,
	REPLICATION_STATUS_INVALID,
	REPLICATION_STATUS_VALID,
};

enum {
	VMA_OP_NOP = 0,
	VMA_OP_UNMAP,
	VMA_OP_PROTECT,
	VMA_OP_REMAP,
	VMA_OP_MAP,
	VMA_OP_BRK,
	VMA_OP_MADVISE,
	VMA_OP_SAVE = -70,
	VMA_OP_NOT_SAVE = -71,
};


enum {
	EXIT_ALIVE = 0,
	EXIT_THREAD,
	EXIT_PROCESS,
	EXIT_FLUSHING,
	EXIT_NOT_ACTIVE,
};

#define NOT_REPLICATED_VMA_MANAGEMENT 1


//#define MAX_KERNEL_IDS NR_CPUS
#define MAX_KERNEL_IDS 2

/**
 * Use the preprocessor to turn off printk.
 */
#define MIGRATION_PROFILE	0
#define PROCESS_SERVER_VERBOSE  1
#define PROCESS_SERVER_VMA_VERBOSE 0
#define PROCESS_SERVER_NEW_THREAD_VERBOSE 1
#define PROCESS_SERVER_MINIMAL_PGF_VERBOSE 0

#define READ_PAGE 0
#define PAGE_ADDR 0

#define CHECKSUM 0
#define STATISTICS 0

#if PROCESS_SERVER_VERBOSE
#define PSPRINTK(...) printk(__VA_ARGS__)
#undef STATISTICS
#define STATISTICS 1
#else
#define PSPRINTK(...) ;
#endif

#if PROCESS_SERVER_VMA_VERBOSE
#define PSVMAPRINTK(...) printk(__VA_ARGS__)
#else
#define PSVMAPRINTK(...) ;
#endif

#if PROCESS_SERVER_NEW_THREAD_VERBOSE
#define PSNEWTHREADPRINTK(...) printk(__VA_ARGS__)
#else
#define PSNEWTHREADPRINTK(...) ;
#endif

#if PROCESS_SERVER_MINIMAL_PGF_VERBOSE
#define PSMINPRINTK(...) printk(__VA_ARGS__)
#else
#define PSMINPRINTK(...) ;
#endif


#define PAD_LONG_MESSAGE(x) (((x-(x%PCN_KMSG_PAYLOAD_SIZE))/PCN_KMSG_PAYLOAD_SIZE)*PCN_KMSG_PAYLOAD_SIZE)+PCN_KMSG_PAYLOAD_SIZE

#define RATE_DO_REMOTE_OPERATION 5000000

#define MIGRATE_FPU 0

//#define MAX_KERNEL_IDS NR_CPUS
#define MAX_KERNEL_IDS 2

#define VMA_OP_SAVE -70
#define VMA_OP_NOT_SAVE -71


#endif /* PROCESS_SERVER_MACRO_H_ */
