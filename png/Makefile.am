ACLOCAL_AMFLAGS='-I m4'

lib_LTLIBRARIES = libmmutil_png.la

installmmutil_pngdir = $(includedir)/mmf
installmmutil_png_HEADERS = include/mm_util_png.h

libmmutil_png_la_SOURCES = mm_util_png.c

libmmutil_png_la_CFLAGS = -I$(srcdir)/include \
				-I$(srcdir)/../imgp/include \
				$(GLIB_CFLAGS) \
				$(MEDIA_CFLAGS) \
				$(MMCOMMON_CFLAGS) \
				-I/usr/include \
				$(SYSTEMINFO_CFLAGS) \
				$(srcdir)/../imgp/libmmutil_imgp.la

libmmutil_png_la_LIBADD = $(MMCOMMON_LIBS) -lpng -ldl -lexif -lttrace\
				$(GLIB_LIBS) \
				$(GMODULE_LIBS) \
				$(SYSTEMINFO_LIBS)\
				$(MEDIA_LIBS)

libmmutil_png_la_CFLAGS += -DLIBPREFIX=\"$(libdir)\"

libmmutil_png_la_DEPENDENCIES = $(srcdir)/../imgp/libmmutil_imgp.la

pcfiles = mmutil-png.pc
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)
EXTRA_DIST = $(pcfiles)
SUBDIRS = . test
