/*
 * libmm-utility
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: YoungHun Kim <yh8004.kim@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *ranklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <mm_debug.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h> /* fopen() */
#include <jpeglib.h>
#include <mm_error.h>
#include <setjmp.h>
#include <glib.h>
#include <mm_attrs.h>
#include <mm_attrs_private.h>
#include <mm_ta.h>

#include "mm_util_jpeg.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#define MAX_SIZE	5000
#ifndef YUV420_SIZE
#define YUV420_SIZE(width, height)	(width*height*3>>1)
#endif
#ifndef YUV422_SIZE
#define YUV422_SIZE(width, height)	(width*height<<1)
#endif

#define HW_JPEG_DECODE 0
#define LIBJPEG 1
#define PARTIAL_DECODE 0
#define LIBJPEG_TURBO 0//libjpeg-turbo
#define YUV_DECODE 0
#define YUV_ENCODE 0
#if LIBJPEG_TURBO
#include <turbojpeg.h> //extern void jpeg_mem_src_tj(j_decompress_ptr, unsigned char *, unsigned long); //extern void jpeg_mem_dest_tj(j_compress_ptr, unsigned char **, unsigned long *, boolean);
#if YUV_DECODE
#define MM_LIBJPEG_TURBO_ROUND_UP_2(num)  (((num)+1)&~1)
#define MM_LIBJPEG_TURBO_ROUND_UP_4(num)  (((num)+3)&~3)
#define MM_LIBJPEG_TURBO_ROUND_UP_8(num)  (((num)+7)&~7)
#endif
#define PAD(v, p) ((v+(p)-1)&(~((p)-1)))
#define _throwtj() {printf("TurboJPEG ERROR:\n%s\n", tjGetErrorStr());}
#define _tj(f) {if((f)==-1) _throwtj();}

const char *subName[TJ_NUMSAMP]={"444", "422", "420", "GRAY", "440"};

/*
int exitStatus=0;
#define bailout() {exitStatus=-1;  goto bailout;}*/
#endif
#if HW_JPEG_DECODE
/*	HW JPEG DECODE	 */
typedef unsigned int	UINT32;
typedef unsigned char	UINT8;
typedef	unsigned int	UINT;
//////////////////////////////////////////////////////////////////////////////////////
// Definition																			//
//////////////////////////////////////////////////////////////////////////////////////
#define JPG_DRIVER_NAME		"/dev/s3c-jpg"

#define IOCTL_JPG_DECODE				0x00000006
#define IOCTL_JPG_ENCODE				0x00000007
#define IOCTL_JPG_GET_STRBUF			0x00000004
#define IOCTL_JPG_GET_FRMBUF			0x00000005
#define IOCTL_JPG_GET_THUMB_STRBUF		0x0000000A
#define IOCTL_JPG_GET_THUMB_FRMBUF		0x0000000B
#define IOCTL_JPG_GET_PHY_FRMBUF		0x0000000C
#define IOCTL_JPG_GET_PHY_THUMB_FRMBUF	0x0000000D

#define MAX_JPG_WIDTH				8192 //3072		
#define MAX_JPG_HEIGHT				8192 //2048         	
#define MAX_YUV_SIZE				(MAX_JPG_WIDTH * MAX_JPG_HEIGHT * 3)
#define	MAX_FILE_SIZE				(MAX_JPG_WIDTH * MAX_JPG_HEIGHT)
#define MAX_JPG_THUMBNAIL_WIDTH			320
#define MAX_JPG_THUMBNAIL_HEIGHT		240
#define MAX_YUV_THUMB_SIZE			(MAX_JPG_THUMBNAIL_WIDTH * MAX_JPG_THUMBNAIL_HEIGHT * 3)
#define	MAX_FILE_THUMB_SIZE			(MAX_JPG_THUMBNAIL_WIDTH * MAX_JPG_THUMBNAIL_HEIGHT)
#define EXIF_FILE_SIZE				28800

#define JPG_STREAM_BUF_SIZE			(MAX_JPG_WIDTH * MAX_JPG_HEIGHT)
#define JPG_STREAM_THUMB_BUF_SIZE	(MAX_JPG_THUMBNAIL_WIDTH * MAX_JPG_THUMBNAIL_HEIGHT)
#define JPG_FRAME_BUF_SIZE			(MAX_JPG_WIDTH * MAX_JPG_HEIGHT * 3)
#define JPG_FRAME_THUMB_BUF_SIZE	(MAX_JPG_THUMBNAIL_WIDTH * MAX_JPG_THUMBNAIL_HEIGHT * 3)

#define JPG_TOTAL_BUF_SIZE			(JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE \
			      + JPG_FRAME_BUF_SIZE + JPG_FRAME_THUMB_BUF_SIZE)

typedef enum {
	JPG_444,
	JPG_422,
	JPG_420,
	JPG_400,
	RESERVED1,
	RESERVED2,
	JPG_411,
	JPG_SAMPLE_UNKNOWN
} sample_mode_e;

typedef enum {
	JPG_QUALITY_LEVEL_1 = 0, /*high quality*/
	JPG_QUALITY_LEVEL_2,
	JPG_QUALITY_LEVEL_3,
	JPG_QUALITY_LEVEL_4     /*low quality*/
} image_quality_type_e;

typedef enum {
	JPEG_GET_DECODE_WIDTH,
	JPEG_GET_DECODE_HEIGHT,
	JPEG_SET_DECODE_OUT_FORMAT,
	JPEG_GET_SAMPING_MODE,
	JPEG_SET_ENCODE_WIDTH,
	JPEG_SET_ENCODE_HEIGHT,
	JPEG_SET_ENCODE_QUALITY,
	JPEG_SET_ENCODE_THUMBNAIL,
	JPEG_SET_ENCODE_IN_FORMAT,
	JPEG_SET_SAMPING_MODE,
	JPEG_SET_THUMBNAIL_WIDTH,
	JPEG_SET_THUMBNAIL_HEIGHT
} jpeg_conf_e;

typedef enum {
	JPEG_FAIL,
	JPEG_OK,
	JPEG_ENCODE_FAIL,
	JPEG_ENCODE_OK,
	JPEG_DECODE_FAIL,
	JPEG_DECODE_OK,
	JPEG_HEADER_PARSE_FAIL,
	JPEG_HEADER_PARSE_OK,
	JPEG_DISPLAY_FAIL,
	JPEG_OUT_OF_MEMORY,
	JPEG_UNKNOWN_ERROR
} jpeg_error_type_e;

typedef enum {
	YCBCR_422,
	YCBCR_420,
	YCBCR_SAMPLE_UNKNOWN
} jpeg_out_mode_e;

typedef enum {
	JPG_MODESEL_YCBCR = 1,
	JPG_MODESEL_RGB,
	JPG_MODESEL_UNKNOWN
} jpeg_in_mode_e;


typedef struct {
	char	make[32];
	char	Model[32];
	char	Version[32];
	char	DateTime[32];
	char	CopyRight[32];

	UINT	Height;
	UINT	Width;
	UINT	Orientation;
	UINT	ColorSpace;
	UINT	Process;
	UINT	Flash;

	UINT	FocalLengthNum;
	UINT	FocalLengthDen;

	UINT	ExposureTimeNum;
	UINT	ExposureTimeDen;

	UINT	FNumberNum;
	UINT	FNumberDen;

	UINT	ApertureFNumber;

	int		SubjectDistanceNum;
	int		SubjectDistanceDen;

	UINT	CCDWidth;

	int		ExposureBiasNum;
	int		ExposureBiasDen;


	int		WhiteBalance;

	UINT	MeteringMode;

	int		ExposureProgram;

	UINT	ISOSpeedRatings[2];

	UINT	FocalPlaneXResolutionNum;
	UINT	FocalPlaneXResolutionDen;

	UINT	FocalPlaneYResolutionNum;
	UINT	FocalPlaneYResolutionDen;

	UINT	FocalPlaneResolutionUnit;

	UINT	XResolutionNum;
	UINT	XResolutionDen;
	UINT	YResolutionNum;
	UINT	YResolutionDen;
	UINT	RUnit;

	int		BrightnessNum;
	int		BrightnessDen;

	char	UserComments[150];
} exif_file_info_s;

typedef enum {
	LOG_TRACE   = 0,
	LOG_WARNING = 1,
	LOG_ERROR   = 2
} log_level_e;

//////////////////////////////////////////////////////////////////////////////////////
// Type Define																		//
//////////////////////////////////////////////////////////////////////////////////////

typedef enum {
	JPG_MAIN,
	JPG_THUMBNAIL
} enc_dec_type_e;

typedef struct {
	sample_mode_e	sample_mode;
	enc_dec_type_e	dec_type;
	jpeg_out_mode_e	out_format;
	UINT32	width;
	UINT32	height;
	UINT32	data_size;
	UINT32	file_size;
} jpg_dec_proc_param_s;

typedef struct {
	sample_mode_e	sample_mode;
	enc_dec_type_e	enc_type;
	jpeg_in_mode_e      in_format;
	image_quality_type_e quality;
	UINT32	width;
	UINT32	height;
	UINT32	data_size;
	UINT32	file_size;
} jpg_enc_proc_param_s;

typedef struct {
	char			*in_buf;
	char			*phy_in_buf;
	int			in_buf_size;
	char			*out_buf;
	char			*phy_out_buf;
	int			out_buf_size;
	char			*in_thumb_buf;
	char			*phy_in_thumb_buf;
	int			in_thumb_buf_size;
	char			*out_thumb_buf;
	char			*phy_out_thumb_buf;
	int			out_thumb_buf_size;
	char			*mmapped_addr;
	jpg_dec_proc_param_s	*dec_param;
	jpg_enc_proc_param_s	*enc_param;
	jpg_enc_proc_param_s	*thumb_enc_param;
} jpg_args_s;


typedef struct _JPGLIB {
	int  jpg_fd;
	int  cmm_fd;
	jpg_args_s  args;
	UINT8	thumbnail_flag;
	exif_file_info_s *exif_info;
} jpg_lib_s;

/***************************************** decode config **************************************************/
#define TEST_DECODE					1  // enable decoder test
#define TEST_DECODE_OUTPUT_YCBYCR422			1  // output file format is YUV422(non-interleaved)
#define TEST_DECODE_OUTPUT_YCBYCR420			2  // output file format is YCBYCR(interleaved)

/***************************************** test config **************************************************/
#define TEST_DECODE_OUTPUT		TEST_DECODE_OUTPUT_YCBYCR422

//////////////////////////////////////////////////////////////////////////////////////
// Variables																			//
//////////////////////////////////////////////////////////////////////////////////////
jpg_lib_s  *jpg_ctx = NULL;
unsigned int fp_source  = -1;
#endif

/* OPEN SOURCE */
struct my_error_mgr_s
{
	struct jpeg_error_mgr pub;		/* "public" fields */
	jmp_buf setjmp_buffer;		/* for return to caller */
} my_error_mgr_s;

typedef struct my_error_mgr_s * my_error_ptr;

struct {
	struct jpeg_destination_mgr pub; /* public fields */

	FILE * outfile;		/* target stream */
	JOCTET * buffer;	/* start of buffer */
} my_destination_mgr_s;

typedef struct  my_destination_mgr_s * my_dest_ptr;

#define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */

/* Expanded data destination object for memory output */

struct {
	struct jpeg_destination_mgr pub; /* public fields */

	unsigned char ** outbuffer;   /* target buffer */
	unsigned long * outsize;
	unsigned char * newbuffer;    /* newly allocated buffer */
	JOCTET * buffer;      /* start of buffer */
	size_t bufsize;
} my_mem_destination_mgr;

typedef struct my_mem_destination_mgr * my_mem_dest_ptr;


/* 	OPEN SOURCE	*/
static void
my_error_exit (j_common_ptr cinfo)
{
	my_error_ptr myerr = (my_error_ptr) cinfo->err; //cinfo->err really points to a my_error_mgr_s struct, so coerce pointer
	(*cinfo->err->output_message) (cinfo); // Always display the message. We could postpone this until after returning, if we chose.
	longjmp(myerr->setjmp_buffer, 1); // Return control to the setjmp point 
}

static int
mm_image_encode_to_jpeg_file_with_libjpeg(char *pFileName, void *rawdata,  int width, int height, mm_util_jpeg_yuv_format fmt, int quality)
{
	int iErrorCode	= MM_ERROR_NONE;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	FILE * fpWriter;
	
	JSAMPROW y[16],cb[16],cr[16]; // y[2][5] = color sample of row 2 and pixel column 5; (one plane) 
	JSAMPARRAY data[3]; // t[0][2][5] = color sample 0 of row 2 and column 5 

	int i,j;
	data[0] = y;
	data[1] = cb;
	data[2] = cr;

	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] Enter", __func__, __LINE__);

	cinfo.err = jpeg_std_error(&jerr);

	jpeg_create_compress(&cinfo);

	if ((fpWriter = fopen(pFileName, "wb")) == NULL) {
		mmf_debug(MMF_DEBUG_ERROR,"Exit on error", __func__, __LINE__);
		return MM_ERROR_IMAGE_FILEOPEN;
	}

	 if ( fmt == MM_UTIL_JPEG_FMT_RGB888) {
		jpeg_stdio_dest(&cinfo, fpWriter);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_stdio_dest for MM_UTIL_JPEG_FMT_RGB888", __func__, __LINE__);
	}

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;

	if(fmt ==MM_UTIL_JPEG_FMT_YUV420) {
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] MM_UTIL_JPEG_FMT_YUV420", __func__, __LINE__);
		cinfo.in_color_space = JCS_YCbCr;
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] JCS_YCbCr", __func__, __LINE__);

		jpeg_set_defaults(&cinfo); 
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_set_defaults", __func__, __LINE__);

		cinfo.raw_data_in = TRUE; // Supply downsampled data 
		cinfo.do_fancy_downsampling = FALSE;

		cinfo.comp_info[0].h_samp_factor = 2;
		cinfo.comp_info[0].v_samp_factor = 2;
		cinfo.comp_info[1].h_samp_factor = 1;
		cinfo.comp_info[1].v_samp_factor = 1;
		cinfo.comp_info[2].h_samp_factor = 1;
		cinfo.comp_info[2].v_samp_factor = 1;

		jpeg_set_quality(&cinfo, quality, TRUE);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_set_quality", __func__, __LINE__);
		cinfo.dct_method = JDCT_FASTEST;

		jpeg_stdio_dest(&cinfo, fpWriter); //jpeg_mem_dest(&cinfo, mem, csize);
		
		jpeg_start_compress(&cinfo, TRUE);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_start_compress", __func__, __LINE__);

		for (j = 0; j < height; j += 16) {
			for (i = 0; i < 16; i++) {
				y[i] = (char*)rawdata + width * (i + j);
				if (i % 2 == 0) {
					cb[i / 2] = (char*)rawdata + width * height + width / 2 * ((i + j) / 2);
					cr[i / 2] = (char*)rawdata + width * height + width * height / 4 + width / 2 * ((i + j) / 2);
				}
			}
			jpeg_write_raw_data(&cinfo, data, 16);
		}
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] #for loop#", __func__, __LINE__);

		jpeg_finish_compress(&cinfo);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_finish_compress", __func__, __LINE__);

		jpeg_destroy_compress(&cinfo);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_destroy_compress", __func__, __LINE__);
	}else if ( fmt == MM_UTIL_JPEG_FMT_RGB888) {
		JSAMPROW row_pointer[1];
		int iRowStride;

		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] MM_UTIL_JPEG_FMT_RGB888", __func__, __LINE__);

		cinfo.in_color_space = JCS_RGB;
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] JCS_RGB");

		jpeg_set_defaults(&cinfo);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_set_defaults", __func__, __LINE__);
		jpeg_set_quality(&cinfo, quality, TRUE);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_set_quality", __func__, __LINE__);
		jpeg_start_compress(&cinfo, TRUE);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_start_compress", __func__, __LINE__);
		iRowStride = width * 3;

		while (cinfo.next_scanline < cinfo.image_height) {
			 row_pointer[0] = &rawdata[cinfo.next_scanline * iRowStride];
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] while", __func__, __LINE__);
		jpeg_finish_compress(&cinfo);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_finish_compress", __func__, __LINE__);

		jpeg_destroy_compress(&cinfo);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_destroy_compress", __func__, __LINE__);
	}else {
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] We can just encode the MM_UTIL_JPEG_FMT_YUV420", __func__, __LINE__);
		return MM_ERROR_IMAGE_NOT_SUPPORT_FORMAT;
	}

	fclose(fpWriter);
	return iErrorCode;
}

static int  
mm_image_encode_to_jpeg_memory_with_libjpeg(void **mem, int *csize, void *rawdata,  int width, int height, mm_util_jpeg_yuv_format fmt,int quality)
{
	int iErrorCode	= MM_ERROR_NONE;

	JSAMPROW y[16],cb[16],cr[16]; // y[2][5] = color sample of row 2 and pixel column 5; (one plane)
	JSAMPARRAY data[3]; // t[0][2][5] = color sample 0 of row 2 and column 5

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	 int i, j;
	*csize = 0;
	data[0] = y;
	data[1] = cb;
	data[2] = cr;

	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] Enter", __func__, __LINE__);
	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] #Before Enter#, mem: %p\t rawdata:%p\t width: %d\t height: %d\t fmt: %d\t quality: %d", __func__, __LINE__
		, mem, rawdata, width, height, fmt, quality);

	if(rawdata == NULL) {
		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] Exit on error -rawdata", __func__, __LINE__);
		return MM_ERROR_IMAGE_FILEOPEN;
	}

	if (mem == NULL) {
		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] Exit on error -mem", __func__, __LINE__);
		return MM_ERROR_IMAGE_FILEOPEN;
	}

	cinfo.err = jpeg_std_error(&jerr); // Errors get written to stderr

	jpeg_create_compress(&cinfo);

	mmf_debug(MMF_DEBUG_LOG,"[mm_image_encode_to_jpeg_memory_with_libjpeg] #After jpeg_mem_dest#, mem: %p\t width: %d\t height: %d\t fmt: %d\t quality: %d"
		, mem, width, height, fmt, quality);

	if ( fmt == MM_UTIL_JPEG_FMT_RGB888) {
		jpeg_mem_dest(&cinfo, (unsigned char **)mem, (unsigned long *)csize);
		mmf_debug(MMF_DEBUG_LOG,"[mm_image_encode_to_jpeg_file_with_libjpeg] jpeg_stdio_dest for MM_UTIL_JPEG_FMT_RGB888");
	}

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	if(fmt ==MM_UTIL_JPEG_FMT_YUV420) {
		cinfo.in_color_space = JCS_YCbCr;
		jpeg_set_defaults(&cinfo);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_set_defaults", __func__, __LINE__);

		cinfo.raw_data_in = TRUE; // Supply downsampled data 
		cinfo.do_fancy_downsampling = FALSE;

		cinfo.comp_info[0].h_samp_factor = 2;
		cinfo.comp_info[0].v_samp_factor = 2;
		cinfo.comp_info[1].h_samp_factor = 1;
		cinfo.comp_info[1].v_samp_factor = 1;
		cinfo.comp_info[2].h_samp_factor = 1;
		cinfo.comp_info[2].v_samp_factor = 1;

		jpeg_set_quality(&cinfo, quality, TRUE);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_set_quality", __func__, __LINE__);
		cinfo.dct_method = JDCT_FASTEST; 

		jpeg_mem_dest(&cinfo, (unsigned char **)mem, (unsigned long *)csize);
		
		jpeg_start_compress(&cinfo, TRUE);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_start_compress", __func__, __LINE__);

		for (j = 0; j < height; j += 16) {
			for (i = 0; i < 16; i++) {
				y[i] = (char*)rawdata + width * (i + j);
				if (i % 2 == 0) {
					cb[i / 2] = (char*)rawdata + width * height + width / 2 * ((i + j) / 2);
					cr[i / 2] = (char*)rawdata + width * height + width * height / 4 + width / 2 * ((i + j) / 2);
				}
			}
			jpeg_write_raw_data(&cinfo, data, 16);
		}
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] #for loop#", __func__, __LINE__);

		jpeg_finish_compress(&cinfo);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_finish_compress", __func__, __LINE__);
		jpeg_destroy_compress(&cinfo);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] Exit jpeg_destroy_compress, mem: %p\t size:%d", __func__, __LINE__, *mem, *csize);
	}

	else if ( fmt == MM_UTIL_JPEG_FMT_RGB888) {
		JSAMPROW row_pointer[1];
		int iRowStride;

		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] MM_UTIL_JPEG_FMT_RGB888", __func__, __LINE__);

		cinfo.in_color_space = JCS_RGB;
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] JCS_RGB", __func__, __LINE__);

		jpeg_set_defaults(&cinfo);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_set_defaults", __func__, __LINE__);
		jpeg_set_quality(&cinfo, quality, TRUE);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_set_quality", __func__, __LINE__);
		jpeg_start_compress(&cinfo, TRUE);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_start_compress", __func__, __LINE__);
		iRowStride = width * 3;

		while (cinfo.next_scanline < cinfo.image_height) {
			 row_pointer[0] = &rawdata[cinfo.next_scanline * iRowStride];
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] while", __func__, __LINE__);
		jpeg_finish_compress(&cinfo);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] jpeg_finish_compress", __func__, __LINE__);
		
		jpeg_destroy_compress(&cinfo);

		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] Exit", __func__, __LINE__);
	}else {
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] We can just encode the MM_UTIL_JPEG_FMT_YUV420", __func__, __LINE__);
		return MM_ERROR_IMAGE_NOT_SUPPORT_FORMAT;
	}

	return iErrorCode;
}

#if HW_JPEG_DECODE
/*----------------------------------------------------------------------------
*Function: initDecodeParam

*Parameters:
*Return Value:
*Implementation Notes: Initialize JPEG Decoder Default value
-----------------------------------------------------------------------------*/
static void 
mm_hw_decode_init_decode_param(void)
{
	jpg_ctx = (jpg_lib_s *)malloc(sizeof(jpg_lib_s));
	memset(jpg_ctx, 0x00, sizeof(jpg_lib_s));

	jpg_ctx->args.dec_param = (jpg_dec_proc_param_s *)malloc(sizeof(jpg_dec_proc_param_s));
	memset(jpg_ctx->args.dec_param, 0x00, sizeof(jpg_dec_proc_param_s));

	jpg_ctx->args.dec_param->dec_type = JPG_MAIN;
}

/*----------------------------------------------------------------------------
*Function: SsbSipJPEGDecodeInit

*Parameters:
*Return Value:		Decoder Init handle
*Implementation Notes: Initialize JPEG Decoder Deivce Driver
-----------------------------------------------------------------------------*/
static int 
mm_hw_decode_SsbSipJPEGDecodeInit(void)
{
	mm_hw_decode_init_decode_param();
	jpg_ctx->jpg_fd = open(JPG_DRIVER_NAME, O_RDWR /*| O_NONBLOCK O_NDELAY*/); //block mode
	//jpg_ctx->jpg_fd = open(JPG_DRIVER_NAME, O_RDWR | O_NDELAY);			//non block mode
	mmf_debug(MMF_DEBUG_LOG, "[SsbSipJPEGDecodeInit] jpg_ctx->jpg_fd: %d", jpg_ctx->jpg_fd);
	if (jpg_ctx->jpg_fd < 0) {
		mmf_debug(MMF_DEBUG_ERROR, "SsbSipJPEGDecodeInit JPEG Driver open failed"); //log_msg(LOG_ERROR, "SsbSipJPEGDecodeInit", "JPEG Driver open failed");
		return MM_ERROR_IMAGE_FILEOPEN;
	}

	jpg_ctx->args.mmapped_addr = (char *) mmap(0, 
			JPG_TOTAL_BUF_SIZE, 
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			jpg_ctx->jpg_fd,
			0
			);
	if (jpg_ctx->args.mmapped_addr == NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "SsbSipJPEGDecodeInit JPEG mmap failed");//log_msg(LOG_ERROR, "SsbSipJPEGDecodeInit", "JPEG mmap failed");
		return MM_ERROR_IMAGE_INTERNAL;
	}

	mmf_debug(MMF_DEBUG_LOG, "SsbSipJPEGDecodeInit jpg_ctx->args.out_buf  0x%x, jpg_ctx->args.phy_out_buf : 0x%x", jpg_ctx->args.out_buf , jpg_ctx->args.phy_out_buf); //log_msg(LOG_TRACE, "SsbSipJPEGDecodeInit", "jpg_ctx->args.out_buf  0x%x, jpg_ctx->args.phy_out_buf : 0x%x", jpg_ctx->args.out_buf , jpg_ctx->args.phy_out_buf);
	mmf_debug(MMF_DEBUG_LOG, "SsbSipJPEGDecodeInit init decode handle : %d", jpg_ctx->jpg_fd); //log_msg(LOG_TRACE, "SsbSipJPEGDecodeInit", "init decode handle : %d", jpg_ctx->jpg_fd);

	return jpg_ctx->jpg_fd;
}

/*----------------------------------------------------------------------------
*Function: SsbSipJPEGGetDecodeInBuf

*Parameters: 		*openHandle : openhandle from SsbSipJPEGDecodeInit
					size : input stream(YUV/RGB) size
*Return Value:		virtual address of Decoder input buffer
*Implementation Notes: allocate decoder input buffer
-----------------------------------------------------------------------------*/
static void *
mm_hw_decode_SsbSipJPEGGetDecodeInBuf(long size)
{
	if (size < 0 || size > MAX_FILE_SIZE) {
		mmf_debug(MMF_DEBUG_ERROR, "SsbSipJPEGGetDecodeInBuf Invalid Decoder input buffer size\r");
		return NULL;
	}

	jpg_ctx->args.dec_param->file_size= size;
	jpg_ctx->args.in_buf = (char *)ioctl(jpg_ctx->jpg_fd, IOCTL_JPG_GET_STRBUF, jpg_ctx->args.mmapped_addr);

	return (char *)(jpg_ctx->args.in_buf);
}


/*----------------------------------------------------------------------------
*Function: SsbSipJPEGDecodeDeInit

*Parameters: 		*openHandle : openhandle from SsbSipJPEGDecodeInit
*Return Value:		JPEG_ERRORTYPE
*Implementation Notes: Deinitialize JPEG Decoder Device Driver
-----------------------------------------------------------------------------*/
static jpeg_error_type_e
mm_hw_decode_SsbSipJPEGDecodeDeInit(int dev_fd)
{
	mmf_debug(MMF_DEBUG_LOG, "[SsbSipJPEGGetDecodeInBuf] SsbSipJPEGDecodeDeInit dev: %d", dev_fd);

	munmap(jpg_ctx->args.mmapped_addr, JPG_TOTAL_BUF_SIZE);

	close(jpg_ctx->jpg_fd);
	close(jpg_ctx->cmm_fd);


	if (jpg_ctx->args.dec_param != NULL)
		free(jpg_ctx->args.dec_param);


	free(jpg_ctx);

	return JPEG_OK;
}

/*----------------------------------------------------------------------------
*Function: SsbSipJPEGSetConfig

*Parameters: 		type : config type to set
					*value : config value to set
*Return Value:		JPEG_ERRORTYPE
*Implementation Notes: set JPEG config value from user
-----------------------------------------------------------------------------*/
static jpeg_error_type_e
mm_hw_decode_SsbSipJPEGSetConfig(jpeg_conf_e type, INT32 value)
{
	jpeg_error_type_e result = JPEG_OK;

	mmf_debug(MMF_DEBUG_LOG, "[SsbSipJPEGSetConfig] SsbSipJPEGSetConfig SsbSipJPEGSetConfig(%d : %d)", type, value);

	switch (type) {
	case JPEG_SET_DECODE_OUT_FORMAT: {
		if (value != YCBCR_420 && value != YCBCR_422) {
			mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGSetConfig] SsbSipJPEGSetConfig API :: Invalid decode output format\r");
			result = JPEG_FAIL;
		} else {
			jpg_ctx->args.dec_param->out_format = value;
		}

		break;
	}
	case JPEG_SET_ENCODE_WIDTH: {
		if (value < 0 || value > MAX_JPG_WIDTH) {
			mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGSetConfig] SsbSipJPEGSetConfig Invalid width\r");
			result = JPEG_FAIL;
		} else
			jpg_ctx->args.enc_param->width = value;

		break;
	}
	case JPEG_SET_ENCODE_HEIGHT: {
		if (value < 0 || value > MAX_JPG_HEIGHT) {
			mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGSetConfig] SsbSipJPEGSetConfig Invalid height");
			result = JPEG_FAIL;
		} else
			jpg_ctx->args.enc_param->height = value;

		break;
	}
	case JPEG_SET_ENCODE_QUALITY: {
		if (value < JPG_QUALITY_LEVEL_1 || value > JPG_QUALITY_LEVEL_4) {
			mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGSetConfig] SsbSipJPEGSetConfig Invalid Quality value");
			result = JPEG_FAIL;
		} else
			jpg_ctx->args.enc_param->quality = value;

		break;
	}
	case JPEG_SET_ENCODE_THUMBNAIL: {
		if (value != TRUE && value != FALSE) {
			mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGSetConfig] SsbSipJPEGSetConfig Invalid thumbnail_flag");
			result = JPEG_FAIL;
		} else
			jpg_ctx->thumbnail_flag = value;

		break;
	}
	case JPEG_SET_ENCODE_IN_FORMAT: {
		if (value != JPG_MODESEL_YCBCR && value != JPG_MODESEL_RGB) {
			mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGSetConfig] SsbSipJPEGSetConfig Invalid informat");
			result = JPEG_FAIL;
		} else {
			jpg_ctx->args.enc_param->in_format = value;
			jpg_ctx->args.thumb_enc_param->in_format = value;
		}

		break;
	}
	case JPEG_SET_SAMPING_MODE: {
		if (value != JPG_420 && value != JPG_422) {
			mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGSetConfig] SsbSipJPEGSetConfig Invalid sample_mode\r");
			result = JPEG_FAIL;
		} else {
			jpg_ctx->args.enc_param->sample_mode = value;
			jpg_ctx->args.thumb_enc_param->sample_mode = value;
		}

		break;
	}
	case JPEG_SET_THUMBNAIL_WIDTH: {
		if (value < 0 || value > MAX_JPG_THUMBNAIL_WIDTH) {
			mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGSetConfig] SsbSipJPEGSetConfig Invalid thumbWidth\r");
			result = JPEG_FAIL;
		} else
			jpg_ctx->args.thumb_enc_param->width = value;

		break;
	}
	case JPEG_SET_THUMBNAIL_HEIGHT: {
		if (value < 0 || value > MAX_JPG_THUMBNAIL_HEIGHT) {
			mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGSetConfig] SsbSipJPEGSetConfig Invalid thumbHeight\r");
			result = JPEG_FAIL;
		} else
			jpg_ctx->args.thumb_enc_param->height = value;

		break;
	}
	//for the future work....
	default :
		mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGSetConfig] SsbSipJPEGSetConfig Invalid Config type");
		result = JPEG_FAIL;
	}

	return result;
}


/*----------------------------------------------------------------------------
*Function: SsbSipJPEGDecodeExe

*Parameters: 		*openHandle	: openhandle from SsbSipJPEGDecodeInit
*Return Value:		JPEG_ERRORTYPE
*Implementation Notes: Decode JPEG file
-----------------------------------------------------------------------------*/
static jpeg_error_type_e
mm_hw_decode_SsbSipJPEGDecodeExe(void)
{
	jpg_args_s *pArgs;
	int ret;
	mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGDecodeExe] SsbSipJPEGDecodeExe jpg_ctx->args.dec_param->file_size: %d", jpg_ctx->args.dec_param->file_size);
	pArgs = &(jpg_ctx->args);

	if (jpg_ctx->args.dec_param->dec_type == JPG_MAIN) {
		mmf_debug(MMF_DEBUG_LOG, "[SsbSipJPEGDecodeExe] # before ioctl #jpg_ctx->args.dec_param->dec_type == JPG_MAIN jpg_ctx->jpg_fd:%d", jpg_ctx->jpg_fd);
		ret = ioctl(jpg_ctx->jpg_fd, IOCTL_JPG_DECODE, pArgs);
		mmf_debug(MMF_DEBUG_LOG, "[SsbSipJPEGDecodeExe] # after ioctl #jpg_ctx->args.dec_param->dec_type == JPG_MAIN jpg_ctx->jpg_fd:%d", jpg_ctx->jpg_fd);

		mmf_debug(MMF_DEBUG_LOG, "[SsbSipJPEGDecodeExe] jpg_ctx->args->in_buf:%p  pArgs->in_buf:%p", jpg_ctx->args.in_buf, pArgs->in_buf);
		
		mmf_debug(MMF_DEBUG_LOG, "[SsbSipJPEGDecodeExe] SsbSipJPEGDecodeExe dec_param->width : %d dec_param->height : %d", jpg_ctx->args.dec_param->width, jpg_ctx->args.dec_param->height);
		mmf_debug(MMF_DEBUG_LOG, "[SsbSipJPEGDecodeExe] SsbSipJPEGDecodeExe streamSize : %d", jpg_ctx->args.dec_param->data_size);
	} else {
		// thumbnail decode, for the future work.
		mmf_debug(MMF_DEBUG_LOG, "[SsbSipJPEGDecodeExe] else");
	}

	if( ret < 0 ) {
		mmf_debug( MMF_DEBUG_ERROR, "error code : %x", ret );
		return JPEG_FAIL;
	}else {
		return JPEG_OK;
	}
}


/*----------------------------------------------------------------------------
*Function: SsbSipJPEGGetDecodeOutBuf

*Parameters: 		*openHandle : openhandle from SsbSipJPEGDecodeInit
					size : output JPEG file size
*Return Value:		virtual address of Decoder output buffer
*Implementation Notes: return decoder output buffer
-----------------------------------------------------------------------------*/
static void *
mm_hw_decode_SsbSipJPEGGetDecodeOutBuf( long *size)
{

	jpg_ctx->args.out_buf = (char *)ioctl(jpg_ctx->jpg_fd, IOCTL_JPG_GET_FRMBUF, jpg_ctx->args.mmapped_addr);
	mmf_debug(MMF_DEBUG_LOG, "[SsbSipJPEGGetDecodeOutBuf] jpg_ctx->jpg_fd: %d, jpg_ctx->args.out_buf:%p", jpg_ctx->jpg_fd, jpg_ctx->args.out_buf);
	*size = jpg_ctx->args.dec_param->data_size;

	mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGGetDecodeOutBuf] DecodeOutBuf : 0x%x  size : %d", jpg_ctx->args.out_buf, jpg_ctx->args.dec_param->data_size);


	return (void *)(jpg_ctx->args.out_buf);
}

/*----------------------------------------------------------------------------
*Function: SsbSipJPEGGetConfig

*Parameters: 		type : config type to get
					*value : config value to get
*Return Value:		JPEG_ERRORTYPE
*Implementation Notes: return JPEG config value to user
-----------------------------------------------------------------------------*/
static jpeg_error_type_e
mm_hw_decode_SsbSipJPEGGetConfig(jpeg_conf_e type, INT32 *value)
{
	jpeg_error_type_e result = JPEG_OK;
	mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGGetConfig] Config type %d, value : %d",type,*value);
	switch (type) {
	case JPEG_GET_DECODE_WIDTH:
		*value = (INT32)jpg_ctx->args.dec_param->width;
		break;
	case JPEG_GET_DECODE_HEIGHT:
		*value = (INT32)jpg_ctx->args.dec_param->height;
		break;
	case JPEG_GET_SAMPING_MODE:
		*value = (INT32)jpg_ctx->args.dec_param->sample_mode;
		break;
	default :
		mmf_debug(MMF_DEBUG_ERROR, "[SsbSipJPEGGetConfig] Invalid Config type");
		result = JPEG_FAIL;
	}

	return result;
}

/*
*******************************************************************************
Name            : DecodeFileOutYUV422
Description     : To change YCBYCR to YUV422, and write result file.
Parameter       :
Return Value    : void
*******************************************************************************
*/
static void 
mm_hw_decode_DecodeFileOutYUV422(char *out_buf, UINT32 streamSize, char *filename)
{
	UINT32	i;
	UINT32  indexY, indexCB, indexCR;
	char *Buf;
	FILE *fp;



	Buf = (char *)malloc(MAX_YUV_SIZE);
	memset(Buf, 0x00, MAX_YUV_SIZE);

	indexY = 0;
	indexCB = streamSize >> 1;
	indexCR = indexCB + (streamSize >> 2);

	//printf("indexY(%ld), indexCB(%ld), indexCR(%ld)", indexY, indexCB, indexCR);
	log_msg(LOG_TRACE, "DecodeFileOutYUV422", "out_buf(addr : 0x%x  size : %d)", Buf, streamSize);
	for (i = 0; i < streamSize; i++) {
		if ((i % 2) == 0) {
			Buf[indexY++] = out_buf[i];
		}

		if ((i % 4) == 1) {
			Buf[indexCB++] = out_buf[i];
		}

		if ((i % 4) == 3) {
			Buf[indexCR++] = out_buf[i];
		}
	}

	fp = fopen(filename, "wb");
	fwrite(Buf, 1, streamSize, fp);
	fclose(fp);
	free(Buf);

}

static void 
mm_hw_decode_DecodeFileOutYCBYCR(char *out_buf, UINT32 streamSize, char *filename)
{
	FILE *fp;

	fp = fopen(filename, "wb");
	fwrite(out_buf, 1, streamSize, fp);
	fclose(fp);

}

/*
*******************************************************************************
Name            : DecodeFileOutYUV420
Description     : To change YCBYCR420 2 planer to YCBYCR420 3 planer, and write result file.
Parameter       :
Return Value    : void
*******************************************************************************
*/
static void
mm_hw_decode_DecodeFileOutYUV420(char *out_buf, UINT32 streamSize, char *filename)
{
	UINT32	i;
	UINT32  indexY, indexCB, indexCR;
	char *Buf;
	FILE *fp;



	Buf = (char *)malloc(MAX_YUV_SIZE);
	memset(Buf, 0x00, MAX_YUV_SIZE);

	indexY = 0;
	indexCB = (streamSize * 2) / 3 ;
	indexCR = indexCB + (streamSize - indexCB >> 1);

	//printf("indexY(%ld), indexCB(%ld), indexCR(%ld)", indexY, indexCB, indexCR);
	mmf_debug(MMF_DEBUG_ERROR, "[DecodeFileOutYUV420] indexY(%ld), indexCB(%ld), indexCR(%ld)", indexY, indexCB, indexCR);

	for ( i =0; i < indexCB -1; i++)
		Buf[indexY++] = out_buf[i];

	for (i = indexCB; i < streamSize;) {
			Buf[indexCB++] = out_buf[i++];
			Buf[indexCR++] = out_buf[i++];
	}

	fp = fopen(filename, "wb");
	fwrite(Buf, 1, streamSize, fp);
	fclose(fp);
	free(Buf);

}

static int 
mm_image_decode_from_jpeg_file_with_c110hw(mm_util_jpeg_yuv_data * decoded_data, char *pFileName, mm_util_jpeg_yuv_format input_fmt)
{
	int iErrorCode	= MM_ERROR_NONE;
	char 	*in_buf = NULL; 	//char 	*out_buf = NULL;
	FILE 	*fp;
	FILE 	*CTRfp;
	UINT32 	file_size;
	long 	streamSize;
	int		handle;
	INT32 	width, height, samplemode;
	jpeg_error_type_e ret;	//char 	*outFilename = "hw_jpeg.jpg";
	boolean	result = TRUE;
	int	dec_out_mode;
	mmf_debug(MMF_DEBUG_LOG, "[mm_image_decode_from_jpeg_file_with_c110hw] ------------------------Decoder Test Start ---------------------");
	//////////////////////////////////////////////////////////////
	// 0. Get input/output file name                            					//
	//////////////////////////////////////////////////////////////

	CTRfp = fopen(pFileName, "rb");
	
	if (CTRfp == NULL) {
		return MM_ERROR_INVALID_ARGUMENT;
	}
	/*mkdir("./testVectors/testOut", 0644);
	mmf_debug(MMF_DEBUG_ERROR, "[mm_image_decode_from_jpeg_file_with_c110hw] mkdir");*/

	//////////////////////////////////////////////////////////////
	// 1. handle Init                                         							//
	//////////////////////////////////////////////////////////////

	handle = mm_hw_decode_SsbSipJPEGDecodeInit();
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] SsbSipJPEGDecodeInit handle: %d", __func__, __LINE__, handle);
	if (handle < 0) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s] [%05d] #SsbSipJPEGDecodeInit ERROR#", __func__, __LINE__);
	}
	jpg_ctx->cmm_fd = handle;

	//////////////////////////////////////////////////////////////
	// 2. open JPEG file to decode                             						//
	//////////////////////////////////////////////////////////////
	fp = fopen(pFileName, "rb");

	if (fp == NULL) {
		result = FALSE;
		mmf_debug(MMF_DEBUG_ERROR,"[%s] [%05d] file open error : %s", __func__, __LINE__, pFileName);
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	mmf_debug(MMF_DEBUG_LOG,"filesize : %d", file_size);
	//////////////////////////////////////////////////////////////
	// 3. get Input buffer address                            					//
	//////////////////////////////////////////////////////////////
	in_buf = mm_hw_decode_SsbSipJPEGGetDecodeInBuf(file_size);

	if (in_buf == NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s] [%05d] Input buffer is NULL", __func__, __LINE__);
		result = FALSE;
	}

	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] inBuf : %p", __func__, __LINE__, in_buf);

	//////////////////////////////////////////////////////////////
	// 4. put JPEG frame to Input buffer                        					//
	//////////////////////////////////////////////////////////////
	mmf_debug(MMF_DEBUG_LOG, "fp: %p", fp);
	fread((char*)in_buf, 1, file_size, fp); //memcpy(in_buf, fp, file_size); //
	fclose(fp);
	mmf_debug(MMF_DEBUG_LOG, "fread");
	//////////////////////////////////////////////////////////////
	// 5. set decode config.                                   						//
	//////////////////////////////////////////////////////////////
	if(input_fmt == MM_UTIL_JPEG_FMT_YUV420)
		dec_out_mode = YCBCR_420;

	if ((ret = mm_hw_decode_SsbSipJPEGSetConfig(JPEG_SET_DECODE_OUT_FORMAT, dec_out_mode)) != JPEG_OK) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s] [%05d] SsbSipJPEGSetConfig ERROR", __func__, __LINE__);
		result = FALSE;
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] SsbSipJPEGSetConfig", __func__, __LINE__);
	//////////////////////////////////////////////////////////////
	// 6. Decode JPEG frame                                   					//
	//////////////////////////////////////////////////////////////

	ret = mm_hw_decode_SsbSipJPEGDecodeExe(); // ????????????????????????????????????????????????????????????????????????????????????????????????????????
	mmf_debug(MMF_DEBUG_LOG, "[mm_image_decode_from_jpeg_file_with_c110hw] SsbSipJPEGDecodeExe, ret:%d", __func__, __LINE__, ret);

	if (ret != JPEG_OK) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s] [%05d]SsbSipJPEGDecodeExe ERROR", __func__, __LINE__);
		result = FALSE;
	}

	//////////////////////////////////////////////////////////////
	// 7. get Output buffer address                            					//
	//////////////////////////////////////////////////////////////
	decoded_data->data = mm_hw_decode_SsbSipJPEGGetDecodeOutBuf(&streamSize);

	if (decoded_data->data == NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s] [%05d] Output buffer is NULL", __func__, __LINE__);
		result = FALSE;
	}

	mmf_debug(MMF_DEBUG_LOG,"[%s] [%05d] out_buf : 0x%x streamsize : %d", __func__, __LINE__, decoded_data->data, streamSize);

	//////////////////////////////////////////////////////////////
	// 8.  get decode config.                                    					//
	//////////////////////////////////////////////////////////////
	mm_hw_decode_SsbSipJPEGGetConfig(JPEG_GET_DECODE_WIDTH, &width);
	mm_hw_decode_SsbSipJPEGGetConfig(JPEG_GET_DECODE_HEIGHT, &height);
	mm_hw_decode_SsbSipJPEGGetConfig(JPEG_GET_SAMPING_MODE, &samplemode);

	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] width : %d height : %d samplemode : %d", __func__, __LINE__, width, height, samplemode);

	decoded_data->width = width;
	decoded_data->height= height;
	//////////////////////////////////////////////////////////////
	// 9. wirte output file & dispaly to LCD                    					//
	//////////////////////////////////////////////////////////////
	
	/*
	if(input_fmt == MM_UTIL_JPEG_FMT_YUV420)
	mm_hw_decode_DecodeFileOutYUV420(decoded_data->data, streamSize, outFilename);  // YCBYCR420 interleaved 2 plane
	else if(input_fmt == MM_UTIL_JPEG_FMT_YUV422)
	mm_hw_decode_DecodeFileOutYCBYCR(decoded_data->data, streamSize, outFilename); // YCBYCR422 interleaved	
	*/
	//////////////////////////////////////////////////////////////
	// 10. finalize handle                                      						//
	//////////////////////////////////////////////////////////////
	mm_hw_decode_SsbSipJPEGDecodeDeInit(handle);

	fclose(CTRfp);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] ------------------------Decoder Test Done ---------------------", __func__, __LINE__);

	return iErrorCode;

}
#endif

#if LIBJPEG_TURBO
void initBuf(unsigned char *buf, int w, int h, int pf, int flags)
{
	int roffset=tjRedOffset[pf];
	int goffset=tjGreenOffset[pf];
	int boffset=tjBlueOffset[pf];
	int ps=tjPixelSize[pf];
	int index, row, col, halfway=16;

	memset(buf, 0, w*h*ps);
	if(pf==TJPF_GRAY) {
		for(row=0; row<h; row++) {
			for(col=0; col<w; col++) {
				if(flags&TJFLAG_BOTTOMUP) index=(h-row-1)*w+col;
				else index=row*w+col;
				if(((row/8)+(col/8))%2==0) buf[index]=(row<halfway)? 255:0;
				else buf[index]=(row<halfway)? 76:226;
			}
		}
	}else {
		for(row=0; row<h; row++) {
			for(col=0; col<w; col++) {
				if(flags&TJFLAG_BOTTOMUP) index=(h-row-1)*w+col;
				else index=row*w+col;
				if(((row/8)+(col/8))%2==0) {
					if(row<halfway) {
						buf[index*ps+roffset]=255;
						buf[index*ps+goffset]=255;
						buf[index*ps+boffset]=255;
					}
				}else {
					buf[index*ps+roffset]=255;
					if(row>=halfway) buf[index*ps+goffset]=255;
				}
			}
		}
	}
}

static void 
mm_image_codec_writeJPEG(unsigned char *jpegBuf, unsigned long jpegSize, char *filename)
{
	FILE *file=fopen(filename, "wb");
	if(!file || fwrite(jpegBuf, jpegSize, 1, file)!=1) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] ERROR: Could not write to %s \t %s", __func__, __LINE__, filename, strerror(errno)); 
	}

	if(file) fclose(file);
}

static void 
_mm_decode_libjpeg_turbo_decompress(tjhandle handle, unsigned char *jpegBuf, unsigned long jpegSize, int TD_BU,mm_util_jpeg_yuv_data* decoded_data, mm_util_jpeg_yuv_format input_fmt)
{
	unsigned char *dstBuf=NULL;
	int _hdrw=0, _hdrh=0, _hdrsubsamp=-1; 
	int scaledWidth=0;
	int scaledHeight=0;
	unsigned long dstSize=0;
	int i=0,n=0;
	tjscalingfactor _sf;
	tjscalingfactor*sf=tjGetScalingFactors(&n), sf1={1, 1};

	if(jpegBuf == NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] jpegBuf is NULL", __func__, __LINE__);
	}

	mmf_debug(MMF_DEBUG_LOG,  "[%s][%05d] 0x%2x, 0x%2x ", __func__, __LINE__, jpegBuf[0], jpegBuf[1]);

	_tj(tjDecompressHeader2(handle, jpegBuf, jpegSize, &_hdrw, &_hdrh, &_hdrsubsamp));

	if(!sf || !n) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] scaledfactor is NULL", __func__, __LINE__);
	}

	if((_hdrsubsamp==TJSAMP_444 || _hdrsubsamp==TJSAMP_GRAY) || input_fmt==MM_UTIL_JPEG_FMT_RGB888) {
		_sf= sf[i];
	}else {
		_sf=sf1;
	}

	scaledWidth=TJSCALED(_hdrw, _sf);
	scaledHeight=TJSCALED(_hdrh, _sf);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d]_hdrw:%d _hdrh:%d, _hdrsubsamp:%d, scaledWidth:%d, scaledHeight:%d", __func__, __LINE__,  _hdrw, _hdrh, _hdrsubsamp, scaledWidth, scaledHeight);

	if(input_fmt==MM_UTIL_JPEG_FMT_YUV420) {
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] JPEG -> YUV %s ... ", __func__, __LINE__, subName[_hdrsubsamp]);
	}else if(input_fmt==MM_UTIL_JPEG_FMT_RGB888) {
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] JPEG -> RGB,%d/%d ... ",__func__, __LINE__,  _sf.num, _sf.denom);
	}

	#if YUV_DECODE
	if(input_fmt==MM_UTIL_JPEG_FMT_YUV420) { // || input_fmt==MM_UTIL_JPEG_FMT_YUV422)
		dstSize=TJBUFSIZEYUV(_hdrw, _hdrh, _hdrsubsamp);
		mmf_debug(MMF_DEBUG_LOG,  "[%s][%05d] MM_UTIL_JPEG_FMT_YUV420 dstSize: %d  _hdrsubsamp:%d TJBUFSIZEYUV(w, h, _hdrsubsamp): %d", __func__, __LINE__,
			dstSize, _hdrsubsamp, TJBUFSIZEYUV(_hdrw, _hdrh, _hdrsubsamp));
	}
	else if(input_fmt==MM_UTIL_JPEG_FMT_RGB888)
	#endif
	{
		dstSize=scaledWidth*scaledHeight*tjPixelSize[TJPF_RGB];
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] MM_UTIL_JPEG_FMT_RGB888 dstSize: %d _hdrsubsamp:%d", __func__, __LINE__, dstSize, _hdrsubsamp);
	}

	if((dstBuf=(unsigned char *)malloc(dstSize))==NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] dstBuf is NULL", __func__, __LINE__);
	}

	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] dstBuf:%p",  __func__, __LINE__, dstBuf);

	//memset(dstBuf, 0, dstSize);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] memset:%p",  __func__, __LINE__, dstBuf);
	#if YUV_DECODE
	if(input_fmt==MM_UTIL_JPEG_FMT_YUV420) {
		_tj(tjDecompressToYUV(handle, jpegBuf, jpegSize, dstBuf, TD_BU)); 
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] MM_UTIL_JPEG_FMT_YUV420, dstBuf: %d",  __func__, __LINE__, dstBuf);
		decoded_data->format=MM_UTIL_JPEG_FMT_YUV420;
	}
	else if(input_fmt==MM_UTIL_JPEG_FMT_RGB888)
	#endif
	{
		_tj(tjDecompress2(handle, jpegBuf, jpegSize, dstBuf, scaledWidth, 0, scaledHeight, TJPF_RGB, TD_BU));
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] MM_UTIL_JPEG_FMT_RGB888, dstBuf: %p",  __func__, __LINE__, dstBuf);
		decoded_data->format=MM_UTIL_JPEG_FMT_RGB888;
	}
	decoded_data->data=malloc(dstSize);
	memcpy(decoded_data->data, dstBuf, dstSize);
	decoded_data->size=dstSize; //decoded_data->yuv_subsample=_hdrsubsamp;
	#if YUV_DECODE
	if(input_fmt==MM_UTIL_JPEG_FMT_RGB888) 
	#endif
	{
		decoded_data->width=scaledWidth; decoded_data->height=scaledHeight;
	}
	#if YUV_DECODE
	else if(input_fmt==MM_UTIL_JPEG_FMT_YUV420)
	{
		if(_hdrsubsamp == TJSAMP_422) {
			decoded_data->width=MM_LIBJPEG_TURBO_ROUND_UP_2(_hdrw); //dstSize / _hdrh
		}else {
			if(_hdrw% 4 != 0) {
				decoded_data->width=MM_LIBJPEG_TURBO_ROUND_UP_4(_hdrw);
			}else {
				decoded_data->width=_hdrw;
			}
		}

		if(_hdrsubsamp == TJSAMP_420) {
			if(_hdrh% 4 != 0) {decoded_data->height=MM_LIBJPEG_TURBO_ROUND_UP_4(_hdrh);
			}else {
			decoded_data->height=_hdrh;
			}
		}else {
		decoded_data->height=_hdrh;
	}
		char tmp[100]="yuv.yuv";  	//sprintf (tmp, "rgb%d.rgb", sf.denom);
		mm_image_codec_writeJPEG(dstBuf, dstSize, tmp);
	}
	char tmp[100]="yuv.yuv";  	//sprintf (tmp, "rgb%d.rgb", sf.denom);
	mm_image_codec_writeJPEG(dstBuf, dstSize, tmp);
	#endif
}

static int
mm_image_decode_from_jpeg_file_with_libjpeg_turbo(mm_util_jpeg_yuv_data * decoded_data, char *pFileName, mm_util_jpeg_yuv_format input_fmt)
{
	int iErrorCode	= MM_ERROR_NONE;
	tjhandle dhandle=NULL;
	unsigned char *srcBuf=NULL;
	int jpegSize;
	int TD_BU=0; //flags|=TJFLAG_BOTTOMUP;

	void *src = fopen(pFileName, "rb" );
	fseek (src, 0,  SEEK_END);
	jpegSize = ftell(src);
	rewind(src);

	FILE * fp =fopen(pFileName, "rb");
	if(fp == NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d]Error", __func__, __LINE__);
	}

	if((dhandle=tjInitDecompress())==NULL) {
		mmf_debug(MMF_DEBUG_ERROR,  "[%s][%05d] dhandle=tjInitDecompress()) is NULL");
	}
	srcBuf=(unsigned char *)malloc(sizeof(char*) * jpegSize);
	fread(srcBuf,1, jpegSize, fp);
	mmf_debug(MMF_DEBUG_LOG,  "[%s][%05d] srcBuf[0]: 0x%2x, srcBuf[1]: 0x%2x, jpegSize:%d", __func__, __LINE__, srcBuf[0], srcBuf[1], jpegSize);
	_mm_decode_libjpeg_turbo_decompress(dhandle, srcBuf, jpegSize, TD_BU, decoded_data, input_fmt);
	fclose(fp);

	if(dhandle) {
		tjDestroy(dhandle);
	}
	if(srcBuf) {
		tjFree(srcBuf);
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] fclose", __func__, __LINE__);

	return iErrorCode;
}

static void
_mm_encode_libjpeg_turbo_compress(tjhandle handle, void *src, unsigned char **dstBuf, unsigned long *dstSize, int w, int h, int jpegQual, int flags, mm_util_jpeg_yuv_format fmt)
{
	unsigned char *srcBuf=NULL;
	unsigned long jpegSize=0;

	jpegSize=w*h*tjPixelSize[TJPF_RGB];
	srcBuf=(unsigned char *)malloc(jpegSize);

	if(srcBuf==NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] srcBuf is NULL", __func__, __LINE__);
	}else if(srcBuf != NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] srcBuf: %p", __func__, __LINE__, srcBuf);
		if(fmt==MM_UTIL_JPEG_FMT_RGB888) {
			initBuf(srcBuf, w, h, TJPF_RGB, flags);
		}else if(fmt==MM_UTIL_JPEG_FMT_YUV420 || fmt==MM_UTIL_JPEG_FMT_YUV420) {
			initBuf(srcBuf, w, h, TJPF_GRAY, flags);
		}
	}

	if(src != NULL) {
		memcpy(srcBuf, src, jpegSize);
	}else {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] src is NULL", __func__, __LINE__);
	}

	if(*dstBuf && *dstSize>0) memset(*dstBuf, 0, *dstSize);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Done.", __func__, __LINE__);
	*dstSize=TJBUFSIZE(w, h);
	#if YUV_ENCODE
	if(fmt==MM_UTIL_JPEG_FMT_RGB888)
	#endif
	{
		_tj(tjCompress2(handle, srcBuf, w, 0, h, TJPF_RGB, dstBuf, dstSize, TJPF_RGB, jpegQual, flags));
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] *dstSize: %d", __func__, __LINE__, *dstSize); 
	}
	#if YUV_ENCODE
	else if(fmt==MM_UTIL_JPEG_FMT_YUV420) {
		*dstSize=TJBUFSIZEYUV(w, h, TJSAMP_420);
		_tj(tjEncodeYUV2(handle, srcBuf, w, 0, h, TJPF_RGB, *dstBuf, TJSAMP_420, flags)); //	_tj(tjCompress2(handle, srcBuf, w, 0, h, TJPF_RGB, dstBuf, dstSize, TJSAMP_422, jpegQual, flags));
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] *dstSize: %d \t decode_yuv_subsample: %d TJBUFSIZE(w, h):%d", __func__, __LINE__, *dstSize, TJSAMP_422, TJBUFSIZE(w, h));
	}else {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] fmt:%d  is wrong", __func__, __LINE__, fmt);
	}
	#endif
	if(srcBuf) free(srcBuf);
}

static int
mm_image_encode_to_jpeg_file_with_libjpeg_turbo(char *filename, void* src, int width, int height, mm_util_jpeg_yuv_format fmt, int quality)
{
	int iErrorCode	= MM_ERROR_NONE;
	tjhandle chandle=NULL;
	unsigned char *dstBuf=NULL;
	unsigned long size=0;
	int TD_BU=0;
	FILE *fout=fopen(filename, "w+");
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] fmt: %d", __func__, __LINE__, fmt); 

	if(fmt==MM_UTIL_JPEG_FMT_RGB888) {
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] MM_UTIL_JPEG_FMT_RGB888", __func__, __LINE__);
	}else {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] fmt:%d  is wrong", __func__, __LINE__, fmt); 
	}
	size=TJBUFSIZE(width, height);
	if((dstBuf=(unsigned char *)tjAlloc(size))==NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] dstBuf is NULL", __func__, __LINE__);
	}

	if((chandle=tjInitCompress())==NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] dstBuf is NULL", __func__, __LINE__);
	}
	_mm_encode_libjpeg_turbo_compress(chandle, src, &dstBuf, &size, width, height, quality, TD_BU, fmt);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] dstBuf: %p\t size: %d", __func__, __LINE__, dstBuf, size);
	fwrite(dstBuf, 1, size, fout);
	fclose(fout);
	if(chandle) {
		tjDestroy(chandle);
	}
	if(dstBuf) {
		tjFree(dstBuf);
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Done", __func__, __LINE__);
	return iErrorCode;
}

static int
mm_image_encode_to_jpeg_memory_with_libjpeg_turbo(void **mem, int *csize, void *rawdata,  int w, int h, mm_util_jpeg_yuv_format fmt,int quality)
{
	int iErrorCode	= MM_ERROR_NONE;
	tjhandle chandle=NULL;
	unsigned char *dstBuf=NULL;
	unsigned long size=0;
	int TD_BU=0;

	size=TJBUFSIZE(w, h);
	if(fmt==MM_UTIL_JPEG_FMT_RGB888) {
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] MM_UTIL_JPEG_FMT_RGB888 size: %d", __func__, __LINE__, size);
	}else if(fmt==MM_UTIL_JPEG_FMT_YUV420) {
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] TJSAMP_420 size: %d", __func__, __LINE__,size); 
	}else {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] fmt:%d  is wrong", __func__, __LINE__, fmt); 
	}

	if((dstBuf=(void *)tjAlloc(size))==NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] dstBuf is NULL", __func__, __LINE__);
	}

	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] dstBuf", __func__, __LINE__);

	if((chandle=tjInitCompress())==NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] chandle is NULL", __func__, __LINE__);
	}

	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] width: %d height: %d, size: %d", __func__, __LINE__, w, h, size);
	_mm_encode_libjpeg_turbo_compress(chandle, rawdata, &dstBuf, &size, w, h, quality, TD_BU, fmt);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] dstBuf: %p &dstBuf:%p size: %d", __func__, __LINE__, dstBuf, &dstBuf, size);
	*mem=dstBuf;
	*csize=size;
	if(chandle) {
		tjDestroy(chandle);
	}
	return iErrorCode;
}
#endif

static int
mm_image_decode_from_jpeg_file_with_libjpeg(mm_util_jpeg_yuv_data * decoded_data, char *pFileName, mm_util_jpeg_yuv_format input_fmt)
{
	int iErrorCode	= MM_ERROR_NONE;
	FILE *infile		= NULL;
	struct jpeg_decompress_struct dinfo;
	struct my_error_mgr_s jerr;
	JSAMPARRAY buffer; /* Output row buffer */
	int row_stride = 0, state = 0; /* physical row width in output buffer */
	char *image, *u_image, *v_image;
	JSAMPROW row[1];

	mmf_debug(MMF_DEBUG_LOG,"[%s] [%05d] Enter", __func__, __LINE__);

	infile = fopen(pFileName, "rb");
	if(infile == NULL) {
	mmf_debug(MMF_DEBUG_ERROR, "[%s] [%05d] [infile] Exit on error", __func__, __LINE__);
	return MM_ERROR_IMAGE_FILEOPEN;
}
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] infile", __func__, __LINE__);
	/* allocate and initialize JPEG decompression object   We set up the normal JPEG error routines, then override error_exit. */
	dinfo.err = jpeg_std_error(&jerr.pub);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jpeg_std_error", __func__, __LINE__);
	jerr.pub.error_exit = my_error_exit;
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jerr.pub.error_exit ", __func__, __LINE__);

	/* Establish the setjmp return context for my_error_exit to use. */ //
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.  We need to clean up the JPEG object, close the input file, and return.*/
		mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] setjmp", __func__, __LINE__);
		jpeg_destroy_decompress(&dinfo);
		fclose(infile);
		return MM_ERROR_IMAGE_FILEOPEN;
	}

	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] if(setjmp)", __func__, __LINE__);
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&dinfo);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jpeg_create_decompress", __func__, __LINE__);

	/*specify data source (eg, a file) */
	jpeg_stdio_src(&dinfo, infile);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jpeg_stdio_src", __func__, __LINE__);

	/*read file parameters with jpeg_read_header() */
	jpeg_read_header(&dinfo, TRUE);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jpeg_read_header", __func__, __LINE__);

#if PARTIAL_DECODE
	mmf_debug(MMF_DEBUG_LOG, "PARTIAL_DECODE");
	dinfo.do_fancy_upsampling = FALSE;
	dinfo.do_block_smoothing = FALSE;
	dinfo.dct_method = JDCT_IFAST;
	dinfo.dither_mode = JDITHER_ORDERED;
#endif

	if(input_fmt == MM_UTIL_JPEG_FMT_YUV420) {
		dinfo.out_color_space=JCS_YCbCr;
		jpeg_calc_output_dimensions(&dinfo);
		decoded_data->format = MM_UTIL_JPEG_FMT_YUV420;
		mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] cinfo.out_color_space=JCS_YCbCr", __func__, __LINE__);
	}else if(input_fmt == MM_UTIL_JPEG_FMT_GraySacle) {
		dinfo.out_color_space=JCS_GRAYSCALE;
		jpeg_calc_output_dimensions(&dinfo);
		decoded_data->format = MM_UTIL_JPEG_FMT_GraySacle;
		mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] cinfo.out_color_space=JCS_GRAYSCALE", __func__, __LINE__);
	}else if (input_fmt == MM_UTIL_JPEG_FMT_RGB888) {
		dinfo.out_color_space=JCS_RGB;
		decoded_data->format = MM_UTIL_JPEG_FMT_RGB888;
		mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] cinfo.out_color_space=JCS_RGB", __func__, __LINE__);
	}

	/* Start decompressor*/
	jpeg_start_decompress(&dinfo);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jpeg_start_decompress", __func__, __LINE__);

#if PARTIAL_DECODE
	dinfo.region_w = 400;
	dinfo.region_h = 360;
	mmf_debug(MMF_DEBUG_LOG, "[cinfo.region] cinfo.region_x :%d, cinfo.region_y:%d ", dinfo.region_x , dinfo.region_y );
	dinfo.output_width = dinfo.region_w;
	dinfo.output_height =  dinfo.region_h;

	decoded_data->width = dinfo.output_width;
	decoded_data->height= dinfo.output_height;
#endif
	/* JSAMPLEs per row in output buffer */
	row_stride = dinfo.output_width * dinfo.output_components;
	
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*dinfo.mem->alloc_sarray) ((j_common_ptr) &dinfo, JPOOL_IMAGE, row_stride, 1);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] buffer", __func__, __LINE__);

	decoded_data->data= (void*) g_malloc0(dinfo.output_height * row_stride);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] decoded_data->data", __func__, __LINE__);

	if(input_fmt == MM_UTIL_JPEG_FMT_YUV420) {
		image = decoded_data->data;
		u_image = image + (dinfo.output_width * dinfo.output_height);
		v_image = u_image + (dinfo.output_width*dinfo.output_height)/4;
		row[0]=(unsigned char *)buffer;
		int i=0; int y=0;
		while (dinfo.output_scanline  < (unsigned int)dinfo.output_height) {
			jpeg_read_scanlines(&dinfo, row, 1);
			for (i = 0; i < row_stride; i += 3) {
				image[i/3]=((unsigned char *)buffer)[i];
				if (i & 1) {
					u_image[(i/3)/2]=((unsigned char *)buffer)[i+1];
					v_image[(i/3)/2]=((unsigned char *)buffer)[i+2];
				}
			}
			image += row_stride/3;
			if (y++ & 1) {
				u_image += dinfo.output_width / 2;
				v_image += dinfo.output_width / 2;
			}
		}
	}else {
		/* while (scan lines remain to be read) jpeg_read_scanlines(...); */
		while (dinfo.output_scanline < dinfo.output_height) {
			/* jpeg_read_scanlines expects an array of pointers to scanlines. Here the array is only one element long, but you could ask formore than one scanline at a time if that's more convenient. */
			jpeg_read_scanlines(&dinfo, buffer, 1);
			memcpy(decoded_data->data + state, buffer[0], row_stride);
			state += row_stride;
		}
		mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] while", __func__, __LINE__);
	}

	decoded_data->width = dinfo.output_width;
	decoded_data->height= dinfo.output_height;
	decoded_data->size= dinfo.output_height * row_stride;
	decoded_data->format = input_fmt;

	/* Finish decompression */
	jpeg_finish_decompress(&dinfo);

	/* Release JPEG decompression object */
	jpeg_destroy_decompress(&dinfo);

	fclose(infile);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] fclose", __func__, __LINE__);

	return iErrorCode;
}

static int
mm_image_decode_from_jpeg_memory_with_libjpeg(mm_util_jpeg_yuv_data * decoded_data, void *src, int size, mm_util_jpeg_yuv_format input_fmt)
{
	int iErrorCode	= MM_ERROR_NONE;
	struct jpeg_decompress_struct dinfo;
	struct my_error_mgr_s jerr;
	JSAMPARRAY buffer; /* Output row buffer */
	int row_stride = 0, state = 0; /* physical row width in output buffer */
	char *image, *u_image, *v_image;
	JSAMPROW row[1];
	mmf_debug(MMF_DEBUG_LOG,"[%s] [%05d] Enter", __func__, __LINE__);

	if(src == NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s] [%05d] [infile] Exit on error", __func__, __LINE__);
		return MM_ERROR_IMAGE_FILEOPEN;
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] infile", __func__, __LINE__);

	/* allocate and initialize JPEG decompression object   We set up the normal JPEG error routines, then override error_exit. */
	dinfo.err = jpeg_std_error(&jerr.pub);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jpeg_std_error ", __func__, __LINE__);
	jerr.pub.error_exit = my_error_exit;
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jerr.pub.error_exit ", __func__, __LINE__);

	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.  We need to clean up the JPEG object, close the input file, and return.*/
		mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] setjmp", __func__, __LINE__);
		jpeg_destroy_decompress(&dinfo);
		return MM_ERROR_IMAGE_FILEOPEN;
	}

	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] if(setjmp)", __func__, __LINE__);
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&dinfo);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jpeg_create_decompress", __func__, __LINE__);

	/*specify data source (eg, a file) */
	jpeg_mem_src(&dinfo, src, size);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jpeg_stdio_src", __func__, __LINE__);

	/*read file parameters with jpeg_read_header() */
	jpeg_read_header(&dinfo, TRUE);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jpeg_read_header", __func__, __LINE__);

	/* set parameters for decompression */
	if(input_fmt == MM_UTIL_JPEG_FMT_YUV420) {
		dinfo.out_color_space=JCS_YCbCr;
		jpeg_calc_output_dimensions(&dinfo);
		decoded_data->format = MM_UTIL_JPEG_FMT_YUV420;
		mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] dinfo.out_color_space=JCS_YCbCr", __func__, __LINE__);
	}else if(input_fmt == MM_UTIL_JPEG_FMT_GraySacle) {
		dinfo.out_color_space=JCS_GRAYSCALE;
		jpeg_calc_output_dimensions(&dinfo);
		decoded_data->format = MM_UTIL_JPEG_FMT_GraySacle;
		mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] dinfo.out_color_space=JCS_GRAYSCALE", __func__, __LINE__);
	}else if (input_fmt == MM_UTIL_JPEG_FMT_RGB888) {
		dinfo.out_color_space=JCS_RGB;
		mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] dinfo.out_color_space=JCS_RGB", __func__, __LINE__);
	}

	/* Start decompressor*/
	jpeg_start_decompress(&dinfo);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jpeg_start_decompress", __func__, __LINE__);

	/* JSAMPLEs per row in output buffer */
	row_stride = dinfo.output_width * dinfo.output_components;

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*dinfo.mem->alloc_sarray) ((j_common_ptr) &dinfo, JPOOL_IMAGE, row_stride, 1);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] buffer", __func__, __LINE__);

	decoded_data->data= (void*) g_malloc0(dinfo.output_height * row_stride);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] decoded_data->data", __func__, __LINE__);

	/* while (scan lines remain to be read) jpeg_read_scanlines(...); */
	if(input_fmt == MM_UTIL_JPEG_FMT_YUV420) {
		image = decoded_data->data;
		u_image = image + (dinfo.output_width * dinfo.output_height);
		v_image = u_image + (dinfo.output_width*dinfo.output_height)/4;
		row[0]=(unsigned char *)buffer;
		int i=0; int y=0;
		while (dinfo.output_scanline  < (unsigned int)dinfo.output_height) {
			jpeg_read_scanlines(&dinfo, row, 1);
			for (i = 0; i < row_stride; i += 3) {
				image[i/3]=((unsigned char *)buffer)[i];
				if (i & 1) {
					u_image[(i/3)/2]=((unsigned char *)buffer)[i+1];
					v_image[(i/3)/2]=((unsigned char *)buffer)[i+2];
				}
			}
			image += row_stride/3;
			if (y++ & 1) {
				u_image += dinfo.output_width / 2;
				v_image += dinfo.output_width / 2;
			}
		}
	}else {
		while (dinfo.output_scanline < dinfo.output_height) {
			/* jpeg_read_scanlines expects an array of pointers to scanlines. Here the array is only one element long, but you could ask formore than one scanline at a time if that's more convenient. */
			jpeg_read_scanlines(&dinfo, buffer, 1);

			memcpy(decoded_data->data + state, buffer[0], row_stride);
			state += row_stride;
		}
		mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] while", __func__, __LINE__);

	}
	decoded_data->width = dinfo.output_width;
	decoded_data->height= dinfo.output_height;
	decoded_data->size= dinfo.output_height * row_stride;
	decoded_data->format = input_fmt;
	/* Finish decompression */
	jpeg_finish_decompress(&dinfo);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jpeg_finish_decompress", __func__, __LINE__);

	/* Release JPEG decompression object */
	jpeg_destroy_decompress(&dinfo);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] jpeg_destroy_decompress", __func__, __LINE__);
	
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] fclose", __func__, __LINE__);

	return iErrorCode;
}

EXPORT_API int
mm_util_jpeg_encode_to_file(char *filename, void* src, int width, int height, mm_util_jpeg_yuv_format fmt, int quality)
{
	int ret = MM_ERROR_NONE;

	if( !filename || !src) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# filename || src buffer is NULL", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (width < 0) || (height < 0)) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# src_width || src_height value ", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (fmt < MM_UTIL_JPEG_FMT_YUV420) || (fmt > MM_UTIL_JPEG_FMT_GraySacle) ) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# fmt value", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (quality < 0 ) || (quality>100) ) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# quality vaule", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #START# LIBJPEG", __func__, __LINE__);
	ret=mm_image_encode_to_jpeg_file_with_libjpeg(filename, src, width, height, fmt, quality);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #END# libjpeg, ret: %d", __func__, __LINE__, ret);

	return ret;
}

EXPORT_API int
mm_util_jpeg_encode_to_memory(void **mem, int *size, void* src, int width, int height, mm_util_jpeg_yuv_format fmt, int quality)
{
	int ret = MM_ERROR_NONE;

	if( !mem || !size || !src) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# filename ||size ||  src buffer is NULL", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (width < 0) || (height < 0)) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# src_width || src_height value ", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (fmt < MM_UTIL_JPEG_FMT_YUV420) || (fmt > MM_UTIL_JPEG_FMT_GraySacle) ) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# fmt value", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if(  (quality < 0 ) || (quality>100) ) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# quality vaule", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #START# libjpeg", __func__, __LINE__);
	ret=mm_image_encode_to_jpeg_memory_with_libjpeg(mem, size, src, width, height, fmt, quality);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #END# libjpeg, ret: %d", __func__, __LINE__, ret);

	return ret;
}

EXPORT_API int
mm_util_decode_from_jpeg_file(mm_util_jpeg_yuv_data *decoded, char *filename, mm_util_jpeg_yuv_format fmt)
{
	int ret = MM_ERROR_NONE;

	if( !decoded || !filename) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# decoded || filename buffer is NULL", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (fmt < MM_UTIL_JPEG_FMT_YUV420) || (fmt > MM_UTIL_JPEG_FMT_GraySacle) ) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# fmt value", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

#if HW_JPEG_DECODE
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] [mm_image_decode_from_jpeg_file_with_c110hw] Start", __func__, __LINE__);
	ret = mm_image_decode_from_jpeg_file_with_c110hw(decoded, filename, fmt);
	mmf_debug(MMF_DEBUG_LOG, "decoded->data: %p\t width: %d\t height:%d\n", decoded->data, decoded->width, decoded->height);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] [mm_image_decode_from_jpeg_file_with_c110hw] End, ret: %d", ret);

#elif LIBJPEG
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #START# libjpeg", __func__, __LINE__);
	ret = mm_image_decode_from_jpeg_file_with_libjpeg(decoded, filename, fmt);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] decoded->data: %p\t width: %d\t height:%d\n", __func__, __LINE__, decoded->data, decoded->width, decoded->height);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #End# libjpeg, ret: %d", __func__, __LINE__,  ret);
#endif

	return ret;
}

EXPORT_API int
mm_util_decode_from_jpeg_memory(mm_util_jpeg_yuv_data* decoded, void* src, int size, mm_util_jpeg_yuv_format fmt)
{
	int ret = MM_ERROR_NONE;

	if( !decoded || !src) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# decoded || src buffer is NULL", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if(size < 0) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# size", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (fmt < MM_UTIL_JPEG_FMT_YUV420) || (fmt > MM_UTIL_JPEG_FMT_GraySacle) ) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# fmt value", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}


	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #START# libjpeg", __func__, __LINE__);
	ret = mm_image_decode_from_jpeg_memory_with_libjpeg(decoded, src, size, fmt);

	mmf_debug(MMF_DEBUG_LOG, "decoded->data: %p\t width: %d\t height:%d\n", decoded->data, decoded->width, decoded->height);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #END# libjpeg, ret: %d", __func__, __LINE__, ret);

	return ret;
}
#if LIBJPEG_TURBO
EXPORT_API int
mm_util_jpeg_turbo_encode_to_file(char *filename, void* src, int width, int height, mm_util_jpeg_yuv_format fmt, int quality)
{
	int ret = MM_ERROR_NONE;

	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #START# LIBJPEG_TURBO", __func__, __LINE__);
	ret=mm_image_encode_to_jpeg_file_with_libjpeg_turbo(filename, src, width, height, fmt, quality);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #End# libjpeg, ret: %d", __func__, __LINE__, ret);

	return ret;
}

EXPORT_API int
mm_util_jpeg_turbo_encode_to_memory(void **mem, int *size, void* src, int width, int height, mm_util_jpeg_yuv_format fmt, int quality)
{
	int ret = MM_ERROR_NONE;

	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #START# libjpeg", __func__, __LINE__);
	ret = mm_image_encode_to_jpeg_memory_with_libjpeg_turbo(mem, size, src, width, height, fmt, quality);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #End# libjpeg, ret: %d", __func__, __LINE__, ret);

	return ret;
}

EXPORT_API int
mm_util_decode_from_jpeg_turbo_file(mm_util_jpeg_yuv_data *decoded, char *filename, mm_util_jpeg_yuv_format fmt)
{
	int ret = MM_ERROR_NONE;

	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #START# LIBJPEG_TURBO", __func__, __LINE__);
	ret = mm_image_decode_from_jpeg_file_with_libjpeg_turbo(decoded, filename, fmt);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] decoded->data: %p\t width: %d\t height:%d\n", __func__, __LINE__, decoded->data, decoded->width, decoded->height);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #End# LIBJPEG_TURBO, ret: %d", __func__, __LINE__, ret);
	return ret;
}

EXPORT_API int
mm_util_decode_from_jpeg_turbo_memory(mm_util_jpeg_yuv_data* decoded, void* src, int size, mm_util_jpeg_yuv_format fmt)
{
	int ret = MM_ERROR_NONE;

	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #START# libjpeg", __func__, __LINE__);
	ret = mm_image_decode_from_jpeg_memory_with_libjpeg(decoded, src, size, fmt);

	mmf_debug(MMF_DEBUG_LOG, "decoded->data: %p\t width: %d\t height:%d\n", decoded->data, decoded->width, decoded->height);
	mmf_debug(MMF_DEBUG_LOG, "[%s] [%05d] #END# libjpeg, ret: %d", __func__, __LINE__, ret);

	return ret;
}
#endif
