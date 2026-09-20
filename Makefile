EXE = debug8

IMGUI_DIR = imgui

SOURCES = main.cpp chip8.cpp sdl2_chip8.cpp debug8.cpp \
          $(IMGUI_DIR)/imgui.cpp \
          $(IMGUI_DIR)/imgui_draw.cpp \
          $(IMGUI_DIR)/imgui_tables.cpp \
          $(IMGUI_DIR)/imgui_widgets.cpp \
          $(IMGUI_DIR)/imgui_demo.cpp \
          $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp \
          $(IMGUI_DIR)/backends/imgui_impl_sdlrenderer2.cpp

OBJS = $(SOURCES:.cpp=.o)
DEPS = $(OBJS:.o=.d)

CXXFLAGS = -I./include -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends `sdl2-config --cflags` -g -Wall -Wformat -DDEBUG
LIBS = `sdl2-config --libs`

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

-include $(DEPS)

clean:
	rm -f $(EXE) $(OBJS) $(DEPS)
