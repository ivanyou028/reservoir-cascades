CXX      := clang++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
SRC      := core/scene.cpp core/image.cpp core/vanilla.cpp core/restir.cpp \
            core/reference.cpp metrics/metrics.cpp cpu-ref/main.cpp
HDR      := $(wildcard core/*.h metrics/*.h)

build/rc: $(SRC) $(HDR) | build
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

build:
	mkdir -p build

.PHONY: clean test
clean:
	rm -rf build out

test: build/rc
	./build/rc selftest --scene scenes/s1.json --size 128
	./build/rc selftest --scene scenes/s2.json --size 128
