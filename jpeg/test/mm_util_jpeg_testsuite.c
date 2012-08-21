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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <mm_util_jpeg.h>
#include <mm_ta.h>
#include <mm_error.h>
#define ENCODE_RESULT_PATH "/opt/media/encode_test.jpg"
#define DECODE_RESULT_PATH "/opt/media/decode_test.yuv"
#define TRUE  1
#define FALSE 0
#define GST_EXT_TIME_ANALYSIS

static inline void flush_stdin()
{
	int ch;
	while((ch=getchar()) != EOF && ch != '\n');
}


static int _read_file(char *file_name, void **data, int *data_size)
{
	FILE *fp = NULL;
	int file_size = 0;

	if (!file_name || !data || !data_size) {
		fprintf(stderr, "\tNULL pointer\n");
		return FALSE;
	}

	fprintf(stderr, "\tTry to open %s to read\n", file_name);

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		fprintf(stderr, "\tfile open failed %d\n", errno);
		return FALSE;
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);
	*data = (void *)malloc(file_size);
	if (*data == NULL) {
		fprintf(stderr, "\tmalloc failed %d\n", errno);
	} else {
		fread(*data, 1, file_size, fp);
	}

	fclose(fp);
	fp = NULL;

	if (*data) {
		*data_size = file_size;
		return TRUE;
	} else {
		*data_size = 0;
		return FALSE;
	}
}


static int _write_file(char *file_name, void *data, int data_size)
{
	FILE *fp = NULL;

	if (!file_name || !data || data_size <= 0) {
		fprintf(stderr, "\tinvalid data %s %p size:%d\n", file_name, data, data_size);
		return FALSE;
	}

	fprintf(stderr, "\tTry to open %s to write\n", file_name);

	fp = fopen(file_name, "w");
	fwrite(data, 1, data_size, fp);
	fclose(fp);
	fp = NULL;

	fprintf(stderr, "\tfile [%s] write DONE\n", file_name);

	return TRUE;
}


int main(int argc, char *argv[])
{
	int ret = 0;
	int src_size = 0;
	int dst_size = 0;
	int width = 0;
	int height = 0;
	int quality = 0;
	void *src = NULL;
	void *dst = NULL;

	mm_util_jpeg_yuv_data decoded_data = {0,};
	mm_util_jpeg_yuv_format fmt = MM_UTIL_JPEG_FMT_YUV420;

	if (argc < 2) {
		fprintf(stderr, "\t[usage]\n");
		fprintf(stderr, "\t\t1. encode : mm_util_jpeg_testsuite encode filepath.yuv width height quality\n");
		fprintf(stderr, "\t\t2. decode : mm_util_jpeg_testsuite decode filepath.jpg\n");
		return 0;
	}

	//g_type_init();  //when you include WINK

	MMTA_INIT();

	if (!strcmp("encode", argv[1])) {
		if (_read_file(argv[2], &src, &src_size)) {
			width = atoi(argv[3]);
			height = atoi(argv[4]);
			quality = atoi(argv[5]);

			__ta__("mm_util_jpeg_encode_to_memory",
			ret = mm_util_jpeg_encode_to_memory(&dst, &dst_size, src, width, height, fmt, quality);
			);
		} else {
			ret = MM_ERROR_IMAGE_INTERNAL;
		}
	} else if (!strcmp("decode", argv[1])) {
		if (_read_file(argv[2], &src, &src_size)) {
			__ta__("mm_util_decode_from_jpeg_memory",
			ret = mm_util_decode_from_jpeg_memory(&decoded_data, src, src_size, fmt);
			);

			free(src);
			src = NULL;
		} else {
			ret = MM_ERROR_IMAGE_INTERNAL;
		}
	} else {
		fprintf(stderr, "\tunknown command [%s]\n", argv[1]);
		return 0;
	}

	if (ret != MM_ERROR_NONE) {
		fprintf(stderr, "\tERROR is occurred %x\n", ret);
	}else {
		fprintf(stderr, "\tJPEG OPERATION SUCCESS\n");
		if (!strcmp("encode", argv[1])) {
			if (dst) {
				fprintf(stderr, "\t##Encoded data##: %p\tsize: %d\n", dst, dst_size);

				_write_file(ENCODE_RESULT_PATH, dst, dst_size);

				free(dst);
				dst = NULL;
				dst_size = 0;
			} else {
				fprintf(stderr, "\tENCODED data is NULL\n");
			}
		} else {
			if(decoded_data.data) {
				fprintf(stderr, "\t##Decoded data##: %p\t width: %d\t height:%d\t size: %d\n",
				                decoded_data.data, decoded_data.width, decoded_data.height, decoded_data.size);

				_write_file(DECODE_RESULT_PATH, decoded_data.data, decoded_data.size);

				free(decoded_data.data);
				decoded_data.data = NULL;
				decoded_data.size = 0;
			} else {
				fprintf(stderr, "\tDECODED data is NULL\n");
			}
		}
	}

	MMTA_ACUM_ITEM_SHOW_RESULT();
	MMTA_RELEASE ();

	return 0;
}
