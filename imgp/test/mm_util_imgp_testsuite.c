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
#include "mm_util_imgp.h"
#include "mm_util_imgp_internal.h"
#include "mm_util_debug.h"
#include <mm_error.h>
#define ONE_ALL 0
#define IMAGE_FORMAT_LABEL_BUFFER_SIZE 4
MMHandleType MMHandle = 0;
bool completed = false;


int packet_finalize_callback(media_packet_h packet, int err, void* userdata)
{
	mm_util_debug("==> finalize callback func is called [%d] \n", err);
	return MEDIA_PACKET_FINALIZE;
}

bool
transform_completed_cb(media_packet_h *packet, int error, void *user_param)
{
	uint64_t size = 0;
	char output_file[25] = {};
	mm_util_debug("MMHandle: 0x%2x", MMHandle);

	media_format_h dst_fmt;
	media_format_mimetype_e dst_mimetype;
	int dst_width, dst_height, dst_avg_bps, dst_max_bps;
	char *output_fmt = NULL;

	if(error == MM_ERROR_NONE) {
		mm_util_debug("completed");
		output_fmt = (char*)malloc(sizeof(char) * IMAGE_FORMAT_LABEL_BUFFER_SIZE);
		if(output_fmt) {
			if(media_packet_get_format(*packet, &dst_fmt) != MM_ERROR_NONE) {
				mm_util_error("Imedia_packet_get_format");
				IMGP_FREE(output_fmt);
				return FALSE;
			}

			if(media_format_get_video_info(dst_fmt, &dst_mimetype, &dst_width, &dst_height, &dst_avg_bps, &dst_max_bps) ==MEDIA_FORMAT_ERROR_NONE) {
				memset(output_fmt, 0, IMAGE_FORMAT_LABEL_BUFFER_SIZE);
				if(dst_mimetype  ==MEDIA_FORMAT_YV12 || dst_mimetype == MEDIA_FORMAT_422P ||dst_mimetype == MEDIA_FORMAT_I420
					|| dst_mimetype == MEDIA_FORMAT_NV12 || dst_mimetype == MEDIA_FORMAT_UYVY ||dst_mimetype == MEDIA_FORMAT_YUYV) {
					strncpy(output_fmt, "yuv", strlen("yuv"));
				} else {
					strncpy(output_fmt,"rgb", strlen("rgb"));
				}
				mm_util_debug("[mimetype: %d] W x H : %d x %d", dst_mimetype, dst_width, dst_height);
				snprintf(output_file, 25, "result_%dx%d.%s", dst_width,dst_height, output_fmt);
			}
		}

		FILE *fpout = fopen(output_file, "w");
		if(fpout) {
			media_packet_get_buffer_size(*packet, &size);
			void *dst = NULL;
			if(media_packet_get_buffer_data_ptr(*packet, &dst) != MM_ERROR_NONE) {
				IMGP_FREE(dst);
				IMGP_FREE(output_fmt);
				fclose(fpout);
				mm_util_error ("[dst] media_packet_get_extra");
				return FALSE;
			}
			mm_util_debug("dst: %p [%d]", dst, size);
			fwrite(dst, 1, size, fpout);
			mm_util_debug("FREE");
			fclose(fpout);
		}

		mm_util_debug("write result");
		mm_util_debug("Free (output_fmt)");
		IMGP_FREE(output_fmt);
	} else {
		mm_util_error("[ERROR] complete cb");
	}

	completed = true;
	mm_util_debug("Destory - dst packet");
	media_packet_destroy(*packet);

	return TRUE;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	mm_util_s *handle = NULL;
	void *src;
	void *ptr;
	media_packet_h src_packet;

	if (argc < 1) {
		mm_util_error("[%s][%05d] Usage: ./mm_image_testsuite filename [yuv420 | yuv420p | yuv422 | uyvy | vyuy | nv12 | nv12t | rgb565 | rgb888 | argb | jpeg] width height\n");
		return ret;
	}

	/* Create Transform */
	ret = mm_util_create (&MMHandle);
	if(ret == MM_ERROR_NONE) {
		mm_util_debug("Success - Create Transcode Handle [MMHandle: 0x%2x]", MMHandle);
	} else {
		mm_util_debug("ERROR - Create Transcode Handle");
		return ret;
	}

	handle = (mm_util_s*) MMHandle;

	media_format_h fmt;
	if(media_format_create(&fmt) == MEDIA_FORMAT_ERROR_NONE) {
		if(media_format_set_video_mime(fmt, MEDIA_FORMAT_I420) != MEDIA_FORMAT_ERROR_NONE) {
			media_format_unref(fmt);
			mm_util_error("[Error] Set - video mime");
			return MM_ERROR_IMAGE_INVALID_VALUE;
		}

		if(media_format_set_video_width(fmt, 320) != MEDIA_FORMAT_ERROR_NONE) {
			media_format_unref(fmt);
			mm_util_error("[Error] Set - video width");
			return MM_ERROR_IMAGE_INVALID_VALUE;
		}

		if(media_format_set_video_height(fmt, 240) != MEDIA_FORMAT_ERROR_NONE) {
			media_format_unref(fmt);
			mm_util_error("[Error] Set - video height");
			return MM_ERROR_IMAGE_INVALID_VALUE;
		}

		if(media_format_set_video_avg_bps(fmt, 15000000) != MEDIA_FORMAT_ERROR_NONE) {
			media_format_unref(fmt);
			mm_util_error("[Error] Set - video avg bps");
			return MM_ERROR_IMAGE_INVALID_VALUE;
		}

		if(media_format_set_video_max_bps(fmt, 20000000) != MEDIA_FORMAT_ERROR_NONE) {
			media_format_unref(fmt);
			mm_util_error("[Error] Set - video max bps");
			return MM_ERROR_IMAGE_INVALID_VALUE;
		}

		mm_util_debug("media_format_set_video_info success! w:320, h:240, MEDIA_FORMAT_I420\n");
	}
	else {
		mm_util_error("media_format_create failed...");
	}

	ret = media_packet_create_alloc(fmt, (media_packet_finalize_cb)packet_finalize_callback, NULL, &src_packet);
	if(ret == MM_ERROR_NONE) {
		mm_util_debug("Success - Create Media Packet(%p)", src_packet);
		uint64_t size =0;
		if (media_packet_get_buffer_size(src_packet, &size) == MEDIA_PACKET_ERROR_NONE) {
			ptr = malloc(size);
			if (ptr == NULL) {
				mm_util_debug("\tmemory allocation failed\n");
				return MM_ERROR_IMAGE_INTERNAL;
			}
			if (media_packet_get_buffer_data_ptr(src_packet, &ptr) == MEDIA_PACKET_ERROR_NONE) {
				FILE *fp = fopen(argv[1], "r");
				if (fp == NULL) {
					mm_util_debug("\tfile open failed %d\n", errno);
					return MM_ERROR_IMAGE_INTERNAL;
				}
				src = malloc(size);
				if (src == NULL) {
					mm_util_debug("\tmemory allocation failed\n");
					return MM_ERROR_IMAGE_INTERNAL;
				}
				if(fread(src, 1, (int)size, fp)) {
					mm_util_debug("#Success# fread");
					memcpy(ptr, src, (int)size);
					mm_util_debug("memcpy");
				} else {
					mm_util_error("#Error# fread");
				}
			}
		}
	} else {
		mm_util_debug("ERROR - Create Media Packet");
		return ret;
	}

	/* Set Source */
	ret = mm_util_set_hardware_acceleration(MMHandle, atoi(argv[2]));
	if(ret == MM_ERROR_NONE) {
		mm_util_debug("Success - Set hardware_acceleration");
	} else {
		mm_util_debug("ERROR - Set hardware_acceleration");
		return ret;
	}

	ret = mm_util_set_resolution(MMHandle, 176, 144);
	if(ret == MM_ERROR_NONE) {
		mm_util_debug("Success - Set Convert Info");
	} else {
		media_format_unref(fmt);
		mm_util_debug("ERROR - Set Convert Info");
		return ret;
	}

	/* Transform */
	ret = mm_util_transform(MMHandle, src_packet, (mm_util_completed_callback) transform_completed_cb, handle);
	if(ret == MM_ERROR_NONE) {
		mm_util_debug("Success - Transform");
	} else {
		media_format_unref(fmt);
		mm_util_error("ERROR - Transform");
		return ret;
	}

	mm_util_debug("Wait...");
	while (false == completed) {} // polling

	ret = mm_util_destroy(MMHandle);
	if(ret == MM_ERROR_NONE) {
		mm_util_debug("Success - Destroy");
	} else {
		media_format_unref(fmt);
		mm_util_error("ERROR - Destroy");
		return ret;
	}

	media_format_unref(fmt);
	mm_util_debug("Destory - src packet");
	media_packet_destroy(src_packet);
	mm_util_debug("destroy");

	return ret;
}

