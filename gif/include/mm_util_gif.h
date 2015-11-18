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
#include "gif_lib.h"

/**
    @addtogroup UTILITY
    @{

    @par
    This part describes the APIs with repect to multimedia image library.
*/

typedef struct {
	int delay_time;		   /**< pre-display delay in 0.01sec units */
	void *data;		   /**< data */
} mm_util_gif_frame_data;

typedef struct {
	mm_util_gif_frame_data *frames;		  /**< Frames*/
	unsigned long width;			  /**< width */
	unsigned long height;			  /**< heigt */
	unsigned long long size;		  /**< size */
	unsigned int image_count;		  /**< ImageCount */
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

/**
 * This function encodes raw data to gif file
 *
 * @param encoded  [in ]    pointer of mm_util_gif_data.
 *                            After using it, please free the allocated memory.
 * @param filename [out]    output file name on to which the encoded stream will be written.
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark                  image_count should be set to a valid value depending on number of images
 *                          to be encoded in the gif file.
 * @see                     mm_util_gif_data
 * @since                   R1, 1.0
 */
int mm_util_encode_gif_to_file(mm_util_gif_data * encoded, const char *filename);

/**
 * This function encodes raw data to gif memory
 *
 * @param encoded  [in ]    pointer of mm_util_gif_data.
 *                          After using it, please free the allocated memory.
 * @param filename [out]    output file memory on to which the encoded stream will be written.
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark                  image_count should be set to a valid value depending on number of images
 *                          to be encoded in the gif file.
 * @see                     mm_util_gif_data
 * @since                   R1, 1.0
 */
int mm_util_encode_gif_to_memory(mm_util_gif_data * encoded, void **memory);

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

#ifdef __cplusplus
}
#endif
#endif	  /*__MM_UTIL_GIF_H__*/
