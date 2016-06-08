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

#include "mm_util_png.h"
#include "mm_util_debug.h"

#define MM_UTIL_PNG_BYTES_TO_CHECK 4
#define MM_UTIL_PROGRESSIVE_BYTES_DEFAULT 1024

#define png_infopp_NULL (png_infopp)NULL
#define png_voidp_NULL (png_voidp)NULL

typedef struct {
	void *mem;
	png_size_t size;
} read_data;

#if 0
bool mm_util_check_if_png(const char *fpath)
{
	char buf[MM_UTIL_PNG_BYTES_TO_CHECK];
	FILE *fp;

	mm_util_debug("mm_util_check_if_png");
	if ((fp = fopen(fpath, "rb")) == NULL)
		return MM_UTIL_ERROR_NO_SUCH_FILE;

	if (fread(buf, 1, MM_UTIL_PNG_BYTES_TO_CHECK, fp) != MM_UTIL_PNG_BYTES_TO_CHECK) {
		fclose(fp);
		return 0;
	}

	fclose(fp);
	fp = NULL;
	return png_check_sig((void *)buf, MM_UTIL_PNG_BYTES_TO_CHECK);
}
#endif

static void __user_error_fn(png_structp png_ptr, png_const_charp error_msg)
{
	mm_util_error("%s", error_msg);
}

static void __user_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
	mm_util_error("%s", warning_msg);
}

static void __dec_set_prop(mm_util_png_data *decoded, png_structp png_ptr, png_infop info)
{
	png_color_16 my_background, *image_background;

	mm_util_debug("__dec_set_prop");

	png_read_info(png_ptr, info);

	png_get_IHDR(png_ptr, info, &decoded->width, &decoded->height, &decoded->png.bit_depth, &decoded->png.color_type, &decoded->png.interlace_type, NULL, &decoded->png.filter_type);

	/* Get bits per channel */
	decoded->png.bit_depth = png_get_bit_depth(png_ptr, info);

	/* Get Color type */
	decoded->png.color_type = png_get_color_type(png_ptr, info);

	/* Gray scale converted to upscaled to 8 bits */
	if ((decoded->png.color_type == PNG_COLOR_TYPE_GRAY_ALPHA) || (decoded->png.color_type == PNG_COLOR_TYPE_GRAY)) {
		/* Gray scale with alpha channel converted to RGB */
		if (decoded->png.color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(png_ptr);
		if (decoded->png.bit_depth < MM_UTIL_BIT_DEPTH_8)/* Convert to 8 bits */
			png_set_expand_gray_1_2_4_to_8(png_ptr);
	}
	/* Palette converted to RGB */
	else if (decoded->png.color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	/* Add alpha channel, but not for GRAY images */
	if ((decoded->png.bit_depth >= MM_UTIL_BIT_DEPTH_8)
		&& (decoded->png.color_type != PNG_COLOR_TYPE_GRAY)) {
		png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
	}

	if (png_get_valid(png_ptr, info, PNG_INFO_tRNS) != 0)
		png_set_tRNS_to_alpha(png_ptr);

	if (png_get_bKGD(png_ptr, info, &image_background) != 0)
		png_set_background(png_ptr, image_background, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
	else
		png_set_background(png_ptr, &my_background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);

	png_set_interlace_handling(png_ptr);

	/* Update the info structure */
	png_read_update_info(png_ptr, info);

	decoded->png.rowbytes = png_get_rowbytes(png_ptr, info);
	decoded->size = decoded->height * decoded->png.rowbytes;
}

static void __read_function(png_structp png_ptr, png_bytep data, png_size_t size)
{
	read_data *read_data_ptr = (read_data *) png_get_io_ptr(png_ptr);

	if (read_data_ptr->mem && size > 0) {
		memcpy(data, read_data_ptr->mem + read_data_ptr->size, size);
		read_data_ptr->size += size;
	}
}

static int __read_png(mm_util_png_data *decoded, FILE * fp, void *memory)
{
	png_structp png_ptr;
	png_infop info_ptr;
	guint row_index;
	read_data read_data_ptr;

	mm_util_debug("__read_png");
	decoded->data = NULL;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp) NULL, __user_error_fn, __user_warning_fn);

	if (png_ptr == NULL) {
		mm_util_error("could not create read struct");
		if (fp)
			fclose(fp);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		mm_util_error("could not create info struct");
		if (fp)
			fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		mm_util_error("setjmp called due to internal libpng error");
		if (decoded->data) {
			png_free(png_ptr, decoded->data);
			decoded->data = NULL;
			mm_util_debug("free data");
		}
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		if (fp)
			fclose(fp);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	if (fp)
		png_init_io(png_ptr, fp);
	else if (memory) {
		read_data_ptr.mem = memory;
		read_data_ptr.size = 0;
		png_set_read_fn(png_ptr, &read_data_ptr, __read_function);
	} else {
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	__dec_set_prop(decoded, png_ptr, info_ptr);

	{
		png_bytep row_pointers[decoded->height];

		for (row_index = 0; row_index < decoded->height; row_index++)
			row_pointers[row_index] = png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));

		png_read_image(png_ptr, row_pointers);

		decoded->data = (void *)png_malloc(png_ptr, sizeof(png_bytep) * decoded->size);

		for (row_index = 0; row_index < decoded->height; row_index++) {
			memcpy(decoded->data + (row_index * decoded->png.rowbytes), row_pointers[row_index], decoded->png.rowbytes);
			png_free(png_ptr, row_pointers[row_index]);
		}
	}

	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
	if (fp)
		fclose(fp);

	return MM_UTIL_ERROR_NONE;
}

static void user_info_callback(png_structp png_ptr, png_infop info)
{
	mm_util_png_data *decoded = (mm_util_png_data *) png_get_io_ptr(png_ptr);

	mm_util_debug("user_info_callback");
	__dec_set_prop(decoded, png_ptr, info);

	/* Allocate output buffer */
	decoded->data = (void *)png_malloc(png_ptr, sizeof(png_bytep) * decoded->size);
}

static void user_endrow_callback(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass)
{
	mm_util_png_data *decoded = (mm_util_png_data *) png_get_io_ptr(png_ptr);

	if (new_row) {
		png_uint_32 offset = row_num * decoded->png.rowbytes;

		png_progressive_combine_row(png_ptr, decoded->data + offset, new_row);
	}
}

static void user_end_callback(png_structp png_ptr, png_infop info)
{
	mm_util_debug("and we are done reading this image");
}

static int __read_png_progressive(mm_util_png_data *decoded, FILE * fp, void **memory, unsigned long long src_size)
{
	png_structp png_ptr;
	png_infop info_ptr;
	static unsigned long long old_length = 0;

	mm_util_debug("__read_png_progressive");
	decoded->data = NULL;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp) NULL, __user_error_fn, __user_warning_fn);

	if (png_ptr == NULL) {
		mm_util_error("could not create read struct");
		if (fp)
			fclose(fp);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		mm_util_error("could not create info struct");
		if (fp)
			fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		mm_util_error("setjmp called due to internal libpng error");
		if (decoded->data) {
			png_free(png_ptr, decoded->data);
			decoded->data = NULL;
			mm_util_debug("free data");
		}
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		if (fp)
			fclose(fp);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	png_set_progressive_read_fn(png_ptr, decoded, user_info_callback, user_endrow_callback, user_end_callback);

	while (1) {
		char buf[decoded->png.progressive_bytes];
		unsigned long long bytes_read = 0;

		if (fp) {
			if (!(bytes_read = fread(buf, 1, decoded->png.progressive_bytes, fp)))
				break;
		} else if (*memory) {
			unsigned long long length = src_size - old_length;

			if (length == 0)
				break;
			else if (length >= decoded->png.progressive_bytes)
				bytes_read = decoded->png.progressive_bytes;
			else
				bytes_read = length;

			memcpy(buf, *memory + old_length, bytes_read);

			old_length += bytes_read;
		} else
			break;
		png_process_data(png_ptr, info_ptr, (png_bytep) buf, bytes_read);
	};

	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
	if (fp)
		fclose(fp);

	return MM_UTIL_ERROR_NONE;
}

int mm_util_decode_from_png_file(mm_util_png_data *decoded, const char *fpath)
{
	int ret = MM_UTIL_ERROR_NONE;
	FILE *fp;

	mm_util_debug("mm_util_decode_from_png");
	if ((fp = fopen(fpath, "r")) == NULL)
		return MM_UTIL_ERROR_NO_SUCH_FILE;

	if (fp) {
		if (decoded->png.progressive)
			ret = __read_png_progressive(decoded, fp, NULL, 0);
		else
			ret = __read_png(decoded, fp, NULL);
	}

	return ret;
}

int mm_util_decode_from_png_memory(mm_util_png_data *decoded, void **memory, unsigned long long src_size)
{
	int ret = MM_UTIL_ERROR_NONE;

	mm_util_debug("mm_util_decode_from_memory");
	if (decoded->png.progressive)
		ret = __read_png_progressive(decoded, NULL, memory, src_size);
	else
		ret = __read_png(decoded, NULL, *memory);

	return ret;
}

void mm_util_init_decode_png(mm_util_png_data *data)
{
	mm_util_debug("mm_util_init_decode_png");
	data->png.progressive = 0;
	data->png.progressive_bytes = MM_UTIL_PROGRESSIVE_BYTES_DEFAULT;
	data->png.transform = MM_UTIL_PNG_TRANSFORM_IDENTITY;
}

#if 0
void mm_util_png_decode_set_progressive(mm_util_png_data *data, int progressive)
{
	data->png.progressive = progressive;
}

void mm_util_png_decode_set_progressive_bytes(mm_util_png_data *data, int progressive_bytes)
{
	data->png.progressive_bytes = progressive_bytes;
}

void mm_util_png_decode_set_transform(mm_util_png_data *data, int transform)
{
	data->png.transform = transform;
}

png_uint_32 mm_util_png_decode_get_width(mm_util_png_data *data)
{
	return data->width;
}

png_uint_32 mm_util_png_decode_get_height(mm_util_png_data *data)
{
	return data->height;
}

