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
#include <mm_util_gif.h>

#define DECODE_RESULT_PATH "/media/decode_test."
#define TRUE  1
#define FALSE 0
#define BUFFER_SIZE 128

static inline void flush_stdin()
{
	int ch;
	while ((ch = getchar()) != EOF && ch != '\n') ;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	mm_util_gif_data decoded;

	if (argc < 2) {
		fprintf(stderr, "\t[usage]\n");
		fprintf(stderr, "\t\t1. decode : mm_util_gif_testsuite decode filepath.gif\n");
		return 0;
	}

	if (!strcmp("decode", argv[1])) {
		ret = mm_util_decode_from_gif_file(&decoded, argv[2]);
	} else {
		fprintf(stderr, "\tunknown command [%s]\n", argv[1]);
		return 0;
	}

	if (ret != MM_UTIL_ERROR_NONE) {
		fprintf(stderr, "\tERROR is occurred %x\n", ret);
	} else {
		fprintf(stderr, "\tGIF OPERATION SUCCESS\n");
		{
			if (decoded.frames) {
				fprintf(stderr, "\t##Decoded data##: %p\t width: %lu\t height:%lu\t size: %llu\n", decoded.frames[0], decoded.width, decoded.height, decoded.size);
				char filename[BUFFER_SIZE] = { 0, };
				memset(filename, 0, BUFFER_SIZE);
				{
					snprintf(filename, BUFFER_SIZE, "%s%s", DECODE_RESULT_PATH, "gif");
				}
				mm_util_gif_encode_set_image_count(&decoded, 1);
				ret = mm_util_encode_open_gif_file(&decoded, filename);
				ret = mm_util_encode_gif(&decoded);
				ret = mm_util_encode_close_gif(&decoded);

				free(decoded.frames[0]->data);
				free(decoded.frames);
			} else {
				fprintf(stderr, "\tDECODED data is NULL\n");
			}
		}
	}
	return 0;
}
