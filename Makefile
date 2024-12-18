CC=gcc
CFLAGS=`pkg-config sdl3 sdl3-ttf --cflags`
LIBS=`pkg-config sdl3 sdl3-ttf --libs`

all: main

main: main.c
	$(CC) $(CFLAGS) -o main main.c $(LIBS)