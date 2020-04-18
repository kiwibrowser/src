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
 * \brief Texture Param State Query tests.
 *//*--------------------------------------------------------------------*/

#include "es31fTextureStateQueryTests.hpp"
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
		case QUERY_TEXTURE_PARAM_FLOAT:
		case QUERY_TEXTURE_PARAM_FLOAT_VEC4:					return "_float";

		case QUERY_TEXTURE_PARAM_INTEGER:
		case QUERY_TEXTURE_PARAM_INTEGER_VEC4:					return "_integer";

		case QUERY_TEXTURE_PARAM_PURE_INTEGER:
		case QUERY_TEXTURE_PARAM_PURE_INTEGER_VEC4:				return "_pure_int";

		case QUERY_TEXTURE_PARAM_PURE_UNSIGNED_INTEGER:
		case QUERY_TEXTURE_PARAM_PURE_UNSIGNED_INTEGER_VEC4:	return "_pure_uint";

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

TextureStateQueryTests::TextureStateQueryTests (Context& context)
	: TestCaseGroup(context, "texture", "Texture State Query tests")
{
}

TextureStateQueryTests::~TextureStateQueryTests (void)
{
}

void TextureStateQueryTests::init (void)
{
	static const QueryType scalarVerifiers[] =
	{
		QUERY_TEXTURE_PARAM_INTEGER,
		QUERY_TEXTURE_PARAM_FLOAT,
		QUERY_TEXTURE_PARAM_PURE_INTEGER,
		QUERY_TEXTURE_PARAM_PURE_UNSIGNED_INTEGER,
	};
	static const QueryType nonPureVerifiers[] =
	{
		QUERY_TEXTURE_PARAM_INTEGER,
		QUERY_TEXTURE_PARAM_FLOAT,
	};
	static const QueryType vec4Verifiers[] =
	{
		QUERY_TEXTURE_PARAM_INTEGER_VEC4,
		QUERY_TEXTURE_PARAM_FLOAT_VEC4,
		QUERY_TEXTURE_PARAM_PURE_INTEGER_VEC4,
		QUERY_TEXTURE_PARAM_PURE_UNSIGNED_INTEGER_VEC4,
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
		glw::GLenum	target;
		bool		newInGLES31;
	} textureTargets[] =
	{
		{ "texture_2d",						GL_TEXTURE_2D,						false,	},
		{ "texture_3d",						GL_TEXTURE_3D,						false,	},
		{ "texture_2d_array",				GL_TEXTURE_2D_ARRAY,				false,	},
		{ "texture_cube_map",				GL_TEXTURE_CUBE_MAP,				false,	},
		{ "texture_2d_multisample",			GL_TEXTURE_2D_MULTISAMPLE,			true,	},
		{ "texture_2d_multisample_array",	GL_TEXTURE_2D_MULTISAMPLE_ARRAY,	true,	}, // GL_OES_texture_storage_multisample_2d_array
		{ "texture_buffer",					GL_TEXTURE_BUFFER,					true,	}, // GL_EXT_texture_buffer
		{ "texture_cube_array",				GL_TEXTURE_CUBE_MAP_ARRAY,			true,	}, // GL_EXT_texture_cube_map_array
	};
	static const struct
	{
		const char*	name;
		const char*	desc;
		TesterType	tester;
		bool		newInGLES31;
	} states[] =
	{
		{ "texture_swizzle_r",				"TEXTURE_SWIZZLE_R",				TESTER_TEXTURE_SWIZZLE_R,				false	},
		{ "texture_swizzle_g",				"TEXTURE_SWIZZLE_G",				TESTER_TEXTURE_SWIZZLE_G,				false	},
		{ "texture_swizzle_b",				"TEXTURE_SWIZZLE_B",				TESTER_TEXTURE_SWIZZLE_B,				false	},
		{ "texture_swizzle_a",				"TEXTURE_SWIZZLE_A",				TESTER_TEXTURE_SWIZZLE_A,				false	},
		{ "texture_wrap_s",					"TEXTURE_WRAP_S",					TESTER_TEXTURE_WRAP_S,					false	},
		{ "texture_wrap_t",					"TEXTURE_WRAP_T",					TESTER_TEXTURE_WRAP_T,					false	},
		{ "texture_wrap_r",					"TEXTURE_WRAP_R",					TESTER_TEXTURE_WRAP_R,					false	},
		{ "texture_mag_filter",				"TEXTURE_MAG_FILTER",				TESTER_TEXTURE_MAG_FILTER,				false	},
		{ "texture_min_filter",				"TEXTURE_MIN_FILTER",				TESTER_TEXTURE_MIN_FILTER,				false	},
		{ "texture_min_lod",				"TEXTURE_MIN_LOD",					TESTER_TEXTURE_MIN_LOD,					false	},
		{ "texture_max_lod",				"TEXTURE_MAX_LOD",					TESTER_TEXTURE_MAX_LOD,					false	},
		{ "texture_base_level",				"TEXTURE_BASE_LEVEL",				TESTER_TEXTURE_BASE_LEVEL,				false	},
		{ "texture_max_level",				"TEXTURE_MAX_LEVEL",				TESTER_TEXTURE_MAX_LEVEL,				false	},
		{ "texture_compare_mode",			"TEXTURE_COMPARE_MODE",				TESTER_TEXTURE_COMPARE_MODE,			false	},
		{ "texture_compare_func",			"TEXTURE_COMPARE_FUNC",				TESTER_TEXTURE_COMPARE_FUNC,			false	},
		{ "texture_immutable_levels",		"TEXTURE_IMMUTABLE_LEVELS",			TESTER_TEXTURE_IMMUTABLE_LEVELS,		false	},
		{ "texture_immutable_format",		"TEXTURE_IMMUTABLE_FORMAT",			TESTER_TEXTURE_IMMUTABLE_FORMAT,		false	},
		{ "depth_stencil_mode",				"DEPTH_STENCIL_TEXTURE_MODE",		TESTER_DEPTH_STENCIL_TEXTURE_MODE,		true	},
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
		{ "set_pure_int",	"Set state with pure int",			QUERY_TEXTURE_PARAM_PURE_INTEGER			},
		{ "set_pure_uint",	"Set state with pure unsigned int",	QUERY_TEXTURE_PARAM_PURE_UNSIGNED_INTEGER	},
	};
	static const struct
	{
		const char*	name;
		const char*	desc;
		TesterType	intTester;
		TesterType	uintTester;
	} pureStates[] =
	{
		{ "texture_swizzle_r",				"TEXTURE_SWIZZLE_R",				TESTER_TEXTURE_SWIZZLE_R_SET_PURE_INT,			TESTER_TEXTURE_SWIZZLE_R_SET_PURE_UINT				},
		{ "texture_swizzle_g",				"TEXTURE_SWIZZLE_G",				TESTER_TEXTURE_SWIZZLE_G_SET_PURE_INT,			TESTER_TEXTURE_SWIZZLE_G_SET_PURE_UINT				},
		{ "texture_swizzle_b",				"TEXTURE_SWIZZLE_B",				TESTER_TEXTURE_SWIZZLE_B_SET_PURE_INT,			TESTER_TEXTURE_SWIZZLE_B_SET_PURE_UINT				},
		{ "texture_swizzle_a",				"TEXTURE_SWIZZLE_A",				TESTER_TEXTURE_SWIZZLE_A_SET_PURE_INT,			TESTER_TEXTURE_SWIZZLE_A_SET_PURE_UINT				},
		{ "texture_wrap_s",					"TEXTURE_WRAP_S",					TESTER_TEXTURE_WRAP_S_SET_PURE_INT,				TESTER_TEXTURE_WRAP_S_SET_PURE_UINT					},
		{ "texture_wrap_t",					"TEXTURE_WRAP_T",					TESTER_TEXTURE_WRAP_T_SET_PURE_INT,				TESTER_TEXTURE_WRAP_T_SET_PURE_UINT					},
		{ "texture_wrap_r",					"TEXTURE_WRAP_R",					TESTER_TEXTURE_WRAP_R_SET_PURE_INT,				TESTER_TEXTURE_WRAP_R_SET_PURE_UINT					},
		{ "texture_mag_filter",				"TEXTURE_MAG_FILTER",				TESTER_TEXTURE_MAG_FILTER_SET_PURE_INT,			TESTER_TEXTURE_MAG_FILTER_SET_PURE_UINT				},
		{ "texture_min_filter",				"TEXTURE_MIN_FILTER",				TESTER_TEXTURE_MIN_FILTER_SET_PURE_INT,			TESTER_TEXTURE_MIN_FILTER_SET_PURE_UINT				},
		{ "texture_min_lod",				"TEXTURE_MIN_LOD",					TESTER_TEXTURE_MIN_LOD_SET_PURE_INT,			TESTER_TEXTURE_MIN_LOD_SET_PURE_UINT				},
		{ "texture_max_lod",				"TEXTURE_MAX_LOD",					TESTER_TEXTURE_MAX_LOD_SET_PURE_INT,			TESTER_TEXTURE_MAX_LOD_SET_PURE_UINT				},
		{ "texture_base_level",				"TEXTURE_BASE_LEVEL",				TESTER_TEXTURE_BASE_LEVEL_SET_PURE_INT,			TESTER_TEXTURE_BASE_LEVEL_SET_PURE_UINT				},
		{ "texture_max_level",				"TEXTURE_MAX_LEVEL",				TESTER_TEXTURE_MAX_LEVEL_SET_PURE_INT,			TESTER_TEXTURE_MAX_LEVEL_SET_PURE_UINT				},
		{ "texture_compare_mode",			"TEXTURE_COMPARE_MODE",				TESTER_TEXTURE_COMPARE_MODE_SET_PURE_INT,		TESTER_TEXTURE_COMPARE_MODE_SET_PURE_UINT			},
		{ "texture_compare_func",			"TEXTURE_COMPARE_FUNC",				TESTER_TEXTURE_COMPARE_FUNC_SET_PURE_INT,		TESTER_TEXTURE_COMPARE_FUNC_SET_PURE_UINT			},
		// \note texture_immutable_levels is not settable
		// \note texture_immutable_format is not settable
		{ "depth_stencil_mode",				"DEPTH_STENCIL_TEXTURE_MODE",		TESTER_DEPTH_STENCIL_TEXTURE_MODE_SET_PURE_INT,	TESTER_DEPTH_STENCIL_TEXTURE_MODE_SET_PURE_UINT		},
		{ "texture_srgb_decode",			"TEXTURE_SRGB_DECODE_EXT",			TESTER_TEXTURE_SRGB_DECODE_EXT_SET_PURE_INT,	TESTER_TEXTURE_SRGB_DECODE_EXT_SET_PURE_UINT		},
		// \note texture_border_color is already checked
		// \note texture_wrap_*_clamp_to_border brings no additional coverage
	};

	for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(textureTargets); ++targetNdx)
	{
		tcu::TestCaseGroup* const targetGroup = new tcu::TestCaseGroup(m_testCtx, textureTargets[targetNdx].name, textureTargets[targetNdx].name);
		addChild(targetGroup);

		if (textureTargets[targetNdx].newInGLES31)
		{
			targetGroup->addChild(createIsTextureTest(m_testCtx,
													  m_context.getRenderContext(),
													  "is_texture",
													  "IsTexture",
													  textureTargets[targetNdx].target));
		}

		for (int stateNdx = 0; stateNdx < DE_LENGTH_OF_ARRAY(states); ++stateNdx)
		{
			if (!isLegalTesterForTarget(textureTargets[targetNdx].target, states[stateNdx].tester))
				continue;

			// for old targets, check only new states
			if (!textureTargets[targetNdx].newInGLES31 && !states[stateNdx].newInGLES31)
				continue;

			if (isExtendedParamQuery(states[stateNdx].tester))
			{
				// no need to cover for all getters if the only thing new is the param name
				FOR_EACH_VERIFIER(nonPureVerifiers, createTexParamTest(m_testCtx,
																	   m_context.getRenderContext(),
																	   std::string() + states[stateNdx].name + verifierSuffix,
																	   states[stateNdx].desc,
																	   verifier,
																	   textureTargets[targetNdx].target,
																	   states[stateNdx].tester));
			}
			else if (isIsVectorQuery(states[stateNdx].tester))
			{
				FOR_EACH_VERIFIER(vec4Verifiers, createTexParamTest(m_testCtx,
																	m_context.getRenderContext(),
																	std::string() + states[stateNdx].name + verifierSuffix,
																	states[stateNdx].desc,
																	verifier,
																	textureTargets[targetNdx].target,
																	states[stateNdx].tester));
			}
			else
			{
				FOR_EACH_VERIFIER(scalarVerifiers, createTexParamTest(m_testCtx,
																	  m_context.getRenderContext(),
																	  std::string() + states[stateNdx].name + verifierSuffix,
																	  states[stateNdx].desc,
																	  verifier,
																	  textureTargets[targetNdx].target,
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
			const TesterType	tester	= (pureSetters[setterNdx].verifier == QUERY_TEXTURE_PARAM_PURE_INTEGER)				? (pureStates[stateNdx].intTester)
										: (pureSetters[setterNdx].verifier == QUERY_TEXTURE_PARAM_PURE_UNSIGNED_INTEGER)	? (pureStates[stateNdx].uintTester)
										: (TESTER_LAST);
			// need 3d texture to test R wrap
			const glw::GLenum	target	= (pureStates[stateNdx].intTester == TESTER_TEXTURE_WRAP_R_SET_PURE_INT) ? (GL_TEXTURE_3D) : (GL_TEXTURE_2D);

			targetGroup->addChild(createTexParamTest(m_testCtx,
													 m_context.getRenderContext(),
													 std::string() + pureStates[stateNdx].name,
													 pureStates[stateNdx].desc,
													 pureSetters[setterNdx].verifier,
													 target,
													 tester));
		}
	}
}

} // Functional
} // gles31
} // deqp
