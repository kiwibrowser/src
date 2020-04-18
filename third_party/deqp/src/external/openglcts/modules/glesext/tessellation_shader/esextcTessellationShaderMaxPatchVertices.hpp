#ifndef _ESEXTCTESSELLATIONSHADERMAXPATCHVERTICES_HPP
#define _ESEXTCTESSELLATIONSHADERMAXPATCHVERTICES_HPP
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
#include "esextcTessellationShaderUtils.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

/*!
 * \file esextcTessellationShaderMaxPatchVertices.hpp
 * \brief TessellationShadeMaxPatchVertices (Test 19)
 */ /*-------------------------------------------------------------------*/

namespace glcts
{

/*   Implementation for Test Case 19
 *
 *   Make sure it is possible to use up to gl_MaxPatchVertices vertices in
 *   a patch and that the tessellation control shader, if it is present, can
 *   correctly access all of the vertices passed in a draw call. Input
 *   variables should be defined as arrays without explicit array size assigned.
 *   Test two cases:
 *
 *   * one where the patch size is explicitly defined to be equal to
 *     gl_MaxPatchVertices (tessellation control shader is present);
 *   * another one where the patch size is implicitly assumed to be equal to
 *     gl_MaxPatchVertices (tessellation control shader is not present) (*);
 *
 *   Technical details:
 *
 *   0. For each patch vertex, vec4 and ivec4 input attributes should be defined in
 *      TC. Values retrieved from these attributes should be passed to TE for XFB
 *      and then compared against reference data sets.
 *
 *   (*) Only checked on Desktop
 *
 *   Category: Functional Test.
 *   Priority: Must-Have
 */
class TessellationShaderMaxPatchVertices : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderMaxPatchVertices(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderMaxPatchVertices()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* private methods */
	void initTest(void);
	void initProgramObjects(void);
	void initVertexBufferObjects(void);
	void initTransformFeedbackBufferObjects(void);
	void initReferenceValues(void);

	bool compareResults(const char* description, glw::GLfloat ref_fv[4]);
	bool compareResults(const char* description, glw::GLint ref_iv[4]);

	/* Private variables */
	glw::GLuint m_bo_id_f_1; /* buffer object name for transform feedback of vec4 for case 1 */
	glw::GLuint m_bo_id_f_2; /* buffer object name for transform feedback of vec4 for case 2 */
	glw::GLuint m_bo_id_i_1; /* buffer object name for transform feedback of ivec4 for case 1 */
	glw::GLuint m_bo_id_i_2; /* buffer object name for transform feedback of ivec4 for case 2 */
	glw::GLuint m_fs_id;	 /* fragment shader object name */
	glw::GLuint m_po_id_1;   /* program object name for case 1 */
	glw::GLuint m_po_id_2;   /* program object name for case 2 */
	glw::GLuint m_tc_id;	 /* tessellation control shader object name for case 1 */
	glw::GLuint m_te_id;	 /* tessellation evaluation shader object name */
	glw::GLuint m_tf_id_1;   /* transform feedback object name for case 1 */
	glw::GLuint m_tf_id_2;   /* transform feedback object name for case 2 */
	glw::GLuint m_vs_id;	 /* vertex shader object name */
	glw::GLuint m_vao_id;	/* vertex array object */

	glw::GLint m_gl_max_patch_vertices; /* GL_MAX_PATCH_VERTICES_EXT pname value */

	glw::GLuint   m_patch_vertices_bo_f_id; /* buffer object name for patch vertices submission */
	glw::GLuint   m_patch_vertices_bo_i_id; /* buffer object name for patch vertices submission */
	glw::GLfloat* m_patch_vertices_f;		/* input array of patches for submission */
	glw::GLint*   m_patch_vertices_i;		/* input array of patches for submission */

	static const char*		 m_fs_code;		  /* fragment shader code */
	static const char*		 m_vs_code;		  /* vertex shader code */
	static const char*		 m_tc_code;		  /* tessellation control shader code */
	static const char*		 m_te_code;		  /* tessellation evaluation shader code */
	static const char* const m_tf_varyings[]; /* transform feedback varyings */

	glw::GLfloat m_ref_fv_case_1[4]; /* reference values of vec4 outputs for case 1 */
	glw::GLint   m_ref_iv_case_1[4]; /* reference values of ivec4 outputs for case 1 */

	glw::GLfloat m_ref_fv_case_2[4]; /* reference values of vec4 outputs for case 2 */
	glw::GLint   m_ref_iv_case_2[4]; /* reference values of ivec4 outputs for case 2 */
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERMAXPATCHVERTICES_HPP
