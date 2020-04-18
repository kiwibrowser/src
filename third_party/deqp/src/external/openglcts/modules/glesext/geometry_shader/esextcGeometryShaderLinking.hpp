#ifndef _ESEXTCGEOMETRYSHADERLINKING_HPP
#define _ESEXTCGEOMETRYSHADERLINKING_HPP
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
/** Implementation of "Group 17", tests 1 & 2 from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. Make sure that linking a program object consisting of geometry shader
 *     object only will fail, assuming the program object is not separable.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Create a program object and a compilable geometry shader object.
 *
 *     Attach the geometry shader object to the program object, compile it, link
 *     the program object.
 *
 *     The test passes if GL_LINK_STATUS for the program object is reported as
 *     GL_FALSE.
 *
 *     Should separate shader objects be supported, the linking should pass,
 *     assuming the program object is marked as separable prior to linking.
 *
 *
 *  2. Make sure that linking a program object consisting of fragment and
 *     geometry shader objects will fail, assuming the program object is not
 *     separable.
 *
 *     Category: API;
 *               Negative Test;
 *
 *     Create a program object and a compilable fragment & geometry shader
 *     objects.
 *
 *     Attach the shader objects to the program object, compile them, link the
 *     program object.
 *
 *     The test passes if GL_LINK_STATUS for the program object is reported as
 *     GL_FALSE.
 *
 *     Should separate shader objects be supported, the linking should pass,
 *     assuming the program object is marked as separable prior to linking.
 *
 **/
class GeometryShaderIncompleteProgramObjectsTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderIncompleteProgramObjectsTest(Context& context, const ExtParameters& extParams, const char* name,
											   const char* description);

	virtual ~GeometryShaderIncompleteProgramObjectsTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definition */
	typedef struct _run
	{
		bool use_fs;
		bool use_gs;
		bool use_separable_po;

		explicit _run(bool in_use_fs, bool in_use_gs, bool in_use_separable_po)
		{
			use_fs			 = in_use_fs;
			use_gs			 = in_use_gs;
			use_separable_po = in_use_separable_po;
		}
	} _run;

	/* Private methods */
	void initShaderObjects();
	void initTestRuns();

	/* Private variables */
	glw::GLuint		  m_fs_id;
	glw::GLuint		  m_gs_id;
	glw::GLuint		  m_po_id;
	std::vector<_run> m_test_runs;
};

/* Implementation of "Group 17", test 3 from CTS_EXT_geometry_shader. Description follows:
 *
 *  3. Make sure that program objects with a single geometry shader do not link
 *     if any of the required geometry stage-specific information is missing.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Create 5 program objects and a boilerplate fragment & vertex shader
 *     objects. Consider the following cases for a geometry shader:
 *
 *     - input primitive type is only defined;
 *     - output primitive type is only defined;
 *     - maximum output vertex count is defined;
 *     - input & output primitive types are only defined
 *     - output primitive type & maximum output vertex count are only defined;
 *
 *     For each of these cases, create a geometry shader with corresponding
 *     implementation. Each such shader can,  but does not necessarily have to,
 *     compile. Whichever be the case, the test should carry on executing.
 *
 *     Attach aforementioned fragment & vertex shaders to each of the program
 *     objects. Attach each of the discussed geometry shaders to subsequent
 *     program objects.
 *
 *     Test succeeds if all program objects failed to link.
 *
 */
class GeometryShaderIncompleteGSTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderIncompleteGSTest(Context& context, const ExtParameters& extParams, const char* name,
								   const char* description);

	virtual ~GeometryShaderIncompleteGSTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definition */
	typedef struct _run
	{
		bool is_input_primitive_type_defined;
		bool is_max_vertices_defined;
		bool is_output_primitive_type_defined;

		explicit _run(bool in_is_input_primitive_type_defined, bool in_is_max_vertices_defined,
					  bool in_is_output_primitive_type_defined)
		{
			is_input_primitive_type_defined  = in_is_input_primitive_type_defined;
			is_max_vertices_defined			 = in_is_max_vertices_defined;
			is_output_primitive_type_defined = in_is_output_primitive_type_defined;
		}
	} _run;

	/* Private methods */
	void deinitSOs();

	std::string getGeometryShaderCode(const _run& current_run);

	void initShaderObjects(const _run& current_run, bool* out_has_fs_compiled_successfully,
						   bool* out_has_gs_compiled_successfully, bool* out_has_vs_compiled_successfully);

	void initTestRuns();

	/* Private variables */
	glw::GLuint		  m_fs_id;
	glw::GLuint		  m_gs_id;
	glw::GLuint		  m_po_id;
	glw::GLuint		  m_vs_id;
	std::vector<_run> m_test_runs;
};

/* Implementation of "Group 17", test 4 from CTS_EXT_geometry_shader. Description follows:
 *
 *  4. Make sure linking fails if input variables of a geometry shader are
 *     declared as arrays of incorrect size.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Consider 5 geometry shaders, each using a different input primitive type
 *     available for geometry shader's usage. Each geometry shader should define
 *     an arrayed input variable of size N called invalid, where N is equal to:
 *
 *               (valid size provided the input primitive type) + 1
 *
 *     The geometry shader should output a compatible output primitive type with
 *     max count set to 1. Rest of the code can be boilerplate, but it must be
 *     valid.
 *
 *     The vertex shader should output non-arrayed variable invalid, but the
 *     rest of the code can be boilerplate, provided it is valid.
 *
 *     Same applies for the fragment shader.
 *
 *     5 program objects, each consisting of a fragment and vertex shaders, as
 *     well as unique geometry shader, should fail to link.
 *
 */
class GeometryShaderInvalidArrayedInputVariablesTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderInvalidArrayedInputVariablesTest(Context& context, const ExtParameters& extParams, const char* name,
												   const char* description);

	virtual ~GeometryShaderInvalidArrayedInputVariablesTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definition */

	/* Private methods */
	void		deinitSOs();
	std::string getGSCode(glw::GLenum gs_input_primitive_type) const;
	std::string getInputPrimitiveTypeQualifier(glw::GLenum gs_input_primitive_type) const;
	std::string getSpecializedVSCode() const;
	glw::GLuint getValidInputVariableArraySize(glw::GLenum gs_input_primitive_type) const;

	void initShaderObjects(glw::GLenum gs_input_primitive_type, bool* out_has_fs_compiled_successfully,
						   bool* out_has_gs_compiled_successfully, bool* out_has_vs_compiled_successfully);

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/* Implementation of "Group 20", test 1 from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. It is a linking error to declare an output variable in a vertex shader
 *     and an input variable in a geometry shader, that is of different type
 *     but has the same qualification and name.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Use a boilerplate vertex shader that declares the output variable
 *     described in summary, a boilerplate geometry shader that declares the
 *     input variable. A geometry shader should use a different type than the
 *     vertex shader for the variable declaration, but should use the same
 *     qualifier.
 *
 *     A boilerplate fragment shader should be used.
 *
 *     Linking is expect to fail under this configuration.
 *
 */
class GeometryShaderVSGSVariableTypeMismatchTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderVSGSVariableTypeMismatchTest(Context& context, const ExtParameters& extParams, const char* name,
											   const char* description);

	virtual ~GeometryShaderVSGSVariableTypeMismatchTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/* Implementation of "Group 20", test 2 from CTS_EXT_geometry_shader. Description follows:
 *
 *  2. It is a linking error to declare an output variable in a vertex shader
 *     and an input variable in a geometry shader, that is of different
 *     qualification but has the same type and name.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Use a boilerplate vertex shader that declares the output variable
 *     described in summary, a boilerplate geometry shader that declares the
 *     input variable. A geometry shader should use a different qualifier than
 *     the vertex shader for the variable declaration, but should use the same
 *     type.
 *
 *     A boilerplate fragment shader should be used.
 *
 *     Linking is expected to fail under this configuration.
 *
 */
class GeometryShaderVSGSVariableQualifierMismatchTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderVSGSVariableQualifierMismatchTest(Context& context, const ExtParameters& extParams, const char* name,
													const char* description);

	virtual ~GeometryShaderVSGSVariableQualifierMismatchTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/* Implementation of "Group 20", test 3 from CTS_EXT_geometry_shader. Description follows:
 *
 *  3. It is a linking error to declare arrayed input variables in a geometry
 *     size if the array sizes do not match.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Create a program object, for which a boilerplate fragment and vertex
 *     shaders will be used. A geometry shader should also be attached to the
 *     program object. The shader should include the following incorrect input
 *     variable declarations:
 *
 *     in vec4 Color1[];
 *     in vec4 Color2[2];
 *     in vec4 Color3[3];
 *
 *     Linking of the program object is expected to fail under this configuration.
 *
 */
