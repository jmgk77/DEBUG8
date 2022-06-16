#!/bin/sh
g++ main.cpp chip8.cpp ncurses_chip8.cpp -I. -o chip8_ncurses -lstdc++ -lncurses -DCONSOLE
g++ main.cpp chip8.cpp sdl1_chip8.cpp -I. -o chip8_sdl1 -lstdc++ -lSDL -DSDL1
g++ main.cpp chip8.cpp sdl2_chip8.cpp -I. -o chip8 -lstdc++ -lSDL2 -DSDL2
g++ main.cpp chip8.cpp sdl2_chip8.cpp debug8.cpp -I. -o debug8 -lstdc++ -lSDL2 -lSDL2_ttf -DDEBUG
