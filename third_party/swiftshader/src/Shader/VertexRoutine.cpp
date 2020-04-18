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

#include "VertexRoutine.hpp"

#include "VertexShader.hpp"
#include "Constants.hpp"
#include "Renderer/Vertex.hpp"
#include "Renderer/Renderer.hpp"
#include "Common/Half.hpp"
#include "Common/Debug.hpp"

namespace sw
{
	extern bool halfIntegerCoordinates;     // Pixel centers are not at integer coordinates
	extern bool symmetricNormalizedDepth;   // [-1, 1] instead of [0, 1]

	VertexRoutine::VertexRoutine(const VertexProcessor::State &state, const VertexShader *shader)
		: v(shader && shader->dynamicallyIndexedInput),
		  o(shader && shader->dynamicallyIndexedOutput),
		  state(state)
	{
	}

	VertexRoutine::~VertexRoutine()
	{
	}

	void VertexRoutine::generate()
	{
		const bool textureSampling = state.textureSampling;

		Pointer<Byte> cache = task + OFFSET(VertexTask,vertexCache);
		Pointer<Byte> vertexCache = cache + OFFSET(VertexCache,vertex);
		Pointer<Byte> tagCache = cache + OFFSET(VertexCache,tag);

		UInt vertexCount = *Pointer<UInt>(task + OFFSET(VertexTask,vertexCount));
		UInt primitiveNumber = *Pointer<UInt>(task + OFFSET(VertexTask, primitiveStart));
		UInt indexInPrimitive = 0;

		constants = *Pointer<Pointer<Byte>>(data + OFFSET(DrawData,constants));

		Do
		{
			UInt index = *Pointer<UInt>(batch);
			UInt tagIndex = index & 0x0000003C;
			UInt indexQ = !textureSampling ? UInt(index & 0xFFFFFFFC) : index;   // FIXME: TEXLDL hack to have independent LODs, hurts performance.

			If(*Pointer<UInt>(tagCache + tagIndex) != indexQ)
			{
				*Pointer<UInt>(tagCache + tagIndex) = indexQ;

				readInput(indexQ);
				pipeline(indexQ);
				postTransform();
				computeClipFlags();

				Pointer<Byte> cacheLine0 = vertexCache + tagIndex * UInt((int)sizeof(Vertex));
				writeCache(cacheLine0);
			}

			UInt cacheIndex = index & 0x0000003F;
			Pointer<Byte> cacheLine = vertexCache + cacheIndex * UInt((int)sizeof(Vertex));
			writeVertex(vertex, cacheLine);

			if(state.transformFeedbackEnabled != 0)
			{
				transformFeedback(vertex, primitiveNumber, indexInPrimitive);

				indexInPrimitive++;
				If(indexInPrimitive == 3)
				{
					primitiveNumber++;
					indexInPrimitive = 0;
				}
			}

			vertex += sizeof(Vertex);
			batch += sizeof(unsigned int);
			vertexCount--;
		}
		Until(vertexCount == 0)

		Return();
	}

	void VertexRoutine::readInput(UInt &index)
	{
		for(int i = 0; i < MAX_VERTEX_INPUTS; i++)
		{
			Pointer<Byte> input = *Pointer<Pointer<Byte>>(data + OFFSET(DrawData,input) + sizeof(void*) * i);
			UInt stride = *Pointer<UInt>(data + OFFSET(DrawData,stride) + sizeof(unsigned int) * i);

			v[i] = readStream(input, stride, state.input[i], index);
		}
	}

