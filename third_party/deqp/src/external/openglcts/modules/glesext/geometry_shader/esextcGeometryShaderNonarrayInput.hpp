#ifndef _ESEXTCGEOMETRYSHADERNONARRAYINPUT_HPP
#define _ESEXTCGEOMETRYSHADERNONARRAYINPUT_HPP
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
/** Implementation of "Group 4" from CTS_EXT_geometry_shader. Description follows:
 *
 * 1. Make sure GLES implementation does not accept non-array input variables
 *    in geometry shaders. Geometry shaders using unsized array-based input
 *    variables and blocks should not be detrimental to the linking process.
 *
 *    Category : API;
 *               Negative & positive tests.
 *
 *    Create two program objects, a boilerplate fragment shader object and two
 *    vertex shader objects.
 *
 *    Create a geometry shader object A. The implementation should not use any
 *    interface blocks - all input variables (discussed below) should be
 *    located in the default interface block. Variable named v4 should lack
 *    array brackets (as required by the spec). A corresponding vertex shader
 *    object A should output all variables(see later discussion) expected by
 *    the geometry shader, so that the interfaces match.
 *
 *    Create a geometry shader object B. The implementation should use a single
 *    input interface block *not* being declared as an unsized array (opposite
 *    to spec requirements). A corresponding vertex shader object B should
 *    output the interface block, so that that interfaces match. Both stages
 *    should use variables as discussed in next paragraph.
 *
 *    Vertex shader stage should output the following variables (in a separate
 *    output interface block or in the default one, depending on geometry
 *    shader object considered):
 *
 *    * vec4 v1 (set to (-1, -1, 0, 1) )
 *    * vec4 v2 (set to (-1,  1, 0, 1) )
 *    * vec4 v3 (set to ( 1, -1, 0, 1) )
 *    * vec4 v4 (set to ( 1,  1, 0, 1) )
 *
 *    Both geometry shaders should take points on input and output a triangle
 *    strip, consisting of a maximum of 4 vertices. For each input point, they
 *    should emit 4 vertices, of which coordinates are subsequently described
 *    by vectors v1, v2, v3 and v4.
 *
 *    Fragment shader stage should set output vec4 variable to (0, 255, 0, 255).
 *
 *    Program object A should use fragment shader object as described above,
 *    geometry shader object A and vertex shader object A.
 *    Program object B should use fragment shader object as described above,
 *    geometry shader object B and vertex shader object B.
 *
 *    Both program objects A and B are expected not to link correctly (their
 *    GL_LINK_STATUS after linking should be reported as GL_FALSE).
 *
 *    Delete both program objects and geometry shader objects. Geometry shader
 *    objects should now use a slightly modified implementation where the input
 *    variables and the interface block is correctly defined as unsized array.
 *
 *    Attach all shaders to corresponding program objects, compile them, link
 *    program objects. Both program objects should now link correctly.
 *
 *    The test should now set up a FBO. A 2D texture of 4x4 resolution using
 *    GL_RGBA8 internal format should be attached to its color attachment 0.
 *
 *    The test should generate and bind a vertex array object, as well as the
 *    framebuffer object.
 *
 *    For each program object, the test should clear the color buffer with
 *    (255, 0, 0, 0) color and then draw a single point. The test passes if all
 *    of the texels of the texture attached to color attachment 0 are equal to
 *    (0, 255, 0, 255).
 */

class GeometryShaderNonarrayInputCase : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderNonarrayInputCase(Context& context, const ExtParameters& extParams, const char* name,
									const char* description);

	virtual ~GeometryShaderNonarrayInputCase(void)
	{
	}

	void		  deinit(void);
	IterateResult iterate(void);

protected:
	/* Protected variables */
	glw::GLuint m_fbo_id;
	glw::GLuint m_fs;
	glw::GLuint m_gs_invalid_non_ib;
	glw::GLuint m_gs_invalid_ib;
	glw::GLuint m_gs_valid_non_ib;
	glw::GLuint m_gs_valid_ib;
	glw::GLuint m_po_a_invalid;
	glw::GLuint m_po_b_invalid;
	glw::GLuint m_po_a_valid;
	glw::GLuint m_po_b_valid;
	glw::GLuint m_to_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_valid_ib;
	glw::GLuint m_vs_valid_non_ib;

	static const char* m_fs_code;
	static const char* m_gs_code_preamble;
	static const char* m_gs_code_body;
	static const char* m_vs_code_preamble;
	static const char* m_vs_code_body;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERNONARRAYINPUT_HPP
