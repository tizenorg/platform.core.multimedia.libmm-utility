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
#ifndef __MM_UTILITY_DEBUG_H__
#define __MM_UTILITY_DEBUG_H__

#include <dlog.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "MM_UTIL"

#define FONT_COLOR_RESET    "\033[0m"
#define FONT_COLOR_RED      "\033[31m"

#define mm_util_debug(fmt, arg...) do { \
		LOGD(FONT_COLOR_RESET""fmt"", ##arg);     \
	} while (0)

#define mm_util_info(fmt, arg...) do { \
		LOGI(FONT_COLOR_RESET""fmt"", ##arg);     \
	} while (0)

#define mm_util_warn(fmt, arg...) do { \
		LOGW(FONT_COLOR_RESET""fmt"", ##arg);     \
	} while (0)

#define mm_util_error(fmt, arg...) do { \
		LOGE(FONT_COLOR_RED""fmt""FONT_COLOR_RESET, ##arg);     \
	} while (0)

#define mm_util_fenter() do { \
		LOGD(FONT_COLOR_RESET"<Enter>");     \
	} while (0)

#define mm_util_fleave() do { \
		LOGD(FONT_COLOR_RESET"<Leave>");     \
	} while (0)

#define mm_util_retm_if(expr, fmt, arg...) do { \
		if(expr) { \
			LOGE(FONT_COLOR_RED""fmt""FONT_COLOR_RESET, ##arg);     \
			return; \
		} \
	} while (0)

#define mm_util_retvm_if(expr, val, fmt, arg...) do { \
		if(expr) { \
			LOGE(FONT_COLOR_RED""fmt""FONT_COLOR_RESET, ##arg);     \
			return (val); \
		} \
	} while (0)

/**
* @}
*/

#ifdef __cplusplus
}
#endif

#endif	/*__MM_UTILITY_DEBUG_H__*/
