all: soundscope

UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin)
	CXXC = clang++
	CXXFLAGS = -framework OpenGL
else
	CXXC = g++
	CXXFLAGS = -lGL
endif

soundscope: soundscope.cpp Makefile
	$(CXXC) -O3 -o soundscope soundscope.cpp $(CXXFLAGS) `sdl-config --cflags` `sdl-config --libs`
clean:
	rm soundscope
