bin_PROGRAMS = mm_util_jpeg_testsuite

mm_util_jpeg_testsuite_SOURCES = mm_util_jpeg_testsuite.c

mm_util_jpeg_testsuite_CFLAGS = -I$(srcdir)/../include \
				-I$(srcdir)/../../imgp/include
mm_util_jpeg_testsuite_CFLAGS += $(MEDIA_CFLAGS) \
				 -fPIE -pie \
				 $(TZ_PLATFORM_CONFIG_FLAGS)


############################################

mm_util_jpeg_testsuite_DEPENDENCIES = $(srcdir)/../libmmutil_jpeg.la

mm_util_jpeg_testsuite_LDADD = $(srcdir)/../libmmutil_jpeg.la
mm_util_jpeg_testsuite_LDADD += $(MEDIA_LIBS) \
				$(TZ_PLATFORM_CONFIG_LIBS)

