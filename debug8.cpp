#include "debug8.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstring>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

namespace {
const ImVec4 COL_NORMAL(0.55f, 0.55f, 0.55f, 1.0f);
const ImVec4 COL_WHITE(1.00f, 1.00f, 1.00f, 1.0f);
const ImVec4 COL_CODE(1.00f, 0.30f, 0.30f, 1.0f);
const ImVec4 COL_MEMORY(0.40f, 0.50f, 1.00f, 1.0f);
const ImVec4 COL_RUN(0.40f, 1.00f, 0.40f, 1.0f);
const ImVec4 COL_PAUSED(1.00f, 0.75f, 0.30f, 1.0f);
} // namespace

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
    if ((i.ptr == ptr) && (i.type == type)) {
      return true;
    }
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
  // find current path
  char fp[512];
  readlink("/proc/self/exe", fp, 512);
  strcpy(strrchr(fp, '/') + 1, ".config.bin");
  //
  wnd_pos.magic = CONFIG_MAGIC;
  FILE *f;
  f = fopen(fp, "wb");
  if (!f) {
    return false;
  }
  fwrite(&wnd_pos, 1, sizeof(wnd_pos), f);
  fclose(f);
  return true;
}

bool debug8::load_struct() {
  // find current path
  char fp[512];
  readlink("/proc/self/exe", fp, 512);
  strcpy(strrchr(fp, '/') + 1, ".config.bin");
  //
  FILE *f;
  f = fopen(fp, "rb");
  if (!f) {
    return false;
  }
  memset(&wnd_pos, 0, sizeof(wnd_pos));
  size_t n = fread(&wnd_pos, 1, sizeof(wnd_pos), f);
  fclose(f);
  return (n == sizeof(wnd_pos)) && (wnd_pos.magic == CONFIG_MAGIC);
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

void debug8::draw_toolbar() {
  if (ImGui::Button(debug ? "Run (F5)" : "Pause (F5)")) {
    debug = !debug;
  }
  ImGui::SameLine();
  if (ImGui::Button("Step (F9)")) {
    debug = true;
    single_step = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Reload (F12)")) {
    debug = true;
    single_step = false;
    last_break = -1;
    init();
    load(backup_rom, 4096 - 0x200);
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear BRK (F11)")) {
    breakpoints.clear();
    brk_ptr = 0;
  }
  ImGui::SameLine();
  ImGui::TextColored(debug ? COL_PAUSED : COL_RUN,
                     debug ? "PAUSED" : "RUNNING");

  // second row: addresses + explicit breakpoint toggles
  ImGui::SetNextItemWidth(120);
  uint16_t step = 0x10, fast = 0x100;
  if (ImGui::InputScalar("MEM", ImGuiDataType_U16, &mem_ptr, &step, &fast,
                         "%04X", ImGuiInputTextFlags_CharsHexadecimal)) {
    mem_ptr &= 0xfff;
  }
  ImGui::SameLine();
  ImGui::SetNextItemWidth(120);
  uint16_t bstep = 0x1, bfast = 0x10;
  if (ImGui::InputScalar("BRK", ImGuiDataType_U16, &brk_ptr, &bstep, &bfast,
                         "%04X", ImGuiInputTextFlags_CharsHexadecimal)) {
    brk_ptr &= 0xfff;
  }
  ImGui::SameLine();
  if (ImGui::Button("Toggle Code BRK (F8)")) {
    toogle_brk(brk_ptr, BREAKPOINT_CODE);
  }
  ImGui::SameLine();
  if (ImGui::Button("Toggle Mem BRK (Shift+F8)")) {
    toogle_brk(brk_ptr, BREAKPOINT_MEMORY);
  }
  ImGui::TextDisabled(
      "Click a disasm line / memory byte to select its address; double-click "
      "toggles the breakpoint");
}

void debug8::draw_registers() {
  if (!ImGui::CollapsingHeader("REGISTERS", ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }
  // 4 registers per row so the panel never clips on narrow layouts
  for (int i = 0; i < 16; i++) {
    ImGui::TextColored(reg[i] != shadow_reg[i] ? COL_WHITE : COL_NORMAL,
                       "V%02d=%02X", i, reg[i]);
    if ((i % 4) != 3) {
      ImGui::SameLine(0, 20);
    }
  }
  ImGui::TextColored(COL_NORMAL, "DT=%02X  ST=%02X  PC=%04X", delay_timer,
                     sound_timer, pc);
  ImGui::TextColored(sp != shadow_sp ? COL_WHITE : COL_NORMAL, "SP=%02X", sp);
  ImGui::SameLine(0, 14);
  ImGui::TextColored(index != shadow_index ? COL_WHITE : COL_NORMAL,
                     "INDEX=%04X", index);
}

void debug8::draw_stack() {
  if (!ImGui::CollapsingHeader("STACK", ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }
  for (int i = 0; i < 16; i++) {
    ImGui::TextColored(sp == i ? COL_WHITE : COL_NORMAL, "[%02d] 0x%04X", i,
                       stack[i]);
  }
}

void debug8::draw_breakpoints() {
  if (!ImGui::CollapsingHeader("BREAKPOINTS", ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }
  ImGui::TextColored(COL_WHITE, "> 0x%04X <", brk_ptr);
  ImGui::TextDisabled("F8=code  Shift+F8=mem");
  ImGui::TextDisabled("dbl-click disasm/mem toggles");
  if (breakpoints.empty()) {
    ImGui::TextDisabled("(none)");
    return;
  }
  for (size_t i = 0; i < breakpoints.size();) {
    ImGui::PushID((int)i);
    if (ImGui::SmallButton("x")) {
      remove_brk(breakpoints[i].ptr, breakpoints[i].type);
      ImGui::PopID();
      continue;
    }
    ImGui::SameLine();
    const _brk &bp = breakpoints[i];
    char label[64];
    snprintf(label, sizeof(label), "0x%04X  %s", bp.ptr,
             bp.type == BREAKPOINT_MEMORY ? "MEM" : "CODE");
    if (ImGui::Selectable(label)) {
      brk_ptr = bp.ptr;
    }
    ImGui::PopID();
    i++;
  }
}

void debug8::draw_memory() {
  if (!ImGui::CollapsingHeader("MEMORY", ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }
  // zero padding keeps each byte cell exactly as wide as its text
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
  uint16_t p = mem_ptr;
  for (int lin = 0; lin < 10; lin++) {
    ImGui::TextColored(COL_NORMAL, "%04X ", p);
    for (int col = 1; col <= 16; col++) {
      ImVec4 c = COL_NORMAL;
      if (check_brk(p, BREAKPOINT_CODE)) {
        c = COL_CODE;
      } else if (check_brk(p, BREAKPOINT_MEMORY)) {
        c = COL_MEMORY;
      }
      const char *pad = (col % 8 == 0) ? "   " : (col % 4 == 0) ? "  " : " ";
      char b[8];
      snprintf(b, sizeof(b), "%02X%s", memory[p], pad);
      float cellw = ImGui::CalcTextSize("00").x + ImGui::CalcTextSize(pad).x;
      ImGui::SameLine(0, 0);
      ImGui::PushID(p);
      ImGui::PushStyleColor(ImGuiCol_Text, c);
      bool clicked = ImGui::Selectable(b, p == brk_ptr, 0, ImVec2(cellw, 0));
      ImGui::PopStyleColor();
      if (clicked) {
        brk_ptr = p;
      }
      if (ImGui::IsItemHovered() &&
          ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        toogle_brk(p, BREAKPOINT_MEMORY);
      }
      if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Toggle memory breakpoint")) {
          toogle_brk(p, BREAKPOINT_MEMORY);
        }
        if (ImGui::MenuItem("Toggle code breakpoint")) {
          toogle_brk(p, BREAKPOINT_CODE);
        }
        ImGui::EndPopup();
      }
      ImGui::PopID();
      p = (p + 1) & 0xfff;
    }
  }
  ImGui::PopStyleVar();
}

void debug8::draw_disasm() {
  if (!ImGui::CollapsingHeader("DISASSEMBLY", ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }
  char address[64];
  char op[64];
  char disasm[64];
  for (int i = pc - (7 * 2); i <= pc + (7 * 2); i += 2) {
    uint16_t a = (uint16_t)i;
    disassemble(a, address, op, disasm);
    ImVec4 c = COL_NORMAL;
    if (a == pc) {
      c = COL_WHITE;
    } else if (check_brk(a, BREAKPOINT_CODE)) {
      c = COL_CODE;
    } else if (check_brk(a, BREAKPOINT_MEMORY)) {
      c = COL_MEMORY;
    }
    char line[256];
    snprintf(line, sizeof(line), "%s  %s     %s", address, op, disasm);
    ImGui::PushStyleColor(ImGuiCol_Text, c);
    bool clicked = ImGui::Selectable(line, a == brk_ptr);
    ImGui::PopStyleColor();
    if (clicked) {
      brk_ptr = a;
    }
    if (ImGui::IsItemHovered() &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
      toogle_brk(a, BREAKPOINT_CODE);
    }
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem("Select this address")) {
        brk_ptr = a;
      }
      if (ImGui::MenuItem("Toggle code breakpoint")) {
        toogle_brk(a, BREAKPOINT_CODE);
      }
      if (ImGui::MenuItem("Toggle memory breakpoint")) {
        toogle_brk(a, BREAKPOINT_MEMORY);
      }
      ImGui::EndPopup();
    }
  }
}

void debug8::disassemble(uint16_t pc, char *address, char *op, char *disasm) {
  uint16_t opcode = (memory[pc & 0xfff] << 8) + memory[(pc + 1) & 0xfff];

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
      break;
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

  // main (emulator) window + renderer
  sdl2_chip8::init_screen();

  // single debugger window + renderer (all panels live inside it)
  if (SDL_CreateWindowAndRenderer(1100, 720, 0, &dbg_window, &dbg_renderer)) {
    printf("ERROR: %s\n", SDL_GetError());
    SDL_Quit();
    exit(-1);
  }
  SDL_SetWindowTitle(dbg_window, "DEBUGGER");

  if (load_ok) {
    SDL_SetWindowPosition(window, wnd_pos.x[MAIN], wnd_pos.y[MAIN]);
    SDL_SetWindowPosition(dbg_window, wnd_pos.x[DEBUGGER], wnd_pos.y[DEBUGGER]);
  }
  SDL_RaiseWindow(window);

  // init dear imgui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  ImGui::StyleColorsDark();

  // the built-in default font is monospace (ProggyForever) and scales
  // cleanly, so no external font file is required
  ImGui::GetStyle().FontSizeBase = 18.0f;
  io.Fonts->AddFontDefault();

  ImGui_ImplSDL2_InitForSDLRenderer(dbg_window, dbg_renderer);
  ImGui_ImplSDLRenderer2_Init(dbg_renderer);

  // backup rom (for reload)
  backup_rom = (uint8_t *)malloc(4096 - 0x200);
  memcpy(backup_rom, &memory[0x0200], 4096 - 0x200);
}

void debug8::end_screen() {
  // get windows positions
  SDL_GetWindowPosition(window, &wnd_pos.x[MAIN], &wnd_pos.y[MAIN]);
  SDL_GetWindowPosition(dbg_window, &wnd_pos.x[DEBUGGER], &wnd_pos.y[DEBUGGER]);
  // save to disk
  save_struct();

  // shutdown dear imgui
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  // destroy debugger window
  SDL_DestroyRenderer(dbg_renderer);
  SDL_DestroyWindow(dbg_window);

  // destroy main window and quit sdl
  sdl2_chip8::end_screen();

  // free backup rom
  free(backup_rom);
}

bool debug8::handle_input() {
  bool quit = false;
  ImGuiIO &io = ImGui::GetIO();
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
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
      // don't send keys to the emulator while typing in the debugger ui
      if (io.WantCaptureKeyboard || io.WantTextInput) {
        break;
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
      // don't send keys to the emulator while typing in the debugger ui
      if (io.WantCaptureKeyboard || io.WantTextInput) {
        break;
      }
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
        debug = true;
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

void debug8::init() {
  for (int i = 0; i < 16; i++) {
    shadow_reg[i] = 0;
  }
  shadow_sp = 0;
  shadow_index = 0;
  chip8::init();
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
  // debugger
  if (d) {
    // single step
    if (!s) {
      return false;
    }
    last_break = -1;
    single_step = false;
    for (int i = 0; i < 16; i++) {
      shadow_reg[i] = reg[i];
    }
    shadow_sp = sp;
    shadow_index = index;
    // execute exactly one instruction (no timers, no cycle budget)
    return chip8::step();
  } else {
    // run with the normal cpu/timer budget
    return sdl2_chip8::loop();
  }
}

void debug8::show_debugger() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  draw_debugger();

  ImGui::Render();
  SDL_SetRenderDrawColor(dbg_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(dbg_renderer);
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), dbg_renderer);
  SDL_RenderPresent(dbg_renderer);
}

void debug8::draw_debugger() {
  ImGuiIO &io = ImGui::GetIO();
  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar;

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::Begin("DEBUG8", nullptr, flags);

  draw_toolbar();
  ImGui::Separator();

  float left = ImGui::GetContentRegionAvail().x * 0.38f;
  ImGui::BeginChild("regs_stack_brk", ImVec2(left, 0), true);
  draw_registers();
  draw_stack();
  draw_breakpoints();
  ImGui::EndChild();

  ImGui::SameLine();

  ImGui::BeginChild("disasm_mem", ImVec2(0, 0), true);
  draw_disasm();
  draw_memory();
  ImGui::EndChild();

  ImGui::End();
}
