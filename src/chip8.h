#pragma once

#include <array>
#include <cstdint>
#include <random>

constexpr size_t MEMORY_SIZE  = 4096;   /* number of bytes in a 4K memory */
constexpr size_t STACK_SIZE   = 16;     /* number of layers on the stack */
constexpr size_t DISPLAY_SIZE  = 2048;  /* size of a 32 * 64 pixel screen */
constexpr size_t NUM_REGISTERS = 16;    /* number of general-purpose registers */
constexpr size_t NUM_KEYS  = 16;        /* number of keys on the keypad */

class Chip8 {
    public: 
        Chip8();
        void load_rom(const std::string&);
        void play(void);
        void execute_instruction(void);
        void decrement_timers(void);
        uint8_t pixel_state(const size_t&);
        void handle_keypress(const size_t&);
        bool get_draw_flag(void);
        void set_draw_flag(const bool&);

    private:
        std::array<uint16_t, STACK_SIZE> stack;
        std::array<uint8_t, MEMORY_SIZE> memory;
        std::array<uint8_t, DISPLAY_SIZE> display;
        std::array<uint8_t, NUM_REGISTERS> registers;
        std::array<bool, NUM_KEYS> keys;
        uint16_t sp;
        uint16_t pc;
        uint16_t index;
        uint8_t delay_timer;
        uint8_t sound_timer;
        bool draw_flag;
        std::mt19937 mt;
};