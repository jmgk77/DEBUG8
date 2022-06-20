# DEBUG8

CHIP-8 debugger

## Features

* Code windows
* Data windows
* Stack windows
* Register windows
* Code breakpoints
* Memory breakpoints

## Usage

Run `./debug8 <ROM>`. Press <F1> to view key bindings. Code breakpoints are RED, memory breakpoints are BLUE.

## Images

![Screenshot from 2022-06-20 09-55-17](https://user-images.githubusercontent.com/46632344/174606721-5e4f85e6-7102-4653-9422-4498123a3b88.png)
![Screenshot from 2022-06-20 10-12-14](https://user-images.githubusercontent.com/46632344/174609414-7a7bbf15-3f57-40c7-b672-d8694b7da45b.png)
![Screenshot from 2022-06-20 09-54-53](https://user-images.githubusercontent.com/46632344/174606711-846019ac-1618-442e-bf98-1526a6dc74ce.png)

## Build

Run `make` to generate debug8. Included are Makefile.sdl1, Makefile.sdl2 and Makefile.ncurses, to build only the interpreter (without debugger) for SDL1.2, SDL2 and ncurses (text only).
