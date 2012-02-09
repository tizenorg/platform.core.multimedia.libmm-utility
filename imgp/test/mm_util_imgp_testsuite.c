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

#include "mm_util_imgp.h"
#include "mm_log.h"
#include "mm_debug.h"
#include <mm_ta.h>
#include <unistd.h>
#define _ROTATE_ 1
#define _RESIZE_ 0
#define _CONVERT_ 0
#define ONE_ALL 0

int main(int argc, char *argv[])
{
		unsigned char *src = NULL;
		unsigned char *dst = NULL;
		unsigned int src_width = 0;
		unsigned int src_height = 0;
		unsigned int dst_width = 0;
		unsigned int dst_height = 0;
		char output_file[25];
		unsigned int src_size = 0;
		unsigned int dst_size = 0;
		if (argc < 5) {
			mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] Usage: ./mm_image_testsuite filename [yuv420 | yuv420p | yuv422 | uyvy | vyuy | nv12 | nv12t | rgb565 | rgb888 | argb | jpeg] width height\n", __func__, __LINE__);
			exit (0);
		}

		int src_cs =atoi(argv[4]);
		#if _CONVERT_
		int dst_cs = atoi(argv[5]);
		#endif
		#if _ROTATE_
		mm_util_img_rotate_type angle = MM_UTIL_ROTATE_270;
		#endif
		int ret = 0;
		int cnt = 0;

		MMTA_INIT();
		#if ONE_ALL
		while(cnt++ < 10000) {
		#endif
		FILE *fp = fopen(argv[1], "r");

		src_width = atoi(argv[2]);
		src_height = atoi(argv[3]);
		
		mm_util_get_image_size(src_cs, src_width, src_height, &src_size);
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] convert src buffer size=%d\n", __func__, __LINE__, src_size);
		src = malloc(src_size);
		fread(src, 1, src_size, fp);

#if _ROTATE_
		//rotate 90
		dst_width = src_width ;
		dst_height = src_width;
		mm_util_get_image_size(src_cs, dst_width, dst_height, &dst_size);
		dst = malloc(dst_size);
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] dst: %p", __func__, __LINE__, dst);

		__ta__("mm_util_rotate_image",
		ret = mm_util_rotate_image(src, src_width, src_height, src_cs, dst, &dst_width, &dst_height, angle);
		);
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] mm_util_rotate_image dst: %p ret = %d\n", __func__, __LINE__, dst, ret);
		//free(src);
		#if _RESIZE_ || _CONVERT_
		src = dst;
		#endif
#endif
#if _RESIZE_
		//scale 1/2

		#if _ROTATE_
		src_width = dst_width; 
		src_height = dst_height;
		#endif
		dst_width = src_width/2;
		dst_height = src_height/2;
		mm_util_get_image_size(src_cs, dst_width, dst_height, &dst_size);
		dst = malloc(dst_size);
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] dst: %p", __func__, __LINE__, dst);

		__ta__("mm_util_resize_image",
		ret = mm_util_resize_image(src, src_width, src_height, src_cs, dst, &dst_width, &dst_height);
		);
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] mm_util_resize_image dst: %p ret = %d", __func__, __LINE__, dst, ret);
		//free(src);
		#if _CONVERT_ && !_ROTATE_
		src = dst;
		#endif

#endif
#if _CONVERT_
		#if !(_ROTATE_ && _RESIZE_)
		dst_width = src_width;
		dst_height = src_height;
		#endif
		mm_util_get_image_size(dst_cs, dst_width, dst_height, &dst_size);
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] convert dst buffer size=%d\n", __func__, __LINE__, dst_size);
		dst = malloc(dst_size);
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] dst: %p", __func__, __LINE__, dst);

		__ta__("mm_util_convert_colorspace",
		ret = mm_util_convert_colorspace(src, src_width, src_height, src_cs, dst, dst_cs);
		);
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] mm_util_convert_colorspace dst:%p ret = %d",__func__, __LINE__, dst, ret);
		MMTA_ACUM_ITEM_END("colorspace", 0);
		MMTA_ACUM_ITEM_SHOW_RESULT();
		MMTA_RELEASE ();
#endif		
		if(ret==0) {
			if(cnt == 0) {
				sprintf(output_file, "result%d.rgb", cnt);
				FILE *fpout = fopen(output_file, "w");
				mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] %s = %dx%d dst_size: %d", __func__, __LINE__, output_file, src_width, src_height, dst_size);
				mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] mm_util_convert_colorspace dst:%p ret = %d",__func__, __LINE__, dst, ret);
				fwrite(dst, 1, dst_size, fpout);
				fflush(fpout);
				if(fpout) fclose(fpout);
			}
		}
		if(fp) {
			fclose(fp);
		}
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] src: %p",__func__, __LINE__, src);
		if(src) {
			free(src); src = NULL;
		}
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] dst: %p",__func__, __LINE__,dst);
		if(dst) {
			free(dst); dst = NULL;
		}
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Success -  free src & dst",__func__, __LINE__);

		#if ONE_ALL
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] cnt: %d",__func__, __LINE__, cnt);
		}
		#endif

		return ret;
}

