ACLOCAL_AMFLAGS='-I m4'

lib_LTLIBRARIES = libmmutil_imgcv.la

installmmutil_imgcvdir = $(includedir)/mmf
installmmutil_imgcv_HEADERS = include/mm_util_imgcv.h
noinst_HEADERS = include/mm_util_imgcv_internal.h

libmmutil_imgcv_la_SOURCES = mm_util_imgcv.c

libmmutil_imgcv_la_CFLAGS = -I$(srcdir)/include \
			   -I$(includedir)/drm \
			   $(MMCOMMON_CFLAGS) \
                           $(GLIB_CFLAGS) \
                           $(GMODULE_CFLAGS) \
						   $(OPENCV_CFLAGS) \
                           $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x0100 -DMMF_DEBUG_PREFIX=\"MMF-IMAGE\"

libmmutil_imgcv_la_LIBADD = $(MMCOMMON_LIBS) \
                           $(GLIB_LIBS) \
                           $(GMODULE_LIBS) \
						   $(OPENCV_LIBS) \
                           $(MMLOG_LIBS)

libmmutil_imgcv_la_CFLAGS += -lttrace $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x0100 -DMMF_DEBUG_PREFIX=\"MMF-IMAGE\" -DLIBPREFIX=\"$(libdir)\"

pcfiles = mmutil-imgcv.pc
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)
EXTRA_DIST = $(pcfiles)
#SUBDIRS = . test
