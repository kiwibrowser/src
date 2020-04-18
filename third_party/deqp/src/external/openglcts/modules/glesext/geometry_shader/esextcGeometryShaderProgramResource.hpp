#ifndef _ESEXTCGEOMETRYSHADERPROGRAMRESOURCE_HPP
#define _ESEXTCGEOMETRYSHADERPROGRAMRESOURCE_HPP
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

/** Implementation of Group 3. Test description follows:
 *
 * 1. Make sure that GL_REFERENCED_BY_GEOMETRY_SHADER_EXT property works
 *    correctly for all supported interfaces.
 *
 *    Category: API.
 *
 *    1. Create a program object with boilerplate fragment and vertex shaders,
 *       as well as an attached geometry shader using all of the features,
 *       properties of which can later be examined using
 *       glGetProgramResourceiv() call.
 *    2. Link the program object.
 *    3. Use GetProgramResourceiv() calls to verify values reported for
 *       geometry stage are as expected.
 **/
class GeometryShaderProgramResourceTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderProgramResourceTest(Context& context, const ExtParameters& extParams, const char* name,
									  const char* description);

	virtual ~GeometryShaderProgramResourceTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void initTest(void);

	bool checkIfResourceAtIndexIsReferenced(glw::GLuint program_object_id, glw::GLenum interface,
											glw::GLuint index) const;

	bool checkIfResourceIsReferenced(glw::GLuint program_object_id, glw::GLenum interface, const char* name) const;

	/* Private fields */

	/* Shader objects */
	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_geometry_shader_id;
	glw::GLuint m_vertex_shader_id;

	/* Program object */
	glw::GLuint m_program_object_id;

	static const char* const m_common_shader_code_definitions_body;
	static const char* const m_common_shader_code_definitions_atomic_counter_body;
	static const char* const m_common_shader_code_definitions_ssbo_body;

	static const char* const m_vertex_shader_code_preamble;
	static const char* const m_vertex_shader_code_body;
	static const char* const m_vertex_shader_code_atomic_counter_body;
	static const char* const m_vertex_shader_code_ssbo_body;

	static const char* const m_geometry_shader_code_preamble;
	static const char* const m_geometry_shader_code_body;
	static const char* const m_geometry_shader_code_atomic_counter_body;
	static const char* const m_geometry_shader_code_ssbo_body;

	static const char* const m_fragment_shader_code;

	bool m_atomic_counters_supported;
	bool m_ssbos_supported;
};

} /* glcts */

#endif // _ESEXTCGEOMETRYSHADERPROGRAMRESOURCE_HPP
