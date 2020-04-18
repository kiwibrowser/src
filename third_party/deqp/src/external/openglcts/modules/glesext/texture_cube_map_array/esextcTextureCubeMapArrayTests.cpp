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

#include "esextcTextureCubeMapArrayTests.hpp"
#include "esextcTextureCubeMapArrayColorDepthAttachments.hpp"
#include "esextcTextureCubeMapArrayETC2Support.hpp"
#include "esextcTextureCubeMapArrayFBOIncompleteness.hpp"
#include "esextcTextureCubeMapArrayGenerateMipMap.hpp"
#include "esextcTextureCubeMapArrayGetterCalls.hpp"
#include "esextcTextureCubeMapArrayImageOperations.hpp"
#include "esextcTextureCubeMapArrayImageTextureSize.hpp"
#include "esextcTextureCubeMapArraySampling.hpp"
#include "esextcTextureCubeMapArrayStencilAttachments.hpp"
#include "esextcTextureCubeMapArraySubImage3D.hpp"
#include "esextcTextureCubeMapArrayTex3DValidation.hpp"

namespace glcts
{
/** Constructor
 *
 * @param context       Test context
 * @param glslVersion   GLSL version
 **/
TextureCubeMapArrayTests::TextureCubeMapArrayTests(glcts::Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "texture_cube_map_array", "Texture Cube Map Array Tests")
{
	/* No implementation needed */
}

/** Initializes test cases for texture cube map array tests
 **/
void TextureCubeMapArrayTests::init(void)
{
	/* Initialize base class */
	TestCaseGroupBase::init();

	/* Texture Cube Map Array Sampling (Test 1) */
	addChild(new TextureCubeMapArraySamplingTest(m_context, m_extParams, "sampling", "Test 1"));

	/* Texture Cube Map Array Attachment (Test 2) */
	addChild(
		new TextureCubeMapArrayColorDepthAttachmentsTest(m_context, m_extParams, "color_depth_attachments", "Test 2"));

	/* Texture Cube Map Array Stencil Attachments (Test 3) */
	addChild(new TextureCubeMapArrayStencilAttachments(m_context, m_extParams, "stencil_attachments_mutable_nonlayered",
													   "Test 3", false, false));
	addChild(new TextureCubeMapArrayStencilAttachments(m_context, m_extParams, "stencil_attachments_mutable_layered",
													   "Test 3", false, true));
	addChild(new TextureCubeMapArrayStencilAttachments(
		m_context, m_extParams, "stencil_attachments_immutable_nonlayered", "Test 3", true, false));
	addChild(new TextureCubeMapArrayStencilAttachments(m_context, m_extParams, "stencil_attachments_immutable_layered",
													   "Test 3", true, true));

	/* Texture Cube Map Array glTexImage3D() and glTexStorage3D() Validation (Test 4) */
	addChild(new TextureCubeMapArrayTex3DValidation(m_context, m_extParams, "tex3D_validation", "Test 4"));

	/* Texture Cube Map Array SubImage3D() (Test 5) */
	addChild(new TextureCubeMapArraySubImage3D(m_context, m_extParams, "subimage3D", "Test 5"));

	/* Texture Cube Map Array Getter Calls (Test 6) */
	addChild(new TextureCubeMapArrayGetterCalls(m_context, m_extParams, "getter_calls", "Test 6"));

	/* Texture Cube Map Array glGenerateMipmap() Validation (Test 7) */
	addChild(new TextureCubeMapArrayGenerateMipMapFilterable(
		m_context, m_extParams, "generate_mip_map_filterable_internalformat_mutable",
		"Test 7.1 filterable internalformat - mutable storage", ST_MUTABLE));

	addChild(new TextureCubeMapArrayGenerateMipMapFilterable(
		m_context, m_extParams, "generate_mip_map_filterable_internalformat_immutable",
		"Test 7.1 filterable internalformat - immutable storage", ST_IMMUTABLE));

	/* The following tests, as written, are only applicable to ES. */
	addChild(new TextureCubeMapArrayGenerateMipMapNonFilterable(
		m_context, m_extParams, "generate_mip_map_non_filterable_mutable_storage",
		"Test 7.2 non-filterable format - mutable storage", ST_MUTABLE));

	addChild(new TextureCubeMapArrayGenerateMipMapNonFilterable(
		m_context, m_extParams, "generate_mip_map_non_filterable_immutable_storage",
		"Test 7.2 non-filterable format - immutable storage", ST_IMMUTABLE));

	/* Texture Cube Map Array Image Operations (Test 8) */
	addChild(new TextureCubeMapArrayImageOpCompute(m_context, m_extParams, "image_op_compute_sh",
												   "Test 8 compute shader", STC_COMPUTE_SHADER));
	addChild(new TextureCubeMapArrayImageOpCompute(m_context, m_extParams, "image_op_vertex_sh", "Test 8 vertex shader",
												   STC_VERTEX_SHADER));
	addChild(new TextureCubeMapArrayImageOpCompute(m_context, m_extParams, "image_op_fragment_sh",
												   "Test 8 fragment shader", STC_FRAGMENT_SHADER));
	addChild(new TextureCubeMapArrayImageOpCompute(m_context, m_extParams, "image_op_geometry_sh",
												   "Test 8 geometry shader", STC_GEOMETRY_SHADER));

	addChild(new TextureCubeMapArrayImageOpCompute(m_context, m_extParams, "image_op_tessellation_control_sh",
												   "Test 8 tessellation control shader ",
												   STC_TESSELLATION_CONTROL_SHADER));

	addChild(new TextureCubeMapArrayImageOpCompute(m_context, m_extParams, "image_op_tessellation_evaluation_sh",
												   "Test 8 tessellation evaluation sahder",
												   STC_TESSELLATION_EVALUATION_SHADER));

	/* Texture Cube Map Array FBO incompleteness (Test 9) */
	addChild(new TextureCubeMapArrayFBOIncompleteness(m_context, m_extParams, "fbo_incompleteness", "Test 9"));

	/* textureSize and imageSize validation (Test 10) */
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

	/* Cube Map Array support for ETC2 textures (Test 11) */
	glu::ContextType contextType = m_context.getRenderContext().getType();
	if (glu::isContextTypeGLCore(contextType) || glu::contextSupports(contextType, glu::ApiType::es(3, 2)))
		addChild(new TextureCubeMapArrayETC2Support(m_context, m_extParams, "etc2_texture", "test 11"));
}

} // namespace glcts
