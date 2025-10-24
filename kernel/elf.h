#ifndef ELF_H
#define ELF_H

#include "misc.h"

#define ELF_MAGIC "\177ELF"
#define ELF_32BIT 1
#define ELF_64BIT 2
#define ELF_LITTLEENDIAN 1
#define ELF_BIGENDIAN 2
#define ELF_LINUX_ABI 3
#define ELF_EXECUTABLE 2
#define ELF_DYNAMIC 3
#define ELF_X86 3
#define ELF_X86_64 62  // x86-64 machine type

// ELF32 header (32-bit addresses and offsets)
struct elf32_header {
    uint32_t magic;
    byte_t bitness;
    byte_t endian;
    byte_t elfversion1;
    byte_t abi;
    byte_t abi_version;
    byte_t padding[7];
    uint16_t type;
    uint16_t machine;
    uint32_t elfversion2;
    dword_t entry_point;     // 32-bit entry point
    dword_t prghead_off;     // 32-bit program header offset
    dword_t secthead_off;    // 32-bit section header offset
    uint32_t flags;
    uint16_t header_size;
    uint16_t phent_size;
    uint16_t phent_count;
    uint16_t shent_size;
    uint16_t shent_count;
    uint16_t sectname_index;
};

// ELF64 header (64-bit addresses and offsets)
struct elf64_header {
    uint32_t magic;
    byte_t bitness;
    byte_t endian;
    byte_t elfversion1;
    byte_t abi;
    byte_t abi_version;
    byte_t padding[7];
    uint16_t type;
    uint16_t machine;
    uint32_t elfversion2;
    qword_t entry_point;     // 64-bit entry point
    qword_t prghead_off;     // 64-bit program header offset
    qword_t secthead_off;    // 64-bit section header offset
    uint32_t flags;
    uint16_t header_size;
    uint16_t phent_size;
    uint16_t phent_count;
    uint16_t shent_size;
    uint16_t shent_count;
    uint16_t sectname_index;
};

// Generic header structure for code that needs to work with both
struct elf_header {
    uint32_t magic;
    byte_t bitness;
    byte_t endian;
    byte_t elfversion1;
    byte_t abi;
    byte_t abi_version;
    byte_t padding[7];
    uint16_t type;
    uint16_t machine;
    uint32_t elfversion2;
    qword_t entry_point;     // Always use 64-bit for uniform access
    qword_t prghead_off;     // Always use 64-bit for uniform access
    qword_t secthead_off;    // Always use 64-bit for uniform access
    uint32_t flags;
    uint16_t header_size;
    uint16_t phent_size;
    uint16_t phent_count;
    uint16_t shent_size;
    uint16_t shent_count;
    uint16_t sectname_index;
};

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_TLS 7
#define PT_NUM 8

// ELF32 program header
struct prg32_header {
    uint32_t type;
    dword_t offset;          // 32-bit file offset
    dword_t vaddr;           // 32-bit virtual address
    dword_t paddr;           // 32-bit physical address
    dword_t filesize;        // 32-bit file size
    dword_t memsize;         // 32-bit memory size
    uint32_t flags;
    dword_t alignment;       // 32-bit alignment
};

// ELF64 program header
struct prg64_header {
    uint32_t type;
    uint32_t flags;          // In ELF64, flags come before offsets
    qword_t offset;          // 64-bit file offset
    qword_t vaddr;           // 64-bit virtual address
    qword_t paddr;           // 64-bit physical address
    qword_t filesize;        // 64-bit file size
    qword_t memsize;         // 64-bit memory size
    qword_t alignment;       // 64-bit alignment
};

// Generic program header for code that needs to work with both
struct prg_header {
    uint32_t type;
    uint32_t flags;
    qword_t offset;          // Always use 64-bit for uniform access
    qword_t vaddr;           // Always use 64-bit for uniform access
    qword_t paddr;           // Always use 64-bit for uniform access
    qword_t filesize;        // Always use 64-bit for uniform access
    qword_t memsize;         // Always use 64-bit for uniform access
    qword_t alignment;       // Always use 64-bit for uniform access
};

#define PH_R (1 << 2)
#define PH_W (1 << 1)
#define PH_X (1 << 0)

// ELF32 auxiliary vector entry
struct aux32_ent {
    dword_t type;
    dword_t value;
};

// ELF64 auxiliary vector entry
struct aux64_ent {
    qword_t type;
    qword_t value;
};

// Generic auxiliary vector entry (for internal use)
struct aux_ent {
    qword_t type;   // Always use 64-bit internally
    qword_t value;  // Always use 64-bit internally
};

#define AX_PHDR 3
#define AX_PHENT 4
#define AX_PHNUM 5
#define AX_PAGESZ 6
#define AX_BASE 7
#define AX_FLAGS 8
#define AX_ENTRY 9
#define AX_UID 11
#define AX_EUID 12
#define AX_GID 13
#define AX_EGID 14
#define AX_PLATFORM 15
#define AX_HWCAP 16
#define AX_CLKTCK 17
#define AX_SECURE 23
#define AX_RANDOM 25
#define AX_HWCAP2 26
#define AX_EXECFN 31
#define AX_SYSINFO 32
#define AX_SYSINFO_EHDR 33

struct dyn_ent {
    qword_t tag;    // 64-bit dynamic entry tag
    qword_t val;    // 64-bit dynamic entry value
};

#define DT_NULL 0
#define DT_HASH 4
#define DT_STRTAB 5
#define DT_SYMTAB 6

struct elf_sym {
    uint32_t name;
    byte_t info;
    byte_t other;
    uint16_t shndx;
    qword_t value;  // 64-bit symbol value
    qword_t size;   // 64-bit symbol size
};

#endif
