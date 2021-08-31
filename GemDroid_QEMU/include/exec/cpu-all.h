/*
 * defines common to all virtual CPUs
 *
 *  Copyright (c) 2003 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef CPU_ALL_H
#define CPU_ALL_H

#include "qemu-common.h"
#include "qemu/queue.h"
#include "qemu/thread.h"
#include "qemu/tls.h"
#include "exec/cpu-common.h"

/* some important defines:
 *
 * WORDS_ALIGNED : if defined, the host cpu can only make word aligned
 * memory accesses.
 *
 * HOST_WORDS_BIGENDIAN : if defined, the host cpu is big endian and
 * otherwise little endian.
 *
 * (TARGET_WORDS_ALIGNED : same for target cpu (not supported yet))
 *
 * TARGET_WORDS_BIGENDIAN : same for target cpu
 */

#if defined(HOST_WORDS_BIGENDIAN) != defined(TARGET_WORDS_BIGENDIAN)
#define BSWAP_NEEDED
#endif

#ifdef BSWAP_NEEDED

static inline uint16_t tswap16(uint16_t s)
{
    return bswap16(s);
}

static inline uint32_t tswap32(uint32_t s)
{
    return bswap32(s);
}

static inline uint64_t tswap64(uint64_t s)
{
    return bswap64(s);
}

static inline void tswap16s(uint16_t *s)
{
    *s = bswap16(*s);
}

static inline void tswap32s(uint32_t *s)
{
    *s = bswap32(*s);
}

static inline void tswap64s(uint64_t *s)
{
    *s = bswap64(*s);
}

#else

static inline uint16_t tswap16(uint16_t s)
{
    return s;
}

static inline uint32_t tswap32(uint32_t s)
{
    return s;
}

static inline uint64_t tswap64(uint64_t s)
{
    return s;
}

static inline void tswap16s(uint16_t *s)
{
}

static inline void tswap32s(uint32_t *s)
{
}

static inline void tswap64s(uint64_t *s)
{
}

#endif

#if TARGET_LONG_SIZE == 4
#define tswapl(s) tswap32(s)
#define tswapls(s) tswap32s((uint32_t *)(s))
#define bswaptls(s) bswap32s(s)
#else
#define tswapl(s) tswap64(s)
#define tswapls(s) tswap64s((uint64_t *)(s))
#define bswaptls(s) bswap64s(s)
#endif

/* CPU memory access without any memory or io remapping */

/*
 * the generic syntax for the memory accesses is:
 *
 * load: ld{type}{sign}{size}{endian}_{access_type}(ptr)
 *
 * store: st{type}{size}{endian}_{access_type}(ptr, val)
 *
 * type is:
 * (empty): integer access
 *   f    : float access
 *
 * sign is:
 * (empty): for floats or 32 bit size
 *   u    : unsigned
 *   s    : signed
 *
 * size is:
 *   b: 8 bits
 *   w: 16 bits
 *   l: 32 bits
 *   q: 64 bits
 *
 * endian is:
 * (empty): target cpu endianness or 8 bit access
 *   r    : reversed target cpu endianness (not implemented yet)
 *   be   : big endian (not implemented yet)
 *   le   : little endian (not implemented yet)
 *
 * access_type is:
 *   raw    : host memory access
 *   user   : user mode access using soft MMU
 *   kernel : kernel mode access using soft MMU
 */

/* target-endianness CPU memory access functions */
#if defined(TARGET_WORDS_BIGENDIAN)
#define lduw_p(p) lduw_be_p(p)
#define ldsw_p(p) ldsw_be_p(p)
#define ldl_p(p) ldl_be_p(p)
#define ldq_p(p) ldq_be_p(p)
#define ldfl_p(p) ldfl_be_p(p)
#define ldfq_p(p) ldfq_be_p(p)
#define stw_p(p, v) stw_be_p(p, v)
#define stl_p(p, v) stl_be_p(p, v)
#define stq_p(p, v) stq_be_p(p, v)
#define stfl_p(p, v) stfl_be_p(p, v)
#define stfq_p(p, v) stfq_be_p(p, v)
#else
#define lduw_p(p) lduw_le_p(p)
#define ldsw_p(p) ldsw_le_p(p)
#define ldl_p(p) ldl_le_p(p)
#define ldq_p(p) ldq_le_p(p)
#define ldfl_p(p) ldfl_le_p(p)
#define ldfq_p(p) ldfq_le_p(p)
#define stw_p(p, v) stw_le_p(p, v)
#define stl_p(p, v) stl_le_p(p, v)
#define stq_p(p, v) stq_le_p(p, v)
#define stfl_p(p, v) stfl_le_p(p, v)
#define stfq_p(p, v) stfq_le_p(p, v)
#endif

/* MMU memory access macros */

#if defined(CONFIG_USER_ONLY)
#include <assert.h>
#include "exec/user/abitypes.h"

/* On some host systems the guest address space is reserved on the host.
 * This allows the guest address space to be offset to a convenient location.
 */
#if defined(CONFIG_USE_GUEST_BASE)
extern unsigned long guest_base;
extern int have_guest_base;
extern unsigned long reserved_va;
#define GUEST_BASE guest_base
#define RESERVED_VA reserved_va
#else
#define GUEST_BASE 0ul
#define RESERVED_VA 0ul
#endif

/* All direct uses of g2h and h2g need to go away for usermode softmmu.  */
#define g2h(x) ((void *)((unsigned long)(target_ulong)(x) + GUEST_BASE))

#if HOST_LONG_BITS <= TARGET_VIRT_ADDR_SPACE_BITS
#define h2g_valid(x) 1
#else
#define h2g_valid(x) ({ \
    unsigned long __guest = (unsigned long)(x) - GUEST_BASE; \
    (__guest < (1ul << TARGET_VIRT_ADDR_SPACE_BITS)) && \
    (!RESERVED_VA || (__guest < RESERVED_VA)); \
})
#endif

#define h2g_nocheck(x) ({ \
    unsigned long __ret = (unsigned long)(x) - GUEST_BASE; \
    (abi_ulong)__ret; \
})

#define h2g(x) ({ \
    /* Check if given address fits target address space */ \
    assert(h2g_valid(x)); \
    h2g_nocheck(x); \
})

#define saddr(x) g2h(x)
#define laddr(x) g2h(x)

#else /* !CONFIG_USER_ONLY */
/* NOTE: we use double casts if pointers and target_ulong have
   different sizes */
#define saddr(x) (uint8_t *)(intptr_t)(x)
#define laddr(x) (uint8_t *)(intptr_t)(x)
#endif

#define ldub_raw(p) ldub_p(laddr((p)))
#define ldsb_raw(p) ldsb_p(laddr((p)))
#define lduw_raw(p) lduw_p(laddr((p)))
#define ldsw_raw(p) ldsw_p(laddr((p)))
#define ldl_raw(p) ldl_p(laddr((p)))
#define ldq_raw(p) ldq_p(laddr((p)))
#define ldfl_raw(p) ldfl_p(laddr((p)))
#define ldfq_raw(p) ldfq_p(laddr((p)))
#define stb_raw(p, v) stb_p(saddr((p)), v)
#define stw_raw(p, v) stw_p(saddr((p)), v)
#define stl_raw(p, v) stl_p(saddr((p)), v)
#define stq_raw(p, v) stq_p(saddr((p)), v)
#define stfl_raw(p, v) stfl_p(saddr((p)), v)
#define stfq_raw(p, v) stfq_p(saddr((p)), v)


#if defined(CONFIG_USER_ONLY)

/* if user mode, no other memory access functions */
#define ldub(p) ldub_raw(p)
#define ldsb(p) ldsb_raw(p)
#define lduw(p) lduw_raw(p)
#define ldsw(p) ldsw_raw(p)
#define ldl(p) ldl_raw(p)
#define ldq(p) ldq_raw(p)
#define ldfl(p) ldfl_raw(p)
#define ldfq(p) ldfq_raw(p)
#define stb(p, v) stb_raw(p, v)
#define stw(p, v) stw_raw(p, v)
#define stl(p, v) stl_raw(p, v)
#define stq(p, v) stq_raw(p, v)
#define stfl(p, v) stfl_raw(p, v)
#define stfq(p, v) stfq_raw(p, v)

