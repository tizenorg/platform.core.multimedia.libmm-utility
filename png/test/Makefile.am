bin_PROGRAMS = mm_util_png_testsuite

mm_util_png_testsuite_SOURCES = mm_util_png_testsuite.c

mm_util_png_testsuite_CFLAGS = -I$(srcdir)/../include \
				-I$(srcdir)/../../imgp/include
mm_util_png_testsuite_CFLAGS += $(MEDIA_CFLAGS) \
				 -fPIE -pie \
				 $(TZ_PLATFORM_CONFIG_FLAGS)


############################################

mm_util_png_testsuite_DEPENDENCIES = $(srcdir)/../libmmutil_png.la

mm_util_png_testsuite_LDADD = $(srcdir)/../libmmutil_png.la
mm_util_png_testsuite_LDADD += $(MEDIA_LIBS) \
				$(TZ_PLATFORM_CONFIG_LIBS)

