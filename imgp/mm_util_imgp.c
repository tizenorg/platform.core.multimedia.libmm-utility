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
#include <limits.h>
#include <mm_debug.h>
#include "mm_util_imgp.h"
#include "mm_util_imgp_internal.h"
#include <gmodule.h>
#include <mm_error.h>

#define MM_UTIL_ROUND_UP_2(num)  (((num)+1)&~1)
#define MM_UTIL_ROUND_UP_4(num)  (((num)+3)&~3)
#define MM_UTIL_ROUND_UP_8(num)  (((num)+7)&~7)
#define MM_UTIL_ROUND_UP_16(num)  (((num)+15)&~15)
#define GEN_MASK(x) ((1<<(x))-1)
#define ROUND_UP_X(v,x) (((v) + GEN_MASK(x)) & ~GEN_MASK(x))
#define DIV_ROUND_UP_X(v,x) (((v) + GEN_MASK(x)) >> (x))

typedef gboolean(*IMGPInfoFunc)  (imgp_info*, imgp_plugin_type);
/*########################################################################################*/
#define setup_image_size_I420(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * MM_UTIL_ROUND_UP_2 (height) + MM_UTIL_ROUND_UP_8 (width) * MM_UTIL_ROUND_UP_2 (height) /2); \
	return size; \
}

#define setup_image_size_Y42B(width, height)  { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * height + MM_UTIL_ROUND_UP_8 (width)  * height); \
	return size; \
}	

#define setup_image_size_Y444(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * height  * 3); \
	return size; \
}

#define setup_image_size_UYVY(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_2 (width) * 2 * height); \
	return size; \
}

#define setup_image_size_YUYV(width, height)  { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_2 (width) * 2 * height); \
	return size; \
}

#define setup_image_size_YV12(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * MM_UTIL_ROUND_UP_2 (height)+ MM_UTIL_ROUND_UP_8 (width) * MM_UTIL_ROUND_UP_2 (height) / 2); \
	return size; \
}

#define setup_image_size_NV12(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * MM_UTIL_ROUND_UP_2 (height) *1.5); \
	return size; \
}

#define setup_image_size_RGB565(width, height)  { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width * 2) *height); \
	return size; \
}

#define setup_image_size_RGB888(width, height)  { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width*3) * height); \
	return size; \
}

#define setup_image_size_BGR888(width, height)  { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width*3) * height); \
	return size; \
}

#define setup_image_size_SN12(width, height) { \
	int size=0; \
	size = MM_UTIL_ROUND_UP_4 (width) * MM_UTIL_ROUND_UP_2 (height) *1.5;  \
	return size; \
}

#define setup_image_size_ST12(width, height) { \
	int size=0; \
	size = MM_UTIL_ROUND_UP_4 (width) * MM_UTIL_ROUND_UP_2 (height) *1.5;  \
	return size; \
}

#define setup_image_size_UYVY(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_2 (width) * 2 * height); \
	return size; \
}

#define setup_image_size_BGRX888(width, height) {  \
	int size=0; \
	size = MM_UTIL_ROUND_UP_4 (width*3) * height); \
	return size; \
}

/*########################################################################################*/

static int
check_valid_picture_size(int width, int height)
{
    if((int)width>0 && (int)height>0 && (width+128)*(unsigned long long)(height+128) < INT_MAX/4)
        return MM_ERROR_NONE;
    return MM_ERROR_IMAGE_INVALID_VALUE;
}

static int
_mm_setup_image_size(const char* _format_label, int width, int height)
{
	int size=0;
	if(strcmp(_format_label, "I420") == 0)		      		{setup_image_size_I420(width, height);} //width * height *1.5;	
	else if(strcmp(_format_label, "Y42B") == 0)			{setup_image_size_Y42B(width, height);}//width * height *2;	 			
	else if(strcmp(_format_label, "YUV422") == 0)		{setup_image_size_Y42B(width, height);}//width * height *2;	 			
	else if(strcmp(_format_label, "Y444") == 0)			{setup_image_size_Y444(width, height);}//width * height *3;
	else if(strcmp(_format_label, "YV12") == 0)			{setup_image_size_YV12(width, height);}//width * height *1;
	else if(strcmp(_format_label, "NV12") == 0)			{setup_image_size_NV12(width, height) }//width * height *1.5;
	else if(strcmp(_format_label, "ST12") == 0)			{setup_image_size_ST12(width, height);}//width * height *1.5;
	else if(strcmp(_format_label, "SN12") == 0)			{setup_image_size_SN12(width, height) }//width * height *1.5;
	else if(strcmp(_format_label, "UYVY") == 0)			{setup_image_size_UYVY(width, height); } //width * height *2;	
	else if(strcmp(_format_label, "YUYV") == 0)			{setup_image_size_YUYV(width, height); } //width * height *2;	
	else if(strcmp(_format_label, "RGB565") == 0)		{setup_image_size_RGB565(width, height);}//width * height *2;
	else if(strcmp(_format_label, "RGB888") == 0)		{setup_image_size_RGB888(width, height);}//width * height *3;
	else if(strcmp(_format_label, "BGR888") == 0)		{setup_image_size_BGR888(width, height);}//width * height *3;	
	else if(strcmp(_format_label, "ARGB8888") == 0)		{ size = width * height *4; mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] file_size: %d\n", __func__, __LINE__, size);}
	else if(strcmp(_format_label, "BGRA8888") == 0)		{ size = width * height *4; mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] file_size: %d\n", __func__, __LINE__, size);}
	else if(strcmp(_format_label, "RGBA8888") == 0)		{ size = width * height *4; mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] file_size: %d\n", __func__, __LINE__, size);}
	else if(strcmp(_format_label, "ABGR8888") == 0)		{ size = width * height *4; mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] file_size: %d\n", __func__, __LINE__, size);} 
	else if(strcmp(_format_label, "BGRX") == 0)			{ size = width * height *4; mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] file_size: %d\n", __func__, __LINE__, size);}
	return size;
}

static gboolean
_mm_cannot_convert_format(mm_util_img_format src_format, mm_util_img_format dst_format )
{
	gboolean _bool=FALSE;
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d]  src_format: %d,  dst_format:%d", __func__, __LINE__, src_format, dst_format);
	if(  ((src_format == MM_UTIL_IMG_FMT_YUV422) && (dst_format == MM_UTIL_IMG_FMT_NV12_TILED))  ||
		
		((src_format == MM_UTIL_IMG_FMT_NV12) && (dst_format == MM_UTIL_IMG_FMT_NV12_TILED))  ||

		((src_format == MM_UTIL_IMG_FMT_UYVY) && (dst_format == MM_UTIL_IMG_FMT_NV12_TILED))  ||

		((src_format == MM_UTIL_IMG_FMT_YUYV) && (dst_format == MM_UTIL_IMG_FMT_NV12_TILED))  ||

		((src_format == MM_UTIL_IMG_FMT_RGB565) && (dst_format == MM_UTIL_IMG_FMT_NV12_TILED))  ||
		
		((src_format == MM_UTIL_IMG_FMT_RGB888) && (dst_format == MM_UTIL_IMG_FMT_NV12_TILED)) ||

		((src_format == MM_UTIL_IMG_FMT_BGRX8888) && (dst_format == MM_UTIL_IMG_FMT_NV12_TILED)) ||
		
		((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_YUV422))  || ((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_UYVY))  ||
		((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_YUYV))  || ((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_ARGB8888))  ||
		((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_RGBA8888))  ||((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_BGRX8888)) )
		   
		{
			_bool = TRUE;
		}

	return _bool;
}

