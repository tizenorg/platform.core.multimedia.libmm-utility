bin_PROGRAMS = mm_util_gif_testsuite

mm_util_gif_testsuite_SOURCES = mm_util_gif_testsuite.c

mm_util_gif_testsuite_CFLAGS = -I$(srcdir)/../include
mm_util_gif_testsuite_CFLAGS += $(MMCOMMON_CFLAGS) \
				 $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x0100 -DMMF_DEBUG_PREFIX=\"MMF-IMAGE\" -fPIE -pie


############################################

mm_util_gif_testsuite_DEPENDENCIES = $(srcdir)/../libmmutil_gif.la

mm_util_gif_testsuite_LDADD = $(srcdir)/../libmmutil_gif.la
mm_util_gif_testsuite_LDADD += $(MMCOMMON_LIBS)

