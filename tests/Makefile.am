
MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = $(XCBAUX_CFLAGS) $(XCBIMAGE_CFLAGS)

bin_PROGRAMS = julia lissajoux flames

julia_LDADD = $(XCBAUX_LIBS) $(XCBIMAGE_LIBS) $(XCBICCCM_LIBS) $(XCBATOM_LIBS)
julia_SOURCES = julia.c julia.h

lissajoux_LDADD = $(XCBAUX_LIBS) $(XCBIMAGE_LIBS) -lm
lissajoux_SOURCES = lissajoux.c lissajoux.h

flames_CFLAGS = -O3
flames_LDADD = $(XCBAUX_LIBS) $(XCBIMAGE_LIBS) $(XCBICCCM_LIBS) $(XCBATOM_LIBS)
flames_SOURCES = flames.c
