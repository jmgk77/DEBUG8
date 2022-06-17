#include <curses.h>

#include "chip8.h"

class ncurses_chip8 : public chip8
{
private:
  uint8_t ncurses_keys[16] = {'x', '1', '2', '3', 'q', 'w', 'e', 'a',
                              's', 'd', 'z', 'c', '4', 'r', 'f', 'v'};

  WINDOW *wchip8;

public:
  bool loop();
  void show_display();
  void init_screen();
  void end_screen();
  bool handle_input();
  uint32_t get_ticks();
  void delay(uint16_t x);
};
