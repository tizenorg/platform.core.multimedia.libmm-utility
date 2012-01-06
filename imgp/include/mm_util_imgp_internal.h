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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include "mm_util_imgp.h"
#ifdef __cplusplus
        extern "C" {
#endif

#include <gmodule.h>
#include <mm_debug.h>

#define PATH_NEON_LIB        		 				"/usr/lib/libmmutil_imgp_neon.so"
#define PATH_GSTCS_LIB         				 		"/usr/lib/libmmutil_imgp_gstcs.so"

#define IMGP_FUNC_NAME  				 		 "mm_imgp"
#define buf_size 9

/**
 * YUV format for jpeg
 */
typedef enum
{
    /* YUV planar format */
    MM_UTIL_JPEG_FMT_YUV420 = 0x00,  /**< YUV420 format - planer */
    MM_UTIL_JPEG_FMT_YUV422,         /**< YUV422 format - planer */
    /* YUV packed format */
    MM_UTIL_JPEG_FMT_UYVY,            /**< UYVY format - YUV packed format */
    /* RGB888 format */
    MM_UTIL_JPEG_FMT_RGB888,	     /**< RGB888 format  */
    /* GrayScale format */
    MM_UTIL_JPEG_FMT_GraySacle,    /**< GrayScale format */
} mm_util_jpeg_yuv_format;

/**
 * Image Process Info for dlopen 
 */
typedef struct _imgp_info
{
	unsigned char *src;
	char   input_format_label[buf_size];
	mm_util_img_format src_format;
	unsigned int src_width;
	unsigned int src_height;
	unsigned char *dst;
	char  output_format_label[buf_size];
	mm_util_img_format dst_format;
	unsigned int dst_width;
	unsigned int dst_height;
	unsigned int output_stride;
	unsigned int output_elevation;
	mm_util_img_rotate_type angle;
} imgp_info;

/* Enumerations */
typedef enum
{
        IMGP_CSC = 0,
	IMGP_RSZ,
	IMGP_ROT,
} imgp_type;

/* Enumerations */
typedef enum
{
        IMGP_NEON = 0,
	IMGP_GSTCS,
} imgp_plugin_type;
#ifdef __cplusplus
}
#endif


