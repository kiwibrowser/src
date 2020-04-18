#ifndef _ESEXTCGEOMETRYSHADERCONSTANTVARIABLES_HPP
#define _ESEXTCGEOMETRYSHADERCONSTANTVARIABLES_HPP
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

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
/** Implementation of Test Group 15 from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. gl_MaxGeometry* constants should reflect values reported for
 *     corresponding GL_MAX_GEOMETRY_* properties.
 *
 *     Category: API;
 *               Functional Test.
 *
 *     Using transform feedback, query values of the following constants in
 *     a geometry shader and transfer them to a buffer object:
 *
 *     * gl_MaxGeometryInputComponents;
 *     * gl_MaxGeometryOutputComponents;
 *     * gl_MaxGeometryImageUniforms;
 *     * gl_MaxGeometryTextureImageUnits;
 *     * gl_MaxGeometryOutputVertices;
 *     * gl_MaxGeometryTotalOutputComponents;
 *     * gl_MaxGeometryUniformComponents;
 *     * gl_MaxGeometryAtomicCounters;
 *     * gl_MaxGeometryAtomicCounterBuffers;
 *
 *     The test should then map the buffer object into client-space for reading
 *     and compare them against values reported by glGetIntegerv() for
 *     corresponding pnames. Test should fail if any of the values do not match.
 *
 *
 *  2. Implementation-dependent constants are the same for all compatible
 *     getters. The value reported meets minimum maximum requirements enforced
 *     by the specification.
 *
 *     Category: API;
 *
 *     The following properties should be checked:
 *
 *     1)  GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT              (min: 0);
 *     2)  GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT       (min: 0);
 *     3)  GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT         (min: 16);
 *     4)  GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT      (min: 0);
 *     5)  GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT             (min: 0);
 *     6)  GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT          (min: 1024);
 *     7)  GL_MAX_GEOMETRY_UNIFORM_BLOCKS_EXT              (min: 12);
 *     8)  GL_MAX_GEOMETRY_INPUT_COMPONENTS_EXT            (min: 64);
 *     9)  GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_EXT           (min: 128);
 *     10) GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT             (min: 256);
 *     11) GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT     (min: 1024);
 *     12) GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT          (min: 32);
 *     13) GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS_EXT (min:
 *             GL_MAX_GEOMETRY_UNIFORM_BLOCKS_EXT * GL_MAX_UNIFORM_BLOCK_SIZE / 4 +
 *             + GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT);
 *     14) GL_LAYER_PROVOKING_VERTEX_EXT                   (accepted values:
 *             GL_PROVOKING_VERTEX (*),
 *             GL_FIRST_VERTEX_CONVENTION_EXT,
 *             GL_LAST_VERTEX_CONVENTION_EXT,
 *             GL_UNDEFINED_VERTEX_EXT)
 *     15) GL_MAX_FRAMEBUFFER_LAYERS_EXT                   (min: 2048)
 *
 *     (*) Only applicable to OpenGL. The ES extension does not allow this value.
 **/

class GeometryShaderConstantVariables : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderConstantVariables(Context& context, const ExtParameters& extParams, const char* name,
									const char* description);

	virtual ~GeometryShaderConstantVariables()
	{
	}

	void		  deinit(void);
	IterateResult iterate(void);

private:
	/* Private methods */
	void initTest(void);

	/* Private variables */
	static const char* m_fragment_shader_code;
	static const char* m_geometry_shader_code;
	static const char* m_vertex_shader_code;
	static const char* m_feedbackVaryings[];

	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_geometry_shader_id;
	glw::GLuint m_vertex_shader_id;
	glw::GLuint m_program_id;

	glw::GLuint m_bo_id;
	glw::GLuint m_vao_id;

	/* EXT_geometry_shader specific constant values */
	const int m_min_MaxGeometryImagesUniforms;
	const int m_min_MaxGeometryTextureImagesUnits;
	const int m_min_MaxGeometryShaderStorageBlocks;
	const int m_min_MaxGeometryAtomicCounterBuffers;
	const int m_min_MaxGeometryAtomicCounters;

	const int m_min_MaxFramebufferLayers;
	const int m_min_MaxGeometryInputComponents;
	const int m_min_MaxGeometryOutputComponents;
	const int m_min_MaxGeometryOutputVertices;
	const int m_min_MaxGeometryShaderInvocations;
	const int m_min_MaxGeometryTotalOutputComponents;
	const int m_min_MaxGeometryUniformBlocks;
	const int m_min_MaxGeometryUniformComponents;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERCONSTANTVARIABLES_HPP
