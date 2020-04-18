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

#include "SetupProcessor.hpp"

#include "Primitive.hpp"
#include "Polygon.hpp"
#include "Context.hpp"
#include "Renderer.hpp"
#include "Shader/SetupRoutine.hpp"
#include "Shader/Constants.hpp"
#include "Common/Debug.hpp"

namespace sw
{
	extern bool complementaryDepthBuffer;
	extern bool fullPixelPositionRegister;

	bool precacheSetup = false;

	unsigned int SetupProcessor::States::computeHash()
	{
		unsigned int *state = (unsigned int*)this;
		unsigned int hash = 0;

		for(unsigned int i = 0; i < sizeof(States) / 4; i++)
		{
			hash ^= state[i];
		}

		return hash;
	}

	SetupProcessor::State::State(int i)
	{
		memset(this, 0, sizeof(State));
	}

	bool SetupProcessor::State::operator==(const State &state) const
	{
		if(hash != state.hash)
		{
			return false;
		}

		return memcmp(static_cast<const States*>(this), static_cast<const States*>(&state), sizeof(States)) == 0;
	}

	SetupProcessor::SetupProcessor(Context *context) : context(context)
	{
		routineCache = 0;
		setRoutineCacheSize(1024);
	}

	SetupProcessor::~SetupProcessor()
	{
		delete routineCache;
		routineCache = 0;
	}

	SetupProcessor::State SetupProcessor::update() const
	{
		State state;

		bool vPosZW = (context->pixelShader && context->pixelShader->isVPosDeclared() && fullPixelPositionRegister);

		state.isDrawPoint = context->isDrawPoint(true);
		state.isDrawLine = context->isDrawLine(true);
		state.isDrawTriangle = context->isDrawTriangle(false);
		state.isDrawSolidTriangle = context->isDrawTriangle(true);
		state.interpolateZ = context->depthBufferActive() || context->pixelFogActive() != FOG_NONE || vPosZW;
		state.interpolateW = context->perspectiveActive() || vPosZW;
		state.perspective = context->perspectiveActive();
		state.pointSprite = context->pointSpriteActive();
		state.cullMode = context->cullMode;
		state.twoSidedStencil = context->stencilActive() && context->twoSidedStencil;
		state.slopeDepthBias = context->slopeDepthBias != 0.0f;
		state.vFace = context->pixelShader && context->pixelShader->isVFaceDeclared();

		state.positionRegister = Pos;
		state.pointSizeRegister = Unused;

		state.multiSample = context->getMultiSampleCount();
		state.rasterizerDiscard = context->rasterizerDiscard;

		if(context->vertexShader)
		{
			state.positionRegister = context->vertexShader->getPositionRegister();
			state.pointSizeRegister = context->vertexShader->getPointSizeRegister();
		}
		else if(context->pointSizeActive())
		{
			state.pointSizeRegister = Pts;
		}

		for(int interpolant = 0; interpolant < MAX_FRAGMENT_INPUTS; interpolant++)
		{
			for(int component = 0; component < 4; component++)
			{
				state.gradient[interpolant][component].attribute = Unused;
				state.gradient[interpolant][component].flat = false;
				state.gradient[interpolant][component].wrap = false;
			}
		}

		state.fog.attribute = Unused;
		state.fog.flat = false;
		state.fog.wrap = false;

		const bool point = context->isDrawPoint(true);
		const bool sprite = context->pointSpriteActive();
		const bool flatShading = (context->shadingMode == SHADING_FLAT) || point;

		if(context->vertexShader && context->pixelShader)
		{
			for(int interpolant = 0; interpolant < MAX_FRAGMENT_INPUTS; interpolant++)
			{
				for(int component = 0; component < 4; component++)
				{
					int project = context->isProjectionComponent(interpolant - 2, component) ? 1 : 0;
					const Shader::Semantic& semantic = context->pixelShader->getInput(interpolant, component - project);

					if(semantic.active())
					{
						int input = interpolant;
						for(int i = 0; i < MAX_VERTEX_OUTPUTS; i++)
						{
							if(semantic == context->vertexShader->getOutput(i, component - project))
							{
								input = i;
								break;
							}
						}

						bool flat = point;

						switch(semantic.usage)
						{
						case Shader::USAGE_TEXCOORD: flat = point && !sprite;             break;
						case Shader::USAGE_COLOR:    flat = semantic.flat || flatShading; break;
						}

						state.gradient[interpolant][component].attribute = input;
						state.gradient[interpolant][component].flat = flat;
					}
				}
			}
		}
		else if(context->preTransformed && context->pixelShader)
		{
			for(int interpolant = 0; interpolant < MAX_FRAGMENT_INPUTS; interpolant++)
			{
				for(int component = 0; component < 4; component++)
				{
					const Shader::Semantic& semantic = context->pixelShader->getInput(interpolant, component);

					switch(semantic.usage)
					{
					case 0xFF:
						break;
					case Shader::USAGE_TEXCOORD:
						state.gradient[interpolant][component].attribute = T0 + semantic.index;
						state.gradient[interpolant][component].flat = semantic.flat || (point && !sprite);
						break;
					case Shader::USAGE_COLOR:
						state.gradient[interpolant][component].attribute = C0 + semantic.index;
						state.gradient[interpolant][component].flat = semantic.flat || flatShading;
						break;
					default:
						ASSERT(false);
					}
				}
			}
		}
		else if(context->pixelShaderModel() < 0x0300)
		{
			for(int coordinate = 0; coordinate < 8; coordinate++)
			{
				for(int component = 0; component < 4; component++)
				{
					if(context->textureActive(coordinate, component))
					{
						state.texture[coordinate][component].attribute = T0 + coordinate;
						state.texture[coordinate][component].flat = point && !sprite;
						state.texture[coordinate][component].wrap = (context->textureWrap[coordinate] & (1 << component)) != 0;
					}
				}
			}

			for(int color = 0; color < 2; color++)
			{
				for(int component = 0; component < 4; component++)
				{
					if(context->colorActive(color, component))
					{
						state.color[color][component].attribute = C0 + color;
						state.color[color][component].flat = flatShading;
					}
				}
			}
		}
		else ASSERT(false);

		if(context->fogActive())
		{
			state.fog.attribute = Fog;
			state.fog.flat = point;
		}

		state.hash = state.computeHash();

		return state;
	}

	Routine *SetupProcessor::routine(const State &state)
	{
		Routine *routine = routineCache->query(state);

		if(!routine)
		{
			SetupRoutine *generator = new SetupRoutine(state);
			generator->generate();
			routine = generator->getRoutine();
			delete generator;

			routineCache->add(state, routine);
		}

		return routine;
	}

	void SetupProcessor::setRoutineCacheSize(int cacheSize)
	{
		delete routineCache;
		routineCache = new RoutineCache<State>(clamp(cacheSize, 1, 65536), precacheSetup ? "sw-setup" : 0);
	}
}
