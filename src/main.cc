#include <iostream>
#include <SDL.h>
#include "chip8.h"

constexpr int WIDTH = 64;                           /* Display width */
constexpr int HEIGHT = 32;                          /* Display height */
constexpr int SCALE = 10;                           /* Factor that width and height are multiplied by */
constexpr int PRIMARY_COLOR = 0x00000000;           /* Hex code for primary screen color */
constexpr int SECONDARY_COLOR = 0x0000AA00;         /* Hex code for secondary screen color */
constexpr int NUM_INSTRUCTIONS = 10;                /* Number of instructions executed per cycle */
constexpr int NUM_TICKS = 500 / 60;                 /* Number of ticks per cycle (500 Hz / 60 Hz) */
constexpr std::array<SDL_Keycode, 16> keymap = {    /* Mapping of SDL keycodes to the keypad (0 - F) */
    SDLK_x, SDLK_1, SDLK_2, SDLK_3,
    SDLK_q, SDLK_w, SDLK_e, SDLK_a,  
    SDLK_s, SDLK_d, SDLK_z, SDLK_c,
    SDLK_4, SDLK_r, SDLK_f, SDLK_v 
};

bool init_screen(SDL_Window*& window, SDL_Renderer*& renderer, SDL_Texture*& texture) {
    bool success = true;
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        success = false;
    }
    window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * SCALE, HEIGHT * SCALE, SDL_WINDOW_SHOWN);
    if (!window) {
        success = false;
    }    
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        success = false;
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
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
            chip8.set_draw_flag(false);
            std::array<uint32_t, WIDTH * HEIGHT> pixels;
            for (auto i = 0; i < pixels.size(); i++) {
                pixels[i] = (chip8.pixel_state(i) ? SECONDARY_COLOR : PRIMARY_COLOR);
            }
            SDL_UpdateTexture(texture, nullptr, reinterpret_cast<void *>(pixels.data()), WIDTH * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
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