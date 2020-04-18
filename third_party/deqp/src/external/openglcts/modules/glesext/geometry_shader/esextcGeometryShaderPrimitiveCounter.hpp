#ifndef _ESEXTCGEOMETRYSHADERPRIMITIVECOUNTER_HPP
#define _ESEXTCGEOMETRYSHADERPRIMITIVECOUNTER_HPP
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
#include <string.h>

namespace glcts
{

struct DataPrimitiveIDInCounter
{
	glw::GLenum m_drawMode;
	std::string m_emitVertexCodeForGeometryShader;
	glw::GLenum m_feedbackMode;
	std::string m_layoutIn;
	std::string m_layoutOut;
	glw::GLuint m_numberOfVerticesPerOneOutputPrimitive;
	glw::GLuint m_numberOfVerticesPerOneInputPrimitive;
	glw::GLuint m_numberOfDrawnPrimitives;
};

/** Implementation of "Group 6" from CTS_EXT_geometry_shader.
 *   Test Group for "Group 6" tests, storing configuration data and initializing the tests.
 *   All tests from "Group 6" and children of GeometryShaderPrimitiveCounterTestGroup.
 */
class GeometryShaderPrimitiveCounterTestGroup : public TestCaseGroupBase
{
public:
	/* Public methods */
	GeometryShaderPrimitiveCounterTestGroup(Context& context, const ExtParameters& extParams, const char* name,
											const char* description);

	virtual ~GeometryShaderPrimitiveCounterTestGroup()
	{
	}

	virtual void init(void);
};

/**
 * 1. Make sure that a built-in geometry shader's input variable
 *    gl_PrimitiveIDIn increments correctly for all valid <draw call mode,
 *    input primitive type, output primitive type> tuples.
 "
 *    Category: API;
 *              Functional Test.
 *
 *    Let N_VERTICES represent amount of vertices that must be emitted, given
 *    active geometry shader's output primitive type, to form a single
 *    primitive that can be passed to rasterization stage
 *
 *    Use Transform Feedback functionality to capture gl_PrimitiveIDIn value
 *    as read in the geometry shader:
 *
 *    - The shader should emit:
 *
 *      floor(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT / N_VERTICES) / (4 + 1)
 *
 *      primitives. It should call EndPrimitive() every N_VERTICES vertices
 *      emitted.
 *
 *    - The draw call should be made for 1024 primitives.
 *
 *    Test should fail if the result data for any of the tuples is determined
 *    to be incorrect.
 */
class GeometryShaderPrimitiveCounter : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderPrimitiveCounter(Context& context, const ExtParameters& extParams, const char* name,
								   const char* description, const DataPrimitiveIDInCounter& testConfiguration);

	virtual ~GeometryShaderPrimitiveCounter(void)
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	/* Protected methods */
	bool checkResult(const glw::GLint* feedbackResult);
	void createAndBuildProgramWithFeedback(void);
	void drawAndGetFeedback(glw::GLint* feedbackResult);
	void prepareBufferObjects();

	/* Protected virtual methods */
	virtual void drawFunction();

	/* Protected members */
	DataPrimitiveIDInCounter m_testConfiguration;

	glw::GLuint m_nrVaryings;
	glw::GLuint m_sizeOfDataArray;
	glw::GLuint m_sizeOfFeedbackBuffer;

	const glw::GLuint m_n_components;

private:
	/* Private functions */
	std::string GetGeometryShaderCode(const std::string& max_vertices, const std::string& layout_in,
									  const std::string& layout_out, const std::string& emit_vertices,
									  const std::string& n_iterations);

	/* Private variables */
	static const char* m_fragment_shader_code;
	static const char* m_vertex_shader_code;

	glw::GLint m_maxGeometryOutputVertices;

	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_geometry_shader_id;
	glw::GLuint m_vertex_shader_id;
	glw::GLuint m_program_id;

	glw::GLuint m_tbo;
	glw::GLuint m_vao;
	glw::GLuint m_vbo;
};

