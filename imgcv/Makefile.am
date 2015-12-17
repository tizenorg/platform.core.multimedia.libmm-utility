ACLOCAL_AMFLAGS='-I m4'

lib_LTLIBRARIES = libmmutil_imgcv.la

installmmutil_imgcvdir = $(includedir)/mmf
installmmutil_imgcv_HEADERS = include/mm_util_imgcv.h
noinst_HEADERS = include/mm_util_imgcv_internal.h

libmmutil_imgcv_la_SOURCES = mm_util_imgcv.c

libmmutil_imgcv_la_CFLAGS = -I$(srcdir)/include \
				-I$(srcdir)/../imgp/include \
				$(GLIB_CFLAGS) \
				$(GMODULE_CFLAGS) \
				$(MEDIA_CFLAGS) \
				$(OPENCV_CFLAGS) \
				$(srcdir)/../imgp/libmmutil_imgp.la

libmmutil_imgcv_la_LIBADD = $(GLIB_LIBS) \
				$(GMODULE_LIBS) \
				$(MEDIA_LIBS) \
				$(OPENCV_LIBS)

libmmutil_imgcv_la_CFLAGS += -lttrace -DLIBPREFIX=\"$(libdir)\"

pcfiles = mmutil-imgcv.pc
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)
EXTRA_DIST = $(pcfiles)
SUBDIRS = . test
