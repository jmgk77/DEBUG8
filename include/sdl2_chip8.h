#include <SDL2/SDL.h>

#include "chip8.h"

class sdl2_chip8 : public chip8 {
private:
  uint8_t sdl_keys[16] = {SDLK_x, SDLK_1, SDLK_2, SDLK_3, SDLK_q, SDLK_w,
                          SDLK_e, SDLK_a, SDLK_s, SDLK_d, SDLK_z, SDLK_c,
                          SDLK_4, SDLK_r, SDLK_f, SDLK_v};

protected:
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Event event;
  void check_keypress(SDL_Event *event);
  void check_keyrelease(SDL_Event *event);

public:
  bool loop();
  void show_display();
  void init_screen();
  void end_screen();
  bool handle_input();
  uint32_t get_ticks();
  void delay(uint16_t x);
};
