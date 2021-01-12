#pragma once

#include <array>
#include <cstdint>
#include <stack>
#include <random>

const size_t MEMORY_SIZE   = 4096;  /* number of bytes in a 4K memory */
const size_t STACK_SIZE    = 16;    /* number of layers on the stack */
const size_t DISPLAY_SIZE  = 2048;  /* size of a 32 * 64 pixel screen */
const size_t NUM_REGISTERS = 16;    /* number of general-purpose registers */
const size_t NUM_KEYS      = 16;    /* number of keys on the keypad */

class Chip8 {
    public: 
        Chip8();

        void load_rom(const std::string&);
        void execute_instruction(void);
        void decrement_timers(void);
        uint8_t get_pixel_state(const size_t&);
        void handle_keypress(const size_t&);
        bool get_draw_flag(void);
        void set_draw_flag(const bool&);

    private:
        std::array<uint8_t, MEMORY_SIZE> memory;
        std::array<uint8_t, DISPLAY_SIZE> pixels;
        std::array<uint8_t, NUM_REGISTERS> registers;
        std::array<bool, NUM_KEYS> keys;
        std::stack<uint16_t> stack;
        uint16_t pc;
        uint16_t index;
        uint8_t delay_timer;
        uint8_t sound_timer;
        bool draw_flag;
        std::mt19937 gen;

        const std::array<uint8_t, 80> fontset = {
            0xF0, 0x90, 0x90, 0x90, 0xF0,   /* 0 */
            0x20, 0x60, 0x20, 0x20, 0x70,   /* 1 */
            0xF0, 0x10, 0xF0, 0x80, 0xF0,   /* 2 */
            0xF0, 0x10, 0xF0, 0x10, 0xF0,   /* 3 */
            0x90, 0x90, 0xF0, 0x10, 0x10,   /* 4 */
            0xF0, 0x80, 0xF0, 0x10, 0xF0,   /* 5 */
            0xF0, 0x80, 0xF0, 0x90, 0xF0,   /* 6 */
            0xF0, 0x10, 0x20, 0x40, 0x40,   /* 7 */
            0xF0, 0x90, 0xF0, 0x90, 0xF0,   /* 8 */
            0xF0, 0x90, 0xF0, 0x10, 0xF0,   /* 9 */
            0xF0, 0x90, 0xF0, 0x90, 0x90,   /* A */
            0xE0, 0x90, 0xE0, 0x90, 0xE0,   /* B */
            0xF0, 0x80, 0x80, 0x80, 0xF0,   /* C */
            0xE0, 0x90, 0x90, 0x90, 0xE0,   /* D */
            0xF0, 0x80, 0xF0, 0x80, 0xF0,   /* E */
            0xF0, 0x80, 0xF0, 0x80, 0x80    /* F */
        };
};