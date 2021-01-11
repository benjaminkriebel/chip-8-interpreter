#include <fstream>
#include <iostream>
#include <vector>
#include "chip8.h"

constexpr size_t INSTR_START  = 512;            /* address of first instruction in memory */
constexpr std::array<uint8_t, 80> FONTSET = {   /* default fontset */
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

Chip8::Chip8() :
    stack(),
    memory(),
    display(),
    registers(),
    keys(),
    sp(0),
    pc(INSTR_START),
    index(0),
    delay_timer(0),
    sound_timer(0),
    draw_flag(false),
    mt(std::random_device()())
{
    std::copy(FONTSET.begin(), FONTSET.end(), memory.begin());
}

void Chip8::load_rom(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);

    file.seekg(0, std::ios::end);
    std::ifstream::pos_type filesize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(filesize);
    file.read(reinterpret_cast<char *>(buffer.data()), filesize);
    std::copy(buffer.begin(), buffer.end(), memory.begin() + INSTR_START);
}

void Chip8::execute_instruction(void) {
    uint16_t opcode = memory[pc] << 8 | memory[pc + 1];
    pc += 2;

    uint16_t x = (opcode & 0x0F00) >> 8;
    uint16_t y = (opcode & 0x00F0) >> 4;
    uint16_t n = opcode & 0x000F;
    uint16_t kk = opcode & 0x00FF;
    uint16_t nnn = opcode & 0x0FFF;

    /* Execute a single instruction */
    switch(opcode & 0xF000) {
        /* Miscellaneous instructions */
        case 0x0000:
            switch(opcode) {
                /* 00E0 (CLS) */
                /* Clear the screen */
                case 0x00E0:
                    display.fill(0);
                    draw_flag = true;
                    break;

                /* 00EE (RET) */
                /* Return from subroutine call */
                case 0x00EE:
                    pc = stack[--sp];
                    break;
                
                default:
                    break;
            }
            break;

        /* 1nnn (JP addr) */
        /* Jump to address */
        case 0x1000:
            pc = nnn;
            break;

        /* 2nnn (CALL addr) */
        /* Jump to subroutine */
        case 0x2000:
            stack[sp++] = pc;
            pc = nnn;
            break;

        /* 3xkk (SE Vx, byte) */
        /* Skip next instructions if register x equals constant */
        case 0x3000:
            if (registers[x] == kk) {
                pc += 2;
            }
            break;
        
        /* 4xkk (SNE Vx, byte) */
        /* Skip next instruction if register x does not equal constant */
        case 0x4000:
            if (registers[x] != kk) {
                pc += 2;
            }
            break;
        
        /* 5xy0 (SE Vx, Vy) */
        /* Skip next instruction if register x equals register y */
        case 0x5000:
            if (registers[x] == registers[y]) {
                pc += 2;
            }
            break;
        
        /* 6xkk (LD Vx, byte) */
        /* Move constant to register x */
        case 0x6000:
            registers[x] = kk;
            break;
        
        /* 7xkk (ADD Vx, byte) */
        /* Add constant to register x */
        case 0x7000:
            registers[x] += kk;
            break;
        
        /* Logical instructions */
        case 0x8000:
            switch (n) {
                /* 8xy0 (LD Vx, Vy) */
                /* Move register Vy into Vx */
                case 0x0000:
                    registers[x] = registers[y];
                    break;
                
                /* 8xy1 (OR Vx, Vy) */
                /* OR register Vy into Vx */
                case 0x0001:
                    registers[x] |= registers[y];
                    break;
                
                /* 8xy2 (AND Vx, Vy) */
                /* AND register Vy into Vx */
                case 0x0002:
                    registers[x] &= registers[y];
                    break;
                
                /* 8xy3 (XOR Vx, Vy) */
                /* XOR register Vy into Vx */
                case 0x0003:
                    registers[x] ^= registers[y];
                    break;
                
                /* 8xy4 (ADD Vx, Vy) */
                /* ADD register y into register x */
                case 0x0004:
                    registers[0xF] = (registers[x] + registers[y]) > 0xFF;
                    registers[x] += registers[y];
                    break;
                
                /* 8xy5 (OR Vx, Vy) */
                /* OR register y into register x */
                case 0x0005:
                    registers[0xF] = (registers[y] > registers[x]);
                    registers[x] -= registers[y];
                    break;
                
                /* 8xy6 (SHR Vx, {, Vy}) */
                /* Shift register x right, bit 0 goes into register 0xF */
                case 0x0006:
                    registers[0xF] = registers[x] & 1;
                    registers[x] >>= 1;
                    break;
                
                /* 8xy7 (SUBN Vx, Vy) */
                /* subtract register x from register y, result in register y */
                case 0x0007:
                    registers[0xF] = (registers[x] > registers[y]);
                    registers[x] = registers[y] - registers[x];
                    break;
                
                /* 8xyE (SHL Vx, {, Vy})*/
                /* shift register x right, bit 7 goes in register 0xF */
                case 0x000E:
                    registers[0xF] = registers[x] >> 7;
                    registers[x] <<= 1;
                    break;
                
                default:
                    break;
            }
            break;
        
        /* 9xy0 (SNE Vx, Vy) */
        /* Skip if register x does not equal register y */
        case 0x9000:
            if (registers[x] != registers[y]) {
                pc += 2;
            }
            break;
        
        /* Annn (LD I, addr) */
        /* Load index register with constant nnn */
        case 0xA000:
            index = nnn;
            break;
        
        /* Bnnn (JP V0, addr) */
        /* Jump to address nnn plus register 0 */
        case 0xB000:
            pc = (nnn + registers[0]);
            break;
        
        /* Cxkk (RND Vx, byte) */
        /* and random byte with a constant, result in register x */
        case 0xC000:
            registers[x] = std::uniform_int_distribution<>(0x00, 0xFF)(mt) & kk;
            break;
    
        /* Dxyn (DRW, Vx, Vy, nibble) */
        /* Draw a sprite at location (register x, register y) with height n */
        case 0xD000:
            registers[0xF] = 0;
            for (auto i = 0; i < n; i++) {
                uint8_t pixel = memory[index + i];
                for (auto j  = 0; j < 8; j++) {
                    if (pixel & (0x80 >> j)) {
                        uint16_t pos = registers[x] + j + ((registers[y] + i) * 64);
                        registers[0xF] = display[pos];
                        display[pos] ^= 1;
                    }
                }
            }
            draw_flag = true;
            break;
        
        /* Keypad instructions */
        case 0xE000: {
            switch (kk) {
                /* Ex9E (SKP Vx) */
                /* Skip if key in register x is pressed */
                case 0x009E:
                    if (keys[registers[x]]) {
                        pc += 2;
                    }
                    break;
                
                /* ExA1 (SKNP Vx) */
                /* Skip if key in register x is not pressed */
                case 0x00A1:
                    if (!keys[registers[x]]) {
                        pc += 2;
                    }
                    break;
                
                default:
                    break;
            }
            break;
        }

        /* Miscellaneous instructions */
        case 0xF000:
            switch (kk) {
                /* Fx07 (LD Vx, DT) */
                /* Get delay timer into register x */
                case 0x0007:
                    registers[x] = delay_timer;
                    break;
                
                /* Fx0a (LD Vx, K) */
                /* Wait for keypress, put key in register x */
                case 0x000A: {
                    bool waiting = true;
                    for (auto i = 0; i < keys.size(); i++) {
                        if (keys[i]) {
                            registers[x] = keys[i];
                            waiting = false;
                            break;
                        }
                    }
                    if (waiting) pc -= 2;
                    break;
                }

                /* Fx15 (LD DT, Vx) */
                /* Set the delay timer to register x */
                case 0x0015:
                    delay_timer = registers[x];
                    break;
                
                /* Fx18 (LD ST, Vx) */
                /* Set the sound timer to register x */
                case 0x0018:
                    sound_timer = registers[x];
                    break;
                
                /* Fx1E (ADD I, Vx) */
                /* Add register x to the index register */
                case 0x001E:
                    index += registers[x];
                    break;
                
                /* Fx29 (LD F, Vx) */
                /* Point the index register to the sprite for hex character in register x */
                case 0x0029:
                    index = registers[x] * 5;
                    break;
                
                /* Fx33 (LD B, Vx) */
                /* Store the bcd representation of register x starting from the address in the index register */
                case 0x0033:
                    memory[index] = registers[x] / 100;
                    memory[index + 1] = (registers[x] / 10) % 10;
                    memory[index + 2] = registers[x] % 10;
                    break;
                
                /* Fx55 (LD [I], Vx) */
                /* Store registers 0 through x starting from the address in the index register */
                case 0x0055:
                    for (auto i = 0; i <= x; i++) {
                        memory[index + i] = registers[i];
                    }
                    break;
                
                /* Fx65 (LD Vx, [I]) */
                /* load registers 0 through x starting from the address in the index register */
                case 0x0065:
                    for (auto i = 0; i <= x; i++) {
                        registers[i] = memory[index + i];
                    }
                    break;
                
                default:
                    break;
            }
            break;

        default:
            break;
    }
}

void Chip8::decrement_timers(void) {
    if (delay_timer > 0) delay_timer--;
    if (sound_timer > 0) sound_timer--;
}

uint8_t Chip8::pixel_state(const size_t& pixel) {
    return display[pixel];
}

void Chip8::handle_keypress(const size_t& key) {
    keys[key] = !keys[key];
}

bool Chip8::get_draw_flag(void) {
    return draw_flag;
}

void Chip8::set_draw_flag(const bool& state) {
    draw_flag = state;
}