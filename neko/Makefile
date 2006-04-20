CFLAGS = `pkg-config --cflags xcb` -g -Wall -Wpointer-arith -Wstrict-prototypes
LIBS = `pkg-config --libs xcb-aux xcb-icccm xcb-atom xcb-keysyms` -lm

xcbneko:	xcbneko.c
	$(CC) $(CFLAGS) xcbneko.c $(LIBS) -o xcbneko
