# iSH 64-bit Architecture Reference

## Quick Architecture Comparison

### Register Set

#### 32-bit (x86)
```
General Purpose: EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP
Instruction Pointer: EIP
XMM Registers: XMM0-XMM7
Count: 8 GPRs, 8 XMM
```

#### 64-bit (x86-64)
```
General Purpose: RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP, R8-R15
Instruction Pointer: RIP
XMM Registers: XMM0-XMM15
Count: 16 GPRs, 16 XMM
```

### Register Access Modes

```c
// 64-bit register: RAX
cpu->rax = 0x123456789ABCDEF0;

// 32-bit (lower half): EAX
cpu->eax = 0x12345678;  // Zeros upper 32 bits!

// 16-bit (lower word): AX
cpu->ax = 0x1234;  // Preserves upper bits

// 8-bit low: AL
cpu->al = 0x12;

// 8-bit high: AH (only for A,B,C,D)
cpu->ah = 0x34;
```

### System Call Comparison

| Aspect | 32-bit (i386) | 64-bit (x86-64) |
|--------|---------------|-----------------|
| **Instruction** | `INT 0x80` | `SYSCALL` |
| **Syscall Number** | EAX | RAX |
| **Arg 1** | EBX | RDI |
| **Arg 2** | ECX | RSI |
| **Arg 3** | EDX | RDX |
| **Arg 4** | ESI | R10 |
| **Arg 5** | EDI | R8 |
| **Arg 6** | EBP | R9 |
| **Return Value** | EAX | RAX |

### Syscall Number Differences

| Syscall | i386 | x86-64 |
|---------|------|--------|
| read | 3 | 0 |
| write | 4 | 1 |
| open | 5 | 2 |
| close | 6 | 3 |
| stat | 106 | 4 |
| fstat | 108 | 5 |
| lstat | 107 | 6 |
| poll | 168 | 7 |
| lseek | 19 | 8 |
| mmap | 90 | 9 |
| mprotect | 125 | 10 |
| fork | 2 | 57 |
| execve | 11 | 59 |
| exit | 1 | 60 |

**Full table:** `/usr/include/asm/unistd_64.h` vs `unistd_32.h`

### REX Prefix

```
Format: 0100WRXB (0x40 - 0x4F)

Bits:
- W (bit 3): 1 = 64-bit operand size, 0 = default size
- R (bit 2): Extends ModRM.reg field (adds 8)
- X (bit 1): Extends SIB.index field (adds 8)
- B (bit 0): Extends ModRM.r/m or SIB.base (adds 8)

Examples:
- REX.W = 0x48: 64-bit operand size
- REX.WR = 0x4C: 64-bit + extend reg
- REX.WRB = 0x4D: 64-bit + extend reg + extend r/m
```

### Register Encoding with REX

```
Without REX:
- reg field (3 bits): 0-7 → registers 0-7 (RAX-RDI)

With REX.R=1:
- reg field (3 bits): 0-7 → registers 8-15 (R8-R15)

Example:
- ModRM.reg = 3, REX.R = 0 → RBX (register 3)
- ModRM.reg = 3, REX.R = 1 → R11 (register 11 = 3 + 8)
```

### Addressing Modes

#### 32-bit Addressing
```
[base + index*scale + disp]
- base: any GPR (EAX-EDI)
- index: any GPR except ESP
- scale: 1, 2, 4
- disp: 8-bit or 32-bit
```

#### 64-bit Addressing
```
[base + index*scale + disp]
- base: any GPR (RAX-R15)
- index: any GPR except RSP
- scale: 1, 2, 4, 8
- disp: 8-bit or 32-bit

PLUS: RIP-relative addressing
[RIP + disp32]
- Used for position-independent code
- RIP = address of next instruction
```

### RIP-Relative Addressing

```
Encoding: ModRM with mod=00, r/m=101

Example:
MOV RAX, [RIP+0x1234]

Effective address = RIP + 0x1234
where RIP points to the NEXT instruction

Disassembly:
48 8B 05 34 12 00 00
REX.W MOV_r/m_to_r ModRM disp32
0x48  0x8B       0x05  [0x00001234]
```

### Memory Address Space

