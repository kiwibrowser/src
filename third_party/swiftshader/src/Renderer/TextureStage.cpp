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

#include "TextureStage.hpp"

#include "Sampler.hpp"
#include "Common/Debug.hpp"

#include <string.h>

namespace sw
{
	TextureStage::State::State()
	{
		memset(this, 0, sizeof(State));
	}

	TextureStage::TextureStage() : sampler(0), previousStage(0)
	{
	}

	TextureStage::~TextureStage()
	{
	}

	void TextureStage::init(int stage, const Sampler *sampler, const TextureStage *previousStage)
	{
		this->stage = stage;

		stageOperation = (stage == 0 ? STAGE_MODULATE : STAGE_DISABLE);
		firstArgument = SOURCE_TEXTURE;
		secondArgument = SOURCE_CURRENT;
		thirdArgument = SOURCE_CURRENT;
		stageOperationAlpha = (stage == 0 ? STAGE_SELECTARG1 : STAGE_DISABLE);
		firstArgumentAlpha = SOURCE_DIFFUSE;
		secondArgumentAlpha = SOURCE_CURRENT;
		thirdArgumentAlpha = SOURCE_CURRENT;
		firstModifier = MODIFIER_COLOR;
		secondModifier = MODIFIER_COLOR;
		thirdModifier = MODIFIER_COLOR;
	    firstModifierAlpha = MODIFIER_COLOR;
		secondModifierAlpha = MODIFIER_COLOR;
		thirdModifierAlpha = MODIFIER_COLOR;
		destinationArgument = DESTINATION_CURRENT;

		texCoordIndex = stage;
		this->sampler = sampler;
		this->previousStage = previousStage;
	}

	TextureStage::State TextureStage::textureStageState() const
	{
		State state;

		if(!isStageDisabled())
		{
			state.stageOperation = stageOperation;
			state.firstArgument = firstArgument;
			state.secondArgument = secondArgument;
			state.thirdArgument = thirdArgument;
			state.stageOperationAlpha = stageOperationAlpha;
			state.firstArgumentAlpha = firstArgumentAlpha;
			state.secondArgumentAlpha = secondArgumentAlpha;
			state.thirdArgumentAlpha = thirdArgumentAlpha;
			state.firstModifier = firstModifier;
			state.secondModifier = secondModifier;
			state.thirdModifier = thirdModifier;
			state.firstModifierAlpha = firstModifierAlpha;
			state.secondModifierAlpha = secondModifierAlpha;
			state.thirdModifierAlpha = thirdModifierAlpha;
			state.destinationArgument = destinationArgument;
			state.texCoordIndex = texCoordIndex;

			state.cantUnderflow = sampler->hasUnsignedTexture() || !usesTexture();
			state.usesTexture = usesTexture();
		}

		return state;
	}

	void TextureStage::setConstantColor(const Color<float> &constantColor)
	{
		// FIXME: Compact into generic function   // FIXME: Clamp
		short r = iround(4095 * constantColor.r);
		short g = iround(4095 * constantColor.g);
		short b = iround(4095 * constantColor.b);
		short a = iround(4095 * constantColor.a);

		uniforms.constantColor4[0][0] = uniforms.constantColor4[0][1] = uniforms.constantColor4[0][2] = uniforms.constantColor4[0][3] = r;
		uniforms.constantColor4[1][0] = uniforms.constantColor4[1][1] = uniforms.constantColor4[1][2] = uniforms.constantColor4[1][3] = g;
		uniforms.constantColor4[2][0] = uniforms.constantColor4[2][1] = uniforms.constantColor4[2][2] = uniforms.constantColor4[2][3] = b;
		uniforms.constantColor4[3][0] = uniforms.constantColor4[3][1] = uniforms.constantColor4[3][2] = uniforms.constantColor4[3][3] = a;
	}

	void TextureStage::setBumpmapMatrix(int element, float value)
	{
		uniforms.bumpmapMatrix4F[element / 2][element % 2][0] = value;
		uniforms.bumpmapMatrix4F[element / 2][element % 2][1] = value;
		uniforms.bumpmapMatrix4F[element / 2][element % 2][2] = value;
		uniforms.bumpmapMatrix4F[element / 2][element % 2][3] = value;

		uniforms.bumpmapMatrix4W[element / 2][element % 2][0] = iround(4095 * value);
		uniforms.bumpmapMatrix4W[element / 2][element % 2][1] = iround(4095 * value);
		uniforms.bumpmapMatrix4W[element / 2][element % 2][2] = iround(4095 * value);
		uniforms.bumpmapMatrix4W[element / 2][element % 2][3] = iround(4095 * value);
	}

	void TextureStage::setLuminanceScale(float value)
	{
		short scale = iround(4095 * value);

		uniforms.luminanceScale4[0] = uniforms.luminanceScale4[1] = uniforms.luminanceScale4[2] = uniforms.luminanceScale4[3] = scale;
	}

	void TextureStage::setLuminanceOffset(float value)
	{
		short offset = iround(4095 * value);

		uniforms.luminanceOffset4[0] = uniforms.luminanceOffset4[1] = uniforms.luminanceOffset4[2] = uniforms.luminanceOffset4[3] = offset;
	}

	void TextureStage::setTexCoordIndex(unsigned int texCoordIndex)
	{
		ASSERT(texCoordIndex < 8);

		this->texCoordIndex = texCoordIndex;
	}

	void TextureStage::setStageOperation(StageOperation stageOperation)
	{
		this->stageOperation = stageOperation;
	}

	void TextureStage::setFirstArgument(SourceArgument firstArgument)
	{
		this->firstArgument = firstArgument;
	}

	void TextureStage::setSecondArgument(SourceArgument secondArgument)
	{
		this->secondArgument = secondArgument;
	}

	void TextureStage::setThirdArgument(SourceArgument thirdArgument)
	{
		this->thirdArgument = thirdArgument;
	}

	void TextureStage::setStageOperationAlpha(StageOperation stageOperationAlpha)
	{
		this->stageOperationAlpha = stageOperationAlpha;
	}

	void TextureStage::setFirstArgumentAlpha(SourceArgument firstArgumentAlpha)
	{
		this->firstArgumentAlpha = firstArgumentAlpha;
	}

	void TextureStage::setSecondArgumentAlpha(SourceArgument secondArgumentAlpha)
	{
		this->secondArgumentAlpha = secondArgumentAlpha;
	}

	void TextureStage::setThirdArgumentAlpha(SourceArgument thirdArgumentAlpha)
	{
		this->thirdArgumentAlpha= thirdArgumentAlpha;
	}

	void TextureStage::setFirstModifier(ArgumentModifier firstModifier)
	{
		this->firstModifier = firstModifier;
	}

	void TextureStage::setSecondModifier(ArgumentModifier secondModifier)
	{
		this->secondModifier = secondModifier;
	}

	void TextureStage::setThirdModifier(ArgumentModifier thirdModifier)
	{
		this->thirdModifier = thirdModifier;
	}

	void TextureStage::setFirstModifierAlpha(ArgumentModifier firstModifierAlpha)
	{
		this->firstModifierAlpha = firstModifierAlpha;
	}

	void TextureStage::setSecondModifierAlpha(ArgumentModifier secondModifierAlpha)
	{
		this->secondModifierAlpha = secondModifierAlpha;
	}

	void TextureStage::setThirdModifierAlpha(ArgumentModifier thirdModifierAlpha)
	{
		this->thirdModifierAlpha = thirdModifierAlpha;
	}

	void TextureStage::setDestinationArgument(DestinationArgument destinationArgument)
	{
		this->destinationArgument = destinationArgument;
	}

	bool TextureStage::usesColor(SourceArgument source) const
	{
		// One argument
		if(stageOperation == STAGE_SELECTARG1 || stageOperation == STAGE_PREMODULATE)
		{
			return firstArgument == source;
		}
		else if(stageOperation == STAGE_SELECTARG2)
		{
			return secondArgument == source;
		}
		else if(stageOperation == STAGE_SELECTARG3)
		{
			return thirdArgument == source;
		}
		else
		{
			// Two arguments or more
			if(firstArgument == source || secondArgument == source)
			{
				return true;
			}

			// Three arguments
			if(stageOperation == STAGE_MULTIPLYADD || stageOperation == STAGE_LERP)
			{
				return thirdArgument == source;
			}
		}
	
		return false;
	}

	bool TextureStage::usesAlpha(SourceArgument source) const
	{
		if(stageOperationAlpha == STAGE_DISABLE)
		{
			return false;
		}

		if(source == SOURCE_TEXTURE)
		{
			if(stageOperation == STAGE_BLENDTEXTUREALPHA ||	stageOperation == STAGE_BLENDTEXTUREALPHAPM)
			{
				return true;
			}
		}
		else if(source == SOURCE_CURRENT)
		{
			if(stageOperation == STAGE_BLENDCURRENTALPHA)
			{
				return true;
			}
		}
		else if(source == SOURCE_DIFFUSE)
		{
			if(stageOperation == STAGE_BLENDDIFFUSEALPHA)
			{
				return true;
			}
		}
		else if(source == SOURCE_TFACTOR)
		{
			if(stageOperation == STAGE_BLENDFACTORALPHA)
			{
				return true;
			}
		}

		// One argument
		if(stageOperation == STAGE_SELECTARG1 || stageOperation == STAGE_PREMODULATE)
		{
			if(firstArgument == source && (firstModifier == MODIFIER_ALPHA || firstModifier == MODIFIER_INVALPHA))
			{
				return true;
			}
		}
		else if(stageOperation == STAGE_SELECTARG2)
		{
			if(secondArgument == source && (secondModifier == MODIFIER_ALPHA || secondModifier == MODIFIER_INVALPHA))
			{
				return true;
			}
		}
		else if(stageOperation == STAGE_SELECTARG3)
		{
			if(thirdArgument == source && (thirdModifier == MODIFIER_ALPHA || thirdModifier == MODIFIER_INVALPHA))
			{
				return true;
			}
		}
		else
		{
			// Two arguments or more
			if(firstArgument == source || secondArgument == source)
			{
				if(firstArgument == source && (firstModifier == MODIFIER_ALPHA || firstModifier == MODIFIER_INVALPHA))
				{
					return true;
				}

				if(secondArgument == source && (secondModifier == MODIFIER_ALPHA || secondModifier == MODIFIER_INVALPHA))
				{
					return true;
				}
			}

			// Three arguments
			if(stageOperation == STAGE_MULTIPLYADD || stageOperation == STAGE_LERP)
			{
				if(thirdArgument == source && (thirdModifier == MODIFIER_ALPHA || thirdModifier == MODIFIER_INVALPHA))
				{
					return true;
				}
			}
		}

		// One argument
		if(stageOperationAlpha == STAGE_SELECTARG1 || stageOperationAlpha == STAGE_PREMODULATE)
		{
			return firstArgumentAlpha == source;
		}
		else if(stageOperationAlpha == STAGE_SELECTARG2)
		{
			return secondArgumentAlpha == source;
		}
		else if(stageOperationAlpha == STAGE_SELECTARG3)
		{
			return thirdArgumentAlpha == source;
		}
		else
		{
			// Two arguments or more
			if(firstArgumentAlpha == source || secondArgumentAlpha == source)
			{
				return true;
			}

			// Three arguments
			if(stageOperationAlpha == STAGE_MULTIPLYADD || stageOperationAlpha == STAGE_LERP)
			{
				return thirdArgumentAlpha == source;
			}
		}
		
		return false;
	}

	bool TextureStage::uses(SourceArgument source) const
	{
		return usesColor(source) || usesAlpha(source);
	}

	bool TextureStage::usesCurrent() const
	{
		return uses(SOURCE_CURRENT) || (stageOperation == STAGE_BLENDCURRENTALPHA || stageOperationAlpha == STAGE_BLENDCURRENTALPHA);
	}

	bool TextureStage::usesDiffuse() const
	{
		return uses(SOURCE_DIFFUSE) || (stageOperation == STAGE_BLENDDIFFUSEALPHA || stageOperationAlpha == STAGE_BLENDDIFFUSEALPHA);
	}

	bool TextureStage::usesSpecular() const
	{
		return uses(SOURCE_SPECULAR);
	}

	bool TextureStage::usesTexture() const
	{
		return uses(SOURCE_TEXTURE) ||
		       stageOperation == STAGE_BLENDTEXTUREALPHA ||
		       stageOperationAlpha == STAGE_BLENDTEXTUREALPHA ||
		       stageOperation == STAGE_BLENDTEXTUREALPHAPM ||
		       stageOperationAlpha == STAGE_BLENDTEXTUREALPHAPM ||
		       (previousStage && previousStage->stageOperation == STAGE_PREMODULATE) ||
		       (previousStage && previousStage->stageOperationAlpha == STAGE_PREMODULATE);
	}

	bool TextureStage::isStageDisabled() const
	{
		bool disabled = (stageOperation == STAGE_DISABLE) || (!sampler->hasTexture() && usesTexture());

		if(!previousStage || disabled)
		{
			return disabled;
		}
		else
		{
			return previousStage->isStageDisabled();
		}
	}

	bool TextureStage::writesCurrent() const
	{
		return !isStageDisabled() && destinationArgument == DESTINATION_CURRENT && stageOperation != STAGE_BUMPENVMAP && stageOperation != STAGE_BUMPENVMAPLUMINANCE;
	}
}
