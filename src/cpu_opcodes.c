#include "cpu_opcodes.h"
#include <stdio.h>
#include <stdlib.h>
#include "clock.h"
#include "cpu.h"
#include "cpu_utils.h"
#include "interrupt.h"
#include "mmu.h"

extern cpu_t CPU;

/*************** Helper funcions. ***************/

/**
 * Increment register n.
 * n = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Not affected.
 */
static uint8_t inc_n(uint8_t value)
{
    if ((value & 0x0f) == 0x0f) {
        FLAG_SET(FLAG_H);
    } else {
        FLAG_CLEAR(FLAG_H);
    }
    ++value;
    FLAG_SET_ZERO(!value);
    FLAG_CLEAR(FLAG_N);
    return value;
}

/**
 * Decrement register n.
 * n = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Not affected.
 */
static uint8_t dec_n(uint8_t value)
{
    if (value & 0x0f) {
        FLAG_CLEAR(FLAG_H);
    } else {
        FLAG_SET(FLAG_H);
    }
    --value;
    FLAG_SET_ZERO(!value);
    FLAG_SET(FLAG_N);
    return value;
}

/**
 * Add n to nn.
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
static uint8_t add8(uint8_t val1, uint8_t val2)
{
    uint32_t result32 = (uint32_t)(val1 + val2);
    uint8_t result8 = (uint8_t)(result32 & 0xff);
    FLAG_CLEAR(FLAG_N);
    if (((val1 & 0x0f) + (val2 & 0x0f)) > 0x0f) {
        FLAG_SET(FLAG_H);
    } else {
        FLAG_CLEAR(FLAG_H);
    }
    if (result32 & 0xff00) {
        FLAG_SET(FLAG_C);
    } else {
        FLAG_CLEAR(FLAG_C);
    }
    FLAG_SET_ZERO(!result8);
    return result8;
}

/**
 * Add n to nn.
 * Flags affected:
 * Z - Not affected.
 * N - Reset.
 * H - Set if carry from bit 11.
 * C - Set if carry from bit 15.
 */
static uint16_t add16(uint16_t val1, uint16_t val2)
{
    uint32_t result = (uint32_t)(val1 + val2);
    FLAG_CLEAR(FLAG_N);
    if (((val1 & 0x0fff) + (val2 & 0x0fff)) > 0x0fff) {
        FLAG_SET(FLAG_H);
    } else {
        FLAG_CLEAR(FLAG_H);
    }
    if (result & 0xffff0000) {
        FLAG_SET(FLAG_C);
    } else {
        FLAG_CLEAR(FLAG_C);
    }
    clock_step(4);
    /* Zero flag is not updated. */
    return (uint16_t)(result & 0xffff);
}

/**
 * Add n to nn with carry.
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
static void adc(uint8_t val)
{
    uint8_t carry = FLAG_IS_SET(FLAG_C) >> 4;
    uint32_t result32 = (uint32_t)(CPU.reg.a + val + carry);
    uint8_t result8 = (uint8_t)(result32 & 0xff);
    FLAG_CLEAR(FLAG_N);
    if (((CPU.reg.a & 0x0f) + (val & 0x0f) + carry) > 0x0f) {
        FLAG_SET(FLAG_H);
    } else {
        FLAG_CLEAR(FLAG_H);
    }
    if (result32 & 0xff00) {
        FLAG_SET(FLAG_C);
    } else {
        FLAG_CLEAR(FLAG_C);
    }
    FLAG_SET_ZERO(!result8);
    CPU.reg.a = result8;
}

/**
 * Subtract n from A.
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if borrow from bit 4.
 * C - Set if borrow.
 */
static void sub(uint8_t val)
{
    FLAG_SET(FLAG_N);
    if (((val & 0x0f) > (CPU.reg.a & 0x0f))) {
        FLAG_SET(FLAG_H);
    } else {
        FLAG_CLEAR(FLAG_H);
    }
    if (val > CPU.reg.a) {
        FLAG_SET(FLAG_C);
    } else {
        FLAG_CLEAR(FLAG_C);
    }
    CPU.reg.a = (uint8_t)(CPU.reg.a - val);
    FLAG_SET_ZERO(!CPU.reg.a);
}

/**
 * Subtract n from A with cary.
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if borrow from bit 4.
 * C - Set if borrow.
 */
static void sbc(uint8_t val)
{
    FLAG_SET(FLAG_N);
    uint8_t carry = FLAG_IS_SET(FLAG_C) >> 4;
    if (((val & 0x0f) + carry > (CPU.reg.a & 0x0f))) {
        FLAG_SET(FLAG_H);
    } else {
        FLAG_CLEAR(FLAG_H);
    }
    int nc = val + carry;
    if (nc > CPU.reg.a) {
        FLAG_SET(FLAG_C);
    } else {
        FLAG_CLEAR(FLAG_C);
    }
    CPU.reg.a -= nc;
    FLAG_SET_ZERO(!CPU.reg.a);
}

/**
 * Bitwise AND n with A, result in A.
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set.
 * C - Reset.
 */
static void and8(uint8_t val)
{
    CPU.reg.a &= val;
    FLAG_SET_ZERO(!CPU.reg.a);
    FLAG_CLEAR(FLAG_N | FLAG_C);
    FLAG_SET(FLAG_H);
}

/**
 * Bitwise XOR n with A, result in A.
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
static void xor8(uint8_t val)
{
    CPU.reg.a ^= val;
    FLAG_SET_ZERO(!CPU.reg.a);
    FLAG_CLEAR(FLAG_N | FLAG_H | FLAG_C);
}

/**
 * Bitwise OR n with A, result in A.
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
static void or8(uint8_t val)
{
    CPU.reg.a |= val;
    FLAG_SET_ZERO(!CPU.reg.a);
    FLAG_CLEAR(FLAG_N | FLAG_H | FLAG_C);
}

/**
 * Compare A with n.
 * Flags affected:
 * Z - Set if result is zero. (Set if A = n.)
 * N - Set.
 * H - Set if borrow from bit 4.
 * C - Set for borrow. (Set if A < n.)
 */