```
32-bit: 4 GB (2^32 bytes)
├─ 0x00000000 - 0xBFFFFFFF: User space (3 GB)
└─ 0xC0000000 - 0xFFFFFFFF: Kernel space (1 GB)

64-bit: 256 TB (2^48 bytes) - current implementation
├─ 0x0000000000000000 - 0x00007FFFFFFFFFFF: User space (128 TB)
└─ 0xFFFF800000000000 - 0xFFFFFFFFFFFFFFFF: Kernel space (128 TB)

Note: Full 64-bit would be 16 EB (exabytes), but x86-64 currently
uses only 48-bit virtual addresses (canonical addressing).
```

### Instruction Encoding Examples

#### 32-bit: MOV EAX, EBX
```
Bytes: 89 D8
- 0x89: MOV r/m32, r32
- 0xD8: ModRM byte
  - mod = 11 (register direct)
  - reg = 011 (EBX)
  - r/m = 000 (EAX)
```

#### 64-bit: MOV RAX, RBX
```
Bytes: 48 89 D8
- 0x48: REX.W (64-bit operand)
- 0x89: MOV r/m64, r64
- 0xD8: ModRM byte (same as 32-bit)
```

#### 64-bit: MOV R8, R11
```
Bytes: 4D 89 D8
- 0x4D: REX.WRB (64-bit + extend reg + extend r/m)
- 0x89: MOV r/m64, r64
- 0xD8: ModRM byte
  - reg = 011 → 011 + 8 = R11 (due to REX.R)
  - r/m = 000 → 000 + 8 = R8 (due to REX.B)
```

### Stack Operations

#### 32-bit
```asm
push eax        ; ESP -= 4, [ESP] = EAX
pop eax         ; EAX = [ESP], ESP += 4
call function   ; push EIP, jmp function
ret             ; pop EIP
```

#### 64-bit (default 64-bit mode)
```asm
push rax        ; RSP -= 8, [RSP] = RAX
pop rax         ; RAX = [RSP], RSP += 8
call function   ; push RIP, jmp function
ret             ; pop RIP

; Can still use 32-bit with operand size override:
push eax        ; RSP -= 8, [RSP] = sign_extend(EAX)
```

### Function Calling Conventions

#### 32-bit (cdecl)
```c
int func(int a, int b, int c);

// Arguments on stack (right-to-left):
push c
push b
push a
call func
add esp, 12   ; Caller cleans up

// Return value in EAX
```

#### 64-bit (System V ABI)
```c
int func(int a, int b, int c, int d, int e, int f, int g);

// First 6 args in registers:
// RDI = a, RSI = b, RDX = c, RCX = d, R8 = e, R9 = f
// 7th arg (g) on stack
push g
call func
add rsp, 8    ; Caller cleans up

// Return value in RAX
// Float return values in XMM0
```

### Flag Register (RFLAGS)

```
Bit  Name  Description
0    CF    Carry Flag
2    PF    Parity Flag
4    AF    Auxiliary Carry Flag
6    ZF    Zero Flag
7    SF    Sign Flag
8    TF    Trap Flag
9    IF    Interrupt Enable Flag
10   DF    Direction Flag
11   OF    Overflow Flag
12-13 IOPL  I/O Privilege Level
14   NT    Nested Task
16   RF    Resume Flag
17   VM    Virtual 8086 Mode
18   AC    Alignment Check
19   VIF   Virtual Interrupt Flag
20   VIP   Virtual Interrupt Pending
21   ID    ID Flag

Same as 32-bit EFLAGS, but extended to 64 bits.
Upper 32 bits reserved (must be zero).
```

### ELF Binary Format

#### ELF32 Header
```c
struct elf32_header {
    uint8_t  e_ident[16];    // Magic: 7F 45 4C 46 01...
    uint16_t e_type;         // ET_EXEC, ET_DYN
    uint16_t e_machine;      // EM_386 (3)
    uint32_t e_version;
    uint32_t e_entry;        // 32-bit entry point
    uint32_t e_phoff;        // 32-bit program header offset
    uint32_t e_shoff;        // 32-bit section header offset
    // ...
};
```

#### ELF64 Header
```c
struct elf64_header {
    uint8_t  e_ident[16];    // Magic: 7F 45 4C 46 02...
    uint16_t e_type;
    uint16_t e_machine;      // EM_X86_64 (62)
    uint32_t e_version;
    uint64_t e_entry;        // 64-bit entry point
    uint64_t e_phoff;        // 64-bit program header offset
    uint64_t e_shoff;        // 64-bit section header offset
    // ...
};
```

