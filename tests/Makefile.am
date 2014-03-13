AM_CPPFLAGS = -I$(top_srcdir)/include

TESTS = \
	test \
	test-dummy

noinst_PROGRAMS = \
	$(TESTS)

test_SOURCES = \
	main.c

test_dummy_SOURCES = \
	main.c

test_LDADD = \
	../src/libocean.la

test_dummy_LDADD = \
	../src/libocean-dummy.la
