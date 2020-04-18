#ifndef _ESEXTCTESSELLATIONSHADERPRIMITIVECOVERAGE_HPP
#define _ESEXTCTESSELLATIONSHADERPRIMITIVECOVERAGE_HPP
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

/*!
 * \file esextcTessellationShaderPrimitiveCoverage.hpp
 * \brief TessellationShadePrimitiveCoverage (Test 31)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "esextcTessellationShaderUtils.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

namespace glcts
{
/** Implementation of Test 31 from CTS_EXT_tessellation_shader. Description follows:
 *
 *  Assume this test is run separately for a tessellated full-screen triangle
 *  and a tessellated full-screen quad.
 *  Using a stencil buffer, make sure that the whole domain is always
 *  covered by exactly one triangle generated during the process of tessellation.
 *  Make sure that the whole domain is covered by the set of all triangles
 *  generated during the process of tessellation.
 *
 *    Category: Functional Test.
 *    Priority: Must-Have
 */
class TessellationShaderPrimitiveCoverage : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderPrimitiveCoverage(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderPrimitiveCoverage()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void initBufferObjects(void);
	void initFramebuffer(void);
	void initProgramObjects(void);
	void initTest(void);

	void drawPatch(glw::GLuint po_id, glw::GLuint n_patch_vertices, glw::GLuint n_draw_call_vertices,
				   const glw::GLfloat inner_levels[], const glw::GLfloat outer_levels[]);

	bool verifyDrawBufferContents(void);

	/* Private variables */
	glw::GLuint m_vao_id;						/* name of vertex array object */
	glw::GLuint m_quad_tessellation_po_id;		/* name of program object that performs quad tessellation */
	glw::GLuint m_stencil_verification_po_id;   /* name of program object for stencil setup */
	glw::GLuint m_triangles_tessellation_po_id; /* name of program object that performs triangle tessellation */

	glw::GLuint m_bo_id; /* name of a buffer object used to hold input vertex data */

	glw::GLuint m_fs_id; /* name of fragment shader object */
	glw::GLuint
		m_quad_tessellation_tcs_id; /* name of tessellation control    shader object that performs quad tessellation */
	glw::GLuint
		m_quad_tessellation_tes_id; /* name of tessellation evaluation shader object that performs quad tessellation */
	glw::GLuint
		m_triangles_tessellation_tcs_id; /* name of tessellation control    shader object that performs triangle tessellation */
	glw::GLuint
				m_triangles_tessellation_tes_id; /* name of tessellation evaluation shader object that performs triangle tessellation */
	glw::GLuint m_vs_id;						 /* nameof vertex shader object */

	glw::GLuint m_fbo_id;		  /* name of a framebuffer object */
	glw::GLuint m_color_rbo_id;   /* name of a renderbuffer object to be used as color attachment */
	glw::GLuint m_stencil_rbo_id; /* name of a renderbuffer object to be used as stencil attachment */

	static const char* m_fs_code;						  /* fragment shader source code */
	static const char* m_quad_tessellation_tcs_code;	  /* tessellation control    shader source code */
	static const char* m_quad_tessellation_tes_code;	  /* tessellation evaluation shader source code */
	static const char* m_triangles_tessellation_tcs_code; /* tessellation control    shader source code */
	static const char* m_triangles_tessellation_tes_code; /* tessellation evaluation shader source code */
	static const char* m_vs_code;						  /* vertex shader source code */

	static const glw::GLuint m_height;		 /* height of the rendering area */
	static const glw::GLuint m_n_components; /* number of components used by a single texel */
	static const glw::GLuint m_width;		 /* width of the rendering area */

	static const glw::GLfloat m_clear_color[4];			   /* clear color */
	static const glw::GLfloat m_stencil_pass_color[4];	 /* primitive color when stencil fails */
	glw::GLubyte*			  m_rendered_data_buffer;	  /* pixel buffer for fetched data */
	static const glw::GLuint  m_rendered_data_buffer_size; /* pixel buffer size */
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERPRIMITIVECOVERAGE_HPP
