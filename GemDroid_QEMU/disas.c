/* General "disassemble this chunk" code.  Used for debugging. */
#include "config.h"
#include "disas/bfd.h"
#include "elf.h"
#include <errno.h>

//GemDroid added
//For GemDroid Tracer Functionality
#include "gemdroid-tracer.h"
//GemDroid end
#include "cpu.h"
#include "disas/disas.h"

typedef struct CPUDebug {
    struct disassemble_info info;
    CPUArchState *env;
} CPUDebug;

/* Filled in by elfload.c.  Simplistic, but will do for now. */
struct syminfo *syminfos = NULL;
//pras adds all these
int allPidsChecked[10000];
int allPidsLength = 0;
#ifdef TARGET_ARM
struct CPUARMState *myPrevState = NULL;
#endif
#define print_fields_arm(field, format) do {if(my_env->field != myPrevState->field){printf(#field" = "#format" ", my_env->field); myPrevState->field = my_env->field;} } while(0)
void printFloatStatus(CPUArchState *env, int my_byte_index, char  dest[3000])
{

#ifdef TARGET_ARM
	CPUARMState *my_env = (CPUARMState *)env;
	//printf("fp_status: ");
	print_fields_arm(vfp.fp_status.float_detect_tininess, %d);
    print_fields_arm(vfp.fp_status.float_rounding_mode, %d);
    print_fields_arm(vfp.fp_status.float_exception_flags, %d);
    print_fields_arm(vfp.fp_status.floatx80_rounding_precision, %d);
    print_fields_arm(vfp.fp_status.flush_to_zero, %d);
    print_fields_arm(vfp.fp_status.flush_inputs_to_zero, %u);
    print_fields_arm(vfp.fp_status.default_nan_mode, %u);
	//printf("std_fp_status: ");
    print_fields_arm(vfp.standard_fp_status.float_detect_tininess, %d);
    print_fields_arm(vfp.standard_fp_status.float_rounding_mode, %d);
    print_fields_arm(vfp.standard_fp_status.float_exception_flags, %d);
    print_fields_arm(vfp.standard_fp_status.floatx80_rounding_precision, %d);
    print_fields_arm(vfp.standard_fp_status.flush_to_zero, %d);
    print_fields_arm(vfp.standard_fp_status.flush_inputs_to_zero, %u);
    print_fields_arm(vfp.standard_fp_status.default_nan_mode, %u);


#endif
	printf("\n");
}
void printRegisters(CPUArchState *env, const char *text)
{
	printf(text);
	//it works, but a bit laaaaaaaaaater
	//printf("very happy!!!!\n");
	//exit(0);
#ifdef TARGET_ARM
	char dest[3000] = "";
	int my_index = 0;
	if(myPrevState == NULL)
	{
		myPrevState = (struct CPUARMState *)calloc(sizeof(struct CPUARMState),1);
	}
	CPUARMState *my_env = (CPUARMState *)env;
	int i = 0;
	for(i = 0; i < 16; i++)
	{
		//sprintf(dest, "r%d = %u ", i, my_env->regs[i]);
		if(my_env->regs[i] != myPrevState->regs[i])
		{
			printf("r%d = %x ", i, my_env->regs[i]);
			myPrevState->regs[i] = my_env->regs[i];
		}
		//print_fields_arm(regs[i], %u);
	}
	sprintf(dest, "uncached_cpsr = %x ", my_env->uncached_cpsr);
	sprintf(dest, "spsr = %x ", my_env->spsr);
	for(i = 0; i < 7; i++)
	{
		//print_fields_arm(banked_spsr[i], %u);
		//print_fields_arm(banked_r13[i], %u);
		//print_fields_arm(banked_r14[i], %u);
		if(my_env->banked_spsr[i] != myPrevState->banked_spsr[i])
		{
			printf("b_spsr%d = %x ", i, my_env->banked_spsr[i]);
			myPrevState->banked_spsr[i] = my_env->banked_spsr[i];
		}
		if(my_env->banked_r13[i] != myPrevState->banked_r13[i])
		{
			printf("b_r13_%d = %x ", i, my_env->banked_r13[i]);
			myPrevState->banked_r13[i] = my_env->banked_r13[i];
		}
		if(my_env->banked_r14[i] != myPrevState->banked_r14[i])
		{
			printf("b_r14_%d = %x ", i, my_env->banked_r14[i]);
			myPrevState->banked_r14[i] = my_env->banked_r14[i];
		}
	//	sprintf(dest, "b_spsr%d = %u ", i, my_env->banked_spsr[i]);
	//	sprintf(dest, "b_r13_%d = %u ", i, my_env->banked_r13[i]);
	//	sprintf(dest, "b_r14_%d = %u ", i, my_env->banked_r14[i]);
	}
	for(i = 0; i < 5; i++)
	{
		if(my_env->usr_regs[i] != myPrevState->usr_regs[i])
		{
			printf("usr_regs[%d] = %x ", i, my_env->usr_regs[i]);
			myPrevState->usr_regs[i] = my_env->usr_regs[i];
		}
		if(my_env->fiq_regs[i] != myPrevState->fiq_regs[i])
		{
			printf("fiq_regs[%d] = %x ", i, my_env->fiq_regs[i]);
			myPrevState->fiq_regs[i] = my_env->fiq_regs[i];
		}
		//sprintf(dest, "usr_regs[%d] = %u ", i, my_env->usr_regs[i]);
		//sprintf(dest, "fiq_regs[%d] = %u ", i, my_env->fiq_regs[i]);
	}
	//printf("cp15: ");
//	c2_base0 = %u c2_base1 = %u c2_control = %u c2_mask = %u c2_base_mask = %u "
//			"c5_insn = %u c5_data = %u c13_tls1 = %u c13_tls2 = %u c13_tls3 = %u "
//			"v7m: other_sp = %u vecbase = %u basepri = %u control = %u current_sp = %u "
//			"exception = %d pending_exception = %d "
//			"vfp: vec_len = %d vec_stride = %d ", 
	print_fields_arm(cp15.c0_cpuid, %u);
	print_fields_arm(cp15.c0_cachetype, %u);
	for(i = 0; i < 16; i++)
	{
		if(myPrevState->cp15.c0_ccsid[i] != my_env->cp15.c0_ccsid[i])
		{
			printf("cp15.c0_ccsid[%d]=%u ", i, my_env->cp15.c0_ccsid[i]);
			myPrevState->cp15.c0_ccsid[i] = my_env->cp15.c0_ccsid[i];
		}
	}
	print_fields_arm(cp15.c0_clid, %u);
	print_fields_arm(cp15.c0_cssel, %u);
	for(i = 0; i < 8; i++)
	{
		if(myPrevState->cp15.c0_c1[i] != my_env->cp15.c0_c1[i])
		{
			printf("cp15.c0_c1[%d]=%u ", i, my_env->cp15.c0_c1[i]);
			myPrevState->cp15.c0_c1[i] = my_env->cp15.c0_c1[i];
		}
		if(myPrevState->cp15.c0_c2[i] != my_env->cp15.c0_c2[i])
		{
			printf("cp15.c0_c2[%d]=%u ", i,  my_env->cp15.c0_c2[i]);
			myPrevState->cp15.c0_c2[i] = my_env->cp15.c0_c2[i];
		}
	}
	print_fields_arm(cp15.c1_sys, %u);
	print_fields_arm(cp15.c1_coproc, %u);
	print_fields_arm(cp15.c1_xscaleauxcr, %u);
	print_fields_arm(cp15.c1_secfg, %u);
	print_fields_arm(cp15.c1_sedbg, %u);
	print_fields_arm(cp15.c1_nseac, %u);
	print_fields_arm(cp15.c2_base0, %u);
	print_fields_arm(cp15.c2_base1, %u);
	print_fields_arm(cp15.c2_control, %u);
	print_fields_arm(cp15.c2_mask, %u);
	print_fields_arm(cp15.c2_base_mask, %u);
	print_fields_arm(cp15.c2_insn, %u);
	print_fields_arm(cp15.c2_data, %u);
	print_fields_arm(cp15.c3, %u);
	print_fields_arm(cp15.c5_insn, %u);
	print_fields_arm(cp15.c5_data, %u);
	for(i = 0; i < 8; i++)
	{
		if(myPrevState->cp15.c6_region[i] != my_env->cp15.c6_region[i])
		{
			printf("cp15.c6_region[%d]=%u ", i,  my_env->cp15.c6_region[i]);
			myPrevState->cp15.c6_region[i] = my_env->cp15.c6_region[i];
		}
	}
	print_fields_arm(cp15.c6_insn, %u);
	print_fields_arm(cp15.c6_data, %u);
	print_fields_arm(cp15.c7_par, %u);
	print_fields_arm(cp15.c9_insn, %u);
	print_fields_arm(cp15.c9_data, %u);
	print_fields_arm(cp15.c9_pmcr, %u);
	print_fields_arm(cp15.c9_pmcnten, %u);
	print_fields_arm(cp15.c9_pmovsr, %u);
	print_fields_arm(cp15.c9_pmxevtyper, %u);
	print_fields_arm(cp15.c9_pmuserenr, %u);
	print_fields_arm(cp15.c9_pminten, %u);
	print_fields_arm(cp15.c12_vbar, %u);
	print_fields_arm(cp15.c12_mvbar, %u);
	print_fields_arm(cp15.c13_fcse, %u);
	print_fields_arm(cp15.c13_context, %u);
	print_fields_arm(cp15.c13_tls1, %u);
	print_fields_arm(cp15.c13_tls2, %u);
	print_fields_arm(cp15.c13_tls3, %u);
	print_fields_arm(cp15.c15_cpar, %u);
	print_fields_arm(cp15.c15_ticonfig, %u);
	print_fields_arm(cp15.c15_i_max, %u);
	print_fields_arm(cp15.c15_i_min, %u);
	print_fields_arm(cp15.c15_threadid, %u);
	print_fields_arm(v7m.other_sp, %u);
	print_fields_arm(v7m.vecbase, %u);
	print_fields_arm(v7m.basepri, %u);
	print_fields_arm(v7m.control, %u);
	print_fields_arm(v7m.current_sp, %u);
	print_fields_arm(v7m.exception, %d);
	print_fields_arm(v7m.pending_exception, %d);
	print_fields_arm(teecr, %u);
	print_fields_arm(teehbr, %u);
	print_fields_arm(features, %u);
	print_fields_arm(vfp.vec_len, %d);
	print_fields_arm(vfp.vec_stride, %d);
	for(i = 0; i <  32 ; i++)
	{
//		sprintf(dest, "r%d = %lf ",i, my_env->vfp.regs[i]);
		if(my_env->vfp.regs[i] != myPrevState->vfp.regs[i])
		{
			printf("r%d = %2.4lf ",i, my_env->vfp.regs[i]);
			myPrevState->vfp.regs[i] = my_env->vfp.regs[i];
		}
	}
	for(i = 0; i <  16 ; i++)
	{
//		sprintf(dest, "xr%d = %6.4lf ",i, my_env->vfp.xregs[i]);
		if(my_env->vfp.xregs[i] != myPrevState->vfp.xregs[i])
		{
			printf("xr%d = %2.4lf ",i, my_env->vfp.xregs[i]);
			myPrevState->vfp.xregs[i] = my_env->vfp.xregs[i];
		}
	}
	for(i = 0; i <  8 ; i++)
	{
		sprintf(dest, "scratch%d = %x ",i, my_env->vfp.scratch[i]);
		if(my_env->vfp.scratch[i] != myPrevState->vfp.scratch[i])
		{
			printf("scratch%d = %x ",i, my_env->vfp.scratch[i]);
			myPrevState->vfp.scratch[i] = my_env->vfp.scratch[i];
		}
	}
	printFloatStatus(env, my_index, dest);
	print_fields_arm(iwmmxt.val, %lu);
	for(i = 0; i <  16 ; i++)
	{
		if(my_env->iwmmxt.cregs[i] != myPrevState->iwmmxt.cregs[i])
		{
			printf("iwmmxt.cregs[%d] = %lu ",i, my_env->iwmmxt.cregs[i]);
			myPrevState->iwmmxt.cregs[i] = my_env->iwmmxt.cregs[i];
		}
		if(my_env->iwmmxt.regs[i] != myPrevState->iwmmxt.regs[i])
		{
			printf("iwmmxt.regs[%d] = %lu ",i, my_env->iwmmxt.regs[i]);
			myPrevState->iwmmxt.regs[i] = my_env->iwmmxt.regs[i];
		}
	}
	//printf("%s\n", dest);
	if(strlen(dest) > 3000)
	{
		printf("make it 4?\n");
		exit(0);
	}
#else
	assert(false);
#endif
}
inline int getMePCVal(CPUArchState *env)
{

#ifdef TARGET_ARM
	if(env == NULL)
	{
		env = cpu_single_env;
	}
	int current = ((CPUARMState *)env)->regs[15];
    return current; 
#else
	printf("pc val get is not implemented for other archs!\n");
	assert(false);
	return  -1;
#endif
}

inline int getMeContextIdNoArgsDEPRECATED()//context is not same at caller and callee
{
#ifdef TARGET_ARM
	CPUArchState* env = cpu_single_env;
	if(!env)
	{
		printf("env is null!\n");
		exit(0);
	}
	int current = ((CPUARMState *)env)->cp15.c13_context;
    return current; 
#else
	printf("context id get is not implemented for other archs!\n");
	assert(false);
	return  -1;
#endif
}
inline int getMeContextId(CPUArchState *env)
{

#ifdef TARGET_ARM
	if(env == NULL)
	{
		printf("env is null!\n");
		exit(0);
		env = cpu_single_env;
	}
	if(!env)
	{
		printf("env is null!\n");
		exit(0);
	}
	int current = ((CPUARMState *)env)->cp15.c13_context;
	/*
	int current = ((CPUARMState *)env)->cp15.c15_threadid;
	if(current == 0)
	{
		current = ((CPUARMState *)env)->cp15.c13_context;
		//if(needed_tid_length > 0)
		//{
		//	printf("current=%d\n", current);
		//}
	}
	//else
	{
	none of these things work.. except context.
		static int removeAfterDebug = -1;
		if(removeAfterDebug != current)
		{
			removeAfterDebug = current;
			printf("threadid=%d, context=%d fcse=%d\n",  ((CPUARMState *)env)->cp15.c15_threadid, ((CPUARMState *)env)->cp15.c13_context,  ((CPUARMState *)env)->cp15.c13_fcse);
		}
	}
	*/
	//int current = 0;
	//while(current1)
	//{
	//	current <<= 4;
	//	current |= (current1 & 0xff);
	//	current1 >>= 4;
	//}
//	current >>= 8;
	//if(needed_pid != -1)
	//{
	////	printf("current = %d needed = %d, in disas figuring current out\n",current, needed_pid);
	//}
	//if(needed_pid > 0 && needed_pid == current)
	//{
	//	printf("getting in context id returner..\n");
	//}
    return current; 
#else
	printf("context id get is not implemented for other archs!\n");
	assert(false);
	return  -1;
#endif
}
int indexPidTid(int tid)
{
	//return 1;
	//printf("coming here so often? %d\n", needed_tid_length);
	int context = tid; 
	if(needed_pid == context) 
	{
		return 0;
	}
	int i = 0;
	if(needed_tid_length > 0)
	{
		//printf("%d", context);
		for(i = 0; i < needed_tid_length; i++)
		{
		//	printf(" == %d? ", needed_tids[i]);
			if(context == needed_tids[i])return i;
		}
		//printf("\n");
	}
	return -1;
}
void addPrintTid(CPUArchState *env, int val)
{
	if(env == NULL)
	{
		if(current_cpu)
		{
			env = cpu_single_env;
		}
	}
	if(env == NULL)
	{
		return ;
	}
	int context = getMeContextId(env);
	//return 1;
	//printf("coming here so often? %d\n", needed_tid_length);
	if(needed_pid == context) 
	{
	//	printf("need: %d = %d times\n", needed_pid, tidCounter[context]);
		if(val > 0 && print_tids[0] & val > 0)
		{
			printf("disas 958th line %d",1/0);
		}
		print_tids[0]+=val;
		return;
	}
	int i = 0;
	if(needed_tid_length > 0)
	{
		//printf("%d", context);
		for(i = 0; i < needed_tid_length; i++)
		{
			//printf("%d == %d? ", context, needed_tids[i]);
	//		if(tidCounter[needed_tids[i]] > 0)
	//		{
	//			printf("needed: %d = %d times\n", needed_tids[i], tidCounter[context]);
	//		}
			if(context == needed_tids[i])
			{
				//swap to 0th location for speed [splaying :P]
				int temp = needed_tids[0];
				needed_tids[0] = context;
				needed_tids[i] = temp;
				needed_pid = context;
				if(needed_pid == 0)
				{
					printf("URYU: here? len=%d context=%d i=%d temp=%d\n", needed_tid_length, context,i, needed_tids[i], temp);
					exit(0);
				}
				temp = print_tids[0];
				print_tids[0]=print_tids[i];
				print_tids[i] = temp;
				if(val > 0 && print_tids[0] & val > 0)
				{
					printf("%d",1/0);
				}
				print_tids[0]+=val;
				return;
				//ORIG: return i+1;
			}
		}
		//printf("\n");
	}
	return;
}
void insertIntoAllPids(int one, int two)
{
	int oneF = 0, twoF = 0, i = 0;
	for(i = 0; i < allPidsLength; i++)
	{
		oneF |= (one == allPidsChecked[i]);
		twoF |= (two == allPidsChecked[i]);
		if(oneF)
		{
			int t = allPidsChecked[i];
			allPidsChecked[i] = allPidsChecked[0];
			allPidsChecked[0] = t;
			if(twoF)
			{
				return;
			}
		}
	}
	if(!oneF)
	{
		//printf("checking matchmaking for %d at %d\n", one, allPidsLength);
			int t = allPidsChecked[0];
			allPidsChecked[0] = one;
			allPidsChecked[allPidsLength] = t;
		allPidsLength ++;
	}
	if(one != two && twoF)
	{
		allPidsChecked[allPidsLength] = two;
		//printf("checking matchmaking for %d at %d\n", two, allPidsLength);
		allPidsLength ++;
	}
}
int matchMeInPidTid(CPUArchState *env)
{
	//return CPU_tracer;
	if(!CPU_tracer)
	{
		return 0;
	}
	if(env == NULL)
	{
		printf("env null in matchme is happening?\n");
		exit(0);
		if(current_cpu)
		{
			env = cpu_single_env;
		}
	}
	//if(env == NULL)
	//{
	//	//printf("returning 0.. null\n");
	//	return 0;
	//}
	int context = getMeContextId(env);
	int context1 = getMeContextId(cpu_single_env);
	insertIntoAllPids(context, context1);
	if(needed_pid == 0)
	{
		printf("this is happening?\n");
		exit(0);
	}
	//return 1;
	//printf("coming here so often? %d\n", needed_tid_length);
	if(needed_pid == context || context1 == needed_pid) 
	{
		//printf("need: %d = %d times\n", needed_pid, context);
		
		//to use who called infos if(print_tids[0] == 0)
		//to use who called infos {
		//to use who called infos 	return 1;
		//to use who called infos }
		//to use who called infos else return print_tids[0];
		//printf("needed = %d, context = %d\n", needed_pid, context);
		arm_helper_tid_now = context;
		return 1;
	}
	int i = 0;
	if(needed_tid_length > 0)
	{
		//printf("%d", context);
		for(i = 0; i < needed_tid_length; i++)
		{
			//printf("%d == %d? ", context, needed_tids[i]);
	//		if(tidCounter[needed_tids[i]] > 0)
	//		{
	//			printf("needed: %d = %d times\n", needed_tids[i], tidCounter[context]);
	//		}
			if(context == needed_tids[i] || needed_tids[i] == context1)
			{
				//swap to 0th location for speed [splaying :P]
				int temp = needed_tids[0];
				needed_tids[0]=context;
				needed_tids[i] = temp;
				needed_pid = context;
				if(needed_pid == 0)
				{
					printf("HOLLOW: matching here? len=%d context=%d\n", needed_tid_length, context);
					exit(0);
				}
				temp = print_tids[0];
				print_tids[0]=print_tids[i];
				print_tids[i] = temp;
				//to use who called infos if(print_tids[0] == 0)
				//to use who called infos {
				//to use who called infos 	return 1;
				//to use who called infos }
				//to use who called infos return print_tids[0];
				//printf("needed = %d, context = %d at the right place %d\n", needed_pid, context, i+1);
				arm_helper_tid_now = context;
				return i+1;
			}
		}
		//printf("\n");
	}
	//printf("returning 0..\n");
	return 0;
}
int matchMeInPidTidNoArgsDEPRECATED()//context is not same at caller and callee
{
	return matchMeInPidTid(NULL);
}
typedef struct hash_member{
	target_ulong pc, size;
}hash_member;
typedef struct hashset{
	hash_member *printed_blocks;
	int count, max;
} hashset;
static hashset *my_hashset = NULL;
static int foundInHashset(target_ulong pc, target_ulong size)
{
	int i = 0;
	for(i = 0; i < my_hashset->count; i++)
	{
		if(my_hashset->printed_blocks[i].pc == pc && my_hashset->printed_blocks[i].size == size)
		{
			return i;
		}
	}
	return 0;
}
static int alreadyPrinted(target_ulong pc, target_ulong size)
{
	//REMOVE REMOVE REMOVE
	//return 1;
	if(my_hashset == NULL)
	{
		my_hashset = (hashset*)malloc(sizeof(hashset));
		my_hashset->printed_blocks = (hash_member *)malloc(sizeof(hash_member)*100);
		my_hashset->max = 100;
		my_hashset->count = 0;
		my_hashset->printed_blocks[my_hashset->count].pc = pc;
		my_hashset->printed_blocks[my_hashset->count].size = size;
		my_hashset->count++;
		return 0;
	}
	int i = 0;
	for(i = 0; i < my_hashset->count; i++)
	{
		if(my_hashset->printed_blocks[i].pc == pc && my_hashset->printed_blocks[i].size == size)
		{
			return i+1;
		}
	}
	//if(foundInHashset(pc, size))return 1;
	if(my_hashset->count < my_hashset->max)
	{
		my_hashset->printed_blocks[my_hashset->count].pc = pc;
		my_hashset->printed_blocks[my_hashset->count].size = size;
		my_hashset->count++;
	}
	else
	{
		hash_member *new_alloc = realloc(my_hashset->printed_blocks, sizeof(hash_member)*(my_hashset->max+100));
		if(!new_alloc)
		{
			printf("hashset is a memory overhead.. allocated %d bytes OR %d basic blocks: use/implement tb_fast to do this..\n", sizeof(hash_member)*my_hashset->max, my_hashset->max);
			exit(0);
		}
		else
		{
			my_hashset->printed_blocks = new_alloc;
			my_hashset->max = my_hashset->max+100;
			my_hashset->printed_blocks[my_hashset->count].pc = pc;
			my_hashset->printed_blocks[my_hashset->count].size = size;
			my_hashset->count++;
		}
	}
	return 0;
}
/* Get LENGTH bytes from info's buffer, at target address memaddr.
   Transfer them to myaddr.  */
int
buffer_read_memory(bfd_vma memaddr, bfd_byte *myaddr, int length,
                   struct disassemble_info *info)
{
    if (memaddr < info->buffer_vma
        || memaddr + length > info->buffer_vma + info->buffer_length)
        /* Out of bounds.  Use EIO because GDB uses it.  */
        return EIO;
    memcpy (myaddr, info->buffer + (memaddr - info->buffer_vma), length);
    return 0;
}

/* Get LENGTH bytes from info's buffer, at target address memaddr.
   Transfer them to myaddr.  */
static int
target_read_memory (bfd_vma memaddr,
                    bfd_byte *myaddr,
                    int length,
                    struct disassemble_info *info)

{
    CPUDebug *s = container_of(info, CPUDebug, info);

    cpu_memory_rw_debug(ENV_GET_CPU(s->env), memaddr, myaddr, length, 0, /*pras*/MEM_REQ_DISAS);
//this prints all opcodes in disas	static uint32_t count = 0x0;
//this prints all opcodes in disas	count++;
//this prints all opcodes in disas	memcpy(myaddr, &count, length); 
    return 0;
}

/* Print an error message.  We can assume that this is in response to
   an error return from buffer_read_memory.  */
void
perror_memory (int status, bfd_vma memaddr, struct disassemble_info *info)
{
  if (status != EIO)
    /* Can't happen.  */
    (*info->fprintf_func) (info->stream, "Unknown error %d\n", status);
  else
    /* Actually, address between memaddr and memaddr + len was
       out of bounds.  */
    (*info->fprintf_func) (info->stream,
			   "Address 0x%" PRIx64 " is out of bounds.\n", memaddr);
}

/* This could be in a separate file, to save minuscule amounts of space
   in statically linked executables.  */

/* Just print the address is hex.  This is included for completeness even
   though both GDB and objdump provide their own (to print symbolic
   addresses).  */

void
generic_print_address (bfd_vma addr, struct disassemble_info *info)
{
    (*info->fprintf_func) (info->stream, "0x%" PRIx64, addr);
}

/* Print address in hex, truncated to the width of a target virtual address. */
static void
generic_print_target_address(bfd_vma addr, struct disassemble_info *info)
{
    uint64_t mask = ~0ULL >> (64 - TARGET_VIRT_ADDR_SPACE_BITS);
    generic_print_address(addr & mask, info);
}

/* Print address in hex, truncated to the width of a host virtual address. */
static void
generic_print_host_address(bfd_vma addr, struct disassemble_info *info)
{
    uint64_t mask = ~0ULL >> (64 - (sizeof(void *) * 8));
    generic_print_address(addr & mask, info);
}

/* Just return the given address.  */

int
generic_symbol_at_address (bfd_vma addr, struct disassemble_info *info)
{
  return 1;
}

bfd_vma bfd_getl64 (const bfd_byte *addr)
{
  unsigned long long v;

  v = (unsigned long long) addr[0];
  v |= (unsigned long long) addr[1] << 8;
  v |= (unsigned long long) addr[2] << 16;
  v |= (unsigned long long) addr[3] << 24;
  v |= (unsigned long long) addr[4] << 32;
  v |= (unsigned long long) addr[5] << 40;
  v |= (unsigned long long) addr[6] << 48;
  v |= (unsigned long long) addr[7] << 56;
  return (bfd_vma) v;
}

bfd_vma bfd_getl32 (const bfd_byte *addr)
{
  unsigned long v;

  v = (unsigned long) addr[0];
  v |= (unsigned long) addr[1] << 8;
  v |= (unsigned long) addr[2] << 16;
  v |= (unsigned long) addr[3] << 24;
  return (bfd_vma) v;
}

bfd_vma bfd_getb32 (const bfd_byte *addr)
{
  unsigned long v;

  v = (unsigned long) addr[0] << 24;
  v |= (unsigned long) addr[1] << 16;
  v |= (unsigned long) addr[2] << 8;
  v |= (unsigned long) addr[3];
  return (bfd_vma) v;
}

bfd_vma bfd_getl16 (const bfd_byte *addr)
{
  unsigned long v;

  v = (unsigned long) addr[0];
  v |= (unsigned long) addr[1] << 8;
  return (bfd_vma) v;
}

bfd_vma bfd_getb16 (const bfd_byte *addr)
{
  unsigned long v;

  v = (unsigned long) addr[0] << 24;
  v |= (unsigned long) addr[1] << 16;
  return (bfd_vma) v;
}

#ifdef TARGET_ARM
static int
print_insn_thumb1(bfd_vma pc, disassemble_info *info)
{
  return print_insn_arm(pc | 1, info);
}
#endif

