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

#include "mm_util_gif.h"
#include "mm_util_debug.h"

static int __convert_gif_to_rgba(mm_util_gif_data *decoded, ColorMapObject *color_map, GifRowType *screen_buffer, unsigned long width, unsigned long height)
{
	unsigned long i, j;
	GifRowType gif_row;
	GifColorType *color_map_entry;
	GifByteType *buffer;

	mm_util_debug("__convert_gif_to_rgba");
	if ((decoded->frames = (mm_util_gif_frame_data **) realloc(decoded->frames, sizeof(mm_util_gif_frame_data *)))
		== NULL) {
		mm_util_error("Failed to allocate memory required, aborted.");
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}
	if ((decoded->frames[0] = (mm_util_gif_frame_data *) calloc(1, sizeof(mm_util_gif_frame_data)))
		== NULL) {
		mm_util_error("Failed to allocate memory required, aborted.");
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	if ((decoded->frames[0]->data = (void *)malloc(width * height * 4)) == NULL) {
		mm_util_error("Failed to allocate memory required, aborted.");
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	buffer = (GifByteType *) decoded->frames[0]->data;
	for (i = 0; i < height; i++) {
		gif_row = screen_buffer[i];
		for (j = 0; j < width; j++) {
			color_map_entry = &color_map->Colors[gif_row[j]];
			*buffer++ = color_map_entry->Red;
			*buffer++ = color_map_entry->Green;
			*buffer++ = color_map_entry->Blue;
			*buffer++ = 255;
		}
	}
	return MM_UTIL_ERROR_NONE;
}

static int __read_function(GifFileType *gft, GifByteType *data, int size)
{
	read_data *read_data_ptr = (read_data *) gft->UserData;

	if (read_data_ptr->mem && size > 0) {
		memcpy(data, read_data_ptr->mem + read_data_ptr->size, size);
		read_data_ptr->size += size;
	}
	return size;
}

static int __read_gif(mm_util_gif_data *decoded, const char *filename, void *memory)
{
	int ExtCode, i, j, Row, Col, Width, Height;
	unsigned long Size;
	GifRecordType record_type;
	GifRowType *screen_buffer = NULL;
	GifFileType *GifFile;
	unsigned int image_num = 0;
	ColorMapObject *ColorMap;
	read_data read_data_ptr;
	int ret;

	mm_util_debug("mm_util_decode_from_gif");
	if (filename) {
		if ((GifFile = DGifOpenFileName(filename, NULL)) == NULL) {
			mm_util_error("could not open Gif File");
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
	} else if (memory) {
		read_data_ptr.mem = memory;
		read_data_ptr.size = 0;
		if ((GifFile = DGifOpen(&read_data_ptr, __read_function, NULL)) == NULL) {
			mm_util_error("could not open Gif File");
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
	} else {
		mm_util_error("Gif File wrong decode parameters");
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	decoded->width = GifFile->SWidth;
	decoded->height = GifFile->SHeight;
	decoded->size = (unsigned long long)GifFile->SWidth * (unsigned long long)GifFile->SHeight * 4;

	if ((screen_buffer = (GifRowType *)
		 malloc(GifFile->SHeight * sizeof(GifRowType))) == NULL) {
		mm_util_error("Failed to allocate memory required, aborted.");
		ret = MM_UTIL_ERROR_INVALID_OPERATION;
		goto error;
	}

	Size = GifFile->SWidth * sizeof(GifPixelType);	/* Size in bytes one row. */
	if ((screen_buffer[0] = (GifRowType) malloc(Size)) == NULL) {	/* First row. */
		mm_util_error("Failed to allocate memory required, aborted.");
		ret = MM_UTIL_ERROR_INVALID_OPERATION;
		goto error;
	}

	for (i = 0; i < GifFile->SWidth; i++)	/* Set its color to BackGround. */
		screen_buffer[0][i] = GifFile->SBackGroundColor;
	for (i = 1; i < GifFile->SHeight; i++) {
		/* Allocate the other rows, and set their color to background too: */
		if ((screen_buffer[i] = (GifRowType) malloc(Size)) == NULL) {
			mm_util_error("Failed to allocate memory required, aborted.");
			ret = MM_UTIL_ERROR_INVALID_OPERATION;
			goto error;
		}

		memcpy(screen_buffer[i], screen_buffer[0], Size);
	}

	/* Scan the content of the GIF file and load the image(s) in: */
	do {
		if (DGifGetRecordType(GifFile, &record_type) == GIF_ERROR) {
			mm_util_error("could not get record type");
			ret = MM_UTIL_ERROR_INVALID_OPERATION;
			goto error;
		}
		switch (record_type) {
		case IMAGE_DESC_RECORD_TYPE:
			if (DGifGetImageDesc(GifFile) == GIF_ERROR) {
				mm_util_error("could not get image description");
				ret = MM_UTIL_ERROR_INVALID_OPERATION;
				goto error;
			}
			Row = GifFile->Image.Top;	/* Image Position relative to Screen. */
			Col = GifFile->Image.Left;
			Width = GifFile->Image.Width;
			Height = GifFile->Image.Height;
			mm_util_debug("Image %d at (%d, %d) [%dx%d]:     ", image_num + 1, Col, Row, Width, Height);
			if (GifFile->Image.Left + GifFile->Image.Width > GifFile->SWidth || GifFile->Image.Top + GifFile->Image.Height > GifFile->SHeight) {
				mm_util_debug("Image %d is not confined to screen dimension, aborted.", image_num + 1);
				ret = MM_UTIL_ERROR_INVALID_OPERATION;
				goto error;
			}
			image_num++;
			if (GifFile->Image.Interlace) {
				int interlaced_offset[] = { 0, 4, 2, 1 }, interlaced_jumps[] = {
				8, 8, 4, 2};
				/* Need to perform 4 passes on the images: */
				for (i = 0; i < 4; i++)
					for (j = Row + interlaced_offset[i]; j < Row + Height; j += interlaced_jumps[i]) {
						if (DGifGetLine(GifFile, &screen_buffer[j][Col], Width) == GIF_ERROR) {
							mm_util_error("could not get line");
							ret = MM_UTIL_ERROR_INVALID_OPERATION;
							goto error;
						}
					}
			} else {
				for (i = 0; i < Height; i++) {
					if (DGifGetLine(GifFile, &screen_buffer[Row++][Col], Width) == GIF_ERROR) {
						mm_util_error("could not get line");
						ret = MM_UTIL_ERROR_INVALID_OPERATION;
						goto error;
					}
				}
			}
			break;
		case EXTENSION_RECORD_TYPE:
			{
				GifByteType *extension;
				/* Skip any extension blocks in file: */
				if (DGifGetExtension(GifFile, &ExtCode, &extension) == GIF_ERROR) {
					mm_util_error("could not get extension");
					ret = MM_UTIL_ERROR_INVALID_OPERATION;
					goto error;
				}
				while (extension != NULL) {
					if (DGifGetExtensionNext(GifFile, &extension) == GIF_ERROR) {
						mm_util_error("could not get next extension");
						ret = MM_UTIL_ERROR_INVALID_OPERATION;
						goto error;
					}
				}
			}
			break;
		case TERMINATE_RECORD_TYPE:
			break;
		default:				/* Should be trapped by DGifGetRecordType. */
			break;
		}
		if (image_num > 0)
			break;
	} while (record_type != TERMINATE_RECORD_TYPE);

	ColorMap = (GifFile->Image.ColorMap ? GifFile->Image.ColorMap : GifFile->SColorMap);
	if (ColorMap == NULL) {
		mm_util_error("Gif Image does not have a colormap\n");
		ret = MM_UTIL_ERROR_INVALID_OPERATION;
		goto error;
	}

	__convert_gif_to_rgba(decoded, ColorMap, screen_buffer, GifFile->SWidth, GifFile->SHeight);

	ret = MM_UTIL_ERROR_NONE;
error:
	if (screen_buffer) {
		if (screen_buffer[0])
			(void)free(screen_buffer[0]);
		for (i = 1; i < GifFile->SHeight; i++) {
			if (screen_buffer[i])
				(void)free(screen_buffer[i]);
		}
		(void)free(screen_buffer);
	}
	if (DGifCloseFile(GifFile, NULL) == GIF_ERROR) {
		mm_util_error("could not close file");
		ret = MM_UTIL_ERROR_INVALID_OPERATION;
	}
	return ret;
}

int mm_util_decode_from_gif_file(mm_util_gif_data *decoded, const char *fpath)
{
	int ret;

	mm_util_debug("mm_util_decode_from_gif_file");

	ret = __read_gif(decoded, fpath, NULL);

	return ret;
}

int mm_util_decode_from_gif_memory(mm_util_gif_data *decoded, void **memory)
{
	int ret;

	mm_util_debug("mm_util_decode_from_gif_memory");

	ret = __read_gif(decoded, NULL, *memory);

	return ret;
}

#if 0
unsigned long mm_util_decode_get_width(mm_util_gif_data *data)
{
	return data->width;
}

unsigned long mm_util_decode_get_height(mm_util_gif_data *data)
{
	return data->height;
}

unsigned long long mm_util_decode_get_size(mm_util_gif_data *data)
{
	return data->size;
}
#endif

static void __load_rgb_from_buffer(GifByteType *buffer, GifByteType **red, GifByteType **green, GifByteType **blue, unsigned long width, unsigned long height)
{
	unsigned long i, j;
	unsigned long Size;
	GifByteType *redP, *greenP, *blueP;

	mm_util_debug("__load_rgb_from_buffer");
	Size = ((long)width) * height * sizeof(GifByteType);

	if ((*red = (GifByteType *) malloc((unsigned int)Size)) == NULL || (*green = (GifByteType *) malloc((unsigned int)Size)) == NULL || (*blue = (GifByteType *) malloc((unsigned int)Size)) == NULL) {
		mm_util_error("Failed to allocate memory required, aborted.");
		return;
	}

	redP = *red;
	greenP = *green;
	blueP = *blue;

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			*redP++ = *buffer++;
			*greenP++ = *buffer++;
			*blueP++ = *buffer++;
			buffer++;
		}
	}
	return;
}

#define ABS(x)    ((x) > 0 ? (x) : (-(x)))
static int __save_buffer_to_gif(GifFileType *GifFile, GifByteType *OutputBuffer, ColorMapObject *OutputColorMap, mm_util_gif_frame_data * frame)
{
	unsigned long i, j, pixels = 0;
	GifByteType *Ptr = OutputBuffer;
	static unsigned char extension[4] = {0, };
	int z;
	int transparent_index = 0, minDist = 256 * 256 * 256;
	GifColorType *color;

	mm_util_debug("__save_buffer_to_gif");

	/* Find closest color in first color map for the transparent color. */
	color = OutputColorMap->Colors;
	for (z = 0; z < OutputColorMap->ColorCount; z++) {
		int dr = ABS(color[z].Red - frame->transparent_color.Red);
		int dg = ABS(color[z].Green - frame->transparent_color.Green);
		int db = ABS(color[z].Blue - frame->transparent_color.Blue);

		int dist = dr * dr + dg * dg + db * db;

		if (minDist > dist) {
			minDist = dist;
			transparent_index = z;
		}
	}

	extension[0] = frame->disposal_mode << 2;
	if(frame->is_transparent) {
		extension[0] |= 1;
		extension[3] = transparent_index;
	} else {
		extension[3] = -1;
	}
	extension[1] = frame->delay_time % 256;
	extension[2] = frame->delay_time / 256;

	/* Dump graphics control block. */
	EGifPutExtension(GifFile, GRAPHICS_EXT_FUNC_CODE, 4, extension);

	if (EGifPutImageDesc(GifFile, frame->x, frame->y, frame->width, frame->height, false, OutputColorMap) == GIF_ERROR) {
		mm_util_error("could not put image description");
		if (GifFile != NULL)
			EGifCloseFile(GifFile, NULL);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	for (i = 0; i < frame->height; i++) {
		for (j = 0; j < frame->width; j++) {
			unsigned char c = Ptr[pixels++];
			if (EGifPutPixel(GifFile, c) == GIF_ERROR) {
				mm_util_error("could not put pixel");
				if (GifFile != NULL)
					EGifCloseFile(GifFile, NULL);
				return MM_UTIL_ERROR_INVALID_OPERATION;
			}
		}
	}

	return MM_UTIL_ERROR_NONE;
}

static int __write_function(GifFileType *gft, const GifByteType *data, int size)
{
	write_data *write_data_ptr = (write_data *) gft->UserData;

	if (size > 0) {
		*(write_data_ptr->mem) = (void *)realloc(*(write_data_ptr->mem), (write_data_ptr->size + size));
		memcpy(*(write_data_ptr->mem) + write_data_ptr->size, data, size);
		write_data_ptr->size += size;
	}
	return size;
}

int mm_util_encode_open_gif_file(mm_util_gif_data *encoded, const char *filename)
{
	mm_util_debug("mm_util_encode_open_gif_file");
	if (filename) {
		if ((encoded->GifFile = EGifOpenFileName(filename, 0, NULL)) == NULL) {
			mm_util_error("could not open file");
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
	} else
		return MM_UTIL_ERROR_INVALID_PARAMETER;

	return MM_UTIL_ERROR_NONE;
}

int mm_util_encode_open_gif_memory(mm_util_gif_data *encoded, void **data)
{
	mm_util_debug("mm_util_encode_open_gif_memory");
	if (data) {
		encoded->write_data_ptr.mem = data;
		encoded->write_data_ptr.size = 0;

		if ((encoded->GifFile = EGifOpen(&(encoded->write_data_ptr), __write_function, NULL)) == NULL) {
			mm_util_error("could not open File");
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
	} else
		return MM_UTIL_ERROR_INVALID_PARAMETER;

	return MM_UTIL_ERROR_NONE;
}

int mm_util_encode_close_gif(mm_util_gif_data *encoded)
{
	mm_util_debug("mm_util_encode_close_gif");
	if (EGifCloseFile(encoded->GifFile, NULL) == GIF_ERROR) {
		mm_util_error("could not close file");
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	return MM_UTIL_ERROR_NONE;
}

static int __write_gif(mm_util_gif_data *encoded)
{
	int ColorMapSize;
	unsigned int i = encoded->current_count;
	GifByteType *red = NULL, *green = NULL, *blue = NULL, *OutputBuffer = NULL;
	ColorMapObject *OutputColorMap = NULL;

	if (!encoded->screen_desc_updated) {
		if (EGifPutScreenDesc(encoded->GifFile, encoded->frames[0]->width, encoded->frames[0]->height, 8, 0, NULL) == GIF_ERROR) {
			mm_util_error("could not put screen description");
			if (encoded->GifFile != NULL)
				EGifCloseFile(encoded->GifFile, NULL);
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
		encoded->screen_desc_updated = true;
	}

	for (i = encoded->current_count; i < encoded->image_count; i++) {
		if ((OutputBuffer = (GifByteType *) malloc(encoded->frames[i]->width * encoded->frames[i]->height * sizeof(GifByteType))) == NULL) {
			mm_util_error("Failed to allocate memory required, aborted.");
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}

		ColorMapSize = 1 << 8;
		__load_rgb_from_buffer((GifByteType *) encoded->frames[i]->data, &red, &green, &blue, encoded->frames[i]->width, encoded->frames[i]->height);

		if ((OutputColorMap = GifMakeMapObject(ColorMapSize, NULL)) == NULL) {
			mm_util_error("could not map object");
			free((char *)red);
			free((char *)green);
			free((char *)blue);
			free(OutputBuffer);
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
		if (GifQuantizeBuffer(encoded->frames[i]->width, encoded->frames[i]->height, &ColorMapSize, red, green, blue, OutputBuffer, OutputColorMap->Colors) == GIF_ERROR) {
			mm_util_error("could not quantize buffer");
			free((char *)red);
			free((char *)green);
			free((char *)blue);
			free(OutputBuffer);
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
		free((char *)red);
		free((char *)green);
		free((char *)blue);

		encoded->frames[i]->transparent_color.Red = 0xff;
		encoded->frames[i]->transparent_color.Green = 0xff;
		encoded->frames[i]->transparent_color.Blue = 0xff;

		if (!encoded->frames[i]->x && !encoded->frames[i]->y) {
			encoded->frames[i]->x = (encoded->frames[0]->width - encoded->frames[i]->width) / 2;
			encoded->frames[i]->y = (encoded->frames[0]->height - encoded->frames[i]->height) / 2;
		}

		encoded->frames[i]->disposal_mode = MM_UTIL_GIF_DISPOSAL_UNSPECIFIED;
		encoded->frames[i]->is_transparent = false;

		if (__save_buffer_to_gif(encoded->GifFile, OutputBuffer, OutputColorMap, encoded->frames[i]) != MM_UTIL_ERROR_NONE)
			return MM_UTIL_ERROR_INVALID_OPERATION;

		free(OutputBuffer);
		encoded->current_count++;
	}
	encoded->size = encoded->write_data_ptr.size;

	return MM_UTIL_ERROR_NONE;
}

int mm_util_encode_gif(mm_util_gif_data *encoded)
{
	int ret;

	mm_util_debug("mm_util_encode_gif");
	ret = __write_gif(encoded);

	return ret;
}

void mm_util_gif_encode_set_width(mm_util_gif_data *data, unsigned long width)
{
	data->width = width;
}

void mm_util_gif_encode_set_height(mm_util_gif_data *data, unsigned long height)
{
	data->height = height;
}

void mm_util_gif_encode_set_image_count(mm_util_gif_data *data, unsigned int image_count)
{
	data->image_count = image_count;
}

void mm_util_gif_encode_set_frame_delay_time(mm_util_gif_frame_data *frame, unsigned long long delay_time)
{
	frame->delay_time = delay_time;
}

void mm_util_gif_encode_set_frame_disposal_mode(mm_util_gif_frame_data *frame, mm_util_gif_disposal disposal_mode)
{
	frame->disposal_mode = disposal_mode;
}

void mm_util_gif_encode_set_frame_position(mm_util_gif_frame_data *frame, unsigned long x, unsigned long y)
{
	frame->x = x;
	frame->y = y;
}

void mm_util_gif_encode_set_frame_transparency_color(mm_util_gif_frame_data *frame, GifColorType transparent_color)
{
	frame->is_transparent = true;
	frame->transparent_color = transparent_color;
}
