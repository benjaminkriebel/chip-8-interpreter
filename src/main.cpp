#include <iostream>
#include <SDL.h>
#include "chip8.hpp"

const int WINDOW_WIDTH     = 64;        /* display width */
const int WINDOW_HEIGHT    = 32;        /* display height */
const int WINDOW_SCALE     = 10;        /* factor that width and height are multiplied by */
const int NUM_INSTRUCTIONS = 10;        /* number of instructions executed per cycle */
const int NUM_TICKS        = 500 / 60;  /* number of ticks per cycle (500 Hz / 60 Hz) */

const std::array<SDL_Keycode, 16> keymap = { /* ,apping of SDL keycodes to the keypad (0 - F) */
    SDLK_x, SDLK_1, SDLK_2, SDLK_3,
    SDLK_q, SDLK_w, SDLK_e, SDLK_a,  
    SDLK_s, SDLK_d, SDLK_z, SDLK_c,
    SDLK_4, SDLK_r, SDLK_f, SDLK_v 
};

bool init_screen(SDL_Window*& window, SDL_Renderer*& renderer, SDL_Texture*& texture) {
    bool success = true;
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        success = false;
    }
    
    window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH * WINDOW_SCALE, WINDOW_HEIGHT * WINDOW_SCALE, SDL_WINDOW_SHOWN);
    if (!window) {
        success = false;
    }
    
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        success = false;
    }
    
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!texture) {
        success = false;
    }

    return success;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: ./chip8.exe <PATH_TO_ROM>.\n";
        return 1;
    }

    std::string filepath = std::string(argv[1]);
    Chip8 chip8;
    chip8.load_rom(filepath);

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    if (!init_screen(window, renderer, texture)) {
        std::cout << "Error: Unable to create screen " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    bool running = true;
    while (running) {
        int start_time = SDL_GetTicks();
        for (size_t i = 0; i < NUM_INSTRUCTIONS; i++) {
            chip8.execute_instruction();
        }
        
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch(ev.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_KEYUP:
                case SDL_KEYDOWN:
                    for (auto i = 0; i < keymap.size(); i++) {
                        if (ev.key.keysym.sym == keymap[i]) {
                            chip8.handle_keypress(i);
                        }
                    }
                    break;
                
                default:
                    break;
            }
        }

        if (chip8.get_draw_flag()) {
            std::array<uint32_t, WINDOW_WIDTH * WINDOW_HEIGHT> pixel_buf;
            for (auto i = 0; i < pixel_buf.size(); i++) {
                pixel_buf[i] = (chip8.get_pixel_state(i) ? 0xFFFFFFFF : 0x00000000);
            }
            SDL_UpdateTexture(texture, nullptr, reinterpret_cast<void *>(pixel_buf.data()), WINDOW_WIDTH * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);

            chip8.set_draw_flag(false);
        }

        int delta_time = SDL_GetTicks() - start_time;
        if (NUM_TICKS > delta_time) {
            chip8.decrement_timers();
            SDL_Delay(NUM_TICKS - delta_time);
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}