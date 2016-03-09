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

#define MAX_STRING_LEN 128
#define IMAGE_FORMAT_LABEL_BUFFER_SIZE 4
mm_util_imgp_h imgp_handle = 0;
bool completed = false;

GCond g_thread_cond;
GMutex g_thread_mutex;

void _wait()
{
	g_mutex_lock(&g_thread_mutex);
	fprintf(stderr, "waiting... until finishing \n");
	g_cond_wait(&g_thread_cond, &g_thread_mutex);
	fprintf(stderr, "<=== get signal from callback \n");
	g_mutex_unlock(&g_thread_mutex);
}

void _signal()
{
	g_mutex_lock(&g_thread_mutex);
	g_cond_signal(&g_thread_cond);
	fprintf(stderr, "===> send signal to test proc \n");
	g_mutex_unlock(&g_thread_mutex);
}

media_format_mimetype_e _format_to_mime(mm_util_img_format colorspace)
{
	media_format_mimetype_e mimetype = -1;

	switch (colorspace) {
	case MM_UTIL_IMG_FMT_NV12:
		mimetype = MEDIA_FORMAT_NV12;
		break;
	case MM_UTIL_IMG_FMT_NV16:
		mimetype = MEDIA_FORMAT_NV16;
		break;
	case MM_UTIL_IMG_FMT_YUYV:
		mimetype = MEDIA_FORMAT_YUYV;
		break;
	case MM_UTIL_IMG_FMT_UYVY:
		mimetype = MEDIA_FORMAT_UYVY;
		break;
	case MM_UTIL_IMG_FMT_YUV422:
		mimetype = MEDIA_FORMAT_422P;
		break;
	case MM_UTIL_IMG_FMT_I420:
		mimetype = MEDIA_FORMAT_I420;
		break;
	case MM_UTIL_IMG_FMT_RGB565:
		mimetype = MEDIA_FORMAT_RGB565;
		break;
	case MM_UTIL_IMG_FMT_RGB888:
		mimetype = MEDIA_FORMAT_RGB888;
		break;
	case MM_UTIL_IMG_FMT_RGBA8888:
		mimetype = MEDIA_FORMAT_RGBA;
		break;
	case MM_UTIL_IMG_FMT_ARGB8888:
		mimetype = MEDIA_FORMAT_ARGB;
		break;
	case MM_UTIL_IMG_FMT_BGRA8888:
	case MM_UTIL_IMG_FMT_BGRX8888:
	case MM_UTIL_IMG_FMT_NV61:
	default:
		mimetype = -1;
		mm_util_debug("Not Supported Format");
		break;
	}

	mm_util_debug("imgp fmt: %d mimetype fmt: %d", colorspace, mimetype);
	return mimetype;
}

int _packet_finalize_callback(media_packet_h packet, int err, void *user_data)
{
	mm_util_debug("==> finalize callback func is called [%d] \n", err);
	return MEDIA_PACKET_FINALIZE;
}