static gboolean
_mm_check_convert_format(mm_util_img_format src_format, mm_util_img_format dst_format )
{
	gboolean _bool=FALSE;
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d]  src_format: %d,  dst_format:%d", __func__, __LINE__, src_format, dst_format);
	if(((src_format == MM_UTIL_IMG_FMT_YUV420) && (dst_format == MM_UTIL_IMG_FMT_NV12))  || ((src_format == MM_UTIL_IMG_FMT_YUV420) && (dst_format == MM_UTIL_IMG_FMT_RGB565))  ||
		((src_format == MM_UTIL_IMG_FMT_YUV420) && (dst_format == MM_UTIL_IMG_FMT_RGB888))  || ((src_format == MM_UTIL_IMG_FMT_YUV420) && (dst_format == MM_UTIL_IMG_FMT_ARGB8888))  ||
		((src_format == MM_UTIL_IMG_FMT_YUV420) && (dst_format == MM_UTIL_IMG_FMT_BGRA8888))  || ((src_format == MM_UTIL_IMG_FMT_YUV420) && (dst_format == MM_UTIL_IMG_FMT_RGBA8888))  ||	
		((src_format == MM_UTIL_IMG_FMT_YUV420) && (dst_format == MM_UTIL_IMG_FMT_NV12_TILED)) ||
		
		((src_format == MM_UTIL_IMG_FMT_I420) && (dst_format == MM_UTIL_IMG_FMT_NV12))  || ((src_format == MM_UTIL_IMG_FMT_I420) && (dst_format == MM_UTIL_IMG_FMT_RGB565))  ||
		((src_format == MM_UTIL_IMG_FMT_I420) && (dst_format == MM_UTIL_IMG_FMT_RGB888))  || ((src_format == MM_UTIL_IMG_FMT_I420) && (dst_format == MM_UTIL_IMG_FMT_ARGB8888))  ||
		((src_format == MM_UTIL_IMG_FMT_I420) && (dst_format == MM_UTIL_IMG_FMT_BGRA8888))  || ((src_format == MM_UTIL_IMG_FMT_I420) && (dst_format == MM_UTIL_IMG_FMT_RGBA8888))  ||
		((src_format == MM_UTIL_IMG_FMT_I420) && (dst_format == MM_UTIL_IMG_FMT_NV12_TILED)) ||
		
		((src_format == MM_UTIL_IMG_FMT_NV12) && (dst_format == MM_UTIL_IMG_FMT_YUV420))  || ((src_format == MM_UTIL_IMG_FMT_NV12) && (dst_format == MM_UTIL_IMG_FMT_I420))  ||
		((src_format == MM_UTIL_IMG_FMT_NV12) && (dst_format == MM_UTIL_IMG_FMT_RGB565))  || ((src_format == MM_UTIL_IMG_FMT_NV12) && (dst_format == MM_UTIL_IMG_FMT_RGB888))  ||

		((src_format == MM_UTIL_IMG_FMT_UYVY) && (dst_format == MM_UTIL_IMG_FMT_RGB565))  || ((src_format == MM_UTIL_IMG_FMT_UYVY) && (dst_format == MM_UTIL_IMG_FMT_RGB888))  ||

		((src_format == MM_UTIL_IMG_FMT_YUYV) && (dst_format == MM_UTIL_IMG_FMT_RGB565))  || ((src_format == MM_UTIL_IMG_FMT_YUYV) && (dst_format == MM_UTIL_IMG_FMT_RGB888))  ||

		((src_format == MM_UTIL_IMG_FMT_RGB565) && (dst_format == MM_UTIL_IMG_FMT_YUV420))  || ((src_format == MM_UTIL_IMG_FMT_RGB565) && (dst_format == MM_UTIL_IMG_FMT_I420))  ||
		((src_format == MM_UTIL_IMG_FMT_RGB565) && (dst_format == MM_UTIL_IMG_FMT_NV12))  || 
		
		((src_format == MM_UTIL_IMG_FMT_RGB888) && (dst_format == MM_UTIL_IMG_FMT_YUV420))  || ((src_format == MM_UTIL_IMG_FMT_RGB888) && (dst_format == MM_UTIL_IMG_FMT_I420))  ||
		((src_format == MM_UTIL_IMG_FMT_RGB888) && (dst_format == MM_UTIL_IMG_FMT_NV12))  || 

		((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_YUV420))  || ((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_I420))  ||
		((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_NV12))  || ((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_RGB565))  ||
		((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_RGB888))  || 
		((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_BGRA8888))  || ((src_format == MM_UTIL_IMG_FMT_NV12_TILED) && (dst_format == MM_UTIL_IMG_FMT_RGBA8888)))
		   
		{
			_bool = TRUE;
		}

	return _bool;
}

static gboolean
_mm_check_resize_format(mm_util_img_format _format)
{
	gboolean _bool = FALSE;
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d]  _format: %d", __func__, __LINE__, _format);
	if( (_format == MM_UTIL_IMG_FMT_UYVY) || (_format == MM_UTIL_IMG_FMT_YUYV) || (_format == MM_UTIL_IMG_FMT_RGBA8888) || (_format == MM_UTIL_IMG_FMT_BGRX8888) )
	{
		_bool = FALSE;
	}
	else 
	{
		_bool = TRUE;
	}
	return _bool;
}

static gboolean
_mm_check_rotate_format(mm_util_img_format _format)
{
	gboolean _bool = FALSE;
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] _format: %d", __func__, __LINE__, _format);
	if( (_format == MM_UTIL_IMG_FMT_YUV420) || (_format == MM_UTIL_IMG_FMT_I420)
		|| (_format == MM_UTIL_IMG_FMT_NV12) || (_format == MM_UTIL_IMG_FMT_RGB888) )
		{
			_bool = TRUE;
		}

		return _bool;
}

