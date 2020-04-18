#ifndef _ESEXTCGEOMETRYSHADERCLIPPING_HPP
#define _ESEXTCGEOMETRYSHADERCLIPPING_HPP
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
/** Implementation of "Group 8" from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. Make sure that, should geometry shader be present in the pipeline,
 *     vertices that are outside clipping region are not discarded but are
 *     passed down to geometry shader for further processing.
 *
 *     Category: API;
 *               Functional Test.
 *
 *     Create a program object and a fragment, geometry and a vertex shader
 *     object:
 *
 *     - Vertex shader should set gl_Position to (-10, -10, -10, 0).
 *     - Geometry shader object should accept points and output triangle strips
 *       (a maximum of 4 vertices). It should emit following vertices:
 *
 *     1) (-1, -1, 0, 1)
 *     2) (-1,  1, 0, 1)
 *     3) ( 1, -1, 0, 1)
 *     4) ( 1,  1, 0, 1)
 *
 *     - Fragment shader object should store (0, 1, 0, 0) to result variable.
 *
 *     These shaders should be attached to the program object and compiled. The
 *     program object should be accordingly configured and linked.
 *
 *     The test should generate a vertex array object and bind it.
 *
 *     The test should then clear the draw buffer with red color (1, 0, 0, 0).
 *     Afterward, it should activate the program object and run it for a single
 *     point. The contents of the draw buffer should then be read. The test
 *     passes if the draw buffer contains a green color, fails otherwise.
 *
 **/
class GeometryShaderClipping : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderClipping(Context& context, const ExtParameters& extParams, const char* name, const char* description);

	virtual ~GeometryShaderClipping()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private variables */
	static const char* m_fs_code;
	static const char* m_vs_code;
	static const char* m_gs_code;

	static const int m_texture_height		= 4;
	static const int m_texture_width		= 4;
	static const int m_texture_n_levels		= 1;
	static const int m_texture_n_components = 4;

	/* Variables for general usege*/
	glw::GLuint m_fbo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_to_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERCLIPPING_HPP