static int print_insn_objdump(bfd_vma pc, disassemble_info *info,
                              const char *prefix)
{
    int i, n = info->buffer_length;
    uint8_t *buf = g_malloc(n);

    info->read_memory_func(pc, buf, n, info);

    for (i = 0; i < n; ++i) {
        if (i % 32 == 0) {
            info->fprintf_func(info->stream, "\n%s: ", prefix);
        }
        info->fprintf_func(info->stream, "%02x", buf[i]);
    }

    g_free(buf);
    return n;
}

static int print_insn_od_host(bfd_vma pc, disassemble_info *info)
{
    return print_insn_objdump(pc, info, "OBJD-H");
}

static int print_insn_od_target(bfd_vma pc, disassemble_info *info)
{
    return print_insn_objdump(pc, info, "OBJD-T");
}

/* Disassemble this for me please... (debugging). 'flags' has the following
   values:
    i386 - 1 means 16 bit code, 2 means 64 bit code
    arm  - bit 0 = thumb, bit 1 = reverse endian
    ppc  - nonzero means little endian
    other targets - unused
 */
void target_disas(FILE *out, CPUArchState *env, target_ulong code,
                  target_ulong size, int flags)//, int pid_matched_gemdroid)
{
    target_ulong pc;
	//Gemdroid added
	if(out == 0)
		out = stdout;
	//Gemdroid end
    int count;
    CPUDebug s;
    int (*print_insn)(bfd_vma pc, disassemble_info *info) = NULL;

    INIT_DISASSEMBLE_INFO(s.info, out, fprintf);

    s.env = env;
    s.info.read_memory_func = target_read_memory;
    s.info.buffer_vma = code;
    s.info.buffer_length = size;
    s.info.print_address_func = generic_print_target_address;

#ifdef TARGET_WORDS_BIGENDIAN
    s.info.endian = BFD_ENDIAN_BIG;
#else
    s.info.endian = BFD_ENDIAN_LITTLE;
#endif
#if defined(TARGET_I386)
    if (flags == 2) {
        s.info.mach = bfd_mach_x86_64;
    } else if (flags == 1) {
        s.info.mach = bfd_mach_i386_i8086;
    } else {
        s.info.mach = bfd_mach_i386_i386;
    }
    print_insn = print_insn_i386;
#elif defined(TARGET_ARM)
    if (flags & 1) {
        print_insn = print_insn_thumb1;
    } else {
        print_insn = print_insn_arm;
    }
    if (flags & 2) {
#ifdef TARGET_WORDS_BIGENDIAN
        s.info.endian = BFD_ENDIAN_LITTLE;
#else
        s.info.endian = BFD_ENDIAN_BIG;
#endif
    }
#elif defined(TARGET_SPARC)
    print_insn = print_insn_sparc;
#ifdef TARGET_SPARC64
    s.info.mach = bfd_mach_sparc_v9b;
#endif
#elif defined(TARGET_PPC)
    if (flags >> 16) {
        s.info.endian = BFD_ENDIAN_LITTLE;
    }
    if (flags & 0xFFFF) {
        /* If we have a precise definitions of the instructions set, use it */
        s.info.mach = flags & 0xFFFF;
    } else {
#ifdef TARGET_PPC64
        s.info.mach = bfd_mach_ppc64;
#else
        s.info.mach = bfd_mach_ppc;
#endif
    }
    s.info.disassembler_options = (char *)"any";
    print_insn = print_insn_ppc;
#elif defined(TARGET_M68K)
    print_insn = print_insn_m68k;
#elif defined(TARGET_MIPS)
#ifdef TARGET_WORDS_BIGENDIAN
    print_insn = print_insn_big_mips;
#else
    print_insn = print_insn_little_mips;
#endif
#elif defined(TARGET_SH4)
    s.info.mach = bfd_mach_sh4;
    print_insn = print_insn_sh;
#elif defined(TARGET_ALPHA)
    s.info.mach = bfd_mach_alpha_ev6;
    print_insn = print_insn_alpha;
#elif defined(TARGET_CRIS)
    if (flags != 32) {
        s.info.mach = bfd_mach_cris_v0_v10;
        print_insn = print_insn_crisv10;
    } else {
        s.info.mach = bfd_mach_cris_v32;
        print_insn = print_insn_crisv32;
    }
#elif defined(TARGET_S390X)
    s.info.mach = bfd_mach_s390_64;
    print_insn = print_insn_s390;
#elif defined(TARGET_MICROBLAZE)
    s.info.mach = bfd_arch_microblaze;
    print_insn = print_insn_microblaze;
#elif defined(TARGET_MOXIE)
    s.info.mach = bfd_arch_moxie;
    print_insn = print_insn_moxie;
#elif defined(TARGET_LM32)
    s.info.mach = bfd_mach_lm32;
    print_insn = print_insn_lm32;
#endif
    if (print_insn == NULL) {
        print_insn = print_insn_od_target;
    }
	//static int prev_tid = -1;
	//if(prev_tid != getMeContextId(env) && CPU_tracer)
	//{
	//	prev_tid = getMeContextId(env);
	//	printf("cpu-trcer = %d, matched=%d, current=%d\n", CPU_tracer, matchMeInPidTid(env), getMeContextId(env));
	//}
	// this not getting called for some reason!
	//REMOVE REMOVE REMOVE
		//if(CPU_tracer)/// &&  matchMeInPidTid(env))//current_pid == needed_pid && needed_pid > 0)
		if(CPU_tracer &&  matchMeInPidTid(env))//current_pid == needed_pid && needed_pid > 0)
		{
	//REMOVE REMOVE REMOVE
			printRegisters(env, "before entering:");
			if(!alreadyPrinted(code,  size))
			{

    for (pc = code; size > 0; pc += count, size -= count) {
					fprintf(out, "%d %x ",getMeContextId(env),  pc);
					//	fprintf(out, "0x" TARGET_FMT_lx " ", pc);
					//GemDroid End.
					//fprintf(out, "0x" TARGET_FMT_lx ":  ", pc);
					//pras comments: addPrintTid(env, +64);
					uint64_t deb = 0;
				//	for(deb = 0x0; deb < 0xfffffffful;deb++)
				//	{
				//		fprintf(out, "%d %x ",getMeContextId(env),  pc);
						count = print_insn(pc, &s.info);
			//			fprintf(out, "\n");
			//		}
			//		exit(0);
					//pras comments: addPrintTid(env, -64);
	//////////fprintf(out, "0x" TARGET_FMT_lx ":  ", pc);
	//////////////count = print_insn(pc, &s.info);
#if 0
        {
            int i;
            uint8_t b;
            fprintf(out, " {");
            for(i = 0; i < count; i++) {
                target_read_memory(pc + i, &b, 1, &s.info);
                fprintf(out, " %02x", b);
            }
            fprintf(out, " }");
        }
#endif
	fprintf(out, "\n");
	if (count < 0)
	    break;
        if (size < count) {
            fprintf(out,
                    "Disassembler disagrees with translator over instruction "
                    "decoding\n"
                    "Please report this to qemu-devel@nongnu.org\n");
            break;
        }
    }
	fprintf(out, "***end***\n");
			}
			else
			{

				//REMOVE REMOVE REMOVE
				fprintf(out, "%d, Lookup: %x %d\n",getMeContextId(env),  code, size);
			}
		}
		//pras adds this new
	fflush(out);
}

