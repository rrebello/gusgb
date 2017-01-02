#ifndef __MMU_H__
#define __MMU_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t rom[0x8000];  /* Cartridge ROM. */
    uint8_t vram[0x2000]; /* Video RAM. */
    uint8_t eram[0x2000]; /* External RAM. */
    uint8_t wram[0x2000]; /* Working RAM. */
    uint8_t oam[0x100];   /* Sprite info. */
    uint8_t zram[0x80];   /* Zero-page RAM. */
    uint8_t io[0x100];    /* Memory-mapped I/O. */
    uint8_t read_ext_rom;
    uint8_t cart_type;
} mmu_t;

/* Init MMU subsystem. */
int mmu_init(const char *rom_path);

/* Read byte from a given address. */
uint8_t mmu_read_byte(uint16_t addr);

/* Read word from a given address. */
uint16_t mmu_read_word(uint16_t addr);

/* Write byte to a given address. */
void mmu_write_byte(uint16_t addr, uint8_t value);

/* Write word to a given address. */
void mmu_write_word(uint16_t addr, uint16_t value);

#endif /* __MMU_H__ */
