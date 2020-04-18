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

#include "Device.hpp"

#include "Image.hpp"
#include "Texture.h"

#include "Renderer/Renderer.hpp"
#include "Renderer/Clipper.hpp"
#include "Shader/PixelShader.hpp"
#include "Shader/VertexShader.hpp"
#include "Main/Config.hpp"
#include "Main/FrameBuffer.hpp"
#include "Common/Math.hpp"
#include "Common/Configurator.hpp"
#include "Common/Memory.hpp"
#include "Common/Timer.hpp"
#include "../common/debug.h"

namespace gl
{
	using namespace sw;

	Device::Device(Context *context) : Renderer(context, OpenGL, true), context(context)
	{
		depthStencil = 0;
		renderTarget = 0;

		setDepthBufferEnable(true);
		setFillMode(FILL_SOLID);
		setShadingMode(SHADING_GOURAUD);
		setDepthWriteEnable(true);
		setAlphaTestEnable(false);
		setSourceBlendFactor(BLEND_ONE);
		setDestBlendFactor(BLEND_ZERO);
		setCullMode(CULL_COUNTERCLOCKWISE);
		setDepthCompare(DEPTH_LESSEQUAL);
		setAlphaReference(0.0f);
		setAlphaCompare(ALPHA_ALWAYS);
		setAlphaBlendEnable(false);
		setFogEnable(false);
		setSpecularEnable(false);
		setFogColor(0);
		setPixelFogMode(FOG_NONE);
		setFogStart(0.0f);
		setFogEnd(1.0f);
		setFogDensity(1.0f);
		setRangeFogEnable(false);
		setStencilEnable(false);
		setStencilFailOperation(OPERATION_KEEP);
		setStencilZFailOperation(OPERATION_KEEP);
		setStencilPassOperation(OPERATION_KEEP);
		setStencilCompare(STENCIL_ALWAYS);
		setStencilReference(0);
		setStencilMask(0xFFFFFFFF);
		setStencilWriteMask(0xFFFFFFFF);
		setVertexFogMode(FOG_NONE);
		setClipFlags(0);
		setPointSize(1.0f);
		setPointSizeMin(0.125f);
		setPointSizeMax(8192.0f);
		setColorWriteMask(0, 0x0000000F);
		setBlendOperation(BLENDOP_ADD);
		scissorEnable = false;
		setSlopeDepthBias(0.0f);
		setTwoSidedStencil(false);
		setStencilFailOperationCCW(OPERATION_KEEP);
		setStencilZFailOperationCCW(OPERATION_KEEP);
		setStencilPassOperationCCW(OPERATION_KEEP);
		setStencilCompareCCW(STENCIL_ALWAYS);
		setColorWriteMask(1, 0x0000000F);
		setColorWriteMask(2, 0x0000000F);
		setColorWriteMask(3, 0x0000000F);
		setBlendConstant(0xFFFFFFFF);
		setWriteSRGB(false);
		setDepthBias(0.0f);
		setSeparateAlphaBlendEnable(false);
		setSourceBlendFactorAlpha(BLEND_ONE);
		setDestBlendFactorAlpha(BLEND_ZERO);
		setBlendOperationAlpha(BLENDOP_ADD);
		setPointSpriteEnable(true);
		setColorLogicOpEnabled(false);
		setLogicalOperation(LOGICALOP_COPY);

		for(int i = 0; i < 16; i++)
		{
			setAddressingModeU(sw::SAMPLER_PIXEL, i, ADDRESSING_WRAP);
			setAddressingModeV(sw::SAMPLER_PIXEL, i, ADDRESSING_WRAP);
			setAddressingModeW(sw::SAMPLER_PIXEL, i, ADDRESSING_WRAP);
			setBorderColor(sw::SAMPLER_PIXEL, i, 0x00000000);
			setTextureFilter(sw::SAMPLER_PIXEL, i, FILTER_POINT);
			setMipmapFilter(sw::SAMPLER_PIXEL, i, MIPMAP_NONE);
			setMipmapLOD(sw::SAMPLER_PIXEL, i, 0.0f);
		}

		for(int i = 0; i < 4; i++)
		{
			setAddressingModeU(sw::SAMPLER_VERTEX, i, ADDRESSING_WRAP);
			setAddressingModeV(sw::SAMPLER_VERTEX, i, ADDRESSING_WRAP);
			setAddressingModeW(sw::SAMPLER_VERTEX, i, ADDRESSING_WRAP);
			setBorderColor(sw::SAMPLER_VERTEX, i, 0x00000000);
			setTextureFilter(sw::SAMPLER_VERTEX, i, FILTER_POINT);
			setMipmapFilter(sw::SAMPLER_VERTEX, i, MIPMAP_NONE);
			setMipmapLOD(sw::SAMPLER_VERTEX, i, 0.0f);
		}

		for(int i = 0; i < 6; i++)
		{
			float plane[4] = {0, 0, 0, 0};

			setClipPlane(i, plane);
		}

		pixelShader = 0;
		vertexShader = 0;

		pixelShaderDirty = true;
		pixelShaderConstantsFDirty = 0;
		vertexShaderDirty = true;
		vertexShaderConstantsFDirty = 0;

		for(int i = 0; i < FRAGMENT_UNIFORM_VECTORS; i++)
		{
			float zero[4] = {0, 0, 0, 0};

			setPixelShaderConstantF(i, zero, 1);
		}

		for(int i = 0; i < VERTEX_UNIFORM_VECTORS; i++)
		{
			float zero[4] = {0, 0, 0, 0};

			setVertexShaderConstantF(i, zero, 1);
		}

		setLightingEnable(false);

		setGlobalAmbient(sw::Color<float>(0.2f, 0.2f, 0.2f, 1.0f));
		setMaterialAmbient(sw::Color<float>(0.2f, 0.2f, 0.2f, 1.0f));
		setMaterialDiffuse(sw::Color<float>(0.8f, 0.8f, 0.8f, 1.0f));
		setMaterialSpecular(sw::Color<float>(0.0f, 0.0f, 0.0f, 1.0f));
		setMaterialEmission(sw::Color<float>(0.0f, 0.0f, 0.0f, 1.0f));

		for(int i = 0; i < 8; i++)
		{
			setLightEnable(i, false);
			setLightAttenuation(i, 1.0f, 0.0f, 0.0f);
		}

		setDiffuseMaterialSource(sw::MATERIAL_COLOR1);
		setSpecularMaterialSource(sw::MATERIAL_MATERIAL);
		setAmbientMaterialSource(sw::MATERIAL_COLOR1);
		setEmissiveMaterialSource(sw::MATERIAL_MATERIAL);
	}

	Device::~Device()
	{
		if(depthStencil)
		{
			depthStencil->release();
			depthStencil = 0;
		}

		if(renderTarget)
		{
			renderTarget->release();
			renderTarget = 0;
		}

		delete context;
	}

	// This object has to be mem aligned
	void* Device::operator new(size_t size)
	{
		ASSERT(size == sizeof(Device)); // This operator can't be called from a derived class
		return sw::allocate(sizeof(gl::Device), 16);
	}

	void Device::operator delete(void * mem)
	{
		sw::deallocate(mem);
	}

	void Device::clearColor(float red, float green, float blue, float alpha, unsigned int rgbaMask)
	{
		if(!renderTarget || !rgbaMask)
		{
			return;
		}

		sw::Rect clearRect = renderTarget->getRect();

		if(scissorEnable)
		{
			clearRect.clip(scissorRect.x0, scissorRect.y0, scissorRect.x1, scissorRect.y1);
		}

		float rgba[4];
		rgba[0] = red;
		rgba[1] = green;
		rgba[2] = blue;
		rgba[3] = alpha;

		clear(rgba, FORMAT_A32B32G32R32F, renderTarget, clearRect, rgbaMask);
	}

	void Device::clearDepth(float z)
	{
		if(!depthStencil)
		{
			return;
		}

		z = clamp01(z);
		sw::Rect clearRect = depthStencil->getRect();

		if(scissorEnable)
		{
			clearRect.clip(scissorRect.x0, scissorRect.y0, scissorRect.x1, scissorRect.y1);
		}

		depthStencil->clearDepth(z, clearRect.x0, clearRect.y0, clearRect.width(), clearRect.height());
	}

	void Device::clearStencil(unsigned int stencil, unsigned int mask)
	{
		if(!depthStencil)
		{
			return;
		}

		sw::Rect clearRect = depthStencil->getRect();

		if(scissorEnable)
		{
			clearRect.clip(scissorRect.x0, scissorRect.y0, scissorRect.x1, scissorRect.y1);
		}

		depthStencil->clearStencil(stencil, mask, clearRect.x0, clearRect.y0, clearRect.width(), clearRect.height());
	}

	Image *Device::createDepthStencilSurface(unsigned int width, unsigned int height, sw::Format format, int multiSampleDepth, bool discard)
	{
		if(height > OUTLINE_RESOLUTION)
		{
			ERR("Invalid parameters: %dx%d", width, height);
			return 0;
		}

		bool lockable = true;

		switch(format)
		{
	//	case FORMAT_D15S1:
		case FORMAT_D24S8:
		case FORMAT_D24X8:
	//	case FORMAT_D24X4S4:
		case FORMAT_D24FS8:
		case FORMAT_D32:
		case FORMAT_D16:
			lockable = false;
			break;
	//	case FORMAT_S8_LOCKABLE:
	//	case FORMAT_D16_LOCKABLE:
		case FORMAT_D32F_LOCKABLE:
	//	case FORMAT_D32_LOCKABLE:
		case FORMAT_DF24S8:
		case FORMAT_DF16S8:
			lockable = true;
			break;
		default:
			UNREACHABLE(format);
		}

		Image *surface = new Image(0, width, height, format, multiSampleDepth, lockable, true);

		if(!surface)
		{
			ERR("Out of memory");
			return 0;
		}

		return surface;
	}

	Image *Device::createRenderTarget(unsigned int width, unsigned int height, sw::Format format, int multiSampleDepth, bool lockable)
	{
		if(height > OUTLINE_RESOLUTION)
		{
			ERR("Invalid parameters: %dx%d", width, height);
			return 0;
		}

		Image *surface = new Image(0, width, height, format, multiSampleDepth, lockable, true);

		if(!surface)
		{
			ERR("Out of memory");
			return 0;
		}

		return surface;
	}

	void Device::drawIndexedPrimitive(PrimitiveType type, unsigned int indexOffset, unsigned int primitiveCount, int indexSize)
	{
		if(!bindResources() || !primitiveCount)
		{
			return;
		}

		DrawType drawType;

		if(indexSize == 4)
		{
			switch(type)
			{
			case DRAW_POINTLIST:     drawType = sw::DRAW_INDEXEDPOINTLIST32;     break;
			case DRAW_LINELIST:      drawType = sw::DRAW_INDEXEDLINELIST32;      break;
			case DRAW_LINESTRIP:     drawType = sw::DRAW_INDEXEDLINESTRIP32;     break;
			case DRAW_LINELOOP:      drawType = sw::DRAW_INDEXEDLINELOOP32;      break;
			case DRAW_TRIANGLELIST:  drawType = sw::DRAW_INDEXEDTRIANGLELIST32;  break;
			case DRAW_TRIANGLESTRIP: drawType = sw::DRAW_INDEXEDTRIANGLESTRIP32; break;
			case DRAW_TRIANGLEFAN:   drawType = sw::DRAW_INDEXEDTRIANGLEFAN32;	  break;
			default: UNREACHABLE(type);
			}
		}
		else if(indexSize == 2)
		{
			switch(type)
			{
			case DRAW_POINTLIST:     drawType = sw::DRAW_INDEXEDPOINTLIST16;     break;
			case DRAW_LINELIST:      drawType = sw::DRAW_INDEXEDLINELIST16;      break;
			case DRAW_LINESTRIP:     drawType = sw::DRAW_INDEXEDLINESTRIP16;     break;
			case DRAW_LINELOOP:      drawType = sw::DRAW_INDEXEDLINELOOP16;      break;
			case DRAW_TRIANGLELIST:  drawType = sw::DRAW_INDEXEDTRIANGLELIST16;  break;
			case DRAW_TRIANGLESTRIP: drawType = sw::DRAW_INDEXEDTRIANGLESTRIP16; break;
			case DRAW_TRIANGLEFAN:   drawType = sw::DRAW_INDEXEDTRIANGLEFAN16;   break;
			default: UNREACHABLE(type);
			}
		}
		else if(indexSize == 1)
		{
			switch(type)
			{
			case DRAW_POINTLIST:     drawType = sw::DRAW_INDEXEDPOINTLIST8;     break;
			case DRAW_LINELIST:      drawType = sw::DRAW_INDEXEDLINELIST8;      break;
			case DRAW_LINESTRIP:     drawType = sw::DRAW_INDEXEDLINESTRIP8;     break;
			case DRAW_LINELOOP:      drawType = sw::DRAW_INDEXEDLINELOOP8;      break;
			case DRAW_TRIANGLELIST:  drawType = sw::DRAW_INDEXEDTRIANGLELIST8;  break;
			case DRAW_TRIANGLESTRIP: drawType = sw::DRAW_INDEXEDTRIANGLESTRIP8; break;
			case DRAW_TRIANGLEFAN:   drawType = sw::DRAW_INDEXEDTRIANGLEFAN8;   break;
			default: UNREACHABLE(type);
			}
		}
		else UNREACHABLE(indexSize);

		draw(drawType, indexOffset, primitiveCount);
	}

	void Device::drawPrimitive(PrimitiveType primitiveType, unsigned int primitiveCount)
	{
		if(!bindResources() || !primitiveCount)
		{
			return;
		}

		setIndexBuffer(0);

		DrawType drawType;

		switch(primitiveType)
		{
		case DRAW_POINTLIST:     drawType = sw::DRAW_POINTLIST;     break;
		case DRAW_LINELIST:      drawType = sw::DRAW_LINELIST;      break;
		case DRAW_LINESTRIP:     drawType = sw::DRAW_LINESTRIP;     break;
		case DRAW_LINELOOP:      drawType = sw::DRAW_LINELOOP;      break;
		case DRAW_TRIANGLELIST:  drawType = sw::DRAW_TRIANGLELIST;  break;
		case DRAW_TRIANGLESTRIP: drawType = sw::DRAW_TRIANGLESTRIP; break;
		case DRAW_TRIANGLEFAN:   drawType = sw::DRAW_TRIANGLEFAN;   break;
		case DRAW_QUADLIST:      drawType = sw::DRAW_QUADLIST;      break;
		default: UNREACHABLE(primitiveType);
		}

		draw(drawType, 0, primitiveCount);
	}

	void Device::setDepthStencilSurface(Image *depthStencil)
	{
		if(this->depthStencil == depthStencil)
		{
			return;
		}

		if(depthStencil)
		{
			depthStencil->addRef();
		}

		if(this->depthStencil)
		{
			this->depthStencil->release();
		}

		this->depthStencil = depthStencil;

		setDepthBuffer(depthStencil);
		setStencilBuffer(depthStencil);
	}

	void Device::setPixelShader(PixelShader *pixelShader)
	{
		this->pixelShader = pixelShader;
		pixelShaderDirty = true;
	}

	void Device::setPixelShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count)
	{
		for(unsigned int i = 0; i < count && startRegister + i < FRAGMENT_UNIFORM_VECTORS; i++)
		{
			pixelShaderConstantF[startRegister + i][0] = constantData[i * 4 + 0];
			pixelShaderConstantF[startRegister + i][1] = constantData[i * 4 + 1];
			pixelShaderConstantF[startRegister + i][2] = constantData[i * 4 + 2];
			pixelShaderConstantF[startRegister + i][3] = constantData[i * 4 + 3];
		}

		pixelShaderConstantsFDirty = max(startRegister + count, pixelShaderConstantsFDirty);
		pixelShaderDirty = true;   // Reload DEF constants
	}

	void Device::setScissorEnable(bool enable)
	{
		scissorEnable = enable;
	}

	void Device::setRenderTarget(int index, Image *renderTarget)
	{
		if(renderTarget)
		{
			renderTarget->addRef();
		}

		if(this->renderTarget)
		{
			this->renderTarget->release();
		}

		this->renderTarget = renderTarget;

		Renderer::setRenderTarget(index, renderTarget);
	}

	void Device::setScissorRect(const sw::Rect &rect)
	{
		scissorRect = rect;
	}

	void Device::setVertexShader(VertexShader *vertexShader)
	{
		this->vertexShader = vertexShader;
		vertexShaderDirty = true;
	}

	void Device::setVertexShaderConstantF(unsigned int startRegister, const float *constantData, unsigned int count)
	{
		for(unsigned int i = 0; i < count && startRegister + i < VERTEX_UNIFORM_VECTORS; i++)
		{
			vertexShaderConstantF[startRegister + i][0] = constantData[i * 4 + 0];
			vertexShaderConstantF[startRegister + i][1] = constantData[i * 4 + 1];
			vertexShaderConstantF[startRegister + i][2] = constantData[i * 4 + 2];
			vertexShaderConstantF[startRegister + i][3] = constantData[i * 4 + 3];
		}

		vertexShaderConstantsFDirty = max(startRegister + count, vertexShaderConstantsFDirty);
		vertexShaderDirty = true;   // Reload DEF constants
	}

	void Device::setViewport(const Viewport &viewport)
	{
		this->viewport = viewport;
	}

	bool Device::stretchRect(Image *source, const sw::SliceRect *sourceRect, Image *dest, const sw::SliceRect *destRect, bool filter)
	{
		if(!source || !dest || !validRectangle(sourceRect, source) || !validRectangle(destRect, dest))
		{
			ERR("Invalid parameters");
			return false;
		}

		int sWidth = source->getWidth();
		int sHeight = source->getHeight();
		int dWidth = dest->getWidth();
		int dHeight = dest->getHeight();

		SliceRect sRect;
		SliceRect dRect;

		if(sourceRect)
		{
			sRect = *sourceRect;
		}
		else
		{
			sRect.y0 = 0;
			sRect.x0 = 0;
			sRect.y1 = sHeight;
			sRect.x1 = sWidth;
		}

		if(destRect)
		{
			dRect = *destRect;
		}
		else
		{
			dRect.y0 = 0;
			dRect.x0 = 0;
			dRect.y1 = dHeight;
			dRect.x1 = dWidth;
		}

		bool scaling = (sRect.x1 - sRect.x0 != dRect.x1 - dRect.x0) || (sRect.y1 - sRect.y0 != dRect.y1 - dRect.y0);
		bool equalFormats = source->getInternalFormat() == dest->getInternalFormat();
		bool depthStencil = Image::isDepth(source->getInternalFormat()) || Image::isStencil(source->getInternalFormat());
		bool alpha0xFF = false;

		if((source->getInternalFormat() == FORMAT_A8R8G8B8 && dest->getInternalFormat() == FORMAT_X8R8G8B8) ||
		   (source->getInternalFormat() == FORMAT_X8R8G8B8 && dest->getInternalFormat() == FORMAT_A8R8G8B8))
		{
			equalFormats = true;
			alpha0xFF = true;
		}

		if(depthStencil)   // Copy entirely, internally   // FIXME: Check
		{
			if(source->hasDepth())
			{
				sw::byte *sourceBuffer = (sw::byte*)source->lockInternal(0, 0, sRect.slice, LOCK_READONLY, PUBLIC);
				sw::byte *destBuffer = (sw::byte*)dest->lockInternal(0, 0, dRect.slice, LOCK_DISCARD, PUBLIC);

				unsigned int width = source->getWidth();
				unsigned int height = source->getHeight();
				unsigned int pitch = source->getInternalPitchB();

				for(unsigned int y = 0; y < height; y++)
				{
					memcpy(destBuffer, sourceBuffer, pitch);   // FIXME: Only copy width * bytes

					sourceBuffer += pitch;
					destBuffer += pitch;
				}

				source->unlockInternal();
				dest->unlockInternal();
			}

			if(source->hasStencil())
			{
				sw::byte *sourceBuffer = (sw::byte*)source->lockStencil(0, 0, 0, PUBLIC);
				sw::byte *destBuffer = (sw::byte*)dest->lockStencil(0, 0, 0, PUBLIC);

				unsigned int width = source->getWidth();
				unsigned int height = source->getHeight();
				unsigned int pitch = source->getStencilPitchB();

				for(unsigned int y = 0; y < height; y++)
				{
					memcpy(destBuffer, sourceBuffer, pitch);   // FIXME: Only copy width * bytes

					sourceBuffer += pitch;
					destBuffer += pitch;
				}

				source->unlockStencil();
				dest->unlockStencil();
			}
		}
		else if(!scaling && equalFormats)
		{
			unsigned char *sourceBytes = (unsigned char*)source->lockInternal(sRect.x0, sRect.y0, sRect.slice, LOCK_READONLY, PUBLIC);
			unsigned char *destBytes = (unsigned char*)dest->lockInternal(dRect.x0, dRect.y0, dRect.slice, LOCK_READWRITE, PUBLIC);
			unsigned int sourcePitch = source->getInternalPitchB();
			unsigned int destPitch = dest->getInternalPitchB();

			unsigned int width = dRect.x1 - dRect.x0;
			unsigned int height = dRect.y1 - dRect.y0;
			unsigned int bytes = width * Image::bytes(source->getInternalFormat());

			for(unsigned int y = 0; y < height; y++)
			{
				memcpy(destBytes, sourceBytes, bytes);

				if(alpha0xFF)
				{
					for(unsigned int x = 0; x < width; x++)
					{
						destBytes[4 * x + 3] = 0xFF;
					}
				}

				sourceBytes += sourcePitch;
				destBytes += destPitch;
			}

			source->unlockInternal();
			dest->unlockInternal();
		}
		else
		{
			sw::SliceRectF sRectF((float)sRect.x0, (float)sRect.y0, (float)sRect.x1, (float)sRect.y1, sRect.slice);
			blit(source, sRectF, dest, dRect, scaling && filter);
		}

		return true;
	}

	bool Device::bindResources()
	{
		if(!bindViewport())
		{
			return false;   // Zero-area target region
		}

		bindShaderConstants();

		return true;
	}

	void Device::bindShaderConstants()
	{
		if(pixelShaderDirty)
		{
			if(pixelShader)
			{
				if(pixelShaderConstantsFDirty)
				{
					Renderer::setPixelShaderConstantF(0, pixelShaderConstantF[0], pixelShaderConstantsFDirty);
				}

				Renderer::setPixelShader(pixelShader);   // Loads shader constants set with DEF
				pixelShaderConstantsFDirty = pixelShader->dirtyConstantsF;   // Shader DEF'ed constants are dirty
			}
			else
			{
				setPixelShader(0);
			}

			pixelShaderDirty = false;
		}

		if(vertexShaderDirty)
		{
			if(vertexShader)
			{
				if(vertexShaderConstantsFDirty)
				{
					Renderer::setVertexShaderConstantF(0, vertexShaderConstantF[0], vertexShaderConstantsFDirty);
				}

				Renderer::setVertexShader(vertexShader);   // Loads shader constants set with DEF
				vertexShaderConstantsFDirty = vertexShader->dirtyConstantsF;   // Shader DEF'ed constants are dirty
			}
			else
			{
				setVertexShader(0);
			}

			vertexShaderDirty = false;
		}
	}

	bool Device::bindViewport()
	{
		if(viewport.width <= 0 || viewport.height <= 0)
		{
			return false;
		}

		if(scissorEnable)
		{
			if(scissorRect.x0 >= scissorRect.x1 || scissorRect.y0 >= scissorRect.y1)
			{
				return false;
			}

			sw::Rect scissor;
			scissor.x0 = scissorRect.x0;
			scissor.x1 = scissorRect.x1;
			scissor.y0 = scissorRect.y0;
			scissor.y1 = scissorRect.y1;

			setScissor(scissor);
		}
		else
		{
			sw::Rect scissor;
			scissor.x0 = viewport.x0;
			scissor.x1 = viewport.x0 + viewport.width;
			scissor.y0 = viewport.y0;
			scissor.y1 = viewport.y0 + viewport.height;

			if(renderTarget)
			{
				scissor.x0 = max(scissor.x0, 0);
				scissor.x1 = min(scissor.x1, renderTarget->getWidth());
				scissor.y0 = max(scissor.y0, 0);
				scissor.y1 = min(scissor.y1, renderTarget->getHeight());
			}

			if(depthStencil)
			{
				scissor.x0 = max(scissor.x0, 0);
				scissor.x1 = min(scissor.x1, depthStencil->getWidth());
				scissor.y0 = max(scissor.y0, 0);
				scissor.y1 = min(scissor.y1, depthStencil->getHeight());
			}

			setScissor(scissor);
		}

		sw::Viewport view;
		view.x0 = (float)viewport.x0;
		view.y0 = (float)viewport.y0;
		view.width = (float)viewport.width;
		view.height = (float)viewport.height;
		view.minZ = viewport.minZ;
		view.maxZ = viewport.maxZ;

		Renderer::setViewport(view);

		return true;
	}

	bool Device::validRectangle(const sw::Rect *rect, Image *surface)
	{
		if(!rect)
		{
			return true;
		}

		if(rect->x1 <= rect->x0 || rect->y1 <= rect->y0)
		{
			return false;
		}

		if(rect->x0 < 0 || rect->y0 < 0)
		{
			return false;
		}

		if(rect->x1 > (int)surface->getWidth() || rect->y1 > (int)surface->getHeight())
		{
			return false;
		}

		return true;
	}

	void Device::finish()
	{
		synchronize();
	}
}
