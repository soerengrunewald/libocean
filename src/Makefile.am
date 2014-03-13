AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src

lib_LTLIBRARIES = \
	libocean.la \
	libocean-dummy.la

noinst_HEADERS = \
	libocean_p.h

libocean_la_SOURCES = \
	ocean-common.c \
	ocean-nirquest.c

libocean_la_CFLAGS = \
	$(LIBUSB_CFLAGS)

libocean_la_LIBADD = \
	$(LIBUSB_LIBS)

libocean_dummy_la_SOURCES = \
	ocean-dummy.c

if WIN32
libocean_la_LIBADD += -lws2_32
libocean_dummy_la_LIBADD = -lws2_32
endif