bool _transform_completed_cb(media_packet_h *packet, int error, void *user_data)
{
	uint64_t size = 0;
	char* output_file = (char *)user_data;
	mm_util_debug("imgp_handle: 0x%2x", imgp_handle);

	mm_util_debug("output_file: %s", output_file);

	if (error == MM_UTIL_ERROR_NONE) {
		mm_util_debug("completed");
		FILE *fp = fopen(output_file, "w");
		if (fp) {
			media_packet_get_buffer_size(*packet, &size);
			void *dst = NULL;
			int err = media_packet_get_buffer_data_ptr(*packet, &dst);
			if (err != MEDIA_PACKET_ERROR_NONE) {
				IMGP_FREE(dst);
				fclose(fp);
				mm_util_error("Error media_packet_get_buffer_data_ptr (%d)", err);
				_signal();
				return FALSE;
			}
			mm_util_debug("dst: %p [%d]", dst, size);
			fwrite(dst, 1, size, fp);
			mm_util_debug("FREE");
			fclose(fp);
		}

		mm_util_debug("write result");
	} else {
		mm_util_error("[Error] complete cb (%d)", error);
	}

	completed = true;
	mm_util_debug("Destory - dst packet");
	media_packet_destroy(*packet);

	_signal();

	return TRUE;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	void *src;
	unsigned char *dst = NULL;

	if (argc < 12) {
		fprintf(stderr, "Usage: mm_util_imgp_testsuite sync {filename} {command} src_width src_height src_foramt dst_width dst_height dst_format roation crop_start_x crop_start_y \n");
		fprintf(stderr, "Usage: mm_util_imgp_testsuite async {filename} {command} src_width src_height src_foramt dst_width dst_height dst_format roation crop_start_x crop_start_y \n");
		fprintf(stderr, "ex: mm_util_imgp_testsuite sync test.rgb resize 1920 1080 7 1280 720 7 0 0 0 \n");
		return ret;
	}

	uint64_t src_size = 0;
	uint64_t dst_size = 0;
	bool sync_mode = (strcmp(argv[1], "sync") == 0) ? TRUE : FALSE;
	char *filename = strdup(argv[2]);
	char *command = strdup(argv[3]);
	unsigned int src_width = (unsigned int)atoi(argv[4]);
	unsigned int src_height = (unsigned int)atoi(argv[5]);
	mm_util_img_format src_format = atoi(argv[6]);
	unsigned int dst_width = (unsigned int)atoi(argv[7]);
	unsigned int dst_height = (unsigned int)atoi(argv[8]);
	mm_util_img_format dst_format = atoi(argv[9]);
	mm_util_img_rotate_type rotation = atoi(argv[10]);
	unsigned int start_x = (unsigned int)atoi(argv[11]);
	unsigned int start_y = (unsigned int)atoi(argv[12]);
	char output_file[40] = {};

	/* async mode */
	media_packet_h src_packet;
	void *ptr = NULL;
	media_format_h fmt;

	unsigned int size = 0;

	mm_util_debug("command: %s src_width: %d, src_height: %d, src_format: %d, dst_width: %d, dst_height: %d, dst_format:%d, rotation:%d", command, src_width, src_height, src_format, dst_width, dst_height, dst_format, rotation);

	/* mem allocation for src dst buffer */
	mm_util_get_image_size(src_format, src_width, src_height, &size);
	src_size = (uint64_t)size;
	mm_util_get_image_size(dst_format, dst_width, dst_height, &size);
	dst_size = (uint64_t)size;
	src = malloc(src_size);
	dst = malloc(dst_size);

	{ /* read input file */
		FILE *fp = fopen(filename, "r");
		if (fp == NULL) {
			mm_util_debug("\tfile open failed %d\n", errno);
			goto TEST_FAIL;
		}

		if (src == NULL) {
			mm_util_debug("\tmemory allocation failed\n");
			goto TEST_FAIL;
		}

		if (fread(src, 1, (int)src_size, fp))
			mm_util_debug("#Success# fread");
		else
			mm_util_error("#Error# fread");

		if (src == NULL || src_size <= 0) {
			mm_util_error("#Error# fread");
			goto TEST_FAIL;
		}

	}

	{  /* ready output file */
		char *output_fmt = (char *) g_malloc(sizeof(char) * IMAGE_FORMAT_LABEL_BUFFER_SIZE);
		memset(output_fmt, 0, IMAGE_FORMAT_LABEL_BUFFER_SIZE);
		if (dst_format == MM_UTIL_IMG_FMT_YUV420 ||
			dst_format == MM_UTIL_IMG_FMT_YUV422 ||
			dst_format == MM_UTIL_IMG_FMT_I420) {
			strncpy(output_fmt, "yuv", strlen("yuv"));
		} else {
			strncpy(output_fmt, "rgb", strlen("rgb"));
		}
		snprintf(output_file, 40, "result_%s_%dx%d.%s", command, dst_width, dst_height, output_fmt);
	}

	if (sync_mode) {
		mm_util_debug("SYNC");
		if (strcmp(command, "convert") == 0)
			ret = mm_util_convert_colorspace(src, src_width, src_height, src_format, dst, dst_format);
		else if (strcmp(command, "resize") == 0)
			ret = mm_util_resize_image(src, src_width, src_height, src_format, dst, &dst_width, &dst_height);
		else if (strcmp(command, "rotate") == 0)
			ret = mm_util_rotate_image(src, src_width, src_height, src_format, dst, &dst_width, &dst_height, rotation);
		else if (strcmp(command, "crop") == 0)
			ret = mm_util_crop_image(src, src_width, src_height, src_format, start_x, start_y, &dst_width, &dst_height, dst);

		if (ret == MM_UTIL_ERROR_NONE) {
			mm_util_debug("Success - %s", command);
		} else {
			mm_util_debug("ERROR - %s", command);
			goto TEST_FAIL;
		}

		{  /* write output file */
			FILE *fpout = fopen(output_file, "w");
			if (fpout) {
				mm_util_debug("dst: %p [%d]", dst, dst_size);
				fwrite(dst, 1, dst_size, fpout);
				mm_util_debug("FREE");
				fclose(fpout);
			}
		}
	} else {  /* Async mode */
		mm_util_debug("ASYNC");

		/* Create Transform */
		ret = mm_util_create(&imgp_handle);
		if (ret == MM_UTIL_ERROR_NONE) {
			mm_util_debug("Success - Create Transcode Handle [imgp_handle: 0x%2x]", imgp_handle);
		} else {
			mm_util_debug("ERROR - Create Transcode Handle");
			goto TEST_FAIL;
		}
		if (media_format_create(&fmt) == MEDIA_FORMAT_ERROR_NONE) {
			if (media_format_set_video_mime(fmt, _format_to_mime(src_format)) != MEDIA_FORMAT_ERROR_NONE) {
				media_format_unref(fmt);
				mm_util_error("[Error] Set - video mime");
				goto TEST_FAIL;
			}

			if (media_format_set_video_width(fmt, src_width) != MEDIA_FORMAT_ERROR_NONE) {
				media_format_unref(fmt);
				mm_util_error("[Error] Set - video width");
				goto TEST_FAIL;
			}

			if (media_format_set_video_height(fmt, src_height) != MEDIA_FORMAT_ERROR_NONE) {
				media_format_unref(fmt);
				mm_util_error("[Error] Set - video height");
				goto TEST_FAIL;
			}

			if (media_format_set_video_avg_bps(fmt, 15000000) != MEDIA_FORMAT_ERROR_NONE) {
				media_format_unref(fmt);
				mm_util_error("[Error] Set - video avg bps");
				goto TEST_FAIL;
			}

			if (media_format_set_video_max_bps(fmt, 20000000) != MEDIA_FORMAT_ERROR_NONE) {
				media_format_unref(fmt);
				mm_util_error("[Error] Set - video max bps");
				goto TEST_FAIL;
			}

			mm_util_debug("media_format_set_video_info success! w:%d, h:%d, format:%d\n", src_width, src_height, src_format);
		} else {
			mm_util_error("media_format_create failed...");
		}

		/* Set Source */
		ret = media_packet_create_alloc(fmt, (media_packet_finalize_cb)_packet_finalize_callback, NULL, &src_packet);
		if (ret == MM_UTIL_ERROR_NONE) {
			mm_util_debug("Success - Create Media Packet(%p)", src_packet);
			uint64_t buffer_size = (uint64_t)size;
			if (media_packet_get_buffer_size(src_packet, &buffer_size) == MEDIA_PACKET_ERROR_NONE) {
				ptr = malloc(buffer_size);
				if (ptr == NULL) {
					mm_util_debug("\tmemory allocation failed\n");
					goto TEST_FAIL;
				}
				if (media_packet_get_buffer_data_ptr(src_packet, &ptr) == MEDIA_PACKET_ERROR_NONE) {
					if (src != NULL && src_size > 0) {
						memcpy(ptr, src, buffer_size);
						mm_util_debug("memcpy");
					}
				}
			}
		} else {
			mm_util_debug("ERROR - Create Media Packet");
			return ret;
		}

		ret = mm_util_set_hardware_acceleration(imgp_handle, FALSE);
		if (ret == MM_UTIL_ERROR_NONE) {
			mm_util_debug("Success - Set hardware_acceleration");
		} else {
			mm_util_debug("ERROR - Set hardware_acceleration");
			goto TEST_FAIL;
		}

		if (strcmp(command, "convert") == 0) {
			ret = mm_util_set_colorspace_convert(imgp_handle, dst_format);
			if (ret == MM_UTIL_ERROR_NONE) {
				mm_util_debug("Success - Set colorspace Info");
			} else {
				mm_util_debug("ERROR - Set colorspace Info");
				goto TEST_FAIL;
			}
		}

		if (strcmp(command, "crop") == 0) {
			ret = mm_util_set_crop_area(imgp_handle, start_x, start_y, (start_x + dst_width), (start_y + dst_height));
			if (ret == MM_UTIL_ERROR_NONE) {
				mm_util_debug("Success - Set crop Info");
			} else {
				mm_util_debug("ERROR - Set crop Info");
				goto TEST_FAIL;
			}
		}

		if (strcmp(command, "resize") == 0) {
			ret = mm_util_set_resolution(imgp_handle, dst_width, dst_height);
			if (ret == MM_UTIL_ERROR_NONE) {
				mm_util_debug("Success - Set resolution Info");
			} else {
				mm_util_debug("ERROR - Set resolution Info");
				goto TEST_FAIL;
			}
		}

		if (strcmp(command, "rotate") == 0) {
			ret = mm_util_set_rotation(imgp_handle, rotation);
			if (ret == MM_UTIL_ERROR_NONE) {
				mm_util_debug("Success - Set rotation Info");
			} else {
				mm_util_debug("ERROR - Set rotation Info");
				goto TEST_FAIL;
			}
		}

		/* Transform */
		ret = mm_util_transform(imgp_handle, src_packet, (mm_util_completed_callback) _transform_completed_cb, output_file);
		if (ret == MM_UTIL_ERROR_NONE) {
			mm_util_debug("Success - Transform");
		} else {
			mm_util_error("ERROR - Transform");
			goto TEST_FAIL;
		}

		mm_util_debug("Wait...");
		g_mutex_init(&g_thread_mutex);
		g_cond_init(&g_thread_cond);
		_wait();
		g_mutex_clear(&g_thread_mutex);
		g_cond_clear(&g_thread_cond);

		ret = mm_util_destroy(imgp_handle);
		if (ret == MM_UTIL_ERROR_NONE) {
			mm_util_debug("Success - Destroy");
		} else {
			mm_util_error("ERROR - Destroy");
			goto TEST_FAIL;
		}

	}
TEST_FAIL:

	IMGP_FREE(src);
	IMGP_FREE(dst);
	IMGP_FREE(command);
	IMGP_FREE(filename);
	if (!sync_mode) {
		media_format_unref(fmt);
		mm_util_debug("Destory - src packet");
		media_packet_destroy(src_packet);
		mm_util_debug("destroy");
	}

	return 0;
}

