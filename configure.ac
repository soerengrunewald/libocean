dnl required version of autoconf
AC_PREREQ([2.60])

AC_INIT([libocean], [0.1.3], [soeren.grunewald@desy.de])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([no-dist-gzip dist-xz foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl enable mainainer mode by default
AM_MAINTAINER_MODE([enable])

dnl
dnl check for tools (compiler etc.)
dnl
AC_PROG_INSTALL
AC_PROG_CC_C99
AM_PROG_CC_C_O
AM_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG()

dnl
dnl Allow builds for windows, yet untested
dnl
case $host_os in
	mingw32*)
		win32=yes
		;;
esac

if test "x$win32" != xyes; then
	CFLAGS="$CFLAGS -fvisibility=hidden"
fi

AM_CONDITIONAL(WIN32, [test "x$win32" = xyes])

dnl
dnl Update CFLAGS
dnl
CFLAGS="$CFLAGS -Wall"

AC_ARG_ENABLE([werror],
	[AS_HELP_STRING([--enable-werror],
		[Treat warnings as errors (default: disabled)])],
	[enable_werror="$enableval"],
	[enable_werror=no])
if test "x$enable_werror" = "xyes"; then
	CFLAGS="$CFLAGS -Werror"
fi

PKG_CHECK_MODULES(LIBUSB, [libusb-1.0 > 1.0.13], [have_libusbx=1],
	[PKG_CHECK_MODULES(LIBUSB, [libusb-1.0], [have_libusbx=0],
		[AC_MSG_ERROR([Unable to find libusb-1.0])])])

AC_DEFINE_UNQUOTED([HAVE_LIBUSB_EXTENDED], [$have_libusbx],
	[Whether we can use some functions from later libusb version])

dnl
dnl Gen Makefiles
dnl
AC_OUTPUT([
	Makefile
	include/Makefile
	src/Makefile
	tests/Makefile
	libocean.pc
	libocean-dummy.pc
])
