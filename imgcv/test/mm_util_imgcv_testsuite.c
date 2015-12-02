/*
 * libmm-utility
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Tae-Young Chung <ty83.chung@samsung.com>
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

#include "mm_util_jpeg.h"
#include <mm_util_imgcv.h>
#include <mm_util_imgcv_internal.h>
#include <mm_error.h>

#define MAX_FILENAME_LEN 1024

int main(int argc, char *argv[])
{
	int ret = 0;
	int width;
	int height;
	int len;

	unsigned char *img_buffer;
	unsigned int  img_buffer_size;

	char filename [MAX_FILENAME_LEN];

	if (argc < 1) {
		debug_error("[%s][%05d] Usage: ./mm_imgcv_testsuite filename(jpg format only)\n");
		return ret;
	}

	len = strlen(argv[1]);
	if (len > MAX_FILENAME_LEN) {
		debug_error("[%s] filename is too long\n");
		return -1;
	}

	strncpy(filename, argv[1], len);
	debug_log("Filename is %s\n", filename);

	/* decode jpg image */
	mm_util_jpeg_yuv_data decoded;
	memset(&decoded, 0, sizeof(mm_util_jpeg_yuv_data));

	ret = mm_util_decode_from_jpeg_file(&decoded, filename, MM_UTIL_JPEG_FMT_RGB888);

	if (ret == MM_ERROR_NONE) {
		img_buffer = decoded.data;
		width = decoded.width;
		height = decoded.height;
		img_buffer_size = decoded.size;
		debug_log("Success - buffer[%p], width[%d], height[%d], size[%d]",
								img_buffer, width, height, img_buffer_size);
	} else {
		debug_log("ERROR - Fail to decode jpeg image file");
		return ret;
	}

	/* extract color */
	unsigned char rgb_r, rgb_g, rgb_b;
	ret = image_util_color_extract_from_buffer(img_buffer, width, height, MM_UTIL_IMGCV_FMT_RGB888, &rgb_r, &rgb_g, &rgb_b);

	if (ret == MM_ERROR_NONE) {
		debug_log("Success - R[%d], G[%d], B[%d]", rgb_r, rgb_g, rgb_b);
	} else {
		debug_log("Error - fail to extract color");
	}

	free(img_buffer);
	img_buffer = NULL;

	return ret;
}
