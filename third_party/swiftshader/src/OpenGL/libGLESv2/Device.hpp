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

namespace es2
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
		enum : unsigned char
		{
			USE_FILTER = 0x01,
			COLOR_BUFFER = 0x02,
			DEPTH_BUFFER = 0x04,
			STENCIL_BUFFER = 0x08,
			ALL_BUFFERS = COLOR_BUFFER | DEPTH_BUFFER | STENCIL_BUFFER,
		};

		explicit Device(sw::Context *context);

		virtual ~Device();

		void *operator new(size_t size);
		void operator delete(void * mem);

		void clearColor(float red, float green, float blue, float alpha, unsigned int rgbaMask);
		void clearDepth(float z);
		void clearStencil(unsigned int stencil, unsigned int mask);
		void drawIndexedPrimitive(sw::DrawType type, unsigned int indexOffset, unsigned int primitiveCount);
		void drawPrimitive(sw::DrawType type, unsigned int primiveCount);
		void setPixelShader(const sw::PixelShader *shader);
		void setPixelShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count);
		void setScissorEnable(bool enable);
		void setRenderTarget(int index, egl::Image *renderTarget, unsigned int layer);
		void setDepthBuffer(egl::Image *depthBuffer, unsigned int layer);
		void setStencilBuffer(egl::Image *stencilBuffer, unsigned int layer);
		void setScissorRect(const sw::Rect &rect);
		void setVertexShader(const sw::VertexShader *shader);
		void setVertexShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count);
		void setViewport(const Viewport &viewport);

		bool stretchRect(sw::Surface *sourceSurface, const sw::SliceRectF *sourceRect, sw::Surface *destSurface, const sw::SliceRect *destRect, unsigned char flags);
		bool stretchCube(sw::Surface *sourceSurface, sw::Surface *destSurface);
		void finish();

		static void ClipDstRect(sw::RectF &srcRect, sw::Rect &dstRect, sw::Rect &clipRect, bool flipX = false, bool flipY = false);
		static void ClipSrcRect(sw::RectF &srcRect, sw::Rect &dstRect, sw::Rect &clipRect, bool flipX = false, bool flipY = false);

	private:
		sw::Context *const context;

		bool bindResources();
		void bindShaderConstants();
		bool bindViewport();   // Also adjusts for scissoring

		bool validRectangle(const sw::Rect *rect, sw::Surface *surface);
		bool validRectangle(const sw::RectF *rect, sw::Surface *surface);

		void copyBuffer(sw::byte *sourceBuffer, sw::byte *destBuffer, unsigned int width, unsigned int height, unsigned int sourcePitch, unsigned int destPitch, unsigned int bytes, bool flipX, bool flipY);

		Viewport viewport;
		sw::Rect scissorRect;
		bool scissorEnable;

		const sw::PixelShader *pixelShader;
		const sw::VertexShader *vertexShader;

		bool pixelShaderDirty;
		unsigned int pixelShaderConstantsFDirty;
		bool vertexShaderDirty;
		unsigned int vertexShaderConstantsFDirty;

		float pixelShaderConstantF[sw::FRAGMENT_UNIFORM_VECTORS][4];
		float vertexShaderConstantF[sw::VERTEX_UNIFORM_VECTORS][4];

		egl::Image *renderTarget[sw::RENDERTARGETS];
		egl::Image *depthBuffer;
		egl::Image *stencilBuffer;
	};
}

#endif   // gl_Device_hpp
