EXE = debug8

SOURCES = main.cpp chip8.cpp sdl2_chip8.cpp debug8.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

CXXFLAGS = -I./include -g -Wall -Wformat -DDEBUG
LIBS = `sdl2-config --libs` -lSDL2_ttf

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
