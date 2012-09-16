CFLAGS = -g -std=c99 -W -Wall -O3 -D_GNU_SOURCE $(shell sdl-config --cflags)
LDFLAGS = -lm $(shell sdl-config --libs)

pxl: pxl.o reader.o

test: pxl
	./pxl *.ppm
clean:
	rm -f pxl *.o
