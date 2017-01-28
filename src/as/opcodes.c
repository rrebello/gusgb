#include "opcodes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern unsigned int linenum;
extern FILE *output;
unsigned int pc = 0;

const char *regs[REG_LEN] = {"a", "f", "b",  "c",  "d",  "e",
                             "h", "l", "af", "bc", "de", "hl"};

void error_register(register_e reg)
{
    fprintf(stderr, "%u: invalid register: %s\n", linenum, regs[reg]);
    exit(EXIT_FAILURE);
}

void error_bit(unsigned int bit)
{
    fprintf(stderr, "%u: invalid bit: %u\n", linenum, bit);
    exit(EXIT_FAILURE);
}

static inline void op_write1(uint8_t opcode)
{
    fwrite(&opcode, 1, 1, output);
    pc += 1;
}

static inline void op_write2(uint8_t opcode, uint8_t val)
{
    uint8_t buf[2];
    buf[0] = opcode;
    buf[1] = val;
    fwrite(buf, 2, 1, output);
    pc += 2;
}

static inline void op_write3(uint8_t opcode, uint16_t val)
{
    uint8_t buf[3];
    buf[0] = opcode;
    buf[1] = (uint8_t)(val & 0x00ff);
    buf[2] = (uint8_t)((val & 0xff00) >> 8);
    fwrite(buf, 3, 1, output);
    pc += 3;
}

static inline void op_write_cb(uint8_t opcode)
{
    uint8_t buf[2];
    buf[0] = 0xcb;
    buf[1] = opcode;
    fwrite(buf, 2, 1, output);
    pc += 2;
}

void ascii(char *str)
{
    size_t size = strlen(str) - 1;
    for (int i = 1; i < size; ++i) {
        op_write1(str[i]);
    }
}

void data(uint8_t val)
{
    op_write1(val);
}

void jump(long offset)
{
    pc = offset;
    fseek(output, offset, SEEK_SET);
}

void memsetf(uint8_t c, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        op_write1(c);
    }
}

void nop(void)
{
    op_write1(0x00);
}

void ld_bc_nn(uint16_t val)
{
    op_write3(0x01, val);
}

void ld_bcp_a(void)
{
    op_write1(0x02);
}

void inc_bc(void)
{
    op_write1(0x03);
}

void inc_b(void)
{
    op_write1(0x04);
}

void dec_b(void)
{
    op_write1(0x05);
}

void ld_b_n(uint8_t val)
{
    op_write2(0x06, val);
}

void rlca(void)
{
    op_write1(0x07);
}

void ld_nnp_sp(uint16_t val)
{
    op_write3(0x08, val);
}

void add_hl_bc(void)
{
    op_write1(0x09);
}

void ld_a_bcp(void)
{
    op_write1(0x0a);
}

void dec_bc(void)
{
    op_write1(0x0b);
}

void inc_c(void)
{
    op_write1(0x0c);
}

void dec_c(void)
{
    op_write1(0x0d);
}

void ld_c_n(uint8_t val)
{
    op_write2(0x0e, val);
}

void rrca(void)
{
    op_write1(0x0f);
}

void stop(void)
{
    op_write1(0x10);
}

void ld_de_nn(uint16_t val)
{
    op_write3(0x11, val);
}

void ld_dep_a(void)
{
    op_write1(0x12);
}

void inc_de(void)
{
    op_write1(0x13);
}

void inc_d(void)
{
    op_write1(0x14);
}

void dec_d(void)
{
    op_write1(0x15);
}

void ld_d_n(uint8_t val)
{
    op_write2(0x16, val);
}

void rla(void)
{
    op_write1(0x17);
}

void jr_n(uint8_t val)
{
    op_write2(0x18, val);
}

void add_hl_de(void)
{
    op_write1(0x19);
}

void ld_a_dep(void)
{
    op_write1(0x1a);
}

void dec_de(void)
{
    op_write1(0x1b);
}

void inc_e(void)
{
    op_write1(0x1c);
}

void dec_e(void)
{
    op_write1(0x1d);
}

void ld_e_n(uint8_t val)
{
    op_write2(0x1e, val);
}

void rra(void)
{
    op_write1(0x1f);
}

void jr_nz_n(uint8_t val)
{
    op_write2(0x20, val);
}

void ld_hl_nn(uint16_t val)
{
    op_write3(0x21, val);
}

void ldi_hlp_a(void)
{
    op_write1(0x22);
}

void inc_hl(void)
{
    op_write1(0x23);
}

void inc_h(void)
{
    op_write1(0x24);
}

void dec_h(void)
{
    op_write1(0x25);
}

void ld_h_n(uint8_t val)
{
    op_write2(0x26, val);
}

void daa(void)
{
    op_write1(0x27);
}

void jr_z_n(uint8_t val)
{
    op_write2(0x28, val);
}

void add_hl_hl(void)
{
    op_write1(0x29);
}

void ldi_a_hlp(void)
{
    op_write1(0x2a);
}

void dec_hl(void)
{
    op_write1(0x2b);
}

void inc_l(void)
{
    op_write1(0x2c);
}

void dec_l(void)
{
    op_write1(0x2d);
}

void ld_l_n(uint8_t val)
{
    op_write2(0x2e, val);
}

void cpl(void)
{
    op_write1(0x2f);
}

void jr_nc_n(uint8_t val)
{
    op_write2(0x30, val);
}

void ld_sp_nn(uint16_t val)
{
    op_write3(0x31, val);
}

void ldd_hlp_a(void)
{
    op_write1(0x32);
}

void inc_sp(void)
{
    op_write1(0x33);
}

void inc_hlp(void)
{
    op_write1(0x34);
}

void dec_hlp(void)
{
    op_write1(0x35);
}

void ld_hlp_n(uint8_t val)
{
    op_write2(0x36, val);
}

void scf(void)
{
    op_write1(0x37);
}

void jr_c_n(uint8_t val)
{
    op_write2(0x38, val);
}

void add_hl_sp(void)
{
    op_write1(0x39);
}

void ldd_a_hlp(void)
{
    op_write1(0x3a);
}

void dec_sp(void)
{
    op_write1(0x3b);
}

void inc_a(void)
{
    op_write1(0x3c);
}

void dec_a(void)
{
    op_write1(0x3d);
}

void ld_a_n(uint8_t val)
{
    op_write2(0x3e, val);
}

void ccf(void)
{
    op_write1(0x3f);
}

void ld_b_b(void)
{
    op_write1(0x40);
}

void ld_b_c(void)
{
    op_write1(0x41);
}

void ld_b_d(void)
{
    op_write1(0x42);
}

void ld_b_e(void)
{
    op_write1(0x43);
}

void ld_b_h(void)
{
    op_write1(0x44);
}

void ld_b_l(void)
{
    op_write1(0x45);
}

void ld_b_hlp(void)
{
    op_write1(0x46);
}

void ld_b_a(void)
{
    op_write1(0x47);
}

void ld_c_b(void)
{
    op_write1(0x48);
}

void ld_c_c(void)
{
    op_write1(0x49);
}

void ld_c_d(void)
{
    op_write1(0x4a);
}

void ld_c_e(void)
{
    op_write1(0x4b);
}

void ld_c_h(void)
{
    op_write1(0x4c);
}

void ld_c_l(void)
{
    op_write1(0x4d);
}

void ld_c_hlp(void)
{
    op_write1(0x4e);
}

void ld_c_a(void)
{
    op_write1(0x4f);
}

void ld_d_b(void)
{
    op_write1(0x50);
}

void ld_d_c(void)
{
    op_write1(0x51);
}

void ld_d_d(void)
{
    op_write1(0x52);
}

void ld_d_e(void)
{
    op_write1(0x53);
}

void ld_d_h(void)
{
    op_write1(0x54);
}

void ld_d_l(void)
{
    op_write1(0x55);
}

void ld_d_hlp(void)
{
    op_write1(0x56);
}

void ld_d_a(void)
{
    op_write1(0x57);
}

void ld_e_b(void)
{
    op_write1(0x58);
}

void ld_e_c(void)
{
    op_write1(0x59);
}

void ld_e_d(void)
{
    op_write1(0x5a);
}

void ld_e_e(void)
{
    op_write1(0x5b);
}

void ld_e_h(void)
{
    op_write1(0x5c);
}

void ld_e_l(void)
{
    op_write1(0x5d);
}

void ld_e_hlp(void)
{
    op_write1(0x5e);
}

void ld_e_a(void)
{
    op_write1(0x5f);
}

void ld_h_b(void)
{
    op_write1(0x60);
}

void ld_h_c(void)
{
    op_write1(0x61);
}

void ld_h_d(void)
{
    op_write1(0x62);
}

void ld_h_e(void)
{
    op_write1(0x63);
}

void ld_h_h(void)
{
    op_write1(0x64);
}

void ld_h_l(void)
{
    op_write1(0x65);
}

void ld_h_hlp(void)
{
    op_write1(0x66);
}

void ld_h_a(void)
{
    op_write1(0x67);
}

void ld_l_b(void)
{
    op_write1(0x68);
}

void ld_l_c(void)
{
    op_write1(0x69);
}

void ld_l_d(void)
{
    op_write1(0x6a);
}

void ld_l_e(void)
{
    op_write1(0x6b);
}

void ld_l_h(void)
{
    op_write1(0x6c);
}

void ld_l_l(void)
{
    op_write1(0x6d);
}

void ld_l_hlp(void)
{
    op_write1(0x6e);
}

void ld_l_a(void)
{
    op_write1(0x6f);
}

void ld_hlp_b(void)
{
    op_write1(0x70);
}

void ld_hlp_c(void)
{
    op_write1(0x71);
}

void ld_hlp_d(void)
{
    op_write1(0x72);
}

void ld_hlp_e(void)
{
    op_write1(0x73);
}

void ld_hlp_h(void)
{
    op_write1(0x74);
}

void ld_hlp_l(void)
{
    op_write1(0x75);
}

void halt(void)
{
    op_write1(0x76);
}

void ld_hlp_a(void)
{
    op_write1(0x77);
}

void ld_a_b(void)
{
    op_write1(0x78);
}

void ld_a_c(void)
{
    op_write1(0x79);
}

void ld_a_d(void)
{
    op_write1(0x7a);
}

void ld_a_e(void)
{
    op_write1(0x7b);
}

void ld_a_h(void)
{
    op_write1(0x7c);
}

void ld_a_l(void)
{
    op_write1(0x7d);
}

void ld_a_hlp(void)
{
    op_write1(0x7e);
}

void ld_a_a(void)
{
    op_write1(0x7f);
}

void add_a_b(void)
{
    op_write1(0x80);
}

void add_a_c(void)
{
    op_write1(0x81);
}

void add_a_d(void)
{
    op_write1(0x82);
}

void add_a_e(void)
{
    op_write1(0x83);
}

void add_a_h(void)
{
    op_write1(0x84);
}

void add_a_l(void)
{
    op_write1(0x85);
}

void add_a_hlp(void)
{
    op_write1(0x86);
}

void add_a_a(void)
{
    op_write1(0x87);
}

void adc(register_e reg)
{
    unsigned int opcode = 0x88;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write1((uint8_t)opcode);
}

void sub(register_e reg)
{
    unsigned int opcode = 0x90;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write1((uint8_t)opcode);
}

void sbc(register_e reg)
{
    unsigned int opcode = 0x98;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write1((uint8_t)opcode);
}

void andf(register_e reg)
{
    unsigned int opcode = 0xa0;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write1((uint8_t)opcode);
}

void xorf(register_e reg)
{
    unsigned int opcode = 0xa8;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write1((uint8_t)opcode);
}

void orf(register_e reg)
{
    unsigned int opcode = 0xb0;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write1((uint8_t)opcode);
}

void cp(register_e reg)
{
    unsigned int opcode = 0xb8;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write1((uint8_t)opcode);
}

void ret_nz(void)
{
    op_write1(0xc0);
}

void pop_bc(void)
{
    op_write1(0xc1);
}

void jp_nz_nn(uint16_t val)
{
    op_write3(0xc2, val);
}

void jp_nn(uint16_t val)
{
    op_write3(0xc3, val);
}

void call_nz_nn(uint16_t val)
{
    op_write3(0xc4, val);
}

void push_bc(void)
{
    op_write1(0xc5);
}

void add_a_n(uint8_t val)
{
    op_write2(0xc6, val);
}

void rst(uint8_t val)
{
    if (val == 0x00) {
        op_write1(0xc7);
    } else if (val == 0x08) {
        op_write1(0xcf);
    } else if (val == 0x10) {
        op_write1(0xd7);
    } else if (val == 0x18) {
        op_write1(0xdf);
    } else if (val == 0x20) {
        op_write1(0xe7);
    } else if (val == 0x28) {
        op_write1(0xef);
    } else if (val == 0x30) {
        op_write1(0xf7);
    } else if (val == 0x38) {
        op_write1(0xff);
    } else {
        fprintf(stderr, "rst: invalid value: %hhu\n", val);
        exit(EXIT_FAILURE);
    }
}

void ret_z(void)
{
    op_write1(0xc8);
}

void ret(void)
{
    op_write1(0xc9);
}

void jp_z_nn(uint16_t val)
{
    op_write3(0xca, val);
}

void cb_n(uint8_t val)
{
    op_write2(0xcb, val);
}

