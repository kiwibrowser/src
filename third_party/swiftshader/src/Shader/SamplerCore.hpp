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

#ifndef sw_SamplerCore_hpp
#define sw_SamplerCore_hpp

#include "PixelRoutine.hpp"
#include "Reactor/Reactor.hpp"

namespace sw
{
	enum SamplerMethod
	{
		Implicit,  // Compute gradients (pixel shader only).
		Bias,      // Compute gradients and add provided bias.
		Lod,       // Use provided LOD.
		Grad,      // Use provided gradients.
		Fetch,     // Use provided integer coordinates.
		Base       // Sample base level.
	};

	enum SamplerOption
	{
		None,
		Offset   // Offset sample location by provided integer coordinates.
	};

	struct SamplerFunction
	{
		SamplerFunction(SamplerMethod method, SamplerOption option = None) : method(method), option(option) {}
		operator SamplerMethod() { return method; }

		const SamplerMethod method;
		const SamplerOption option;
 	};

	class SamplerCore
	{
	public:
		SamplerCore(Pointer<Byte> &constants, const Sampler::State &state);

		Vector4s sampleTexture(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Float4 &bias, Vector4f &dsx, Vector4f &dsy);
		Vector4f sampleTexture(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Float4 &bias, Vector4f &dsx, Vector4f &dsy, Vector4f &offset, SamplerFunction function);
		static Vector4f textureSize(Pointer<Byte> &mipmap, Float4 &lod);

	private:
		Vector4s sampleTexture(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Float4 &bias, Vector4f &dsx, Vector4f &dsy, Vector4f &offset, SamplerFunction function, bool fixed12);

		void border(Short4 &mask, Float4 &coordinates);
		void border(Int4 &mask, Float4 &coordinates);
		Short4 offsetSample(Short4 &uvw, Pointer<Byte> &mipmap, int halfOffset, bool wrap, int count, Float &lod);
		Vector4s sampleFilter(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Vector4f &offset, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Int face[4], SamplerFunction function);
		Vector4s sampleAniso(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Vector4f &offset, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Int face[4], bool secondLOD, SamplerFunction function);
		Vector4s sampleQuad(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Vector4f &offset, Float &lod, Int face[4], bool secondLOD, SamplerFunction function);
		Vector4s sampleQuad2D(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Vector4f &offset, Float &lod, Int face[4], bool secondLOD, SamplerFunction function);
		Vector4s sample3D(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Vector4f &offset, Float &lod, bool secondLOD, SamplerFunction function);
		Vector4f sampleFloatFilter(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Vector4f &offset, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Int face[4], SamplerFunction function);
		Vector4f sampleFloatAniso(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Vector4f &offset, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Int face[4], bool secondLOD, SamplerFunction function);
		Vector4f sampleFloat(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Vector4f &offset, Float &lod, Int face[4], bool secondLOD, SamplerFunction function);
		Vector4f sampleFloat2D(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Float4 &q, Vector4f &offset, Float &lod, Int face[4], bool secondLOD, SamplerFunction function);
		Vector4f sampleFloat3D(Pointer<Byte> &texture, Float4 &u, Float4 &v, Float4 &w, Vector4f &offset, Float &lod, bool secondLOD, SamplerFunction function);
		Float log2sqrt(Float lod);
		Float log2(Float lod);
		void computeLod(Pointer<Byte> &texture, Float &lod, Float &anisotropy, Float4 &uDelta, Float4 &vDelta, Float4 &u, Float4 &v, const Float &lodBias, Vector4f &dsx, Vector4f &dsy, SamplerFunction function);
		void computeLodCube(Pointer<Byte> &texture, Float &lod, Float4 &u, Float4 &v, Float4 &w, const Float &lodBias, Vector4f &dsx, Vector4f &dsy, Float4 &M, SamplerFunction function);
		void computeLod3D(Pointer<Byte> &texture, Float &lod, Float4 &u, Float4 &v, Float4 &w, const Float &lodBias, Vector4f &dsx, Vector4f &dsy, SamplerFunction function);
		void cubeFace(Int face[4], Float4 &U, Float4 &V, Float4 &x, Float4 &y, Float4 &z, Float4 &M);
		Short4 applyOffset(Short4 &uvw, Float4 &offset, const Int4 &whd, AddressingMode mode);
		void computeIndices(UInt index[4], Short4 uuuu, Short4 vvvv, Short4 wwww, Vector4f &offset, const Pointer<Byte> &mipmap, SamplerFunction function);
		void computeIndices(UInt index[4], Int4& uuuu, Int4& vvvv, Int4& wwww, const Pointer<Byte> &mipmap, SamplerFunction function);
		Vector4s sampleTexel(Short4 &u, Short4 &v, Short4 &s, Vector4f &offset, Pointer<Byte> &mipmap, Pointer<Byte> buffer[4], SamplerFunction function);
		Vector4s sampleTexel(UInt index[4], Pointer<Byte> buffer[4]);
		Vector4f sampleTexel(Int4 &u, Int4 &v, Int4 &s, Float4 &z, Pointer<Byte> &mipmap, Pointer<Byte> buffer[4], SamplerFunction function);
		void selectMipmap(Pointer<Byte> &texture, Pointer<Byte> buffer[4], Pointer<Byte> &mipmap, Float &lod, Int face[4], bool secondLOD);
		Short4 address(Float4 &uw, AddressingMode addressingMode, Pointer<Byte>& mipmap);
		void address(Float4 &uw, Int4& xyz0, Int4& xyz1, Float4& f, Pointer<Byte>& mipmap, Float4 &texOffset, Int4 &filter, int whd, AddressingMode addressingMode, SamplerFunction function);
		Int4 computeFilterOffset(Float &lod);

		void convertFixed12(Short4 &ci, Float4 &cf);
		void convertFixed12(Vector4s &cs, Vector4f &cf);
		void convertSigned12(Float4 &cf, Short4 &ci);
		void convertSigned15(Float4 &cf, Short4 &ci);
		void convertUnsigned16(Float4 &cf, Short4 &ci);
		void sRGBtoLinear16_8_16(Short4 &c);
		void sRGBtoLinear16_6_16(Short4 &c);
		void sRGBtoLinear16_5_16(Short4 &c);

		bool hasFloatTexture() const;
		bool hasUnnormalizedIntegerTexture() const;
		bool hasUnsignedTextureComponent(int component) const;
		int textureComponentCount() const;
		bool hasThirdCoordinate() const;
		bool has16bitTextureFormat() const;
		bool has8bitTextureComponents() const;
		bool has16bitTextureComponents() const;
		bool has32bitIntegerTextureComponents() const;
		bool hasYuvFormat() const;
		bool isRGBComponent(int component) const;

		Pointer<Byte> &constants;
		const Sampler::State &state;
	};
}

#endif   // sw_SamplerCore_hpp
