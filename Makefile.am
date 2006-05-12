
MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = tests

EXTRA_DIST = autogen.sh README

INCLUDES = $(XCB_CFLAGS)
LDADD = $(XCB_LIBS)

bin_PROGRAMS = dpms hypnomoire rendertest xcb-test xcbrandr xcbxf86dri \
               xcbxvinfo xcbdpyinfo

hypnomoire_CFLAGS = $(XCBAUX_CFLAGS)
hypnomoire_LDADD = $(XCBAUX_LIBS) -lm -lpthread
hypnomoire_SOURCES = hypnomoire.c reply_formats.c reply_formats.h

xcbdpyinfo_SOURCES = xcbdpyinfo.c

xcb_test_CFLAGS = $(XCBAUX_CFLAGS)
xcb_test_LDADD = $(XCBAUX_LIBS) -lpthread
xcb_test_SOURCES = xcb-test.c reply_formats.c reply_formats.h

dpms_CFLAGS = $(XCBDPMS_CFLAGS)
dpms_LDADD = $(XCBDPMS_LIBS)
dpms_SOURCES = dpms.c

rendertest_CFLAGS = $(XCBAUX_CFLAGS) $(XCBRENDER_CFLAGS)
rendertest_LDADD = $(XCBAUX_LIBS) $(XCBRENDER_LIBS)
rendertest_SOURCES = rendertest.c

xcbrandr_CFLAGS = $(XCBAUX_CFLAGS) $(XCBRANDR_CFLAGS)
xcbrandr_LDADD = $(XCBAUX_LIBS) $(XCBRANDR_LIBS)
xcbrandr_SOURCES = xcbrandr.c

xcbxf86dri_CFLAGS = $(XCBXF86DRI_CFLAGS)
xcbxf86dri_LDADD = $(XCBXF86DRI_LIBS)
xcbxf86dri_SOURCES = xcbxf86dri.c

xcbxvinfo_CFLAGS = $(XCBXV_CFLAGS)
xcbxvinfo_LDADD = $(XCBXV_LIBS)
xcbxvinfo_SOURCES = xcbxvinfo.c
