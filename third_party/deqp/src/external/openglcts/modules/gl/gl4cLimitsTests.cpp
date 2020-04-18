/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file gl4cLimitsTests.cpp
 * \brief Verifies all limits
 */ /*-------------------------------------------------------------------*/

#include "gl4cLimitsTests.hpp"
#include "glcLimitTest.hpp"

using namespace glw;

namespace gl4cts
{

template <typename Type>
struct Limit
{
	const char* name;
	deUint32	token;
	Type		boundry;
	bool		isMaximum; // when true boundry is maximal acceptable value
	const char* builtin;
};

/** Constructor.
 *
 *  @param context Rendering context.
 **/
LimitsTests::LimitsTests(deqp::Context& context) : TestCaseGroup(context, "limits", "Verifies all limits")
{
}

void LimitsTests::init(void)
{
	const GLint	minVertexUniformBlocks		= 14;
	const GLint	minVertexUniformComponents	= 1024;

	const GLint	minGeometryUniformBlocks		= 14;
	const GLint	minGeometryUniformComponents	= 1024;

	const GLint	minTessControlUniformBlocks		= 14;
	const GLint	minTessControlUniformComponents	= 1024;

	const GLint	minTessEvaluationUniformBlocks		= 14;
	const GLint	minTessEvaluationUniformComponents	= 1024;

	const GLint	minFragmentUniformBlocks		= 14;
	const GLint	minFragmentUniformComponents	= 1024;

	const GLint	minUniformBlockSize	= 16384;

	const GLint	cvuc  = (minVertexUniformBlocks*minUniformBlockSize)/4 + minVertexUniformComponents;
	const GLint	cguc  = (minGeometryUniformBlocks*minUniformBlockSize)/4 + minGeometryUniformComponents;
	const GLint	ctcuc = (minTessControlUniformBlocks*minUniformBlockSize)/4 + minTessControlUniformComponents;
	const GLint	cteuc = (minTessEvaluationUniformBlocks*minUniformBlockSize)/4 + minTessEvaluationUniformComponents;
	const GLint	cfuc  = (minFragmentUniformBlocks*minUniformBlockSize)/4 + minFragmentUniformComponents;

	static const Limit<GLint> intLimits[] =
	{
		{ "max_clip_distances",								 GL_MAX_CLIP_DISTANCES,								 8,		0, "gl_MaxClipDistances" },
		{ "max_cull_distances",								 GL_MAX_CULL_DISTANCES,								 8,		0, "gl_MaxCullDistances" },
		{ "max_combined_clip_and_cull_distances",			 GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES,			 8,		0, "gl_MaxCombinedClipAndCullDistances" },
		{ "max_3d_texture_size",							 GL_MAX_3D_TEXTURE_SIZE,							 2048,  0, "" },
		{ "max_texture_size",								 GL_MAX_TEXTURE_SIZE,								 16384, 0, "" },
		{ "max_array_texture_layers",						 GL_MAX_ARRAY_TEXTURE_LAYERS,						 2048,  0, "" },
		{ "max_cube_map_texture_size",						 GL_MAX_CUBE_MAP_TEXTURE_SIZE,						 16384, 0, "" },
		{ "max_renderbuffer_size",							 GL_MAX_RENDERBUFFER_SIZE,							 16384, 0, "" },
		{ "max_viewports",									 GL_MAX_VIEWPORTS,									 16,	0, "gl_MaxViewports" },
		{ "max_elements_indices",							 GL_MAX_ELEMENTS_INDICES,							 0,		0, "" }, // there is no minimum
		{ "max_elements_vertices",							 GL_MAX_ELEMENTS_VERTICES,							 0,		0, "" },
		{ "max_vertex_attrib_relative_offset",				 GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET,				 2047,  0, "" },
		{ "max_vertex_attrib_bindings",						 GL_MAX_VERTEX_ATTRIB_BINDINGS,						 16,	0, "" },
		{ "max_vertex_attrib_stride",						 GL_MAX_VERTEX_ATTRIB_STRIDE,						 2048,  0, "" },
		{ "max_texture_buffer_size",						 GL_MAX_TEXTURE_BUFFER_SIZE,						 65536, 0, "" },
		{ "max_rectangle_texture_size",						 GL_MAX_RECTANGLE_TEXTURE_SIZE,						 16384, 0, "" },
		{ "min_map_buffer_alignment",						 GL_MIN_MAP_BUFFER_ALIGNMENT,						 64,	1, "" },
		{ "max_vertex_attribs",								 GL_MAX_VERTEX_ATTRIBS,								 16,	0, "gl_MaxVertexAttribs" },
		{ "max_vertex_uniform_components",					 GL_MAX_VERTEX_UNIFORM_COMPONENTS,					 1024,  0, "gl_MaxVertexUniformComponents" },
		{ "max_vertex_uniform_vectors",						 GL_MAX_VERTEX_UNIFORM_VECTORS,						 256,   0, "gl_MaxVertexUniformVectors" },
		{ "max_vertex_uniform_blocks",						 GL_MAX_VERTEX_UNIFORM_BLOCKS,						 14,	0, "" },
		{ "max_vertex_output_components",					 GL_MAX_VERTEX_OUTPUT_COMPONENTS,					 64,	0, "gl_MaxVertexOutputComponents" },
		{ "max_vertex_texture_image_units",					 GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,					 16,	0, "gl_MaxVertexTextureImageUnits" },
		{ "max_vertex_atomic_counter_buffers",				 GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS,				 0,		0, "gl_MaxVertexAtomicCounterBuffers" },
		{ "max_vertex_atomic_counters",						 GL_MAX_VERTEX_ATOMIC_COUNTERS,						 0,		0, "gl_MaxVertexAtomicCounters" },
		{ "max_vertex_shader_storage_blocks",				 GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,				 0,		0, "" },
		{ "max_tess_gen_level",								 GL_MAX_TESS_GEN_LEVEL,								 64,	0, "gl_MaxTessGenLevel" },
		{ "max_patch_vertices",								 GL_MAX_PATCH_VERTICES,								 32,	0, "gl_MaxPatchVertices" },
		{ "max_tess_control_uniform_components",			 GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS,			 1024,  0, "gl_MaxTessControlUniformComponents" },
		{ "max_tess_control_texture_image_units",			 GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS,			 16,	0, "gl_MaxTessControlTextureImageUnits" },
		{ "max_tess_control_output_components",				 GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS,				 128,   0, "gl_MaxTessControlOutputComponents" },
		{ "max_tess_patch_components",						 GL_MAX_TESS_PATCH_COMPONENTS,						 120,   0, "gl_MaxTessPatchComponents" },
		{ "max_tess_control_total_output_components",		 GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS,		 4096,  0, "gl_MaxTessControlTotalOutputComponents" },
		{ "max_tess_control_input_components",				 GL_MAX_TESS_CONTROL_INPUT_COMPONENTS,				 128,   0, "gl_MaxTessControlInputComponents" },
		{ "max_tess_control_uniform_blocks",				 GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS,				 14,	0, "" },
		{ "max_tess_control_atomic_counter_buffers",		 GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS,		 0,		0, "gl_MaxTessControlAtomicCounterBuffers" },
		{ "max_tess_control_atomic_counters",				 GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS,				 0,		0, "gl_MaxTessControlAtomicCounters" },
		{ "max_tess_control_shader_storage_blocks",			 GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS,			 0,		0, "" },
		{ "max_tess_evaluation_uniform_components",			 GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS,			 1024,  0, "gl_MaxTessEvaluationUniformComponents" },
		{ "max_tess_evaluation_texture_image_units",		 GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS,		 16,	0, "gl_MaxTessEvaluationTextureImageUnits" },
		{ "max_tess_evaluation_output_components",			 GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS,			 128,	0, "gl_MaxTessEvaluationOutputComponents" },
		{ "max_tess_evaluation_input_components",			 GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS,			 128,	0, "gl_MaxTessEvaluationInputComponents" },
		{ "max_tess_evaluation_uniform_blocks",				 GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS,				 14,	0, "" },
		{ "max_tess_evaluation_atomic_counter_buffers",		 GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS,		 0,		0, "gl_MaxTessEvaluationAtomicCounterBuffers" },
		{ "max_tess_evaluation_atomic_counters",			 GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS,			 0,		0, "gl_MaxTessEvaluationAtomicCounters" },
		{ "max_tess_evaluation_shader_storage_blocks",		 GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS,		 0,		0, "" },
		{ "max_geometry_uniform_components",				 GL_MAX_GEOMETRY_UNIFORM_COMPONENTS,				 1024,  0, "gl_MaxGeometryUniformComponents" },
		{ "max_geometry_uniform_blocks",					 GL_MAX_GEOMETRY_UNIFORM_BLOCKS,					 14,	0, "" },
		{ "max_geometry_input_components",					 GL_MAX_GEOMETRY_INPUT_COMPONENTS,					 64,	0, "gl_MaxGeometryInputComponents" },
		{ "max_geometry_output_components",					 GL_MAX_GEOMETRY_OUTPUT_COMPONENTS,					 128,   0, "gl_MaxGeometryOutputComponents" },
		{ "max_geometry_output_vertices",					 GL_MAX_GEOMETRY_OUTPUT_VERTICES,					 256,   0, "gl_MaxGeometryOutputVertices" },
		{ "max_geometry_total_output_components",			 GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS,			 1024,  0, "gl_MaxGeometryTotalOutputComponents" },
		{ "max_geometry_texture_image_units",				 GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS,				 16,	0, "gl_MaxGeometryTextureImageUnits" },
		{ "max_geometry_shader_invocations",				 GL_MAX_GEOMETRY_SHADER_INVOCATIONS,				 32,	0, "" },
		{ "max_vertex_streams",								 GL_MAX_VERTEX_STREAMS,								 4,		0, "" },
		{ "max_geometry_atomic_counter_buffers",			 GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS,			 0,		0, "gl_MaxGeometryAtomicCounterBuffers" },
		{ "max_geometry_atomic_counters",					 GL_MAX_GEOMETRY_ATOMIC_COUNTERS,					 0,		0, "gl_MaxGeometryAtomicCounters" },
		{ "max_geometry_shader_storage_blocks",				 GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS,				 0,		0, "" },
		{ "max_fragment_uniform_components",				 GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,				 1024,  0, "gl_MaxFragmentUniformComponents" },
		{ "max_fragment_uniform_vectors",					 GL_MAX_FRAGMENT_UNIFORM_VECTORS,					 256,   0, "gl_MaxFragmentUniformVectors" },
		{ "max_fragment_uniform_blocks",					 GL_MAX_FRAGMENT_UNIFORM_BLOCKS,					 14,	0, "" },
		{ "max_fragment_input_components",					 GL_MAX_FRAGMENT_INPUT_COMPONENTS,					 128,   0, "gl_MaxFragmentInputComponents" },
		{ "max_texture_image_units",						 GL_MAX_TEXTURE_IMAGE_UNITS,						 16,	0, "gl_MaxTextureImageUnits" },
		{ "min_program_texture_gather_offset",				 GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET,				-8,		1, "" },
		{ "max_program_texture_gather_offset",				 GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET,				 7,		0, "" },
		{ "max_fragment_atomic_counter_buffers",			 GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS,			 1,		0, "gl_MaxFragmentAtomicCounterBuffers" },
		{ "max_fragment_atomic_counters",					 GL_MAX_FRAGMENT_ATOMIC_COUNTERS,					 8,		0, "gl_MaxFragmentAtomicCounters" },
		{ "max_fragment_shader_storage_blocks",				 GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,				 8,		0, "" },
		{ "max_compute_work_group_invocations",				 GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,				 1024,	0, "" },
		{ "max_compute_uniform_blocks",						 GL_MAX_COMPUTE_UNIFORM_BLOCKS,						 14,	0, "" },
		{ "max_compute_texture_image_units",				 GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS,				 16,	0, "gl_MaxComputeTextureImageUnits" },
		{ "max_compute_atomic_counter_buffers",				 GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS,				 8,		0, "gl_MaxComputeAtomicCounterBuffers" },
		{ "max_compute_atomic_counters",					 GL_MAX_COMPUTE_ATOMIC_COUNTERS,					 8,		0, "gl_MaxComputeAtomicCounters" },
		{ "max_compute_shared_memory_size",					 GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,					 32768, 0, "" },
		{ "max_compute_uniform_components",					 GL_MAX_COMPUTE_UNIFORM_COMPONENTS,					 1024,  0, "gl_MaxComputeUniformComponents" },
		{ "max_compute_image_uniforms",						 GL_MAX_COMPUTE_IMAGE_UNIFORMS,						 8,		0, "gl_MaxComputeImageUniforms" },
		{ "max_combined_compute_uniform_components",		 GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS,		 0,		0, "" }, // minimum was not specified
		{ "max_compute_shader_storage_blocks",				 GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,				 8,		0, "" },
		{ "min_program_texel_offset",						 GL_MIN_PROGRAM_TEXEL_OFFSET,						-8,		1, "gl_MinProgramTexelOffset" },
		{ "max_program_texel_offset",						 GL_MAX_PROGRAM_TEXEL_OFFSET,						 7,		0, "gl_MaxProgramTexelOffset" },
		{ "max_uniform_buffer_bindings",					 GL_MAX_UNIFORM_BUFFER_BINDINGS,					 84,	0, "" },
		{ "max_uniform_block_size",							 GL_MAX_UNIFORM_BLOCK_SIZE,							 16384, 0, "" },
		{ "max_combined_uniform_blocks",					 GL_MAX_COMBINED_UNIFORM_BLOCKS,					 70,	0, "" },
		{ "max_varying_components",							 GL_MAX_VARYING_COMPONENTS,							 60,	0, "gl_MaxVaryingComponents" },
		{ "max_varying_vectors",							 GL_MAX_VARYING_VECTORS,							 15,	0, "gl_MaxVaryingVectors" },
		{ "max_combined_texture_image_units",				 GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,				 80,	0, "gl_MaxCombinedTextureImageUnits" },
		{ "max_subroutines",								 GL_MAX_SUBROUTINES,								 256,	0, "" },
		{ "max_subroutine_uniform_locations",				 GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS,				 1024,  0, "" },
		{ "max_uniform_locations",							 GL_MAX_UNIFORM_LOCATIONS,							 1024,  0, "" },
		{ "max_atomic_counter_buffer_bindings",				 GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS,				 1,		0, "" },
		{ "max_atomic_counter_buffer_size",					 GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE,					 32,	0, "gl_MaxAtomicCounterBufferSize" },
		{ "max_combined_atomic_counter_buffers",			 GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS,			 1,		0, "gl_MaxCombinedAtomicCounterBuffers" },
		{ "max_combined_atomic_counters",					 GL_MAX_COMBINED_ATOMIC_COUNTERS,					 8,		0, "gl_MaxCombinedAtomicCounters" },
		{ "max_shader_storage_buffer_bindings",				 GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,				 8,		0, "" },
		{ "max_combined_shader_storage_blocks",				 GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS,				 8,		0, "" },
		{ "max_image_units",								 GL_MAX_IMAGE_UNITS,								 8,		0, "gl_MaxImageUnits" },
		{ "max_combined_shader_output_resources",			 GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES,			 8,		0, "gl_MaxCombinedShaderOutputResources" },
		{ "max_image_samples",								 GL_MAX_IMAGE_SAMPLES,								 0,		0, "gl_MaxImageSamples" },
		{ "max_vertex_image_uniforms",						 GL_MAX_VERTEX_IMAGE_UNIFORMS,						 0,		0, "gl_MaxVertexImageUniforms" },
		{ "max_tess_control_image_uniforms",				 GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS,				 0,		0, "gl_MaxTessControlImageUniforms" },
		{ "max_tess_evaluation_image_uniforms",				 GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS,				 0,		0, "gl_MaxTessEvaluationImageUniforms" },
		{ "max_geometry_image_uniforms",					 GL_MAX_GEOMETRY_IMAGE_UNIFORMS,					 0,		0, "gl_MaxGeometryImageUniforms" },
		{ "max_fragment_image_uniforms",					 GL_MAX_FRAGMENT_IMAGE_UNIFORMS,					 8,		0, "gl_MaxFragmentImageUniforms" },
		{ "max_combined_image_uniforms",					 GL_MAX_COMBINED_IMAGE_UNIFORMS,					 8,		0, "gl_MaxCombinedImageUniforms" },
		{ "max_combined_vertex_uniform_components",			 GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS,			 cvuc,	0, "" },
		{ "max_combined_geometry_uniform_components",		 GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS,		 cguc,	0, "" },
		{ "max_combined_tess_control_uniform_components",	 GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS,	 ctcuc,	0, "" },
		{ "max_combined_tess_evaluation_uniform_components", GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS, cteuc, 0, "" },
		{ "max_combined_fragment_uniform_components",		 GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS,		 cfuc,	0, "" },
		{ "max_debug_message_length",						 GL_MAX_DEBUG_MESSAGE_LENGTH,						 1,		0, "" },
		{ "max_debug_logged_messages",						 GL_MAX_DEBUG_LOGGED_MESSAGES,						 1,		0, "" },
		{ "max_debug_group_stack_depth",					 GL_MAX_DEBUG_GROUP_STACK_DEPTH,					 64,	0, "" },
		{ "max_label_length",								 GL_MAX_LABEL_LENGTH,								 256,	0, "" },
		{ "max_framebuffer_width",							 GL_MAX_FRAMEBUFFER_WIDTH,							 16384, 0, "" },
		{ "max_framebuffer_height",							 GL_MAX_FRAMEBUFFER_HEIGHT,							 16384, 0, "" },
		{ "max_framebuffer_layers",							 GL_MAX_FRAMEBUFFER_LAYERS,							 2048,  0, "" },
		{ "max_framebuffer_samples",						 GL_MAX_FRAMEBUFFER_SAMPLES,						 4,		0, "" },
		{ "max_sample_mask_words",							 GL_MAX_SAMPLE_MASK_WORDS,							 1,		0, "" },
		{ "max_samples",									 GL_MAX_SAMPLES,									 4,		0, "gl_MaxSamples" },
		{ "max_color_texture_samples",						 GL_MAX_COLOR_TEXTURE_SAMPLES,						 1,		0, "" },
		{ "max_depth_texture_samples",						 GL_MAX_DEPTH_TEXTURE_SAMPLES,						 1,		0, "" },
		{ "max_integer_samples",							 GL_MAX_INTEGER_SAMPLES,							 1,		0, "" },
		{ "max_draw_buffers",								 GL_MAX_DRAW_BUFFERS,								 8,		0, "gl_MaxDrawBuffers" },
		{ "max_dual_source_draw_buffers",					 GL_MAX_DUAL_SOURCE_DRAW_BUFFERS,					 1,		0, "" },
		{ "max_color_attachments",							 GL_MAX_COLOR_ATTACHMENTS,							 8,		0, "" },
		{ "max_transform_feedback_interleaved_components",   GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS,   64,	0, "gl_MaxTransformFeedbackInterleavedComponents" },
		{ "max_transform_feedback_separate_attribs",		 GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS,		 4,		0, "" },
		{ "max_transform_feedback_separate_components",		 GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS,		 4,		0, "" },
		{ "max_transform_feedback_buffers",					 GL_MAX_TRANSFORM_FEEDBACK_BUFFERS,					 4,		0, "gl_MaxTransformFeedbackBuffers" },
		{ "max_atomic_counter_bindings",					 GL_NONE,											 4,		0, "gl_MaxAtomicCounterBindings" },
		{ "max_combined_image_units_and_fragment_outputs",	 GL_NONE,											 4,		0, "gl_MaxCombinedImageUnitsAndFragmentOutputs" },
		{ "max_geometry_varying_components",				 GL_NONE,											 4,		0, "gl_MaxGeometryVaryingComponents" }
	};

	static const Limit<GLint64> int64Limits[] =
	{
		{ "max_shader_storage_block_size",					 GL_MAX_SHADER_STORAGE_BLOCK_SIZE,					 134217728,		0, "" },
		{ "max_element_index",								 GL_MAX_ELEMENT_INDEX,								 4294967295LL,	0, "" },
	};

	static const Limit<GLuint64> uint64Limits[] =
	{
		{ "max_server_wait_timeout",						 GL_MAX_SERVER_WAIT_TIMEOUT,						 0,				0, "" },
	};

	static const Limit<GLfloat> floatLimits[] =
	{
		{ "max_texture_lod_bias",							 GL_MAX_TEXTURE_LOD_BIAS,							 2.0,	0, "" },
		{ "min_fragment_interpolation_offset",				 GL_MIN_FRAGMENT_INTERPOLATION_OFFSET,				-0.5,	1, "" },
		{ "max_fragment_interpolation_offset",				 GL_MAX_FRAGMENT_INTERPOLATION_OFFSET,				 0.5,	0, "" },
	};

	static const Limit<tcu::IVec3> ivec3Limits[] =
	{
		{ "max_compute_work_group_count",					 GL_MAX_COMPUTE_WORK_GROUP_COUNT,	 tcu::IVec3(65535,65535,65535),	 0, "gl_MaxComputeWorkGroupCount" },
		{ "max_compute_work_group_size",					 GL_MAX_COMPUTE_WORK_GROUP_SIZE,	 tcu::IVec3(1024, 1024, 64),	 0, "gl_MaxComputeWorkGroupSize" },
};

	for (int idx = 0; idx < DE_LENGTH_OF_ARRAY(intLimits); idx++)
	{
		const Limit<GLint>& limit = intLimits[idx];
		addChild(new glcts::LimitCase<GLint>(m_context, limit.name, limit.token, limit.boundry, limit.isMaximum, "450", limit.builtin ));
	}

	for (int idx = 0; idx < DE_LENGTH_OF_ARRAY(int64Limits); idx++)
	{
		const Limit<GLint64>& limit = int64Limits[idx];
		addChild(new glcts::LimitCase<GLint64>(m_context, limit.name, limit.token, limit.boundry, limit.isMaximum, "450", limit.builtin ));
	}

	for (int idx = 0; idx < DE_LENGTH_OF_ARRAY(uint64Limits); idx++)
	{
		const Limit<GLuint64>& limit = uint64Limits[idx];
		addChild(new glcts::LimitCase<GLuint64>(m_context, limit.name, limit.token, limit.boundry, limit.isMaximum, "450", limit.builtin ));
	}

	for (int idx = 0; idx < DE_LENGTH_OF_ARRAY(floatLimits); idx++)
	{
		const Limit<GLfloat>& limit = floatLimits[idx];
		addChild(new glcts::LimitCase<GLfloat>(m_context, limit.name, limit.token, limit.boundry, limit.isMaximum, "450", limit.builtin ));
	}

	for (int idx = 0; idx < DE_LENGTH_OF_ARRAY(ivec3Limits); idx++)
	{
		const Limit<tcu::IVec3>& limit = ivec3Limits[idx];
		addChild(new glcts::LimitCase<tcu::IVec3>(m_context, limit.name, limit.token, limit.boundry, limit.isMaximum, "450", limit.builtin ));
	}
}

} /* glcts namespace */
