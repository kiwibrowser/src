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

#include "Surface.hpp"

#include "Color.hpp"
#include "Context.hpp"
#include "ETC_Decoder.hpp"
#include "Renderer.hpp"
#include "Common/Half.hpp"
#include "Common/Memory.hpp"
#include "Common/CPUID.hpp"
#include "Common/Resource.hpp"
#include "Common/Debug.hpp"
#include "Reactor/Reactor.hpp"

#if defined(__i386__) || defined(__x86_64__)
	#include <xmmintrin.h>
	#include <emmintrin.h>
#endif

#undef min
#undef max

namespace sw
{
	extern bool quadLayoutEnabled;
	extern bool complementaryDepthBuffer;
	extern TranscendentalPrecision logPrecision;

	unsigned int *Surface::palette = 0;
	unsigned int Surface::paletteID = 0;

	void Surface::Buffer::write(int x, int y, int z, const Color<float> &color)
	{
		byte *element = (byte*)buffer + (x + border) * bytes + (y + border) * pitchB + z * samples * sliceB;

		for(int i = 0; i < samples; i++)
		{
			write(element, color);
			element += sliceB;
		}
	}

	void Surface::Buffer::write(int x, int y, const Color<float> &color)
	{
		byte *element = (byte*)buffer + (x + border) * bytes + (y + border) * pitchB;

		for(int i = 0; i < samples; i++)
		{
			write(element, color);
			element += sliceB;
		}
	}

	inline void Surface::Buffer::write(void *element, const Color<float> &color)
	{
		float r = color.r;
		float g = color.g;
		float b = color.b;
		float a = color.a;

		if(isSRGBformat(format))
		{
			r = linearToSRGB(r);
			g = linearToSRGB(g);
			b = linearToSRGB(b);
		}

		switch(format)
		{
		case FORMAT_A8:
			*(unsigned char*)element = unorm<8>(a);
			break;
		case FORMAT_R8_SNORM:
			*(char*)element = snorm<8>(r);
			break;
		case FORMAT_R8:
			*(unsigned char*)element = unorm<8>(r);
			break;
		case FORMAT_R8I:
			*(char*)element = scast<8>(r);
			break;
		case FORMAT_R8UI:
			*(unsigned char*)element = ucast<8>(r);
			break;
		case FORMAT_R16I:
			*(short*)element = scast<16>(r);
			break;
		case FORMAT_R16UI:
			*(unsigned short*)element = ucast<16>(r);
			break;
		case FORMAT_R32I:
			*(int*)element = static_cast<int>(r);
			break;
		case FORMAT_R32UI:
			*(unsigned int*)element = static_cast<unsigned int>(r);
			break;
		case FORMAT_R3G3B2:
			*(unsigned char*)element = (unorm<3>(r) << 5) | (unorm<3>(g) << 2) | (unorm<2>(b) << 0);
			break;
		case FORMAT_A8R3G3B2:
			*(unsigned short*)element = (unorm<8>(a) << 8) | (unorm<3>(r) << 5) | (unorm<3>(g) << 2) | (unorm<2>(b) << 0);
			break;
		case FORMAT_X4R4G4B4:
			*(unsigned short*)element = 0xF000 | (unorm<4>(r) << 8) | (unorm<4>(g) << 4) | (unorm<4>(b) << 0);
			break;
		case FORMAT_A4R4G4B4:
			*(unsigned short*)element = (unorm<4>(a) << 12) | (unorm<4>(r) << 8) | (unorm<4>(g) << 4) | (unorm<4>(b) << 0);
			break;
		case FORMAT_R4G4B4A4:
			*(unsigned short*)element = (unorm<4>(r) << 12) | (unorm<4>(g) << 8) | (unorm<4>(b) << 4) | (unorm<4>(a) << 0);
			break;
		case FORMAT_R5G6B5:
			*(unsigned short*)element = (unorm<5>(r) << 11) | (unorm<6>(g) << 5) | (unorm<5>(b) << 0);
			break;
		case FORMAT_A1R5G5B5:
			*(unsigned short*)element = (unorm<1>(a) << 15) | (unorm<5>(r) << 10) | (unorm<5>(g) << 5) | (unorm<5>(b) << 0);
			break;
		case FORMAT_R5G5B5A1:
			*(unsigned short*)element = (unorm<5>(r) << 11) | (unorm<5>(g) << 6) | (unorm<5>(b) << 1) | (unorm<5>(a) << 0);
			break;
		case FORMAT_X1R5G5B5:
			*(unsigned short*)element = 0x8000 | (unorm<5>(r) << 10) | (unorm<5>(g) << 5) | (unorm<5>(b) << 0);
			break;
		case FORMAT_A8R8G8B8:
			*(unsigned int*)element = (unorm<8>(a) << 24) | (unorm<8>(r) << 16) | (unorm<8>(g) << 8) | (unorm<8>(b) << 0);
			break;
		case FORMAT_X8R8G8B8:
			*(unsigned int*)element = 0xFF000000 | (unorm<8>(r) << 16) | (unorm<8>(g) << 8) | (unorm<8>(b) << 0);
			break;
		case FORMAT_A8B8G8R8_SNORM:
			*(unsigned int*)element = (static_cast<unsigned int>(snorm<8>(a)) << 24) |
			                          (static_cast<unsigned int>(snorm<8>(b)) << 16) |
			                          (static_cast<unsigned int>(snorm<8>(g)) << 8) |
			                          (static_cast<unsigned int>(snorm<8>(r)) << 0);
			break;
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_A8:
			*(unsigned int*)element = (unorm<8>(a) << 24) | (unorm<8>(b) << 16) | (unorm<8>(g) << 8) | (unorm<8>(r) << 0);
			break;
		case FORMAT_A8B8G8R8I:
			*(unsigned int*)element = (static_cast<unsigned int>(scast<8>(a)) << 24) |
			                          (static_cast<unsigned int>(scast<8>(b)) << 16) |
			                          (static_cast<unsigned int>(scast<8>(g)) << 8) |
			                          (static_cast<unsigned int>(scast<8>(r)) << 0);
			break;
		case FORMAT_A8B8G8R8UI:
			*(unsigned int*)element = (ucast<8>(a) << 24) | (ucast<8>(b) << 16) | (ucast<8>(g) << 8) | (ucast<8>(r) << 0);
			break;
		case FORMAT_X8B8G8R8_SNORM:
			*(unsigned int*)element = 0x7F000000 |
			                          (static_cast<unsigned int>(snorm<8>(b)) << 16) |
			                          (static_cast<unsigned int>(snorm<8>(g)) << 8) |
			                          (static_cast<unsigned int>(snorm<8>(r)) << 0);
			break;
		case FORMAT_X8B8G8R8:
		case FORMAT_SRGB8_X8:
			*(unsigned int*)element = 0xFF000000 | (unorm<8>(b) << 16) | (unorm<8>(g) << 8) | (unorm<8>(r) << 0);
			break;
		case FORMAT_X8B8G8R8I:
			*(unsigned int*)element = 0x7F000000 |
			                          (static_cast<unsigned int>(scast<8>(b)) << 16) |
			                          (static_cast<unsigned int>(scast<8>(g)) << 8) |
			                          (static_cast<unsigned int>(scast<8>(r)) << 0);
		case FORMAT_X8B8G8R8UI:
			*(unsigned int*)element = 0xFF000000 | (ucast<8>(b) << 16) | (ucast<8>(g) << 8) | (ucast<8>(r) << 0);
			break;
		case FORMAT_A2R10G10B10:
			*(unsigned int*)element = (unorm<2>(a) << 30) | (unorm<10>(r) << 20) | (unorm<10>(g) << 10) | (unorm<10>(b) << 0);
			break;
		case FORMAT_A2B10G10R10:
		case FORMAT_A2B10G10R10UI:
			*(unsigned int*)element = (unorm<2>(a) << 30) | (unorm<10>(b) << 20) | (unorm<10>(g) << 10) | (unorm<10>(r) << 0);
			break;
		case FORMAT_G8R8_SNORM:
			*(unsigned short*)element = (static_cast<unsigned short>(snorm<8>(g)) << 8) |
			                            (static_cast<unsigned short>(snorm<8>(r)) << 0);
			break;
		case FORMAT_G8R8:
			*(unsigned short*)element = (unorm<8>(g) << 8) | (unorm<8>(r) << 0);
			break;
		case FORMAT_G8R8I:
			*(unsigned short*)element = (static_cast<unsigned short>(scast<8>(g)) << 8) |
			                            (static_cast<unsigned short>(scast<8>(r)) << 0);
			break;
		case FORMAT_G8R8UI:
			*(unsigned short*)element = (ucast<8>(g) << 8) | (ucast<8>(r) << 0);
			break;
		case FORMAT_G16R16:
			*(unsigned int*)element = (unorm<16>(g) << 16) | (unorm<16>(r) << 0);
			break;
		case FORMAT_G16R16I:
			*(unsigned int*)element = (static_cast<unsigned int>(scast<16>(g)) << 16) |
			                          (static_cast<unsigned int>(scast<16>(r)) << 0);
			break;
		case FORMAT_G16R16UI:
			*(unsigned int*)element = (ucast<16>(g) << 16) | (ucast<16>(r) << 0);
			break;
		case FORMAT_G32R32I:
		case FORMAT_G32R32UI:
			((unsigned int*)element)[0] = static_cast<unsigned int>(r);
			((unsigned int*)element)[1] = static_cast<unsigned int>(g);
			break;
		case FORMAT_A16B16G16R16:
			((unsigned short*)element)[0] = unorm<16>(r);
			((unsigned short*)element)[1] = unorm<16>(g);
			((unsigned short*)element)[2] = unorm<16>(b);
			((unsigned short*)element)[3] = unorm<16>(a);
			break;
		case FORMAT_A16B16G16R16I:
			((unsigned short*)element)[0] = static_cast<unsigned short>(scast<16>(r));
			((unsigned short*)element)[1] = static_cast<unsigned short>(scast<16>(g));
			((unsigned short*)element)[2] = static_cast<unsigned short>(scast<16>(b));
			((unsigned short*)element)[3] = static_cast<unsigned short>(scast<16>(a));
			break;
		case FORMAT_A16B16G16R16UI:
			((unsigned short*)element)[0] = static_cast<unsigned short>(ucast<16>(r));
			((unsigned short*)element)[1] = static_cast<unsigned short>(ucast<16>(g));
			((unsigned short*)element)[2] = static_cast<unsigned short>(ucast<16>(b));
			((unsigned short*)element)[3] = static_cast<unsigned short>(ucast<16>(a));
			break;
		case FORMAT_X16B16G16R16I:
			((unsigned short*)element)[0] = static_cast<unsigned short>(scast<16>(r));
			((unsigned short*)element)[1] = static_cast<unsigned short>(scast<16>(g));
			((unsigned short*)element)[2] = static_cast<unsigned short>(scast<16>(b));
			break;
		case FORMAT_X16B16G16R16UI:
			((unsigned short*)element)[0] = static_cast<unsigned short>(ucast<16>(r));
			((unsigned short*)element)[1] = static_cast<unsigned short>(ucast<16>(g));
			((unsigned short*)element)[2] = static_cast<unsigned short>(ucast<16>(b));
			break;
		case FORMAT_A32B32G32R32I:
		case FORMAT_A32B32G32R32UI:
			((unsigned int*)element)[0] = static_cast<unsigned int>(r);
			((unsigned int*)element)[1] = static_cast<unsigned int>(g);
			((unsigned int*)element)[2] = static_cast<unsigned int>(b);
			((unsigned int*)element)[3] = static_cast<unsigned int>(a);
			break;
		case FORMAT_X32B32G32R32I:
		case FORMAT_X32B32G32R32UI:
			((unsigned int*)element)[0] = static_cast<unsigned int>(r);
			((unsigned int*)element)[1] = static_cast<unsigned int>(g);
			((unsigned int*)element)[2] = static_cast<unsigned int>(b);
			break;
		case FORMAT_V8U8:
			*(unsigned short*)element = (snorm<8>(g) << 8) | (snorm<8>(r) << 0);
			break;
		case FORMAT_L6V5U5:
			*(unsigned short*)element = (unorm<6>(b) << 10) | (snorm<5>(g) << 5) | (snorm<5>(r) << 0);
			break;
		case FORMAT_Q8W8V8U8:
			*(unsigned int*)element = (snorm<8>(a) << 24) | (snorm<8>(b) << 16) | (snorm<8>(g) << 8) | (snorm<8>(r) << 0);
			break;
		case FORMAT_X8L8V8U8:
			*(unsigned int*)element = 0xFF000000 | (unorm<8>(b) << 16) | (snorm<8>(g) << 8) | (snorm<8>(r) << 0);
			break;
		case FORMAT_V16U16:
			*(unsigned int*)element = (snorm<16>(g) << 16) | (snorm<16>(r) << 0);
			break;
		case FORMAT_A2W10V10U10:
			*(unsigned int*)element = (unorm<2>(a) << 30) | (snorm<10>(b) << 20) | (snorm<10>(g) << 10) | (snorm<10>(r) << 0);
			break;
		case FORMAT_A16W16V16U16:
			((unsigned short*)element)[0] = snorm<16>(r);
			((unsigned short*)element)[1] = snorm<16>(g);
			((unsigned short*)element)[2] = snorm<16>(b);
			((unsigned short*)element)[3] = unorm<16>(a);
			break;
		case FORMAT_Q16W16V16U16:
			((unsigned short*)element)[0] = snorm<16>(r);
			((unsigned short*)element)[1] = snorm<16>(g);
			((unsigned short*)element)[2] = snorm<16>(b);
			((unsigned short*)element)[3] = snorm<16>(a);
			break;
		case FORMAT_R8G8B8:
			((unsigned char*)element)[0] = unorm<8>(b);
			((unsigned char*)element)[1] = unorm<8>(g);
			((unsigned char*)element)[2] = unorm<8>(r);
			break;
		case FORMAT_B8G8R8:
			((unsigned char*)element)[0] = unorm<8>(r);
			((unsigned char*)element)[1] = unorm<8>(g);
			((unsigned char*)element)[2] = unorm<8>(b);
			break;
		case FORMAT_R16F:
			*(half*)element = (half)r;
			break;
		case FORMAT_A16F:
			*(half*)element = (half)a;
			break;
		case FORMAT_G16R16F:
			((half*)element)[0] = (half)r;
			((half*)element)[1] = (half)g;
			break;
		case FORMAT_X16B16G16R16F_UNSIGNED:
			r = max(r, 0.0f); g = max(g, 0.0f); b = max(b, 0.0f);
			// Fall through to FORMAT_X16B16G16R16F.
		case FORMAT_X16B16G16R16F:
			((half*)element)[3] = 1.0f;
			// Fall through to FORMAT_B16G16R16F.
		case FORMAT_B16G16R16F:
			((half*)element)[0] = (half)r;
			((half*)element)[1] = (half)g;
			((half*)element)[2] = (half)b;
			break;
		case FORMAT_A16B16G16R16F:
			((half*)element)[0] = (half)r;
			((half*)element)[1] = (half)g;
			((half*)element)[2] = (half)b;
			((half*)element)[3] = (half)a;
			break;
		case FORMAT_A32F:
			*(float*)element = a;
			break;
		case FORMAT_R32F:
			*(float*)element = r;
			break;
		case FORMAT_G32R32F:
			((float*)element)[0] = r;
			((float*)element)[1] = g;
			break;
		case FORMAT_X32B32G32R32F_UNSIGNED:
			r = max(r, 0.0f); g = max(g, 0.0f); b = max(b, 0.0f);
			// Fall through to FORMAT_X32B32G32R32F.
		case FORMAT_X32B32G32R32F:
			((float*)element)[3] = 1.0f;
			// Fall through to FORMAT_B32G32R32F.
		case FORMAT_B32G32R32F:
			((float*)element)[0] = r;
			((float*)element)[1] = g;
			((float*)element)[2] = b;
			break;
		case FORMAT_A32B32G32R32F:
			((float*)element)[0] = r;
			((float*)element)[1] = g;
			((float*)element)[2] = b;
			((float*)element)[3] = a;
			break;
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
			*((float*)element) = r;
			break;
		case FORMAT_D32F_COMPLEMENTARY:
		case FORMAT_D32FS8_COMPLEMENTARY:
			*((float*)element) = 1 - r;
			break;
		case FORMAT_S8:
			*((unsigned char*)element) = unorm<8>(r);
			break;
		case FORMAT_L8:
			*(unsigned char*)element = unorm<8>(r);
			break;
		case FORMAT_A4L4:
			*(unsigned char*)element = (unorm<4>(a) << 4) | (unorm<4>(r) << 0);
			break;
		case FORMAT_L16:
			*(unsigned short*)element = unorm<16>(r);
			break;
		case FORMAT_A8L8:
			*(unsigned short*)element = (unorm<8>(a) << 8) | (unorm<8>(r) << 0);
			break;
		case FORMAT_L16F:
			*(half*)element = (half)r;
			break;
		case FORMAT_A16L16F:
			((half*)element)[0] = (half)r;
			((half*)element)[1] = (half)a;
			break;
		case FORMAT_L32F:
			*(float*)element = r;
			break;
		case FORMAT_A32L32F:
			((float*)element)[0] = r;
			((float*)element)[1] = a;
			break;
		default:
			ASSERT(false);
		}
	}

	Color<float> Surface::Buffer::read(int x, int y, int z) const
	{
		void *element = (unsigned char*)buffer + (x + border) * bytes + (y + border) * pitchB + z * samples * sliceB;

		return read(element);
	}

	Color<float> Surface::Buffer::read(int x, int y) const
	{
		void *element = (unsigned char*)buffer + (x + border) * bytes + (y + border) * pitchB;

		return read(element);
	}

	inline Color<float> Surface::Buffer::read(void *element) const
	{
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		float a = 1.0f;

		switch(format)
		{
		case FORMAT_P8:
			{
				ASSERT(palette);

				unsigned int abgr = palette[*(unsigned char*)element];

				r = (abgr & 0x000000FF) * (1.0f / 0x000000FF);
				g = (abgr & 0x0000FF00) * (1.0f / 0x0000FF00);
				b = (abgr & 0x00FF0000) * (1.0f / 0x00FF0000);
				a = (abgr & 0xFF000000) * (1.0f / 0xFF000000);
			}
			break;
		case FORMAT_A8P8:
			{
				ASSERT(palette);

				unsigned int bgr = palette[((unsigned char*)element)[0]];

				r = (bgr & 0x000000FF) * (1.0f / 0x000000FF);
				g = (bgr & 0x0000FF00) * (1.0f / 0x0000FF00);
				b = (bgr & 0x00FF0000) * (1.0f / 0x00FF0000);
				a = ((unsigned char*)element)[1] * (1.0f / 0xFF);
			}
			break;
		case FORMAT_A8:
			r = 0;
			g = 0;
			b = 0;
			a = *(unsigned char*)element * (1.0f / 0xFF);
			break;
		case FORMAT_R8_SNORM:
			r = max((*(signed char*)element) * (1.0f / 0x7F), -1.0f);
			break;
		case FORMAT_R8:
			r = *(unsigned char*)element * (1.0f / 0xFF);
			break;
		case FORMAT_R8I:
			r = *(signed char*)element;
			break;
		case FORMAT_R8UI:
			r = *(unsigned char*)element;
			break;
		case FORMAT_R3G3B2:
			{
				unsigned char rgb = *(unsigned char*)element;

				r = (rgb & 0xE0) * (1.0f / 0xE0);
				g = (rgb & 0x1C) * (1.0f / 0x1C);
				b = (rgb & 0x03) * (1.0f / 0x03);
			}
			break;
		case FORMAT_A8R3G3B2:
			{
				unsigned short argb = *(unsigned short*)element;

				a = (argb & 0xFF00) * (1.0f / 0xFF00);
				r = (argb & 0x00E0) * (1.0f / 0x00E0);
				g = (argb & 0x001C) * (1.0f / 0x001C);
				b = (argb & 0x0003) * (1.0f / 0x0003);
			}
			break;
		case FORMAT_X4R4G4B4:
			{
				unsigned short rgb = *(unsigned short*)element;

				r = (rgb & 0x0F00) * (1.0f / 0x0F00);
				g = (rgb & 0x00F0) * (1.0f / 0x00F0);
				b = (rgb & 0x000F) * (1.0f / 0x000F);
			}
			break;
		case FORMAT_A4R4G4B4:
			{
				unsigned short argb = *(unsigned short*)element;

				a = (argb & 0xF000) * (1.0f / 0xF000);
				r = (argb & 0x0F00) * (1.0f / 0x0F00);
				g = (argb & 0x00F0) * (1.0f / 0x00F0);
				b = (argb & 0x000F) * (1.0f / 0x000F);
			}
			break;
		case FORMAT_R4G4B4A4:
			{
				unsigned short rgba = *(unsigned short*)element;

				r = (rgba & 0xF000) * (1.0f / 0xF000);
				g = (rgba & 0x0F00) * (1.0f / 0x0F00);
				b = (rgba & 0x00F0) * (1.0f / 0x00F0);
				a = (rgba & 0x000F) * (1.0f / 0x000F);
			}
			break;
		case FORMAT_R5G6B5:
			{
				unsigned short rgb = *(unsigned short*)element;

				r = (rgb & 0xF800) * (1.0f / 0xF800);
				g = (rgb & 0x07E0) * (1.0f / 0x07E0);
				b = (rgb & 0x001F) * (1.0f / 0x001F);
			}
			break;
		case FORMAT_A1R5G5B5:
			{
				unsigned short argb = *(unsigned short*)element;

				a = (argb & 0x8000) * (1.0f / 0x8000);
				r = (argb & 0x7C00) * (1.0f / 0x7C00);
				g = (argb & 0x03E0) * (1.0f / 0x03E0);
				b = (argb & 0x001F) * (1.0f / 0x001F);
			}
			break;
		case FORMAT_R5G5B5A1:
			{
				unsigned short rgba = *(unsigned short*)element;

				r = (rgba & 0xF800) * (1.0f / 0xF800);
				g = (rgba & 0x07C0) * (1.0f / 0x07C0);
				b = (rgba & 0x003E) * (1.0f / 0x003E);
				a = (rgba & 0x0001) * (1.0f / 0x0001);
			}
			break;
		case FORMAT_X1R5G5B5:
			{
				unsigned short xrgb = *(unsigned short*)element;

				r = (xrgb & 0x7C00) * (1.0f / 0x7C00);
				g = (xrgb & 0x03E0) * (1.0f / 0x03E0);
				b = (xrgb & 0x001F) * (1.0f / 0x001F);
			}
			break;
		case FORMAT_A8R8G8B8:
			{
				unsigned int argb = *(unsigned int*)element;

				a = (argb & 0xFF000000) * (1.0f / 0xFF000000);
				r = (argb & 0x00FF0000) * (1.0f / 0x00FF0000);
				g = (argb & 0x0000FF00) * (1.0f / 0x0000FF00);
				b = (argb & 0x000000FF) * (1.0f / 0x000000FF);
			}
			break;
		case FORMAT_X8R8G8B8:
			{
				unsigned int xrgb = *(unsigned int*)element;

				r = (xrgb & 0x00FF0000) * (1.0f / 0x00FF0000);
				g = (xrgb & 0x0000FF00) * (1.0f / 0x0000FF00);
				b = (xrgb & 0x000000FF) * (1.0f / 0x000000FF);
			}
			break;
		case FORMAT_A8B8G8R8_SNORM:
			{
				signed char* abgr = (signed char*)element;

				r = max(abgr[0] * (1.0f / 0x7F), -1.0f);
				g = max(abgr[1] * (1.0f / 0x7F), -1.0f);
				b = max(abgr[2] * (1.0f / 0x7F), -1.0f);
				a = max(abgr[3] * (1.0f / 0x7F), -1.0f);
			}
			break;
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_A8:
			{
				unsigned int abgr = *(unsigned int*)element;

				a = (abgr & 0xFF000000) * (1.0f / 0xFF000000);
				b = (abgr & 0x00FF0000) * (1.0f / 0x00FF0000);
				g = (abgr & 0x0000FF00) * (1.0f / 0x0000FF00);
				r = (abgr & 0x000000FF) * (1.0f / 0x000000FF);
			}
			break;
		case FORMAT_A8B8G8R8I:
			{
				signed char* abgr = (signed char*)element;

				r = abgr[0];
				g = abgr[1];
				b = abgr[2];
				a = abgr[3];
			}
			break;
		case FORMAT_A8B8G8R8UI:
			{
				unsigned char* abgr = (unsigned char*)element;

				r = abgr[0];
				g = abgr[1];
				b = abgr[2];
				a = abgr[3];
			}
			break;
		case FORMAT_X8B8G8R8_SNORM:
			{
				signed char* bgr = (signed char*)element;

				r = max(bgr[0] * (1.0f / 0x7F), -1.0f);
				g = max(bgr[1] * (1.0f / 0x7F), -1.0f);
				b = max(bgr[2] * (1.0f / 0x7F), -1.0f);
			}
			break;
		case FORMAT_X8B8G8R8:
		case FORMAT_SRGB8_X8:
			{
				unsigned int xbgr = *(unsigned int*)element;

				b = (xbgr & 0x00FF0000) * (1.0f / 0x00FF0000);
				g = (xbgr & 0x0000FF00) * (1.0f / 0x0000FF00);
				r = (xbgr & 0x000000FF) * (1.0f / 0x000000FF);
			}
			break;
		case FORMAT_X8B8G8R8I:
			{
				signed char* bgr = (signed char*)element;

				r = bgr[0];
				g = bgr[1];
				b = bgr[2];
			}
			break;
		case FORMAT_X8B8G8R8UI:
			{
				unsigned char* bgr = (unsigned char*)element;

				r = bgr[0];
				g = bgr[1];
				b = bgr[2];
			}
			break;
		case FORMAT_G8R8_SNORM:
			{
				signed char* gr = (signed char*)element;

				r = (gr[0] & 0xFF00) * (1.0f / 0xFF00);
				g = (gr[1] & 0x00FF) * (1.0f / 0x00FF);
			}
			break;
		case FORMAT_G8R8:
			{
				unsigned short gr = *(unsigned short*)element;

				g = (gr & 0xFF00) * (1.0f / 0xFF00);
				r = (gr & 0x00FF) * (1.0f / 0x00FF);
			}
			break;
		case FORMAT_G8R8I:
			{
				signed char* gr = (signed char*)element;

				r = gr[0];
				g = gr[1];
			}
			break;
		case FORMAT_G8R8UI:
			{
				unsigned char* gr = (unsigned char*)element;

				r = gr[0];
				g = gr[1];
			}
			break;
		case FORMAT_R16I:
			r = *((short*)element);
			break;
		case FORMAT_R16UI:
			r = *((unsigned short*)element);
			break;
		case FORMAT_G16R16I:
			{
				short* gr = (short*)element;

				r = gr[0];
				g = gr[1];
			}
			break;
		case FORMAT_G16R16:
			{
				unsigned int gr = *(unsigned int*)element;

				g = (gr & 0xFFFF0000) * (1.0f / 0xFFFF0000);
				r = (gr & 0x0000FFFF) * (1.0f / 0x0000FFFF);
			}
			break;
		case FORMAT_G16R16UI:
			{
				unsigned short* gr = (unsigned short*)element;

				r = gr[0];
				g = gr[1];
			}
			break;
		case FORMAT_A2R10G10B10:
			{
				unsigned int argb = *(unsigned int*)element;

				a = (argb & 0xC0000000) * (1.0f / 0xC0000000);
				r = (argb & 0x3FF00000) * (1.0f / 0x3FF00000);
				g = (argb & 0x000FFC00) * (1.0f / 0x000FFC00);
				b = (argb & 0x000003FF) * (1.0f / 0x000003FF);
			}
			break;
		case FORMAT_A2B10G10R10:
			{
				unsigned int abgr = *(unsigned int*)element;

				a = (abgr & 0xC0000000) * (1.0f / 0xC0000000);
				b = (abgr & 0x3FF00000) * (1.0f / 0x3FF00000);
				g = (abgr & 0x000FFC00) * (1.0f / 0x000FFC00);
				r = (abgr & 0x000003FF) * (1.0f / 0x000003FF);
			}
			break;
		case FORMAT_A2B10G10R10UI:
			{
				unsigned int abgr = *(unsigned int*)element;

				a = static_cast<float>((abgr & 0xC0000000) >> 30);
				b = static_cast<float>((abgr & 0x3FF00000) >> 20);
				g = static_cast<float>((abgr & 0x000FFC00) >> 10);
				r = static_cast<float>(abgr & 0x000003FF);
			}
			break;
		case FORMAT_A16B16G16R16I:
			{
				short* abgr = (short*)element;

				r = abgr[0];
				g = abgr[1];
				b = abgr[2];
				a = abgr[3];
			}
			break;
		case FORMAT_A16B16G16R16:
			r = ((unsigned short*)element)[0] * (1.0f / 0xFFFF);
			g = ((unsigned short*)element)[1] * (1.0f / 0xFFFF);
			b = ((unsigned short*)element)[2] * (1.0f / 0xFFFF);
			a = ((unsigned short*)element)[3] * (1.0f / 0xFFFF);
			break;
		case FORMAT_A16B16G16R16UI:
			{
				unsigned short* abgr = (unsigned short*)element;

				r = abgr[0];
				g = abgr[1];
				b = abgr[2];
				a = abgr[3];
			}
			break;
		case FORMAT_X16B16G16R16I:
			{
				short* bgr = (short*)element;

				r = bgr[0];
				g = bgr[1];
				b = bgr[2];
			}
			break;
		case FORMAT_X16B16G16R16UI:
			{
				unsigned short* bgr = (unsigned short*)element;

				r = bgr[0];
				g = bgr[1];
				b = bgr[2];
			}
			break;
		case FORMAT_A32B32G32R32I:
			{
				int* abgr = (int*)element;

				r = static_cast<float>(abgr[0]);
				g = static_cast<float>(abgr[1]);
				b = static_cast<float>(abgr[2]);
				a = static_cast<float>(abgr[3]);
			}
			break;
		case FORMAT_A32B32G32R32UI:
			{
				unsigned int* abgr = (unsigned int*)element;

				r = static_cast<float>(abgr[0]);
				g = static_cast<float>(abgr[1]);
				b = static_cast<float>(abgr[2]);
				a = static_cast<float>(abgr[3]);
			}
			break;
		case FORMAT_X32B32G32R32I:
			{
				int* bgr = (int*)element;

				r = static_cast<float>(bgr[0]);
				g = static_cast<float>(bgr[1]);
				b = static_cast<float>(bgr[2]);
			}
			break;
		case FORMAT_X32B32G32R32UI:
			{
				unsigned int* bgr = (unsigned int*)element;

				r = static_cast<float>(bgr[0]);
				g = static_cast<float>(bgr[1]);
				b = static_cast<float>(bgr[2]);
			}
			break;
		case FORMAT_G32R32I:
			{
				int* gr = (int*)element;

				r = static_cast<float>(gr[0]);
				g = static_cast<float>(gr[1]);
			}
			break;
		case FORMAT_G32R32UI:
			{
				unsigned int* gr = (unsigned int*)element;

				r = static_cast<float>(gr[0]);
				g = static_cast<float>(gr[1]);
			}
			break;
		case FORMAT_R32I:
			r = static_cast<float>(*((int*)element));
			break;
		case FORMAT_R32UI:
			r = static_cast<float>(*((unsigned int*)element));
			break;
		case FORMAT_V8U8:
			{
				unsigned short vu = *(unsigned short*)element;

				r = ((int)(vu & 0x00FF) << 24) * (1.0f / 0x7F000000);
				g = ((int)(vu & 0xFF00) << 16) * (1.0f / 0x7F000000);
			}
			break;
		case FORMAT_L6V5U5:
			{
				unsigned short lvu = *(unsigned short*)element;

				r = ((int)(lvu & 0x001F) << 27) * (1.0f / 0x78000000);
				g = ((int)(lvu & 0x03E0) << 22) * (1.0f / 0x78000000);
				b = (lvu & 0xFC00) * (1.0f / 0xFC00);
			}
			break;
		case FORMAT_Q8W8V8U8:
			{
				unsigned int qwvu = *(unsigned int*)element;

				r = ((int)(qwvu & 0x000000FF) << 24) * (1.0f / 0x7F000000);
				g = ((int)(qwvu & 0x0000FF00) << 16) * (1.0f / 0x7F000000);
				b = ((int)(qwvu & 0x00FF0000) << 8)  * (1.0f / 0x7F000000);
				a = ((int)(qwvu & 0xFF000000) << 0)  * (1.0f / 0x7F000000);
			}
			break;
		case FORMAT_X8L8V8U8:
			{
				unsigned int xlvu = *(unsigned int*)element;

				r = ((int)(xlvu & 0x000000FF) << 24) * (1.0f / 0x7F000000);
				g = ((int)(xlvu & 0x0000FF00) << 16) * (1.0f / 0x7F000000);
				b = (xlvu & 0x00FF0000) * (1.0f / 0x00FF0000);
			}
			break;
		case FORMAT_R8G8B8:
			r = ((unsigned char*)element)[2] * (1.0f / 0xFF);
			g = ((unsigned char*)element)[1] * (1.0f / 0xFF);
			b = ((unsigned char*)element)[0] * (1.0f / 0xFF);
			break;
		case FORMAT_B8G8R8:
			r = ((unsigned char*)element)[0] * (1.0f / 0xFF);
			g = ((unsigned char*)element)[1] * (1.0f / 0xFF);
			b = ((unsigned char*)element)[2] * (1.0f / 0xFF);
			break;
		case FORMAT_V16U16:
			{
				unsigned int vu = *(unsigned int*)element;

				r = ((int)(vu & 0x0000FFFF) << 16) * (1.0f / 0x7FFF0000);
				g = ((int)(vu & 0xFFFF0000) << 0)  * (1.0f / 0x7FFF0000);
			}
			break;
		case FORMAT_A2W10V10U10:
			{
				unsigned int awvu = *(unsigned int*)element;

				r = ((int)(awvu & 0x000003FF) << 22) * (1.0f / 0x7FC00000);
				g = ((int)(awvu & 0x000FFC00) << 12) * (1.0f / 0x7FC00000);
				b = ((int)(awvu & 0x3FF00000) << 2)  * (1.0f / 0x7FC00000);
				a = (awvu & 0xC0000000) * (1.0f / 0xC0000000);
			}
			break;
		case FORMAT_A16W16V16U16:
			r = ((signed short*)element)[0] * (1.0f / 0x7FFF);
			g = ((signed short*)element)[1] * (1.0f / 0x7FFF);
			b = ((signed short*)element)[2] * (1.0f / 0x7FFF);
			a = ((unsigned short*)element)[3] * (1.0f / 0xFFFF);
			break;
		case FORMAT_Q16W16V16U16:
			r = ((signed short*)element)[0] * (1.0f / 0x7FFF);
			g = ((signed short*)element)[1] * (1.0f / 0x7FFF);
			b = ((signed short*)element)[2] * (1.0f / 0x7FFF);
			a = ((signed short*)element)[3] * (1.0f / 0x7FFF);
			break;
		case FORMAT_L8:
			r =
			g =
			b = *(unsigned char*)element * (1.0f / 0xFF);
			break;
		case FORMAT_A4L4:
			{
				unsigned char al = *(unsigned char*)element;

				r =
				g =
				b = (al & 0x0F) * (1.0f / 0x0F);
				a = (al & 0xF0) * (1.0f / 0xF0);
			}
			break;
		case FORMAT_L16:
			r =
			g =
			b = *(unsigned short*)element * (1.0f / 0xFFFF);
			break;
		case FORMAT_A8L8:
			r =
			g =
			b = ((unsigned char*)element)[0] * (1.0f / 0xFF);
			a = ((unsigned char*)element)[1] * (1.0f / 0xFF);
			break;
		case FORMAT_L16F:
			r =
			g =
			b = *(half*)element;
			break;
		case FORMAT_A16L16F:
			r =
			g =
			b = ((half*)element)[0];
			a = ((half*)element)[1];
			break;
		case FORMAT_L32F:
			r =
			g =
			b = *(float*)element;
			break;
		case FORMAT_A32L32F:
			r =
			g =
			b = ((float*)element)[0];
			a = ((float*)element)[1];
			break;
		case FORMAT_A16F:
			a = *(half*)element;
			break;
		case FORMAT_R16F:
			r = *(half*)element;
			break;
		case FORMAT_G16R16F:
			r = ((half*)element)[0];
			g = ((half*)element)[1];
			break;
		case FORMAT_X16B16G16R16F:
		case FORMAT_X16B16G16R16F_UNSIGNED:
		case FORMAT_B16G16R16F:
			r = ((half*)element)[0];
			g = ((half*)element)[1];
			b = ((half*)element)[2];
			break;
		case FORMAT_A16B16G16R16F:
			r = ((half*)element)[0];
			g = ((half*)element)[1];
			b = ((half*)element)[2];
			a = ((half*)element)[3];
			break;
		case FORMAT_A32F:
			a = *(float*)element;
			break;
		case FORMAT_R32F:
			r = *(float*)element;
			break;
		case FORMAT_G32R32F:
			r = ((float*)element)[0];
			g = ((float*)element)[1];
			break;
		case FORMAT_X32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_B32G32R32F:
			r = ((float*)element)[0];
			g = ((float*)element)[1];
			b = ((float*)element)[2];
			break;
		case FORMAT_A32B32G32R32F:
			r = ((float*)element)[0];
			g = ((float*)element)[1];
			b = ((float*)element)[2];
			a = ((float*)element)[3];
			break;
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
			r = *(float*)element;
			g = r;
			b = r;
			a = r;
			break;
		case FORMAT_D32F_COMPLEMENTARY:
		case FORMAT_D32FS8_COMPLEMENTARY:
			r = 1.0f - *(float*)element;
			g = r;
			b = r;
			a = r;
			break;
		case FORMAT_S8:
			r = *(unsigned char*)element * (1.0f / 0xFF);
			break;
		default:
			ASSERT(false);
		}

		if(isSRGBformat(format))
		{
			r = sRGBtoLinear(r);
			g = sRGBtoLinear(g);
			b = sRGBtoLinear(b);
		}

		return Color<float>(r, g, b, a);
	}

	Color<float> Surface::Buffer::sample(float x, float y, float z) const
	{
		x -= 0.5f;
		y -= 0.5f;
		z -= 0.5f;

		int x0 = clamp((int)x, 0, width - 1);
		int x1 = (x0 + 1 >= width) ? x0 : x0 + 1;

		int y0 = clamp((int)y, 0, height - 1);
		int y1 = (y0 + 1 >= height) ? y0 : y0 + 1;

		int z0 = clamp((int)z, 0, depth - 1);
		int z1 = (z0 + 1 >= depth) ? z0 : z0 + 1;

		Color<float> c000 = read(x0, y0, z0);
		Color<float> c100 = read(x1, y0, z0);
		Color<float> c010 = read(x0, y1, z0);
		Color<float> c110 = read(x1, y1, z0);
		Color<float> c001 = read(x0, y0, z1);
		Color<float> c101 = read(x1, y0, z1);
		Color<float> c011 = read(x0, y1, z1);
		Color<float> c111 = read(x1, y1, z1);

		float fx = x - x0;
		float fy = y - y0;
		float fz = z - z0;

		c000 *= (1 - fx) * (1 - fy) * (1 - fz);
		c100 *= fx * (1 - fy) * (1 - fz);
		c010 *= (1 - fx) * fy * (1 - fz);
		c110 *= fx * fy * (1 - fz);
		c001 *= (1 - fx) * (1 - fy) * fz;
		c101 *= fx * (1 - fy) * fz;
		c011 *= (1 - fx) * fy * fz;
		c111 *= fx * fy * fz;

		return c000 + c100 + c010 + c110 + c001 + c101 + c011 + c111;
	}

	Color<float> Surface::Buffer::sample(float x, float y, int layer) const
	{
		x -= 0.5f;
		y -= 0.5f;

		int x0 = clamp((int)x, 0, width - 1);
		int x1 = (x0 + 1 >= width) ? x0 : x0 + 1;

		int y0 = clamp((int)y, 0, height - 1);
		int y1 = (y0 + 1 >= height) ? y0 : y0 + 1;

		Color<float> c00 = read(x0, y0, layer);
		Color<float> c10 = read(x1, y0, layer);
		Color<float> c01 = read(x0, y1, layer);
		Color<float> c11 = read(x1, y1, layer);

		float fx = x - x0;
		float fy = y - y0;

		c00 *= (1 - fx) * (1 - fy);
		c10 *= fx * (1 - fy);
		c01 *= (1 - fx) * fy;
		c11 *= fx * fy;

		return c00 + c10 + c01 + c11;
	}

	void *Surface::Buffer::lockRect(int x, int y, int z, Lock lock)
	{
		this->lock = lock;

		switch(lock)
		{
		case LOCK_UNLOCKED:
		case LOCK_READONLY:
		case LOCK_UPDATE:
			break;
		case LOCK_WRITEONLY:
		case LOCK_READWRITE:
		case LOCK_DISCARD:
			dirty = true;
			break;
		default:
			ASSERT(false);
		}

		if(buffer)
		{
			x += border;
			y += border;

			switch(format)
			{
			case FORMAT_DXT1:
			case FORMAT_ATI1:
			case FORMAT_ETC1:
			case FORMAT_R11_EAC:
			case FORMAT_SIGNED_R11_EAC:
			case FORMAT_RGB8_ETC2:
			case FORMAT_SRGB8_ETC2:
			case FORMAT_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
			case FORMAT_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
				return (unsigned char*)buffer + 8 * (x / 4) + (y / 4) * pitchB + z * sliceB;
			case FORMAT_RG11_EAC:
			case FORMAT_SIGNED_RG11_EAC:
			case FORMAT_RGBA8_ETC2_EAC:
			case FORMAT_SRGB8_ALPHA8_ETC2_EAC:
			case FORMAT_RGBA_ASTC_4x4_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_4x4_KHR:
				return (unsigned char*)buffer + 16 * (x / 4) + (y / 4) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_5x4_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_5x4_KHR:
				return (unsigned char*)buffer + 16 * (x / 5) + (y / 4) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_5x5_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_5x5_KHR:
				return (unsigned char*)buffer + 16 * (x / 5) + (y / 5) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_6x5_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_6x5_KHR:
				return (unsigned char*)buffer + 16 * (x / 6) + (y / 5) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_6x6_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_6x6_KHR:
				return (unsigned char*)buffer + 16 * (x / 6) + (y / 6) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_8x5_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_8x5_KHR:
				return (unsigned char*)buffer + 16 * (x / 8) + (y / 5) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_8x6_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_8x6_KHR:
				return (unsigned char*)buffer + 16 * (x / 8) + (y / 6) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_8x8_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_8x8_KHR:
				return (unsigned char*)buffer + 16 * (x / 8) + (y / 8) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_10x5_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_10x5_KHR:
				return (unsigned char*)buffer + 16 * (x / 10) + (y / 5) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_10x6_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_10x6_KHR:
				return (unsigned char*)buffer + 16 * (x / 10) + (y / 6) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_10x8_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_10x8_KHR:
				return (unsigned char*)buffer + 16 * (x / 10) + (y / 8) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_10x10_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_10x10_KHR:
				return (unsigned char*)buffer + 16 * (x / 10) + (y / 10) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_12x10_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_12x10_KHR:
				return (unsigned char*)buffer + 16 * (x / 12) + (y / 10) * pitchB + z * sliceB;
			case FORMAT_RGBA_ASTC_12x12_KHR:
			case FORMAT_SRGB8_ALPHA8_ASTC_12x12_KHR:
				return (unsigned char*)buffer + 16 * (x / 12) + (y / 12) * pitchB + z * sliceB;
			case FORMAT_DXT3:
			case FORMAT_DXT5:
			case FORMAT_ATI2:
				return (unsigned char*)buffer + 16 * (x / 4) + (y / 4) * pitchB + z * sliceB;
			default:
				return (unsigned char*)buffer + x * bytes + y * pitchB + z * samples * sliceB;
			}
		}

		return nullptr;
	}

	void Surface::Buffer::unlockRect()
	{
		lock = LOCK_UNLOCKED;
	}

	class SurfaceImplementation : public Surface
	{
	public:
		SurfaceImplementation(int width, int height, int depth, Format format, void *pixels, int pitch, int slice)
			: Surface(width, height, depth, format, pixels, pitch, slice) {}
		SurfaceImplementation(Resource *texture, int width, int height, int depth, int border, int samples, Format format, bool lockable, bool renderTarget, int pitchP = 0)
			: Surface(texture, width, height, depth, border, samples, format, lockable, renderTarget, pitchP) {}
		~SurfaceImplementation() override {};

		void *lockInternal(int x, int y, int z, Lock lock, Accessor client) override
		{
			return Surface::lockInternal(x, y, z, lock, client);
		}

		void unlockInternal() override
		{
			Surface::unlockInternal();
		}
	};

	Surface *Surface::create(int width, int height, int depth, Format format, void *pixels, int pitch, int slice)
	{
		return new SurfaceImplementation(width, height, depth, format, pixels, pitch, slice);
	}

	Surface *Surface::create(Resource *texture, int width, int height, int depth, int border, int samples, Format format, bool lockable, bool renderTarget, int pitchPprovided)
	{
		return new SurfaceImplementation(texture, width, height, depth, border, samples, format, lockable, renderTarget, pitchPprovided);
	}

	Surface::Surface(int width, int height, int depth, Format format, void *pixels, int pitch, int slice) : lockable(true), renderTarget(false)
	{
		resource = new Resource(0);
		hasParent = false;
		ownExternal = false;
		depth = max(1, depth);

		external.buffer = pixels;
		external.width = width;
		external.height = height;
		external.depth = depth;
		external.samples = 1;
		external.format = format;
		external.bytes = bytes(external.format);
		external.pitchB = pitch;
		external.pitchP = external.bytes ? pitch / external.bytes : 0;
		external.sliceB = slice;
		external.sliceP = external.bytes ? slice / external.bytes : 0;
		external.border = 0;
		external.lock = LOCK_UNLOCKED;
		external.dirty = true;

		internal.buffer = nullptr;
		internal.width = width;
		internal.height = height;
		internal.depth = depth;
		internal.samples = 1;
		internal.format = selectInternalFormat(format);
		internal.bytes = bytes(internal.format);
		internal.pitchB = pitchB(internal.width, 0, internal.format, false);
		internal.pitchP = pitchP(internal.width, 0, internal.format, false);
		internal.sliceB = sliceB(internal.width, internal.height, 0, internal.format, false);
		internal.sliceP = sliceP(internal.width, internal.height, 0, internal.format, false);
		internal.border = 0;
		internal.lock = LOCK_UNLOCKED;
		internal.dirty = false;

		stencil.buffer = nullptr;
		stencil.width = width;
		stencil.height = height;
		stencil.depth = depth;
		stencil.samples = 1;
		stencil.format = isStencil(format) ? FORMAT_S8 : FORMAT_NULL;
		stencil.bytes = bytes(stencil.format);
		stencil.pitchB = pitchB(stencil.width, 0, stencil.format, false);
		stencil.pitchP = pitchP(stencil.width, 0, stencil.format, false);
		stencil.sliceB = sliceB(stencil.width, stencil.height, 0, stencil.format, false);
		stencil.sliceP = sliceP(stencil.width, stencil.height, 0, stencil.format, false);
		stencil.border = 0;
		stencil.lock = LOCK_UNLOCKED;
		stencil.dirty = false;

		dirtyContents = true;
		paletteUsed = 0;
	}

	Surface::Surface(Resource *texture, int width, int height, int depth, int border, int samples, Format format, bool lockable, bool renderTarget, int pitchPprovided) : lockable(lockable), renderTarget(renderTarget)
	{
		resource = texture ? texture : new Resource(0);
		hasParent = texture != nullptr;
		ownExternal = true;
		depth = max(1, depth);
		samples = max(1, samples);

		external.buffer = nullptr;
		external.width = width;
		external.height = height;
		external.depth = depth;
		external.samples = (short)samples;
		external.format = format;
		external.bytes = bytes(external.format);
		external.pitchB = !pitchPprovided ? pitchB(external.width, 0, external.format, renderTarget && !texture) : pitchPprovided * external.bytes;
		external.pitchP = !pitchPprovided ? pitchP(external.width, 0, external.format, renderTarget && !texture) : pitchPprovided;
		external.sliceB = sliceB(external.width, external.height, 0, external.format, renderTarget && !texture);
		external.sliceP = sliceP(external.width, external.height, 0, external.format, renderTarget && !texture);
		external.border = 0;
		external.lock = LOCK_UNLOCKED;
		external.dirty = false;

		internal.buffer = nullptr;
		internal.width = width;
		internal.height = height;
		internal.depth = depth;
		internal.samples = (short)samples;
		internal.format = selectInternalFormat(format);
		internal.bytes = bytes(internal.format);
		internal.pitchB = !pitchPprovided ? pitchB(internal.width, border, internal.format, renderTarget) : pitchPprovided * internal.bytes;
		internal.pitchP = !pitchPprovided ? pitchP(internal.width, border, internal.format, renderTarget) : pitchPprovided;
		internal.sliceB = sliceB(internal.width, internal.height, border, internal.format, renderTarget);
		internal.sliceP = sliceP(internal.width, internal.height, border, internal.format, renderTarget);
		internal.border = (short)border;
		internal.lock = LOCK_UNLOCKED;
		internal.dirty = false;

		stencil.buffer = nullptr;
		stencil.width = width;
		stencil.height = height;
		stencil.depth = depth;
		stencil.samples = (short)samples;
		stencil.format = isStencil(format) ? FORMAT_S8 : FORMAT_NULL;
		stencil.bytes = bytes(stencil.format);
		stencil.pitchB = pitchB(stencil.width, 0, stencil.format, renderTarget);
		stencil.pitchP = pitchP(stencil.width, 0, stencil.format, renderTarget);
		stencil.sliceB = sliceB(stencil.width, stencil.height, 0, stencil.format, renderTarget);
		stencil.sliceP = sliceP(stencil.width, stencil.height, 0, stencil.format, renderTarget);
		stencil.border = 0;
		stencil.lock = LOCK_UNLOCKED;
		stencil.dirty = false;

		dirtyContents = true;
		paletteUsed = 0;
	}

	Surface::~Surface()
	{
		// sync() must be called before this destructor to ensure all locks have been released.
		// We can't call it here because the parent resource may already have been destroyed.
		ASSERT(isUnlocked());

		if(!hasParent)
		{
			resource->destruct();
		}

		if(ownExternal)
		{
			deallocate(external.buffer);
		}

		if(internal.buffer != external.buffer)
		{
			deallocate(internal.buffer);
		}

		deallocate(stencil.buffer);

		external.buffer = 0;
		internal.buffer = 0;
		stencil.buffer = 0;
	}

	void *Surface::lockExternal(int x, int y, int z, Lock lock, Accessor client)
	{
		resource->lock(client);

		if(!external.buffer)
		{
			if(internal.buffer && identicalFormats())
			{
				external.buffer = internal.buffer;
			}
			else
			{
				external.buffer = allocateBuffer(external.width, external.height, external.depth, external.border, external.samples, external.format);
			}
		}

		if(internal.dirty)
		{
			if(lock != LOCK_DISCARD)
			{
				update(external, internal);
			}

			internal.dirty = false;
		}

		switch(lock)
		{
		case LOCK_READONLY:
			break;
		case LOCK_WRITEONLY:
		case LOCK_READWRITE:
		case LOCK_DISCARD:
			dirtyContents = true;
			break;
		default:
			ASSERT(false);
		}

		return external.lockRect(x, y, z, lock);
	}

	void Surface::unlockExternal()
	{
		external.unlockRect();

		resource->unlock();
	}

	void *Surface::lockInternal(int x, int y, int z, Lock lock, Accessor client)
	{
		if(lock != LOCK_UNLOCKED)
		{
			resource->lock(client);
		}

		if(!internal.buffer)
		{
			if(external.buffer && identicalFormats())
			{
				internal.buffer = external.buffer;
			}
			else
			{
				internal.buffer = allocateBuffer(internal.width, internal.height, internal.depth, internal.border, internal.samples, internal.format);
			}
		}

		// FIXME: WHQL requires conversion to lower external precision and back
		if(logPrecision >= WHQL)
		{
			if(internal.dirty && renderTarget && internal.format != external.format)
			{
				if(lock != LOCK_DISCARD)
				{
					switch(external.format)
					{
					case FORMAT_R3G3B2:
					case FORMAT_A8R3G3B2:
					case FORMAT_A1R5G5B5:
					case FORMAT_A2R10G10B10:
					case FORMAT_A2B10G10R10:
						lockExternal(0, 0, 0, LOCK_READWRITE, client);
						unlockExternal();
						break;
					default:
						// Difference passes WHQL
						break;
					}
				}
			}
		}

		if(external.dirty || (isPalette(external.format) && paletteUsed != Surface::paletteID))
		{
			if(lock != LOCK_DISCARD)
			{
				update(internal, external);
			}

			external.dirty = false;
			paletteUsed = Surface::paletteID;
		}

		switch(lock)
		{
		case LOCK_UNLOCKED:
		case LOCK_READONLY:
			break;
		case LOCK_WRITEONLY:
		case LOCK_READWRITE:
		case LOCK_DISCARD:
			dirtyContents = true;
			break;
		default:
			ASSERT(false);
		}

		if(lock == LOCK_READONLY && client == PUBLIC)
		{
			resolve();
		}

		return internal.lockRect(x, y, z, lock);
	}

	void Surface::unlockInternal()
	{
		internal.unlockRect();

		resource->unlock();
	}

	void *Surface::lockStencil(int x, int y, int front, Accessor client)
	{
		if(stencil.format == FORMAT_NULL)
		{
			return nullptr;
		}

		resource->lock(client);

		if(!stencil.buffer)
		{
			stencil.buffer = allocateBuffer(stencil.width, stencil.height, stencil.depth, stencil.border, stencil.samples, stencil.format);
		}

		return stencil.lockRect(x, y, front, LOCK_READWRITE);   // FIXME
	}

	void Surface::unlockStencil()
	{
		stencil.unlockRect();

		resource->unlock();
	}

	int Surface::bytes(Format format)
	{
		switch(format)
		{
		case FORMAT_NULL:				return 0;
		case FORMAT_P8:					return 1;
		case FORMAT_A8P8:				return 2;
		case FORMAT_A8:					return 1;
		case FORMAT_R8I:				return 1;
		case FORMAT_R8:					return 1;
		case FORMAT_R3G3B2:				return 1;
		case FORMAT_R16I:				return 2;
		case FORMAT_R16UI:				return 2;
		case FORMAT_A8R3G3B2:			return 2;
		case FORMAT_R5G6B5:				return 2;
		case FORMAT_A1R5G5B5:			return 2;
		case FORMAT_X1R5G5B5:			return 2;
		case FORMAT_R5G5B5A1:           return 2;
		case FORMAT_X4R4G4B4:			return 2;
		case FORMAT_A4R4G4B4:			return 2;
		case FORMAT_R4G4B4A4:           return 2;
		case FORMAT_R8G8B8:				return 3;
		case FORMAT_B8G8R8:             return 3;
		case FORMAT_R32I:				return 4;
		case FORMAT_R32UI:				return 4;
		case FORMAT_X8R8G8B8:			return 4;
	//	case FORMAT_X8G8R8B8Q:			return 4;
		case FORMAT_A8R8G8B8:			return 4;
	//	case FORMAT_A8G8R8B8Q:			return 4;
		case FORMAT_X8B8G8R8I:			return 4;
		case FORMAT_X8B8G8R8:			return 4;
		case FORMAT_SRGB8_X8:			return 4;
		case FORMAT_SRGB8_A8:			return 4;
		case FORMAT_A8B8G8R8I:			return 4;
		case FORMAT_R8UI:				return 1;
		case FORMAT_G8R8UI:				return 2;
		case FORMAT_X8B8G8R8UI:			return 4;
		case FORMAT_A8B8G8R8UI:			return 4;
		case FORMAT_A8B8G8R8:			return 4;
		case FORMAT_R8_SNORM:			return 1;
		case FORMAT_G8R8_SNORM:		return 2;
		case FORMAT_X8B8G8R8_SNORM:	return 4;
		case FORMAT_A8B8G8R8_SNORM:	return 4;
		case FORMAT_A2R10G10B10:		return 4;
		case FORMAT_A2B10G10R10:		return 4;
		case FORMAT_A2B10G10R10UI:		return 4;
		case FORMAT_G8R8I:				return 2;
		case FORMAT_G8R8:				return 2;
		case FORMAT_G16R16I:			return 4;
		case FORMAT_G16R16UI:			return 4;
		case FORMAT_G16R16:				return 4;
		case FORMAT_G32R32I:			return 8;
		case FORMAT_G32R32UI:			return 8;
		case FORMAT_X16B16G16R16I:		return 8;
		case FORMAT_X16B16G16R16UI:		return 8;
		case FORMAT_A16B16G16R16I:		return 8;
		case FORMAT_A16B16G16R16UI:		return 8;
		case FORMAT_A16B16G16R16:		return 8;
		case FORMAT_X32B32G32R32I:		return 16;
		case FORMAT_X32B32G32R32UI:		return 16;
		case FORMAT_A32B32G32R32I:		return 16;
		case FORMAT_A32B32G32R32UI:		return 16;
		// Compressed formats
		case FORMAT_DXT1:				return 2;   // Column of four pixels
		case FORMAT_DXT3:				return 4;   // Column of four pixels
		case FORMAT_DXT5:				return 4;   // Column of four pixels
		case FORMAT_ATI1:				return 2;   // Column of four pixels
		case FORMAT_ATI2:				return 4;   // Column of four pixels
		case FORMAT_ETC1:				return 2;   // Column of four pixels
		case FORMAT_R11_EAC:			return 2;
		case FORMAT_SIGNED_R11_EAC:		return 2;
		case FORMAT_RG11_EAC:			return 4;
		case FORMAT_SIGNED_RG11_EAC:	return 4;
		case FORMAT_RGB8_ETC2:			return 2;
		case FORMAT_SRGB8_ETC2:			return 2;
		case FORMAT_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:	return 2;
		case FORMAT_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:	return 2;
		case FORMAT_RGBA8_ETC2_EAC:			return 4;
		case FORMAT_SRGB8_ALPHA8_ETC2_EAC:	return 4;
		case FORMAT_RGBA_ASTC_4x4_KHR:
		case FORMAT_RGBA_ASTC_5x4_KHR:
		case FORMAT_RGBA_ASTC_5x5_KHR:
		case FORMAT_RGBA_ASTC_6x5_KHR:
		case FORMAT_RGBA_ASTC_6x6_KHR:
		case FORMAT_RGBA_ASTC_8x5_KHR:
		case FORMAT_RGBA_ASTC_8x6_KHR:
		case FORMAT_RGBA_ASTC_8x8_KHR:
		case FORMAT_RGBA_ASTC_10x5_KHR:
		case FORMAT_RGBA_ASTC_10x6_KHR:
		case FORMAT_RGBA_ASTC_10x8_KHR:
		case FORMAT_RGBA_ASTC_10x10_KHR:
		case FORMAT_RGBA_ASTC_12x10_KHR:
		case FORMAT_RGBA_ASTC_12x12_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_4x4_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_5x4_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_5x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_6x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_6x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x8_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x8_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x10_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_12x10_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_12x12_KHR: return 0; // FIXME
		// Bumpmap formats
		case FORMAT_V8U8:				return 2;
		case FORMAT_L6V5U5:				return 2;
		case FORMAT_Q8W8V8U8:			return 4;
		case FORMAT_X8L8V8U8:			return 4;
		case FORMAT_A2W10V10U10:		return 4;
		case FORMAT_V16U16:				return 4;
		case FORMAT_A16W16V16U16:		return 8;
		case FORMAT_Q16W16V16U16:		return 8;
		// Luminance formats
		case FORMAT_L8:					return 1;
		case FORMAT_A4L4:				return 1;
		case FORMAT_L16:				return 2;
		case FORMAT_A8L8:				return 2;
		case FORMAT_L16F:               return 2;
		case FORMAT_A16L16F:            return 4;
		case FORMAT_L32F:               return 4;
		case FORMAT_A32L32F:            return 8;
		// Floating-point formats
		case FORMAT_A16F:				return 2;
		case FORMAT_R16F:				return 2;
		case FORMAT_G16R16F:			return 4;
		case FORMAT_B16G16R16F:			return 6;
		case FORMAT_X16B16G16R16F:		return 8;
		case FORMAT_A16B16G16R16F:		return 8;
		case FORMAT_X16B16G16R16F_UNSIGNED: return 8;
		case FORMAT_A32F:				return 4;
		case FORMAT_R32F:				return 4;
		case FORMAT_G32R32F:			return 8;
		case FORMAT_B32G32R32F:			return 12;
		case FORMAT_X32B32G32R32F:		return 16;
		case FORMAT_A32B32G32R32F:		return 16;
		case FORMAT_X32B32G32R32F_UNSIGNED: return 16;
		// Depth/stencil formats
		case FORMAT_D16:				return 2;
		case FORMAT_D32:				return 4;
		case FORMAT_D24X8:				return 4;
		case FORMAT_D24S8:				return 4;
		case FORMAT_D24FS8:				return 4;
		case FORMAT_D32F:				return 4;
		case FORMAT_D32FS8:				return 4;
		case FORMAT_D32F_COMPLEMENTARY:	return 4;
		case FORMAT_D32FS8_COMPLEMENTARY: return 4;
		case FORMAT_D32F_LOCKABLE:		return 4;
		case FORMAT_D32FS8_TEXTURE:		return 4;
		case FORMAT_D32F_SHADOW:		return 4;
		case FORMAT_D32FS8_SHADOW:		return 4;
		case FORMAT_DF24S8:				return 4;
		case FORMAT_DF16S8:				return 2;
		case FORMAT_INTZ:				return 4;
		case FORMAT_S8:					return 1;
		case FORMAT_YV12_BT601:         return 1;   // Y plane only
		case FORMAT_YV12_BT709:         return 1;   // Y plane only
		case FORMAT_YV12_JFIF:          return 1;   // Y plane only
		default:
			ASSERT(false);
		}

		return 0;
	}

	int Surface::pitchB(int width, int border, Format format, bool target)
	{
		width += 2 * border;

		// Render targets require 2x2 quads
		if(target || isDepth(format) || isStencil(format))
		{
			width = align<2>(width);
		}

		switch(format)
		{
		case FORMAT_DXT1:
		case FORMAT_ETC1:
		case FORMAT_R11_EAC:
		case FORMAT_SIGNED_R11_EAC:
		case FORMAT_RGB8_ETC2:
		case FORMAT_SRGB8_ETC2:
		case FORMAT_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		case FORMAT_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
			return 8 * ((width + 3) / 4);    // 64 bit per 4x4 block, computed per 4 rows
		case FORMAT_RG11_EAC:
		case FORMAT_SIGNED_RG11_EAC:
		case FORMAT_RGBA8_ETC2_EAC:
		case FORMAT_SRGB8_ALPHA8_ETC2_EAC:
		case FORMAT_RGBA_ASTC_4x4_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_4x4_KHR:
			return 16 * ((width + 3) / 4);    // 128 bit per 4x4 block, computed per 4 rows
		case FORMAT_RGBA_ASTC_5x4_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_5x4_KHR:
		case FORMAT_RGBA_ASTC_5x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_5x5_KHR:
			return 16 * ((width + 4) / 5);
		case FORMAT_RGBA_ASTC_6x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_6x5_KHR:
		case FORMAT_RGBA_ASTC_6x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_6x6_KHR:
			return 16 * ((width + 5) / 6);
		case FORMAT_RGBA_ASTC_8x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x5_KHR:
		case FORMAT_RGBA_ASTC_8x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x6_KHR:
		case FORMAT_RGBA_ASTC_8x8_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x8_KHR:
			return 16 * ((width + 7) / 8);
		case FORMAT_RGBA_ASTC_10x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x5_KHR:
		case FORMAT_RGBA_ASTC_10x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x6_KHR:
		case FORMAT_RGBA_ASTC_10x8_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x8_KHR:
		case FORMAT_RGBA_ASTC_10x10_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x10_KHR:
			return 16 * ((width + 9) / 10);
		case FORMAT_RGBA_ASTC_12x10_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_12x10_KHR:
		case FORMAT_RGBA_ASTC_12x12_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_12x12_KHR:
			return 16 * ((width + 11) / 12);
		case FORMAT_DXT3:
		case FORMAT_DXT5:
			return 16 * ((width + 3) / 4);   // 128 bit per 4x4 block, computed per 4 rows
		case FORMAT_ATI1:
			return 2 * ((width + 3) / 4);    // 64 bit per 4x4 block, computed per row
		case FORMAT_ATI2:
			return 4 * ((width + 3) / 4);    // 128 bit per 4x4 block, computed per row
		case FORMAT_YV12_BT601:
		case FORMAT_YV12_BT709:
		case FORMAT_YV12_JFIF:
			return align<16>(width);
		default:
			return bytes(format) * width;
		}
	}

	int Surface::pitchP(int width, int border, Format format, bool target)
	{
		int B = bytes(format);

		return B > 0 ? pitchB(width, border, format, target) / B : 0;
	}

	int Surface::sliceB(int width, int height, int border, Format format, bool target)
	{
		height += 2 * border;

		// Render targets require 2x2 quads
		if(target || isDepth(format) || isStencil(format))
		{
			height = align<2>(height);
		}

		switch(format)
		{
		case FORMAT_DXT1:
		case FORMAT_DXT3:
		case FORMAT_DXT5:
		case FORMAT_ETC1:
		case FORMAT_R11_EAC:
		case FORMAT_SIGNED_R11_EAC:
		case FORMAT_RG11_EAC:
		case FORMAT_SIGNED_RG11_EAC:
		case FORMAT_RGB8_ETC2:
		case FORMAT_SRGB8_ETC2:
		case FORMAT_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		case FORMAT_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		case FORMAT_RGBA8_ETC2_EAC:
		case FORMAT_SRGB8_ALPHA8_ETC2_EAC:
		case FORMAT_RGBA_ASTC_4x4_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_4x4_KHR:
		case FORMAT_RGBA_ASTC_5x4_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_5x4_KHR:
			return pitchB(width, border, format, target) * ((height + 3) / 4);   // Pitch computed per 4 rows
		case FORMAT_RGBA_ASTC_5x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_5x5_KHR:
		case FORMAT_RGBA_ASTC_6x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_6x5_KHR:
		case FORMAT_RGBA_ASTC_8x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x5_KHR:
		case FORMAT_RGBA_ASTC_10x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x5_KHR:
			return pitchB(width, border, format, target) * ((height + 4) / 5);   // Pitch computed per 5 rows
		case FORMAT_RGBA_ASTC_6x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_6x6_KHR:
		case FORMAT_RGBA_ASTC_8x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x6_KHR:
		case FORMAT_RGBA_ASTC_10x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x6_KHR:
			return pitchB(width, border, format, target) * ((height + 5) / 6);   // Pitch computed per 6 rows
		case FORMAT_RGBA_ASTC_8x8_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x8_KHR:
		case FORMAT_RGBA_ASTC_10x8_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x8_KHR:
			return pitchB(width, border, format, target) * ((height + 7) / 8);   // Pitch computed per 8 rows
		case FORMAT_RGBA_ASTC_10x10_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x10_KHR:
		case FORMAT_RGBA_ASTC_12x10_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_12x10_KHR:
			return pitchB(width, border, format, target) * ((height + 9) / 10);   // Pitch computed per 10 rows
		case FORMAT_RGBA_ASTC_12x12_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_12x12_KHR:
			return pitchB(width, border, format, target) * ((height + 11) / 12);   // Pitch computed per 12 rows
		case FORMAT_ATI1:
		case FORMAT_ATI2:
			return pitchB(width, border, format, target) * align<4>(height);   // Pitch computed per row
		default:
			return pitchB(width, border, format, target) * height;   // Pitch computed per row
		}
	}

	int Surface::sliceP(int width, int height, int border, Format format, bool target)
	{
		int B = bytes(format);

		return B > 0 ? sliceB(width, height, border, format, target) / B : 0;
	}

	void Surface::update(Buffer &destination, Buffer &source)
	{
	//	ASSERT(source.lock != LOCK_UNLOCKED);
	//	ASSERT(destination.lock != LOCK_UNLOCKED);

		if(destination.buffer != source.buffer)
		{
			ASSERT(source.dirty && !destination.dirty);

			switch(source.format)
			{
			case FORMAT_R8G8B8:		decodeR8G8B8(destination, source);		break;   // FIXME: Check destination format
			case FORMAT_X1R5G5B5:	decodeX1R5G5B5(destination, source);	break;   // FIXME: Check destination format
			case FORMAT_A1R5G5B5:	decodeA1R5G5B5(destination, source);	break;   // FIXME: Check destination format
			case FORMAT_X4R4G4B4:	decodeX4R4G4B4(destination, source);	break;   // FIXME: Check destination format
			case FORMAT_A4R4G4B4:	decodeA4R4G4B4(destination, source);	break;   // FIXME: Check destination format
			case FORMAT_P8:			decodeP8(destination, source);			break;   // FIXME: Check destination format
			case FORMAT_DXT1:		decodeDXT1(destination, source);		break;   // FIXME: Check destination format
			case FORMAT_DXT3:		decodeDXT3(destination, source);		break;   // FIXME: Check destination format
			case FORMAT_DXT5:		decodeDXT5(destination, source);		break;   // FIXME: Check destination format
			case FORMAT_ATI1:		decodeATI1(destination, source);		break;   // FIXME: Check destination format
			case FORMAT_ATI2:		decodeATI2(destination, source);		break;   // FIXME: Check destination format
			case FORMAT_R11_EAC:         decodeEAC(destination, source, 1, false); break; // FIXME: Check destination format
			case FORMAT_SIGNED_R11_EAC:  decodeEAC(destination, source, 1, true);  break; // FIXME: Check destination format
			case FORMAT_RG11_EAC:        decodeEAC(destination, source, 2, false); break; // FIXME: Check destination format
			case FORMAT_SIGNED_RG11_EAC: decodeEAC(destination, source, 2, true);  break; // FIXME: Check destination format
			case FORMAT_ETC1:
			case FORMAT_RGB8_ETC2:                      decodeETC2(destination, source, 0, false); break; // FIXME: Check destination format
			case FORMAT_SRGB8_ETC2:                     decodeETC2(destination, source, 0, true);  break; // FIXME: Check destination format
			case FORMAT_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:  decodeETC2(destination, source, 1, false); break; // FIXME: Check destination format
			case FORMAT_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2: decodeETC2(destination, source, 1, true);  break; // FIXME: Check destination format
			case FORMAT_RGBA8_ETC2_EAC:                 decodeETC2(destination, source, 8, false); break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ETC2_EAC:          decodeETC2(destination, source, 8, true);  break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_4x4_KHR:           decodeASTC(destination, source, 4,  4,  1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_5x4_KHR:           decodeASTC(destination, source, 5,  4,  1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_5x5_KHR:           decodeASTC(destination, source, 5,  5,  1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_6x5_KHR:           decodeASTC(destination, source, 6,  5,  1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_6x6_KHR:           decodeASTC(destination, source, 6,  6,  1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_8x5_KHR:           decodeASTC(destination, source, 8,  5,  1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_8x6_KHR:           decodeASTC(destination, source, 8,  6,  1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_8x8_KHR:           decodeASTC(destination, source, 8,  8,  1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_10x5_KHR:          decodeASTC(destination, source, 10, 5,  1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_10x6_KHR:          decodeASTC(destination, source, 10, 6,  1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_10x8_KHR:          decodeASTC(destination, source, 10, 8,  1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_10x10_KHR:         decodeASTC(destination, source, 10, 10, 1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_12x10_KHR:         decodeASTC(destination, source, 12, 10, 1, false); break; // FIXME: Check destination format
			case FORMAT_RGBA_ASTC_12x12_KHR:         decodeASTC(destination, source, 12, 12, 1, false); break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_4x4_KHR:   decodeASTC(destination, source, 4,  4,  1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_5x4_KHR:   decodeASTC(destination, source, 5,  4,  1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_5x5_KHR:   decodeASTC(destination, source, 5,  5,  1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_6x5_KHR:   decodeASTC(destination, source, 6,  5,  1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_6x6_KHR:   decodeASTC(destination, source, 6,  6,  1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_8x5_KHR:   decodeASTC(destination, source, 8,  5,  1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_8x6_KHR:   decodeASTC(destination, source, 8,  6,  1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_8x8_KHR:   decodeASTC(destination, source, 8,  8,  1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_10x5_KHR:  decodeASTC(destination, source, 10, 5,  1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_10x6_KHR:  decodeASTC(destination, source, 10, 6,  1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_10x8_KHR:  decodeASTC(destination, source, 10, 8,  1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_10x10_KHR: decodeASTC(destination, source, 10, 10, 1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_12x10_KHR: decodeASTC(destination, source, 12, 10, 1, true);  break; // FIXME: Check destination format
			case FORMAT_SRGB8_ALPHA8_ASTC_12x12_KHR: decodeASTC(destination, source, 12, 12, 1, true);  break; // FIXME: Check destination format
			default:				genericUpdate(destination, source);		break;
			}
		}
	}

	void Surface::genericUpdate(Buffer &destination, Buffer &source)
	{
		unsigned char *sourceSlice = (unsigned char*)source.lockRect(0, 0, 0, sw::LOCK_READONLY);
		unsigned char *destinationSlice = (unsigned char*)destination.lockRect(0, 0, 0, sw::LOCK_UPDATE);

		int depth = min(destination.depth, source.depth);
		int height = min(destination.height, source.height);
		int width = min(destination.width, source.width);
		int rowBytes = width * source.bytes;

		for(int z = 0; z < depth; z++)
		{
			unsigned char *sourceRow = sourceSlice;
			unsigned char *destinationRow = destinationSlice;

			for(int y = 0; y < height; y++)
			{
				if(source.format == destination.format)
				{
					memcpy(destinationRow, sourceRow, rowBytes);
				}
				else
				{
					unsigned char *sourceElement = sourceRow;
					unsigned char *destinationElement = destinationRow;

					for(int x = 0; x < width; x++)
					{
						Color<float> color = source.read(sourceElement);
						destination.write(destinationElement, color);

						sourceElement += source.bytes;
						destinationElement += destination.bytes;
					}
				}

				sourceRow += source.pitchB;
				destinationRow += destination.pitchB;
			}

			sourceSlice += source.sliceB;
			destinationSlice += destination.sliceB;
		}

		source.unlockRect();
		destination.unlockRect();
	}

	void Surface::decodeR8G8B8(Buffer &destination, Buffer &source)
	{
		unsigned char *sourceSlice = (unsigned char*)source.lockRect(0, 0, 0, sw::LOCK_READONLY);
		unsigned char *destinationSlice = (unsigned char*)destination.lockRect(0, 0, 0, sw::LOCK_UPDATE);

		int depth = min(destination.depth, source.depth);
		int height = min(destination.height, source.height);
		int width = min(destination.width, source.width);

		for(int z = 0; z < depth; z++)
		{
			unsigned char *sourceRow = sourceSlice;
			unsigned char *destinationRow = destinationSlice;

			for(int y = 0; y < height; y++)
			{
				unsigned char *sourceElement = sourceRow;
				unsigned char *destinationElement = destinationRow;

				for(int x = 0; x < width; x++)
				{
					unsigned int b = sourceElement[0];
					unsigned int g = sourceElement[1];
					unsigned int r = sourceElement[2];

					*(unsigned int*)destinationElement = 0xFF000000 | (r << 16) | (g << 8) | (b << 0);

					sourceElement += source.bytes;
					destinationElement += destination.bytes;
				}

				sourceRow += source.pitchB;
				destinationRow += destination.pitchB;
			}

			sourceSlice += source.sliceB;
			destinationSlice += destination.sliceB;
		}

		source.unlockRect();
		destination.unlockRect();
	}

	void Surface::decodeX1R5G5B5(Buffer &destination, Buffer &source)
	{
		unsigned char *sourceSlice = (unsigned char*)source.lockRect(0, 0, 0, sw::LOCK_READONLY);
		unsigned char *destinationSlice = (unsigned char*)destination.lockRect(0, 0, 0, sw::LOCK_UPDATE);

		int depth = min(destination.depth, source.depth);
		int height = min(destination.height, source.height);
		int width = min(destination.width, source.width);

		for(int z = 0; z < depth; z++)
		{
			unsigned char *sourceRow = sourceSlice;
			unsigned char *destinationRow = destinationSlice;

			for(int y = 0; y < height; y++)
			{
				unsigned char *sourceElement = sourceRow;
				unsigned char *destinationElement = destinationRow;

				for(int x = 0; x < width; x++)
				{
					unsigned int xrgb = *(unsigned short*)sourceElement;

					unsigned int r = (((xrgb & 0x7C00) * 134771 + 0x800000) >> 8) & 0x00FF0000;
					unsigned int g = (((xrgb & 0x03E0) * 16846 + 0x8000) >> 8) & 0x0000FF00;
					unsigned int b = (((xrgb & 0x001F) * 2106  + 0x80) >> 8);

					*(unsigned int*)destinationElement = 0xFF000000 | r | g | b;

					sourceElement += source.bytes;
					destinationElement += destination.bytes;
				}

				sourceRow += source.pitchB;
				destinationRow += destination.pitchB;
			}

			sourceSlice += source.sliceB;
			destinationSlice += destination.sliceB;
		}

		source.unlockRect();
		destination.unlockRect();
	}

	void Surface::decodeA1R5G5B5(Buffer &destination, Buffer &source)
	{
		unsigned char *sourceSlice = (unsigned char*)source.lockRect(0, 0, 0, sw::LOCK_READONLY);
		unsigned char *destinationSlice = (unsigned char*)destination.lockRect(0, 0, 0, sw::LOCK_UPDATE);

		int depth = min(destination.depth, source.depth);
		int height = min(destination.height, source.height);
		int width = min(destination.width, source.width);

		for(int z = 0; z < depth; z++)
		{
			unsigned char *sourceRow = sourceSlice;
			unsigned char *destinationRow = destinationSlice;

			for(int y = 0; y < height; y++)
			{
				unsigned char *sourceElement = sourceRow;
				unsigned char *destinationElement = destinationRow;

				for(int x = 0; x < width; x++)
				{
					unsigned int argb = *(unsigned short*)sourceElement;

					unsigned int a =   (argb & 0x8000) * 130560;
					unsigned int r = (((argb & 0x7C00) * 134771 + 0x800000) >> 8) & 0x00FF0000;
					unsigned int g = (((argb & 0x03E0) * 16846  + 0x8000) >> 8) & 0x0000FF00;
					unsigned int b = (((argb & 0x001F) * 2106   + 0x80) >> 8);

					*(unsigned int*)destinationElement = a | r | g | b;

					sourceElement += source.bytes;
					destinationElement += destination.bytes;
				}

				sourceRow += source.pitchB;
				destinationRow += destination.pitchB;
			}

			sourceSlice += source.sliceB;
			destinationSlice += destination.sliceB;
		}

		source.unlockRect();
		destination.unlockRect();
	}

	void Surface::decodeX4R4G4B4(Buffer &destination, Buffer &source)
	{
		unsigned char *sourceSlice = (unsigned char*)source.lockRect(0, 0, 0, sw::LOCK_READONLY);
		unsigned char *destinationSlice = (unsigned char*)destination.lockRect(0, 0, 0, sw::LOCK_UPDATE);

		int depth = min(destination.depth, source.depth);
		int height = min(destination.height, source.height);
		int width = min(destination.width, source.width);

		for(int z = 0; z < depth; z++)
		{
			unsigned char *sourceRow = sourceSlice;
			unsigned char *destinationRow = destinationSlice;

			for(int y = 0; y < height; y++)
			{
				unsigned char *sourceElement = sourceRow;
				unsigned char *destinationElement = destinationRow;

				for(int x = 0; x < width; x++)
				{
					unsigned int xrgb = *(unsigned short*)sourceElement;

					unsigned int r = ((xrgb & 0x0F00) * 0x00001100) & 0x00FF0000;
					unsigned int g = ((xrgb & 0x00F0) * 0x00000110) & 0x0000FF00;
					unsigned int b =  (xrgb & 0x000F) * 0x00000011;

					*(unsigned int*)destinationElement = 0xFF000000 | r | g | b;

					sourceElement += source.bytes;
					destinationElement += destination.bytes;
				}

				sourceRow += source.pitchB;
				destinationRow += destination.pitchB;
			}

			sourceSlice += source.sliceB;
			destinationSlice += destination.sliceB;
		}

		source.unlockRect();
		destination.unlockRect();
	}

	void Surface::decodeA4R4G4B4(Buffer &destination, Buffer &source)
	{
		unsigned char *sourceSlice = (unsigned char*)source.lockRect(0, 0, 0, sw::LOCK_READONLY);
		unsigned char *destinationSlice = (unsigned char*)destination.lockRect(0, 0, 0, sw::LOCK_UPDATE);

		int depth = min(destination.depth, source.depth);
		int height = min(destination.height, source.height);
		int width = min(destination.width, source.width);

		for(int z = 0; z < depth; z++)
		{
			unsigned char *sourceRow = sourceSlice;
			unsigned char *destinationRow = destinationSlice;

			for(int y = 0; y < height; y++)
			{
				unsigned char *sourceElement = sourceRow;
				unsigned char *destinationElement = destinationRow;

				for(int x = 0; x < width; x++)
				{
					unsigned int argb = *(unsigned short*)sourceElement;

					unsigned int a = ((argb & 0xF000) * 0x00011000) & 0xFF000000;
					unsigned int r = ((argb & 0x0F00) * 0x00001100) & 0x00FF0000;
					unsigned int g = ((argb & 0x00F0) * 0x00000110) & 0x0000FF00;
					unsigned int b =  (argb & 0x000F) * 0x00000011;

					*(unsigned int*)destinationElement = a | r | g | b;

					sourceElement += source.bytes;
					destinationElement += destination.bytes;
				}

				sourceRow += source.pitchB;
				destinationRow += destination.pitchB;
			}

			sourceSlice += source.sliceB;
			destinationSlice += destination.sliceB;
		}

		source.unlockRect();
		destination.unlockRect();
	}

	void Surface::decodeP8(Buffer &destination, Buffer &source)
	{
		unsigned char *sourceSlice = (unsigned char*)source.lockRect(0, 0, 0, sw::LOCK_READONLY);
		unsigned char *destinationSlice = (unsigned char*)destination.lockRect(0, 0, 0, sw::LOCK_UPDATE);

		int depth = min(destination.depth, source.depth);
		int height = min(destination.height, source.height);
		int width = min(destination.width, source.width);

		for(int z = 0; z < depth; z++)
		{
			unsigned char *sourceRow = sourceSlice;
			unsigned char *destinationRow = destinationSlice;

			for(int y = 0; y < height; y++)
			{
				unsigned char *sourceElement = sourceRow;
				unsigned char *destinationElement = destinationRow;

				for(int x = 0; x < width; x++)
				{
					unsigned int abgr = palette[*(unsigned char*)sourceElement];

					unsigned int r = (abgr & 0x000000FF) << 16;
					unsigned int g = (abgr & 0x0000FF00) << 0;
					unsigned int b = (abgr & 0x00FF0000) >> 16;
					unsigned int a = (abgr & 0xFF000000) >> 0;

					*(unsigned int*)destinationElement = a | r | g | b;

					sourceElement += source.bytes;
					destinationElement += destination.bytes;
				}

				sourceRow += source.pitchB;
				destinationRow += destination.pitchB;
			}

			sourceSlice += source.sliceB;
			destinationSlice += destination.sliceB;
		}

		source.unlockRect();
		destination.unlockRect();
	}

	void Surface::decodeDXT1(Buffer &internal, Buffer &external)
	{
		unsigned int *destSlice = (unsigned int*)internal.lockRect(0, 0, 0, LOCK_UPDATE);
		const DXT1 *source = (const DXT1*)external.lockRect(0, 0, 0, LOCK_READONLY);

		for(int z = 0; z < external.depth; z++)
		{
			unsigned int *dest = destSlice;

			for(int y = 0; y < external.height; y += 4)
			{
				for(int x = 0; x < external.width; x += 4)
				{
					Color<byte> c[4];

					c[0] = source->c0;
					c[1] = source->c1;

					if(source->c0 > source->c1)   // No transparency
					{
						// c2 = 2 / 3 * c0 + 1 / 3 * c1
						c[2].r = (byte)((2 * (word)c[0].r + (word)c[1].r + 1) / 3);
						c[2].g = (byte)((2 * (word)c[0].g + (word)c[1].g + 1) / 3);
						c[2].b = (byte)((2 * (word)c[0].b + (word)c[1].b + 1) / 3);
						c[2].a = 0xFF;

						// c3 = 1 / 3 * c0 + 2 / 3 * c1
						c[3].r = (byte)(((word)c[0].r + 2 * (word)c[1].r + 1) / 3);
						c[3].g = (byte)(((word)c[0].g + 2 * (word)c[1].g + 1) / 3);
						c[3].b = (byte)(((word)c[0].b + 2 * (word)c[1].b + 1) / 3);
						c[3].a = 0xFF;
					}
					else   // c3 transparent
					{
						// c2 = 1 / 2 * c0 + 1 / 2 * c1
						c[2].r = (byte)(((word)c[0].r + (word)c[1].r) / 2);
						c[2].g = (byte)(((word)c[0].g + (word)c[1].g) / 2);
						c[2].b = (byte)(((word)c[0].b + (word)c[1].b) / 2);
						c[2].a = 0xFF;

						c[3].r = 0;
						c[3].g = 0;
						c[3].b = 0;
						c[3].a = 0;
					}

					for(int j = 0; j < 4 && (y + j) < internal.height; j++)
					{
						for(int i = 0; i < 4 && (x + i) < internal.width; i++)
						{
							dest[(x + i) + (y + j) * internal.width] = c[(unsigned int)(source->lut >> 2 * (i + j * 4)) % 4];
						}
					}

					source++;
				}
			}

			(byte*&)destSlice += internal.sliceB;
		}

		external.unlockRect();
		internal.unlockRect();
	}

	void Surface::decodeDXT3(Buffer &internal, Buffer &external)
	{
		unsigned int *destSlice = (unsigned int*)internal.lockRect(0, 0, 0, LOCK_UPDATE);
		const DXT3 *source = (const DXT3*)external.lockRect(0, 0, 0, LOCK_READONLY);

		for(int z = 0; z < external.depth; z++)
		{
			unsigned int *dest = destSlice;

			for(int y = 0; y < external.height; y += 4)
			{
				for(int x = 0; x < external.width; x += 4)
				{
					Color<byte> c[4];

					c[0] = source->c0;
					c[1] = source->c1;

					// c2 = 2 / 3 * c0 + 1 / 3 * c1
					c[2].r = (byte)((2 * (word)c[0].r + (word)c[1].r + 1) / 3);
					c[2].g = (byte)((2 * (word)c[0].g + (word)c[1].g + 1) / 3);
					c[2].b = (byte)((2 * (word)c[0].b + (word)c[1].b + 1) / 3);

					// c3 = 1 / 3 * c0 + 2 / 3 * c1
					c[3].r = (byte)(((word)c[0].r + 2 * (word)c[1].r + 1) / 3);
					c[3].g = (byte)(((word)c[0].g + 2 * (word)c[1].g + 1) / 3);
					c[3].b = (byte)(((word)c[0].b + 2 * (word)c[1].b + 1) / 3);

					for(int j = 0; j < 4 && (y + j) < internal.height; j++)
					{
						for(int i = 0; i < 4 && (x + i) < internal.width; i++)
						{
							unsigned int a = (unsigned int)(source->a >> 4 * (i + j * 4)) & 0x0F;
							unsigned int color = (c[(unsigned int)(source->lut >> 2 * (i + j * 4)) % 4] & 0x00FFFFFF) | ((a << 28) + (a << 24));

							dest[(x + i) + (y + j) * internal.width] = color;
						}
					}

					source++;
				}
			}

			(byte*&)destSlice += internal.sliceB;
		}

		external.unlockRect();
		internal.unlockRect();
	}

	void Surface::decodeDXT5(Buffer &internal, Buffer &external)
	{
		unsigned int *destSlice = (unsigned int*)internal.lockRect(0, 0, 0, LOCK_UPDATE);
		const DXT5 *source = (const DXT5*)external.lockRect(0, 0, 0, LOCK_READONLY);

		for(int z = 0; z < external.depth; z++)
		{
			unsigned int *dest = destSlice;

			for(int y = 0; y < external.height; y += 4)
			{
				for(int x = 0; x < external.width; x += 4)
				{
					Color<byte> c[4];

					c[0] = source->c0;
					c[1] = source->c1;

					// c2 = 2 / 3 * c0 + 1 / 3 * c1
					c[2].r = (byte)((2 * (word)c[0].r + (word)c[1].r + 1) / 3);
					c[2].g = (byte)((2 * (word)c[0].g + (word)c[1].g + 1) / 3);
					c[2].b = (byte)((2 * (word)c[0].b + (word)c[1].b + 1) / 3);

					// c3 = 1 / 3 * c0 + 2 / 3 * c1
					c[3].r = (byte)(((word)c[0].r + 2 * (word)c[1].r + 1) / 3);
					c[3].g = (byte)(((word)c[0].g + 2 * (word)c[1].g + 1) / 3);
					c[3].b = (byte)(((word)c[0].b + 2 * (word)c[1].b + 1) / 3);

					byte a[8];

					a[0] = source->a0;
					a[1] = source->a1;

					if(a[0] > a[1])
					{
						a[2] = (byte)((6 * (word)a[0] + 1 * (word)a[1] + 3) / 7);
						a[3] = (byte)((5 * (word)a[0] + 2 * (word)a[1] + 3) / 7);
						a[4] = (byte)((4 * (word)a[0] + 3 * (word)a[1] + 3) / 7);
						a[5] = (byte)((3 * (word)a[0] + 4 * (word)a[1] + 3) / 7);
						a[6] = (byte)((2 * (word)a[0] + 5 * (word)a[1] + 3) / 7);
						a[7] = (byte)((1 * (word)a[0] + 6 * (word)a[1] + 3) / 7);
					}
					else
					{
						a[2] = (byte)((4 * (word)a[0] + 1 * (word)a[1] + 2) / 5);
						a[3] = (byte)((3 * (word)a[0] + 2 * (word)a[1] + 2) / 5);
						a[4] = (byte)((2 * (word)a[0] + 3 * (word)a[1] + 2) / 5);
						a[5] = (byte)((1 * (word)a[0] + 4 * (word)a[1] + 2) / 5);
						a[6] = 0;
						a[7] = 0xFF;
					}

					for(int j = 0; j < 4 && (y + j) < internal.height; j++)
					{
						for(int i = 0; i < 4 && (x + i) < internal.width; i++)
						{
							unsigned int alpha = (unsigned int)a[(unsigned int)(source->alut >> (16 + 3 * (i + j * 4))) % 8] << 24;
							unsigned int color = (c[(source->clut >> 2 * (i + j * 4)) % 4] & 0x00FFFFFF) | alpha;

							dest[(x + i) + (y + j) * internal.width] = color;
						}
					}

					source++;
				}
			}

			(byte*&)destSlice += internal.sliceB;
		}

		external.unlockRect();
		internal.unlockRect();
	}

	void Surface::decodeATI1(Buffer &internal, Buffer &external)
	{
		byte *destSlice = (byte*)internal.lockRect(0, 0, 0, LOCK_UPDATE);
		const ATI1 *source = (const ATI1*)external.lockRect(0, 0, 0, LOCK_READONLY);

		for(int z = 0; z < external.depth; z++)
		{
			byte *dest = destSlice;

			for(int y = 0; y < external.height; y += 4)
			{
				for(int x = 0; x < external.width; x += 4)
				{
					byte r[8];

					r[0] = source->r0;
					r[1] = source->r1;

					if(r[0] > r[1])
					{
						r[2] = (byte)((6 * (word)r[0] + 1 * (word)r[1] + 3) / 7);
						r[3] = (byte)((5 * (word)r[0] + 2 * (word)r[1] + 3) / 7);
						r[4] = (byte)((4 * (word)r[0] + 3 * (word)r[1] + 3) / 7);
						r[5] = (byte)((3 * (word)r[0] + 4 * (word)r[1] + 3) / 7);
						r[6] = (byte)((2 * (word)r[0] + 5 * (word)r[1] + 3) / 7);
						r[7] = (byte)((1 * (word)r[0] + 6 * (word)r[1] + 3) / 7);
					}
					else
					{
						r[2] = (byte)((4 * (word)r[0] + 1 * (word)r[1] + 2) / 5);
						r[3] = (byte)((3 * (word)r[0] + 2 * (word)r[1] + 2) / 5);
						r[4] = (byte)((2 * (word)r[0] + 3 * (word)r[1] + 2) / 5);
						r[5] = (byte)((1 * (word)r[0] + 4 * (word)r[1] + 2) / 5);
						r[6] = 0;
						r[7] = 0xFF;
					}

					for(int j = 0; j < 4 && (y + j) < internal.height; j++)
					{
						for(int i = 0; i < 4 && (x + i) < internal.width; i++)
						{
							dest[(x + i) + (y + j) * internal.width] = r[(unsigned int)(source->rlut >> (16 + 3 * (i + j * 4))) % 8];
						}
					}

					source++;
				}
			}

			destSlice += internal.sliceB;
		}

		external.unlockRect();
		internal.unlockRect();
	}

	void Surface::decodeATI2(Buffer &internal, Buffer &external)
	{
		word *destSlice = (word*)internal.lockRect(0, 0, 0, LOCK_UPDATE);
		const ATI2 *source = (const ATI2*)external.lockRect(0, 0, 0, LOCK_READONLY);

		for(int z = 0; z < external.depth; z++)
		{
			word *dest = destSlice;

			for(int y = 0; y < external.height; y += 4)
			{
				for(int x = 0; x < external.width; x += 4)
				{
					byte X[8];

					X[0] = source->x0;
					X[1] = source->x1;

					if(X[0] > X[1])
					{
						X[2] = (byte)((6 * (word)X[0] + 1 * (word)X[1] + 3) / 7);
						X[3] = (byte)((5 * (word)X[0] + 2 * (word)X[1] + 3) / 7);
						X[4] = (byte)((4 * (word)X[0] + 3 * (word)X[1] + 3) / 7);
						X[5] = (byte)((3 * (word)X[0] + 4 * (word)X[1] + 3) / 7);
						X[6] = (byte)((2 * (word)X[0] + 5 * (word)X[1] + 3) / 7);
						X[7] = (byte)((1 * (word)X[0] + 6 * (word)X[1] + 3) / 7);
					}
					else
					{
						X[2] = (byte)((4 * (word)X[0] + 1 * (word)X[1] + 2) / 5);
						X[3] = (byte)((3 * (word)X[0] + 2 * (word)X[1] + 2) / 5);
						X[4] = (byte)((2 * (word)X[0] + 3 * (word)X[1] + 2) / 5);
						X[5] = (byte)((1 * (word)X[0] + 4 * (word)X[1] + 2) / 5);
						X[6] = 0;
						X[7] = 0xFF;
					}

					byte Y[8];

					Y[0] = source->y0;
					Y[1] = source->y1;

					if(Y[0] > Y[1])
					{
						Y[2] = (byte)((6 * (word)Y[0] + 1 * (word)Y[1] + 3) / 7);
						Y[3] = (byte)((5 * (word)Y[0] + 2 * (word)Y[1] + 3) / 7);
						Y[4] = (byte)((4 * (word)Y[0] + 3 * (word)Y[1] + 3) / 7);
						Y[5] = (byte)((3 * (word)Y[0] + 4 * (word)Y[1] + 3) / 7);
						Y[6] = (byte)((2 * (word)Y[0] + 5 * (word)Y[1] + 3) / 7);
						Y[7] = (byte)((1 * (word)Y[0] + 6 * (word)Y[1] + 3) / 7);
					}
					else
					{
						Y[2] = (byte)((4 * (word)Y[0] + 1 * (word)Y[1] + 2) / 5);
						Y[3] = (byte)((3 * (word)Y[0] + 2 * (word)Y[1] + 2) / 5);
						Y[4] = (byte)((2 * (word)Y[0] + 3 * (word)Y[1] + 2) / 5);
						Y[5] = (byte)((1 * (word)Y[0] + 4 * (word)Y[1] + 2) / 5);
						Y[6] = 0;
						Y[7] = 0xFF;
					}

					for(int j = 0; j < 4 && (y + j) < internal.height; j++)
					{
						for(int i = 0; i < 4 && (x + i) < internal.width; i++)
						{
							word r = X[(unsigned int)(source->xlut >> (16 + 3 * (i + j * 4))) % 8];
							word g = Y[(unsigned int)(source->ylut >> (16 + 3 * (i + j * 4))) % 8];

							dest[(x + i) + (y + j) * internal.width] = (g << 8) + r;
						}
					}

					source++;
				}
			}

			(byte*&)destSlice += internal.sliceB;
		}

		external.unlockRect();
		internal.unlockRect();
	}

	void Surface::decodeETC2(Buffer &internal, Buffer &external, int nbAlphaBits, bool isSRGB)
	{
		ETC_Decoder::Decode((const byte*)external.lockRect(0, 0, 0, LOCK_READONLY), (byte*)internal.lockRect(0, 0, 0, LOCK_UPDATE), external.width, external.height, internal.width, internal.height, internal.pitchB, internal.bytes,
		                    (nbAlphaBits == 8) ? ETC_Decoder::ETC_RGBA : ((nbAlphaBits == 1) ? ETC_Decoder::ETC_RGB_PUNCHTHROUGH_ALPHA : ETC_Decoder::ETC_RGB));
		external.unlockRect();
		internal.unlockRect();

		if(isSRGB)
		{
			static byte sRGBtoLinearTable[256];
			static bool sRGBtoLinearTableDirty = true;
			if(sRGBtoLinearTableDirty)
			{
				for(int i = 0; i < 256; i++)
				{
					sRGBtoLinearTable[i] = static_cast<byte>(sRGBtoLinear(static_cast<float>(i) / 255.0f) * 255.0f + 0.5f);
				}
				sRGBtoLinearTableDirty = false;
			}

			// Perform sRGB conversion in place after decoding
			byte *src = (byte*)internal.lockRect(0, 0, 0, LOCK_READWRITE);
			for(int y = 0; y < internal.height; y++)
			{
				byte *srcRow = src + y * internal.pitchB;
				for(int x = 0; x <  internal.width; x++)
				{
					byte *srcPix = srcRow + x * internal.bytes;
					for(int i = 0; i < 3; i++)
					{
						srcPix[i] = sRGBtoLinearTable[srcPix[i]];
					}
				}
			}
			internal.unlockRect();
		}
	}

	void Surface::decodeEAC(Buffer &internal, Buffer &external, int nbChannels, bool isSigned)
	{
		ASSERT(nbChannels == 1 || nbChannels == 2);

		byte *src = (byte*)internal.lockRect(0, 0, 0, LOCK_READWRITE);
		ETC_Decoder::Decode((const byte*)external.lockRect(0, 0, 0, LOCK_READONLY), src, external.width, external.height, internal.width, internal.height, internal.pitchB, internal.bytes,
		                    (nbChannels == 1) ? (isSigned ? ETC_Decoder::ETC_R_SIGNED : ETC_Decoder::ETC_R_UNSIGNED) : (isSigned ? ETC_Decoder::ETC_RG_SIGNED : ETC_Decoder::ETC_RG_UNSIGNED));
		external.unlockRect();

		// FIXME: We convert EAC data to float, until signed short internal formats are supported
		//        This code can be removed if ETC2 images are decoded to internal 16 bit signed R/RG formats
		const float normalization = isSigned ? (1.0f / (8.0f * 127.875f)) : (1.0f / (8.0f * 255.875f));
		for(int y = 0; y < internal.height; y++)
		{
			byte* srcRow = src + y * internal.pitchB;
			for(int x = internal.width - 1; x >= 0; x--)
			{
				int* srcPix = reinterpret_cast<int*>(srcRow + x * internal.bytes);
				float* dstPix = reinterpret_cast<float*>(srcPix);
				for(int c = nbChannels - 1; c >= 0; c--)
				{
					dstPix[c] = clamp(static_cast<float>(srcPix[c]) * normalization, -1.0f, 1.0f);
				}
			}
		}

		internal.unlockRect();
	}

	void Surface::decodeASTC(Buffer &internal, Buffer &external, int xBlockSize, int yBlockSize, int zBlockSize, bool isSRGB)
	{
	}

	size_t Surface::size(int width, int height, int depth, int border, int samples, Format format)
	{
		samples = max(1, samples);

		switch(format)
		{
		default:
			{
				uint64_t size = (uint64_t)sliceB(width, height, border, format, true) * depth * samples;

				// FIXME: Unpacking byte4 to short4 in the sampler currently involves reading 8 bytes,
				// and stencil operations also read 8 bytes per four 8-bit stencil values,
				// so we have to allocate 4 extra bytes to avoid buffer overruns.
				size += 4;

				// We can only sample buffers smaller than 2 GiB.
				// Force an out-of-memory if larger, or let the caller report an error.
				return size < 0x80000000u ? (size_t)size : std::numeric_limits<size_t>::max();
			}
		case FORMAT_YV12_BT601:
		case FORMAT_YV12_BT709:
		case FORMAT_YV12_JFIF:
			{
				width += 2 * border;
				height += 2 * border;

				size_t YStride = align<16>(width);
				size_t YSize = YStride * height;
				size_t CStride = align<16>(YStride / 2);
				size_t CSize = CStride * height / 2;

				return YSize + 2 * CSize;
			}
		}
	}

	bool Surface::isStencil(Format format)
	{
		switch(format)
		{
		case FORMAT_D32:
		case FORMAT_D16:
		case FORMAT_D24X8:
		case FORMAT_D32F:
		case FORMAT_D32F_COMPLEMENTARY:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32F_SHADOW:
			return false;
		case FORMAT_D24S8:
		case FORMAT_D24FS8:
		case FORMAT_S8:
		case FORMAT_DF24S8:
		case FORMAT_DF16S8:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32FS8_SHADOW:
		case FORMAT_D32FS8:
		case FORMAT_D32FS8_COMPLEMENTARY:
		case FORMAT_INTZ:
			return true;
		default:
			return false;
		}
	}

	bool Surface::isDepth(Format format)
	{
		switch(format)
		{
		case FORMAT_D32:
		case FORMAT_D16:
		case FORMAT_D24X8:
		case FORMAT_D24S8:
		case FORMAT_D24FS8:
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_COMPLEMENTARY:
		case FORMAT_D32FS8_COMPLEMENTARY:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_DF24S8:
		case FORMAT_DF16S8:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
		case FORMAT_INTZ:
			return true;
		case FORMAT_S8:
			return false;
		default:
			return false;
		}
	}

	bool Surface::hasQuadLayout(Format format)
	{
		switch(format)
		{
		case FORMAT_D32:
		case FORMAT_D16:
		case FORMAT_D24X8:
		case FORMAT_D24S8:
		case FORMAT_D24FS8:
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_COMPLEMENTARY:
		case FORMAT_D32FS8_COMPLEMENTARY:
		case FORMAT_DF24S8:
		case FORMAT_DF16S8:
		case FORMAT_INTZ:
		case FORMAT_S8:
		case FORMAT_A8G8R8B8Q:
		case FORMAT_X8G8R8B8Q:
			return true;
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
		default:
			break;
		}

		return false;
	}

	bool Surface::isPalette(Format format)
	{
		switch(format)
		{
		case FORMAT_P8:
		case FORMAT_A8P8:
			return true;
		default:
			return false;
		}
	}

	bool Surface::isFloatFormat(Format format)
	{
		switch(format)
		{
		case FORMAT_R5G6B5:
		case FORMAT_R8G8B8:
		case FORMAT_B8G8R8:
		case FORMAT_X8R8G8B8:
		case FORMAT_X8B8G8R8I:
		case FORMAT_X8B8G8R8:
		case FORMAT_A8R8G8B8:
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
		case FORMAT_A8B8G8R8I:
		case FORMAT_R8UI:
		case FORMAT_G8R8UI:
		case FORMAT_X8B8G8R8UI:
		case FORMAT_A8B8G8R8UI:
		case FORMAT_A8B8G8R8:
		case FORMAT_G8R8I:
		case FORMAT_G8R8:
		case FORMAT_A2B10G10R10:
		case FORMAT_A2B10G10R10UI:
		case FORMAT_R8_SNORM:
		case FORMAT_G8R8_SNORM:
		case FORMAT_X8B8G8R8_SNORM:
		case FORMAT_A8B8G8R8_SNORM:
		case FORMAT_R16I:
		case FORMAT_R16UI:
		case FORMAT_G16R16I:
		case FORMAT_G16R16UI:
		case FORMAT_G16R16:
		case FORMAT_X16B16G16R16I:
		case FORMAT_X16B16G16R16UI:
		case FORMAT_A16B16G16R16I:
		case FORMAT_A16B16G16R16UI:
		case FORMAT_A16B16G16R16:
		case FORMAT_V8U8:
		case FORMAT_Q8W8V8U8:
		case FORMAT_X8L8V8U8:
		case FORMAT_V16U16:
		case FORMAT_A16W16V16U16:
		case FORMAT_Q16W16V16U16:
		case FORMAT_A8:
		case FORMAT_R8I:
		case FORMAT_R8:
		case FORMAT_S8:
		case FORMAT_L8:
		case FORMAT_L16:
		case FORMAT_A8L8:
		case FORMAT_YV12_BT601:
		case FORMAT_YV12_BT709:
		case FORMAT_YV12_JFIF:
		case FORMAT_R32I:
		case FORMAT_R32UI:
		case FORMAT_G32R32I:
		case FORMAT_G32R32UI:
		case FORMAT_X32B32G32R32I:
		case FORMAT_X32B32G32R32UI:
		case FORMAT_A32B32G32R32I:
		case FORMAT_A32B32G32R32UI:
			return false;
		case FORMAT_R16F:
		case FORMAT_G16R16F:
		case FORMAT_B16G16R16F:
		case FORMAT_X16B16G16R16F:
		case FORMAT_A16B16G16R16F:
		case FORMAT_X16B16G16R16F_UNSIGNED:
		case FORMAT_R32F:
		case FORMAT_G32R32F:
		case FORMAT_B32G32R32F:
		case FORMAT_X32B32G32R32F:
		case FORMAT_A32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_COMPLEMENTARY:
		case FORMAT_D32FS8_COMPLEMENTARY:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
		case FORMAT_L16F:
		case FORMAT_A16L16F:
		case FORMAT_L32F:
		case FORMAT_A32L32F:
			return true;
		default:
			ASSERT(false);
		}

		return false;
	}

	bool Surface::isUnsignedComponent(Format format, int component)
	{
		switch(format)
		{
		case FORMAT_NULL:
		case FORMAT_R5G6B5:
		case FORMAT_R8G8B8:
		case FORMAT_B8G8R8:
		case FORMAT_X8R8G8B8:
		case FORMAT_X8B8G8R8:
		case FORMAT_A8R8G8B8:
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
		case FORMAT_G8R8:
		case FORMAT_A2B10G10R10:
		case FORMAT_A2B10G10R10UI:
		case FORMAT_R16UI:
		case FORMAT_G16R16:
		case FORMAT_G16R16UI:
		case FORMAT_X16B16G16R16UI:
		case FORMAT_A16B16G16R16:
		case FORMAT_A16B16G16R16UI:
		case FORMAT_R32UI:
		case FORMAT_G32R32UI:
		case FORMAT_X32B32G32R32UI:
		case FORMAT_A32B32G32R32UI:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_R8UI:
		case FORMAT_G8R8UI:
		case FORMAT_X8B8G8R8UI:
		case FORMAT_A8B8G8R8UI:
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_COMPLEMENTARY:
		case FORMAT_D32FS8_COMPLEMENTARY:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
		case FORMAT_A8:
		case FORMAT_R8:
		case FORMAT_L8:
		case FORMAT_L16:
		case FORMAT_A8L8:
		case FORMAT_YV12_BT601:
		case FORMAT_YV12_BT709:
		case FORMAT_YV12_JFIF:
			return true;
		case FORMAT_A8B8G8R8I:
		case FORMAT_A16B16G16R16I:
		case FORMAT_A32B32G32R32I:
		case FORMAT_A8B8G8R8_SNORM:
		case FORMAT_Q8W8V8U8:
		case FORMAT_Q16W16V16U16:
		case FORMAT_A32B32G32R32F:
			return false;
		case FORMAT_R32F:
		case FORMAT_R8I:
		case FORMAT_R16I:
		case FORMAT_R32I:
		case FORMAT_R8_SNORM:
			return component >= 1;
		case FORMAT_V8U8:
		case FORMAT_X8L8V8U8:
		case FORMAT_V16U16:
		case FORMAT_G32R32F:
		case FORMAT_G8R8I:
		case FORMAT_G16R16I:
		case FORMAT_G32R32I:
		case FORMAT_G8R8_SNORM:
			return component >= 2;
		case FORMAT_A16W16V16U16:
		case FORMAT_B32G32R32F:
		case FORMAT_X32B32G32R32F:
		case FORMAT_X8B8G8R8I:
		case FORMAT_X16B16G16R16I:
		case FORMAT_X32B32G32R32I:
		case FORMAT_X8B8G8R8_SNORM:
			return component >= 3;
		default:
			ASSERT(false);
		}

		return false;
	}

	bool Surface::isSRGBreadable(Format format)
	{
		// Keep in sync with Capabilities::isSRGBreadable
		switch(format)
		{
		case FORMAT_L8:
		case FORMAT_A8L8:
		case FORMAT_R8G8B8:
		case FORMAT_A8R8G8B8:
		case FORMAT_X8R8G8B8:
		case FORMAT_A8B8G8R8:
		case FORMAT_X8B8G8R8:
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
		case FORMAT_R5G6B5:
		case FORMAT_X1R5G5B5:
		case FORMAT_A1R5G5B5:
		case FORMAT_A4R4G4B4:
		case FORMAT_DXT1:
		case FORMAT_DXT3:
		case FORMAT_DXT5:
		case FORMAT_ATI1:
		case FORMAT_ATI2:
			return true;
		default:
			return false;
		}
	}

	bool Surface::isSRGBwritable(Format format)
	{
		// Keep in sync with Capabilities::isSRGBwritable
		switch(format)
		{
		case FORMAT_NULL:
		case FORMAT_A8R8G8B8:
		case FORMAT_X8R8G8B8:
		case FORMAT_A8B8G8R8:
		case FORMAT_X8B8G8R8:
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
		case FORMAT_R5G6B5:
			return true;
		default:
			return false;
		}
	}

	bool Surface::isSRGBformat(Format format)
	{
		switch(format)
		{
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
			return true;
		default:
			return false;
		}
	}

	bool Surface::isCompressed(Format format)
	{
		switch(format)
		{
		case FORMAT_DXT1:
		case FORMAT_DXT3:
		case FORMAT_DXT5:
		case FORMAT_ATI1:
		case FORMAT_ATI2:
		case FORMAT_ETC1:
		case FORMAT_R11_EAC:
		case FORMAT_SIGNED_R11_EAC:
		case FORMAT_RG11_EAC:
		case FORMAT_SIGNED_RG11_EAC:
		case FORMAT_RGB8_ETC2:
		case FORMAT_SRGB8_ETC2:
		case FORMAT_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		case FORMAT_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		case FORMAT_RGBA8_ETC2_EAC:
		case FORMAT_SRGB8_ALPHA8_ETC2_EAC:
		case FORMAT_RGBA_ASTC_4x4_KHR:
		case FORMAT_RGBA_ASTC_5x4_KHR:
		case FORMAT_RGBA_ASTC_5x5_KHR:
		case FORMAT_RGBA_ASTC_6x5_KHR:
		case FORMAT_RGBA_ASTC_6x6_KHR:
		case FORMAT_RGBA_ASTC_8x5_KHR:
		case FORMAT_RGBA_ASTC_8x6_KHR:
		case FORMAT_RGBA_ASTC_8x8_KHR:
		case FORMAT_RGBA_ASTC_10x5_KHR:
		case FORMAT_RGBA_ASTC_10x6_KHR:
		case FORMAT_RGBA_ASTC_10x8_KHR:
		case FORMAT_RGBA_ASTC_10x10_KHR:
		case FORMAT_RGBA_ASTC_12x10_KHR:
		case FORMAT_RGBA_ASTC_12x12_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_4x4_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_5x4_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_5x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_6x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_6x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x8_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x8_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x10_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_12x10_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_12x12_KHR:
			return true;
		default:
			return false;
		}
	}

	bool Surface::isSignedNonNormalizedInteger(Format format)
	{
		switch(format)
		{
		case FORMAT_A8B8G8R8I:
		case FORMAT_X8B8G8R8I:
		case FORMAT_G8R8I:
		case FORMAT_R8I:
		case FORMAT_A16B16G16R16I:
		case FORMAT_X16B16G16R16I:
		case FORMAT_G16R16I:
		case FORMAT_R16I:
		case FORMAT_A32B32G32R32I:
		case FORMAT_X32B32G32R32I:
		case FORMAT_G32R32I:
		case FORMAT_R32I:
			return true;
		default:
			return false;
		}
	}

	bool Surface::isUnsignedNonNormalizedInteger(Format format)
	{
		switch(format)
		{
		case FORMAT_A8B8G8R8UI:
		case FORMAT_X8B8G8R8UI:
		case FORMAT_G8R8UI:
		case FORMAT_R8UI:
		case FORMAT_A16B16G16R16UI:
		case FORMAT_X16B16G16R16UI:
		case FORMAT_G16R16UI:
		case FORMAT_R16UI:
		case FORMAT_A32B32G32R32UI:
		case FORMAT_X32B32G32R32UI:
		case FORMAT_G32R32UI:
		case FORMAT_R32UI:
			return true;
		default:
			return false;
		}
	}

	bool Surface::isNonNormalizedInteger(Format format)
	{
		return isSignedNonNormalizedInteger(format) ||
		       isUnsignedNonNormalizedInteger(format);
	}

	bool Surface::isNormalizedInteger(Format format)
	{
		return !isFloatFormat(format) &&
		       !isNonNormalizedInteger(format) &&
		       !isCompressed(format) &&
		       !isDepth(format) &&
		       !isStencil(format);
	}

	int Surface::componentCount(Format format)
	{
		switch(format)
		{
		case FORMAT_R5G6B5:         return 3;
		case FORMAT_X8R8G8B8:       return 3;
		case FORMAT_X8B8G8R8I:      return 3;
		case FORMAT_X8B8G8R8:       return 3;
		case FORMAT_A8R8G8B8:       return 4;
		case FORMAT_SRGB8_X8:       return 3;
		case FORMAT_SRGB8_A8:       return 4;
		case FORMAT_A8B8G8R8I:      return 4;
		case FORMAT_A8B8G8R8:       return 4;
		case FORMAT_G8R8I:          return 2;
		case FORMAT_G8R8:           return 2;
		case FORMAT_R8_SNORM:      return 1;
		case FORMAT_G8R8_SNORM:    return 2;
		case FORMAT_X8B8G8R8_SNORM:return 3;
		case FORMAT_A8B8G8R8_SNORM:return 4;
		case FORMAT_R8UI:           return 1;
		case FORMAT_G8R8UI:         return 2;
		case FORMAT_X8B8G8R8UI:     return 3;
		case FORMAT_A8B8G8R8UI:     return 4;
		case FORMAT_A2B10G10R10:    return 4;
		case FORMAT_A2B10G10R10UI:  return 4;
		case FORMAT_G16R16I:        return 2;
		case FORMAT_G16R16UI:       return 2;
		case FORMAT_G16R16:         return 2;
		case FORMAT_G32R32I:        return 2;
		case FORMAT_G32R32UI:       return 2;
		case FORMAT_X16B16G16R16I:  return 3;
		case FORMAT_X16B16G16R16UI: return 3;
		case FORMAT_A16B16G16R16I:  return 4;
		case FORMAT_A16B16G16R16UI: return 4;
		case FORMAT_A16B16G16R16:   return 4;
		case FORMAT_X32B32G32R32I:  return 3;
		case FORMAT_X32B32G32R32UI: return 3;
		case FORMAT_A32B32G32R32I:  return 4;
		case FORMAT_A32B32G32R32UI: return 4;
		case FORMAT_V8U8:           return 2;
		case FORMAT_Q8W8V8U8:       return 4;
		case FORMAT_X8L8V8U8:       return 3;
		case FORMAT_V16U16:         return 2;
		case FORMAT_A16W16V16U16:   return 4;
		case FORMAT_Q16W16V16U16:   return 4;
		case FORMAT_R32F:           return 1;
		case FORMAT_G32R32F:        return 2;
		case FORMAT_X32B32G32R32F:  return 3;
		case FORMAT_A32B32G32R32F:  return 4;
		case FORMAT_X32B32G32R32F_UNSIGNED: return 3;
		case FORMAT_D32F:           return 1;
		case FORMAT_D32FS8:         return 1;
		case FORMAT_D32F_LOCKABLE:  return 1;
		case FORMAT_D32FS8_TEXTURE: return 1;
		case FORMAT_D32F_SHADOW:    return 1;
		case FORMAT_D32FS8_SHADOW:  return 1;
		case FORMAT_A8:             return 1;
		case FORMAT_R8I:            return 1;
		case FORMAT_R8:             return 1;
		case FORMAT_R16I:           return 1;
		case FORMAT_R16UI:          return 1;
		case FORMAT_R32I:           return 1;
		case FORMAT_R32UI:          return 1;
		case FORMAT_L8:             return 1;
		case FORMAT_L16:            return 1;
		case FORMAT_A8L8:           return 2;
		case FORMAT_YV12_BT601:     return 3;
		case FORMAT_YV12_BT709:     return 3;
		case FORMAT_YV12_JFIF:      return 3;
		default:
			ASSERT(false);
		}

		return 1;
	}

	void *Surface::allocateBuffer(int width, int height, int depth, int border, int samples, Format format)
	{
		return allocate(size(width, height, depth, border, samples, format));
	}

	void Surface::memfill4(void *buffer, int pattern, int bytes)
	{
		while((size_t)buffer & 0x1 && bytes >= 1)
		{
			*(char*)buffer = (char)pattern;
			(char*&)buffer += 1;
			bytes -= 1;
		}

		while((size_t)buffer & 0x3 && bytes >= 2)
		{
			*(short*)buffer = (short)pattern;
			(short*&)buffer += 1;
			bytes -= 2;
		}

		#if defined(__i386__) || defined(__x86_64__)
			if(CPUID::supportsSSE())
			{
				while((size_t)buffer & 0xF && bytes >= 4)
				{
					*(int*)buffer = pattern;
					(int*&)buffer += 1;
					bytes -= 4;
				}

				__m128 quad = _mm_set_ps1((float&)pattern);

				float *pointer = (float*)buffer;
				int qxwords = bytes / 64;
				bytes -= qxwords * 64;

				while(qxwords--)
				{
					_mm_stream_ps(pointer + 0, quad);
					_mm_stream_ps(pointer + 4, quad);
					_mm_stream_ps(pointer + 8, quad);
					_mm_stream_ps(pointer + 12, quad);

					pointer += 16;
				}

				buffer = pointer;
			}
		#endif

		while(bytes >= 4)
		{
			*(int*)buffer = (int)pattern;
			(int*&)buffer += 1;
			bytes -= 4;
		}

		while(bytes >= 2)
		{
			*(short*)buffer = (short)pattern;
			(short*&)buffer += 1;
			bytes -= 2;
		}

		while(bytes >= 1)
		{
			*(char*)buffer = (char)pattern;
			(char*&)buffer += 1;
			bytes -= 1;
		}
	}

	void Surface::sync()
	{
		resource->lock(EXCLUSIVE);
		resource->unlock();
	}

	bool Surface::isEntire(const Rect& rect) const
	{
		return (rect.x0 == 0 && rect.y0 == 0 && rect.x1 == internal.width && rect.y1 == internal.height && internal.depth == 1);
	}

	Rect Surface::getRect() const
	{
		return Rect(0, 0, internal.width, internal.height);
	}

	void Surface::clearDepth(float depth, int x0, int y0, int width, int height)
	{
		if(width == 0 || height == 0)
		{
			return;
		}

		if(internal.format == FORMAT_NULL)
		{
			return;
		}

		// Not overlapping
		if(x0 > internal.width) return;
		if(y0 > internal.height) return;
		if(x0 + width < 0) return;
		if(y0 + height < 0) return;

		// Clip against dimensions
		if(x0 < 0) {width += x0; x0 = 0;}
		if(x0 + width > internal.width) width = internal.width - x0;
		if(y0 < 0) {height += y0; y0 = 0;}
		if(y0 + height > internal.height) height = internal.height - y0;

		const bool entire = x0 == 0 && y0 == 0 && width == internal.width && height == internal.height;
		const Lock lock = entire ? LOCK_DISCARD : LOCK_WRITEONLY;

		int x1 = x0 + width;
		int y1 = y0 + height;

		if(!hasQuadLayout(internal.format))
		{
			float *target = (float*)lockInternal(x0, y0, 0, lock, PUBLIC);

			for(int z = 0; z < internal.samples; z++)
			{
				float *row = target;
				for(int y = y0; y < y1; y++)
				{
					memfill4(row, (int&)depth, width * sizeof(float));
					row += internal.pitchP;
				}
				target += internal.sliceP;
			}

			unlockInternal();
		}
		else   // Quad layout
		{
			if(complementaryDepthBuffer)
			{
				depth = 1 - depth;
			}

			float *buffer = (float*)lockInternal(0, 0, 0, lock, PUBLIC);

			int oddX0 = (x0 & ~1) * 2 + (x0 & 1);
			int oddX1 = (x1 & ~1) * 2;
			int evenX0 = ((x0 + 1) & ~1) * 2;
			int evenBytes = (oddX1 - evenX0) * sizeof(float);

			for(int z = 0; z < internal.samples; z++)
			{
				for(int y = y0; y < y1; y++)
				{
					float *target = buffer + (y & ~1) * internal.pitchP + (y & 1) * 2;

					if((y & 1) == 0 && y + 1 < y1)   // Fill quad line at once
					{
						if((x0 & 1) != 0)
						{
							target[oddX0 + 0] = depth;
							target[oddX0 + 2] = depth;
						}

					//	for(int x2 = evenX0; x2 < x1 * 2; x2 += 4)
					//	{
					//		target[x2 + 0] = depth;
					//		target[x2 + 1] = depth;
					//		target[x2 + 2] = depth;
					//		target[x2 + 3] = depth;
					//	}

					//	__asm
					//	{
					//		movss xmm0, depth
					//		shufps xmm0, xmm0, 0x00
					//
					//		mov eax, x0
					//		add eax, 1
					//		and eax, 0xFFFFFFFE
					//		cmp eax, x1
					//		jge qEnd
					//
					//		mov edi, target
					//
					//	qLoop:
					//		movntps [edi+8*eax], xmm0
					//
					//		add eax, 2
					//		cmp eax, x1
					//		jl qLoop
					//	qEnd:
					//	}

						memfill4(&target[evenX0], (int&)depth, evenBytes);

						if((x1 & 1) != 0)
						{
							target[oddX1 + 0] = depth;
							target[oddX1 + 2] = depth;
						}

						y++;
					}
					else
					{
						for(int x = x0, i = oddX0; x < x1; x++, i = (x & ~1) * 2 + (x & 1))
						{
							target[i] = depth;
						}
					}
				}

				buffer += internal.sliceP;
			}

			unlockInternal();
		}
	}

	void Surface::clearStencil(unsigned char s, unsigned char mask, int x0, int y0, int width, int height)
	{
		if(mask == 0 || width == 0 || height == 0)
		{
			return;
		}

		if(stencil.format == FORMAT_NULL)
		{
			return;
		}

		// Not overlapping
		if(x0 > internal.width) return;
		if(y0 > internal.height) return;
		if(x0 + width < 0) return;
		if(y0 + height < 0) return;

		// Clip against dimensions
		if(x0 < 0) {width += x0; x0 = 0;}
		if(x0 + width > internal.width) width = internal.width - x0;
		if(y0 < 0) {height += y0; y0 = 0;}
		if(y0 + height > internal.height) height = internal.height - y0;

		int x1 = x0 + width;
		int y1 = y0 + height;

		int oddX0 = (x0 & ~1) * 2 + (x0 & 1);
		int oddX1 = (x1 & ~1) * 2;
		int evenX0 = ((x0 + 1) & ~1) * 2;
		int evenBytes = oddX1 - evenX0;

		unsigned char maskedS = s & mask;
		unsigned char invMask = ~mask;
		unsigned int fill = maskedS;
		fill = fill | (fill << 8) | (fill << 16) | (fill << 24);

		char *buffer = (char*)lockStencil(0, 0, 0, PUBLIC);

		// Stencil buffers are assumed to use quad layout
		for(int z = 0; z < stencil.samples; z++)
		{
			for(int y = y0; y < y1; y++)
			{
				char *target = buffer + (y & ~1) * stencil.pitchP + (y & 1) * 2;

				if((y & 1) == 0 && y + 1 < y1 && mask == 0xFF)   // Fill quad line at once
				{
					if((x0 & 1) != 0)
					{
						target[oddX0 + 0] = fill;
						target[oddX0 + 2] = fill;
					}

					memfill4(&target[evenX0], fill, evenBytes);

					if((x1 & 1) != 0)
					{
						target[oddX1 + 0] = fill;
						target[oddX1 + 2] = fill;
					}

					y++;
				}
				else
				{
					for(int x = x0; x < x1; x++)
					{
						int i = (x & ~1) * 2 + (x & 1);
						target[i] = maskedS | (target[i] & invMask);
					}
				}
			}

			buffer += stencil.sliceP;
		}

		unlockStencil();
	}

	void Surface::fill(const Color<float> &color, int x0, int y0, int width, int height)
	{
		unsigned char *row;
		Buffer *buffer;

		if(internal.dirty)
		{
			row = (unsigned char*)lockInternal(x0, y0, 0, LOCK_WRITEONLY, PUBLIC);
			buffer = &internal;
		}
		else
		{
			row = (unsigned char*)lockExternal(x0, y0, 0, LOCK_WRITEONLY, PUBLIC);
			buffer = &external;
		}

		if(buffer->bytes <= 4)
		{
			int c;
			buffer->write(&c, color);

			if(buffer->bytes <= 1) c = (c << 8)  | c;
			if(buffer->bytes <= 2) c = (c << 16) | c;

			for(int y = 0; y < height; y++)
			{
				memfill4(row, c, width * buffer->bytes);

				row += buffer->pitchB;
			}
		}
		else   // Generic
		{
			for(int y = 0; y < height; y++)
			{
				unsigned char *element = row;

				for(int x = 0; x < width; x++)
				{
					buffer->write(element, color);

					element += buffer->bytes;
				}

				row += buffer->pitchB;
			}
		}

		if(buffer == &internal)
		{
			unlockInternal();
		}
		else
		{
			unlockExternal();
		}
	}

	void Surface::copyInternal(const Surface *source, int x, int y, float srcX, float srcY, bool filter)
	{
		ASSERT(internal.lock != LOCK_UNLOCKED && source && source->internal.lock != LOCK_UNLOCKED);

		sw::Color<float> color;

		if(!filter)
		{
			color = source->internal.read((int)srcX, (int)srcY, 0);
		}
		else   // Bilinear filtering
		{
			color = source->internal.sample(srcX, srcY, 0);
		}

		internal.write(x, y, color);
	}

	void Surface::copyInternal(const Surface *source, int x, int y, int z, float srcX, float srcY, float srcZ, bool filter)
	{
		ASSERT(internal.lock != LOCK_UNLOCKED && source && source->internal.lock != LOCK_UNLOCKED);

		sw::Color<float> color;

		if(!filter)
		{
			color = source->internal.read((int)srcX, (int)srcY, int(srcZ));
		}
		else   // Bilinear filtering
		{
			color = source->internal.sample(srcX, srcY, srcZ);
		}

		internal.write(x, y, z, color);
	}

	void Surface::copyCubeEdge(Edge dstEdge, Surface *src, Edge srcEdge)
	{
		Surface *dst = this;

		// Figure out if the edges to be copied in reverse order respectively from one another
		// The copy should be reversed whenever the same edges are contiguous or if we're
		// copying top <-> right or bottom <-> left. This is explained by the layout, which is:
		//
		//      | +y |
		// | -x | +z | +x | -z |
		//      | -y |

		bool reverse = (srcEdge == dstEdge) ||
		               ((srcEdge == TOP) && (dstEdge == RIGHT)) ||
		               ((srcEdge == RIGHT) && (dstEdge == TOP)) ||
		               ((srcEdge == BOTTOM) && (dstEdge == LEFT)) ||
		               ((srcEdge == LEFT) && (dstEdge == BOTTOM));

		int srcBytes = src->bytes(src->Surface::getInternalFormat());
		int srcPitch = src->getInternalPitchB();
		int dstBytes = dst->bytes(dst->Surface::getInternalFormat());
		int dstPitch = dst->getInternalPitchB();

		int srcW = src->getWidth();
		int srcH = src->getHeight();
		int dstW = dst->getWidth();
		int dstH = dst->getHeight();

		ASSERT(srcW == srcH && dstW == dstH && srcW == dstW && srcBytes == dstBytes);

		// Src is expressed in the regular [0, width-1], [0, height-1] space
		int srcDelta = ((srcEdge == TOP) || (srcEdge == BOTTOM)) ? srcBytes : srcPitch;
		int srcStart = ((srcEdge == BOTTOM) ? srcPitch * (srcH - 1) : ((srcEdge == RIGHT) ? srcBytes * (srcW - 1) : 0));

		// Dst contains borders, so it is expressed in the [-1, width+1], [-1, height+1] space
		int dstDelta = (((dstEdge == TOP) || (dstEdge == BOTTOM)) ? dstBytes : dstPitch) * (reverse ? -1 : 1);
		int dstStart = ((dstEdge == BOTTOM) ? dstPitch * (dstH + 1) : ((dstEdge == RIGHT) ? dstBytes * (dstW + 1) : 0)) + (reverse ? dstW * -dstDelta : dstDelta);

		char *srcBuf = (char*)src->lockInternal(0, 0, 0, sw::LOCK_READONLY, sw::PRIVATE) + srcStart;
		char *dstBuf = (char*)dst->lockInternal(-1, -1, 0, sw::LOCK_READWRITE, sw::PRIVATE) + dstStart;

		for(int i = 0; i < srcW; ++i, dstBuf += dstDelta, srcBuf += srcDelta)
		{
			memcpy(dstBuf, srcBuf, srcBytes);
		}

		if(dstEdge == LEFT || dstEdge == RIGHT)
		{
			// TOP and BOTTOM are already set, let's average out the corners
			int x0 = (dstEdge == RIGHT) ? dstW : -1;
			int y0 = -1;
			int x1 = (dstEdge == RIGHT) ? dstW - 1 : 0;
			int y1 = 0;
			dst->computeCubeCorner(x0, y0, x1, y1);
			y0 = dstH;
			y1 = dstH - 1;
			dst->computeCubeCorner(x0, y0, x1, y1);
		}

		src->unlockInternal();
		dst->unlockInternal();
	}

	void Surface::computeCubeCorner(int x0, int y0, int x1, int y1)
	{
		ASSERT(internal.lock != LOCK_UNLOCKED);

		sw::Color<float> color = internal.read(x0, y1);
		color += internal.read(x1, y0);
		color += internal.read(x1, y1);
		color *= (1.0f / 3.0f);

		internal.write(x0, y0, color);
	}

	bool Surface::hasStencil() const
	{
		return isStencil(external.format);
	}

	bool Surface::hasDepth() const
	{
		return isDepth(external.format);
	}

	bool Surface::hasPalette() const
	{
		return isPalette(external.format);
	}

	bool Surface::isRenderTarget() const
	{
		return renderTarget;
	}

	bool Surface::hasDirtyContents() const
	{
		return dirtyContents;
	}

	void Surface::markContentsClean()
	{
		dirtyContents = false;
	}

	Resource *Surface::getResource()
	{
		return resource;
	}

	bool Surface::identicalFormats() const
	{
		return external.format == internal.format &&
		       external.width  == internal.width &&
		       external.height == internal.height &&
		       external.depth  == internal.depth &&
		       external.pitchB == internal.pitchB &&
		       external.sliceB == internal.sliceB &&
		       external.border == internal.border &&
		       external.samples == internal.samples;
	}

	Format Surface::selectInternalFormat(Format format) const
	{
		switch(format)
		{
		case FORMAT_NULL:
			return FORMAT_NULL;
		case FORMAT_P8:
		case FORMAT_A8P8:
		case FORMAT_A4R4G4B4:
		case FORMAT_A1R5G5B5:
		case FORMAT_A8R3G3B2:
			return FORMAT_A8R8G8B8;
		case FORMAT_A8:
			return FORMAT_A8;
		case FORMAT_R8I:
			return FORMAT_R8I;
		case FORMAT_R8UI:
			return FORMAT_R8UI;
		case FORMAT_R8_SNORM:
			return FORMAT_R8_SNORM;
		case FORMAT_R8:
			return FORMAT_R8;
		case FORMAT_R16I:
			return FORMAT_R16I;
		case FORMAT_R16UI:
			return FORMAT_R16UI;
		case FORMAT_R32I:
			return FORMAT_R32I;
		case FORMAT_R32UI:
			return FORMAT_R32UI;
		case FORMAT_X16B16G16R16I:
			return FORMAT_X16B16G16R16I;
		case FORMAT_A16B16G16R16I:
			return FORMAT_A16B16G16R16I;
		case FORMAT_X16B16G16R16UI:
			return FORMAT_X16B16G16R16UI;
		case FORMAT_A16B16G16R16UI:
			return FORMAT_A16B16G16R16UI;
		case FORMAT_A2R10G10B10:
		case FORMAT_A2B10G10R10:
		case FORMAT_A16B16G16R16:
			return FORMAT_A16B16G16R16;
		case FORMAT_A2B10G10R10UI:
			return FORMAT_A16B16G16R16UI;
		case FORMAT_X32B32G32R32I:
			return FORMAT_X32B32G32R32I;
		case FORMAT_A32B32G32R32I:
			return FORMAT_A32B32G32R32I;
		case FORMAT_X32B32G32R32UI:
			return FORMAT_X32B32G32R32UI;
		case FORMAT_A32B32G32R32UI:
			return FORMAT_A32B32G32R32UI;
		case FORMAT_G8R8I:
			return FORMAT_G8R8I;
		case FORMAT_G8R8UI:
			return FORMAT_G8R8UI;
		case FORMAT_G8R8_SNORM:
			return FORMAT_G8R8_SNORM;
		case FORMAT_G8R8:
			return FORMAT_G8R8;
		case FORMAT_G16R16I:
			return FORMAT_G16R16I;
		case FORMAT_G16R16UI:
			return FORMAT_G16R16UI;
		case FORMAT_G16R16:
			return FORMAT_G16R16;
		case FORMAT_G32R32I:
			return FORMAT_G32R32I;
		case FORMAT_G32R32UI:
			return FORMAT_G32R32UI;
		case FORMAT_A8R8G8B8:
			if(lockable || !quadLayoutEnabled)
			{
				return FORMAT_A8R8G8B8;
			}
			else
			{
				return FORMAT_A8G8R8B8Q;
			}
		case FORMAT_A8B8G8R8I:
			return FORMAT_A8B8G8R8I;
		case FORMAT_A8B8G8R8UI:
			return FORMAT_A8B8G8R8UI;
		case FORMAT_A8B8G8R8_SNORM:
			return FORMAT_A8B8G8R8_SNORM;
		case FORMAT_R5G5B5A1:
		case FORMAT_R4G4B4A4:
		case FORMAT_A8B8G8R8:
			return FORMAT_A8B8G8R8;
		case FORMAT_R5G6B5:
			return FORMAT_R5G6B5;
		case FORMAT_R3G3B2:
		case FORMAT_R8G8B8:
		case FORMAT_X4R4G4B4:
		case FORMAT_X1R5G5B5:
		case FORMAT_X8R8G8B8:
			if(lockable || !quadLayoutEnabled)
			{
				return FORMAT_X8R8G8B8;
			}
			else
			{
				return FORMAT_X8G8R8B8Q;
			}
		case FORMAT_X8B8G8R8I:
			return FORMAT_X8B8G8R8I;
		case FORMAT_X8B8G8R8UI:
			return FORMAT_X8B8G8R8UI;
		case FORMAT_X8B8G8R8_SNORM:
			return FORMAT_X8B8G8R8_SNORM;
		case FORMAT_B8G8R8:
		case FORMAT_X8B8G8R8:
			return FORMAT_X8B8G8R8;
		case FORMAT_SRGB8_X8:
			return FORMAT_SRGB8_X8;
		case FORMAT_SRGB8_A8:
			return FORMAT_SRGB8_A8;
		// Compressed formats
		case FORMAT_DXT1:
		case FORMAT_DXT3:
		case FORMAT_DXT5:
		case FORMAT_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		case FORMAT_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
		case FORMAT_RGBA8_ETC2_EAC:
		case FORMAT_SRGB8_ALPHA8_ETC2_EAC:
		case FORMAT_SRGB8_ALPHA8_ASTC_4x4_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_5x4_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_5x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_6x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_6x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_8x8_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x5_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x6_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x8_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_10x10_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_12x10_KHR:
		case FORMAT_SRGB8_ALPHA8_ASTC_12x12_KHR:
			return FORMAT_A8R8G8B8;
		case FORMAT_RGBA_ASTC_4x4_KHR:
		case FORMAT_RGBA_ASTC_5x4_KHR:
		case FORMAT_RGBA_ASTC_5x5_KHR:
		case FORMAT_RGBA_ASTC_6x5_KHR:
		case FORMAT_RGBA_ASTC_6x6_KHR:
		case FORMAT_RGBA_ASTC_8x5_KHR:
		case FORMAT_RGBA_ASTC_8x6_KHR:
		case FORMAT_RGBA_ASTC_8x8_KHR:
		case FORMAT_RGBA_ASTC_10x5_KHR:
		case FORMAT_RGBA_ASTC_10x6_KHR:
		case FORMAT_RGBA_ASTC_10x8_KHR:
		case FORMAT_RGBA_ASTC_10x10_KHR:
		case FORMAT_RGBA_ASTC_12x10_KHR:
		case FORMAT_RGBA_ASTC_12x12_KHR:
			// ASTC supports HDR, so a floating point format is required to represent it properly
			return FORMAT_A32B32G32R32F; // FIXME: 16FP is probably sufficient, but it's currently unsupported
		case FORMAT_ATI1:
			return FORMAT_R8;
		case FORMAT_R11_EAC:
		case FORMAT_SIGNED_R11_EAC:
			return FORMAT_R32F; // FIXME: Signed 8bit format would be sufficient
		case FORMAT_ATI2:
			return FORMAT_G8R8;
		case FORMAT_RG11_EAC:
		case FORMAT_SIGNED_RG11_EAC:
			return FORMAT_G32R32F; // FIXME: Signed 8bit format would be sufficient
		case FORMAT_ETC1:
		case FORMAT_RGB8_ETC2:
		case FORMAT_SRGB8_ETC2:
			return FORMAT_X8R8G8B8;
		// Bumpmap formats
		case FORMAT_V8U8:			return FORMAT_V8U8;
		case FORMAT_L6V5U5:			return FORMAT_X8L8V8U8;
		case FORMAT_Q8W8V8U8:		return FORMAT_Q8W8V8U8;
		case FORMAT_X8L8V8U8:		return FORMAT_X8L8V8U8;
		case FORMAT_V16U16:			return FORMAT_V16U16;
		case FORMAT_A2W10V10U10:	return FORMAT_A16W16V16U16;
		case FORMAT_Q16W16V16U16:	return FORMAT_Q16W16V16U16;
		// Floating-point formats
		case FORMAT_A16F:			return FORMAT_A32B32G32R32F;
		case FORMAT_R16F:			return FORMAT_R32F;
		case FORMAT_G16R16F:		return FORMAT_G32R32F;
		case FORMAT_B16G16R16F:     return FORMAT_X32B32G32R32F;
		case FORMAT_X16B16G16R16F:	return FORMAT_X32B32G32R32F;
		case FORMAT_A16B16G16R16F:	return FORMAT_A32B32G32R32F;
		case FORMAT_X16B16G16R16F_UNSIGNED: return FORMAT_X32B32G32R32F_UNSIGNED;
		case FORMAT_A32F:			return FORMAT_A32B32G32R32F;
		case FORMAT_R32F:			return FORMAT_R32F;
		case FORMAT_G32R32F:		return FORMAT_G32R32F;
		case FORMAT_B32G32R32F:     return FORMAT_X32B32G32R32F;
		case FORMAT_X32B32G32R32F:  return FORMAT_X32B32G32R32F;
		case FORMAT_A32B32G32R32F:	return FORMAT_A32B32G32R32F;
		case FORMAT_X32B32G32R32F_UNSIGNED: return FORMAT_X32B32G32R32F_UNSIGNED;
		// Luminance formats
		case FORMAT_L8:				return FORMAT_L8;
		case FORMAT_A4L4:			return FORMAT_A8L8;
		case FORMAT_L16:			return FORMAT_L16;
		case FORMAT_A8L8:			return FORMAT_A8L8;
		case FORMAT_L16F:           return FORMAT_X32B32G32R32F;
		case FORMAT_A16L16F:        return FORMAT_A32B32G32R32F;
		case FORMAT_L32F:           return FORMAT_X32B32G32R32F;
		case FORMAT_A32L32F:        return FORMAT_A32B32G32R32F;
		// Depth/stencil formats
		case FORMAT_D16:
		case FORMAT_D32:
		case FORMAT_D24X8:
			if(hasParent)   // Texture
			{
				return FORMAT_D32F_SHADOW;
			}
			else if(complementaryDepthBuffer)
			{
				return FORMAT_D32F_COMPLEMENTARY;
			}
			else
			{
				return FORMAT_D32F;
			}
		case FORMAT_D24S8:
		case FORMAT_D24FS8:
			if(hasParent)   // Texture
			{
				return FORMAT_D32FS8_SHADOW;
			}
			else if(complementaryDepthBuffer)
			{
				return FORMAT_D32FS8_COMPLEMENTARY;
			}
			else
			{
				return FORMAT_D32FS8;
			}
		case FORMAT_D32F:           return FORMAT_D32F;
		case FORMAT_D32FS8:         return FORMAT_D32FS8;
		case FORMAT_D32F_LOCKABLE:  return FORMAT_D32F_LOCKABLE;
		case FORMAT_D32FS8_TEXTURE: return FORMAT_D32FS8_TEXTURE;
		case FORMAT_INTZ:           return FORMAT_D32FS8_TEXTURE;
		case FORMAT_DF24S8:         return FORMAT_D32FS8_SHADOW;
		case FORMAT_DF16S8:         return FORMAT_D32FS8_SHADOW;
		case FORMAT_S8:             return FORMAT_S8;
		// YUV formats
		case FORMAT_YV12_BT601:     return FORMAT_YV12_BT601;
		case FORMAT_YV12_BT709:     return FORMAT_YV12_BT709;
		case FORMAT_YV12_JFIF:      return FORMAT_YV12_JFIF;
		default:
			ASSERT(false);
		}

		return FORMAT_NULL;
	}

	void Surface::setTexturePalette(unsigned int *palette)
	{
		Surface::palette = palette;
		Surface::paletteID++;
	}

	void Surface::resolve()
	{
		if(internal.samples <= 1 || !internal.dirty || !renderTarget || internal.format == FORMAT_NULL)
		{
			return;
		}

		ASSERT(internal.depth == 1);  // Unimplemented

		void *source = internal.lockRect(0, 0, 0, LOCK_READWRITE);

		int width = internal.width;
		int height = internal.height;
		int pitch = internal.pitchB;
		int slice = internal.sliceB;

		unsigned char *source0 = (unsigned char*)source;
		unsigned char *source1 = source0 + slice;
		unsigned char *source2 = source1 + slice;
		unsigned char *source3 = source2 + slice;
		unsigned char *source4 = source3 + slice;
		unsigned char *source5 = source4 + slice;
		unsigned char *source6 = source5 + slice;
		unsigned char *source7 = source6 + slice;
		unsigned char *source8 = source7 + slice;
		unsigned char *source9 = source8 + slice;
		unsigned char *sourceA = source9 + slice;
		unsigned char *sourceB = sourceA + slice;
		unsigned char *sourceC = sourceB + slice;
		unsigned char *sourceD = sourceC + slice;
		unsigned char *sourceE = sourceD + slice;
		unsigned char *sourceF = sourceE + slice;

		if(internal.format == FORMAT_X8R8G8B8 || internal.format == FORMAT_A8R8G8B8 ||
		   internal.format == FORMAT_X8B8G8R8 || internal.format == FORMAT_A8B8G8R8 ||
		   internal.format == FORMAT_SRGB8_X8 || internal.format == FORMAT_SRGB8_A8)
		{
			#if defined(__i386__) || defined(__x86_64__)
				if(CPUID::supportsSSE2() && (width % 4) == 0)
				{
					if(internal.samples == 2)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 4)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 4 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 4 * x));

								c0 = _mm_avg_epu8(c0, c1);

								_mm_store_si128((__m128i*)(source0 + 4 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
						}
					}
					else if(internal.samples == 4)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 4)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 4 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 4 * x));
								__m128i c2 = _mm_load_si128((__m128i*)(source2 + 4 * x));
								__m128i c3 = _mm_load_si128((__m128i*)(source3 + 4 * x));

								c0 = _mm_avg_epu8(c0, c1);
								c2 = _mm_avg_epu8(c2, c3);
								c0 = _mm_avg_epu8(c0, c2);

								_mm_store_si128((__m128i*)(source0 + 4 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
						}
					}
					else if(internal.samples == 8)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 4)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 4 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 4 * x));
								__m128i c2 = _mm_load_si128((__m128i*)(source2 + 4 * x));
								__m128i c3 = _mm_load_si128((__m128i*)(source3 + 4 * x));
								__m128i c4 = _mm_load_si128((__m128i*)(source4 + 4 * x));
								__m128i c5 = _mm_load_si128((__m128i*)(source5 + 4 * x));
								__m128i c6 = _mm_load_si128((__m128i*)(source6 + 4 * x));
								__m128i c7 = _mm_load_si128((__m128i*)(source7 + 4 * x));

								c0 = _mm_avg_epu8(c0, c1);
								c2 = _mm_avg_epu8(c2, c3);
								c4 = _mm_avg_epu8(c4, c5);
								c6 = _mm_avg_epu8(c6, c7);
								c0 = _mm_avg_epu8(c0, c2);
								c4 = _mm_avg_epu8(c4, c6);
								c0 = _mm_avg_epu8(c0, c4);

								_mm_store_si128((__m128i*)(source0 + 4 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
						}
					}
					else if(internal.samples == 16)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 4)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 4 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 4 * x));
								__m128i c2 = _mm_load_si128((__m128i*)(source2 + 4 * x));
								__m128i c3 = _mm_load_si128((__m128i*)(source3 + 4 * x));
								__m128i c4 = _mm_load_si128((__m128i*)(source4 + 4 * x));
								__m128i c5 = _mm_load_si128((__m128i*)(source5 + 4 * x));
								__m128i c6 = _mm_load_si128((__m128i*)(source6 + 4 * x));
								__m128i c7 = _mm_load_si128((__m128i*)(source7 + 4 * x));
								__m128i c8 = _mm_load_si128((__m128i*)(source8 + 4 * x));
								__m128i c9 = _mm_load_si128((__m128i*)(source9 + 4 * x));
								__m128i cA = _mm_load_si128((__m128i*)(sourceA + 4 * x));
								__m128i cB = _mm_load_si128((__m128i*)(sourceB + 4 * x));
								__m128i cC = _mm_load_si128((__m128i*)(sourceC + 4 * x));
								__m128i cD = _mm_load_si128((__m128i*)(sourceD + 4 * x));
								__m128i cE = _mm_load_si128((__m128i*)(sourceE + 4 * x));
								__m128i cF = _mm_load_si128((__m128i*)(sourceF + 4 * x));

								c0 = _mm_avg_epu8(c0, c1);
								c2 = _mm_avg_epu8(c2, c3);
								c4 = _mm_avg_epu8(c4, c5);
								c6 = _mm_avg_epu8(c6, c7);
								c8 = _mm_avg_epu8(c8, c9);
								cA = _mm_avg_epu8(cA, cB);
								cC = _mm_avg_epu8(cC, cD);
								cE = _mm_avg_epu8(cE, cF);
								c0 = _mm_avg_epu8(c0, c2);
								c4 = _mm_avg_epu8(c4, c6);
								c8 = _mm_avg_epu8(c8, cA);
								cC = _mm_avg_epu8(cC, cE);
								c0 = _mm_avg_epu8(c0, c4);
								c8 = _mm_avg_epu8(c8, cC);
								c0 = _mm_avg_epu8(c0, c8);

								_mm_store_si128((__m128i*)(source0 + 4 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
							source8 += pitch;
							source9 += pitch;
							sourceA += pitch;
							sourceB += pitch;
							sourceC += pitch;
							sourceD += pitch;
							sourceE += pitch;
							sourceF += pitch;
						}
					}
					else ASSERT(false);
				}
				else
			#endif
			{
				#define AVERAGE(x, y) (((x) & (y)) + ((((x) ^ (y)) >> 1) & 0x7F7F7F7F) + (((x) ^ (y)) & 0x01010101))

				if(internal.samples == 2)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							unsigned int c0 = *(unsigned int*)(source0 + 4 * x);
							unsigned int c1 = *(unsigned int*)(source1 + 4 * x);

							c0 = AVERAGE(c0, c1);

							*(unsigned int*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
					}
				}
				else if(internal.samples == 4)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							unsigned int c0 = *(unsigned int*)(source0 + 4 * x);
							unsigned int c1 = *(unsigned int*)(source1 + 4 * x);
							unsigned int c2 = *(unsigned int*)(source2 + 4 * x);
							unsigned int c3 = *(unsigned int*)(source3 + 4 * x);

							c0 = AVERAGE(c0, c1);
							c2 = AVERAGE(c2, c3);
							c0 = AVERAGE(c0, c2);

							*(unsigned int*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
					}
				}
				else if(internal.samples == 8)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							unsigned int c0 = *(unsigned int*)(source0 + 4 * x);
							unsigned int c1 = *(unsigned int*)(source1 + 4 * x);
							unsigned int c2 = *(unsigned int*)(source2 + 4 * x);
							unsigned int c3 = *(unsigned int*)(source3 + 4 * x);
							unsigned int c4 = *(unsigned int*)(source4 + 4 * x);
							unsigned int c5 = *(unsigned int*)(source5 + 4 * x);
							unsigned int c6 = *(unsigned int*)(source6 + 4 * x);
							unsigned int c7 = *(unsigned int*)(source7 + 4 * x);

							c0 = AVERAGE(c0, c1);
							c2 = AVERAGE(c2, c3);
							c4 = AVERAGE(c4, c5);
							c6 = AVERAGE(c6, c7);
							c0 = AVERAGE(c0, c2);
							c4 = AVERAGE(c4, c6);
							c0 = AVERAGE(c0, c4);

							*(unsigned int*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
					}
				}
				else if(internal.samples == 16)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							unsigned int c0 = *(unsigned int*)(source0 + 4 * x);
							unsigned int c1 = *(unsigned int*)(source1 + 4 * x);
							unsigned int c2 = *(unsigned int*)(source2 + 4 * x);
							unsigned int c3 = *(unsigned int*)(source3 + 4 * x);
							unsigned int c4 = *(unsigned int*)(source4 + 4 * x);
							unsigned int c5 = *(unsigned int*)(source5 + 4 * x);
							unsigned int c6 = *(unsigned int*)(source6 + 4 * x);
							unsigned int c7 = *(unsigned int*)(source7 + 4 * x);
							unsigned int c8 = *(unsigned int*)(source8 + 4 * x);
							unsigned int c9 = *(unsigned int*)(source9 + 4 * x);
							unsigned int cA = *(unsigned int*)(sourceA + 4 * x);
							unsigned int cB = *(unsigned int*)(sourceB + 4 * x);
							unsigned int cC = *(unsigned int*)(sourceC + 4 * x);
							unsigned int cD = *(unsigned int*)(sourceD + 4 * x);
							unsigned int cE = *(unsigned int*)(sourceE + 4 * x);
							unsigned int cF = *(unsigned int*)(sourceF + 4 * x);

							c0 = AVERAGE(c0, c1);
							c2 = AVERAGE(c2, c3);
							c4 = AVERAGE(c4, c5);
							c6 = AVERAGE(c6, c7);
							c8 = AVERAGE(c8, c9);
							cA = AVERAGE(cA, cB);
							cC = AVERAGE(cC, cD);
							cE = AVERAGE(cE, cF);
							c0 = AVERAGE(c0, c2);
							c4 = AVERAGE(c4, c6);
							c8 = AVERAGE(c8, cA);
							cC = AVERAGE(cC, cE);
							c0 = AVERAGE(c0, c4);
							c8 = AVERAGE(c8, cC);
							c0 = AVERAGE(c0, c8);

							*(unsigned int*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
						source8 += pitch;
						source9 += pitch;
						sourceA += pitch;
						sourceB += pitch;
						sourceC += pitch;
						sourceD += pitch;
						sourceE += pitch;
						sourceF += pitch;
					}
				}
				else ASSERT(false);

				#undef AVERAGE
			}
		}
		else if(internal.format == FORMAT_G16R16)
		{

			#if defined(__i386__) || defined(__x86_64__)
				if(CPUID::supportsSSE2() && (width % 4) == 0)
				{
					if(internal.samples == 2)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 4)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 4 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 4 * x));

								c0 = _mm_avg_epu16(c0, c1);

								_mm_store_si128((__m128i*)(source0 + 4 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
						}
					}
					else if(internal.samples == 4)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 4)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 4 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 4 * x));
								__m128i c2 = _mm_load_si128((__m128i*)(source2 + 4 * x));
								__m128i c3 = _mm_load_si128((__m128i*)(source3 + 4 * x));

								c0 = _mm_avg_epu16(c0, c1);
								c2 = _mm_avg_epu16(c2, c3);
								c0 = _mm_avg_epu16(c0, c2);

								_mm_store_si128((__m128i*)(source0 + 4 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
						}
					}
					else if(internal.samples == 8)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 4)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 4 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 4 * x));
								__m128i c2 = _mm_load_si128((__m128i*)(source2 + 4 * x));
								__m128i c3 = _mm_load_si128((__m128i*)(source3 + 4 * x));
								__m128i c4 = _mm_load_si128((__m128i*)(source4 + 4 * x));
								__m128i c5 = _mm_load_si128((__m128i*)(source5 + 4 * x));
								__m128i c6 = _mm_load_si128((__m128i*)(source6 + 4 * x));
								__m128i c7 = _mm_load_si128((__m128i*)(source7 + 4 * x));

								c0 = _mm_avg_epu16(c0, c1);
								c2 = _mm_avg_epu16(c2, c3);
								c4 = _mm_avg_epu16(c4, c5);
								c6 = _mm_avg_epu16(c6, c7);
								c0 = _mm_avg_epu16(c0, c2);
								c4 = _mm_avg_epu16(c4, c6);
								c0 = _mm_avg_epu16(c0, c4);

								_mm_store_si128((__m128i*)(source0 + 4 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
						}
					}
					else if(internal.samples == 16)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 4)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 4 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 4 * x));
								__m128i c2 = _mm_load_si128((__m128i*)(source2 + 4 * x));
								__m128i c3 = _mm_load_si128((__m128i*)(source3 + 4 * x));
								__m128i c4 = _mm_load_si128((__m128i*)(source4 + 4 * x));
								__m128i c5 = _mm_load_si128((__m128i*)(source5 + 4 * x));
								__m128i c6 = _mm_load_si128((__m128i*)(source6 + 4 * x));
								__m128i c7 = _mm_load_si128((__m128i*)(source7 + 4 * x));
								__m128i c8 = _mm_load_si128((__m128i*)(source8 + 4 * x));
								__m128i c9 = _mm_load_si128((__m128i*)(source9 + 4 * x));
								__m128i cA = _mm_load_si128((__m128i*)(sourceA + 4 * x));
								__m128i cB = _mm_load_si128((__m128i*)(sourceB + 4 * x));
								__m128i cC = _mm_load_si128((__m128i*)(sourceC + 4 * x));
								__m128i cD = _mm_load_si128((__m128i*)(sourceD + 4 * x));
								__m128i cE = _mm_load_si128((__m128i*)(sourceE + 4 * x));
								__m128i cF = _mm_load_si128((__m128i*)(sourceF + 4 * x));

								c0 = _mm_avg_epu16(c0, c1);
								c2 = _mm_avg_epu16(c2, c3);
								c4 = _mm_avg_epu16(c4, c5);
								c6 = _mm_avg_epu16(c6, c7);
								c8 = _mm_avg_epu16(c8, c9);
								cA = _mm_avg_epu16(cA, cB);
								cC = _mm_avg_epu16(cC, cD);
								cE = _mm_avg_epu16(cE, cF);
								c0 = _mm_avg_epu16(c0, c2);
								c4 = _mm_avg_epu16(c4, c6);
								c8 = _mm_avg_epu16(c8, cA);
								cC = _mm_avg_epu16(cC, cE);
								c0 = _mm_avg_epu16(c0, c4);
								c8 = _mm_avg_epu16(c8, cC);
								c0 = _mm_avg_epu16(c0, c8);

								_mm_store_si128((__m128i*)(source0 + 4 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
							source8 += pitch;
							source9 += pitch;
							sourceA += pitch;
							sourceB += pitch;
							sourceC += pitch;
							sourceD += pitch;
							sourceE += pitch;
							sourceF += pitch;
						}
					}
					else ASSERT(false);
				}
				else
			#endif
			{
				#define AVERAGE(x, y) (((x) & (y)) + ((((x) ^ (y)) >> 1) & 0x7FFF7FFF) + (((x) ^ (y)) & 0x00010001))

				if(internal.samples == 2)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							unsigned int c0 = *(unsigned int*)(source0 + 4 * x);
							unsigned int c1 = *(unsigned int*)(source1 + 4 * x);

							c0 = AVERAGE(c0, c1);

							*(unsigned int*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
					}
				}
				else if(internal.samples == 4)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							unsigned int c0 = *(unsigned int*)(source0 + 4 * x);
							unsigned int c1 = *(unsigned int*)(source1 + 4 * x);
							unsigned int c2 = *(unsigned int*)(source2 + 4 * x);
							unsigned int c3 = *(unsigned int*)(source3 + 4 * x);

							c0 = AVERAGE(c0, c1);
							c2 = AVERAGE(c2, c3);
							c0 = AVERAGE(c0, c2);

							*(unsigned int*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
					}
				}
				else if(internal.samples == 8)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							unsigned int c0 = *(unsigned int*)(source0 + 4 * x);
							unsigned int c1 = *(unsigned int*)(source1 + 4 * x);
							unsigned int c2 = *(unsigned int*)(source2 + 4 * x);
							unsigned int c3 = *(unsigned int*)(source3 + 4 * x);
							unsigned int c4 = *(unsigned int*)(source4 + 4 * x);
							unsigned int c5 = *(unsigned int*)(source5 + 4 * x);
							unsigned int c6 = *(unsigned int*)(source6 + 4 * x);
							unsigned int c7 = *(unsigned int*)(source7 + 4 * x);

							c0 = AVERAGE(c0, c1);
							c2 = AVERAGE(c2, c3);
							c4 = AVERAGE(c4, c5);
							c6 = AVERAGE(c6, c7);
							c0 = AVERAGE(c0, c2);
							c4 = AVERAGE(c4, c6);
							c0 = AVERAGE(c0, c4);

							*(unsigned int*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
					}
				}
				else if(internal.samples == 16)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							unsigned int c0 = *(unsigned int*)(source0 + 4 * x);
							unsigned int c1 = *(unsigned int*)(source1 + 4 * x);
							unsigned int c2 = *(unsigned int*)(source2 + 4 * x);
							unsigned int c3 = *(unsigned int*)(source3 + 4 * x);
							unsigned int c4 = *(unsigned int*)(source4 + 4 * x);
							unsigned int c5 = *(unsigned int*)(source5 + 4 * x);
							unsigned int c6 = *(unsigned int*)(source6 + 4 * x);
							unsigned int c7 = *(unsigned int*)(source7 + 4 * x);
							unsigned int c8 = *(unsigned int*)(source8 + 4 * x);
							unsigned int c9 = *(unsigned int*)(source9 + 4 * x);
							unsigned int cA = *(unsigned int*)(sourceA + 4 * x);
							unsigned int cB = *(unsigned int*)(sourceB + 4 * x);
							unsigned int cC = *(unsigned int*)(sourceC + 4 * x);
							unsigned int cD = *(unsigned int*)(sourceD + 4 * x);
							unsigned int cE = *(unsigned int*)(sourceE + 4 * x);
							unsigned int cF = *(unsigned int*)(sourceF + 4 * x);

							c0 = AVERAGE(c0, c1);
							c2 = AVERAGE(c2, c3);
							c4 = AVERAGE(c4, c5);
							c6 = AVERAGE(c6, c7);
							c8 = AVERAGE(c8, c9);
							cA = AVERAGE(cA, cB);
							cC = AVERAGE(cC, cD);
							cE = AVERAGE(cE, cF);
							c0 = AVERAGE(c0, c2);
							c4 = AVERAGE(c4, c6);
							c8 = AVERAGE(c8, cA);
							cC = AVERAGE(cC, cE);
							c0 = AVERAGE(c0, c4);
							c8 = AVERAGE(c8, cC);
							c0 = AVERAGE(c0, c8);

							*(unsigned int*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
						source8 += pitch;
						source9 += pitch;
						sourceA += pitch;
						sourceB += pitch;
						sourceC += pitch;
						sourceD += pitch;
						sourceE += pitch;
						sourceF += pitch;
					}
				}
				else ASSERT(false);

				#undef AVERAGE
			}
		}
		else if(internal.format == FORMAT_A16B16G16R16)
		{
			#if defined(__i386__) || defined(__x86_64__)
				if(CPUID::supportsSSE2() && (width % 2) == 0)
				{
					if(internal.samples == 2)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 2)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 8 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 8 * x));

								c0 = _mm_avg_epu16(c0, c1);

								_mm_store_si128((__m128i*)(source0 + 8 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
						}
					}
					else if(internal.samples == 4)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 2)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 8 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 8 * x));
								__m128i c2 = _mm_load_si128((__m128i*)(source2 + 8 * x));
								__m128i c3 = _mm_load_si128((__m128i*)(source3 + 8 * x));

								c0 = _mm_avg_epu16(c0, c1);
								c2 = _mm_avg_epu16(c2, c3);
								c0 = _mm_avg_epu16(c0, c2);

								_mm_store_si128((__m128i*)(source0 + 8 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
						}
					}
					else if(internal.samples == 8)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 2)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 8 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 8 * x));
								__m128i c2 = _mm_load_si128((__m128i*)(source2 + 8 * x));
								__m128i c3 = _mm_load_si128((__m128i*)(source3 + 8 * x));
								__m128i c4 = _mm_load_si128((__m128i*)(source4 + 8 * x));
								__m128i c5 = _mm_load_si128((__m128i*)(source5 + 8 * x));
								__m128i c6 = _mm_load_si128((__m128i*)(source6 + 8 * x));
								__m128i c7 = _mm_load_si128((__m128i*)(source7 + 8 * x));

								c0 = _mm_avg_epu16(c0, c1);
								c2 = _mm_avg_epu16(c2, c3);
								c4 = _mm_avg_epu16(c4, c5);
								c6 = _mm_avg_epu16(c6, c7);
								c0 = _mm_avg_epu16(c0, c2);
								c4 = _mm_avg_epu16(c4, c6);
								c0 = _mm_avg_epu16(c0, c4);

								_mm_store_si128((__m128i*)(source0 + 8 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
						}
					}
					else if(internal.samples == 16)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 2)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 8 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 8 * x));
								__m128i c2 = _mm_load_si128((__m128i*)(source2 + 8 * x));
								__m128i c3 = _mm_load_si128((__m128i*)(source3 + 8 * x));
								__m128i c4 = _mm_load_si128((__m128i*)(source4 + 8 * x));
								__m128i c5 = _mm_load_si128((__m128i*)(source5 + 8 * x));
								__m128i c6 = _mm_load_si128((__m128i*)(source6 + 8 * x));
								__m128i c7 = _mm_load_si128((__m128i*)(source7 + 8 * x));
								__m128i c8 = _mm_load_si128((__m128i*)(source8 + 8 * x));
								__m128i c9 = _mm_load_si128((__m128i*)(source9 + 8 * x));
								__m128i cA = _mm_load_si128((__m128i*)(sourceA + 8 * x));
								__m128i cB = _mm_load_si128((__m128i*)(sourceB + 8 * x));
								__m128i cC = _mm_load_si128((__m128i*)(sourceC + 8 * x));
								__m128i cD = _mm_load_si128((__m128i*)(sourceD + 8 * x));
								__m128i cE = _mm_load_si128((__m128i*)(sourceE + 8 * x));
								__m128i cF = _mm_load_si128((__m128i*)(sourceF + 8 * x));

								c0 = _mm_avg_epu16(c0, c1);
								c2 = _mm_avg_epu16(c2, c3);
								c4 = _mm_avg_epu16(c4, c5);
								c6 = _mm_avg_epu16(c6, c7);
								c8 = _mm_avg_epu16(c8, c9);
								cA = _mm_avg_epu16(cA, cB);
								cC = _mm_avg_epu16(cC, cD);
								cE = _mm_avg_epu16(cE, cF);
								c0 = _mm_avg_epu16(c0, c2);
								c4 = _mm_avg_epu16(c4, c6);
								c8 = _mm_avg_epu16(c8, cA);
								cC = _mm_avg_epu16(cC, cE);
								c0 = _mm_avg_epu16(c0, c4);
								c8 = _mm_avg_epu16(c8, cC);
								c0 = _mm_avg_epu16(c0, c8);

								_mm_store_si128((__m128i*)(source0 + 8 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
							source8 += pitch;
							source9 += pitch;
							sourceA += pitch;
							sourceB += pitch;
							sourceC += pitch;
							sourceD += pitch;
							sourceE += pitch;
							sourceF += pitch;
						}
					}
					else ASSERT(false);
				}
				else
			#endif
			{
				#define AVERAGE(x, y) (((x) & (y)) + ((((x) ^ (y)) >> 1) & 0x7FFF7FFF) + (((x) ^ (y)) & 0x00010001))

				if(internal.samples == 2)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < 2 * width; x++)
						{
							unsigned int c0 = *(unsigned int*)(source0 + 4 * x);
							unsigned int c1 = *(unsigned int*)(source1 + 4 * x);

							c0 = AVERAGE(c0, c1);

							*(unsigned int*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
					}
				}
				else if(internal.samples == 4)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < 2 * width; x++)
						{
							unsigned int c0 = *(unsigned int*)(source0 + 4 * x);
							unsigned int c1 = *(unsigned int*)(source1 + 4 * x);
							unsigned int c2 = *(unsigned int*)(source2 + 4 * x);
							unsigned int c3 = *(unsigned int*)(source3 + 4 * x);

							c0 = AVERAGE(c0, c1);
							c2 = AVERAGE(c2, c3);
							c0 = AVERAGE(c0, c2);

							*(unsigned int*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
					}
				}
				else if(internal.samples == 8)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < 2 * width; x++)
						{
							unsigned int c0 = *(unsigned int*)(source0 + 4 * x);
							unsigned int c1 = *(unsigned int*)(source1 + 4 * x);
							unsigned int c2 = *(unsigned int*)(source2 + 4 * x);
							unsigned int c3 = *(unsigned int*)(source3 + 4 * x);
							unsigned int c4 = *(unsigned int*)(source4 + 4 * x);
							unsigned int c5 = *(unsigned int*)(source5 + 4 * x);
							unsigned int c6 = *(unsigned int*)(source6 + 4 * x);
							unsigned int c7 = *(unsigned int*)(source7 + 4 * x);

							c0 = AVERAGE(c0, c1);
							c2 = AVERAGE(c2, c3);
							c4 = AVERAGE(c4, c5);
							c6 = AVERAGE(c6, c7);
							c0 = AVERAGE(c0, c2);
							c4 = AVERAGE(c4, c6);
							c0 = AVERAGE(c0, c4);

							*(unsigned int*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
					}
				}
				else if(internal.samples == 16)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < 2 * width; x++)
						{
							unsigned int c0 = *(unsigned int*)(source0 + 4 * x);
							unsigned int c1 = *(unsigned int*)(source1 + 4 * x);
							unsigned int c2 = *(unsigned int*)(source2 + 4 * x);
							unsigned int c3 = *(unsigned int*)(source3 + 4 * x);
							unsigned int c4 = *(unsigned int*)(source4 + 4 * x);
							unsigned int c5 = *(unsigned int*)(source5 + 4 * x);
							unsigned int c6 = *(unsigned int*)(source6 + 4 * x);
							unsigned int c7 = *(unsigned int*)(source7 + 4 * x);
							unsigned int c8 = *(unsigned int*)(source8 + 4 * x);
							unsigned int c9 = *(unsigned int*)(source9 + 4 * x);
							unsigned int cA = *(unsigned int*)(sourceA + 4 * x);
							unsigned int cB = *(unsigned int*)(sourceB + 4 * x);
							unsigned int cC = *(unsigned int*)(sourceC + 4 * x);
							unsigned int cD = *(unsigned int*)(sourceD + 4 * x);
							unsigned int cE = *(unsigned int*)(sourceE + 4 * x);
							unsigned int cF = *(unsigned int*)(sourceF + 4 * x);

							c0 = AVERAGE(c0, c1);
							c2 = AVERAGE(c2, c3);
							c4 = AVERAGE(c4, c5);
							c6 = AVERAGE(c6, c7);
							c8 = AVERAGE(c8, c9);
							cA = AVERAGE(cA, cB);
							cC = AVERAGE(cC, cD);
							cE = AVERAGE(cE, cF);
							c0 = AVERAGE(c0, c2);
							c4 = AVERAGE(c4, c6);
							c8 = AVERAGE(c8, cA);
							cC = AVERAGE(cC, cE);
							c0 = AVERAGE(c0, c4);
							c8 = AVERAGE(c8, cC);
							c0 = AVERAGE(c0, c8);

							*(unsigned int*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
						source8 += pitch;
						source9 += pitch;
						sourceA += pitch;
						sourceB += pitch;
						sourceC += pitch;
						sourceD += pitch;
						sourceE += pitch;
						sourceF += pitch;
					}
				}
				else ASSERT(false);

				#undef AVERAGE
			}
		}
		else if(internal.format == FORMAT_R32F)
		{
			#if defined(__i386__) || defined(__x86_64__)
				if(CPUID::supportsSSE() && (width % 4) == 0)
				{
					if(internal.samples == 2)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 4)
							{
								__m128 c0 = _mm_load_ps((float*)(source0 + 4 * x));
								__m128 c1 = _mm_load_ps((float*)(source1 + 4 * x));

								c0 = _mm_add_ps(c0, c1);
								c0 = _mm_mul_ps(c0, _mm_set1_ps(1.0f / 2.0f));

								_mm_store_ps((float*)(source0 + 4 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
						}
					}
					else if(internal.samples == 4)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 4)
							{
								__m128 c0 = _mm_load_ps((float*)(source0 + 4 * x));
								__m128 c1 = _mm_load_ps((float*)(source1 + 4 * x));
								__m128 c2 = _mm_load_ps((float*)(source2 + 4 * x));
								__m128 c3 = _mm_load_ps((float*)(source3 + 4 * x));

								c0 = _mm_add_ps(c0, c1);
								c2 = _mm_add_ps(c2, c3);
								c0 = _mm_add_ps(c0, c2);
								c0 = _mm_mul_ps(c0, _mm_set1_ps(1.0f / 4.0f));

								_mm_store_ps((float*)(source0 + 4 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
						}
					}
					else if(internal.samples == 8)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 4)
							{
								__m128 c0 = _mm_load_ps((float*)(source0 + 4 * x));
								__m128 c1 = _mm_load_ps((float*)(source1 + 4 * x));
								__m128 c2 = _mm_load_ps((float*)(source2 + 4 * x));
								__m128 c3 = _mm_load_ps((float*)(source3 + 4 * x));
								__m128 c4 = _mm_load_ps((float*)(source4 + 4 * x));
								__m128 c5 = _mm_load_ps((float*)(source5 + 4 * x));
								__m128 c6 = _mm_load_ps((float*)(source6 + 4 * x));
								__m128 c7 = _mm_load_ps((float*)(source7 + 4 * x));

								c0 = _mm_add_ps(c0, c1);
								c2 = _mm_add_ps(c2, c3);
								c4 = _mm_add_ps(c4, c5);
								c6 = _mm_add_ps(c6, c7);
								c0 = _mm_add_ps(c0, c2);
								c4 = _mm_add_ps(c4, c6);
								c0 = _mm_add_ps(c0, c4);
								c0 = _mm_mul_ps(c0, _mm_set1_ps(1.0f / 8.0f));

								_mm_store_ps((float*)(source0 + 4 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
						}
					}
					else if(internal.samples == 16)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 4)
							{
								__m128 c0 = _mm_load_ps((float*)(source0 + 4 * x));
								__m128 c1 = _mm_load_ps((float*)(source1 + 4 * x));
								__m128 c2 = _mm_load_ps((float*)(source2 + 4 * x));
								__m128 c3 = _mm_load_ps((float*)(source3 + 4 * x));
								__m128 c4 = _mm_load_ps((float*)(source4 + 4 * x));
								__m128 c5 = _mm_load_ps((float*)(source5 + 4 * x));
								__m128 c6 = _mm_load_ps((float*)(source6 + 4 * x));
								__m128 c7 = _mm_load_ps((float*)(source7 + 4 * x));
								__m128 c8 = _mm_load_ps((float*)(source8 + 4 * x));
								__m128 c9 = _mm_load_ps((float*)(source9 + 4 * x));
								__m128 cA = _mm_load_ps((float*)(sourceA + 4 * x));
								__m128 cB = _mm_load_ps((float*)(sourceB + 4 * x));
								__m128 cC = _mm_load_ps((float*)(sourceC + 4 * x));
								__m128 cD = _mm_load_ps((float*)(sourceD + 4 * x));
								__m128 cE = _mm_load_ps((float*)(sourceE + 4 * x));
								__m128 cF = _mm_load_ps((float*)(sourceF + 4 * x));

								c0 = _mm_add_ps(c0, c1);
								c2 = _mm_add_ps(c2, c3);
								c4 = _mm_add_ps(c4, c5);
								c6 = _mm_add_ps(c6, c7);
								c8 = _mm_add_ps(c8, c9);
								cA = _mm_add_ps(cA, cB);
								cC = _mm_add_ps(cC, cD);
								cE = _mm_add_ps(cE, cF);
								c0 = _mm_add_ps(c0, c2);
								c4 = _mm_add_ps(c4, c6);
								c8 = _mm_add_ps(c8, cA);
								cC = _mm_add_ps(cC, cE);
								c0 = _mm_add_ps(c0, c4);
								c8 = _mm_add_ps(c8, cC);
								c0 = _mm_add_ps(c0, c8);
								c0 = _mm_mul_ps(c0, _mm_set1_ps(1.0f / 16.0f));

								_mm_store_ps((float*)(source0 + 4 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
							source8 += pitch;
							source9 += pitch;
							sourceA += pitch;
							sourceB += pitch;
							sourceC += pitch;
							sourceD += pitch;
							sourceE += pitch;
							sourceF += pitch;
						}
					}
					else ASSERT(false);
				}
				else
			#endif
			{
				if(internal.samples == 2)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							float c0 = *(float*)(source0 + 4 * x);
							float c1 = *(float*)(source1 + 4 * x);

							c0 = c0 + c1;
							c0 *= 1.0f / 2.0f;

							*(float*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
					}
				}
				else if(internal.samples == 4)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							float c0 = *(float*)(source0 + 4 * x);
							float c1 = *(float*)(source1 + 4 * x);
							float c2 = *(float*)(source2 + 4 * x);
							float c3 = *(float*)(source3 + 4 * x);

							c0 = c0 + c1;
							c2 = c2 + c3;
							c0 = c0 + c2;
							c0 *= 1.0f / 4.0f;

							*(float*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
					}
				}
				else if(internal.samples == 8)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							float c0 = *(float*)(source0 + 4 * x);
							float c1 = *(float*)(source1 + 4 * x);
							float c2 = *(float*)(source2 + 4 * x);
							float c3 = *(float*)(source3 + 4 * x);
							float c4 = *(float*)(source4 + 4 * x);
							float c5 = *(float*)(source5 + 4 * x);
							float c6 = *(float*)(source6 + 4 * x);
							float c7 = *(float*)(source7 + 4 * x);

							c0 = c0 + c1;
							c2 = c2 + c3;
							c4 = c4 + c5;
							c6 = c6 + c7;
							c0 = c0 + c2;
							c4 = c4 + c6;
							c0 = c0 + c4;
							c0 *= 1.0f / 8.0f;

							*(float*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
					}
				}
				else if(internal.samples == 16)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							float c0 = *(float*)(source0 + 4 * x);
							float c1 = *(float*)(source1 + 4 * x);
							float c2 = *(float*)(source2 + 4 * x);
							float c3 = *(float*)(source3 + 4 * x);
							float c4 = *(float*)(source4 + 4 * x);
							float c5 = *(float*)(source5 + 4 * x);
							float c6 = *(float*)(source6 + 4 * x);
							float c7 = *(float*)(source7 + 4 * x);
							float c8 = *(float*)(source8 + 4 * x);
							float c9 = *(float*)(source9 + 4 * x);
							float cA = *(float*)(sourceA + 4 * x);
							float cB = *(float*)(sourceB + 4 * x);
							float cC = *(float*)(sourceC + 4 * x);
							float cD = *(float*)(sourceD + 4 * x);
							float cE = *(float*)(sourceE + 4 * x);
							float cF = *(float*)(sourceF + 4 * x);

							c0 = c0 + c1;
							c2 = c2 + c3;
							c4 = c4 + c5;
							c6 = c6 + c7;
							c8 = c8 + c9;
							cA = cA + cB;
							cC = cC + cD;
							cE = cE + cF;
							c0 = c0 + c2;
							c4 = c4 + c6;
							c8 = c8 + cA;
							cC = cC + cE;
							c0 = c0 + c4;
							c8 = c8 + cC;
							c0 = c0 + c8;
							c0 *= 1.0f / 16.0f;

							*(float*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
						source8 += pitch;
						source9 += pitch;
						sourceA += pitch;
						sourceB += pitch;
						sourceC += pitch;
						sourceD += pitch;
						sourceE += pitch;
						sourceF += pitch;
					}
				}
				else ASSERT(false);
			}
		}
		else if(internal.format == FORMAT_G32R32F)
		{
			#if defined(__i386__) || defined(__x86_64__)
				if(CPUID::supportsSSE() && (width % 2) == 0)
				{
					if(internal.samples == 2)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 2)
							{
								__m128 c0 = _mm_load_ps((float*)(source0 + 8 * x));
								__m128 c1 = _mm_load_ps((float*)(source1 + 8 * x));

								c0 = _mm_add_ps(c0, c1);
								c0 = _mm_mul_ps(c0, _mm_set1_ps(1.0f / 2.0f));

								_mm_store_ps((float*)(source0 + 8 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
						}
					}
					else if(internal.samples == 4)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 2)
							{
								__m128 c0 = _mm_load_ps((float*)(source0 + 8 * x));
								__m128 c1 = _mm_load_ps((float*)(source1 + 8 * x));
								__m128 c2 = _mm_load_ps((float*)(source2 + 8 * x));
								__m128 c3 = _mm_load_ps((float*)(source3 + 8 * x));

								c0 = _mm_add_ps(c0, c1);
								c2 = _mm_add_ps(c2, c3);
								c0 = _mm_add_ps(c0, c2);
								c0 = _mm_mul_ps(c0, _mm_set1_ps(1.0f / 4.0f));

								_mm_store_ps((float*)(source0 + 8 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
						}
					}
					else if(internal.samples == 8)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 2)
							{
								__m128 c0 = _mm_load_ps((float*)(source0 + 8 * x));
								__m128 c1 = _mm_load_ps((float*)(source1 + 8 * x));
								__m128 c2 = _mm_load_ps((float*)(source2 + 8 * x));
								__m128 c3 = _mm_load_ps((float*)(source3 + 8 * x));
								__m128 c4 = _mm_load_ps((float*)(source4 + 8 * x));
								__m128 c5 = _mm_load_ps((float*)(source5 + 8 * x));
								__m128 c6 = _mm_load_ps((float*)(source6 + 8 * x));
								__m128 c7 = _mm_load_ps((float*)(source7 + 8 * x));

								c0 = _mm_add_ps(c0, c1);
								c2 = _mm_add_ps(c2, c3);
								c4 = _mm_add_ps(c4, c5);
								c6 = _mm_add_ps(c6, c7);
								c0 = _mm_add_ps(c0, c2);
								c4 = _mm_add_ps(c4, c6);
								c0 = _mm_add_ps(c0, c4);
								c0 = _mm_mul_ps(c0, _mm_set1_ps(1.0f / 8.0f));

								_mm_store_ps((float*)(source0 + 8 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
						}
					}
					else if(internal.samples == 16)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 2)
							{
								__m128 c0 = _mm_load_ps((float*)(source0 + 8 * x));
								__m128 c1 = _mm_load_ps((float*)(source1 + 8 * x));
								__m128 c2 = _mm_load_ps((float*)(source2 + 8 * x));
								__m128 c3 = _mm_load_ps((float*)(source3 + 8 * x));
								__m128 c4 = _mm_load_ps((float*)(source4 + 8 * x));
								__m128 c5 = _mm_load_ps((float*)(source5 + 8 * x));
								__m128 c6 = _mm_load_ps((float*)(source6 + 8 * x));
								__m128 c7 = _mm_load_ps((float*)(source7 + 8 * x));
								__m128 c8 = _mm_load_ps((float*)(source8 + 8 * x));
								__m128 c9 = _mm_load_ps((float*)(source9 + 8 * x));
								__m128 cA = _mm_load_ps((float*)(sourceA + 8 * x));
								__m128 cB = _mm_load_ps((float*)(sourceB + 8 * x));
								__m128 cC = _mm_load_ps((float*)(sourceC + 8 * x));
								__m128 cD = _mm_load_ps((float*)(sourceD + 8 * x));
								__m128 cE = _mm_load_ps((float*)(sourceE + 8 * x));
								__m128 cF = _mm_load_ps((float*)(sourceF + 8 * x));

								c0 = _mm_add_ps(c0, c1);
								c2 = _mm_add_ps(c2, c3);
								c4 = _mm_add_ps(c4, c5);
								c6 = _mm_add_ps(c6, c7);
								c8 = _mm_add_ps(c8, c9);
								cA = _mm_add_ps(cA, cB);
								cC = _mm_add_ps(cC, cD);
								cE = _mm_add_ps(cE, cF);
								c0 = _mm_add_ps(c0, c2);
								c4 = _mm_add_ps(c4, c6);
								c8 = _mm_add_ps(c8, cA);
								cC = _mm_add_ps(cC, cE);
								c0 = _mm_add_ps(c0, c4);
								c8 = _mm_add_ps(c8, cC);
								c0 = _mm_add_ps(c0, c8);
								c0 = _mm_mul_ps(c0, _mm_set1_ps(1.0f / 16.0f));

								_mm_store_ps((float*)(source0 + 8 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
							source8 += pitch;
							source9 += pitch;
							sourceA += pitch;
							sourceB += pitch;
							sourceC += pitch;
							sourceD += pitch;
							sourceE += pitch;
							sourceF += pitch;
						}
					}
					else ASSERT(false);
				}
				else
			#endif
			{
				if(internal.samples == 2)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < 2 * width; x++)
						{
							float c0 = *(float*)(source0 + 4 * x);
							float c1 = *(float*)(source1 + 4 * x);

							c0 = c0 + c1;
							c0 *= 1.0f / 2.0f;

							*(float*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
					}
				}
				else if(internal.samples == 4)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < 2 * width; x++)
						{
							float c0 = *(float*)(source0 + 4 * x);
							float c1 = *(float*)(source1 + 4 * x);
							float c2 = *(float*)(source2 + 4 * x);
							float c3 = *(float*)(source3 + 4 * x);

							c0 = c0 + c1;
							c2 = c2 + c3;
							c0 = c0 + c2;
							c0 *= 1.0f / 4.0f;

							*(float*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
					}
				}
				else if(internal.samples == 8)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < 2 * width; x++)
						{
							float c0 = *(float*)(source0 + 4 * x);
							float c1 = *(float*)(source1 + 4 * x);
							float c2 = *(float*)(source2 + 4 * x);
							float c3 = *(float*)(source3 + 4 * x);
							float c4 = *(float*)(source4 + 4 * x);
							float c5 = *(float*)(source5 + 4 * x);
							float c6 = *(float*)(source6 + 4 * x);
							float c7 = *(float*)(source7 + 4 * x);

							c0 = c0 + c1;
							c2 = c2 + c3;
							c4 = c4 + c5;
							c6 = c6 + c7;
							c0 = c0 + c2;
							c4 = c4 + c6;
							c0 = c0 + c4;
							c0 *= 1.0f / 8.0f;

							*(float*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
					}
				}
				else if(internal.samples == 16)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < 2 * width; x++)
						{
							float c0 = *(float*)(source0 + 4 * x);
							float c1 = *(float*)(source1 + 4 * x);
							float c2 = *(float*)(source2 + 4 * x);
							float c3 = *(float*)(source3 + 4 * x);
							float c4 = *(float*)(source4 + 4 * x);
							float c5 = *(float*)(source5 + 4 * x);
							float c6 = *(float*)(source6 + 4 * x);
							float c7 = *(float*)(source7 + 4 * x);
							float c8 = *(float*)(source8 + 4 * x);
							float c9 = *(float*)(source9 + 4 * x);
							float cA = *(float*)(sourceA + 4 * x);
							float cB = *(float*)(sourceB + 4 * x);
							float cC = *(float*)(sourceC + 4 * x);
							float cD = *(float*)(sourceD + 4 * x);
							float cE = *(float*)(sourceE + 4 * x);
							float cF = *(float*)(sourceF + 4 * x);

							c0 = c0 + c1;
							c2 = c2 + c3;
							c4 = c4 + c5;
							c6 = c6 + c7;
							c8 = c8 + c9;
							cA = cA + cB;
							cC = cC + cD;
							cE = cE + cF;
							c0 = c0 + c2;
							c4 = c4 + c6;
							c8 = c8 + cA;
							cC = cC + cE;
							c0 = c0 + c4;
							c8 = c8 + cC;
							c0 = c0 + c8;
							c0 *= 1.0f / 16.0f;

							*(float*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
						source8 += pitch;
						source9 += pitch;
						sourceA += pitch;
						sourceB += pitch;
						sourceC += pitch;
						sourceD += pitch;
						sourceE += pitch;
						sourceF += pitch;
					}
				}
				else ASSERT(false);
			}
		}
		else if(internal.format == FORMAT_A32B32G32R32F ||
		        internal.format == FORMAT_X32B32G32R32F ||
		        internal.format == FORMAT_X32B32G32R32F_UNSIGNED)
		{
			#if defined(__i386__) || defined(__x86_64__)
				if(CPUID::supportsSSE())
				{
					if(internal.samples == 2)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								__m128 c0 = _mm_load_ps((float*)(source0 + 16 * x));
								__m128 c1 = _mm_load_ps((float*)(source1 + 16 * x));

								c0 = _mm_add_ps(c0, c1);
								c0 = _mm_mul_ps(c0, _mm_set1_ps(1.0f / 2.0f));

								_mm_store_ps((float*)(source0 + 16 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
						}
					}
					else if(internal.samples == 4)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								__m128 c0 = _mm_load_ps((float*)(source0 + 16 * x));
								__m128 c1 = _mm_load_ps((float*)(source1 + 16 * x));
								__m128 c2 = _mm_load_ps((float*)(source2 + 16 * x));
								__m128 c3 = _mm_load_ps((float*)(source3 + 16 * x));

								c0 = _mm_add_ps(c0, c1);
								c2 = _mm_add_ps(c2, c3);
								c0 = _mm_add_ps(c0, c2);
								c0 = _mm_mul_ps(c0, _mm_set1_ps(1.0f / 4.0f));

								_mm_store_ps((float*)(source0 + 16 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
						}
					}
					else if(internal.samples == 8)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								__m128 c0 = _mm_load_ps((float*)(source0 + 16 * x));
								__m128 c1 = _mm_load_ps((float*)(source1 + 16 * x));
								__m128 c2 = _mm_load_ps((float*)(source2 + 16 * x));
								__m128 c3 = _mm_load_ps((float*)(source3 + 16 * x));
								__m128 c4 = _mm_load_ps((float*)(source4 + 16 * x));
								__m128 c5 = _mm_load_ps((float*)(source5 + 16 * x));
								__m128 c6 = _mm_load_ps((float*)(source6 + 16 * x));
								__m128 c7 = _mm_load_ps((float*)(source7 + 16 * x));

								c0 = _mm_add_ps(c0, c1);
								c2 = _mm_add_ps(c2, c3);
								c4 = _mm_add_ps(c4, c5);
								c6 = _mm_add_ps(c6, c7);
								c0 = _mm_add_ps(c0, c2);
								c4 = _mm_add_ps(c4, c6);
								c0 = _mm_add_ps(c0, c4);
								c0 = _mm_mul_ps(c0, _mm_set1_ps(1.0f / 8.0f));

								_mm_store_ps((float*)(source0 + 16 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
						}
					}
					else if(internal.samples == 16)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								__m128 c0 = _mm_load_ps((float*)(source0 + 16 * x));
								__m128 c1 = _mm_load_ps((float*)(source1 + 16 * x));
								__m128 c2 = _mm_load_ps((float*)(source2 + 16 * x));
								__m128 c3 = _mm_load_ps((float*)(source3 + 16 * x));
								__m128 c4 = _mm_load_ps((float*)(source4 + 16 * x));
								__m128 c5 = _mm_load_ps((float*)(source5 + 16 * x));
								__m128 c6 = _mm_load_ps((float*)(source6 + 16 * x));
								__m128 c7 = _mm_load_ps((float*)(source7 + 16 * x));
								__m128 c8 = _mm_load_ps((float*)(source8 + 16 * x));
								__m128 c9 = _mm_load_ps((float*)(source9 + 16 * x));
								__m128 cA = _mm_load_ps((float*)(sourceA + 16 * x));
								__m128 cB = _mm_load_ps((float*)(sourceB + 16 * x));
								__m128 cC = _mm_load_ps((float*)(sourceC + 16 * x));
								__m128 cD = _mm_load_ps((float*)(sourceD + 16 * x));
								__m128 cE = _mm_load_ps((float*)(sourceE + 16 * x));
								__m128 cF = _mm_load_ps((float*)(sourceF + 16 * x));

								c0 = _mm_add_ps(c0, c1);
								c2 = _mm_add_ps(c2, c3);
								c4 = _mm_add_ps(c4, c5);
								c6 = _mm_add_ps(c6, c7);
								c8 = _mm_add_ps(c8, c9);
								cA = _mm_add_ps(cA, cB);
								cC = _mm_add_ps(cC, cD);
								cE = _mm_add_ps(cE, cF);
								c0 = _mm_add_ps(c0, c2);
								c4 = _mm_add_ps(c4, c6);
								c8 = _mm_add_ps(c8, cA);
								cC = _mm_add_ps(cC, cE);
								c0 = _mm_add_ps(c0, c4);
								c8 = _mm_add_ps(c8, cC);
								c0 = _mm_add_ps(c0, c8);
								c0 = _mm_mul_ps(c0, _mm_set1_ps(1.0f / 16.0f));

								_mm_store_ps((float*)(source0 + 16 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
							source8 += pitch;
							source9 += pitch;
							sourceA += pitch;
							sourceB += pitch;
							sourceC += pitch;
							sourceD += pitch;
							sourceE += pitch;
							sourceF += pitch;
						}
					}
					else ASSERT(false);
				}
				else
			#endif
			{
				if(internal.samples == 2)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < 4 * width; x++)
						{
							float c0 = *(float*)(source0 + 4 * x);
							float c1 = *(float*)(source1 + 4 * x);

							c0 = c0 + c1;
							c0 *= 1.0f / 2.0f;

							*(float*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
					}
				}
				else if(internal.samples == 4)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < 4 * width; x++)
						{
							float c0 = *(float*)(source0 + 4 * x);
							float c1 = *(float*)(source1 + 4 * x);
							float c2 = *(float*)(source2 + 4 * x);
							float c3 = *(float*)(source3 + 4 * x);

							c0 = c0 + c1;
							c2 = c2 + c3;
							c0 = c0 + c2;
							c0 *= 1.0f / 4.0f;

							*(float*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
					}
				}
				else if(internal.samples == 8)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < 4 * width; x++)
						{
							float c0 = *(float*)(source0 + 4 * x);
							float c1 = *(float*)(source1 + 4 * x);
							float c2 = *(float*)(source2 + 4 * x);
							float c3 = *(float*)(source3 + 4 * x);
							float c4 = *(float*)(source4 + 4 * x);
							float c5 = *(float*)(source5 + 4 * x);
							float c6 = *(float*)(source6 + 4 * x);
							float c7 = *(float*)(source7 + 4 * x);

							c0 = c0 + c1;
							c2 = c2 + c3;
							c4 = c4 + c5;
							c6 = c6 + c7;
							c0 = c0 + c2;
							c4 = c4 + c6;
							c0 = c0 + c4;
							c0 *= 1.0f / 8.0f;

							*(float*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
					}
				}
				else if(internal.samples == 16)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < 4 * width; x++)
						{
							float c0 = *(float*)(source0 + 4 * x);
							float c1 = *(float*)(source1 + 4 * x);
							float c2 = *(float*)(source2 + 4 * x);
							float c3 = *(float*)(source3 + 4 * x);
							float c4 = *(float*)(source4 + 4 * x);
							float c5 = *(float*)(source5 + 4 * x);
							float c6 = *(float*)(source6 + 4 * x);
							float c7 = *(float*)(source7 + 4 * x);
							float c8 = *(float*)(source8 + 4 * x);
							float c9 = *(float*)(source9 + 4 * x);
							float cA = *(float*)(sourceA + 4 * x);
							float cB = *(float*)(sourceB + 4 * x);
							float cC = *(float*)(sourceC + 4 * x);
							float cD = *(float*)(sourceD + 4 * x);
							float cE = *(float*)(sourceE + 4 * x);
							float cF = *(float*)(sourceF + 4 * x);

							c0 = c0 + c1;
							c2 = c2 + c3;
							c4 = c4 + c5;
							c6 = c6 + c7;
							c8 = c8 + c9;
							cA = cA + cB;
							cC = cC + cD;
							cE = cE + cF;
							c0 = c0 + c2;
							c4 = c4 + c6;
							c8 = c8 + cA;
							cC = cC + cE;
							c0 = c0 + c4;
							c8 = c8 + cC;
							c0 = c0 + c8;
							c0 *= 1.0f / 16.0f;

							*(float*)(source0 + 4 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
						source8 += pitch;
						source9 += pitch;
						sourceA += pitch;
						sourceB += pitch;
						sourceC += pitch;
						sourceD += pitch;
						sourceE += pitch;
						sourceF += pitch;
					}
				}
				else ASSERT(false);
			}
		}
		else if(internal.format == FORMAT_R5G6B5)
		{
			#if defined(__i386__) || defined(__x86_64__)
				if(CPUID::supportsSSE2() && (width % 8) == 0)
				{
					if(internal.samples == 2)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 8)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 2 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 2 * x));

								static const ushort8 r_b = {0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F};
								static const ushort8 _g_ = {0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0};
								__m128i c0_r_b = _mm_and_si128(c0, reinterpret_cast<const __m128i&>(r_b));
								__m128i c0__g_ = _mm_and_si128(c0, reinterpret_cast<const __m128i&>(_g_));
								__m128i c1_r_b = _mm_and_si128(c1, reinterpret_cast<const __m128i&>(r_b));
								__m128i c1__g_ = _mm_and_si128(c1, reinterpret_cast<const __m128i&>(_g_));

								c0 = _mm_avg_epu8(c0_r_b, c1_r_b);
								c0 = _mm_and_si128(c0, reinterpret_cast<const __m128i&>(r_b));
								c1 = _mm_avg_epu16(c0__g_, c1__g_);
								c1 = _mm_and_si128(c1, reinterpret_cast<const __m128i&>(_g_));
								c0 = _mm_or_si128(c0, c1);

								_mm_store_si128((__m128i*)(source0 + 2 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
						}
					}
					else if(internal.samples == 4)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 8)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 2 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 2 * x));
								__m128i c2 = _mm_load_si128((__m128i*)(source2 + 2 * x));
								__m128i c3 = _mm_load_si128((__m128i*)(source3 + 2 * x));

								static const ushort8 r_b = {0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F};
								static const ushort8 _g_ = {0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0};
								__m128i c0_r_b = _mm_and_si128(c0, reinterpret_cast<const __m128i&>(r_b));
								__m128i c0__g_ = _mm_and_si128(c0, reinterpret_cast<const __m128i&>(_g_));
								__m128i c1_r_b = _mm_and_si128(c1, reinterpret_cast<const __m128i&>(r_b));
								__m128i c1__g_ = _mm_and_si128(c1, reinterpret_cast<const __m128i&>(_g_));
								__m128i c2_r_b = _mm_and_si128(c2, reinterpret_cast<const __m128i&>(r_b));
								__m128i c2__g_ = _mm_and_si128(c2, reinterpret_cast<const __m128i&>(_g_));
								__m128i c3_r_b = _mm_and_si128(c3, reinterpret_cast<const __m128i&>(r_b));
								__m128i c3__g_ = _mm_and_si128(c3, reinterpret_cast<const __m128i&>(_g_));

								c0 = _mm_avg_epu8(c0_r_b, c1_r_b);
								c2 = _mm_avg_epu8(c2_r_b, c3_r_b);
								c0 = _mm_avg_epu8(c0, c2);
								c0 = _mm_and_si128(c0, reinterpret_cast<const __m128i&>(r_b));
								c1 = _mm_avg_epu16(c0__g_, c1__g_);
								c3 = _mm_avg_epu16(c2__g_, c3__g_);
								c1 = _mm_avg_epu16(c1, c3);
								c1 = _mm_and_si128(c1, reinterpret_cast<const __m128i&>(_g_));
								c0 = _mm_or_si128(c0, c1);

								_mm_store_si128((__m128i*)(source0 + 2 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
						}
					}
					else if(internal.samples == 8)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 8)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 2 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 2 * x));
								__m128i c2 = _mm_load_si128((__m128i*)(source2 + 2 * x));
								__m128i c3 = _mm_load_si128((__m128i*)(source3 + 2 * x));
								__m128i c4 = _mm_load_si128((__m128i*)(source4 + 2 * x));
								__m128i c5 = _mm_load_si128((__m128i*)(source5 + 2 * x));
								__m128i c6 = _mm_load_si128((__m128i*)(source6 + 2 * x));
								__m128i c7 = _mm_load_si128((__m128i*)(source7 + 2 * x));

								static const ushort8 r_b = {0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F};
								static const ushort8 _g_ = {0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0};
								__m128i c0_r_b = _mm_and_si128(c0, reinterpret_cast<const __m128i&>(r_b));
								__m128i c0__g_ = _mm_and_si128(c0, reinterpret_cast<const __m128i&>(_g_));
								__m128i c1_r_b = _mm_and_si128(c1, reinterpret_cast<const __m128i&>(r_b));
								__m128i c1__g_ = _mm_and_si128(c1, reinterpret_cast<const __m128i&>(_g_));
								__m128i c2_r_b = _mm_and_si128(c2, reinterpret_cast<const __m128i&>(r_b));
								__m128i c2__g_ = _mm_and_si128(c2, reinterpret_cast<const __m128i&>(_g_));
								__m128i c3_r_b = _mm_and_si128(c3, reinterpret_cast<const __m128i&>(r_b));
								__m128i c3__g_ = _mm_and_si128(c3, reinterpret_cast<const __m128i&>(_g_));
								__m128i c4_r_b = _mm_and_si128(c4, reinterpret_cast<const __m128i&>(r_b));
								__m128i c4__g_ = _mm_and_si128(c4, reinterpret_cast<const __m128i&>(_g_));
								__m128i c5_r_b = _mm_and_si128(c5, reinterpret_cast<const __m128i&>(r_b));
								__m128i c5__g_ = _mm_and_si128(c5, reinterpret_cast<const __m128i&>(_g_));
								__m128i c6_r_b = _mm_and_si128(c6, reinterpret_cast<const __m128i&>(r_b));
								__m128i c6__g_ = _mm_and_si128(c6, reinterpret_cast<const __m128i&>(_g_));
								__m128i c7_r_b = _mm_and_si128(c7, reinterpret_cast<const __m128i&>(r_b));
								__m128i c7__g_ = _mm_and_si128(c7, reinterpret_cast<const __m128i&>(_g_));

								c0 = _mm_avg_epu8(c0_r_b, c1_r_b);
								c2 = _mm_avg_epu8(c2_r_b, c3_r_b);
								c4 = _mm_avg_epu8(c4_r_b, c5_r_b);
								c6 = _mm_avg_epu8(c6_r_b, c7_r_b);
								c0 = _mm_avg_epu8(c0, c2);
								c4 = _mm_avg_epu8(c4, c6);
								c0 = _mm_avg_epu8(c0, c4);
								c0 = _mm_and_si128(c0, reinterpret_cast<const __m128i&>(r_b));
								c1 = _mm_avg_epu16(c0__g_, c1__g_);
								c3 = _mm_avg_epu16(c2__g_, c3__g_);
								c5 = _mm_avg_epu16(c4__g_, c5__g_);
								c7 = _mm_avg_epu16(c6__g_, c7__g_);
								c1 = _mm_avg_epu16(c1, c3);
								c5 = _mm_avg_epu16(c5, c7);
								c1 = _mm_avg_epu16(c1, c5);
								c1 = _mm_and_si128(c1, reinterpret_cast<const __m128i&>(_g_));
								c0 = _mm_or_si128(c0, c1);

								_mm_store_si128((__m128i*)(source0 + 2 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
						}
					}
					else if(internal.samples == 16)
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x += 8)
							{
								__m128i c0 = _mm_load_si128((__m128i*)(source0 + 2 * x));
								__m128i c1 = _mm_load_si128((__m128i*)(source1 + 2 * x));
								__m128i c2 = _mm_load_si128((__m128i*)(source2 + 2 * x));
								__m128i c3 = _mm_load_si128((__m128i*)(source3 + 2 * x));
								__m128i c4 = _mm_load_si128((__m128i*)(source4 + 2 * x));
								__m128i c5 = _mm_load_si128((__m128i*)(source5 + 2 * x));
								__m128i c6 = _mm_load_si128((__m128i*)(source6 + 2 * x));
								__m128i c7 = _mm_load_si128((__m128i*)(source7 + 2 * x));
								__m128i c8 = _mm_load_si128((__m128i*)(source8 + 2 * x));
								__m128i c9 = _mm_load_si128((__m128i*)(source9 + 2 * x));
								__m128i cA = _mm_load_si128((__m128i*)(sourceA + 2 * x));
								__m128i cB = _mm_load_si128((__m128i*)(sourceB + 2 * x));
								__m128i cC = _mm_load_si128((__m128i*)(sourceC + 2 * x));
								__m128i cD = _mm_load_si128((__m128i*)(sourceD + 2 * x));
								__m128i cE = _mm_load_si128((__m128i*)(sourceE + 2 * x));
								__m128i cF = _mm_load_si128((__m128i*)(sourceF + 2 * x));

								static const ushort8 r_b = {0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F};
								static const ushort8 _g_ = {0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0};
								__m128i c0_r_b = _mm_and_si128(c0, reinterpret_cast<const __m128i&>(r_b));
								__m128i c0__g_ = _mm_and_si128(c0, reinterpret_cast<const __m128i&>(_g_));
								__m128i c1_r_b = _mm_and_si128(c1, reinterpret_cast<const __m128i&>(r_b));
								__m128i c1__g_ = _mm_and_si128(c1, reinterpret_cast<const __m128i&>(_g_));
								__m128i c2_r_b = _mm_and_si128(c2, reinterpret_cast<const __m128i&>(r_b));
								__m128i c2__g_ = _mm_and_si128(c2, reinterpret_cast<const __m128i&>(_g_));
								__m128i c3_r_b = _mm_and_si128(c3, reinterpret_cast<const __m128i&>(r_b));
								__m128i c3__g_ = _mm_and_si128(c3, reinterpret_cast<const __m128i&>(_g_));
								__m128i c4_r_b = _mm_and_si128(c4, reinterpret_cast<const __m128i&>(r_b));
								__m128i c4__g_ = _mm_and_si128(c4, reinterpret_cast<const __m128i&>(_g_));
								__m128i c5_r_b = _mm_and_si128(c5, reinterpret_cast<const __m128i&>(r_b));
								__m128i c5__g_ = _mm_and_si128(c5, reinterpret_cast<const __m128i&>(_g_));
								__m128i c6_r_b = _mm_and_si128(c6, reinterpret_cast<const __m128i&>(r_b));
								__m128i c6__g_ = _mm_and_si128(c6, reinterpret_cast<const __m128i&>(_g_));
								__m128i c7_r_b = _mm_and_si128(c7, reinterpret_cast<const __m128i&>(r_b));
								__m128i c7__g_ = _mm_and_si128(c7, reinterpret_cast<const __m128i&>(_g_));
								__m128i c8_r_b = _mm_and_si128(c8, reinterpret_cast<const __m128i&>(r_b));
								__m128i c8__g_ = _mm_and_si128(c8, reinterpret_cast<const __m128i&>(_g_));
								__m128i c9_r_b = _mm_and_si128(c9, reinterpret_cast<const __m128i&>(r_b));
								__m128i c9__g_ = _mm_and_si128(c9, reinterpret_cast<const __m128i&>(_g_));
								__m128i cA_r_b = _mm_and_si128(cA, reinterpret_cast<const __m128i&>(r_b));
								__m128i cA__g_ = _mm_and_si128(cA, reinterpret_cast<const __m128i&>(_g_));
								__m128i cB_r_b = _mm_and_si128(cB, reinterpret_cast<const __m128i&>(r_b));
								__m128i cB__g_ = _mm_and_si128(cB, reinterpret_cast<const __m128i&>(_g_));
								__m128i cC_r_b = _mm_and_si128(cC, reinterpret_cast<const __m128i&>(r_b));
								__m128i cC__g_ = _mm_and_si128(cC, reinterpret_cast<const __m128i&>(_g_));
								__m128i cD_r_b = _mm_and_si128(cD, reinterpret_cast<const __m128i&>(r_b));
								__m128i cD__g_ = _mm_and_si128(cD, reinterpret_cast<const __m128i&>(_g_));
								__m128i cE_r_b = _mm_and_si128(cE, reinterpret_cast<const __m128i&>(r_b));
								__m128i cE__g_ = _mm_and_si128(cE, reinterpret_cast<const __m128i&>(_g_));
								__m128i cF_r_b = _mm_and_si128(cF, reinterpret_cast<const __m128i&>(r_b));
								__m128i cF__g_ = _mm_and_si128(cF, reinterpret_cast<const __m128i&>(_g_));

								c0 = _mm_avg_epu8(c0_r_b, c1_r_b);
								c2 = _mm_avg_epu8(c2_r_b, c3_r_b);
								c4 = _mm_avg_epu8(c4_r_b, c5_r_b);
								c6 = _mm_avg_epu8(c6_r_b, c7_r_b);
								c8 = _mm_avg_epu8(c8_r_b, c9_r_b);
								cA = _mm_avg_epu8(cA_r_b, cB_r_b);
								cC = _mm_avg_epu8(cC_r_b, cD_r_b);
								cE = _mm_avg_epu8(cE_r_b, cF_r_b);
								c0 = _mm_avg_epu8(c0, c2);
								c4 = _mm_avg_epu8(c4, c6);
								c8 = _mm_avg_epu8(c8, cA);
								cC = _mm_avg_epu8(cC, cE);
								c0 = _mm_avg_epu8(c0, c4);
								c8 = _mm_avg_epu8(c8, cC);
								c0 = _mm_avg_epu8(c0, c8);
								c0 = _mm_and_si128(c0, reinterpret_cast<const __m128i&>(r_b));
								c1 = _mm_avg_epu16(c0__g_, c1__g_);
								c3 = _mm_avg_epu16(c2__g_, c3__g_);
								c5 = _mm_avg_epu16(c4__g_, c5__g_);
								c7 = _mm_avg_epu16(c6__g_, c7__g_);
								c9 = _mm_avg_epu16(c8__g_, c9__g_);
								cB = _mm_avg_epu16(cA__g_, cB__g_);
								cD = _mm_avg_epu16(cC__g_, cD__g_);
								cF = _mm_avg_epu16(cE__g_, cF__g_);
								c1 = _mm_avg_epu8(c1, c3);
								c5 = _mm_avg_epu8(c5, c7);
								c9 = _mm_avg_epu8(c9, cB);
								cD = _mm_avg_epu8(cD, cF);
								c1 = _mm_avg_epu8(c1, c5);
								c9 = _mm_avg_epu8(c9, cD);
								c1 = _mm_avg_epu8(c1, c9);
								c1 = _mm_and_si128(c1, reinterpret_cast<const __m128i&>(_g_));
								c0 = _mm_or_si128(c0, c1);

								_mm_store_si128((__m128i*)(source0 + 2 * x), c0);
							}

							source0 += pitch;
							source1 += pitch;
							source2 += pitch;
							source3 += pitch;
							source4 += pitch;
							source5 += pitch;
							source6 += pitch;
							source7 += pitch;
							source8 += pitch;
							source9 += pitch;
							sourceA += pitch;
							sourceB += pitch;
							sourceC += pitch;
							sourceD += pitch;
							sourceE += pitch;
							sourceF += pitch;
						}
					}
					else ASSERT(false);
				}
				else
			#endif
			{
				#define AVERAGE(x, y) (((x) & (y)) + ((((x) ^ (y)) >> 1) & 0x7BEF) + (((x) ^ (y)) & 0x0821))

				if(internal.samples == 2)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							unsigned short c0 = *(unsigned short*)(source0 + 2 * x);
							unsigned short c1 = *(unsigned short*)(source1 + 2 * x);

							c0 = AVERAGE(c0, c1);

							*(unsigned short*)(source0 + 2 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
					}
				}
				else if(internal.samples == 4)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							unsigned short c0 = *(unsigned short*)(source0 + 2 * x);
							unsigned short c1 = *(unsigned short*)(source1 + 2 * x);
							unsigned short c2 = *(unsigned short*)(source2 + 2 * x);
							unsigned short c3 = *(unsigned short*)(source3 + 2 * x);

							c0 = AVERAGE(c0, c1);
							c2 = AVERAGE(c2, c3);
							c0 = AVERAGE(c0, c2);

							*(unsigned short*)(source0 + 2 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
					}
				}
				else if(internal.samples == 8)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							unsigned short c0 = *(unsigned short*)(source0 + 2 * x);
							unsigned short c1 = *(unsigned short*)(source1 + 2 * x);
							unsigned short c2 = *(unsigned short*)(source2 + 2 * x);
							unsigned short c3 = *(unsigned short*)(source3 + 2 * x);
							unsigned short c4 = *(unsigned short*)(source4 + 2 * x);
							unsigned short c5 = *(unsigned short*)(source5 + 2 * x);
							unsigned short c6 = *(unsigned short*)(source6 + 2 * x);
							unsigned short c7 = *(unsigned short*)(source7 + 2 * x);

							c0 = AVERAGE(c0, c1);
							c2 = AVERAGE(c2, c3);
							c4 = AVERAGE(c4, c5);
							c6 = AVERAGE(c6, c7);
							c0 = AVERAGE(c0, c2);
							c4 = AVERAGE(c4, c6);
							c0 = AVERAGE(c0, c4);

							*(unsigned short*)(source0 + 2 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
					}
				}
				else if(internal.samples == 16)
				{
					for(int y = 0; y < height; y++)
					{
						for(int x = 0; x < width; x++)
						{
							unsigned short c0 = *(unsigned short*)(source0 + 2 * x);
							unsigned short c1 = *(unsigned short*)(source1 + 2 * x);
							unsigned short c2 = *(unsigned short*)(source2 + 2 * x);
							unsigned short c3 = *(unsigned short*)(source3 + 2 * x);
							unsigned short c4 = *(unsigned short*)(source4 + 2 * x);
							unsigned short c5 = *(unsigned short*)(source5 + 2 * x);
							unsigned short c6 = *(unsigned short*)(source6 + 2 * x);
							unsigned short c7 = *(unsigned short*)(source7 + 2 * x);
							unsigned short c8 = *(unsigned short*)(source8 + 2 * x);
							unsigned short c9 = *(unsigned short*)(source9 + 2 * x);
							unsigned short cA = *(unsigned short*)(sourceA + 2 * x);
							unsigned short cB = *(unsigned short*)(sourceB + 2 * x);
							unsigned short cC = *(unsigned short*)(sourceC + 2 * x);
							unsigned short cD = *(unsigned short*)(sourceD + 2 * x);
							unsigned short cE = *(unsigned short*)(sourceE + 2 * x);
							unsigned short cF = *(unsigned short*)(sourceF + 2 * x);

							c0 = AVERAGE(c0, c1);
							c2 = AVERAGE(c2, c3);
							c4 = AVERAGE(c4, c5);
							c6 = AVERAGE(c6, c7);
							c8 = AVERAGE(c8, c9);
							cA = AVERAGE(cA, cB);
							cC = AVERAGE(cC, cD);
							cE = AVERAGE(cE, cF);
							c0 = AVERAGE(c0, c2);
							c4 = AVERAGE(c4, c6);
							c8 = AVERAGE(c8, cA);
							cC = AVERAGE(cC, cE);
							c0 = AVERAGE(c0, c4);
							c8 = AVERAGE(c8, cC);
							c0 = AVERAGE(c0, c8);

							*(unsigned short*)(source0 + 2 * x) = c0;
						}

						source0 += pitch;
						source1 += pitch;
						source2 += pitch;
						source3 += pitch;
						source4 += pitch;
						source5 += pitch;
						source6 += pitch;
						source7 += pitch;
						source8 += pitch;
						source9 += pitch;
						sourceA += pitch;
						sourceB += pitch;
						sourceC += pitch;
						sourceD += pitch;
						sourceE += pitch;
						sourceF += pitch;
					}
				}
				else ASSERT(false);

				#undef AVERAGE
			}
		}
		else
		{
		//	UNIMPLEMENTED();
		}
	}
}
