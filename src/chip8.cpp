#include <fstream>
#include <vector>
#include "chip8.hpp"

const size_t PROGRAM_START = 512;   /* address of first instruction in memory */

Chip8::Chip8() :
    memory(),
    pixels(),
    registers(),
    keys(),
    stack(),
    pc(PROGRAM_START),
    index(0),
    delay_timer(0),
    sound_timer(0),
    draw_flag(false),
    gen(std::random_device()())
{
    std::copy(fontset.begin(), fontset.end(), memory.begin());
}

void Chip8::load_rom(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);

    file.seekg(0, std::ios::end);
    std::ifstream::pos_type filesize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(filesize);
    file.read(reinterpret_cast<char *>(buffer.data()), filesize);
    std::copy(buffer.begin(), buffer.end(), memory.begin() + PROGRAM_START);
}

void Chip8::execute_instruction(void) {
    uint16_t opcode = memory[pc] << 8 | memory[pc + 1];
    pc += 2;

    uint16_t x = (opcode & 0x0F00) >> 8;
    uint16_t y = (opcode & 0x00F0) >> 4;
    uint16_t n = opcode & 0x000F;
    uint16_t kk = opcode & 0x00FF;
    uint16_t nnn = opcode & 0x0FFF;

    switch(opcode & 0xF000) {
        case 0x0000:
            switch(opcode) {
                /* 00E0 (CLS) */
                /* Clear the display */
                case 0x00E0:
                    pixels.fill(0);
                    draw_flag = true;
                    break;

                /* 00EE (RET) */
                /* Return from a subroutine */
                case 0x00EE:
                    pc = stack.top();
                    stack.pop();
                    break;
                
                default:
                    break;
            }
            break;

        /* 1nnn (JP addr) */
        /* Jump to location nnn */
        case 0x1000:
            pc = nnn;
            break;

        /* 2nnn (CALL addr) */
        /* Call subroutine at nnn */
        case 0x2000:
            stack.push(pc);
            pc = nnn;
            break;

        /* 3xkk (SE Vx, byte) */
        /* Skip next instruction if Vx = kk */
        case 0x3000:
            if (registers[x] == kk) {
                pc += 2;
            }
            break;
        
        /* 4xkk (SNE Vx, byte) */
        /* Skip next instruction if Vx != kk */
        case 0x4000:
            if (registers[x] != kk) {
                pc += 2;
            }
            break;
        
        /* 5xy0 (SE Vx, Vy) */
        /* Skip next instruction if Vx != Vy */
        case 0x5000:
            if (registers[x] == registers[y]) {
                pc += 2;
            }
            break;
        
        /* 6xkk (LD Vx, byte) */
        /* Set Vx = kk */
        case 0x6000:
            registers[x] = kk;
            break;
        
        /* 7xkk (ADD Vx, byte) */
        /* Set Vx = Vx + kk */
        case 0x7000:
            registers[x] += kk;
            break;
        
        case 0x8000:
            switch (n) {
                /* 8xy0 (LD Vx, Vy) */
                /* Set Vx = Vy */
                case 0x0000:
                    registers[x] = registers[y];
                    break;
                
                /* 8xy1 (OR Vx, Vy) */
                /* Set Vx = Vx OR Vy */
                case 0x0001:
                    registers[x] |= registers[y];
                    break;
                
                /* 8xy2 (AND Vx, Vy) */
                /* Set Vx = Vx AND Vy */
                case 0x0002:
                    registers[x] &= registers[y];
                    break;
                
                /* 8xy3 (XOR Vx, Vy) */
                /* Set Vx = Vx XOR Vy */
                case 0x0003:
                    registers[x] ^= registers[y];
                    break;
                
                /* 8xy4 (ADD Vx, Vy) */
                /* Set Vx = Vx + Vy, set VF = carry */
                case 0x0004:
                    registers[x] += registers[y];
                    registers[0xF] = (registers[x] + registers[y]) > 0xFF;
                    break;
                
                /* 8xy5 (SUB Vx, Vy) */
                /* Set Vx = Vx - Vy, set VF = NOT borrow*/
                case 0x0005:
                    registers[x] -= registers[y];
                    registers[0xF] = (registers[x] > registers[y]);
                    break;
                
                /* 8xy6 (SHR Vx, {, Vy}) */
                /* Set Vx = Vx SHR 1, store LSB of Vx in VF */
                case 0x0006:
                    registers[x] >>= 1;
                    registers[0xF] = registers[x] & 1;
                    break;
                
                /* 8xy7 (SUBN Vx, Vy) */
                /* Set Vx = Vy - Vx, set VF = NOT borrow */
                case 0x0007:
                    registers[x] = registers[y] - registers[x];
                    registers[0xF] = (registers[y] > registers[x]);
                    break;
                
                /* 8xyE (SHL Vx, {, Vy}) */
                /* Set Vx = Vx SHL 1 */
                case 0x000E:
                    registers[x] <<= 1;
                    registers[0xF] = registers[x] >> 7;
                    break;
                
                default:
                    break;
            }
            break;
        
        /* 9xy0 (SNE Vx, Vy) */
        /* Skip next instruction if Vx != Vy */
        case 0x9000:
            if (registers[x] != registers[y]) {
                pc += 2;
            }
            break;
        
        /* Annn (LD I, addr) */
        /* Set I = nnn */
        case 0xA000:
            index = nnn;
            break;
        
        /* Bnnn (JP V0, addr) */
        /* Jump to location nnn + V0 */
        case 0xB000:
            pc = nnn + registers[0];
            break;
        
        /* Cxkk (RND Vx, byte) */
        /* Set Vx = random byte AND kk */
        case 0xC000:
            registers[x] = std::uniform_int_distribution<>(0x00, 0xFF)(gen) & kk;
            break;
    
        /* Dxyn (DRW, Vx, Vy, nibble) */
        /* Display n-byte sprite starting at memory location I at *Vx, Vy), set VF = collision */
        case 0xD000: {
            uint16_t x_pos = registers[x];
            uint16_t y_pos = registers[y];

            registers[0xF] = 0;
            for (auto i = 0; i < n; i++) {
                uint8_t pixel = memory[index + i];
                for (auto j  = 0; j < 8; j++) {
                    if (pixel & (0x80 >> j)) {
                        uint16_t coord = x_pos + j + ((y_pos + i) * 64) % 2048;
                        if (pixels[coord] == 1) {
                            registers[0xF] = 1;
                        }
                        pixels[coord] ^= 1;
                    }
                }
            }

            draw_flag = true;
            break;
        }
        
        case 0xE000: {
            switch (kk) {
                /* Ex9E (SKP Vx) */
                /* Skip next instruction if key with the value of Vx is pressed */
                case 0x009E:
                    if (keys[registers[x]]) {
                        pc += 2;
                    }
                    break;
                
                /* ExA1 (SKNP Vx) */
                /* Skip next instruction if key with the value of Vx is not pressed */
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

        case 0xF000:
            switch (kk) {
                /* Fx07 (LD Vx, DT) */
                /* Set Vx = delay timer value */
                case 0x0007:
                    registers[x] = delay_timer;
                    break;
                
                /* Fx0A (LD Vx, K) */
                /* Wait for a key press, store the value of the key in Vx */
                case 0x000A: {
                    bool waiting = true;
                    for (auto i = 0; i < keys.size(); i++) {
                        if (keys[i]) {
                            registers[x] = keys[i];
                            waiting = false;
                            break;
                        }
                    }

                    if (waiting) {
                        pc -= 2;
                    }
                    
                    break;
                }

                /* Fx15 (LD DT, Vx) */
                /* Set delay timer = Vx */
                case 0x0015:
                    delay_timer = registers[x];
                    break;
                
                /* Fx18 (LD ST, Vx) */
                /* Set sound timer = Vx */
                case 0x0018:
                    sound_timer = registers[x];
                    break;
                
                /* Fx1E (ADD I, Vx) */
                /* Set I = I + Vx */
                case 0x001E:
                    index += registers[x];
                    break;
                
                /* Fx29 (LD F, Vx) */
                /* Set I = location of sprite for digit Vx */
                case 0x0029:
                    index = registers[x] * 5;
                    break;
                
                /* Fx33 (LD B, Vx) */
                /* Store BCD representation of Vx in memory locations I, I + 1, I + 2 */
                case 0x0033:
                    memory[index] = registers[x] / 100;
                    memory[index + 1] = (registers[x] / 10) % 10;
                    memory[index + 2] = registers[x] % 10;
                    break;
                
                /* Fx55 (LD [I], Vx) */
                /* Store registers V0 through Vx in memory starting at location I */
                case 0x0055:
                    for (auto i = 0; i <= x; i++) {
                        memory[index + i] = registers[i];
                    }
                    break;
                
                /* Fx65 (LD Vx, [I]) */
                /* Read registers V0 through Vx from memory starting at location I */
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
    if (delay_timer > 0) {
        delay_timer--;
    }
    if (sound_timer > 0) {
        sound_timer--;
    }
}

uint8_t Chip8::get_pixel_state(const size_t& i) {
    return pixels[i];
}

void Chip8::handle_keypress(const size_t& i) {
    keys[i] = !keys[i];
}

bool Chip8::get_draw_flag(void) {
    return draw_flag;
}

void Chip8::set_draw_flag(const bool& value) {
    draw_flag = value;
}