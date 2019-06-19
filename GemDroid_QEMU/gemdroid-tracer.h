#ifndef _GEMDROID_TRACER_HEADER_H
#define _GEMDROID_TRACER_HEADER_H
typedef enum MEM_REQ_ORIGIN_t {
	MEM_REQ_NOTHING = 9,//
	MEM_REQ_ARM_HELPER,//2
	MEM_REQ_PRINTABLE,//5
	MEM_REQ_INVALID,//6

	MEM_REQ_EXEC,
	MEM_REQ_KVM,//1

	//temporarily invaliding.. because of lack of understanding on how to get env at pci
	MEM_REQ_PCI_HW,//3
	MEM_REQ_NET, //4

	MEM_REQ_GDB,//
	MEM_REQ_CODE,//
	MEM_REQ_DISAS,//

	MEM_REQ_SEMI,//
	MEM_REQ_SOFT_MMU,//
	MEM_REQ_VMEM,//
	//second part
	MEM_REQ_CONTEXT_SWITCH,//
	MEM_REQ_TLB,//
	MEM_REQ_TRANSLATE,//
	MEM_REQ_DIRTY_BITMAP,//
	OTHER_TARGET_NOT_IMPLEMENTED//15
} MEM_REQ_ORIGIN;

/*
ORIG:
typedef enum MEM_REQ_ORIGIN_t {
	MEM_REQ_INVALID = 0,
	MEM_REQ_GDB,//1
	MEM_REQ_CODE,//2
	MEM_REQ_DISAS,//3
	MEM_REQ_EXEC,//4
	MEM_REQ_SEMI,//5
	MEM_REQ_SOFT_MMU,//6
	MEM_REQ_VMEM,//7
	MEM_REQ_KVM,
	//second part
	MEM_REQ_CONTEXT_SWITCH,//8
	MEM_REQ_TLB,//9
	MEM_REQ_ARM_HELPER,//10
	MEM_REQ_TRANSLATE,//11
	MEM_REQ_DIRTY_BITMAP,//12
	MEM_REQ_PCI_HW,//13
	MEM_REQ_NET, //14
	OTHER_TARGET_NOT_IMPLEMENTED//15
} MEM_REQ_ORIGIN;
*/
#include "qemu-common.h"

/*#ifdef QEMU_ANDROID_QEMULATOR_H
  #define EXTERN
#else
  #define EXTERN extern
#endif*/

#define READ_ACCESS 0
#define WRITE_ACCESS 1

#define DRAM_latency 0

extern int IP_tracer;
extern int MMU_tracer;
extern int CPU_tracer;
extern int ICOUNT_tracer;
extern int twoAts;//prints from softmmu
extern int threeAts;//prints from target-arm/translate
extern int twoCarets;//prints from tcg-target

extern int notInsLoad, notPageTableLookup, _365_softmmu;
extern int current_pid, needed_pid, garb_pid;
extern int arm_helper_tid_now;
extern int needed_tid_length;
extern int needed_tids[1000];
extern int print_tids[1000];
//#define NEEDED_PROC_NAME "pras: tracked: "
//#define NEEDED_PROC_NAME_LOGCAT_FLAG 1

#define NEEDED_PROC_NAME "beeready"
#define NEEDED_PROC_NAME_LOGCAT_FLAG 0

//extern char *needed_proc_name, *garbage;
extern char pid_string[10], garb_string[10];
extern char current_pid_path[4096];

//Counters
extern unsigned long mmu1_ld;
extern unsigned long mmu2_ld;
extern unsigned long mmu1_st;
extern unsigned long mmu2_st;
extern unsigned long IO_ld;
extern unsigned long IO_st;

extern unsigned long cache_hits;
extern unsigned long cache_misses;
extern unsigned long this_phase_l1_miss;
extern unsigned long this_phase_l1_access;
extern unsigned long this_phase_l2_miss;

extern unsigned long long total_insts;
extern unsigned long long new_total_insts;
extern unsigned long long cpu_cycles;

extern int64_t curr_gemdroid_tick;

extern int timer_print_flag;
extern int cpu_inst_print_flag;
extern int64_t last_ticks;
extern int first_flag;
extern int miss_status;

extern int init_cache();
extern int check_miss(uint32_t addr, int size, int read_or_write);


#endif
