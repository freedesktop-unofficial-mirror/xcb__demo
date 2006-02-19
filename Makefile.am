
MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = tests

EXTRA_DIST = autogen.sh README

INCLUDES = $(XCB_CFLAGS)
LDADD = $(XCB_LIBS)

bin_PROGRAMS = hypnomoire xdpyinfo xcb-test dpms rendertest xcbrandr

hypnomoire_LDADD = $(LDADD) $(XCBAUX_LIBS) -lm -lpthread
hypnomoire_SOURCES = hypnomoire.c reply_formats.c

xdpyinfo_SOURCES = xdpyinfo.c

xcb_test_LDADD = $(LDADD) $(XCBAUX_LIBS) -lpthread
xcb_test_SOURCES = main.c reply_formats.c

dpms_SOURCES = dpms.c

rendertest_LDADD = $(XCBAUX_LIBS)
rendertest_SOURCES = rendertest.c

xcbrandr_LDADD = $(XCBAUX_LIBS)
xcbrandr_SOURCES = xcbrandr.c
