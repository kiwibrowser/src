// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "FrameBufferAndroid.hpp"

#include "Common/GrallocAndroid.hpp"

#include <system/window.h>
#include <cutils/log.h>

namespace sw
{
	inline int dequeueBuffer(ANativeWindow* window, ANativeWindowBuffer** buffer)
	{
		#if ANDROID_PLATFORM_SDK_VERSION > 16
			return native_window_dequeue_buffer_and_wait(window, buffer);
		#else
			return window->dequeueBuffer(window, buffer);
		#endif
	}

	inline int queueBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer, int fenceFd)
	{
		#if ANDROID_PLATFORM_SDK_VERSION > 16
			return window->queueBuffer(window, buffer, fenceFd);
		#else
			return window->queueBuffer(window, buffer);
		#endif
	}

	inline int cancelBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer, int fenceFd)
	{
		#if ANDROID_PLATFORM_SDK_VERSION > 16
			return window->cancelBuffer(window, buffer, fenceFd);
		#else
			return window->cancelBuffer(window, buffer);
		#endif
	}

	FrameBufferAndroid::FrameBufferAndroid(ANativeWindow* window, int width, int height)
		: FrameBuffer(width, height, false, false),
		  nativeWindow(window), buffer(nullptr)
	{
		nativeWindow->common.incRef(&nativeWindow->common);
		native_window_set_usage(nativeWindow, GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN);
	}

	FrameBufferAndroid::~FrameBufferAndroid()
	{
		nativeWindow->common.decRef(&nativeWindow->common);
	}

	void FrameBufferAndroid::blit(sw::Surface *source, const Rect *sourceRect, const Rect *destRect)
	{
		copy(source);

		if(buffer)
		{
			if(framebuffer)
			{
				framebuffer = nullptr;
				unlock();
			}

			queueBuffer(nativeWindow, buffer, -1);
		}
	}

	void *FrameBufferAndroid::lock()
	{
		if(dequeueBuffer(nativeWindow, &buffer) != 0)
		{
			return nullptr;
		}

		if(GrallocModule::getInstance()->lock(buffer->handle,
		                 GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN,
		                 0, 0, buffer->width, buffer->height, &framebuffer) != 0)
		{
			ALOGE("%s failed to lock buffer %p", __FUNCTION__, buffer);
			return nullptr;
		}

		if((buffer->width < width) || (buffer->height < height))
		{
			ALOGI("lock failed: buffer of %dx%d too small for window of %dx%d",
				  buffer->width, buffer->height, width, height);
			return nullptr;
		}

		switch(buffer->format)
		{
		case HAL_PIXEL_FORMAT_RGB_565:   format = FORMAT_R5G6B5; break;
		case HAL_PIXEL_FORMAT_RGBA_8888: format = FORMAT_A8B8G8R8; break;
#if ANDROID_PLATFORM_SDK_VERSION > 16
		case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED: format = FORMAT_X8B8G8R8; break;
#endif
		case HAL_PIXEL_FORMAT_RGBX_8888: format = FORMAT_X8B8G8R8; break;
		case HAL_PIXEL_FORMAT_BGRA_8888: format = FORMAT_A8R8G8B8; break;
		case HAL_PIXEL_FORMAT_RGB_888:
			// Frame buffers are expected to have 16-bit or 32-bit colors, not 24-bit.
			ALOGE("Unsupported frame buffer format RGB_888"); ASSERT(false);
			format = FORMAT_R8G8B8;   // Wrong component order.
			break;
		default:
			ALOGE("Unsupported frame buffer format %d", buffer->format); ASSERT(false);
			format = FORMAT_NULL;
			break;
		}

		stride = buffer->stride * Surface::bytes(format);
		return framebuffer;
	}

	void FrameBufferAndroid::unlock()
	{
		if(!buffer)
		{
			ALOGE("%s: badness unlock with no active buffer", __FUNCTION__);
			return;
		}

		framebuffer = nullptr;

		if(GrallocModule::getInstance()->unlock(buffer->handle) != 0)
		{
			ALOGE("%s: badness unlock failed", __FUNCTION__);
		}
	}
}

sw::FrameBuffer *createFrameBuffer(void *display, ANativeWindow* window, int width, int height)
{
	return new sw::FrameBufferAndroid(window, width, height);
}
