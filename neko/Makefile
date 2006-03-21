CFLAGS = -g -Wall -Wpointer-arith -Wstrict-prototypes
LIBS = -lxcb -lXCBAux -lXCBICCCM -lXCBAtom -lXCBKeysyms

xcbneko:	xcbneko.c
	$(CC) $(CFLAGS) xcbneko.c $(LIBS) -o xcbneko

all:	xcbneko
