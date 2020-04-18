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

#include "esextcGPUShader5Tests.hpp"
#include "esextcGPUShader5AtomicCountersArrayIndexing.hpp"
#include "esextcGPUShader5FmaAccuracy.hpp"
#include "esextcGPUShader5FmaPrecision.cpp"
#include "esextcGPUShader5ImagesArrayIndexing.hpp"
#include "esextcGPUShader5PreciseQualifier.hpp"
#include "esextcGPUShader5SSBOArrayIndexing.hpp"
#include "esextcGPUShader5SamplerArrayIndexing.hpp"
#include "esextcGPUShader5TextureGatherOffset.hpp"
#include "esextcGPUShader5UniformBlocksArrayIndexing.hpp"

namespace glcts
{
/** Constructor
 *
 * @param context       Test context
 * @param glslVersion   GLSL version
 **/
GPUShader5Tests::GPUShader5Tests(glcts::Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "gpu_shader5", "GPU Shader5 tests")
{
	m_glslVersion = extParams.glslVersion;
}

/**
 * Initializes test groups for geometry shader tests
 **/
void GPUShader5Tests::init(void)
{
	/* Base class init */
	TestCaseGroupBase::init();

	/* Sampler Array Indexing (Test 1) */
	addChild(new GPUShader5SamplerArrayIndexing(m_context, m_extParams, "sampler_array_indexing", "Test 1"));

	/* Images Array Indexing (Test 2) */
	addChild(new GPUShader5ImagesArrayIndexing(m_context, m_extParams, "images_array_indexing", "Test 2"));

	/* Atomic Counters Array Indexing (Test 3) */
	addChild(
		new GPUShader5AtomicCountersArrayIndexing(m_context, m_extParams, "atomic_counters_array_indexing", "Test 3"));

	/* Uniform Blocks Array Indexing (Test 4) */
	addChild(
		new GPUShader5UniformBlocksArrayIndexing(m_context, m_extParams, "uniform_blocks_array_indexing", "Test 4"));

	if (m_glslVersion >= glu::GLSL_VERSION_430)
	{
		/* SSBO Array Indexing (Test 5) applicable only to OpenGL 4.x*/
		addChild(new GPUShader5SSBOArrayIndexing(m_context, m_extParams, "ssbo_array_indexing", "Test 5"));
	}

	/* GPUShader5 Precise Qualifier (Test 6) */
	addChild(new GPUShader5PreciseQualifier(m_context, m_extParams, "precise_qualifier", "Test 6"));

	/* Accuracy of the fma function (Test 7) */
	addChild(new GPUShader5FmaAccuracyTest(m_context, m_extParams, "fma_accuracy", "Test 7"));

	/* Uniform Blocks Array Indexing (Test 8) */
	addChild(new GPUShader5FmaPrecision<IDT_FLOAT>(m_context, m_extParams, "fma_precision_float", "Test 8 float"));
	addChild(new GPUShader5FmaPrecision<IDT_VEC2>(m_context, m_extParams, "fma_precision_vec2", "Test 8 vec2"));
	addChild(new GPUShader5FmaPrecision<IDT_VEC3>(m_context, m_extParams, "fma_precision_vec3", "Test 8 vec3"));
	addChild(new GPUShader5FmaPrecision<IDT_VEC4>(m_context, m_extParams, "fma_precision_vec4", "Test 8 vec4"));

	/* Texture gather offset (Tests 9, 10 and 11) */
	addChild(new GPUShader5TextureGatherOffsetColor2DRepeatCaseTest(
		m_context, m_extParams, "texture_gather_offset_color_repeat", "Test 9 - Color repeat case"));
	addChild(new GPUShader5TextureGatherOffsetColor2DArrayCaseTest(
		m_context, m_extParams, "texture_gather_offset_color_array", "Test 9 - Color texture array case"));
	addChild(new GPUShader5TextureGatherOffsetColor2DOffsetsCaseTest(
		m_context, m_extParams, "texture_gather_offsets_color", "Test 9 - Color offsets case"));
	addChild(new GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest(
		m_context, m_extParams, "texture_gather_offset_depth_repeat", "Test 10 - Depth repeat case"));
	addChild(new GPUShader5TextureGatherOffsetDepth2DRepeatYCaseTest(
		m_context, m_extParams, "texture_gather_offset_depth_repeat_y", "Test 10 - Depth repeat, vertical case"));
	addChild(new GPUShader5TextureGatherOffsetDepth2DArrayCaseTest(
		m_context, m_extParams, "texture_gather_offset_depth_array", "Test 10 - Depth array case"));
	addChild(new GPUShader5TextureGatherOffsetDepth2DOffsetsCaseTest(
		m_context, m_extParams, "texture_gather_offsets_depth", "Test 10 - Depth offsets case"));
	addChild(new GPUShader5TextureGatherOffsetColor2DClampToBorderCaseTest(
		m_context, m_extParams, "texture_gather_offset_color_clamp_to_border", "Test 11 - Color clamp to border case"));
	addChild(new GPUShader5TextureGatherOffsetColor2DClampToEdgeCaseTest(
		m_context, m_extParams, "texture_gather_offset_color_clamp_to_edge", "Test 11 - Color clamp to edge case"));
	addChild(new GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest(
		m_context, m_extParams, "texture_gather_offset_depth_clamp_border", "Test 11 - Depth clamp to border case"));
	addChild(new GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest(
		m_context, m_extParams, "texture_gather_offset_depth_clamp_edge", "Test 11 - Depth clamp to edge case"));
}

} // namespace glcts
