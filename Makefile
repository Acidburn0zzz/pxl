CFLAGS = -g -std=c99 -W -Wall -O3 -D_GNU_SOURCE $(shell sdl-config --cflags)
LDFLAGS = -lm $(shell sdl-config --libs)

pxl: pxl.c

clean:
	rm -f pxl
