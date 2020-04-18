/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Sampler object state query tests
 *//*--------------------------------------------------------------------*/

#include "es31fSamplerStateQueryTests.hpp"
#include "glsTextureStateQueryTests.hpp"
#include "glsStateQueryUtil.hpp"
#include "glwEnums.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{

using namespace gls::StateQueryUtil;
using namespace gls::TextureStateQueryTests;

static const char* getVerifierSuffix (QueryType type)
{
	switch (type)
	{
		case QUERY_SAMPLER_PARAM_FLOAT:
		case QUERY_SAMPLER_PARAM_FLOAT_VEC4:					return "_float";

		case QUERY_SAMPLER_PARAM_INTEGER:
		case QUERY_SAMPLER_PARAM_INTEGER_VEC4:					return "_integer";

		case QUERY_SAMPLER_PARAM_PURE_INTEGER:
		case QUERY_SAMPLER_PARAM_PURE_INTEGER_VEC4:				return "_pure_int";

		case QUERY_SAMPLER_PARAM_PURE_UNSIGNED_INTEGER:
		case QUERY_SAMPLER_PARAM_PURE_UNSIGNED_INTEGER_VEC4:	return "_pure_uint";

		default:
			DE_ASSERT(DE_FALSE);
			return DE_NULL;
	}
}

static bool isIsVectorQuery (TesterType tester)
{
	return tester == TESTER_TEXTURE_BORDER_COLOR;
}

static bool isExtendedParamQuery (TesterType tester)
{
	return	tester == TESTER_TEXTURE_WRAP_S_CLAMP_TO_BORDER ||
			tester == TESTER_TEXTURE_WRAP_T_CLAMP_TO_BORDER ||
			tester == TESTER_TEXTURE_WRAP_R_CLAMP_TO_BORDER;
}

SamplerStateQueryTests::SamplerStateQueryTests (Context& context)
	: TestCaseGroup(context, "sampler", "Sampler state query tests")
{
}

void SamplerStateQueryTests::init (void)
{
	using namespace gls::TextureStateQueryTests;

	static const QueryType scalarVerifiers[] =
	{
		QUERY_SAMPLER_PARAM_INTEGER,
		QUERY_SAMPLER_PARAM_FLOAT,
		QUERY_SAMPLER_PARAM_PURE_INTEGER,
		QUERY_SAMPLER_PARAM_PURE_UNSIGNED_INTEGER,
	};
	static const QueryType nonPureVerifiers[] =
	{
		QUERY_SAMPLER_PARAM_INTEGER,
		QUERY_SAMPLER_PARAM_FLOAT,
	};
	static const QueryType vectorVerifiers[] =
	{
		QUERY_SAMPLER_PARAM_INTEGER_VEC4,
		QUERY_SAMPLER_PARAM_FLOAT_VEC4,
		QUERY_SAMPLER_PARAM_PURE_INTEGER_VEC4,
		QUERY_SAMPLER_PARAM_PURE_UNSIGNED_INTEGER_VEC4,
	};

#define FOR_EACH_VERIFIER(VERIFIERS, X) \
	for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(VERIFIERS); ++verifierNdx)	\
	{																						\
		const char* verifierSuffix = getVerifierSuffix((VERIFIERS)[verifierNdx]);			\
		const QueryType verifier = (VERIFIERS)[verifierNdx];								\
		targetGroup->addChild(X);															\
	}

	static const struct
	{
		const char*	name;
		const char*	desc;
		TesterType	tester;
		bool		newInGLES31;
	} states[] =
	{
		{ "texture_wrap_s",					"TEXTURE_WRAP_S",					TESTER_TEXTURE_WRAP_S,					false	},
		{ "texture_wrap_t",					"TEXTURE_WRAP_T",					TESTER_TEXTURE_WRAP_T,					false	},
		{ "texture_wrap_r",					"TEXTURE_WRAP_R",					TESTER_TEXTURE_WRAP_R,					false	},
		{ "texture_mag_filter",				"TEXTURE_MAG_FILTER",				TESTER_TEXTURE_MAG_FILTER,				false	},
		{ "texture_min_filter",				"TEXTURE_MIN_FILTER",				TESTER_TEXTURE_MIN_FILTER,				false	},
		{ "texture_min_lod",				"TEXTURE_MIN_LOD",					TESTER_TEXTURE_MIN_LOD,					false	},
		{ "texture_max_lod",				"TEXTURE_MAX_LOD",					TESTER_TEXTURE_MAX_LOD,					false	},
		{ "texture_compare_mode",			"TEXTURE_COMPARE_MODE",				TESTER_TEXTURE_COMPARE_MODE,			false	},
		{ "texture_compare_func",			"TEXTURE_COMPARE_FUNC",				TESTER_TEXTURE_COMPARE_FUNC,			false	},
		{ "texture_srgb_decode",			"TEXTURE_SRGB_DECODE_EXT",			TESTER_TEXTURE_SRGB_DECODE_EXT,			true	},
		{ "texture_border_color",			"TEXTURE_BORDER_COLOR",				TESTER_TEXTURE_BORDER_COLOR,			true	},
		{ "texture_wrap_s_clamp_to_border",	"TEXTURE_WRAP_S_CLAMP_TO_BORDER",	TESTER_TEXTURE_WRAP_S_CLAMP_TO_BORDER,	true	},
		{ "texture_wrap_t_clamp_to_border",	"TEXTURE_WRAP_T_CLAMP_TO_BORDER",	TESTER_TEXTURE_WRAP_T_CLAMP_TO_BORDER,	true	},
		{ "texture_wrap_r_clamp_to_border",	"TEXTURE_WRAP_R_CLAMP_TO_BORDER",	TESTER_TEXTURE_WRAP_R_CLAMP_TO_BORDER,	true	},
	};
	static const struct
	{
		const char* name;
		const char* desc;
		QueryType	verifier;
	} pureSetters[] =
	{
		{ "set_pure_int",	"Set state with pure int",			QUERY_SAMPLER_PARAM_PURE_INTEGER			},
		{ "set_pure_uint",	"Set state with pure unsigned int",	QUERY_SAMPLER_PARAM_PURE_UNSIGNED_INTEGER	},
	};
	static const struct
	{
		const char*	name;
		const char*	desc;
		TesterType	intTester;
		TesterType	uintTester;
	} pureStates[] =
	{
		{ "texture_wrap_s",					"TEXTURE_WRAP_S",					TESTER_TEXTURE_WRAP_S_SET_PURE_INT,				TESTER_TEXTURE_WRAP_S_SET_PURE_UINT					},
		{ "texture_wrap_t",					"TEXTURE_WRAP_T",					TESTER_TEXTURE_WRAP_T_SET_PURE_INT,				TESTER_TEXTURE_WRAP_T_SET_PURE_UINT					},
		{ "texture_wrap_r",					"TEXTURE_WRAP_R",					TESTER_TEXTURE_WRAP_R_SET_PURE_INT,				TESTER_TEXTURE_WRAP_R_SET_PURE_UINT					},
		{ "texture_mag_filter",				"TEXTURE_MAG_FILTER",				TESTER_TEXTURE_MAG_FILTER_SET_PURE_INT,			TESTER_TEXTURE_MAG_FILTER_SET_PURE_UINT				},
		{ "texture_min_filter",				"TEXTURE_MIN_FILTER",				TESTER_TEXTURE_MIN_FILTER_SET_PURE_INT,			TESTER_TEXTURE_MIN_FILTER_SET_PURE_UINT				},
		{ "texture_min_lod",				"TEXTURE_MIN_LOD",					TESTER_TEXTURE_MIN_LOD_SET_PURE_INT,			TESTER_TEXTURE_MIN_LOD_SET_PURE_UINT				},
		{ "texture_max_lod",				"TEXTURE_MAX_LOD",					TESTER_TEXTURE_MAX_LOD_SET_PURE_INT,			TESTER_TEXTURE_MAX_LOD_SET_PURE_UINT				},
		{ "texture_compare_mode",			"TEXTURE_COMPARE_MODE",				TESTER_TEXTURE_COMPARE_MODE_SET_PURE_INT,		TESTER_TEXTURE_COMPARE_MODE_SET_PURE_UINT			},
		{ "texture_compare_func",			"TEXTURE_COMPARE_FUNC",				TESTER_TEXTURE_COMPARE_FUNC_SET_PURE_INT,		TESTER_TEXTURE_COMPARE_FUNC_SET_PURE_UINT			},
		{ "texture_srgb_decode",			"TEXTURE_SRGB_DECODE_EXT",			TESTER_TEXTURE_SRGB_DECODE_EXT_SET_PURE_INT,	TESTER_TEXTURE_SRGB_DECODE_EXT_SET_PURE_UINT		},
		// \note texture_border_color is already checked
		// \note texture_wrap_*_clamp_to_border brings no additional coverage
	};

	// set_value
	{
		tcu::TestCaseGroup* const targetGroup = new tcu::TestCaseGroup(m_testCtx, "set_value", "Set value and query it");
		addChild(targetGroup);

		for (int stateNdx = 0; stateNdx < DE_LENGTH_OF_ARRAY(states); ++stateNdx)
		{
			// already tested in gles3
			if (!states[stateNdx].newInGLES31)
				continue;

			if (isExtendedParamQuery(states[stateNdx].tester))
			{
				// no need to cover for all getters if the only thing new is the param name
				FOR_EACH_VERIFIER(nonPureVerifiers, createSamplerParamTest(m_testCtx,
																		   m_context.getRenderContext(),
																		   std::string() + states[stateNdx].name + verifierSuffix,
																		   states[stateNdx].desc,
																		   verifier,
																		   states[stateNdx].tester));
			}
			else if (isIsVectorQuery(states[stateNdx].tester))
			{
				FOR_EACH_VERIFIER(vectorVerifiers, createSamplerParamTest(m_testCtx,
																		  m_context.getRenderContext(),
																		  std::string() + states[stateNdx].name + verifierSuffix,
																		  states[stateNdx].desc,
																		  verifier,
																		  states[stateNdx].tester));
			}
			else
			{
				FOR_EACH_VERIFIER(scalarVerifiers, createSamplerParamTest(m_testCtx,
																		  m_context.getRenderContext(),
																		  std::string() + states[stateNdx].name + verifierSuffix,
																		  states[stateNdx].desc,
																		  verifier,
																		  states[stateNdx].tester));
			}
		}
	}

#undef FOR_EACH_VERIFIER

	// set_pure_uint
	// set_pure_int
	for (int setterNdx = 0; setterNdx < DE_LENGTH_OF_ARRAY(pureSetters); ++setterNdx)
	{
		tcu::TestCaseGroup* const targetGroup = new tcu::TestCaseGroup(m_testCtx, pureSetters[setterNdx].name, pureSetters[setterNdx].desc);
		addChild(targetGroup);

		for (int stateNdx = 0; stateNdx < DE_LENGTH_OF_ARRAY(pureStates); ++stateNdx)
		{
			const TesterType	tester	= (pureSetters[setterNdx].verifier == QUERY_SAMPLER_PARAM_PURE_INTEGER)				? (pureStates[stateNdx].intTester)
										: (pureSetters[setterNdx].verifier == QUERY_SAMPLER_PARAM_PURE_UNSIGNED_INTEGER)	? (pureStates[stateNdx].uintTester)
										: (TESTER_LAST);

			targetGroup->addChild(createSamplerParamTest(m_testCtx,
														 m_context.getRenderContext(),
														 std::string() + pureStates[stateNdx].name,
														 pureStates[stateNdx].desc,
														 pureSetters[setterNdx].verifier,
														 tester));
		}
	}
}

} // Functional
} // gles31
} // deqp