#define cpu_ldub_code(env1, p) ldub_raw(p)
#define cpu_ldsb_code(env1, p) ldsb_raw(p)
#define cpu_lduw_code(env1, p) lduw_raw(p)
#define cpu_ldsw_code(env1, p) ldsw_raw(p)
#define cpu_ldl_code(env1, p) ldl_raw(p)
#define cpu_ldq_code(env1, p) ldq_raw(p)

#define cpu_ldub_data(env, addr) ldub_raw(addr)
#define cpu_lduw_data(env, addr) lduw_raw(addr)
#define cpu_ldsw_data(env, addr) ldsw_raw(addr)
#define cpu_ldl_data(env, addr) ldl_raw(addr)
#define cpu_ldq_data(env, addr) ldq_raw(addr)

#define cpu_stb_data(env, addr, data) stb_raw(addr, data)
#define cpu_stw_data(env, addr, data) stw_raw(addr, data)
#define cpu_stl_data(env, addr, data) stl_raw(addr, data)
#define cpu_stq_data(env, addr, data) stq_raw(addr, data)

#define cpu_ldub_kernel(env, addr) ldub_raw(addr)
#define cpu_lduw_kernel(env, addr) lduw_raw(addr)
#define cpu_ldsw_kernel(env, addr) ldsw_raw(addr)
#define cpu_ldl_kernel(env, addr) ldl_raw(addr)
#define cpu_ldq_kernel(env, addr) ldq_raw(addr)

#define cpu_stb_kernel(env, addr, data) stb_raw(addr, data)
#define cpu_stw_kernel(env, addr, data) stw_raw(addr, data)
#define cpu_stl_kernel(env, addr, data) stl_raw(addr, data)
#define cpu_stq_kernel(env, addr, data) stq_raw(addr, data)

#define ldub_kernel(p) ldub_raw(p)
#define ldsb_kernel(p) ldsb_raw(p)
#define lduw_kernel(p) lduw_raw(p)
#define ldsw_kernel(p) ldsw_raw(p)
#define ldl_kernel(p) ldl_raw(p)
#define ldq_kernel(p) ldq_raw(p)
#define ldfl_kernel(p) ldfl_raw(p)
#define ldfq_kernel(p) ldfq_raw(p)
#define stb_kernel(p, v) stb_raw(p, v)
#define stw_kernel(p, v) stw_raw(p, v)
#define stl_kernel(p, v) stl_raw(p, v)
#define stq_kernel(p, v) stq_raw(p, v)
#define stfl_kernel(p, v) stfl_raw(p, v)
#define stfq_kernel(p, vt) stfq_raw(p, v)

#define cpu_ldub_data(env, addr) ldub_raw(addr)
#define cpu_lduw_data(env, addr) lduw_raw(addr)
#define cpu_ldl_data(env, addr) ldl_raw(addr)

#define cpu_stb_data(env, addr, data) stb_raw(addr, data)
#define cpu_stw_data(env, addr, data) stw_raw(addr, data)
#define cpu_stl_data(env, addr, data) stl_raw(addr, data)
#endif /* defined(CONFIG_USER_ONLY) */

/* page related stuff */

#define TARGET_PAGE_SIZE (1 << TARGET_PAGE_BITS)
#define TARGET_PAGE_MASK ~(TARGET_PAGE_SIZE - 1)
#define TARGET_PAGE_ALIGN(addr) (((addr) + TARGET_PAGE_SIZE - 1) & TARGET_PAGE_MASK)
#ifdef TARGET_X86_64
#define TARGET_PTE_MASK 0x7fffffffffffULL
#endif

/* ??? These should be the larger of uintptr_t and target_ulong.  */
extern uintptr_t qemu_real_host_page_size;
extern uintptr_t qemu_host_page_size;
extern uintptr_t qemu_host_page_mask;

#define HOST_PAGE_ALIGN(addr) (((addr) + qemu_host_page_size - 1) & qemu_host_page_mask)

/* same as PROT_xxx */
#define PAGE_READ      0x0001
#define PAGE_WRITE     0x0002
#define PAGE_EXEC      0x0004
#define PAGE_BITS      (PAGE_READ | PAGE_WRITE | PAGE_EXEC)
#define PAGE_VALID     0x0008
/* original state of the write flag (used when tracking self-modifying
   code */
