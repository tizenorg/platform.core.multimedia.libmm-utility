ACLOCAL_AMFLAGS='-I m4'

lib_LTLIBRARIES = libmmutil_jpeg.la

installmmutil_jpegdir = $(includedir)/mmf
installmmutil_jpeg_HEADERS = include/mm_util_jpeg.h

libmmutil_jpeg_la_SOURCES = mm_util_jpeg.c

libmmutil_jpeg_la_CFLAGS = -I$(srcdir)/include \
				-I$(srcdir)/../imgp/include \
				$(GLIB_CFLAGS) \
				$(MEDIA_CFLAGS) \
				-I/usr/include \
				$(SYSTEMINFO_CFLAGS) \
				$(srcdir)/../imgp/libmmutil_imgp.la

libmmutil_jpeg_la_LIBADD = $(GLIB_LIBS) -ljpeg -ldl -lexif -lttrace -lturbojpeg\
				$(GMODULE_LIBS) \
				$(SYSTEMINFO_LIBS)\
				$(MEDIA_LIBS)

libmmutil_jpeg_la_CFLAGS += -DLIBPREFIX=\"$(libdir)\"

libmmutil_jpeg_la_DEPENDENCIES = $(srcdir)/../imgp/libmmutil_imgp.la

pcfiles = mmutil-jpeg.pc
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)
EXTRA_DIST = $(pcfiles)
SUBDIRS = . test
