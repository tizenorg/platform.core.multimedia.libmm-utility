ACLOCAL_AMFLAGS='-I m4'

lib_LTLIBRARIES = libmmutil_bmp.la

installmmutil_bmpdir = $(includedir)/mmf
installmmutil_bmp_HEADERS = include/mm_util_bmp.h

libmmutil_bmp_la_SOURCES = mm_util_bmp.c

libmmutil_bmp_la_CFLAGS = -I$(srcdir)/include \
				-I$(srcdir)/../imgp/include \
				$(GLIB_CFLAGS) \
				$(MEDIA_CFLAGS) \
				$(MMCOMMON_CFLAGS) \
				-I/usr/include \
				$(SYSTEMINFO_CFLAGS) \
				$(srcdir)/../imgp/libmmutil_imgp.la

libmmutil_bmp_la_LIBADD = $(MMCOMMON_LIBS) -lnsbmp -lbmp -ldl -lexif -lttrace\
				$(GLIB_LIBS) \
				$(GMODULE_LIBS) \
				$(SYSTEMINFO_LIBS)\
				$(MEDIA_LIBS)

libmmutil_bmp_la_CFLAGS += $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x0100 -DMMF_DEBUG_PREFIX=\"MMF-IMAGE\"

libmmutil_bmp_la_DEPENDENCIES = $(srcdir)/../imgp/libmmutil_imgp.la

pcfiles = mmutil-bmp.pc
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)
EXTRA_DIST = $(pcfiles)
SUBDIRS = . test
