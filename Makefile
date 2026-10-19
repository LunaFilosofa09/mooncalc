CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

mooncalc: main.cpp calculator.hpp
	$(CXX) $(CXXFLAGS) main.cpp -o mooncalc

test: mooncalc
	sh tests/smoke.sh

clean:
	rm -f mooncalc

.PHONY: test clean
