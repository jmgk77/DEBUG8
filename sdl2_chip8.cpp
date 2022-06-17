#include "sdl2_chip8.h"

#include <SDL2/SDL.h>

void sdl2_chip8::check_keypress(SDL_Event *event)
{
  for (uint16_t i = 0; i < sizeof(sdl_keys); i++)
  {
    if (event->key.keysym.sym == sdl_keys[i])
    {
      key_press(i);
    }
  }
}

void sdl2_chip8::check_keyrelease(SDL_Event *event)
{
  for (uint16_t i = 0; i < sizeof(sdl_keys); i++)
  {
    if (event->key.keysym.sym == sdl_keys[i])
    {
      key_release(i);
    }
  }
}

bool sdl2_chip8::loop() { return chip8::loop(); }

void sdl2_chip8::show_display()
{
  // show display
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);
  SDL_Rect dstrect;
  dstrect.h = 9;
  dstrect.w = 9;
  for (int yy = 0; yy < 32; yy++)
  {
    for (int xx = 0; xx < 64; xx++)
    {
      if (display[xx + (yy * 64)])
      {
        dstrect.x = xx * 10;
        dstrect.y = yy * 10;
        SDL_SetRenderDrawColor(renderer, 255, 128, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &dstrect);
      }
    }
  }

  SDL_RenderPresent(renderer);
}

void sdl2_chip8::init_screen()
{
  // init SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    printf("ERROR: %s\n", SDL_GetError());
    exit(-1);
  }

  if (SDL_CreateWindowAndRenderer(640, 320, 0, &window, &renderer))
  {
    printf("ERROR: %s\n", SDL_GetError());
    SDL_Quit();
    exit(-1);
  }
}

void sdl2_chip8::end_screen()
{
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

bool sdl2_chip8::handle_input()
{
  bool quit = false;
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
    case SDL_QUIT:
      quit = true;
      break;
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_ESCAPE)
      {
        quit = true;
      }
      // chip8 keyboard handler
      check_keypress(&event);
      break;
    case SDL_KEYUP:
      // chip8 keyboard handler
      check_keyrelease(&event);
      break;
    default:
      break;
    }
  }
  return quit;
}

uint32_t sdl2_chip8::get_ticks() { return SDL_GetTicks(); }

void sdl2_chip8::delay(uint16_t x) { SDL_Delay(x); }