static void cp(uint8_t val)
{
    uint16_t result = (uint16_t)(CPU.reg.a - val);
    FLAG_SET_ZERO(!result);
    FLAG_SET(FLAG_N);
    if (((val & 0x0f) > (CPU.reg.a & 0x0f))) {
        FLAG_SET(FLAG_H);
    } else {
        FLAG_CLEAR(FLAG_H);
    }
    if (val > CPU.reg.a) {
        FLAG_SET(FLAG_C);
    } else {
        FLAG_CLEAR(FLAG_C);
    }
}

/**
 * Update 16-bit register. Also take care of clock increase.
 */
static inline void reg16_set(uint16_t *reg, uint16_t val)
{
    *reg = val;
    clock_step(4);
}

/**
 * Increment and update 16-bit register. Also take care of clock increase.
 */
static inline void reg16_inc(uint16_t *reg, uint16_t val)
{
    *reg += val;
    clock_step(4);
}

/* Push to stack. */
void push(uint16_t val)
{
    reg16_inc(&CPU.reg.sp, -2);
    mmu_write_word(CPU.reg.sp, val);
}

/* Pop from stack. */
uint16_t pop(void)
{
    uint16_t val = mmu_read_word(CPU.reg.sp);
    CPU.reg.sp = (uint16_t)(CPU.reg.sp + 2);
    return val;
}

/* Function for undefined instructions. */
void undefined(void)
{
    CPU.reg.pc--;
    uint8_t opcode = mmu_read_byte(CPU.reg.pc);
    printf("ERROR: undefined instruction 0x%02x!\n", opcode);
    exit(EXIT_FAILURE);
}

/*************** Opcodes implementation. ***************/

/* 0x00: No operation. */
void nop(void)
{
}

/* 0x01: Load 16-bit immediate into BC. */
void ld_bc_nn(uint16_t value)
{
    CPU.reg.bc = value;
}

/* 0x02: Save A to address pointed by BC. */
void ld_bcp_a(void)
{
    mmu_write_byte(CPU.reg.bc, CPU.reg.a);
}

/* 0x03: Increment 16-bit BC. */
void inc_bc(void)
{
    reg16_inc(&CPU.reg.bc, 1);
}

/* 0x04: Increment B. */
void inc_b(void)
{
    CPU.reg.b = inc_n(CPU.reg.b);
}

/* 0x05: Decrement B. */
void dec_b(void)
{
    CPU.reg.b = dec_n(CPU.reg.b);
}

/* 0x06: Load 8-bit immediate into B. */
void ld_b_n(uint8_t val)
{
    CPU.reg.b = val;
}

/* 0x07: Rotate A left. Old bit 7 to Carry flag. */
void rlca(void)
{
    uint8_t a = CPU.reg.a;
    FLAG_SET_CARRY((a & 0x80) >> 7);
    FLAG_CLEAR(FLAG_Z | FLAG_N | FLAG_H);
    CPU.reg.a = (a << 1) | (a >> 7);
}

/* 0x08: Save SP to given address. */
void ld_nnp_sp(uint16_t addr)
{
    mmu_write_word(addr, CPU.reg.sp);
}

/* 0x09: Add 16-bit BC to HL. */
void add_hl_bc(void)
{
    CPU.reg.hl = add16(CPU.reg.hl, CPU.reg.bc);
}

/* 0x0a: Put value pointed by BC into A. */
void ld_a_bcp(void)
{
    CPU.reg.a = mmu_read_byte(CPU.reg.bc);
}

/* 0x0b: Decrement BC. */
void dec_bc(void)
{
    reg16_inc(&CPU.reg.bc, -1);
}

/* 0x0c: Increment C. */
void inc_c(void)
{
    CPU.reg.c = inc_n(CPU.reg.c);
}

/* 0x0d: Decrement C. */
void dec_c(void)
{
    CPU.reg.c = dec_n(CPU.reg.c);
}

/* 0x0e: Load 8-bit immediate into C. */
void ld_c_n(uint8_t val)
{
    CPU.reg.c = val;
}

/* 0x0f: Rotate A right. Old bit 0 to Carry flag. */
void rrca(void)
{
    uint8_t a = CPU.reg.a;
    FLAG_SET_CARRY(a);
    FLAG_CLEAR(FLAG_Z | FLAG_N | FLAG_H);
    CPU.reg.a = (a << 7) | (a >> 1);
}

/* 0x10: The STOP command halts the GameBoy processor and screen until any
 * button is pressed. */
void stop(void)
{
    mmu_stop();
    printf("Received STOP command!\n");
}

/* 0x11: Load 16-bit immediate into DE. */
void ld_de_nn(uint16_t value)
{
    CPU.reg.de = value;
}

/* 0x12: Save A to address pointed by DE. */
void ld_dep_a(void)
{
    mmu_write_byte(CPU.reg.de, CPU.reg.a);
}

/* 0x13: Increment 16-bit DE. */
void inc_de(void)
{
    reg16_inc(&CPU.reg.de, 1);
}

/* 0x14: Increment D. */
void inc_d(void)
{
    CPU.reg.d = inc_n(CPU.reg.d);
}

/* 0x15: Decrement D. */
void dec_d(void)
{
    CPU.reg.d = dec_n(CPU.reg.d);
}

/* 0x16: Load 8-bit immediate into D. */
void ld_d_n(uint8_t val)
{
    CPU.reg.d = val;
}

