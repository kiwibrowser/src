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

#ifndef gl_Device_hpp
#define gl_Device_hpp

#include "Renderer/Renderer.hpp"

namespace egl
{
	class Image;
}

namespace es1
{
	class Texture;

	struct Viewport
	{
		int x0;
		int y0;
		unsigned int width;
		unsigned int height;
		float minZ;
		float maxZ;
	};

	class Device : public sw::Renderer
	{
	public:
		explicit Device(sw::Context *context);

		virtual ~Device();

		void *operator new(size_t size);
		void operator delete(void * mem);

		void clearColor(float red, float green, float blue, float alpha, unsigned int rgbaMask);
		void clearDepth(float z);
		void clearStencil(unsigned int stencil, unsigned int mask);
		void drawIndexedPrimitive(sw::DrawType type, unsigned int indexOffset, unsigned int primitiveCount);
		void drawPrimitive(sw::DrawType type, unsigned int primiveCount);
		void setScissorEnable(bool enable);
		void setRenderTarget(int index, egl::Image *renderTarget);
		void setDepthBuffer(egl::Image *depthBuffer);
		void setStencilBuffer(egl::Image *stencilBuffer);
		void setScissorRect(const sw::Rect &rect);
		void setViewport(const Viewport &viewport);

		bool stretchRect(sw::Surface *sourceSurface, const sw::SliceRect *sourceRect, sw::Surface *destSurface, const sw::SliceRect *destRect, bool filter);
		void finish();

	private:
		sw::Context *const context;

		bool bindResources();
		bool bindViewport();   // Also adjusts for scissoring

		bool validRectangle(const sw::Rect *rect, sw::Surface *surface);

		Viewport viewport;
		sw::Rect scissorRect;
		bool scissorEnable;

		egl::Image *renderTarget;
		egl::Image *depthBuffer;
		egl::Image *stencilBuffer;
	};
}

#endif   // gl_Device_hpp
