# iSH 32-bit to 64-bit Migration Guide

## Overview

This guide provides complete, reproducible steps to transform the iSH terminal emulator from a 32-bit x86 architecture to a full 64-bit x86-64 architecture. This transformation enables iSH to run modern 64-bit Linux binaries on iOS.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Phase 1: Core Infrastructure (COMPLETED)](#phase-1-core-infrastructure-completed)
3. [Phase 2: Instruction Decoder Updates (REQUIRED)](#phase-2-instruction-decoder-updates-required)
4. [Phase 3: JIT Compiler Updates (REQUIRED)](#phase-3-jit-compiler-updates-required)
5. [Phase 4: System Call Handler (REQUIRED)](#phase-4-system-call-handler-required)
6. [Phase 5: Testing and Validation (REQUIRED)](#phase-5-testing-and-validation-required)
7. [Complete File Change Summary](#complete-file-change-summary)

---

## Architecture Overview

### x86 vs x86-64 Key Differences

| Feature | x86 (32-bit) | x86-64 (64-bit) |
|---------|-------------|----------------|
| **Registers** | 8 GPRs (EAX-EDI) | 16 GPRs (RAX-R15) |
| **Register Size** | 32-bit | 64-bit |
| **Address Space** | 4 GB (32-bit) | 256 TB (48-bit virtual) |
| **Instruction Pointer** | EIP | RIP |
| **XMM Registers** | 8 (XMM0-XMM7) | 16 (XMM0-XMM15) |
| **REX Prefix** | N/A | 0x40-0x4F (enables 64-bit ops) |
| **System Calls** | INT 0x80 | SYSCALL instruction |
| **Syscall Args** | EBX, ECX, EDX, ESI, EDI, EBP | RDI, RSI, RDX, R10, R8, R9 |
| **Addressing Modes** | 32-bit | 32-bit + RIP-relative |
| **ELF Format** | ELF32 | ELF64 |

---

## Phase 1: Core Infrastructure (COMPLETED ✅)

These changes have been implemented and committed:

### 1.1 Type System (`misc.h`)

**Changes Made:**
```c
// Before (32-bit):
typedef dword_t addr_t;  // 32-bit address
typedef dword_t uint_t;
typedef sdword_t int_t;
typedef dword_t time_t_;

// After (64-bit):
typedef qword_t addr_t;  // 64-bit address
typedef qword_t uint_t;
typedef sqword_t int_t;
typedef qword_t time_t_;
typedef qword_t clock_t_;
```

**File:** `/home/user/ish/misc.h` (Lines 112-122)

### 1.2 CPU State (`emu/cpu.h`)

**Changes Made:**

1. **Extended Register Set:**
   - Added 8 new registers: R8-R15
   - All registers now support 64-bit access (RAX, RBX, etc.)
   - Maintained backward compatibility (EAX still works)

2. **Register Structure:**
```c
// Now supports 16 64-bit registers:
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
qword_t rip;  // 64-bit instruction pointer
```

3. **Extended XMM Registers:**
```c
union xmm_reg xmm[16];  // Increased from 8 to 16
```

4. **Updated Register Enum:**
```c
enum reg64 {
    reg_rax = 0, reg_rcx, reg_rdx, reg_rbx,
    reg_rsp, reg_rbp, reg_rsi, reg_rdi,
    reg_r8, reg_r9, reg_r10, reg_r11,
    reg_r12, reg_r13, reg_r14, reg_r15,
    reg_count = 16,
    reg_none = reg_count,
};
```

**File:** `/home/user/ish/emu/cpu.h` (Lines 39-90, 142, 246-300)

### 1.3 Memory Management (`emu/mmu.h`)

**Changes Made:**
```c
// Before:
typedef dword_t page_t;           // 20 bits
#define MEM_PAGES (1 << 20)       // 1M pages = 4GB

// After:
typedef qword_t page_t;           // 52 bits
#define MEM_PAGES (1ULL << 36)    // 64M pages = 256TB
#define MEM_PAGES_INITIAL (1ULL << 27)  // 512GB initial
```

**File:** `/home/user/ish/emu/mmu.h` (Lines 6-23)

### 1.4 TLB Updates (`emu/tlb.h`)

**Changes Made:**
```c
// Updated page mask for 64-bit addresses:
#define TLB_PAGE(addr) (addr & 0xfffffffffffff000ULL)
```

**File:** `/home/user/ish/emu/tlb.h` (Line 26)

### 1.5 ELF Loader (`kernel/elf.h`)

**Changes Made:**

1. **ELF Header:**
```c
struct elf_header {
    // ... metadata ...
    qword_t entry_point;     // 64-bit (was dword_t)
    qword_t prghead_off;     // 64-bit
    qword_t secthead_off;    // 64-bit
    // ...
};
```

2. **Program Header:**
```c
struct prg_header {
    uint32_t type;
    uint32_t flags;      // Moved before offsets (ELF64 requirement)
    qword_t offset;      // 64-bit
    qword_t vaddr;       // 64-bit
    qword_t paddr;       // 64-bit
    qword_t filesize;    // 64-bit
    qword_t memsize;     // 64-bit
    qword_t alignment;   // 64-bit
};
```

3. **Auxiliary Vector:**
```c
struct aux_ent {
    qword_t type;   // 64-bit
    qword_t value;  // 64-bit
};
```

**File:** `/home/user/ish/kernel/elf.h` (Lines 15-108)

### 1.6 ModRM Decoder (`emu/modrm.h`)

**Changes Made:**

1. **Added REX Prefix Support:**
```c
struct rex_prefix {
    bool present;
    bool w;  // 64-bit operand size
    bool r;  // Extension to ModRM.reg
    bool x;  // Extension to SIB.index
    bool b;  // Extension to ModRM.r/m or SIB.base
};
```

2. **Extended ModRM Structure:**
```c
struct modrm {
    enum reg64 reg;          // Now supports 16 registers
    enum {
        modrm_reg,
        modrm_mem,
        modrm_mem_si,
        modrm_rip_rel        // NEW: RIP-relative addressing
    } type;
    enum reg64 base;
    int64_t offset;          // 64-bit offset
    enum reg64 index;
    enum {
        times_1 = 0,
        times_2 = 1,
        times_4 = 2,
        times_8 = 3,         // NEW: scale factor of 8
    } shift;
    struct rex_prefix rex;
};
```

3. **New Decoder Function:**
```c
bool modrm_decode64(addr_t *ip, struct tlb *tlb,
                    struct modrm *modrm, struct rex_prefix rex);
```

**Features:**
- Applies REX.R to extend reg field (adds 8)
- Applies REX.B to extend r/m field (adds 8)
- Applies REX.X to extend SIB index (adds 8)
- Detects RIP-relative addressing (mod=00, r/m=101)

**File:** `/home/user/ish/emu/modrm.h` (Lines 12-148)

### 1.7 Interrupt Definitions (`emu/interrupt.h`)

**Changes Made:**
```c
#define INT_SYSCALL 0x80        // 32-bit (INT 0x80)
#define INT_SYSCALL64 0x81      // 64-bit (SYSCALL instruction)
```

**File:** `/home/user/ish/emu/interrupt.h` (Lines 15-16)

---

## Phase 2: Instruction Decoder Updates (REQUIRED)

The instruction decoder (`emu/decode.h`) needs extensive modifications to support 64-bit operations.

### 2.1 Add REX Prefix Parsing

**Location:** `/home/user/ish/emu/decode.h` (Before line 36 - before `restart:`)

**Add this code:**
```c
// REX prefix parsing for x86-64
struct rex_prefix rex = {0};
byte_t peek_byte;

// Check for REX prefix (0x40-0x4F)
if (_PEEK(peek_byte) && peek_byte >= REX_PREFIX_MIN && peek_byte <= REX_PREFIX_MAX) {
    READINSN;  // Consume REX prefix
    rex.present = true;
    rex.w = REX_W(peek_byte);
    rex.r = REX_R(peek_byte);
    rex.x = REX_X(peek_byte);
    rex.b = REX_B(peek_byte);
    TRACE("REX.W=%d REX.R=%d REX.X=%d REX.B=%d ", rex.w, rex.r, rex.x, rex.b);
}
```

**Add `_PEEK` macro:**
```c
#define _PEEK(byte) (tlb_read(tlb, *ip, &(byte), sizeof(byte)))
```

### 2.2 Update READMODRM Macro

**Location:** `/home/user/ish/emu/decode.h` (After REX prefix code)

**Replace:**
```c
#define READMODRM if (!modrm_decode32(&state->ip, tlb, &modrm)) SEGFAULT
```

**With:**
```c
#define READMODRM if (!modrm_decode64(&state->ip, tlb, &modrm, rex)) SEGFAULT
```

### 2.3 Add 64-bit Operand Size Support

**Location:** Throughout `decode.h`

**Pattern to follow:**
```c
// Determine operand size based on REX.W
#define ACTUAL_OP_SIZE (rex.w ? 64 : OP_SIZE)
```

**Example instruction update:**
```c
// Before:
case 0x89: TRACEI("mov reg, modrm");
           READMODRM; MOV(modrm_reg, modrm_val, oz); break;

// After:
case 0x89: TRACEI("mov reg%s, modrm", rex.w ? "64" : "");
           READMODRM; MOV(modrm_reg, modrm_val, rex.w ? 64 : oz); break;
```

### 2.4 Add SYSCALL Instruction

**Location:** `/home/user/ish/emu/decode.h` (In the `case 0x0f:` section)

**Add after line 60:**
```c
case 0x05: TRACEI("syscall");
           INT(INT_SYSCALL64); break;
```

### 2.5 Add 64-bit Only Instructions

**Location:** `/home/user/ish/emu/decode.h` (In the `case 0x0f:` section)

**Instructions to add:**

1. **MOVSXD (Sign-extend DWORD to QWORD)** - Opcode: 0x63
```c
case 0x63: TRACEI("movsxd modrm32, reg64");
           READMODRM; MOVSX(modrm_val, modrm_reg, 32, 64); break;
```

2. **SWAPGS** - Opcode: 0x0F 0x01 0xF8
```c
case 0x01:
    READINSN;
    if (insn == 0xf8) {
        TRACEI("swapgs");
        SWAPGS(); break;
    }
    UNDEFINED;
```

### 2.6 Update All Arithmetic/Logic Instructions

**Pattern for each instruction group:**

```c
// Example: ADD instruction
MAKE_OP(0x00, ADD, "add");

// Ensure ADD macro supports 64-bit:
#define ADD(src, dst, size) do { \
    if (size == 64) { \
        /* 64-bit addition logic */ \
    } else if (size == 32) { \
        /* 32-bit addition logic */ \
    } /* ... */ \
} while (0)
```

**Instructions requiring 64-bit support:**
- Arithmetic: ADD, SUB, MUL, DIV, IMUL, IDIV
- Logic: AND, OR, XOR, NOT, NEG
- Shifts: SHL, SHR, SAR, ROL, ROR, RCL, RCR
- Comparisons: CMP, TEST
- Bit operations: BT, BTS, BTR, BTC, BSF, BSR
- Move: MOV, MOVZX, MOVSX, LEA
- Stack: PUSH, POP
- Conditional: CMOVcc, SETcc

---

## Phase 3: JIT Compiler Updates (REQUIRED)

The JIT compiler (`asbestos/`) needs updates to generate 64-bit gadgets.

### 3.1 Update Gadget Generation (`asbestos/gen.c`)

**Key changes needed:**

1. **Update gen_step function:**
   - Parse REX prefix before decoding instruction
   - Pass REX information to gadget generators
   - Generate 64-bit vs 32-bit gadgets based on REX.W

2. **Add 64-bit register support:**
```c
// In gen.c, update register access functions:
static inline void gen_load_reg64(struct gen_state *state, enum reg64 reg);
static inline void gen_store_reg64(struct gen_state *state, enum reg64 reg);
```

### 3.2 Update Gadgets (`asbestos/gadgets-x86_64/`)

**Files to update:**

1. **`bits.S`** - Bit manipulation operations
   - Add 64-bit variants of BT, BTS, BTR, BTC
   - Update BSF, BSR for 64-bit operands

2. **`math.S`** - Arithmetic operations
   - Add 64-bit ADD, SUB, MUL, DIV, IMUL, IDIV
   - Update flag computation for 64-bit operations

3. **`memory.S`** - Memory access
   - Update load/store gadgets for 64-bit addresses
   - Add RIP-relative addressing support

4. **`control.S`** - Control flow
   - Update CALL/RET for 64-bit addresses
   - Add SYSCALL gadget

**Example gadget update:**

```asm
# Before (32-bit ADD):
.global gadget_add32
gadget_add32:
    movl (%r11), %eax        # Load dst
    addl %edx, %eax          # Add src
    movl %eax, (%r11)        # Store result
    # Update flags...
    ret

# After (64-bit ADD):
.global gadget_add64
gadget_add64:
    movq (%r11), %rax        # Load dst (64-bit)
    addq %rdx, %rax          # Add src (64-bit)
    movq %rax, (%r11)        # Store result
    # Update flags...
    ret
```

### 3.3 Update gadget_table.S

**Add entries for 64-bit gadgets:**

```asm
.global gadget_table
gadget_table:
    # 32-bit gadgets
    .quad gadget_add32
    .quad gadget_sub32
    # ...

    # 64-bit gadgets (new)
    .quad gadget_add64
    .quad gadget_sub64
    # ...
```

### 3.4 Update RIP Calculation

**In `asbestos/gen.c`:**

The JIT needs to handle RIP-relative addressing:

```c
static addr_t gen_effective_address(struct gen_state *state,
                                     struct modrm *modrm) {
    if (modrm->type == modrm_rip_rel) {
        // RIP-relative: address = RIP + displacement
        // Note: RIP points to NEXT instruction
        return state->rip + modrm->offset;
    }
    // ... existing logic for other addressing modes ...
}
```

---

## Phase 4: System Call Handler (REQUIRED)

System calls in x86-64 use different registers and the SYSCALL instruction.

### 4.1 Update System Call Dispatcher (`kernel/calls.c`)

**Add 64-bit system call handler:**

```c
// Existing 32-bit handler (INT 0x80):
dword_t syscall_handler_32(struct cpu_state *cpu) {
    dword_t syscall_num = cpu->eax;
    dword_t arg1 = cpu->ebx;
    dword_t arg2 = cpu->ecx;
    dword_t arg3 = cpu->edx;
    dword_t arg4 = cpu->esi;
    dword_t arg5 = cpu->edi;
    dword_t arg6 = cpu->ebp;

    // Dispatch to syscall table...
}

// NEW: 64-bit handler (SYSCALL):
qword_t syscall_handler_64(struct cpu_state *cpu) {
    qword_t syscall_num = cpu->rax;
    qword_t arg1 = cpu->rdi;   // Different register order!
    qword_t arg2 = cpu->rsi;
    qword_t arg3 = cpu->rdx;
    qword_t arg4 = cpu->r10;   // R10 instead of RCX
    qword_t arg5 = cpu->r8;
    qword_t arg6 = cpu->r9;

    // Need separate syscall table for 64-bit
    // (syscall numbers differ between 32-bit and 64-bit!)

    // Dispatch to 64-bit syscall table...
    qword_t result = syscall_table_64[syscall_num](arg1, arg2, arg3,
                                                    arg4, arg5, arg6);
    cpu->rax = result;  // Return value in RAX
    return result;
}
```

### 4.2 Update Interrupt Handler (`linux/emu_asbestos.c` or similar)

**Find the main execution loop and update:**

```c
int interrupt = cpu_run_to_interrupt(cpu, tlb);

switch (interrupt) {
    case INT_SYSCALL:
        // 32-bit system call (INT 0x80)
        syscall_handler_32(cpu);
        break;

    case INT_SYSCALL64:
        // NEW: 64-bit system call (SYSCALL instruction)
        syscall_handler_64(cpu);
        break;

    // ... other interrupts ...
}
```

### 4.3 Create 64-bit System Call Table

**Location:** `kernel/calls.c`

Linux x86-64 has **different syscall numbers** than i386!

**Examples:**
| Syscall | i386 (32-bit) | x86-64 |
|---------|---------------|--------|
| read | 3 | 0 |
| write | 4 | 1 |
| open | 5 | 2 |
| close | 6 | 3 |
| fork | 2 | 57 |
| execve | 11 | 59 |

**Implementation:**

```c
// Define 64-bit syscall numbers
#define SYS64_read 0
#define SYS64_write 1
#define SYS64_open 2
#define SYS64_close 3
// ... (see /usr/include/asm/unistd_64.h)

// Create syscall table
static syscall_handler_t syscall_table_64[] = {
    [SYS64_read] = sys_read,
    [SYS64_write] = sys_write,
    [SYS64_open] = sys_open,
    [SYS64_close] = sys_close,
    // ...
};
```

### 4.4 Update Pointer Passing

Many syscalls pass pointers. These must be treated as 64-bit:

```c
// Before (32-bit):
dword_t user_ptr = cpu->ebx;
char *kernel_ptr = (char *)mem_ptr(&current->mem, user_ptr, MEM_READ);

// After (64-bit):
qword_t user_ptr = cpu->rdi;
char *kernel_ptr = (char *)mem_ptr(&current->mem, user_ptr, MEM_READ);
```

---

## Phase 5: Testing and Validation (REQUIRED)

### 5.1 Create Test Program

**File:** `test_64bit.c`

```c
#include <stdio.h>
#include <unistd.h>

int main() {
    // Test 64-bit integer
    unsigned long long big_num = 0xFFFFFFFF00000000ULL;
    printf("64-bit number: %llx\n", big_num);

    // Test 64-bit pointer
    void *ptr = &big_num;
    printf("Pointer address: %p\n", ptr);

    // Test system call
    write(1, "Hello from 64-bit!\n", 19);

    // Test extended registers (if using inline asm)
    unsigned long r8_val, r15_val;
    asm volatile("mov $0x1234, %%r8" ::: "r8");
    asm volatile("mov $0x5678, %%r15" ::: "r15");
    asm volatile("mov %%r8, %0" : "=r"(r8_val));
    asm volatile("mov %%r15, %0" : "=r"(r15_val));
    printf("R8: %lx, R15: %lx\n", r8_val, r15_val);

    return 0;
}
```

**Compile as 64-bit:**
```bash
gcc -m64 -static test_64bit.c -o test_64bit
file test_64bit  # Should show: ELF 64-bit LSB executable, x86-64
```

### 5.2 Build iSH

```bash
cd /home/user/ish
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### 5.3 Test Execution

```bash
# Run test program
./ish-emu test_64bit

# Expected output:
# 64-bit number: ffffffff00000000
# Pointer address: 0x7fffffff1234
# Hello from 64-bit!
# R8: 1234, R15: 5678
```

### 5.4 Debug Common Issues

#### Issue 1: Segfault on Startup
**Cause:** ModRM decoder not handling REX prefix correctly
**Solution:** Verify REX prefix parsing in decode.h, ensure modrm_decode64 is called

#### Issue 2: Wrong Syscall Behavior
**Cause:** Using 32-bit syscall numbers for 64-bit
**Solution:** Create separate syscall table with correct x86-64 numbers

#### Issue 3: Register Access Failures
**Cause:** R8-R15 not recognized
**Solution:** Verify REX.R, REX.X, REX.B are applied in ModRM decoder

#### Issue 4: Memory Address Errors
**Cause:** Sign extension or truncation of 64-bit addresses
**Solution:** Ensure all address calculations use qword_t (not dword_t)

### 5.5 Validation Checklist

- [ ] Test program compiles and identifies as 64-bit
- [ ] ELF loader accepts ELF64 format
- [ ] Registers RAX-R15 are accessible
- [ ] RIP (instruction pointer) works correctly
- [ ] 64-bit arithmetic produces correct results
- [ ] System calls execute successfully
- [ ] Memory addresses above 4GB work (if supported)
- [ ] REX prefix correctly enables 64-bit operations
- [ ] RIP-relative addressing works
- [ ] Stack operations work with 64-bit addresses

---

## Complete File Change Summary

### Files Modified (Phase 1 - COMPLETED)

| File | Lines Changed | Key Changes |
|------|--------------|-------------|
| `misc.h` | 10 | Type definitions: addr_t, uint_t, time_t_ |
| `emu/cpu.h` | 150+ | CPU state, registers, RIP, XMM expansion |
| `emu/mmu.h` | 15 | page_t, MEM_PAGES for 64-bit |
| `emu/tlb.h` | 3 | TLB_PAGE macro |
| `kernel/elf.h` | 40 | ELF64 structures |
| `emu/modrm.h` | 120 | REX prefix, modrm_decode64 |
| `emu/interrupt.h` | 2 | INT_SYSCALL64 |

### Files Requiring Changes (Phase 2-4)

| File | Estimated Lines | Changes Needed |
|------|----------------|----------------|
| `emu/decode.h` | 300+ | REX parsing, 64-bit instructions, SYSCALL |
| `asbestos/gen.c` | 200+ | 64-bit gadget generation |
| `asbestos/gadgets-x86_64/*.S` | 500+ | 64-bit operation gadgets |
| `kernel/calls.c` | 100+ | 64-bit syscall handler & table |
| `kernel/calls.h` | 50+ | 64-bit syscall prototypes |
| `linux/emu_asbestos.c` | 20+ | INT_SYSCALL64 handling |

---

## Additional Resources

### x86-64 Documentation

- **Intel SDM Volume 2:** Instruction Set Reference
  - [https://www.intel.com/sdm](https://www.intel.com/sdm)
- **AMD64 Architecture Manual**
  - [https://developer.amd.com/resources/developer-guides-manuals/](https://developer.amd.com/resources/developer-guides-manuals/)
- **System V ABI x86-64**
  - [https://gitlab.com/x86-psABIs/x86-64-ABI](https://gitlab.com/x86-psABIs/x86-64-ABI)

### Linux x86-64 Syscall Reference

- **Syscall Table:** `/usr/include/asm/unistd_64.h`
- **Online Reference:** [https://syscalls.w3challs.com/?arch=x86_64](https://syscalls.w3challs.com/?arch=x86_64)

### REX Prefix Encoding

```
REX Prefix: 0100WRXB
- W: 1 = 64-bit operand size
- R: Extension to ModRM.reg
- X: Extension to SIB.index
- B: Extension to ModRM.r/m or SIB.base

Example: REX.W REX.R REX.B = 01001101 = 0x4D
```

### RIP-Relative Addressing

```
ModRM byte: [mod=00][reg=rrr][r/m=101]

Effective Address = RIP + displacement32
- Used for position-independent code (PIC)
- RIP points to next instruction after current
```

---

## Troubleshooting

### Build Errors

**Error:** `unknown type name 'qword_t'`
**Fix:** Ensure misc.h is included and qword_t is defined

**Error:** `'rax' undeclared`
**Fix:** Update cpu_state structure with 64-bit register definitions

**Error:** `invalid operands to binary` in gadget files
**Fix:** Update assembly gadgets to use 64-bit registers (rax vs eax)

### Runtime Errors

**Segfault during instruction decode:**
- Check REX prefix parsing
- Verify modrm_decode64 is called with correct parameters
- Ensure TLB handles 64-bit addresses

**Wrong syscall results:**
- Verify correct syscall number for x86-64
- Check argument registers (RDI, RSI, RDX, not EBX, ECX, EDX)
- Ensure return value goes to RAX (not EAX)

**Register corruption:**
- Verify static_assert checks pass for register layout
- Check that regs[16] array aligns with individual register fields
- Ensure gadgets don't clobber extended registers

---

## Performance Considerations

### JIT Compilation

64-bit code may run **slower initially** because:
- More bytes per instruction (REX prefixes add 1 byte)
- Larger register set = more save/restore overhead
- 64-bit memory accesses may be slower on some platforms

**Optimization strategies:**
- Cache commonly-used gadgets
- Use 32-bit operations when REX.W=0 (smaller/faster)
- Implement fast path for non-REX instructions

### Memory Usage

64-bit pointers double the size of:
- Page tables (if fully allocated)
- TLB entries
- Stack frames

**Mitigation:**
- Use sparse page tables (only allocate used pages)
- Keep TLB_SIZE reasonable (1024 entries)
- Limit initial address space to 512GB (MEM_PAGES_INITIAL)

---

## Known Limitations

1. **Address Space:** Currently limited to 48-bit virtual addresses (256TB), not full 64-bit
2. **AVX Support:** XMM registers extended to 16, but AVX (YMM) not yet implemented
3. **Syscall Compatibility:** Some syscalls may need special handling for 32/64-bit differences
4. **Kernel Structures:** Some kernel structures (task_struct, etc.) still use 32-bit fields

---

## Future Enhancements

1. **AVX/AVX2 Support:**
   - Extend XMM to YMM (256-bit)
   - Implement VEX prefix handling

2. **Full 64-bit Address Space:**
   - Support 52-bit physical addresses
   - Implement 5-level paging

3. **Performance Optimization:**
   - Superblock compilation (combine multiple blocks)
   - Register allocation optimization
   - Profile-guided optimization

4. **Compatibility:**
   - Add IA-32e mode switching
   - Support mixed 32/64-bit code
   - Implement SYSCALL/SYSRET properly

---

## Conclusion

This guide provides a complete roadmap for transforming iSH to 64-bit. Phase 1 (core infrastructure) is complete and committed. Phases 2-4 require implementing instruction decoder updates, JIT gadget modifications, and system call handler changes.

The changes are systematic and well-defined. Following this guide step-by-step will result in a fully functional 64-bit x86-64 emulator capable of running modern Linux binaries on iOS.

**Estimated effort for remaining phases:**
- Phase 2 (Instruction Decoder): 2-3 days
- Phase 3 (JIT Compiler): 3-5 days
- Phase 4 (System Call Handler): 1-2 days
- Phase 5 (Testing/Debugging): 2-3 days

**Total:** ~2 weeks of focused development

---

**Generated:** 2025-10-22
**Author:** Claude AI (Claude Code)
**Repository:** https://github.com/ish-app/ish
**Branch:** `claude/ish-64bit-port-011CUN2o2wGVYoigGBk1Yh98`
