#ifndef _ESEXTCTESSELLATIONSHADERPROPERTIES_HPP
#define _ESEXTCTESSELLATIONSHADERPROPERTIES_HPP
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
#include "glwEnums.hpp"

namespace glcts
{

/** Implementation for Test Case 5
 *
 *  Make sure that the following tessellation stage properties (some of which
 *  are context-wide, some associated with active program object) have correct
 *  default values reported by relevant getters:
 *
 *  * GL_PATCH_VERTICES_EXT             (default value: 3);
 *  * GL_PATCH_DEFAULT_OUTER_LEVEL (*)  (default value: 4 x 1.0);
 *  * GL_PATCH_DEFAULT_INNER_LEVEL (*)  (default value: 2 x 1.0);
 *
 *  (*) Only checked on Desktop
 *
 **/
class TessellationShaderPropertiesDefaultContextWideValues : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderPropertiesDefaultContextWideValues(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderPropertiesDefaultContextWideValues(void)
	{
	}

	virtual IterateResult iterate(void);
};

/*  Make sure that the following tessellation stage properties (some of which
 *  are context-wide, some associated with active program object) have correct
 *  default values reported by relevant getters:
 *
 *  * GL_TESS_CONTROL_OUTPUT_VERTICES_EXT (default value: 0);
 *  * GL_TESS_GEN_MODE_EXT                (default value: GL_QUADS_EXT);
 *  * GL_TESS_GEN_SPACING_EXT             (default value: GL_EQUAL);
 *  * GL_TESS_GEN_VERTEX_ORDER_EXT        (default value: GL_CCW);
 *  * GL_TESS_GEN_POINT_MODE_EXT          (default value: GL_FALSE);
 *
 *  The test should also iterate through a number of program objects, for which
 *  different tessellation control and evaluation shaders were defined, and verify
 *  the GL_TESS_* values returned are as defined in input layout qualifiers for
 *  relevant shaders.
 *
 */
class TessellationShaderPropertiesProgramObject : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderPropertiesProgramObject(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderPropertiesProgramObject(void)
	{
	}

	virtual void		  deinit(void);
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definitions */
	/* Define a few different tc/te/tc+te shaders we'll attach to a program object,
	 * which will then be queried for tessellation-specific properties
	 */
	typedef struct _test_descriptor
	{
		glw::GLint  expected_control_output_vertices_value;
		glw::GLenum expected_gen_mode_value;
		glw::GLenum expected_gen_point_mode_value;
		glw::GLenum expected_gen_spacing_value;
		glw::GLenum expected_gen_vertex_order_value;
		const char* tc_body;
		const char* te_body;

		_test_descriptor()
		{
			expected_control_output_vertices_value = 0;
			expected_gen_mode_value				   = 0;
			expected_gen_point_mode_value		   = 0;
			expected_gen_spacing_value			   = 0;
			expected_gen_vertex_order_value		   = 0;
			tc_body								   = DE_NULL;
			te_body								   = DE_NULL;
		}
	} _test_descriptor;

	typedef std::vector<_test_descriptor> _tests;
	typedef _tests::const_iterator		  _tests_const_iterator;

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vs_id;
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERPROPERTIES_HPP
