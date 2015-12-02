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
#include <stdio.h>				/* fopen() */
#include <system_info.h>

#include <glib.h>

#include "mm_util_bmp.h"
#include "mm_util_debug.h"

#define BYTES_PER_PIXEL 4

void *bitmap_create(int width, int height, unsigned int state);
unsigned char *bitmap_get_buffer(void *bitmap);
size_t bitmap_get_bpp(void *bitmap);
void bitmap_destroy(void *bitmap);

unsigned char *load_file(const char *path, size_t * data_size)
{
	FILE *fd;
	struct stat sb;
	unsigned char *buffer;
	size_t size;
	size_t n;

	fd = fopen(path, "rb");
	if (!fd) {
		mm_util_error("file open failed");
		return NULL;
	}

	if (stat(path, &sb)) {
		mm_util_error("file stat failed");
		fclose(fd);
		return NULL;
	}
	size = sb.st_size;

	buffer = malloc(size);
	if (!buffer) {
		mm_util_error("Unable to allocate %lld bytes", (long long)size);
		fclose(fd);
		return NULL;
	}

	n = fread(buffer, 1, size, fd);
	if (n != size) {
		mm_util_error("file read failed");
		free(buffer);
		return NULL;
	}

	fclose(fd);
	*data_size = size;
	return buffer;
}

void print_error(const char *context, bmp_result code)
{
	switch (code) {
	case BMP_INSUFFICIENT_MEMORY:
		mm_util_error("%s failed: BMP_INSUFFICIENT_MEMORY", context);
		break;
	case BMP_INSUFFICIENT_DATA:
		mm_util_error("%s failed: BMP_INSUFFICIENT_DATA", context);
		break;
	case BMP_DATA_ERROR:
		mm_util_error("%s failed: BMP_DATA_ERROR", context);
		break;
	default:
		mm_util_error("%s failed: unknown code %i", context, code);
		break;
	}
}

void *bitmap_create(int width, int height, unsigned int state)
{
	return calloc(width * height, BYTES_PER_PIXEL);
}

unsigned char *bitmap_get_buffer(void *bitmap)
{
	return bitmap;
}

size_t bitmap_get_bpp(void *bitmap)
{
	return BYTES_PER_PIXEL;
}

void bitmap_destroy(void *bitmap)
{
	free(bitmap);
}

static int read_bmp(mm_util_bmp_data * decoded, const char *filename, void *memory, unsigned long long src_size)
{
	bmp_bitmap_callback_vt bitmap_callbacks = {
		bitmap_create,
		bitmap_destroy,
		bitmap_get_buffer,
		bitmap_get_bpp
	};
	bmp_result code;
	bmp_image bmp;
	size_t size;
	int res = MM_UTIL_ERROR_NONE;
	unsigned char *data = NULL;

	if (filename) {
		data = load_file(filename, &size);
		if (data == NULL)
			return MM_UTIL_ERROR_INVALID_OPERATION;
	} else if (memory) {
		data = (unsigned char *)memory;
		size = src_size;
	} else {
		mm_util_error("Bmp File wrong decode parameters");
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	nsbmp_create(&bmp, &bitmap_callbacks);

	code = bmp_analyse(&bmp, size, data);
	if (code != BMP_OK) {
		print_error("bmp_analyse", code);
		res = MM_UTIL_ERROR_INVALID_OPERATION;
		goto cleanup;
	}

	code = bmp_decode(&bmp);

	if (code != BMP_OK) {
		print_error("bmp_decode", code);
		/* allow partially decoded images */
		if (code != BMP_INSUFFICIENT_DATA) {
			res = MM_UTIL_ERROR_INVALID_OPERATION;
			goto cleanup;
		}
	}

	decoded->width = bmp.width;
	decoded->height = bmp.height;
	decoded->size = bmp.width * bmp.height * BYTES_PER_PIXEL;
	decoded->data = malloc(decoded->size);
	memcpy(decoded->data, bmp.bitmap, decoded->size);

 cleanup:
	bmp_finalise(&bmp);
	if (filename)
		free(data);
	return res;
}

int mm_util_decode_from_bmp_file(mm_util_bmp_data * decoded, const char *filename)
{
	int ret;

	mm_util_debug("mm_util_decode_from_gif_file");

	ret = read_bmp(decoded, filename, NULL, 0);

	return ret;
}

int mm_util_decode_from_bmp_memory(mm_util_bmp_data * decoded, void **memory, unsigned long long src_size)
{
	int ret;

	mm_util_debug("mm_util_decode_from_gif_file");

	ret = read_bmp(decoded, NULL, *memory, src_size);

	return ret;
}

unsigned long mm_util_decode_get_width(mm_util_bmp_data * data)
{
	return data->width;
}

unsigned long mm_util_decode_get_height(mm_util_bmp_data * data)
{
	return data->height;
}

unsigned long long mm_util_decode_get_size(mm_util_bmp_data * data)
{
	return data->size;
}

int mm_util_encode_bmp_to_file(mm_util_bmp_data * encoded, const char *filename)
{
	bmpfile_t *bmp;
	rgb_pixel_t pixel = { 0, 0, 0, 0 };
	uint16_t row, col;
	uint8_t *image;

	if ((bmp = bmp_create(encoded->width, encoded->height, BYTES_PER_PIXEL * 8)) == NULL) {
		printf("Invalid depth value.\n");
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	image = (uint8_t *) encoded->data;
	for (row = 0; row != encoded->height; row++) {
		for (col = 0; col != encoded->width; col++) {
			size_t z = (row * encoded->width + col) * BYTES_PER_PIXEL;
			pixel.red = image[z];
			pixel.green = image[z + 1];
			pixel.blue = image[z + 2];
			bmp_set_pixel(bmp, col, row, pixel);
		}
	}

	bmp_save(bmp, filename);
	bmp_destroy(bmp);
	return MM_UTIL_ERROR_NONE;
}

void mm_util_bmp_encode_set_width(mm_util_bmp_data * data, unsigned long width)
{
	data->width = width;
}

void mm_util_bmp_encode_set_height(mm_util_bmp_data * data, unsigned long height)
{
	data->height = height;
}
