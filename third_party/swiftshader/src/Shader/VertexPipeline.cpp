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

#include "VertexPipeline.hpp"

#include "Renderer/Vertex.hpp"
#include "Renderer/Renderer.hpp"
#include "Common/Debug.hpp"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#undef max
#undef min

namespace sw
{
	extern bool secondaryColor;

	VertexPipeline::VertexPipeline(const VertexProcessor::State &state) : VertexRoutine(state, 0)
	{
	}

	VertexPipeline::~VertexPipeline()
	{
	}

	Vector4f VertexPipeline::transformBlend(const Register &src, const Pointer<Byte> &matrix, bool homogeneous)
	{
		Vector4f dst;

		if(state.vertexBlendMatrixCount == 0)
		{
			dst = transform(src, matrix, homogeneous);
		}
		else
		{
			UInt index0[4];
			UInt index1[4];
			UInt index2[4];
			UInt index3[4];

			if(state.indexedVertexBlendEnable)
			{
				for(int i = 0; i < 4; i++)
				{
					Float4 B = v[BlendIndices].x;
					UInt indices;

					switch(i)
					{
					case 0: indices = As<UInt>(Float(B.x)); break;
					case 1: indices = As<UInt>(Float(B.y)); break;
					case 2: indices = As<UInt>(Float(B.z)); break;
					case 3: indices = As<UInt>(Float(B.w)); break;
					}

					index0[i] = (indices & 0x000000FF) << 6;
					index1[i] = (indices & 0x0000FF00) >> 2;
					index2[i] = (indices & 0x00FF0000) >> 10;
					index3[i] = (indices & 0xFF000000) >> 18;
				}
			}
			else
			{
				for(int i = 0; i < 4; i++)
				{
					index0[i] = 0 * 64;
					index1[i] = 1 * 64;
					index2[i] = 2 * 64;
					index3[i] = 3 * 64;
				}
			}

			Float4 weight0;
			Float4 weight1;
			Float4 weight2;
			Float4 weight3;

			switch(state.vertexBlendMatrixCount)
			{
			case 4: weight2 = v[BlendWeight].z;
			case 3: weight1 = v[BlendWeight].y;
			case 2: weight0 = v[BlendWeight].x;
			case 1:
				break;
			}

			if(state.vertexBlendMatrixCount == 1)
			{
				dst = transform(src, matrix, index0, homogeneous);
			}
			else if(state.vertexBlendMatrixCount == 2)
			{
				weight1 = Float4(1.0f) - weight0;

				Vector4f pos0;
				Vector4f pos1;

				pos0 = transform(src, matrix, index0, homogeneous);
				pos1 = transform(src, matrix, index1, homogeneous);

				dst.x = pos0.x * weight0 + pos1.x * weight1;   // FIXME: Vector4f operators
				dst.y = pos0.y * weight0 + pos1.y * weight1;
				dst.z = pos0.z * weight0 + pos1.z * weight1;
				dst.w = pos0.w * weight0 + pos1.w * weight1;
			}
			else if(state.vertexBlendMatrixCount == 3)
			{
				weight2 = Float4(1.0f) - (weight0 + weight1);

				Vector4f pos0;
				Vector4f pos1;
				Vector4f pos2;

				pos0 = transform(src, matrix, index0, homogeneous);
				pos1 = transform(src, matrix, index1, homogeneous);
				pos2 = transform(src, matrix, index2, homogeneous);

				dst.x = pos0.x * weight0 + pos1.x * weight1 + pos2.x * weight2;
				dst.y = pos0.y * weight0 + pos1.y * weight1 + pos2.y * weight2;
				dst.z = pos0.z * weight0 + pos1.z * weight1 + pos2.z * weight2;
				dst.w = pos0.w * weight0 + pos1.w * weight1 + pos2.w * weight2;
			}
			else if(state.vertexBlendMatrixCount == 4)
			{
				weight3 = Float4(1.0f) - (weight0 + weight1 + weight2);

				Vector4f pos0;
				Vector4f pos1;
				Vector4f pos2;
				Vector4f pos3;

				pos0 = transform(src, matrix, index0, homogeneous);
				pos1 = transform(src, matrix, index1, homogeneous);
				pos2 = transform(src, matrix, index2, homogeneous);
				pos3 = transform(src, matrix, index3, homogeneous);

				dst.x = pos0.x * weight0 + pos1.x * weight1 + pos2.x * weight2 + pos3.x * weight3;
				dst.y = pos0.y * weight0 + pos1.y * weight1 + pos2.y * weight2 + pos3.y * weight3;
				dst.z = pos0.z * weight0 + pos1.z * weight1 + pos2.z * weight2 + pos3.z * weight3;
				dst.w = pos0.w * weight0 + pos1.w * weight1 + pos2.w * weight2 + pos3.w * weight3;
			}
		}

		return dst;
	}

