//Nachi added
#include "gemdroid-tracer.h"
//pras comments d4-7 - we can add the cache stuffs later..
//#include "d4-7/d4.h"

//int init_cache();

int notInsLoad = 0, notPageTableLookup = 1, _365_softmmu = 1;
int twoAts = 1;
int threeAts = 0;
int twoCarets = 0;
int IP_tracer = 0;
int MMU_tracer = 0;
//PRAS: int CPU_tracer = 1;
int CPU_tracer = 1;
//PRAS: original int CPU_tracer = 0;
int ICOUNT_tracer = 0;

//char *needed_proc_name = "snatik";//
//char *needed_proc_name = "pras: tracked: ";//
char pid_string[10] = "(  000: ";//this is how logcat -v thread
char garb_string[10] = "(  000: ";//this is how logcat -v thread
int needed_pid = -1, garb_pid = -1;
int needed_tids[1000];
int print_tids[1000];

int needed_tid_length = 0;
int arm_helper_tid_now = -1;
int current_pid = -1;
char current_pid_path[4096];


//Counters
unsigned long mmu1_ld = 0;
unsigned long mmu2_ld = 0;
unsigned long mmu1_st = 0;
unsigned long mmu2_st = 0;
unsigned long IO_ld = 0;
unsigned long IO_st = 0;

unsigned long cache_hits = 0;
unsigned long cache_misses = 0;
unsigned long this_phase_l1_miss = 0;
unsigned long this_phase_l1_access = 0;
unsigned long this_phase_l2_miss = 0;

unsigned long long total_insts = 0;
unsigned long long new_total_insts = 0;
unsigned long long cpu_cycles = 0;


int64_t curr_gemdroid_tick = 0;

//Should the timer be printed now or not.
int timer_print_flag = 0;
//Should the cpu_inst be printed now or not.
int cpu_inst_print_flag = 1;
int64_t last_ticks = 0;
//Initialize Cache first flag -- should the cache be initialized or not
int first_flag = 0;
int miss_status = 0;

//pras comments cache: d4cache* Mem;
//pras comments cache: d4cache* L1;
//pras comments cache: d4cache* L2;

//pras comments cache: unsigned long misses_seen_l1 = 0;
//pras comments cache: unsigned long curr_miss_l1 = 0;
//pras comments cache: unsigned long misses_seen_l2 = 0;
//pras comments cache: unsigned long curr_miss_l2 = 0;


int init_cache()
{
//pras comments cache:  Mem = d4new(0);
//pras comments cache:
//pras comments cache:  L2 = d4new(Mem);
//pras comments cache:  L2->name = "L2";
//pras comments cache:  L2->flags = D4F_CCC;
//pras comments cache:  L2->lg2blocksize = 6;    //log of block size - 2^5 = 32
//pras comments cache:  L2->lg2subblocksize = L2->lg2blocksize;
//pras comments cache:  L2->lg2size = 19;   //log of size = 2^20 (1MB cache)  2^10 bytes is 1KB.
//pras comments cache:  L2->assoc = 8;
//pras comments cache:  L2->replacementf = d4rep_lru;
//pras comments cache:  L2->prefetchf = d4prefetch_none;
//pras comments cache:  L2->wallocf = d4walloc_always;
//pras comments cache:  L2->wbackf = d4wback_always;
//pras comments cache:  L2->name_replacement = L2->name_prefetch = L2->name_walloc = L2->name_wback = "L2";
//pras comments cache:
//pras comments cache:  L1 = d4new(Mem);
//pras comments cache:  L1->name = "L1";
//pras comments cache:  L1->flags = D4F_CCC;
//pras comments cache:  L1->lg2blocksize = 6;    //log of block size - 2^5 = 32
//pras comments cache:  L1->lg2subblocksize = L1->lg2blocksize;
//pras comments cache:  L1->lg2size = 15;   //log of size = 2^20 (1MB cache)  2^10 bytes is 1KB.
//pras comments cache:  L1->assoc = 2;
//pras comments cache:  L1->replacementf = d4rep_lru;
//pras comments cache:  L1->prefetchf = d4prefetch_none;
//pras comments cache:  L1->wallocf = d4walloc_always;
//pras comments cache:  L1->wbackf = d4wback_always;
//pras comments cache:  L1->name_replacement = L1->name_prefetch = L1->name_walloc = L1->name_wback = "L1";
//pras comments cache:
//pras comments cache:  if(d4setup()!=0)
//pras comments cache:	{
//pras comments cache:		printf("\n Error setting up cache!!!\n\n\n\n\n");
//pras comments cache:	}
//pras comments cache:	else
//pras comments cache:	{
//pras comments cache:		printf("\n Successfully finished cache setup.\n");
//pras comments cache:	}
	return 0;
}

int check_miss(uint32_t addr, int size, int read_or_write)
{
	return 0;
//pras comments cache:	this_phase_l1_access++;
//pras comments cache:	d4memref R;
//pras comments cache:	R.address = (d4addr)addr;
//pras comments cache:	//printf("Address being checked : %x\n",R.address);
//pras comments cache:	R.size = size;
//pras comments cache:	if(read_or_write == READ_ACCESS)	{
//pras comments cache:		R.accesstype = D4XREAD;
//pras comments cache:	}
//pras comments cache:	else if(read_or_write == WRITE_ACCESS)	{
//pras comments cache:		R.accesstype = D4XWRITE;
//pras comments cache:	}
//pras comments cache:
//pras comments cache:	d4ref(L1, R);
//pras comments cache:
//pras comments cache:	curr_miss_l1 = L1->miss[D4XREAD] + L1->miss[D4XWRITE];
//pras comments cache:	curr_miss_l2 = L2->miss[D4XREAD] + L2->miss[D4XWRITE];
//pras comments cache:
//pras comments cache:	if((curr_miss_l1 == misses_seen_l1) && (curr_miss_l2 == misses_seen_l2)){
//pras comments cache:		return 0;
//pras comments cache:	}
//pras comments cache:	else if(curr_miss_l2 > misses_seen_l2)	{			//First check L2 miss. Because, if its a L2 miss(even an access itself), it would have definitely been a L1 miss.
//pras comments cache:		this_phase_l2_miss++;
//pras comments cache:		this_phase_l1_miss++;
//pras comments cache:
//pras comments cache:		misses_seen_l2 = curr_miss_l2;
//pras comments cache:		misses_seen_l1 = curr_miss_l1;
//pras comments cache:
//pras comments cache:		return 2;	//L2 cache miss
//pras comments cache:	}
//pras comments cache:	else if(curr_miss_l1 > misses_seen_l1)	{
//pras comments cache:		misses_seen_l2 = curr_miss_l2;
//pras comments cache:		misses_seen_l1 = curr_miss_l1;
//pras comments cache:
//pras comments cache:		this_phase_l1_miss++;
//pras comments cache:		return 2;	//FOr Nandhini--changed from 1 to 2 to get traces from L1 cache. //L1 cache miss
//pras comments cache:	}
//pras comments cache:	else{
//pras comments cache:		printf("Difference in misses = %ld\n", (curr_miss_l1 - misses_seen_l1) + (curr_miss_l2 - misses_seen_l2));
//pras comments cache:		printf("Difference in L1 misses = %ld\n", (curr_miss_l1 - misses_seen_l1));
//pras comments cache:		printf("Difference in l2 misses = %ld\n", (curr_miss_l2 - misses_seen_l2));
//pras comments cache:
//pras comments cache:		misses_seen_l2 = curr_miss_l2;
//pras comments cache:		misses_seen_l1 = curr_miss_l1;
//pras comments cache:		return 0;
//pras comments cache:	}
}