static int
_mm_set_format_label(char* format_label, mm_util_img_format _format)
{
	int ret = MM_ERROR_NONE;
	if(format_label == NULL)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] format_label: %s", __func__, __LINE__, format_label);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	
	if(_format == MM_UTIL_IMG_FMT_YUV420) 		strncpy(format_label, "I420", buf_size);
	else if(_format == MM_UTIL_IMG_FMT_YUV422)	strncpy(format_label, "Y42B", buf_size);
	else if(_format == MM_UTIL_IMG_FMT_I420)           strncpy(format_label, "I420", buf_size);
	else if(_format == MM_UTIL_IMG_FMT_NV12)          strncpy(format_label, "NV12", buf_size);
	else if(_format == MM_UTIL_IMG_FMT_UYVY)          strncpy(format_label, "UYVY", buf_size);
	else if(_format == MM_UTIL_IMG_FMT_YUYV)          strncpy(format_label, "YUYV", buf_size);
	else if(_format ==MM_UTIL_IMG_FMT_RGB565)       strncpy(format_label, "RGB565", buf_size);
	else if(_format ==MM_UTIL_IMG_FMT_RGB888)       strncpy(format_label, "RGB888", buf_size);
	else if(_format ==MM_UTIL_IMG_FMT_ARGB8888)   strncpy(format_label, "ARGB8888", buf_size);
	else if(_format ==MM_UTIL_IMG_FMT_BGRA8888)   strncpy(format_label, "BGRA8888", buf_size);
	else if(_format ==MM_UTIL_IMG_FMT_RGBA8888)   strncpy(format_label, "RGBA8888", buf_size); 
	else if(_format ==MM_UTIL_IMG_FMT_BGRX8888)	  strncpy(format_label, "BGRX", buf_size);
	else if(_format ==MM_UTIL_IMG_FMT_NV12_TILED) strncpy(format_label, "NV12T", buf_size);
	else mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] ERROR - You check mm_util_img_format", __func__, __LINE__);
	
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] format_label: %s", __func__, __LINE__, format_label);
	return ret;
}

static int
_mm_set_imgp_info(imgp_info * _imgp_info, unsigned char *src,  mm_util_img_format src_format, unsigned int src_width, unsigned int src_height, mm_util_img_format dst_format, unsigned int dst_width, unsigned int dst_height, mm_util_img_rotate_type angle)
{ 
	int ret = MM_ERROR_NONE;
	if(_imgp_info == NULL)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] _imgp_info is NULL", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	
	unsigned int src_size=0;
	unsigned int dst_size=0;

	char input_format_label[buf_size]; 			
	char output_format_label[buf_size];
	
	ret=_mm_set_format_label(input_format_label, src_format); 
	ret=_mm_set_format_label(output_format_label, dst_format);
	if(ret != MM_ERROR_NONE)
	{
		mmf_debug (MMF_DEBUG_ERROR, "[%s][%05d] mm_set_format_label error", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	strncpy(_imgp_info->input_format_label, input_format_label, buf_size);
	mm_util_get_image_size(src_format, src_width, src_height, &src_size);
	if(src_size != _mm_setup_image_size(input_format_label, src_width, src_height)) 
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] src image size error", __func__, __LINE__);
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] src_size: %d\n", __func__, __LINE__, src_size);
	_imgp_info->src=(char*)malloc(sizeof(char*) * src_size);
	if(_imgp_info->src == NULL)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] _imgp_info->src is NULL", __func__, __LINE__);
		free(_imgp_info->src); 
		_imgp_info->src = NULL;
		return MM_ERROR_IMAGE_FILEOPEN;
	}

	memcpy(_imgp_info->src, src, src_size); 
	_imgp_info->src_format=src_format;
	_imgp_info->src_width = src_width;
	_imgp_info->src_height= src_height;
	
	strncpy(_imgp_info->output_format_label, output_format_label, buf_size);
	mm_util_get_image_size(dst_format, dst_width, dst_height, &dst_size);
	if(dst_size != _mm_setup_image_size(output_format_label,dst_width, dst_height))
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] dst image size error", __func__, __LINE__);
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] dst_size: %d\n", __func__, __LINE__, dst_size);
	_imgp_info->dst=(char*)malloc(sizeof(char*) * dst_size);
	if(_imgp_info->dst == NULL)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] _imgp_info->src is NULL", __func__, __LINE__);
		free(_imgp_info->dst); 
		_imgp_info->dst = NULL;
		return MM_ERROR_IMAGE_FILEOPEN;
	}
	_imgp_info->dst_format=dst_format;
	_imgp_info->dst_width =  dst_width;
	_imgp_info->dst_height =  dst_height;
	_imgp_info->angle= angle;

	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] [input] format label : %s  src: %p width: %d  height: %d [output] format label: %s width: %d height: %d rotation_value: %d",
	__func__, __LINE__, _imgp_info->input_format_label, _imgp_info->src, _imgp_info->src_width, _imgp_info->src_height, 
	_imgp_info->output_format_label, _imgp_info->dst_width, _imgp_info->dst_height, _imgp_info->angle);

	return ret;	
}

