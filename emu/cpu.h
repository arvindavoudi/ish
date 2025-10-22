#ifndef EMU_H
#define EMU_H

#include "misc.h"
#include "emu/mmu.h"
#include "emu/float80.h"

#ifdef __KERNEL__
#include <linux/stddef.h>
#else
#include <stddef.h>
#endif

struct cpu_state;
struct tlb;
int cpu_run_to_interrupt(struct cpu_state *cpu, struct tlb *tlb);
void cpu_poke(struct cpu_state *cpu);

union mm_reg {
    qword_t qw;
    dword_t dw[2];
};
union xmm_reg {
    unsigned __int128 u128;
    qword_t qw[2];
    uint32_t u32[4];
    uint16_t u16[8];
    uint8_t u8[16];
    float f32[4];
    double f64[2];
};
static_assert(sizeof(union xmm_reg) == 16, "xmm_reg size");
static_assert(sizeof(union mm_reg) == 8, "mm_reg size");

struct cpu_state {
    struct mmu *mmu;
    long cycle;

    // general registers - x86-64 has 16 64-bit registers
    // assumes little endian (as does literally everything)
#define _REG64(n) \
    union { \
        qword_t r##n; \
        dword_t e##n; \
        word_t n; \
    }
#define _REG64X(n) \
    union { \
        qword_t r##n##x; \
        dword_t e##n##x; \
        word_t n##x; \
        struct { \
            byte_t n##l; \
            byte_t n##h; \
        }; \
    }
#define _REG64_SIMPLE(n) \
    union { \
        qword_t r##n; \
        dword_t r##n##d; \
        word_t r##n##w; \
        byte_t r##n##b; \
    }

    union {
        struct {
            _REG64X(a);     // rax/eax/ax/al/ah
            _REG64X(c);     // rcx/ecx/cx/cl/ch
            _REG64X(d);     // rdx/edx/dx/dl/dh
            _REG64X(b);     // rbx/ebx/bx/bl/bh
            _REG64(sp);     // rsp/esp/sp
            _REG64(bp);     // rbp/ebp/bp
            _REG64(si);     // rsi/esi/si
            _REG64(di);     // rdi/edi/di
            _REG64_SIMPLE(8);   // r8/r8d/r8w/r8b
            _REG64_SIMPLE(9);   // r9/r9d/r9w/r9b
            _REG64_SIMPLE(10);  // r10/r10d/r10w/r10b
            _REG64_SIMPLE(11);  // r11/r11d/r11w/r11b
            _REG64_SIMPLE(12);  // r12/r12d/r12w/r12b
            _REG64_SIMPLE(13);  // r13/r13d/r13w/r13b
            _REG64_SIMPLE(14);  // r14/r14d/r14w/r14b
            _REG64_SIMPLE(15);  // r15/r15d/r15w/r15b
        };
        qword_t regs[16];
    };
#undef _REG64_SIMPLE
#undef _REG64X
#undef _REG64

    qword_t rip;  // 64-bit instruction pointer

    // flags - x86-64 uses RFLAGS (64-bit)
    union {
        qword_t rflags;
        dword_t eflags;  // Lower 32 bits for compatibility
        struct {
            bitfield cf_bit:1;
            bitfield pad1_1:1;
            bitfield pf:1;
            bitfield pad2_0:1;
            bitfield af:1;
            bitfield pad3_0:1;
            bitfield zf:1;
            bitfield sf:1;
            bitfield tf:1;
            bitfield if_:1;
            bitfield df:1;
            bitfield of_bit:1;
            bitfield iopl:2;
        };
        // for asm
#define PF_FLAG (1 << 2)
#define AF_FLAG (1 << 4)
#define ZF_FLAG (1 << 6)
#define SF_FLAG (1 << 7)
#define DF_FLAG (1 << 10)
    };
    // please pretend this doesn't exist
    dword_t df_offset;
    // for maximum efficiency these are stored in bytes
    byte_t cf;
    byte_t of;
    // whether the true flag values are in the above struct, or computed from
    // the stored result and operands
    dword_t res, op1, op2;
    union {
        struct {
            bitfield pf_res:1;
            bitfield zf_res:1;
            bitfield sf_res:1;
            bitfield af_ops:1;
        };
        // for asm
#define PF_RES (1 << 0)
#define ZF_RES (1 << 1)
#define SF_RES (1 << 2)
#define AF_OPS (1 << 3)
        byte_t flags_res;
    };

    union mm_reg mm[8];
    union xmm_reg xmm[16];  // x86-64 has 16 XMM registers

    // fpu
    float80 fp[8];
    union {
        word_t fsw;
        struct {
            bitfield ie:1; // invalid operation
            bitfield de:1; // denormalized operand
            bitfield ze:1; // divide by zero
            bitfield oe:1; // overflow
            bitfield ue:1; // underflow
            bitfield pe:1; // precision
            bitfield stf:1; // stack fault
            bitfield es:1; // exception status
            bitfield c0:1;
            bitfield c1:1;
            bitfield c2:1;
            unsigned top:3;
            bitfield c3:1;
            bitfield b:1; // fpu busy (?)
        };
    };
    union {
        word_t fcw;
        struct {
            bitfield im:1;
            bitfield dm:1;
            bitfield zm:1;
            bitfield om:1;
            bitfield um:1;
            bitfield pm:1;
            bitfield pad4:2;
            bitfield pc:2;
            bitfield rc:2;
            bitfield y:1;
        };
    };

    // TLS bullshit
    word_t gs;
    addr_t tls_ptr;

    // for the page fault handler
    addr_t segfault_addr;
    bool segfault_was_write;

    dword_t trapno;
    // access atomically
    bool *poked_ptr;
    bool _poked;
};

// Backward compatibility aliases for 32-bit register names
#define eax eax
#define ebx ebx
#define ecx ecx
#define edx edx
#define esi esi
#define edi edi
#define esp esp
#define ebp ebp
#define eip rip  // Map eip to rip for compatibility

#define CPU_OFFSET(field) offsetof(struct cpu_state, field)

// Verify register layout matches array indices
static_assert(CPU_OFFSET(rax) == CPU_OFFSET(regs[0]), "register order");
static_assert(CPU_OFFSET(rcx) == CPU_OFFSET(regs[1]), "register order");
static_assert(CPU_OFFSET(rdx) == CPU_OFFSET(regs[2]), "register order");
static_assert(CPU_OFFSET(rbx) == CPU_OFFSET(regs[3]), "register order");
static_assert(CPU_OFFSET(rsp) == CPU_OFFSET(regs[4]), "register order");
static_assert(CPU_OFFSET(rbp) == CPU_OFFSET(regs[5]), "register order");
static_assert(CPU_OFFSET(rsi) == CPU_OFFSET(regs[6]), "register order");
static_assert(CPU_OFFSET(rdi) == CPU_OFFSET(regs[7]), "register order");
static_assert(CPU_OFFSET(r8) == CPU_OFFSET(regs[8]), "register order");
static_assert(CPU_OFFSET(r9) == CPU_OFFSET(regs[9]), "register order");
static_assert(CPU_OFFSET(r10) == CPU_OFFSET(regs[10]), "register order");
static_assert(CPU_OFFSET(r11) == CPU_OFFSET(regs[11]), "register order");
static_assert(CPU_OFFSET(r12) == CPU_OFFSET(regs[12]), "register order");
static_assert(CPU_OFFSET(r13) == CPU_OFFSET(regs[13]), "register order");
static_assert(CPU_OFFSET(r14) == CPU_OFFSET(regs[14]), "register order");
static_assert(CPU_OFFSET(r15) == CPU_OFFSET(regs[15]), "register order");
static_assert(sizeof(struct cpu_state) < 0x1ffff, "cpu struct is too big for vector gadgets");

// flags
#define ZF (cpu->zf_res ? cpu->res == 0 : cpu->zf)
#define SF (cpu->sf_res ? (int32_t) cpu->res < 0 : cpu->sf)
#define CF (cpu->cf)
#define OF (cpu->of)
#define PF (cpu->pf_res ? !__builtin_parity(cpu->res & 0xff) : cpu->pf)
#define AF (cpu->af_ops ? ((cpu->op1 ^ cpu->op2 ^ cpu->res) >> 4) & 1 : cpu->af)

static inline void collapse_flags(struct cpu_state *cpu) {
    cpu->zf = ZF;
    cpu->sf = SF;
    cpu->pf = PF;
    cpu->zf_res = cpu->sf_res = cpu->pf_res = 0;
    cpu->of_bit = cpu->of;
    cpu->cf_bit = cpu->cf;
    cpu->af = AF;
    cpu->af_ops = 0;
    cpu->pad1_1 = 1;
    cpu->pad2_0 = cpu->pad3_0 = 0;
    cpu->if_ = 1;
}

static inline void expand_flags(struct cpu_state *cpu) {
    cpu->of = cpu->of_bit;
    cpu->cf = cpu->cf_bit;
    cpu->zf_res = cpu->sf_res = cpu->pf_res = cpu->af_ops = 0;
}

// x86-64 register enumeration (16 registers)
enum reg64 {
    reg_rax = 0, reg_rcx, reg_rdx, reg_rbx, reg_rsp, reg_rbp, reg_rsi, reg_rdi,
    reg_r8, reg_r9, reg_r10, reg_r11, reg_r12, reg_r13, reg_r14, reg_r15,
    reg_count = 16,
    reg_none = reg_count,
    // Backward compatibility aliases
    reg_eax = reg_rax, reg_ecx = reg_rcx, reg_edx = reg_rdx, reg_ebx = reg_rbx,
    reg_esp = reg_rsp, reg_ebp = reg_rbp, reg_esi = reg_rsi, reg_edi = reg_rdi,
};
typedef enum reg64 reg32;  // For backward compatibility

static inline const char *reg64_name(enum reg64 reg) {
    switch (reg) {
        case reg_rax: return "rax";
        case reg_rcx: return "rcx";
        case reg_rdx: return "rdx";
        case reg_rbx: return "rbx";
        case reg_rsp: return "rsp";
        case reg_rbp: return "rbp";
        case reg_rsi: return "rsi";
        case reg_rdi: return "rdi";
        case reg_r8: return "r8";
        case reg_r9: return "r9";
        case reg_r10: return "r10";
        case reg_r11: return "r11";
        case reg_r12: return "r12";
        case reg_r13: return "r13";
        case reg_r14: return "r14";
        case reg_r15: return "r15";
        default: return "?";
    }
}

static inline const char *reg32_name(enum reg64 reg) {
    switch (reg) {
        case reg_rax: return "eax";
        case reg_rcx: return "ecx";
        case reg_rdx: return "edx";
        case reg_rbx: return "ebx";
        case reg_rsp: return "esp";
        case reg_rbp: return "ebp";
        case reg_rsi: return "esi";
        case reg_rdi: return "edi";
        case reg_r8: return "r8d";
        case reg_r9: return "r9d";
        case reg_r10: return "r10d";
        case reg_r11: return "r11d";
        case reg_r12: return "r12d";
        case reg_r13: return "r13d";
        case reg_r14: return "r14d";
        case reg_r15: return "r15d";
        default: return "?";
    }
}

#endif
