#ifndef _ESEXTCTESSELLATIONSHADERXFB_HPP
#define _ESEXTCTESSELLATIONSHADERXFB_HPP
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
/** Implementation for Test Case 10
 *
 *  Verify transform feed-back captures data from appropriate shader stage.
 *  Consider the following scenarios:
 *
 *  * vertex shader, tessellation control + evaluation shader, geometry shaders
 *    are defined (output should be taken from geometry shader);
 *  * vertex shader, tessellation control + evaluation shaders are defined
 *    (output should be taken from tessellation evaluation shader);
 *
 *  Verify the following shader/stage combination is invalid and neither links
 *  nor is considered a valid combination of stages for a pipeline object:
 *
 *  * vertex shader, tessellation control shaders are defined;
 *
 *  Make sure to include separate shader objects in the test.
 **/
class TessellationShaderXFB : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderXFB(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderXFB(void)
	{
	}

	virtual void		  deinit(void);
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private definitions */
	typedef struct _test_descriptor
	{
		/* Allowed values:
		 *
		 * GL_GEOMETRY_SHADER_EXT / GL_TESS_CONTROL_SHADER_EXT / GL_TESS_EVALUATION_SHADER_EXT /
		 * GL_VERTEX_SHADER.
		 */
		glw::GLenum expected_data_source;
		int			expected_n_values;
		bool		should_draw_call_fail;
		bool		requires_pipeline;
		glw::GLenum tf_mode;

		bool use_gs;
		bool use_tc;
		bool use_te;

		_test_descriptor()
		{
			expected_data_source  = GL_NONE;
			expected_n_values	 = 0;
			should_draw_call_fail = false;
			requires_pipeline	 = false;
			tf_mode				  = GL_POINTS;
			use_gs				  = false;
			use_tc				  = false;
			use_te				  = false;
		}
	} _test_descriptor;

	/* Private variables */
	glw::GLuint m_bo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vs_id;

	glw::GLuint m_pipeline_id;
	glw::GLuint m_fs_program_id;
	glw::GLuint m_gs_program_id;
	glw::GLuint m_tc_program_id;
	glw::GLuint m_te_program_id;
	glw::GLuint m_vs_program_id;

	glw::GLuint m_vao_id;

	/* Private methods */
	glw::GLuint createSeparableProgram(glw::GLenum shader_type, unsigned int n_strings, const char* const* strings,
									   unsigned int n_varyings, const char* const* varyings, bool should_succeed);
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERXFB_HPP