/* 0x17: Rotate A left through Carry flag. */
void rla(void)
{
    uint8_t old_carry = (uint8_t)(FLAG_IS_SET(FLAG_C) >> 4);
    uint8_t a = CPU.reg.a;
    FLAG_SET_CARRY((a & 0x80) >> 7);
    FLAG_CLEAR(FLAG_Z | FLAG_N | FLAG_H);
    CPU.reg.a = (a << 1) | old_carry;
}

/* 0x18: Relative jump by signed immediate. */
void jr_n(uint8_t val)
{
    reg16_inc(&CPU.reg.pc, (int8_t)val);
}

/* 0x19: Add 16-bit DE to HL. */
void add_hl_de(void)
{
    CPU.reg.hl = add16(CPU.reg.hl, CPU.reg.de);
}

/* 0x1a: Put value pointed by DE into A. */
void ld_a_dep(void)
{
    CPU.reg.a = mmu_read_byte(CPU.reg.de);
}

/* 0x1b: Decrement DE. */
void dec_de(void)
{
    reg16_inc(&CPU.reg.de, -1);
}

/* 0x1c: Increment E. */
void inc_e(void)
{
    CPU.reg.e = inc_n(CPU.reg.e);
}

/* 0x1d: Decrement E. */
void dec_e(void)
{
    CPU.reg.e = dec_n(CPU.reg.e);
}

/* 0x1e: Load 8-bit immediate into E. */
void ld_e_n(uint8_t val)
{
    CPU.reg.e = val;
}

/* 0x1f: Rotate A right through Carry flag. */
void rra(void)
{
    uint8_t old_carry = (uint8_t)(FLAG_IS_SET(FLAG_C) << 3);
    uint8_t a = CPU.reg.a;
    FLAG_SET_CARRY(a);
    FLAG_CLEAR(FLAG_N | FLAG_Z | FLAG_H);
    CPU.reg.a = old_carry | a >> 1;
}

/* 0x20: Jump if Z flag is not set. */
void jr_nz_n(uint8_t val)
{
    if (!FLAG_IS_SET(FLAG_Z)) {
        reg16_inc(&CPU.reg.pc, (int8_t)val);
    }
}

/* 0x21: Load 16-bit immediate into HL. */
void ld_hl_nn(uint16_t value)
{
    CPU.reg.hl = value;
}

/* 0x22: Put A into memory address HL and increment HL. */
void ldi_hlp_a(void)
{
    mmu_write_byte(CPU.reg.hl++, CPU.reg.a);
}

/* 0x23: Increment 16-bit HL. */
void inc_hl(void)
{
    reg16_inc(&CPU.reg.hl, 1);
}

/* 0x24: Increment H. */
void inc_h(void)
{
    CPU.reg.h = inc_n(CPU.reg.h);
}

/* 0x25: Decrement H. */
void dec_h(void)
{
    CPU.reg.h = dec_n(CPU.reg.h);
}

/* 0x26: Load 8-bit immediate into H. */
void ld_h_n(uint8_t val)
{
    CPU.reg.h = val;
}

/* 0x27: Adjust A for BCD addition. */
void daa(void)
{
    uint16_t s = CPU.reg.a;

    if (FLAG_IS_SET(FLAG_N)) {
        if (FLAG_IS_SET(FLAG_H))
            s = (s - 0x06) & 0xFF;
        if (FLAG_IS_SET(FLAG_C))
            s = (uint16_t)(s - 0x60);
    } else {
        if (FLAG_IS_SET(FLAG_H) || (s & 0xF) > 9)
            s = (uint16_t)(s + 0x06);
        if (FLAG_IS_SET(FLAG_C) || s > 0x9F)
            s = (uint16_t)(s + 0x60);
    }

    CPU.reg.a = (uint8_t)s;
    FLAG_CLEAR(FLAG_H);
    FLAG_SET_ZERO(!CPU.reg.a);
    if (s >= 0x100)
        FLAG_SET(FLAG_C);
}

/* 0x28: Jump if Z flag is set. */
void jr_z_n(uint8_t val)
{
    if (FLAG_IS_SET(FLAG_Z)) {
        reg16_inc(&CPU.reg.pc, (int8_t)val);
    }
}

/* 0x29: Add 16-bit HL to HL. */
void add_hl_hl(void)
{
    CPU.reg.hl = add16(CPU.reg.hl, CPU.reg.hl);
}

/* 0x2a: Put value at address HL into A and increment HL. */
void ldi_a_hlp(void)
{
    CPU.reg.a = mmu_read_byte(CPU.reg.hl++);
}

/* 0x2b: Decrement HL. */
void dec_hl(void)
{
    reg16_inc(&CPU.reg.hl, -1);
}

/* 0x2c: Increment L. */
void inc_l(void)
{
    CPU.reg.l = inc_n(CPU.reg.l);
}

/* 0x2d: Decrement L. */
void dec_l(void)
{
    CPU.reg.l = dec_n(CPU.reg.l);
}

/* 0x2e: Load 8-bit immediate into L. */
void ld_l_n(uint8_t val)
{
    CPU.reg.l = val;
}

/* 0x2f: Complement A register. */
void cpl(void)
{
    CPU.reg.a = (uint8_t)(~CPU.reg.a);
    FLAG_SET(FLAG_N | FLAG_H);
}

/* 0x30: Jump if C flag is not set. */
void jr_nc_n(uint8_t val)
{
    if (!FLAG_IS_SET(FLAG_C)) {
        reg16_inc(&CPU.reg.pc, (int8_t)val);
    }
}

/* 0x31: Load 16-bit immediate into SP */
void ld_sp_nn(uint16_t value)
{
    CPU.reg.sp = value;
}

