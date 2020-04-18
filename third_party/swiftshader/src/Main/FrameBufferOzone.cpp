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

#include "FrameBufferOzone.hpp"

namespace sw
{
	FrameBufferOzone::FrameBufferOzone(intptr_t display, intptr_t window, int width, int height) : FrameBuffer(width, height, false, false)
	{
		buffer = sw::Surface::create(width, height, 1, format, nullptr,
		                             sw::Surface::pitchB(width, 0, format, true),
		                             sw::Surface::sliceB(width, height, 0, format, true));
	}

	FrameBufferOzone::~FrameBufferOzone()
	{
		delete buffer;
	}

	void *FrameBufferOzone::lock()
	{
		framebuffer = buffer->lockInternal(0, 0, 0, sw::LOCK_READWRITE, sw::PUBLIC);

		return framebuffer;
	}

	void FrameBufferOzone::unlock()
	{
		buffer->unlockInternal();

		framebuffer = nullptr;
	}

	void FrameBufferOzone::blit(sw::Surface *source, const Rect *sourceRect, const Rect *destRect)
	{
		copy(source);
	}
}

NO_SANITIZE_FUNCTION sw::FrameBuffer *createFrameBuffer(void* display, intptr_t window, int width, int height)
{
	return new sw::FrameBufferOzone((intptr_t)display, window, width, height);
}