int mm_util_png_decode_get_bit_depth(mm_util_png_data *data)
{
	return data->png.bit_depth;
}

png_uint_32 mm_util_png_decode_get_size(mm_util_png_data *data)
{
	return data->size;
}
#endif
static void user_flush_data(png_structp png_ptr G_GNUC_UNUSED)
{
}

static void user_write_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
	mm_util_png_data *encoded = (mm_util_png_data *) png_get_io_ptr(png_ptr);

	if (data) {
		encoded->data = (void *)realloc(encoded->data, sizeof(png_bytep) * (encoded->size + length));

		memcpy(encoded->data + encoded->size, data, length);
		encoded->size += length;
	}
}

int write_png(void **data, mm_util_png_data *encoded, FILE *fp)
{
	png_structp png_ptr;
	png_infop info_ptr;
	guint row_index;
	static png_bytepp row_pointers = NULL;

	mm_util_debug("write_png");
	encoded->data = NULL;
	encoded->size = 0;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp) NULL, __user_error_fn, __user_warning_fn);

	if (png_ptr == NULL) {
		mm_util_error("could not create write struct");
		if (fp)
			fclose(fp);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		mm_util_error("could not create info struct");
		png_destroy_write_struct(&png_ptr, png_infopp_NULL);
		if (fp)
			fclose(fp);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		mm_util_error("setjmp called due to internal libpng error");
		if (encoded->data) {
			png_free(png_ptr, encoded->data);
			encoded->data = NULL;
			mm_util_debug("free data");
		}
		if (row_pointers)
			png_free(png_ptr, row_pointers);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		if (fp)
			fclose(fp);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	png_set_filter(png_ptr, 0, encoded->png.filter);
	png_set_compression_level(png_ptr, encoded->png.compression_level);

	png_set_IHDR(png_ptr, info_ptr, encoded->width, encoded->height, encoded->png.bit_depth, encoded->png.color_type, encoded->png.interlace_type, PNG_COMPRESSION_TYPE_BASE, encoded->png.filter_type);

	if (fp)
		png_init_io(png_ptr, fp);
	else
		png_set_write_fn(png_ptr, encoded, (png_rw_ptr) user_write_data, user_flush_data);

	encoded->png.rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	row_pointers = png_malloc(png_ptr, sizeof(png_bytep) * encoded->height);

	for (row_index = 0; row_index < encoded->height; row_index++) {
		row_pointers[row_index] = (*data) + (row_index * encoded->png.rowbytes);
	}

	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, NULL);

	png_free(png_ptr, row_pointers);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	if (fp)
		fclose(fp);

	return MM_UTIL_ERROR_NONE;
}

int mm_util_encode_to_png_file(void **data, mm_util_png_data *encoded, const char *fpath)
{
	int ret = MM_UTIL_ERROR_NONE;
	FILE *fp;

	mm_util_debug("mm_util_encode_to_png");
	if ((fp = fopen(fpath, "w")) == NULL)
		return MM_UTIL_ERROR_NO_SUCH_FILE;

	ret = write_png(data, encoded, fp);

	return ret;
}

int mm_util_encode_to_png_memory(void **data, mm_util_png_data *encoded)
{
	int ret;

	mm_util_debug("mm_util_encode_to_memory");
	ret = write_png(data, encoded, NULL);

	return ret;
}

void mm_util_init_encode_png(mm_util_png_data *data)
{
	mm_util_debug("mm_util_init_encode_png");
	data->png.compression_level = MM_UTIL_COMPRESSION_6;
	data->png.filter = MM_UTIL_PNG_FILTER_NONE;
	data->png.color_type = MM_UTIL_PNG_COLOR_TYPE_RGB_ALPHA;
	data->png.filter_type = MM_UTIL_PNG_FILTER_TYPE_BASE;
	data->png.interlace_type = MM_UTIL_PNG_INTERLACE_NONE;
	data->png.bit_depth = MM_UTIL_BIT_DEPTH_8;
}

void mm_util_png_encode_set_compression_level(mm_util_png_data *data, mm_util_png_compression compression_level)
{
	data->png.compression_level = compression_level;
}

void mm_util_png_encode_set_filter(mm_util_png_data *data, mm_util_png_filter filter)
{
	data->png.filter = filter;
}

void mm_util_png_encode_set_color_type(mm_util_png_data *data, mm_util_png_color_type color_type)
{
	data->png.color_type = color_type;
}

void mm_util_png_encode_set_filter_type(mm_util_png_data *data, mm_util_png_filter_type filter_type)
{
	data->png.filter_type = filter_type;
}

void mm_util_png_encode_set_interlace_type(mm_util_png_data *data, mm_util_png_interlace_type interlace_type)
{
	data->png.interlace_type = interlace_type;
}

void mm_util_png_encode_set_width(mm_util_png_data *data, png_uint_32 width)
{
	data->width = width;
}

void mm_util_png_encode_set_height(mm_util_png_data *data, png_uint_32 height)
{
	data->height = height;
}

void mm_util_png_encode_set_bit_depth(mm_util_png_data *data, mm_util_png_bit_depth bit_depth)
{
	data->png.bit_depth = bit_depth;
}
