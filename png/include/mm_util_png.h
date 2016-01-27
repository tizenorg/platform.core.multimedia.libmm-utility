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

#ifndef __MM_UTIL_PNG_H__
#define __MM_UTIL_PNG_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "mm_util_imgp.h"
#include "png.h"

/**
    @addtogroup UTILITY
    @{

    @par
    This part describes the APIs with repect to multimedia image library.
*/

/* These describe the color_type field in png_info. */
/* color types.  Note that not all combinations are legal */
typedef enum {
	MM_UTIL_PNG_COLOR_TYPE_GRAY       = PNG_COLOR_TYPE_GRAY,
	MM_UTIL_PNG_COLOR_TYPE_PALETTE    = PNG_COLOR_TYPE_PALETTE,
	MM_UTIL_PNG_COLOR_TYPE_RGB        = PNG_COLOR_TYPE_RGB,
	MM_UTIL_PNG_COLOR_TYPE_RGB_ALPHA  = PNG_COLOR_TYPE_RGB_ALPHA,
	MM_UTIL_PNG_COLOR_TYPE_GRAY_ALPHA = PNG_COLOR_TYPE_GRAY_ALPHA,
/* aliases */
	MM_UTIL_PNG_COLOR_TYPE_RGBA       = PNG_COLOR_TYPE_RGBA,
	MM_UTIL_PNG_COLOR_TYPE_GA         = PNG_COLOR_TYPE_GA
} mm_util_png_color_type;

/* This is for filter type.*/
typedef enum {
	MM_UTIL_PNG_FILTER_TYPE_BASE        = PNG_FILTER_TYPE_BASE,		/* Single row per-byte filtering. Value 0 */
	MM_UTIL_PNG_INTRAPIXEL_DIFFERENCING = PNG_INTRAPIXEL_DIFFERENCING,	/* Used only in MNG datastreams. Value 64 */
	MM_UTIL_PNG_FILTER_TYPE_DEFAULT     = PNG_FILTER_TYPE_DEFAULT		/* Value 0 */
} mm_util_png_filter_type;

/* These are for the interlacing type.  These values should NOT be changed. */
typedef enum {
	MM_UTIL_PNG_INTERLACE_NONE  = PNG_INTERLACE_NONE,	/* Non-interlaced image. Value 0 */
	MM_UTIL_PNG_INTERLACE_ADAM7 = PNG_INTERLACE_ADAM7,	/* Adam7 interlacing. Value 1 */
} mm_util_png_interlace_type;

/* Transform masks for the high-level interface */
typedef enum {
	MM_UTIL_PNG_TRANSFORM_IDENTITY            = PNG_TRANSFORM_IDENTITY,		/* read and write */
	MM_UTIL_PNG_TRANSFORM_STRIP_16            = PNG_TRANSFORM_STRIP_16,		/* read only */
	MM_UTIL_PNG_TRANSFORM_STRIP_ALPHA         = PNG_TRANSFORM_STRIP_ALPHA,		/* read only */
	MM_UTIL_PNG_TRANSFORM_PACKING             = PNG_TRANSFORM_PACKING,		/* read and write */
	MM_UTIL_PNG_TRANSFORM_PACKSWAP            = PNG_TRANSFORM_PACKSWAP,		/* read and write */
	MM_UTIL_PNG_TRANSFORM_EXPAND              = PNG_TRANSFORM_EXPAND,		/* read only */
	MM_UTIL_PNG_TRANSFORM_INVERT_MONO         = PNG_TRANSFORM_INVERT_MONO,		/* read and write */
	MM_UTIL_PNG_TRANSFORM_SHIFT               = PNG_TRANSFORM_SHIFT,		/* read and write */
	MM_UTIL_PNG_TRANSFORM_BGR                 = PNG_TRANSFORM_BGR,			/* read and write */
	MM_UTIL_PNG_TRANSFORM_SWAP_ALPHA          = PNG_TRANSFORM_SWAP_ALPHA,		/* read and write */
	MM_UTIL_PNG_TRANSFORM_SWAP_ENDIAN         = PNG_TRANSFORM_SWAP_ENDIAN,		/* read and write */
	MM_UTIL_PNG_TRANSFORM_INVERT_ALPHA        = PNG_TRANSFORM_INVERT_ALPHA,		/* read and write */
	MM_UTIL_PNG_TRANSFORM_STRIP_FILLER        = PNG_TRANSFORM_STRIP_FILLER,		/* write only */
	MM_UTIL_PNG_TRANSFORM_STRIP_FILLER_BEFORE = PNG_TRANSFORM_STRIP_FILLER_BEFORE,	/* write only */
	MM_UTIL_PNG_TRANSFORM_STRIP_FILLER_AFTER  = PNG_TRANSFORM_STRIP_FILLER_AFTER,	/* write only */
	MM_UTIL_PNG_TRANSFORM_GRAY_TO_RGB         = PNG_TRANSFORM_GRAY_TO_RGB,		/* read only */
#if 0
/* Added to libpng-1.5.4 */
	MM_UTIL_PNG_TRANSFORM_EXPAND_16           = PNG_TRANSFORM_EXPAND_16,		/* read only */
	MM_UTIL_PNG_TRANSFORM_SCALE_16            = PNG_TRANSFORM_SCALE_16		/* read only */
#endif
} mm_util_png_transform_type;

/* Flags for png_set_filter() to say which filters to use.  The flags
 * are chosen so that they don't conflict with real filter types
 * below, in case they are supplied instead of the #defined constants.
 * These values should NOT be changed.
 */
typedef enum {
	MM_UTIL_PNG_NO_FILTERS   = PNG_NO_FILTERS,	/*0x00 */
	MM_UTIL_PNG_FILTER_NONE  = PNG_FILTER_NONE,	/*0x08 */
	MM_UTIL_PNG_FILTER_SUB   = PNG_FILTER_SUB,	/*0x10 */
	MM_UTIL_PNG_FILTER_UP    = PNG_FILTER_UP,	/*0x20 */
	MM_UTIL_PNG_FILTER_AVG   = PNG_FILTER_AVG,	/*0x40 */
	MM_UTIL_PNG_FILTER_PAETH = PNG_FILTER_PAETH,	/*0x80 */
	MM_UTIL_PNG_ALL_FILTERS  = PNG_ALL_FILTERS	/*PNG_FILTER_NONE | PNG_FILTER_SUB | PNG_FILTER_UP | \
												   PNG_FILTER_AVG | PNG_FILTER_PAETH */
} mm_util_png_filter;

typedef enum {
	MM_UTIL_COMPRESSION_0 = 0,	/* No compression */
	MM_UTIL_COMPRESSION_1 = 1,	/* Best speed */
	MM_UTIL_COMPRESSION_2 = 2,
	MM_UTIL_COMPRESSION_3 = 3,
	MM_UTIL_COMPRESSION_4 = 4,
	MM_UTIL_COMPRESSION_5 = 5,
	MM_UTIL_COMPRESSION_6 = 6,	/* Default compression */
	MM_UTIL_COMPRESSION_7 = 7,
	MM_UTIL_COMPRESSION_8 = 8,
	MM_UTIL_COMPRESSION_9 = 9	/* Best compression */
} mm_util_png_compression;

typedef enum {
	MM_UTIL_BIT_DEPTH_1  = 1,
	MM_UTIL_BIT_DEPTH_2  = 2,
	MM_UTIL_BIT_DEPTH_4  = 4,
	MM_UTIL_BIT_DEPTH_8  = 8,
	MM_UTIL_BIT_DEPTH_16 = 16
} mm_util_png_bit_depth;

typedef struct {
	int bit_depth;				/**< Bit depth*/
	int color_type;				/**< Color type*/
	int interlace_type;			/**< Interlace type */
	int filter_type;			/**< Filter type*/
	png_size_t rowbytes;			/**< Number or bytes in each row*/

	int progressive;			/**< Is progressive decoding */
	unsigned int progressive_bytes;		/**< Number of bytes to process at a time for progressive decoding*/
	int transform;				/**< Transform masks for decoding */
	int compression_level;			/**< Compression level of encoding */
	int filter;				/**< Filter */
} mm_util_png;

typedef struct {
	mm_util_png png;
	png_uint_32 width;			/**< Width*/
	png_uint_32 height;			/**< Height*/
	png_uint_32 size;			/**< size */
	void *data;				/**< data */
} mm_util_png_data;

#if 0
/**
 * This function checks if the given file is a png file.
 *
 * @param filename [in]     file name
 * @return                  This function returns 1 if file is png, or 0 if not a png.
 * @remark
 * @see
 * @since                   R1, 1.0
 */
bool mm_util_check_if_png(const char *fpath);
#endif
/**
 * This function extracts raw data from png file
 *
 * @param decoded  [out]    pointer of mm_util_png_data.
 *                          After using it, please free the allocated memory.
 * @param filename [in]     input file name, encoded stream file
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark
 * @see                     mm_util_png_data, mm_util_init_decode_png
 * @since                   R1, 1.0
 */
int mm_util_decode_from_png_file(mm_util_png_data * decoded, const char *fpath);

/**
 * This function extracts raw data from png buffer
 *
 * @param decoded  [out]    pointer of mm_util_png_data.
 *                          After using it, please free the allocated memory.
 * @param filename [in]     input stream pointer(pointer of encoded png stream data)
 * @param memory   [in]     size of input stream(size of pointer of encoded png stream data)
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark
 * @see                     mm_util_png_data, mm_util_init_decode_png
 * @since                   R1, 1.0
 */
