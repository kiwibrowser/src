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

#include "QuadRasterizer.hpp"

#include "Primitive.hpp"
#include "Renderer.hpp"
#include "Shader/Constants.hpp"
#include "Common/Math.hpp"
#include "Common/Debug.hpp"

namespace sw
{
	extern bool veryEarlyDepthTest;
	extern bool complementaryDepthBuffer;
	extern bool fullPixelPositionRegister;

	extern int clusterCount;

	QuadRasterizer::QuadRasterizer(const PixelProcessor::State &state, const PixelShader *pixelShader) : state(state), shader(pixelShader)
	{
	}

	QuadRasterizer::~QuadRasterizer()
	{
	}

	void QuadRasterizer::generate()
	{
		#if PERF_PROFILE
			for(int i = 0; i < PERF_TIMERS; i++)
			{
				cycles[i] = 0;
			}

			Long pixelTime = Ticks();
		#endif

		constants = *Pointer<Pointer<Byte>>(data + OFFSET(DrawData,constants));
		occlusion = 0;
		int clusterCount = Renderer::getClusterCount();

		Do
		{
			Int yMin = *Pointer<Int>(primitive + OFFSET(Primitive,yMin));
			Int yMax = *Pointer<Int>(primitive + OFFSET(Primitive,yMax));

			Int cluster2 = cluster + cluster;
			yMin += clusterCount * 2 - 2 - cluster2;
			yMin &= -clusterCount * 2;
			yMin += cluster2;

			If(yMin < yMax)
			{
				rasterize(yMin, yMax);
			}

			primitive += sizeof(Primitive) * state.multiSample;
			count--;
		}
		Until(count == 0)

		if(state.occlusionEnabled)
		{
			UInt clusterOcclusion = *Pointer<UInt>(data + OFFSET(DrawData,occlusion) + 4 * cluster);
			clusterOcclusion += occlusion;
			*Pointer<UInt>(data + OFFSET(DrawData,occlusion) + 4 * cluster) = clusterOcclusion;
		}

		#if PERF_PROFILE
			cycles[PERF_PIXEL] = Ticks() - pixelTime;

			for(int i = 0; i < PERF_TIMERS; i++)
			{
				*Pointer<Long>(data + OFFSET(DrawData,cycles[i]) + 8 * cluster) += cycles[i];
			}
		#endif

		Return();
	}

	void QuadRasterizer::rasterize(Int &yMin, Int &yMax)
	{
		Pointer<Byte> cBuffer[RENDERTARGETS];
		Pointer<Byte> zBuffer;
		Pointer<Byte> sBuffer;

		for(int index = 0; index < RENDERTARGETS; index++)
		{
			if(state.colorWriteActive(index))
			{
				cBuffer[index] = *Pointer<Pointer<Byte>>(data + OFFSET(DrawData,colorBuffer[index])) + yMin * *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));
			}
		}

		if(state.depthTestActive)
		{
			zBuffer = *Pointer<Pointer<Byte>>(data + OFFSET(DrawData,depthBuffer)) + yMin * *Pointer<Int>(data + OFFSET(DrawData,depthPitchB));
		}

		if(state.stencilActive)
		{
			sBuffer = *Pointer<Pointer<Byte>>(data + OFFSET(DrawData,stencilBuffer)) + yMin * *Pointer<Int>(data + OFFSET(DrawData,stencilPitchB));
		}

		Int y = yMin;

		Do
		{
			Int x0a = Int(*Pointer<Short>(primitive + OFFSET(Primitive,outline->left) + (y + 0) * sizeof(Primitive::Span)));
			Int x0b = Int(*Pointer<Short>(primitive + OFFSET(Primitive,outline->left) + (y + 1) * sizeof(Primitive::Span)));
			Int x0 = Min(x0a, x0b);

			for(unsigned int q = 1; q < state.multiSample; q++)
			{
				x0a = Int(*Pointer<Short>(primitive + q * sizeof(Primitive) + OFFSET(Primitive,outline->left) + (y + 0) * sizeof(Primitive::Span)));
				x0b = Int(*Pointer<Short>(primitive + q * sizeof(Primitive) + OFFSET(Primitive,outline->left) + (y + 1) * sizeof(Primitive::Span)));
				x0 = Min(x0, Min(x0a, x0b));
			}

			x0 &= 0xFFFFFFFE;

			Int x1a = Int(*Pointer<Short>(primitive + OFFSET(Primitive,outline->right) + (y + 0) * sizeof(Primitive::Span)));
			Int x1b = Int(*Pointer<Short>(primitive + OFFSET(Primitive,outline->right) + (y + 1) * sizeof(Primitive::Span)));
			Int x1 = Max(x1a, x1b);

			for(unsigned int q = 1; q < state.multiSample; q++)
			{
				x1a = Int(*Pointer<Short>(primitive + q * sizeof(Primitive) + OFFSET(Primitive,outline->right) + (y + 0) * sizeof(Primitive::Span)));
				x1b = Int(*Pointer<Short>(primitive + q * sizeof(Primitive) + OFFSET(Primitive,outline->right) + (y + 1) * sizeof(Primitive::Span)));
				x1 = Max(x1, Max(x1a, x1b));
			}

			Float4 yyyy = Float4(Float(y)) + *Pointer<Float4>(primitive + OFFSET(Primitive,yQuad), 16);

			if(interpolateZ())
			{
				for(unsigned int q = 0; q < state.multiSample; q++)
				{
					Float4 y = yyyy;

					if(state.multiSample > 1)
					{
						y -= *Pointer<Float4>(constants + OFFSET(Constants,Y) + q * sizeof(float4));
					}

					Dz[q] = *Pointer<Float4>(primitive + OFFSET(Primitive,z.C), 16) + y * *Pointer<Float4>(primitive + OFFSET(Primitive,z.B), 16);
				}
			}

			if(veryEarlyDepthTest && state.multiSample == 1 && !state.depthOverride)
			{
				if(!state.stencilActive && state.depthTestActive && (state.depthCompareMode == DEPTH_LESSEQUAL || state.depthCompareMode == DEPTH_LESS))   // FIXME: Both modes ok?
				{
					Float4 xxxx = Float4(Float(x0)) + *Pointer<Float4>(primitive + OFFSET(Primitive,xQuad), 16);

					Pointer<Byte> buffer;
					Int pitch;

					if(!state.quadLayoutDepthBuffer)
					{
						buffer = zBuffer + 4 * x0;
						pitch = *Pointer<Int>(data + OFFSET(DrawData,depthPitchB));
					}
					else
					{
						buffer = zBuffer + 8 * x0;
					}

					For(Int x = x0, x < x1, x += 2)
					{
						Float4 z = interpolate(xxxx, Dz[0], z, primitive + OFFSET(Primitive,z), false, false, state.depthClamp);

						Float4 zValue;

						if(!state.quadLayoutDepthBuffer)
						{
							// FIXME: Properly optimizes?
							zValue.xy = *Pointer<Float4>(buffer);
							zValue.zw = *Pointer<Float4>(buffer + pitch - 8);
						}
						else
						{
							zValue = *Pointer<Float4>(buffer, 16);
						}

						Int4 zTest;

						if(complementaryDepthBuffer)
						{
							zTest = CmpLE(zValue, z);
						}
						else
						{
							zTest = CmpNLT(zValue, z);
						}

						Int zMask = SignMask(zTest);

						If(zMask == 0)
						{
							x0 += 2;
						}
						Else
						{
							x = x1;
						}

						xxxx += Float4(2);

						if(!state.quadLayoutDepthBuffer)
						{
							buffer += 8;
						}
						else
						{
							buffer += 16;
						}
					}
				}
			}

			If(x0 < x1)
			{
				if(interpolateW())
				{
					Dw = *Pointer<Float4>(primitive + OFFSET(Primitive,w.C), 16) + yyyy * *Pointer<Float4>(primitive + OFFSET(Primitive,w.B), 16);
				}

				for(int interpolant = 0; interpolant < MAX_FRAGMENT_INPUTS; interpolant++)
				{
					for(int component = 0; component < 4; component++)
					{
						if(state.interpolant[interpolant].component & (1 << component))
						{
							Dv[interpolant][component] = *Pointer<Float4>(primitive + OFFSET(Primitive,V[interpolant][component].C), 16);

							if(!(state.interpolant[interpolant].flat & (1 << component)))
							{
								Dv[interpolant][component] += yyyy * *Pointer<Float4>(primitive + OFFSET(Primitive,V[interpolant][component].B), 16);
							}
						}
					}
				}

				if(state.fog.component)
				{
					Df = *Pointer<Float4>(primitive + OFFSET(Primitive,f.C), 16);

					if(!state.fog.flat)
					{
						Df += yyyy * *Pointer<Float4>(primitive + OFFSET(Primitive,f.B), 16);
					}
				}

				Short4 xLeft[4];
				Short4 xRight[4];

				for(unsigned int q = 0; q < state.multiSample; q++)
				{
					xLeft[q] = *Pointer<Short4>(primitive + q * sizeof(Primitive) + OFFSET(Primitive,outline) + y * sizeof(Primitive::Span));
					xRight[q] = xLeft[q];

					xLeft[q] = Swizzle(xLeft[q], 0xA0) - Short4(1, 2, 1, 2);
					xRight[q] = Swizzle(xRight[q], 0xF5) - Short4(0, 1, 0, 1);
				}

				For(Int x = x0, x < x1, x += 2)
				{
					Short4 xxxx = Short4(x);
					Int cMask[4];

					for(unsigned int q = 0; q < state.multiSample; q++)
					{
						Short4 mask = CmpGT(xxxx, xLeft[q]) & CmpGT(xRight[q], xxxx);
						cMask[q] = SignMask(PackSigned(mask, mask)) & 0x0000000F;
					}

					quad(cBuffer, zBuffer, sBuffer, cMask, x, y);
				}
			}

			int clusterCount = Renderer::getClusterCount();

			for(int index = 0; index < RENDERTARGETS; index++)
			{
				if(state.colorWriteActive(index))
				{
					cBuffer[index] += *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index])) << (1 + sw::log2(clusterCount));   // FIXME: Precompute
				}
			}

			if(state.depthTestActive)
			{
				zBuffer += *Pointer<Int>(data + OFFSET(DrawData,depthPitchB)) << (1 + sw::log2(clusterCount));   // FIXME: Precompute
			}

			if(state.stencilActive)
			{
				sBuffer += *Pointer<Int>(data + OFFSET(DrawData,stencilPitchB)) << (1 + sw::log2(clusterCount));   // FIXME: Precompute
			}

			y += 2 * clusterCount;
		}
		Until(y >= yMax)
	}

	Float4 QuadRasterizer::interpolate(Float4 &x, Float4 &D, Float4 &rhw, Pointer<Byte> planeEquation, bool flat, bool perspective, bool clamp)
	{
		Float4 interpolant = D;

		if(!flat)
		{
			interpolant += x * *Pointer<Float4>(planeEquation + OFFSET(PlaneEquation, A), 16);

			if(perspective)
			{
				interpolant *= rhw;
			}
		}

		if(clamp)
		{
			interpolant = Min(Max(interpolant, Float4(0.0f)), Float4(1.0f));
		}

		return interpolant;
	}

	bool QuadRasterizer::interpolateZ() const
	{
		return state.depthTestActive || state.pixelFogActive() || (shader && shader->isVPosDeclared() && fullPixelPositionRegister);
	}

	bool QuadRasterizer::interpolateW() const
	{
		return state.perspective || (shader && shader->isVPosDeclared() && fullPixelPositionRegister);
	}
}
