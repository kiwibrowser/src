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

#ifndef sw_PixelPipeline_hpp
#define sw_PixelPipeline_hpp

#include "PixelRoutine.hpp"

namespace sw
{
	class PixelPipeline : public PixelRoutine
	{
	public:
		PixelPipeline(const PixelProcessor::State &state, const PixelShader *shader) :
			PixelRoutine(state, shader), current(rs[0]), diffuse(vs[0]), specular(vs[1]), perturbate(false), luminance(false), previousScaling(false) {}
		virtual ~PixelPipeline() {}

	protected:
		virtual void setBuiltins(Int &x, Int &y, Float4(&z)[4], Float4 &w);
		virtual void applyShader(Int cMask[4]);
		virtual Bool alphaTest(Int cMask[4]);
		virtual void rasterOperation(Float4 &fog, Pointer<Byte> cBuffer[4], Int &x, Int sMask[4], Int zMask[4], Int cMask[4]);

	private:
		Vector4s &current;
		Vector4s &diffuse;
		Vector4s &specular;

		Vector4s rs[6];
		Vector4s vs[2];
		Vector4s ts[6];

		// bem(l) offsets and luminance
		Float4 du;
		Float4 dv;
		Short4 L;

		// texm3x3 temporaries
		Float4 u_; // FIXME
		Float4 v_; // FIXME
		Float4 w_; // FIXME
		Float4 U;  // FIXME
		Float4 V;  // FIXME
		Float4 W;  // FIXME

		void fixedFunction();
		void blendTexture(Vector4s &temp, Vector4s &texture, int stage);
		void fogBlend(Vector4s &current, Float4 &fog);
		void specularPixel(Vector4s &current, Vector4s &specular);

		Vector4s sampleTexture(int coordinates, int sampler, bool project = false);
		Vector4s sampleTexture(int sampler, Float4 &u, Float4 &v, Float4 &w, Float4 &q, bool project = false);

		Short4 convertFixed12(RValue<Float4> cf);
		void convertFixed12(Vector4s &cs, Vector4f &cf);
		Float4 convertSigned12(Short4 &cs);
		void convertSigned12(Vector4f &cf, Vector4s &cs);

		void writeDestination(Vector4s &d, const Dst &dst);
		Vector4s fetchRegister(const Src &src);

		// Instructions
		void MOV(Vector4s &dst, Vector4s &src0);
		void ADD(Vector4s &dst, Vector4s &src0, Vector4s &src1);
		void SUB(Vector4s &dst, Vector4s &src0, Vector4s &src1);
		void MAD(Vector4s &dst, Vector4s &src0, Vector4s &src1, Vector4s &src2);
		void MUL(Vector4s &dst, Vector4s &src0, Vector4s &src1);
		void DP3(Vector4s &dst, Vector4s &src0, Vector4s &src1);
		void DP4(Vector4s &dst, Vector4s &src0, Vector4s &src1);
		void LRP(Vector4s &dst, Vector4s &src0, Vector4s &src1, Vector4s &src2);
		void TEXCOORD(Vector4s &dst, Float4 &u, Float4 &v, Float4 &s, int coordinate);
		void TEXCRD(Vector4s &dst, Float4 &u, Float4 &v, Float4 &s, int coordinate, bool project);
		void TEXDP3(Vector4s &dst, Float4 &u, Float4 &v, Float4 &s, Vector4s &src);
		void TEXDP3TEX(Vector4s &dst, Float4 &u, Float4 &v, Float4 &s, int stage, Vector4s &src0);
		void TEXKILL(Int cMask[4], Float4 &u, Float4 &v, Float4 &s);
		void TEXKILL(Int cMask[4], Vector4s &dst);
		void TEX(Vector4s &dst, Float4 &u, Float4 &v, Float4 &s, int stage, bool project);
		void TEXLD(Vector4s &dst, Vector4s &src, int stage, bool project);
		void TEXBEM(Vector4s &dst, Vector4s &src, Float4 &u, Float4 &v, Float4 &s, int stage);
		void TEXBEML(Vector4s &dst, Vector4s &src, Float4 &u, Float4 &v, Float4 &s, int stage);
		void TEXREG2AR(Vector4s &dst, Vector4s &src0, int stage);
		void TEXREG2GB(Vector4s &dst, Vector4s &src0, int stage);
		void TEXREG2RGB(Vector4s &dst, Vector4s &src0, int stage);
		void TEXM3X2DEPTH(Vector4s &dst, Float4 &u, Float4 &v, Float4 &s, Vector4s &src, bool signedScaling);
		void TEXM3X2PAD(Float4 &u, Float4 &v, Float4 &s, Vector4s &src0, int component, bool signedScaling);
		void TEXM3X2TEX(Vector4s &dst, Float4 &u, Float4 &v, Float4 &s, int stage, Vector4s &src0, bool signedScaling);
		void TEXM3X3(Vector4s &dst, Float4 &u, Float4 &v, Float4 &s, Vector4s &src0, bool signedScaling);
		void TEXM3X3PAD(Float4 &u, Float4 &v, Float4 &s, Vector4s &src0, int component, bool signedScaling);
		void TEXM3X3SPEC(Vector4s &dst, Float4 &u, Float4 &v, Float4 &s, int stage, Vector4s &src0, Vector4s &src1);
		void TEXM3X3TEX(Vector4s &dst, Float4 &u, Float4 &v, Float4 &s, int stage, Vector4s &src0, bool singedScaling);
		void TEXM3X3VSPEC(Vector4s &dst, Float4 &u, Float4 &v, Float4 &s, int stage, Vector4s &src0);
		void TEXDEPTH();
		void CND(Vector4s &dst, Vector4s &src0, Vector4s &src1, Vector4s &src2);
		void CMP(Vector4s &dst, Vector4s &src0, Vector4s &src1, Vector4s &src2);
		void BEM(Vector4s &dst, Vector4s &src0, Vector4s &src1, int stage);

		bool perturbate;
		bool luminance;
		bool previousScaling;
	};
}

#endif
