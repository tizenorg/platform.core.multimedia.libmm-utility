bin_PROGRAMS = mm_util_png_testsuite

mm_util_png_testsuite_SOURCES = mm_util_png_testsuite.c

mm_util_png_testsuite_CFLAGS = -I$(srcdir)/../include
mm_util_png_testsuite_CFLAGS += $(MMCOMMON_CFLAGS) \
				 $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x0100 -DMMF_DEBUG_PREFIX=\"MMF-IMAGE\" -fPIE -pie


############################################

mm_util_png_testsuite_DEPENDENCIES = $(srcdir)/../libmmutil_png.la

mm_util_png_testsuite_LDADD = $(srcdir)/../libmmutil_png.la
mm_util_png_testsuite_LDADD += $(MMCOMMON_LIBS)

