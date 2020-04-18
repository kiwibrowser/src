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

#include "PixelProcessor.hpp"

#include "Surface.hpp"
#include "Primitive.hpp"
#include "Shader/PixelPipeline.hpp"
#include "Shader/PixelProgram.hpp"
#include "Shader/PixelShader.hpp"
#include "Shader/Constants.hpp"
#include "Common/Debug.hpp"

#include <string.h>

namespace sw
{
	extern bool complementaryDepthBuffer;
	extern TransparencyAntialiasing transparencyAntialiasing;
	extern bool perspectiveCorrection;

	bool precachePixel = false;

	unsigned int PixelProcessor::States::computeHash()
	{
		unsigned int *state = (unsigned int*)this;
		unsigned int hash = 0;

		for(unsigned int i = 0; i < sizeof(States) / 4; i++)
		{
			hash ^= state[i];
		}

		return hash;
	}

	PixelProcessor::State::State()
	{
		memset(this, 0, sizeof(State));
	}

	bool PixelProcessor::State::operator==(const State &state) const
	{
		if(hash != state.hash)
		{
			return false;
		}

		return memcmp(static_cast<const States*>(this), static_cast<const States*>(&state), sizeof(States)) == 0;
	}

	PixelProcessor::UniformBufferInfo::UniformBufferInfo()
	{
		buffer = nullptr;
		offset = 0;
	}

	PixelProcessor::PixelProcessor(Context *context) : context(context)
	{
		setGlobalMipmapBias(0.0f);   // Round to highest LOD [0.5, 1.0]: -0.5
		                             // Round to nearest LOD [0.7, 1.4]:  0.0
		                             // Round to lowest LOD  [1.0, 2.0]:  0.5

		routineCache = 0;
		setRoutineCacheSize(1024);
	}

	PixelProcessor::~PixelProcessor()
	{
		delete routineCache;
		routineCache = 0;
	}

	void PixelProcessor::setFloatConstant(unsigned int index, const float value[4])
	{
		if(index < FRAGMENT_UNIFORM_VECTORS)
		{
			c[index][0] = value[0];
			c[index][1] = value[1];
			c[index][2] = value[2];
			c[index][3] = value[3];
		}
		else ASSERT(false);

		if(index < 8)   // ps_1_x constants
		{
			// FIXME: Compact into generic function
			short x = iround(4095 * clamp(value[0], -1.0f, 1.0f));
			short y = iround(4095 * clamp(value[1], -1.0f, 1.0f));
			short z = iround(4095 * clamp(value[2], -1.0f, 1.0f));
			short w = iround(4095 * clamp(value[3], -1.0f, 1.0f));

			cW[index][0][0] = x;
			cW[index][0][1] = x;
			cW[index][0][2] = x;
			cW[index][0][3] = x;

			cW[index][1][0] = y;
			cW[index][1][1] = y;
			cW[index][1][2] = y;
			cW[index][1][3] = y;

			cW[index][2][0] = z;
			cW[index][2][1] = z;
			cW[index][2][2] = z;
			cW[index][2][3] = z;

			cW[index][3][0] = w;
			cW[index][3][1] = w;
			cW[index][3][2] = w;
			cW[index][3][3] = w;
		}
	}

	void PixelProcessor::setIntegerConstant(unsigned int index, const int value[4])
	{
		if(index < 16)
		{
			i[index][0] = value[0];
			i[index][1] = value[1];
			i[index][2] = value[2];
			i[index][3] = value[3];
		}
		else ASSERT(false);
	}

	void PixelProcessor::setBooleanConstant(unsigned int index, int boolean)
	{
		if(index < 16)
		{
			b[index] = boolean != 0;
		}
		else ASSERT(false);
	}

	void PixelProcessor::setUniformBuffer(int index, sw::Resource* buffer, int offset)
	{
		uniformBufferInfo[index].buffer = buffer;
		uniformBufferInfo[index].offset = offset;
	}

	void PixelProcessor::lockUniformBuffers(byte** u, sw::Resource* uniformBuffers[])
	{
		for(int i = 0; i < MAX_UNIFORM_BUFFER_BINDINGS; ++i)
		{
			u[i] = uniformBufferInfo[i].buffer ? static_cast<byte*>(uniformBufferInfo[i].buffer->lock(PUBLIC, PRIVATE)) + uniformBufferInfo[i].offset : nullptr;
			uniformBuffers[i] = uniformBufferInfo[i].buffer;
		}
	}

	void PixelProcessor::setRenderTarget(int index, Surface *renderTarget, unsigned int layer)
	{
		context->renderTarget[index] = renderTarget;
		context->renderTargetLayer[index] = layer;
	}

	void PixelProcessor::setDepthBuffer(Surface *depthBuffer, unsigned int layer)
	{
		context->depthBuffer = depthBuffer;
		context->depthBufferLayer = layer;
	}

	void PixelProcessor::setStencilBuffer(Surface *stencilBuffer, unsigned int layer)
	{
		context->stencilBuffer = stencilBuffer;
		context->stencilBufferLayer = layer;
	}

	void PixelProcessor::setTexCoordIndex(unsigned int stage, int texCoordIndex)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setTexCoordIndex(texCoordIndex);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setStageOperation(unsigned int stage, TextureStage::StageOperation stageOperation)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setStageOperation(stageOperation);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setFirstArgument(unsigned int stage, TextureStage::SourceArgument firstArgument)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setFirstArgument(firstArgument);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setSecondArgument(unsigned int stage, TextureStage::SourceArgument secondArgument)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setSecondArgument(secondArgument);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setThirdArgument(unsigned int stage, TextureStage::SourceArgument thirdArgument)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setThirdArgument(thirdArgument);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setStageOperationAlpha(unsigned int stage, TextureStage::StageOperation stageOperationAlpha)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setStageOperationAlpha(stageOperationAlpha);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setFirstArgumentAlpha(unsigned int stage, TextureStage::SourceArgument firstArgumentAlpha)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setFirstArgumentAlpha(firstArgumentAlpha);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setSecondArgumentAlpha(unsigned int stage, TextureStage::SourceArgument secondArgumentAlpha)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setSecondArgumentAlpha(secondArgumentAlpha);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setThirdArgumentAlpha(unsigned int stage, TextureStage::SourceArgument thirdArgumentAlpha)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setThirdArgumentAlpha(thirdArgumentAlpha);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setFirstModifier(unsigned int stage, TextureStage::ArgumentModifier firstModifier)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setFirstModifier(firstModifier);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setSecondModifier(unsigned int stage, TextureStage::ArgumentModifier secondModifier)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setSecondModifier(secondModifier);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setThirdModifier(unsigned int stage, TextureStage::ArgumentModifier thirdModifier)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setThirdModifier(thirdModifier);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setFirstModifierAlpha(unsigned int stage, TextureStage::ArgumentModifier firstModifierAlpha)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setFirstModifierAlpha(firstModifierAlpha);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setSecondModifierAlpha(unsigned int stage, TextureStage::ArgumentModifier secondModifierAlpha)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setSecondModifierAlpha(secondModifierAlpha);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setThirdModifierAlpha(unsigned int stage, TextureStage::ArgumentModifier thirdModifierAlpha)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setThirdModifierAlpha(thirdModifierAlpha);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setDestinationArgument(unsigned int stage, TextureStage::DestinationArgument destinationArgument)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setDestinationArgument(destinationArgument);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setConstantColor(unsigned int stage, const Color<float> &constantColor)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setConstantColor(constantColor);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setBumpmapMatrix(unsigned int stage, int element, float value)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setBumpmapMatrix(element, value);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setLuminanceScale(unsigned int stage, float value)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setLuminanceScale(value);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setLuminanceOffset(unsigned int stage, float value)
	{
		if(stage < 8)
		{
			context->textureStage[stage].setLuminanceOffset(value);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setTextureFilter(unsigned int sampler, FilterType textureFilter)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setTextureFilter(textureFilter);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setMipmapFilter(unsigned int sampler, MipmapType mipmapFilter)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setMipmapFilter(mipmapFilter);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setGatherEnable(unsigned int sampler, bool enable)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setGatherEnable(enable);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setAddressingModeU(unsigned int sampler, AddressingMode addressMode)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setAddressingModeU(addressMode);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setAddressingModeV(unsigned int sampler, AddressingMode addressMode)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setAddressingModeV(addressMode);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setAddressingModeW(unsigned int sampler, AddressingMode addressMode)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setAddressingModeW(addressMode);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setReadSRGB(unsigned int sampler, bool sRGB)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setReadSRGB(sRGB);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setMipmapLOD(unsigned int sampler, float bias)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setMipmapLOD(bias);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setBorderColor(unsigned int sampler, const Color<float> &borderColor)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setBorderColor(borderColor);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setMaxAnisotropy(unsigned int sampler, float maxAnisotropy)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setMaxAnisotropy(maxAnisotropy);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setHighPrecisionFiltering(unsigned int sampler, bool highPrecisionFiltering)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setHighPrecisionFiltering(highPrecisionFiltering);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setSwizzleR(unsigned int sampler, SwizzleType swizzleR)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setSwizzleR(swizzleR);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setSwizzleG(unsigned int sampler, SwizzleType swizzleG)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setSwizzleG(swizzleG);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setSwizzleB(unsigned int sampler, SwizzleType swizzleB)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setSwizzleB(swizzleB);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setSwizzleA(unsigned int sampler, SwizzleType swizzleA)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setSwizzleA(swizzleA);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setCompareFunc(unsigned int sampler, CompareFunc compFunc)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setCompareFunc(compFunc);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setBaseLevel(unsigned int sampler, int baseLevel)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setBaseLevel(baseLevel);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setMaxLevel(unsigned int sampler, int maxLevel)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setMaxLevel(maxLevel);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setMinLod(unsigned int sampler, float minLod)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setMinLod(minLod);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setMaxLod(unsigned int sampler, float maxLod)
	{
		if(sampler < TEXTURE_IMAGE_UNITS)
		{
			context->sampler[sampler].setMaxLod(maxLod);
		}
		else ASSERT(false);
	}

	void PixelProcessor::setWriteSRGB(bool sRGB)
	{
		context->setWriteSRGB(sRGB);
	}

	void PixelProcessor::setColorLogicOpEnabled(bool colorLogicOpEnabled)
	{
		context->setColorLogicOpEnabled(colorLogicOpEnabled);
	}

	void PixelProcessor::setLogicalOperation(LogicalOperation logicalOperation)
	{
		context->setLogicalOperation(logicalOperation);
	}

	void PixelProcessor::setDepthBufferEnable(bool depthBufferEnable)
	{
		context->setDepthBufferEnable(depthBufferEnable);
	}

	void PixelProcessor::setDepthCompare(DepthCompareMode depthCompareMode)
	{
		context->depthCompareMode = depthCompareMode;
	}

	void PixelProcessor::setAlphaCompare(AlphaCompareMode alphaCompareMode)
	{
		context->alphaCompareMode = alphaCompareMode;
	}

	void PixelProcessor::setDepthWriteEnable(bool depthWriteEnable)
	{
		context->depthWriteEnable = depthWriteEnable;
	}

	void PixelProcessor::setAlphaTestEnable(bool alphaTestEnable)
	{
		context->alphaTestEnable = alphaTestEnable;
	}

	void PixelProcessor::setCullMode(CullMode cullMode)
	{
		context->cullMode = cullMode;
	}

	void PixelProcessor::setColorWriteMask(int index, int rgbaMask)
	{
		context->setColorWriteMask(index, rgbaMask);
	}

	void PixelProcessor::setStencilEnable(bool stencilEnable)
	{
		context->stencilEnable = stencilEnable;
	}

	void PixelProcessor::setStencilCompare(StencilCompareMode stencilCompareMode)
	{
		context->stencilCompareMode = stencilCompareMode;
	}

	void PixelProcessor::setStencilReference(int stencilReference)
	{
		context->stencilReference = stencilReference;
		stencil.set(stencilReference, context->stencilMask, context->stencilWriteMask);
	}

	void PixelProcessor::setStencilReferenceCCW(int stencilReferenceCCW)
	{
		context->stencilReferenceCCW = stencilReferenceCCW;
		stencilCCW.set(stencilReferenceCCW, context->stencilMaskCCW, context->stencilWriteMaskCCW);
	}

	void PixelProcessor::setStencilMask(int stencilMask)
	{
		context->stencilMask = stencilMask;
		stencil.set(context->stencilReference, stencilMask, context->stencilWriteMask);
	}

	void PixelProcessor::setStencilMaskCCW(int stencilMaskCCW)
	{
		context->stencilMaskCCW = stencilMaskCCW;
		stencilCCW.set(context->stencilReferenceCCW, stencilMaskCCW, context->stencilWriteMaskCCW);
	}

	void PixelProcessor::setStencilFailOperation(StencilOperation stencilFailOperation)
	{
		context->stencilFailOperation = stencilFailOperation;
	}

	void PixelProcessor::setStencilPassOperation(StencilOperation stencilPassOperation)
	{
		context->stencilPassOperation = stencilPassOperation;
	}

	void PixelProcessor::setStencilZFailOperation(StencilOperation stencilZFailOperation)
	{
		context->stencilZFailOperation = stencilZFailOperation;
	}

	void PixelProcessor::setStencilWriteMask(int stencilWriteMask)
	{
		context->stencilWriteMask = stencilWriteMask;
		stencil.set(context->stencilReference, context->stencilMask, stencilWriteMask);
	}

	void PixelProcessor::setStencilWriteMaskCCW(int stencilWriteMaskCCW)
	{
		context->stencilWriteMaskCCW = stencilWriteMaskCCW;
		stencilCCW.set(context->stencilReferenceCCW, context->stencilMaskCCW, stencilWriteMaskCCW);
	}

	void PixelProcessor::setTwoSidedStencil(bool enable)
	{
		context->twoSidedStencil = enable;
	}

	void PixelProcessor::setStencilCompareCCW(StencilCompareMode stencilCompareMode)
	{
		context->stencilCompareModeCCW = stencilCompareMode;
	}

	void PixelProcessor::setStencilFailOperationCCW(StencilOperation stencilFailOperation)
	{
		context->stencilFailOperationCCW = stencilFailOperation;
	}

	void PixelProcessor::setStencilPassOperationCCW(StencilOperation stencilPassOperation)
	{
		context->stencilPassOperationCCW = stencilPassOperation;
	}

	void PixelProcessor::setStencilZFailOperationCCW(StencilOperation stencilZFailOperation)
	{
		context->stencilZFailOperationCCW = stencilZFailOperation;
	}

	void PixelProcessor::setTextureFactor(const Color<float> &textureFactor)
	{
		// FIXME: Compact into generic function   // FIXME: Clamp
		short textureFactorR = iround(4095 * textureFactor.r);
		short textureFactorG = iround(4095 * textureFactor.g);
		short textureFactorB = iround(4095 * textureFactor.b);
		short textureFactorA = iround(4095 * textureFactor.a);

		factor.textureFactor4[0][0] = textureFactorR;
		factor.textureFactor4[0][1] = textureFactorR;
		factor.textureFactor4[0][2] = textureFactorR;
		factor.textureFactor4[0][3] = textureFactorR;

		factor.textureFactor4[1][0] = textureFactorG;
		factor.textureFactor4[1][1] = textureFactorG;
		factor.textureFactor4[1][2] = textureFactorG;
		factor.textureFactor4[1][3] = textureFactorG;

		factor.textureFactor4[2][0] = textureFactorB;
		factor.textureFactor4[2][1] = textureFactorB;
		factor.textureFactor4[2][2] = textureFactorB;
		factor.textureFactor4[2][3] = textureFactorB;

		factor.textureFactor4[3][0] = textureFactorA;
		factor.textureFactor4[3][1] = textureFactorA;
		factor.textureFactor4[3][2] = textureFactorA;
		factor.textureFactor4[3][3] = textureFactorA;
	}

	void PixelProcessor::setBlendConstant(const Color<float> &blendConstant)
	{
		// FIXME: Compact into generic function   // FIXME: Clamp
		short blendConstantR = iround(65535 * blendConstant.r);
		short blendConstantG = iround(65535 * blendConstant.g);
		short blendConstantB = iround(65535 * blendConstant.b);
		short blendConstantA = iround(65535 * blendConstant.a);

		factor.blendConstant4W[0][0] = blendConstantR;
		factor.blendConstant4W[0][1] = blendConstantR;
		factor.blendConstant4W[0][2] = blendConstantR;
		factor.blendConstant4W[0][3] = blendConstantR;

		factor.blendConstant4W[1][0] = blendConstantG;
		factor.blendConstant4W[1][1] = blendConstantG;
		factor.blendConstant4W[1][2] = blendConstantG;
		factor.blendConstant4W[1][3] = blendConstantG;

		factor.blendConstant4W[2][0] = blendConstantB;
		factor.blendConstant4W[2][1] = blendConstantB;
		factor.blendConstant4W[2][2] = blendConstantB;
		factor.blendConstant4W[2][3] = blendConstantB;

		factor.blendConstant4W[3][0] = blendConstantA;
		factor.blendConstant4W[3][1] = blendConstantA;
		factor.blendConstant4W[3][2] = blendConstantA;
		factor.blendConstant4W[3][3] = blendConstantA;

		// FIXME: Compact into generic function   // FIXME: Clamp
		short invBlendConstantR = iround(65535 * (1 - blendConstant.r));
		short invBlendConstantG = iround(65535 * (1 - blendConstant.g));
		short invBlendConstantB = iround(65535 * (1 - blendConstant.b));
		short invBlendConstantA = iround(65535 * (1 - blendConstant.a));

		factor.invBlendConstant4W[0][0] = invBlendConstantR;
		factor.invBlendConstant4W[0][1] = invBlendConstantR;
		factor.invBlendConstant4W[0][2] = invBlendConstantR;
		factor.invBlendConstant4W[0][3] = invBlendConstantR;

		factor.invBlendConstant4W[1][0] = invBlendConstantG;
		factor.invBlendConstant4W[1][1] = invBlendConstantG;
		factor.invBlendConstant4W[1][2] = invBlendConstantG;
		factor.invBlendConstant4W[1][3] = invBlendConstantG;

		factor.invBlendConstant4W[2][0] = invBlendConstantB;
		factor.invBlendConstant4W[2][1] = invBlendConstantB;
		factor.invBlendConstant4W[2][2] = invBlendConstantB;
		factor.invBlendConstant4W[2][3] = invBlendConstantB;

		factor.invBlendConstant4W[3][0] = invBlendConstantA;
		factor.invBlendConstant4W[3][1] = invBlendConstantA;
		factor.invBlendConstant4W[3][2] = invBlendConstantA;
		factor.invBlendConstant4W[3][3] = invBlendConstantA;

		factor.blendConstant4F[0][0] = blendConstant.r;
		factor.blendConstant4F[0][1] = blendConstant.r;
		factor.blendConstant4F[0][2] = blendConstant.r;
		factor.blendConstant4F[0][3] = blendConstant.r;

		factor.blendConstant4F[1][0] = blendConstant.g;
		factor.blendConstant4F[1][1] = blendConstant.g;
		factor.blendConstant4F[1][2] = blendConstant.g;
		factor.blendConstant4F[1][3] = blendConstant.g;

		factor.blendConstant4F[2][0] = blendConstant.b;
		factor.blendConstant4F[2][1] = blendConstant.b;
		factor.blendConstant4F[2][2] = blendConstant.b;
		factor.blendConstant4F[2][3] = blendConstant.b;

		factor.blendConstant4F[3][0] = blendConstant.a;
		factor.blendConstant4F[3][1] = blendConstant.a;
		factor.blendConstant4F[3][2] = blendConstant.a;
		factor.blendConstant4F[3][3] = blendConstant.a;

		factor.invBlendConstant4F[0][0] = 1 - blendConstant.r;
		factor.invBlendConstant4F[0][1] = 1 - blendConstant.r;
		factor.invBlendConstant4F[0][2] = 1 - blendConstant.r;
		factor.invBlendConstant4F[0][3] = 1 - blendConstant.r;

		factor.invBlendConstant4F[1][0] = 1 - blendConstant.g;
		factor.invBlendConstant4F[1][1] = 1 - blendConstant.g;
		factor.invBlendConstant4F[1][2] = 1 - blendConstant.g;
		factor.invBlendConstant4F[1][3] = 1 - blendConstant.g;

		factor.invBlendConstant4F[2][0] = 1 - blendConstant.b;
		factor.invBlendConstant4F[2][1] = 1 - blendConstant.b;
		factor.invBlendConstant4F[2][2] = 1 - blendConstant.b;
		factor.invBlendConstant4F[2][3] = 1 - blendConstant.b;

		factor.invBlendConstant4F[3][0] = 1 - blendConstant.a;
		factor.invBlendConstant4F[3][1] = 1 - blendConstant.a;
		factor.invBlendConstant4F[3][2] = 1 - blendConstant.a;
		factor.invBlendConstant4F[3][3] = 1 - blendConstant.a;
	}

	void PixelProcessor::setFillMode(FillMode fillMode)
	{
		context->fillMode = fillMode;
	}

	void PixelProcessor::setShadingMode(ShadingMode shadingMode)
	{
		context->shadingMode = shadingMode;
	}

	void PixelProcessor::setAlphaBlendEnable(bool alphaBlendEnable)
	{
		context->setAlphaBlendEnable(alphaBlendEnable);
	}

	void PixelProcessor::setSourceBlendFactor(BlendFactor sourceBlendFactor)
	{
		context->setSourceBlendFactor(sourceBlendFactor);
	}

	void PixelProcessor::setDestBlendFactor(BlendFactor destBlendFactor)
	{
		context->setDestBlendFactor(destBlendFactor);
	}

	void PixelProcessor::setBlendOperation(BlendOperation blendOperation)
	{
		context->setBlendOperation(blendOperation);
	}

	void PixelProcessor::setSeparateAlphaBlendEnable(bool separateAlphaBlendEnable)
	{
		context->setSeparateAlphaBlendEnable(separateAlphaBlendEnable);
	}

	void PixelProcessor::setSourceBlendFactorAlpha(BlendFactor sourceBlendFactorAlpha)
	{
		context->setSourceBlendFactorAlpha(sourceBlendFactorAlpha);
	}

	void PixelProcessor::setDestBlendFactorAlpha(BlendFactor destBlendFactorAlpha)
	{
		context->setDestBlendFactorAlpha(destBlendFactorAlpha);
	}

	void PixelProcessor::setBlendOperationAlpha(BlendOperation blendOperationAlpha)
	{
		context->setBlendOperationAlpha(blendOperationAlpha);
	}

	void PixelProcessor::setAlphaReference(float alphaReference)
	{
		context->alphaReference = alphaReference;

		factor.alphaReference4[0] = (word)iround(alphaReference * 0x1000 / 0xFF);
		factor.alphaReference4[1] = (word)iround(alphaReference * 0x1000 / 0xFF);
		factor.alphaReference4[2] = (word)iround(alphaReference * 0x1000 / 0xFF);
		factor.alphaReference4[3] = (word)iround(alphaReference * 0x1000 / 0xFF);
	}

	void PixelProcessor::setGlobalMipmapBias(float bias)
	{
		context->setGlobalMipmapBias(bias);
	}

	void PixelProcessor::setFogStart(float start)
	{
		setFogRanges(start, context->fogEnd);
	}

	void PixelProcessor::setFogEnd(float end)
	{
		setFogRanges(context->fogStart, end);
	}

	void PixelProcessor::setFogColor(Color<float> fogColor)
	{
		// TODO: Compact into generic function
		word fogR = (unsigned short)(65535 * fogColor.r);
		word fogG = (unsigned short)(65535 * fogColor.g);
		word fogB = (unsigned short)(65535 * fogColor.b);

		fog.color4[0][0] = fogR;
		fog.color4[0][1] = fogR;
		fog.color4[0][2] = fogR;
		fog.color4[0][3] = fogR;

		fog.color4[1][0] = fogG;
		fog.color4[1][1] = fogG;
		fog.color4[1][2] = fogG;
		fog.color4[1][3] = fogG;

		fog.color4[2][0] = fogB;
		fog.color4[2][1] = fogB;
		fog.color4[2][2] = fogB;
		fog.color4[2][3] = fogB;

		fog.colorF[0] = replicate(fogColor.r);
		fog.colorF[1] = replicate(fogColor.g);
		fog.colorF[2] = replicate(fogColor.b);
	}

	void PixelProcessor::setFogDensity(float fogDensity)
	{
		fog.densityE = replicate(-fogDensity * 1.442695f);   // 1/e^x = 2^(-x*1.44)
		fog.density2E = replicate(-fogDensity * fogDensity * 1.442695f);
	}

	void PixelProcessor::setPixelFogMode(FogMode fogMode)
	{
		context->pixelFogMode = fogMode;
	}

	void PixelProcessor::setPerspectiveCorrection(bool perspectiveEnable)
	{
		perspectiveCorrection = perspectiveEnable;
	}

	void PixelProcessor::setOcclusionEnabled(bool enable)
	{
		context->occlusionEnabled = enable;
	}

	void PixelProcessor::setRoutineCacheSize(int cacheSize)
	{
		delete routineCache;
		routineCache = new RoutineCache<State>(clamp(cacheSize, 1, 65536), precachePixel ? "sw-pixel" : 0);
	}

	void PixelProcessor::setFogRanges(float start, float end)
	{
		context->fogStart = start;
		context->fogEnd = end;

		if(start == end)
		{
			end += 0.001f;   // Hack: ensure there is a small range
		}

		float fogScale = -1.0f / (end - start);
		float fogOffset = end * -fogScale;

		fog.scale = replicate(fogScale);
		fog.offset = replicate(fogOffset);
	}

	const PixelProcessor::State PixelProcessor::update() const
	{
		State state;

		if(context->pixelShader)
		{
			state.shaderID = context->pixelShader->getSerialID();
		}
		else
		{
			state.shaderID = 0;
		}

		state.depthOverride = context->pixelShader && context->pixelShader->depthOverride();
		state.shaderContainsKill = context->pixelShader ? context->pixelShader->containsKill() : false;

		if(context->alphaTestActive())
		{
			state.alphaCompareMode = context->alphaCompareMode;

			state.transparencyAntialiasing = context->getMultiSampleCount() > 1 ? transparencyAntialiasing : TRANSPARENCY_NONE;
		}

		state.depthWriteEnable = context->depthWriteActive();

		if(context->stencilActive())
		{
			state.stencilActive = true;
			state.stencilCompareMode = context->stencilCompareMode;
			state.stencilFailOperation = context->stencilFailOperation;
			state.stencilPassOperation = context->stencilPassOperation;
			state.stencilZFailOperation = context->stencilZFailOperation;
			state.noStencilMask = (context->stencilMask == 0xFF);
			state.noStencilWriteMask = (context->stencilWriteMask == 0xFF);
			state.stencilWriteMasked = (context->stencilWriteMask == 0x00);

			state.twoSidedStencil = context->twoSidedStencil;
			state.stencilCompareModeCCW = context->twoSidedStencil ? context->stencilCompareModeCCW : state.stencilCompareMode;
			state.stencilFailOperationCCW = context->twoSidedStencil ? context->stencilFailOperationCCW : state.stencilFailOperation;
			state.stencilPassOperationCCW = context->twoSidedStencil ? context->stencilPassOperationCCW : state.stencilPassOperation;
			state.stencilZFailOperationCCW = context->twoSidedStencil ? context->stencilZFailOperationCCW : state.stencilZFailOperation;
			state.noStencilMaskCCW = context->twoSidedStencil ? (context->stencilMaskCCW == 0xFF) : state.noStencilMask;
			state.noStencilWriteMaskCCW = context->twoSidedStencil ? (context->stencilWriteMaskCCW == 0xFF) : state.noStencilWriteMask;
			state.stencilWriteMaskedCCW = context->twoSidedStencil ? (context->stencilWriteMaskCCW == 0x00) : state.stencilWriteMasked;
		}

		if(context->depthBufferActive())
		{
			state.depthTestActive = true;
			state.depthCompareMode = context->depthCompareMode;
			state.quadLayoutDepthBuffer = Surface::hasQuadLayout(context->depthBuffer->getInternalFormat());
		}

		state.occlusionEnabled = context->occlusionEnabled;

		state.fogActive = context->fogActive();
		state.pixelFogMode = context->pixelFogActive();
		state.wBasedFog = context->wBasedFog && context->pixelFogActive() != FOG_NONE;
		state.perspective = context->perspectiveActive();
		state.depthClamp = (context->depthBias != 0.0f) || (context->slopeDepthBias != 0.0f);

		if(context->alphaBlendActive())
		{
			state.alphaBlendActive = true;
			state.sourceBlendFactor = context->sourceBlendFactor();
			state.destBlendFactor = context->destBlendFactor();
			state.blendOperation = context->blendOperation();
			state.sourceBlendFactorAlpha = context->sourceBlendFactorAlpha();
			state.destBlendFactorAlpha = context->destBlendFactorAlpha();
			state.blendOperationAlpha = context->blendOperationAlpha();
		}

		state.logicalOperation = context->colorLogicOp();

		for(int i = 0; i < RENDERTARGETS; i++)
		{
			state.colorWriteMask |= context->colorWriteActive(i) << (4 * i);
			state.targetFormat[i] = context->renderTargetInternalFormat(i);
		}

		state.writeSRGB	= context->writeSRGB && context->renderTarget[0] && Surface::isSRGBwritable(context->renderTarget[0]->getExternalFormat());
		state.multiSample = context->getMultiSampleCount();
		state.multiSampleMask = context->multiSampleMask;

		if(state.multiSample > 1 && context->pixelShader)
		{
			state.centroid = context->pixelShader->containsCentroid();
		}

		if(!context->pixelShader)
		{
			for(unsigned int i = 0; i < 8; i++)
			{
				state.textureStage[i] = context->textureStage[i].textureStageState();
			}

			state.specularAdd = context->specularActive() && context->specularEnable;
		}

		for(unsigned int i = 0; i < 16; i++)
		{
			if(context->pixelShader)
			{
				if(context->pixelShader->usesSampler(i))
				{
					state.sampler[i] = context->sampler[i].samplerState();
				}
			}
			else
			{
				if(i < 8 && state.textureStage[i].stageOperation != TextureStage::STAGE_DISABLE)
				{
					state.sampler[i] = context->sampler[i].samplerState();
				}
				else break;
			}
		}

		const bool point = context->isDrawPoint(true);
		const bool sprite = context->pointSpriteActive();
		const bool flatShading = (context->shadingMode == SHADING_FLAT) || point;

		if(context->pixelShaderModel() < 0x0300)
		{
			for(int coordinate = 0; coordinate < 8; coordinate++)
			{
				for(int component = 0; component < 4; component++)
				{
					if(context->textureActive(coordinate, component))
					{
						state.texture[coordinate].component |= 1 << component;

						if(point && !sprite)
						{
							state.texture[coordinate].flat |= 1 << component;
						}
					}
				}

				if(context->textureTransformProject[coordinate] && context->pixelShaderModel() <= 0x0103)
				{
					if(context->textureTransformCount[coordinate] == 2)
					{
						state.texture[coordinate].project = 1;
					}
					else if(context->textureTransformCount[coordinate] == 3)
					{
						state.texture[coordinate].project = 2;
					}
					else if(context->textureTransformCount[coordinate] == 4 || context->textureTransformCount[coordinate] == 0)
					{
						state.texture[coordinate].project = 3;
					}
				}
			}

			for(int color = 0; color < 2; color++)
			{
				for(int component = 0; component < 4; component++)
				{
					if(context->colorActive(color, component))
					{
						state.color[color].component |= 1 << component;

						if(point || flatShading)
						{
							state.color[color].flat |= 1 << component;
						}
					}
				}
			}

			if(context->fogActive())
			{
				state.fog.component = true;

				if(point)
				{
					state.fog.flat = true;
				}
			}
		}
		else
		{
			for(int interpolant = 0; interpolant < MAX_FRAGMENT_INPUTS; interpolant++)
			{
				for(int component = 0; component < 4; component++)
				{
					const Shader::Semantic &semantic = context->pixelShader->getInput(interpolant, component);

					if(semantic.active())
					{
						bool flat = point;

						switch(semantic.usage)
						{
						case Shader::USAGE_TEXCOORD: flat = point && !sprite;             break;
						case Shader::USAGE_COLOR:    flat = semantic.flat || flatShading; break;
						}

						state.interpolant[interpolant].component |= 1 << component;

						if(flat)
						{
							state.interpolant[interpolant].flat |= 1 << component;
						}
					}
				}
			}
		}

		if(state.centroid)
		{
			for(int interpolant = 0; interpolant < MAX_FRAGMENT_INPUTS; interpolant++)
			{
				for(int component = 0; component < 4; component++)
				{
					state.interpolant[interpolant].centroid = context->pixelShader->getInput(interpolant, 0).centroid;
				}
			}
		}

		state.hash = state.computeHash();

		return state;
	}

	Routine *PixelProcessor::routine(const State &state)
	{
		Routine *routine = routineCache->query(state);

		if(!routine)
		{
			const bool integerPipeline = (context->pixelShaderModel() <= 0x0104);
			QuadRasterizer *generator = nullptr;

			if(integerPipeline)
			{
				generator = new PixelPipeline(state, context->pixelShader);
			}
			else
			{
				generator = new PixelProgram(state, context->pixelShader);
			}

			generator->generate();
			routine = (*generator)(L"PixelRoutine_%0.8X", state.shaderID);
			delete generator;

			routineCache->add(state, routine);
		}

		return routine;
	}
}
