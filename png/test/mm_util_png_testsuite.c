/*
 * libmm-utility
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Vineeth T M <vineeth.tm@samsung.com>
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
#include <mm_util_png.h>

#define DECODE_RESULT_PATH "/media/decode_test."
#define TRUE  1
#define FALSE 0
#define BUFFER_SIZE 128

static inline void flush_stdin()
{
	int ch;
	while ((ch = getchar()) != EOF && ch != '\n') ;
}

static int _read_file(char *file_name, void **data, int *data_size)
{
	FILE *fp = NULL;
	long file_size = 0;

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
	if (file_size > -1L) {
		rewind(fp);
		*data = (void *)malloc(file_size);
		if (*data == NULL) {
			fprintf(stderr, "\tmalloc failed %d\n", errno);
			fclose(fp);
			fp = NULL;
			return FALSE;
		} else {
			if (fread(*data, 1, file_size, fp)) {
				fprintf(stderr, "\t#Success# fread\n");
			} else {
				fprintf(stderr, "\t#Error# fread\n");
				fclose(fp);
				fp = NULL;
				return FALSE;
			}
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
	} else {
		fprintf(stderr, "\t#Success# ftell\n");
		fclose(fp);
		fp = NULL;
		return FALSE;
	}
}

static int _write_file(const char *file_name, void *data, int data_size)
{
	FILE *fp = NULL;

	if (!file_name || !data || data_size <= 0) {
		fprintf(stderr, "\tinvalid data %s %p size:%d\n", file_name, data, data_size);
		return FALSE;
	}

	fprintf(stderr, "\tTry to open %s to write\n", file_name);

	fp = fopen(file_name, "w");
	if (fp == NULL) {
		fprintf(stderr, "\tfile open failed %d\n", errno);
		return FALSE;
	}

	fwrite(data, 1, data_size, fp);
	fclose(fp);
	fp = NULL;

	fprintf(stderr, "\tfile [%s] write DONE\n", file_name);

	return TRUE;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	mm_util_png_data decoded_data;
	mm_util_png_data encoded_data;
	void *data;
	void *src = NULL;
	int src_size = 0;

	if (argc < 2) {
		fprintf(stderr, "\t[usage]\n");
		fprintf(stderr, "\t\t1. decode : mm_util_png_testsuite decode filepath.png is_progressive compression_level\n");
		return 0;
	}

	if (!strcmp("decode", argv[1])) {
		mm_util_init_decode_png(&decoded_data);
		if (argv[3]) {
#if 0
			mm_util_png_decode_set_progressive(&decoded_data, atoi(argv[3]));
			mm_util_png_decode_set_progressive_bytes(&decoded_data, 10000);
#endif
		}
		ret = mm_util_decode_from_png_file(&decoded_data, argv[2]);
	} else if (!strcmp("decode-mem", argv[1])) {
		if (_read_file(argv[2], &src, &src_size)) {
			mm_util_init_decode_png(&decoded_data);
			if (argv[3]) {
#if 0
				mm_util_png_decode_set_progressive(&decoded_data, atoi(argv[3]));
				mm_util_png_decode_set_progressive_bytes(&decoded_data, 10000);
#endif
			}
			ret = mm_util_decode_from_png_memory(&decoded_data, &src, src_size);

			free(src);
			src = NULL;
		} else {
			ret = MM_UTIL_ERROR_INVALID_OPERATION;
		}
	} else {
		fprintf(stderr, "\tunknown command [%s]\n", argv[1]);
		return 0;
	}

	if (ret != MM_UTIL_ERROR_NONE) {
		fprintf(stderr, "\tERROR is occurred %x\n", ret);
	} else {
		fprintf(stderr, "\tPNG OPERATION SUCCESS\n");
		{
			if (decoded_data.data) {
				fprintf(stderr, "\t##Decoded data##: %p\t width: %lu\t height:%lu\t size: %lu\n", decoded_data.data, (long unsigned int)decoded_data.width, (long unsigned int)decoded_data.height, (long unsigned int)decoded_data.size);
				char filename[BUFFER_SIZE] = { 0, };
				memset(filename, 0, BUFFER_SIZE);
				{
					snprintf(filename, BUFFER_SIZE, "%s%s", DECODE_RESULT_PATH, "png");
				}

				data = malloc(decoded_data.size);
				memcpy(data, decoded_data.data, decoded_data.size);

				mm_util_init_encode_png(&encoded_data);
				if (argv[4])
					mm_util_png_encode_set_compression_level(&encoded_data, atoi(argv[4]));
				mm_util_png_encode_set_width(&encoded_data, decoded_data.width);
				mm_util_png_encode_set_height(&encoded_data, decoded_data.height);
				mm_util_png_encode_set_bit_depth(&encoded_data, decoded_data.png.bit_depth);
				mm_util_png_encode_set_color_type(&encoded_data, decoded_data.png.color_type);
				mm_util_png_encode_set_filter_type(&encoded_data, decoded_data.png.filter_type);
				mm_util_png_encode_set_interlace_type(&encoded_data, decoded_data.png.interlace_type);

				if (!strcmp("decode", argv[1])) {
					ret = mm_util_encode_to_png_memory(&data, &encoded_data);
					fprintf(stderr, "Finished encoding encoded_data.size %lu\n", (long unsigned int)encoded_data.size);
					_write_file(filename, encoded_data.data, encoded_data.size);
					free(encoded_data.data);
				} else if (!strcmp("decode-mem", argv[1])) {
					ret = mm_util_encode_to_png_file(&data, &encoded_data, filename);
				}
				free(decoded_data.data);
				free(data);
			} else {
				fprintf(stderr, "\tDECODED data is NULL\n");
			}
		}
	}
	return 0;
}
