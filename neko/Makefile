pkgs = xcb xcb-aux xcb-icccm xcb-atom xcb-keysyms
CFLAGS = `pkg-config --cflags $(pkgs)` -g -Wall -Wpointer-arith -Wstrict-prototypes
LIBS = `pkg-config --libs $(pkgs)` -lm

xcbneko:	xcbneko.c
	$(CC) $(CFLAGS) xcbneko.c $(LIBS) -o xcbneko
