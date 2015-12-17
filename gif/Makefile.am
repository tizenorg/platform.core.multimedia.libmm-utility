ACLOCAL_AMFLAGS='-I m4'

lib_LTLIBRARIES = libmmutil_gif.la

installmmutil_gifdir = $(includedir)/mmf
installmmutil_gif_HEADERS = include/mm_util_gif.h

libmmutil_gif_la_SOURCES = mm_util_gif.c

libmmutil_gif_la_CFLAGS = -I$(srcdir)/include \
				-I$(srcdir)/../imgp/include \
				$(GLIB_CFLAGS) \
				$(MEDIA_CFLAGS) \
				-I/usr/include \
				$(SYSTEMINFO_CFLAGS) \
				$(srcdir)/../imgp/libmmutil_imgp.la

libmmutil_gif_la_LIBADD = $(GLIB_LIBS) -lgif -ldl -lexif -lttrace\
				$(GMODULE_LIBS) \
				$(SYSTEMINFO_LIBS)\
				$(MEDIA_LIBS)

libmmutil_gif_la_CFLAGS += -DLIBPREFIX=\"$(libdir)\"

libmmutil_gif_la_DEPENDENCIES = $(srcdir)/../imgp/libmmutil_imgp.la

pcfiles = mmutil-gif.pc
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)
EXTRA_DIST = $(pcfiles)
SUBDIRS = . test