class GeometryShaderVSGSArrayedVariableSizeMismatchTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderVSGSArrayedVariableSizeMismatchTest(Context& context, const ExtParameters& extParams,
													  const char* name, const char* description);

	virtual ~GeometryShaderVSGSArrayedVariableSizeMismatchTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/* Implementation of "Group 20", test 4 from CTS_EXT_geometry_shader. Description follows:
 *
 *  4. It is a linking error to re-declare gl_FragCoord in a geometry shader.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Create a program object, for which a boilerplate fragment and vertex
 *     shaders will be used. A geometry shader should also be attached to the
 *     program object. The shader should include the following incorrect input
 *     variable declaration:
 *
 *     in vec4 gl_FragCoord;
 *
 *     Linking of the program object is expected to fail under this
 *     configuration.
 *
 */
class GeometryShaderFragCoordRedeclarationTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderFragCoordRedeclarationTest(Context& context, const ExtParameters& extParams, const char* name,
											 const char* description);

	virtual ~GeometryShaderFragCoordRedeclarationTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/* Implementation of "Group 20", test 5 from CTS_EXT_geometry_shader. Description follows:
 *
 *  5. It is a linking error to use the same location for two output variables
 *     in a geometry shader.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Create a program object, for which a boilerplate fragment and vertex
 *     shaders will be used. A geometry shader should also be attached to the
 *     program object. The shader should include the following incorrect input
 *     variable declaration:
 *
 *     layout(location = 2) out vec4 test;
 *     layout(location = 2) out vec4 test2;
 *
 *     Linking of the program object is expected to fail under this
 *     configuration.
 *
 */
class GeometryShaderLocationAliasingTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderLocationAliasingTest(Context& context, const ExtParameters& extParams, const char* name,
									   const char* description);

	virtual ~GeometryShaderLocationAliasingTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/* Implementation of "Group 23", test 4 from CTS_EXT_geometry_shader. Description follows:
 *
 *  4. Make sure using more than GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT atomic
 *     counters from within a geometry shader results in a linking error.
 *
 *     Category: API.
 *
 *     Create a program object. Define a boilerplate fragment and vertex shader
 *     objects, as well as a geometry shader. The geometry shader should:
 *
 *     - define exactly (GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT+1) atomic counters.
 *     - take points on input and output a maximum of 1 point;
 *     - use only one invocation.
 *     - for each invocation, the shader should increment all atomic counter if
 *       (gl_PrimitiveIDIn % counter_id) == 0, where counter_id stands for "id" of
 *       a shader atomic counter, assuming first shader atomic counter has an
 *       "id" of 1.
 *     - The shader should set gl_Position to (0, 0, 0, 1) for the vertex that
 *       will be emitted.
 *
 *     The test succeeds if linking of the program object fails.
 *
 */
class GeometryShaderMoreACsInGSThanSupportedTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderMoreACsInGSThanSupportedTest(Context& context, const ExtParameters& extParams, const char* name,
											   const char* description);

	virtual ~GeometryShaderMoreACsInGSThanSupportedTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definition */

	/* Private methods */
	std::string getGSCode();

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/* Implementation of "Group 23", test 6 from CTS_EXT_geometry_shader. Description follows:
 *
 *  6. Make sure using more than GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT
 *     buffer objects to back up atomic counter storage for geometry shader
 *     atomic counters results in a linking error.
 *
 *     Category: API.
 *
 *     Create a program object. Define a boilerplate fragment and vertex shader
 *     objects, as well as a geometry shader. The geometry shader should:
 *
 *     - define exactly (GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT+1) atomic
 *       counters.
 *     - take points on input and output a maximum of 1 point;
 *     - use only one invocation.
 *     - for each invocation, the shader should increment all atomic counter if
 *       (gl_PrimitiveIDIn % counter_id) == 0, where counter_id stands for "id" of
 *       a shader atomic counter, assuming first shader atomic counter has an
 *       "id" of 1.
 *     - The shader should set gl_Position to (0, 0, 0, 1) for the vertex that
 *       will be emitted.
 *     - Each atomic counter should use a separate buffer object binding.
 *
 *     The test succeeds if linking of the program object fails.
 *
 */
class GeometryShaderMoreACBsInGSThanSupportedTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderMoreACBsInGSThanSupportedTest(Context& context, const ExtParameters& extParams, const char* name,
												const char* description);

	virtual ~GeometryShaderMoreACBsInGSThanSupportedTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definition */

	/* Private methods */
	std::string getGSCode();

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/* Implementation of "Group 24", test 1 from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. Make sure that linking a program object consisting of a fragment,
 *     geometry and vertex shaders will fail, if geometry shader compilation
 *     status is GL_FALSE.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Create a program object and a fragment, geometry, vertex shader objects:
 *
 *     - Fragment and vertex shader object should be compilable;
 *     - Geometry shader object should not compile.
 *
 *     Compile all three shaders. Attach them to the program object, try to link
 *     the program object.
 *
 *     The test passes if GL_LINK_STATUS for the program object is reported as
 *     GL_FALSE.
 *
 */
class GeometryShaderCompilationFailTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderCompilationFailTest(Context& context, const ExtParameters& extParams, const char* name,
									  const char* description);

	virtual ~GeometryShaderCompilationFailTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/* Implementation of "Group 24", test 4 from CTS_EXT_geometry_shader. Description follows:
 *
 *  4. A geometry shader using more input vertices than are available should
 *     compile, but a program object with the shader attach should not link.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Following is a list of vertex count allowed for each geometry shader
 *     primitive type:
 *
 *     * points - 1;
 *     * lines - 2;
 *     * triangles - 3;
 *     * lines_adjacency - 4;
 *     * triangles_adjacency - 6;
 *
 *     For each geometry shader primitive type, create a geometry shader object.
 *     Each shader should output a maximum of a single point. Result vertex
 *     position should be set to gl_in[X].gl_Position, where X should be equal
 *     to a relevant value from the list above. This geometry shader must
 *     successfully compile.
 *
 *     Create 5 program objects and a fragment and vertex shader objects. Each
 *     of these shaders should use a boilerplate but valid implementation. Each
 *     program object should be assigned both of these shaders, as well as one
 *     of the geometry shaders enlisted above, so that all program objects in
 *     total use up all geometry shaders considered. These program objects
 *     should fail to link.
 *
 */
class GeometryShaderMoreInputVerticesThanAvailableTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderMoreInputVerticesThanAvailableTest(Context& context, const ExtParameters& extParams, const char* name,
													 const char* description);

	virtual ~GeometryShaderMoreInputVerticesThanAvailableTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint		  m_fs_id;
	glw::GLuint*	  m_gs_ids;
	const glw::GLuint m_number_of_gs;
	glw::GLuint*	  m_po_ids;
	glw::GLuint		  m_vs_id;
	glw::GLuint		  m_vao_id;
};

/* Implementation of "Group 25", test 1 from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. Transform feedback which captures data from two variables where:
 *
 *     - the first one is set by vertex shader;
 *     - the second one is set by geometry shader;
 *
 *     should cause linking operation to fail.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     A vertex shader should declare an output variable out_vs_1 of ivec4 type.
 *     It should set it to:
 *
 *          (gl_VertexID, gl_VertexID+1, gl_VertexID+2, gl_VertexID+3)
 *
 *     Rest of the code can be boilerplate but must be valid.
 *
 *     A geometry shader should declare an output variable out_gs_1 of vec4
 *     type. It should set it to:
 *
 *     (gl_VertexID*2, gl_VertexID*2+1, gl_VertexID*2+2, gl_VertexID*2+3)
 *                        (conversions omitted)
 *
 *     The shader should accept points as input and is expected to emit 1 point
 *     at (0, 0, 0, 1). Rest of the code can be boilerplate but should be valid.
 *
 *     The test should configure the program object to use transform feedback so
 *     that values the shaders set for both of the variables are captured. The
 *     test should then attempt to link the program object.
 *
 *     The test passes if GL_LINK_STATUS state of the program object is reported
 *     to be GL_FALSE after glLinkProgram() call.
 *
 */
class GeometryShaderTransformFeedbackVertexAndGeometryShaderCaptureTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderTransformFeedbackVertexAndGeometryShaderCaptureTest(Context& context, const ExtParameters& extParams,
																	  const char* name, const char* description);

	virtual ~GeometryShaderTransformFeedbackVertexAndGeometryShaderCaptureTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERLINKING_HPP
