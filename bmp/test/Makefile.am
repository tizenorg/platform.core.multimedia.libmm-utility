bin_PROGRAMS = mm_util_bmp_testsuite

mm_util_bmp_testsuite_SOURCES = mm_util_bmp_testsuite.c

mm_util_bmp_testsuite_CFLAGS = -I$(srcdir)/../include
mm_util_bmp_testsuite_CFLAGS += $(MMCOMMON_CFLAGS) \
				 $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x0100 -DMMF_DEBUG_PREFIX=\"MMF-IMAGE\" -fPIE -pie


############################################

mm_util_bmp_testsuite_DEPENDENCIES = $(srcdir)/../libmmutil_bmp.la

mm_util_bmp_testsuite_LDADD = $(srcdir)/../libmmutil_bmp.la
mm_util_bmp_testsuite_LDADD += $(MMCOMMON_LIBS)

