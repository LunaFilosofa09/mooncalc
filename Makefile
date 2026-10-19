CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
HEADERS = value.hpp numeric.hpp bigint.hpp units.hpp calculator.hpp lineedit.hpp

mooncalc: main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) main.cpp -o mooncalc

test: mooncalc
	sh tests/smoke.sh

clean:
	rm -f mooncalc

.PHONY: test clean