/* 0x32: Put A into memory address HL and decrement HL. */
void ldd_hlp_a(void)
{
    mmu_write_byte(CPU.reg.hl--, CPU.reg.a);
}

/* 0x33: Increment 16-bit SP. */
void inc_sp(void)
{
    reg16_inc(&CPU.reg.sp, 1);
}

/* 0x34: Increment value pointed by HL. */
void inc_hlp(void)
{
    uint8_t val = mmu_read_byte(CPU.reg.hl);
    mmu_write_byte(CPU.reg.hl, inc_n(val));
}

/* 0x35: Decrement value pointed by HL. */
void dec_hlp(void)
{
    uint8_t val = mmu_read_byte(CPU.reg.hl);
    mmu_write_byte(CPU.reg.hl, dec_n(val));
}

/* 0x36: Load 8-bit immediate into address pointed by HL. */
void ld_hlp_n(uint8_t val)
{
    mmu_write_byte(CPU.reg.hl, val);
}

/* 0x37: Set carry flag. */
void scf(void)
{
    FLAG_SET(FLAG_C);
    FLAG_CLEAR(FLAG_N | FLAG_H);
}

/* 0x38: Jump if C flag is set. */
void jr_c_n(uint8_t val)
{
    if (FLAG_IS_SET(FLAG_C)) {
        reg16_inc(&CPU.reg.pc, (int8_t)val);
    }
}

/* 0x39: Add 16-bit SP to HL. */
void add_hl_sp(void)
{
    CPU.reg.hl = add16(CPU.reg.hl, CPU.reg.sp);
}

/* 0x3a: Put value at address HL into A and decrement HL. */
void ldd_a_hlp(void)
{
    CPU.reg.a = mmu_read_byte(CPU.reg.hl--);
}

/* 0x3b: Decrement SP. */
void dec_sp(void)
{
    reg16_inc(&CPU.reg.sp, -1);
}

/* 0x3c: Increment A. */
void inc_a(void)
{
    CPU.reg.a = inc_n(CPU.reg.a);
}

/* 0x3d: Decrement A. */
void dec_a(void)
{
    CPU.reg.a = dec_n(CPU.reg.a);
}

/* 0x3e: Put value into A. */
void ld_a_n(uint8_t val)
{
    CPU.reg.a = val;
}

/* 0x3f: Complement carry flag. */
void ccf(void)
{
    CPU.reg.f ^= FLAG_C;
    FLAG_CLEAR(FLAG_N | FLAG_H);
}

/* 0x41: Copy C to B. */
void ld_b_c(void)
{
    CPU.reg.b = CPU.reg.c;
}

/* 0x42: Copy D to B. */
void ld_b_d(void)
{
    CPU.reg.b = CPU.reg.d;
}

/* 0x43: Copy E to B. */
void ld_b_e(void)
{
    CPU.reg.b = CPU.reg.e;
}

/* 0x44: Copy H to B. */
void ld_b_h(void)
{
    CPU.reg.b = CPU.reg.h;
}

/* 0x45: Copy L to B. */
void ld_b_l(void)
{
    CPU.reg.b = CPU.reg.l;
}

/* 0x46: Copy value pointed by HL into B. */
void ld_b_hlp(void)
{
    CPU.reg.b = mmu_read_byte(CPU.reg.hl);
}

/* 0x47: Copy A to B. */
void ld_b_a(void)
{
    CPU.reg.b = CPU.reg.a;
}

/* 0x48: Copy B to C. */
void ld_c_b(void)
{
    CPU.reg.c = CPU.reg.b;
}

/* 0x4a: Copy D to C. */
void ld_c_d(void)
{
    CPU.reg.c = CPU.reg.d;
}

/* 0x4b: Copy E to C. */
void ld_c_e(void)
{
    CPU.reg.c = CPU.reg.e;
}

/* 0x4c: Copy H to C. */
void ld_c_h(void)
{
    CPU.reg.c = CPU.reg.h;
}

/* 0x4d: Copy L to C. */
void ld_c_l(void)
{
    CPU.reg.c = CPU.reg.l;
}

/* 0x4e: Copy value pointed by HL into C. */
void ld_c_hlp(void)
{
    CPU.reg.c = mmu_read_byte(CPU.reg.hl);
}

/* 0x4f: Copy A to C. */
void ld_c_a(void)
{
    CPU.reg.c = CPU.reg.a;
}

/* 0x50: Copy B to D. */
void ld_d_b(void)
{
    CPU.reg.d = CPU.reg.b;
}

/* 0x51: Copy C to D. */
void ld_d_c(void)
{
    CPU.reg.d = CPU.reg.c;
}

/* 0x53: Copy E to D. */
void ld_d_e(void)
{
    CPU.reg.d = CPU.reg.e;
}

/* 0x54: Copy H to D. */
void ld_d_h(void)
{
    CPU.reg.d = CPU.reg.h;
}

/* 0x55: Copy L to D. */
void ld_d_l(void)
{
    CPU.reg.d = CPU.reg.l;
}

/* 0x56: Copy value pointed by HL into D. */
void ld_d_hlp(void)
{
    CPU.reg.d = mmu_read_byte(CPU.reg.hl);
}

/* 0x57: Copy A to D. */
void ld_d_a(void)
{
    CPU.reg.d = CPU.reg.a;
}

/* 0x58: Copy B to E. */
void ld_e_b(void)
{
    CPU.reg.e = CPU.reg.b;
}

/* 0x59: Copy C to E. */
void ld_e_c(void)
{
    CPU.reg.e = CPU.reg.c;
}

/* 0x5a: Copy D to E. */
void ld_e_d(void)
{
    CPU.reg.e = CPU.reg.d;
}

/* 0x5c: Copy H to E. */
void ld_e_h(void)
{
    CPU.reg.e = CPU.reg.h;
}

/* 0x5d: Copy L to E. */
void ld_e_l(void)
{
    CPU.reg.e = CPU.reg.l;
}

/* 0x5e: Copy value pointed by HL into E. */
void ld_e_hlp(void)
{
    CPU.reg.e = mmu_read_byte(CPU.reg.hl);
}

/* 0x5f: Copy A to E. */
void ld_e_a(void)
{
    CPU.reg.e = CPU.reg.a;
}

/* 0x60: Copy B to H. */
void ld_h_b(void)
{
    CPU.reg.h = CPU.reg.b;
}

/* 0x61: Copy C to H. */
void ld_h_c(void)
{
    CPU.reg.h = CPU.reg.c;
}

/* 0x62: Copy D to H. */
void ld_h_d(void)
{
    CPU.reg.h = CPU.reg.d;
}

/* 0x63: Copy E to H. */
void ld_h_e(void)
{
    CPU.reg.h = CPU.reg.e;
}

/* 0x65: Copy L to H. */
void ld_h_l(void)
{
    CPU.reg.h = CPU.reg.l;
}

/* 0x66: Copy value pointed by HL into H. */
void ld_h_hlp(void)
{
    CPU.reg.h = mmu_read_byte(CPU.reg.hl);
}

/* 0x67: Copy A to H. */
void ld_h_a(void)
{
    CPU.reg.h = CPU.reg.a;
}

/* 0x68: Copy B to L. */
void ld_l_b(void)
{
    CPU.reg.l = CPU.reg.b;
}

/* 0x69: Copy C to L. */
void ld_l_c(void)
{
    CPU.reg.l = CPU.reg.c;
}

/* 0x6a: Copy D to L. */
void ld_l_d(void)
{
    CPU.reg.l = CPU.reg.d;
}

/* 0x6b: Copy E to L. */
void ld_l_e(void)
{
    CPU.reg.l = CPU.reg.e;
}

/* 0x6c: Copy H to L. */
void ld_l_h(void)
{
    CPU.reg.l = CPU.reg.h;
}

/* 0x6e: Copy value pointed by HL into L. */
void ld_l_hlp(void)
{
    CPU.reg.l = mmu_read_byte(CPU.reg.hl);
}

/* 0x6f: Copy A to L. */
void ld_l_a(void)
{
    CPU.reg.l = CPU.reg.a;
}

/* 0x70: Save B to address pointed by HL. */
void ld_hlp_b(void)
{
    mmu_write_byte(CPU.reg.hl, CPU.reg.b);
}

/* 0x71: Save C to address pointed by HL. */
void ld_hlp_c(void)
{
    mmu_write_byte(CPU.reg.hl, CPU.reg.c);
}

/* 0x72: Save D to address pointed by HL. */
void ld_hlp_d(void)
{
    mmu_write_byte(CPU.reg.hl, CPU.reg.d);
}

/* 0x73: Save E to address pointed by HL. */
void ld_hlp_e(void)
{
    mmu_write_byte(CPU.reg.hl, CPU.reg.e);
}

/* 0x74: Save H to address pointed by HL. */
void ld_hlp_h(void)
{
    mmu_write_byte(CPU.reg.hl, CPU.reg.h);
}

/* 0x75: Save L to address pointed by HL. */
void ld_hlp_l(void)
{
    mmu_write_byte(CPU.reg.hl, CPU.reg.l);
}

/* 0x76: Power down CPU until an interrupt occurs. */
void halt(void)
{
    CPU.halt = true;
}

/* 0x77: Save A to address pointed by HL. */
void ld_hlp_a(void)
{
    mmu_write_byte(CPU.reg.hl, CPU.reg.a);
}

/* 0x78: Copy B to A. */
void ld_a_b(void)
{
    CPU.reg.a = CPU.reg.b;
}

/* 0x79: Copy C to A. */
void ld_a_c(void)
{
    CPU.reg.a = CPU.reg.c;
}

/* 0x7a: Copy D to A. */
void ld_a_d(void)
{
    CPU.reg.a = CPU.reg.d;
}

/* 0x7b: Copy E to A. */
void ld_a_e(void)
{
    CPU.reg.a = CPU.reg.e;
}

/* 0x7c: Copy H to A. */
void ld_a_h(void)
{
    CPU.reg.a = CPU.reg.h;
}

/* 0x7d: Copy L to A. */
void ld_a_l(void)
{
    CPU.reg.a = CPU.reg.l;
}

/* 0x7e: Copy value pointed by HL into A. */
void ld_a_hlp(void)
{
    CPU.reg.a = mmu_read_byte(CPU.reg.hl);
}

/* 0x80: Add B to A. */
void add_a_b(void)
{
    CPU.reg.a = add8(CPU.reg.a, CPU.reg.b);
}

/* 0x81: Add C to A. */
void add_a_c(void)
{
    CPU.reg.a = add8(CPU.reg.a, CPU.reg.c);
}

/* 0x82: Add D to A. */
void add_a_d(void)
{
    CPU.reg.a = add8(CPU.reg.a, CPU.reg.d);
}

/* 0x83: Add E to A. */
void add_a_e(void)
{
    CPU.reg.a = add8(CPU.reg.a, CPU.reg.e);
}

/* 0x84: Add H to A. */
void add_a_h(void)
{
    CPU.reg.a = add8(CPU.reg.a, CPU.reg.h);
}

/* 0x85: Add L to A. */
void add_a_l(void)
{
    CPU.reg.a = add8(CPU.reg.a, CPU.reg.l);
}

/* 0x86: Add value pointed by HL to A. */
void add_a_hlp(void)
{
    uint8_t val = mmu_read_byte(CPU.reg.hl);
    CPU.reg.a = add8(CPU.reg.a, val);
}

/* 0x87: Add A to A. */
void add_a_a(void)
{
    CPU.reg.a = add8(CPU.reg.a, CPU.reg.a);
}

/* 0x88: Add B and carry flag to A. */
void adc_b(void)
{
    adc(CPU.reg.b);
}

/* 0x89: Add C and carry flag to A. */
void adc_c(void)
{
    adc(CPU.reg.c);
}

/* 0x8a: Add D and carry flag to A. */
void adc_d(void)
{
    adc(CPU.reg.d);
}

/* 0x8b: Add E and carry flag to A. */
void adc_e(void)
{
    adc(CPU.reg.e);
}

/* 0x8c: Add H and carry flag to A. */
void adc_h(void)
{
    adc(CPU.reg.h);
}

/* 0x8d: Add L and carry flag to A. */
void adc_l(void)
{
    adc(CPU.reg.l);
}

/* 0x8e: Add (HL) and carry flag to A. */
void adc_hlp(void)
{
    adc(mmu_read_byte(CPU.reg.hl));
}

/* 0x8f: Add A and carry flag to A. */
void adc_a(void)
{
    adc(CPU.reg.a);
}

/* 0x90: Subtract B from A. */
void sub_b(void)
{
    sub(CPU.reg.b);
}

/* 0x91: Subtract C from A. */
void sub_c(void)
{
    sub(CPU.reg.c);
}

/* 0x92: Subtract D from A. */
void sub_d(void)
{
    sub(CPU.reg.d);
}

/* 0x93: Subtract E from A. */
void sub_e(void)
{
    sub(CPU.reg.e);
}

/* 0x94: Subtract H from A. */
void sub_h(void)
{
    sub(CPU.reg.h);
}

/* 0x95: Subtract L from A. */
void sub_l(void)
{
    sub(CPU.reg.l);
}

/* 0x96: Subtract (HL) from A. */
void sub_hlp(void)
{
    sub(mmu_read_byte(CPU.reg.hl));
}

/* 0x97: Subtract A from A. */
void sub_a(void)
{
    sub(CPU.reg.a);
}

/* 0x98: Subtract B and carry flag from A. */
void sbc_b(void)
{
    sbc(CPU.reg.b);
}

/* 0x99: Subtract C and carry flag from A. */
void sbc_c(void)
{
    sbc(CPU.reg.c);
}

/* 0x9a: Subtract D and carry flag from A. */
void sbc_d(void)
{
    sbc(CPU.reg.d);
}

/* 0x9b: Subtract E and carry flag from A. */
void sbc_e(void)
{
    sbc(CPU.reg.e);
}

/* 0x9c: Subtract H and carry flag from A. */
void sbc_h(void)
{
    sbc(CPU.reg.h);
}

/* 0x9d: Subtract L and carry flag from A. */
void sbc_l(void)
{
    sbc(CPU.reg.l);
}

/* 0x9e: Subtract (HL) and carry flag from A. */
void sbc_hlp(void)
{
    sbc(mmu_read_byte(CPU.reg.hl));
}

/* 0x9f: Subtract A and carry flag from A. */
void sbc_a(void)
{
    sbc(CPU.reg.a);
}

/* 0xa0: Bitwise AND B against A. */
void and_b(void)
{
    and8(CPU.reg.b);
}

/* 0xa1: Bitwise AND C against A. */
void and_c(void)
{
    and8(CPU.reg.c);
}

/* 0xa2: Bitwise AND D against A. */
void and_d(void)
{
    and8(CPU.reg.d);
}

/* 0xa3: Bitwise AND E against A. */
void and_e(void)
{
    and8(CPU.reg.e);
}

/* 0xa4: Bitwise AND H against A. */
void and_h(void)
{
    and8(CPU.reg.h);
}

/* 0xa5: Bitwise AND L against A. */
void and_l(void)
{
    and8(CPU.reg.l);
}

/* 0xa6: Bitwise AND (HL) against A. */
void and_hlp(void)
{
    and8(mmu_read_byte(CPU.reg.hl));
}

/* 0xa7: Bitwise AND A against A. */
void and_a(void)
{
    and8(CPU.reg.a);
}

/* 0xa8: Bitwise XOR B against A. */
void xor_b(void)
{
    xor8(CPU.reg.b);
}

/* 0xa9: Bitwise XOR C against A. */
void xor_c(void)
{
    xor8(CPU.reg.c);
}

/* 0xaa: Bitwise XOR D against A. */
void xor_d(void)
{
    xor8(CPU.reg.d);
}

/* 0xab: Bitwise XOR E against A. */
void xor_e(void)
{
    xor8(CPU.reg.e);
}

/* 0xac: Bitwise XOR H against A. */
void xor_h(void)
{
    xor8(CPU.reg.h);
}

/* 0xad: Bitwise XOR L against A. */
void xor_l(void)
{
    xor8(CPU.reg.l);
}

/* 0xae: Bitwise XOR (HL) against A. */
void xor_hlp(void)
{
    xor8(mmu_read_byte(CPU.reg.hl));
}

/* 0xaf: Bitwise XOR A against A. */
void xor_a(void)
{
    xor8(CPU.reg.a);
}

/* 0xb0: Bitwise OR B against A. */
void or_b(void)
{
    or8(CPU.reg.b);
}

/* 0xb1: Bitwise OR C against A. */
void or_c(void)
{
    or8(CPU.reg.c);
}

/* 0xb2: Bitwise OR D against A. */
void or_d(void)
{
    or8(CPU.reg.d);
}

/* 0xb3: Bitwise OR E against A. */
void or_e(void)
{
    or8(CPU.reg.e);
}

/* 0xb4: Bitwise OR H against A. */
void or_h(void)
{
    or8(CPU.reg.h);
}

