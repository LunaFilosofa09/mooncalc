CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
HEADERS = value.hpp numeric.hpp bigint.hpp units.hpp calculator.hpp lineedit.hpp format.hpp

mooncalc: main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) main.cpp -o mooncalc

test: mooncalc
	sh tests/smoke.sh

clean:
	rm -f mooncalc mooncalc-gui

IMGUI = third_party/imgui
IMGUI_SRC = $(IMGUI)/imgui.cpp $(IMGUI)/imgui_draw.cpp $(IMGUI)/imgui_tables.cpp $(IMGUI)/imgui_widgets.cpp \
            $(IMGUI)/imgui_impl_metal.mm $(IMGUI)/imgui_impl_osx.mm

gui: mooncalc-gui

mooncalc-gui: gui.mm gui_ui.hpp $(HEADERS) $(IMGUI_SRC)
	$(CXX) -std=c++17 -O2 -fobjc-arc -I$(IMGUI) gui.mm $(IMGUI_SRC) \
		-framework Cocoa -framework Metal -framework MetalKit -framework QuartzCore -framework GameController \
		-o mooncalc-gui

.PHONY: test clean gui
