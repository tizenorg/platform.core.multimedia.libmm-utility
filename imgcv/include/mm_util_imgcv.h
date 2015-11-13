/*
 * libmm-utility
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd., Ltd. All rights reserved.
 * Contact: Tae-Young Chung <yh8004.kim@samsung.com>
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

#ifndef __MM_UTILITY_IMGCV_H__
#define __MM_UTILITY_IMGCV_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
    @addtogroup UTILITY
    @{

    @par
    This part describes the APIs with repect to image color processing library.
 */

int mm_util_cv_extract_representative_color(void *image_buffer, int width, int height, unsigned char *r_color, unsigned char *g_color, unsigned char *b_color);

#ifdef __cplusplus
}
#endif

#endif	/*__MM_UTILITY_IMGP_H__*/