void call_z_nn(uint16_t val)
{
    op_write3(0xcc, val);
}

void call_nn(uint16_t val)
{
    op_write3(0xcd, val);
}

void adc_n(uint8_t val)
{
    op_write2(0xce, val);
}

void ret_nc(void)
{
    op_write1(0xd0);
}

void pop_de(void)
{
    op_write1(0xd1);
}

void jp_nc_nn(uint16_t val)
{
    op_write3(0xd2, val);
}

void call_nc_nn(uint16_t val)
{
    op_write3(0xd4, val);
}

void push_de(void)
{
    op_write1(0xd5);
}

void sub_n(uint8_t val)
{
    op_write2(0xd6, val);
}

void ret_c(void)
{
    op_write1(0xd8);
}

void reti(void)
{
    op_write1(0xd9);
}

void jp_c_nn(uint16_t val)
{
    op_write3(0xda, val);
}

void call_c_nn(uint16_t val)
{
    op_write3(0xdc, val);
}

void sbc_n(uint8_t val)
{
    op_write2(0xde, val);
}

void ldh_n_a(uint8_t val)
{
    op_write2(0xe0, val);
}

void pop_hl(void)
{
    op_write1(0xe1);
}

void ld_cp_a(void)
{
    op_write1(0xe2);
}

void push_hl(void)
{
    op_write1(0xe5);
}

void and_n(uint8_t val)
{
    op_write2(0xe6, val);
}

void add_sp_n(uint8_t val)
{
    op_write2(0xe8, val);
}

void jp_hl(void)
{
    op_write1(0xe9);
}

void ld_nnp_a(uint16_t val)
{
    op_write3(0xea, val);
}

void xor_n(uint8_t val)
{
    op_write2(0xee, val);
}

void ldh_a_n(uint8_t val)
{
    op_write2(0xf0, val);
}

void pop_af(void)
{
    op_write1(0xf1);
}

void ld_a_cp(void)
{
    op_write1(0xf2);
}

void di(void)
{
    op_write1(0xf3);
}

void push_af(void)
{
    op_write1(0xf5);
}

void or_n(uint8_t val)
{
    op_write2(0xf6, val);
}

void ldhl_sp_n(uint8_t val)
{
    op_write2(0xf8, val);
}

void ld_sp_hl(void)
{
    op_write1(0xf9);
}

void ld_a_nnp(uint16_t val)
{
    op_write3(0xfa, val);
}

void ei(void)
{
    op_write1(0xfb);
}

void cp_n(uint8_t val)
{
    op_write2(0xfe, val);
}

/* CB */

void rlc(register_e reg)
{
    unsigned int opcode = 0x00;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write_cb(opcode);
}

void rrc(register_e reg)
{
    unsigned int opcode = 0x08;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write_cb(opcode);
}

void rl(register_e reg)
{
    unsigned int opcode = 0x10;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write_cb(opcode);
}

void rr(register_e reg)
{
    unsigned int opcode = 0x18;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write_cb(opcode);
}

void sla(register_e reg)
{
    unsigned int opcode = 0x20;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write_cb(opcode);
}

void sra(register_e reg)
{
    unsigned int opcode = 0x28;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write_cb(opcode);
}

void swap(register_e reg)
{
    unsigned int opcode = 0x30;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write_cb(opcode);
}

void srl(register_e reg)
{
    unsigned int opcode = 0x38;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write_cb(opcode);
}

void bit(unsigned int b, register_e reg)
{
    if (b > 7) {
        error_bit(b);
    }
    unsigned int opcode = 0x40u + 8u * b;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write_cb((uint8_t)opcode);
}

void res(unsigned int b, register_e reg)
{
    if (b > 7) {
        error_bit(b);
    }
    unsigned int opcode = 0x80u + 8u * b;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write_cb((uint8_t)opcode);
}

void set(unsigned int b, register_e reg)
{
    if (b > 7) {
        error_bit(b);
    }
    unsigned int opcode = 0xc0u + 8u * b;
    switch (reg) {
        case REG_B:
            opcode += 0;
            break;
        case REG_C:
            opcode += 1;
            break;
        case REG_D:
            opcode += 2;
            break;
        case REG_E:
            opcode += 3;
            break;
        case REG_H:
            opcode += 4;
            break;
        case REG_L:
            opcode += 5;
            break;
        case REG_HL:
            opcode += 6;
            break;
        case REG_A:
            opcode += 7;
            break;
        default:
            error_register(reg);
    }
    op_write_cb((uint8_t)opcode);
}