static GModule *
_mm_util_imgp_initialize(imgp_plugin_type _imgp_plugin_type)
{
	GModule *module = NULL;
	mmf_debug(MMF_DEBUG_LOG,  "[%s][%05d] #Start dlopen#", __func__, __LINE__ );

	if( _imgp_plugin_type == IMGP_NEON )	   
	{	
		module = g_module_open(PATH_NEON_LIB, G_MODULE_BIND_LAZY );		
	}
	else if( _imgp_plugin_type == IMGP_GSTCS)
	{			
		module = g_module_open(PATH_GSTCS_LIB, G_MODULE_BIND_LAZY );
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] #Success g_module_open#", __func__, __LINE__ );	
	if( module == NULL )
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] %s | %s module open failed", __func__, __LINE__, PATH_NEON_LIB, PATH_GSTCS_LIB);
		return NULL;
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] module: %p,  g_module_name: %s", __func__, __LINE__, module,  g_module_name (module));
	return module;
}

static IMGPInfoFunc
_mm_util_imgp_process(GModule *module)
{
	IMGPInfoFunc mm_util_imgp_func = NULL;

	if(module == NULL)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] module is NULL", __func__, __LINE__);
		return NULL;
	}
	
	mmf_debug(MMF_DEBUG_LOG,  "[%s][%05d] #_mm_util_imgp_process#", __func__, __LINE__ );
	
	g_module_symbol(module, IMGP_FUNC_NAME, (gpointer*)&mm_util_imgp_func);	
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] mm_util_imgp_func: %p", __func__, __LINE__, mm_util_imgp_func);
	
	return mm_util_imgp_func;
}

static int
_mm_util_imgp_finalize(GModule *module, imgp_info *_imgp_info)
{
	int ret = MM_ERROR_NONE;

	if(module)
	{
		mmf_debug(MMF_DEBUG_LOG,  "[%s][%05d] module : %p", __func__, __LINE__, module);
		g_module_close( module );
		mmf_debug(MMF_DEBUG_LOG,  "[%s][%05d] #End g_module_close#", __func__, __LINE__ );
		module = NULL;	
	}
	else
	{
		mmf_debug(MMF_DEBUG_ERROR,  "[%s][%05d] #module is NULL#", __func__, __LINE__ );
		return MM_ERROR_IMAGE_INTERNAL;
	}

	if(_imgp_info)
	{
		if(_imgp_info->src) 		  { free(_imgp_info->src);_imgp_info->src=NULL; }
		mmf_debug(MMF_DEBUG_LOG,  "[%s][%05d] #Success _imgp_info->src#", __func__, __LINE__ );
	        if(_imgp_info->dst) 		  { free(_imgp_info->dst);_imgp_info->dst=NULL; }
		mmf_debug(MMF_DEBUG_LOG,  "[%s][%05d] #Success _imgp_info->dst#", __func__, __LINE__ );
	        if(_imgp_info) 			  { free(_imgp_info); _imgp_info=NULL;}
		mmf_debug(MMF_DEBUG_LOG,  "[%s][%05d] #Success _imgp_info#", __func__, __LINE__ );	
	}
	else
	{
		mmf_debug(MMF_DEBUG_ERROR,  "[%s][%05d] #_imgp_info is NULL#", __func__, __LINE__ );
		return MM_ERROR_IMAGE_INTERNAL;
	}		
	return ret;
}

EXPORT_API int
mm_util_convert_colorspace(unsigned char *src, unsigned int src_width, unsigned int src_height, mm_util_img_format src_format, unsigned char *dst, mm_util_img_format dst_format)
{
	int ret = MM_ERROR_NONE;

	if (!src || !dst)
	{
	    mmf_debug (MMF_DEBUG_ERROR, "[%s][%05d] invalid mm_util_convert_colorspace\n", __func__, __LINE__);
	    return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (src_format < MM_UTIL_IMG_FMT_YUV420) || (src_format > MM_UTIL_IMG_FMT_NUM) || (dst_format < MM_UTIL_IMG_FMT_YUV420) || (dst_format > MM_UTIL_IMG_FMT_NUM) )
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# src_format: %d || dst_format:%d  value ", __func__, __LINE__, src_format, dst_format);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (src_width < 0) || (src_height < 0))
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# src_width || src_height value ", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] #START#", __func__, __LINE__);
	
	if (_mm_cannot_convert_format(src_format, dst_format))
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# Cannot Support Image Format Convert", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	
	imgp_info* _imgp_info=(imgp_info*)malloc(sizeof(imgp_info));		
	unsigned int dst_size=0;
	IMGPInfoFunc  _mm_util_imgp_func  = NULL;
	GModule *_module  = NULL;
	imgp_plugin_type _imgp_plugin_type=-1;

	/* Initialize */
	if( _mm_check_convert_format(src_format, dst_format))	    _imgp_plugin_type = IMGP_NEON;
	else  												 	    _imgp_plugin_type = IMGP_GSTCS;
	_module  = _mm_util_imgp_initialize(_imgp_plugin_type);
	
	if(_module == NULL) //when IMGP_NEON is NULL	
	{
		_imgp_plugin_type = IMGP_GSTCS;
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] You use %s module", __func__, __LINE__, PATH_GSTCS_LIB);
		_module  = _mm_util_imgp_initialize(_imgp_plugin_type);
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] _mm_util_imgp_func: %p", __func__, __LINE__, _module);
	ret=_mm_set_imgp_info(_imgp_info, src, src_format, src_width, src_height, dst_format, src_width, src_height, MM_UTIL_ROTATE_0);
	if(ret != MM_ERROR_NONE)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] _mm_set_imgp_info failed", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Sucess _mm_set_imgp_info", __func__, __LINE__);

	/* image processing */
	_mm_util_imgp_func = _mm_util_imgp_process(_module);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Sucess _mm_util_imgp_process", __func__, __LINE__);
	
	if(_mm_util_imgp_func)
	{
		ret=_mm_util_imgp_func(_imgp_info, IMGP_CSC);
		 if (ret != MM_ERROR_NONE)
		{
			mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] image processing failed", __func__, __LINE__); 
			return MM_ERROR_IMAGE_INVALID_VALUE;
		}
	}
	else		
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] g_module_symbol failed", __func__, __LINE__); 
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	
        /* Output result*/
	mm_util_get_image_size(_imgp_info->dst_format, _imgp_info->dst_width, _imgp_info->dst_height, &dst_size);
	memcpy(dst, _imgp_info->dst, dst_size);

	/* Finalize */
	ret = _mm_util_imgp_finalize(_module, _imgp_info);
	if(ret != MM_ERROR_NONE)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] _mm_util_imgp_finalize failed", __func__, __LINE__);
		return MM_ERROR_IMAGE_INTERNAL;
	}
    	return ret;
}

