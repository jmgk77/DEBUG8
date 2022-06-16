#include "debug8.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstring>

void debug8::show_help() {
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

void debug8::toogle_brk(uint16_t ptr, uint8_t type) {
  if (check_brk(ptr, type)) {
    remove_brk(ptr, type);
  } else {
    set_brk(ptr, type);
  }
}

bool debug8::check_brk(uint16_t ptr, uint8_t type) {
  for (auto i : breakpoints) {
    if ((i.ptr == ptr) && (i.type == type)) return true;
  }
  return false;
}

void debug8::set_brk(uint16_t ptr, uint8_t type) {
  breakpoints.push_back({ptr, type});
}

void debug8::remove_brk(uint16_t ptr, uint8_t type) {
  uint16_t j = 0;
  for (auto i : breakpoints) {
    if ((i.ptr == ptr) && (i.type == type)) {
      breakpoints.erase(breakpoints.begin() + j);
    }
    j++;
  }
}

bool debug8::save_struct() {
  FILE *f;
  f = fopen(".config.bin", "wb");
  if (!f) return false;
  fwrite(&wnd, 1, sizeof(wnd), f);
  fclose(f);
  return true;
}

bool debug8::load_struct() {
  FILE *f;
  f = fopen(".config.bin", "rb");
  if (!f) return false;
  fread(&wnd, 1, sizeof(wnd), f);
  fclose(f);
  return true;
}

void debug8::set_RAM(uint16_t ptr, uint8_t val) {
  if (check_brk(ptr, BREAKPOINT_MEMORY)) {
    debug = true;
    single_step = false;
  }
  chip8::set_RAM(ptr, val);
}

uint8_t debug8::get_RAM(uint16_t ptr) {
  if (check_brk(ptr, BREAKPOINT_MEMORY)) {
    debug = true;
    single_step = false;
  }
  return chip8::get_RAM(ptr);
}

void debug8::create_text(char *dump, uint16_t jnl, uint16_t w, SDL_Color c) {
  SDL_Surface *text_surf = TTF_RenderText_Blended_Wrapped(font, dump, c, w);
  SDL_Texture *text_texture =
      SDL_CreateTextureFromSurface(wnd.r[jnl], text_surf);
  SDL_Rect r;
  r.x = 0;
  r.y = 0;
  r.w = text_surf->w;
  r.h = text_surf->h;
  SDL_RenderClear(wnd.r[jnl]);
  SDL_RenderCopy(wnd.r[jnl], text_texture, &r, &r);
  SDL_FreeSurface(text_surf);
  SDL_DestroyTexture(text_texture);
  SDL_RenderPresent(wnd.r[jnl]);
}

void debug8::dump_breakpoints() {
  char dump[4096];
  char tmp[32];
  dump[0] = 0x0;
  snprintf(tmp, sizeof(tmp), "> 0x%04x <\n", brk_ptr);
  strcat(dump, tmp);
  if (!breakpoints.empty()) {
    for (auto i : breakpoints) {
      snprintf(tmp, sizeof(tmp), "%c 0x%04x\n",
               (i.type == BREAKPOINT_MEMORY) ? 'M' : 'C', i.ptr);
      strcat(dump, tmp);
    }
  }
  //
  create_text(dump, BRK_LIST, 100);
}

void debug8::dump_stack() {
  char dump[4096];
  char tmp[32];
  dump[0] = 0x0;
  for (int i = 0; i < 16; i++) {
    snprintf(tmp, sizeof(tmp), "%1$c 0x%2$04x %1$c\n", (sp == i) ? '*' : ' ',
             stack[i]);
    strcat(dump, tmp);
  }
  //
  create_text(dump, STACK, 100);
}

void debug8::dump_registers() {
  char dump[4096];
  char tmp[512];
  dump[0] = 0x0;
  // Vx (0..7)
  for (int i = 0; i < 8; i++) {
    snprintf(tmp, sizeof(tmp), "V%02d=0x%04x ", i, reg[i]);
    strcat(dump, tmp);
  }
  // Vx (8..15)
  strcat(dump, "\n");
  for (int i = 8; i < 16; i++) {
    snprintf(tmp, sizeof(tmp), "V%02d=0x%04x ", i, reg[i]);
    strcat(dump, tmp);
  }
  // others
  snprintf(
      tmp, sizeof(tmp),
      "\nDT=%04X            ST=%04X             SP=%04X            PC=%04X "
      "           INDEX=%04X\n",
      delay_timer, sound_timer, sp, pc, index);
  strcat(dump, tmp);
  //
  create_text(dump, REGISTERS, 875);
}

void debug8::dump_memory() {
  char dump[4096];
  char tmp[16];
  dump[0] = 0x0;
  //
  uint16_t p = mem_ptr;
  for (int lin = 0; lin < 10; lin++) {
    snprintf(tmp, sizeof(tmp), "0x%04x  ", p);
    strcat(dump, tmp);
    for (int col = 0; col < 16; col++) {
      snprintf(tmp, sizeof(tmp), "%02x %s", memory[p++], (col == 7) ? " " : "");
      strcat(dump, tmp);
    }
    strcat(dump, "\n");
    p &= 0xfff;
  }
  //
  create_text(dump, MEMORY, 565);
}

void debug8::dump_disasm() {
  char dump[4096];
  char address[16];
  char op[16];
  char disasm[32];
  char tmp[1024];
  dump[0] = 0x0;
  for (int i = pc - (7 * 2); i <= pc + (7 * 2); i += 2) {
    disassemble(i, address, op, disasm);
    snprintf(tmp, sizeof(tmp), "%s %s  %c  %s\n", address, op,
             (pc == i) ? '*' : ' ', disasm);
    strcat(dump, tmp);
  }
  create_text(dump, DISASM, 550);
}

void debug8::disassemble(uint16_t pc, char *address, char *op, char *disasm) {
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

  switch (_op) {
    case 0x00:
      switch (_kk) {
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
      switch (_n) {
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
      switch (_kk) {
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
      switch (_kk) {
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

void debug8::init_screen() {
  // load windows positions from disk
  bool load_ok = load_struct();

  // create all windows and renderers
  for (int i = 0; i < WND_SIZE; i++) {
    if (SDL_CreateWindowAndRenderer(wnd_data[i].w, wnd_data[i].h, 0, &wnd.w[i],
                                    &wnd.r[i])) {
      printf("ERROR: %s\n", SDL_GetError());
      SDL_Quit();
      exit(-1);
    }
    SDL_SetWindowTitle(wnd.w[i], wnd_data[i].title);
    if (load_ok) {
      SDL_SetWindowPosition(wnd.w[i], wnd.x[i], wnd.y[i]);
    }
  }
  // copy for parent object
  window = wnd.w[MAIN];
  renderer = wnd.r[MAIN];
  SDL_RaiseWindow(window);

  // init font
  if (TTF_Init() < 0) {
    printf("ERROR: %s\n", TTF_GetError());
    exit(-1);
  }
  font = TTF_OpenFont("font.ttf", 20);
  if (!font) {
    printf("ERROR: %s\n", TTF_GetError());
    exit(-1);
  }

  // backup rom (for reload)
  backup_rom = (uint8_t *)malloc(4096 - 0x200);
  memcpy(backup_rom, &memory[0x0200], 4096 - 0x200);
}

void debug8::end_screen() {
  // get windows positions
  for (int i = 0; i < WND_SIZE; i++) {
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
  for (int i = 1; i < WND_SIZE; i++) {
    SDL_DestroyRenderer(wnd.r[i]);
    SDL_DestroyWindow(wnd.w[i]);
  }

  // destroy main windows and quit sdl
  sdl2_chip8::end_screen();

  // free backup rom
  free(backup_rom);
}

bool debug8::handle_input() {
  bool quit = false;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      bool shift;
      bool ctrl;
      case SDL_QUIT:
        quit = true;
        break;
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          quit = true;
        }
        // chip8 keyboard handler
        check_keypress(&event);
        // debugger keys (with repeat)
        shift = event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT);
        ctrl = event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL);
        // memory dump keys
        if (event.key.keysym.sym == SDLK_F2) {
          mem_ptr -= shift ? 0x100 : 0x10;
        } else if (event.key.keysym.sym == SDLK_F3) {
          mem_ptr += shift ? 0x100 : 0x10;
        } else if (event.key.keysym.sym == SDLK_F4) {
          mem_ptr = 0;
        }
        mem_ptr &= 0xfff;
        // breakpoints position keys
        if (event.key.keysym.sym == SDLK_F6) {
          brk_ptr -= shift ? 0x100 : ctrl ? 0x10 : 0x1;
        } else if (event.key.keysym.sym == SDLK_F7) {
          brk_ptr += shift ? 0x100 : ctrl ? 0x10 : 0x1;
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
        if (event.key.keysym.sym == SDLK_F1) {
          show_help();
        }
        // run
        if (event.key.keysym.sym == SDLK_F5) {
          debug = !debug;
        }
        // breakpoint toogle
        if (event.key.keysym.sym == SDLK_F8) {
          toogle_brk(brk_ptr, shift ? BREAKPOINT_MEMORY : BREAKPOINT_CODE);
        }
        // single step
        if (event.key.keysym.sym == SDLK_F9) {
          single_step = true;
        }
        // clear all breakpoints
        if (event.key.keysym.sym == SDLK_F11) {
          breakpoints.clear();
          brk_ptr = 0;
        }
        // reload
        if (event.key.keysym.sym == SDLK_F12) {
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

bool debug8::loop(bool d, bool s) {
  // if previous loop caused exception, rewind and debug
  if (exception != NO_EXCEPTION) {
    pc -= 2;
    debug = d = true;
    single_step = s = false;
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR",
                             exception_text[exception], 0);
    exception = NO_EXCEPTION;
  }
  // check breakpoint
  if (check_brk(pc, BREAKPOINT_CODE)) {
    if (last_break != pc) {
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
  if (d) {
    // single step
    if (!s) {
      return false;
    }
    last_break = -1;
    single_step = false;
    return sdl2_chip8::loop();
  } else {
    // run
    return sdl2_chip8::loop();
  }
}
