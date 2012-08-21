ACLOCAL_AMFLAGS='-I m4'

lib_LTLIBRARIES = libmmutil_jpeg.la

installmmutil_jpegdir = $(includedir)/mmf
installmmutil_jpeg_HEADERS = include/mm_util_jpeg.h

libmmutil_jpeg_la_SOURCES = mm_util_jpeg.c
	
libmmutil_jpeg_la_CFLAGS = -I$(srcdir)/include \
			   $(MMTA_CFLAGS) \
			   $(MMCOMMON_CFLAGS)
			
libmmutil_jpeg_la_LIBADD = $(MMCOMMON_LIBS) -ljpeg -ldl \
			   $(MMTA_LIBS)  

libmmutil_jpeg_la_CFLAGS += $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x0100 -DMMF_DEBUG_PREFIX=\"MMF-IMAGE\"

pcfiles = mmutil-jpeg.pc
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)
EXTRA_DIST = $(pcfiles)
SUBDIRS = . test