EXPORT_API int
mm_util_resize_image(unsigned char *src, unsigned int src_width, unsigned int src_height, mm_util_img_format src_format, unsigned char *dst, unsigned int *dst_width, unsigned int *dst_height)
{
	int ret = MM_ERROR_NONE;
	if (!src || !dst)
	{
		mmf_debug (MMF_DEBUG_ERROR, "[%s][%05d] invalid argument\n", __func__, __LINE__);
	    	return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (src_format < MM_UTIL_IMG_FMT_YUV420) || (src_format > MM_UTIL_IMG_FMT_NUM) )
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# src_format value ", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( !dst_width || !dst_height )
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# dst width/height buffer is NULL", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (src_width < 0) || (src_height < 0))
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# src_width || src_height value ", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	
	imgp_info* _imgp_info=(imgp_info*)malloc(sizeof(imgp_info));	
	unsigned int dst_size=0;
	IMGPInfoFunc  _mm_util_imgp_func  = NULL;	
	GModule *_module  = NULL;
	imgp_plugin_type _imgp_plugin_type=-1;

	/* Initialize */
	if( _mm_check_resize_format(src_format))	    _imgp_plugin_type = IMGP_NEON;
	else  										    _imgp_plugin_type = IMGP_GSTCS;
	_module  = _mm_util_imgp_initialize(_imgp_plugin_type);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] _mm_util_imgp_init: %p", __func__, __LINE__, _module);
	if(_module == NULL)  //when IMGP_NEON is NULL
	{
		 _imgp_plugin_type = IMGP_GSTCS;
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] You use %s module", __func__, __LINE__, PATH_GSTCS_LIB);
		_module  = _mm_util_imgp_initialize(_imgp_plugin_type);
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] _mm_set_imgp_info", __func__, __LINE__);
	ret=_mm_set_imgp_info(_imgp_info, src, src_format, src_width, src_height, src_format, *dst_width, *dst_height, MM_UTIL_ROTATE_0);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] _mm_set_imgp_info ret: %d", __func__, __LINE__, ret);
	if(ret != MM_ERROR_NONE)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] _mm_set_imgp_info failed", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Sucess _mm_set_imgp_info", __func__, __LINE__);

	/* image processing */
	_mm_util_imgp_func = _mm_util_imgp_process(_module);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Sucess _mm_util_imgp_process", __func__, __LINE__);
	if(_mm_util_imgp_func)
	{
		ret=_mm_util_imgp_func(_imgp_info, IMGP_RSZ);
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] _mm_util_imgp_func, ret: %d", __func__, __LINE__, ret);
		 if (ret != MM_ERROR_NONE)
		{
			mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] image processing failed", __func__, __LINE__); 
			return MM_ERROR_IMAGE_INVALID_VALUE;
		}
	}
	else		
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] g_module_symbol failed", __func__, __LINE__); 
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	 /* Output result*/
	mm_util_get_image_size(_imgp_info->dst_format, _imgp_info->dst_width, _imgp_info->dst_height, &dst_size);
	memcpy(dst, _imgp_info->dst, dst_size);

	/* Finalize */
	ret = _mm_util_imgp_finalize(_module, _imgp_info);
	if(ret != MM_ERROR_NONE)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] _mm_util_imgp_finalize failed", __func__, __LINE__);
		return MM_ERROR_IMAGE_INTERNAL;
	}
	return ret;
}

EXPORT_API int
mm_util_rotate_image(unsigned char *src, unsigned int src_width, unsigned int src_height, mm_util_img_format src_format, unsigned char *dst, unsigned int *dst_width, unsigned int *dst_height, mm_util_img_rotate_type angle)
{
	int ret = MM_ERROR_NONE;

	if (!src || !dst)
	{
		mmf_debug (MMF_DEBUG_ERROR, "[%s][%05d] invalid argument\n", __func__, __LINE__);
	    	return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (src_format < MM_UTIL_IMG_FMT_YUV420) || (src_format > MM_UTIL_IMG_FMT_NUM) )
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# src_format value", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( !dst_width || !dst_height )
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# dst width/height buffer is NUL", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (src_width < 0) || (src_height < 0))
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# src_width || src_height value ", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if( (angle < MM_UTIL_ROTATE_0) || (angle > MM_UTIL_ROTATE_NUM) )
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] #ERROR# angle vaule", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d]  #START#", __func__, __LINE__);
	imgp_info* _imgp_info=(imgp_info*)malloc(sizeof(imgp_info));
	unsigned int dst_size=0;
	IMGPInfoFunc  _mm_util_imgp_func  = NULL;
	GModule *_module  = NULL;
	imgp_plugin_type _imgp_plugin_type=-1;

	/* Initialize */
	if( _mm_check_rotate_format(src_format))	    _imgp_plugin_type = IMGP_NEON;
	else  										    _imgp_plugin_type = IMGP_GSTCS;
	_module  = _mm_util_imgp_initialize(_imgp_plugin_type);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] _mm_util_imgp_func: %p", __func__, __LINE__, _module);
	if(_module == NULL)  //when IMGP_NEON is NULL
	{
		 _imgp_plugin_type = IMGP_GSTCS;
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] You use %s module", __func__, __LINE__, PATH_GSTCS_LIB);
		_module  = _mm_util_imgp_initialize(_imgp_plugin_type);
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] _mm_set_imgp_info", __func__, __LINE__);
	ret=_mm_set_imgp_info(_imgp_info, src, src_format, src_width, src_height, src_format, *dst_width, *dst_height, angle);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] _mm_set_imgp_info", __func__, __LINE__);
	if(ret != MM_ERROR_NONE)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] _mm_set_imgp_info failed", __func__, __LINE__);
		return MM_ERROR_IMAGE_INTERNAL;
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Sucess _mm_set_imgp_info", __func__, __LINE__);

	/* image processing */
	_mm_util_imgp_func = _mm_util_imgp_process(_module);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Sucess _mm_util_imgp_process", __func__, __LINE__);
	
	if(_mm_util_imgp_func)
	{
		ret=_mm_util_imgp_func(_imgp_info, IMGP_ROT);
		if (ret!= MM_ERROR_NONE)
		{
			mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] image processing failed", __func__, __LINE__); 
			return MM_ERROR_IMAGE_INTERNAL;
		}
	}
	else		
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] g_module_symbol failed", __func__, __LINE__); 
		return MM_ERROR_IMAGE_INTERNAL;
	}

	/* Output result*/
	mm_util_get_image_size(_imgp_info->dst_format, _imgp_info->dst_width, _imgp_info->dst_height, &dst_size);
	memcpy(dst, _imgp_info->dst, dst_size);

	/* Finalize */
	ret = _mm_util_imgp_finalize(_module, _imgp_info);
	if(ret != MM_ERROR_NONE)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] _mm_util_imgp_finalize failed", __func__, __LINE__);
		return MM_ERROR_IMAGE_INTERNAL;
	}
	return ret;
}

