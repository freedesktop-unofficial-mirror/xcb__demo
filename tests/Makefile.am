
MAINTAINERCLEANFILES = Makefile.in

INCLUDES = $(XCBAUX_CFLAGS) $(XCBIMAGE_CFLAGS)

bin_PROGRAMS = julia lissajoux flames

julia_LDADD = $(XCBAUX_LIBS) $(XCBIMAGE_LIBS)
julia_SOURCES = julia.c

lissajoux_LDADD = $(XCBAUX_LIBS) $(XCBIMAGE_LIBS) -lm
lissajoux_SOURCES =lissajoux.c

flames_CFLAGS = -O3
flames_LDADD = $(XCBAUX_LIBS) $(XCBIMAGE_LIBS) $(XCBICCCM_LIBS)
flames_SOURCES = flames.c
