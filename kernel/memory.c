#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define DEFAULT_CHANNEL memory
#include "debug.h"
#include "kernel/errno.h"
#include "kernel/signal.h"
#include "kernel/memory.h"
#include "asbestos/asbestos.h"
#include "kernel/vdso.h"
#include "kernel/task.h"
#include "fs/fd.h"

// increment the change count
static void mem_changed(struct mem *mem);
static struct mmu_ops mem_mmu_ops;

void mem_init(struct mem *mem) {
    // Allocate level 1 page directory (65K entries, 512KB)
    mem->pgdir = calloc(MEM_PGDIR_L1_SIZE, sizeof(void **));
    mem->pgdir_used = 0;
    mem->mmu.ops = &mem_mmu_ops;
    mem->mmu.asbestos = asbestos_new(&mem->mmu);
    mem->mmu.changes = 0;
    wrlock_init(&mem->lock);
}

void mem_destroy(struct mem *mem) {
    write_wrlock(&mem->lock);
    pt_unmap_always(mem, 0, MEM_PAGES);
    asbestos_free(mem->mmu.asbestos);

    // Free all 3 levels of page directory
    for (size_t i = 0; i < MEM_PGDIR_L1_SIZE; i++) {
        if (mem->pgdir[i] != NULL) {
            void **l2_dir = (void **)mem->pgdir[i];
            for (size_t j = 0; j < MEM_PGDIR_L2_SIZE; j++) {
                if (l2_dir[j] != NULL) {
                    free(l2_dir[j]);  // Free L3 (pt_entry array)
                }
            }
            free(l2_dir);  // Free L2
        }
    }
    free(mem->pgdir);  // Free L1

    write_wrunlock(&mem->lock);
    wrlock_destroy(&mem->lock);
}

// 3-level page directory indexing
// Page number (36 bits): [L1: 16 bits][L2: 10 bits][L3: 10 bits]
#define PGDIR_L1(page) ((page) >> (MEM_PGDIR_L2_BITS + MEM_PGDIR_L3_BITS))
#define PGDIR_L2(page) (((page) >> MEM_PGDIR_L3_BITS) & (MEM_PGDIR_L2_SIZE - 1))
#define PGDIR_L3(page) ((page) & (MEM_PGDIR_L3_SIZE - 1))

// Backwards compatibility (for old 2-level code)
#define PGDIR_TOP(page) PGDIR_L1(page)
#define PGDIR_BOTTOM(page) PGDIR_L3(page)

static struct pt_entry *mem_pt_new(struct mem *mem, page_t page) {
    size_t l1_idx = PGDIR_L1(page);
    size_t l2_idx = PGDIR_L2(page);
    size_t l3_idx = PGDIR_L3(page);

    // Allocate L2 if needed
    void **l2_dir = (void **)mem->pgdir[l1_idx];
    if (l2_dir == NULL) {
        l2_dir = (void **)calloc(MEM_PGDIR_L2_SIZE, sizeof(void *));
        mem->pgdir[l1_idx] = l2_dir;
    }

    // Allocate L3 if needed
    struct pt_entry *l3_dir = (struct pt_entry *)l2_dir[l2_idx];
    if (l3_dir == NULL) {
        l3_dir = (struct pt_entry *)calloc(MEM_PGDIR_L3_SIZE, sizeof(struct pt_entry));
        l2_dir[l2_idx] = l3_dir;
        mem->pgdir_used++;
    }

    return &l3_dir[l3_idx];
}

struct pt_entry *mem_pt(struct mem *mem, page_t page) {
    size_t l1_idx = PGDIR_L1(page);
    size_t l2_idx = PGDIR_L2(page);
    size_t l3_idx = PGDIR_L3(page);

    // Traverse L1
    void **l2_dir = (void **)mem->pgdir[l1_idx];
    if (l2_dir == NULL)
        return NULL;

    // Traverse L2
    struct pt_entry *l3_dir = (struct pt_entry *)l2_dir[l2_idx];
    if (l3_dir == NULL)
        return NULL;

    // Get entry from L3
    struct pt_entry *entry = &l3_dir[l3_idx];
    if (entry->data == NULL)
        return NULL;

    return entry;
}

static void mem_pt_del(struct mem *mem, page_t page) {
    struct pt_entry *entry = mem_pt(mem, page);
    if (entry != NULL)
        entry->data = NULL;
}

void mem_next_page(struct mem *mem, page_t *page) {
    (*page)++;
    if (*page >= MEM_PAGES)
        return;

    // Skip unallocated L2 and L3 directories
    while (*page < MEM_PAGES) {
        size_t l1_idx = PGDIR_L1(*page);
        size_t l2_idx = PGDIR_L2(*page);
        size_t l3_idx = PGDIR_L3(*page);

        // If L2 doesn't exist, skip to next L2
        void **l2_dir = (void **)mem->pgdir[l1_idx];
        if (l2_dir == NULL) {
            // Skip to next L1 entry
            *page = (*page - (l2_idx << MEM_PGDIR_L3_BITS) - l3_idx) + (1ULL << (MEM_PGDIR_L2_BITS + MEM_PGDIR_L3_BITS));
            continue;
        }

        // If L3 doesn't exist, skip to next L3
        struct pt_entry *l3_dir = (struct pt_entry *)l2_dir[l2_idx];
        if (l3_dir == NULL) {
            // Skip to next L2 entry
            *page = (*page - l3_idx) + MEM_PGDIR_L3_SIZE;
            continue;
        }

        // L3 exists, so we can proceed
        break;
    }
}

page_t pt_find_hole(struct mem *mem, pages_t size) {
    page_t hole_end = 0; // this can never be used before initializing but gcc doesn't realize
    bool in_hole = false;
    for (page_t page = 0xf7ffd; page > 0x40000; page--) {
        // I don't know how this works but it does
        if (!in_hole && mem_pt(mem, page) == NULL) {
            in_hole = true;
            hole_end = page + 1;
        }
        if (mem_pt(mem, page) != NULL)
            in_hole = false;
        else if (hole_end - page == size)
            return page;
    }
    return BAD_PAGE;
}

bool pt_is_hole(struct mem *mem, page_t start, pages_t pages) {
    for (page_t page = start; page < start + pages; page++) {
        if (mem_pt(mem, page) != NULL)
            return false;
    }
    return true;
}

int pt_map(struct mem *mem, page_t start, pages_t pages, void *memory, size_t offset, unsigned flags) {
    if (memory == MAP_FAILED)
        return errno_map();

    // If this fails, the munmap in pt_unmap would probably fail.
    assert((uintptr_t) memory % real_page_size == 0 || memory == vdso_data);

    struct data *data = malloc(sizeof(struct data));
    if (data == NULL)
        return _ENOMEM;
    *data = (struct data) {
        .data = memory,
        .size = pages * PAGE_SIZE + offset,

#if LEAK_DEBUG
        .pid = current ? current->pid : 0,
        .dest = start << PAGE_BITS,
#endif
    };

    for (page_t page = start; page < start + pages; page++) {
        if (mem_pt(mem, page) != NULL)
            pt_unmap(mem, page, 1);
        data->refcount++;
        struct pt_entry *pt = mem_pt_new(mem, page);
        pt->data = data;
        pt->offset = ((page - start) << PAGE_BITS) + offset;
        pt->flags = flags;
    }
    return 0;
}

int pt_unmap(struct mem *mem, page_t start, pages_t pages) {
    for (page_t page = start; page < start + pages; page++)
        if (mem_pt(mem, page) == NULL)
            return -1;
    return pt_unmap_always(mem, start, pages);
}

int pt_unmap_always(struct mem *mem, page_t start, pages_t pages) {
    for (page_t page = start; page < start + pages; mem_next_page(mem, &page)) {
        struct pt_entry *pt = mem_pt(mem, page);
        if (pt == NULL)
            continue;
        asbestos_invalidate_page(mem->mmu.asbestos, page);
        struct data *data = pt->data;
        mem_pt_del(mem, page);
        if (--data->refcount == 0) {
            // vdso wasn't allocated with mmap, it's just in our data segment
            if (data->data != vdso_data) {
                int err = munmap(data->data, data->size);
                if (err != 0)
                    die("munmap(%p, %lu) failed: %s", data->data, data->size, strerror(errno));
            }
            if (data->fd != NULL) {
                fd_close(data->fd);
            }
            free(data);
        }
    }
    mem_changed(mem);
    return 0;
}