EXPORT_API int
mm_util_get_image_size(mm_util_img_format format, unsigned int width, unsigned int height, unsigned int *imgsize)
{
    int ret = MM_ERROR_NONE;
    unsigned char x_chroma_shift = 0;
    unsigned char y_chroma_shift = 0;
    int size, w2, h2, size2;
    int stride, stride2;

    if (!imgsize)
    {
        mmf_debug (MMF_DEBUG_ERROR, "[%s][%05d] imgsize can't be null\n", __func__, __LINE__);
        return MM_ERROR_IMAGE_FILEOPEN;
    }

    *imgsize = 0;


    if (check_valid_picture_size(width, height) < 0)
    {
        mmf_debug (MMF_DEBUG_ERROR, "[%s][%05d] invalid width and height\n", __func__, __LINE__);
        return MM_ERROR_IMAGE_INVALID_VALUE;
    }

    switch (format)
    {
    	
        case MM_UTIL_IMG_FMT_I420:
        case MM_UTIL_IMG_FMT_YUV420:
            x_chroma_shift = 1;
            y_chroma_shift = 1;
            stride = MM_UTIL_ROUND_UP_4 (width);
            h2 = ROUND_UP_X (height, x_chroma_shift);
            size = stride * h2;
            w2 = DIV_ROUND_UP_X (width, x_chroma_shift);
            stride2 = MM_UTIL_ROUND_UP_4 (w2);
            h2 = DIV_ROUND_UP_X (height, y_chroma_shift);
            size2 = stride2 * h2;
            *imgsize = size + 2 * size2;
            break;
	case MM_UTIL_IMG_FMT_YUV422:		
        case MM_UTIL_IMG_FMT_YUYV:
        case MM_UTIL_IMG_FMT_UYVY:
            stride = MM_UTIL_ROUND_UP_4 (width * 2);
            size = stride * height;
            *imgsize = size;
            break;

        case MM_UTIL_IMG_FMT_RGB565:
            stride = MM_UTIL_ROUND_UP_4 (width * 2);
            size = stride * height;
            *imgsize = size;
            break;

        case MM_UTIL_IMG_FMT_RGB888:
            stride = width * 3;
            size = stride * height;
            *imgsize = size;
            break;

        case MM_UTIL_IMG_FMT_ARGB8888:
        case MM_UTIL_IMG_FMT_BGRA8888:  //added by yh8004.kim
        case MM_UTIL_IMG_FMT_RGBA8888:		
	case MM_UTIL_IMG_FMT_BGRX8888: ////added by yh8004.kim
            stride = width * 4;
            size = stride * height;
            *imgsize = size;
            break;


        case MM_UTIL_IMG_FMT_NV12:
        case MM_UTIL_IMG_FMT_NV12_TILED:
            x_chroma_shift = 1;
            y_chroma_shift = 1;
            stride = MM_UTIL_ROUND_UP_4 (width);
            h2 = ROUND_UP_X (height, y_chroma_shift);
            size = stride * h2;
            w2 = 2 * DIV_ROUND_UP_X (width, x_chroma_shift);
            stride2 = MM_UTIL_ROUND_UP_4 (w2);
            h2 = DIV_ROUND_UP_X (height, y_chroma_shift);
            size2 = stride2 * h2;
            *imgsize = size + size2;
            break;

        default:
            mmf_debug (MMF_DEBUG_ERROR, "[%s][%05d] Not supported format\n", __func__, __LINE__);
            return MM_ERROR_IMAGE_NOT_SUPPORT_FORMAT;
    }

    return ret;
}

