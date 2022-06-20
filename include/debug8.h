#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <vector>

#include "sdl2_chip8.h"

class debug8 : public sdl2_chip8 {
 private:
  void show_help();
  void dump_stack();
  void dump_registers();
  void dump_memory();
  void dump_breakpoints();
  void dump_disasm();
  void disassemble(uint16_t pc, char *address, char *op, char *disasm);

  bool save_struct();
  bool load_struct();

  void text(int r, char *tmp, SDL_Color c, int &x, int &y);

  uint8_t shadow_sp;
  uint8_t shadow_reg[16];
  uint16_t shadow_index;

  TTF_Font *font;
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

  uint8_t *backup_rom;

  enum { MAIN, REGISTERS, STACK, MEMORY, DISASM, BRK_LIST, WND_SIZE };

  struct {
    const char *title;
    uint16_t w;
    uint16_t h;
  } wnd_data[WND_SIZE] = {{"DEBUG8", 640, 320},      {"REGISTERS", 875, 63},
                          {"STACK", 100, 336},       {"MEMORY", 500, 210},
                          {"DISASSEMBLY", 325, 320}, {"BREAKPOINTS", 100, 336}};

  const char *exception_text[4] = {"NO_EXCEPTION", "EXCEPTION_UNKNOWN_OPCODE",
                                   "EXCEPTION_STACK_UNDERFLOW",
                                   "EXCEPTION_STACK_OVERFLOW"};

  struct {
    SDL_Window *w[WND_SIZE];
    SDL_Renderer *r[WND_SIZE];
    int x[WND_SIZE];
    int y[WND_SIZE];
  } wnd;

  void set_RAM(uint16_t ptr, uint8_t val);
  uint8_t get_RAM(uint16_t ptr);
  uint16_t last_break = -1;

  void init();

 public:
  void init_screen();
  void end_screen();
  bool handle_input();
  bool loop(bool debug, bool single_step);
  bool debug = true;
  bool single_step = false;
};
