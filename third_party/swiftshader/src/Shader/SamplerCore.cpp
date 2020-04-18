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

#include "SamplerCore.hpp"

#include "Constants.hpp"
#include "Common/Debug.hpp"

namespace
{
	void applySwizzle(sw::SwizzleType swizzle, sw::Short4& s, const sw::Vector4s& c)
	{
		switch(swizzle)
		{
		case sw::SWIZZLE_RED:	s = c.x; break;
		case sw::SWIZZLE_GREEN: s = c.y; break;
		case sw::SWIZZLE_BLUE:  s = c.z; break;
		case sw::SWIZZLE_ALPHA: s = c.w; break;
		case sw::SWIZZLE_ZERO:  s = sw::Short4(0x0000); break;
		case sw::SWIZZLE_ONE:   s = sw::Short4(0x1000); break;
		default: ASSERT(false);
		}
	}

	void applySwizzle(sw::SwizzleType swizzle, sw::Float4& f, const sw::Vector4f& c)
	{
		switch(swizzle)
		{
		case sw::SWIZZLE_RED:	f = c.x; break;
		case sw::SWIZZLE_GREEN: f = c.y; break;
		case sw::SWIZZLE_BLUE:  f = c.z; break;
		case sw::SWIZZLE_ALPHA: f = c.w; break;
		case sw::SWIZZLE_ZERO:  f = sw::Float4(0.0f, 0.0f, 0.0f, 0.0f); break;
		case sw::SWIZZLE_ONE:   f = sw::Float4(1.0f, 1.0f, 1.0f, 1.0f); break;
		default: ASSERT(false);
		}
	}
}

namespace sw
{
	extern bool colorsDefaultToZero;

	SamplerCore::SamplerCore(Pointer<Byte> &constants, const Sampler::State &state) : constants(constants), state(state)
	{
	}

	Vector4s SamplerCore::sampleTexture(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Float4 &bias, Vector4f &dsx, Vector4f &dsy)
	{
		return sampleTexture(texture, u, v, w, q, q, dsx, dsy, (dsx), Implicit, true);
	}

	Vector4s SamplerCore::sampleTexture(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Float4 &bias, Vector4f &dsx, Vector4f &dsy, Vector4f &offset, SamplerFunction function, bool fixed12)
	{
		Vector4s c;

		#if PERF_PROFILE
			AddAtomic(Pointer<Long>(&profiler.texOperations), 4);

			if(state.compressedFormat)
			{
				AddAtomic(Pointer<Long>(&profiler.compressedTex), 4);
			}
		#endif

		if(state.textureType == TEXTURE_NULL)
		{
			c.x = Short4(0x0000);
			c.y = Short4(0x0000);
			c.z = Short4(0x0000);

			if(fixed12)   // FIXME: Convert to fixed12 at higher level, when required
			{
				c.w = Short4(0x1000);
			}
			else
			{
				c.w = Short4(0xFFFFu);   // FIXME
			}
		}
		else
		{
			Float4 uuuu = u;
			Float4 vvvv = v;
			Float4 wwww = w;
			Float4 qqqq = q;

			Int face[4];
			Float lod;
			Float anisotropy;
			Float4 uDelta;
			Float4 vDelta;

			if(state.textureType != TEXTURE_3D)
			{
				if(state.textureType != TEXTURE_CUBE)
				{
					computeLod(texture, lod, anisotropy, uDelta, vDelta, uuuu, vvvv, bias.x, dsx, dsy, function);
				}
				else
				{
					Float4 M;
					cubeFace(face, uuuu, vvvv, u, v, w, M);
					computeLodCube(texture, lod, u, v, w, bias.x, dsx, dsy, M, function);
				}
			}
			else
			{
				computeLod3D(texture, lod, uuuu, vvvv, wwww, bias.x, dsx, dsy, function);
			}

			if(!hasFloatTexture())
			{
				c = sampleFilter(texture, uuuu, vvvv, wwww, offset, lod, anisotropy, uDelta, vDelta, face, function);
			}
			else
			{
				Vector4f cf = sampleFloatFilter(texture, uuuu, vvvv, wwww, qqqq, offset, lod, anisotropy, uDelta, vDelta, face, function);

				convertFixed12(c, cf);
			}

			if(fixed12)
			{
				if(!hasFloatTexture())
				{
					if(state.textureFormat == FORMAT_R5G6B5)
					{
						c.x = MulHigh(As<UShort4>(c.x), UShort4(0x10000000 / 0xF800));
						c.y = MulHigh(As<UShort4>(c.y), UShort4(0x10000000 / 0xFC00));
						c.z = MulHigh(As<UShort4>(c.z), UShort4(0x10000000 / 0xF800));
					}
					else
					{
						for(int component = 0; component < textureComponentCount(); component++)
						{
							if(hasUnsignedTextureComponent(component))
							{
								c[component] = As<UShort4>(c[component]) >> 4;
							}
							else
							{
								c[component] = c[component] >> 3;
							}
						}
					}
				}

				if(state.textureFilter != FILTER_GATHER)
				{
					int componentCount = textureComponentCount();
					short defaultColorValue = colorsDefaultToZero ? 0x0000 : 0x1000;

					switch(state.textureFormat)
					{
					case FORMAT_R8_SNORM:
					case FORMAT_G8R8_SNORM:
					case FORMAT_X8B8G8R8_SNORM:
					case FORMAT_A8B8G8R8_SNORM:
					case FORMAT_R8:
					case FORMAT_R5G6B5:
					case FORMAT_G8R8:
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
					case FORMAT_A16B16G16R16:
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
					case FORMAT_X8R8G8B8:
					case FORMAT_X8B8G8R8:
					case FORMAT_A8R8G8B8:
					case FORMAT_A8B8G8R8:
					case FORMAT_SRGB8_X8:
					case FORMAT_SRGB8_A8:
					case FORMAT_V8U8:
					case FORMAT_Q8W8V8U8:
					case FORMAT_X8L8V8U8:
					case FORMAT_V16U16:
					case FORMAT_A16W16V16U16:
					case FORMAT_Q16W16V16U16:
					case FORMAT_YV12_BT601:
					case FORMAT_YV12_BT709:
					case FORMAT_YV12_JFIF:
						if(componentCount < 2) c.y = Short4(defaultColorValue);
						if(componentCount < 3) c.z = Short4(defaultColorValue);
						if(componentCount < 4) c.w = Short4(0x1000);
						break;
					case FORMAT_A8:
						c.w = c.x;
						c.x = Short4(0x0000);
						c.y = Short4(0x0000);
						c.z = Short4(0x0000);
						break;
					case FORMAT_L8:
					case FORMAT_L16:
						c.y = c.x;
						c.z = c.x;
						c.w = Short4(0x1000);
						break;
					case FORMAT_A8L8:
						c.w = c.y;
						c.y = c.x;
						c.z = c.x;
						break;
					case FORMAT_R32F:
						c.y = Short4(defaultColorValue);
					case FORMAT_G32R32F:
						c.z = Short4(defaultColorValue);
					case FORMAT_X32B32G32R32F:
					case FORMAT_X32B32G32R32F_UNSIGNED:
						c.w = Short4(0x1000);
					case FORMAT_A32B32G32R32F:
						break;
					case FORMAT_D32F:
					case FORMAT_D32FS8:
					case FORMAT_D32F_LOCKABLE:
					case FORMAT_D32FS8_TEXTURE:
					case FORMAT_D32F_SHADOW:
					case FORMAT_D32FS8_SHADOW:
						c.y = c.x;
						c.z = c.x;
						c.w = c.x;
						break;
					default:
						ASSERT(false);
					}
				}

				if((state.swizzleR != SWIZZLE_RED) ||
				   (state.swizzleG != SWIZZLE_GREEN) ||
				   (state.swizzleB != SWIZZLE_BLUE) ||
				   (state.swizzleA != SWIZZLE_ALPHA))
				{
					const Vector4s col(c);
					applySwizzle(state.swizzleR, c.x, col);
					applySwizzle(state.swizzleG, c.y, col);
					applySwizzle(state.swizzleB, c.z, col);
					applySwizzle(state.swizzleA, c.w, col);
				}
			}
		}

		return c;
	}

	Vector4f SamplerCore::sampleTexture(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Float4 &bias, Vector4f &dsx, Vector4f &dsy, Vector4f &offset, SamplerFunction function)
	{
		Vector4f c;

		#if PERF_PROFILE
			AddAtomic(Pointer<Long>(&profiler.texOperations), 4);

			if(state.compressedFormat)
			{
				AddAtomic(Pointer<Long>(&profiler.compressedTex), 4);
			}
		#endif

		if(state.textureType == TEXTURE_NULL)
		{
			c.x = Float4(0.0f);
			c.y = Float4(0.0f);
			c.z = Float4(0.0f);
			c.w = Float4(1.0f);
		}
		else
		{
			// FIXME: YUV is not supported by the floating point path
			bool forceFloatFiltering = state.highPrecisionFiltering && !hasYuvFormat() && (state.textureFilter != FILTER_POINT);
			bool seamlessCube = (state.addressingModeU == ADDRESSING_SEAMLESS);
			bool rectangleTexture = (state.textureType == TEXTURE_RECTANGLE);
			if(hasFloatTexture() || hasUnnormalizedIntegerTexture() || forceFloatFiltering || seamlessCube || rectangleTexture)   // FIXME: Mostly identical to integer sampling
			{
				Float4 uuuu = u;
				Float4 vvvv = v;
				Float4 wwww = w;
				Float4 qqqq = q;

				Int face[4];
				Float lod;
				Float anisotropy;
				Float4 uDelta;
				Float4 vDelta;

				if(state.textureType != TEXTURE_3D)
				{
					if(state.textureType != TEXTURE_CUBE)
					{
						computeLod(texture, lod, anisotropy, uDelta, vDelta, uuuu, vvvv, bias.x, dsx, dsy, function);
					}
					else
					{
						Float4 M;
						cubeFace(face, uuuu, vvvv, u, v, w, M);
						computeLodCube(texture, lod, u, v, w, bias.x, dsx, dsy, M, function);
					}
				}
				else
				{
					computeLod3D(texture, lod, uuuu, vvvv, wwww, bias.x, dsx, dsy, function);
				}

				c = sampleFloatFilter(texture, uuuu, vvvv, wwww, qqqq, offset, lod, anisotropy, uDelta, vDelta, face, function);

				if(!hasFloatTexture() && !hasUnnormalizedIntegerTexture())
				{
					if(has16bitTextureFormat())
					{
						switch(state.textureFormat)
						{
						case FORMAT_R5G6B5:
							c.x *= Float4(1.0f / 0xF800);
							c.y *= Float4(1.0f / 0xFC00);
							c.z *= Float4(1.0f / 0xF800);
							break;
						default:
							ASSERT(false);
						}
					}
					else
					{
						for(int component = 0; component < textureComponentCount(); component++)
						{
							c[component] *= Float4(hasUnsignedTextureComponent(component) ? 1.0f / 0xFFFF : 1.0f / 0x7FFF);
						}
					}
				}
			}
			else
			{
				Vector4s cs = sampleTexture(texture, u, v, w, q, bias, dsx, dsy, offset, function, false);

				if(state.textureFormat ==  FORMAT_R5G6B5)
				{
					c.x = Float4(As<UShort4>(cs.x)) * Float4(1.0f / 0xF800);
					c.y = Float4(As<UShort4>(cs.y)) * Float4(1.0f / 0xFC00);
					c.z = Float4(As<UShort4>(cs.z)) * Float4(1.0f / 0xF800);
				}
				else
				{
					for(int component = 0; component < textureComponentCount(); component++)
					{
						if(hasUnsignedTextureComponent(component))
						{
							convertUnsigned16(c[component], cs[component]);
						}
						else
						{
							convertSigned15(c[component], cs[component]);
						}
					}
				}
			}

			int componentCount = textureComponentCount();
			float defaultColorValue = colorsDefaultToZero ? 0.0f : 1.0f;

			if(state.textureFilter != FILTER_GATHER)
			{
				switch(state.textureFormat)
				{
				case FORMAT_R8I:
				case FORMAT_R8UI:
				case FORMAT_R16I:
				case FORMAT_R16UI:
				case FORMAT_R32I:
				case FORMAT_R32UI:
					c.y = As<Float4>(UInt4(0));
				case FORMAT_G8R8I:
				case FORMAT_G8R8UI:
				case FORMAT_G16R16I:
				case FORMAT_G16R16UI:
				case FORMAT_G32R32I:
				case FORMAT_G32R32UI:
					c.z = As<Float4>(UInt4(0));
				case FORMAT_X8B8G8R8I:
				case FORMAT_X8B8G8R8UI:
				case FORMAT_X16B16G16R16I:
				case FORMAT_X16B16G16R16UI:
				case FORMAT_X32B32G32R32I:
				case FORMAT_X32B32G32R32UI:
					c.w = As<Float4>(UInt4(1));
				case FORMAT_A8B8G8R8I:
				case FORMAT_A8B8G8R8UI:
				case FORMAT_A16B16G16R16I:
				case FORMAT_A16B16G16R16UI:
				case FORMAT_A32B32G32R32I:
				case FORMAT_A32B32G32R32UI:
					break;
				case FORMAT_R8_SNORM:
				case FORMAT_G8R8_SNORM:
				case FORMAT_X8B8G8R8_SNORM:
				case FORMAT_A8B8G8R8_SNORM:
				case FORMAT_R8:
				case FORMAT_R5G6B5:
				case FORMAT_G8R8:
				case FORMAT_G16R16:
				case FORMAT_A16B16G16R16:
				case FORMAT_X8R8G8B8:
				case FORMAT_X8B8G8R8:
				case FORMAT_A8R8G8B8:
				case FORMAT_A8B8G8R8:
				case FORMAT_SRGB8_X8:
				case FORMAT_SRGB8_A8:
				case FORMAT_V8U8:
				case FORMAT_Q8W8V8U8:
				case FORMAT_X8L8V8U8:
				case FORMAT_V16U16:
				case FORMAT_A16W16V16U16:
				case FORMAT_Q16W16V16U16:
				case FORMAT_YV12_BT601:
				case FORMAT_YV12_BT709:
				case FORMAT_YV12_JFIF:
					if(componentCount < 2) c.y = Float4(defaultColorValue);
					if(componentCount < 3) c.z = Float4(defaultColorValue);
					if(componentCount < 4) c.w = Float4(1.0f);
					break;
				case FORMAT_A8:
					c.w = c.x;
					c.x = Float4(0.0f);
					c.y = Float4(0.0f);
					c.z = Float4(0.0f);
					break;
				case FORMAT_L8:
				case FORMAT_L16:
					c.y = c.x;
					c.z = c.x;
					c.w = Float4(1.0f);
					break;
				case FORMAT_A8L8:
					c.w = c.y;
					c.y = c.x;
					c.z = c.x;
					break;
				case FORMAT_R32F:
					c.y = Float4(defaultColorValue);
				case FORMAT_G32R32F:
					c.z = Float4(defaultColorValue);
				case FORMAT_X32B32G32R32F:
				case FORMAT_X32B32G32R32F_UNSIGNED:
					c.w = Float4(1.0f);
				case FORMAT_A32B32G32R32F:
					break;
				case FORMAT_D32F:
				case FORMAT_D32FS8:
				case FORMAT_D32F_LOCKABLE:
				case FORMAT_D32FS8_TEXTURE:
				case FORMAT_D32F_SHADOW:
				case FORMAT_D32FS8_SHADOW:
					c.y = Float4(0.0f);
					c.z = Float4(0.0f);
					c.w = Float4(1.0f);
					break;
				default:
					ASSERT(false);
				}
			}

			if((state.swizzleR != SWIZZLE_RED) ||
			   (state.swizzleG != SWIZZLE_GREEN) ||
			   (state.swizzleB != SWIZZLE_BLUE) ||
			   (state.swizzleA != SWIZZLE_ALPHA))
			{
				const Vector4f col(c);
				applySwizzle(state.swizzleR, c.x, col);
				applySwizzle(state.swizzleG, c.y, col);
				applySwizzle(state.swizzleB, c.z, col);
				applySwizzle(state.swizzleA, c.w, col);
			}
		}

		return c;
	}

	Vector4f SamplerCore::textureSize(Pointer<Byte> &texture, Float4 &lod)
	{
		Vector4f size;

		for(int i = 0; i < 4; ++i)
		{
			Int baseLevel = *Pointer<Int>(texture + OFFSET(Texture, baseLevel));
			Pointer<Byte> mipmap = texture + OFFSET(Texture, mipmap) + (As<Int>(Extract(lod, i)) + baseLevel) * sizeof(Mipmap);
			size.x = Insert(size.x, As<Float>(Int(*Pointer<Short>(mipmap + OFFSET(Mipmap, width)))), i);
			size.y = Insert(size.y, As<Float>(Int(*Pointer<Short>(mipmap + OFFSET(Mipmap, height)))), i);
			size.z = Insert(size.z, As<Float>(Int(*Pointer<Short>(mipmap + OFFSET(Mipmap, depth)))), i);
		}

		return size;
	}

	void SamplerCore::border(Short4 &mask, Float4 &coordinates)
	{
		Int4 border = As<Int4>(CmpLT(Abs(coordinates - Float4(0.5f)), Float4(0.5f)));
		mask = As<Short4>(Int2(As<Int4>(PackSigned(border, border))));
	}

	void SamplerCore::border(Int4 &mask, Float4 &coordinates)
	{
		mask = As<Int4>(CmpLT(Abs(coordinates - Float4(0.5f)), Float4(0.5f)));
	}

	Short4 SamplerCore::offsetSample(Short4 &uvw, Pointer<Byte> &mipmap, int halfOffset, bool wrap, int count, Float &lod)
	{
		Short4 offset = *Pointer<Short4>(mipmap + halfOffset);

		if(state.textureFilter == FILTER_MIN_LINEAR_MAG_POINT)
		{
			offset &= Short4(CmpNLE(Float4(lod), Float4(0.0f)));
		}
		else if(state.textureFilter == FILTER_MIN_POINT_MAG_LINEAR)
		{
			offset &= Short4(CmpLE(Float4(lod), Float4(0.0f)));
		}

		if(wrap)
		{
			switch(count)
			{
			case -1: return uvw - offset;
			case  0: return uvw;
			case +1: return uvw + offset;
			case  2: return uvw + offset + offset;
			}
		}
		else   // Clamp or mirror
		{
			switch(count)
			{
			case -1: return SubSat(As<UShort4>(uvw), As<UShort4>(offset));
			case  0: return uvw;
			case +1: return AddSat(As<UShort4>(uvw), As<UShort4>(offset));
			case  2: return AddSat(AddSat(As<UShort4>(uvw), As<UShort4>(offset)), As<UShort4>(offset));
			}
		}

		return uvw;
	}

	Vector4s SamplerCore::sampleFilter(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Vector4f &offset, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Int face[4], SamplerFunction function)
	{
		Vector4s c = sampleAniso(texture, u, v, w, offset, lod, anisotropy, uDelta, vDelta, face, false, function);

		if(function == Fetch)
		{
			return c;
		}

		if(state.mipmapFilter == MIPMAP_LINEAR)
		{
			Vector4s cc = sampleAniso(texture, u, v, w, offset, lod, anisotropy, uDelta, vDelta, face, true, function);

			lod *= Float(1 << 16);

			UShort4 utri = UShort4(Float4(lod));   // FIXME: Optimize
			Short4 stri = utri >> 1;   // FIXME: Optimize

			if(hasUnsignedTextureComponent(0)) cc.x = MulHigh(As<UShort4>(cc.x), utri); else cc.x = MulHigh(cc.x, stri);
			if(hasUnsignedTextureComponent(1)) cc.y = MulHigh(As<UShort4>(cc.y), utri); else cc.y = MulHigh(cc.y, stri);
			if(hasUnsignedTextureComponent(2)) cc.z = MulHigh(As<UShort4>(cc.z), utri); else cc.z = MulHigh(cc.z, stri);
			if(hasUnsignedTextureComponent(3)) cc.w = MulHigh(As<UShort4>(cc.w), utri); else cc.w = MulHigh(cc.w, stri);

			utri = ~utri;
			stri = Short4(0x7FFF) - stri;

			if(hasUnsignedTextureComponent(0)) c.x = MulHigh(As<UShort4>(c.x), utri); else c.x = MulHigh(c.x, stri);
			if(hasUnsignedTextureComponent(1)) c.y = MulHigh(As<UShort4>(c.y), utri); else c.y = MulHigh(c.y, stri);
			if(hasUnsignedTextureComponent(2)) c.z = MulHigh(As<UShort4>(c.z), utri); else c.z = MulHigh(c.z, stri);
			if(hasUnsignedTextureComponent(3)) c.w = MulHigh(As<UShort4>(c.w), utri); else c.w = MulHigh(c.w, stri);

			c.x += cc.x;
			c.y += cc.y;
			c.z += cc.z;
			c.w += cc.w;

			if(!hasUnsignedTextureComponent(0)) c.x += c.x;
			if(!hasUnsignedTextureComponent(1)) c.y += c.y;
			if(!hasUnsignedTextureComponent(2)) c.z += c.z;
			if(!hasUnsignedTextureComponent(3)) c.w += c.w;
		}

		Short4 borderMask;

		if(state.addressingModeU == ADDRESSING_BORDER)
		{
			Short4 u0;

			border(u0, u);

			borderMask = u0;
		}

		if(state.addressingModeV == ADDRESSING_BORDER)
		{
			Short4 v0;

			border(v0, v);

			if(state.addressingModeU == ADDRESSING_BORDER)
			{
				borderMask &= v0;
			}
			else
			{
				borderMask = v0;
			}
		}

		if(state.addressingModeW == ADDRESSING_BORDER && state.textureType == TEXTURE_3D)
		{
			Short4 s0;

			border(s0, w);

			if(state.addressingModeU == ADDRESSING_BORDER ||
			   state.addressingModeV == ADDRESSING_BORDER)
			{
				borderMask &= s0;
			}
			else
			{
				borderMask = s0;
			}
		}

		if(state.addressingModeU == ADDRESSING_BORDER ||
		   state.addressingModeV == ADDRESSING_BORDER ||
		   (state.addressingModeW == ADDRESSING_BORDER && state.textureType == TEXTURE_3D))
		{
			Short4 b;

			c.x = (borderMask & c.x) | (~borderMask & (*Pointer<Short4>(texture + OFFSET(Texture,borderColor4[0])) >> (hasUnsignedTextureComponent(0) ? 0 : 1)));
			c.y = (borderMask & c.y) | (~borderMask & (*Pointer<Short4>(texture + OFFSET(Texture,borderColor4[1])) >> (hasUnsignedTextureComponent(1) ? 0 : 1)));
			c.z = (borderMask & c.z) | (~borderMask & (*Pointer<Short4>(texture + OFFSET(Texture,borderColor4[2])) >> (hasUnsignedTextureComponent(2) ? 0 : 1)));
			c.w = (borderMask & c.w) | (~borderMask & (*Pointer<Short4>(texture + OFFSET(Texture,borderColor4[3])) >> (hasUnsignedTextureComponent(3) ? 0 : 1)));
		}

		return c;
	}

	Vector4s SamplerCore::sampleAniso(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Vector4f &offset, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Int face[4], bool secondLOD, SamplerFunction function)
	{
		Vector4s c;

		if(state.textureFilter != FILTER_ANISOTROPIC || function == Lod || function == Fetch)
		{
			c = sampleQuad(texture, u, v, w, offset, lod, face, secondLOD, function);
		}
		else
		{
			Int a = RoundInt(anisotropy);

			Vector4s cSum;

			cSum.x = Short4(0);
			cSum.y = Short4(0);
			cSum.z = Short4(0);
			cSum.w = Short4(0);

			Float4 A = *Pointer<Float4>(constants + OFFSET(Constants,uvWeight) + 16 * a);
			Float4 B = *Pointer<Float4>(constants + OFFSET(Constants,uvStart) + 16 * a);
			UShort4 cw = *Pointer<UShort4>(constants + OFFSET(Constants,cWeight) + 8 * a);
			Short4 sw = Short4(cw >> 1);

			Float4 du = uDelta;
			Float4 dv = vDelta;

			Float4 u0 = u + B * du;
			Float4 v0 = v + B * dv;

			du *= A;
			dv *= A;

			Int i = 0;

			Do
			{
				c = sampleQuad(texture, u0, v0, w, offset, lod, face, secondLOD, function);

				u0 += du;
				v0 += dv;

				if(hasUnsignedTextureComponent(0)) cSum.x += As<Short4>(MulHigh(As<UShort4>(c.x), cw)); else cSum.x += MulHigh(c.x, sw);
				if(hasUnsignedTextureComponent(1)) cSum.y += As<Short4>(MulHigh(As<UShort4>(c.y), cw)); else cSum.y += MulHigh(c.y, sw);
				if(hasUnsignedTextureComponent(2)) cSum.z += As<Short4>(MulHigh(As<UShort4>(c.z), cw)); else cSum.z += MulHigh(c.z, sw);
				if(hasUnsignedTextureComponent(3)) cSum.w += As<Short4>(MulHigh(As<UShort4>(c.w), cw)); else cSum.w += MulHigh(c.w, sw);

				i++;
			}
			Until(i >= a)

			if(hasUnsignedTextureComponent(0)) c.x = cSum.x; else c.x = AddSat(cSum.x, cSum.x);
			if(hasUnsignedTextureComponent(1)) c.y = cSum.y; else c.y = AddSat(cSum.y, cSum.y);
			if(hasUnsignedTextureComponent(2)) c.z = cSum.z; else c.z = AddSat(cSum.z, cSum.z);
			if(hasUnsignedTextureComponent(3)) c.w = cSum.w; else c.w = AddSat(cSum.w, cSum.w);
		}

		return c;
	}

	Vector4s SamplerCore::sampleQuad(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Vector4f &offset, Float &lod, Int face[4], bool secondLOD, SamplerFunction function)
	{
		if(state.textureType != TEXTURE_3D)
		{
			return sampleQuad2D(texture, u, v, w, offset, lod, face, secondLOD, function);
		}
		else
		{
			return sample3D(texture, u, v, w, offset, lod, secondLOD, function);
		}
	}

	Vector4s SamplerCore::sampleQuad2D(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Vector4f &offset, Float &lod, Int face[4], bool secondLOD, SamplerFunction function)
	{
		Vector4s c;

		int componentCount = textureComponentCount();
		bool gather = state.textureFilter == FILTER_GATHER;

		Pointer<Byte> mipmap;
		Pointer<Byte> buffer[4];

		selectMipmap(texture, buffer, mipmap, lod, face, secondLOD);

		bool texelFetch = (function == Fetch);

		Short4 uuuu = texelFetch ? Short4(As<Int4>(u)) : address(u, state.addressingModeU, mipmap);
		Short4 vvvv = texelFetch ? Short4(As<Int4>(v)) : address(v, state.addressingModeV, mipmap);
		Short4 wwww = texelFetch ? Short4(As<Int4>(w)) : address(w, state.addressingModeW, mipmap);

		if(state.textureFilter == FILTER_POINT || texelFetch)
		{
			c = sampleTexel(uuuu, vvvv, wwww, offset, mipmap, buffer, function);
		}
		else
		{
			Short4 uuuu0 = offsetSample(uuuu, mipmap, OFFSET(Mipmap,uHalf), state.addressingModeU == ADDRESSING_WRAP, gather ? 0 : -1, lod);
			Short4 vvvv0 = offsetSample(vvvv, mipmap, OFFSET(Mipmap,vHalf), state.addressingModeV == ADDRESSING_WRAP, gather ? 0 : -1, lod);
			Short4 uuuu1 = offsetSample(uuuu, mipmap, OFFSET(Mipmap,uHalf), state.addressingModeU == ADDRESSING_WRAP, gather ? 2 : +1, lod);
			Short4 vvvv1 = offsetSample(vvvv, mipmap, OFFSET(Mipmap,vHalf), state.addressingModeV == ADDRESSING_WRAP, gather ? 2 : +1, lod);

			Vector4s c0 = sampleTexel(uuuu0, vvvv0, wwww, offset, mipmap, buffer, function);
			Vector4s c1 = sampleTexel(uuuu1, vvvv0, wwww, offset, mipmap, buffer, function);
			Vector4s c2 = sampleTexel(uuuu0, vvvv1, wwww, offset, mipmap, buffer, function);
			Vector4s c3 = sampleTexel(uuuu1, vvvv1, wwww, offset, mipmap, buffer, function);

			if(!gather)   // Blend
			{
				// Fractions
				UShort4 f0u = As<UShort4>(uuuu0) * *Pointer<UShort4>(mipmap + OFFSET(Mipmap,width));
				UShort4 f0v = As<UShort4>(vvvv0) * *Pointer<UShort4>(mipmap + OFFSET(Mipmap,height));

				UShort4 f1u = ~f0u;
				UShort4 f1v = ~f0v;

				UShort4 f0u0v = MulHigh(f0u, f0v);
				UShort4 f1u0v = MulHigh(f1u, f0v);
				UShort4 f0u1v = MulHigh(f0u, f1v);
				UShort4 f1u1v = MulHigh(f1u, f1v);

				// Signed fractions
				Short4 f1u1vs;
				Short4 f0u1vs;
				Short4 f1u0vs;
				Short4 f0u0vs;

				if(!hasUnsignedTextureComponent(0) || !hasUnsignedTextureComponent(1) || !hasUnsignedTextureComponent(2) || !hasUnsignedTextureComponent(3))
				{
					f1u1vs = f1u1v >> 1;
					f0u1vs = f0u1v >> 1;
					f1u0vs = f1u0v >> 1;
					f0u0vs = f0u0v >> 1;
				}

				// Bilinear interpolation
				if(componentCount >= 1)
				{
					if(has16bitTextureComponents() && hasUnsignedTextureComponent(0))
					{
						c0.x = As<UShort4>(c0.x) - MulHigh(As<UShort4>(c0.x), f0u) + MulHigh(As<UShort4>(c1.x), f0u);
						c2.x = As<UShort4>(c2.x) - MulHigh(As<UShort4>(c2.x), f0u) + MulHigh(As<UShort4>(c3.x), f0u);
						c.x  = As<UShort4>(c0.x) - MulHigh(As<UShort4>(c0.x), f0v) + MulHigh(As<UShort4>(c2.x), f0v);
					}
					else
					{
						if(hasUnsignedTextureComponent(0))
						{
							c0.x = MulHigh(As<UShort4>(c0.x), f1u1v);
							c1.x = MulHigh(As<UShort4>(c1.x), f0u1v);
							c2.x = MulHigh(As<UShort4>(c2.x), f1u0v);
							c3.x = MulHigh(As<UShort4>(c3.x), f0u0v);
						}
						else
						{
							c0.x = MulHigh(c0.x, f1u1vs);
							c1.x = MulHigh(c1.x, f0u1vs);
							c2.x = MulHigh(c2.x, f1u0vs);
							c3.x = MulHigh(c3.x, f0u0vs);
						}

						c.x = (c0.x + c1.x) + (c2.x + c3.x);
						if(!hasUnsignedTextureComponent(0)) c.x = AddSat(c.x, c.x);   // Correct for signed fractions
					}
				}

				if(componentCount >= 2)
				{
					if(has16bitTextureComponents() && hasUnsignedTextureComponent(1))
					{
						c0.y = As<UShort4>(c0.y) - MulHigh(As<UShort4>(c0.y), f0u) + MulHigh(As<UShort4>(c1.y), f0u);
						c2.y = As<UShort4>(c2.y) - MulHigh(As<UShort4>(c2.y), f0u) + MulHigh(As<UShort4>(c3.y), f0u);
						c.y  = As<UShort4>(c0.y) - MulHigh(As<UShort4>(c0.y), f0v) + MulHigh(As<UShort4>(c2.y), f0v);
					}
					else
					{
						if(hasUnsignedTextureComponent(1))
						{
							c0.y = MulHigh(As<UShort4>(c0.y), f1u1v);
							c1.y = MulHigh(As<UShort4>(c1.y), f0u1v);
							c2.y = MulHigh(As<UShort4>(c2.y), f1u0v);
							c3.y = MulHigh(As<UShort4>(c3.y), f0u0v);
						}
						else
						{
							c0.y = MulHigh(c0.y, f1u1vs);
							c1.y = MulHigh(c1.y, f0u1vs);
							c2.y = MulHigh(c2.y, f1u0vs);
							c3.y = MulHigh(c3.y, f0u0vs);
						}

						c.y = (c0.y + c1.y) + (c2.y + c3.y);
						if(!hasUnsignedTextureComponent(1)) c.y = AddSat(c.y, c.y);   // Correct for signed fractions
					}
				}

				if(componentCount >= 3)
				{
					if(has16bitTextureComponents() && hasUnsignedTextureComponent(2))
					{
						c0.z = As<UShort4>(c0.z) - MulHigh(As<UShort4>(c0.z), f0u) + MulHigh(As<UShort4>(c1.z), f0u);
						c2.z = As<UShort4>(c2.z) - MulHigh(As<UShort4>(c2.z), f0u) + MulHigh(As<UShort4>(c3.z), f0u);
						c.z  = As<UShort4>(c0.z) - MulHigh(As<UShort4>(c0.z), f0v) + MulHigh(As<UShort4>(c2.z), f0v);
					}
					else
					{
						if(hasUnsignedTextureComponent(2))
						{
							c0.z = MulHigh(As<UShort4>(c0.z), f1u1v);
							c1.z = MulHigh(As<UShort4>(c1.z), f0u1v);
							c2.z = MulHigh(As<UShort4>(c2.z), f1u0v);
							c3.z = MulHigh(As<UShort4>(c3.z), f0u0v);
						}
						else
						{
							c0.z = MulHigh(c0.z, f1u1vs);
							c1.z = MulHigh(c1.z, f0u1vs);
							c2.z = MulHigh(c2.z, f1u0vs);
							c3.z = MulHigh(c3.z, f0u0vs);
						}

						c.z = (c0.z + c1.z) + (c2.z + c3.z);
						if(!hasUnsignedTextureComponent(2)) c.z = AddSat(c.z, c.z);   // Correct for signed fractions
					}
				}

				if(componentCount >= 4)
				{
					if(has16bitTextureComponents() && hasUnsignedTextureComponent(3))
					{
						c0.w = As<UShort4>(c0.w) - MulHigh(As<UShort4>(c0.w), f0u) + MulHigh(As<UShort4>(c1.w), f0u);
						c2.w = As<UShort4>(c2.w) - MulHigh(As<UShort4>(c2.w), f0u) + MulHigh(As<UShort4>(c3.w), f0u);
						c.w  = As<UShort4>(c0.w) - MulHigh(As<UShort4>(c0.w), f0v) + MulHigh(As<UShort4>(c2.w), f0v);
					}
					else
					{
						if(hasUnsignedTextureComponent(3))
						{
							c0.w = MulHigh(As<UShort4>(c0.w), f1u1v);
							c1.w = MulHigh(As<UShort4>(c1.w), f0u1v);
							c2.w = MulHigh(As<UShort4>(c2.w), f1u0v);
							c3.w = MulHigh(As<UShort4>(c3.w), f0u0v);
						}
						else
						{
							c0.w = MulHigh(c0.w, f1u1vs);
							c1.w = MulHigh(c1.w, f0u1vs);
							c2.w = MulHigh(c2.w, f1u0vs);
							c3.w = MulHigh(c3.w, f0u0vs);
						}

						c.w = (c0.w + c1.w) + (c2.w + c3.w);
						if(!hasUnsignedTextureComponent(3)) c.w = AddSat(c.w, c.w);   // Correct for signed fractions
					}
				}
			}
			else
			{
				c.x = c1.x;
				c.y = c2.x;
				c.z = c3.x;
				c.w = c0.x;
			}
		}

		return c;
	}

	Vector4s SamplerCore::sample3D(Pointer<Byte> &texture, Float4 &u_, Float4 &v_, Float4 &w_, Vector4f &offset, Float &lod, bool secondLOD, SamplerFunction function)
	{
		Vector4s c_;

		int componentCount = textureComponentCount();

		Pointer<Byte> mipmap;
		Pointer<Byte> buffer[4];
		Int face[4];

		selectMipmap(texture, buffer, mipmap, lod, face, secondLOD);

		bool texelFetch = (function == Fetch);

		Short4 uuuu = texelFetch ? Short4(As<Int4>(u_)) : address(u_, state.addressingModeU, mipmap);
		Short4 vvvv = texelFetch ? Short4(As<Int4>(v_)) : address(v_, state.addressingModeV, mipmap);
		Short4 wwww = texelFetch ? Short4(As<Int4>(w_)) : address(w_, state.addressingModeW, mipmap);

		if(state.textureFilter == FILTER_POINT || texelFetch)
		{
			c_ = sampleTexel(uuuu, vvvv, wwww, offset, mipmap, buffer, function);
		}
		else
		{
			Vector4s c[2][2][2];

			Short4 u[2][2][2];
			Short4 v[2][2][2];
			Short4 s[2][2][2];

			for(int i = 0; i < 2; i++)
			{
				for(int j = 0; j < 2; j++)
				{
					for(int k = 0; k < 2; k++)
					{
						u[i][j][k] = offsetSample(uuuu, mipmap, OFFSET(Mipmap,uHalf), state.addressingModeU == ADDRESSING_WRAP, i * 2 - 1, lod);
						v[i][j][k] = offsetSample(vvvv, mipmap, OFFSET(Mipmap,vHalf), state.addressingModeV == ADDRESSING_WRAP, j * 2 - 1, lod);
						s[i][j][k] = offsetSample(wwww, mipmap, OFFSET(Mipmap,wHalf), state.addressingModeW == ADDRESSING_WRAP, k * 2 - 1, lod);
					}
				}
			}

			// Fractions
			UShort4 f0u = As<UShort4>(u[0][0][0]) * *Pointer<UShort4>(mipmap + OFFSET(Mipmap,width));
			UShort4 f0v = As<UShort4>(v[0][0][0]) * *Pointer<UShort4>(mipmap + OFFSET(Mipmap,height));
			UShort4 f0s = As<UShort4>(s[0][0][0]) * *Pointer<UShort4>(mipmap + OFFSET(Mipmap,depth));

			UShort4 f1u = ~f0u;
			UShort4 f1v = ~f0v;
			UShort4 f1s = ~f0s;

			UShort4 f[2][2][2];
			Short4 fs[2][2][2];

			f[1][1][1] = MulHigh(f1u, f1v);
			f[0][1][1] = MulHigh(f0u, f1v);
			f[1][0][1] = MulHigh(f1u, f0v);
			f[0][0][1] = MulHigh(f0u, f0v);
			f[1][1][0] = MulHigh(f1u, f1v);
			f[0][1][0] = MulHigh(f0u, f1v);
			f[1][0][0] = MulHigh(f1u, f0v);
			f[0][0][0] = MulHigh(f0u, f0v);

			f[1][1][1] = MulHigh(f[1][1][1], f1s);
			f[0][1][1] = MulHigh(f[0][1][1], f1s);
			f[1][0][1] = MulHigh(f[1][0][1], f1s);
			f[0][0][1] = MulHigh(f[0][0][1], f1s);
			f[1][1][0] = MulHigh(f[1][1][0], f0s);
			f[0][1][0] = MulHigh(f[0][1][0], f0s);
			f[1][0][0] = MulHigh(f[1][0][0], f0s);
			f[0][0][0] = MulHigh(f[0][0][0], f0s);

			// Signed fractions
			if(!hasUnsignedTextureComponent(0) || !hasUnsignedTextureComponent(1) || !hasUnsignedTextureComponent(2) || !hasUnsignedTextureComponent(3))
			{
				fs[0][0][0] = f[0][0][0] >> 1;
				fs[0][0][1] = f[0][0][1] >> 1;
				fs[0][1][0] = f[0][1][0] >> 1;
				fs[0][1][1] = f[0][1][1] >> 1;
				fs[1][0][0] = f[1][0][0] >> 1;
				fs[1][0][1] = f[1][0][1] >> 1;
				fs[1][1][0] = f[1][1][0] >> 1;
				fs[1][1][1] = f[1][1][1] >> 1;
			}

			for(int i = 0; i < 2; i++)
			{
				for(int j = 0; j < 2; j++)
				{
					for(int k = 0; k < 2; k++)
					{
						c[i][j][k] = sampleTexel(u[i][j][k], v[i][j][k], s[i][j][k], offset, mipmap, buffer, function);

						if(componentCount >= 1) { if(hasUnsignedTextureComponent(0)) c[i][j][k].x = MulHigh(As<UShort4>(c[i][j][k].x), f[1 - i][1 - j][1 - k]); else c[i][j][k].x = MulHigh(c[i][j][k].x, fs[1 - i][1 - j][1 - k]); }
						if(componentCount >= 2) { if(hasUnsignedTextureComponent(1)) c[i][j][k].y = MulHigh(As<UShort4>(c[i][j][k].y), f[1 - i][1 - j][1 - k]); else c[i][j][k].y = MulHigh(c[i][j][k].y, fs[1 - i][1 - j][1 - k]); }
						if(componentCount >= 3) { if(hasUnsignedTextureComponent(2)) c[i][j][k].z = MulHigh(As<UShort4>(c[i][j][k].z), f[1 - i][1 - j][1 - k]); else c[i][j][k].z = MulHigh(c[i][j][k].z, fs[1 - i][1 - j][1 - k]); }
						if(componentCount >= 4) { if(hasUnsignedTextureComponent(3)) c[i][j][k].w = MulHigh(As<UShort4>(c[i][j][k].w), f[1 - i][1 - j][1 - k]); else c[i][j][k].w = MulHigh(c[i][j][k].w, fs[1 - i][1 - j][1 - k]); }

						if(i != 0 || j != 0 || k != 0)
						{
							if(componentCount >= 1) c[0][0][0].x += c[i][j][k].x;
							if(componentCount >= 2) c[0][0][0].y += c[i][j][k].y;
							if(componentCount >= 3) c[0][0][0].z += c[i][j][k].z;
							if(componentCount >= 4) c[0][0][0].w += c[i][j][k].w;
						}
					}
				}
			}

			if(componentCount >= 1) c_.x = c[0][0][0].x;
			if(componentCount >= 2) c_.y = c[0][0][0].y;
			if(componentCount >= 3) c_.z = c[0][0][0].z;
			if(componentCount >= 4) c_.w = c[0][0][0].w;

			// Correct for signed fractions
			if(componentCount >= 1) if(!hasUnsignedTextureComponent(0)) c_.x = AddSat(c_.x, c_.x);
			if(componentCount >= 2) if(!hasUnsignedTextureComponent(1)) c_.y = AddSat(c_.y, c_.y);
			if(componentCount >= 3) if(!hasUnsignedTextureComponent(2)) c_.z = AddSat(c_.z, c_.z);
			if(componentCount >= 4) if(!hasUnsignedTextureComponent(3)) c_.w = AddSat(c_.w, c_.w);
		}

		return c_;
	}

	Vector4f SamplerCore::sampleFloatFilter(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Vector4f &offset, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Int face[4], SamplerFunction function)
	{
		Vector4f c = sampleFloatAniso(texture, u, v, w, q, offset, lod, anisotropy, uDelta, vDelta, face, false, function);

		if(function == Fetch)
		{
			return c;
		}

		if(state.mipmapFilter == MIPMAP_LINEAR)
		{
			Vector4f cc = sampleFloatAniso(texture, u, v, w, q, offset, lod, anisotropy, uDelta, vDelta, face, true, function);

			Float4 lod4 = Float4(Frac(lod));

			c.x = (cc.x - c.x) * lod4 + c.x;
			c.y = (cc.y - c.y) * lod4 + c.y;
			c.z = (cc.z - c.z) * lod4 + c.z;
			c.w = (cc.w - c.w) * lod4 + c.w;
		}

		Int4 borderMask;

		if(state.addressingModeU == ADDRESSING_BORDER)
		{
			Int4 u0;

			border(u0, u);

			borderMask = u0;
		}

		if(state.addressingModeV == ADDRESSING_BORDER)
		{
			Int4 v0;

			border(v0, v);

			if(state.addressingModeU == ADDRESSING_BORDER)
			{
				borderMask &= v0;
			}
			else
			{
				borderMask = v0;
			}
		}

		if(state.addressingModeW == ADDRESSING_BORDER && state.textureType == TEXTURE_3D)
		{
			Int4 s0;

			border(s0, w);

			if(state.addressingModeU == ADDRESSING_BORDER ||
			   state.addressingModeV == ADDRESSING_BORDER)
			{
				borderMask &= s0;
			}
			else
			{
				borderMask = s0;
			}
		}

		if(state.addressingModeU == ADDRESSING_BORDER ||
		   state.addressingModeV == ADDRESSING_BORDER ||
		   (state.addressingModeW == ADDRESSING_BORDER && state.textureType == TEXTURE_3D))
		{
			Int4 b;

			c.x = As<Float4>((borderMask & As<Int4>(c.x)) | (~borderMask & *Pointer<Int4>(texture + OFFSET(Texture,borderColorF[0]))));
			c.y = As<Float4>((borderMask & As<Int4>(c.y)) | (~borderMask & *Pointer<Int4>(texture + OFFSET(Texture,borderColorF[1]))));
			c.z = As<Float4>((borderMask & As<Int4>(c.z)) | (~borderMask & *Pointer<Int4>(texture + OFFSET(Texture,borderColorF[2]))));
			c.w = As<Float4>((borderMask & As<Int4>(c.w)) | (~borderMask & *Pointer<Int4>(texture + OFFSET(Texture,borderColorF[3]))));
		}

		return c;
	}

	Vector4f SamplerCore::sampleFloatAniso(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Vector4f &offset, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Int face[4], bool secondLOD, SamplerFunction function)
	{
		Vector4f c;

		if(state.textureFilter != FILTER_ANISOTROPIC || function == Lod || function == Fetch)
		{
			c = sampleFloat(texture, u, v, w, q, offset, lod, face, secondLOD, function);
		}
		else
		{
			Int a = RoundInt(anisotropy);

			Vector4f cSum;

			cSum.x = Float4(0.0f);
			cSum.y = Float4(0.0f);
			cSum.z = Float4(0.0f);
			cSum.w = Float4(0.0f);

			Float4 A = *Pointer<Float4>(constants + OFFSET(Constants,uvWeight) + 16 * a);
			Float4 B = *Pointer<Float4>(constants + OFFSET(Constants,uvStart) + 16 * a);

			Float4 du = uDelta;
			Float4 dv = vDelta;

			Float4 u0 = u + B * du;
			Float4 v0 = v + B * dv;

			du *= A;
			dv *= A;

			Int i = 0;

			Do
			{
				c = sampleFloat(texture, u0, v0, w, q, offset, lod, face, secondLOD, function);

				u0 += du;
				v0 += dv;

				cSum.x += c.x * A;
				cSum.y += c.y * A;
				cSum.z += c.z * A;
				cSum.w += c.w * A;

				i++;
			}
			Until(i >= a)

			c.x = cSum.x;
			c.y = cSum.y;
			c.z = cSum.z;
			c.w = cSum.w;
		}

		return c;
	}

	Vector4f SamplerCore::sampleFloat(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Vector4f &offset, Float &lod, Int face[4], bool secondLOD, SamplerFunction function)
	{
		if(state.textureType != TEXTURE_3D)
		{
			return sampleFloat2D(texture, u, v, w, q, offset, lod, face, secondLOD, function);
		}
		else
		{
			return sampleFloat3D(texture, u, v, w, offset, lod, secondLOD, function);
		}
	}

	Vector4f SamplerCore::sampleFloat2D(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Vector4f &offset, Float &lod, Int face[4], bool secondLOD, SamplerFunction function)
	{
		Vector4f c;

		int componentCount = textureComponentCount();
		bool gather = state.textureFilter == FILTER_GATHER;

		Pointer<Byte> mipmap;
		Pointer<Byte> buffer[4];

		selectMipmap(texture, buffer, mipmap, lod, face, secondLOD);

		Int4 x0, x1, y0, y1, z0;
		Float4 fu, fv;
		Int4 filter = computeFilterOffset(lod);
		address(u, x0, x1, fu, mipmap, offset.x, filter, OFFSET(Mipmap, width), state.addressingModeU, function);
		address(v, y0, y1, fv, mipmap, offset.y, filter, OFFSET(Mipmap, height), state.addressingModeV, function);
		address(w, z0, z0, fv, mipmap, offset.z, filter, OFFSET(Mipmap, depth), state.addressingModeW, function);

		Int4 pitchP = *Pointer<Int4>(mipmap + OFFSET(Mipmap, pitchP), 16);
		y0 *= pitchP;
		if(hasThirdCoordinate())
		{
			Int4 sliceP = *Pointer<Int4>(mipmap + OFFSET(Mipmap, sliceP), 16);
			z0 *= sliceP;
		}

		if(state.textureFilter == FILTER_POINT || (function == Fetch))
		{
			c = sampleTexel(x0, y0, z0, q, mipmap, buffer, function);
		}
		else
		{
			y1 *= pitchP;

			Vector4f c0 = sampleTexel(x0, y0, z0, q, mipmap, buffer, function);
			Vector4f c1 = sampleTexel(x1, y0, z0, q, mipmap, buffer, function);
			Vector4f c2 = sampleTexel(x0, y1, z0, q, mipmap, buffer, function);
			Vector4f c3 = sampleTexel(x1, y1, z0, q, mipmap, buffer, function);

			if(!gather)   // Blend
			{
				if(componentCount >= 1) c0.x = c0.x + fu * (c1.x - c0.x);
				if(componentCount >= 2) c0.y = c0.y + fu * (c1.y - c0.y);
				if(componentCount >= 3) c0.z = c0.z + fu * (c1.z - c0.z);
				if(componentCount >= 4) c0.w = c0.w + fu * (c1.w - c0.w);

				if(componentCount >= 1) c2.x = c2.x + fu * (c3.x - c2.x);
				if(componentCount >= 2) c2.y = c2.y + fu * (c3.y - c2.y);
				if(componentCount >= 3) c2.z = c2.z + fu * (c3.z - c2.z);
				if(componentCount >= 4) c2.w = c2.w + fu * (c3.w - c2.w);

				if(componentCount >= 1) c.x = c0.x + fv * (c2.x - c0.x);
				if(componentCount >= 2) c.y = c0.y + fv * (c2.y - c0.y);
				if(componentCount >= 3) c.z = c0.z + fv * (c2.z - c0.z);
				if(componentCount >= 4) c.w = c0.w + fv * (c2.w - c0.w);
			}
			else
			{
				c.x = c1.x;
				c.y = c2.x;
				c.z = c3.x;
				c.w = c0.x;
			}
		}

		return c;
	}

	Vector4f SamplerCore::sampleFloat3D(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Vector4f &offset, Float &lod, bool secondLOD, SamplerFunction function)
	{
		Vector4f c;

		int componentCount = textureComponentCount();

		Pointer<Byte> mipmap;
		Pointer<Byte> buffer[4];
		Int face[4];

		selectMipmap(texture, buffer, mipmap, lod, face, secondLOD);

		Int4 x0, x1, y0, y1, z0, z1;
		Float4 fu, fv, fw;
		Int4 filter = computeFilterOffset(lod);
		address(u, x0, x1, fu, mipmap, offset.x, filter, OFFSET(Mipmap, width), state.addressingModeU, function);
		address(v, y0, y1, fv, mipmap, offset.y, filter, OFFSET(Mipmap, height), state.addressingModeV, function);
		address(w, z0, z1, fw, mipmap, offset.z, filter, OFFSET(Mipmap, depth), state.addressingModeW, function);

		Int4 pitchP = *Pointer<Int4>(mipmap + OFFSET(Mipmap, pitchP), 16);
		Int4 sliceP = *Pointer<Int4>(mipmap + OFFSET(Mipmap, sliceP), 16);
		y0 *= pitchP;
		z0 *= sliceP;

		if(state.textureFilter == FILTER_POINT || (function == Fetch))
		{
			c = sampleTexel(x0, y0, z0, w, mipmap, buffer, function);
		}
		else
		{
			y1 *= pitchP;
			z1 *= sliceP;

			Vector4f c0 = sampleTexel(x0, y0, z0, w, mipmap, buffer, function);
			Vector4f c1 = sampleTexel(x1, y0, z0, w, mipmap, buffer, function);
			Vector4f c2 = sampleTexel(x0, y1, z0, w, mipmap, buffer, function);
			Vector4f c3 = sampleTexel(x1, y1, z0, w, mipmap, buffer, function);
			Vector4f c4 = sampleTexel(x0, y0, z1, w, mipmap, buffer, function);
			Vector4f c5 = sampleTexel(x1, y0, z1, w, mipmap, buffer, function);
			Vector4f c6 = sampleTexel(x0, y1, z1, w, mipmap, buffer, function);
			Vector4f c7 = sampleTexel(x1, y1, z1, w, mipmap, buffer, function);

			// Blend first slice
			if(componentCount >= 1) c0.x = c0.x + fu * (c1.x - c0.x);
			if(componentCount >= 2) c0.y = c0.y + fu * (c1.y - c0.y);
			if(componentCount >= 3) c0.z = c0.z + fu * (c1.z - c0.z);
			if(componentCount >= 4) c0.w = c0.w + fu * (c1.w - c0.w);

			if(componentCount >= 1) c2.x = c2.x + fu * (c3.x - c2.x);
			if(componentCount >= 2) c2.y = c2.y + fu * (c3.y - c2.y);
			if(componentCount >= 3) c2.z = c2.z + fu * (c3.z - c2.z);
			if(componentCount >= 4) c2.w = c2.w + fu * (c3.w - c2.w);

			if(componentCount >= 1) c0.x = c0.x + fv * (c2.x - c0.x);
			if(componentCount >= 2) c0.y = c0.y + fv * (c2.y - c0.y);
			if(componentCount >= 3) c0.z = c0.z + fv * (c2.z - c0.z);
			if(componentCount >= 4) c0.w = c0.w + fv * (c2.w - c0.w);

			// Blend second slice
			if(componentCount >= 1) c4.x = c4.x + fu * (c5.x - c4.x);
			if(componentCount >= 2) c4.y = c4.y + fu * (c5.y - c4.y);
			if(componentCount >= 3) c4.z = c4.z + fu * (c5.z - c4.z);
			if(componentCount >= 4) c4.w = c4.w + fu * (c5.w - c4.w);

			if(componentCount >= 1) c6.x = c6.x + fu * (c7.x - c6.x);
			if(componentCount >= 2) c6.y = c6.y + fu * (c7.y - c6.y);
			if(componentCount >= 3) c6.z = c6.z + fu * (c7.z - c6.z);
			if(componentCount >= 4) c6.w = c6.w + fu * (c7.w - c6.w);

			if(componentCount >= 1) c4.x = c4.x + fv * (c6.x - c4.x);
			if(componentCount >= 2) c4.y = c4.y + fv * (c6.y - c4.y);
			if(componentCount >= 3) c4.z = c4.z + fv * (c6.z - c4.z);
			if(componentCount >= 4) c4.w = c4.w + fv * (c6.w - c4.w);

			// Blend slices
			if(componentCount >= 1) c.x = c0.x + fw * (c4.x - c0.x);
			if(componentCount >= 2) c.y = c0.y + fw * (c4.y - c0.y);
			if(componentCount >= 3) c.z = c0.z + fw * (c4.z - c0.z);
			if(componentCount >= 4) c.w = c0.w + fw * (c4.w - c0.w);
		}

		return c;
	}

	Float SamplerCore::log2sqrt(Float lod)
	{
		// log2(sqrt(lod))                               // Equals 0.25 * log2(lod^2).
		lod *= lod;                                      // Squaring doubles the exponent and produces an extra bit of precision.
		lod = Float(As<Int>(lod)) - Float(0x3F800000);   // Interpret as integer and subtract the exponent bias.
		lod *= As<Float>(Int(0x33000000));               // Scale by 0.25 * 2^-23 (mantissa length).

		return lod;
	}

	Float SamplerCore::log2(Float lod)
	{
		lod *= lod;                                      // Squaring doubles the exponent and produces an extra bit of precision.
		lod = Float(As<Int>(lod)) - Float(0x3F800000);   // Interpret as integer and subtract the exponent bias.
		lod *= As<Float>(Int(0x33800000));               // Scale by 0.5 * 2^-23 (mantissa length).

		return lod;
	}

	void SamplerCore::computeLod(Pointer<Byte> &texture, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Float4 &uuuu, Float4 &vvvv, const Float &lodBias, Vector4f &dsx, Vector4f &dsy, SamplerFunction function)
	{
		if(function != Lod && function != Fetch)
		{
			Float4 duvdxy;

			if(function != Grad)   // Implicit
			{
				duvdxy = Float4(uuuu.yz, vvvv.yz) - Float4(uuuu.xx, vvvv.xx);
			}
			else
			{
				Float4 dudxy = Float4(dsx.x.xx, dsy.x.xx);
				Float4 dvdxy = Float4(dsx.y.xx, dsy.y.xx);

				duvdxy = Float4(dudxy.xz, dvdxy.xz);
			}

			// Scale by texture dimensions and global LOD.
			Float4 dUVdxy = duvdxy * *Pointer<Float4>(texture + OFFSET(Texture,widthHeightLOD));

			Float4 dUV2dxy = dUVdxy * dUVdxy;
			Float4 dUV2 = dUV2dxy.xy + dUV2dxy.zw;

			lod = Max(Float(dUV2.x), Float(dUV2.y));   // Square length of major axis

			if(state.textureFilter == FILTER_ANISOTROPIC)
			{
				Float det = Abs(Float(dUVdxy.x) * Float(dUVdxy.w) - Float(dUVdxy.y) * Float(dUVdxy.z));

				Float4 dudx = duvdxy.xxxx;
				Float4 dudy = duvdxy.yyyy;
				Float4 dvdx = duvdxy.zzzz;
				Float4 dvdy = duvdxy.wwww;

				Int4 mask = As<Int4>(CmpNLT(dUV2.x, dUV2.y));
				uDelta = As<Float4>((As<Int4>(dudx) & mask) | ((As<Int4>(dudy) & ~mask)));
				vDelta = As<Float4>((As<Int4>(dvdx) & mask) | ((As<Int4>(dvdy) & ~mask)));

				anisotropy = lod * Rcp_pp(det);
				anisotropy = Min(anisotropy, *Pointer<Float>(texture + OFFSET(Texture,maxAnisotropy)));

				lod *= Rcp_pp(anisotropy * anisotropy);
			}

			lod = log2sqrt(lod);   // log2(sqrt(lod))

			if(function == Bias)
			{
				lod += lodBias;
			}
		}
		else if(function == Lod)
		{
			lod = lodBias;
		}
		else if(function == Fetch)
		{
			// TODO: Eliminate int-float-int conversion.
			lod = Float(As<Int>(lodBias));
		}
		else if(function == Base)
		{
			lod = Float(0);
		}
		else assert(false);

		lod = Max(lod, *Pointer<Float>(texture + OFFSET(Texture, minLod)));
		lod = Min(lod, *Pointer<Float>(texture + OFFSET(Texture, maxLod)));
	}

	void SamplerCore::computeLodCube(Pointer<Byte> &texture, Float &lod, Float4 &u, Float4 &v, Float4 &w, const Float &lodBias, Vector4f &dsx, Vector4f &dsy, Float4 &M, SamplerFunction function)
	{
		if(function != Lod && function != Fetch)
		{
			Float4 dudxy, dvdxy, dsdxy;

			if(function != Grad)  // Implicit
			{
				Float4 U = u * M;
				Float4 V = v * M;
				Float4 W = w * M;

				dudxy = Abs(U - U.xxxx);
				dvdxy = Abs(V - V.xxxx);
				dsdxy = Abs(W - W.xxxx);
			}
			else
			{
				dudxy = Float4(dsx.x.xx, dsy.x.xx);
				dvdxy = Float4(dsx.y.xx, dsy.y.xx);
				dsdxy = Float4(dsx.z.xx, dsy.z.xx);

				dudxy = Abs(dudxy * Float4(M.x));
				dvdxy = Abs(dvdxy * Float4(M.x));
				dsdxy = Abs(dsdxy * Float4(M.x));
			}

			// Compute the largest Manhattan distance in two dimensions.
			// This takes the footprint across adjacent faces into account.
			Float4 duvdxy = dudxy + dvdxy;
			Float4 dusdxy = dudxy + dsdxy;
			Float4 dvsdxy = dvdxy + dsdxy;

			dudxy = Max(Max(duvdxy, dusdxy), dvsdxy);

			lod = Max(Float(dudxy.y), Float(dudxy.z));   // FIXME: Max(dudxy.y, dudxy.z);

			// Scale by texture dimension and global LOD.
			lod *= *Pointer<Float>(texture + OFFSET(Texture,widthLOD));

			lod = log2(lod);

			if(function == Bias)
			{
				lod += lodBias;
			}
		}
		else if(function == Lod)
		{
			lod = lodBias;
		}
		else if(function == Fetch)
		{
			// TODO: Eliminate int-float-int conversion.
			lod = Float(As<Int>(lodBias));
		}
		else if(function == Base)
		{
			lod = Float(0);
		}
		else assert(false);

		lod = Max(lod, *Pointer<Float>(texture + OFFSET(Texture, minLod)));
		lod = Min(lod, *Pointer<Float>(texture + OFFSET(Texture, maxLod)));
	}

	void SamplerCore::computeLod3D(Pointer<Byte> &texture, Float &lod, Float4 &uuuu, Float4 &vvvv, Float4 &wwww, const Float &lodBias, Vector4f &dsx, Vector4f &dsy, SamplerFunction function)
	{
		if(function != Lod && function != Fetch)
		{
			Float4 dudxy, dvdxy, dsdxy;

			if(function != Grad)   // Implicit
			{
				dudxy = uuuu - uuuu.xxxx;
				dvdxy = vvvv - vvvv.xxxx;
				dsdxy = wwww - wwww.xxxx;
			}
			else
			{
				dudxy = Float4(dsx.x.xx, dsy.x.xx);
				dvdxy = Float4(dsx.y.xx, dsy.y.xx);
				dsdxy = Float4(dsx.z.xx, dsy.z.xx);
			}

			// Scale by texture dimensions and global LOD.
			dudxy *= *Pointer<Float4>(texture + OFFSET(Texture,widthLOD));
			dvdxy *= *Pointer<Float4>(texture + OFFSET(Texture,heightLOD));
			dsdxy *= *Pointer<Float4>(texture + OFFSET(Texture,depthLOD));

			dudxy *= dudxy;
			dvdxy *= dvdxy;
			dsdxy *= dsdxy;

			dudxy += dvdxy;
			dudxy += dsdxy;

			lod = Max(Float(dudxy.y), Float(dudxy.z));   // FIXME: Max(dudxy.y, dudxy.z);

			lod = log2sqrt(lod);   // log2(sqrt(lod))

			if(function == Bias)
			{
				lod += lodBias;
			}
		}
		else if(function == Lod)
		{
			lod = lodBias;
		}
		else if(function == Fetch)
		{
			// TODO: Eliminate int-float-int conversion.
			lod = Float(As<Int>(lodBias));
		}
		else if(function == Base)
		{
			lod = Float(0);
		}
		else assert(false);

		lod = Max(lod, *Pointer<Float>(texture + OFFSET(Texture, minLod)));
		lod = Min(lod, *Pointer<Float>(texture + OFFSET(Texture, maxLod)));
	}

	void SamplerCore::cubeFace(Int face[4], Float4 &U, Float4 &V, Float4 &x, Float4 &y, Float4 &z, Float4 &M)
	{
		Int4 xn = CmpLT(x, Float4(0.0f));   // x < 0
		Int4 yn = CmpLT(y, Float4(0.0f));   // y < 0
		Int4 zn = CmpLT(z, Float4(0.0f));   // z < 0

		Float4 absX = Abs(x);
		Float4 absY = Abs(y);
		Float4 absZ = Abs(z);

		Int4 xy = CmpNLE(absX, absY);   // abs(x) > abs(y)
		Int4 yz = CmpNLE(absY, absZ);   // abs(y) > abs(z)
		Int4 zx = CmpNLE(absZ, absX);   // abs(z) > abs(x)
		Int4 xMajor = xy & ~zx;   // abs(x) > abs(y) && abs(x) > abs(z)
		Int4 yMajor = yz & ~xy;   // abs(y) > abs(z) && abs(y) > abs(x)
		Int4 zMajor = zx & ~yz;   // abs(z) > abs(x) && abs(z) > abs(y)

		// FACE_POSITIVE_X = 000b
		// FACE_NEGATIVE_X = 001b
		// FACE_POSITIVE_Y = 010b
		// FACE_NEGATIVE_Y = 011b
		// FACE_POSITIVE_Z = 100b
		// FACE_NEGATIVE_Z = 101b

		Int yAxis = SignMask(yMajor);
		Int zAxis = SignMask(zMajor);

		Int4 n = ((xn & xMajor) | (yn & yMajor) | (zn & zMajor)) & Int4(0x80000000);
		Int negative = SignMask(n);

		face[0] = *Pointer<Int>(constants + OFFSET(Constants,transposeBit0) + negative * 4);
		face[0] |= *Pointer<Int>(constants + OFFSET(Constants,transposeBit1) + yAxis * 4);
		face[0] |= *Pointer<Int>(constants + OFFSET(Constants,transposeBit2) + zAxis * 4);
		face[1] = (face[0] >> 4)  & 0x7;
		face[2] = (face[0] >> 8)  & 0x7;
		face[3] = (face[0] >> 12) & 0x7;
		face[0] &= 0x7;

		M = Max(Max(absX, absY), absZ);

		// U = xMajor ? (neg ^ -z) : ((zMajor & neg) ^ x)
		U = As<Float4>((xMajor & (n ^ As<Int4>(-z))) | (~xMajor & ((zMajor & n) ^ As<Int4>(x))));

		// V = !yMajor ? -y : (n ^ z)
		V = As<Float4>((~yMajor & As<Int4>(-y)) | (yMajor & (n ^ As<Int4>(z))));

		M = reciprocal(M) * Float4(0.5f);
		U = U * M + Float4(0.5f);
		V = V * M + Float4(0.5f);
	}

	Short4 SamplerCore::applyOffset(Short4 &uvw, Float4 &offset, const Int4 &whd, AddressingMode mode)
	{
		Int4 tmp = Int4(As<UShort4>(uvw));
		tmp = tmp + As<Int4>(offset);

		switch(mode)
		{
		case AddressingMode::ADDRESSING_WRAP:
			tmp = (tmp + whd * Int4(-MIN_PROGRAM_TEXEL_OFFSET)) % whd;
			break;
		case AddressingMode::ADDRESSING_CLAMP:
		case AddressingMode::ADDRESSING_MIRROR:
		case AddressingMode::ADDRESSING_MIRRORONCE:
		case AddressingMode::ADDRESSING_BORDER: // FIXME: Implement and test ADDRESSING_MIRROR, ADDRESSING_MIRRORONCE, ADDRESSING_BORDER
			tmp = Min(Max(tmp, Int4(0)), whd - Int4(1));
			break;
		case ADDRESSING_TEXELFETCH:
			break;
		case AddressingMode::ADDRESSING_SEAMLESS:
			ASSERT(false);   // Cube sampling doesn't support offset.
		default:
			ASSERT(false);
		}

		return As<Short4>(UShort4(tmp));
	}

	void SamplerCore::computeIndices(UInt index[4], Short4 uuuu, Short4 vvvv, Short4 wwww, Vector4f &offset, const Pointer<Byte> &mipmap, SamplerFunction function)
	{
		bool texelFetch = (function == Fetch);
		bool hasOffset = (function.option == Offset);

		if(!texelFetch)
		{
			uuuu = MulHigh(As<UShort4>(uuuu), *Pointer<UShort4>(mipmap + OFFSET(Mipmap, width)));
			vvvv = MulHigh(As<UShort4>(vvvv), *Pointer<UShort4>(mipmap + OFFSET(Mipmap, height)));
		}

		if(hasOffset)
		{
			UShort4 w = *Pointer<UShort4>(mipmap + OFFSET(Mipmap, width));
			uuuu = applyOffset(uuuu, offset.x, Int4(w), texelFetch ? ADDRESSING_TEXELFETCH : state.addressingModeU);
			UShort4 h = *Pointer<UShort4>(mipmap + OFFSET(Mipmap, height));
			vvvv = applyOffset(vvvv, offset.y, Int4(h), texelFetch ? ADDRESSING_TEXELFETCH : state.addressingModeV);
		}

		Short4 uuu2 = uuuu;
		uuuu = As<Short4>(UnpackLow(uuuu, vvvv));
		uuu2 = As<Short4>(UnpackHigh(uuu2, vvvv));
		uuuu = As<Short4>(MulAdd(uuuu, *Pointer<Short4>(mipmap + OFFSET(Mipmap,onePitchP))));
		uuu2 = As<Short4>(MulAdd(uuu2, *Pointer<Short4>(mipmap + OFFSET(Mipmap,onePitchP))));

		if(hasThirdCoordinate())
		{
			if(state.textureType != TEXTURE_2D_ARRAY)
			{
				if(!texelFetch)
				{
					wwww = MulHigh(As<UShort4>(wwww), *Pointer<UShort4>(mipmap + OFFSET(Mipmap, depth)));
				}

				if(hasOffset)
				{
					UShort4 d = *Pointer<UShort4>(mipmap + OFFSET(Mipmap, depth));
					wwww = applyOffset(wwww, offset.z, Int4(d), texelFetch ? ADDRESSING_TEXELFETCH : state.addressingModeW);
				}
			}

			UInt4 uv(As<UInt2>(uuuu), As<UInt2>(uuu2));
			uv += As<UInt4>(Int4(As<UShort4>(wwww))) * *Pointer<UInt4>(mipmap + OFFSET(Mipmap, sliceP));

			index[0] = Extract(As<Int4>(uv), 0);
			index[1] = Extract(As<Int4>(uv), 1);
			index[2] = Extract(As<Int4>(uv), 2);
			index[3] = Extract(As<Int4>(uv), 3);
		}
		else
		{
			index[0] = Extract(As<Int2>(uuuu), 0);
			index[1] = Extract(As<Int2>(uuuu), 1);
			index[2] = Extract(As<Int2>(uuu2), 0);
			index[3] = Extract(As<Int2>(uuu2), 1);
		}

		if(texelFetch)
		{
			Int size = Int(*Pointer<Int>(mipmap + OFFSET(Mipmap, sliceP)));
			if(hasThirdCoordinate())
			{
				size *= Int(*Pointer<Short>(mipmap + OFFSET(Mipmap, depth)));
			}
			UInt min = 0;
			UInt max = size - 1;

			for(int i = 0; i < 4; i++)
			{
				index[i] = Min(Max(index[i], min), max);
			}
		}
	}

	void SamplerCore::computeIndices(UInt index[4], Int4& uuuu, Int4& vvvv, Int4& wwww, const Pointer<Byte> &mipmap, SamplerFunction function)
	{
		UInt4 indices = uuuu + vvvv;

		if(hasThirdCoordinate())
		{
			indices += As<UInt4>(wwww);
		}

		for(int i = 0; i < 4; i++)
		{
			index[i] = Extract(As<Int4>(indices), i);
		}
	}

	Vector4s SamplerCore::sampleTexel(UInt index[4], Pointer<Byte> buffer[4])
	{
		Vector4s c;

		int f0 = state.textureType == TEXTURE_CUBE ? 0 : 0;
		int f1 = state.textureType == TEXTURE_CUBE ? 1 : 0;
		int f2 = state.textureType == TEXTURE_CUBE ? 2 : 0;
		int f3 = state.textureType == TEXTURE_CUBE ? 3 : 0;

		if(has16bitTextureFormat())
		{
			c.x = Insert(c.x, Pointer<Short>(buffer[f0])[index[0]], 0);
			c.x = Insert(c.x, Pointer<Short>(buffer[f1])[index[1]], 1);
			c.x = Insert(c.x, Pointer<Short>(buffer[f2])[index[2]], 2);
			c.x = Insert(c.x, Pointer<Short>(buffer[f3])[index[3]], 3);

			switch(state.textureFormat)
			{
			case FORMAT_R5G6B5:
				c.z = (c.x & Short4(0x001Fu)) << 11;
				c.y = (c.x & Short4(0x07E0u)) << 5;
				c.x = (c.x & Short4(0xF800u));
				break;
			default:
				ASSERT(false);
			}
		}
		else if(has8bitTextureComponents())
		{
			switch(textureComponentCount())
			{
			case 4:
				{
					Byte4 c0 = Pointer<Byte4>(buffer[f0])[index[0]];
					Byte4 c1 = Pointer<Byte4>(buffer[f1])[index[1]];
					Byte4 c2 = Pointer<Byte4>(buffer[f2])[index[2]];
					Byte4 c3 = Pointer<Byte4>(buffer[f3])[index[3]];
					c.x = Unpack(c0, c1);
					c.y = Unpack(c2, c3);

					switch(state.textureFormat)
					{
					case FORMAT_A8R8G8B8:
						c.z = As<Short4>(UnpackLow(c.x, c.y));
						c.x = As<Short4>(UnpackHigh(c.x, c.y));
						c.y = c.z;
						c.w = c.x;
						c.z = UnpackLow(As<Byte8>(c.z), As<Byte8>(c.z));
						c.y = UnpackHigh(As<Byte8>(c.y), As<Byte8>(c.y));
						c.x = UnpackLow(As<Byte8>(c.x), As<Byte8>(c.x));
						c.w = UnpackHigh(As<Byte8>(c.w), As<Byte8>(c.w));
						break;
					case FORMAT_A8B8G8R8:
					case FORMAT_A8B8G8R8I:
					case FORMAT_A8B8G8R8_SNORM:
					case FORMAT_Q8W8V8U8:
					case FORMAT_SRGB8_A8:
						c.z = As<Short4>(UnpackHigh(c.x, c.y));
						c.x = As<Short4>(UnpackLow(c.x, c.y));
						c.y = c.x;
						c.w = c.z;
						c.x = UnpackLow(As<Byte8>(c.x), As<Byte8>(c.x));
						c.y = UnpackHigh(As<Byte8>(c.y), As<Byte8>(c.y));
						c.z = UnpackLow(As<Byte8>(c.z), As<Byte8>(c.z));
						c.w = UnpackHigh(As<Byte8>(c.w), As<Byte8>(c.w));
						// Propagate sign bit
						if(state.textureFormat == FORMAT_A8B8G8R8I)
						{
							c.x >>= 8;
							c.y >>= 8;
							c.z >>= 8;
							c.w >>= 8;
						}
						break;
					case FORMAT_A8B8G8R8UI:
						c.z = As<Short4>(UnpackHigh(c.x, c.y));
						c.x = As<Short4>(UnpackLow(c.x, c.y));
						c.y = c.x;
						c.w = c.z;
						c.x = UnpackLow(As<Byte8>(c.x), As<Byte8>(Short4(0)));
						c.y = UnpackHigh(As<Byte8>(c.y), As<Byte8>(Short4(0)));
						c.z = UnpackLow(As<Byte8>(c.z), As<Byte8>(Short4(0)));
						c.w = UnpackHigh(As<Byte8>(c.w), As<Byte8>(Short4(0)));
						break;
					default:
						ASSERT(false);
					}
				}
				break;
			case 3:
				{
					Byte4 c0 = Pointer<Byte4>(buffer[f0])[index[0]];
					Byte4 c1 = Pointer<Byte4>(buffer[f1])[index[1]];
					Byte4 c2 = Pointer<Byte4>(buffer[f2])[index[2]];
					Byte4 c3 = Pointer<Byte4>(buffer[f3])[index[3]];
					c.x = Unpack(c0, c1);
					c.y = Unpack(c2, c3);

					switch(state.textureFormat)
					{
					case FORMAT_X8R8G8B8:
						c.z = As<Short4>(UnpackLow(c.x, c.y));
						c.x = As<Short4>(UnpackHigh(c.x, c.y));
						c.y = c.z;
						c.z = UnpackLow(As<Byte8>(c.z), As<Byte8>(c.z));
						c.y = UnpackHigh(As<Byte8>(c.y), As<Byte8>(c.y));
						c.x = UnpackLow(As<Byte8>(c.x), As<Byte8>(c.x));
						break;
					case FORMAT_X8B8G8R8_SNORM:
					case FORMAT_X8B8G8R8I:
					case FORMAT_X8B8G8R8:
					case FORMAT_X8L8V8U8:
					case FORMAT_SRGB8_X8:
						c.z = As<Short4>(UnpackHigh(c.x, c.y));
						c.x = As<Short4>(UnpackLow(c.x, c.y));
						c.y = c.x;
						c.x = UnpackLow(As<Byte8>(c.x), As<Byte8>(c.x));
						c.y = UnpackHigh(As<Byte8>(c.y), As<Byte8>(c.y));
						c.z = UnpackLow(As<Byte8>(c.z), As<Byte8>(c.z));
						// Propagate sign bit
						if(state.textureFormat == FORMAT_X8B8G8R8I)
						{
							c.x >>= 8;
							c.y >>= 8;
							c.z >>= 8;
						}
						break;
					case FORMAT_X8B8G8R8UI:
						c.z = As<Short4>(UnpackHigh(c.x, c.y));
						c.x = As<Short4>(UnpackLow(c.x, c.y));
						c.y = c.x;
						c.x = UnpackLow(As<Byte8>(c.x), As<Byte8>(Short4(0)));
						c.y = UnpackHigh(As<Byte8>(c.y), As<Byte8>(Short4(0)));
						c.z = UnpackLow(As<Byte8>(c.z), As<Byte8>(Short4(0)));
						break;
					default:
						ASSERT(false);
					}
				}
				break;
			case 2:
				c.x = Insert(c.x, Pointer<Short>(buffer[f0])[index[0]], 0);
				c.x = Insert(c.x, Pointer<Short>(buffer[f1])[index[1]], 1);
				c.x = Insert(c.x, Pointer<Short>(buffer[f2])[index[2]], 2);
				c.x = Insert(c.x, Pointer<Short>(buffer[f3])[index[3]], 3);

				switch(state.textureFormat)
				{
				case FORMAT_G8R8:
				case FORMAT_G8R8_SNORM:
				case FORMAT_V8U8:
				case FORMAT_A8L8:
					c.y = (c.x & Short4(0xFF00u)) | As<Short4>(As<UShort4>(c.x) >> 8);
					c.x = (c.x & Short4(0x00FFu)) | (c.x << 8);
					break;
				case FORMAT_G8R8I:
					c.y = c.x >> 8;
					c.x = (c.x << 8) >> 8; // Propagate sign bit
					break;
				case FORMAT_G8R8UI:
					c.y = As<Short4>(As<UShort4>(c.x) >> 8);
					c.x &= Short4(0x00FFu);
					break;
				default:
					ASSERT(false);
				}
				break;
			case 1:
				{
					Int c0 = Int(*Pointer<Byte>(buffer[f0] + index[0]));
					Int c1 = Int(*Pointer<Byte>(buffer[f1] + index[1]));
					Int c2 = Int(*Pointer<Byte>(buffer[f2] + index[2]));
					Int c3 = Int(*Pointer<Byte>(buffer[f3] + index[3]));
					c0 = c0 | (c1 << 8) | (c2 << 16) | (c3 << 24);

					switch(state.textureFormat)
					{
					case FORMAT_R8I:
					case FORMAT_R8UI:
						{
							Int zero(0);
							c.x = Unpack(As<Byte4>(c0), As<Byte4>(zero));
							// Propagate sign bit
							if(state.textureFormat == FORMAT_R8I)
							{
								c.x = (c.x << 8) >> 8;
							}
						}
						break;
					default:
						c.x = Unpack(As<Byte4>(c0));
						break;
					}
				}
				break;
			default:
				ASSERT(false);
			}
		}
		else if(has16bitTextureComponents())
		{
			switch(textureComponentCount())
			{
			case 4:
				c.x = Pointer<Short4>(buffer[f0])[index[0]];
				c.y = Pointer<Short4>(buffer[f1])[index[1]];
				c.z = Pointer<Short4>(buffer[f2])[index[2]];
				c.w = Pointer<Short4>(buffer[f3])[index[3]];
				transpose4x4(c.x, c.y, c.z, c.w);
				break;
			case 3:
				c.x = Pointer<Short4>(buffer[f0])[index[0]];
				c.y = Pointer<Short4>(buffer[f1])[index[1]];
				c.z = Pointer<Short4>(buffer[f2])[index[2]];
				c.w = Pointer<Short4>(buffer[f3])[index[3]];
				transpose4x3(c.x, c.y, c.z, c.w);
				break;
			case 2:
				c.x = *Pointer<Short4>(buffer[f0] + 4 * index[0]);
				c.x = As<Short4>(UnpackLow(c.x, *Pointer<Short4>(buffer[f1] + 4 * index[1])));
				c.z = *Pointer<Short4>(buffer[f2] + 4 * index[2]);
				c.z = As<Short4>(UnpackLow(c.z, *Pointer<Short4>(buffer[f3] + 4 * index[3])));
				c.y = c.x;
				c.x = UnpackLow(As<Int2>(c.x), As<Int2>(c.z));
				c.y = UnpackHigh(As<Int2>(c.y), As<Int2>(c.z));
				break;
			case 1:
				c.x = Insert(c.x, Pointer<Short>(buffer[f0])[index[0]], 0);
				c.x = Insert(c.x, Pointer<Short>(buffer[f1])[index[1]], 1);
				c.x = Insert(c.x, Pointer<Short>(buffer[f2])[index[2]], 2);
				c.x = Insert(c.x, Pointer<Short>(buffer[f3])[index[3]], 3);
				break;
			default:
				ASSERT(false);
			}
		}
		else ASSERT(false);

		if(state.sRGB)
		{
			if(state.textureFormat == FORMAT_R5G6B5)
			{
				sRGBtoLinear16_5_16(c.x);
				sRGBtoLinear16_6_16(c.y);
				sRGBtoLinear16_5_16(c.z);
			}
			else
			{
				for(int i = 0; i < textureComponentCount(); i++)
				{
					if(isRGBComponent(i))
					{
						sRGBtoLinear16_8_16(c[i]);
					}
				}
			}
		}

		return c;
	}

	Vector4s SamplerCore::sampleTexel(Short4 &uuuu, Short4 &vvvv, Short4 &wwww, Vector4f &offset, Pointer<Byte> &mipmap, Pointer<Byte> buffer[4], SamplerFunction function)
	{
		Vector4s c;

		UInt index[4];
		computeIndices(index, uuuu, vvvv, wwww, offset, mipmap, function);

		if(hasYuvFormat())
		{
			// Generic YPbPr to RGB transformation
			// R = Y                               +           2 * (1 - Kr) * Pr
			// G = Y - 2 * Kb * (1 - Kb) / Kg * Pb - 2 * Kr * (1 - Kr) / Kg * Pr
			// B = Y +           2 * (1 - Kb) * Pb

			float Kb = 0.114f;
			float Kr = 0.299f;
			int studioSwing = 1;

			switch(state.textureFormat)
			{
			case FORMAT_YV12_BT601:
				Kb = 0.114f;
				Kr = 0.299f;
				studioSwing = 1;
				break;
			case FORMAT_YV12_BT709:
				Kb = 0.0722f;
				Kr = 0.2126f;
				studioSwing = 1;
				break;
			case FORMAT_YV12_JFIF:
				Kb = 0.114f;
				Kr = 0.299f;
				studioSwing = 0;
				break;
			default:
				ASSERT(false);
			}

			const float Kg = 1.0f - Kr - Kb;

			const float Rr = 2 * (1 - Kr);
			const float Gb = -2 * Kb * (1 - Kb) / Kg;
			const float Gr = -2 * Kr * (1 - Kr) / Kg;
			const float Bb = 2 * (1 - Kb);

			// Scaling and bias for studio-swing range: Y = [16 .. 235], U/V = [16 .. 240]
			const float Yy = studioSwing ? 255.0f / (235 - 16) : 1.0f;
			const float Uu = studioSwing ? 255.0f / (240 - 16) : 1.0f;
			const float Vv = studioSwing ? 255.0f / (240 - 16) : 1.0f;

			const float Rv = Vv *  Rr;
			const float Gu = Uu *  Gb;
			const float Gv = Vv *  Gr;
			const float Bu = Uu *  Bb;

			const float R0 = (studioSwing * -16 * Yy - 128 * Rv) / 255;
			const float G0 = (studioSwing * -16 * Yy - 128 * Gu - 128 * Gv) / 255;
			const float B0 = (studioSwing * -16 * Yy - 128 * Bu) / 255;

			Int c0 = Int(buffer[0][index[0]]);
			Int c1 = Int(buffer[0][index[1]]);
			Int c2 = Int(buffer[0][index[2]]);
			Int c3 = Int(buffer[0][index[3]]);
			c0 = c0 | (c1 << 8) | (c2 << 16) | (c3 << 24);
			UShort4 Y = As<UShort4>(Unpack(As<Byte4>(c0)));

			computeIndices(index, uuuu, vvvv, wwww, offset, mipmap + sizeof(Mipmap), function);
			c0 = Int(buffer[1][index[0]]);
			c1 = Int(buffer[1][index[1]]);
			c2 = Int(buffer[1][index[2]]);
			c3 = Int(buffer[1][index[3]]);
			c0 = c0 | (c1 << 8) | (c2 << 16) | (c3 << 24);
			UShort4 V = As<UShort4>(Unpack(As<Byte4>(c0)));

			c0 = Int(buffer[2][index[0]]);
			c1 = Int(buffer[2][index[1]]);
			c2 = Int(buffer[2][index[2]]);
			c3 = Int(buffer[2][index[3]]);
			c0 = c0 | (c1 << 8) | (c2 << 16) | (c3 << 24);
			UShort4 U = As<UShort4>(Unpack(As<Byte4>(c0)));

			const UShort4 yY = UShort4(iround(Yy * 0x4000));
			const UShort4 rV = UShort4(iround(Rv * 0x4000));
			const UShort4 gU = UShort4(iround(-Gu * 0x4000));
			const UShort4 gV = UShort4(iround(-Gv * 0x4000));
			const UShort4 bU = UShort4(iround(Bu * 0x4000));

			const UShort4 r0 = UShort4(iround(-R0 * 0x4000));
			const UShort4 g0 = UShort4(iround(G0 * 0x4000));
			const UShort4 b0 = UShort4(iround(-B0 * 0x4000));

			UShort4 y = MulHigh(Y, yY);
			UShort4 r = SubSat(y + MulHigh(V, rV), r0);
			UShort4 g = SubSat(y + g0, MulHigh(U, gU) + MulHigh(V, gV));
			UShort4 b = SubSat(y + MulHigh(U, bU), b0);

			c.x = Min(r, UShort4(0x3FFF)) << 2;
			c.y = Min(g, UShort4(0x3FFF)) << 2;
			c.z = Min(b, UShort4(0x3FFF)) << 2;
		}
		else
		{
			return sampleTexel(index, buffer);
		}

		return c;
	}

	Vector4f SamplerCore::sampleTexel(Int4 &uuuu, Int4 &vvvv, Int4 &wwww, Float4 &z, Pointer<Byte> &mipmap, Pointer<Byte> buffer[4], SamplerFunction function)
	{
		Vector4f c;

		UInt index[4];
		computeIndices(index, uuuu, vvvv, wwww, mipmap, function);

		if(hasFloatTexture() || has32bitIntegerTextureComponents())
		{
			int f0 = state.textureType == TEXTURE_CUBE ? 0 : 0;
			int f1 = state.textureType == TEXTURE_CUBE ? 1 : 0;
			int f2 = state.textureType == TEXTURE_CUBE ? 2 : 0;
			int f3 = state.textureType == TEXTURE_CUBE ? 3 : 0;

			// Read texels
			switch(textureComponentCount())
			{
			case 4:
				c.x = *Pointer<Float4>(buffer[f0] + index[0] * 16, 16);
				c.y = *Pointer<Float4>(buffer[f1] + index[1] * 16, 16);
				c.z = *Pointer<Float4>(buffer[f2] + index[2] * 16, 16);
				c.w = *Pointer<Float4>(buffer[f3] + index[3] * 16, 16);
				transpose4x4(c.x, c.y, c.z, c.w);
				break;
			case 3:
				c.x = *Pointer<Float4>(buffer[f0] + index[0] * 16, 16);
				c.y = *Pointer<Float4>(buffer[f1] + index[1] * 16, 16);
				c.z = *Pointer<Float4>(buffer[f2] + index[2] * 16, 16);
				c.w = *Pointer<Float4>(buffer[f3] + index[3] * 16, 16);
				transpose4x3(c.x, c.y, c.z, c.w);
				break;
			case 2:
				// FIXME: Optimal shuffling?
				c.x.xy = *Pointer<Float4>(buffer[f0] + index[0] * 8);
				c.x.zw = *Pointer<Float4>(buffer[f1] + index[1] * 8 - 8);
				c.z.xy = *Pointer<Float4>(buffer[f2] + index[2] * 8);
				c.z.zw = *Pointer<Float4>(buffer[f3] + index[3] * 8 - 8);
				c.y = c.x;
				c.x = Float4(c.x.xz, c.z.xz);
				c.y = Float4(c.y.yw, c.z.yw);
				break;
			case 1:
				// FIXME: Optimal shuffling?
				c.x.x = *Pointer<Float>(buffer[f0] + index[0] * 4);
				c.x.y = *Pointer<Float>(buffer[f1] + index[1] * 4);
				c.x.z = *Pointer<Float>(buffer[f2] + index[2] * 4);
				c.x.w = *Pointer<Float>(buffer[f3] + index[3] * 4);
				break;
			default:
				ASSERT(false);
			}

			if(state.compare != COMPARE_BYPASS)
			{
				Float4 ref = z;

				if(!hasFloatTexture())
				{
					ref = Min(Max(ref, Float4(0.0f)), Float4(1.0f));
				}

				Int4 boolean;

				switch(state.compare)
				{
				case COMPARE_LESSEQUAL:    boolean = CmpLE(ref, c.x);  break;
				case COMPARE_GREATEREQUAL: boolean = CmpNLT(ref, c.x); break;
				case COMPARE_LESS:         boolean = CmpLT(ref, c.x);  break;
				case COMPARE_GREATER:      boolean = CmpNLE(ref, c.x); break;
				case COMPARE_EQUAL:        boolean = CmpEQ(ref, c.x);  break;
				case COMPARE_NOTEQUAL:     boolean = CmpNEQ(ref, c.x); break;
				case COMPARE_ALWAYS:       boolean = Int4(-1);         break;
				case COMPARE_NEVER:        boolean = Int4(0);          break;
				default:                   ASSERT(false);
				}

				c.x = As<Float4>(boolean & As<Int4>(Float4(1.0f)));
				c.y = Float4(0.0f);
				c.z = Float4(0.0f);
				c.w = Float4(1.0f);
			}
		}
		else
		{
			ASSERT(!hasYuvFormat());

			Vector4s cs = sampleTexel(index, buffer);

			bool isInteger = Surface::isNonNormalizedInteger(state.textureFormat);
			int componentCount = textureComponentCount();
			for(int n = 0; n < componentCount; n++)
			{
				if(hasUnsignedTextureComponent(n))
				{
					if(isInteger)
					{
						c[n] = As<Float4>(Int4(As<UShort4>(cs[n])));
					}
					else
					{
						c[n] = Float4(As<UShort4>(cs[n]));
					}
				}
				else
				{
					if(isInteger)
					{
						c[n] = As<Float4>(Int4(cs[n]));
					}
					else
					{
						c[n] = Float4(cs[n]);
					}
				}
			}
		}

		return c;
	}

	void SamplerCore::selectMipmap(Pointer<Byte> &texture, Pointer<Byte> buffer[4], Pointer<Byte> &mipmap, Float &lod, Int face[4], bool secondLOD)
	{
		if(state.mipmapFilter == MIPMAP_NONE)
		{
			mipmap = texture + OFFSET(Texture,mipmap[0]);
		}
		else
		{
			Int ilod;

			if(state.mipmapFilter == MIPMAP_POINT)
			{
				ilod = RoundInt(lod);
			}
			else   // MIPMAP_LINEAR
			{
				ilod = Int(lod);
			}

			mipmap = texture + OFFSET(Texture,mipmap) + ilod * sizeof(Mipmap) + secondLOD * sizeof(Mipmap);
		}

		if(state.textureType != TEXTURE_CUBE)
		{
			buffer[0] = *Pointer<Pointer<Byte> >(mipmap + OFFSET(Mipmap,buffer[0]));

			if(hasYuvFormat())
			{
				buffer[1] = *Pointer<Pointer<Byte> >(mipmap + OFFSET(Mipmap,buffer[1]));
				buffer[2] = *Pointer<Pointer<Byte> >(mipmap + OFFSET(Mipmap,buffer[2]));
			}
		}
		else
		{
			for(int i = 0; i < 4; i++)
			{
				buffer[i] = *Pointer<Pointer<Byte> >(mipmap + OFFSET(Mipmap,buffer) + face[i] * sizeof(void*));
			}
		}
	}

	Int4 SamplerCore::computeFilterOffset(Float &lod)
	{
		Int4 filter = -1;

		if(state.textureFilter == FILTER_POINT)
		{
			filter = 0;
		}
		else if(state.textureFilter == FILTER_MIN_LINEAR_MAG_POINT)
		{
			filter = CmpNLE(Float4(lod), Float4(0.0f));
		}
		else if(state.textureFilter == FILTER_MIN_POINT_MAG_LINEAR)
		{
			filter = CmpLE(Float4(lod), Float4(0.0f));
		}

		return filter;
	}

	Short4 SamplerCore::address(Float4 &uw, AddressingMode addressingMode, Pointer<Byte> &mipmap)
	{
		if(addressingMode == ADDRESSING_LAYER && state.textureType != TEXTURE_2D_ARRAY)
		{
			return Short4();   // Unused
		}
		else if(addressingMode == ADDRESSING_LAYER && state.textureType == TEXTURE_2D_ARRAY)
		{
			return Min(Max(Short4(RoundInt(uw)), Short4(0)), *Pointer<Short4>(mipmap + OFFSET(Mipmap, depth)) - Short4(1));
		}
		else if(addressingMode == ADDRESSING_CLAMP || addressingMode == ADDRESSING_BORDER)
		{
			Float4 clamp = Min(Max(uw, Float4(0.0f)), Float4(65535.0f / 65536.0f));

			return Short4(Int4(clamp * Float4(1 << 16)));
		}
		else if(addressingMode == ADDRESSING_MIRROR)
		{
			Int4 convert = Int4(uw * Float4(1 << 16));
			Int4 mirror = (convert << 15) >> 31;

			convert ^= mirror;

			return Short4(convert);
		}
		else if(addressingMode == ADDRESSING_MIRRORONCE)
		{
			// Absolute value
			Int4 convert = Int4(Abs(uw * Float4(1 << 16)));

			// Clamp
			convert -= Int4(0x00008000, 0x00008000, 0x00008000, 0x00008000);
			convert = As<Int4>(PackSigned(convert, convert));

			return As<Short4>(Int2(convert)) + Short4(0x8000u);
		}
		else   // Wrap
		{
			return Short4(Int4(uw * Float4(1 << 16)));
		}
	}

	void SamplerCore::address(Float4 &uvw, Int4 &xyz0, Int4 &xyz1, Float4 &f, Pointer<Byte> &mipmap, Float4 &texOffset, Int4 &filter, int whd, AddressingMode addressingMode, SamplerFunction function)
	{
		if(addressingMode == ADDRESSING_LAYER && state.textureType != TEXTURE_2D_ARRAY)
		{
			return;   // Unused
		}

		Int4 dim = Int4(*Pointer<Short4>(mipmap + whd, 16));
		Int4 maxXYZ = dim - Int4(1);

		if(function == Fetch)
		{
			xyz0 = Min(Max(((function.option == Offset) && (addressingMode != ADDRESSING_LAYER)) ? As<Int4>(uvw) + As<Int4>(texOffset) : As<Int4>(uvw), Int4(0)), maxXYZ);
		}
		else if(addressingMode == ADDRESSING_LAYER && state.textureType == TEXTURE_2D_ARRAY)   // Note: Offset does not apply to array layers
		{
			xyz0 = Min(Max(RoundInt(uvw), Int4(0)), maxXYZ);
		}
		else
		{
			const int halfBits = 0x3EFFFFFF;   // Value just under 0.5f
			const int oneBits  = 0x3F7FFFFF;   // Value just under 1.0f
			const int twoBits  = 0x3FFFFFFF;   // Value just under 2.0f

			bool pointFilter = state.textureFilter == FILTER_POINT ||
			                   state.textureFilter == FILTER_MIN_POINT_MAG_LINEAR ||
			                   state.textureFilter == FILTER_MIN_LINEAR_MAG_POINT;

			Float4 coord = uvw;

			if(state.textureType == TEXTURE_RECTANGLE)
			{
				// According to https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_rectangle.txt
				// "CLAMP_TO_EDGE causes the s coordinate to be clamped to the range[0.5, wt - 0.5].
				//  CLAMP_TO_EDGE causes the t coordinate to be clamped to the range[0.5, ht - 0.5]."
				// Unless SwiftShader implements support for ADDRESSING_BORDER, other modes should be equivalent
				// to CLAMP_TO_EDGE. Rectangle textures have no support for any MIRROR or REPEAT modes.
				coord = Min(Max(coord, Float4(0.5f)), Float4(dim) - Float4(0.5f));
			}
			else
			{
				switch(addressingMode)
				{
				case ADDRESSING_CLAMP:
				case ADDRESSING_BORDER:
				case ADDRESSING_SEAMLESS:
					// Linear filtering of cube doesn't require clamping because the coordinates
					// are already in [0, 1] range and numerical imprecision is tolerated.
					if(addressingMode != ADDRESSING_SEAMLESS || pointFilter)
					{
						Float4 one = As<Float4>(Int4(oneBits));
						coord = Min(Max(coord, Float4(0.0f)), one);
					}
					break;
				case ADDRESSING_MIRROR:
				{
					Float4 half = As<Float4>(Int4(halfBits));
					Float4 one = As<Float4>(Int4(oneBits));
					Float4 two = As<Float4>(Int4(twoBits));
					coord = one - Abs(two * Frac(coord * half) - one);
				}
				break;
				case ADDRESSING_MIRRORONCE:
				{
					Float4 half = As<Float4>(Int4(halfBits));
					Float4 one = As<Float4>(Int4(oneBits));
					Float4 two = As<Float4>(Int4(twoBits));
					coord = one - Abs(two * Frac(Min(Max(coord, -one), two) * half) - one);
				}
				break;
				default:   // Wrap
					coord = Frac(coord);
					break;
				}

				coord = coord * Float4(dim);
			}

			if(state.textureFilter == FILTER_POINT ||
			   state.textureFilter == FILTER_GATHER)
			{
				xyz0 = Int4(coord);
			}
			else
			{
				if(state.textureFilter == FILTER_MIN_POINT_MAG_LINEAR ||
				   state.textureFilter == FILTER_MIN_LINEAR_MAG_POINT)
				{
					coord -= As<Float4>(As<Int4>(Float4(0.5f)) & filter);
				}
				else
				{
					coord -= Float4(0.5f);
				}

				Float4 floor = Floor(coord);
				xyz0 = Int4(floor);
				f = coord - floor;
			}

			if(function.option == Offset)
			{
				xyz0 += As<Int4>(texOffset);
			}

			if(addressingMode == ADDRESSING_SEAMLESS)
			{
				xyz0 += Int4(1);
			}

			xyz1 = xyz0 - filter;   // Increment

			if(function.option == Offset)
			{
				switch(addressingMode)
				{
				case ADDRESSING_SEAMLESS:
					ASSERT(false);   // Cube sampling doesn't support offset.
				case ADDRESSING_MIRROR:
				case ADDRESSING_MIRRORONCE:
				case ADDRESSING_BORDER:
					// FIXME: Implement ADDRESSING_MIRROR, ADDRESSING_MIRRORONCE, and ADDRESSING_BORDER.
					// Fall through to Clamp.
				case ADDRESSING_CLAMP:
					xyz0 = Min(Max(xyz0, Int4(0)), maxXYZ);
					xyz1 = Min(Max(xyz1, Int4(0)), maxXYZ);
					break;
				default:   // Wrap
					xyz0 = (xyz0 + dim * Int4(-MIN_PROGRAM_TEXEL_OFFSET)) % dim;
					xyz1 = (xyz1 + dim * Int4(-MIN_PROGRAM_TEXEL_OFFSET)) % dim;
					break;
				}
			}
			else if(state.textureFilter != FILTER_POINT)
			{
				switch(addressingMode)
				{
				case ADDRESSING_SEAMLESS:
					break;
				case ADDRESSING_MIRROR:
				case ADDRESSING_MIRRORONCE:
				case ADDRESSING_BORDER:
				case ADDRESSING_CLAMP:
					xyz0 = Max(xyz0, Int4(0));
					xyz1 = Min(xyz1, maxXYZ);
					break;
				default:   // Wrap
					{
						Int4 under = CmpLT(xyz0, Int4(0));
						xyz0 = (under & maxXYZ) | (~under & xyz0);   // xyz < 0 ? dim - 1 : xyz   // FIXME: IfThenElse()

						Int4 nover = CmpLT(xyz1, dim);
						xyz1 = nover & xyz1;   // xyz >= dim ? 0 : xyz
					}
					break;
				}
			}
		}
	}

	void SamplerCore::convertFixed12(Short4 &cs, Float4 &cf)
	{
		cs = RoundShort4(cf * Float4(0x1000));
	}

	void SamplerCore::convertFixed12(Vector4s &cs, Vector4f &cf)
	{
		convertFixed12(cs.x, cf.x);
		convertFixed12(cs.y, cf.y);
		convertFixed12(cs.z, cf.z);
		convertFixed12(cs.w, cf.w);
	}

	void SamplerCore::convertSigned12(Float4 &cf, Short4 &cs)
	{
		cf = Float4(cs) * Float4(1.0f / 0x0FFE);
	}

//	void SamplerCore::convertSigned12(Vector4f &cf, Vector4s &cs)
//	{
//		convertSigned12(cf.x, cs.x);
//		convertSigned12(cf.y, cs.y);
//		convertSigned12(cf.z, cs.z);
//		convertSigned12(cf.w, cs.w);
//	}

	void SamplerCore::convertSigned15(Float4 &cf, Short4 &cs)
	{
		cf = Float4(cs) * Float4(1.0f / 0x7FFF);
	}

	void SamplerCore::convertUnsigned16(Float4 &cf, Short4 &cs)
	{
		cf = Float4(As<UShort4>(cs)) * Float4(1.0f / 0xFFFF);
	}

	void SamplerCore::sRGBtoLinear16_8_16(Short4 &c)
	{
		c = As<UShort4>(c) >> 8;

		Pointer<Byte> LUT = Pointer<Byte>(constants + OFFSET(Constants,sRGBtoLinear8_16));

		c = Insert(c, *Pointer<Short>(LUT + 2 * Int(Extract(c, 0))), 0);
		c = Insert(c, *Pointer<Short>(LUT + 2 * Int(Extract(c, 1))), 1);
		c = Insert(c, *Pointer<Short>(LUT + 2 * Int(Extract(c, 2))), 2);
		c = Insert(c, *Pointer<Short>(LUT + 2 * Int(Extract(c, 3))), 3);
	}

	void SamplerCore::sRGBtoLinear16_6_16(Short4 &c)
	{
		c = As<UShort4>(c) >> 10;

		Pointer<Byte> LUT = Pointer<Byte>(constants + OFFSET(Constants,sRGBtoLinear6_16));

		c = Insert(c, *Pointer<Short>(LUT + 2 * Int(Extract(c, 0))), 0);
		c = Insert(c, *Pointer<Short>(LUT + 2 * Int(Extract(c, 1))), 1);
		c = Insert(c, *Pointer<Short>(LUT + 2 * Int(Extract(c, 2))), 2);
		c = Insert(c, *Pointer<Short>(LUT + 2 * Int(Extract(c, 3))), 3);
	}

	void SamplerCore::sRGBtoLinear16_5_16(Short4 &c)
	{
		c = As<UShort4>(c) >> 11;

		Pointer<Byte> LUT = Pointer<Byte>(constants + OFFSET(Constants,sRGBtoLinear5_16));

		c = Insert(c, *Pointer<Short>(LUT + 2 * Int(Extract(c, 0))), 0);
		c = Insert(c, *Pointer<Short>(LUT + 2 * Int(Extract(c, 1))), 1);
		c = Insert(c, *Pointer<Short>(LUT + 2 * Int(Extract(c, 2))), 2);
		c = Insert(c, *Pointer<Short>(LUT + 2 * Int(Extract(c, 3))), 3);
	}

	bool SamplerCore::hasFloatTexture() const
	{
		return Surface::isFloatFormat(state.textureFormat);
	}

	bool SamplerCore::hasUnnormalizedIntegerTexture() const
	{
		return Surface::isNonNormalizedInteger(state.textureFormat);
	}

	bool SamplerCore::hasUnsignedTextureComponent(int component) const
	{
		return Surface::isUnsignedComponent(state.textureFormat, component);
	}

	int SamplerCore::textureComponentCount() const
	{
		return Surface::componentCount(state.textureFormat);
	}

	bool SamplerCore::hasThirdCoordinate() const
	{
		return (state.textureType == TEXTURE_3D) || (state.textureType == TEXTURE_2D_ARRAY);
	}

	bool SamplerCore::has16bitTextureFormat() const
	{
		switch(state.textureFormat)
		{
		case FORMAT_R5G6B5:
			return true;
		case FORMAT_R8_SNORM:
		case FORMAT_G8R8_SNORM:
		case FORMAT_X8B8G8R8_SNORM:
		case FORMAT_A8B8G8R8_SNORM:
		case FORMAT_R8I:
		case FORMAT_R8UI:
		case FORMAT_G8R8I:
		case FORMAT_G8R8UI:
		case FORMAT_X8B8G8R8I:
		case FORMAT_X8B8G8R8UI:
		case FORMAT_A8B8G8R8I:
		case FORMAT_A8B8G8R8UI:
		case FORMAT_R32I:
		case FORMAT_R32UI:
		case FORMAT_G32R32I:
		case FORMAT_G32R32UI:
		case FORMAT_X32B32G32R32I:
		case FORMAT_X32B32G32R32UI:
		case FORMAT_A32B32G32R32I:
		case FORMAT_A32B32G32R32UI:
		case FORMAT_G8R8:
		case FORMAT_X8R8G8B8:
		case FORMAT_X8B8G8R8:
		case FORMAT_A8R8G8B8:
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
		case FORMAT_V8U8:
		case FORMAT_Q8W8V8U8:
		case FORMAT_X8L8V8U8:
		case FORMAT_R32F:
		case FORMAT_G32R32F:
		case FORMAT_X32B32G32R32F:
		case FORMAT_A32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_A8:
		case FORMAT_R8:
		case FORMAT_L8:
		case FORMAT_A8L8:
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
		case FORMAT_L16:
		case FORMAT_G16R16:
		case FORMAT_A16B16G16R16:
		case FORMAT_V16U16:
		case FORMAT_A16W16V16U16:
		case FORMAT_Q16W16V16U16:
		case FORMAT_R16I:
		case FORMAT_R16UI:
		case FORMAT_G16R16I:
		case FORMAT_G16R16UI:
		case FORMAT_X16B16G16R16I:
		case FORMAT_X16B16G16R16UI:
		case FORMAT_A16B16G16R16I:
		case FORMAT_A16B16G16R16UI:
		case FORMAT_YV12_BT601:
		case FORMAT_YV12_BT709:
		case FORMAT_YV12_JFIF:
			return false;
		default:
			ASSERT(false);
		}

		return false;
	}

	bool SamplerCore::has8bitTextureComponents() const
	{
		switch(state.textureFormat)
		{
		case FORMAT_G8R8:
		case FORMAT_X8R8G8B8:
		case FORMAT_X8B8G8R8:
		case FORMAT_A8R8G8B8:
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
		case FORMAT_V8U8:
		case FORMAT_Q8W8V8U8:
		case FORMAT_X8L8V8U8:
		case FORMAT_A8:
		case FORMAT_R8:
		case FORMAT_L8:
		case FORMAT_A8L8:
		case FORMAT_R8_SNORM:
		case FORMAT_G8R8_SNORM:
		case FORMAT_X8B8G8R8_SNORM:
		case FORMAT_A8B8G8R8_SNORM:
		case FORMAT_R8I:
		case FORMAT_R8UI:
		case FORMAT_G8R8I:
		case FORMAT_G8R8UI:
		case FORMAT_X8B8G8R8I:
		case FORMAT_X8B8G8R8UI:
		case FORMAT_A8B8G8R8I:
		case FORMAT_A8B8G8R8UI:
			return true;
		case FORMAT_R5G6B5:
		case FORMAT_R32F:
		case FORMAT_G32R32F:
		case FORMAT_X32B32G32R32F:
		case FORMAT_A32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
		case FORMAT_L16:
		case FORMAT_G16R16:
		case FORMAT_A16B16G16R16:
		case FORMAT_V16U16:
		case FORMAT_A16W16V16U16:
		case FORMAT_Q16W16V16U16:
		case FORMAT_R32I:
		case FORMAT_R32UI:
		case FORMAT_G32R32I:
		case FORMAT_G32R32UI:
		case FORMAT_X32B32G32R32I:
		case FORMAT_X32B32G32R32UI:
		case FORMAT_A32B32G32R32I:
		case FORMAT_A32B32G32R32UI:
		case FORMAT_R16I:
		case FORMAT_R16UI:
		case FORMAT_G16R16I:
		case FORMAT_G16R16UI:
		case FORMAT_X16B16G16R16I:
		case FORMAT_X16B16G16R16UI:
		case FORMAT_A16B16G16R16I:
		case FORMAT_A16B16G16R16UI:
		case FORMAT_YV12_BT601:
		case FORMAT_YV12_BT709:
		case FORMAT_YV12_JFIF:
			return false;
		default:
			ASSERT(false);
		}

		return false;
	}

	bool SamplerCore::has16bitTextureComponents() const
	{
		switch(state.textureFormat)
		{
		case FORMAT_R5G6B5:
		case FORMAT_R8_SNORM:
		case FORMAT_G8R8_SNORM:
		case FORMAT_X8B8G8R8_SNORM:
		case FORMAT_A8B8G8R8_SNORM:
		case FORMAT_R8I:
		case FORMAT_R8UI:
		case FORMAT_G8R8I:
		case FORMAT_G8R8UI:
		case FORMAT_X8B8G8R8I:
		case FORMAT_X8B8G8R8UI:
		case FORMAT_A8B8G8R8I:
		case FORMAT_A8B8G8R8UI:
		case FORMAT_R32I:
		case FORMAT_R32UI:
		case FORMAT_G32R32I:
		case FORMAT_G32R32UI:
		case FORMAT_X32B32G32R32I:
		case FORMAT_X32B32G32R32UI:
		case FORMAT_A32B32G32R32I:
		case FORMAT_A32B32G32R32UI:
		case FORMAT_G8R8:
		case FORMAT_X8R8G8B8:
		case FORMAT_X8B8G8R8:
		case FORMAT_A8R8G8B8:
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
		case FORMAT_V8U8:
		case FORMAT_Q8W8V8U8:
		case FORMAT_X8L8V8U8:
		case FORMAT_R32F:
		case FORMAT_G32R32F:
		case FORMAT_X32B32G32R32F:
		case FORMAT_A32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_A8:
		case FORMAT_R8:
		case FORMAT_L8:
		case FORMAT_A8L8:
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
		case FORMAT_YV12_BT601:
		case FORMAT_YV12_BT709:
		case FORMAT_YV12_JFIF:
			return false;
		case FORMAT_L16:
		case FORMAT_G16R16:
		case FORMAT_A16B16G16R16:
		case FORMAT_R16I:
		case FORMAT_R16UI:
		case FORMAT_G16R16I:
		case FORMAT_G16R16UI:
		case FORMAT_X16B16G16R16I:
		case FORMAT_X16B16G16R16UI:
		case FORMAT_A16B16G16R16I:
		case FORMAT_A16B16G16R16UI:
		case FORMAT_V16U16:
		case FORMAT_A16W16V16U16:
		case FORMAT_Q16W16V16U16:
			return true;
		default:
			ASSERT(false);
		}

		return false;
	}

	bool SamplerCore::has32bitIntegerTextureComponents() const
	{
		switch(state.textureFormat)
		{
		case FORMAT_R5G6B5:
		case FORMAT_R8_SNORM:
		case FORMAT_G8R8_SNORM:
		case FORMAT_X8B8G8R8_SNORM:
		case FORMAT_A8B8G8R8_SNORM:
		case FORMAT_R8I:
		case FORMAT_R8UI:
		case FORMAT_G8R8I:
		case FORMAT_G8R8UI:
		case FORMAT_X8B8G8R8I:
		case FORMAT_X8B8G8R8UI:
		case FORMAT_A8B8G8R8I:
		case FORMAT_A8B8G8R8UI:
		case FORMAT_G8R8:
		case FORMAT_X8R8G8B8:
		case FORMAT_X8B8G8R8:
		case FORMAT_A8R8G8B8:
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
		case FORMAT_V8U8:
		case FORMAT_Q8W8V8U8:
		case FORMAT_X8L8V8U8:
		case FORMAT_L16:
		case FORMAT_G16R16:
		case FORMAT_A16B16G16R16:
		case FORMAT_R16I:
		case FORMAT_R16UI:
		case FORMAT_G16R16I:
		case FORMAT_G16R16UI:
		case FORMAT_X16B16G16R16I:
		case FORMAT_X16B16G16R16UI:
		case FORMAT_A16B16G16R16I:
		case FORMAT_A16B16G16R16UI:
		case FORMAT_V16U16:
		case FORMAT_A16W16V16U16:
		case FORMAT_Q16W16V16U16:
		case FORMAT_R32F:
		case FORMAT_G32R32F:
		case FORMAT_X32B32G32R32F:
		case FORMAT_A32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_A8:
		case FORMAT_R8:
		case FORMAT_L8:
		case FORMAT_A8L8:
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
		case FORMAT_YV12_BT601:
		case FORMAT_YV12_BT709:
		case FORMAT_YV12_JFIF:
			return false;
		case FORMAT_R32I:
		case FORMAT_R32UI:
		case FORMAT_G32R32I:
		case FORMAT_G32R32UI:
		case FORMAT_X32B32G32R32I:
		case FORMAT_X32B32G32R32UI:
		case FORMAT_A32B32G32R32I:
		case FORMAT_A32B32G32R32UI:
			return true;
		default:
			ASSERT(false);
		}

		return false;
	}

	bool SamplerCore::hasYuvFormat() const
	{
		switch(state.textureFormat)
		{
		case FORMAT_YV12_BT601:
		case FORMAT_YV12_BT709:
		case FORMAT_YV12_JFIF:
			return true;
		case FORMAT_R5G6B5:
		case FORMAT_R8_SNORM:
		case FORMAT_G8R8_SNORM:
		case FORMAT_X8B8G8R8_SNORM:
		case FORMAT_A8B8G8R8_SNORM:
		case FORMAT_R8I:
		case FORMAT_R8UI:
		case FORMAT_G8R8I:
		case FORMAT_G8R8UI:
		case FORMAT_X8B8G8R8I:
		case FORMAT_X8B8G8R8UI:
		case FORMAT_A8B8G8R8I:
		case FORMAT_A8B8G8R8UI:
		case FORMAT_R32I:
		case FORMAT_R32UI:
		case FORMAT_G32R32I:
		case FORMAT_G32R32UI:
		case FORMAT_X32B32G32R32I:
		case FORMAT_X32B32G32R32UI:
		case FORMAT_A32B32G32R32I:
		case FORMAT_A32B32G32R32UI:
		case FORMAT_G8R8:
		case FORMAT_X8R8G8B8:
		case FORMAT_X8B8G8R8:
		case FORMAT_A8R8G8B8:
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
		case FORMAT_V8U8:
		case FORMAT_Q8W8V8U8:
		case FORMAT_X8L8V8U8:
		case FORMAT_R32F:
		case FORMAT_G32R32F:
		case FORMAT_X32B32G32R32F:
		case FORMAT_A32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_A8:
		case FORMAT_R8:
		case FORMAT_L8:
		case FORMAT_A8L8:
		case FORMAT_D32F:
		case FORMAT_D32FS8:
		case FORMAT_D32F_LOCKABLE:
		case FORMAT_D32FS8_TEXTURE:
		case FORMAT_D32F_SHADOW:
		case FORMAT_D32FS8_SHADOW:
		case FORMAT_L16:
		case FORMAT_G16R16:
		case FORMAT_A16B16G16R16:
		case FORMAT_R16I:
		case FORMAT_R16UI:
		case FORMAT_G16R16I:
		case FORMAT_G16R16UI:
		case FORMAT_X16B16G16R16I:
		case FORMAT_X16B16G16R16UI:
		case FORMAT_A16B16G16R16I:
		case FORMAT_A16B16G16R16UI:
		case FORMAT_V16U16:
		case FORMAT_A16W16V16U16:
		case FORMAT_Q16W16V16U16:
			return false;
		default:
			ASSERT(false);
		}

		return false;
	}

	bool SamplerCore::isRGBComponent(int component) const
	{
		switch(state.textureFormat)
		{
		case FORMAT_R5G6B5:         return component < 3;
		case FORMAT_R8_SNORM:      return component < 1;
		case FORMAT_G8R8_SNORM:    return component < 2;
		case FORMAT_X8B8G8R8_SNORM: return component < 3;
		case FORMAT_A8B8G8R8_SNORM: return component < 3;
		case FORMAT_R8I:            return component < 1;
		case FORMAT_R8UI:           return component < 1;
		case FORMAT_G8R8I:          return component < 2;
		case FORMAT_G8R8UI:         return component < 2;
		case FORMAT_X8B8G8R8I:      return component < 3;
		case FORMAT_X8B8G8R8UI:     return component < 3;
		case FORMAT_A8B8G8R8I:      return component < 3;
		case FORMAT_A8B8G8R8UI:     return component < 3;
		case FORMAT_R32I:           return component < 1;
		case FORMAT_R32UI:          return component < 1;
		case FORMAT_G32R32I:        return component < 2;
		case FORMAT_G32R32UI:       return component < 2;
		case FORMAT_X32B32G32R32I:  return component < 3;
		case FORMAT_X32B32G32R32UI: return component < 3;
		case FORMAT_A32B32G32R32I:  return component < 3;
		case FORMAT_A32B32G32R32UI: return component < 3;
		case FORMAT_G8R8:           return component < 2;
		case FORMAT_X8R8G8B8:       return component < 3;
		case FORMAT_X8B8G8R8:       return component < 3;
		case FORMAT_A8R8G8B8:       return component < 3;
		case FORMAT_A8B8G8R8:       return component < 3;
		case FORMAT_SRGB8_X8:       return component < 3;
		case FORMAT_SRGB8_A8:       return component < 3;
		case FORMAT_V8U8:           return false;
		case FORMAT_Q8W8V8U8:       return false;
		case FORMAT_X8L8V8U8:       return false;
		case FORMAT_R32F:           return component < 1;
		case FORMAT_G32R32F:        return component < 2;
		case FORMAT_X32B32G32R32F:  return component < 3;
		case FORMAT_A32B32G32R32F:  return component < 3;
		case FORMAT_X32B32G32R32F_UNSIGNED: return component < 3;
		case FORMAT_A8:             return false;
		case FORMAT_R8:             return component < 1;
		case FORMAT_L8:             return component < 1;
		case FORMAT_A8L8:           return component < 1;
		case FORMAT_D32F:           return false;
		case FORMAT_D32FS8:         return false;
		case FORMAT_D32F_LOCKABLE:  return false;
		case FORMAT_D32FS8_TEXTURE: return false;
		case FORMAT_D32F_SHADOW:    return false;
		case FORMAT_D32FS8_SHADOW:  return false;
		case FORMAT_L16:            return component < 1;
		case FORMAT_G16R16:         return component < 2;
		case FORMAT_A16B16G16R16:   return component < 3;
		case FORMAT_R16I:           return component < 1;
		case FORMAT_R16UI:          return component < 1;
		case FORMAT_G16R16I:        return component < 2;
		case FORMAT_G16R16UI:       return component < 2;
		case FORMAT_X16B16G16R16I:  return component < 3;
		case FORMAT_X16B16G16R16UI: return component < 3;
		case FORMAT_A16B16G16R16I:  return component < 3;
		case FORMAT_A16B16G16R16UI: return component < 3;
		case FORMAT_V16U16:         return false;
		case FORMAT_A16W16V16U16:   return false;
		case FORMAT_Q16W16V16U16:   return false;
		case FORMAT_YV12_BT601:     return component < 3;
		case FORMAT_YV12_BT709:     return component < 3;
		case FORMAT_YV12_JFIF:      return component < 3;
		default:
			ASSERT(false);
		}

		return false;
	}
}