**Key difference:** `e_ident[4] = 1` (32-bit) vs `2` (64-bit)

### Common Instructions with 64-bit Support

| Instruction | 32-bit | 64-bit | Notes |
|-------------|--------|--------|-------|
| **MOV** | ✓ | ✓ | Immediate to 64-bit reg uses 48-bit encoding |
| **ADD/SUB** | ✓ | ✓ | Flags work same as 32-bit |
| **PUSH/POP** | 4 bytes | 8 bytes | Default operand size in 64-bit mode |
| **LEA** | ✓ | ✓ | Can use RIP-relative |
| **IMUL** | ✓ | ✓ | Three-operand form common |
| **MOVSX** | ✓ | ✓ | Sign-extend 32→64 uses MOVSXD (0x63) |
| **MOVZX** | ✓ | ✓ | 32→64 doesn't need it (implicit zero-extend) |
| **SHL/SHR** | ✓ | ✓ | Shift count still in CL |
| **CALL/RET** | 4-byte addr | 8-byte addr | Near calls only in 64-bit |
| **JMP/Jcc** | ✓ | ✓ | Still use 32-bit relative offsets |

### Performance Characteristics

#### Advantages of 64-bit:
- More registers (16 vs 8) = less memory traffic
- Larger address space = no need for memory segmentation hacks
- 64-bit integers native (no emulation)
- RIP-relative addressing = better position-independent code

#### Disadvantages:
- Larger pointers = 2x memory for pointer-heavy data structures
- More prefixes = slightly larger code size
- Register save/restore overhead increases

#### Typical performance:
- Integer code: 0-10% faster (more registers help)
- Pointer-heavy code: 0-10% slower (cache pressure)
- FP code: No significant difference
- Overall: Usually neutral to slightly positive

### Common Pitfalls

1. **Zero-extension vs Sign-extension**
   ```c
   // 32-bit writes to 64-bit registers zero upper 32 bits:
   mov eax, 0x12345678  ; RAX = 0x0000000012345678

   // But 8/16-bit writes preserve upper bits:
   mov al, 0x12         ; Preserves bits 8-63 of RAX
   ```

2. **Address Size**
   ```c
   // Address calculations use 64 bits by default
   lea rax, [rbx + rcx]  ; Full 64-bit address calculation

   // Can force 32-bit with address size prefix (0x67)
   lea rax, [ebx + ecx]  ; 32-bit address, zero-extended to 64
   ```

3. **Immediate Values**
   ```c
   // Most immediates are sign-extended:
   mov rax, -1          ; RAX = 0xFFFFFFFFFFFFFFFF

   // Exception: MOVABS can use full 64-bit immediate
   movabs rax, 0x123456789ABCDEF0  ; Full 64-bit immediate (10 bytes!)
   ```

4. **Default Operand Size**
   ```asm
   ; In 64-bit mode, default operand size is still 32-bit
   mov rax, rbx     ; Needs REX.W: 48 89 D8
   mov eax, ebx     ; No prefix:   89 D8
   push rax         ; Always 64-bit in 64-bit mode
   ```

### Debug Tips

#### GDB Commands
```bash
# View 64-bit registers
info registers
# or
i r rax rbx rcx rdx rsi rdi rbp rsp r8 r9 r10 r11 r12 r13 r14 r15 rip

# Disassemble with flavor
set disassembly-flavor intel
disassemble /r $rip,+20

# Examine memory (64-bit)
x/8gx $rsp    # 8 qwords at RSP
x/s $rdi      # String at RDI (first arg)

# Watch register
watch $rax

# Break on syscall
catch syscall
```

#### QEMU Debugging
```bash
# Run with GDB server
qemu-x86_64 -g 1234 ./program

# In another terminal
gdb ./program
target remote :1234
```

### Reference Documents

- **Intel® 64 and IA-32 Architectures Software Developer's Manual**
  Volume 1: Basic Architecture
  Volume 2: Instruction Set Reference
  Volume 3: System Programming Guide

- **System V Application Binary Interface (AMD64)**
  https://gitlab.com/x86-psABIs/x86-64-ABI

- **Linux Syscall Reference (x86-64)**
  https://filippo.io/linux-syscall-table/
  https://syscalls.w3challs.com/?arch=x86_64

- **Agner Fog's Optimization Manuals**
  https://www.agner.org/optimize/

---

**Last Updated:** 2025-10-22
**For:** iSH 64-bit Migration Project
