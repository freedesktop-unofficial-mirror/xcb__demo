AC_PREREQ(2.57)
AC_INIT([xcb-demo],0.8,[jamey@minilop.net])
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
dnl AC_PROG_INSTALL
dnl AC_PROG_LN_S
dnl AM_PROG_LIBTOOL
dnl AC_PROG_MAKE_SET

PKG_CHECK_MODULES(XCB, xcb)
PKG_CHECK_MODULES(XCBAUX, xcb-aux)
PKG_CHECK_MODULES(XCBIMAGE, xcb-image)
PKG_CHECK_MODULES(XCBICCCM, xcb-icccm)

AC_OUTPUT([Makefile tests/Makefile])
