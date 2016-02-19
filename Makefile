.PHONY: all
all: pong

pong:
	gcc -o pong src/pong.c `sdl2-config --libs --cflags` -lm
