#ifndef _ESEXTCGEOMETRYSHADERPRIMITIVEQUERIES_HPP
#define _ESEXTCGEOMETRYSHADERPRIMITIVEQUERIES_HPP
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
/** Implementation of "Group 13" from from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. Make sure that values reported for
 *     GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN primitive query are correct if
 *     a geometry stage is active in program object used for a draw call.
 *
 *     Category: API.
 *
 *     Iterate through geometry shaders using all valid output primitive types.
 *     They should emit a few primitives for each output primitive type (make
 *     sure the vertices emitted are all in screen-space and that depth test
 *     is disabled). Vertex and fragment shaders are irrelevant, but they should
 *     be valid.
 *
 *     The test should make a few draw calls using each of the program objects,
 *     with the primitive query active. After doing a round of draw calls, the
 *     counter should be checked and verified.
 *
 *
 *  2. Make sure that GL_PRIMITIVES_GENERATED_EXT and
 *     GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN primitive queries are assigned
 *     different values if a buffer object bound to a TF binding point has an
 *     insufficient size to hold all primitives generated in geometry shader
 *     stage.
 *
 *     Category: API;
 *               Functional Test.
 *
 *     Test should generate a buffer object and set up a storage that can hold
 *     up to 4 vec4s.
 *
 *     It should then create a program object and boilerplate fragment & vertex
 *     shader objects. It should also create a geometry shader that:
 *
 *     * Takes points on input;
 *     * Outputs triangles (a maximum of 3 vertices);
 *     * Defines an output vec4 variable test_variable;
 *     * For each emitted vertex, it should set test_variable to
 *       (1.0, 2.0, 3.0, 4.0);
 *
 *     All the shader objects should now be attached to the program object and
 *     compiled. Transform feedback should be configured for the program object,
 *     so that values stored in test_variable should be stored in the buffer
 *     object described at the beginning.
 *
 *     Test should then generate and bind a vertex array object.
 *
 *     Test should also generate two query objects:
 *
 *         - Query object A of type GL_PRIMITIVES_GENERATED_EXT;
 *         - Query object B of type GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN;
 *
 *     Both of these query objects should be then started.
 *
 *     At this point, the test should activate the program object, start
 *     transform feedback, and draw 8 points.
 *
 *     Test succeeds if values stored in query objects are both not equal
 *     to 0 and different from each other.*/

class GeometryShaderPrimitiveQueries : public TestCaseBase
{
public:
	/* Public methods */
	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	/* Protected methods */
	GeometryShaderPrimitiveQueries(Context& context, const ExtParameters& extParams, const char* name,
								   const char* description);

	virtual ~GeometryShaderPrimitiveQueries(void)
	{
	}

	/* Protected abstract methods */
	virtual glw::GLint  getAmountOfEmittedVertices() = 0;
	virtual const char* getGeometryShaderCode()		 = 0;
	virtual glw::GLenum getTFMode()					 = 0;

private:
	/* Private methods */
	void readPrimitiveQueryValues(glw::GLint bufferId, glw::GLuint* nPrimitivesGenerated,
								  glw::GLuint* nPrimitivesWritten);

	/* Private variables */
	static const char* m_fs_code;
	static const char* m_vs_code;

	const glw::GLint m_n_texture_components;
	glw::GLuint		 m_bo_large_id;
	glw::GLuint		 m_bo_small_id;
	glw::GLuint		 m_fs_id;
	glw::GLuint		 m_gs_id;
	glw::GLuint		 m_po_id;
	glw::GLuint		 m_qo_primitives_generated_id;
	glw::GLuint		 m_qo_tf_primitives_written_id;
	glw::GLuint		 m_vao_id;
	glw::GLuint		 m_vs_id;
};

/**
 * The tests uses points as output primitive type for a geometry shader
 */
class GeometryShaderPrimitiveQueriesPoints : public GeometryShaderPrimitiveQueries
{
public:
	/* Public methods */
	GeometryShaderPrimitiveQueriesPoints(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~GeometryShaderPrimitiveQueriesPoints(void)
	{
	}

protected:
	/* Protected methods */
	glw::GLint  getAmountOfEmittedVertices();
	const char* getGeometryShaderCode();
	glw::GLenum getTFMode();

private:
	/* Private variables */
	static const char* m_gs_code;
};

/**
 * Test uses lines as output primitive type for a geometry shader
 */
class GeometryShaderPrimitiveQueriesLines : public GeometryShaderPrimitiveQueries
{
public:
	/* Public methods */
	GeometryShaderPrimitiveQueriesLines(Context& context, const ExtParameters& extParams, const char* name,
										const char* description);

	virtual ~GeometryShaderPrimitiveQueriesLines(void)
	{
	}

protected:
	/* Protected methods */
	glw::GLint  getAmountOfEmittedVertices();
	const char* getGeometryShaderCode();
	glw::GLenum getTFMode();

private:
	/* Private variables */
	static const char* m_gs_code;
};

/**
 * Test uses triangles as output primitive type for a geometry shader
 */
class GeometryShaderPrimitiveQueriesTriangles : public GeometryShaderPrimitiveQueries
{
public:
	/* Public methods */
	GeometryShaderPrimitiveQueriesTriangles(Context& context, const ExtParameters& extParams, const char* name,
											const char* description);

	virtual ~GeometryShaderPrimitiveQueriesTriangles(void)
	{
	}

protected:
	/* Protected methods */
	glw::GLint  getAmountOfEmittedVertices();
	const char* getGeometryShaderCode();
	glw::GLenum getTFMode();

private:
	/* Private variables */
	static const char* m_gs_code;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERPRIMITIVEQUERIES_HPP
