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

#ifndef	sw_FrameBuffer_hpp
#define	sw_FrameBuffer_hpp

#include "Reactor/Reactor.hpp"
#include "Renderer/Surface.hpp"
#include "Common/Thread.hpp"

namespace sw
{
	class Surface;

	struct BlitState
	{
		int width;
		int height;
		Format destFormat;
		Format sourceFormat;
		int destStride;
		int sourceStride;
		int cursorWidth;
		int cursorHeight;
	};

	class [[clang::lto_visibility_public]] FrameBuffer
	{
	public:
		FrameBuffer(int width, int height, bool fullscreen, bool topLeftOrigin);

		virtual ~FrameBuffer() = 0;

		virtual void flip(sw::Surface *source) = 0;
		virtual void blit(sw::Surface *source, const Rect *sourceRect, const Rect *destRect) = 0;

		virtual void *lock() = 0;
		virtual void unlock() = 0;

		static void setCursorImage(sw::Surface *cursor);
		static void setCursorOrigin(int x0, int y0);
		static void setCursorPosition(int x, int y);

		static Routine *copyRoutine(const BlitState &state);

	protected:
		void copy(sw::Surface *source);

		bool windowed;

		void *framebuffer;   // Native window buffer.
		int width;
		int height;
		int stride;
		Format format;

	private:
		void copyLocked();

		static void threadFunction(void *parameters);

		void *renderbuffer;   // Render target buffer.

		struct Cursor
		{
			void *image;
			int x;
			int y;
			int width;
			int height;
			int hotspotX;
			int hotspotY;
			int positionX;
			int positionY;
		};

		static Cursor cursor;

		void (*blitFunction)(void *dst, void *src, Cursor *cursor);
		Routine *blitRoutine;
		BlitState blitState;     // State of the current blitRoutine.
		BlitState updateState;   // State of the routine to be generated.

		static void blend(const BlitState &state, const Pointer<Byte> &d, const Pointer<Byte> &s, const Pointer<Byte> &c);

		Thread *blitThread;
		Event syncEvent;
		Event blitEvent;
		volatile bool terminate;

		static bool topLeftOrigin;
	};
}

#endif	 //	sw_FrameBuffer_hpp
