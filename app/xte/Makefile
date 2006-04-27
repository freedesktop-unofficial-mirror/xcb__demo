pkgs = xcb xcb-keysyms xcb-xtest
CFLAGS = `pkg-config --cflags $(pkgs)` -g -Wall -Wpointer-arith -Wstrict-prototypes
LIBS = `pkg-config --libs $(pkgs)`

xte:	xte.c
	$(CC) $(CFLAGS) xte.c $(LIBS) -o xte