	void VertexPipeline::pipeline(UInt &index)
	{
		Vector4f position;
		Vector4f normal;

		if(!state.preTransformed)
		{
			position = transformBlend(v[Position], Pointer<Byte>(data + OFFSET(DrawData,ff.transformT)), true);
		}
		else
		{
			position = v[PositionT];
		}

		o[Pos].x = position.x;
		o[Pos].y = position.y;
		o[Pos].z = position.z;
		o[Pos].w = position.w;

		Vector4f vertexPosition = transformBlend(v[Position], Pointer<Byte>(data + OFFSET(DrawData,ff.cameraTransformT)), true);

		if(state.vertexNormalActive)
		{
			normal = transformBlend(v[Normal], Pointer<Byte>(data + OFFSET(DrawData,ff.normalTransformT)), false);

			if(state.normalizeNormals)
			{
				normal = normalize(normal);
			}
		}

		if(!state.vertexLightingActive)
		{
			// FIXME: Don't process if not used at all
			if(state.diffuseActive && state.input[Color0])
			{
				Vector4f diffuse = v[Color0];

				o[C0].x = diffuse.x;
				o[C0].y = diffuse.y;
				o[C0].z = diffuse.z;
				o[C0].w = diffuse.w;
			}
			else
			{
				o[C0].x = Float4(1.0f);
				o[C0].y = Float4(1.0f);
				o[C0].z = Float4(1.0f);
				o[C0].w = Float4(1.0f);
			}

			// FIXME: Don't process if not used at all
			if(state.specularActive && state.input[Color1])
			{
				Vector4f specular = v[Color1];

				o[C1].x = specular.x;
				o[C1].y = specular.y;
				o[C1].z = specular.z;
				o[C1].w = specular.w;
			}
			else
			{
				o[C1].x = Float4(0.0f);
				o[C1].y = Float4(0.0f);
				o[C1].z = Float4(0.0f);
				o[C1].w = Float4(1.0f);
			}
		}
		else
		{
			o[C0].x = Float4(0.0f);
			o[C0].y = Float4(0.0f);
			o[C0].z = Float4(0.0f);
			o[C0].w = Float4(0.0f);

			o[C1].x = Float4(0.0f);
			o[C1].y = Float4(0.0f);
			o[C1].z = Float4(0.0f);
			o[C1].w = Float4(0.0f);

			Vector4f ambient;
			Float4 globalAmbient = *Pointer<Float4>(data + OFFSET(DrawData,ff.globalAmbient));   // FIXME: Unpack

			ambient.x = globalAmbient.x;
			ambient.y = globalAmbient.y;
			ambient.z = globalAmbient.z;

			for(int i = 0; i < 8; i++)
			{
				if(!(state.vertexLightActive & (1 << i)))
				{
					continue;
				}

				Vector4f L;    // Light vector
				Float4 att;   // Attenuation

				// Attenuation
				{
					Float4 d;   // Distance

					L.x = L.y = L.z = *Pointer<Float4>(data + OFFSET(DrawData,ff.lightPosition[i]));   // FIXME: Unpack
					L.x = L.x.xxxx;
					L.y = L.y.yyyy;
					L.z = L.z.zzzz;

					L.x -= vertexPosition.x;
					L.y -= vertexPosition.y;
					L.z -= vertexPosition.z;
					d = dot3(L, L);
					d = RcpSqrt_pp(d);     // FIXME: Sufficient precision?
					L.x *= d;
					L.y *= d;
					L.z *= d;
					d = Rcp_pp(d);       // FIXME: Sufficient precision?

					Float4 q = *Pointer<Float4>(data + OFFSET(DrawData,ff.attenuationQuadratic[i]));
					Float4 l = *Pointer<Float4>(data + OFFSET(DrawData,ff.attenuationLinear[i]));
					Float4 c = *Pointer<Float4>(data + OFFSET(DrawData,ff.attenuationConstant[i]));

					att = Rcp_pp((q * d + l) * d + c);
				}

				// Ambient per light
				{
					Float4 lightAmbient = *Pointer<Float4>(data + OFFSET(DrawData,ff.lightAmbient[i]));   // FIXME: Unpack

					ambient.x = ambient.x + lightAmbient.x * att;
					ambient.y = ambient.y + lightAmbient.y * att;
					ambient.z = ambient.z + lightAmbient.z * att;
				}

				// Diffuse
				if(state.vertexNormalActive)
				{
					Float4 dot;

					dot = dot3(L, normal);
					dot = Max(dot, Float4(0.0f));
					dot *= att;

					Vector4f diff;

					if(state.vertexDiffuseMaterialSourceActive == MATERIAL_MATERIAL)
					{
						diff.x = diff.y = diff.z = *Pointer<Float4>(data + OFFSET(DrawData,ff.materialDiffuse));   // FIXME: Unpack
						diff.x = diff.x.xxxx;
						diff.y = diff.y.yyyy;
						diff.z = diff.z.zzzz;
					}
					else if(state.vertexDiffuseMaterialSourceActive == MATERIAL_COLOR1)
					{
						diff = v[Color0];
					}
					else if(state.vertexDiffuseMaterialSourceActive == MATERIAL_COLOR2)
					{
						diff = v[Color1];
					}
					else ASSERT(false);

					Float4 lightDiffuse = *Pointer<Float4>(data + OFFSET(DrawData,ff.lightDiffuse[i]));

					o[C0].x = o[C0].x + diff.x * dot * lightDiffuse.x;   // FIXME: Clamp first?
					o[C0].y = o[C0].y + diff.y * dot * lightDiffuse.y;   // FIXME: Clamp first?
					o[C0].z = o[C0].z + diff.z * dot * lightDiffuse.z;   // FIXME: Clamp first?
				}

				// Specular
				if(state.vertexSpecularActive)
				{
					Vector4f S;
					Vector4f C;   // Camera vector
					Float4 pow;

					pow = *Pointer<Float>(data + OFFSET(DrawData,ff.materialShininess));

					S.x = Float4(0.0f) - vertexPosition.x;
					S.y = Float4(0.0f) - vertexPosition.y;
					S.z = Float4(0.0f) - vertexPosition.z;
					C = normalize(S);

					S.x = L.x + C.x;
					S.y = L.y + C.y;
					S.z = L.z + C.z;
					C = normalize(S);

					Float4 dot = Max(dot3(C, normal), Float4(0.0f));   // FIXME: max(dot3(C, normal), 0)

					Float4 P = power(dot, pow);
					P *= att;

					Vector4f spec;

					if(state.vertexSpecularMaterialSourceActive == MATERIAL_MATERIAL)
					{
						Float4 materialSpecular = *Pointer<Float4>(data + OFFSET(DrawData,ff.materialSpecular));   // FIXME: Unpack

						spec.x = materialSpecular.x;
						spec.y = materialSpecular.y;
						spec.z = materialSpecular.z;
					}
					else if(state.vertexSpecularMaterialSourceActive == MATERIAL_COLOR1)
					{
						spec = v[Color0];
					}
					else if(state.vertexSpecularMaterialSourceActive == MATERIAL_COLOR2)
					{
						spec = v[Color1];
					}
					else ASSERT(false);

					Float4 lightSpecular = *Pointer<Float4>(data + OFFSET(DrawData,ff.lightSpecular[i]));

					spec.x *= lightSpecular.x;
					spec.y *= lightSpecular.y;
					spec.z *= lightSpecular.z;

					spec.x *= P;
					spec.y *= P;
					spec.z *= P;

					spec.x = Max(spec.x, Float4(0.0f));
					spec.y = Max(spec.y, Float4(0.0f));
					spec.z = Max(spec.z, Float4(0.0f));

					if(secondaryColor)
					{
						o[C1].x = o[C1].x + spec.x;
						o[C1].y = o[C1].y + spec.y;
						o[C1].z = o[C1].z + spec.z;
					}
					else
					{
						o[C0].x = o[C0].x + spec.x;
						o[C0].y = o[C0].y + spec.y;
						o[C0].z = o[C0].z + spec.z;
					}
				}
			}

			if(state.vertexAmbientMaterialSourceActive == MATERIAL_MATERIAL)
			{
				Float4 materialAmbient = *Pointer<Float4>(data + OFFSET(DrawData,ff.materialAmbient));   // FIXME: Unpack

				ambient.x = ambient.x * materialAmbient.x;
				ambient.y = ambient.y * materialAmbient.y;
				ambient.z = ambient.z * materialAmbient.z;
			}
			else if(state.vertexAmbientMaterialSourceActive == MATERIAL_COLOR1)
			{
				Vector4f materialDiffuse = v[Color0];

				ambient.x = ambient.x * materialDiffuse.x;
				ambient.y = ambient.y * materialDiffuse.y;
				ambient.z = ambient.z * materialDiffuse.z;
			}
			else if(state.vertexAmbientMaterialSourceActive == MATERIAL_COLOR2)
			{
				Vector4f materialSpecular = v[Color1];

				ambient.x = ambient.x * materialSpecular.x;
				ambient.y = ambient.y * materialSpecular.y;
				ambient.z = ambient.z * materialSpecular.z;
			}
			else ASSERT(false);

			o[C0].x = o[C0].x + ambient.x;
			o[C0].y = o[C0].y + ambient.y;
			o[C0].z = o[C0].z + ambient.z;

			// Emissive
			if(state.vertexEmissiveMaterialSourceActive == MATERIAL_MATERIAL)
			{
				Float4 materialEmission = *Pointer<Float4>(data + OFFSET(DrawData,ff.materialEmission));   // FIXME: Unpack

				o[C0].x = o[C0].x + materialEmission.x;
				o[C0].y = o[C0].y + materialEmission.y;
				o[C0].z = o[C0].z + materialEmission.z;
			}
			else if(state.vertexEmissiveMaterialSourceActive == MATERIAL_COLOR1)
			{
				Vector4f materialSpecular = v[Color0];

				o[C0].x = o[C0].x + materialSpecular.x;
				o[C0].y = o[C0].y + materialSpecular.y;
				o[C0].z = o[C0].z + materialSpecular.z;
			}
			else if(state.vertexEmissiveMaterialSourceActive == MATERIAL_COLOR2)
			{
				Vector4f materialSpecular = v[Color1];

				o[C0].x = o[C0].x + materialSpecular.x;
				o[C0].y = o[C0].y + materialSpecular.y;
				o[C0].z = o[C0].z + materialSpecular.z;
			}
			else ASSERT(false);

			// Diffuse alpha component
			if(state.vertexDiffuseMaterialSourceActive == MATERIAL_MATERIAL)
			{
				o[C0].w = Float4(*Pointer<Float4>(data + OFFSET(DrawData,ff.materialDiffuse[0]))).wwww;   // FIXME: Unpack
			}
			else if(state.vertexDiffuseMaterialSourceActive == MATERIAL_COLOR1)
			{
				Vector4f alpha = v[Color0];
				o[C0].w = alpha.w;
			}
			else if(state.vertexDiffuseMaterialSourceActive == MATERIAL_COLOR2)
			{
				Vector4f alpha = v[Color1];
				o[C0].w = alpha.w;
			}
			else ASSERT(false);

			if(state.vertexSpecularActive)
			{
				// Specular alpha component
				if(state.vertexSpecularMaterialSourceActive == MATERIAL_MATERIAL)
				{
					o[C1].w = Float4(*Pointer<Float4>(data + OFFSET(DrawData,ff.materialSpecular[3]))).wwww;   // FIXME: Unpack
				}
				else if(state.vertexSpecularMaterialSourceActive == MATERIAL_COLOR1)
				{
					Vector4f alpha = v[Color0];
					o[C1].w = alpha.w;
				}
				else if(state.vertexSpecularMaterialSourceActive == MATERIAL_COLOR2)
				{
					Vector4f alpha = v[Color1];
					o[C1].w = alpha.w;
				}
				else ASSERT(false);
			}
		}

		if(state.fogActive)
		{
			Float4 f;

			if(!state.rangeFogActive)
			{
				f = Abs(vertexPosition.z);
			}
			else
			{
				f = Sqrt(dot3(vertexPosition, vertexPosition));   // FIXME: f = length(vertexPosition);
			}

			switch(state.vertexFogMode)
			{
			case FOG_NONE:
				if(state.specularActive)
				{
					o[Fog].x = o[C1].w;
				}
				else
				{
					o[Fog].x = Float4(0.0f);
				}
				break;
			case FOG_LINEAR:
				o[Fog].x = f * *Pointer<Float4>(data + OFFSET(DrawData,fog.scale)) + *Pointer<Float4>(data + OFFSET(DrawData,fog.offset));
				break;
			case FOG_EXP:
				o[Fog].x = exponential2(f * *Pointer<Float4>(data + OFFSET(DrawData,fog.densityE)), true);
				break;
			case FOG_EXP2:
				o[Fog].x = exponential2((f * f) * *Pointer<Float4>(data + OFFSET(DrawData,fog.density2E)), true);
				break;
			default:
				ASSERT(false);
			}
		}

		for(int stage = 0; stage < 8; stage++)
		{
			processTextureCoordinate(stage, normal, position);
		}

		processPointSize();
	}

