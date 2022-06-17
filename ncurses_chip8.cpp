#include "ncurses_chip8.h"

#include <ctype.h>
#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void ncurses_chip8::init_screen()
{
  // init ncurses
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  scrollok(stdscr, FALSE);
  nodelay(stdscr, TRUE);
  curs_set(0);
  wchip8 = newwin(34, 66, 0, 0);
  clear();
}

void ncurses_chip8::end_screen()
{
  curs_set(1);
  endwin();
}

bool ncurses_chip8::loop() { return chip8::loop(); }

void ncurses_chip8::show_display()
{
  wclear(wchip8);
  box(wchip8, 0, 0);
  // show display
  for (int yy = 0; yy < 32; yy++)
  {
    for (int xx = 0; xx < 64; xx++)
    {
      if (display[xx + (yy * 64)])
      {
        mvwaddch(wchip8, yy + 1, xx + 1, '#');
      }
    }
  }
  wrefresh(wchip8);
}

bool ncurses_chip8::handle_input()
{
  int ch = getch();
  mvwaddch(wchip8, 0, 0, ch);
  for (uint16_t i = 0; i < sizeof(ncurses_keys); i++)
  {
    key_release(i);
    if ((ch == ncurses_keys[i]) || (ch == toupper(ncurses_keys[i])))
    {
      key_press(i);
    }
  }

  return (ch == 27 /* KEY_ESCAPE */);
}

uint32_t ncurses_chip8::get_ticks()
{
  struct timespec ts;
  uint32_t tick = 0;
  clock_gettime(CLOCK_REALTIME, &ts);
  tick = ts.tv_nsec / 1000000;
  tick += ts.tv_sec * 1000;
  return tick;
}

void ncurses_chip8::delay(uint16_t x) { usleep(x); }