/* Disassemble this for me please... (debugging). */
void disas(FILE *out, void *code, unsigned long size)
{
	//Gemdroid added
	if(out == 0)
		out = stdout;
	//Gemdroid end
    uintptr_t pc;
    int count;
    CPUDebug s;
    int (*print_insn)(bfd_vma pc, disassemble_info *info) = NULL;

    INIT_DISASSEMBLE_INFO(s.info, out, fprintf);
    s.info.print_address_func = generic_print_host_address;

    s.info.buffer = code;
    s.info.buffer_vma = (uintptr_t)code;
    s.info.buffer_length = size;

#ifdef HOST_WORDS_BIGENDIAN
    s.info.endian = BFD_ENDIAN_BIG;
#else
    s.info.endian = BFD_ENDIAN_LITTLE;
#endif
#if defined(CONFIG_TCG_INTERPRETER)
    print_insn = print_insn_tci;
#elif defined(__i386__)
    s.info.mach = bfd_mach_i386_i386;
    print_insn = print_insn_i386;
#elif defined(__x86_64__)
    s.info.mach = bfd_mach_x86_64;
    print_insn = print_insn_i386;
#elif defined(_ARCH_PPC)
    s.info.disassembler_options = (char *)"any";
    print_insn = print_insn_ppc;
#elif defined(__alpha__)
    print_insn = print_insn_alpha;
#elif defined(__sparc__)
    print_insn = print_insn_sparc;
    s.info.mach = bfd_mach_sparc_v9b;
#elif defined(__arm__)
    print_insn = print_insn_arm;
#elif defined(__MIPSEB__)
    print_insn = print_insn_big_mips;
#elif defined(__MIPSEL__)
    print_insn = print_insn_little_mips;
#elif defined(__m68k__)
    print_insn = print_insn_m68k;
#elif defined(__s390__)
    print_insn = print_insn_s390;
#elif defined(__hppa__)
    print_insn = print_insn_hppa;
#elif defined(__ia64__)
    print_insn = print_insn_ia64;
#endif
    if (print_insn == NULL) {
        print_insn = print_insn_od_host;
    }
    for (pc = (uintptr_t)code; size > 0; pc += count, size -= count) {
        fprintf(out, "0x%08" PRIxPTR ":  ", pc);
        count = print_insn(pc, &s.info);
	fprintf(out, "\n");
	if (count < 0)
	    break;
    }
}