	void VertexPipeline::processTextureCoordinate(int stage, Vector4f &normal, Vector4f &position)
	{
		if(state.output[T0 + stage].write)
		{
			int i = state.textureState[stage].texCoordIndexActive;

			switch(state.textureState[stage].texGenActive)
			{
			case TEXGEN_NONE:
				{
					Vector4f &&varying = v[TexCoord0 + i];

					o[T0 + stage].x = varying.x;
					o[T0 + stage].y = varying.y;
					o[T0 + stage].z = varying.z;
					o[T0 + stage].w = varying.w;
				}
				break;
			case TEXGEN_PASSTHRU:
				{
					Vector4f &&varying = v[TexCoord0 + i];

					o[T0 + stage].x = varying.x;
					o[T0 + stage].y = varying.y;
					o[T0 + stage].z = varying.z;
					o[T0 + stage].w = varying.w;

					if(state.input[TexCoord0 + i])
					{
						switch(state.input[TexCoord0 + i].count)
						{
						case 1:
							o[T0 + stage].y = Float4(1.0f);
							o[T0 + stage].z = Float4(0.0f);
							o[T0 + stage].w = Float4(0.0f);
							break;
						case 2:
							o[T0 + stage].z = Float4(1.0f);
							o[T0 + stage].w = Float4(0.0f);
							break;
						case 3:
							o[T0 + stage].w = Float4(1.0f);
							break;
						case 4:
							break;
						default:
							ASSERT(false);
						}
					}
				}
				break;
			case TEXGEN_NORMAL:
				{
					Vector4f Nc;   // Normal vector in camera space

					if(state.vertexNormalActive)
					{
						Nc = normal;
					}
					else
					{
						Nc.x = Float4(0.0f);
						Nc.y = Float4(0.0f);
						Nc.z = Float4(0.0f);
					}

					Nc.w = Float4(1.0f);

					o[T0 + stage].x = Nc.x;
					o[T0 + stage].y = Nc.y;
					o[T0 + stage].z = Nc.z;
					o[T0 + stage].w = Nc.w;
				}
				break;
			case TEXGEN_POSITION:
				{
					Vector4f Pn = transformBlend(v[Position], Pointer<Byte>(data + OFFSET(DrawData,ff.cameraTransformT)), true);   // Position in camera space

					Pn.w = Float4(1.0f);

					o[T0 + stage].x = Pn.x;
					o[T0 + stage].y = Pn.y;
					o[T0 + stage].z = Pn.z;
					o[T0 + stage].w = Pn.w;
				}
				break;
			case TEXGEN_REFLECTION:
				{
					Vector4f R;   // Reflection vector

					if(state.vertexNormalActive)
					{
						Vector4f Nc;   // Normal vector in camera space

						Nc = normal;

						if(state.localViewerActive)
						{
							Vector4f Ec;   // Eye vector in camera space
							Vector4f N2;

							Ec = transformBlend(v[Position], Pointer<Byte>(data + OFFSET(DrawData,ff.cameraTransformT)), true);
							Ec = normalize(Ec);

							// R = E - 2 * N * (E . N)
							Float4 dot = Float4(2.0f) * dot3(Ec, Nc);

							R.x = Ec.x - Nc.x * dot;
							R.y = Ec.y - Nc.y * dot;
							R.z = Ec.z - Nc.z * dot;
						}
						else
						{
							// u = -2 * Nz * Nx
							// v = -2 * Nz * Ny
							// w = 1 - 2 * Nz * Nz

							R.x = -Float4(2.0f) * Nc.z * Nc.x;
							R.y = -Float4(2.0f) * Nc.z * Nc.y;
							R.z = Float4(1.0f) - Float4(2.0f) * Nc.z * Nc.z;
						}
					}
					else
					{
						R.x = Float4(0.0f);
						R.y = Float4(0.0f);
						R.z = Float4(0.0f);
					}

					R.w = Float4(1.0f);

					o[T0 + stage].x = R.x;
					o[T0 + stage].y = R.y;
					o[T0 + stage].z = R.z;
					o[T0 + stage].w = R.w;
				}
				break;
			case TEXGEN_SPHEREMAP:
				{
					Vector4f R;   // Reflection vector

					if(state.vertexNormalActive)
					{
						Vector4f Nc;   // Normal vector in camera space

						Nc = normal;

						if(state.localViewerActive)
						{
							Vector4f Ec;   // Eye vector in camera space
							Vector4f N2;

							Ec = transformBlend(v[Position], Pointer<Byte>(data + OFFSET(DrawData,ff.cameraTransformT)), true);
							Ec = normalize(Ec);

							// R = E - 2 * N * (E . N)
							Float4 dot = Float4(2.0f) * dot3(Ec, Nc);

							R.x = Ec.x - Nc.x * dot;
							R.y = Ec.y - Nc.y * dot;
							R.z = Ec.z - Nc.z * dot;
						}
						else
						{
							// u = -2 * Nz * Nx
							// v = -2 * Nz * Ny
							// w = 1 - 2 * Nz * Nz

							R.x = -Float4(2.0f) * Nc.z * Nc.x;
							R.y = -Float4(2.0f) * Nc.z * Nc.y;
							R.z = Float4(1.0f) - Float4(2.0f) * Nc.z * Nc.z;
						}
					}
					else
					{
						R.x = Float4(0.0f);
						R.y = Float4(0.0f);
						R.z = Float4(0.0f);
					}

					R.z -= Float4(1.0f);
					R = normalize(R);
					R.x = Float4(0.5f) * R.x + Float4(0.5f);
					R.y = Float4(0.5f) * R.y + Float4(0.5f);

					R.z = Float4(1.0f);
					R.w = Float4(0.0f);

					o[T0 + stage].x = R.x;
					o[T0 + stage].y = R.y;
					o[T0 + stage].z = R.z;
					o[T0 + stage].w = R.w;
				}
				break;
			default:
				ASSERT(false);
			}

			Vector4f texTrans0;
			Vector4f texTrans1;
			Vector4f texTrans2;
			Vector4f texTrans3;

			Vector4f T;
			Vector4f t;

			T.x = o[T0 + stage].x;
			T.y = o[T0 + stage].y;
			T.z = o[T0 + stage].z;
			T.w = o[T0 + stage].w;

			switch(state.textureState[stage].textureTransformCountActive)
			{
			case 4:
				texTrans3.x = texTrans3.y = texTrans3.z = texTrans3.w = *Pointer<Float4>(data + OFFSET(DrawData,ff.textureTransform[stage][3]));   // FIXME: Unpack
				texTrans3.x = texTrans3.x.xxxx;
				texTrans3.y = texTrans3.y.yyyy;
				texTrans3.z = texTrans3.z.zzzz;
				texTrans3.w = texTrans3.w.wwww;
				t.w = dot4(T, texTrans3);
			case 3:
				texTrans2.x = texTrans2.y = texTrans2.z = texTrans2.w = *Pointer<Float4>(data + OFFSET(DrawData,ff.textureTransform[stage][2]));   // FIXME: Unpack
				texTrans2.x = texTrans2.x.xxxx;
				texTrans2.y = texTrans2.y.yyyy;
				texTrans2.z = texTrans2.z.zzzz;
				texTrans2.w = texTrans2.w.wwww;
				t.z = dot4(T, texTrans2);
			case 2:
				texTrans1.x = texTrans1.y = texTrans1.z = texTrans1.w = *Pointer<Float4>(data + OFFSET(DrawData,ff.textureTransform[stage][1]));   // FIXME: Unpack
				texTrans1.x = texTrans1.x.xxxx;
				texTrans1.y = texTrans1.y.yyyy;
				texTrans1.z = texTrans1.z.zzzz;
				texTrans1.w = texTrans1.w.wwww;
				t.y = dot4(T, texTrans1);
			case 1:
				texTrans0.x = texTrans0.y = texTrans0.z = texTrans0.w = *Pointer<Float4>(data + OFFSET(DrawData,ff.textureTransform[stage][0]));   // FIXME: Unpack
				texTrans0.x = texTrans0.x.xxxx;
				texTrans0.y = texTrans0.y.yyyy;
				texTrans0.z = texTrans0.z.zzzz;
				texTrans0.w = texTrans0.w.wwww;
				t.x = dot4(T, texTrans0);

				o[T0 + stage].x = t.x;
				o[T0 + stage].y = t.y;
				o[T0 + stage].z = t.z;
				o[T0 + stage].w = t.w;
			case 0:
				break;
			default:
				ASSERT(false);
			}
		}
	}

	void VertexPipeline::processPointSize()
	{
		if(!state.pointSizeActive)
		{
			return;   // Use global pointsize
		}

		if(state.input[PointSize])
		{
			o[Pts].y = v[PointSize].x;
		}
		else
		{
			o[Pts].y = *Pointer<Float4>(data + OFFSET(DrawData,point.pointSize));
		}

		if(state.pointScaleActive && !state.preTransformed)
		{
			Vector4f p = transformBlend(v[Position], Pointer<Byte>(data + OFFSET(DrawData,ff.cameraTransformT)), true);

			Float4 d = Sqrt(dot3(p, p));   // FIXME: length(p);

			Float4 A = *Pointer<Float>(data + OFFSET(DrawData,point.pointScaleA));   // FIXME: Unpack
			Float4 B = *Pointer<Float>(data + OFFSET(DrawData,point.pointScaleB));   // FIXME: Unpack
			Float4 C = *Pointer<Float>(data + OFFSET(DrawData,point.pointScaleC));   // FIXME: Unpack

			A = RcpSqrt_pp(A + d * (B + d * C));

			o[Pts].y = o[Pts].y * Float4(*Pointer<Float>(data + OFFSET(DrawData,viewportHeight))) * A;   // FIXME: Unpack
		}
	}

	Vector4f VertexPipeline::transform(const Register &src, const Pointer<Byte> &matrix, bool homogeneous)
	{
		Vector4f dst;

		if(homogeneous)
		{
			Float4 m[4][4];

			for(int j = 0; j < 4; j++)
			{
				for(int i = 0; i < 4; i++)
				{
					m[j][i].x = *Pointer<Float>(matrix + 16 * i + 4 * j);
					m[j][i].y = *Pointer<Float>(matrix + 16 * i + 4 * j);
					m[j][i].z = *Pointer<Float>(matrix + 16 * i + 4 * j);
					m[j][i].w = *Pointer<Float>(matrix + 16 * i + 4 * j);
				}
			}

			dst.x = src.x * m[0][0] + src.y * m[0][1] + src.z * m[0][2] + src.w * m[0][3];
			dst.y = src.x * m[1][0] + src.y * m[1][1] + src.z * m[1][2] + src.w * m[1][3];
			dst.z = src.x * m[2][0] + src.y * m[2][1] + src.z * m[2][2] + src.w * m[2][3];
			dst.w = src.x * m[3][0] + src.y * m[3][1] + src.z * m[3][2] + src.w * m[3][3];
		}
		else
		{
			Float4 m[3][3];

			for(int j = 0; j < 3; j++)
			{
				for(int i = 0; i < 3; i++)
				{
					m[j][i].x = *Pointer<Float>(matrix + 16 * i + 4 * j);
					m[j][i].y = *Pointer<Float>(matrix + 16 * i + 4 * j);
					m[j][i].z = *Pointer<Float>(matrix + 16 * i + 4 * j);
					m[j][i].w = *Pointer<Float>(matrix + 16 * i + 4 * j);
				}
			}

			dst.x = src.x * m[0][0] + src.y * m[0][1] + src.z * m[0][2];
			dst.y = src.x * m[1][0] + src.y * m[1][1] + src.z * m[1][2];
			dst.z = src.x * m[2][0] + src.y * m[2][1] + src.z * m[2][2];
		}

		return dst;
	}

	Vector4f VertexPipeline::transform(const Register &src, const Pointer<Byte> &matrix, UInt index[4], bool homogeneous)
	{
		Vector4f dst;

		if(homogeneous)
		{
			Float4 m[4][4];

			for(int j = 0; j < 4; j++)
			{
				for(int i = 0; i < 4; i++)
				{
					m[j][i].x = *Pointer<Float>(matrix + 16 * i + 4 * j + index[0]);
					m[j][i].y = *Pointer<Float>(matrix + 16 * i + 4 * j + index[1]);
					m[j][i].z = *Pointer<Float>(matrix + 16 * i + 4 * j + index[2]);
					m[j][i].w = *Pointer<Float>(matrix + 16 * i + 4 * j + index[3]);
				}
			}

			dst.x = src.x * m[0][0] + src.y * m[0][1] + src.z * m[0][2] + m[0][3];
			dst.y = src.x * m[1][0] + src.y * m[1][1] + src.z * m[1][2] + m[1][3];
			dst.z = src.x * m[2][0] + src.y * m[2][1] + src.z * m[2][2] + m[2][3];
			dst.w = src.x * m[3][0] + src.y * m[3][1] + src.z * m[3][2] + m[3][3];
		}
		else
		{
			Float4 m[3][3];

			for(int j = 0; j < 3; j++)
			{
				for(int i = 0; i < 3; i++)
				{
					m[j][i].x = *Pointer<Float>(matrix + 16 * i + 4 * j + index[0]);
					m[j][i].y = *Pointer<Float>(matrix + 16 * i + 4 * j + index[1]);
					m[j][i].z = *Pointer<Float>(matrix + 16 * i + 4 * j + index[2]);
					m[j][i].w = *Pointer<Float>(matrix + 16 * i + 4 * j + index[3]);
				}
			}

			dst.x = src.x * m[0][0] + src.y * m[0][1] + src.z * m[0][2];
			dst.y = src.x * m[1][0] + src.y * m[1][1] + src.z * m[1][2];
			dst.z = src.x * m[2][0] + src.y * m[2][1] + src.z * m[2][2];
		}

		return dst;
	}

	Vector4f VertexPipeline::normalize(Vector4f &src)
	{
		Vector4f dst;

		Float4 rcpLength = RcpSqrt_pp(dot3(src, src));

		dst.x = src.x * rcpLength;
		dst.y = src.y * rcpLength;
		dst.z = src.z * rcpLength;

		return dst;
	}

	Float4 VertexPipeline::power(Float4 &src0, Float4 &src1)
	{
		Float4 dst = src0;

		dst = dst * dst;
		dst = dst * dst;
		dst = Float4(As<Int4>(dst) - As<Int4>(Float4(1.0f)));

		dst *= src1;

		dst = As<Float4>(Int4(dst) + As<Int4>(Float4(1.0f)));
		dst = RcpSqrt_pp(dst);
		dst = RcpSqrt_pp(dst);

		return dst;
	}
}
