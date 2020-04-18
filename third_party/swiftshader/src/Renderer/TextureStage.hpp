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

#ifndef sw_TextureStage_hpp
#define sw_TextureStage_hpp

#include "Common/Types.hpp"
#include "Common/Math.hpp"
#include "Renderer/Color.hpp"

namespace sw
{
	class Sampler;
	class PixelRoutine;
	class Context;

	class TextureStage
	{
		friend class Context;   // FIXME

	public:
		enum StageOperation
		{
			STAGE_DISABLE,
			STAGE_SELECTARG1,
			STAGE_SELECTARG2,
			STAGE_SELECTARG3,
			STAGE_MODULATE,
			STAGE_MODULATE2X,
			STAGE_MODULATE4X,
			STAGE_ADD,
			STAGE_ADDSIGNED,
			STAGE_ADDSIGNED2X,
			STAGE_SUBTRACT,
			STAGE_ADDSMOOTH,
			STAGE_MULTIPLYADD,
			STAGE_LERP,
			STAGE_DOT3,
			STAGE_BLENDCURRENTALPHA,
			STAGE_BLENDDIFFUSEALPHA,
			STAGE_BLENDFACTORALPHA,
			STAGE_BLENDTEXTUREALPHA,
			STAGE_BLENDTEXTUREALPHAPM,
			STAGE_PREMODULATE,
			STAGE_MODULATEALPHA_ADDCOLOR,
			STAGE_MODULATECOLOR_ADDALPHA,
			STAGE_MODULATEINVALPHA_ADDCOLOR,
			STAGE_MODULATEINVCOLOR_ADDALPHA,
			STAGE_BUMPENVMAP,
			STAGE_BUMPENVMAPLUMINANCE,

			STAGE_LAST = STAGE_BUMPENVMAPLUMINANCE
		};

		enum SourceArgument
		{
			SOURCE_TEXTURE,
			SOURCE_CONSTANT,
			SOURCE_CURRENT,
			SOURCE_DIFFUSE,
			SOURCE_SPECULAR,
			SOURCE_TEMP,
			SOURCE_TFACTOR,

			SOURCE_LAST = SOURCE_TFACTOR
		};

		enum DestinationArgument
		{
			DESTINATION_CURRENT,
			DESTINATION_TEMP,

			DESTINATION_LAST = DESTINATION_TEMP
		};

		enum ArgumentModifier
		{
			MODIFIER_COLOR,
			MODIFIER_INVCOLOR,
			MODIFIER_ALPHA,
			MODIFIER_INVALPHA,

			MODIFIER_LAST = MODIFIER_INVALPHA
		};

		struct State
		{
			State();

			unsigned int stageOperation			: BITS(STAGE_LAST);
			unsigned int firstArgument			: BITS(SOURCE_LAST);
			unsigned int secondArgument			: BITS(SOURCE_LAST);
			unsigned int thirdArgument			: BITS(SOURCE_LAST);
			unsigned int stageOperationAlpha	: BITS(STAGE_LAST);
			unsigned int firstArgumentAlpha		: BITS(SOURCE_LAST);
			unsigned int secondArgumentAlpha	: BITS(SOURCE_LAST);
			unsigned int thirdArgumentAlpha		: BITS(SOURCE_LAST);
			unsigned int firstModifier			: BITS(MODIFIER_LAST);
			unsigned int secondModifier			: BITS(MODIFIER_LAST);
			unsigned int thirdModifier			: BITS(MODIFIER_LAST);
			unsigned int firstModifierAlpha		: BITS(MODIFIER_LAST);
			unsigned int secondModifierAlpha	: BITS(MODIFIER_LAST);
			unsigned int thirdModifierAlpha		: BITS(MODIFIER_LAST);
			unsigned int destinationArgument	: BITS(DESTINATION_LAST);
			unsigned int texCoordIndex			: BITS(7);

			unsigned int cantUnderflow			: 1;
			unsigned int usesTexture			: 1;
		};

		struct Uniforms
		{
			word4 constantColor4[4];
			float4 bumpmapMatrix4F[2][2];
			word4 bumpmapMatrix4W[2][2];
			word4 luminanceScale4;
			word4 luminanceOffset4;
		};

		TextureStage();

		~TextureStage();

		void init(int stage, const Sampler *sampler, const TextureStage *previousStage);

		State textureStageState() const;

		void setConstantColor(const Color<float> &constantColor);
		void setBumpmapMatrix(int element, float value);
		void setLuminanceScale(float value);
		void setLuminanceOffset(float value);

		void setTexCoordIndex(unsigned int texCoordIndex);
		void setStageOperation(StageOperation stageOperation);
		void setFirstArgument(SourceArgument firstArgument);
		void setSecondArgument(SourceArgument secondArgument);
		void setThirdArgument(SourceArgument thirdArgument);
		void setStageOperationAlpha(StageOperation stageOperationAlpha);
		void setFirstArgumentAlpha(SourceArgument firstArgumentAlpha);
		void setSecondArgumentAlpha(SourceArgument secondArgumentAlpha);
		void setThirdArgumentAlpha(SourceArgument thirdArgumentAlpha);
		void setFirstModifier(ArgumentModifier firstModifier);
		void setSecondModifier(ArgumentModifier secondModifier);
		void setThirdModifier(ArgumentModifier thirdModifier);
		void setFirstModifierAlpha(ArgumentModifier firstModifierAlpha);
		void setSecondModifierAlpha(ArgumentModifier secondModifierAlpha);
		void setThirdModifierAlpha(ArgumentModifier thirdModifierAlpha);
		void setDestinationArgument(DestinationArgument destinationArgument);

		Uniforms uniforms;   // FIXME: Private

	private:
		bool usesColor(SourceArgument source) const;
		bool usesAlpha(SourceArgument source) const;
		bool uses(SourceArgument source) const;
		bool usesCurrent() const;
		bool usesDiffuse() const;
		bool usesSpecular() const;
		bool usesTexture() const;
		bool isStageDisabled() const;
		bool writesCurrent() const;

		int stage;

		StageOperation stageOperation;
		SourceArgument firstArgument;
		SourceArgument secondArgument;
		SourceArgument thirdArgument;
		StageOperation stageOperationAlpha;
		SourceArgument firstArgumentAlpha;
		SourceArgument secondArgumentAlpha;
		SourceArgument thirdArgumentAlpha;
		ArgumentModifier firstModifier;
		ArgumentModifier secondModifier;
		ArgumentModifier thirdModifier;
		ArgumentModifier firstModifierAlpha;
		ArgumentModifier secondModifierAlpha;
		ArgumentModifier thirdModifierAlpha;
		DestinationArgument destinationArgument;

		int texCoordIndex;
		const Sampler *sampler;
		const TextureStage *previousStage;
	};
}

#endif  // sw_TextureStage_hpp
