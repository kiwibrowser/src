/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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
 */ /*!
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/*!
 * \file  esextcTextureCubeMapArrayTextureSizeTests.cpp
 * \brief texture_cube_map_array extension - Texture Size Test (Test 10)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureCubeMapArrayImageTextureSizeTests.hpp"
#include "esextcTextureCubeMapArrayImageTextureSize.hpp"
#include "glcTestCase.hpp"
#include "glwEnums.inl"

namespace glcts
{

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
TextureCubeMapArrayImageTextureSizeTests::TextureCubeMapArrayImageTextureSizeTests(Context&				context,
																				   const ExtParameters& extParams,
																				   const char*			name,
																				   const char*			description)
	: TestCaseGroupBase(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Deinitializes tests group
 *
 **/
void TextureCubeMapArrayImageTextureSizeTests::deinit(void)
{
	/* Call base class' deinit() function. */
	glcts::TestCaseGroupBase::deinit();
}

/** Initializes tests group
 *
 **/
void TextureCubeMapArrayImageTextureSizeTests::init(void)
{
	addChild(new TextureCubeMapArrayTextureSizeTFVertexShader(m_context, m_extParams, "texture_size_vertex_sh",
															  "test 10.1"));
	addChild(new TextureCubeMapArrayTextureSizeTFGeometryShader(m_context, m_extParams, "texture_size_geometry_sh",
																"test 10.2"));
	addChild(new TextureCubeMapArrayTextureSizeTFTessControlShader(m_context, m_extParams,
																   "texture_size_tesselation_con_sh", "test 10.3"));
	addChild(new TextureCubeMapArrayTextureSizeTFTessEvaluationShader(m_context, m_extParams,
																	  "texture_size_tesselation_ev_sh", "test 10.4"));
	addChild(new TextureCubeMapArrayTextureSizeRTFragmentShader(m_context, m_extParams, "texture_size_fragment_sh",
																"test 10.5"));
	addChild(new TextureCubeMapArrayTextureSizeRTComputeShader(m_context, m_extParams, "texture_size_compute_sh",
															   "test 10.6"));
}

} // namespace glcts
