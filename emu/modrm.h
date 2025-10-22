#ifndef MODRM_H
#define MODRM_H

#include "debug.h"
#include "misc.h"
#include "emu/cpu.h"
#include "emu/tlb.h"

#undef DEFAULT_CHANNEL
#define DEFAULT_CHANNEL instr

// REX prefix structure for x86-64
// Format: 0100WRXB (0x40-0x4F)
struct rex_prefix {
    bool present;
    bool w;  // 64-bit operand size
    bool r;  // Extension to ModRM.reg
    bool x;  // Extension to SIB.index
    bool b;  // Extension to ModRM.r/m or SIB.base
};

#define REX_PREFIX_MIN 0x40
#define REX_PREFIX_MAX 0x4F
#define REX_W(byte) (((byte) >> 3) & 1)
#define REX_R(byte) (((byte) >> 2) & 1)
#define REX_X(byte) (((byte) >> 1) & 1)
#define REX_B(byte) (((byte) >> 0) & 1)

struct modrm {
    union {
        enum reg64 reg;
        unsigned opcode;
    };
    enum {
        modrm_reg,      // Register direct
        modrm_mem,      // Memory with base + offset
        modrm_mem_si,   // Memory with base + index*scale + offset
        modrm_rip_rel   // RIP-relative (x86-64 only)
    } type;
    union {
        enum reg64 base;
        unsigned rm_opcode;
    };
    int64_t offset;     // 64-bit offset for RIP-relative addressing
    enum reg64 index;
    enum {
        times_1 = 0,
        times_2 = 1,
        times_4 = 2,
        times_8 = 3,    // x86-64 adds scale factor of 8
    } shift;
    struct rex_prefix rex;  // REX prefix information
};

static const unsigned rm_sib = 4;   // reg_rsp/reg_esp
static const unsigned rm_none = 4;  // reg_rsp/reg_esp (no index in SIB)
static const unsigned rm_disp32 = 5; // reg_rbp/reg_ebp
#define MOD(byte) ((byte & 0b11000000) >> 6)
#define REG(byte) ((byte & 0b00111000) >> 3)
#define RM(byte)  ((byte & 0b00000111) >> 0)

// read modrm with REX prefix support, output information into *modrm, return false for segfault
// This is the main decoder for x86-64
static inline bool modrm_decode64(addr_t *ip, struct tlb *tlb, struct modrm *modrm, struct rex_prefix rex) {
#define READ(thing) \
    *ip += sizeof(thing); \
    if (!tlb_read(tlb, *ip - sizeof(thing), &(thing), sizeof(thing))) \
        return false

    byte_t modrm_byte;
    READ(modrm_byte);

    enum {
        mode_disp0,
        mode_disp8,
        mode_disp32,
        mode_reg,
    } mode = MOD(modrm_byte);

    modrm->type = modrm_mem;
    modrm->rex = rex;

    // Apply REX.R extension to reg field
    modrm->reg = REG(modrm_byte) | (rex.r ? 8 : 0);
    modrm->rm_opcode = RM(modrm_byte);

    // Apply REX.B extension to r/m field
    unsigned rm = RM(modrm_byte) | (rex.b ? 8 : 0);

    if (mode == mode_reg) {
        modrm->type = modrm_reg;
        modrm->base = rm;
    } else if (modrm->rm_opcode == rm_disp32 && mode == mode_disp0) {
        // RIP-relative addressing in 64-bit mode (mod=00, r/m=101)
        modrm->type = modrm_rip_rel;
        modrm->base = reg_none;
        mode = mode_disp32;
    } else if (modrm->rm_opcode == rm_sib && mode != mode_reg) {
        byte_t sib_byte;
        READ(sib_byte);

        // Apply REX.B to SIB base
        modrm->base = RM(sib_byte) | (rex.b ? 8 : 0);

        // Apply REX.X to SIB index
        modrm->index = REG(sib_byte) | (rex.x ? 8 : 0);
        modrm->shift = MOD(sib_byte);

        // wtf intel
        if (RM(sib_byte) == rm_disp32) {
            if (mode == mode_disp0) {
                modrm->base = reg_none;
                mode = mode_disp32;
            } else {
                modrm->base = reg_rbp | (rex.b ? 8 : 0);
            }
        }

        if (modrm->index != rm_none)
            modrm->type = modrm_mem_si;
    } else {
        modrm->base = rm;
    }

    if (mode == mode_disp0) {
        modrm->offset = 0;
    } else if (mode == mode_disp8) {
        int8_t offset;
        READ(offset);
        modrm->offset = offset;
    } else if (mode == mode_disp32) {
        int32_t offset;
        READ(offset);
        modrm->offset = offset;
    }
#undef READ

    TRACE("reg=%s opcode=%d ", reg64_name(modrm->reg), modrm->opcode);
    TRACE("base=%s ", reg64_name(modrm->base));
    if (modrm->type != modrm_reg)
        TRACE("offset=%s0x%llx ", modrm->offset < 0 ? "-" : "", (long long)modrm->offset);
    if (modrm->type == modrm_mem_si)
        TRACE("index=%s<<%d ", reg64_name(modrm->index), modrm->shift);
    if (modrm->type == modrm_rip_rel)
        TRACE("RIP-relative ");

    return true;
}