#define PAGE_WRITE_ORG 0x0010
#if defined(CONFIG_BSD) && defined(CONFIG_USER_ONLY)
/* FIXME: Code that sets/uses this is broken and needs to go away.  */
#define PAGE_RESERVED  0x0020
#endif

#if defined(CONFIG_USER_ONLY)
void page_dump(FILE *f);

typedef int (*walk_memory_regions_fn)(void *, abi_ulong,
                                      abi_ulong, unsigned long);
int walk_memory_regions(void *, walk_memory_regions_fn);

int page_get_flags(target_ulong address);
void page_set_flags(target_ulong start, target_ulong end, int flags);
int page_check_range(target_ulong start, target_ulong len, int flags);
#endif

/* Flags for use in ENV->INTERRUPT_PENDING.

   The numbers assigned here are non-sequential in order to preserve
   binary compatibility with the vmstate dump.  Bit 0 (0x0001) was
   previously used for CPU_INTERRUPT_EXIT, and is cleared when loading
   the vmstate dump.  */

/* External hardware interrupt pending.  This is typically used for
   interrupts from devices.  */
#define CPU_INTERRUPT_HARD        0x0002

/* Exit the current TB.  This is typically used when some system-level device
   makes some change to the memory mapping.  E.g. the a20 line change.  */
#define CPU_INTERRUPT_EXITTB      0x0004

/* Halt the CPU.  */
#define CPU_INTERRUPT_HALT        0x0020

/* Debug event pending.  */
#define CPU_INTERRUPT_DEBUG       0x0080

/* Several target-specific external hardware interrupts.  Each target/cpu.h
   should define proper names based on these defines.  */
#define CPU_INTERRUPT_TGT_EXT_0   0x0008
#define CPU_INTERRUPT_TGT_EXT_1   0x0010
#define CPU_INTERRUPT_TGT_EXT_2   0x0040
#define CPU_INTERRUPT_TGT_EXT_3   0x0200
#define CPU_INTERRUPT_TGT_EXT_4   0x1000

/* Several target-specific internal interrupts.  These differ from the
   preceding target-specific interrupts in that they are intended to
   originate from within the cpu itself, typically in response to some
   instruction being executed.  These, therefore, are not masked while
   single-stepping within the debugger.  */
#define CPU_INTERRUPT_TGT_INT_0   0x0100
#define CPU_INTERRUPT_TGT_INT_1   0x0400
#define CPU_INTERRUPT_TGT_INT_2   0x0800
#define CPU_INTERRUPT_TGT_INT_3   0x2000

/* First unused bit: 0x4000.  */

/* The set of all bits that should be masked when single-stepping.  */
#define CPU_INTERRUPT_SSTEP_MASK \
    (CPU_INTERRUPT_HARD          \
     | CPU_INTERRUPT_TGT_EXT_0   \
     | CPU_INTERRUPT_TGT_EXT_1   \
     | CPU_INTERRUPT_TGT_EXT_2   \
     | CPU_INTERRUPT_TGT_EXT_3   \
     | CPU_INTERRUPT_TGT_EXT_4)

/* Breakpoint/watchpoint flags */
#define BP_MEM_READ           0x01
#define BP_MEM_WRITE          0x02
#define BP_MEM_ACCESS         (BP_MEM_READ | BP_MEM_WRITE)
#define BP_STOP_BEFORE_ACCESS 0x04
#define BP_WATCHPOINT_HIT     0x08
#define BP_GDB                0x10
#define BP_CPU                0x20

// several functions below depend on cpu.h
// cpu.h depends on CPU_INTERRUPT_* definitions (above)
#include "cpu.h"

int cpu_breakpoint_insert(CPUArchState *env, target_ulong pc, int flags,
                          CPUBreakpoint **breakpoint);
int cpu_breakpoint_remove(CPUArchState *env, target_ulong pc, int flags);
void cpu_breakpoint_remove_by_ref(CPUArchState *env, CPUBreakpoint *breakpoint);
void cpu_breakpoint_remove_all(CPUArchState *env, int mask);
int cpu_watchpoint_insert(CPUArchState *env, target_ulong addr, target_ulong len,
                          int flags, CPUWatchpoint **watchpoint);
