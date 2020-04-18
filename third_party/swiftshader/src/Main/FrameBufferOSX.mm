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

#include "FrameBufferOSX.hpp"

#include "Common/Debug.hpp"

#include <EGL/egl.h>
#import <QuartzCore/QuartzCore.h>

namespace sw {

	FrameBufferOSX::FrameBufferOSX(CALayer* layer, int width, int height)
		: FrameBuffer(width, height, false, false), width(width), height(height),
		  layer(layer), buffer(nullptr), provider(nullptr), currentImage(nullptr)
	{
		format = sw::FORMAT_X8B8G8R8;
		int bufferSize = width * height * 4 * sizeof(uint8_t);
		buffer = new uint8_t[bufferSize];
		provider = CGDataProviderCreateWithData(nullptr, buffer, bufferSize, nullptr);
		colorspace = CGColorSpaceCreateDeviceRGB();
	}

	FrameBufferOSX::~FrameBufferOSX()
	{
		//[CATransaction begin];
		//[layer setContents:nullptr];
		//[CATransaction commit];

		CGImageRelease(currentImage);
		CGColorSpaceRelease(colorspace);
		CGDataProviderRelease(provider);

		delete[] buffer;
	}

	void FrameBufferOSX::flip(sw::Surface *source)
	{
		blit(source, nullptr, nullptr);
	}

	void FrameBufferOSX::blit(sw::Surface *source, const Rect *sourceRect, const Rect *destRect)
	{
		copy(source);

		int bytesPerRow = width * 4 * sizeof(uint8_t);
		CGImageRef image = CGImageCreate(width, height, 8, 32, bytesPerRow, colorspace, kCGBitmapByteOrder32Big, provider, nullptr, false, kCGRenderingIntentDefault);

		[CATransaction begin];
		[layer setContents:(id)image];
		[CATransaction commit];
		[CATransaction flush];

		if(currentImage)
		{
			CGImageRelease(currentImage);
		}
		currentImage = image;
	}

	void *FrameBufferOSX::lock()
	{
		stride = width * 4 * sizeof(uint8_t);
		framebuffer = buffer;
		return framebuffer;
	};

	void FrameBufferOSX::unlock()
	{
		framebuffer = nullptr;
	};
}

sw::FrameBuffer *createFrameBuffer(void *display, EGLNativeWindowType nativeWindow, int width, int height)
{
	NSObject *window = reinterpret_cast<NSObject*>(nativeWindow);
	CALayer *layer = nullptr;

	if([window isKindOfClass:[NSView class]])
	{
		NSView *view = reinterpret_cast<NSView*>(window);
		[view setWantsLayer:YES];
		layer = [view layer];
	}
	else if([window isKindOfClass:[CALayer class]])
	{
		layer = reinterpret_cast<CALayer*>(window);
	}
	else ASSERT(0);

	return new sw::FrameBufferOSX(layer, width, height);
}
