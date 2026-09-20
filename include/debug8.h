#pragma once

#include <SDL2/SDL.h>

#include <vector>

#include "sdl2_chip8.h"

class debug8 : public sdl2_chip8 {
private:
  void show_help();
  void draw_debugger();
  void draw_toolbar();
  void draw_registers();
  void draw_stack();
  void draw_memory();
  void draw_breakpoints();
  void draw_disasm();
  void disassemble(uint16_t pc, char *address, char *op, char *disasm);

  bool save_struct();
  bool load_struct();

  uint8_t shadow_sp;
  uint8_t shadow_reg[16];
  uint16_t shadow_index;

  uint16_t mem_ptr = 0;

  enum { BREAKPOINT_MEMORY, BREAKPOINT_CODE };
  uint16_t brk_ptr = 0;

  struct _brk {
    uint16_t ptr;
    uint8_t type;
  };
  std::vector<_brk> breakpoints;

  void toogle_brk(uint16_t ptr, uint8_t type);
  bool check_brk(uint16_t ptr, uint8_t type);
  void set_brk(uint16_t ptr, uint8_t type);
  void remove_brk(uint16_t ptr, uint8_t type);

  uint8_t *backup_rom = nullptr;

  enum { MAIN, DEBUGGER, WND_SIZE };

  SDL_Window *dbg_window = nullptr;
  SDL_Renderer *dbg_renderer = nullptr;

  static constexpr uint32_t CONFIG_MAGIC = 0x44423832; // "DB82"
  struct {
    uint32_t magic;
    int x[WND_SIZE];
    int y[WND_SIZE];
  } wnd_pos;

  const char *exception_text[4] = {"NO_EXCEPTION", "EXCEPTION_UNKNOWN_OPCODE",
                                   "EXCEPTION_STACK_UNDERFLOW",
                                   "EXCEPTION_STACK_OVERFLOW"};

  void set_RAM(uint16_t ptr, uint8_t val) override;
  uint8_t get_RAM(uint16_t ptr) override;
  uint16_t last_break = -1;

  void init();

public:
  void init_screen() override;
  void end_screen() override;
  bool handle_input() override;
  bool loop(bool debug, bool single_step);
  void show_debugger();
  bool debug = true;
  bool single_step = false;
};
