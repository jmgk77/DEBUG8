#include "debug8.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstring>

void debug8::show_help()
{
  SDL_ShowSimpleMessageBox(
      SDL_MESSAGEBOX_INFORMATION, "HELP",
      "<F1>           Show this help screen\n"
      "<F2>           Dump memory -0x10\n"
      "<F3>           Dump memory +0x10\n"
      "<SHIFT><F2>    Dump memory -0x100\n"
      "<SHIFT><F3>    Dump memory +0x100\n"
      "<F4>           Dump memory at 0x0000\n"
      "<F5>           Run/stop\n"
      "<F6>           Breakpoint position -0x1\n"
      "<F7>           Breakpoint position +0x1\n"
      "<CTRL><F6>     Breakpoint position -0x10\n"
      "<CTRL><F7>     Breakpoint position +0x10\n"
      "<SHIFT><F6>    Breakpoint position -0x100\n"
      "<SHIFT><F7>    Breakpoint position +0x100\n"
      "<F8>           Toogle code breakpoint at position\n"
      "<SHIFT><F8>    Toogle memory breakpoint at position\n"
      "<F9>           Single step\n"
      "<F11>          Clear all breakpoints\n"
      "<F12>          Reload\n",
      0);
}

void debug8::toogle_brk(uint16_t ptr, uint8_t type)
{
  if (check_brk(ptr, type))
  {
    remove_brk(ptr, type);
  }
  else
  {
    set_brk(ptr, type);
  }
}

bool debug8::check_brk(uint16_t ptr, uint8_t type)
{
  for (auto i : breakpoints)
  {
    if ((i.ptr == ptr) && (i.type == type))
      return true;
  }
  return false;
}

void debug8::set_brk(uint16_t ptr, uint8_t type)
{
  breakpoints.push_back({ptr, type});
}

void debug8::remove_brk(uint16_t ptr, uint8_t type)
{
  uint16_t j = 0;
  for (auto i : breakpoints)
  {
    if ((i.ptr == ptr) && (i.type == type))
    {
      breakpoints.erase(breakpoints.begin() + j);
    }
    j++;
  }
}

bool debug8::save_struct()
{
  FILE *f;
  f = fopen(".config.bin", "wb");
  if (!f)
    return false;
  fwrite(&wnd, 1, sizeof(wnd), f);
  fclose(f);
  return true;
}

bool debug8::load_struct()
{
  FILE *f;
  f = fopen(".config.bin", "rb");
  if (!f)
    return false;
  memset(&wnd, 0, sizeof(wnd));
  fread(&wnd, 1, sizeof(wnd), f);
  fclose(f);
  return true;
}

void debug8::set_RAM(uint16_t ptr, uint8_t val)
{
  if (check_brk(ptr, BREAKPOINT_MEMORY))
  {
    debug = true;
    single_step = false;
  }
  chip8::set_RAM(ptr, val);
}

uint8_t debug8::get_RAM(uint16_t ptr)
{
  if (check_brk(ptr, BREAKPOINT_MEMORY))
  {
    debug = true;
    single_step = false;
  }
  return chip8::get_RAM(ptr);
}

void debug8::dump_breakpoints()
{
  char tmp[32];
  int x = 0;
  int y = 0;
  SDL_RenderClear(wnd.r[BRK_LIST]);

  snprintf(tmp, sizeof(tmp), "> 0x%04X <\n", brk_ptr);
  text(BRK_LIST, tmp, {255, 255, 255, SDL_ALPHA_OPAQUE}, x, y);

  if (!breakpoints.empty())
  {
    for (auto i : breakpoints)
    {
      snprintf(tmp, sizeof(tmp), "  0x%04X\n", i.ptr);
      x = 0;
      // default to code breakpoint
      SDL_Color c = {255, 0, 0, SDL_ALPHA_OPAQUE};
      if (i.type == BREAKPOINT_MEMORY)
      {
        c = {0, 0, 255, SDL_ALPHA_OPAQUE};
      }
      text(BRK_LIST, tmp, c, x, y);
    }
  }
  //
  SDL_RenderPresent(wnd.r[BRK_LIST]);
}

void debug8::dump_stack()
{
  char tmp[32];
  int y = 0;
  SDL_RenderClear(wnd.r[STACK]);

  for (int i = 0; i < 16; i++)
  {
    snprintf(tmp, sizeof(tmp), "  0x%04X  \n", stack[i]);
    SDL_Color c = {128, 128, 128, SDL_ALPHA_OPAQUE};
    if (sp == i)
    {
      c = {255, 255, 255, SDL_ALPHA_OPAQUE};
    }
    int x = 0;
    text(STACK, tmp, c, x, y);
  }

  //
  SDL_RenderPresent(wnd.r[STACK]);
}

void debug8::dump_registers()
{
  char tmp[64];
  SDL_Color c;
  SDL_RenderClear(wnd.r[REGISTERS]);

  int y;
  // Vx (0..7)
  int x = 0;
  for (int i = 0; i < 8; i++)
  {
    snprintf(tmp, sizeof(tmp), "V%02d=0x%04X ", i, reg[i]);
    if (reg[i] != shadow_reg[i])
    {
      c = {255, 255, 255, SDL_ALPHA_OPAQUE};
    }
    else
    {
      c = {128, 128, 128, SDL_ALPHA_OPAQUE};
    };
    y = 0;
    text(REGISTERS, tmp, c, x, y);
  }
  // Vx (8..15)
  x = 0;
  for (int i = 8; i < 16; i++)
  {
    snprintf(tmp, sizeof(tmp), "V%02d=0x%04X ", i, reg[i]);
    c = {128, 128, 128, SDL_ALPHA_OPAQUE};
    if (reg[i] != shadow_reg[i])
    {
      c = {255, 255, 255, SDL_ALPHA_OPAQUE};
    }
    else
    {
      c = {128, 128, 128, SDL_ALPHA_OPAQUE};
    };
    y = TTF_FontHeight(font);
    text(REGISTERS, tmp, c, x, y);
  }
  // DT ST PC
  x = 0;
  snprintf(
      tmp, sizeof(tmp),
      "DT=%04X            ST=%04X             PC=%04X", delay_timer, sound_timer, pc);
  y = TTF_FontHeight(font) * 2;
  c = {128, 128, 128, SDL_ALPHA_OPAQUE};
  text(REGISTERS, tmp, c, x, y);
  // SP
  snprintf(tmp, sizeof(tmp), "            SP=%04X            ", sp);
  y = TTF_FontHeight(font) * 2;
  if (sp != shadow_sp)
  {
    c = {255, 255, 255, SDL_ALPHA_OPAQUE};
  }
  else
  {
    c = {128, 128, 128, SDL_ALPHA_OPAQUE};
  };
  text(REGISTERS, tmp, c, x, y);
  // INDEX
  snprintf(tmp, sizeof(tmp), "INDEX=%04X", index);
  y = TTF_FontHeight(font) * 2;
  if (index != shadow_index)
  {
    c = {255, 255, 255, SDL_ALPHA_OPAQUE};
  }
  else
  {
    c = {128, 128, 128, SDL_ALPHA_OPAQUE};
  };
  text(REGISTERS, tmp, c, x, y);

  //
  SDL_RenderPresent(wnd.r[REGISTERS]);
}

void debug8::dump_memory()
{
  char tmp[16];
  SDL_RenderClear(wnd.r[MEMORY]);
  //
  uint16_t p = mem_ptr;
  int y = 0;
  for (int lin = 0; lin < 10; lin++)
  {
    int x = 0;
    snprintf(tmp, sizeof(tmp), "0x%04X  ", p);
    int yy = y;
    SDL_Color c = {128, 128, 128, SDL_ALPHA_OPAQUE};
    text(MEMORY, tmp, c, x, yy);

    for (int col = 1; col <= 16; col++)
    {
      //
      snprintf(tmp, sizeof(tmp), "%02X", memory[p]);
      if (check_brk(p, BREAKPOINT_CODE))
      {
        // code breakpoint, red
        c = {255, 0, 0, SDL_ALPHA_OPAQUE};
      }
      else if (check_brk(p, BREAKPOINT_MEMORY))
      {
        // memory breakpoint, blue
        c = {0, 0, 255, SDL_ALPHA_OPAQUE};
      }
      else
      {
        // normal memory, grey
        c = {128, 128, 128, SDL_ALPHA_OPAQUE};
      };
      //
      int yy = y;
      text(MEMORY, tmp, c, x, yy);
      x += 4;
      x += (col % 4 == 0) ? 8 : 0;
      x += (col % 8 == 0) ? 8 : 0;
      p++;
    }

    y += TTF_FontHeight(font);
    p &= 0xfff;
  }
  //
  SDL_RenderPresent(wnd.r[MEMORY]);
}

