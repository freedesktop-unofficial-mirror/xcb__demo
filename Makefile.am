
MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = tests

EXTRA_DIST = autogen.sh README

INCLUDES = $(XCB_CFLAGS)
LDADD = $(XCB_LIBS)

bin_PROGRAMS = hypnomoire xdpyinfo xcb-test dpms rendertest xcbrandr

hypnomoire_CFLAGS = $(XCBAUX_CFLAGS)
hypnomoire_LDADD = $(LDADD) $(XCBAUX_LIBS) -lm -lpthread
hypnomoire_SOURCES = hypnomoire.c reply_formats.c

xdpyinfo_SOURCES = xdpyinfo.c

xcb_test_CFLAGS = $(XCBAUX_CFLAGS)
xcb_test_LDADD = $(LDADD) $(XCBAUX_LIBS) -lpthread
xcb_test_SOURCES = main.c reply_formats.c

dpms_SOURCES = dpms.c

rendertest_CFLAGS = $(XCBAUX_CFLAGS)
rendertest_LDADD = $(XCBAUX_LIBS)
rendertest_SOURCES = rendertest.c

xcbrandr_CFLAGS = $(XCBAUX_CFLAGS)
xcbrandr_LDADD = $(XCBAUX_LIBS)
xcbrandr_SOURCES = xcbrandr.c
