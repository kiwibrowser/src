// Copyright 2017 The SwiftShader Authors. All Rights Reserved.
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

#ifndef sw_FrameBufferOzone_hpp
#define sw_FrameBufferOzone_hpp

#include "Main/FrameBuffer.hpp"

namespace sw
{
	class FrameBufferOzone : public FrameBuffer
	{
	public:
		FrameBufferOzone(intptr_t display, intptr_t window, int width, int height);

		~FrameBufferOzone() override;

		void flip(sw::Surface *source) override {blit(source, nullptr, nullptr);};
		void blit(sw::Surface *source, const Rect *sourceRect, const Rect *destRect) override;

		void *lock() override;
		void unlock() override;

	private:
		sw::Surface* buffer;
	};
}

#endif   // sw_FrameBufferOzone_hpp