/* Look up symbol for debugging purpose.  Returns "" if unknown. */
const char *lookup_symbol(target_ulong orig_addr)
{
    const char *symbol = "";
    struct syminfo *s;

    for (s = syminfos; s; s = s->next) {
        symbol = s->lookup_symbol(s, orig_addr);
        if (symbol[0] != '\0') {
            break;
        }
    }

    return symbol;
}

#if !defined(CONFIG_USER_ONLY)

#include "monitor/monitor.h"

static int monitor_disas_is_physical;

static int
monitor_read_memory (bfd_vma memaddr, bfd_byte *myaddr, int length,
                     struct disassemble_info *info)
{
    CPUDebug *s = container_of(info, CPUDebug, info);

    if (monitor_disas_is_physical) {
		int printData = matchMeInPidTid(s->env);
        cpu_physical_memory_read(memaddr, myaddr, length);
    } else {
        cpu_memory_rw_debug(ENV_GET_CPU(s->env), memaddr,myaddr, length, 0, /*pras*/MEM_REQ_DISAS);
    }
    return 0;
}

static int GCC_FMT_ATTR(2, 3)
monitor_fprintf(FILE *stream, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    monitor_vprintf((Monitor *)stream, fmt, ap);
    va_end(ap);
    return 0;
}

void monitor_disas(Monitor *mon, CPUArchState *env,
                   target_ulong pc, int nb_insn, int is_physical, int flags)
{
    int count, i;
    CPUDebug s;
    int (*print_insn)(bfd_vma pc, disassemble_info *info);

    INIT_DISASSEMBLE_INFO(s.info, (FILE *)mon, monitor_fprintf);

    s.env = env;
    monitor_disas_is_physical = is_physical;
    s.info.read_memory_func = monitor_read_memory;
    s.info.print_address_func = generic_print_target_address;

    s.info.buffer_vma = pc;

#ifdef TARGET_WORDS_BIGENDIAN
    s.info.endian = BFD_ENDIAN_BIG;
#else
    s.info.endian = BFD_ENDIAN_LITTLE;
#endif
#if defined(TARGET_I386)
    if (flags == 2) {
        s.info.mach = bfd_mach_x86_64;
    } else if (flags == 1) {
        s.info.mach = bfd_mach_i386_i8086;
    } else {
        s.info.mach = bfd_mach_i386_i386;
    }
    print_insn = print_insn_i386;
#elif defined(TARGET_ARM)
    print_insn = print_insn_arm;
#elif defined(TARGET_ALPHA)
    print_insn = print_insn_alpha;
#elif defined(TARGET_SPARC)
    print_insn = print_insn_sparc;
#ifdef TARGET_SPARC64
    s.info.mach = bfd_mach_sparc_v9b;
#endif
#elif defined(TARGET_PPC)
#ifdef TARGET_PPC64
    s.info.mach = bfd_mach_ppc64;
#else
    s.info.mach = bfd_mach_ppc;
#endif
    print_insn = print_insn_ppc;
#elif defined(TARGET_M68K)
    print_insn = print_insn_m68k;
#elif defined(TARGET_MIPS)
#ifdef TARGET_WORDS_BIGENDIAN
    print_insn = print_insn_big_mips;
#else
    print_insn = print_insn_little_mips;
#endif
#elif defined(TARGET_SH4)
    s.info.mach = bfd_mach_sh4;
    print_insn = print_insn_sh;
#elif defined(TARGET_S390X)
    s.info.mach = bfd_mach_s390_64;
    print_insn = print_insn_s390;
#elif defined(TARGET_MOXIE)
    s.info.mach = bfd_arch_moxie;
    print_insn = print_insn_moxie;
#elif defined(TARGET_LM32)
    s.info.mach = bfd_mach_lm32;
    print_insn = print_insn_lm32;
#else
    monitor_printf(mon, "0x" TARGET_FMT_lx
                   ": Asm output not supported on this arch\n", pc);
    return;
#endif

    for(i = 0; i < nb_insn; i++) {
	monitor_printf(mon, "0x" TARGET_FMT_lx ":  ", pc);
        count = print_insn(pc, &s.info);
	monitor_printf(mon, "\n");
	if (count < 0)
	    break;
        pc += count;
    }
}
#endif