/* 0xb5: Bitwise OR L against A. */
void or_l(void)
{
    or8(CPU.reg.l);
}

/* 0xb6: Bitwise OR (HL) against A. */
void or_hlp(void)
{
    or8(mmu_read_byte(CPU.reg.hl));
}

/* 0xb7: Bitwise OR A against A. */
void or_a(void)
{
    or8(CPU.reg.a);
}

/* 0xb8: Compare A with B. */
void cp_b(void)
{
    cp(CPU.reg.b);
}

/* 0xb9: Compare A with C. */
void cp_c(void)
{
    cp(CPU.reg.c);
}

/* 0xba: Compare A with D. */
void cp_d(void)
{
    cp(CPU.reg.d);
}

/* 0xbb: Compare A with E. */
void cp_e(void)
{
    cp(CPU.reg.e);
}

/* 0xbc: Compare A with H. */
void cp_h(void)
{
    cp(CPU.reg.h);
}

/* 0xbd: Compare A with L. */
void cp_l(void)
{
    cp(CPU.reg.l);
}

/* 0xbe: Compare A with (HL). */
void cp_hlp(void)
{
    cp(mmu_read_byte(CPU.reg.hl));
}

/* 0xbf: Compare A with A. */
void cp_a(void)
{
    cp(CPU.reg.a);
}

/* 0xc0: Return if Z flag is not set. */
void ret_nz(void)
{
    if (!FLAG_IS_SET(FLAG_Z)) {
        reg16_set(&CPU.reg.pc, pop());
    }
    clock_step(4);
}

/* 0xc1: Pop two bytes off stack into register pair nn. */
void pop_bc(void)
{
    CPU.reg.bc = pop();
}

/* 0xc2: Jump to address. */
void jp_nz_nn(uint16_t addr)
{
    if (!FLAG_IS_SET(FLAG_Z)) {
        reg16_set(&CPU.reg.pc, addr);
    }
}

/* 0xc3: Jump to address. */
void jp_nn(uint16_t addr)
{
    reg16_set(&CPU.reg.pc, addr);
}

/* 0xc4: Push PC to stack and Jump to address. */
void call_nz_nn(uint16_t addr)
{
    if (!FLAG_IS_SET(FLAG_Z)) {
        push(CPU.reg.pc);
        CPU.reg.pc = addr;
    }
}

/* 0xc5: Push BC to stack. */
void push_bc(void)
{
    push(CPU.reg.bc);
}

/* 0xc6: Add 8-bit immediate to A. */
void add_a_n(uint8_t val)
{
    CPU.reg.a = add8(CPU.reg.a, val);
}

/* 0xc7: Call routine at address 0x0000. */
void rst_00(void)
{
    push(CPU.reg.pc);
    CPU.reg.pc = 0x0000;
}

/* 0xc8: Return if Z flag is set. */
void ret_z(void)
{
    if (FLAG_IS_SET(FLAG_Z)) {
        reg16_set(&CPU.reg.pc, pop());
    }
    clock_step(4);
}

/* 0xc9: Return if Z flag is set. */
void ret(void)
{
    reg16_set(&CPU.reg.pc, pop());
}

/* 0xca: Jump to address. */
void jp_z_nn(uint16_t addr)
{
    if (FLAG_IS_SET(FLAG_Z)) {
        reg16_set(&CPU.reg.pc, addr);
    }
}

/* 0xcc: Push PC to stack and Jump to address. */
void call_z_nn(uint16_t addr)
{
    if (FLAG_IS_SET(FLAG_Z)) {
        push(CPU.reg.pc);
        CPU.reg.pc = addr;
    }
}

/* 0xcd: Push PC to stack and Jump to address. */
void call_nn(uint16_t addr)
{
    push(CPU.reg.pc);
    CPU.reg.pc = addr;
}

/* 0xce: Add immediate 8-bit value and carry flag to A. */
void adc_n(uint8_t n)
{
    adc(n);
}

/* 0xcf: Call routine at address 0x0008. */
void rst_08(void)
{
    push(CPU.reg.pc);
    CPU.reg.pc = 0x0008;
}

/* 0xd0: Return if C flag is not set. */
void ret_nc(void)
{
    if (!FLAG_IS_SET(FLAG_C)) {
        reg16_set(&CPU.reg.pc, pop());
    }
    clock_step(4);
}

/* 0xd1: Pop two bytes off stack into register pair nn. */
void pop_de(void)
{
    CPU.reg.de = pop();
}

/* 0xd2: Jump to address. */
void jp_nc_nn(uint16_t addr)
{
    if (!FLAG_IS_SET(FLAG_C)) {
        reg16_set(&CPU.reg.pc, addr);
    }
}

/* 0xd4: Push PC to stack and Jump to address. */
void call_nc_nn(uint16_t addr)
{
    if (!FLAG_IS_SET(FLAG_C)) {
        push(CPU.reg.pc);
        CPU.reg.pc = addr;
    }
}

/* 0xd5: Push DE to stack. */
void push_de(void)
{
    push(CPU.reg.de);
}

/* 0xd6: Subtract n from A. */
void sub_n(uint8_t val)
{
    sub(val);
}

/* 0xd7: Call routine at address 0x0010. */
void rst_10(void)
{
    push(CPU.reg.pc);
    CPU.reg.pc = 0x0010;
}

/* 0xd8: Return if C flag is set. */
void ret_c(void)
{
    if (FLAG_IS_SET(FLAG_C)) {
        reg16_set(&CPU.reg.pc, pop());
    }
    clock_step(4);
}

/* 0xd9: Pop two bytes from stack, jump to that address then enable interrupts.
 */
void reti(void)
{
    reg16_set(&CPU.reg.pc, pop());
    interrupt_set_master(1);
}

