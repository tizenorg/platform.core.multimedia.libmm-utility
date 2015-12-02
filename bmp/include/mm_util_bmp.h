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

#ifndef __MM_UTIL_BMP_H__
#define __MM_UTIL_BMP_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "mm_util_imgp.h"
#include "libnsbmp.h"
#include "bmpfile.h"

/**
    @addtogroup UTILITY
    @{

    @par
    This part describes the APIs with repect to multimedia image library.
*/

/**
 * format for bmp
 */
typedef enum
{
	MM_UTIL_BMP_FMT_RGBA8888,     /**< RGBA8888 format */
} mm_util_bmp_format;

typedef struct {
	unsigned long width;		/**< width */
	unsigned long height;		/**< height */
	unsigned long long size;	/**< size */
	void *data;			/**< data */
} mm_util_bmp_data;

/**
 * This function extracts raw data from bmp file
 *
 * @param decoded  [out]    pointer of mm_util_bmp_data.
 *                          After using it, please free the allocated memory.
 * @param filename [in]     input file name, encoded stream file
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark
 * @see                     mm_util_bmp_data
 * @since                   R1, 1.0
 */
int mm_util_decode_from_bmp_file(mm_util_bmp_data * decoded, const char *filename);

/**
 * This function extracts raw data from bmp memory
 *
 * @param decoded  [out]    pointer of mm_util_bmp_data.
 *                          After using it, please free the allocated memory.
 * @param memory [in]       input memory, encoded stream file
 * @param src_size [in]     input src size, encoded stream file
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark
 * @see                     mm_util_bmp_data
 * @since                   R1, 1.0
 */
int mm_util_decode_from_bmp_memory(mm_util_bmp_data * decoded, void **memory, unsigned long long src_size);

/**
 * This function gets the width of the decoded image.
 * This should be called after the actual decoding.
 *
 * @param data       [in]     pointer of mm_util_bmp_data.
 * @return                    Width of the decoded image.
 * @remark
 * @see                       mm_util_bmp_data
 * @since                     R1, 1.0
 */
unsigned long mm_util_bmp_decode_get_width(mm_util_bmp_data * data);

/**
 * This function gets the height of the decoded image.
 * This should be called after the actual decoding.
 *
 * @param data       [in]     pointer of mm_util_bmp_data.
 * @return                    Height of the decoded image.
 * @remark
 * @see                       mm_util_bmp_data
 * @since                     R1, 1.0
 */
unsigned long mm_util_bmp_decode_get_height(mm_util_bmp_data * data);

/**
 * This function gets the size of the decoded image.
 * This should be called after the actual decoding.
 *
 * @param data       [in]     pointer of mm_util_bmp_data.
 * @return                    Size of the decoded image.
 * @remark
 * @see                       mm_util_bmp_data
 * @since                     R1, 1.0
 */
unsigned long long mm_util_bmp_decode_get_size(mm_util_bmp_data * data);

/**
 * This function encodes raw data to bmp file
 *
 * @param encoded  [in ]    pointer of mm_util_bmp_data.
 *                          After using it, please free the allocated memory.
 * @param filename [out]    output file name on to which the encoded stream will be written.
 * @return                  This function returns zero on success, or negative value with error code.
 * @remark
 * @see                     mm_util_bmp_data
 * @since                   R1, 1.0
 */
int mm_util_encode_bmp_to_file(mm_util_bmp_data * encoded, const char *filename);

/**
 * This function sets width of the encoded image.
 *
 * @param data               [in]     pointer of mm_util_bmp_data.
 * @param width              [in]     width of the encoded image.
 * @return                            None.
 * @remark
 * @see                               mm_util_bmp_data
 * @since                             R1, 1.0
 */
void mm_util_bmp_encode_set_width(mm_util_bmp_data * data, unsigned long width);

/**
 * This function sets height of the encoded image.
 *
 * @param data               [in]     pointer of mm_util_bmp_data.
 * @param height             [in]     height of the encoded image.
 * @return                            None.
 * @remark
 * @see                               mm_util_bmp_data
 * @since                             R1, 1.0
 */
void mm_util_bmp_encode_set_height(mm_util_bmp_data * data, unsigned long height);

#ifdef __cplusplus
}
#endif
#endif	  /*__MM_UTIL_BMP_H__*/