/** 2. Make sure that restarting a primitive topology with a primitive restart
 *     index does not affect primitive ID counter, when a element-driven draw
 *     call is made.
 *
 *     Category: API;
 *               Functional Test.
 *
 *     Modify test case 6.1 so that it uses element-sourced data. Insert the
 *     restart primitive index right after first primitive is rendered but
 *     before indices for the second one start.
 */
class GeometryShaderPrimitiveCounterRestartingPrimitive : public GeometryShaderPrimitiveCounter
{
public:
	/* Public methods */
	GeometryShaderPrimitiveCounterRestartingPrimitive(Context& context, const ExtParameters& extParams,
													  const char* name, const char* description,
													  const DataPrimitiveIDInCounter& testConfiguration);

	virtual ~GeometryShaderPrimitiveCounterRestartingPrimitive(void)
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	/* Protected functions */
	virtual void drawFunction();
	void		 setUpVertexAttributeArrays();

private:
	/* Private fields */
	glw::GLuint m_bo_id;
	glw::GLuint m_numberOfRestarts;
};

/** 3. Make sure that gl_PrimitiveID built-in variable can be read correctly
 *      from a fragment shader, assuming geometry shader writes to it in
 *      previous stage.
 *
 *      Category: API;
 *                Functional Test.
 *
 *      Vertex shader should output an int variable called vs_vertex_id, to which
 *      gl_VertexID has been saved.
 *
 *      Geometry shader should take points and output a triangle strip. It will
 *      emit up to 4 vertices. The geometry shader should define an input int
 *      variable called vs_vertex_id. Let:
 *
 *      int column = vs_vertex_id % 64;
 *      int row = floor(vs_vertex_id / 64);
 *
 *      The shader should emit the following vertices:
 *
 *      1) (-1.0 + (column+1) / 32.0, -1.0 + (row + 1) / 32.0, 0, 1)
 *      2) (-1.0 + (column+1) / 32.0, -1.0 + (row)     / 32.0, 0, 1)
 *      3) (-1.0 + (column)   / 32.0, -1.0 + (row + 1) / 32.0, 0, 1)
 *      4) (-1.0 + (column)   / 32.0, -1.0 + (row)     / 32.0, 0, 1)
 *
 *      Finally, the geometry shader should set gl_PrimitiveID for each of the
 *      vertices to vs_vertex_id.
 *
 *      Fragment shader should set output variable's result value as follows: (*)
 *
 *      result[0] =      (gl_PrimitiveID % 64) / 64.0;
 *      result[1] = floor(gl_PrimitiveID / 64) / 64.0;
 *      result[2] =       gl_PrimitiveID       / 4096.0;
 *      result[3] = ((gl_PrimitiveID % 2) == 0) ? 1.0 : 0.0;
 *
 *      Using a program object built of these shader, the test should issue
 *      a draw call for 4096 points. The test is considered to have passed
 *      successfully if the rendered data consists of 4096 rectangles of edge
 *      equal to 1/64th of the output resolution, centers of which have values
 *      as described in (*).
 *
 *      Texture object used as color attachment should be at least of 1024x1024
 *      resolution, in which case quad edge will be 4px long.
 */
class GeometryShaderPrimitiveIDFromFragmentShader : public TestCaseBase
{
public:
	/* Publi methods */
	GeometryShaderPrimitiveIDFromFragmentShader(Context& context, const ExtParameters& extParams, const char* name,
												const char* description);

	virtual ~GeometryShaderPrimitiveIDFromFragmentShader(void)
	{
	}

	void		  deinit(void);
	IterateResult iterate(void);

private:
	/* Private fields */
	static const char* m_fragment_shader_code;
	static const char* m_geometry_shader_code;
	static const char* m_vertex_shader_code;

	const glw::GLuint m_n_drawn_vertices;
	const glw::GLuint m_squareEdgeSize;
	const glw::GLuint m_texture_height;
	const glw::GLuint m_texture_n_components;
	const glw::GLuint m_texture_n_levels;
	const glw::GLuint m_texture_width;

	glw::GLuint m_fbo_id;
	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_geometry_shader_id;
	glw::GLuint m_program_id;
	glw::GLuint m_texture_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vbo_id;
	glw::GLuint m_vertex_shader_id;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERPRIMITIVECOUNTER_HPP
