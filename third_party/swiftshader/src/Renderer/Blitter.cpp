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

#include "Blitter.hpp"

#include "Shader/ShaderCore.hpp"
#include "Reactor/Reactor.hpp"
#include "Common/Memory.hpp"
#include "Common/Debug.hpp"

namespace sw
{
	Blitter::Blitter()
	{
		blitCache = new RoutineCache<State>(1024);
	}

	Blitter::~Blitter()
	{
		delete blitCache;
	}

	void Blitter::clear(void *pixel, sw::Format format, Surface *dest, const SliceRect &dRect, unsigned int rgbaMask)
	{
		if(fastClear(pixel, format, dest, dRect, rgbaMask))
		{
			return;
		}

		sw::Surface *color = sw::Surface::create(1, 1, 1, format, pixel, sw::Surface::bytes(format), sw::Surface::bytes(format));
		SliceRectF sRect((float)dRect.x0, (float)dRect.y0, (float)dRect.x1, (float)dRect.y1, 0);
		blit(color, sRect, dest, dRect, {rgbaMask});
		delete color;
	}

	bool Blitter::fastClear(void *pixel, sw::Format format, Surface *dest, const SliceRect &dRect, unsigned int rgbaMask)
	{
		if(format != FORMAT_A32B32G32R32F)
		{
			return false;
		}

		float *color = (float*)pixel;
		float r = color[0];
		float g = color[1];
		float b = color[2];
		float a = color[3];

		uint32_t packed;

		switch(dest->getFormat())
		{
		case FORMAT_R5G6B5:
			if((rgbaMask & 0x7) != 0x7) return false;
			packed = ((uint16_t)(31 * b + 0.5f) << 0) |
			         ((uint16_t)(63 * g + 0.5f) << 5) |
			         ((uint16_t)(31 * r + 0.5f) << 11);
			break;
		case FORMAT_X8B8G8R8:
			if((rgbaMask & 0x7) != 0x7) return false;
			packed = ((uint32_t)(255) << 24) |
			         ((uint32_t)(255 * b + 0.5f) << 16) |
			         ((uint32_t)(255 * g + 0.5f) << 8) |
			         ((uint32_t)(255 * r + 0.5f) << 0);
			break;
		case FORMAT_A8B8G8R8:
			if((rgbaMask & 0xF) != 0xF) return false;
			packed = ((uint32_t)(255 * a + 0.5f) << 24) |
			         ((uint32_t)(255 * b + 0.5f) << 16) |
			         ((uint32_t)(255 * g + 0.5f) << 8) |
			         ((uint32_t)(255 * r + 0.5f) << 0);
			break;
		case FORMAT_X8R8G8B8:
			if((rgbaMask & 0x7) != 0x7) return false;
			packed = ((uint32_t)(255) << 24) |
			         ((uint32_t)(255 * r + 0.5f) << 16) |
			         ((uint32_t)(255 * g + 0.5f) << 8) |
			         ((uint32_t)(255 * b + 0.5f) << 0);
			break;
		case FORMAT_A8R8G8B8:
			if((rgbaMask & 0xF) != 0xF) return false;
			packed = ((uint32_t)(255 * a + 0.5f) << 24) |
			         ((uint32_t)(255 * r + 0.5f) << 16) |
			         ((uint32_t)(255 * g + 0.5f) << 8) |
			         ((uint32_t)(255 * b + 0.5f) << 0);
			break;
		default:
			return false;
		}

		bool useDestInternal = !dest->isExternalDirty();
		uint8_t *slice = (uint8_t*)dest->lock(dRect.x0, dRect.y0, dRect.slice, sw::LOCK_WRITEONLY, sw::PUBLIC, useDestInternal);

		for(int j = 0; j < dest->getSamples(); j++)
		{
			uint8_t *d = slice;

			switch(Surface::bytes(dest->getFormat()))
			{
			case 2:
				for(int i = dRect.y0; i < dRect.y1; i++)
				{
					sw::clear((uint16_t*)d, packed, dRect.x1 - dRect.x0);
					d += dest->getPitchB(useDestInternal);
				}
				break;
			case 4:
				for(int i = dRect.y0; i < dRect.y1; i++)
				{
					sw::clear((uint32_t*)d, packed, dRect.x1 - dRect.x0);
					d += dest->getPitchB(useDestInternal);
				}
				break;
			default:
				assert(false);
			}

			slice += dest->getSliceB(useDestInternal);
		}

		dest->unlock(useDestInternal);

		return true;
	}

	void Blitter::blit(Surface *source, const SliceRectF &sourceRect, Surface *dest, const SliceRect &destRect, const Blitter::Options& options)
	{
		if(dest->getInternalFormat() == FORMAT_NULL)
		{
			return;
		}

		if(blitReactor(source, sourceRect, dest, destRect, options))
		{
			return;
		}

		SliceRectF sRect = sourceRect;
		SliceRect dRect = destRect;

		bool flipX = destRect.x0 > destRect.x1;
		bool flipY = destRect.y0 > destRect.y1;

		if(flipX)
		{
			swap(dRect.x0, dRect.x1);
			swap(sRect.x0, sRect.x1);
		}
		if(flipY)
		{
			swap(dRect.y0, dRect.y1);
			swap(sRect.y0, sRect.y1);
		}

		source->lockInternal((int)sRect.x0, (int)sRect.y0, sRect.slice, sw::LOCK_READONLY, sw::PUBLIC);
		dest->lockInternal(dRect.x0, dRect.y0, dRect.slice, sw::LOCK_WRITEONLY, sw::PUBLIC);

		float w = sRect.width() / dRect.width();
		float h = sRect.height() / dRect.height();

		const float xStart = sRect.x0 + 0.5f * w;
		float y = sRect.y0 + 0.5f * h;

		for(int j = dRect.y0; j < dRect.y1; j++)
		{
			float x = xStart;

			for(int i = dRect.x0; i < dRect.x1; i++)
			{
				// FIXME: Support RGBA mask
				dest->copyInternal(source, i, j, x, y, options.filter);

				x += w;
			}

			y += h;
		}

		source->unlockInternal();
		dest->unlockInternal();
	}

	void Blitter::blit3D(Surface *source, Surface *dest)
	{
		source->lockInternal(0, 0, 0, sw::LOCK_READONLY, sw::PUBLIC);
		dest->lockInternal(0, 0, 0, sw::LOCK_WRITEONLY, sw::PUBLIC);

		float w = static_cast<float>(source->getWidth())  / static_cast<float>(dest->getWidth());
		float h = static_cast<float>(source->getHeight()) / static_cast<float>(dest->getHeight());
		float d = static_cast<float>(source->getDepth())  / static_cast<float>(dest->getDepth());

		float z = 0.5f * d;
		for(int k = 0; k < dest->getDepth(); k++)
		{
			float y = 0.5f * h;
			for(int j = 0; j < dest->getHeight(); j++)
			{
				float x = 0.5f * w;
				for(int i = 0; i < dest->getWidth(); i++)
				{
					dest->copyInternal(source, i, j, k, x, y, z, true);
					x += w;
				}
				y += h;
			}
			z += d;
		}

		source->unlockInternal();
		dest->unlockInternal();
	}

	bool Blitter::read(Float4 &c, Pointer<Byte> element, const State &state)
	{
		c = Float4(0.0f, 0.0f, 0.0f, 1.0f);

		switch(state.sourceFormat)
		{
		case FORMAT_L8:
			c.xyz = Float(Int(*Pointer<Byte>(element)));
			c.w = float(0xFF);
			break;
		case FORMAT_A8:
			c.w = Float(Int(*Pointer<Byte>(element)));
			break;
		case FORMAT_R8I:
		case FORMAT_R8_SNORM:
			c.x = Float(Int(*Pointer<SByte>(element)));
			c.w = float(0x7F);
			break;
		case FORMAT_R8:
		case FORMAT_R8UI:
			c.x = Float(Int(*Pointer<Byte>(element)));
			c.w = float(0xFF);
			break;
		case FORMAT_R16I:
			c.x = Float(Int(*Pointer<Short>(element)));
			c.w = float(0x7FFF);
			break;
		case FORMAT_R16UI:
			c.x = Float(Int(*Pointer<UShort>(element)));
			c.w = float(0xFFFF);
			break;
		case FORMAT_R32I:
			c.x = Float(*Pointer<Int>(element));
			c.w = float(0x7FFFFFFF);
			break;
		case FORMAT_R32UI:
			c.x = Float(*Pointer<UInt>(element));
			c.w = float(0xFFFFFFFF);
			break;
		case FORMAT_A8R8G8B8:
			c = Float4(*Pointer<Byte4>(element)).zyxw;
			break;
		case FORMAT_A8B8G8R8I:
		case FORMAT_A8B8G8R8_SNORM:
			c = Float4(*Pointer<SByte4>(element));
			break;
		case FORMAT_A8B8G8R8:
		case FORMAT_A8B8G8R8UI:
		case FORMAT_SRGB8_A8:
			c = Float4(*Pointer<Byte4>(element));
			break;
		case FORMAT_X8R8G8B8:
			c = Float4(*Pointer<Byte4>(element)).zyxw;
			c.w = float(0xFF);
			break;
		case FORMAT_R8G8B8:
			c.z = Float(Int(*Pointer<Byte>(element + 0)));
			c.y = Float(Int(*Pointer<Byte>(element + 1)));
			c.x = Float(Int(*Pointer<Byte>(element + 2)));
			c.w = float(0xFF);
			break;
		case FORMAT_B8G8R8:
			c.x = Float(Int(*Pointer<Byte>(element + 0)));
			c.y = Float(Int(*Pointer<Byte>(element + 1)));
			c.z = Float(Int(*Pointer<Byte>(element + 2)));
			c.w = float(0xFF);
			break;
		case FORMAT_X8B8G8R8I:
		case FORMAT_X8B8G8R8_SNORM:
			c = Float4(*Pointer<SByte4>(element));
			c.w = float(0x7F);
			break;
		case FORMAT_X8B8G8R8:
		case FORMAT_X8B8G8R8UI:
		case FORMAT_SRGB8_X8:
			c = Float4(*Pointer<Byte4>(element));
			c.w = float(0xFF);
			break;
		case FORMAT_A16B16G16R16I:
			c = Float4(*Pointer<Short4>(element));
			break;
		case FORMAT_A16B16G16R16:
		case FORMAT_A16B16G16R16UI:
			c = Float4(*Pointer<UShort4>(element));
			break;
		case FORMAT_X16B16G16R16I:
			c = Float4(*Pointer<Short4>(element));
			c.w = float(0x7FFF);
			break;
		case FORMAT_X16B16G16R16UI:
			c = Float4(*Pointer<UShort4>(element));
			c.w = float(0xFFFF);
			break;
		case FORMAT_A32B32G32R32I:
			c = Float4(*Pointer<Int4>(element));
			break;
		case FORMAT_A32B32G32R32UI:
			c = Float4(*Pointer<UInt4>(element));
			break;
		case FORMAT_X32B32G32R32I:
			c = Float4(*Pointer<Int4>(element));
			c.w = float(0x7FFFFFFF);
			break;
		case FORMAT_X32B32G32R32UI:
			c = Float4(*Pointer<UInt4>(element));
			c.w = float(0xFFFFFFFF);
			break;
		case FORMAT_G8R8I:
		case FORMAT_G8R8_SNORM:
			c.x = Float(Int(*Pointer<SByte>(element + 0)));
			c.y = Float(Int(*Pointer<SByte>(element + 1)));
			c.w = float(0x7F);
			break;
		case FORMAT_G8R8:
		case FORMAT_G8R8UI:
			c.x = Float(Int(*Pointer<Byte>(element + 0)));
			c.y = Float(Int(*Pointer<Byte>(element + 1)));
			c.w = float(0xFF);
			break;
		case FORMAT_G16R16I:
			c.x = Float(Int(*Pointer<Short>(element + 0)));
			c.y = Float(Int(*Pointer<Short>(element + 2)));
			c.w = float(0x7FFF);
			break;
		case FORMAT_G16R16:
		case FORMAT_G16R16UI:
			c.x = Float(Int(*Pointer<UShort>(element + 0)));
			c.y = Float(Int(*Pointer<UShort>(element + 2)));
			c.w = float(0xFFFF);
			break;
		case FORMAT_G32R32I:
			c.x = Float(*Pointer<Int>(element + 0));
			c.y = Float(*Pointer<Int>(element + 4));
			c.w = float(0x7FFFFFFF);
			break;
		case FORMAT_G32R32UI:
			c.x = Float(*Pointer<UInt>(element + 0));
			c.y = Float(*Pointer<UInt>(element + 4));
			c.w = float(0xFFFFFFFF);
			break;
		case FORMAT_A32B32G32R32F:
			c = *Pointer<Float4>(element);
			break;
		case FORMAT_X32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_B32G32R32F:
			c.z = *Pointer<Float>(element + 8);
		case FORMAT_G32R32F:
			c.x = *Pointer<Float>(element + 0);
			c.y = *Pointer<Float>(element + 4);
			break;
		case FORMAT_R32F:
			c.x = *Pointer<Float>(element);
			break;
		case FORMAT_R5G6B5:
			c.x = Float(Int((*Pointer<UShort>(element) & UShort(0xF800)) >> UShort(11)));
			c.y = Float(Int((*Pointer<UShort>(element) & UShort(0x07E0)) >> UShort(5)));
			c.z = Float(Int(*Pointer<UShort>(element) & UShort(0x001F)));
			break;
		case FORMAT_A2B10G10R10:
		case FORMAT_A2B10G10R10UI:
			c.x = Float(Int((*Pointer<UInt>(element) & UInt(0x000003FF))));
			c.y = Float(Int((*Pointer<UInt>(element) & UInt(0x000FFC00)) >> 10));
			c.z = Float(Int((*Pointer<UInt>(element) & UInt(0x3FF00000)) >> 20));
			c.w = Float(Int((*Pointer<UInt>(element) & UInt(0xC0000000)) >> 30));
			break;
		case FORMAT_D16:
			c.x = Float(Int((*Pointer<UShort>(element))));
			break;
		case FORMAT_D24S8:
			c.x = Float(Int((*Pointer<UInt>(element))));
			break;
		case FORMAT_D32:
			c.x = Float(Int((*Pointer<UInt>(element))));
			break;
		case FORMAT_D32F_COMPLEMENTARY:
		case FORMAT_D32FS8_COMPLEMENTARY:
			c.x = 1.0f - *Pointer<Float>(element);
			break;
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
			c.x = *Pointer<Float>(element);
			break;
		case FORMAT_S8:
			c.x = Float(Int(*Pointer<Byte>(element)));
			break;
		default:
			return false;
		}

		return true;
	}

	bool Blitter::write(Float4 &c, Pointer<Byte> element, const State &state)
	{
		bool writeR = state.writeRed;
		bool writeG = state.writeGreen;
		bool writeB = state.writeBlue;
		bool writeA = state.writeAlpha;
		bool writeRGBA = writeR && writeG && writeB && writeA;

		switch(state.destFormat)
		{
		case FORMAT_L8:
			*Pointer<Byte>(element) = Byte(RoundInt(Float(c.x)));
			break;
		case FORMAT_A8:
			if(writeA) { *Pointer<Byte>(element) = Byte(RoundInt(Float(c.w))); }
			break;
		case FORMAT_A8R8G8B8:
			if(writeRGBA)
			{
				Short4 c0 = RoundShort4(c.zyxw);
				*Pointer<Byte4>(element) = Byte4(PackUnsigned(c0, c0));
			}
			else
			{
				if(writeB) { *Pointer<Byte>(element + 0) = Byte(RoundInt(Float(c.z))); }
				if(writeG) { *Pointer<Byte>(element + 1) = Byte(RoundInt(Float(c.y))); }
				if(writeR) { *Pointer<Byte>(element + 2) = Byte(RoundInt(Float(c.x))); }
				if(writeA) { *Pointer<Byte>(element + 3) = Byte(RoundInt(Float(c.w))); }
			}
			break;
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_A8:
			if(writeRGBA)
			{
				Short4 c0 = RoundShort4(c);
				*Pointer<Byte4>(element) = Byte4(PackUnsigned(c0, c0));
			}
			else
			{
				if(writeR) { *Pointer<Byte>(element + 0) = Byte(RoundInt(Float(c.x))); }
				if(writeG) { *Pointer<Byte>(element + 1) = Byte(RoundInt(Float(c.y))); }
				if(writeB) { *Pointer<Byte>(element + 2) = Byte(RoundInt(Float(c.z))); }
				if(writeA) { *Pointer<Byte>(element + 3) = Byte(RoundInt(Float(c.w))); }
			}
			break;
		case FORMAT_X8R8G8B8:
			if(writeRGBA)
			{
				Short4 c0 = RoundShort4(c.zyxw) | Short4(0x0000, 0x0000, 0x0000, 0x00FF);
				*Pointer<Byte4>(element) = Byte4(PackUnsigned(c0, c0));
			}
			else
			{
				if(writeB) { *Pointer<Byte>(element + 0) = Byte(RoundInt(Float(c.z))); }
				if(writeG) { *Pointer<Byte>(element + 1) = Byte(RoundInt(Float(c.y))); }
				if(writeR) { *Pointer<Byte>(element + 2) = Byte(RoundInt(Float(c.x))); }
				if(writeA) { *Pointer<Byte>(element + 3) = Byte(0xFF); }
			}
			break;
		case FORMAT_X8B8G8R8:
		case FORMAT_SRGB8_X8:
			if(writeRGBA)
			{
				Short4 c0 = RoundShort4(c) | Short4(0x0000, 0x0000, 0x0000, 0x00FF);
				*Pointer<Byte4>(element) = Byte4(PackUnsigned(c0, c0));
			}
			else
			{
				if(writeR) { *Pointer<Byte>(element + 0) = Byte(RoundInt(Float(c.x))); }
				if(writeG) { *Pointer<Byte>(element + 1) = Byte(RoundInt(Float(c.y))); }
				if(writeB) { *Pointer<Byte>(element + 2) = Byte(RoundInt(Float(c.z))); }
				if(writeA) { *Pointer<Byte>(element + 3) = Byte(0xFF); }
			}
			break;
		case FORMAT_R8G8B8:
			if(writeR) { *Pointer<Byte>(element + 2) = Byte(RoundInt(Float(c.x))); }
			if(writeG) { *Pointer<Byte>(element + 1) = Byte(RoundInt(Float(c.y))); }
			if(writeB) { *Pointer<Byte>(element + 0) = Byte(RoundInt(Float(c.z))); }
			break;
		case FORMAT_B8G8R8:
			if(writeR) { *Pointer<Byte>(element + 0) = Byte(RoundInt(Float(c.x))); }
			if(writeG) { *Pointer<Byte>(element + 1) = Byte(RoundInt(Float(c.y))); }
			if(writeB) { *Pointer<Byte>(element + 2) = Byte(RoundInt(Float(c.z))); }
			break;
		case FORMAT_A32B32G32R32F:
			if(writeRGBA)
			{
				*Pointer<Float4>(element) = c;
			}
			else
			{
				if(writeR) { *Pointer<Float>(element) = c.x; }
				if(writeG) { *Pointer<Float>(element + 4) = c.y; }
				if(writeB) { *Pointer<Float>(element + 8) = c.z; }
				if(writeA) { *Pointer<Float>(element + 12) = c.w; }
			}
			break;
		case FORMAT_X32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
			if(writeA) { *Pointer<Float>(element + 12) = 1.0f; }
		case FORMAT_B32G32R32F:
			if(writeR) { *Pointer<Float>(element) = c.x; }
			if(writeG) { *Pointer<Float>(element + 4) = c.y; }
			if(writeB) { *Pointer<Float>(element + 8) = c.z; }
			break;
		case FORMAT_G32R32F:
			if(writeR && writeG)
			{
				*Pointer<Float2>(element) = Float2(c);
			}
			else
			{
				if(writeR) { *Pointer<Float>(element) = c.x; }
				if(writeG) { *Pointer<Float>(element + 4) = c.y; }
			}
			break;
		case FORMAT_R32F:
			if(writeR) { *Pointer<Float>(element) = c.x; }
			break;
		case FORMAT_A8B8G8R8I:
		case FORMAT_A8B8G8R8_SNORM:
			if(writeA) { *Pointer<SByte>(element + 3) = SByte(RoundInt(Float(c.w))); }
		case FORMAT_X8B8G8R8I:
		case FORMAT_X8B8G8R8_SNORM:
			if(writeA && (state.destFormat == FORMAT_X8B8G8R8I || state.destFormat == FORMAT_X8B8G8R8_SNORM))
			{
				*Pointer<SByte>(element + 3) = SByte(0x7F);
			}
			if(writeB) { *Pointer<SByte>(element + 2) = SByte(RoundInt(Float(c.z))); }
		case FORMAT_G8R8I:
		case FORMAT_G8R8_SNORM:
			if(writeG) { *Pointer<SByte>(element + 1) = SByte(RoundInt(Float(c.y))); }
		case FORMAT_R8I:
		case FORMAT_R8_SNORM:
			if(writeR) { *Pointer<SByte>(element) = SByte(RoundInt(Float(c.x))); }
			break;
		case FORMAT_A8B8G8R8UI:
			if(writeA) { *Pointer<Byte>(element + 3) = Byte(RoundInt(Float(c.w))); }
		case FORMAT_X8B8G8R8UI:
			if(writeA && (state.destFormat == FORMAT_X8B8G8R8UI))
			{
				*Pointer<Byte>(element + 3) = Byte(0xFF);
			}
			if(writeB) { *Pointer<Byte>(element + 2) = Byte(RoundInt(Float(c.z))); }
		case FORMAT_G8R8UI:
		case FORMAT_G8R8:
			if(writeG) { *Pointer<Byte>(element + 1) = Byte(RoundInt(Float(c.y))); }
		case FORMAT_R8UI:
		case FORMAT_R8:
			if(writeR) { *Pointer<Byte>(element) = Byte(RoundInt(Float(c.x))); }
			break;
		case FORMAT_A16B16G16R16I:
			if(writeRGBA)
			{
				*Pointer<Short4>(element) = Short4(RoundInt(c));
			}
			else
			{
				if(writeR) { *Pointer<Short>(element) = Short(RoundInt(Float(c.x))); }
				if(writeG) { *Pointer<Short>(element + 2) = Short(RoundInt(Float(c.y))); }
				if(writeB) { *Pointer<Short>(element + 4) = Short(RoundInt(Float(c.z))); }
				if(writeA) { *Pointer<Short>(element + 6) = Short(RoundInt(Float(c.w))); }
			}
			break;
		case FORMAT_X16B16G16R16I:
			if(writeRGBA)
			{
				*Pointer<Short4>(element) = Short4(RoundInt(c));
			}
			else
			{
				if(writeR) { *Pointer<Short>(element) = Short(RoundInt(Float(c.x))); }
				if(writeG) { *Pointer<Short>(element + 2) = Short(RoundInt(Float(c.y))); }
				if(writeB) { *Pointer<Short>(element + 4) = Short(RoundInt(Float(c.z))); }
			}
			if(writeA) { *Pointer<Short>(element + 6) = Short(0x7F); }
			break;
		case FORMAT_G16R16I:
			if(writeR && writeG)
			{
				*Pointer<Short2>(element) = Short2(Short4(RoundInt(c)));
			}
			else
			{
				if(writeR) { *Pointer<Short>(element) = Short(RoundInt(Float(c.x))); }
				if(writeG) { *Pointer<Short>(element + 2) = Short(RoundInt(Float(c.y))); }
			}
			break;
		case FORMAT_R16I:
			if(writeR) { *Pointer<Short>(element) = Short(RoundInt(Float(c.x))); }
			break;
		case FORMAT_A16B16G16R16UI:
		case FORMAT_A16B16G16R16:
			if(writeRGBA)
			{
				*Pointer<UShort4>(element) = UShort4(RoundInt(c));
			}
			else
			{
				if(writeR) { *Pointer<UShort>(element) = UShort(RoundInt(Float(c.x))); }
				if(writeG) { *Pointer<UShort>(element + 2) = UShort(RoundInt(Float(c.y))); }
				if(writeB) { *Pointer<UShort>(element + 4) = UShort(RoundInt(Float(c.z))); }
				if(writeA) { *Pointer<UShort>(element + 6) = UShort(RoundInt(Float(c.w))); }
			}
			break;
		case FORMAT_X16B16G16R16UI:
			if(writeRGBA)
			{
				*Pointer<UShort4>(element) = UShort4(RoundInt(c));
			}
			else
			{
				if(writeR) { *Pointer<UShort>(element) = UShort(RoundInt(Float(c.x))); }
				if(writeG) { *Pointer<UShort>(element + 2) = UShort(RoundInt(Float(c.y))); }
				if(writeB) { *Pointer<UShort>(element + 4) = UShort(RoundInt(Float(c.z))); }
			}
			if(writeA) { *Pointer<UShort>(element + 6) = UShort(0xFF); }
			break;
		case FORMAT_G16R16UI:
		case FORMAT_G16R16:
			if(writeR && writeG)
			{
				*Pointer<UShort2>(element) = UShort2(UShort4(RoundInt(c)));
			}
			else
			{
				if(writeR) { *Pointer<UShort>(element) = UShort(RoundInt(Float(c.x))); }
				if(writeG) { *Pointer<UShort>(element + 2) = UShort(RoundInt(Float(c.y))); }
			}
			break;
		case FORMAT_R16UI:
			if(writeR) { *Pointer<UShort>(element) = UShort(RoundInt(Float(c.x))); }
			break;
		case FORMAT_A32B32G32R32I:
			if(writeRGBA)
			{
				*Pointer<Int4>(element) = RoundInt(c);
			}
			else
			{
				if(writeR) { *Pointer<Int>(element) = RoundInt(Float(c.x)); }
				if(writeG) { *Pointer<Int>(element + 4) = RoundInt(Float(c.y)); }
				if(writeB) { *Pointer<Int>(element + 8) = RoundInt(Float(c.z)); }
				if(writeA) { *Pointer<Int>(element + 12) = RoundInt(Float(c.w)); }
			}
			break;
		case FORMAT_X32B32G32R32I:
			if(writeRGBA)
			{
				*Pointer<Int4>(element) = RoundInt(c);
			}
			else
			{
				if(writeR) { *Pointer<Int>(element) = RoundInt(Float(c.x)); }
				if(writeG) { *Pointer<Int>(element + 4) = RoundInt(Float(c.y)); }
				if(writeB) { *Pointer<Int>(element + 8) = RoundInt(Float(c.z)); }
			}
			if(writeA) { *Pointer<Int>(element + 12) = Int(0x7FFFFFFF); }
			break;
		case FORMAT_G32R32I:
			if(writeG) { *Pointer<Int>(element + 4) = RoundInt(Float(c.y)); }
		case FORMAT_R32I:
			if(writeR) { *Pointer<Int>(element) = RoundInt(Float(c.x)); }
			break;
		case FORMAT_A32B32G32R32UI:
			if(writeRGBA)
			{
				*Pointer<UInt4>(element) = UInt4(RoundInt(c));
			}
			else
			{
				if(writeR) { *Pointer<UInt>(element) = As<UInt>(RoundInt(Float(c.x))); }
				if(writeG) { *Pointer<UInt>(element + 4) = As<UInt>(RoundInt(Float(c.y))); }
				if(writeB) { *Pointer<UInt>(element + 8) = As<UInt>(RoundInt(Float(c.z))); }
				if(writeA) { *Pointer<UInt>(element + 12) = As<UInt>(RoundInt(Float(c.w))); }
			}
			break;
		case FORMAT_X32B32G32R32UI:
			if(writeRGBA)
			{
				*Pointer<UInt4>(element) = UInt4(RoundInt(c));
			}
			else
			{
				if(writeR) { *Pointer<UInt>(element) = As<UInt>(RoundInt(Float(c.x))); }
				if(writeG) { *Pointer<UInt>(element + 4) = As<UInt>(RoundInt(Float(c.y))); }
				if(writeB) { *Pointer<UInt>(element + 8) = As<UInt>(RoundInt(Float(c.z))); }
			}
			if(writeA) { *Pointer<UInt4>(element + 12) = UInt4(0xFFFFFFFF); }
			break;
		case FORMAT_G32R32UI:
			if(writeG) { *Pointer<UInt>(element + 4) = As<UInt>(RoundInt(Float(c.y))); }
		case FORMAT_R32UI:
			if(writeR) { *Pointer<UInt>(element) = As<UInt>(RoundInt(Float(c.x))); }
			break;
		case FORMAT_R5G6B5:
			if(writeR && writeG && writeB)
			{
				*Pointer<UShort>(element) = UShort(RoundInt(Float(c.z)) |
				                                  (RoundInt(Float(c.y)) << Int(5)) |
				                                  (RoundInt(Float(c.x)) << Int(11)));
			}
			else
			{
				unsigned short mask = (writeB ? 0x001F : 0x0000) | (writeG ? 0x07E0 : 0x0000) | (writeR ? 0xF800 : 0x0000);
				unsigned short unmask = ~mask;
				*Pointer<UShort>(element) = (*Pointer<UShort>(element) & UShort(unmask)) |
				                            (UShort(RoundInt(Float(c.z)) |
				                                   (RoundInt(Float(c.y)) << Int(5)) |
				                                   (RoundInt(Float(c.x)) << Int(11))) & UShort(mask));
			}
			break;
		case FORMAT_A2B10G10R10:
		case FORMAT_A2B10G10R10UI:
			if(writeRGBA)
			{
				*Pointer<UInt>(element) = UInt(RoundInt(Float(c.x)) |
				                              (RoundInt(Float(c.y)) << 10) |
				                              (RoundInt(Float(c.z)) << 20) |
				                              (RoundInt(Float(c.w)) << 30));
			}
			else
			{
				unsigned int mask = (writeA ? 0xC0000000 : 0x0000) |
				                    (writeB ? 0x3FF00000 : 0x0000) |
				                    (writeG ? 0x000FFC00 : 0x0000) |
				                    (writeR ? 0x000003FF : 0x0000);
				unsigned int unmask = ~mask;
				*Pointer<UInt>(element) = (*Pointer<UInt>(element) & UInt(unmask)) |
				                            (UInt(RoundInt(Float(c.x)) |
				                                  (RoundInt(Float(c.y)) << 10) |
				                                  (RoundInt(Float(c.z)) << 20) |
				                                  (RoundInt(Float(c.w)) << 30)) & UInt(mask));
			}
			break;
		case FORMAT_D16:
			*Pointer<UShort>(element) = UShort(RoundInt(Float(c.x)));
			break;
		case FORMAT_D24S8:
			*Pointer<UInt>(element) = UInt(RoundInt(Float(c.x)));
			break;
		case FORMAT_D32:
			*Pointer<UInt>(element) = UInt(RoundInt(Float(c.x)));
			break;
		case FORMAT_D32F_COMPLEMENTARY:
		case FORMAT_D32FS8_COMPLEMENTARY:
			*Pointer<Float>(element) = 1.0f - c.x;
			break;
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
			*Pointer<Float>(element) = c.x;
			break;
		case FORMAT_S8:
			*Pointer<Byte>(element) = Byte(RoundInt(Float(c.x)));
			break;
		default:
			return false;
		}
		return true;
	}

	bool Blitter::read(Int4 &c, Pointer<Byte> element, const State &state)
	{
		c = Int4(0, 0, 0, 1);

		switch(state.sourceFormat)
		{
		case FORMAT_A8B8G8R8I:
			c = Insert(c, Int(*Pointer<SByte>(element + 3)), 3);
		case FORMAT_X8B8G8R8I:
			c = Insert(c, Int(*Pointer<SByte>(element + 2)), 2);
		case FORMAT_G8R8I:
			c = Insert(c, Int(*Pointer<SByte>(element + 1)), 1);
		case FORMAT_R8I:
			c = Insert(c, Int(*Pointer<SByte>(element)), 0);
			break;
		case FORMAT_A8B8G8R8UI:
			c = Insert(c, Int(*Pointer<Byte>(element + 3)), 3);
		case FORMAT_X8B8G8R8UI:
			c = Insert(c, Int(*Pointer<Byte>(element + 2)), 2);
		case FORMAT_G8R8UI:
			c = Insert(c, Int(*Pointer<Byte>(element + 1)), 1);
		case FORMAT_R8UI:
			c = Insert(c, Int(*Pointer<Byte>(element)), 0);
			break;
		case FORMAT_A16B16G16R16I:
			c = Insert(c, Int(*Pointer<Short>(element + 6)), 3);
		case FORMAT_X16B16G16R16I:
			c = Insert(c, Int(*Pointer<Short>(element + 4)), 2);
		case FORMAT_G16R16I:
			c = Insert(c, Int(*Pointer<Short>(element + 2)), 1);
		case FORMAT_R16I:
			c = Insert(c, Int(*Pointer<Short>(element)), 0);
			break;
		case FORMAT_A16B16G16R16UI:
			c = Insert(c, Int(*Pointer<UShort>(element + 6)), 3);
		case FORMAT_X16B16G16R16UI:
			c = Insert(c, Int(*Pointer<UShort>(element + 4)), 2);
		case FORMAT_G16R16UI:
			c = Insert(c, Int(*Pointer<UShort>(element + 2)), 1);
		case FORMAT_R16UI:
			c = Insert(c, Int(*Pointer<UShort>(element)), 0);
			break;
		case FORMAT_A32B32G32R32I:
		case FORMAT_A32B32G32R32UI:
			c = *Pointer<Int4>(element);
			break;
		case FORMAT_X32B32G32R32I:
		case FORMAT_X32B32G32R32UI:
			c = Insert(c, *Pointer<Int>(element + 8), 2);
		case FORMAT_G32R32I:
		case FORMAT_G32R32UI:
			c = Insert(c, *Pointer<Int>(element + 4), 1);
		case FORMAT_R32I:
		case FORMAT_R32UI:
			c = Insert(c, *Pointer<Int>(element), 0);
			break;
		default:
			return false;
		}

		return true;
	}

	bool Blitter::write(Int4 &c, Pointer<Byte> element, const State &state)
	{
		bool writeR = state.writeRed;
		bool writeG = state.writeGreen;
		bool writeB = state.writeBlue;
		bool writeA = state.writeAlpha;
		bool writeRGBA = writeR && writeG && writeB && writeA;

		switch(state.destFormat)
		{
		case FORMAT_A8B8G8R8I:
			if(writeA) { *Pointer<SByte>(element + 3) = SByte(Extract(c, 3)); }
		case FORMAT_X8B8G8R8I:
			if(writeA && (state.destFormat != FORMAT_A8B8G8R8I))
			{
				*Pointer<SByte>(element + 3) = SByte(0x7F);
			}
			if(writeB) { *Pointer<SByte>(element + 2) = SByte(Extract(c, 2)); }
		case FORMAT_G8R8I:
			if(writeG) { *Pointer<SByte>(element + 1) = SByte(Extract(c, 1)); }
		case FORMAT_R8I:
			if(writeR) { *Pointer<SByte>(element) = SByte(Extract(c, 0)); }
			break;
		case FORMAT_A8B8G8R8UI:
			if(writeA) { *Pointer<Byte>(element + 3) = Byte(Extract(c, 3)); }
		case FORMAT_X8B8G8R8UI:
			if(writeA && (state.destFormat != FORMAT_A8B8G8R8UI))
			{
				*Pointer<Byte>(element + 3) = Byte(0xFF);
			}
			if(writeB) { *Pointer<Byte>(element + 2) = Byte(Extract(c, 2)); }
		case FORMAT_G8R8UI:
			if(writeG) { *Pointer<Byte>(element + 1) = Byte(Extract(c, 1)); }
		case FORMAT_R8UI:
			if(writeR) { *Pointer<Byte>(element) = Byte(Extract(c, 0)); }
			break;
		case FORMAT_A16B16G16R16I:
			if(writeA) { *Pointer<Short>(element + 6) = Short(Extract(c, 3)); }
		case FORMAT_X16B16G16R16I:
			if(writeA && (state.destFormat != FORMAT_A16B16G16R16I))
			{
				*Pointer<Short>(element + 6) = Short(0x7FFF);
			}
			if(writeB) { *Pointer<Short>(element + 4) = Short(Extract(c, 2)); }
		case FORMAT_G16R16I:
			if(writeG) { *Pointer<Short>(element + 2) = Short(Extract(c, 1)); }
		case FORMAT_R16I:
			if(writeR) { *Pointer<Short>(element) = Short(Extract(c, 0)); }
			break;
		case FORMAT_A16B16G16R16UI:
			if(writeA) { *Pointer<UShort>(element + 6) = UShort(Extract(c, 3)); }
		case FORMAT_X16B16G16R16UI:
			if(writeA && (state.destFormat != FORMAT_A16B16G16R16UI))
			{
				*Pointer<UShort>(element + 6) = UShort(0xFFFF);
			}
			if(writeB) { *Pointer<UShort>(element + 4) = UShort(Extract(c, 2)); }
		case FORMAT_G16R16UI:
			if(writeG) { *Pointer<UShort>(element + 2) = UShort(Extract(c, 1)); }
		case FORMAT_R16UI:
			if(writeR) { *Pointer<UShort>(element) = UShort(Extract(c, 0)); }
			break;
		case FORMAT_A32B32G32R32I:
			if(writeRGBA)
			{
				*Pointer<Int4>(element) = c;
			}
			else
			{
				if(writeR) { *Pointer<Int>(element) = Extract(c, 0); }
				if(writeG) { *Pointer<Int>(element + 4) = Extract(c, 1); }
				if(writeB) { *Pointer<Int>(element + 8) = Extract(c, 2); }
				if(writeA) { *Pointer<Int>(element + 12) = Extract(c, 3); }
			}
			break;
		case FORMAT_X32B32G32R32I:
			if(writeRGBA)
			{
				*Pointer<Int4>(element) = c;
			}
			else
			{
				if(writeR) { *Pointer<Int>(element) = Extract(c, 0); }
				if(writeG) { *Pointer<Int>(element + 4) = Extract(c, 1); }
				if(writeB) { *Pointer<Int>(element + 8) = Extract(c, 2); }
			}
			if(writeA) { *Pointer<Int>(element + 12) = Int(0x7FFFFFFF); }
			break;
		case FORMAT_G32R32I:
			if(writeR) { *Pointer<Int>(element) = Extract(c, 0); }
			if(writeG) { *Pointer<Int>(element + 4) = Extract(c, 1); }
			break;
		case FORMAT_R32I:
			if(writeR) { *Pointer<Int>(element) = Extract(c, 0); }
			break;
		case FORMAT_A32B32G32R32UI:
			if(writeRGBA)
			{
				*Pointer<UInt4>(element) = As<UInt4>(c);
			}
			else
			{
				if(writeR) { *Pointer<UInt>(element) = As<UInt>(Extract(c, 0)); }
				if(writeG) { *Pointer<UInt>(element + 4) = As<UInt>(Extract(c, 1)); }
				if(writeB) { *Pointer<UInt>(element + 8) = As<UInt>(Extract(c, 2)); }
				if(writeA) { *Pointer<UInt>(element + 12) = As<UInt>(Extract(c, 3)); }
			}
			break;
		case FORMAT_X32B32G32R32UI:
			if(writeRGBA)
			{
				*Pointer<UInt4>(element) = As<UInt4>(c);
			}
			else
			{
				if(writeR) { *Pointer<UInt>(element) = As<UInt>(Extract(c, 0)); }
				if(writeG) { *Pointer<UInt>(element + 4) = As<UInt>(Extract(c, 1)); }
				if(writeB) { *Pointer<UInt>(element + 8) = As<UInt>(Extract(c, 2)); }
			}
			if(writeA) { *Pointer<UInt>(element + 3) = UInt(0xFFFFFFFF); }
			break;
		case FORMAT_G32R32UI:
			if(writeR) { *Pointer<UInt>(element) = As<UInt>(Extract(c, 0)); }
			if(writeG) { *Pointer<UInt>(element + 4) = As<UInt>(Extract(c, 1)); }
			break;
		case FORMAT_R32UI:
			if(writeR) { *Pointer<UInt>(element) = As<UInt>(Extract(c, 0)); }
			break;
		default:
			return false;
		}

		return true;
	}

	bool Blitter::GetScale(float4 &scale, Format format)
	{
		switch(format)
		{
		case FORMAT_L8:
		case FORMAT_A8:
		case FORMAT_A8R8G8B8:
		case FORMAT_X8R8G8B8:
		case FORMAT_R8:
		case FORMAT_G8R8:
		case FORMAT_R8G8B8:
		case FORMAT_B8G8R8:
		case FORMAT_X8B8G8R8:
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
			scale = vector(0xFF, 0xFF, 0xFF, 0xFF);
			break;
		case FORMAT_R8_SNORM:
		case FORMAT_G8R8_SNORM:
		case FORMAT_X8B8G8R8_SNORM:
		case FORMAT_A8B8G8R8_SNORM:
			scale = vector(0x7F, 0x7F, 0x7F, 0x7F);
			break;
		case FORMAT_A16B16G16R16:
			scale = vector(0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF);
			break;
		case FORMAT_R8I:
		case FORMAT_R8UI:
		case FORMAT_G8R8I:
		case FORMAT_G8R8UI:
		case FORMAT_X8B8G8R8I:
		case FORMAT_X8B8G8R8UI:
		case FORMAT_A8B8G8R8I:
		case FORMAT_A8B8G8R8UI:
		case FORMAT_R16I:
		case FORMAT_R16UI:
		case FORMAT_G16R16:
		case FORMAT_G16R16I:
		case FORMAT_G16R16UI:
		case FORMAT_X16B16G16R16I:
		case FORMAT_X16B16G16R16UI:
		case FORMAT_A16B16G16R16I:
		case FORMAT_A16B16G16R16UI:
		case FORMAT_R32I:
		case FORMAT_R32UI:
		case FORMAT_G32R32I:
		case FORMAT_G32R32UI:
		case FORMAT_X32B32G32R32I:
		case FORMAT_X32B32G32R32UI:
		case FORMAT_A32B32G32R32I:
		case FORMAT_A32B32G32R32UI:
		case FORMAT_A32B32G32R32F:
		case FORMAT_X32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_B32G32R32F:
		case FORMAT_G32R32F:
		case FORMAT_R32F:
		case FORMAT_A2B10G10R10UI:
			scale = vector(1.0f, 1.0f, 1.0f, 1.0f);
			break;
		case FORMAT_R5G6B5:
			scale = vector(0x1F, 0x3F, 0x1F, 1.0f);
			break;
		case FORMAT_A2B10G10R10:
			scale = vector(0x3FF, 0x3FF, 0x3FF, 0x03);
			break;
		case FORMAT_D16:
			scale = vector(0xFFFF, 0.0f, 0.0f, 0.0f);
			break;
		case FORMAT_D24S8:
			scale = vector(0xFFFFFF, 0.0f, 0.0f, 0.0f);
			break;
		case FORMAT_D32:
			scale = vector(static_cast<float>(0xFFFFFFFF), 0.0f, 0.0f, 0.0f);
			break;
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_COMPLEMENTARY:
		case FORMAT_D32FS8_COMPLEMENTARY:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
		case FORMAT_S8:
			scale = vector(1.0f, 1.0f, 1.0f, 1.0f);
			break;
		default:
			return false;
		}

		return true;
	}

	bool Blitter::ApplyScaleAndClamp(Float4 &value, const State &state, bool preScaled)
	{
		float4 scale, unscale;
		if(state.clearOperation &&
		   Surface::isNonNormalizedInteger(state.sourceFormat) &&
		   !Surface::isNonNormalizedInteger(state.destFormat))
		{
			// If we're clearing a buffer from an int or uint color into a normalized color,
			// then the whole range of the int or uint color must be scaled between 0 and 1.
			switch(state.sourceFormat)
			{
			case FORMAT_A32B32G32R32I:
				unscale = replicate(static_cast<float>(0x7FFFFFFF));
				break;
			case FORMAT_A32B32G32R32UI:
				unscale = replicate(static_cast<float>(0xFFFFFFFF));
				break;
			default:
				return false;
			}
		}
		else if(!GetScale(unscale, state.sourceFormat))
		{
			return false;
		}

		if(!GetScale(scale, state.destFormat))
		{
			return false;
		}

		bool srcSRGB = Surface::isSRGBformat(state.sourceFormat);
		bool dstSRGB = Surface::isSRGBformat(state.destFormat);

		if(state.convertSRGB && ((srcSRGB && !preScaled) || dstSRGB))   // One of the formats is sRGB encoded.
		{
			value *= preScaled ? Float4(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z, 1.0f / scale.w) : // Unapply scale
			                     Float4(1.0f / unscale.x, 1.0f / unscale.y, 1.0f / unscale.z, 1.0f / unscale.w); // Apply unscale
			value = (srcSRGB && !preScaled) ? sRGBtoLinear(value) : LinearToSRGB(value);
			value *= Float4(scale.x, scale.y, scale.z, scale.w); // Apply scale
		}
		else if(unscale != scale)
		{
			value *= Float4(scale.x / unscale.x, scale.y / unscale.y, scale.z / unscale.z, scale.w / unscale.w);
		}

		if(state.destFormat == FORMAT_X32B32G32R32F_UNSIGNED)
		{
			value = Max(value, Float4(0.0f));  // TODO: Only necessary if source is signed.
		}
		else if(Surface::isFloatFormat(state.sourceFormat) && !Surface::isFloatFormat(state.destFormat))
		{
			value = Min(value, Float4(scale.x, scale.y, scale.z, scale.w));

			value = Max(value, Float4(Surface::isUnsignedComponent(state.destFormat, 0) ? 0.0f : -scale.x,
			                          Surface::isUnsignedComponent(state.destFormat, 1) ? 0.0f : -scale.y,
			                          Surface::isUnsignedComponent(state.destFormat, 2) ? 0.0f : -scale.z,
			                          Surface::isUnsignedComponent(state.destFormat, 3) ? 0.0f : -scale.w));
		}

		return true;
	}

	Int Blitter::ComputeOffset(Int &x, Int &y, Int &pitchB, int bytes, bool quadLayout)
	{
		if(!quadLayout)
		{
			return y * pitchB + x * bytes;
		}
		else
		{
			// (x & ~1) * 2 + (x & 1) == (x - (x & 1)) * 2 + (x & 1) == x * 2 - (x & 1) * 2 + (x & 1) == x * 2 - (x & 1)
			return (y & Int(~1)) * pitchB +
			       ((y & Int(1)) * 2 + x * 2 - (x & Int(1))) * bytes;
		}
	}

	Float4 Blitter::LinearToSRGB(Float4 &c)
	{
		Float4 lc = Min(c, Float4(0.0031308f)) * Float4(12.92f);
		Float4 ec = Float4(1.055f) * power(c, Float4(1.0f / 2.4f)) - Float4(0.055f);

		Float4 s = c;
		s.xyz = Max(lc, ec);

		return s;
	}

	Float4 Blitter::sRGBtoLinear(Float4 &c)
	{
		Float4 lc = c * Float4(1.0f / 12.92f);
		Float4 ec = power((c + Float4(0.055f)) * Float4(1.0f / 1.055f), Float4(2.4f));

		Int4 linear = CmpLT(c, Float4(0.04045f));

		Float4 s = c;
		s.xyz = As<Float4>((linear & As<Int4>(lc)) | (~linear & As<Int4>(ec)));   // FIXME: IfThenElse()

		return s;
	}

	Routine *Blitter::generate(const State &state)
	{
		Function<Void(Pointer<Byte>)> function;
		{
			Pointer<Byte> blit(function.Arg<0>());

			Pointer<Byte> source = *Pointer<Pointer<Byte>>(blit + OFFSET(BlitData,source));
			Pointer<Byte> dest = *Pointer<Pointer<Byte>>(blit + OFFSET(BlitData,dest));
			Int sPitchB = *Pointer<Int>(blit + OFFSET(BlitData,sPitchB));
			Int dPitchB = *Pointer<Int>(blit + OFFSET(BlitData,dPitchB));

			Float x0 = *Pointer<Float>(blit + OFFSET(BlitData,x0));
			Float y0 = *Pointer<Float>(blit + OFFSET(BlitData,y0));
			Float w = *Pointer<Float>(blit + OFFSET(BlitData,w));
			Float h = *Pointer<Float>(blit + OFFSET(BlitData,h));

			Int x0d = *Pointer<Int>(blit + OFFSET(BlitData,x0d));
			Int x1d = *Pointer<Int>(blit + OFFSET(BlitData,x1d));
			Int y0d = *Pointer<Int>(blit + OFFSET(BlitData,y0d));
			Int y1d = *Pointer<Int>(blit + OFFSET(BlitData,y1d));

			Int sWidth = *Pointer<Int>(blit + OFFSET(BlitData,sWidth));
			Int sHeight = *Pointer<Int>(blit + OFFSET(BlitData,sHeight));

			bool intSrc = Surface::isNonNormalizedInteger(state.sourceFormat);
			bool intDst = Surface::isNonNormalizedInteger(state.destFormat);
			bool intBoth = intSrc && intDst;
			bool srcQuadLayout = Surface::hasQuadLayout(state.sourceFormat);
			bool dstQuadLayout = Surface::hasQuadLayout(state.destFormat);
			int srcBytes = Surface::bytes(state.sourceFormat);
			int dstBytes = Surface::bytes(state.destFormat);

			bool hasConstantColorI = false;
			Int4 constantColorI;
			bool hasConstantColorF = false;
			Float4 constantColorF;
			if(state.clearOperation)
			{
				if(intBoth) // Integer types
				{
					if(!read(constantColorI, source, state))
					{
						return nullptr;
					}
					hasConstantColorI = true;
				}
				else
				{
					if(!read(constantColorF, source, state))
					{
						return nullptr;
					}
					hasConstantColorF = true;

					if(!ApplyScaleAndClamp(constantColorF, state))
					{
						return nullptr;
					}
				}
			}

			Float y = y0;

			For(Int j = y0d, j < y1d, j++)
			{
				Float x = x0;
				Pointer<Byte> destLine = dest + (dstQuadLayout ? j & Int(~1) : RValue<Int>(j)) * dPitchB;

				For(Int i = x0d, i < x1d, i++)
				{
					Pointer<Byte> d = destLine + (dstQuadLayout ? (((j & Int(1)) << 1) + (i * 2) - (i & Int(1))) : RValue<Int>(i)) * dstBytes;

					if(hasConstantColorI)
					{
						if(!write(constantColorI, d, state))
						{
							return nullptr;
						}
					}
					else if(hasConstantColorF)
					{
						for(int s = 0; s < state.destSamples; s++)
						{
							if(!write(constantColorF, d, state))
							{
								return nullptr;
							}

							d += *Pointer<Int>(blit + OFFSET(BlitData, dSliceB));
						}
					}
					else if(intBoth) // Integer types do not support filtering
					{
						Int4 color; // When both formats are true integer types, we don't go to float to avoid losing precision
						Int X = Int(x);
						Int Y = Int(y);

						if(state.clampToEdge)
						{
							X = Clamp(X, 0, sWidth - 1);
							Y = Clamp(Y, 0, sHeight - 1);
						}

						Pointer<Byte> s = source + ComputeOffset(X, Y, sPitchB, srcBytes, srcQuadLayout);

						if(!read(color, s, state))
						{
							return nullptr;
						}

						if(!write(color, d, state))
						{
							return nullptr;
						}
					}
					else
					{
						Float4 color;

						bool preScaled = false;
						if(!state.filter || intSrc)
						{
							Int X = Int(x);
							Int Y = Int(y);

							if(state.clampToEdge)
							{
								X = Clamp(X, 0, sWidth - 1);
								Y = Clamp(Y, 0, sHeight - 1);
							}

							Pointer<Byte> s = source + ComputeOffset(X, Y, sPitchB, srcBytes, srcQuadLayout);

							if(!read(color, s, state))
							{
								return nullptr;
							}
						}
						else   // Bilinear filtering
						{
							Float X = x;
							Float Y = y;

							if(state.clampToEdge)
							{
								X = Min(Max(x, 0.5f), Float(sWidth) - 0.5f);
								Y = Min(Max(y, 0.5f), Float(sHeight) - 0.5f);
							}

							Float x0 = X - 0.5f;
							Float y0 = Y - 0.5f;

							Int X0 = Max(Int(x0), 0);
							Int Y0 = Max(Int(y0), 0);

							Int X1 = X0 + 1;
							Int Y1 = Y0 + 1;
							X1 = IfThenElse(X1 >= sWidth, X0, X1);
							Y1 = IfThenElse(Y1 >= sHeight, Y0, Y1);

							Pointer<Byte> s00 = source + ComputeOffset(X0, Y0, sPitchB, srcBytes, srcQuadLayout);
							Pointer<Byte> s01 = source + ComputeOffset(X1, Y0, sPitchB, srcBytes, srcQuadLayout);
							Pointer<Byte> s10 = source + ComputeOffset(X0, Y1, sPitchB, srcBytes, srcQuadLayout);
							Pointer<Byte> s11 = source + ComputeOffset(X1, Y1, sPitchB, srcBytes, srcQuadLayout);

							Float4 c00; if(!read(c00, s00, state)) return nullptr;
							Float4 c01; if(!read(c01, s01, state)) return nullptr;
							Float4 c10; if(!read(c10, s10, state)) return nullptr;
							Float4 c11; if(!read(c11, s11, state)) return nullptr;

							if(state.convertSRGB && Surface::isSRGBformat(state.sourceFormat)) // sRGB -> RGB
							{
								if(!ApplyScaleAndClamp(c00, state)) return nullptr;
								if(!ApplyScaleAndClamp(c01, state)) return nullptr;
								if(!ApplyScaleAndClamp(c10, state)) return nullptr;
								if(!ApplyScaleAndClamp(c11, state)) return nullptr;
								preScaled = true;
							}

							Float4 fx = Float4(x0 - Float(X0));
							Float4 fy = Float4(y0 - Float(Y0));
							Float4 ix = Float4(1.0f) - fx;
							Float4 iy = Float4(1.0f) - fy;

							color = (c00 * ix + c01 * fx) * iy +
							        (c10 * ix + c11 * fx) * fy;
						}

						if(!ApplyScaleAndClamp(color, state, preScaled))
						{
							return nullptr;
						}

						for(int s = 0; s < state.destSamples; s++)
						{
							if(!write(color, d, state))
							{
								return nullptr;
							}

							d += *Pointer<Int>(blit + OFFSET(BlitData,dSliceB));
						}
					}

					if(!state.clearOperation) { x += w; }
				}

				if(!state.clearOperation) { y += h; }
			}
		}

		return function(L"BlitRoutine");
	}

	bool Blitter::blitReactor(Surface *source, const SliceRectF &sourceRect, Surface *dest, const SliceRect &destRect, const Blitter::Options &options)
	{
		ASSERT(!options.clearOperation || ((source->getWidth() == 1) && (source->getHeight() == 1) && (source->getDepth() == 1)));

		Rect dRect = destRect;
		RectF sRect = sourceRect;
		if(destRect.x0 > destRect.x1)
		{
			swap(dRect.x0, dRect.x1);
			swap(sRect.x0, sRect.x1);
		}
		if(destRect.y0 > destRect.y1)
		{
			swap(dRect.y0, dRect.y1);
			swap(sRect.y0, sRect.y1);
		}

		State state(options);
		state.clampToEdge = (sourceRect.x0 < 0.0f) ||
		                    (sourceRect.y0 < 0.0f) ||
		                    (sourceRect.x1 > (float)source->getWidth()) ||
		                    (sourceRect.y1 > (float)source->getHeight());

		bool useSourceInternal = !source->isExternalDirty();
		bool useDestInternal = !dest->isExternalDirty();
		bool isStencil = options.useStencil;

		state.sourceFormat = isStencil ? source->getStencilFormat() : source->getFormat(useSourceInternal);
		state.destFormat = isStencil ? dest->getStencilFormat() : dest->getFormat(useDestInternal);
		state.destSamples = dest->getSamples();

		criticalSection.lock();
		Routine *blitRoutine = blitCache->query(state);

		if(!blitRoutine)
		{
			blitRoutine = generate(state);

			if(!blitRoutine)
			{
				criticalSection.unlock();
				return false;
			}

			blitCache->add(state, blitRoutine);
		}

		criticalSection.unlock();

		void (*blitFunction)(const BlitData *data) = (void(*)(const BlitData*))blitRoutine->getEntry();

		BlitData data;

		bool isRGBA = options.writeMask == 0xF;
		bool isEntireDest = dest->isEntire(destRect);

		data.source = isStencil ? source->lockStencil(0, 0, 0, sw::PUBLIC) :
		                          source->lock(0, 0, sourceRect.slice, sw::LOCK_READONLY, sw::PUBLIC, useSourceInternal);
		data.dest = isStencil ? dest->lockStencil(0, 0, 0, sw::PUBLIC) :
		                        dest->lock(0, 0, destRect.slice, isRGBA ? (isEntireDest ? sw::LOCK_DISCARD : sw::LOCK_WRITEONLY) : sw::LOCK_READWRITE, sw::PUBLIC, useDestInternal);
		data.sPitchB = isStencil ? source->getStencilPitchB() : source->getPitchB(useSourceInternal);
		data.dPitchB = isStencil ? dest->getStencilPitchB() : dest->getPitchB(useDestInternal);
		data.dSliceB = isStencil ? dest->getStencilSliceB() : dest->getSliceB(useDestInternal);

		data.w = sRect.width() / dRect.width();
		data.h = sRect.height() / dRect.height();
		data.x0 = sRect.x0 + 0.5f * data.w;
		data.y0 = sRect.y0 + 0.5f * data.h;

		data.x0d = dRect.x0;
		data.x1d = dRect.x1;
		data.y0d = dRect.y0;
		data.y1d = dRect.y1;

		data.sWidth = source->getWidth();
		data.sHeight = source->getHeight();

		blitFunction(&data);

		if(isStencil)
		{
			source->unlockStencil();
			dest->unlockStencil();
		}
		else
		{
			source->unlock(useSourceInternal);
			dest->unlock(useDestInternal);
		}

		return true;
	}
}
