bin_PROGRAMS = mm_util_imgcv_testsuite

mm_util_imgcv_testsuite_SOURCES = mm_util_imgcv_testsuite.c

mm_util_imgcv_testsuite_CFLAGS = -I$(srcdir)/../include \
								-I$(srcdir)/../../jpeg/include \
								-I$(srcdir)/../../imgp/include

mm_util_imgcv_testsuite_CFLAGS += $(MMCOMON_CFLAGS) \
								-fPIE -pie
############################################

mm_util_imgcv_testsuite_DEPENDENCIES = $(srcdir)/../libmmutil_imgcv.la
mm_util_imgcv_testsuite_DEPENDENCIES += $(srcdir)/../../jpeg/libmmutil_jpeg.la

mm_util_imgcv_testsuite_LDADD = $(srcdir)/../libmmutil_imgcv.la \
				  $(srcdir)/../../jpeg/libmmutil_jpeg.la \
				  $(MMCOMMON_LIBS)

