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
#include <math.h>
#include "mm_util_debug.h"
#include "mm_util_imgp.h"
#include "mm_util_imgcv.h"
#include "mm_util_imgcv_internal.h"

#ifdef ENABLE_TTRACE
#include <ttrace.h>
#define TTRACE_BEGIN(NAME) traceBegin(TTRACE_TAG_IMAGE, NAME)
#define TTRACE_END() traceEnd(TTRACE_TAG_IMAGE)
#else //ENABLE_TTRACE
#define TTRACE_BEGIN(NAME)
#define TTRACE_END()
#endif //ENABLE_TTRACE

#define RGB_COLOR_CHANNELS 3
#define HSV_COLOR_CHANNELS 3

#define HISTOGRAM_CHANNELS 3
#define DEFAULT_NUM_HBINS 30
#define DEFAULT_NUM_SBINS 32
#define DEFAULT_NUM_VBINS 30

#define DEFAULT_RANGE_HUE 180
#define DEFAULT_RANGE_SATURATION 256
#define DEFAULT_RANGE_VALUE 256

static int _mm_util_imgcv_init(mm_util_imgcv_s *handle, int width, int height)
{
	mm_util_debug("Enter _mm_util_imgcv_init");

	handle->width = width;
	handle->height = height;

	handle->inImg = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, RGB_COLOR_CHANNELS);
	if (handle->inImg == NULL) {
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	handle->hBins = DEFAULT_NUM_HBINS;
	handle->sBins = DEFAULT_NUM_SBINS;
	handle->vBins = DEFAULT_NUM_VBINS;

	handle->sizeOfHist[0] = handle->hBins;
	handle->sizeOfHist[1] = handle->sBins;
	handle->sizeOfHist[2] = handle->vBins;

	handle->hRanges[0] = 0;
	handle->hRanges[1] = DEFAULT_RANGE_HUE;
	handle->sRanges[0] = 0;
	handle->sRanges[1] = DEFAULT_RANGE_SATURATION;
	handle->vRanges[0] = 0;
	handle->vRanges[1] = DEFAULT_RANGE_VALUE;

	mm_util_debug("Leave _mm_util_imgcv_uninit");

	return MM_UTIL_ERROR_NONE;
}

static void _mm_util_imgcv_uninit(mm_util_imgcv_s *handle)
{
	mm_util_debug("Enter _mm_util_imgcv_uninit");

	if (handle->inImg != NULL) {
		cvReleaseImageHeader(&handle->inImg);
		handle->inImg = NULL;
	}

	mm_util_debug("Leave _mm_util_imgcv_uninit");
}

static int _mm_util_imgcv_set_buffer(mm_util_imgcv_s *handle, void *image_buffer)
{
	mm_util_debug("Enter _mm_util_imgcv_set_buffer");

	unsigned char *buffer = (unsigned char *)image_buffer;

	mm_util_debug("image_buffer [%p], width [%d]", buffer, handle->width);

	cvSetData(handle->inImg, buffer, RGB_COLOR_CHANNELS*(handle->width));
	if (handle->inImg == NULL) {
		return	MM_UTIL_ERROR_INVALID_OPERATION;
	}

	mm_util_debug("Leave _mm_util_imgcv_set_buffer");

	return MM_UTIL_ERROR_NONE;
}

static void _convert_hsv_to_rgb(int hVal, int sVal, int vVal, float *rVal, float *gVal, float *bVal)
{
	mm_util_debug("Enter _convert_hsv_to_rgb");

	CvMat *mat1 = cvCreateMat(1, 1 , CV_8UC3);
	cvSet2D(mat1, 0, 0, cvScalar((double)hVal, (double)sVal, (double)vVal, 0.0));

	CvMat *mat2 = cvCreateMat(1, 1, CV_8UC3);

	cvCvtColor(mat1, mat2, CV_HSV2BGR);

	CvScalar bgr = cvGet2D(mat2, 0, 0);
	*bVal = (float)bgr.val[0];
	*gVal = (float)bgr.val[1];
	*rVal = (float)bgr.val[2];

	mm_util_debug("from HSV[%f, %f, %f]", (float)hVal, (float)sVal, (float)vVal);
	mm_util_debug("to BGR[%f, %f, %f]", *bVal, *gVal, *rVal);

	mm_util_debug("Leave _convert_hsv_to_rgb");
}

static int _mm_util_imgcv_calculate_hist(mm_util_imgcv_s *handle, unsigned char *rgb_r, unsigned char *rgb_g, unsigned char *rgb_b)
{
	mm_util_debug("Enter _mm_util_imgcv_calculate_hist");

	int nh = 0;
	int ns = 0;
	int nv = 0;

	int hVal = 0;
	int sVal = 0;
	int vVal = 0;

	float rVal = 0.f;
	float gVal = 0.f;
	float bVal = 0.f;

	unsigned int maxBinVal = 0;
	int max_bin_idx[3] = {-1, -1, -1};

	IplImage *hsvImg = cvCreateImage(cvSize(handle->width, handle->height), IPL_DEPTH_8U, HSV_COLOR_CHANNELS);
	IplImage *hImg = cvCreateImage(cvSize(handle->width, handle->height), IPL_DEPTH_8U, 1);
	IplImage *sImg = cvCreateImage(cvSize(handle->width, handle->height), IPL_DEPTH_8U, 1);
	IplImage *vImg = cvCreateImage(cvSize(handle->width, handle->height), IPL_DEPTH_8U, 1);

	if (!hsvImg || !hImg || !sImg || !vImg) {
		mm_util_error("fail to cvCreateImage()\n");
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	IplImage *planes[] = {hImg, sImg, vImg};

	cvCvtColor(handle->inImg, hsvImg, CV_RGB2HSV);
	cvSplit(hsvImg, hImg, sImg, vImg, NULL);

	float * ranges[] = {handle->hRanges, handle->sRanges, handle->vRanges};
	int histsize[] = {handle->sizeOfHist[0], handle->sizeOfHist[1], handle->sizeOfHist[2]};

	/* create histogram*/
	CvHistogram *hist = cvCreateHist(HISTOGRAM_CHANNELS, histsize, CV_HIST_ARRAY, ranges, 1);
	if (hist == NULL) {
		return MM_UTIL_ERROR_INVALID_OPERATION;
	}

	cvCalcHist(planes, hist, 0, NULL);

	for (nh = 0; nh < (handle->hBins); nh++) {
		for (ns = 0; ns < (handle->sBins); ns++) {
			for (nv = 0; nv < (handle->vBins); nv++) {
				unsigned int binVal = (unsigned int)cvGetReal3D((hist)->bins, nh, ns, nv);
				if (binVal > maxBinVal) {
					maxBinVal = binVal;
					max_bin_idx[0] = nh;
					max_bin_idx[1] = ns;
					max_bin_idx[2] = nv;
				}
			}
		}
	}

	rVal = gVal = bVal = 0.0f;
	hVal = cvRound((max_bin_idx[0]+1)*(handle->hRanges[1]/(float)handle->hBins));
	sVal = cvRound((max_bin_idx[1]+1)*(handle->sRanges[1]/(float)handle->sBins));
	vVal = cvRound((max_bin_idx[2]+1)*(handle->vRanges[1]/(float)handle->vBins));

	_convert_hsv_to_rgb(hVal, sVal, vVal, &rVal, &gVal, &bVal);

	mm_util_debug("nh[%d], ns[%d], nv[%d]\n", max_bin_idx[0], max_bin_idx[1], max_bin_idx[2]);
	mm_util_debug("h[%d], s[%d], v[%d]\n", hVal, sVal, vVal);
	*rgb_r = rVal;
	*rgb_g = gVal;
	*rgb_b = bVal;

	cvReleaseImage(&hsvImg);
	cvReleaseImage(&hImg);
	cvReleaseImage(&sImg);
	cvReleaseImage(&vImg);

	cvReleaseHist(&hist);

	mm_util_debug("Leave _mm_util_imgcv_calculate_hist");

	return MM_UTIL_ERROR_NONE;
}

int mm_util_cv_extract_representative_color(void *image_buffer, int width, int height, unsigned char *r_color, unsigned char *g_color, unsigned char *b_color)
{
	mm_util_debug("Enter mm_util_cv_extract_representative_color");
	if (image_buffer == NULL) {
		mm_util_error("#ERROR#: image buffer is NULL");

		return MM_UTIL_ERROR_INVALID_PARAMETER;
	}

	mm_util_imgcv_s *handle = (mm_util_imgcv_s *)malloc(sizeof(mm_util_imgcv_s));
	if (handle == NULL) {
		mm_util_error("#ERROR#: fail to create handle");

		return MM_UTIL_ERROR_OUT_OF_MEMORY;
	}

	int ret = _mm_util_imgcv_init(handle, width, height);
	if (ret != MM_UTIL_ERROR_NONE) {
		_mm_util_imgcv_uninit(handle);
		free(handle);
		handle = NULL;

		mm_util_error("#ERROR#: Fail to mm_util_imgcv_init: ret=%d", ret);

		return ret;
	}

	ret = _mm_util_imgcv_set_buffer(handle, image_buffer);
	if (ret != MM_UTIL_ERROR_NONE) {
		_mm_util_imgcv_uninit(handle);
		free(handle);
		handle = NULL;

		mm_util_error("#ERROR#: Fail to mm_util_imgcv_set_buffer: ret=%d", ret);

		return ret;
	}

	ret = _mm_util_imgcv_calculate_hist(handle, r_color, g_color, b_color);
	if (ret != MM_UTIL_ERROR_NONE) {
		_mm_util_imgcv_uninit(handle);
		free(handle);
		handle = NULL;

		mm_util_error("#ERROR#: Fail to mm_util_imgcv_calculate_hist: ret=%d", ret);

		return ret;
	}

	_mm_util_imgcv_uninit(handle);

	free(handle);
	handle = NULL;

	mm_util_debug("Leave mm_util_cv_extract_representative_color");

	return MM_UTIL_ERROR_NONE;
}
