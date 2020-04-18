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

#ifndef sw_FrameBufferOSX_hpp
#define sw_FrameBufferOSX_hpp

#include "Main/FrameBuffer.hpp"

#import <Cocoa/Cocoa.h>

@class CALayer;

namespace sw
{
	class FrameBufferOSX : public FrameBuffer
	{
	public:
		FrameBufferOSX(CALayer *layer, int width, int height);
		~FrameBufferOSX() override;

		void flip(sw::Surface *source) override;
		void blit(sw::Surface *source, const Rect *sourceRect, const Rect *destRect) override;

		void *lock() override;
		void unlock() override;

	private:
		int width;
		int height;
		CALayer *layer;
		uint8_t *buffer;
		CGDataProviderRef provider;
		CGColorSpaceRef colorspace;
		CGImageRef currentImage;
	};
}

#endif   // sw_FrameBufferOSX