int cpu_watchpoint_remove(CPUArchState *env, target_ulong addr,
                          target_ulong len, int flags);
void cpu_watchpoint_remove_by_ref(CPUArchState *env, CPUWatchpoint *watchpoint);
void cpu_watchpoint_remove_all(CPUArchState *env, int mask);

#define SSTEP_ENABLE  0x1  /* Enable simulated HW single stepping */
#define SSTEP_NOIRQ   0x2  /* Do not use IRQ while single stepping */
#define SSTEP_NOTIMER 0x4  /* Do not Timers while single stepping */

void QEMU_NORETURN cpu_abort(CPUArchState *env, const char *fmt, ...)
GCC_FMT_ATTR(2, 3);

void cpu_single_step(CPUState *cpu, int enabled);


/* IO ports API */
#include "exec/ioport.h"

/* Return the physical page corresponding to a virtual one. Use it
   only for debugging because no protection checks are done. Return -1
   if no page found. */
hwaddr cpu_get_phys_page_debug(CPUArchState *env, target_ulong addr);

/* memory API */

extern int phys_ram_fd;
extern ram_addr_t ram_size;

/* RAM is pre-allocated and passed into qemu_ram_alloc_from_ptr */
#define RAM_PREALLOC_MASK   (1 << 0)

typedef struct RAMBlock {
    uint8_t *host;
    ram_addr_t offset;
    ram_addr_t length;
    uint32_t flags;
    char idstr[256];
    /* Reads can take either the iothread or the ramlist lock.
     * Writes must take both locks.
     */
    QTAILQ_ENTRY(RAMBlock) next;
    int fd;
} RAMBlock;

#define DIRTY_MEMORY_VGA       0
#define DIRTY_MEMORY_CODE      1
#define DIRTY_MEMORY_MIGRATION 2
#define DIRTY_MEMORY_NUM       3        /* num of dirty bits */

typedef struct RAMList {
    QemuMutex mutex;
    unsigned long *dirty_memory[DIRTY_MEMORY_NUM];
    RAMBlock *mru_block;
    QTAILQ_HEAD(ram, RAMBlock) blocks;
    uint32_t version;
} RAMList;
extern RAMList ram_list;

extern const char *mem_path;
extern int mem_prealloc;

/* physical memory access */

/* Flags stored in the low bits of the TLB virtual address.  These are
   defined so that fast path ram access is all zeros.  */
/* Zero if TLB entry is valid.  */
#define TLB_INVALID_MASK   (1 << 3)
/* Set if TLB entry references a clean RAM page.  The iotlb entry will
   contain the page physical address.  */
#define TLB_NOTDIRTY    (1 << 4)
/* Set if TLB entry is an IO callback.  */
#define TLB_MMIO        (1 << 5)

#define VGA_DIRTY_FLAG       0x01
#define CODE_DIRTY_FLAG      0x02
#define MIGRATION_DIRTY_FLAG 0x08

void cpu_tlb_update_dirty(CPUArchState *env);

void dump_exec_info(FILE *f,
                    int (*cpu_fprintf)(FILE *f, const char *fmt, ...));

/* Coalesced MMIO regions are areas where write operations can be reordered.
 * This usually implies that write operations are side-effect free.  This allows
 * batching which can make a major impact on performance when using
 * virtualization.
 */
void qemu_register_coalesced_mmio(hwaddr addr, ram_addr_t size);

void qemu_unregister_coalesced_mmio(hwaddr addr, ram_addr_t size);

void qemu_flush_coalesced_mmio_buffer(void);

/* profiling */
#ifdef CONFIG_PROFILER
static inline int64_t profile_getclock(void)
{
    return cpu_get_real_ticks();
}

extern int64_t qemu_time, qemu_time_start;
extern int64_t tlb_flush_time;
extern int64_t dev_time;
#endif

int cpu_memory_rw_debug(CPUState *cpu, target_ulong addr,
                        void *buf, int len, int is_write);

void cpu_inject_x86_mce(CPUArchState *cenv, int bank, uint64_t status,
                        uint64_t mcg_status, uint64_t addr, uint64_t misc);

#endif /* CPU_ALL_H */
