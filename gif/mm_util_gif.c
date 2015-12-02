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

#include "mm_util_gif.h"
#include "mm_util_debug.h"

typedef struct {
	void *mem;
	unsigned long long size;
} read_data;
typedef struct {
	void **mem;
	unsigned long long size;
} write_data;

static void convert_gif_to_rgba(mm_util_gif_data * decoded, ColorMapObject * color_map, GifRowType * screen_buffer, unsigned long width, unsigned long height)
{
	unsigned long i, j;
	GifRowType gif_row;
	GifColorType *color_map_entry;
	GifByteType *buffer;

	mm_util_debug("convert_gif_to_rgba");
	if ((decoded->frames = (mm_util_gif_frame_data *) malloc(sizeof(mm_util_gif_frame_data)))
		== NULL) {
		mm_util_error("Failed to allocate memory required, aborted.");
		return;
	}

	if ((decoded->frames[0].data = (void *)malloc(width * height * 4)) == NULL)
		mm_util_error("Failed to allocate memory required, aborted.");

	buffer = (GifByteType *) decoded->frames[0].data;
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
}

static int read_function(GifFileType * gft, GifByteType * data, int size)
{
	read_data *read_data_ptr = (read_data *) gft->UserData;

	if (read_data_ptr->mem && size > 0) {
		memcpy(data, read_data_ptr->mem + read_data_ptr->size, size);
		read_data_ptr->size += size;
	}
	return size;
}

int read_gif(mm_util_gif_data * decoded, const char *filename, void *memory)
{
	int ExtCode, i, j, Row, Col, Width, Height;
	unsigned long Size;
	GifRecordType record_type;
	GifRowType *screen_buffer;
	GifFileType *GifFile;
	unsigned int image_num = 0;
	ColorMapObject *ColorMap;
	read_data read_data_ptr;

	mm_util_debug("mm_util_decode_from_gif");
	if (filename) {
		if ((GifFile = DGifOpenFileName(filename)) == NULL) {
			mm_util_error("could not open Gif File");
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
	} else if (memory) {
		read_data_ptr.mem = memory;
		read_data_ptr.size = 0;
		if ((GifFile = DGifOpen(&read_data_ptr, read_function)) == NULL) {
			mm_util_error("could not open Gif File");
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
	} else {
		mm_util_error("Gif File wrong decode parameters");
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	decoded->width = GifFile->SWidth;
	decoded->height = GifFile->SHeight;
	decoded->size = GifFile->SWidth * GifFile->SHeight * 4;

	if ((screen_buffer = (GifRowType *)
		 malloc(GifFile->SHeight * sizeof(GifRowType))) == NULL)
		mm_util_error("Failed to allocate memory required, aborted.");

	Size = GifFile->SWidth * sizeof(GifPixelType);	/* Size in bytes one row. */
	if ((screen_buffer[0] = (GifRowType) malloc(Size)) == NULL)	/* First row. */
		mm_util_error("Failed to allocate memory required, aborted.");

	for (i = 0; i < GifFile->SWidth; i++)	/* Set its color to BackGround. */
		screen_buffer[0][i] = GifFile->SBackGroundColor;
	for (i = 1; i < GifFile->SHeight; i++) {
		/* Allocate the other rows, and set their color to background too: */
		if ((screen_buffer[i] = (GifRowType) malloc(Size)) == NULL)
			mm_util_error("Failed to allocate memory required, aborted.");

		memcpy(screen_buffer[i], screen_buffer[0], Size);
	}

	/* Scan the content of the GIF file and load the image(s) in: */
	do {
		if (DGifGetRecordType(GifFile, &record_type) == GIF_ERROR) {
			mm_util_error("could not get record type");
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
		switch (record_type) {
		case IMAGE_DESC_RECORD_TYPE:
			if (DGifGetImageDesc(GifFile) == GIF_ERROR) {
				mm_util_error("could not get image description");
				return MM_UTIL_ERROR_INVALID_OPERATION;
			}
			Row = GifFile->Image.Top;	/* Image Position relative to Screen. */
			Col = GifFile->Image.Left;
			Width = GifFile->Image.Width;
			Height = GifFile->Image.Height;
			mm_util_debug("Image %d at (%d, %d) [%dx%d]:     ", ++image_num, Col, Row, Width, Height);
			if (GifFile->Image.Left + GifFile->Image.Width > GifFile->SWidth || GifFile->Image.Top + GifFile->Image.Height > GifFile->SHeight) {
				mm_util_debug("Image %d is not confined to screen dimension, aborted.", image_num);
				exit(EXIT_FAILURE);
			}

			if (GifFile->Image.Interlace) {
				int interlaced_offset[] = { 0, 4, 2, 1 }, interlaced_jumps[] = {
				8, 8, 4, 2};
				/* Need to perform 4 passes on the images: */
				for (i = 0; i < 4; i++)
					for (j = Row + interlaced_offset[i]; j < Row + Height; j += interlaced_jumps[i]) {
						if (DGifGetLine(GifFile, &screen_buffer[j][Col], Width) == GIF_ERROR) {
							mm_util_error("could not get line");
							return MM_UTIL_ERROR_INVALID_OPERATION;
						}
					}
			} else {
				for (i = 0; i < Height; i++) {
					if (DGifGetLine(GifFile, &screen_buffer[Row++][Col], Width) == GIF_ERROR) {
						mm_util_error("could not get line");
						return MM_UTIL_ERROR_INVALID_OPERATION;
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
					return MM_UTIL_ERROR_INVALID_OPERATION;
				}
				while (extension != NULL) {
					if (DGifGetExtensionNext(GifFile, &extension) == GIF_ERROR) {
						mm_util_error("could not get next extension");
						return MM_UTIL_ERROR_INVALID_OPERATION;
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
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	convert_gif_to_rgba(decoded, ColorMap, screen_buffer, GifFile->SWidth, GifFile->SHeight);
	(void)free(screen_buffer);

	if (DGifCloseFile(GifFile) == GIF_ERROR) {
		mm_util_error("could not close file");
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}
	return MM_UTIL_ERROR_NONE;
}

int mm_util_decode_from_gif_file(mm_util_gif_data * decoded, const char *fpath)
{
	int ret;

	mm_util_debug("mm_util_decode_from_gif_file");

	ret = read_gif(decoded, fpath, NULL);

	return ret;
}

int mm_util_decode_from_gif_memory(mm_util_gif_data * decoded, void **memory)
{
	int ret;

	mm_util_debug("mm_util_decode_from_gif_memory");

	ret = read_gif(decoded, NULL, *memory);

	return ret;
}

unsigned long mm_util_decode_get_width(mm_util_gif_data * data)
{
	return data->width;
}

unsigned long mm_util_decode_get_height(mm_util_gif_data * data)
{
	return data->height;
}

unsigned long long mm_util_decode_get_size(mm_util_gif_data * data)
{
	return data->size;
}

static void load_rgb_from_buffer(GifByteType * buffer, GifByteType ** red, GifByteType ** green, GifByteType ** blue, unsigned long width, unsigned long height)
{
	unsigned long i, j;
	unsigned long Size;
	GifByteType *redP, *greenP, *blueP;

	mm_util_debug("load_rgb_from_buffer");
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

static int save_buffer_to_gif(GifFileType * GifFile, GifByteType * OutputBuffer, unsigned long width, unsigned long height, unsigned long long delay_time)
{
	unsigned long i;
	GifByteType *Ptr = OutputBuffer;
	static unsigned char extension[4] = { 0x04, 0x00, 0x00, 0xff };

	mm_util_debug("save_buffer_to_gif");
	extension[0] = (false) ? 0x06 : 0x04;
	extension[1] = delay_time % 256;
	extension[2] = delay_time / 256;

	/* Dump graphics control block. */
	EGifPutExtension(GifFile, GRAPHICS_EXT_FUNC_CODE, 4, extension);

	if (EGifPutImageDesc(GifFile, 0, 0, width, height, false, NULL) == GIF_ERROR) {
		mm_util_error("could not put image description");
		if (GifFile != NULL)
			EGifCloseFile(GifFile);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	mm_util_debug(": Image 1 at (%d, %d) [%dx%d]:     ", GifFile->Image.Left, GifFile->Image.Top, GifFile->Image.Width, GifFile->Image.Height);

	for (i = 0; i < height; i++) {
		if (EGifPutLine(GifFile, Ptr, width) == GIF_ERROR) {
			mm_util_error("could not put line");
			if (GifFile != NULL)
				EGifCloseFile(GifFile);
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
		Ptr += width;
	}

	return MM_UTIL_ERROR_NONE;
}

static void outputbuffer_free(GifByteType ** OutputBuffer, unsigned int image_count)
{
	unsigned int j;

	for (j = 0; j < image_count; j++) {
		if (OutputBuffer[j] != NULL)
			free((char *)OutputBuffer[j]);
		else
			break;
	}
	free(OutputBuffer);
}

static int write_function(GifFileType * gft, const GifByteType * data, int size)
{
	write_data *write_data_ptr = (write_data *) gft->UserData;

	if (size > 0) {
		*(write_data_ptr->mem) = (void *)realloc(*(write_data_ptr->mem), (write_data_ptr->size + size));
		memcpy(*(write_data_ptr->mem) + write_data_ptr->size, data, size);
		write_data_ptr->size += size;
	}
	return size;
}

int write_gif(mm_util_gif_data * encoded, const char *filename, void **data)
{
	int ColorMapSize;
	unsigned int i = 0;
	GifByteType *red = NULL, *green = NULL, *blue = NULL, **OutputBuffer = NULL;
	ColorMapObject *OutputColorMap = NULL;
	GifFileType *GifFile;
	write_data write_data_ptr;

	ColorMapSize = 1 << 8;
	OutputBuffer = malloc(sizeof(GifByteType *) * encoded->image_count);

	for (i = 0; i < encoded->image_count; i++) {
		if ((OutputBuffer[i] = (GifByteType *) malloc(encoded->width * encoded->height * sizeof(GifByteType))) == NULL) {
			mm_util_error("Failed to allocate memory required, aborted.");
			outputbuffer_free(OutputBuffer, encoded->image_count);
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}

		if (i == 0) {
			load_rgb_from_buffer((GifByteType *) encoded->frames[i].data, &red, &green, &blue, encoded->width, encoded->height);

			if ((OutputColorMap = MakeMapObject(ColorMapSize, NULL)) == NULL) {
				mm_util_error("could not map object");
				outputbuffer_free(OutputBuffer, encoded->image_count);
				free((char *)red);
				free((char *)green);
				free((char *)blue);
				return MM_UTIL_ERROR_INVALID_OPERATION;
			}
			if (QuantizeBuffer(encoded->width, encoded->height, &ColorMapSize, red, green, blue, OutputBuffer[i], OutputColorMap->Colors) == GIF_ERROR) {
				mm_util_error("could not quantize buffer");
				outputbuffer_free(OutputBuffer, encoded->image_count);
				free((char *)red);
				free((char *)green);
				free((char *)blue);
				return MM_UTIL_ERROR_INVALID_OPERATION;
			}
			free((char *)red);
			free((char *)green);
			free((char *)blue);
		} else {
			unsigned long x, y;
			int z;
			unsigned long long npix = encoded->width * encoded->height;
			GifByteType *buffer = (GifByteType *) encoded->frames[i].data;

			for (x = 0, y = 0; x < npix; x++) {
				int minIndex = 0, minDist = 3 * 256 * 256;
				GifColorType *color = OutputColorMap->Colors;

				/* Find closest color in first color map to this color. */
				for (z = 0; z < OutputColorMap->ColorCount; z++) {
					int dr = (color[z].Red - buffer[y]);
					int dg = (color[z].Green - buffer[y + 1]);
					int db = (color[z].Blue - buffer[y + 2]);

					int dist = dr * dr + dg * dg + db * db;

					if (minDist > dist) {
						minDist = dist;
						minIndex = z;
					}
				}
				y += 3;
				OutputBuffer[i][x] = minIndex;
			}
		}
	}

	if (filename) {
		if ((GifFile = EGifOpenFileName(filename, 0)) == NULL) {
			mm_util_error("could not open file");
			outputbuffer_free(OutputBuffer, encoded->image_count);
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
	} else {
		write_data_ptr.mem = data;
		write_data_ptr.size = 0;

		if ((GifFile = EGifOpen(&write_data_ptr, write_function)) == NULL) {
			mm_util_error("could not open File");
			outputbuffer_free(OutputBuffer, encoded->image_count);
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
	}

	if (EGifPutScreenDesc(GifFile, encoded->width, encoded->height, 8, 0, OutputColorMap) == GIF_ERROR) {
		mm_util_error("could not put screen description");
		if (GifFile != NULL)
			EGifCloseFile(GifFile);
		outputbuffer_free(OutputBuffer, encoded->image_count);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	for (i = 0; i < encoded->image_count; i++) {
		if (save_buffer_to_gif(GifFile, OutputBuffer[i], encoded->width, encoded->height, encoded->frames[i].delay_time) != MM_UTIL_ERROR_NONE) {
			outputbuffer_free(OutputBuffer, encoded->image_count);
			return MM_UTIL_ERROR_INVALID_OPERATION;
		}
	}

	if (EGifCloseFile(GifFile) == GIF_ERROR) {
		mm_util_error("could not close file");
		outputbuffer_free(OutputBuffer, encoded->image_count);
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	encoded->size = write_data_ptr.size;
	outputbuffer_free(OutputBuffer, encoded->image_count);

	return MM_UTIL_ERROR_NONE;
}

int mm_util_encode_gif_to_file(mm_util_gif_data * encoded, const char *fpath)
{
	int ret;

	mm_util_debug("mm_util_encode_to_gif");

	ret = write_gif(encoded, fpath, NULL);

	return ret;
}

int mm_util_encode_gif_to_memory(mm_util_gif_data * encoded, void **data)
{
	int ret;

	mm_util_debug("mm_util_encode_to_memory");
	ret = write_gif(encoded, NULL, data);

	return ret;
}

void mm_util_gif_encode_set_width(mm_util_gif_data * data, unsigned long width)
{
	data->width = width;
}

void mm_util_gif_encode_set_height(mm_util_gif_data * data, unsigned long height)
{
	data->height = height;
}

void mm_util_gif_encode_set_image_count(mm_util_gif_data * data, unsigned int image_count)
{
	data->image_count = image_count;
}

void mm_util_gif_encode_set_frame_delay_time(mm_util_gif_frame_data * frame, unsigned long long delay_time)
{
	frame->delay_time = delay_time;
}