/* 0xda: Jump to address. */
void jp_c_nn(uint16_t addr)
{
    if (FLAG_IS_SET(FLAG_C)) {
        reg16_set(&CPU.reg.pc, addr);
    }
}

/* 0xdc: Push PC to stack and Jump to address. */
void call_c_nn(uint16_t addr)
{
    if (FLAG_IS_SET(FLAG_C)) {
        push(CPU.reg.pc);
        CPU.reg.pc = addr;
    }
}

/* 0xde: Subtract n and carry flag from A. */
void sbc_n(uint8_t val)
{
    sbc(val);
}

/* 0xdf: Call routine at address 0x0018. */
void rst_18(void)
{
    push(CPU.reg.pc);
    CPU.reg.pc = 0x0018;
}

/* 0xe0: Put A into memory address $FF00+n. */
void ldh_n_a(uint8_t val)
{
    uint16_t addr = (uint16_t)(0xff00 + val);
    mmu_write_byte(addr, CPU.reg.a);
}

/* 0xe1: Pop two bytes off stack into register pair nn. */
void pop_hl(void)
{
    CPU.reg.hl = pop();
}

/* 0xe2: Put A into address $FF00 + register C. */
void ld_cp_a(void)
{
    uint16_t addr = (uint16_t)(0xff00 + CPU.reg.c);
    mmu_write_byte(addr, CPU.reg.a);
}

/* 0xe5: Push HL to stack. */
void push_hl(void)
{
    push(CPU.reg.hl);
}

/* 0xe6: Bitwise AND n against A. */
void and_n(uint8_t val)
{
    and8(val);
}

/* 0xe7: Call routine at address 0x0020. */
void rst_20(void)
{
    push(CPU.reg.pc);
    CPU.reg.pc = 0x0020;
}

/* 0xe8: Add n to Stack Pointer (SP). */
void add_sp_n(uint8_t val)
{
    if (((CPU.reg.sp & 0xff) + (val & 0xff)) > 0xff) {
        FLAG_SET(FLAG_C);
    } else {
        FLAG_CLEAR(FLAG_C);
    }
    if (((CPU.reg.sp & 0x0f) + (val & 0x0f)) > 0x0f) {
        FLAG_SET(FLAG_H);
    } else {
        FLAG_CLEAR(FLAG_H);
    }
    FLAG_CLEAR(FLAG_Z | FLAG_N);
    CPU.reg.sp = (uint16_t)(CPU.reg.sp + (int8_t)val);
    clock_step(8);
}

/* 0xe9: Jump to address. */
void jp_hl(void)
{
    CPU.reg.pc = CPU.reg.hl;
}

/* 0xea: Save A at given 16-bit address. */
void ld_nnp_a(uint16_t addr)
{
    mmu_write_byte(addr, CPU.reg.a);
}

/* 0xee: Bitwise XOR n against A. */
void xor_n(uint8_t val)
{
    xor8(val);
}

/* 0xef: Call routine at address 0x0028. */
void rst_28(void)
{
    push(CPU.reg.pc);
    CPU.reg.pc = 0x0028;
}

/* 0xf0: Put memory address $FF00+n into A. */
void ldh_a_n(uint8_t val)
{
    uint16_t addr = (uint16_t)(0xff00 + val);
    CPU.reg.a = mmu_read_byte(addr);
}

/* 0xf1: Pop two bytes off stack into register pair nn. */
void pop_af(void)
{
    CPU.reg.af = pop() & 0xfff0;
}

/* 0xf2: Put value at address $FF00 + register C into A. */
void ld_a_cp(void)
{
    uint16_t addr = (uint16_t)(0xff00 + CPU.reg.c);
    CPU.reg.a = mmu_read_byte(addr);
}

/* 0xf3: This instruction disables interrupts after the next instruction is
 * executed.
 */
void di(void)
{
    interrupt_set_master(0);
}

/* 0xf5: Push AF to stack. */
void push_af(void)
{
    push(CPU.reg.af);
}

/* 0xf6: Bitwise OR n against A. */
void or_n(uint8_t val)
{
    or8(val);
}

/* 0xf7: Call routine at address 0x0030. */
void rst_30(void)
{
    push(CPU.reg.pc);
    CPU.reg.pc = 0x0030;
}

/* 0xf8: Put SP + n effective address into HL. */
void ldhl_sp_n(uint8_t val)
{
    if (((CPU.reg.sp & 0xff) + (val & 0xff)) > 0xff) {
        FLAG_SET(FLAG_C);
    } else {
        FLAG_CLEAR(FLAG_C);
    }
    if (((CPU.reg.sp & 0x0f) + (val & 0x0f)) > 0x0f) {
        FLAG_SET(FLAG_H);
    } else {
        FLAG_CLEAR(FLAG_H);
    }
    FLAG_CLEAR(FLAG_Z | FLAG_N);
    CPU.reg.hl = (uint16_t)(CPU.reg.sp + (int8_t)val);
    clock_step(4);
}

/* 0xf9: Put HL into Stack Pointer (SP). */
void ld_sp_hl(void)
{
    reg16_set(&CPU.reg.sp, CPU.reg.hl);
}

/* 0xfa: Copy value pointed by addr into A. */
void ld_a_nnp(uint16_t addr)
{
    CPU.reg.a = mmu_read_byte(addr);
}

/* 0xfb: This instruction enables interrupts after the next instruction is
 * executed.
 */
void ei(void)
{
    interrupt_set_master(1);
}

/* 0xfe: Compare A with n. */
void cp_n(uint8_t val)
{
    cp(val);
}

/* 0xff: Call routine at address 0x0038. */
void rst_38(void)
{
    push(CPU.reg.pc);
    CPU.reg.pc = 0x0038;
}
