pkgs = xcb xcb-keysyms
CFLAGS = `pkg-config --cflags $(pkgs)` -g -Wall -Wpointer-arith -Wstrict-prototypes
LIBS = `pkg-config --libs $(pkgs)` -lXCBxtest

xte:	xte.c
	$(CC) $(CFLAGS) xte.c $(LIBS) -o xte
