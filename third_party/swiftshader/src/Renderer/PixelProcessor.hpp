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

#ifndef sw_PixelProcessor_hpp
#define sw_PixelProcessor_hpp

#include "Context.hpp"
#include "RoutineCache.hpp"

namespace sw
{
	class PixelShader;
	class Rasterizer;
	struct Texture;
	struct DrawData;

	class PixelProcessor
	{
	public:
		struct States
		{
			unsigned int computeHash();

			int shaderID;

			bool depthOverride                        : 1;   // TODO: Eliminate by querying shader.
			bool shaderContainsKill                   : 1;   // TODO: Eliminate by querying shader.

			DepthCompareMode depthCompareMode         : BITS(DEPTH_LAST);
			AlphaCompareMode alphaCompareMode         : BITS(ALPHA_LAST);
			bool depthWriteEnable                     : 1;
			bool quadLayoutDepthBuffer                : 1;

			bool stencilActive                        : 1;
			StencilCompareMode stencilCompareMode     : BITS(STENCIL_LAST);
			StencilOperation stencilFailOperation     : BITS(OPERATION_LAST);
			StencilOperation stencilPassOperation     : BITS(OPERATION_LAST);
			StencilOperation stencilZFailOperation    : BITS(OPERATION_LAST);
			bool noStencilMask                        : 1;
			bool noStencilWriteMask                   : 1;
			bool stencilWriteMasked                   : 1;
			bool twoSidedStencil                      : 1;
			StencilCompareMode stencilCompareModeCCW  : BITS(STENCIL_LAST);
			StencilOperation stencilFailOperationCCW  : BITS(OPERATION_LAST);
			StencilOperation stencilPassOperationCCW  : BITS(OPERATION_LAST);
			StencilOperation stencilZFailOperationCCW : BITS(OPERATION_LAST);
			bool noStencilMaskCCW                     : 1;
			bool noStencilWriteMaskCCW                : 1;
			bool stencilWriteMaskedCCW                : 1;

			bool depthTestActive                      : 1;
			bool fogActive                            : 1;
			FogMode pixelFogMode                      : BITS(FOG_LAST);
			bool specularAdd                          : 1;
			bool occlusionEnabled                     : 1;
			bool wBasedFog                            : 1;
			bool perspective                          : 1;
			bool depthClamp                           : 1;

			bool alphaBlendActive                     : 1;
			BlendFactor sourceBlendFactor             : BITS(BLEND_LAST);
			BlendFactor destBlendFactor               : BITS(BLEND_LAST);
			BlendOperation blendOperation             : BITS(BLENDOP_LAST);
			BlendFactor sourceBlendFactorAlpha        : BITS(BLEND_LAST);
			BlendFactor destBlendFactorAlpha          : BITS(BLEND_LAST);
			BlendOperation blendOperationAlpha        : BITS(BLENDOP_LAST);

			unsigned int colorWriteMask                       : RENDERTARGETS * 4;   // Four component bit masks
			Format targetFormat[RENDERTARGETS];
			bool writeSRGB                                    : 1;
			unsigned int multiSample                          : 3;
			unsigned int multiSampleMask                      : 4;
			TransparencyAntialiasing transparencyAntialiasing : BITS(TRANSPARENCY_LAST);
			bool centroid                                     : 1;

			LogicalOperation logicalOperation : BITS(LOGICALOP_LAST);

			Sampler::State sampler[TEXTURE_IMAGE_UNITS];
			TextureStage::State textureStage[8];

			struct Interpolant
			{
				unsigned char component : 4;
				unsigned char flat : 4;
				unsigned char project : 2;
				bool centroid : 1;
			};

			union
			{
				struct
				{
					Interpolant color[2];
					Interpolant texture[8];
					Interpolant fog;
				};

				Interpolant interpolant[MAX_FRAGMENT_INPUTS];
			};
		};

		struct State : States
		{
			State();

			bool operator==(const State &state) const;

			int colorWriteActive(int index) const
			{
				return (colorWriteMask >> (index * 4)) & 0xF;
			}

			bool alphaTestActive() const
			{
				return (alphaCompareMode != ALPHA_ALWAYS) || (transparencyAntialiasing != TRANSPARENCY_NONE);
			}

			bool pixelFogActive() const
			{
				return pixelFogMode != FOG_NONE;
			}

			unsigned int hash;
		};

		struct Stencil
		{
			int64_t testMaskQ;
			int64_t referenceMaskedQ;
			int64_t referenceMaskedSignedQ;
			int64_t writeMaskQ;
			int64_t invWriteMaskQ;
			int64_t referenceQ;

			void set(int reference, int testMask, int writeMask)
			{
				referenceQ = replicate(reference);
				testMaskQ = replicate(testMask);
				writeMaskQ = replicate(writeMask);
				invWriteMaskQ = ~writeMaskQ;
				referenceMaskedQ = referenceQ & testMaskQ;
				referenceMaskedSignedQ = replicate(((reference & testMask) + 0x80) & 0xFF);
			}

			static int64_t replicate(int b)
			{
				int64_t w = b & 0xFF;

				return (w << 0) | (w << 8) | (w << 16) | (w << 24) | (w << 32) | (w << 40) | (w << 48) | (w << 56);
			}
		};

		struct Fog
		{
			float4 scale;
			float4 offset;
			word4 color4[3];
			float4 colorF[3];
			float4 densityE;
			float4 density2E;
		};

		struct Factor
		{
			word4 textureFactor4[4];

			word4 alphaReference4;

			word4 blendConstant4W[4];
			float4 blendConstant4F[4];
			word4 invBlendConstant4W[4];
			float4 invBlendConstant4F[4];
		};

	public:
		typedef void (*RoutinePointer)(const Primitive *primitive, int count, int thread, DrawData *draw);

		PixelProcessor(Context *context);

		virtual ~PixelProcessor();

		void setFloatConstant(unsigned int index, const float value[4]);
		void setIntegerConstant(unsigned int index, const int value[4]);
		void setBooleanConstant(unsigned int index, int boolean);

		void setUniformBuffer(int index, sw::Resource* buffer, int offset);
		void lockUniformBuffers(byte** u, sw::Resource* uniformBuffers[]);

		void setRenderTarget(int index, Surface *renderTarget, unsigned int layer = 0);
		void setDepthBuffer(Surface *depthBuffer, unsigned int layer = 0);
		void setStencilBuffer(Surface *stencilBuffer, unsigned int layer = 0);

		void setTexCoordIndex(unsigned int stage, int texCoordIndex);
		void setStageOperation(unsigned int stage, TextureStage::StageOperation stageOperation);
		void setFirstArgument(unsigned int stage, TextureStage::SourceArgument firstArgument);
		void setSecondArgument(unsigned int stage, TextureStage::SourceArgument secondArgument);
		void setThirdArgument(unsigned int stage, TextureStage::SourceArgument thirdArgument);
		void setStageOperationAlpha(unsigned int stage, TextureStage::StageOperation stageOperationAlpha);
		void setFirstArgumentAlpha(unsigned int stage, TextureStage::SourceArgument firstArgumentAlpha);
		void setSecondArgumentAlpha(unsigned int stage, TextureStage::SourceArgument secondArgumentAlpha);
		void setThirdArgumentAlpha(unsigned int stage, TextureStage::SourceArgument thirdArgumentAlpha);
		void setFirstModifier(unsigned int stage, TextureStage::ArgumentModifier firstModifier);
		void setSecondModifier(unsigned int stage, TextureStage::ArgumentModifier secondModifier);
		void setThirdModifier(unsigned int stage, TextureStage::ArgumentModifier thirdModifier);
		void setFirstModifierAlpha(unsigned int stage, TextureStage::ArgumentModifier firstModifierAlpha);
		void setSecondModifierAlpha(unsigned int stage, TextureStage::ArgumentModifier secondModifierAlpha);
		void setThirdModifierAlpha(unsigned int stage, TextureStage::ArgumentModifier thirdModifierAlpha);
		void setDestinationArgument(unsigned int stage, TextureStage::DestinationArgument destinationArgument);
		void setConstantColor(unsigned int stage, const Color<float> &constantColor);
		void setBumpmapMatrix(unsigned int stage, int element, float value);
		void setLuminanceScale(unsigned int stage, float value);
		void setLuminanceOffset(unsigned int stage, float value);

		void setTextureFilter(unsigned int sampler, FilterType textureFilter);
		void setMipmapFilter(unsigned int sampler, MipmapType mipmapFilter);
		void setGatherEnable(unsigned int sampler, bool enable);
		void setAddressingModeU(unsigned int sampler, AddressingMode addressingMode);
		void setAddressingModeV(unsigned int sampler, AddressingMode addressingMode);
		void setAddressingModeW(unsigned int sampler, AddressingMode addressingMode);
		void setReadSRGB(unsigned int sampler, bool sRGB);
		void setMipmapLOD(unsigned int sampler, float bias);
		void setBorderColor(unsigned int sampler, const Color<float> &borderColor);
		void setMaxAnisotropy(unsigned int sampler, float maxAnisotropy);
		void setHighPrecisionFiltering(unsigned int sampler, bool highPrecisionFiltering);
		void setSwizzleR(unsigned int sampler, SwizzleType swizzleR);
		void setSwizzleG(unsigned int sampler, SwizzleType swizzleG);
		void setSwizzleB(unsigned int sampler, SwizzleType swizzleB);
		void setSwizzleA(unsigned int sampler, SwizzleType swizzleA);
		void setCompareFunc(unsigned int sampler, CompareFunc compare);
		void setBaseLevel(unsigned int sampler, int baseLevel);
		void setMaxLevel(unsigned int sampler, int maxLevel);
		void setMinLod(unsigned int sampler, float minLod);
		void setMaxLod(unsigned int sampler, float maxLod);

		void setWriteSRGB(bool sRGB);
		void setDepthBufferEnable(bool depthBufferEnable);
		void setDepthCompare(DepthCompareMode depthCompareMode);
		void setAlphaCompare(AlphaCompareMode alphaCompareMode);
		void setDepthWriteEnable(bool depthWriteEnable);
		void setAlphaTestEnable(bool alphaTestEnable);
		void setCullMode(CullMode cullMode);
		void setColorWriteMask(int index, int rgbaMask);

		void setColorLogicOpEnabled(bool colorLogicOpEnabled);
		void setLogicalOperation(LogicalOperation logicalOperation);

		void setStencilEnable(bool stencilEnable);
		void setStencilCompare(StencilCompareMode stencilCompareMode);
		void setStencilReference(int stencilReference);
		void setStencilMask(int stencilMask);
		void setStencilFailOperation(StencilOperation stencilFailOperation);
		void setStencilPassOperation(StencilOperation stencilPassOperation);
		void setStencilZFailOperation(StencilOperation stencilZFailOperation);
		void setStencilWriteMask(int stencilWriteMask);
		void setTwoSidedStencil(bool enable);
		void setStencilCompareCCW(StencilCompareMode stencilCompareMode);
		void setStencilReferenceCCW(int stencilReference);
		void setStencilMaskCCW(int stencilMask);
		void setStencilFailOperationCCW(StencilOperation stencilFailOperation);
		void setStencilPassOperationCCW(StencilOperation stencilPassOperation);
		void setStencilZFailOperationCCW(StencilOperation stencilZFailOperation);
		void setStencilWriteMaskCCW(int stencilWriteMask);

		void setTextureFactor(const Color<float> &textureFactor);
		void setBlendConstant(const Color<float> &blendConstant);

		void setFillMode(FillMode fillMode);
		void setShadingMode(ShadingMode shadingMode);

		void setAlphaBlendEnable(bool alphaBlendEnable);
		void setSourceBlendFactor(BlendFactor sourceBlendFactor);
		void setDestBlendFactor(BlendFactor destBlendFactor);
		void setBlendOperation(BlendOperation blendOperation);

		void setSeparateAlphaBlendEnable(bool separateAlphaBlendEnable);
		void setSourceBlendFactorAlpha(BlendFactor sourceBlendFactorAlpha);
		void setDestBlendFactorAlpha(BlendFactor destBlendFactorAlpha);
		void setBlendOperationAlpha(BlendOperation blendOperationAlpha);

		void setAlphaReference(float alphaReference);

		void setGlobalMipmapBias(float bias);

		void setFogStart(float start);
		void setFogEnd(float end);
		void setFogColor(Color<float> fogColor);
		void setFogDensity(float fogDensity);
		void setPixelFogMode(FogMode fogMode);

		void setPerspectiveCorrection(bool perspectiveCorrection);

		void setOcclusionEnabled(bool enable);

	protected:
		const State update() const;
		Routine *routine(const State &state);
		void setRoutineCacheSize(int routineCacheSize);

		// Shader constants
		word4 cW[8][4];
		float4 c[FRAGMENT_UNIFORM_VECTORS];
		int4 i[16];
		bool b[16];

		// Other semi-constants
		Stencil stencil;
		Stencil stencilCCW;
		Fog fog;
		Factor factor;

	private:
		struct UniformBufferInfo
		{
			UniformBufferInfo();

			Resource* buffer;
			int offset;
		};
		UniformBufferInfo uniformBufferInfo[MAX_UNIFORM_BUFFER_BINDINGS];

		void setFogRanges(float start, float end);

		Context *const context;

		RoutineCache<State> *routineCache;
	};
}

#endif   // sw_PixelProcessor_hpp
