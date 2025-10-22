#ifndef EMU_CPU_MEM_H
#define EMU_CPU_MEM_H

#include "misc.h"

// top 52 bits of a 64-bit address, i.e. address >> 12
// x86-64 uses 48-bit virtual addresses (256 TB), but we support full 52-bit physical
typedef qword_t page_t;
#define BAD_PAGE 0x10000000000000ULL

#ifndef __KERNEL__
#define PAGE_BITS 12
#undef PAGE_SIZE // defined in system headers somewhere
#define PAGE_SIZE (1 << PAGE_BITS)
#define PAGE(addr) ((addr) >> PAGE_BITS)
#define PGOFFSET(addr) ((addr) & (PAGE_SIZE - 1))
typedef qword_t pages_t;
// bytes MUST be unsigned if you would like this to overflow to zero
#define PAGE_ROUND_UP(bytes) (PAGE((bytes) + PAGE_SIZE - 1))
// x86-64: 48-bit addresses = 2^36 pages (but we'll use sparse page tables)
#define MEM_PAGES (1ULL << 36)
// For practical purposes, we'll support up to 512GB of addressable space initially
#define MEM_PAGES_INITIAL (1ULL << 27)  // 512GB / 4KB = 134M pages
#endif

struct mmu {
    struct mmu_ops *ops;
    struct asbestos *asbestos;
    uint64_t changes;
};

#define MEM_READ 0
#define MEM_WRITE 1
#define MEM_WRITE_PTRACE 2

struct mmu_ops {
    // type is MEM_READ or MEM_WRITE
    void *(*translate)(struct mmu *mmu, addr_t addr, int type);
};

static inline void *mmu_translate(struct mmu *mmu, addr_t addr, int type) {
    return mmu->ops->translate(mmu, addr, type);
}

#endif