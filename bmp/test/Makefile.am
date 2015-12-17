bin_PROGRAMS = mm_util_bmp_testsuite

mm_util_bmp_testsuite_SOURCES = mm_util_bmp_testsuite.c

mm_util_bmp_testsuite_CFLAGS = -I$(srcdir)/../include \
				-I$(srcdir)/../../imgp/include
mm_util_bmp_testsuite_CFLAGS += $(MEDIA_CFLAGS) \
				 -fPIE -pie \
				 $(TZ_PLATFORM_CONFIG_FLAGS)


############################################

mm_util_bmp_testsuite_DEPENDENCIES = $(srcdir)/../libmmutil_bmp.la

mm_util_bmp_testsuite_LDADD = $(srcdir)/../libmmutil_bmp.la
mm_util_bmp_testsuite_LDADD += $(MEDIA_LIBS) \
				$(TZ_PLATFORM_CONFIG_LIBS)