	void VertexRoutine::computeClipFlags()
	{
		int pos = state.positionRegister;

		Int4 maxX = CmpLT(o[pos].w, o[pos].x);
		Int4 maxY = CmpLT(o[pos].w, o[pos].y);
		Int4 maxZ = CmpLT(o[pos].w, o[pos].z);
		Int4 minX = CmpNLE(-o[pos].w, o[pos].x);
		Int4 minY = CmpNLE(-o[pos].w, o[pos].y);
		Int4 minZ = symmetricNormalizedDepth ? CmpNLE(-o[pos].w, o[pos].z) : CmpNLE(Float4(0.0f), o[pos].z);

		clipFlags = *Pointer<Int>(constants + OFFSET(Constants,maxX) + SignMask(maxX) * 4);   // FIXME: Array indexing
		clipFlags |= *Pointer<Int>(constants + OFFSET(Constants,maxY) + SignMask(maxY) * 4);
		clipFlags |= *Pointer<Int>(constants + OFFSET(Constants,maxZ) + SignMask(maxZ) * 4);
		clipFlags |= *Pointer<Int>(constants + OFFSET(Constants,minX) + SignMask(minX) * 4);
		clipFlags |= *Pointer<Int>(constants + OFFSET(Constants,minY) + SignMask(minY) * 4);
		clipFlags |= *Pointer<Int>(constants + OFFSET(Constants,minZ) + SignMask(minZ) * 4);

		Int4 finiteX = CmpLE(Abs(o[pos].x), *Pointer<Float4>(constants + OFFSET(Constants,maxPos)));
		Int4 finiteY = CmpLE(Abs(o[pos].y), *Pointer<Float4>(constants + OFFSET(Constants,maxPos)));
		Int4 finiteZ = CmpLE(Abs(o[pos].z), *Pointer<Float4>(constants + OFFSET(Constants,maxPos)));

		Int4 finiteXYZ = finiteX & finiteY & finiteZ;
		clipFlags |= *Pointer<Int>(constants + OFFSET(Constants,fini) + SignMask(finiteXYZ) * 4);

		if(state.preTransformed)
		{
			clipFlags &= 0xFBFBFBFB;   // Don't clip against far clip plane
		}
	}

	Vector4f VertexRoutine::readStream(Pointer<Byte> &buffer, UInt &stride, const Stream &stream, const UInt &index)
	{
		const bool textureSampling = state.textureSampling;

		Vector4f v;

		Pointer<Byte> source0 = buffer + index * stride;
		Pointer<Byte> source1 = source0 + (!textureSampling ? stride : 0);
		Pointer<Byte> source2 = source1 + (!textureSampling ? stride : 0);
		Pointer<Byte> source3 = source2 + (!textureSampling ? stride : 0);

		bool isNativeFloatAttrib = (stream.attribType == VertexShader::ATTRIBTYPE_FLOAT) || stream.normalized;

		switch(stream.type)
		{
		case STREAMTYPE_FLOAT:
			{
				if(stream.count == 0)
				{
					// Null stream, all default components
				}
				else
				{
					if(stream.count == 1)
					{
						v.x.x = *Pointer<Float>(source0);
						v.x.y = *Pointer<Float>(source1);
						v.x.z = *Pointer<Float>(source2);
						v.x.w = *Pointer<Float>(source3);
					}
					else
					{
						v.x = *Pointer<Float4>(source0);
						v.y = *Pointer<Float4>(source1);
						v.z = *Pointer<Float4>(source2);
						v.w = *Pointer<Float4>(source3);

						transpose4xN(v.x, v.y, v.z, v.w, stream.count);
					}

					switch(stream.attribType)
					{
					case VertexShader::ATTRIBTYPE_INT:
						if(stream.count >= 1) v.x = As<Float4>(Int4(v.x));
						if(stream.count >= 2) v.x = As<Float4>(Int4(v.y));
						if(stream.count >= 3) v.x = As<Float4>(Int4(v.z));
						if(stream.count >= 4) v.x = As<Float4>(Int4(v.w));
						break;
					case VertexShader::ATTRIBTYPE_UINT:
						if(stream.count >= 1) v.x = As<Float4>(UInt4(v.x));
						if(stream.count >= 2) v.x = As<Float4>(UInt4(v.y));
						if(stream.count >= 3) v.x = As<Float4>(UInt4(v.z));
						if(stream.count >= 4) v.x = As<Float4>(UInt4(v.w));
						break;
					default:
						break;
					}
				}
			}
			break;
		case STREAMTYPE_BYTE:
			if(isNativeFloatAttrib) // Stream: UByte, Shader attrib: Float
			{
				v.x = Float4(*Pointer<Byte4>(source0));
				v.y = Float4(*Pointer<Byte4>(source1));
				v.z = Float4(*Pointer<Byte4>(source2));
				v.w = Float4(*Pointer<Byte4>(source3));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);

				if(stream.normalized)
				{
					if(stream.count >= 1) v.x *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleByte));
					if(stream.count >= 2) v.y *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleByte));
					if(stream.count >= 3) v.z *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleByte));
					if(stream.count >= 4) v.w *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleByte));
				}
			}
			else // Stream: UByte, Shader attrib: Int / UInt
			{
				v.x = As<Float4>(Int4(*Pointer<Byte4>(source0)));
				v.y = As<Float4>(Int4(*Pointer<Byte4>(source1)));
				v.z = As<Float4>(Int4(*Pointer<Byte4>(source2)));
				v.w = As<Float4>(Int4(*Pointer<Byte4>(source3)));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);
			}
			break;
		case STREAMTYPE_SBYTE:
			if(isNativeFloatAttrib) // Stream: SByte, Shader attrib: Float
			{
				v.x = Float4(*Pointer<SByte4>(source0));
				v.y = Float4(*Pointer<SByte4>(source1));
				v.z = Float4(*Pointer<SByte4>(source2));
				v.w = Float4(*Pointer<SByte4>(source3));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);

				if(stream.normalized)
				{
					if(stream.count >= 1) v.x *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleSByte));
					if(stream.count >= 2) v.y *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleSByte));
					if(stream.count >= 3) v.z *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleSByte));
					if(stream.count >= 4) v.w *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleSByte));
				}
			}
			else // Stream: SByte, Shader attrib: Int / UInt
			{
				v.x = As<Float4>(Int4(*Pointer<SByte4>(source0)));
				v.y = As<Float4>(Int4(*Pointer<SByte4>(source1)));
				v.z = As<Float4>(Int4(*Pointer<SByte4>(source2)));
				v.w = As<Float4>(Int4(*Pointer<SByte4>(source3)));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);
			}
			break;
		case STREAMTYPE_COLOR:
			{
				v.x = Float4(*Pointer<Byte4>(source0)) * *Pointer<Float4>(constants + OFFSET(Constants,unscaleByte));
				v.y = Float4(*Pointer<Byte4>(source1)) * *Pointer<Float4>(constants + OFFSET(Constants,unscaleByte));
				v.z = Float4(*Pointer<Byte4>(source2)) * *Pointer<Float4>(constants + OFFSET(Constants,unscaleByte));
				v.w = Float4(*Pointer<Byte4>(source3)) * *Pointer<Float4>(constants + OFFSET(Constants,unscaleByte));

				transpose4x4(v.x, v.y, v.z, v.w);

				// Swap red and blue
				Float4 t = v.x;
				v.x = v.z;
				v.z = t;
			}
			break;
		case STREAMTYPE_SHORT:
			if(isNativeFloatAttrib) // Stream: Int, Shader attrib: Float
			{
				v.x = Float4(*Pointer<Short4>(source0));
				v.y = Float4(*Pointer<Short4>(source1));
				v.z = Float4(*Pointer<Short4>(source2));
				v.w = Float4(*Pointer<Short4>(source3));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);

				if(stream.normalized)
				{
					if(stream.count >= 1) v.x *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleShort));
					if(stream.count >= 2) v.y *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleShort));
					if(stream.count >= 3) v.z *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleShort));
					if(stream.count >= 4) v.w *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleShort));
				}
			}
			else // Stream: Short, Shader attrib: Int/UInt, no type conversion
			{
				v.x = As<Float4>(Int4(*Pointer<Short4>(source0)));
				v.y = As<Float4>(Int4(*Pointer<Short4>(source1)));
				v.z = As<Float4>(Int4(*Pointer<Short4>(source2)));
				v.w = As<Float4>(Int4(*Pointer<Short4>(source3)));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);
			}
			break;
		case STREAMTYPE_USHORT:
			if(isNativeFloatAttrib) // Stream: Int, Shader attrib: Float
			{
				v.x = Float4(*Pointer<UShort4>(source0));
				v.y = Float4(*Pointer<UShort4>(source1));
				v.z = Float4(*Pointer<UShort4>(source2));
				v.w = Float4(*Pointer<UShort4>(source3));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);

				if(stream.normalized)
				{
					if(stream.count >= 1) v.x *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleUShort));
					if(stream.count >= 2) v.y *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleUShort));
					if(stream.count >= 3) v.z *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleUShort));
					if(stream.count >= 4) v.w *= *Pointer<Float4>(constants + OFFSET(Constants,unscaleUShort));
				}
			}
			else // Stream: UShort, Shader attrib: Int/UInt, no type conversion
			{
				v.x = As<Float4>(Int4(*Pointer<UShort4>(source0)));
				v.y = As<Float4>(Int4(*Pointer<UShort4>(source1)));
				v.z = As<Float4>(Int4(*Pointer<UShort4>(source2)));
				v.w = As<Float4>(Int4(*Pointer<UShort4>(source3)));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);
			}
			break;
		case STREAMTYPE_INT:
			if(isNativeFloatAttrib) // Stream: Int, Shader attrib: Float
			{
				v.x = Float4(*Pointer<Int4>(source0));
				v.y = Float4(*Pointer<Int4>(source1));
				v.z = Float4(*Pointer<Int4>(source2));
				v.w = Float4(*Pointer<Int4>(source3));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);

				if(stream.normalized)
				{
					if(stream.count >= 1) v.x *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleInt));
					if(stream.count >= 2) v.y *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleInt));
					if(stream.count >= 3) v.z *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleInt));
					if(stream.count >= 4) v.w *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleInt));
				}
			}
			else // Stream: Int, Shader attrib: Int/UInt, no type conversion
			{
				v.x = *Pointer<Float4>(source0);
				v.y = *Pointer<Float4>(source1);
				v.z = *Pointer<Float4>(source2);
				v.w = *Pointer<Float4>(source3);

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);
			}
			break;
		case STREAMTYPE_UINT:
			if(isNativeFloatAttrib) // Stream: UInt, Shader attrib: Float
			{
				v.x = Float4(*Pointer<UInt4>(source0));
				v.y = Float4(*Pointer<UInt4>(source1));
				v.z = Float4(*Pointer<UInt4>(source2));
				v.w = Float4(*Pointer<UInt4>(source3));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);

				if(stream.normalized)
				{
					if(stream.count >= 1) v.x *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleUInt));
					if(stream.count >= 2) v.y *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleUInt));
					if(stream.count >= 3) v.z *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleUInt));
					if(stream.count >= 4) v.w *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleUInt));
				}
			}
			else // Stream: UInt, Shader attrib: Int/UInt, no type conversion
			{
				v.x = *Pointer<Float4>(source0);
				v.y = *Pointer<Float4>(source1);
				v.z = *Pointer<Float4>(source2);
				v.w = *Pointer<Float4>(source3);

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);
			}
			break;
		case STREAMTYPE_UDEC3:
			{
				// FIXME: Vectorize
				{
					Int x, y, z;

					x = y = z = *Pointer<Int>(source0);

					v.x.x = Float(x & 0x000003FF);
					v.x.y = Float(y & 0x000FFC00);
					v.x.z = Float(z & 0x3FF00000);
				}

				{
					Int x, y, z;

					x = y = z = *Pointer<Int>(source1);

					v.y.x = Float(x & 0x000003FF);
					v.y.y = Float(y & 0x000FFC00);
					v.y.z = Float(z & 0x3FF00000);
				}

				{
					Int x, y, z;

					x = y = z = *Pointer<Int>(source2);

					v.z.x = Float(x & 0x000003FF);
					v.z.y = Float(y & 0x000FFC00);
					v.z.z = Float(z & 0x3FF00000);
				}

				{
					Int x, y, z;

					x = y = z = *Pointer<Int>(source3);

					v.w.x = Float(x & 0x000003FF);
					v.w.y = Float(y & 0x000FFC00);
					v.w.z = Float(z & 0x3FF00000);
				}

				transpose4x3(v.x, v.y, v.z, v.w);

				v.y *= Float4(1.0f / 0x00000400);
				v.z *= Float4(1.0f / 0x00100000);
			}
			break;
		case STREAMTYPE_DEC3N:
			{
				// FIXME: Vectorize
				{
					Int x, y, z;

					x = y = z = *Pointer<Int>(source0);

					v.x.x = Float((x << 22) & 0xFFC00000);
					v.x.y = Float((y << 12) & 0xFFC00000);
					v.x.z = Float((z << 2)  & 0xFFC00000);
				}

				{
					Int x, y, z;

					x = y = z = *Pointer<Int>(source1);

					v.y.x = Float((x << 22) & 0xFFC00000);
					v.y.y = Float((y << 12) & 0xFFC00000);
					v.y.z = Float((z << 2)  & 0xFFC00000);
				}

				{
					Int x, y, z;

					x = y = z = *Pointer<Int>(source2);

					v.z.x = Float((x << 22) & 0xFFC00000);
					v.z.y = Float((y << 12) & 0xFFC00000);
					v.z.z = Float((z << 2)  & 0xFFC00000);
				}

				{
					Int x, y, z;

					x = y = z = *Pointer<Int>(source3);

					v.w.x = Float((x << 22) & 0xFFC00000);
					v.w.y = Float((y << 12) & 0xFFC00000);
					v.w.z = Float((z << 2)  & 0xFFC00000);
				}

				transpose4x3(v.x, v.y, v.z, v.w);

				v.x *= Float4(1.0f / 0x00400000 / 511.0f);
				v.y *= Float4(1.0f / 0x00400000 / 511.0f);
				v.z *= Float4(1.0f / 0x00400000 / 511.0f);
			}
			break;
		case STREAMTYPE_FIXED:
			{
				v.x = Float4(*Pointer<Int4>(source0)) * *Pointer<Float4>(constants + OFFSET(Constants,unscaleFixed));
				v.y = Float4(*Pointer<Int4>(source1)) * *Pointer<Float4>(constants + OFFSET(Constants,unscaleFixed));
				v.z = Float4(*Pointer<Int4>(source2)) * *Pointer<Float4>(constants + OFFSET(Constants,unscaleFixed));
				v.w = Float4(*Pointer<Int4>(source3)) * *Pointer<Float4>(constants + OFFSET(Constants,unscaleFixed));

				transpose4xN(v.x, v.y, v.z, v.w, stream.count);
			}
			break;
		case STREAMTYPE_HALF:
			{
				if(stream.count >= 1)
				{
					UShort x0 = *Pointer<UShort>(source0 + 0);
					UShort x1 = *Pointer<UShort>(source1 + 0);
					UShort x2 = *Pointer<UShort>(source2 + 0);
					UShort x3 = *Pointer<UShort>(source3 + 0);

					v.x.x = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(x0) * 4);
					v.x.y = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(x1) * 4);
					v.x.z = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(x2) * 4);
					v.x.w = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(x3) * 4);
				}

				if(stream.count >= 2)
				{
					UShort y0 = *Pointer<UShort>(source0 + 2);
					UShort y1 = *Pointer<UShort>(source1 + 2);
					UShort y2 = *Pointer<UShort>(source2 + 2);
					UShort y3 = *Pointer<UShort>(source3 + 2);

					v.y.x = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(y0) * 4);
					v.y.y = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(y1) * 4);
					v.y.z = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(y2) * 4);
					v.y.w = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(y3) * 4);
				}

				if(stream.count >= 3)
				{
					UShort z0 = *Pointer<UShort>(source0 + 4);
					UShort z1 = *Pointer<UShort>(source1 + 4);
					UShort z2 = *Pointer<UShort>(source2 + 4);
					UShort z3 = *Pointer<UShort>(source3 + 4);

					v.z.x = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(z0) * 4);
					v.z.y = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(z1) * 4);
					v.z.z = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(z2) * 4);
					v.z.w = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(z3) * 4);
				}

				if(stream.count >= 4)
				{
					UShort w0 = *Pointer<UShort>(source0 + 6);
					UShort w1 = *Pointer<UShort>(source1 + 6);
					UShort w2 = *Pointer<UShort>(source2 + 6);
					UShort w3 = *Pointer<UShort>(source3 + 6);

					v.w.x = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(w0) * 4);
					v.w.y = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(w1) * 4);
					v.w.z = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(w2) * 4);
					v.w.w = *Pointer<Float>(constants + OFFSET(Constants,half2float) + Int(w3) * 4);
				}
			}
			break;
		case STREAMTYPE_INDICES:
			{
				v.x.x = *Pointer<Float>(source0);
				v.x.y = *Pointer<Float>(source1);
				v.x.z = *Pointer<Float>(source2);
				v.x.w = *Pointer<Float>(source3);
			}
			break;
		case STREAMTYPE_2_10_10_10_INT:
			{
				Int4 src;
				src = Insert(src, *Pointer<Int>(source0), 0);
				src = Insert(src, *Pointer<Int>(source1), 1);
				src = Insert(src, *Pointer<Int>(source2), 2);
				src = Insert(src, *Pointer<Int>(source3), 3);

				v.x = Float4((src << 22) >> 22);
				v.y = Float4((src << 12) >> 22);
				v.z = Float4((src << 02) >> 22);
				v.w = Float4(src >> 30);

				if(stream.normalized)
				{
					v.x = Max(v.x * Float4(1.0f / 0x1FF), Float4(-1.0f));
					v.y = Max(v.y * Float4(1.0f / 0x1FF), Float4(-1.0f));
					v.z = Max(v.z * Float4(1.0f / 0x1FF), Float4(-1.0f));
					v.w = Max(v.w, Float4(-1.0f));
				}
			}
			break;
		case STREAMTYPE_2_10_10_10_UINT:
			{
				Int4 src;
				src = Insert(src, *Pointer<Int>(source0), 0);
				src = Insert(src, *Pointer<Int>(source1), 1);
				src = Insert(src, *Pointer<Int>(source2), 2);
				src = Insert(src, *Pointer<Int>(source3), 3);

				v.x = Float4(src & Int4(0x3FF));
				v.y = Float4((src >> 10) & Int4(0x3FF));
				v.z = Float4((src >> 20) & Int4(0x3FF));
				v.w = Float4((src >> 30) & Int4(0x3));

				if(stream.normalized)
				{
					v.x *= Float4(1.0f / 0x3FF);
					v.y *= Float4(1.0f / 0x3FF);
					v.z *= Float4(1.0f / 0x3FF);
					v.w *= Float4(1.0f / 0x3);
				}
			}
			break;
		default:
			ASSERT(false);
		}

		if(stream.count < 1) v.x = Float4(0.0f);
		if(stream.count < 2) v.y = Float4(0.0f);
		if(stream.count < 3) v.z = Float4(0.0f);
		if(stream.count < 4) v.w = isNativeFloatAttrib ? As<Float4>(Float4(1.0f)) : As<Float4>(Int4(0));

		return v;
	}

	void VertexRoutine::postTransform()
	{
		int pos = state.positionRegister;

		// Backtransform
		if(state.preTransformed)
		{
			Float4 rhw = Float4(1.0f) / o[pos].w;

			Float4 W = *Pointer<Float4>(data + OFFSET(DrawData,Wx16)) * Float4(1.0f / 16.0f);
			Float4 H = *Pointer<Float4>(data + OFFSET(DrawData,Hx16)) * Float4(1.0f / 16.0f);
			Float4 L = *Pointer<Float4>(data + OFFSET(DrawData,X0x16)) * Float4(1.0f / 16.0f);
			Float4 T = *Pointer<Float4>(data + OFFSET(DrawData,Y0x16)) * Float4(1.0f / 16.0f);

			o[pos].x = (o[pos].x - L) / W * rhw;
			o[pos].y = (o[pos].y - T) / H * rhw;
			o[pos].z = o[pos].z * rhw;
			o[pos].w = rhw;
		}

		if(!halfIntegerCoordinates && !state.preTransformed)
		{
			o[pos].x = o[pos].x + *Pointer<Float4>(data + OFFSET(DrawData,halfPixelX)) * o[pos].w;
			o[pos].y = o[pos].y + *Pointer<Float4>(data + OFFSET(DrawData,halfPixelY)) * o[pos].w;
		}

		if(state.superSampling)
		{
			o[pos].x = o[pos].x + *Pointer<Float4>(data + OFFSET(DrawData,XXXX)) * o[pos].w;
			o[pos].y = o[pos].y + *Pointer<Float4>(data + OFFSET(DrawData,YYYY)) * o[pos].w;
		}
	}

	void VertexRoutine::writeCache(Pointer<Byte> &cacheLine)
	{
		Vector4f v;

		for(int i = 0; i < MAX_VERTEX_OUTPUTS; i++)
		{
			if(state.output[i].write)
			{
				v.x = o[i].x;
				v.y = o[i].y;
				v.z = o[i].z;
				v.w = o[i].w;

				if(state.output[i].xClamp)
				{
					v.x = Max(v.x, Float4(0.0f));
					v.x = Min(v.x, Float4(1.0f));
				}

				if(state.output[i].yClamp)
				{
					v.y = Max(v.y, Float4(0.0f));
					v.y = Min(v.y, Float4(1.0f));
				}

				if(state.output[i].zClamp)
				{
					v.z = Max(v.z, Float4(0.0f));
					v.z = Min(v.z, Float4(1.0f));
				}

				if(state.output[i].wClamp)
				{
					v.w = Max(v.w, Float4(0.0f));
					v.w = Min(v.w, Float4(1.0f));
				}

				if(state.output[i].write == 0x01)
				{
					*Pointer<Float>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 0) = v.x.x;
					*Pointer<Float>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 1) = v.x.y;
					*Pointer<Float>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 2) = v.x.z;
					*Pointer<Float>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 3) = v.x.w;
				}
				else
				{
					if(state.output[i].write == 0x03)
					{
						transpose2x4(v.x, v.y, v.z, v.w);
					}
					else
					{
						transpose4x4(v.x, v.y, v.z, v.w);
					}

					*Pointer<Float4>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 0, 16) = v.x;
					*Pointer<Float4>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 1, 16) = v.y;
					*Pointer<Float4>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 2, 16) = v.z;
					*Pointer<Float4>(cacheLine + OFFSET(Vertex,v[i]) + sizeof(Vertex) * 3, 16) = v.w;
				}
			}
		}

		*Pointer<Int>(cacheLine + OFFSET(Vertex,clipFlags) + sizeof(Vertex) * 0) = (clipFlags >> 0)  & 0x0000000FF;
		*Pointer<Int>(cacheLine + OFFSET(Vertex,clipFlags) + sizeof(Vertex) * 1) = (clipFlags >> 8)  & 0x0000000FF;
		*Pointer<Int>(cacheLine + OFFSET(Vertex,clipFlags) + sizeof(Vertex) * 2) = (clipFlags >> 16) & 0x0000000FF;
		*Pointer<Int>(cacheLine + OFFSET(Vertex,clipFlags) + sizeof(Vertex) * 3) = (clipFlags >> 24) & 0x0000000FF;

		// Viewport transform
		int pos = state.positionRegister;

		v.x = o[pos].x;
		v.y = o[pos].y;
		v.z = o[pos].z;
		v.w = o[pos].w;

		if(symmetricNormalizedDepth)
		{
			v.z = (v.z + v.w) * Float4(0.5f);   // [-1, 1] -> [0, 1]
		}

		Float4 w = As<Float4>(As<Int4>(v.w) | (As<Int4>(CmpEQ(v.w, Float4(0.0f))) & As<Int4>(Float4(1.0f))));
		Float4 rhw = Float4(1.0f) / w;

		v.x = As<Float4>(RoundInt(*Pointer<Float4>(data + OFFSET(DrawData,X0x16)) + v.x * rhw * *Pointer<Float4>(data + OFFSET(DrawData,Wx16))));
		v.y = As<Float4>(RoundInt(*Pointer<Float4>(data + OFFSET(DrawData,Y0x16)) + v.y * rhw * *Pointer<Float4>(data + OFFSET(DrawData,Hx16))));
		v.z = v.z * rhw;
		v.w = rhw;

		transpose4x4(v.x, v.y, v.z, v.w);

		*Pointer<Float4>(cacheLine + OFFSET(Vertex,X) + sizeof(Vertex) * 0, 16) = v.x;
		*Pointer<Float4>(cacheLine + OFFSET(Vertex,X) + sizeof(Vertex) * 1, 16) = v.y;
		*Pointer<Float4>(cacheLine + OFFSET(Vertex,X) + sizeof(Vertex) * 2, 16) = v.z;
		*Pointer<Float4>(cacheLine + OFFSET(Vertex,X) + sizeof(Vertex) * 3, 16) = v.w;
	}

	void VertexRoutine::writeVertex(const Pointer<Byte> &vertex, Pointer<Byte> &cache)
	{
		for(int i = 0; i < MAX_VERTEX_OUTPUTS; i++)
		{
			if(state.output[i].write)
			{
				*Pointer<Int4>(vertex + OFFSET(Vertex,v[i]), 16) = *Pointer<Int4>(cache + OFFSET(Vertex,v[i]), 16);
			}
		}

		*Pointer<Int4>(vertex + OFFSET(Vertex,X)) = *Pointer<Int4>(cache + OFFSET(Vertex,X));
		*Pointer<Int>(vertex + OFFSET(Vertex,clipFlags)) = *Pointer<Int>(cache + OFFSET(Vertex,clipFlags));
	}

	void VertexRoutine::transformFeedback(const Pointer<Byte> &vertex, const UInt &primitiveNumber, const UInt &indexInPrimitive)
	{
		If(indexInPrimitive < state.verticesPerPrimitive)
		{
			UInt tOffset = primitiveNumber * state.verticesPerPrimitive + indexInPrimitive;

			for(int i = 0; i < MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS; i++)
			{
				if(state.transformFeedbackEnabled & (1ULL << i))
				{
					UInt reg = *Pointer<UInt>(data + OFFSET(DrawData, vs.reg[i]));
					UInt row = *Pointer<UInt>(data + OFFSET(DrawData, vs.row[i]));
					UInt col = *Pointer<UInt>(data + OFFSET(DrawData, vs.col[i]));
					UInt str = *Pointer<UInt>(data + OFFSET(DrawData, vs.str[i]));

					Pointer<Byte> t = *Pointer<Pointer<Byte>>(data + OFFSET(DrawData, vs.t[i])) + (tOffset * str * sizeof(float));
					Pointer<Byte> v = vertex + OFFSET(Vertex, v) + reg * sizeof(float);

					For(UInt r = 0, r < row, r++)
					{
						UInt rOffsetX = r * col * sizeof(float);
						UInt rOffset4 = r * sizeof(float4);

						For(UInt c = 0, c < col, c++)
						{
							UInt cOffset = c * sizeof(float);
							*Pointer<Float>(t + rOffsetX + cOffset) = *Pointer<Float>(v + rOffset4 + cOffset);
						}
					}
				}
			}
		}
	}
}
