#ifndef ROM_H
#define ROM_H

#include "stdint.h"

enum {
    ROM_MIRRORING_VERTICAL,
    ROM_MIRRORING_HORIZONTAL,
    ROM_MIRRORING_SINGLE_UPPER,
    ROM_MIRRORING_SINGLE_LOWER,
};

enum {
    ROM_INTERRUPT_NONE,
    ROM_INTERRUPT_NMI,
    ROM_INTERRUPT_RESET,
    ROM_INTERRUPT_IRQ,
};

enum {
    ROM_BUTTON_A,
    ROM_BUTTON_B,
    ROM_BUTTON_SELECT,
    ROM_BUTTON_START,
    ROM_BUTTON_UP,
    ROM_BUTTON_DOWN,
    ROM_BUTTON_LEFT,
    ROM_BUTTON_RIGHT,
};

enum {
    ROM_PAD_STATE_OFF = 0x40,
    ROM_PAD_STATE_ON = 0x41,
};

uint8_t rom_mirroring;
uint8_t rom_chr_bank_count;

// write the chr rom into dest
// void rom_read_chr(uint8_t* dest);

// // starts executing the PRG ROM.
// // this function returns when the RTI instruction is executed,
// // or the program exits.
// // when an interrupt occurs, call rom_start with the interrupt
// // index.
// void rom_start(uint8_t interrupt);

// called after every instruction with the number of
// cpu cycles that have passed.
void rom_cycle(uint8_t t);

// PPU hooks
uint8_t rom_ppu_read_status();
uint8_t rom_ppu_read_oamdata();
uint8_t rom_ppu_read_data();
void rom_ppu_write_control(uint8_t);
void rom_ppu_write_mask(uint8_t);
void rom_ppu_write_oamaddress(uint8_t);
void rom_ppu_write_oamdata(uint8_t);
void rom_ppu_write_scroll(uint8_t);
void rom_ppu_write_address(uint8_t);
void rom_ppu_write_data(uint8_t);
void rom_ppu_write_dma(uint8_t);

// APU hooks
uint8_t rom_apu_read_status();
void rom_apu_write_square1control(uint8_t);
void rom_apu_write_square1sweeps(uint8_t);
void rom_apu_write_square1low(uint8_t);
void rom_apu_write_square1high(uint8_t);
void rom_apu_write_square2control(uint8_t);
void rom_apu_write_square2sweeps(uint8_t);
void rom_apu_write_square2low(uint8_t);
void rom_apu_write_square2high(uint8_t);
void rom_apu_write_trianglecontrol(uint8_t);
void rom_apu_write_trianglelow(uint8_t);
void rom_apu_write_trianglehigh(uint8_t);
void rom_apu_write_noisebase(uint8_t);
void rom_apu_write_noiseperiod(uint8_t);
void rom_apu_write_noiselength(uint8_t);
void rom_apu_write_dmcflags(uint8_t);
void rom_apu_write_dmcdirectload(uint8_t);
void rom_apu_write_dmcsampleaddress(uint8_t);
void rom_apu_write_dmcsamplelength(uint8_t);
void rom_apu_write_controlflags1(uint8_t);
void rom_apu_write_controlflags2(uint8_t);

typedef void (*rom_set_button_state_func) (uint8_t padIndex, uint8_t buttonIndex, uint8_t value);
typedef uint8_t (*rom_ram_read_func) (uint16_t addr);
typedef void (*rom_read_chr_func) (uint8_t* dest);
typedef void (*rom_start_func) (uint8_t interrupt);

void mainentry(rom_set_button_state_func set_button,
    rom_ram_read_func ram_read,
    rom_read_chr_func read_chr,
    rom_start_func start);
// // controller
// void rom_set_button_state(uint8_t padIndex, uint8_t buttonIndex, uint8_t value);

// // RAM
// uint8_t rom_ram_read(uint16_t addr);

#endif /*  ROM_H */
