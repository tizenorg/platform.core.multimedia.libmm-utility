bin_PROGRAMS = mm_util_gif_testsuite

mm_util_gif_testsuite_SOURCES = mm_util_gif_testsuite.c

mm_util_gif_testsuite_CFLAGS = -I$(srcdir)/../include \
				-I$(srcdir)/../../imgp/include
mm_util_gif_testsuite_CFLAGS += $(MEDIA_CFLAGS) \
				 -fPIE -pie \
				 $(TZ_PLATFORM_CONFIG_FLAGS)


############################################

mm_util_gif_testsuite_DEPENDENCIES = $(srcdir)/../libmmutil_gif.la

mm_util_gif_testsuite_LDADD = $(srcdir)/../libmmutil_gif.la
mm_util_gif_testsuite_LDADD += $(MEDIA_LIBS) \
				$(TZ_PLATFORM_CONFIG_LIBS)

