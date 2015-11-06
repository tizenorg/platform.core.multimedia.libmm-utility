/*
 * libmm-utility
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: YoungHun Kim <yh8004.kim@samsung.com>
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

#include <glib.h>
#include "mm_util_imgcv.h"

#ifdef __cplusplus
	extern "C" {
#endif

#include <gmodule.h>
#include <mm_debug.h>
#include <mm_types.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <opencv/cv.h>
#include <opencv2/imgproc/imgproc_c.h>

typedef struct _mm_util_imgcv_s
{
	IplImage *inImg;

	int hBins; /**< Number of bins of Hue channle */
	int sBins; /**< Number of bins of Saturation channle */
	int vBins; /**< Number of bins of Value channle */

	float hRanges[2]; /**< Range of Hues */
	float sRanges[2]; /**< Range of Saturation */
	float vRanges[2]; /**< Range of Value */

	int sizeOfHist[3]; /**< array of bins; hBins,sBins,vBins */

	int width;
	int height;

} mm_util_imgcv_s;

#ifdef __cplusplus
}
#endif