// Backward compatibility wrapper for 32-bit code
static inline bool modrm_decode32(addr_t *ip, struct tlb *tlb, struct modrm *modrm) {
    struct rex_prefix rex = {0};  // No REX prefix in 32-bit mode
    return modrm_decode64(ip, tlb, modrm, rex);
}

// Legacy implementation kept for reference (unused)
static inline bool __modrm_decode32_legacy(addr_t *ip, struct tlb *tlb, struct modrm *modrm) {
#define READ(thing) \
    *ip += sizeof(thing); \
    if (!tlb_read(tlb, *ip - sizeof(thing), &(thing), sizeof(thing))) \
        return false

    byte_t modrm_byte;
    READ(modrm_byte);

    enum {
        mode_disp0,
        mode_disp8,
        mode_disp32,
        mode_reg,
    } mode = MOD(modrm_byte);
    modrm->type = modrm_mem;
    modrm->reg = REG(modrm_byte);
    modrm->rm_opcode = RM(modrm_byte);
    if (mode == mode_reg) {
        modrm->type = modrm_reg;
    } else if (modrm->rm_opcode == rm_disp32 && mode == mode_disp0) {
        modrm->base = reg_none;
        mode = mode_disp32;
    } else if (modrm->rm_opcode == rm_sib && mode != mode_reg) {
        byte_t sib_byte;
        READ(sib_byte);
        modrm->base = RM(sib_byte);
        // wtf intel
        if (modrm->rm_opcode == rm_disp32) {
            if (mode == mode_disp0) {
                modrm->base = reg_none;
                mode = mode_disp32;
            } else {
                modrm->base = reg_ebp;
            }
        }
        modrm->index = REG(sib_byte);
        modrm->shift = MOD(sib_byte);
        if (modrm->index != rm_none)
            modrm->type = modrm_mem_si;
    }

    if (mode == mode_disp0) {
        modrm->offset = 0;
    } else if (mode == mode_disp8) {
        int8_t offset;
        READ(offset);
        modrm->offset = offset;
    } else if (mode == mode_disp32) {
        int32_t offset;
        READ(offset);
        modrm->offset = offset;
    }
#undef READ

    TRACE("reg=%s opcode=%d ", reg32_name(modrm->reg), modrm->opcode);
    TRACE("base=%s ", reg32_name(modrm->base));
    if (modrm->type != modrm_reg)
        TRACE("offset=%s0x%x ", modrm->offset < 0 ? "-" : "", modrm->offset);
    if (modrm->type == modrm_mem_si)
        TRACE("index=%s<<%d ", reg32_name(modrm->index), modrm->shift);

    return true;
}

#endif
