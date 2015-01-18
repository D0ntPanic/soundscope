all: soundscope

soundscope: soundscope.cpp Makefile
	g++ -O3 -o soundscope soundscope.cpp `sdl-config --cflags` `sdl-config --libs` -lGL

clean:
	rm soundscope

