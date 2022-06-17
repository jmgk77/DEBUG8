#include <stdio.h>
#include <stdlib.h>

#define FRAMERATE 120

#if !defined(CONSOLE) and !defined(SDL1) and !defined(SDL2) and !defined(DEBUG)
#define DEBUG
#endif

#if defined(CONSOLE)
#include "ncurses_chip8.h"
#elif defined(SDL1)
#include "sdl1_chip8.h"
#elif defined(SDL2)
#include "sdl2_chip8.h"
#elif defined(DEBUG)
#include "debug8.h"
#endif

int main(int argc, char *argv[])
{
#if defined(CONSOLE)
  ncurses_chip8 c8;
#elif defined(SDL1)
  sdl1_chip8 c8;
#elif defined(SDL2)
  sdl2_chip8 c8;
#elif defined(DEBUG)
  debug8 c8;
#endif

  // handle cmdline
  char *rom = argv[1];
  if (!rom)
  {
    printf("USAGE: %s <rom>\n", (char *)argv[0]);
    // exit(-1);
    rom = (char *)"TEST/c8_test.c8";
  }

  // load ROM
  printf("LOADING ROM (%s)\n", rom);
  if (!c8.load(rom))
  {
    printf("ERROR: cant read ROM\n");
    exit(-1);
  }

  c8.init_screen();

  // fps cap
  unsigned int framerate, framerate_old;
  framerate_old = framerate = c8.get_ticks();

  // sdl loop
  bool quit = false;
  while (!quit)
  {
    //
    framerate = c8.get_ticks();

    // keep FPS
    if ((framerate - framerate_old) > 1000 / FRAMERATE)
    {
      framerate_old = framerate;
      quit = c8.handle_input();
#ifdef DEBUG
      if (c8.loop(c8.debug, c8.single_step))
#else
      if (c8.loop())
#endif
      {
        c8.show_display();
      }
    }
    else
    {
      // Wait remaining time
      c8.delay((1000 / FRAMERATE) - (framerate - framerate_old));
    }
  }

  c8.end_screen();

  exit(0);
}
