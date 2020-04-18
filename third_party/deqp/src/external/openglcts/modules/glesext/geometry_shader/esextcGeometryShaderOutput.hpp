#ifndef _ESEXTCGEOMETRYSHADEROUTPUT_HPP
#define _ESEXTCGEOMETRYSHADEROUTPUT_HPP
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
/** Base class for Geometry Shader Output test implementations. */
class GeometryShaderOutput : public TestCaseBase
{
protected:
	/* Protected methods */
	GeometryShaderOutput(Context& context, const ExtParameters& extParams, const char* name, const char* description);
	virtual ~GeometryShaderOutput(){};

	/* Protected variables */
	static const char* const m_fragment_shader_code_white_color;
	static const char* const m_vertex_shader_code_two_vec4;
	static const char* const m_vertex_shader_code_vec4_0_0_0_1;
};

/** Implementation of test case 12.1. Test description follows:
     *
     *  It is an error to use two conflicting output primitive type declarations
     *  in one geometry shader.
     *
     *  Category: API;
     *            Negative Test.
     *
     *  Create a program object, for which a boilerplate fragment and vertex
     *  shaders will be used. A geometry shader should also be attached to the
     *  program object. The shader should include the following incorrect output
     *  primitive type declarations:
     *
     *  layout(triangle_strip, max_vertices = 60) out;
     *  layout(points) out;
     *
     *  Linking of the program object is expected to fail under this
     *  configuration.
     */
class GeometryShaderDuplicateOutputLayoutQualifierTest : public GeometryShaderOutput
{
public:
	/* Public methods */
	GeometryShaderDuplicateOutputLayoutQualifierTest(Context& context, const ExtParameters& extParams, const char* name,
													 const char* description);
	virtual ~GeometryShaderDuplicateOutputLayoutQualifierTest(){};

	virtual IterateResult iterate(void);

protected:
	/* Protected variables */
	static const char* const m_geometry_shader_code;
};

/** Implementation of test case 12.2. Test description follows:
     *
     *  It is an error to use two conflicting max_vertices declarations in one
     *  geometry shader.
     *
     *  Category: API;
     *            Negative Test.
     *
     *  Create a program object, for which a boilerplate fragment and vertex
     *  shaders will be used. A geometry shader should also be attached to the
     *  program object. The shader should include the following incorrect output
     *  primitive type declarations:
     *
     *  layout(triangle_strip, max_vertices = 60) out;
     *  layout(max_vertices = 20) out;
     *
     *  Linking of the program object is expected to fail under this
     *  configuration.
     **/
class GeometryShaderDuplicateMaxVerticesLayoutQualifierTest : public GeometryShaderOutput
{
public:
	/* Public methods */
	GeometryShaderDuplicateMaxVerticesLayoutQualifierTest(Context& context, const ExtParameters& extParams,
														  const char* name, const char* description);
	virtual ~GeometryShaderDuplicateMaxVerticesLayoutQualifierTest(){};

	virtual IterateResult iterate(void);

protected:
	/* Protected variables */
	static const char* const m_geometry_shader_code;
};

/** Base class for tests 12.3, 12.4 and 12.5.
     *
     *  Test class have to:
         - provide geometry shader code via constructor parameter,
         - override method verifyResult.
     **/
class GeometryShaderOutputRenderingBase : public GeometryShaderOutput
{
public:
	virtual void		  deinit(void);
	virtual IterateResult iterate(void);
	virtual bool verifyResult(const unsigned char* result_image, unsigned int width, unsigned int height,
							  unsigned int pixel_size) const = 0;

protected:
	/* Protected methods */
	GeometryShaderOutputRenderingBase(Context& context, const ExtParameters& extParams, const char* name,
									  const char* description, const char* geometry_shader_code);
	virtual ~GeometryShaderOutputRenderingBase(){};

	void initTest(void);

private:
	/* Private variables */
	const char* m_geometry_shader_code;

	glw::GLuint m_program_object_id;
	glw::GLuint m_vertex_shader_id;
	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_geometry_shader_id;

	glw::GLuint m_vao_id;
	glw::GLuint m_fbo_id;
	glw::GLuint m_color_tex_id;
};

