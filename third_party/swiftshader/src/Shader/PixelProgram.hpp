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

#ifndef sw_PixelProgram_hpp
#define sw_PixelProgram_hpp

#include "PixelRoutine.hpp"
#include "SamplerCore.hpp"

namespace sw
{
	class PixelProgram : public PixelRoutine
	{
	public:
		PixelProgram(const PixelProcessor::State &state, const PixelShader *shader) :
			PixelRoutine(state, shader), r(shader->dynamicallyIndexedTemporaries),
			loopDepth(-1), ifDepth(0), loopRepDepth(0), currentLabel(-1), whileTest(false)
		{
			for(int i = 0; i < 2048; ++i)
			{
				labelBlock[i] = 0;
			}

			enableStack[0] = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);

			if(shader->containsBreakInstruction())
			{
				enableBreak = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
			}

			if(shader->containsContinueInstruction())
			{
				enableContinue = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
			}
		}

		virtual ~PixelProgram() {}

	protected:
		virtual void setBuiltins(Int &x, Int &y, Float4(&z)[4], Float4 &w);
		virtual void applyShader(Int cMask[4]);
		virtual Bool alphaTest(Int cMask[4]);
		virtual void rasterOperation(Float4 &fog, Pointer<Byte> cBuffer[4], Int &x, Int sMask[4], Int zMask[4], Int cMask[4]);

	private:
		// Temporary registers
		RegisterArray<NUM_TEMPORARY_REGISTERS> r;

		// Color outputs
		Vector4f c[RENDERTARGETS];
		RegisterArray<RENDERTARGETS, true> oC;

		// Shader variables
		Vector4f vPos;
		Vector4f vFace;

		// DX9 specific variables
		Vector4f p0;
		Array<Int, 4> aL;
		Array<Int, 4> increment;
		Array<Int, 4> iteration;

		Int loopDepth;    // FIXME: Add support for switch
		Int stackIndex;   // FIXME: Inc/decrement callStack
		Array<UInt, 16> callStack;

		// Per pixel based on conditions reached
		Int enableIndex;
		Array<Int4, 1 + 24> enableStack;
		Int4 enableBreak;
		Int4 enableContinue;
		Int4 enableLeave;

		Vector4f sampleTexture(const Src &sampler, Vector4f &uvwq, Float4 &bias, Vector4f &dsx, Vector4f &dsy, Vector4f &offset, SamplerFunction function);
		Vector4f sampleTexture(int samplerIndex, Vector4f &uvwq, Float4 &bias, Vector4f &dsx, Vector4f &dsy, Vector4f &offset, SamplerFunction function);

		// Raster operations
		void clampColor(Vector4f oC[RENDERTARGETS]);

		Int4 enableMask(const Shader::Instruction *instruction);

		Vector4f fetchRegister(const Src &src, unsigned int offset = 0);
		Vector4f readConstant(const Src &src, unsigned int offset = 0);
		RValue<Pointer<Byte>> uniformAddress(int bufferIndex, unsigned int index);
		RValue<Pointer<Byte>> uniformAddress(int bufferIndex, unsigned int index, Int& offset);
		Int relativeAddress(const Shader::Parameter &var, int bufferIndex = -1);

		Float4 linearToSRGB(const Float4 &x);

		// Instructions
		typedef Shader::Control Control;

		void M3X2(Vector4f &dst, Vector4f &src0, const Src &src1);
		void M3X3(Vector4f &dst, Vector4f &src0, const Src &src1);
		void M3X4(Vector4f &dst, Vector4f &src0, const Src &src1);
		void M4X3(Vector4f &dst, Vector4f &src0, const Src &src1);
		void M4X4(Vector4f &dst, Vector4f &src0, const Src &src1);
		void TEX(Vector4f &dst, Vector4f &src0, const Src &src1, bool project, bool bias);
		void TEXLOD(Vector4f &dst, Vector4f &src0, const Src &src1, Float4 &lod);
		void TEXBIAS(Vector4f &dst, Vector4f &src0, const Src &src1, Float4 &bias);
		void TEXSIZE(Vector4f &dst, Float4 &lod, const Src &src1);
		void TEXKILL(Int cMask[4], Vector4f &src, unsigned char mask);
		void TEXOFFSET(Vector4f &dst, Vector4f &src0, const Src &src1, Vector4f &offset);
		void TEXOFFSETBIAS(Vector4f &dst, Vector4f &src0, const Src &src1, Vector4f &offset, Float4 &bias);
		void TEXLODOFFSET(Vector4f &dst, Vector4f &src0, const Src &src1, Vector4f &offset, Float4 &lod);
		void TEXELFETCH(Vector4f &dst, Vector4f &src, const Src &, Float4 &lod);
		void TEXELFETCHOFFSET(Vector4f &dst, Vector4f &src, const Src &, Vector4f &offset, Float4 &lod);
		void TEXGRAD(Vector4f &dst, Vector4f &src0, const Src &src1, Vector4f &dsx, Vector4f &dsy);
		void TEXGRADOFFSET(Vector4f &dst, Vector4f &src, const Src &, Vector4f &dsx, Vector4f &dsy, Vector4f &offset);
		void DISCARD(Int cMask[4], const Shader::Instruction *instruction);
		void DFDX(Vector4f &dst, Vector4f &src);
		void DFDY(Vector4f &dst, Vector4f &src);
		void FWIDTH(Vector4f &dst, Vector4f &src);
		void BREAK();
		void BREAKC(Vector4f &src0, Vector4f &src1, Control);
		void BREAKP(const Src &predicateRegister);
		void BREAK(Int4 &condition);
		void CONTINUE();
		void TEST();
		void CALL(int labelIndex, int callSiteIndex);
		void CALLNZ(int labelIndex, int callSiteIndex, const Src &src);
		void CALLNZb(int labelIndex, int callSiteIndex, const Src &boolRegister);
		void CALLNZp(int labelIndex, int callSiteIndex, const Src &predicateRegister);
		void ELSE();
		void ENDIF();
		void ENDLOOP();
		void ENDREP();
		void ENDWHILE();
		void ENDSWITCH();
		void IF(const Src &src);
		void IFb(const Src &boolRegister);
		void IFp(const Src &predicateRegister);
		void IFC(Vector4f &src0, Vector4f &src1, Control);
		void IF(Int4 &condition);
		void LABEL(int labelIndex);
		void LOOP(const Src &integerRegister);
		void REP(const Src &integerRegister);
		void WHILE(const Src &temporaryRegister);
		void SWITCH();
		void RET();
		void LEAVE();

		int ifDepth;
		int loopRepDepth;
		int currentLabel;
		bool whileTest;

		BasicBlock *ifFalseBlock[24 + 24];
		BasicBlock *loopRepTestBlock[4];
		BasicBlock *loopRepEndBlock[4];
		BasicBlock *labelBlock[2048];
		std::vector<BasicBlock*> callRetBlock[2048];
		BasicBlock *returnBlock;
		bool isConditionalIf[24 + 24];
	};
}

#endif
