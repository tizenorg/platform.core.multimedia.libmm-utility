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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/times.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h> /* fopen() */
#include <jpeglib.h>
#define LIBJPEG_TURBO 0
#define ENC_MAX_LEN 8192
#if LIBJPEG_TURBO
#include <turbojpeg.h>
#endif
#include <libexif/exif-data.h>

#include <setjmp.h>
#include <glib.h>
#include "mm_util_jpeg.h"
#include "mm_util_imgp.h"
#include "mm_util_debug.h"
#ifdef ENABLE_TTRACE
#include <ttrace.h>
#define TTRACE_BEGIN(NAME) traceBegin(TTRACE_TAG_IMAGE, NAME)
#define TTRACE_END() traceEnd(TTRACE_TAG_IMAGE)
#else //ENABLE_TTRACE
#define TTRACE_BEGIN(NAME)
#define TTRACE_END()
#endif //ENABLE_TTRACE

#ifndef YUV420_SIZE
#define YUV420_SIZE(width, height)	(width*height*3>>1)
#endif
#ifndef YUV422_SIZE
#define YUV422_SIZE(width, height)	(width*height<<1)
#endif

#define PARTIAL_DECODE 1
#define LIBJPEG 1
#define MM_JPEG_ROUND_UP_2(num)  (((num)+1)&~1)
#define MM_JPEG_ROUND_UP_4(num)  (((num)+3)&~3)
#define MM_JPEG_ROUND_UP_8(num)  (((num)+7)&~7)
#define MM_JPEG_ROUND_DOWN_2(num) ((num)&(~1))
#define MM_JPEG_ROUND_DOWN_4(num) ((num)&(~3))
#define MM_JPEG_ROUND_DOWN_16(num) ((num)&(~15))

#define ERR_BUF_LENGHT 256
#define mm_util_stderror(fmt) do { \
			char buf[ERR_BUF_LENGHT] = {0,}; \
			strerror_r(errno, buf, ERR_BUF_LENGHT); \
			mm_util_error(fmt" : standard error= [%s]", buf); \
		} while (0)

#define IMG_JPEG_FREE(src) { if(src != NULL) {free(src); src = NULL;} }

#if LIBJPEG_TURBO
#define PAD(v, p) ((v+(p)-1)&(~((p)-1)))
#define _throwtj() {printf("TurboJPEG ERROR:\n%s\n", tjGetErrorStr());}
#define _tj(f) {if((f)==-1) _throwtj();}

const char *subName[TJ_NUMSAMP]={"444", "422", "420", "GRAY", "440"};

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
	} else {
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
				} else {
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
		mm_util_error("ERROR: Could not write to %s", filename);
		mm_util_stderror("ERROR: Could not write");
	}

	if(file) fclose(file);
}

static void
_mm_decode_libjpeg_turbo_decompress(tjhandle handle, unsigned char *jpegBuf, unsigned long jpegSize, int TD_BU,mm_util_jpeg_yuv_data* decoded_data, mm_util_jpeg_yuv_format input_fmt)
{
	int _hdrw=0, _hdrh=0, _hdrsubsamp=-1;
	int scaledWidth=0;
	int scaledHeight=0;
	unsigned long dstSize=0;
	int i=0,n=0;
	tjscalingfactor _sf;
	tjscalingfactor*sf=tjGetScalingFactors(&n), sf1={1, 1};

	if(jpegBuf == NULL) {
		mm_util_error("jpegBuf is NULL");
		return;
	}

	mm_util_debug("0x%2x, 0x%2x ", jpegBuf[0], jpegBuf[1]);

	_tj(tjDecompressHeader2(handle, jpegBuf, jpegSize, &_hdrw, &_hdrh, &_hdrsubsamp));

	if(!sf || !n) {
		mm_util_error(" scaledfactor is NULL");
		return;
	}

	if((_hdrsubsamp==TJSAMP_444 || _hdrsubsamp==TJSAMP_GRAY) || input_fmt==MM_UTIL_JPEG_FMT_RGB888) {
		_sf= sf[i];
	} else {
		_sf=sf1;
	}

	scaledWidth=TJSCALED(_hdrw, _sf);
	scaledHeight=TJSCALED(_hdrh, _sf);
	mm_util_debug("_hdrw:%d _hdrh:%d, _hdrsubsamp:%d, scaledWidth:%d, scaledHeight:%d",  _hdrw, _hdrh, _hdrsubsamp, scaledWidth, scaledHeight);

	if(input_fmt==MM_UTIL_JPEG_FMT_YUV420 || input_fmt==MM_UTIL_JPEG_FMT_YUV422) {
		mm_util_debug("JPEG -> YUV %s ... ", subName[_hdrsubsamp]);
	} else if(input_fmt==MM_UTIL_JPEG_FMT_RGB888) {
		mm_util_debug("JPEG -> RGB %d/%d ... ", _sf.num, _sf.denom);
	}

	if(input_fmt==MM_UTIL_JPEG_FMT_YUV420 || input_fmt==MM_UTIL_JPEG_FMT_YUV422) {
		dstSize=TJBUFSIZEYUV(_hdrw, _hdrh, _hdrsubsamp);
		mm_util_debug("MM_UTIL_JPEG_FMT_YUV420 dstSize: %d  _hdrsubsamp:%d TJBUFSIZEYUV(w, h, _hdrsubsamp): %d",
			dstSize, _hdrsubsamp, TJBUFSIZEYUV(_hdrw, _hdrh, _hdrsubsamp));
	}
	else if(input_fmt==MM_UTIL_JPEG_FMT_RGB888)
	{
		dstSize=scaledWidth*scaledHeight*tjPixelSize[TJPF_RGB];
		mm_util_debug("MM_UTIL_JPEG_FMT_RGB888 dstSize: %d _hdrsubsamp:%d", dstSize, _hdrsubsamp);
	}

	if((decoded_data->data=(void*)malloc(dstSize))==NULL) {
		mm_util_error("dstBuf is NULL");
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	mm_util_debug("dstBuf:%p", decoded_data->data);

	if(input_fmt==MM_UTIL_JPEG_FMT_YUV420 || input_fmt==MM_UTIL_JPEG_FMT_YUV422) {
		_tj(tjDecompressToYUV(handle, jpegBuf, jpegSize, decoded_data->data, TD_BU));
		mm_util_debug("MM_UTIL_JPEG_FMT_YUV420, dstBuf: %d", decoded_data->data);
		decoded_data->format=input_fmt;
	}
	else if(input_fmt==MM_UTIL_JPEG_FMT_RGB888) {
		_tj(tjDecompress2(handle, jpegBuf, jpegSize, decoded_data->data, scaledWidth, 0, scaledHeight, TJPF_RGB, TD_BU));
		mm_util_debug("MM_UTIL_JPEG_FMT_RGB888, dstBuf: %p", decoded_data->data);
		decoded_data->format=MM_UTIL_JPEG_FMT_RGB888;
	} else {
		mm_util_error("[%s][%05d] We can't support the IMAGE format");
		return;
	}

	decoded_data->size=dstSize;
	if(input_fmt==MM_UTIL_JPEG_FMT_RGB888)
	{
		decoded_data->width=scaledWidth;
		decoded_data->height=scaledHeight;
		decoded_data->size = dstSize;
	}
	else if(input_fmt==MM_UTIL_JPEG_FMT_YUV420 ||input_fmt==MM_UTIL_JPEG_FMT_YUV422)
	{
		if(_hdrsubsamp == TJSAMP_422) {
			mm_util_debug("_hdrsubsamp == TJSAMP_422", decoded_data->width, _hdrw);
			decoded_data->width=MM_JPEG_ROUND_UP_2(_hdrw);
		} else {
			if(_hdrw% 4 != 0) {
				decoded_data->width=MM_JPEG_ROUND_UP_4(_hdrw);
			} else {
				decoded_data->width=_hdrw;
			}
		}

		if(_hdrsubsamp == TJSAMP_420) {
			mm_util_debug("_hdrsubsamp == TJSAMP_420", decoded_data->width, _hdrw);
			if(_hdrh% 4 != 0) {
				decoded_data->height=MM_JPEG_ROUND_UP_4(_hdrh);
			} else {
			decoded_data->height=_hdrh;
			}
		} else {
			decoded_data->height=_hdrh;
		}
		decoded_data->size = dstSize;
	}
}

static int
mm_image_decode_from_jpeg_file_with_libjpeg_turbo(mm_util_jpeg_yuv_data * decoded_data, const char *pFileName, mm_util_jpeg_yuv_format input_fmt)
{
	int iErrorCode = MM_UTIL_ERROR_NONE;
	tjhandle dhandle=NULL;
	unsigned char *srcBuf=NULL;
	int jpegSize;
	int TD_BU=0;

	void *src = fopen(pFileName, "rb" );
	if(src == NULL) {
		mm_util_error("Error [%s] failed", pFileName);
		mm_util_stderror("Error file open failed");
		return MM_UTIL_ERROR_NO_SUCH_FILE;
	}
	fseek (src, 0,  SEEK_END);
	jpegSize = ftell(src);
	rewind(src);

	if((dhandle=tjInitDecompress())==NULL) {
		fclose(src);
		mm_util_error("dhandle=tjInitDecompress()) is NULL");
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}
	srcBuf=(unsigned char *)malloc(sizeof(char) * jpegSize);
	if(srcBuf==NULL) {
		fclose(src);
		tjDestroy(dhandle);
		mm_util_error("srcBuf is NULL");
		return MM_UTIL_ERROR_OUT_OF_MEMORY;
	}

	fread(srcBuf,1, jpegSize, src);
	mm_util_debug("srcBuf[0]: 0x%2x, srcBuf[1]: 0x%2x, jpegSize:%d", srcBuf[0], srcBuf[1], jpegSize);
	_mm_decode_libjpeg_turbo_decompress(dhandle, srcBuf, jpegSize, TD_BU, decoded_data, input_fmt);
	fclose(src);
	mm_util_debug("fclose");

	if(dhandle) {
		tjDestroy(dhandle);
	}
	if(srcBuf) {
		tjFree(srcBuf);
	}

	return iErrorCode;
}

static int
mm_image_decode_from_jpeg_memory_with_libjpeg_turbo(mm_util_jpeg_yuv_data * decoded_data, void *src, int size, mm_util_jpeg_yuv_format input_fmt)
{
	int iErrorCode = MM_UTIL_ERROR_NONE;
	tjhandle dhandle=NULL;
	unsigned char *srcBuf=NULL;
	int TD_BU=0;

	if((dhandle=tjInitDecompress())==NULL) {
		mm_util_error("dhandle=tjInitDecompress()) is NULL");
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}
	srcBuf=(unsigned char *)malloc(sizeof(char) * size);
	if(srcBuf==NULL) {
		fclose(src);
		tjDestroy(dhandle);
		mm_util_error("srcBuf is NULL");
		return MM_UTIL_ERROR_OUT_OF_MEMORY;
	}

	mm_util_debug("srcBuf[0]: 0x%2x, srcBuf[1]: 0x%2x, jpegSize:%d", srcBuf[0], srcBuf[1], size);
	_mm_decode_libjpeg_turbo_decompress(dhandle, src, size, TD_BU, decoded_data, input_fmt);

	if(dhandle) {
		tjDestroy(dhandle);
	}
	if(srcBuf) {
		tjFree(srcBuf);
	}

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
		mm_util_error("srcBuf is NULL");
		return;
	} else {
		mm_util_debug("srcBuf: 0x%2x", srcBuf);
		if(fmt==MM_UTIL_JPEG_FMT_RGB888) {
			initBuf(srcBuf, w, h, TJPF_RGB, flags);
		} else if(fmt==MM_UTIL_JPEG_FMT_YUV420 || fmt==MM_UTIL_JPEG_FMT_YUV422) {
			initBuf(srcBuf, w, h, TJPF_GRAY, flags);
		} else {
			IMG_JPEG_FREE(srcBuf);
			mm_util_error("[%s][%05d] We can't support the IMAGE format");
			return;
		}
		memcpy(srcBuf, src, jpegSize);
	}

	if(*dstBuf && *dstSize>0) memset(*dstBuf, 0, *dstSize);
	mm_util_debug("Done.");
	*dstSize=TJBUFSIZE(w, h);
	if(fmt==MM_UTIL_JPEG_FMT_RGB888) {
		_tj(tjCompress2(handle, srcBuf, w, 0, h, TJPF_RGB, dstBuf, dstSize, TJPF_RGB, jpegQual, flags));
		mm_util_debug("*dstSize: %d", *dstSize);
	} else if(fmt==MM_UTIL_JPEG_FMT_YUV420) {
		*dstSize=TJBUFSIZEYUV(w, h, TJSAMP_420);
		_tj(tjEncodeYUV2(handle, srcBuf, w, 0, h, TJPF_RGB, *dstBuf, TJSAMP_420, flags));
		mm_util_debug("*dstSize: %d \t decode_yuv_subsample: %d TJBUFSIZE(w, h):%d", *dstSize, TJSAMP_420, TJBUFSIZE(w, h));
	} else if(fmt==MM_UTIL_JPEG_FMT_YUV422) {
		*dstSize=TJBUFSIZEYUV(w, h, TJSAMP_422);
		_tj(tjEncodeYUV2(handle, srcBuf, w, 0, h, TJPF_RGB, *dstBuf, TJSAMP_422, flags));
		mm_util_debug("*dstSize: %d \t decode_yuv_subsample: %d TJBUFSIZE(w, h):%d", *dstSize, TJSAMP_422, TJBUFSIZE(w, h));
	} else {
		mm_util_error("fmt:%d  is wrong", fmt);
	}

	IMG_JPEG_FREE(srcBuf);
}

static int
mm_image_encode_to_jpeg_file_with_libjpeg_turbo(char *filename, void* src, int width, int height, mm_util_jpeg_yuv_format fmt, int quality)
{
	int iErrorCode = MM_UTIL_ERROR_NONE;
	tjhandle chandle=NULL;
	unsigned char *dstBuf=NULL;
	unsigned long size=0;
	int TD_BU=0;
	FILE *fout=fopen(filename, "w+");
	if(fout == NULL) {
		mm_util_error("FILE OPEN FAIL [%s] failed", filename);
		mm_util_stderror("FILE OPEN FAIL");
		return MM_UTIL_ERROR_NO_SUCH_FILE;
	}
	mm_util_debug("fmt: %d", fmt);

	size=TJBUFSIZE(width, height);
	if((dstBuf=(unsigned char *)tjAlloc(size))==NULL) {
		fclose(fout);
		mm_util_error("dstBuf is NULL");
		return MM_UTIL_ERROR_OUT_OF_MEMORY;
	}

	if((chandle=tjInitCompress())==NULL) {
		fclose(fout);
		tjFree(dstBuf);
		mm_util_error("dstBuf is NULL");
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}
	_mm_encode_libjpeg_turbo_compress(chandle, src, &dstBuf, &size, width, height, quality, TD_BU, fmt);
	mm_util_debug("dstBuf: %p\t size: %d", dstBuf, size);
	fwrite(dstBuf, 1, size, fout);
	fclose(fout);
	if(chandle) {
		tjDestroy(chandle);
	}
	if(dstBuf) {
		tjFree(dstBuf);
	}
	mm_util_debug("Done");
	return iErrorCode;
}

static int
mm_image_encode_to_jpeg_memory_with_libjpeg_turbo(void **mem, int *csize, void *rawdata,  int w, int h, mm_util_jpeg_yuv_format fmt,int quality)
{
	int iErrorCode = MM_UTIL_ERROR_NONE;
	tjhandle chandle=NULL;
	int TD_BU=0;

	*csize=TJBUFSIZE(w, h);
	if(fmt==MM_UTIL_JPEG_FMT_RGB888) {
		mm_util_debug("MM_UTIL_JPEG_FMT_RGB888 size: %d", *csize);
	}else if(fmt==MM_UTIL_JPEG_FMT_YUV420 ||fmt==MM_UTIL_JPEG_FMT_YUV422) {
		mm_util_debug("TJSAMP_420 ||TJSAMP_422 size: %d",*csize);
	}else {
			mm_util_error("We can't support the IMAGE format");
			return MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	}

	if((*mem=(unsigned char *)tjAlloc(*csize))==NULL) {
		mm_util_error("dstBuf is NULL");
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if((chandle=tjInitCompress())==NULL) {
		tjFree(*mem);
		mm_util_error("chandle is NULL");
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	mm_util_debug("width: %d height: %d, size: %d", w, h, *csize);
	_mm_encode_libjpeg_turbo_compress(chandle, rawdata, mem, csize, w, h, quality, TD_BU, fmt);
	mm_util_debug("dstBuf: %p &dstBuf:%p size: %d", *mem, mem, *csize);
	if(chandle) {
		tjDestroy(chandle);
	}
	return iErrorCode;
}
#endif

/* OPEN SOURCE */
struct my_error_mgr_s
{
	struct jpeg_error_mgr pub; /* "public" fields */
	jmp_buf setjmp_buffer; /* for return to caller */
} my_error_mgr_s;

typedef struct my_error_mgr_s * my_error_ptr;

struct {
	struct jpeg_destination_mgr pub; /* public fields */
	FILE * outfile; /* target stream */
	JOCTET * buffer; /* start of buffer */
} my_destination_mgr_s;

typedef struct  my_destination_mgr_s * my_dest_ptr;

#define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */

/* Expanded data destination object for memory output */

struct {
	struct jpeg_destination_mgr pub; /* public fields */

	unsigned char ** outbuffer; /* target buffer */
	unsigned long * outsize;
	unsigned char * newbuffer; /* newly allocated buffer */
	JOCTET * buffer; /* start of buffer */
	size_t bufsize;
} my_mem_destination_mgr;

typedef struct my_mem_destination_mgr * my_mem_dest_ptr;

static void
my_error_exit (j_common_ptr cinfo)
{
	my_error_ptr myerr = (my_error_ptr) cinfo->err; /* cinfo->err really points to a my_error_mgr_s struct, so coerce pointer */
	(*cinfo->err->output_message) (cinfo); /*  Always display the message. We could postpone this until after returning, if we chose. */
	longjmp(myerr->setjmp_buffer, 1); /*  Return control to the setjmp point */
}

static int mm_image_encode_to_jpeg_file_with_libjpeg(const char *pFileName, void *rawdata, int width, int height, mm_util_jpeg_yuv_format fmt, int quality)
{
	int iErrorCode = MM_UTIL_ERROR_NONE;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	int i, j, flag, _height;
	FILE * fpWriter;

	JSAMPROW y[16],cb[16],cr[16]; /* y[2][5] = color sample of row 2 and pixel column 5; (one plane) */
	JSAMPARRAY data[3]; /* t[0][2][5] = color sample 0 of row 2 and column 5 */
	if(!pFileName) {
		mm_util_error("pFileName");
		return MM_UTIL_ERROR_NO_SUCH_FILE;
	}

	data[0] = y;
	data[1] = cb;
	data[2] = cr;

	mm_util_debug("Enter");

	cinfo.err = jpeg_std_error(&jerr);

	jpeg_create_compress(&cinfo);

	if ((fpWriter = fopen(pFileName, "wb")) == NULL) {
		mm_util_error("[infile] file open [%s] failed", pFileName);
		mm_util_stderror("file open failed");
		return MM_UTIL_ERROR_NO_SUCH_FILE;
	}

	jpeg_stdio_dest(&cinfo, fpWriter);
	cinfo.image_width = width;
	cinfo.image_height = height;
	if (fmt ==MM_UTIL_JPEG_FMT_YUV420 ||fmt ==MM_UTIL_JPEG_FMT_YUV422 || fmt ==MM_UTIL_JPEG_FMT_UYVY) {
		mm_util_debug("[mm_image_encode_to_jpeg_file_with_libjpeg] jpeg_stdio_dest for YUV");
		mm_util_debug("[Height] %d", height);
		_height = MM_JPEG_ROUND_DOWN_16(height);
		flag = height - _height;

		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_YCbCr;
		mm_util_debug("JCS_YCbCr [%d] ", flag);

		jpeg_set_defaults(&cinfo);
		mm_util_debug("jpeg_set_defaults");

		cinfo.raw_data_in = TRUE; /*  Supply downsampled data */
		cinfo.do_fancy_downsampling = FALSE;

		cinfo.comp_info[0].h_samp_factor = 2;
		if (fmt ==MM_UTIL_JPEG_FMT_YUV420) {
			cinfo.comp_info[0].v_samp_factor = 2;
		} else if (fmt ==MM_UTIL_JPEG_FMT_YUV422 || fmt ==MM_UTIL_JPEG_FMT_UYVY) {
			cinfo.comp_info[0].v_samp_factor = 1;
		}
		cinfo.comp_info[1].h_samp_factor = 1;
		cinfo.comp_info[1].v_samp_factor = 1;
		cinfo.comp_info[2].h_samp_factor = 1;
		cinfo.comp_info[2].v_samp_factor = 1;

		jpeg_set_quality(&cinfo, quality, TRUE);
		mm_util_debug("jpeg_set_quality");
		cinfo.dct_method = JDCT_FASTEST;

		jpeg_start_compress(&cinfo, TRUE);
		mm_util_debug("jpeg_start_compress");

		if(flag) {
			void *large_rect = malloc (width);
			void *small_rect = malloc (width);
			if(large_rect) {
				memset(large_rect, 0x10, width);
			} else {
				IMG_JPEG_FREE(small_rect);
				mm_util_error("large rectangle memory");
				return MM_UTIL_ERROR_INVALID_PARAMETER;
			}
			if(small_rect) {
				memset(small_rect, 0x80, width);
			} else {
				IMG_JPEG_FREE(large_rect);
				mm_util_error("small rectangle memory");
				return MM_UTIL_ERROR_INVALID_PARAMETER;
			}

			for (j = 0; j < _height; j += 16) {
				for (i = 0; i < 16; i++) {
					y[i] = (JSAMPROW)rawdata + width * (i + j);
					if (i % 2 == 0) {
						cb[i / 2] = (JSAMPROW)rawdata + width * height + width / 2 * ((i + j) / 2);
						cr[i / 2] = (JSAMPROW)rawdata + width * height + width * height / 4 + width / 2 * ((i + j) / 2);
					}
				}
				jpeg_write_raw_data(&cinfo, data, 16);
			}
			for (i = 0; i < flag; i++) {
				y[i] = (JSAMPROW)rawdata + width * (i + j);
				if (i % 2 == 0) {
					cb[i / 2] = (JSAMPROW)rawdata + width * height + width / 2 * ((i + j) / 2);
					cr[i / 2] = (JSAMPROW)rawdata + width * height + width * height / 4 + width / 2 * ((i + j) / 2);
				}
			}
			for (; i < 16; i++) {
				y[i] = (JSAMPROW)large_rect;
				if (i % 2 == 0) {
					cb[i / 2] = (JSAMPROW)small_rect;
					cr[i / 2] = (JSAMPROW)small_rect;
				}
			}
			jpeg_write_raw_data(&cinfo, data, 16);
			IMG_JPEG_FREE(large_rect);
			IMG_JPEG_FREE(small_rect);
		} else {
			for (j = 0; j < height; j += 16) {
				for (i = 0; i < 16; i++) {
					y[i] = (JSAMPROW)rawdata + width * (i + j);
					if (i % 2 == 0) {
						cb[i / 2] = (JSAMPROW)rawdata + width * height + width / 2 * ((i + j) / 2);
						cr[i / 2] = (JSAMPROW)rawdata + width * height + width * height / 4 + width / 2 * ((i + j) / 2);
					}
				}
				jpeg_write_raw_data(&cinfo, data, 16);
			}
		}
		mm_util_debug("#for loop#");

		jpeg_finish_compress(&cinfo);
		mm_util_debug("jpeg_finish_compress");

		jpeg_destroy_compress(&cinfo);
		mm_util_debug("jpeg_destroy_compress");
	} 

	else if (fmt == MM_UTIL_JPEG_FMT_RGB888 ||fmt == MM_UTIL_JPEG_FMT_GraySacle || fmt == MM_UTIL_JPEG_FMT_RGBA8888 || fmt == MM_UTIL_JPEG_FMT_BGRA8888 || fmt == MM_UTIL_JPEG_FMT_ARGB8888) {
		JSAMPROW row_pointer[1];
		int iRowStride = 0;

		if(fmt == MM_UTIL_JPEG_FMT_RGB888) {
			cinfo.input_components = 3;
			cinfo.in_color_space = JCS_RGB;
			mm_util_debug("JCS_RGB");
		} else if(fmt == MM_UTIL_JPEG_FMT_GraySacle) {
			cinfo.input_components = 1; /* one colour component */
			cinfo.in_color_space = JCS_GRAYSCALE;
			mm_util_debug("JCS_GRAYSCALE");
		} else if(fmt == MM_UTIL_JPEG_FMT_RGBA8888) {
			cinfo.input_components = 4; /* one colour component */
			cinfo.in_color_space = JCS_EXT_RGBA;
			mm_util_debug("JCS_EXT_RGBA");
		} else if(fmt == MM_UTIL_JPEG_FMT_BGRA8888) {
			cinfo.input_components = 4;
			cinfo.in_color_space = JCS_EXT_BGRA;
			mm_util_debug("JCS_EXT_BGRA");
		} else if(fmt == MM_UTIL_JPEG_FMT_ARGB8888) {
			cinfo.input_components = 4;
			cinfo.in_color_space = JCS_EXT_ARGB;
			mm_util_debug("JCS_EXT_ARGB");
		}

		jpeg_set_defaults(&cinfo);
		mm_util_debug("jpeg_set_defaults");
		jpeg_set_quality(&cinfo, quality, TRUE);
		mm_util_debug("jpeg_set_quality");
		jpeg_start_compress(&cinfo, TRUE);
		mm_util_debug("jpeg_start_compress");
		if(fmt == MM_UTIL_JPEG_FMT_RGB888) {
			iRowStride = width * 3;
		} else if(fmt == MM_UTIL_JPEG_FMT_GraySacle) {
			iRowStride = width;
		} else if(fmt == MM_UTIL_JPEG_FMT_RGBA8888 || fmt == MM_UTIL_JPEG_FMT_BGRA8888 || fmt == MM_UTIL_JPEG_FMT_ARGB8888) {
			iRowStride = width * 4;
		}

		JSAMPLE *image_buffer = (JSAMPLE *)rawdata;
		while (cinfo.next_scanline < cinfo.image_height) {
			//row_pointer[0] = (JSAMPROW)&rawdata[cinfo.next_scanline * iRowStride];
			row_pointer[0] = & image_buffer[cinfo.next_scanline * iRowStride];
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
		mm_util_debug("while");
		jpeg_finish_compress(&cinfo);
		mm_util_debug("jpeg_finish_compress");

		jpeg_destroy_compress(&cinfo);
		mm_util_debug("jpeg_destroy_compress");
	} else {
		mm_util_error("We can't encode the IMAGE format");
		return MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	}
	fsync((int)(fpWriter->_fileno));
	mm_util_debug("[fsync] FILE");
	fclose(fpWriter);
	return iErrorCode;
}

static int
mm_image_encode_to_jpeg_memory_with_libjpeg(void **mem, int *csize, void *rawdata, int width, int height, mm_util_jpeg_yuv_format fmt,int quality)
{
	int iErrorCode = MM_UTIL_ERROR_NONE;

	JSAMPROW y[16],cb[16],cr[16]; /*  y[2][5] = color sample of row 2 and pixel column 5; (one plane) */
	JSAMPARRAY data[3]; /*  t[0][2][5] = color sample 0 of row 2 and column 5 */

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	int i, j, flag, _height;
	unsigned long size = 0;
	*csize = 0;
	data[0] = y;
	data[1] = cb;
	data[2] = cr;

	mm_util_debug("Enter");
	mm_util_debug("#Before Enter#, mem: %p\t rawdata:%p\t width: %d\t height: %d\t fmt: %d\t quality: %d"
		, mem, rawdata, width, height, fmt, quality);

	if(rawdata == NULL) {
		mm_util_error("Exit on error -rawdata");
		return MM_UTIL_ERROR_OUT_OF_MEMORY;
	}

	if (mem == NULL) {
		mm_util_error("Exit on error -mem");
		return MM_UTIL_ERROR_OUT_OF_MEMORY;
	}

	cinfo.err = jpeg_std_error(&jerr); /*  Errors get written to stderr */

	jpeg_create_compress(&cinfo);

	mm_util_debug("[mm_image_encode_to_jpeg_memory_with_libjpeg] #After jpeg_mem_dest#, mem: %p\t width: %d\t height: %d\t fmt: %d\t quality: %d"
		, mem, width, height, fmt, quality);

	jpeg_mem_dest(&cinfo, (unsigned char **)mem, &size);
	cinfo.image_width = width;
	cinfo.image_height = height;
	if (fmt ==MM_UTIL_JPEG_FMT_YUV420 ||fmt ==MM_UTIL_JPEG_FMT_YUV422 || fmt ==MM_UTIL_JPEG_FMT_UYVY) {
		mm_util_debug("[mm_image_encode_to_jpeg_file_with_libjpeg] jpeg_stdio_dest for YUV");
		_height = MM_JPEG_ROUND_DOWN_16(height);
		flag = height - _height;

		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_YCbCr;
		jpeg_set_defaults(&cinfo);
		mm_util_debug("jpeg_set_defaults");

		cinfo.raw_data_in = TRUE; /* Supply downsampled data */
		cinfo.do_fancy_downsampling = FALSE;

		cinfo.comp_info[0].h_samp_factor = 2;
		if (fmt ==MM_UTIL_JPEG_FMT_YUV420) {
			cinfo.comp_info[0].v_samp_factor = 2;
		} else if (fmt ==MM_UTIL_JPEG_FMT_YUV422 || fmt ==MM_UTIL_JPEG_FMT_UYVY) {
			cinfo.comp_info[0].v_samp_factor = 1;
		}
		cinfo.comp_info[1].h_samp_factor = 1;
		cinfo.comp_info[1].v_samp_factor = 1;
		cinfo.comp_info[2].h_samp_factor = 1;
		cinfo.comp_info[2].v_samp_factor = 1;

		jpeg_set_quality(&cinfo, quality, TRUE);
		mm_util_debug("jpeg_set_quality");
		cinfo.dct_method = JDCT_FASTEST;

		jpeg_start_compress(&cinfo, TRUE);
		mm_util_debug("jpeg_start_compress");

		if(flag) {
			void *large_rect = malloc (width);
			void *small_rect = malloc (width);
			if(large_rect) {
				memset(large_rect, 0x10, width);
			} else {
				IMG_JPEG_FREE(small_rect);
				mm_util_error("large rectangle memory");
				return MM_UTIL_ERROR_INVALID_PARAMETER;
			}
			if(small_rect) {
				memset(small_rect, 0x80, width);
			} else {
				IMG_JPEG_FREE(large_rect);
				mm_util_error("small rectangle memory");
				return MM_UTIL_ERROR_INVALID_PARAMETER;
			}

			for (j = 0; j < _height; j += 16) {
				for (i = 0; i < 16; i++) {
					y[i] = (JSAMPROW)rawdata + width * (i + j);
					if (i % 2 == 0) {
						cb[i / 2] = (JSAMPROW)rawdata + width * height + width / 2 * ((i + j) / 2);
						cr[i / 2] = (JSAMPROW)rawdata + width * height + width * height / 4 + width / 2 * ((i + j) / 2);
					}
				}
				jpeg_write_raw_data(&cinfo, data, 16);
			}
			for (i = 0; i < flag; i++) {
				y[i] = (JSAMPROW)rawdata + width * (i + j);
				if (i % 2 == 0) {
					cb[i / 2] = (JSAMPROW)rawdata + width * height + width / 2 * ((i + j) / 2);
					cr[i / 2] = (JSAMPROW)rawdata + width * height + width * height / 4 + width / 2 * ((i + j) / 2);
				}
			}
			for (; i < 16; i++) {
				y[i] = (JSAMPROW)large_rect;
				if (i % 2 == 0) {
					cb[i / 2] = (JSAMPROW)small_rect;
					cr[i / 2] = (JSAMPROW)small_rect;
				}
			}
			jpeg_write_raw_data(&cinfo, data, 16);
			IMG_JPEG_FREE(large_rect);
			IMG_JPEG_FREE(small_rect);
		} else {
			for (j = 0; j < height; j += 16) {
				for (i = 0; i < 16; i++) {
					y[i] = (JSAMPROW)rawdata + width * (i + j);
					if (i % 2 == 0) {
						cb[i / 2] = (JSAMPROW)rawdata + width * height + width / 2 * ((i + j) / 2);
						cr[i / 2] = (JSAMPROW)rawdata + width * height + width * height / 4 + width / 2 * ((i + j) / 2);
					}
				}
				jpeg_write_raw_data(&cinfo, data, 16);
			}
		}
		mm_util_debug("#for loop#");

		jpeg_finish_compress(&cinfo);
		mm_util_debug("jpeg_finish_compress");
		jpeg_destroy_compress(&cinfo);
		mm_util_debug("Exit jpeg_destroy_compress, mem: %p\t size:%d", *mem, *csize);
	}

	else if (fmt == MM_UTIL_JPEG_FMT_RGB888 ||fmt == MM_UTIL_JPEG_FMT_GraySacle || fmt == MM_UTIL_JPEG_FMT_RGBA8888 || fmt == MM_UTIL_JPEG_FMT_BGRA8888 || fmt == MM_UTIL_JPEG_FMT_ARGB8888) {
		JSAMPROW row_pointer[1];
		int iRowStride = 0;

		mm_util_debug("MM_UTIL_JPEG_FMT_RGB888");
		if(fmt == MM_UTIL_JPEG_FMT_RGB888) {
			cinfo.input_components = 3;
			cinfo.in_color_space = JCS_RGB;
			mm_util_debug("JCS_RGB");
		} else if(fmt == MM_UTIL_JPEG_FMT_GraySacle) {
			cinfo.input_components = 1; /* one colour component */
			cinfo.in_color_space = JCS_GRAYSCALE;
			mm_util_debug("JCS_GRAYSCALE");
		} else if(fmt == MM_UTIL_JPEG_FMT_RGBA8888) {
			cinfo.input_components = 4;
			cinfo.in_color_space = JCS_EXT_RGBA;
			mm_util_debug("JCS_EXT_RGBA");
		} else if(fmt == MM_UTIL_JPEG_FMT_BGRA8888) {
			cinfo.input_components = 4;
			cinfo.in_color_space = JCS_EXT_BGRA;
			mm_util_debug("JCS_EXT_BGRA");
		} else if(fmt == MM_UTIL_JPEG_FMT_ARGB8888) {
			cinfo.input_components = 4;
			cinfo.in_color_space = JCS_EXT_ARGB;
			mm_util_debug("JCS_EXT_ARGB");
		}

		jpeg_set_defaults(&cinfo);
		mm_util_debug("jpeg_set_defaults");
		jpeg_set_quality(&cinfo, quality, TRUE);
		mm_util_debug("jpeg_set_quality");
		jpeg_start_compress(&cinfo, TRUE);
		mm_util_debug("jpeg_start_compress");
		if(fmt == MM_UTIL_JPEG_FMT_RGB888) {
			iRowStride = width * 3;
		} else if(fmt == MM_UTIL_JPEG_FMT_GraySacle) {
			iRowStride = width;
		} else if(fmt == MM_UTIL_JPEG_FMT_RGBA8888 || fmt == MM_UTIL_JPEG_FMT_BGRA8888 || fmt == MM_UTIL_JPEG_FMT_ARGB8888) {
			iRowStride = width * 4;
		}

		JSAMPLE *image_buffer = (JSAMPLE *)rawdata;
		while (cinfo.next_scanline < cinfo.image_height) {
			//row_pointer[0] = (JSAMPROW)&rawdata[cinfo.next_scanline * iRowStride];
			row_pointer[0] = & image_buffer[cinfo.next_scanline * iRowStride];
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
		mm_util_debug("while");
		jpeg_finish_compress(&cinfo);
		mm_util_debug("jpeg_finish_compress");

		jpeg_destroy_compress(&cinfo);

		mm_util_debug("Exit");
	}	else {
		mm_util_error("We can't encode the IMAGE format");
		return MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	}
	*csize = (int)size;
	return iErrorCode;
}

static int
mm_image_decode_from_jpeg_file_with_libjpeg(mm_util_jpeg_yuv_data * decoded_data, const char *pFileName, mm_util_jpeg_yuv_format input_fmt, mm_util_jpeg_decode_downscale downscale)
{
	int iErrorCode = MM_UTIL_ERROR_NONE;
	FILE *infile = NULL;
	struct jpeg_decompress_struct dinfo;
	struct my_error_mgr_s jerr;
	JSAMPARRAY buffer; /* Output row buffer */
	int row_stride = 0; /* physical row width in output buffer */
	JSAMPROW image, u_image, v_image;
	JSAMPROW row; /* point to buffer[0] */

	mm_util_debug("Enter");

	if(!pFileName) {
		mm_util_error("pFileName");
		return MM_UTIL_ERROR_NO_SUCH_FILE;
	}
	if(decoded_data == NULL) {
		mm_util_error("decoded_data");
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}
	if(decoded_data) {
		decoded_data->data = NULL;
	}

	infile = fopen(pFileName, "rb");
	if(infile == NULL) {
		mm_util_error("[infile] file open [%s]", pFileName);
		mm_util_stderror("file open failed");
		return MM_UTIL_ERROR_NO_SUCH_FILE;
	}
	mm_util_debug("infile");
	/* allocate and initialize JPEG decompression object   We set up the normal JPEG error routines, then override error_exit. */
	dinfo.err = jpeg_std_error(&jerr.pub);
	mm_util_debug("jpeg_std_error");
	jerr.pub.error_exit = my_error_exit;
	mm_util_debug("jerr.pub.error_exit ");

	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error. We need to clean up the JPEG object, close the input file, and return.*/
		mm_util_debug("setjmp");
		jpeg_destroy_decompress(&dinfo);
		IMG_JPEG_FREE(decoded_data->data);
		fclose(infile);
		mm_util_debug("fclose");
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	mm_util_debug("if(setjmp)");
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&dinfo);
	mm_util_debug("jpeg_create_decompress");

	/*specify data source (eg, a file) */
	jpeg_stdio_src(&dinfo, infile);
	mm_util_debug("jpeg_stdio_src");

	/*read file parameters with jpeg_read_header() */
	jpeg_read_header(&dinfo, TRUE);
#if PARTIAL_DECODE
	mm_util_debug("image width: %d height: %d", dinfo.image_width, dinfo.image_height);
	if(dinfo.image_width > ENC_MAX_LEN || dinfo.image_height > ENC_MAX_LEN) {
		dinfo.scale_num = 1;
		dinfo.scale_denom = 8;
		dinfo.do_fancy_upsampling = FALSE;
		dinfo.do_block_smoothing = FALSE;
		dinfo.dither_mode = JDITHER_ORDERED;
	} else if (downscale != MM_UTIL_JPEG_DECODE_DOWNSCALE_1_1) {
		dinfo.scale_num = 1;
		dinfo.scale_denom = (unsigned int)downscale;
		dinfo.do_fancy_upsampling = FALSE;
		dinfo.do_block_smoothing = FALSE;
		dinfo.dither_mode = JDITHER_ORDERED;
	}
#endif
	dinfo.dct_method = JDCT_FASTEST;

	/* set parameters for decompression */
	if (input_fmt == MM_UTIL_JPEG_FMT_RGB888) {
		dinfo.out_color_space=JCS_RGB;
		mm_util_debug("cinfo.out_color_space=JCS_RGB");
	} else if(input_fmt == MM_UTIL_JPEG_FMT_YUV420 || input_fmt == MM_UTIL_JPEG_FMT_YUV422 || input_fmt == MM_UTIL_JPEG_FMT_UYVY) {
		dinfo.out_color_space=JCS_YCbCr;
		mm_util_debug("cinfo.out_color_space=JCS_YCbCr");
	} else if(input_fmt == MM_UTIL_JPEG_FMT_GraySacle) {
		dinfo.out_color_space=JCS_GRAYSCALE;
		mm_util_debug("cinfo.out_color_space=JCS_GRAYSCALE");
	} else if(input_fmt == MM_UTIL_JPEG_FMT_RGBA8888) {
		dinfo.out_color_space=JCS_EXT_RGBA;
		mm_util_debug("cinfo.out_color_space=JCS_EXT_RGBA");
	} else if(input_fmt == MM_UTIL_JPEG_FMT_BGRA8888) {
		dinfo.out_color_space=JCS_EXT_BGRA;
		mm_util_debug("cinfo.out_color_space=JCS_EXT_BGRA");
	} else if(input_fmt == MM_UTIL_JPEG_FMT_ARGB8888) {
		dinfo.out_color_space=JCS_EXT_ARGB;
		mm_util_debug("cinfo.out_color_space=JCS_EXT_ARGB");
	}
	decoded_data->format = input_fmt;

	/* Start decompressor*/
	jpeg_start_decompress(&dinfo);

	/* byte-align for YUV format */
	if (input_fmt == MM_UTIL_JPEG_FMT_YUV420 || input_fmt == MM_UTIL_JPEG_FMT_YUV422) {
		if(dinfo.output_width% 2 != 0) {
			dinfo.output_width = MM_JPEG_ROUND_DOWN_2(dinfo.output_width);
		}
		if(dinfo.output_height% 2 != 0) {
			dinfo.output_height = MM_JPEG_ROUND_DOWN_2(dinfo.output_height);
		}
	}

	/* JSAMPLEs per row in output buffer */
	row_stride = dinfo.output_width * dinfo.output_components;

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*dinfo.mem->alloc_sarray) ((j_common_ptr) &dinfo, JPOOL_IMAGE, row_stride, 1);
	mm_util_debug("JPOOL_IMAGE BUFFER");
	decoded_data->width = dinfo.output_width;
	decoded_data->height= dinfo.output_height;
	if(input_fmt == MM_UTIL_JPEG_FMT_RGB888 || input_fmt == MM_UTIL_JPEG_FMT_RGBA8888 || input_fmt == MM_UTIL_JPEG_FMT_BGRA8888 || input_fmt == MM_UTIL_JPEG_FMT_ARGB8888) {
		decoded_data->size= dinfo.output_height * row_stride;
	} else if(input_fmt == MM_UTIL_JPEG_FMT_YUV420) {
		decoded_data->size= dinfo.output_height * row_stride / 2;
	} else if(input_fmt == MM_UTIL_JPEG_FMT_YUV422 || input_fmt == MM_UTIL_JPEG_FMT_UYVY) {
		decoded_data->size= dinfo.output_height * dinfo.output_width * 2;
	} else if(input_fmt == MM_UTIL_JPEG_FMT_GraySacle) {
		decoded_data->size= dinfo.output_height * dinfo.output_width;
	} else{
		jpeg_destroy_decompress(&dinfo);
		mm_util_error("[%d] We can't decode the IMAGE format", input_fmt);
		fclose(infile);
		return MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	}

	decoded_data->data= (void*) g_malloc0(decoded_data->size);
	decoded_data->format = input_fmt;

	if(decoded_data->data == NULL) {
		jpeg_destroy_decompress(&dinfo);
		mm_util_error("decoded_data->data is NULL");
		fclose(infile);
		return MM_UTIL_ERROR_OUT_OF_MEMORY;
	}
	mm_util_debug("decoded_data->data");

	if(input_fmt == MM_UTIL_JPEG_FMT_YUV420 || input_fmt == MM_UTIL_JPEG_FMT_YUV422 || input_fmt == MM_UTIL_JPEG_FMT_UYVY) {
		image = decoded_data->data;
		u_image = image + (dinfo.output_width * dinfo.output_height);
		v_image = u_image + (dinfo.output_width*dinfo.output_height) / 4;
		row= buffer[0];
		int i = 0;
		int y = 0;
		while (dinfo.output_scanline < dinfo.output_height) {
			jpeg_read_scanlines(&dinfo, buffer, 1);
			for (i = 0; i < row_stride; i += 3) {
				image[i/3]=row[i];
				if (i & 1) {
					u_image[(i/3)/2]=row[i+1];
					v_image[(i/3)/2]=row[i+2];
				}
			}
			image += row_stride/3;
			if (y++ & 1) {
				u_image += dinfo.output_width / 2;
				v_image += dinfo.output_width / 2;
			}
		}
	}else if(input_fmt == MM_UTIL_JPEG_FMT_RGB888 ||input_fmt == MM_UTIL_JPEG_FMT_GraySacle || input_fmt == MM_UTIL_JPEG_FMT_RGBA8888 || input_fmt == MM_UTIL_JPEG_FMT_BGRA8888 || input_fmt == MM_UTIL_JPEG_FMT_ARGB8888) {
		int state = 0;
		/* while (scan lines remain to be read) jpeg_read_scanlines(...); */
		while (dinfo.output_scanline < dinfo.output_height) {
			/* jpeg_read_scanlines expects an array of pointers to scanlines. Here the array is only one element long, but you could ask formore than one scanline at a time if that's more convenient. */
			jpeg_read_scanlines(&dinfo, buffer, 1);
			memcpy(decoded_data->data + state, buffer[0], row_stride);
			state += row_stride;
		}
		mm_util_debug("jpeg_read_scanlines");
	}

	/* Finish decompression */
	jpeg_finish_decompress(&dinfo);

	/* Release JPEG decompression object */
	jpeg_destroy_decompress(&dinfo);

	fclose(infile);
	mm_util_debug("fclose");

	return iErrorCode;
}

static int
mm_image_decode_from_jpeg_memory_with_libjpeg(mm_util_jpeg_yuv_data * decoded_data, void *src, int size, mm_util_jpeg_yuv_format input_fmt, mm_util_jpeg_decode_downscale downscale)
{
	int iErrorCode = MM_UTIL_ERROR_NONE;
	struct jpeg_decompress_struct dinfo;
	struct my_error_mgr_s jerr;
	JSAMPARRAY buffer; /* Output row buffer */
	int row_stride = 0; /* physical row width in output buffer */
	JSAMPROW image, u_image, v_image;
	JSAMPROW row; /* point to buffer[0] */

	mm_util_debug("Enter");

	if(src == NULL) {
		mm_util_error("[infile] Exit on error");
		return MM_UTIL_ERROR_NO_SUCH_FILE;
	}
	mm_util_debug("infile");
	if(decoded_data == NULL) {
		mm_util_error("decoded_data");
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}
	if(decoded_data) {
		decoded_data->data = NULL;
	}

	/* allocate and initialize JPEG decompression object   We set up the normal JPEG error routines, then override error_exit. */
	dinfo.err = jpeg_std_error(&jerr.pub);
	mm_util_debug("jpeg_std_error ");
	jerr.pub.error_exit = my_error_exit;
	mm_util_debug("jerr.pub.error_exit ");

	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.  We need to clean up the JPEG object, close the input file, and return.*/
		mm_util_debug("setjmp");
		IMG_JPEG_FREE(decoded_data->data);
		jpeg_destroy_decompress(&dinfo);
		return MM_UTIL_ERROR_NO_SUCH_FILE;
	}

	mm_util_debug("if(setjmp)");
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&dinfo);
	mm_util_debug("jpeg_create_decompress");

	/*specify data source (eg, a file) */
	jpeg_mem_src(&dinfo, src, size);
	mm_util_debug("jpeg_stdio_src");

	/*read file parameters with jpeg_read_header() */
	jpeg_read_header(&dinfo, TRUE);
	mm_util_debug("jpeg_read_header");
#if PARTIAL_DECODE
	mm_util_debug("image width: %d height: %d", dinfo.image_width, dinfo.image_height);
	if(dinfo.image_width > ENC_MAX_LEN || dinfo.image_height > ENC_MAX_LEN) {
		dinfo.scale_num = 1;
		dinfo.scale_denom = 8;
		dinfo.do_fancy_upsampling = FALSE;
		dinfo.do_block_smoothing = FALSE;
		dinfo.dither_mode = JDITHER_ORDERED;
	} else if (downscale != MM_UTIL_JPEG_DECODE_DOWNSCALE_1_1) {
		dinfo.scale_num = 1;
		dinfo.scale_denom = (unsigned int)downscale;
		dinfo.do_fancy_upsampling = FALSE;
		dinfo.do_block_smoothing = FALSE;
		dinfo.dither_mode = JDITHER_ORDERED;
	}
#endif
	dinfo.dct_method = JDCT_FASTEST;

	/* set parameters for decompression */
	if (input_fmt == MM_UTIL_JPEG_FMT_RGB888) {
		dinfo.out_color_space=JCS_RGB;
		mm_util_debug("cinfo.out_color_space=JCS_RGB");
	} else if(input_fmt == MM_UTIL_JPEG_FMT_YUV420 || input_fmt == MM_UTIL_JPEG_FMT_YUV422 || input_fmt == MM_UTIL_JPEG_FMT_UYVY) {
		dinfo.out_color_space=JCS_YCbCr;
		mm_util_debug("cinfo.out_color_space=JCS_YCbCr");
	} else if(input_fmt == MM_UTIL_JPEG_FMT_GraySacle) {
		dinfo.out_color_space=JCS_GRAYSCALE;
		mm_util_debug("cinfo.out_color_space=JCS_GRAYSCALE");
	} else if(input_fmt == MM_UTIL_JPEG_FMT_RGBA8888) {
		dinfo.out_color_space=JCS_EXT_RGBA;
		mm_util_debug("cinfo.out_color_space=JCS_EXT_RGBA");
	} else if(input_fmt == MM_UTIL_JPEG_FMT_BGRA8888) {
		dinfo.out_color_space=JCS_EXT_BGRA;
		mm_util_debug("cinfo.out_color_space=JCS_EXT_BGRA");
	} else if(input_fmt == MM_UTIL_JPEG_FMT_ARGB8888) {
		dinfo.out_color_space=JCS_EXT_ARGB;
		mm_util_debug("cinfo.out_color_space=JCS_EXT_ARGB");
	}
	decoded_data->format = input_fmt;

	/* Start decompressor*/
	jpeg_start_decompress(&dinfo);
	mm_util_debug("jpeg_start_decompress");

	/* byte-align for YUV format */
	if (input_fmt == MM_UTIL_JPEG_FMT_YUV420 || input_fmt == MM_UTIL_JPEG_FMT_YUV422) {
		if(dinfo.output_width% 2 != 0) {
			dinfo.output_width = MM_JPEG_ROUND_DOWN_2(dinfo.output_width);
		}
		if(dinfo.output_height% 2 != 0) {
			dinfo.output_height = MM_JPEG_ROUND_DOWN_2(dinfo.output_height);
		}
	}

	/* JSAMPLEs per row in output buffer */
	row_stride = dinfo.output_width * dinfo.output_components;

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*dinfo.mem->alloc_sarray) ((j_common_ptr) &dinfo, JPOOL_IMAGE, row_stride, 1);
	mm_util_debug("JPOOL_IMAGE BUFFER");
	decoded_data->width = dinfo.output_width;
	decoded_data->height= dinfo.output_height;
	if(input_fmt == MM_UTIL_JPEG_FMT_RGB888 || input_fmt == MM_UTIL_JPEG_FMT_RGBA8888 || input_fmt == MM_UTIL_JPEG_FMT_BGRA8888 || input_fmt == MM_UTIL_JPEG_FMT_ARGB8888) {
		decoded_data->size= dinfo.output_height * row_stride;
	} else if(input_fmt == MM_UTIL_JPEG_FMT_YUV420) {
		decoded_data->size= dinfo.output_height * row_stride / 2;
	} else if(input_fmt == MM_UTIL_JPEG_FMT_YUV422 || input_fmt == MM_UTIL_JPEG_FMT_UYVY) {
		decoded_data->size= dinfo.output_height * dinfo.output_width * 2;
	} else if(input_fmt == MM_UTIL_JPEG_FMT_GraySacle) {
		decoded_data->size= dinfo.output_height * dinfo.output_width;
	} else{
		jpeg_destroy_decompress(&dinfo);
		mm_util_error("[%d] We can't decode the IMAGE format", input_fmt);
		return MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	}

	decoded_data->data= (void*) g_malloc0(decoded_data->size);
	decoded_data->format = input_fmt;

	if(decoded_data->data == NULL) {
		jpeg_destroy_decompress(&dinfo);
		mm_util_error("decoded_data->data is NULL");
		return MM_UTIL_ERROR_OUT_OF_MEMORY;
	}
	mm_util_debug("decoded_data->data");

	/* while (scan lines remain to be read) jpeg_read_scanlines(...); */
	if(input_fmt == MM_UTIL_JPEG_FMT_YUV420 || input_fmt == MM_UTIL_JPEG_FMT_YUV422 || input_fmt == MM_UTIL_JPEG_FMT_UYVY) {
		image = decoded_data->data;
		u_image = image + (dinfo.output_width * dinfo.output_height);
		v_image = u_image + (dinfo.output_width*dinfo.output_height)/4;
		row= buffer[0];
		int i = 0;
		int y = 0;
		while (dinfo.output_scanline < dinfo.output_height) {
			jpeg_read_scanlines(&dinfo, buffer, 1);
			for (i = 0; i < row_stride; i += 3) {
				image[i/3]=row[i];
				if (i & 1) {
					u_image[(i/3)/2]=row[i+1];
					v_image[(i/3)/2]=row[i+2];
				}
			}
			image += row_stride/3;
			if (y++ & 1) {
				u_image += dinfo.output_width / 2;
				v_image += dinfo.output_width / 2;
			}
		}
	} else if(input_fmt == MM_UTIL_JPEG_FMT_RGB888 ||input_fmt == MM_UTIL_JPEG_FMT_GraySacle || input_fmt == MM_UTIL_JPEG_FMT_RGBA8888 || input_fmt == MM_UTIL_JPEG_FMT_BGRA8888 || input_fmt == MM_UTIL_JPEG_FMT_ARGB8888) {
		int state = 0;
		while (dinfo.output_scanline < dinfo.output_height) {
			/* jpeg_read_scanlines expects an array of pointers to scanlines. Here the array is only one element long, but you could ask formore than one scanline at a time if that's more convenient. */
			jpeg_read_scanlines(&dinfo, buffer, 1);

			memcpy(decoded_data->data + state, buffer[0], row_stride);
			state += row_stride;
		}
		mm_util_debug("jpeg_read_scanlines");
	}

	/* Finish decompression */
	jpeg_finish_decompress(&dinfo);
	mm_util_debug("jpeg_finish_decompress");

	/* Release JPEG decompression object */
	jpeg_destroy_decompress(&dinfo);
	mm_util_debug("jpeg_destroy_decompress");

	mm_util_debug("fclose");

	return iErrorCode;
}

#if 0
static int _mm_util_set_exif_entry(ExifData *exif, ExifIfd ifd, ExifTag tag,ExifFormat format, unsigned long components, unsigned char* data)
{
	ExifData *ed = (ExifData *)exif;
	ExifEntry *e = NULL;

	if (exif == NULL || format <= 0 || components <= 0 || data == NULL) {
		mm_util_error("invalid argument exif = %p format = %d, components = %lu data = %p!",
			exif, format, components, data);
		return -1;
	}

	/*remove same tag in EXIF*/
	exif_content_remove_entry(ed->ifd[ifd], exif_content_get_entry(ed->ifd[ifd], tag));

	/*create new tag*/
	e = exif_entry_new();
	if (e == NULL) {
		mm_util_error("entry create error!");
		return -1;
	}

	exif_entry_initialize(e, tag);

	e->tag = tag;
	e->format = format;
	e->components = components;

	if (e->size == 0) {
		e->data = NULL;
		e->data = malloc(exif_format_get_size(format) * e->components);
		if (!e->data) {
			exif_entry_unref(e);
			return -1;
		}

		if (format == EXIF_FORMAT_ASCII) {
			memset(e->data, '\0', exif_format_get_size(format) * e->components);
		}
	}

	e->size = exif_format_get_size(format) * e->components;
	memcpy(e->data,data,e->size);
	exif_content_add_entry(ed->ifd[ifd], e);
	exif_entry_unref(e);

	return 0;
}
#endif

EXPORT_API int
mm_util_jpeg_encode_to_file(const char *filename, void* src, int width, int height, mm_util_jpeg_yuv_format fmt, int quality)
{
	int ret = MM_UTIL_ERROR_NONE;

	TTRACE_BEGIN("MM_UTILITY:JPEG:ENCODE_TO_FILE");

	if( !filename || !src) {
		mm_util_error("#ERROR# filename || src buffer is NULL");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if( (width <= 0) || (height <= 0)) {
		mm_util_error("#ERROR# src_width || src_height value ");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if( (fmt < MM_UTIL_JPEG_FMT_YUV420) || (fmt > MM_UTIL_JPEG_FMT_ARGB8888) ) {
		mm_util_error("#ERROR# fmt value");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if( (quality < 1 ) || (quality>100) ) {
		mm_util_error("#ERROR# quality vaule");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

#if LIBJPEG_TURBO
	mm_util_debug("#START# LIBJPEG_TURBO");
	ret=mm_image_encode_to_jpeg_file_with_libjpeg_turbo(filename, src, width, height, fmt, quality);
	mm_util_debug("#End# libjpeg, Success!! ret: %d", ret);

#else
	mm_util_debug("#START# LIBJPEG");
	if(fmt == MM_UTIL_JPEG_FMT_NV12) {
		unsigned int dst_size = 0;
		ret = mm_util_get_image_size(MM_UTIL_IMG_FMT_NV12, (unsigned int)width, (unsigned int)height, &dst_size);
		unsigned char *dst = NULL;
		dst = malloc(dst_size);
		if(dst) {
			ret = mm_util_convert_colorspace(src, width, height,MM_UTIL_IMG_FMT_NV12, dst, MM_UTIL_IMG_FMT_YUV420);
			ret = mm_image_encode_to_jpeg_file_with_libjpeg(filename, dst, width, height, MM_UTIL_JPEG_FMT_YUV420, quality);
			IMG_JPEG_FREE(dst);
		} else {
			TTRACE_END();
			return MM_UTIL_ERROR_OUT_OF_MEMORY;
		}
	} else if(fmt == MM_UTIL_JPEG_FMT_NV21) {
		TTRACE_END();
		return MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	} else {
		ret = mm_image_encode_to_jpeg_file_with_libjpeg(filename, src, width, height, fmt, quality);
	}
	mm_util_debug("#END# libjpeg, Success!! ret: %d", ret);
#endif
	TTRACE_END();
	return ret;
}

EXPORT_API int
mm_util_jpeg_encode_to_memory(void **mem, int *size, void* src, int width, int height, mm_util_jpeg_yuv_format fmt, int quality)
{
	int ret = MM_UTIL_ERROR_NONE;

	TTRACE_BEGIN("MM_UTILITY:JPEG:ENCODE_TO_MEMORY");

	if( !mem || !size || !src) {
		mm_util_error("#ERROR# filename ||size ||  src buffer is NULL");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if( (width <= 0) || (height <= 0)) {
		mm_util_error("#ERROR# src_width || src_height value ");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if( (fmt < MM_UTIL_JPEG_FMT_YUV420) || (fmt > MM_UTIL_JPEG_FMT_ARGB8888) ) {
		mm_util_error("#ERROR# fmt value");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if(  (quality < 1 ) || (quality>100) ) {
		mm_util_error("#ERROR# quality vaule");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	#if LIBJPEG_TURBO
	mm_util_debug("#START# libjpeg-turbo");
	ret=mm_image_encode_to_jpeg_memory_with_libjpeg_turbo(mem, size, src, width, height, fmt, quality);
	mm_util_debug("#END# libjpeg-turbo, Success!! ret: %d", ret);
	#else /* LIBJPEG_TURBO */
	mm_util_debug("#START# libjpeg");
	if(fmt == MM_UTIL_JPEG_FMT_NV12) {
		unsigned int dst_size = 0;
		ret = mm_util_get_image_size(MM_UTIL_IMG_FMT_NV12, (unsigned int)width, (unsigned int)height, &dst_size);
		unsigned char *dst = NULL;
		dst = malloc(dst_size);
		if(dst) {
			ret = mm_util_convert_colorspace(src, width, height,MM_UTIL_IMG_FMT_NV12, dst, MM_UTIL_IMG_FMT_YUV420);
			ret = mm_image_encode_to_jpeg_memory_with_libjpeg(mem, size, dst, width, height, MM_UTIL_JPEG_FMT_YUV420, quality);
			IMG_JPEG_FREE(dst);
		} else {
			TTRACE_END();
			return MM_UTIL_ERROR_OUT_OF_MEMORY;
		}
	} else if(fmt == MM_UTIL_JPEG_FMT_NV21) {
		TTRACE_END();
		return MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	} else {
		ret = mm_image_encode_to_jpeg_memory_with_libjpeg(mem, size, src, width, height, fmt, quality);
	}
#endif /* LIBJPEG_TURBO */
	mm_util_debug("#END# libjpeg, Success!! ret: %d", ret);

	TTRACE_END();
	return ret;
}

EXPORT_API int
mm_util_decode_from_jpeg_file(mm_util_jpeg_yuv_data *decoded, const char *filename, mm_util_jpeg_yuv_format fmt)
{
	int ret = MM_UTIL_ERROR_NONE;

	mm_util_jpeg_decode_downscale downscale = MM_UTIL_JPEG_DECODE_DOWNSCALE_1_1;

	TTRACE_BEGIN("MM_UTILITY:JPEG:DECODE_FROM_JPEG_FILE");

	if( !decoded || !filename) {
		mm_util_error("#ERROR# decoded || filename buffer is NULL");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if( (fmt < MM_UTIL_JPEG_FMT_YUV420) || (fmt > MM_UTIL_JPEG_FMT_ARGB8888) ) {
		mm_util_error("#ERROR# fmt value");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	FILE *fp = fopen(filename, "rb");
	unsigned char magic[2] = {0};
	size_t read_size = 0;
	if(fp) {
		read_size = fread((void *)magic, 1, 2, fp);
		if (read_size > 0)
			mm_util_debug("Success fread");

		mm_util_debug("%x %x", magic[0], magic[1]);
		fclose(fp);
	} else {
		mm_util_error("[infile] file open [%s]", filename);
		mm_util_stderror("file open failed");
		TTRACE_END();
		return MM_UTIL_ERROR_NO_SUCH_FILE;
	}

	if(magic[0] == 0xff && magic[1] == 0xd8) {
		#if LIBJPEG_TURBO
		mm_util_debug("#START# LIBJPEG_TURBO");
		ret = mm_image_decode_from_jpeg_file_with_libjpeg_turbo(decoded, filename, fmt);
		mm_util_debug("decoded->data: %p\t width: %d\t height: %d\t size: %d\n", decoded->data, decoded->width, decoded->height, decoded->size);
		mm_util_debug("#End# LIBJPEG_TURBO, Success!! ret: %d", ret);
		#else
		mm_util_debug("#START# libjpeg");
		if(fmt == MM_UTIL_JPEG_FMT_NV12) {
			unsigned int dst_size = 0;
			ret = mm_image_decode_from_jpeg_file_with_libjpeg(decoded, filename, MM_UTIL_IMG_FMT_YUV420, downscale);
			if (ret == MM_UTIL_ERROR_NONE) {
				int err = MM_UTIL_ERROR_NONE;
				err = mm_util_get_image_size(MM_UTIL_IMG_FMT_NV12, decoded->width, decoded->height, &dst_size);
				if (err != MM_UTIL_ERROR_NONE)
					mm_util_error("fail mm_util_get_image_size");

				unsigned char *dst = NULL;
				dst = malloc(dst_size);
				if(dst) {
					ret = mm_util_convert_colorspace(decoded->data, decoded->width, decoded->height, MM_UTIL_IMG_FMT_YUV420, dst, MM_UTIL_IMG_FMT_NV12);
					IMG_JPEG_FREE(decoded->data);
					decoded->data = malloc(dst_size);
					if (decoded->data == NULL) {
						mm_util_debug("memory allocation failed");
						IMG_JPEG_FREE(dst);
						TTRACE_END();
						return MM_UTIL_ERROR_OUT_OF_MEMORY;
					}
					memcpy(decoded->data, dst, dst_size);
					decoded->size = dst_size;
					IMG_JPEG_FREE(dst);
				} else {
					mm_util_debug("memory allocation failed");
					TTRACE_END();
					return MM_UTIL_ERROR_OUT_OF_MEMORY;
				}
			}
		} else {
			ret = mm_image_decode_from_jpeg_file_with_libjpeg(decoded, filename, fmt, downscale);
		}

		mm_util_debug("decoded->data: %p\t width: %d\t height:%d\t size: %d\n", decoded->data, decoded->width, decoded->height, decoded->size);
		mm_util_debug("#End# libjpeg, Success!! ret: %d", ret);
		#endif
	} else if(magic[0] == 0x47 && magic[1] == 0x49) {
		mm_util_error("Not JPEG IMAGE - GIF");
		ret = MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	} else if(magic[0] == 0x89 && magic[1] == 0x50) {
		mm_util_error("Not JPEG IMAGE - PNG");
		ret = MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	} else if(magic[0] == 0x49 && magic[1] == 0x49) {
		mm_util_error("Not JPEG IMAGE - TIFF");
		ret = MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	} else {
		mm_util_error("Not JPEG IMAGE");
		ret = MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	}

	TTRACE_END();
	return ret;
}

EXPORT_API int
mm_util_decode_from_jpeg_memory(mm_util_jpeg_yuv_data* decoded, void* src, int size, mm_util_jpeg_yuv_format fmt)
{
	int ret = MM_UTIL_ERROR_NONE;

	TTRACE_BEGIN("MM_UTILITY:JPEG:DECODE_FROM_JPEG_MEMORY");

	mm_util_jpeg_decode_downscale downscale = MM_UTIL_JPEG_DECODE_DOWNSCALE_1_1;

	if( !decoded || !src) {
		mm_util_error("#ERROR# decoded || src buffer is NULL");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if(size < 0) {
		mm_util_error("#ERROR# size");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if( (fmt < MM_UTIL_JPEG_FMT_YUV420) || (fmt > MM_UTIL_JPEG_FMT_ARGB8888) ) {
		mm_util_error("#ERROR# fmt value");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	#if LIBJPEG_TURBO
	mm_util_debug("#START# libjpeg");
	ret = mm_image_decode_from_jpeg_memory_with_libjpeg_turbo(decoded, src, size, fmt);

	mm_util_debug("decoded->data: %p\t width: %d\t height: %d\t size: %d\n", decoded->data, decoded->width, decoded->height, decoded->size);
	mm_util_debug("#END# libjpeg, Success!! ret: %d", ret);

	#else
	mm_util_debug("#START# libjpeg");
	if(fmt == MM_UTIL_JPEG_FMT_NV12) {
		unsigned int dst_size = 0;
		unsigned char *dst = NULL;

		ret = mm_image_decode_from_jpeg_memory_with_libjpeg(decoded, src, size, MM_UTIL_IMG_FMT_YUV420, downscale);
		if (ret == MM_UTIL_ERROR_NONE) {
			int err = MM_UTIL_ERROR_NONE;
			err = mm_util_get_image_size(MM_UTIL_IMG_FMT_NV12, decoded->width, decoded->height, &dst_size);
			if (err != MM_UTIL_ERROR_NONE)
					mm_util_error("fail mm_util_get_image_size");

			dst = malloc(dst_size);
			if(dst) {
				ret = mm_util_convert_colorspace(decoded->data, decoded->width, decoded->height, MM_UTIL_IMG_FMT_YUV420, dst, MM_UTIL_IMG_FMT_NV12);
				IMG_JPEG_FREE(decoded->data);
				decoded->data = malloc(dst_size);
				if (decoded->data == NULL) {
					mm_util_debug("memory allocation failed");
					IMG_JPEG_FREE(dst);
					TTRACE_END();
					return MM_UTIL_ERROR_OUT_OF_MEMORY;
				}
				memcpy(decoded->data, dst, dst_size);
				decoded->size = dst_size;
				IMG_JPEG_FREE(dst);
			} else {
				mm_util_debug("memory allocation failed");
				TTRACE_END();
				return MM_UTIL_ERROR_OUT_OF_MEMORY;
			}
		}
	} else {
		ret = mm_image_decode_from_jpeg_memory_with_libjpeg(decoded, src, size, fmt, downscale);
	}

	mm_util_debug("decoded->data: %p\t width: %d\t height: %d\t size: %d\n", decoded->data, decoded->width, decoded->height, decoded->size);
	mm_util_debug("#END# libjpeg, Success!! ret: %d", ret);
	#endif

	TTRACE_END();
	return ret;
}

EXPORT_API int
mm_util_decode_from_jpeg_file_with_downscale(mm_util_jpeg_yuv_data *decoded, const char *filename, mm_util_jpeg_yuv_format fmt, mm_util_jpeg_decode_downscale downscale)
{
	int ret = MM_UTIL_ERROR_NONE;

	TTRACE_BEGIN("MM_UTILITY:JPEG:DECODE_FROM_JPEG_FILE_WITH_DOWNSCALE");

	if( !decoded || !filename) {
		mm_util_error("#ERROR# decoded || filename buffer is NULL");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if( (fmt < MM_UTIL_JPEG_FMT_YUV420) || (fmt > MM_UTIL_JPEG_FMT_ARGB8888) ) {
		mm_util_error("#ERROR# fmt value");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if( (downscale != MM_UTIL_JPEG_DECODE_DOWNSCALE_1_1) && (downscale != MM_UTIL_JPEG_DECODE_DOWNSCALE_1_2)
		 && (downscale != MM_UTIL_JPEG_DECODE_DOWNSCALE_1_4) && (downscale != MM_UTIL_JPEG_DECODE_DOWNSCALE_1_8)) {
		mm_util_error("#ERROR# fmt value");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	FILE *fp = fopen(filename, "rb");
	unsigned char magic[2] = {0};
	size_t read_size = 0;
	if(fp) {
		read_size = fread((void *)magic, 1, 2, fp);
		if (read_size > 0)
			mm_util_debug("Success fread");

		mm_util_debug("%x %x", magic[0], magic[1]);
		fclose(fp);
	}
	if(magic[0] == 0xff && magic[1] == 0xd8) {
		#if LIBJPEG_TURBO
		mm_util_debug("#START# LIBJPEG_TURBO");
		ret = mm_image_decode_from_jpeg_file_with_libjpeg_turbo(decoded, filename, fmt);
		mm_util_debug("decoded->data: %p\t width: %d\t height: %d\t size: %d\n", decoded->data, decoded->width, decoded->height, decoded->size);
		mm_util_debug("#End# LIBJPEG_TURBO, Success!! ret: %d", ret);
		#else
		mm_util_debug("#START# libjpeg");
		if(fmt == MM_UTIL_JPEG_FMT_NV12) {
			unsigned int dst_size = 0;
			ret = mm_image_decode_from_jpeg_file_with_libjpeg(decoded, filename, MM_UTIL_IMG_FMT_YUV420, downscale);
			if (ret == MM_UTIL_ERROR_NONE) {
				int err = MM_UTIL_ERROR_NONE;
				err = mm_util_get_image_size(MM_UTIL_IMG_FMT_NV12, decoded->width, decoded->height, &dst_size);
				if (err != MM_UTIL_ERROR_NONE)
					mm_util_error("fail mm_util_get_image_size");

				unsigned char *dst = NULL;
				dst = malloc(dst_size);
				if(dst) {
					ret = mm_util_convert_colorspace(decoded->data, decoded->width, decoded->height, MM_UTIL_IMG_FMT_YUV420, dst, MM_UTIL_IMG_FMT_NV12);
					IMG_JPEG_FREE(decoded->data);
					decoded->data = malloc(dst_size);
					if (decoded->data == NULL) {
						mm_util_debug("memory allocation failed");
						IMG_JPEG_FREE(dst);
						TTRACE_END();
						return MM_UTIL_ERROR_OUT_OF_MEMORY;
					}
					memcpy(decoded->data, dst, dst_size);
					decoded->size = dst_size;
					IMG_JPEG_FREE(dst);
				} else {
					mm_util_debug("memory allocation failed");
					TTRACE_END();
					return MM_UTIL_ERROR_OUT_OF_MEMORY;
				}
			}
		} else {
			ret = mm_image_decode_from_jpeg_file_with_libjpeg(decoded, filename, fmt, downscale);
		}

		mm_util_debug("decoded->data: %p\t width: %d\t height:%d\t size: %d\n", decoded->data, decoded->width, decoded->height, decoded->size);
		mm_util_debug("#End# libjpeg, Success!! ret: %d", ret);
		#endif
	} else if(magic[0] == 0x47 && magic[1] == 0x49) {
		mm_util_error("Not JPEG IMAGE - GIF");
		ret = MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	} else if(magic[0] == 0x89 && magic[1] == 0x50) {
		mm_util_error("Not JPEG IMAGE - PNG");
		ret = MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	} else if(magic[0] == 0x49 && magic[1] == 0x49) {
		mm_util_error("Not JPEG IMAGE - TIFF");
		ret = MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	} else {
		mm_util_error("Not JPEG IMAGE");
		ret = MM_UTIL_ERROR_NOT_SUPPORTED_FORMAT;
	}

	TTRACE_END();
	return ret;
}

EXPORT_API int
mm_util_decode_from_jpeg_memory_with_downscale(mm_util_jpeg_yuv_data* decoded, void* src, int size, mm_util_jpeg_yuv_format fmt, mm_util_jpeg_decode_downscale downscale)
{
	int ret = MM_UTIL_ERROR_NONE;

	TTRACE_BEGIN("MM_UTILITY:JPEG:DECODE_FROM_JPEG_MEMORY_WITH_DOWNSCALE");

	if( !decoded || !src) {
		mm_util_error("#ERROR# decoded || src buffer is NULL");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if(size < 0) {
		mm_util_error("#ERROR# size");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if( (fmt < MM_UTIL_JPEG_FMT_YUV420) || (fmt > MM_UTIL_JPEG_FMT_ARGB8888) ) {
		mm_util_error("#ERROR# fmt value");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	if( (downscale != MM_UTIL_JPEG_DECODE_DOWNSCALE_1_1) && (downscale != MM_UTIL_JPEG_DECODE_DOWNSCALE_1_2)
		 && (downscale != MM_UTIL_JPEG_DECODE_DOWNSCALE_1_4) && (downscale != MM_UTIL_JPEG_DECODE_DOWNSCALE_1_8)) {
		mm_util_error("#ERROR# fmt value");
		TTRACE_END();
		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	#if LIBJPEG_TURBO
	mm_util_debug("#START# libjpeg");
	ret = mm_image_decode_from_jpeg_memory_with_libjpeg_turbo(decoded, src, size, fmt);

	mm_util_debug("decoded->data: %p\t width: %d\t height: %d\t size: %d\n", decoded->data, decoded->width, decoded->height, decoded->size);
	mm_util_debug("#END# libjpeg, Success!! ret: %d", ret);

	#else
	mm_util_debug("#START# libjpeg");
	if(fmt == MM_UTIL_JPEG_FMT_NV12) {
		unsigned int dst_size = 0;
		unsigned char *dst = NULL;

		ret = mm_image_decode_from_jpeg_memory_with_libjpeg(decoded, src, size, MM_UTIL_IMG_FMT_YUV420, downscale);
		if (ret == MM_UTIL_ERROR_NONE) {
			int err = MM_UTIL_ERROR_NONE;
			err = mm_util_get_image_size(MM_UTIL_IMG_FMT_NV12, decoded->width, decoded->height, &dst_size);
			if (err != MM_UTIL_ERROR_NONE)
					mm_util_error("fail mm_util_get_image_size");

			dst = malloc(dst_size);
			if(dst) {
				ret = mm_util_convert_colorspace(decoded->data, decoded->width, decoded->height, MM_UTIL_IMG_FMT_YUV420, dst, MM_UTIL_IMG_FMT_NV12);
				IMG_JPEG_FREE(decoded->data);
				decoded->data = malloc(dst_size);
				if (decoded->data == NULL) {
					mm_util_debug("memory allocation failed");
					IMG_JPEG_FREE(dst);
					TTRACE_END();
					return MM_UTIL_ERROR_OUT_OF_MEMORY;
				}
				memcpy(decoded->data, dst, dst_size);
				decoded->size = dst_size;
				IMG_JPEG_FREE(dst);
			} else {
				mm_util_debug("memory allocation failed");
				TTRACE_END();
				return MM_UTIL_ERROR_OUT_OF_MEMORY;
			}
		}
	} else {
		ret = mm_image_decode_from_jpeg_memory_with_libjpeg(decoded, src, size, fmt, downscale);
	}

	mm_util_debug("decoded->data: %p\t width: %d\t height: %d\t size: %d\n", decoded->data, decoded->width, decoded->height, decoded->size);
	mm_util_debug("#END# libjpeg, Success!! ret: %d", ret);
	#endif

	TTRACE_END();
	return ret;
}