int pt_map_nothing(struct mem *mem, page_t start, pages_t pages, unsigned flags) {
    if (pages == 0) return 0;
    void *memory = mmap(NULL, pages * PAGE_SIZE,
            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    return pt_map(mem, start, pages, memory, 0, flags | P_ANONYMOUS);
}

int pt_set_flags(struct mem *mem, page_t start, pages_t pages, int flags) {
    for (page_t page = start; page < start + pages; page++)
        if (mem_pt(mem, page) == NULL)
            return _ENOMEM;
    for (page_t page = start; page < start + pages; page++) {
        struct pt_entry *entry = mem_pt(mem, page);
        int old_flags = entry->flags;
        entry->flags = flags;
        // check if protection is increasing
        if ((flags & ~old_flags) & (P_READ|P_WRITE)) {
            void *data = (char *) entry->data->data + entry->offset;
            // force to be page aligned
            data = (void *) ((uintptr_t) data & ~(real_page_size - 1));
            int prot = PROT_READ;
            if (flags & P_WRITE) prot |= PROT_WRITE;
            if (mprotect(data, real_page_size, prot) < 0)
                return errno_map();
        }
    }
    mem_changed(mem);
    return 0;
}

int pt_copy_on_write(struct mem *src, struct mem *dst, page_t start, page_t pages) {
    for (page_t page = start; page < start + pages; mem_next_page(src, &page)) {
        struct pt_entry *entry = mem_pt(src, page);
        if (entry == NULL)
            continue;
        if (pt_unmap_always(dst, page, 1) < 0)
            return -1;
        if (!(entry->flags & P_SHARED))
            entry->flags |= P_COW;
        entry->data->refcount++;
        struct pt_entry *dst_entry = mem_pt_new(dst, page);
        dst_entry->data = entry->data;
        dst_entry->offset = entry->offset;
        dst_entry->flags = entry->flags;
    }
    mem_changed(src);
    mem_changed(dst);
    return 0;
}

static void mem_changed(struct mem *mem) {
    mem->mmu.changes++;
}

// This version will return NULL instead of making necessary pagetable changes.
// Used by the emulator to avoid deadlocks.
static void *mem_ptr_nofault(struct mem *mem, addr_t addr, int type) {
    page_t page = PAGE(addr);

    // Validate page number is within supported range
    if (page >= MEM_PAGES) {
        static int warned = 0;
        if (!warned) {
            printk("mem_ptr_nofault: INVALID ADDRESS - page 0x%llx (addr=0x%llx) exceeds MEM_PAGES 0x%llx\n",
                   (unsigned long long)page, (unsigned long long)addr, (unsigned long long)MEM_PAGES);
            warned = 1;
        }
        return NULL;
    }

    struct pt_entry *entry = mem_pt(mem, page);
    if (entry == NULL)
        return NULL;
    if (type == MEM_WRITE && !P_WRITABLE(entry->flags))
        return NULL;
    return entry->data->data + entry->offset + PGOFFSET(addr);
}

void *mem_ptr(struct mem *mem, addr_t addr, int type) {
    void *old_ptr = mem_ptr_nofault(mem, addr, type); // just for an assert

    page_t page = PAGE(addr);
    struct pt_entry *entry = mem_pt(mem, page);

    if (entry == NULL) {
        // page does not exist
        // look to see if the next VM region is willing to grow down
        page_t p = page + 1;
        while (p < MEM_PAGES && mem_pt(mem, p) == NULL)
            p++;
        if (p >= MEM_PAGES)
            return NULL;
        if (!(mem_pt(mem, p)->flags & P_GROWSDOWN))
            return NULL;

        // Changing memory maps must be done with the write lock. But this is
        // called with the read lock.
        // This locking stuff is copy/pasted for all the code in this function
        // which changes memory maps.
        // TODO: factor the lock/unlock code here into a new function. Do this
        // next time you touch this function.
        read_wrunlock(&mem->lock);
        write_wrlock(&mem->lock);
        pt_map_nothing(mem, page, 1, P_WRITE | P_GROWSDOWN);
        write_wrunlock(&mem->lock);
        read_wrlock(&mem->lock);

        entry = mem_pt(mem, page);
    }

    if (entry != NULL && (type == MEM_WRITE || type == MEM_WRITE_PTRACE)) {
        // if page is unwritable, well tough luck
        if (type != MEM_WRITE_PTRACE && !(entry->flags & P_WRITE))
            return NULL;
        if (type == MEM_WRITE_PTRACE) {
            // TODO: Is P_WRITE really correct? The page shouldn't be writable without ptrace.
            entry->flags |= P_WRITE | P_COW;
        }
        // get rid of any compiled blocks in this page
        asbestos_invalidate_page(mem->mmu.asbestos, page);
        // if page is cow, ~~milk~~ copy it
        if (entry->flags & P_COW) {
            void *data = (char *) entry->data->data + entry->offset;
            void *copy = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

            // copy/paste from above
            read_wrunlock(&mem->lock);
            write_wrlock(&mem->lock);
            memcpy(copy, data, PAGE_SIZE);
            pt_map(mem, page, 1, copy, 0, entry->flags &~ P_COW);
            write_wrunlock(&mem->lock);
            read_wrlock(&mem->lock);
        }
    }

    void *ptr = mem_ptr_nofault(mem, addr, type);
    assert(old_ptr == NULL || old_ptr == ptr || type == MEM_WRITE_PTRACE);
    return ptr;
}

static void *mem_mmu_translate(struct mmu *mmu, addr_t addr, int type) {
    return mem_ptr_nofault(container_of(mmu, struct mem, mmu), addr, type);
}

static struct mmu_ops mem_mmu_ops = {
    .translate = mem_mmu_translate,
};

int mem_segv_reason(struct mem *mem, addr_t addr) {
    struct pt_entry *pt = mem_pt(mem, PAGE(addr));
    if (pt == NULL)
        return SEGV_MAPERR_;
    return SEGV_ACCERR_;
}

size_t real_page_size;
__attribute__((constructor)) static void get_real_page_size() {
    real_page_size = sysconf(_SC_PAGESIZE);
}

void mem_coredump(struct mem *mem, const char *file) {
    int fd = open(file, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        perror("open");
        return;
    }
    if (ftruncate(fd, 0xffffffff) < 0) {
        perror("ftruncate");
        return;
    }

    int pages = 0;
    for (page_t page = 0; page < MEM_PAGES; page++) {
        struct pt_entry *entry = mem_pt(mem, page);
        if (entry == NULL)
            continue;
        pages++;
        if (lseek(fd, page << PAGE_BITS, SEEK_SET) < 0) {
            perror("lseek");
            return;
        }
        if (write(fd, entry->data->data, PAGE_SIZE) < 0) {
            perror("write");
            return;
        }
    }
    printk("dumped %d pages\n", pages);
    close(fd);
}
