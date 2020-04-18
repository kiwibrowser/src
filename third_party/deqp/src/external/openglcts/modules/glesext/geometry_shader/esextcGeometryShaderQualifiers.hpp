#ifndef _ESEXTCGEOMETRYSHADERQUALIFIERS_HPP
#define _ESEXTCGEOMETRYSHADERQUALIFIERS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
/* Implementation of Test 22.1 from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. Make sure flat interpolation does not affect data passed from vertex
 *     shader to geometry shader.
 *
 *     Category: API;
 *               Functional Test.
 *
 *     Create a program object and a fragment, geometry and a vertex shader
 *     object:
 *
 *     - Vertex shader object should output a flat interpolated int variable
 *       named out_vertex. The shader should set it to gl_VertexID; gl_Position
 *       should be set to (0, 0, 0, 0).
 *     - Geometry shader object should accept triangles as input and emit
 *       a maximum of 1 point. The point it emits:
 *       - should have (0, 0, 0, 0) coordinates if values of out_vertex variable
 *         being part of subsequent vertices making up the input triangle are not
 *         equal to (0, 1, 2).
 *       - should be set to (1, 1, 1, 1) otherwise.
 *     - Fragment shader object implementation can be boilerplate.
 *
 *     Using this program object, a single triangle should be drawn. Transform
 *     feed-back should be used to capture the result coordinates as set by
 *     geometry shader. The value recorded should be equal to (1, 1, 1, 1).
 *
 **/
class GeometryShaderFlatInterpolationTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderFlatInterpolationTest(Context& context, const ExtParameters& extParams, const char* name,
										const char* description);

	virtual ~GeometryShaderFlatInterpolationTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private methods */
	void initProgram();

	/* Private variables */
	glw::GLuint m_bo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERQUALIFIERS_HPP
