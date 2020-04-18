#ifndef _ESEXTCTESSELLATIONSHADERPROGRAMINTERFACES_HPP
#define _ESEXTCTESSELLATIONSHADERPROGRAMINTERFACES_HPP
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
/** Implementation of Test Case 3
 *
 *  Make sure that the following interfaces report correct values for
 *  GL_REFERENCED_BY_TESS_CONTROL_SHADER_EXT property:
 *
 *  * GL_UNIFORM;
 *  * GL_UNIFORM_BLOCK;
 *  * GL_ATOMIC_COUNTER_BUFFER;
 *  * GL_SHADER_STORAGE_BLOCK;
 *  * GL_BUFFER_VARIABLE;
 *  * GL_PROGRAM_INPUT;
 *  * GL_PROGRAM_OUTPUT;
 *
 *  Make sure that the following interfaces report correct values for
 *  GL_REFERENCED_BY_TESS_EVALUATION_SHADER_EXT property:
 *
 *  * GL_UNIFORM;
 *  * GL_UNIFORM_BLOCK;
 *  * GL_ATOMIC_COUNTER_BUFFER;
 *  * GL_SHADER_STORAGE_BLOCK;
 *  * GL_BUFFER_VARIABLE;
 *  * GL_PROGRAM_INPUT;
 *  * GL_PROGRAM_OUTPUT;
 *
 *  The property should also be recognized by glGetProgram*()
 *
 **/
class TessellationShaderProgramInterfaces : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderProgramInterfaces(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderProgramInterfaces(void)
	{
	}

	virtual void		  deinit();
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void verifyPropertyValue(glw::GLenum interface, glw::GLenum property, glw::GLuint index, glw::GLint expected_value);

	/* Private variables */
	glw::GLuint m_fs_shader_id;
	glw::GLuint m_po_id;
	glw::GLuint m_tc_shader_id;
	glw::GLuint m_te_shader_id;
	glw::GLuint m_vs_shader_id;
	bool		m_is_atomic_counters_supported;
	bool		m_is_shader_storage_blocks_supported;
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERPROGRAMINTERFACES_HPP
