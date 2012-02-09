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
#include <mm_util_jpeg.h>
#include <mm_ta.h>
#include "mm_log.h"
#include <mm_debug.h>
#define _FILE 1
#define _All 1

int main(int argc, char *argv[])
{
	mm_util_jpeg_yuv_data decoded_data = {0,};
	mm_util_jpeg_yuv_format fmt = MM_UTIL_JPEG_FMT_YUV420;
	int ret = 0;

	//g_type_init();  //when you include WINK

	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d]TEST decode jpeg\n", __func__, __LINE__);
	MMTA_INIT();

	#if _FILE
	__ta__("mm_util_decode_from_jpeg_file",
	ret = mm_util_decode_from_jpeg_file(&decoded_data, argv[1], fmt);
	);
	#else
	FILE* Stream = NULL;
	void* src = NULL;
	int file_size = 0;
	Stream = fopen(argv[1], "r");
	fseek (Stream, 0, SEEK_END);
	file_size = ftell(Stream);
	rewind(Stream);
	src = malloc(file_size);
	fread(src, 1, file_size, Stream);
	fclose(Stream);

	__ta__("mm_util_decode_from_jpeg_memory",
	ret = mm_util_decode_from_jpeg_memory(&decoded_data, src, file_size, fmt);
	);
	free(src);
	#endif

	#if 0
	MMTA_ACUM_ITEM_SHOW_RESULT();
	MMTA_RELEASE ();
	#endif
	if (ret < 0) {
		mmf_debug(MMF_DEBUG_LOG,"[%s]%05d[] error: mm_util_decode_from_jpeg_file\n", __func__, __LINE__);
		exit(0);
	}else {
		mmf_debug(MMF_DEBUG_LOG, "Sucess~!! ret %d\n", ret);
		if(decoded_data.data != NULL) {
			mmf_debug(MMF_DEBUG_LOG,"[%s][%05d]##decoded_data.data##: %p\t width: %d\t height:%d\t size: %d\n", __func__, __LINE__ ,
				decoded_data.data, decoded_data.width, decoded_data.height, decoded_data.size);
		}else {
			mmf_debug(MMF_DEBUG_LOG,"decoded_data.data is NULL\n");
		}
	}

#if _All
	char tmp[1024];
	void *mem = NULL;
	FILE* fp_dest = NULL;
	int size = 0;
	strncpy(tmp,"yhkim_jpeg.jpg", 15);
	fp_dest =fopen(tmp, "w+");
	if(fp_dest == NULL) {
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] file open error~!!", __func__, __LINE__);
	}
	__ta__("mm_util_jpeg_encode_to_file",
	ret = mm_util_jpeg_encode_to_memory(/*tmp,*/ &mem, &size, decoded_data.data, decoded_data.width, decoded_data.height, decoded_data.format, 100);
	);
	MMTA_ACUM_ITEM_SHOW_RESULT();
	MMTA_RELEASE ();
	if (ret < 0) {
		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] error: mm_util_jpeg_encode_to_file\n", __func__, __LINE__);
		exit(0);
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] mem : %p\t size: %d\n" , __func__, __LINE__, mem, size);
	fwrite(mem, 1, size, fp_dest);

	if(decoded_data.data) {
		free(decoded_data.data);
		decoded_data.data = NULL;
	}
	fclose(fp_dest);
#endif
}
