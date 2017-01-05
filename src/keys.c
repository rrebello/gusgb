#include <stdio.h>
#include "keys.h"
#include "interrupt.h"

typedef struct {
    uint8_t rows[2];
    uint8_t column;
} keys_t;

keys_t KEY;

void keys_init(void)
{
    KEY.rows[0] = 0x0F;
    KEY.rows[1] = 0x0F;
    KEY.column = 0;
}

uint8_t keys_read(void)
{
    switch (KEY.column) {
        case 0x10:
            printf("KEY.rows[0]=0x%02x\n", KEY.rows[0]);
            return KEY.rows[0];
        case 0x20:
            printf("KEY.rows[1]=0x%02x\n", KEY.rows[1]);
            return KEY.rows[1];
        default:
            return 0;
    }
}

void keys_write(uint8_t value)
{
    printf("KEY.column=0x%02x\n", value & 0x30);
    KEY.column = value & 0x30;
}

void key_press(key_e key)
{
    switch (key) {
        case KEY_START:
            KEY.rows[0] &= 0x7;
            break;
        case KEY_SELECT:
            KEY.rows[0] &= 0xb;
            break;
        case KEY_B:
            KEY.rows[0] &= 0xd;
            break;
        case KEY_A:
            KEY.rows[0] &= 0xe;
            break;
        case KEY_DOWN:
            KEY.rows[1] &= 0x7;
            break;
        case KEY_UP:
            KEY.rows[1] &= 0xb;
            break;
        case KEY_LEFT:
            KEY.rows[1] &= 0xd;
            break;
        case KEY_RIGHT:
            KEY.rows[1] &= 0xe;
            break;
        default:
            printf("%s:%d: unknown key press: %d\n", __func__, __LINE__, key);;
            break;
    }
    interrupt_set_flag_bit(INTERRUPTS_JOYPAD);
}

void key_release(key_e key)
{
    switch (key) {
        case KEY_START:
            KEY.rows[0] |= 0x8;
            break;
        case KEY_SELECT:
            KEY.rows[0] |= 0x4;
            break;
        case KEY_B:
            KEY.rows[0] |= 0x2;
            break;
        case KEY_A:
            KEY.rows[0] |= 0x1;
            break;
        case KEY_DOWN:
            KEY.rows[1] |= 0x8;
            break;
        case KEY_UP:
            KEY.rows[1] |= 0x4;
            break;
        case KEY_LEFT:
            KEY.rows[1] |= 0x2;
            break;
        case KEY_RIGHT:
            KEY.rows[1] |= 0x1;
            break;
        default:
            printf("%s:%d: unknown key release: %d\n", __func__, __LINE__, key);;
            break;
    }
    interrupt_set_flag_bit(INTERRUPTS_JOYPAD);
}