/** Implementation of test case 12.3. Test description follows:
     *
     *  Make sure that EndPrimitive() does not emit a vertex.
     *
     *  Category: API;
     *            Functional Test.
     *
     *  Consider a FBO with a single 16x16 2D texture object attached to color
     *  attachment 0 and made active.
     *
     *  The texture object should be filled with (0, 0, 0, 0).
     *
     *  A boilerplate vertex shader should be assumed for the test case.
     *
     *  Consider a geometry shader that takes a point as input and outputs
     *  a triangle strip (a max of 3 vertices will be emitted). The shader
     *  should:
     *
     *  - Emit a vertex at   (-1, -1, 0, 1).
     *  - Emit a vertex at   (-1,  1, 0, 1).
     *  - Set gl_Position to ( 1,  1, 0, 1).
     *  - Do a EndPrimitive() call.
     *
     *  A fragment shader should set the only output vec4 variable to
     *  (1, 1, 1, 1).
     *
     *  A program object consisting of all three shaders should be created and
     *  linked.
     *
     *  A single point draw call should be issued.
     *
     *  At this point the test should verify that the texture object has not been
     *  modified, that is: all texels are set to (0, 0, 0, 0).
     **/
class GeometryShaderIfVertexEmitIsDoneAtEndTest : public GeometryShaderOutputRenderingBase
{
public:
	/* Public methods */
	GeometryShaderIfVertexEmitIsDoneAtEndTest(Context& context, const ExtParameters& extParams, const char* name,
											  const char* description);
	virtual ~GeometryShaderIfVertexEmitIsDoneAtEndTest(){};

	virtual bool verifyResult(const unsigned char* result_image, unsigned int width, unsigned int height,
							  unsigned int pixel_size) const;

protected:
	/* Protected variables */
	static const char* const m_geometry_shader_code;
};

/** Implementation of test case 12.4. Test description follows:
     *
     *  Make sure that a geometry shader completes current output primitive upon
     *  termination.
     *
     *  Category: API;
     *            Functional Test.
     *
     *  Modify test case 12.3 by:
     *
     *  - Removing EndPrimitive() call the geometry shader should do at the end.
     *  - Changing the test check condition: test now succeeds if texels at
     *    bottom-left, top-left and top-right corners are set to
     *    (255, 255, 255, 255) and bottom-right corner is set to (0, 0, 0, 0),
     *    assuming glReadPixels() was called with GL_RGBA format and
     *    GL_UNSIGNED_BYTE type.
     **/
class GeometryShaderMissingEndPrimitiveCallTest : public GeometryShaderOutputRenderingBase
{
public:
	/* Public methods */
	GeometryShaderMissingEndPrimitiveCallTest(Context& context, const ExtParameters& extParams, const char* name,
											  const char* description);
	virtual ~GeometryShaderMissingEndPrimitiveCallTest(){};

	virtual bool verifyResult(const unsigned char* result_image, unsigned int width, unsigned int height,
							  unsigned int pixel_size) const;

protected:
	/* Protected variables */
	static const char* const m_geometry_shader_code;
};

/** Implementation of test case 12.5. Test description follows:
     *
     *  Make sure that it is not necessary to do an EndPrimitive() call if the
     *  geometry shader only writes a single primitive.
     *
     *  Category: API;
     *            Functional Test.
     *
     *  Modify test case 12.3 by:
     *
     *  - Changing the logic in geometry shader to only set gl_Position to
     *    (-1, -1, 0, 1).
     *  - Changing input and output primitive types in a geometry shader to
     *    points.
     *  - Changing max_vertices to 1.
     *  - Pixel size should also be set to 2 prior to issuing the draw call.
     *  - Changing the test check condition: test now succeeds if texel at
     *    bottom-left corner is set to (255, 255, 255, 255) and remaining corners
     *    are set to (0, 0, 0, 0), assuming glReadPixels() was called with
     *    GL_RGBA format and GL_UNSIGNED_BYTE type.
     **/
class GeometryShaderMissingEndPrimitiveCallForSinglePrimitiveTest : public GeometryShaderOutputRenderingBase
{
public:
	/* Public methods */
	GeometryShaderMissingEndPrimitiveCallForSinglePrimitiveTest(Context& context, const ExtParameters& extParams,
																const char* name, const char* description);
	virtual ~GeometryShaderMissingEndPrimitiveCallForSinglePrimitiveTest(){};

	virtual IterateResult iterate(void);

	virtual bool verifyResult(const unsigned char* result_image, unsigned int width, unsigned int height,
							  unsigned int pixel_size) const;

protected:
	/* Protected variables */
	static const char* const m_geometry_shader_code;
};
} /* glcts */

#endif // _ESEXTCGEOMETRYSHADEROUTPUT_HPP