void debug8::dump_disasm()
{
  char address[16];
  char op[16];
  char disasm[32];
  char tmp[1024];
  int y = 0;
  SDL_Color c;
  SDL_RenderClear(wnd.r[DISASM]);

  for (int i = pc - (7 * 2); i <= pc + (7 * 2); i += 2)
  {
    disassemble(i, address, op, disasm);
    snprintf(tmp, sizeof(tmp), "%s  %s     %s\n", address, op, disasm);
    // normal code, grey
    c = {128, 128, 128, SDL_ALPHA_OPAQUE};
    if (pc == i)
    {
      // current ip
      c = {255, 255, 255, SDL_ALPHA_OPAQUE};
    }
    else if (check_brk(i, BREAKPOINT_CODE))
    {
      // code breakpoint, red
      c = {255, 0, 0, SDL_ALPHA_OPAQUE};
    }
    else if (check_brk(i, BREAKPOINT_MEMORY))
    {
      // memory breakpoint, blue
      c = {0, 0, 255, SDL_ALPHA_OPAQUE};
    }
    int x = 0;
    text(DISASM, tmp, c, x, y);
  }
  SDL_RenderPresent(wnd.r[DISASM]);
}

void debug8::text(int r, char *tmp, SDL_Color c, int &x, int &y)
{
  SDL_Surface *text_surf = TTF_RenderText_Blended(font, tmp, c);
  SDL_Texture *text_texture =
      SDL_CreateTextureFromSurface(wnd.r[r], text_surf);
  SDL_Rect d, s;
  s.x = 0;
  s.y = 0;
  d.x = x;
  d.y = y;
  x += text_surf->w;
  y += text_surf->h;
  s.w = d.w = text_surf->w;
  s.h = d.h = text_surf->h;
  SDL_RenderCopy(wnd.r[r], text_texture, &s, &d);
  SDL_FreeSurface(text_surf);
  SDL_DestroyTexture(text_texture);
}

void debug8::disassemble(uint16_t pc, char *address, char *op, char *disasm)
{
  uint16_t opcode = (memory[pc & 0xfff] << 8) + memory[(pc & 0xfff) + 1];

  // decode
  uint16_t _op = (opcode & 0xf000) >> 12;
  uint16_t _x = (opcode & 0x0f00) >> 8;
  uint16_t _y = (opcode & 0x00f0) >> 4;
  uint16_t _n = (opcode & 0x000f);
  uint16_t _kk = (opcode & 0x00ff);
  uint16_t _nnn = (opcode & 0x0fff);

  snprintf(address, 64, "%04X", pc);
  snprintf(op, 64, "%04X", opcode);

  char dis[64];

  switch (_op)
  {
  case 0x00:
    switch (_kk)
    {
    case 0xe0:
      // CLS
      snprintf(dis, 64, "CLS");
      break;
    case 0xee:
      // RET
      snprintf(dis, 64, "RET");
      break;
    default:
      snprintf(dis, 64, "UNKNOWN");
      break;
    }
    break;
  case 0x01:
    // JP addr
    snprintf(dis, 64, "JP $%04X", _nnn);
    break;
  case 0x02:
    // CALL addr
    snprintf(dis, 64, "CALL $%04X", _nnn);
    break;
  case 0x03:
    // SE Vx, byte
    snprintf(dis, 64, "SE V%d, $%02X", _x, _kk);
    break;
  case 0x04:
    // SNE Vx, byte
    snprintf(dis, 64, "SNE V%d, $%02X", _x, _kk);
    break;
  case 0x05:
    // SE Vx, Vy
    snprintf(dis, 64, "SE V%d, V%d", _x, _y);
    break;
  case 0x06:
    // LD Vx, byte
    snprintf(dis, 64, "LD V%d, $%02X", _x, _kk);
    break;
  case 0x07:
    // ADD Vx, byte
    snprintf(dis, 64, "ADD V%d, $%02X", _x, _kk);
    break;
  case 0x08:
    switch (_n)
    {
    case 0x0:
      // LD Vx, Vy
      snprintf(dis, 64, "LD V%d, V%d", _x, _y);
      break;
    case 0x1:
      // OR Vx, Vy
      snprintf(dis, 64, "OR V%d, V%d", _x, _y);
      break;
    case 0x2:
      // AND Vx, Vy
      snprintf(dis, 64, "AND V%d, V%d", _x, _y);
      break;
    case 0x3:
      // XOR Vx, Vy
      snprintf(dis, 64, "XOR V%d, V%d", _x, _y);
      break;
    case 0x4:
      // ADD Vx, Vy
      snprintf(dis, 64, "ADD V%d, V%d", _x, _y);
      break;
    case 0x5:
      // SUB Vx, Vy
      snprintf(dis, 64, "SUB V%d, V%d", _x, _y);
      break;
    case 0x6:
      // SHR Vx {, Vy}
      snprintf(dis, 64, "SHR V%d", _x);
      break;
    case 0x7:
      // SUBN Vx, Vy
      snprintf(dis, 64, "SUBN V%d, V%d", _x, _y);
      break;
    case 0xe:
      // SHL Vx {, Vy}
      snprintf(dis, 64, "SHL V%d", _x);
      break;
    default:
      snprintf(dis, 64, "UNKNOWN");
      break;
    }
    break;
  case 0x09:
    // SNE Vx, Vy
    snprintf(dis, 64, "SNE V%d, V%d", _x, _y);
    break;
  case 0x0a:
    // LD I, addr
    snprintf(dis, 64, "LD I, $%04X", _nnn);
    break;
  case 0x0b:
    // LJP V0, addr
    snprintf(dis, 64, "LJP V0, $%04X", _nnn);
    break;
  case 0x0c:
    // RND Vx, byte
    snprintf(dis, 64, "RND V%d, $%02X", _x, _kk);
    break;
  case 0x0d:
    // DRW Vx, Vy, nibble
    snprintf(dis, 64, "DRW V%d, V%d, $%02X", _x, _y, _n);
    break;
  case 0x0e:
    switch (_kk)
    {
    case 0x9E:
      // SKP Vx
      snprintf(dis, 64, "SKP V%d", _x);
      break;
    case 0xA1:
      // SKNP Vx
      snprintf(dis, 64, "SKNP V%d", _x);
      break;
    default:
      snprintf(dis, 64, "UNKNOWN");
      break;
    }
    break;
  case 0x0f:
    switch (_kk)
    {
    case 0x07:
      // LD Vx, DT
      snprintf(dis, 64, "LD V%d, DT", _x);
    case 0x0a:
      // LD Vx, K
      snprintf(dis, 64, "LD V%d, K", _x);
      break;
    case 0x15:
      // LD DT, Vx
      snprintf(dis, 64, "LD DT, V%d", _x);
      break;
    case 0x18:
      // LD ST, Vx
      snprintf(dis, 64, "LD ST, V%d", _x);
      break;
    case 0x1e:
      // ADD I, Vx
      snprintf(dis, 64, "ADD I, V%d", _x);
      break;
    case 0x29:
      // LD F, Vx
      snprintf(dis, 64, "LD F, V%d", _x);
      break;
    case 0x33:
      // LD B, Vx
      snprintf(dis, 64, "LD B, V%d", _x);
      break;
    case 0x55:
      // LD [I], Vx
      snprintf(dis, 64, "LD [I], V%d", _x);
      break;
    case 0x65:
      //  LD Vx, [I]
      snprintf(dis, 64, "LD V%d, [I]", _x);
      break;
    default:
      snprintf(dis, 64, "UNKNOWN");
      break;
    }
    break;

  default:
    snprintf(dis, 64, "UNKNOWN");
    break;
  }

  snprintf(disasm, 64, "%s", dis);
}

void debug8::init_screen()
{
  // init SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    printf("ERROR: %s\n", SDL_GetError());
    exit(-1);
  }

  // load windows positions from disk
  bool load_ok = load_struct();

  // create all windows and renderers
  for (int i = 0; i < WND_SIZE; i++)
  {
    if (SDL_CreateWindowAndRenderer(wnd_data[i].w, wnd_data[i].h, 0, &wnd.w[i],
                                    &wnd.r[i]))
    {
      printf("ERROR: %s\n", SDL_GetError());
      SDL_Quit();
      exit(-1);
    }
    SDL_SetWindowTitle(wnd.w[i], wnd_data[i].title);
    if (load_ok)
    {
      SDL_SetWindowPosition(wnd.w[i], wnd.x[i], wnd.y[i]);
    }
  }
  // copy for parent object
  window = wnd.w[MAIN];
  renderer = wnd.r[MAIN];
  SDL_RaiseWindow(window);

  // init font
  if (TTF_Init() < 0)
  {
    printf("ERROR: %s\n", TTF_GetError());
    exit(-1);
  }
  font = TTF_OpenFont("font.ttf", 20);
  if (!font)
  {
    printf("ERROR: %s\n", TTF_GetError());
    exit(-1);
  }

  // backup rom (for reload)
  backup_rom = (uint8_t *)malloc(4096 - 0x200);
  memcpy(backup_rom, &memory[0x0200], 4096 - 0x200);
}

void debug8::end_screen()
{
  // get windows positions
  for (int i = 0; i < WND_SIZE; i++)
  {
    // how much OS moved our windows?
    // int xx = wnd.x[i];
    // int yy = wnd.y[i];
    SDL_GetWindowPosition(wnd.w[i], &wnd.x[i], &wnd.y[i]);
    // printf("[%d]\tread: %d,%d\n\tread: %d,%d\n\tdiff: %d,%d\n", i, xx, yy,
    // wnd.x[i], wnd.y[i], xx - wnd.x[i], yy - wnd.y[i]);
    wnd.x[i] += 0;
    wnd.y[i] += -37;
  }
  // save to disk
  save_struct();

  // skip main
  for (int i = 1; i < WND_SIZE; i++)
  {
    SDL_DestroyRenderer(wnd.r[i]);
    SDL_DestroyWindow(wnd.w[i]);
  }

  // destroy main windows and quit sdl
  sdl2_chip8::end_screen();

  // free backup rom
  free(backup_rom);
}

bool debug8::handle_input()
{
  bool quit = false;
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
      bool shift;
      bool ctrl;
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
      // debugger keys (with repeat)
      shift = event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT);
      ctrl = event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL);
      // memory dump keys
      if (event.key.keysym.sym == SDLK_F2)
      {
        mem_ptr -= shift ? 0x100 : 0x10;
      }
      else if (event.key.keysym.sym == SDLK_F3)
      {
        mem_ptr += shift ? 0x100 : 0x10;
      }
      else if (event.key.keysym.sym == SDLK_F4)
      {
        mem_ptr = 0;
      }
      mem_ptr &= 0xfff;
      // breakpoints position keys
      if (event.key.keysym.sym == SDLK_F6)
      {
        brk_ptr -= shift ? 0x100 : ctrl ? 0x10
                                        : 0x1;
      }
      else if (event.key.keysym.sym == SDLK_F7)
      {
        brk_ptr += shift ? 0x100 : ctrl ? 0x10
                                        : 0x1;
      }
      brk_ptr &= 0xfff;
      break;
    case SDL_KEYUP:
      // chip8 keyboard handler
      check_keyrelease(&event);
      // debugger keys (no repeat)
      shift = event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT);
      ctrl = event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL);
      // help screen
      if (event.key.keysym.sym == SDLK_F1)
      {
        show_help();
      }
      // run
      if (event.key.keysym.sym == SDLK_F5)
      {
        debug = !debug;
      }
      // breakpoint toogle
      if (event.key.keysym.sym == SDLK_F8)
      {
        toogle_brk(brk_ptr, shift ? BREAKPOINT_MEMORY : BREAKPOINT_CODE);
      }
      // single step
      if (event.key.keysym.sym == SDLK_F9)
      {
        single_step = true;
      }
      // clear all breakpoints
      if (event.key.keysym.sym == SDLK_F11)
      {
        breakpoints.clear();
        brk_ptr = 0;
      }
      // reload
      if (event.key.keysym.sym == SDLK_F12)
      {
        debug = true;
        single_step = false;
        last_break = -1;
        init();
        load(backup_rom, 4096 - 0x200);
      }
      break;
    default:
      break;
    }
  }
  return quit;
}

void debug8::init()
{
  for (int i = 0; i < 16; i++)
  {
    shadow_reg[i] = 0;
  }
  shadow_sp = 0;
  shadow_index = 0;
  chip8::init();
}

bool debug8::loop(bool d, bool s)
{
  // if previous loop caused exception, rewind and debug
  if (exception != NO_EXCEPTION)
  {
    pc -= 2;
    debug = d = true;
    single_step = s = false;
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR",
                             exception_text[exception], 0);
    exception = NO_EXCEPTION;
  }
  // check breakpoint
  if (check_brk(pc, BREAKPOINT_CODE))
  {
    if (last_break != pc)
    {
      debug = d = true;
      single_step = s = false;
      last_break = pc;
    }
  }
  // show debugger
  dump_stack();
  dump_registers();
  dump_memory();
  dump_breakpoints();
  dump_disasm();
  // debugger
  if (d)
  {
    // single step
    if (!s)
    {
      return false;
    }
    last_break = -1;
    single_step = false;
    for (int i = 0; i < 16; i++)
    {
      shadow_reg[i] = reg[i];
    }
    shadow_sp = sp;
    shadow_index = index;
    return sdl2_chip8::loop();
  }
  else
  {
    // run
    return sdl2_chip8::loop();
  }
}