int mm_util_decode_from_png_memory(mm_util_png_data * decoded, void **memory, unsigned long long src_size);

/**
 * This function initializes the values of mm_util_png.
 * This function should be called before calling the actual decoding.
 *
 * @param data  [in]     pointer of mm_util_png_data.
 * @return               None.
 * @remark
 * @see                  mm_util_png, mm_util_png_data, mm_util_decode_from_png_file, mm_util_decode_from_png_memory
 * @since                R1, 1.0
 */
void mm_util_init_decode_png(mm_util_png_data * data);

#if 0
/**
 * This function sets if the decoding should be done progressively.
 * This function should be called after mm_util_init_decode_png.
 *
 * @param data         [in]     pointer of mm_util_png_data.
 * @param progressive  [in]     If decoding should be done progressively.
 * @return                      None.
 * @remark
 * @see                         mm_util_png, mm_util_png_data, mm_util_init_decode_png
 * @since                       R1, 1.0
 */
void mm_util_png_decode_set_progressive(mm_util_png_data * data, int progressive);

/**
 * This function sets the amount of bytes to process at a time for progressive decoding.
 * This function should be called only if mm_util_decode_set_progressive is set.
 *
 * @param data               [in]     pointer of mm_util_png_data.
 * @param progressive_bytes  [in]     Number of bytes to process at a time.
 * @return                            None.
 * @remark
 * @see                               mm_util_png, mm_util_png_data, mm_util_decode_set_progressive
 * @since                             R1, 1.0
 */
void mm_util_png_decode_set_progressive_bytes(mm_util_png_data * data, int progressive_bytes);

/**
 * This function sets the transform mask for decoding using high level interface.
 * This should be set for non progressive decoding.
 *
 * @param data       [in]     pointer of mm_util_png_data.
 * @param transform  [in]     Transform mask for decoding.
 * @return                    None.
 * @remark
 * @see                       mm_util_png, mm_util_png_data, mm_util_png_transform_type
 * @since                     R1, 1.0
 */
void mm_util_png_decode_set_transform(mm_util_png_data * data, int transform);

/**
 * This function gets the width of the decoded image.
 * This should be called after the actual decoding.
 *
 * @param data       [in]     pointer of mm_util_png_data.
 * @return                    Width of the decoded image.
 * @remark
 * @see                       mm_util_png_data
 * @since                     R1, 1.0
 */
png_uint_32 mm_util_png_decode_get_width(mm_util_png_data * data);

/**
 * This function gets the height of the decoded image.
 * This should be called after the actual decoding.
 *
 * @param data       [in]     pointer of mm_util_png_data.
 * @return                    Height of the decoded image.
 * @remark
 * @see                       mm_util_png_data
 * @since                     R1, 1.0
 */
png_uint_32 mm_util_png_decode_get_height(mm_util_png_data * data);

/**
 * This function gets the bit depth of the decoded image.
 * This should be called after the actual decoding.
 *
 * @param data       [in]     pointer of mm_util_png_data.
 * @return                    Bit depth of the decoded image.
 * @remark
 * @see                       mm_util_png, mm_util_png_data
 * @since                     R1, 1.0
 */
int mm_util_png_decode_get_bit_depth(mm_util_png_data * data);

/**
 * This function gets the size of the decoded image.
 * This should be called after the actual decoding.
 *
 * @param data       [in]     pointer of mm_util_png_data.
 * @return                    Size of the decoded image.
 * @remark
 * @see                       mm_util_png, mm_util_png_data
 * @since                     R1, 1.0
 */
png_uint_32 mm_util_png_decode_get_size(mm_util_png_data * data);
#endif

/**
 * This function extracts raw data to png file
 *
 * @param data     [in]     address of the raw data.
 *                          After using it, please free the allocated memory.
 * @param encoded  [out]    pointer of mm_util_png_data.
 *                          After using it, please free the allocated memory.
 * @param filename [out]    output file name on to which the encoded stream will be written.
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark
 * @see                     mm_util_png_data, mm_util_init_encode_png
 * @since                   R1, 1.0
 */
int mm_util_encode_to_png_file(void **data, mm_util_png_data * encoded, const char *fpath);

/**
 * This function extracts raw data to memory
 *
 * @param data     [in]     address of the raw data.
 *                          After using it, please free the allocated memory.
 * @param encoded  [out]    pointer of mm_util_png_data.
 *                          After using it, please free the allocated memory.
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark
 * @see                     mm_util_png_data, mm_util_init_encode_png
 * @since                   R1, 1.0
 */
int mm_util_encode_to_png_memory(void **data, mm_util_png_data * encoded);

/**
 * This function initializes the values of mm_util_png.
 * This function should be called before calling the actual encoding.
 *
 * @param data  [in]     pointer of mm_util_png_data.
 * @return               None.
 * @remark
 * @see                  mm_util_png, mm_util_png_data, mm_util_encode_to_png_file, mm_util_encode_to_png_memory
 * @since                R1, 1.0
 */
void mm_util_init_encode_png(mm_util_png_data * data);

/**
 * This function sets compression level of encoding.
 * This function should be called after mm_util_init_encode_png.
 *
 * @param data               [in]     pointer of mm_util_png_data.
 * @param compression_level  [in]     Level of compression while encoding.
 * @return                            None.
 * @remark                            Check mm_util_png_compression for valid values
 * @see                               mm_util_png, mm_util_png_data, mm_util_init_encode_png
 * @since                             R1, 1.0
 */
void mm_util_png_encode_set_compression_level(mm_util_png_data * data, mm_util_png_compression compression_level);

/**
 * This function sets filter and filter value of encoding.
 * This function should be called after mm_util_init_encode_png.
 *
 * @param data               [in]     pointer of mm_util_png_data.
 * @param filter             [in]     Specify the filters to use.
 * @return                            None.
 * @remark                            Check mm_util_png_filter for valid values
 * @see                               mm_util_png, mm_util_png_data, mm_util_init_encode_png
 * @since                             R1, 1.0
 */
void mm_util_png_encode_set_filter(mm_util_png_data * data, mm_util_png_filter filter);

/**
 * This function sets color type of the encoded image.
 * This function should be called after mm_util_init_encode_png.
 *
 * @param data               [in]     pointer of mm_util_png_data.
 * @param color_type         [in]     color type during encoding the image.
 * @return                            None.
 * @remark                            Check mm_util_png_color_type for valid values
 * @see                               mm_util_png, mm_util_png_data, mm_util_init_encode_png
 * @since                             R1, 1.0
 */
void mm_util_png_encode_set_color_type(mm_util_png_data * data, mm_util_png_color_type color_type);

/**
 * This function sets filter type of the encoded image.
 * This function should be called after mm_util_init_encode_png.
 *
 * @param data               [in]     pointer of mm_util_png_data.
 * @param filter_type        [in]     filter type during encoding the image.
 * @return                            None.
 * @remark                            Check mm_util_png_filter_type for valid values
 * @see                               mm_util_png, mm_util_png_data, mm_util_init_encode_png
 * @since                             R1, 1.0
 */
void mm_util_png_encode_set_filter_type(mm_util_png_data * data, mm_util_png_filter_type filter_type);

/**
 * This function sets interlace type of the encoded image.
 * This function should be called after mm_util_init_encode_png.
 *
 * @param data               [in]     pointer of mm_util_png_data.
 * @param interlace_type     [in]     interlace type during encoding the image.
 * @return                            None.
 * @remark                            Check mm_util_png_interlace_type for valid values
 * @see                               mm_util_png, mm_util_png_data, mm_util_init_encode_png
 * @since                             R1, 1.0
 */
void mm_util_png_encode_set_interlace_type(mm_util_png_data * data, mm_util_png_interlace_type interlace_type);

/**
 * This function sets width of the encoded image.
 * This function should be called after mm_util_init_encode_png.
 *
 * @param data               [in]     pointer of mm_util_png_data.
 * @param width              [in]     width of the encoded image.
 * @return                            None.
 * @remark
 * @see                               mm_util_png_data, mm_util_init_encode_png
 * @since                             R1, 1.0
 */
void mm_util_png_encode_set_width(mm_util_png_data * data, png_uint_32 width);

/**
 * This function sets height of the encoded image.
 * This function should be called after mm_util_init_encode_png.
 *
 * @param data               [in]     pointer of mm_util_png_data.
 * @param height             [in]     height of the encoded image.
 * @return                            None.
 * @remark
 * @see                               mm_util_png_data, mm_util_init_encode_png
 * @since                             R1, 1.0
 */
void mm_util_png_encode_set_height(mm_util_png_data * data, png_uint_32 height);

/**
 * This function sets bit depth of the encoded image.
 * This function should be called after mm_util_init_encode_png.
 *
 * @param data               [in]     pointer of mm_util_png_data.
 * @param bit_depth          [in]     bit depth of the encoded image.
 * @return                            None.
 * @remark                            Check mm_util_png_bit_depth for valid values
 * @see                               mm_util_png, mm_util_png_data, mm_util_init_encode_png
 * @since                             R1, 1.0
 */
void mm_util_png_encode_set_bit_depth(mm_util_png_data * data, mm_util_png_bit_depth bit_depth);

#ifdef __cplusplus
}
#endif
#endif	  /*__MM_UTIL_PNG_H__*/
