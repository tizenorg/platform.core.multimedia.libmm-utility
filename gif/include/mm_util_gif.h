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

#ifndef __MM_UTIL_GIF_H__
#define __MM_UTIL_GIF_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "mm_util_imgp.h"
#include "gif_lib.h"

/**
    @addtogroup UTILITY
    @{

    @par
    This part describes the APIs with repect to multimedia image library.
*/

/**
 * Read data attached to decoding callback
 */
typedef struct {
	unsigned long long size;
	void *mem;
} read_data;

/**
 * Write data attached to encoding callback
 */
typedef struct {
	unsigned long long size;
	void **mem;
} write_data;

/**
 * format for gif
 */
typedef enum
{
	MM_UTIL_GIF_FMT_RGBA8888,     /**< RGBA8888 format */
} mm_util_gif_format;

/**
 * Disposal mode for gif encoding
 */
typedef enum
{
	MM_UTIL_GIF_DISPOSAL_UNSPECIFIED = 0,     /**< No disposal specified. */
	MM_UTIL_GIF_DISPOSAL_DO_NOT      = 1,     /**< Leave image in place. */
	MM_UTIL_GIF_DISPOSAL_BACKGROUND  = 2,     /**< Set area too background color. */
	MM_UTIL_GIF_DISPOSAL_PREVIOUS    = 3,     /**< Restore to previous content. */
} mm_util_gif_disposal;

typedef struct {
	unsigned long long delay_time;		/**< Pre-display delay in 0.01sec units */
	unsigned long width;	  		/**< Width */
	unsigned long height;	  		/**< Height */
	unsigned long x;	  		/**< X position */
	unsigned long y;	  		/**< Y position */
	bool is_transparent;                    /**< Is frame transparency set */
	GifColorType transparent_color; 	/**< Transparency color */
	mm_util_gif_disposal disposal_mode;	/**< Disposal mode */
	void *data;		   		/**< Data */
} mm_util_gif_frame_data;

typedef struct {
	unsigned long width;			  /**< Width */
	unsigned long height;			  /**< Height */
	unsigned long long size;		  /**< Size */
	unsigned int image_count;		  /**< ImageCount */
	bool screen_desc_updated;		  /**< Check if screen description is updated */
	unsigned int current_count;		  /**< ImageCount */
	write_data write_data_ptr;		  /**< Encoded output data attached to callback */
	GifFileType *GifFile;			  /**< GifFile opened */
	mm_util_gif_frame_data **frames;	  /**< Frames */
} mm_util_gif_data;

/**
 * This function extracts raw data from gif file
 *
 * @param decoded  [out]    pointer of mm_util_gif_data.
 *                          After using it, please free the allocated memory.
 * @param filename [in]     input file name, encoded stream file
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark
 * @see                     mm_util_gif_data
 * @since                   R1, 1.0
 */
int mm_util_decode_from_gif_file(mm_util_gif_data * decoded, const char *filename);

/**
 * This function extracts raw data from gif memory
 *
 * @param decoded  [out]    pointer of mm_util_gif_data.
 *                          After using it, please free the allocated memory.
 * @param filename [in]     input file memory, encoded stream file
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark
 * @see                     mm_util_gif_data
 * @since                   R1, 1.0
 */
int mm_util_decode_from_gif_memory(mm_util_gif_data * decoded, void **memory);

#if 0
/**
 * This function gets the width of the decoded image.
 * This should be called after the actual decoding.
 *
 * @param data       [in]     pointer of mm_util_gif_data.
 * @return                    Width of the decoded image.
 * @remark
 * @see                       mm_util_gif_data
 * @since                     R1, 1.0
 */
unsigned long mm_util_gif_decode_get_width(mm_util_gif_data * data);

/**
 * This function gets the height of the decoded image.
 * This should be called after the actual decoding.
 *
 * @param data       [in]     pointer of mm_util_gif_data.
 * @return                    Height of the decoded image.
 * @remark
 * @see                       mm_util_gif_data
 * @since                     R1, 1.0
 */
unsigned long mm_util_gif_decode_get_height(mm_util_gif_data * data);

/**
 * This function gets the size of the decoded image.
 * This should be called after the actual decoding.
 *
 * @param data       [in]     pointer of mm_util_gif_data.
 * @return                    Size of the decoded image.
 * @remark
 * @see                       mm_util_gif_data
 * @since                     R1, 1.0
 */
unsigned long long mm_util_gif_decode_get_size(mm_util_gif_data * data);
#endif

/**
 * This function creates gif file for output file
 *
 * @param encoded  [in ]    pointer of mm_util_gif_data.
 * @param filename [out]    output file name on to which the encoded stream will be written.
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark                  image_count should be set to a valid value depending on number of images
 *                          to be encoded in the gif file.
 * @see                     mm_util_gif_data
 * @since                   R1, 1.0
 */
int mm_util_encode_open_gif_file(mm_util_gif_data *encoded, const char *filename);

/**
 * This function creates gif file for output memory
 *
 * @param encoded  [in ]    pointer of mm_util_gif_data.
 * @param filename [out]    output file memory on to which the encoded stream will be written.
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark                  image_count should be set to a valid value depending on number of images
 *                          to be encoded in the gif file.
 * @see                     mm_util_gif_data
 * @since                   R1, 1.0
 */
int mm_util_encode_open_gif_memory(mm_util_gif_data *encoded, void **data);

/**
 * This function encodes raw data to gif file/memory
 *
 * @param encoded  [in ]    pointer of mm_util_gif_data.
 * @param filename [out]    output file name on to which the encoded stream will be written.
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark                  image_count should be set to a valid value depending on number of images
 *                          to be encoded in the gif file.
 * @see                     mm_util_gif_data
 * @since                   R1, 1.0
 */
int mm_util_encode_gif(mm_util_gif_data *encoded);

/**
 * This function closes and deallocates all the memory allocated with giffile.
 *
 * @param encoded  [in ]    pointer of mm_util_gif_data.
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark                  image_count should be set to a valid value depending on number of images
 *                          to be encoded in the gif file.
 * @see                     mm_util_gif_data
 * @since                   R1, 1.0
 */
int mm_util_encode_close_gif(mm_util_gif_data *encoded);

/**
 * This function sets width of the encoded image.
 *
 * @param data               [in]     pointer of mm_util_gif_data.
 * @param width              [in]     width of the encoded image.
 * @return                            None.
 * @remark
 * @see                               mm_util_gif_data
 * @since                             R1, 1.0
 */
void mm_util_gif_encode_set_width(mm_util_gif_data * data, unsigned long width);

/**
 * This function sets height of the encoded image.
 *
 * @param data               [in]     pointer of mm_util_gif_data.
 * @param height             [in]     height of the encoded image.
 * @return                            None.
 * @remark
 * @see                               mm_util_gif_data
 * @since                             R1, 1.0
 */
void mm_util_gif_encode_set_height(mm_util_gif_data * data, unsigned long height);

/**
 * This function sets number of images of the encoded gif image.
 *
 * @param data               [in]     pointer of mm_util_gif_data.
 * @param height             [in]     Image count of the encoded image.
 * @return                            None.
 * @remark
 * @see                               mm_util_gif_data
 * @since                             R1, 1.0
 */
void mm_util_gif_encode_set_image_count(mm_util_gif_data * data, unsigned int image_count);

/**
 * This function sets the time delay after which the particular image of the gif should be displayed.
 *
 * @param data               [in]     pointer of mm_util_gif_frame_data.
 * @param delay_time         [in]     time delay of the frame in the encoded image in 0.01sec units.
 * @return                            None.
 * @remark
 * @see                               mm_util_gif_frame_data
 * @since                             R1, 1.0
 */
void mm_util_gif_encode_set_frame_delay_time(mm_util_gif_frame_data * frame, unsigned long long delay_time);

/**
 * This function sets the disposal mode of each frame in the gif as defined by mm_util_gif_disposal.
 *
 * @param data               [in]     pointer of mm_util_gif_frame_data.
 * @param disposal_mode      [in]     Disposal mode of the frame in the encoded image.
 * @return                            None.
 * @remark
 * @see                               mm_util_gif_frame_data
 * @since                             R1, 1.0
 */
void mm_util_gif_encode_set_frame_disposal_mode(mm_util_gif_frame_data *frame, mm_util_gif_disposal disposal_mode);

/**
 * This function sets the position of each frame in the gif.
 *
 * @param data               [in]     pointer of mm_util_gif_frame_data.
 * @param x                  [in]     x position of the frame in the encoded image.
 * @param y                  [in]     y position of the frame in the encoded image.
 * @return                            None.
 * @remark
 * @see                               mm_util_gif_frame_data
 * @since                             R1, 1.0
 */
void mm_util_gif_encode_set_frame_position(mm_util_gif_frame_data *frame, unsigned long x, unsigned long y);

/**
 * This function sets the transparent color for the frame in the gif.
 *
 * @param data               [in]     pointer of mm_util_gif_frame_data.
 * @param transparent_color  [in]     The color in the frame which should be transparent.
 * @return                            None.
 * @remark
 * @see                               mm_util_gif_frame_data
 * @since                             R1, 1.0
 */
void mm_util_gif_encode_set_frame_transparency_color(mm_util_gif_frame_data *frame, GifColorType transparent_color);

#ifdef __cplusplus
}
#endif
#endif	  /*__MM_UTIL_GIF_H__*/
