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

#include "PixelRoutine.hpp"

#include "SamplerCore.hpp"
#include "Constants.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/QuadRasterizer.hpp"
#include "Renderer/Surface.hpp"
#include "Renderer/Primitive.hpp"
#include "Common/Debug.hpp"

namespace sw
{
	extern bool complementaryDepthBuffer;
	extern bool postBlendSRGB;
	extern bool exactColorRounding;
	extern bool forceClearRegisters;

	PixelRoutine::PixelRoutine(const PixelProcessor::State &state, const PixelShader *shader) : QuadRasterizer(state, shader), v(shader && shader->dynamicallyIndexedInput)
	{
		if(!shader || shader->getShaderModel() < 0x0200 || forceClearRegisters)
		{
			for(int i = 0; i < MAX_FRAGMENT_INPUTS; i++)
			{
				v[i].x = Float4(0.0f);
				v[i].y = Float4(0.0f);
				v[i].z = Float4(0.0f);
				v[i].w = Float4(0.0f);
			}
		}
	}

	PixelRoutine::~PixelRoutine()
	{
	}

	void PixelRoutine::quad(Pointer<Byte> cBuffer[RENDERTARGETS], Pointer<Byte> &zBuffer, Pointer<Byte> &sBuffer, Int cMask[4], Int &x, Int &y)
	{
		#if PERF_PROFILE
			Long pipeTime = Ticks();
		#endif

		const bool earlyDepthTest = !state.depthOverride && !state.alphaTestActive();

		Int zMask[4];   // Depth mask
		Int sMask[4];   // Stencil mask

		for(unsigned int q = 0; q < state.multiSample; q++)
		{
			zMask[q] = cMask[q];
			sMask[q] = cMask[q];
		}

		for(unsigned int q = 0; q < state.multiSample; q++)
		{
			stencilTest(sBuffer, q, x, sMask[q], cMask[q]);
		}

		Float4 f;
		Float4 rhwCentroid;

		Float4 xxxx = Float4(Float(x)) + *Pointer<Float4>(primitive + OFFSET(Primitive,xQuad), 16);

		if(interpolateZ())
		{
			for(unsigned int q = 0; q < state.multiSample; q++)
			{
				Float4 x = xxxx;

				if(state.multiSample > 1)
				{
					x -= *Pointer<Float4>(constants + OFFSET(Constants,X) + q * sizeof(float4));
				}

				z[q] = interpolate(x, Dz[q], z[q], primitive + OFFSET(Primitive,z), false, false, state.depthClamp);
			}
		}

		Bool depthPass = false;

		if(earlyDepthTest)
		{
			for(unsigned int q = 0; q < state.multiSample; q++)
			{
				depthPass = depthPass || depthTest(zBuffer, q, x, z[q], sMask[q], zMask[q], cMask[q]);
			}
		}

		If(depthPass || Bool(!earlyDepthTest))
		{
			#if PERF_PROFILE
				Long interpTime = Ticks();
			#endif

			Float4 yyyy = Float4(Float(y)) + *Pointer<Float4>(primitive + OFFSET(Primitive,yQuad), 16);

			// Centroid locations
			Float4 XXXX = Float4(0.0f);
			Float4 YYYY = Float4(0.0f);

			if(state.centroid)
			{
				Float4 WWWW(1.0e-9f);

				for(unsigned int q = 0; q < state.multiSample; q++)
				{
					XXXX += *Pointer<Float4>(constants + OFFSET(Constants,sampleX[q]) + 16 * cMask[q]);
					YYYY += *Pointer<Float4>(constants + OFFSET(Constants,sampleY[q]) + 16 * cMask[q]);
					WWWW += *Pointer<Float4>(constants + OFFSET(Constants,weight) + 16 * cMask[q]);
				}

				WWWW = Rcp_pp(WWWW);
				XXXX *= WWWW;
				YYYY *= WWWW;

				XXXX += xxxx;
				YYYY += yyyy;
			}

			if(interpolateW())
			{
				w = interpolate(xxxx, Dw, rhw, primitive + OFFSET(Primitive,w), false, false, false);
				rhw = reciprocal(w, false, false, true);

				if(state.centroid)
				{
					rhwCentroid = reciprocal(interpolateCentroid(XXXX, YYYY, rhwCentroid, primitive + OFFSET(Primitive,w), false, false));
				}
			}

			for(int interpolant = 0; interpolant < MAX_FRAGMENT_INPUTS; interpolant++)
			{
				for(int component = 0; component < 4; component++)
				{
					if(state.interpolant[interpolant].component & (1 << component))
					{
						if(!state.interpolant[interpolant].centroid)
						{
							v[interpolant][component] = interpolate(xxxx, Dv[interpolant][component], rhw, primitive + OFFSET(Primitive, V[interpolant][component]), (state.interpolant[interpolant].flat & (1 << component)) != 0, state.perspective, false);
						}
						else
						{
							v[interpolant][component] = interpolateCentroid(XXXX, YYYY, rhwCentroid, primitive + OFFSET(Primitive, V[interpolant][component]), (state.interpolant[interpolant].flat & (1 << component)) != 0, state.perspective);
						}
					}
				}

				Float4 rcp;

				switch(state.interpolant[interpolant].project)
				{
				case 0:
					break;
				case 1:
					rcp = reciprocal(v[interpolant].y);
					v[interpolant].x = v[interpolant].x * rcp;
					break;
				case 2:
					rcp = reciprocal(v[interpolant].z);
					v[interpolant].x = v[interpolant].x * rcp;
					v[interpolant].y = v[interpolant].y * rcp;
					break;
				case 3:
					rcp = reciprocal(v[interpolant].w);
					v[interpolant].x = v[interpolant].x * rcp;
					v[interpolant].y = v[interpolant].y * rcp;
					v[interpolant].z = v[interpolant].z * rcp;
					break;
				}
			}

			if(state.fog.component)
			{
				f = interpolate(xxxx, Df, rhw, primitive + OFFSET(Primitive,f), state.fog.flat & 0x01, state.perspective, false);
			}

			setBuiltins(x, y, z, w);

			#if PERF_PROFILE
				cycles[PERF_INTERP] += Ticks() - interpTime;
			#endif

			Bool alphaPass = true;

			if(colorUsed())
			{
				#if PERF_PROFILE
					Long shaderTime = Ticks();
				#endif

				applyShader(cMask);

				#if PERF_PROFILE
					cycles[PERF_SHADER] += Ticks() - shaderTime;
				#endif

				alphaPass = alphaTest(cMask);

				if((shader && shader->containsKill()) || state.alphaTestActive())
				{
					for(unsigned int q = 0; q < state.multiSample; q++)
					{
						zMask[q] &= cMask[q];
						sMask[q] &= cMask[q];
					}
				}
			}

			If(alphaPass)
			{
				if(!earlyDepthTest)
				{
					for(unsigned int q = 0; q < state.multiSample; q++)
					{
						depthPass = depthPass || depthTest(zBuffer, q, x, z[q], sMask[q], zMask[q], cMask[q]);
					}
				}

				#if PERF_PROFILE
					Long ropTime = Ticks();
				#endif

				If(depthPass || Bool(earlyDepthTest))
				{
					for(unsigned int q = 0; q < state.multiSample; q++)
					{
						if(state.multiSampleMask & (1 << q))
						{
							writeDepth(zBuffer, q, x, z[q], zMask[q]);

							if(state.occlusionEnabled)
							{
								occlusion += *Pointer<UInt>(constants + OFFSET(Constants,occlusionCount) + 4 * (zMask[q] & sMask[q]));
							}
						}
					}

					if(colorUsed())
					{
						#if PERF_PROFILE
							AddAtomic(Pointer<Long>(&profiler.ropOperations), 4);
						#endif

						rasterOperation(f, cBuffer, x, sMask, zMask, cMask);
					}
				}

				#if PERF_PROFILE
					cycles[PERF_ROP] += Ticks() - ropTime;
				#endif
			}
		}

		for(unsigned int q = 0; q < state.multiSample; q++)
		{
			if(state.multiSampleMask & (1 << q))
			{
				writeStencil(sBuffer, q, x, sMask[q], zMask[q], cMask[q]);
			}
		}

		#if PERF_PROFILE
			cycles[PERF_PIPE] += Ticks() - pipeTime;
		#endif
	}

	Float4 PixelRoutine::interpolateCentroid(Float4 &x, Float4 &y, Float4 &rhw, Pointer<Byte> planeEquation, bool flat, bool perspective)
	{
		Float4 interpolant = *Pointer<Float4>(planeEquation + OFFSET(PlaneEquation,C), 16);

		if(!flat)
		{
			interpolant += x * *Pointer<Float4>(planeEquation + OFFSET(PlaneEquation,A), 16) +
			               y * *Pointer<Float4>(planeEquation + OFFSET(PlaneEquation,B), 16);

			if(perspective)
			{
				interpolant *= rhw;
			}
		}

		return interpolant;
	}

	void PixelRoutine::stencilTest(Pointer<Byte> &sBuffer, int q, Int &x, Int &sMask, Int &cMask)
	{
		if(!state.stencilActive)
		{
			return;
		}

		// (StencilRef & StencilMask) CompFunc (StencilBufferValue & StencilMask)

		Pointer<Byte> buffer = sBuffer + 2 * x;

		if(q > 0)
		{
			buffer += q * *Pointer<Int>(data + OFFSET(DrawData,stencilSliceB));
		}

		Byte8 value = *Pointer<Byte8>(buffer);
		Byte8 valueCCW = value;

		if(!state.noStencilMask)
		{
			value &= *Pointer<Byte8>(data + OFFSET(DrawData,stencil[0].testMaskQ));
		}

		stencilTest(value, state.stencilCompareMode, false);

		if(state.twoSidedStencil)
		{
			if(!state.noStencilMaskCCW)
			{
				valueCCW &= *Pointer<Byte8>(data + OFFSET(DrawData,stencil[1].testMaskQ));
			}

			stencilTest(valueCCW, state.stencilCompareModeCCW, true);

			value &= *Pointer<Byte8>(primitive + OFFSET(Primitive,clockwiseMask));
			valueCCW &= *Pointer<Byte8>(primitive + OFFSET(Primitive,invClockwiseMask));
			value |= valueCCW;
		}

		sMask = SignMask(value) & cMask;
	}

	void PixelRoutine::stencilTest(Byte8 &value, StencilCompareMode stencilCompareMode, bool CCW)
	{
		Byte8 equal;

		switch(stencilCompareMode)
		{
		case STENCIL_ALWAYS:
			value = Byte8(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
			break;
		case STENCIL_NEVER:
			value = Byte8(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
			break;
		case STENCIL_LESS:			// a < b ~ b > a
			value += Byte8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
			value = CmpGT(As<SByte8>(value), *Pointer<SByte8>(data + OFFSET(DrawData,stencil[CCW].referenceMaskedSignedQ)));
			break;
		case STENCIL_EQUAL:
			value = CmpEQ(value, *Pointer<Byte8>(data + OFFSET(DrawData,stencil[CCW].referenceMaskedQ)));
			break;
		case STENCIL_NOTEQUAL:		// a != b ~ !(a == b)
			value = CmpEQ(value, *Pointer<Byte8>(data + OFFSET(DrawData,stencil[CCW].referenceMaskedQ)));
			value ^= Byte8(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
			break;
		case STENCIL_LESSEQUAL:	// a <= b ~ (b > a) || (a == b)
			equal = value;
			equal = CmpEQ(equal, *Pointer<Byte8>(data + OFFSET(DrawData,stencil[CCW].referenceMaskedQ)));
			value += Byte8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
			value = CmpGT(As<SByte8>(value), *Pointer<SByte8>(data + OFFSET(DrawData,stencil[CCW].referenceMaskedSignedQ)));
			value |= equal;
			break;
		case STENCIL_GREATER:		// a > b
			equal = *Pointer<Byte8>(data + OFFSET(DrawData,stencil[CCW].referenceMaskedSignedQ));
			value += Byte8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
			equal = CmpGT(As<SByte8>(equal), As<SByte8>(value));
			value = equal;
			break;
		case STENCIL_GREATEREQUAL:	// a >= b ~ !(a < b) ~ !(b > a)
			value += Byte8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
			value = CmpGT(As<SByte8>(value), *Pointer<SByte8>(data + OFFSET(DrawData,stencil[CCW].referenceMaskedSignedQ)));
			value ^= Byte8(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
			break;
		default:
			ASSERT(false);
		}
	}

	Bool PixelRoutine::depthTest(Pointer<Byte> &zBuffer, int q, Int &x, Float4 &z, Int &sMask, Int &zMask, Int &cMask)
	{
		if(!state.depthTestActive)
		{
			return true;
		}

		Float4 Z = z;

		if(shader && shader->depthOverride())
		{
			if(complementaryDepthBuffer)
			{
				Z = Float4(1.0f) - oDepth;
			}
			else
			{
				Z = oDepth;
			}
		}

		Pointer<Byte> buffer;
		Int pitch;

		if(!state.quadLayoutDepthBuffer)
		{
			buffer = zBuffer + 4 * x;
			pitch = *Pointer<Int>(data + OFFSET(DrawData,depthPitchB));
		}
		else
		{
			buffer = zBuffer + 8 * x;
		}

		if(q > 0)
		{
			buffer += q * *Pointer<Int>(data + OFFSET(DrawData,depthSliceB));
		}

		Float4 zValue;

		if(state.depthCompareMode != DEPTH_NEVER || (state.depthCompareMode != DEPTH_ALWAYS && !state.depthWriteEnable))
		{
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
		}

		Int4 zTest;

		switch(state.depthCompareMode)
		{
		case DEPTH_ALWAYS:
			// Optimized
			break;
		case DEPTH_NEVER:
			// Optimized
			break;
		case DEPTH_EQUAL:
			zTest = CmpEQ(zValue, Z);
			break;
		case DEPTH_NOTEQUAL:
			zTest = CmpNEQ(zValue, Z);
			break;
		case DEPTH_LESS:
			if(complementaryDepthBuffer)
			{
				zTest = CmpLT(zValue, Z);
			}
			else
			{
				zTest = CmpNLE(zValue, Z);
			}
			break;
		case DEPTH_GREATEREQUAL:
			if(complementaryDepthBuffer)
			{
				zTest = CmpNLT(zValue, Z);
			}
			else
			{
				zTest = CmpLE(zValue, Z);
			}
			break;
		case DEPTH_LESSEQUAL:
			if(complementaryDepthBuffer)
			{
				zTest = CmpLE(zValue, Z);
			}
			else
			{
				zTest = CmpNLT(zValue, Z);
			}
			break;
		case DEPTH_GREATER:
			if(complementaryDepthBuffer)
			{
				zTest = CmpNLE(zValue, Z);
			}
			else
			{
				zTest = CmpLT(zValue, Z);
			}
			break;
		default:
			ASSERT(false);
		}

		switch(state.depthCompareMode)
		{
		case DEPTH_ALWAYS:
			zMask = cMask;
			break;
		case DEPTH_NEVER:
			zMask = 0x0;
			break;
		default:
			zMask = SignMask(zTest) & cMask;
			break;
		}

		if(state.stencilActive)
		{
			zMask &= sMask;
		}

		return zMask != 0;
	}

	void PixelRoutine::alphaTest(Int &aMask, Short4 &alpha)
	{
		Short4 cmp;
		Short4 equal;

		switch(state.alphaCompareMode)
		{
		case ALPHA_ALWAYS:
			aMask = 0xF;
			break;
		case ALPHA_NEVER:
			aMask = 0x0;
			break;
		case ALPHA_EQUAL:
			cmp = CmpEQ(alpha, *Pointer<Short4>(data + OFFSET(DrawData,factor.alphaReference4)));
			aMask = SignMask(PackSigned(cmp, Short4(0x0000)));
			break;
		case ALPHA_NOTEQUAL:       // a != b ~ !(a == b)
			cmp = CmpEQ(alpha, *Pointer<Short4>(data + OFFSET(DrawData,factor.alphaReference4))) ^ Short4(0xFFFFu);   // FIXME
			aMask = SignMask(PackSigned(cmp, Short4(0x0000)));
			break;
		case ALPHA_LESS:           // a < b ~ b > a
			cmp = CmpGT(*Pointer<Short4>(data + OFFSET(DrawData,factor.alphaReference4)), alpha);
			aMask = SignMask(PackSigned(cmp, Short4(0x0000)));
			break;
		case ALPHA_GREATEREQUAL:   // a >= b ~ (a > b) || (a == b) ~ !(b > a)   // TODO: Approximate
			equal = CmpEQ(alpha, *Pointer<Short4>(data + OFFSET(DrawData,factor.alphaReference4)));
			cmp = CmpGT(alpha, *Pointer<Short4>(data + OFFSET(DrawData,factor.alphaReference4)));
			cmp |= equal;
			aMask = SignMask(PackSigned(cmp, Short4(0x0000)));
			break;
		case ALPHA_LESSEQUAL:      // a <= b ~ !(a > b)
			cmp = CmpGT(alpha, *Pointer<Short4>(data + OFFSET(DrawData,factor.alphaReference4))) ^ Short4(0xFFFFu);   // FIXME
			aMask = SignMask(PackSigned(cmp, Short4(0x0000)));
			break;
		case ALPHA_GREATER:        // a > b
			cmp = CmpGT(alpha, *Pointer<Short4>(data + OFFSET(DrawData,factor.alphaReference4)));
			aMask = SignMask(PackSigned(cmp, Short4(0x0000)));
			break;
		default:
			ASSERT(false);
		}
	}

	void PixelRoutine::alphaToCoverage(Int cMask[4], Float4 &alpha)
	{
		Int4 coverage0 = CmpNLT(alpha, *Pointer<Float4>(data + OFFSET(DrawData,a2c0)));
		Int4 coverage1 = CmpNLT(alpha, *Pointer<Float4>(data + OFFSET(DrawData,a2c1)));
		Int4 coverage2 = CmpNLT(alpha, *Pointer<Float4>(data + OFFSET(DrawData,a2c2)));
		Int4 coverage3 = CmpNLT(alpha, *Pointer<Float4>(data + OFFSET(DrawData,a2c3)));

		Int aMask0 = SignMask(coverage0);
		Int aMask1 = SignMask(coverage1);
		Int aMask2 = SignMask(coverage2);
		Int aMask3 = SignMask(coverage3);

		cMask[0] &= aMask0;
		cMask[1] &= aMask1;
		cMask[2] &= aMask2;
		cMask[3] &= aMask3;
	}

	void PixelRoutine::fogBlend(Vector4f &c0, Float4 &fog)
	{
		if(!state.fogActive)
		{
			return;
		}

		if(state.pixelFogMode != FOG_NONE)
		{
			pixelFog(fog);

			fog = Min(fog, Float4(1.0f));
			fog = Max(fog, Float4(0.0f));
		}

		c0.x -= *Pointer<Float4>(data + OFFSET(DrawData,fog.colorF[0]));
		c0.y -= *Pointer<Float4>(data + OFFSET(DrawData,fog.colorF[1]));
		c0.z -= *Pointer<Float4>(data + OFFSET(DrawData,fog.colorF[2]));

		c0.x *= fog;
		c0.y *= fog;
		c0.z *= fog;

		c0.x += *Pointer<Float4>(data + OFFSET(DrawData,fog.colorF[0]));
		c0.y += *Pointer<Float4>(data + OFFSET(DrawData,fog.colorF[1]));
		c0.z += *Pointer<Float4>(data + OFFSET(DrawData,fog.colorF[2]));
	}

	void PixelRoutine::pixelFog(Float4 &visibility)
	{
		Float4 &zw = visibility;

		if(state.pixelFogMode != FOG_NONE)
		{
			if(state.wBasedFog)
			{
				zw = rhw;
			}
			else
			{
				if(complementaryDepthBuffer)
				{
					zw = Float4(1.0f) - z[0];
				}
				else
				{
					zw = z[0];
				}
			}
		}

		switch(state.pixelFogMode)
		{
		case FOG_NONE:
			break;
		case FOG_LINEAR:
			zw *= *Pointer<Float4>(data + OFFSET(DrawData,fog.scale));
			zw += *Pointer<Float4>(data + OFFSET(DrawData,fog.offset));
			break;
		case FOG_EXP:
			zw *= *Pointer<Float4>(data + OFFSET(DrawData,fog.densityE));
			zw = exponential2(zw, true);
			break;
		case FOG_EXP2:
			zw *= zw;
			zw *= *Pointer<Float4>(data + OFFSET(DrawData,fog.density2E));
			zw = exponential2(zw, true);
			break;
		default:
			ASSERT(false);
		}
	}

	void PixelRoutine::writeDepth(Pointer<Byte> &zBuffer, int q, Int &x, Float4 &z, Int &zMask)
	{
		if(!state.depthWriteEnable)
		{
			return;
		}

		Float4 Z = z;

		if(shader && shader->depthOverride())
		{
			if(complementaryDepthBuffer)
			{
				Z = Float4(1.0f) - oDepth;
			}
			else
			{
				Z = oDepth;
			}
		}

		Pointer<Byte> buffer;
		Int pitch;

		if(!state.quadLayoutDepthBuffer)
		{
			buffer = zBuffer + 4 * x;
			pitch = *Pointer<Int>(data + OFFSET(DrawData,depthPitchB));
		}
		else
		{
			buffer = zBuffer + 8 * x;
		}

		if(q > 0)
		{
			buffer += q * *Pointer<Int>(data + OFFSET(DrawData,depthSliceB));
		}

		Float4 zValue;

		if(state.depthCompareMode != DEPTH_NEVER || (state.depthCompareMode != DEPTH_ALWAYS && !state.depthWriteEnable))
		{
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
		}

		Z = As<Float4>(As<Int4>(Z) & *Pointer<Int4>(constants + OFFSET(Constants,maskD4X) + zMask * 16, 16));
		zValue = As<Float4>(As<Int4>(zValue) & *Pointer<Int4>(constants + OFFSET(Constants,invMaskD4X) + zMask * 16, 16));
		Z = As<Float4>(As<Int4>(Z) | As<Int4>(zValue));

		if(!state.quadLayoutDepthBuffer)
		{
			// FIXME: Properly optimizes?
			*Pointer<Float2>(buffer) = Float2(Z.xy);
			*Pointer<Float2>(buffer + pitch) = Float2(Z.zw);
		}
		else
		{
			*Pointer<Float4>(buffer, 16) = Z;
		}
	}

	void PixelRoutine::writeStencil(Pointer<Byte> &sBuffer, int q, Int &x, Int &sMask, Int &zMask, Int &cMask)
	{
		if(!state.stencilActive)
		{
			return;
		}

		if(state.stencilPassOperation == OPERATION_KEEP && state.stencilZFailOperation == OPERATION_KEEP && state.stencilFailOperation == OPERATION_KEEP)
		{
			if(!state.twoSidedStencil || (state.stencilPassOperationCCW == OPERATION_KEEP && state.stencilZFailOperationCCW == OPERATION_KEEP && state.stencilFailOperationCCW == OPERATION_KEEP))
			{
				return;
			}
		}

		if(state.stencilWriteMasked && (!state.twoSidedStencil || state.stencilWriteMaskedCCW))
		{
			return;
		}

		Pointer<Byte> buffer = sBuffer + 2 * x;

		if(q > 0)
		{
			buffer += q * *Pointer<Int>(data + OFFSET(DrawData,stencilSliceB));
		}

		Byte8 bufferValue = *Pointer<Byte8>(buffer);

		Byte8 newValue;
		stencilOperation(newValue, bufferValue, state.stencilPassOperation, state.stencilZFailOperation, state.stencilFailOperation, false, zMask, sMask);

		if(!state.noStencilWriteMask)
		{
			Byte8 maskedValue = bufferValue;
			newValue &= *Pointer<Byte8>(data + OFFSET(DrawData,stencil[0].writeMaskQ));
			maskedValue &= *Pointer<Byte8>(data + OFFSET(DrawData,stencil[0].invWriteMaskQ));
			newValue |= maskedValue;
		}

		if(state.twoSidedStencil)
		{
			Byte8 newValueCCW;

			stencilOperation(newValueCCW, bufferValue, state.stencilPassOperationCCW, state.stencilZFailOperationCCW, state.stencilFailOperationCCW, true, zMask, sMask);

			if(!state.noStencilWriteMaskCCW)
			{
				Byte8 maskedValue = bufferValue;
				newValueCCW &= *Pointer<Byte8>(data + OFFSET(DrawData,stencil[1].writeMaskQ));
				maskedValue &= *Pointer<Byte8>(data + OFFSET(DrawData,stencil[1].invWriteMaskQ));
				newValueCCW |= maskedValue;
			}

			newValue &= *Pointer<Byte8>(primitive + OFFSET(Primitive,clockwiseMask));
			newValueCCW &= *Pointer<Byte8>(primitive + OFFSET(Primitive,invClockwiseMask));
			newValue |= newValueCCW;
		}

		newValue &= *Pointer<Byte8>(constants + OFFSET(Constants,maskB4Q) + 8 * cMask);
		bufferValue &= *Pointer<Byte8>(constants + OFFSET(Constants,invMaskB4Q) + 8 * cMask);
		newValue |= bufferValue;

		*Pointer<Byte4>(buffer) = Byte4(newValue);
	}

	void PixelRoutine::stencilOperation(Byte8 &newValue, Byte8 &bufferValue, StencilOperation stencilPassOperation, StencilOperation stencilZFailOperation, StencilOperation stencilFailOperation, bool CCW, Int &zMask, Int &sMask)
	{
		Byte8 &pass = newValue;
		Byte8 fail;
		Byte8 zFail;

		stencilOperation(pass, bufferValue, stencilPassOperation, CCW);

		if(stencilZFailOperation != stencilPassOperation)
		{
			stencilOperation(zFail, bufferValue, stencilZFailOperation, CCW);
		}

		if(stencilFailOperation != stencilPassOperation || stencilFailOperation != stencilZFailOperation)
		{
			stencilOperation(fail, bufferValue, stencilFailOperation, CCW);
		}

		if(stencilFailOperation != stencilPassOperation || stencilFailOperation != stencilZFailOperation)
		{
			if(state.depthTestActive && stencilZFailOperation != stencilPassOperation)   // zMask valid and values not the same
			{
				pass &= *Pointer<Byte8>(constants + OFFSET(Constants,maskB4Q) + 8 * zMask);
				zFail &= *Pointer<Byte8>(constants + OFFSET(Constants,invMaskB4Q) + 8 * zMask);
				pass |= zFail;
			}

			pass &= *Pointer<Byte8>(constants + OFFSET(Constants,maskB4Q) + 8 * sMask);
			fail &= *Pointer<Byte8>(constants + OFFSET(Constants,invMaskB4Q) + 8 * sMask);
			pass |= fail;
		}
	}

	void PixelRoutine::stencilOperation(Byte8 &output, Byte8 &bufferValue, StencilOperation operation, bool CCW)
	{
		switch(operation)
		{
		case OPERATION_KEEP:
			output = bufferValue;
			break;
		case OPERATION_ZERO:
			output = Byte8(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
			break;
		case OPERATION_REPLACE:
			output = *Pointer<Byte8>(data + OFFSET(DrawData,stencil[CCW].referenceQ));
			break;
		case OPERATION_INCRSAT:
			output = AddSat(bufferValue, Byte8(1, 1, 1, 1, 1, 1, 1, 1));
			break;
		case OPERATION_DECRSAT:
			output = SubSat(bufferValue, Byte8(1, 1, 1, 1, 1, 1, 1, 1));
			break;
		case OPERATION_INVERT:
			output = bufferValue ^ Byte8(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
			break;
		case OPERATION_INCR:
			output = bufferValue + Byte8(1, 1, 1, 1, 1, 1, 1, 1);
			break;
		case OPERATION_DECR:
			output = bufferValue - Byte8(1, 1, 1, 1, 1, 1, 1, 1);
			break;
		default:
			ASSERT(false);
		}
	}

	void PixelRoutine::blendFactor(Vector4s &blendFactor, const Vector4s &current, const Vector4s &pixel, BlendFactor blendFactorActive)
	{
		switch(blendFactorActive)
		{
		case BLEND_ZERO:
			// Optimized
			break;
		case BLEND_ONE:
			// Optimized
			break;
		case BLEND_SOURCE:
			blendFactor.x = current.x;
			blendFactor.y = current.y;
			blendFactor.z = current.z;
			break;
		case BLEND_INVSOURCE:
			blendFactor.x = Short4(0xFFFFu) - current.x;
			blendFactor.y = Short4(0xFFFFu) - current.y;
			blendFactor.z = Short4(0xFFFFu) - current.z;
			break;
		case BLEND_DEST:
			blendFactor.x = pixel.x;
			blendFactor.y = pixel.y;
			blendFactor.z = pixel.z;
			break;
		case BLEND_INVDEST:
			blendFactor.x = Short4(0xFFFFu) - pixel.x;
			blendFactor.y = Short4(0xFFFFu) - pixel.y;
			blendFactor.z = Short4(0xFFFFu) - pixel.z;
			break;
		case BLEND_SOURCEALPHA:
			blendFactor.x = current.w;
			blendFactor.y = current.w;
			blendFactor.z = current.w;
			break;
		case BLEND_INVSOURCEALPHA:
			blendFactor.x = Short4(0xFFFFu) - current.w;
			blendFactor.y = Short4(0xFFFFu) - current.w;
			blendFactor.z = Short4(0xFFFFu) - current.w;
			break;
		case BLEND_DESTALPHA:
			blendFactor.x = pixel.w;
			blendFactor.y = pixel.w;
			blendFactor.z = pixel.w;
			break;
		case BLEND_INVDESTALPHA:
			blendFactor.x = Short4(0xFFFFu) - pixel.w;
			blendFactor.y = Short4(0xFFFFu) - pixel.w;
			blendFactor.z = Short4(0xFFFFu) - pixel.w;
			break;
		case BLEND_SRCALPHASAT:
			blendFactor.x = Short4(0xFFFFu) - pixel.w;
			blendFactor.x = Min(As<UShort4>(blendFactor.x), As<UShort4>(current.w));
			blendFactor.y = blendFactor.x;
			blendFactor.z = blendFactor.x;
			break;
		case BLEND_CONSTANT:
			blendFactor.x = *Pointer<Short4>(data + OFFSET(DrawData,factor.blendConstant4W[0]));
			blendFactor.y = *Pointer<Short4>(data + OFFSET(DrawData,factor.blendConstant4W[1]));
			blendFactor.z = *Pointer<Short4>(data + OFFSET(DrawData,factor.blendConstant4W[2]));
			break;
		case BLEND_INVCONSTANT:
			blendFactor.x = *Pointer<Short4>(data + OFFSET(DrawData,factor.invBlendConstant4W[0]));
			blendFactor.y = *Pointer<Short4>(data + OFFSET(DrawData,factor.invBlendConstant4W[1]));
			blendFactor.z = *Pointer<Short4>(data + OFFSET(DrawData,factor.invBlendConstant4W[2]));
			break;
		case BLEND_CONSTANTALPHA:
			blendFactor.x = *Pointer<Short4>(data + OFFSET(DrawData,factor.blendConstant4W[3]));
			blendFactor.y = *Pointer<Short4>(data + OFFSET(DrawData,factor.blendConstant4W[3]));
			blendFactor.z = *Pointer<Short4>(data + OFFSET(DrawData,factor.blendConstant4W[3]));
			break;
		case BLEND_INVCONSTANTALPHA:
			blendFactor.x = *Pointer<Short4>(data + OFFSET(DrawData,factor.invBlendConstant4W[3]));
			blendFactor.y = *Pointer<Short4>(data + OFFSET(DrawData,factor.invBlendConstant4W[3]));
			blendFactor.z = *Pointer<Short4>(data + OFFSET(DrawData,factor.invBlendConstant4W[3]));
			break;
		default:
			ASSERT(false);
		}
	}

	void PixelRoutine::blendFactorAlpha(Vector4s &blendFactor, const Vector4s &current, const Vector4s &pixel, BlendFactor blendFactorAlphaActive)
	{
		switch(blendFactorAlphaActive)
		{
		case BLEND_ZERO:
			// Optimized
			break;
		case BLEND_ONE:
			// Optimized
			break;
		case BLEND_SOURCE:
			blendFactor.w = current.w;
			break;
		case BLEND_INVSOURCE:
			blendFactor.w = Short4(0xFFFFu) - current.w;
			break;
		case BLEND_DEST:
			blendFactor.w = pixel.w;
			break;
		case BLEND_INVDEST:
			blendFactor.w = Short4(0xFFFFu) - pixel.w;
			break;
		case BLEND_SOURCEALPHA:
			blendFactor.w = current.w;
			break;
		case BLEND_INVSOURCEALPHA:
			blendFactor.w = Short4(0xFFFFu) - current.w;
			break;
		case BLEND_DESTALPHA:
			blendFactor.w = pixel.w;
			break;
		case BLEND_INVDESTALPHA:
			blendFactor.w = Short4(0xFFFFu) - pixel.w;
			break;
		case BLEND_SRCALPHASAT:
			blendFactor.w = Short4(0xFFFFu);
			break;
		case BLEND_CONSTANT:
		case BLEND_CONSTANTALPHA:
			blendFactor.w = *Pointer<Short4>(data + OFFSET(DrawData,factor.blendConstant4W[3]));
			break;
		case BLEND_INVCONSTANT:
		case BLEND_INVCONSTANTALPHA:
			blendFactor.w = *Pointer<Short4>(data + OFFSET(DrawData,factor.invBlendConstant4W[3]));
			break;
		default:
			ASSERT(false);
		}
	}

	bool PixelRoutine::isSRGB(int index) const
	{
		return Surface::isSRGBformat(state.targetFormat[index]);
	}

	void PixelRoutine::readPixel(int index, Pointer<Byte> &cBuffer, Int &x, Vector4s &pixel)
	{
		Short4 c01;
		Short4 c23;
		Pointer<Byte> buffer;
		Pointer<Byte> buffer2;

		switch(state.targetFormat[index])
		{
		case FORMAT_R5G6B5:
			buffer = cBuffer + 2 * x;
			buffer2 = buffer + *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
			c01 = As<Short4>(Int2(*Pointer<Int>(buffer), *Pointer<Int>(buffer2)));

			pixel.x = c01 & Short4(0xF800u);
			pixel.y = (c01 & Short4(0x07E0u)) << 5;
			pixel.z = (c01 & Short4(0x001Fu)) << 11;
			pixel.w = Short4(0xFFFFu);
			break;
		case FORMAT_A8R8G8B8:
			buffer = cBuffer + 4 * x;
			c01 = *Pointer<Short4>(buffer);
			buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
			c23 = *Pointer<Short4>(buffer);
			pixel.z = c01;
			pixel.y = c01;
			pixel.z = UnpackLow(As<Byte8>(pixel.z), As<Byte8>(c23));
			pixel.y = UnpackHigh(As<Byte8>(pixel.y), As<Byte8>(c23));
			pixel.x = pixel.z;
			pixel.z = UnpackLow(As<Byte8>(pixel.z), As<Byte8>(pixel.y));
			pixel.x = UnpackHigh(As<Byte8>(pixel.x), As<Byte8>(pixel.y));
			pixel.y = pixel.z;
			pixel.w = pixel.x;
			pixel.x = UnpackLow(As<Byte8>(pixel.x), As<Byte8>(pixel.x));
			pixel.y = UnpackHigh(As<Byte8>(pixel.y), As<Byte8>(pixel.y));
			pixel.z = UnpackLow(As<Byte8>(pixel.z), As<Byte8>(pixel.z));
			pixel.w = UnpackHigh(As<Byte8>(pixel.w), As<Byte8>(pixel.w));
			break;
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_A8:
			buffer = cBuffer + 4 * x;
			c01 = *Pointer<Short4>(buffer);
			buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
			c23 = *Pointer<Short4>(buffer);
			pixel.z = c01;
			pixel.y = c01;
			pixel.z = UnpackLow(As<Byte8>(pixel.z), As<Byte8>(c23));
			pixel.y = UnpackHigh(As<Byte8>(pixel.y), As<Byte8>(c23));
			pixel.x = pixel.z;
			pixel.z = UnpackLow(As<Byte8>(pixel.z), As<Byte8>(pixel.y));
			pixel.x = UnpackHigh(As<Byte8>(pixel.x), As<Byte8>(pixel.y));
			pixel.y = pixel.z;
			pixel.w = pixel.x;
			pixel.x = UnpackLow(As<Byte8>(pixel.z), As<Byte8>(pixel.z));
			pixel.y = UnpackHigh(As<Byte8>(pixel.y), As<Byte8>(pixel.y));
			pixel.z = UnpackLow(As<Byte8>(pixel.w), As<Byte8>(pixel.w));
			pixel.w = UnpackHigh(As<Byte8>(pixel.w), As<Byte8>(pixel.w));
			break;
		case FORMAT_A8:
			buffer = cBuffer + 1 * x;
			pixel.w = Insert(pixel.w, *Pointer<Short>(buffer), 0);
			buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
			pixel.w = Insert(pixel.w, *Pointer<Short>(buffer), 1);
			pixel.w = UnpackLow(As<Byte8>(pixel.w), As<Byte8>(pixel.w));
			pixel.x = Short4(0x0000);
			pixel.y = Short4(0x0000);
			pixel.z = Short4(0x0000);
			break;
		case FORMAT_R8:
			buffer = cBuffer + 1 * x;
			pixel.x = Insert(pixel.x, *Pointer<Short>(buffer), 0);
			buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
			pixel.x = Insert(pixel.x, *Pointer<Short>(buffer), 1);
			pixel.x = UnpackLow(As<Byte8>(pixel.x), As<Byte8>(pixel.x));
			pixel.y = Short4(0x0000);
			pixel.z = Short4(0x0000);
			pixel.w = Short4(0xFFFFu);
			break;
		case FORMAT_X8R8G8B8:
			buffer = cBuffer + 4 * x;
			c01 = *Pointer<Short4>(buffer);
			buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
			c23 = *Pointer<Short4>(buffer);
			pixel.z = c01;
			pixel.y = c01;
			pixel.z = UnpackLow(As<Byte8>(pixel.z), As<Byte8>(c23));
			pixel.y = UnpackHigh(As<Byte8>(pixel.y), As<Byte8>(c23));
			pixel.x = pixel.z;
			pixel.z = UnpackLow(As<Byte8>(pixel.z), As<Byte8>(pixel.y));
			pixel.x = UnpackHigh(As<Byte8>(pixel.x), As<Byte8>(pixel.y));
			pixel.y = pixel.z;
			pixel.x = UnpackLow(As<Byte8>(pixel.x), As<Byte8>(pixel.x));
			pixel.y = UnpackHigh(As<Byte8>(pixel.y), As<Byte8>(pixel.y));
			pixel.z = UnpackLow(As<Byte8>(pixel.z), As<Byte8>(pixel.z));
			pixel.w = Short4(0xFFFFu);
			break;
		case FORMAT_G8R8:
			buffer = cBuffer + 2 * x;
			c01 = As<Short4>(Insert(As<Int2>(c01), *Pointer<Int>(buffer), 0));
			buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
			c01 = As<Short4>(Insert(As<Int2>(c01), *Pointer<Int>(buffer), 1));
			pixel.x = (c01 & Short4(0x00FFu)) | (c01 << 8);
			pixel.y = (c01 & Short4(0xFF00u)) | As<Short4>(As<UShort4>(c01) >> 8);
			pixel.z = Short4(0x0000u);
			pixel.w = Short4(0xFFFFu);
			break;
		case FORMAT_X8B8G8R8:
		case FORMAT_SRGB8_X8:
			buffer = cBuffer + 4 * x;
			c01 = *Pointer<Short4>(buffer);
			buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
			c23 = *Pointer<Short4>(buffer);
			pixel.z = c01;
			pixel.y = c01;
			pixel.z = UnpackLow(As<Byte8>(pixel.z), As<Byte8>(c23));
			pixel.y = UnpackHigh(As<Byte8>(pixel.y), As<Byte8>(c23));
			pixel.x = pixel.z;
			pixel.z = UnpackLow(As<Byte8>(pixel.z), As<Byte8>(pixel.y));
			pixel.x = UnpackHigh(As<Byte8>(pixel.x), As<Byte8>(pixel.y));
			pixel.y = pixel.z;
			pixel.w = pixel.x;
			pixel.x = UnpackLow(As<Byte8>(pixel.z), As<Byte8>(pixel.z));
			pixel.y = UnpackHigh(As<Byte8>(pixel.y), As<Byte8>(pixel.y));
			pixel.z = UnpackLow(As<Byte8>(pixel.w), As<Byte8>(pixel.w));
			pixel.w = Short4(0xFFFFu);
			break;
		case FORMAT_A8G8R8B8Q:
			UNIMPLEMENTED();
		//	pixel.z = UnpackLow(As<Byte8>(pixel.z), *Pointer<Byte8>(cBuffer + 8 * x + 0));
		//	pixel.x = UnpackHigh(As<Byte8>(pixel.x), *Pointer<Byte8>(cBuffer + 8 * x + 0));
		//	pixel.y = UnpackLow(As<Byte8>(pixel.y), *Pointer<Byte8>(cBuffer + 8 * x + 8));
		//	pixel.w = UnpackHigh(As<Byte8>(pixel.w), *Pointer<Byte8>(cBuffer + 8 * x + 8));
			break;
		case FORMAT_X8G8R8B8Q:
			UNIMPLEMENTED();
		//	pixel.z = UnpackLow(As<Byte8>(pixel.z), *Pointer<Byte8>(cBuffer + 8 * x + 0));
		//	pixel.x = UnpackHigh(As<Byte8>(pixel.x), *Pointer<Byte8>(cBuffer + 8 * x + 0));
		//	pixel.y = UnpackLow(As<Byte8>(pixel.y), *Pointer<Byte8>(cBuffer + 8 * x + 8));
		//	pixel.w = Short4(0xFFFFu);
			break;
		case FORMAT_A16B16G16R16:
			buffer = cBuffer;
			pixel.x = *Pointer<Short4>(buffer + 8 * x);
			pixel.y = *Pointer<Short4>(buffer + 8 * x + 8);
			buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
			pixel.z = *Pointer<Short4>(buffer + 8 * x);
			pixel.w = *Pointer<Short4>(buffer + 8 * x + 8);
			transpose4x4(pixel.x, pixel.y, pixel.z, pixel.w);
			break;
		case FORMAT_G16R16:
			buffer = cBuffer;
			pixel.x = *Pointer<Short4>(buffer + 4 * x);
			buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
			pixel.y = *Pointer<Short4>(buffer + 4 * x);
			pixel.z = pixel.x;
			pixel.x = As<Short4>(UnpackLow(pixel.x, pixel.y));
			pixel.z = As<Short4>(UnpackHigh(pixel.z, pixel.y));
			pixel.y = pixel.z;
			pixel.x = As<Short4>(UnpackLow(pixel.x, pixel.z));
			pixel.y = As<Short4>(UnpackHigh(pixel.y, pixel.z));
			pixel.z = Short4(0xFFFFu);
			pixel.w = Short4(0xFFFFu);
			break;
		default:
			ASSERT(false);
		}

		if((postBlendSRGB && state.writeSRGB) || isSRGB(index))
		{
			sRGBtoLinear16_12_16(pixel);
		}
	}

	void PixelRoutine::alphaBlend(int index, Pointer<Byte> &cBuffer, Vector4s &current, Int &x)
	{
		if(!state.alphaBlendActive)
		{
			return;
		}

		Vector4s pixel;
		readPixel(index, cBuffer, x, pixel);

		// Final Color = ObjectColor * SourceBlendFactor + PixelColor * DestinationBlendFactor
		Vector4s sourceFactor;
		Vector4s destFactor;

		blendFactor(sourceFactor, current, pixel, state.sourceBlendFactor);
		blendFactor(destFactor, current, pixel, state.destBlendFactor);

		if(state.sourceBlendFactor != BLEND_ONE && state.sourceBlendFactor != BLEND_ZERO)
		{
			current.x = MulHigh(As<UShort4>(current.x), As<UShort4>(sourceFactor.x));
			current.y = MulHigh(As<UShort4>(current.y), As<UShort4>(sourceFactor.y));
			current.z = MulHigh(As<UShort4>(current.z), As<UShort4>(sourceFactor.z));
		}

		if(state.destBlendFactor != BLEND_ONE && state.destBlendFactor != BLEND_ZERO)
		{
			pixel.x = MulHigh(As<UShort4>(pixel.x), As<UShort4>(destFactor.x));
			pixel.y = MulHigh(As<UShort4>(pixel.y), As<UShort4>(destFactor.y));
			pixel.z = MulHigh(As<UShort4>(pixel.z), As<UShort4>(destFactor.z));
		}

		switch(state.blendOperation)
		{
		case BLENDOP_ADD:
			current.x = AddSat(As<UShort4>(current.x), As<UShort4>(pixel.x));
			current.y = AddSat(As<UShort4>(current.y), As<UShort4>(pixel.y));
			current.z = AddSat(As<UShort4>(current.z), As<UShort4>(pixel.z));
			break;
		case BLENDOP_SUB:
			current.x = SubSat(As<UShort4>(current.x), As<UShort4>(pixel.x));
			current.y = SubSat(As<UShort4>(current.y), As<UShort4>(pixel.y));
			current.z = SubSat(As<UShort4>(current.z), As<UShort4>(pixel.z));
			break;
		case BLENDOP_INVSUB:
			current.x = SubSat(As<UShort4>(pixel.x), As<UShort4>(current.x));
			current.y = SubSat(As<UShort4>(pixel.y), As<UShort4>(current.y));
			current.z = SubSat(As<UShort4>(pixel.z), As<UShort4>(current.z));
			break;
		case BLENDOP_MIN:
			current.x = Min(As<UShort4>(current.x), As<UShort4>(pixel.x));
			current.y = Min(As<UShort4>(current.y), As<UShort4>(pixel.y));
			current.z = Min(As<UShort4>(current.z), As<UShort4>(pixel.z));
			break;
		case BLENDOP_MAX:
			current.x = Max(As<UShort4>(current.x), As<UShort4>(pixel.x));
			current.y = Max(As<UShort4>(current.y), As<UShort4>(pixel.y));
			current.z = Max(As<UShort4>(current.z), As<UShort4>(pixel.z));
			break;
		case BLENDOP_SOURCE:
			// No operation
			break;
		case BLENDOP_DEST:
			current.x = pixel.x;
			current.y = pixel.y;
			current.z = pixel.z;
			break;
		case BLENDOP_NULL:
			current.x = Short4(0x0000);
			current.y = Short4(0x0000);
			current.z = Short4(0x0000);
			break;
		default:
			ASSERT(false);
		}

		blendFactorAlpha(sourceFactor, current, pixel, state.sourceBlendFactorAlpha);
		blendFactorAlpha(destFactor, current, pixel, state.destBlendFactorAlpha);

		if(state.sourceBlendFactorAlpha != BLEND_ONE && state.sourceBlendFactorAlpha != BLEND_ZERO)
		{
			current.w = MulHigh(As<UShort4>(current.w), As<UShort4>(sourceFactor.w));
		}

		if(state.destBlendFactorAlpha != BLEND_ONE && state.destBlendFactorAlpha != BLEND_ZERO)
		{
			pixel.w = MulHigh(As<UShort4>(pixel.w), As<UShort4>(destFactor.w));
		}

		switch(state.blendOperationAlpha)
		{
		case BLENDOP_ADD:
			current.w = AddSat(As<UShort4>(current.w), As<UShort4>(pixel.w));
			break;
		case BLENDOP_SUB:
			current.w = SubSat(As<UShort4>(current.w), As<UShort4>(pixel.w));
			break;
		case BLENDOP_INVSUB:
			current.w = SubSat(As<UShort4>(pixel.w), As<UShort4>(current.w));
			break;
		case BLENDOP_MIN:
			current.w = Min(As<UShort4>(current.w), As<UShort4>(pixel.w));
			break;
		case BLENDOP_MAX:
			current.w = Max(As<UShort4>(current.w), As<UShort4>(pixel.w));
			break;
		case BLENDOP_SOURCE:
			// No operation
			break;
		case BLENDOP_DEST:
			current.w = pixel.w;
			break;
		case BLENDOP_NULL:
			current.w = Short4(0x0000);
			break;
		default:
			ASSERT(false);
		}
	}

	void PixelRoutine::logicOperation(int index, Pointer<Byte> &cBuffer, Vector4s &current, Int &x)
	{
		if(state.logicalOperation == LOGICALOP_COPY)
		{
			return;
		}

		Vector4s pixel;
		readPixel(index, cBuffer, x, pixel);

		switch(state.logicalOperation)
		{
		case LOGICALOP_CLEAR:
			current.x = UShort4(0);
			current.y = UShort4(0);
			current.z = UShort4(0);
			break;
		case LOGICALOP_SET:
			current.x = UShort4(0xFFFFu);
			current.y = UShort4(0xFFFFu);
			current.z = UShort4(0xFFFFu);
			break;
		case LOGICALOP_COPY:
			ASSERT(false);   // Optimized out
			break;
		case LOGICALOP_COPY_INVERTED:
			current.x = ~current.x;
			current.y = ~current.y;
			current.z = ~current.z;
			break;
		case LOGICALOP_NOOP:
			current.x = pixel.x;
			current.y = pixel.y;
			current.z = pixel.z;
			break;
		case LOGICALOP_INVERT:
			current.x = ~pixel.x;
			current.y = ~pixel.y;
			current.z = ~pixel.z;
			break;
		case LOGICALOP_AND:
			current.x = pixel.x & current.x;
			current.y = pixel.y & current.y;
			current.z = pixel.z & current.z;
			break;
		case LOGICALOP_NAND:
			current.x = ~(pixel.x & current.x);
			current.y = ~(pixel.y & current.y);
			current.z = ~(pixel.z & current.z);
			break;
		case LOGICALOP_OR:
			current.x = pixel.x | current.x;
			current.y = pixel.y | current.y;
			current.z = pixel.z | current.z;
			break;
		case LOGICALOP_NOR:
			current.x = ~(pixel.x | current.x);
			current.y = ~(pixel.y | current.y);
			current.z = ~(pixel.z | current.z);
			break;
		case LOGICALOP_XOR:
			current.x = pixel.x ^ current.x;
			current.y = pixel.y ^ current.y;
			current.z = pixel.z ^ current.z;
			break;
		case LOGICALOP_EQUIV:
			current.x = ~(pixel.x ^ current.x);
			current.y = ~(pixel.y ^ current.y);
			current.z = ~(pixel.z ^ current.z);
			break;
		case LOGICALOP_AND_REVERSE:
			current.x = ~pixel.x & current.x;
			current.y = ~pixel.y & current.y;
			current.z = ~pixel.z & current.z;
			break;
		case LOGICALOP_AND_INVERTED:
			current.x = pixel.x & ~current.x;
			current.y = pixel.y & ~current.y;
			current.z = pixel.z & ~current.z;
			break;
		case LOGICALOP_OR_REVERSE:
			current.x = ~pixel.x | current.x;
			current.y = ~pixel.y | current.y;
			current.z = ~pixel.z | current.z;
			break;
		case LOGICALOP_OR_INVERTED:
			current.x = pixel.x | ~current.x;
			current.y = pixel.y | ~current.y;
			current.z = pixel.z | ~current.z;
			break;
		default:
			ASSERT(false);
		}
	}

	void PixelRoutine::writeColor(int index, Pointer<Byte> &cBuffer, Int &x, Vector4s &current, Int &sMask, Int &zMask, Int &cMask)
	{
		if((postBlendSRGB && state.writeSRGB) || isSRGB(index))
		{
			linearToSRGB16_12_16(current);
		}

		if(exactColorRounding)
		{
			switch(state.targetFormat[index])
			{
			case FORMAT_R5G6B5:
				current.x = AddSat(As<UShort4>(current.x), UShort4(0x0400));
				current.y = AddSat(As<UShort4>(current.y), UShort4(0x0200));
				current.z = AddSat(As<UShort4>(current.z), UShort4(0x0400));
				break;
			case FORMAT_X8G8R8B8Q:
			case FORMAT_A8G8R8B8Q:
			case FORMAT_X8R8G8B8:
			case FORMAT_X8B8G8R8:
			case FORMAT_A8R8G8B8:
			case FORMAT_A8B8G8R8:
			case FORMAT_SRGB8_X8:
			case FORMAT_SRGB8_A8:
			case FORMAT_G8R8:
			case FORMAT_R8:
				current.x = current.x - As<Short4>(As<UShort4>(current.x) >> 8) + Short4(0x0080);
				current.y = current.y - As<Short4>(As<UShort4>(current.y) >> 8) + Short4(0x0080);
				current.z = current.z - As<Short4>(As<UShort4>(current.z) >> 8) + Short4(0x0080);
				current.w = current.w - As<Short4>(As<UShort4>(current.w) >> 8) + Short4(0x0080);
				break;
			default:
				break;
			}
		}

		int rgbaWriteMask = state.colorWriteActive(index);
		int bgraWriteMask = (rgbaWriteMask & 0x0000000A) | (rgbaWriteMask & 0x00000001) << 2 | (rgbaWriteMask & 0x00000004) >> 2;

		switch(state.targetFormat[index])
		{
		case FORMAT_R5G6B5:
			{
				current.x = current.x & Short4(0xF800u);
				current.y = As<UShort4>(current.y & Short4(0xFC00u)) >> 5;
				current.z = As<UShort4>(current.z) >> 11;

				current.x = current.x | current.y | current.z;
			}
			break;
		case FORMAT_X8G8R8B8Q:
			UNIMPLEMENTED();
		//	current.x = As<Short4>(As<UShort4>(current.x) >> 8);
		//	current.y = As<Short4>(As<UShort4>(current.y) >> 8);
		//	current.z = As<Short4>(As<UShort4>(current.z) >> 8);

		//	current.z = As<Short4>(Pack(As<UShort4>(current.z), As<UShort4>(current.x)));
		//	current.y = As<Short4>(Pack(As<UShort4>(current.y), As<UShort4>(current.y)));
			break;
		case FORMAT_A8G8R8B8Q:
			UNIMPLEMENTED();
		//	current.x = As<Short4>(As<UShort4>(current.x) >> 8);
		//	current.y = As<Short4>(As<UShort4>(current.y) >> 8);
		//	current.z = As<Short4>(As<UShort4>(current.z) >> 8);
		//	current.w = As<Short4>(As<UShort4>(current.w) >> 8);

		//	current.z = As<Short4>(Pack(As<UShort4>(current.z), As<UShort4>(current.x)));
		//	current.y = As<Short4>(Pack(As<UShort4>(current.y), As<UShort4>(current.w)));
			break;
		case FORMAT_X8R8G8B8:
		case FORMAT_A8R8G8B8:
			if(state.targetFormat[index] == FORMAT_X8R8G8B8 || rgbaWriteMask == 0x7)
			{
				current.x = As<Short4>(As<UShort4>(current.x) >> 8);
				current.y = As<Short4>(As<UShort4>(current.y) >> 8);
				current.z = As<Short4>(As<UShort4>(current.z) >> 8);

				current.z = As<Short4>(PackUnsigned(current.z, current.x));
				current.y = As<Short4>(PackUnsigned(current.y, current.y));

				current.x = current.z;
				current.z = UnpackLow(As<Byte8>(current.z), As<Byte8>(current.y));
				current.x = UnpackHigh(As<Byte8>(current.x), As<Byte8>(current.y));
				current.y = current.z;
				current.z = As<Short4>(UnpackLow(current.z, current.x));
				current.y = As<Short4>(UnpackHigh(current.y, current.x));
			}
			else
			{
				current.x = As<Short4>(As<UShort4>(current.x) >> 8);
				current.y = As<Short4>(As<UShort4>(current.y) >> 8);
				current.z = As<Short4>(As<UShort4>(current.z) >> 8);
				current.w = As<Short4>(As<UShort4>(current.w) >> 8);

				current.z = As<Short4>(PackUnsigned(current.z, current.x));
				current.y = As<Short4>(PackUnsigned(current.y, current.w));

				current.x = current.z;
				current.z = UnpackLow(As<Byte8>(current.z), As<Byte8>(current.y));
				current.x = UnpackHigh(As<Byte8>(current.x), As<Byte8>(current.y));
				current.y = current.z;
				current.z = As<Short4>(UnpackLow(current.z, current.x));
				current.y = As<Short4>(UnpackHigh(current.y, current.x));
			}
			break;
		case FORMAT_X8B8G8R8:
		case FORMAT_A8B8G8R8:
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
			if(state.targetFormat[index] == FORMAT_X8B8G8R8 || state.targetFormat[index] == FORMAT_SRGB8_X8 || rgbaWriteMask == 0x7)
			{
				current.x = As<Short4>(As<UShort4>(current.x) >> 8);
				current.y = As<Short4>(As<UShort4>(current.y) >> 8);
				current.z = As<Short4>(As<UShort4>(current.z) >> 8);

				current.z = As<Short4>(PackUnsigned(current.x, current.z));
				current.y = As<Short4>(PackUnsigned(current.y, current.y));

				current.x = current.z;
				current.z = UnpackLow(As<Byte8>(current.z), As<Byte8>(current.y));
				current.x = UnpackHigh(As<Byte8>(current.x), As<Byte8>(current.y));
				current.y = current.z;
				current.z = As<Short4>(UnpackLow(current.z, current.x));
				current.y = As<Short4>(UnpackHigh(current.y, current.x));
			}
			else
			{
				current.x = As<Short4>(As<UShort4>(current.x) >> 8);
				current.y = As<Short4>(As<UShort4>(current.y) >> 8);
				current.z = As<Short4>(As<UShort4>(current.z) >> 8);
				current.w = As<Short4>(As<UShort4>(current.w) >> 8);

				current.z = As<Short4>(PackUnsigned(current.x, current.z));
				current.y = As<Short4>(PackUnsigned(current.y, current.w));

				current.x = current.z;
				current.z = UnpackLow(As<Byte8>(current.z), As<Byte8>(current.y));
				current.x = UnpackHigh(As<Byte8>(current.x), As<Byte8>(current.y));
				current.y = current.z;
				current.z = As<Short4>(UnpackLow(current.z, current.x));
				current.y = As<Short4>(UnpackHigh(current.y, current.x));
			}
			break;
		case FORMAT_G8R8:
			current.x = As<Short4>(As<UShort4>(current.x) >> 8);
			current.y = As<Short4>(As<UShort4>(current.y) >> 8);
			current.x = As<Short4>(PackUnsigned(current.x, current.x));
			current.y = As<Short4>(PackUnsigned(current.y, current.y));
			current.x = UnpackLow(As<Byte8>(current.x), As<Byte8>(current.y));
			break;
		case FORMAT_R8:
			current.x = As<Short4>(As<UShort4>(current.x) >> 8);
			current.x = As<Short4>(PackUnsigned(current.x, current.x));
			break;
		case FORMAT_A8:
			current.w = As<Short4>(As<UShort4>(current.w) >> 8);
			current.w = As<Short4>(PackUnsigned(current.w, current.w));
			break;
		case FORMAT_G16R16:
			current.z = current.x;
			current.x = As<Short4>(UnpackLow(current.x, current.y));
			current.z = As<Short4>(UnpackHigh(current.z, current.y));
			current.y = current.z;
			break;
		case FORMAT_A16B16G16R16:
			transpose4x4(current.x, current.y, current.z, current.w);
			break;
		default:
			ASSERT(false);
		}

		Short4 c01 = current.z;
		Short4 c23 = current.y;

		Int xMask;   // Combination of all masks

		if(state.depthTestActive)
		{
			xMask = zMask;
		}
		else
		{
			xMask = cMask;
		}

		if(state.stencilActive)
		{
			xMask &= sMask;
		}

		switch(state.targetFormat[index])
		{
		case FORMAT_R5G6B5:
			{
				Pointer<Byte> buffer = cBuffer + 2 * x;
				Int value = *Pointer<Int>(buffer);

				Int c01 = Extract(As<Int2>(current.x), 0);

				if((bgraWriteMask & 0x00000007) != 0x00000007)
				{
					Int masked = value;
					c01 &= *Pointer<Int>(constants + OFFSET(Constants,mask565Q[bgraWriteMask & 0x7][0]));
					masked &= *Pointer<Int>(constants + OFFSET(Constants,mask565Q[~bgraWriteMask & 0x7][0]));
					c01 |= masked;
				}

				c01 &= *Pointer<Int>(constants + OFFSET(Constants,maskW4Q[0][0]) + xMask * 8);
				value &= *Pointer<Int>(constants + OFFSET(Constants,invMaskW4Q[0][0]) + xMask * 8);
				c01 |= value;
				*Pointer<Int>(buffer) = c01;

				buffer += *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));
				value = *Pointer<Int>(buffer);

				Int c23 = Extract(As<Int2>(current.x), 1);

				if((bgraWriteMask & 0x00000007) != 0x00000007)
				{
					Int masked = value;
					c23 &= *Pointer<Int>(constants + OFFSET(Constants,mask565Q[bgraWriteMask & 0x7][0]));
					masked &= *Pointer<Int>(constants + OFFSET(Constants,mask565Q[~bgraWriteMask & 0x7][0]));
					c23 |= masked;
				}

				c23 &= *Pointer<Int>(constants + OFFSET(Constants,maskW4Q[0][2]) + xMask * 8);
				value &= *Pointer<Int>(constants + OFFSET(Constants,invMaskW4Q[0][2]) + xMask * 8);
				c23 |= value;
				*Pointer<Int>(buffer) = c23;
			}
			break;
		case FORMAT_A8G8R8B8Q:
		case FORMAT_X8G8R8B8Q:   // FIXME: Don't touch alpha?
			UNIMPLEMENTED();
		//	value = *Pointer<Short4>(cBuffer + 8 * x + 0);

		//	if((state.targetFormat[index] == FORMAT_A8G8R8B8Q && bgraWriteMask != 0x0000000F) ||
		//	   ((state.targetFormat[index] == FORMAT_X8G8R8B8Q && bgraWriteMask != 0x00000007) &&
		//	    (state.targetFormat[index] == FORMAT_X8G8R8B8Q && bgraWriteMask != 0x0000000F)))   // FIXME: Need for masking when XRGB && Fh?
		//	{
		//		Short4 masked = value;
		//		c01 &= *Pointer<Short4>(constants + OFFSET(Constants,maskB4Q[bgraWriteMask][0]));
		//		masked &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskB4Q[bgraWriteMask][0]));
		//		c01 |= masked;
		//	}

		//	c01 &= *Pointer<Short4>(constants + OFFSET(Constants,maskD01Q) + xMask * 8);
		//	value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskD01Q) + xMask * 8);
		//	c01 |= value;
		//	*Pointer<Short4>(cBuffer + 8 * x + 0) = c01;

		//	value = *Pointer<Short4>(cBuffer + 8 * x + 8);

		//	if((state.targetFormat[index] == FORMAT_A8G8R8B8Q && bgraWriteMask != 0x0000000F) ||
		//	   ((state.targetFormat[index] == FORMAT_X8G8R8B8Q && bgraWriteMask != 0x00000007) &&
		//	    (state.targetFormat[index] == FORMAT_X8G8R8B8Q && bgraWriteMask != 0x0000000F)))   // FIXME: Need for masking when XRGB && Fh?
		//	{
		//		Short4 masked = value;
		//		c23 &= *Pointer<Short4>(constants + OFFSET(Constants,maskB4Q[bgraWriteMask][0]));
		//		masked &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskB4Q[bgraWriteMask][0]));
		//		c23 |= masked;
		//	}

		//	c23 &= *Pointer<Short4>(constants + OFFSET(Constants,maskD23Q) + xMask * 8);
		//	value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskD23Q) + xMask * 8);
		//	c23 |= value;
		//	*Pointer<Short4>(cBuffer + 8 * x + 8) = c23;
			break;
		case FORMAT_A8R8G8B8:
		case FORMAT_X8R8G8B8:   // FIXME: Don't touch alpha?
			{
				Pointer<Byte> buffer = cBuffer + x * 4;
				Short4 value = *Pointer<Short4>(buffer);

				if((state.targetFormat[index] == FORMAT_A8R8G8B8 && bgraWriteMask != 0x0000000F) ||
				   ((state.targetFormat[index] == FORMAT_X8R8G8B8 && bgraWriteMask != 0x00000007) &&
					(state.targetFormat[index] == FORMAT_X8R8G8B8 && bgraWriteMask != 0x0000000F)))   // FIXME: Need for masking when XRGB && Fh?
				{
					Short4 masked = value;
					c01 &= *Pointer<Short4>(constants + OFFSET(Constants,maskB4Q[bgraWriteMask][0]));
					masked &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskB4Q[bgraWriteMask][0]));
					c01 |= masked;
				}

				c01 &= *Pointer<Short4>(constants + OFFSET(Constants,maskD01Q) + xMask * 8);
				value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskD01Q) + xMask * 8);
				c01 |= value;
				*Pointer<Short4>(buffer) = c01;

				buffer += *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));
				value = *Pointer<Short4>(buffer);

				if((state.targetFormat[index] == FORMAT_A8R8G8B8 && bgraWriteMask != 0x0000000F) ||
				   ((state.targetFormat[index] == FORMAT_X8R8G8B8 && bgraWriteMask != 0x00000007) &&
					(state.targetFormat[index] == FORMAT_X8R8G8B8 && bgraWriteMask != 0x0000000F)))   // FIXME: Need for masking when XRGB && Fh?
				{
					Short4 masked = value;
					c23 &= *Pointer<Short4>(constants + OFFSET(Constants,maskB4Q[bgraWriteMask][0]));
					masked &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskB4Q[bgraWriteMask][0]));
					c23 |= masked;
				}

				c23 &= *Pointer<Short4>(constants + OFFSET(Constants,maskD23Q) + xMask * 8);
				value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskD23Q) + xMask * 8);
				c23 |= value;
				*Pointer<Short4>(buffer) = c23;
			}
			break;
		case FORMAT_A8B8G8R8:
		case FORMAT_X8B8G8R8:   // FIXME: Don't touch alpha?
		case FORMAT_SRGB8_X8:
		case FORMAT_SRGB8_A8:
			{
				Pointer<Byte> buffer = cBuffer + x * 4;
				Short4 value = *Pointer<Short4>(buffer);

				bool masked = (((state.targetFormat[index] == FORMAT_A8B8G8R8 || state.targetFormat[index] == FORMAT_SRGB8_A8) && rgbaWriteMask != 0x0000000F) ||
				              (((state.targetFormat[index] == FORMAT_X8B8G8R8 || state.targetFormat[index] == FORMAT_SRGB8_X8) && rgbaWriteMask != 0x00000007) &&
				               ((state.targetFormat[index] == FORMAT_X8B8G8R8 || state.targetFormat[index] == FORMAT_SRGB8_X8) && rgbaWriteMask != 0x0000000F))); // FIXME: Need for masking when XBGR && Fh?

				if(masked)
				{
					Short4 masked = value;
					c01 &= *Pointer<Short4>(constants + OFFSET(Constants,maskB4Q[rgbaWriteMask][0]));
					masked &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskB4Q[rgbaWriteMask][0]));
					c01 |= masked;
				}

				c01 &= *Pointer<Short4>(constants + OFFSET(Constants,maskD01Q) + xMask * 8);
				value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskD01Q) + xMask * 8);
				c01 |= value;
				*Pointer<Short4>(buffer) = c01;

				buffer += *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));
				value = *Pointer<Short4>(buffer);

				if(masked)
				{
					Short4 masked = value;
					c23 &= *Pointer<Short4>(constants + OFFSET(Constants,maskB4Q[rgbaWriteMask][0]));
					masked &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskB4Q[rgbaWriteMask][0]));
					c23 |= masked;
				}

				c23 &= *Pointer<Short4>(constants + OFFSET(Constants,maskD23Q) + xMask * 8);
				value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskD23Q) + xMask * 8);
				c23 |= value;
				*Pointer<Short4>(buffer) = c23;
			}
			break;
		case FORMAT_G8R8:
			if((rgbaWriteMask & 0x00000003) != 0x0)
			{
				Pointer<Byte> buffer = cBuffer + 2 * x;
				Int2 value;
				value = Insert(value, *Pointer<Int>(buffer), 0);
				Int pitch = *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
				value = Insert(value, *Pointer<Int>(buffer + pitch), 1);

				Int2 packedCol = As<Int2>(current.x);

				UInt2 mergedMask = *Pointer<UInt2>(constants + OFFSET(Constants, maskW4Q) + xMask * 8);
				if((rgbaWriteMask & 0x3) != 0x3)
				{
					Int tmpMask = *Pointer<Int>(constants + OFFSET(Constants, maskB4Q[5 * (rgbaWriteMask & 0x3)][0]));
					UInt2 rgbaMask = As<UInt2>(Int2(tmpMask, tmpMask));
					mergedMask &= rgbaMask;
				}

				packedCol = As<Int2>((As<UInt2>(packedCol) & mergedMask) | (As<UInt2>(value) & ~mergedMask));

				*Pointer<UInt>(buffer) = As<UInt>(Extract(packedCol, 0));
				*Pointer<UInt>(buffer + pitch) = As<UInt>(Extract(packedCol, 1));
			}
			break;
		case FORMAT_R8:
			if(rgbaWriteMask & 0x00000001)
			{
				Pointer<Byte> buffer = cBuffer + 1 * x;
				Short4 value;
				value = Insert(value, *Pointer<Short>(buffer), 0);
				Int pitch = *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
				value = Insert(value, *Pointer<Short>(buffer + pitch), 1);

				current.x &= *Pointer<Short4>(constants + OFFSET(Constants, maskB4Q) + 8 * xMask);
				value &= *Pointer<Short4>(constants + OFFSET(Constants, invMaskB4Q) + 8 * xMask);
				current.x |= value;

				*Pointer<Short>(buffer) = Extract(current.x, 0);
				*Pointer<Short>(buffer + pitch) = Extract(current.x, 1);
			}
			break;
		case FORMAT_A8:
			if(rgbaWriteMask & 0x00000008)
			{
				Pointer<Byte> buffer = cBuffer + 1 * x;
				Short4 value;
				value = Insert(value, *Pointer<Short>(buffer), 0);
				Int pitch = *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));
				value = Insert(value, *Pointer<Short>(buffer + pitch), 1);

				current.w &= *Pointer<Short4>(constants + OFFSET(Constants,maskB4Q) + 8 * xMask);
				value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskB4Q) + 8 * xMask);
				current.w |= value;

				*Pointer<Short>(buffer) = Extract(current.w, 0);
				*Pointer<Short>(buffer + pitch) = Extract(current.w, 1);
			}
			break;
		case FORMAT_G16R16:
			{
				Pointer<Byte> buffer = cBuffer + 4 * x;

				Short4 value = *Pointer<Short4>(buffer);

				if((rgbaWriteMask & 0x00000003) != 0x00000003)
				{
					Short4 masked = value;
					current.x &= *Pointer<Short4>(constants + OFFSET(Constants,maskW01Q[rgbaWriteMask & 0x3][0]));
					masked &= *Pointer<Short4>(constants + OFFSET(Constants,maskW01Q[~rgbaWriteMask & 0x3][0]));
					current.x |= masked;
				}

				current.x &= *Pointer<Short4>(constants + OFFSET(Constants,maskD01Q) + xMask * 8);
				value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskD01Q) + xMask * 8);
				current.x |= value;
				*Pointer<Short4>(buffer) = current.x;

				buffer += *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));

				value = *Pointer<Short4>(buffer);

				if((rgbaWriteMask & 0x00000003) != 0x00000003)
				{
					Short4 masked = value;
					current.y &= *Pointer<Short4>(constants + OFFSET(Constants,maskW01Q[rgbaWriteMask & 0x3][0]));
					masked &= *Pointer<Short4>(constants + OFFSET(Constants,maskW01Q[~rgbaWriteMask & 0x3][0]));
					current.y |= masked;
				}

				current.y &= *Pointer<Short4>(constants + OFFSET(Constants,maskD23Q) + xMask * 8);
				value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskD23Q) + xMask * 8);
				current.y |= value;
				*Pointer<Short4>(buffer) = current.y;
			}
			break;
		case FORMAT_A16B16G16R16:
			{
				Pointer<Byte> buffer = cBuffer + 8 * x;

				{
					Short4 value = *Pointer<Short4>(buffer);

					if(rgbaWriteMask != 0x0000000F)
					{
						Short4 masked = value;
						current.x &= *Pointer<Short4>(constants + OFFSET(Constants,maskW4Q[rgbaWriteMask][0]));
						masked &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskW4Q[rgbaWriteMask][0]));
						current.x |= masked;
					}

					current.x &= *Pointer<Short4>(constants + OFFSET(Constants,maskQ0Q) + xMask * 8);
					value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskQ0Q) + xMask * 8);
					current.x |= value;
					*Pointer<Short4>(buffer) = current.x;
				}

				{
					Short4 value = *Pointer<Short4>(buffer + 8);

					if(rgbaWriteMask != 0x0000000F)
					{
						Short4 masked = value;
						current.y &= *Pointer<Short4>(constants + OFFSET(Constants,maskW4Q[rgbaWriteMask][0]));
						masked &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskW4Q[rgbaWriteMask][0]));
						current.y |= masked;
					}

					current.y &= *Pointer<Short4>(constants + OFFSET(Constants,maskQ1Q) + xMask * 8);
					value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskQ1Q) + xMask * 8);
					current.y |= value;
					*Pointer<Short4>(buffer + 8) = current.y;
				}

				buffer += *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));

				{
					Short4 value = *Pointer<Short4>(buffer);

					if(rgbaWriteMask != 0x0000000F)
					{
						Short4 masked = value;
						current.z &= *Pointer<Short4>(constants + OFFSET(Constants,maskW4Q[rgbaWriteMask][0]));
						masked &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskW4Q[rgbaWriteMask][0]));
						current.z |= masked;
					}

					current.z &= *Pointer<Short4>(constants + OFFSET(Constants,maskQ2Q) + xMask * 8);
					value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskQ2Q) + xMask * 8);
					current.z |= value;
					*Pointer<Short4>(buffer) = current.z;
				}

				{
					Short4 value = *Pointer<Short4>(buffer + 8);

					if(rgbaWriteMask != 0x0000000F)
					{
						Short4 masked = value;
						current.w &= *Pointer<Short4>(constants + OFFSET(Constants,maskW4Q[rgbaWriteMask][0]));
						masked &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskW4Q[rgbaWriteMask][0]));
						current.w |= masked;
					}

					current.w &= *Pointer<Short4>(constants + OFFSET(Constants,maskQ3Q) + xMask * 8);
					value &= *Pointer<Short4>(constants + OFFSET(Constants,invMaskQ3Q) + xMask * 8);
					current.w |= value;
					*Pointer<Short4>(buffer + 8) = current.w;
				}
			}
			break;
		default:
			ASSERT(false);
		}
	}

	void PixelRoutine::blendFactor(Vector4f &blendFactor, const Vector4f &oC, const Vector4f &pixel, BlendFactor blendFactorActive)
	{
		switch(blendFactorActive)
		{
		case BLEND_ZERO:
			// Optimized
			break;
		case BLEND_ONE:
			// Optimized
			break;
		case BLEND_SOURCE:
			blendFactor.x = oC.x;
			blendFactor.y = oC.y;
			blendFactor.z = oC.z;
			break;
		case BLEND_INVSOURCE:
			blendFactor.x = Float4(1.0f) - oC.x;
			blendFactor.y = Float4(1.0f) - oC.y;
			blendFactor.z = Float4(1.0f) - oC.z;
			break;
		case BLEND_DEST:
			blendFactor.x = pixel.x;
			blendFactor.y = pixel.y;
			blendFactor.z = pixel.z;
			break;
		case BLEND_INVDEST:
			blendFactor.x = Float4(1.0f) - pixel.x;
			blendFactor.y = Float4(1.0f) - pixel.y;
			blendFactor.z = Float4(1.0f) - pixel.z;
			break;
		case BLEND_SOURCEALPHA:
			blendFactor.x = oC.w;
			blendFactor.y = oC.w;
			blendFactor.z = oC.w;
			break;
		case BLEND_INVSOURCEALPHA:
			blendFactor.x = Float4(1.0f) - oC.w;
			blendFactor.y = Float4(1.0f) - oC.w;
			blendFactor.z = Float4(1.0f) - oC.w;
			break;
		case BLEND_DESTALPHA:
			blendFactor.x = pixel.w;
			blendFactor.y = pixel.w;
			blendFactor.z = pixel.w;
			break;
		case BLEND_INVDESTALPHA:
			blendFactor.x = Float4(1.0f) - pixel.w;
			blendFactor.y = Float4(1.0f) - pixel.w;
			blendFactor.z = Float4(1.0f) - pixel.w;
			break;
		case BLEND_SRCALPHASAT:
			blendFactor.x = Float4(1.0f) - pixel.w;
			blendFactor.x = Min(blendFactor.x, oC.w);
			blendFactor.y = blendFactor.x;
			blendFactor.z = blendFactor.x;
			break;
		case BLEND_CONSTANT:
			blendFactor.x = *Pointer<Float4>(data + OFFSET(DrawData,factor.blendConstant4F[0]));
			blendFactor.y = *Pointer<Float4>(data + OFFSET(DrawData,factor.blendConstant4F[1]));
			blendFactor.z = *Pointer<Float4>(data + OFFSET(DrawData,factor.blendConstant4F[2]));
			break;
		case BLEND_INVCONSTANT:
			blendFactor.x = *Pointer<Float4>(data + OFFSET(DrawData,factor.invBlendConstant4F[0]));
			blendFactor.y = *Pointer<Float4>(data + OFFSET(DrawData,factor.invBlendConstant4F[1]));
			blendFactor.z = *Pointer<Float4>(data + OFFSET(DrawData,factor.invBlendConstant4F[2]));
			break;
		default:
			ASSERT(false);
		}
	}

	void PixelRoutine::blendFactorAlpha(Vector4f &blendFactor, const Vector4f &oC, const Vector4f &pixel, BlendFactor blendFactorAlphaActive)
	{
		switch(blendFactorAlphaActive)
		{
		case BLEND_ZERO:
			// Optimized
			break;
		case BLEND_ONE:
			// Optimized
			break;
		case BLEND_SOURCE:
			blendFactor.w = oC.w;
			break;
		case BLEND_INVSOURCE:
			blendFactor.w = Float4(1.0f) - oC.w;
			break;
		case BLEND_DEST:
			blendFactor.w = pixel.w;
			break;
		case BLEND_INVDEST:
			blendFactor.w = Float4(1.0f) - pixel.w;
			break;
		case BLEND_SOURCEALPHA:
			blendFactor.w = oC.w;
			break;
		case BLEND_INVSOURCEALPHA:
			blendFactor.w = Float4(1.0f) - oC.w;
			break;
		case BLEND_DESTALPHA:
			blendFactor.w = pixel.w;
			break;
		case BLEND_INVDESTALPHA:
			blendFactor.w = Float4(1.0f) - pixel.w;
			break;
		case BLEND_SRCALPHASAT:
			blendFactor.w = Float4(1.0f);
			break;
		case BLEND_CONSTANT:
			blendFactor.w = *Pointer<Float4>(data + OFFSET(DrawData,factor.blendConstant4F[3]));
			break;
		case BLEND_INVCONSTANT:
			blendFactor.w = *Pointer<Float4>(data + OFFSET(DrawData,factor.invBlendConstant4F[3]));
			break;
		default:
			ASSERT(false);
		}
	}

	void PixelRoutine::alphaBlend(int index, Pointer<Byte> &cBuffer, Vector4f &oC, Int &x)
	{
		if(!state.alphaBlendActive)
		{
			return;
		}

		Pointer<Byte> buffer;
		Vector4f pixel;

		Vector4s color;
		Short4 c01;
		Short4 c23;

		Float4 one;
		if(Surface::isFloatFormat(state.targetFormat[index]))
		{
			one = Float4(1.0f);
		}
		else if(Surface::isNonNormalizedInteger(state.targetFormat[index]))
		{
			one = As<Float4>(Surface::isUnsignedComponent(state.targetFormat[index], 0) ? Int4(0xFFFFFFFF) : Int4(0x7FFFFFFF));
		}

		switch(state.targetFormat[index])
		{
		case FORMAT_R32I:
		case FORMAT_R32UI:
		case FORMAT_R32F:
			buffer = cBuffer;
			// FIXME: movlps
			pixel.x.x = *Pointer<Float>(buffer + 4 * x + 0);
			pixel.x.y = *Pointer<Float>(buffer + 4 * x + 4);
			buffer += *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));
			// FIXME: movhps
			pixel.x.z = *Pointer<Float>(buffer + 4 * x + 0);
			pixel.x.w = *Pointer<Float>(buffer + 4 * x + 4);
			pixel.y = pixel.z = pixel.w = one;
			break;
		case FORMAT_G32R32I:
		case FORMAT_G32R32UI:
		case FORMAT_G32R32F:
			buffer = cBuffer;
			pixel.x = *Pointer<Float4>(buffer + 8 * x, 16);
			buffer += *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));
			pixel.y = *Pointer<Float4>(buffer + 8 * x, 16);
			pixel.z = pixel.x;
			pixel.x = ShuffleLowHigh(pixel.x, pixel.y, 0x88);
			pixel.z = ShuffleLowHigh(pixel.z, pixel.y, 0xDD);
			pixel.y = pixel.z;
			pixel.z = pixel.w = one;
			break;
		case FORMAT_X32B32G32R32F:
		case FORMAT_A32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_A32B32G32R32I:
		case FORMAT_A32B32G32R32UI:
			buffer = cBuffer;
			pixel.x = *Pointer<Float4>(buffer + 16 * x, 16);
			pixel.y = *Pointer<Float4>(buffer + 16 * x + 16, 16);
			buffer += *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));
			pixel.z = *Pointer<Float4>(buffer + 16 * x, 16);
			pixel.w = *Pointer<Float4>(buffer + 16 * x + 16, 16);
			transpose4x4(pixel.x, pixel.y, pixel.z, pixel.w);
			if(state.targetFormat[index] == FORMAT_X32B32G32R32F ||
			   state.targetFormat[index] == FORMAT_X32B32G32R32F_UNSIGNED)
			{
				pixel.w = Float4(1.0f);
			}
			break;
		default:
			ASSERT(false);
		}

		if((postBlendSRGB && state.writeSRGB) || isSRGB(index))
		{
			sRGBtoLinear(pixel.x);
			sRGBtoLinear(pixel.y);
			sRGBtoLinear(pixel.z);
		}

		// Final Color = ObjectColor * SourceBlendFactor + PixelColor * DestinationBlendFactor
		Vector4f sourceFactor;
		Vector4f destFactor;

		blendFactor(sourceFactor, oC, pixel, state.sourceBlendFactor);
		blendFactor(destFactor, oC, pixel, state.destBlendFactor);

		if(state.sourceBlendFactor != BLEND_ONE && state.sourceBlendFactor != BLEND_ZERO)
		{
			oC.x *= sourceFactor.x;
			oC.y *= sourceFactor.y;
			oC.z *= sourceFactor.z;
		}

		if(state.destBlendFactor != BLEND_ONE && state.destBlendFactor != BLEND_ZERO)
		{
			pixel.x *= destFactor.x;
			pixel.y *= destFactor.y;
			pixel.z *= destFactor.z;
		}

		switch(state.blendOperation)
		{
		case BLENDOP_ADD:
			oC.x += pixel.x;
			oC.y += pixel.y;
			oC.z += pixel.z;
			break;
		case BLENDOP_SUB:
			oC.x -= pixel.x;
			oC.y -= pixel.y;
			oC.z -= pixel.z;
			break;
		case BLENDOP_INVSUB:
			oC.x = pixel.x - oC.x;
			oC.y = pixel.y - oC.y;
			oC.z = pixel.z - oC.z;
			break;
		case BLENDOP_MIN:
			oC.x = Min(oC.x, pixel.x);
			oC.y = Min(oC.y, pixel.y);
			oC.z = Min(oC.z, pixel.z);
			break;
		case BLENDOP_MAX:
			oC.x = Max(oC.x, pixel.x);
			oC.y = Max(oC.y, pixel.y);
			oC.z = Max(oC.z, pixel.z);
			break;
		case BLENDOP_SOURCE:
			// No operation
			break;
		case BLENDOP_DEST:
			oC.x = pixel.x;
			oC.y = pixel.y;
			oC.z = pixel.z;
			break;
		case BLENDOP_NULL:
			oC.x = Float4(0.0f);
			oC.y = Float4(0.0f);
			oC.z = Float4(0.0f);
			break;
		default:
			ASSERT(false);
		}

		blendFactorAlpha(sourceFactor, oC, pixel, state.sourceBlendFactorAlpha);
		blendFactorAlpha(destFactor, oC, pixel, state.destBlendFactorAlpha);

		if(state.sourceBlendFactorAlpha != BLEND_ONE && state.sourceBlendFactorAlpha != BLEND_ZERO)
		{
			oC.w *= sourceFactor.w;
		}

		if(state.destBlendFactorAlpha != BLEND_ONE && state.destBlendFactorAlpha != BLEND_ZERO)
		{
			pixel.w *= destFactor.w;
		}

		switch(state.blendOperationAlpha)
		{
		case BLENDOP_ADD:
			oC.w += pixel.w;
			break;
		case BLENDOP_SUB:
			oC.w -= pixel.w;
			break;
		case BLENDOP_INVSUB:
			pixel.w -= oC.w;
			oC.w = pixel.w;
			break;
		case BLENDOP_MIN:
			oC.w = Min(oC.w, pixel.w);
			break;
		case BLENDOP_MAX:
			oC.w = Max(oC.w, pixel.w);
			break;
		case BLENDOP_SOURCE:
			// No operation
			break;
		case BLENDOP_DEST:
			oC.w = pixel.w;
			break;
		case BLENDOP_NULL:
			oC.w = Float4(0.0f);
			break;
		default:
			ASSERT(false);
		}
	}

	void PixelRoutine::writeColor(int index, Pointer<Byte> &cBuffer, Int &x, Vector4f &oC, Int &sMask, Int &zMask, Int &cMask)
	{
		switch(state.targetFormat[index])
		{
		case FORMAT_R32F:
		case FORMAT_R32I:
		case FORMAT_R32UI:
		case FORMAT_R16I:
		case FORMAT_R16UI:
		case FORMAT_R8I:
		case FORMAT_R8UI:
			break;
		case FORMAT_G32R32F:
		case FORMAT_G32R32I:
		case FORMAT_G32R32UI:
		case FORMAT_G16R16I:
		case FORMAT_G16R16UI:
		case FORMAT_G8R8I:
		case FORMAT_G8R8UI:
			oC.z = oC.x;
			oC.x = UnpackLow(oC.x, oC.y);
			oC.z = UnpackHigh(oC.z, oC.y);
			oC.y = oC.z;
			break;
		case FORMAT_X32B32G32R32F:
		case FORMAT_A32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_A32B32G32R32I:
		case FORMAT_A32B32G32R32UI:
		case FORMAT_A16B16G16R16I:
		case FORMAT_A16B16G16R16UI:
		case FORMAT_A8B8G8R8I:
		case FORMAT_A8B8G8R8UI:
			transpose4x4(oC.x, oC.y, oC.z, oC.w);
			break;
		default:
			ASSERT(false);
		}

		int rgbaWriteMask = state.colorWriteActive(index);

		Int xMask;   // Combination of all masks

		if(state.depthTestActive)
		{
			xMask = zMask;
		}
		else
		{
			xMask = cMask;
		}

		if(state.stencilActive)
		{
			xMask &= sMask;
		}

		Pointer<Byte> buffer;
		Float4 value;

		switch(state.targetFormat[index])
		{
		case FORMAT_R32F:
		case FORMAT_R32I:
		case FORMAT_R32UI:
			if(rgbaWriteMask & 0x00000001)
			{
				buffer = cBuffer + 4 * x;

				// FIXME: movlps
				value.x = *Pointer<Float>(buffer + 0);
				value.y = *Pointer<Float>(buffer + 4);

				buffer += *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));

				// FIXME: movhps
				value.z = *Pointer<Float>(buffer + 0);
				value.w = *Pointer<Float>(buffer + 4);

				oC.x = As<Float4>(As<Int4>(oC.x) & *Pointer<Int4>(constants + OFFSET(Constants,maskD4X) + xMask * 16, 16));
				value = As<Float4>(As<Int4>(value) & *Pointer<Int4>(constants + OFFSET(Constants,invMaskD4X) + xMask * 16, 16));
				oC.x = As<Float4>(As<Int4>(oC.x) | As<Int4>(value));

				// FIXME: movhps
				*Pointer<Float>(buffer + 0) = oC.x.z;
				*Pointer<Float>(buffer + 4) = oC.x.w;

				buffer -= *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));

				// FIXME: movlps
				*Pointer<Float>(buffer + 0) = oC.x.x;
				*Pointer<Float>(buffer + 4) = oC.x.y;
			}
			break;
		case FORMAT_R16I:
		case FORMAT_R16UI:
			if(rgbaWriteMask & 0x00000001)
			{
				buffer = cBuffer + 2 * x;

				UShort4 xyzw;
				xyzw = As<UShort4>(Insert(As<Int2>(xyzw), *Pointer<Int>(buffer), 0));

				buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));

				xyzw = As<UShort4>(Insert(As<Int2>(xyzw), *Pointer<Int>(buffer), 1));
				value = As<Float4>(Int4(xyzw));

				oC.x = As<Float4>(As<Int4>(oC.x) & *Pointer<Int4>(constants + OFFSET(Constants, maskD4X) + xMask * 16, 16));
				value = As<Float4>(As<Int4>(value) & *Pointer<Int4>(constants + OFFSET(Constants, invMaskD4X) + xMask * 16, 16));
				oC.x = As<Float4>(As<Int4>(oC.x) | As<Int4>(value));

				if(state.targetFormat[index] == FORMAT_R16I)
				{
					Float component = oC.x.z;
					*Pointer<Short>(buffer + 0) = Short(As<Int>(component));
					component = oC.x.w;
					*Pointer<Short>(buffer + 2) = Short(As<Int>(component));

					buffer -= *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));

					component = oC.x.x;
					*Pointer<Short>(buffer + 0) = Short(As<Int>(component));
					component = oC.x.y;
					*Pointer<Short>(buffer + 2) = Short(As<Int>(component));
				}
				else // FORMAT_R16UI
				{
					Float component = oC.x.z;
					*Pointer<UShort>(buffer + 0) = UShort(As<Int>(component));
					component = oC.x.w;
					*Pointer<UShort>(buffer + 2) = UShort(As<Int>(component));

					buffer -= *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));

					component = oC.x.x;
					*Pointer<UShort>(buffer + 0) = UShort(As<Int>(component));
					component = oC.x.y;
					*Pointer<UShort>(buffer + 2) = UShort(As<Int>(component));
				}
			}
			break;
		case FORMAT_R8I:
		case FORMAT_R8UI:
			if(rgbaWriteMask & 0x00000001)
			{
				buffer = cBuffer + x;

				UInt xyzw, packedCol;

				xyzw = UInt(*Pointer<UShort>(buffer)) & 0xFFFF;
				buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
				xyzw |= UInt(*Pointer<UShort>(buffer)) << 16;

				Short4 tmpCol = Short4(As<Int4>(oC.x));
				if(state.targetFormat[index] == FORMAT_R8I)
				{
					tmpCol = As<Short4>(PackSigned(tmpCol, tmpCol));
				}
				else
				{
					tmpCol = As<Short4>(PackUnsigned(tmpCol, tmpCol));
				}
				packedCol = Extract(As<Int2>(tmpCol), 0);

				packedCol = (packedCol & *Pointer<UInt>(constants + OFFSET(Constants, maskB4Q) + 8 * xMask)) |
				            (xyzw & *Pointer<UInt>(constants + OFFSET(Constants, invMaskB4Q) + 8 * xMask));

				*Pointer<UShort>(buffer) = UShort(packedCol >> 16);
				buffer -= *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
				*Pointer<UShort>(buffer) = UShort(packedCol);
			}
			break;
		case FORMAT_G32R32F:
		case FORMAT_G32R32I:
		case FORMAT_G32R32UI:
			buffer = cBuffer + 8 * x;

			value = *Pointer<Float4>(buffer);

			if((rgbaWriteMask & 0x00000003) != 0x00000003)
			{
				Float4 masked = value;
				oC.x = As<Float4>(As<Int4>(oC.x) & *Pointer<Int4>(constants + OFFSET(Constants,maskD01X[rgbaWriteMask & 0x3][0])));
				masked = As<Float4>(As<Int4>(masked) & *Pointer<Int4>(constants + OFFSET(Constants,maskD01X[~rgbaWriteMask & 0x3][0])));
				oC.x = As<Float4>(As<Int4>(oC.x) | As<Int4>(masked));
			}

			oC.x = As<Float4>(As<Int4>(oC.x) & *Pointer<Int4>(constants + OFFSET(Constants,maskQ01X) + xMask * 16, 16));
			value = As<Float4>(As<Int4>(value) & *Pointer<Int4>(constants + OFFSET(Constants,invMaskQ01X) + xMask * 16, 16));
			oC.x = As<Float4>(As<Int4>(oC.x) | As<Int4>(value));
			*Pointer<Float4>(buffer) = oC.x;

			buffer += *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));

			value = *Pointer<Float4>(buffer);

			if((rgbaWriteMask & 0x00000003) != 0x00000003)
			{
				Float4 masked;

				masked = value;
				oC.y = As<Float4>(As<Int4>(oC.y) & *Pointer<Int4>(constants + OFFSET(Constants,maskD01X[rgbaWriteMask & 0x3][0])));
				masked = As<Float4>(As<Int4>(masked) & *Pointer<Int4>(constants + OFFSET(Constants,maskD01X[~rgbaWriteMask & 0x3][0])));
				oC.y = As<Float4>(As<Int4>(oC.y) | As<Int4>(masked));
			}

			oC.y = As<Float4>(As<Int4>(oC.y) & *Pointer<Int4>(constants + OFFSET(Constants,maskQ23X) + xMask * 16, 16));
			value = As<Float4>(As<Int4>(value) & *Pointer<Int4>(constants + OFFSET(Constants,invMaskQ23X) + xMask * 16, 16));
			oC.y = As<Float4>(As<Int4>(oC.y) | As<Int4>(value));
			*Pointer<Float4>(buffer) = oC.y;
			break;
		case FORMAT_G16R16I:
		case FORMAT_G16R16UI:
			if((rgbaWriteMask & 0x00000003) != 0x0)
			{
				buffer = cBuffer + 4 * x;

				UInt2 rgbaMask;
				UShort4 packedCol = UShort4(As<Int4>(oC.x));
				UShort4 value = *Pointer<UShort4>(buffer);
				UInt2 mergedMask = *Pointer<UInt2>(constants + OFFSET(Constants, maskD01Q) + xMask * 8);
				if((rgbaWriteMask & 0x3) != 0x3)
				{
					Int tmpMask = *Pointer<Int>(constants + OFFSET(Constants, maskW4Q[rgbaWriteMask & 0x3][0]));
					rgbaMask = As<UInt2>(Int2(tmpMask, tmpMask));
					mergedMask &= rgbaMask;
				}
				*Pointer<UInt2>(buffer) = (As<UInt2>(packedCol) & mergedMask) | (As<UInt2>(value) & ~mergedMask);

				buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));

				packedCol = UShort4(As<Int4>(oC.y));
				value = *Pointer<UShort4>(buffer);
				mergedMask = *Pointer<UInt2>(constants + OFFSET(Constants, maskD23Q) + xMask * 8);
				if((rgbaWriteMask & 0x3) != 0x3)
				{
					mergedMask &= rgbaMask;
				}
				*Pointer<UInt2>(buffer) = (As<UInt2>(packedCol) & mergedMask) | (As<UInt2>(value) & ~mergedMask);
			}
			break;
		case FORMAT_G8R8I:
		case FORMAT_G8R8UI:
			if((rgbaWriteMask & 0x00000003) != 0x0)
			{
				buffer = cBuffer + 2 * x;

				Int2 xyzw, packedCol;

				xyzw = Insert(xyzw, *Pointer<Int>(buffer), 0);
				buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
				xyzw = Insert(xyzw, *Pointer<Int>(buffer), 1);

				if(state.targetFormat[index] == FORMAT_G8R8I)
				{
					packedCol = As<Int2>(PackSigned(Short4(As<Int4>(oC.x)), Short4(As<Int4>(oC.y))));
				}
				else
				{
					packedCol = As<Int2>(PackUnsigned(Short4(As<Int4>(oC.x)), Short4(As<Int4>(oC.y))));
				}

				UInt2 mergedMask = *Pointer<UInt2>(constants + OFFSET(Constants, maskW4Q) + xMask * 8);
				if((rgbaWriteMask & 0x3) != 0x3)
				{
					Int tmpMask = *Pointer<Int>(constants + OFFSET(Constants, maskB4Q[5 * (rgbaWriteMask & 0x3)][0]));
					UInt2 rgbaMask = As<UInt2>(Int2(tmpMask, tmpMask));
					mergedMask &= rgbaMask;
				}

				packedCol = As<Int2>((As<UInt2>(packedCol) & mergedMask) | (As<UInt2>(xyzw) & ~mergedMask));

				*Pointer<UInt>(buffer) = As<UInt>(Extract(packedCol, 1));
				buffer -= *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));
				*Pointer<UInt>(buffer) = As<UInt>(Extract(packedCol, 0));
			}
			break;
		case FORMAT_X32B32G32R32F:
		case FORMAT_A32B32G32R32F:
		case FORMAT_X32B32G32R32F_UNSIGNED:
		case FORMAT_A32B32G32R32I:
		case FORMAT_A32B32G32R32UI:
			buffer = cBuffer + 16 * x;

			{
				value = *Pointer<Float4>(buffer, 16);

				if(rgbaWriteMask != 0x0000000F)
				{
					Float4 masked = value;
					oC.x = As<Float4>(As<Int4>(oC.x) & *Pointer<Int4>(constants + OFFSET(Constants,maskD4X[rgbaWriteMask][0])));
					masked = As<Float4>(As<Int4>(masked) & *Pointer<Int4>(constants + OFFSET(Constants,invMaskD4X[rgbaWriteMask][0])));
					oC.x = As<Float4>(As<Int4>(oC.x) | As<Int4>(masked));
				}

				oC.x = As<Float4>(As<Int4>(oC.x) & *Pointer<Int4>(constants + OFFSET(Constants,maskX0X) + xMask * 16, 16));
				value = As<Float4>(As<Int4>(value) & *Pointer<Int4>(constants + OFFSET(Constants,invMaskX0X) + xMask * 16, 16));
				oC.x = As<Float4>(As<Int4>(oC.x) | As<Int4>(value));
				*Pointer<Float4>(buffer, 16) = oC.x;
			}

			{
				value = *Pointer<Float4>(buffer + 16, 16);

				if(rgbaWriteMask != 0x0000000F)
				{
					Float4 masked = value;
					oC.y = As<Float4>(As<Int4>(oC.y) & *Pointer<Int4>(constants + OFFSET(Constants,maskD4X[rgbaWriteMask][0])));
					masked = As<Float4>(As<Int4>(masked) & *Pointer<Int4>(constants + OFFSET(Constants,invMaskD4X[rgbaWriteMask][0])));
					oC.y = As<Float4>(As<Int4>(oC.y) | As<Int4>(masked));
				}

				oC.y = As<Float4>(As<Int4>(oC.y) & *Pointer<Int4>(constants + OFFSET(Constants,maskX1X) + xMask * 16, 16));
				value = As<Float4>(As<Int4>(value) & *Pointer<Int4>(constants + OFFSET(Constants,invMaskX1X) + xMask * 16, 16));
				oC.y = As<Float4>(As<Int4>(oC.y) | As<Int4>(value));
				*Pointer<Float4>(buffer + 16, 16) = oC.y;
			}

			buffer += *Pointer<Int>(data + OFFSET(DrawData,colorPitchB[index]));

			{
				value = *Pointer<Float4>(buffer, 16);

				if(rgbaWriteMask != 0x0000000F)
				{
					Float4 masked = value;
					oC.z = As<Float4>(As<Int4>(oC.z) & *Pointer<Int4>(constants + OFFSET(Constants,maskD4X[rgbaWriteMask][0])));
					masked = As<Float4>(As<Int4>(masked) & *Pointer<Int4>(constants + OFFSET(Constants,invMaskD4X[rgbaWriteMask][0])));
					oC.z = As<Float4>(As<Int4>(oC.z) | As<Int4>(masked));
				}

				oC.z = As<Float4>(As<Int4>(oC.z) & *Pointer<Int4>(constants + OFFSET(Constants,maskX2X) + xMask * 16, 16));
				value = As<Float4>(As<Int4>(value) & *Pointer<Int4>(constants + OFFSET(Constants,invMaskX2X) + xMask * 16, 16));
				oC.z = As<Float4>(As<Int4>(oC.z) | As<Int4>(value));
				*Pointer<Float4>(buffer, 16) = oC.z;
			}

			{
				value = *Pointer<Float4>(buffer + 16, 16);

				if(rgbaWriteMask != 0x0000000F)
				{
					Float4 masked = value;
					oC.w = As<Float4>(As<Int4>(oC.w) & *Pointer<Int4>(constants + OFFSET(Constants,maskD4X[rgbaWriteMask][0])));
					masked = As<Float4>(As<Int4>(masked) & *Pointer<Int4>(constants + OFFSET(Constants,invMaskD4X[rgbaWriteMask][0])));
					oC.w = As<Float4>(As<Int4>(oC.w) | As<Int4>(masked));
				}

				oC.w = As<Float4>(As<Int4>(oC.w) & *Pointer<Int4>(constants + OFFSET(Constants,maskX3X) + xMask * 16, 16));
				value = As<Float4>(As<Int4>(value) & *Pointer<Int4>(constants + OFFSET(Constants,invMaskX3X) + xMask * 16, 16));
				oC.w = As<Float4>(As<Int4>(oC.w) | As<Int4>(value));
				*Pointer<Float4>(buffer + 16, 16) = oC.w;
			}
			break;
		case FORMAT_A16B16G16R16I:
		case FORMAT_A16B16G16R16UI:
			if((rgbaWriteMask & 0x0000000F) != 0x0)
			{
				buffer = cBuffer + 8 * x;

				UInt4 rgbaMask;
				UShort8 value = *Pointer<UShort8>(buffer);
				UShort8 packedCol = UShort8(UShort4(As<Int4>(oC.x)), UShort4(As<Int4>(oC.y)));
				UInt4 mergedMask = *Pointer<UInt4>(constants + OFFSET(Constants, maskQ01X) + xMask * 16);
				if((rgbaWriteMask & 0xF) != 0xF)
				{
					UInt2 tmpMask = *Pointer<UInt2>(constants + OFFSET(Constants, maskW4Q[rgbaWriteMask][0]));
					rgbaMask = UInt4(tmpMask, tmpMask);
					mergedMask &= rgbaMask;
				}
				*Pointer<UInt4>(buffer) = (As<UInt4>(packedCol) & mergedMask) | (As<UInt4>(value) & ~mergedMask);

				buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));

				value = *Pointer<UShort8>(buffer);
				packedCol = UShort8(UShort4(As<Int4>(oC.z)), UShort4(As<Int4>(oC.w)));
				mergedMask = *Pointer<UInt4>(constants + OFFSET(Constants, maskQ23X) + xMask * 16);
				if((rgbaWriteMask & 0xF) != 0xF)
				{
					mergedMask &= rgbaMask;
				}
				*Pointer<UInt4>(buffer) = (As<UInt4>(packedCol) & mergedMask) | (As<UInt4>(value) & ~mergedMask);
			}
			break;
		case FORMAT_A8B8G8R8I:
		case FORMAT_A8B8G8R8UI:
			if((rgbaWriteMask & 0x0000000F) != 0x0)
			{
				UInt2 value, packedCol, mergedMask;

				buffer = cBuffer + 4 * x;

				if(state.targetFormat[index] == FORMAT_A8B8G8R8I)
				{
					packedCol = As<UInt2>(PackSigned(Short4(As<Int4>(oC.x)), Short4(As<Int4>(oC.y))));
				}
				else
				{
					packedCol = As<UInt2>(PackUnsigned(Short4(As<Int4>(oC.x)), Short4(As<Int4>(oC.y))));
				}
				value = *Pointer<UInt2>(buffer, 16);
				mergedMask = *Pointer<UInt2>(constants + OFFSET(Constants, maskD01Q) + xMask * 8);
				if(rgbaWriteMask != 0xF)
				{
					mergedMask &= *Pointer<UInt2>(constants + OFFSET(Constants, maskB4Q[rgbaWriteMask][0]));
				}
				*Pointer<UInt2>(buffer) = (packedCol & mergedMask) | (value & ~mergedMask);

				buffer += *Pointer<Int>(data + OFFSET(DrawData, colorPitchB[index]));

				if(state.targetFormat[index] == FORMAT_A8B8G8R8I)
				{
					packedCol = As<UInt2>(PackSigned(Short4(As<Int4>(oC.z)), Short4(As<Int4>(oC.w))));
				}
				else
				{
					packedCol = As<UInt2>(PackUnsigned(Short4(As<Int4>(oC.z)), Short4(As<Int4>(oC.w))));
				}
				value = *Pointer<UInt2>(buffer, 16);
				mergedMask = *Pointer<UInt2>(constants + OFFSET(Constants, maskD23Q) + xMask * 8);
				if(rgbaWriteMask != 0xF)
				{
					mergedMask &= *Pointer<UInt2>(constants + OFFSET(Constants, maskB4Q[rgbaWriteMask][0]));
				}
				*Pointer<UInt2>(buffer) = (packedCol & mergedMask) | (value & ~mergedMask);
			}
			break;
		default:
			ASSERT(false);
		}
	}

	UShort4 PixelRoutine::convertFixed16(Float4 &cf, bool saturate)
	{
		return UShort4(cf * Float4(0xFFFF), saturate);
	}

	void PixelRoutine::sRGBtoLinear16_12_16(Vector4s &c)
	{
		Pointer<Byte> LUT = constants + OFFSET(Constants,sRGBtoLinear12_16);

		c.x = As<UShort4>(c.x) >> 4;
		c.y = As<UShort4>(c.y) >> 4;
		c.z = As<UShort4>(c.z) >> 4;

		c.x = Insert(c.x, *Pointer<Short>(LUT + 2 * Int(Extract(c.x, 0))), 0);
		c.x = Insert(c.x, *Pointer<Short>(LUT + 2 * Int(Extract(c.x, 1))), 1);
		c.x = Insert(c.x, *Pointer<Short>(LUT + 2 * Int(Extract(c.x, 2))), 2);
		c.x = Insert(c.x, *Pointer<Short>(LUT + 2 * Int(Extract(c.x, 3))), 3);

		c.y = Insert(c.y, *Pointer<Short>(LUT + 2 * Int(Extract(c.y, 0))), 0);
		c.y = Insert(c.y, *Pointer<Short>(LUT + 2 * Int(Extract(c.y, 1))), 1);
		c.y = Insert(c.y, *Pointer<Short>(LUT + 2 * Int(Extract(c.y, 2))), 2);
		c.y = Insert(c.y, *Pointer<Short>(LUT + 2 * Int(Extract(c.y, 3))), 3);

		c.z = Insert(c.z, *Pointer<Short>(LUT + 2 * Int(Extract(c.z, 0))), 0);
		c.z = Insert(c.z, *Pointer<Short>(LUT + 2 * Int(Extract(c.z, 1))), 1);
		c.z = Insert(c.z, *Pointer<Short>(LUT + 2 * Int(Extract(c.z, 2))), 2);
		c.z = Insert(c.z, *Pointer<Short>(LUT + 2 * Int(Extract(c.z, 3))), 3);
	}

	void PixelRoutine::linearToSRGB16_12_16(Vector4s &c)
	{
		c.x = As<UShort4>(c.x) >> 4;
		c.y = As<UShort4>(c.y) >> 4;
		c.z = As<UShort4>(c.z) >> 4;

		linearToSRGB12_16(c);
	}

	void PixelRoutine::linearToSRGB12_16(Vector4s &c)
	{
		Pointer<Byte> LUT = constants + OFFSET(Constants,linearToSRGB12_16);

		c.x = Insert(c.x, *Pointer<Short>(LUT + 2 * Int(Extract(c.x, 0))), 0);
		c.x = Insert(c.x, *Pointer<Short>(LUT + 2 * Int(Extract(c.x, 1))), 1);
		c.x = Insert(c.x, *Pointer<Short>(LUT + 2 * Int(Extract(c.x, 2))), 2);
		c.x = Insert(c.x, *Pointer<Short>(LUT + 2 * Int(Extract(c.x, 3))), 3);

		c.y = Insert(c.y, *Pointer<Short>(LUT + 2 * Int(Extract(c.y, 0))), 0);
		c.y = Insert(c.y, *Pointer<Short>(LUT + 2 * Int(Extract(c.y, 1))), 1);
		c.y = Insert(c.y, *Pointer<Short>(LUT + 2 * Int(Extract(c.y, 2))), 2);
		c.y = Insert(c.y, *Pointer<Short>(LUT + 2 * Int(Extract(c.y, 3))), 3);

		c.z = Insert(c.z, *Pointer<Short>(LUT + 2 * Int(Extract(c.z, 0))), 0);
		c.z = Insert(c.z, *Pointer<Short>(LUT + 2 * Int(Extract(c.z, 1))), 1);
		c.z = Insert(c.z, *Pointer<Short>(LUT + 2 * Int(Extract(c.z, 2))), 2);
		c.z = Insert(c.z, *Pointer<Short>(LUT + 2 * Int(Extract(c.z, 3))), 3);
	}

	Float4 PixelRoutine::sRGBtoLinear(const Float4 &x)   // Approximates x^2.2
	{
		Float4 linear = x * x;
		linear = linear * Float4(0.73f) + linear * x * Float4(0.27f);

		return Min(Max(linear, Float4(0.0f)), Float4(1.0f));
	}

	bool PixelRoutine::colorUsed()
	{
		return state.colorWriteMask || state.alphaTestActive() || state.shaderContainsKill;
	}
}
