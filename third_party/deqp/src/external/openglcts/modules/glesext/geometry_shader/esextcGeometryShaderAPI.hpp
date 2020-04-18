#ifndef _ESEXTCGEOMETRYSHADERAPI_HPP
#define _ESEXTCGEOMETRYSHADERAPI_HPP
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
/** Implementation of "Group 18", test 1 from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. Make sure glCreateShaderProgramv() works correctly with geometry
 *     shaders.
 *
 *     Category: API;
 *
 *     Consider two geometry shader implementations (consisting of at least
 *     2 body parts): a compilable (A) and a non-compilable one (B). For (A),
 *     vertex and fragment shader stage implementations as in test case 8.1
 *     should be considered.
 *
 *     Call glCreateShaderProgramv() for both codebases:
 *
 *     - In both cases, a new program object should be created;
 *     - For (A), GL_LINK_STATUS for the program object should be reported
 *       as GL_TRUE; Using a pipeline object to which all three separable
 *       program objects have been attached for each corresponding stage, the
 *       test should draw a single point and check the results (as described in
 *       test case 8.1)
 *     - For (B), GL_LINK_STATUS for the program object should be reported as
 *       GL_FALSE.
 *     - No error should be reported.
 *
 **/
class GeometryShaderCreateShaderProgramvTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderCreateShaderProgramvTest(Context& context, const ExtParameters& extParams, const char* name,
										   const char* description);

	virtual ~GeometryShaderCreateShaderProgramvTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */
	void initFBO();
	void initPipelineObject();

	/* Private variables */
	static const char* fs_code;
	static const char* gs_code;
	static const char* vs_code;

	glw::GLuint m_fbo_id;
	glw::GLuint m_fs_po_id;
	glw::GLuint m_gs_po_id;
	glw::GLuint m_pipeline_object_id;
	glw::GLuint m_to_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_po_id;

	static const unsigned int m_to_height;
	static const unsigned int m_to_width;
};

/* Implementation of "Group 18", test 2 from CTS_EXT_geometry_shader. Description follows:
 *
 *  2. GL_GEOMETRY_SHADER_EXT is reported by glGetShaderiv() for geometry shaders.
 *
 *     Category: API.
 *
 *     Create a geometry shader object. Make sure glGetShaderiv() reports
 *     GL_GEOMETRY_SHADER_EXT when passed GL_SHADER_TYPE pname and the shader's
 *     id.
 *
 */
class GeometryShaderGetShaderivTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderGetShaderivTest(Context& context, const ExtParameters& extParams, const char* name,
								  const char* description);

	virtual ~GeometryShaderGetShaderivTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_gs_id;
};

/* Implementation of "Group 18", test 3 from CTS_EXT_geometry_shader. Description follows:
 *
 *  3. GL_INVALID_OPERATION error is generated if geometry stage-specific
 *     queries such as:
 *
 *     * GL_GEOMETRY_LINKED_VERTICES_OUT_EXT;
 *     * GL_GEOMETRY_LINKED_INPUT_TYPE_EXT;
 *     * GL_GEOMETRY_LINKED_OUTPUT_TYPE_EXT;
 *     * GL_GEOMETRY_SHADER_INVOCATIONS_EXT;
 *
 *     are passed to glGetProgramiv() for a program object that was not linked
 *     successfully.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Create a program object.
 *
 *     Issue all of the above queries on the program object. The test fails if
 *     any of these queries does not result in a GL_INVALID_OPERATION error.
 *
 */
class GeometryShaderGetProgramivTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderGetProgramivTest(Context& context, const ExtParameters& extParams, const char* name,
								   const char* description);

	virtual ~GeometryShaderGetProgramivTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_po_id;
};

/* Implementation of "Group 18", test 4 from CTS_EXT_geometry_shader. Description follows:
 *
 *  4. GL_INVALID_OPERATION error is generated if geometry stage-specific
 *     queries such as:
 *
 *     * GL_GEOMETRY_LINKED_VERTICES_OUT_EXT;
 *     * GL_GEOMETRY_LINKED_INPUT_TYPE_EXT;
 *     * GL_GEOMETRY_LINKED_OUTPUT_TYPE_EXT;
 *     * GL_GEOMETRY_SHADER_INVOCATIONS_EXT;
 *
 *     are passed to glGetProgramiv() for a linked program object that does not
 *     have a geometry shader attached.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Create a program object and attach boilerplate fragment and vertex
 *     shaders to it. The program object should link successfully.
 *
 *     Issue all of the above queries on the program object. The test fails if
 *     any of these queries does not result in a GL_INVALID_OPERATION error.
 *
 */
class GeometryShaderGetProgramiv2Test : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderGetProgramiv2Test(Context& context, const ExtParameters& extParams, const char* name,
									const char* description);

	virtual ~GeometryShaderGetProgramiv2Test()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/* Implementation of "Group 19", test 1 from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. Program objects that either have been attached compilable fragment &
 *     geometry & vertex shader objects attached and link successfully OR which
 *     have been assigned successfully linked separate fragment & geometry &
 *     shader object programs can be queried for geometry stage properties
 *
 *     Category: API;
 *
 *     The test should iterate through 3 different fragment + geometry + vertex
 *     shader object configurations.
 *     It should query the following geometry shader states and verify the
 *     reported values:
 *
 *     1) GL_GEOMETRY_LINKED_VERTICES_OUT_EXT,
 *     2) GL_GEOMETRY_LINKED_INPUT_TYPE_EXT,
 *     3) GL_GEOMETRY_LINKED_OUTPUT_TYPE_EXT,
 *     4) GL_GEOMETRY_SHADER_INVOCATIONS_EXT
 *
 *     For pipeline objects, an additional case should be checked: using
 *     a separable program object with fragment & vertex shader stages defined
 *     as a source for geometry shader stage should succeed but reported
 *     geometry shader program object id (retrieved by calling
 *     glGetProgramPipelineiv() with GL_GEOMETRY_SHADER_EXT pname) should return 0.
 *
 */
class GeometryShaderGetProgramiv3Test : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderGetProgramiv3Test(Context& context, const ExtParameters& extParams, const char* name,
									const char* description);

	virtual ~GeometryShaderGetProgramiv3Test()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */
	typedef struct _run
	{
		glw::GLenum input_primitive_type;
		int			invocations;
		int			max_vertices;
		glw::GLenum output_primitive_type;

		explicit _run(glw::GLenum in_input_primitive_type, int in_invocations, int in_max_vertices,
					  glw::GLenum in_output_primitive_type)
		{
			input_primitive_type  = in_input_primitive_type;
			invocations			  = in_invocations;
			max_vertices		  = in_max_vertices;
			output_primitive_type = in_output_primitive_type;
		}
	} _run;

	/* Private methods */
	bool buildShader(glw::GLuint so_id, const char* so_body);

	bool buildShaderProgram(glw::GLuint* out_spo_id, glw::GLenum spo_bits, const char* spo_body);

	void deinitPO();

	void deinitSOs(bool release_all_SOs);

	void deinitSPOs(bool release_all_SPOs);

	std::string getLayoutQualifierForPrimitiveType(glw::GLenum primitive_type);

	std::string getGSCode(const _run& run);

	void initTestRuns();

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_fs_po_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_gs_po_id;
	glw::GLuint m_pipeline_object_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
	glw::GLuint m_vs_po_id;

	std::vector<_run> _runs;
};

/* Implementation of "Group 19", test 2 from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. For an active pipeline object consisting of a geometry shader stage but
 *     lacking a vertex shader stage, GL_INVALID_OPERATION error should be
 *     generated if an application attempts to perform a draw call.
 *
 *     Category: API;
 *               Negative Test;
 *
 *     Create a pipeline object.
 *
 *     Create boilerplate separate fragment and geometry shader program objects.
 *     Configure the pipeline object to use them correspondingly for fragment
 *     and geometry stages.
 *
 *     Generate and bind a vertex array object.
 *
 *     The test should now bind the pipeline object and do a glDrawArrays()
 *     call.
 *
 *     Test succeeds if GL_INVALID_OPERATION error was generated.
 *
 */
class GeometryShaderDrawCallWithFSAndGS : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderDrawCallWithFSAndGS(Context& context, const ExtParameters& extParams, const char* name,
									  const char* description);

	virtual ~GeometryShaderDrawCallWithFSAndGS()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fs_po_id;
	glw::GLuint m_gs_po_id;
	glw::GLuint m_pipeline_object_id;
	glw::GLuint m_vao_id;
};

/* Implementation of "Group 23", test 1 from CTS_EXT_geometry_shader. Description follows:
 *
 *  1. It should be possible to use as many image uniforms in a geometry shader
 *     as reported for GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT property.
 *
 *     Category: API;
 *               Functional Test.
 *
 *     1.  Create a program object consisting of a fragment, geometry and vertex
 *         shader:
 *     1a. The shaders can have boilerplate implementation but should be
 *         compatible with each other.
 *     1b. Geometry shader should take points on input and output a maximum of
 *         1 point.
 *     1c. Geometry shader should define exactly as many image uniforms as
 *         reported for the property.
 *     1d. Geometry shader should load values from all bound images, sum them up,
 *         and store the result in X component of output vertex.
 *     2.  Configure transform feedback to capture gl_Position output from the
 *         program.
 *     3.  All shaders should compile successfully and the program object should
 *         link without problems.
 *     4.  Bind integer 2D texture objects of resolution 1x1 to all image units.
 *         First texture used should use a value of 1, second texture should use
 *         a value of 2, and so on.
 *     5.  Generate, bind a vertex array object, do a single point draw call. The
 *         test succeeds if the first component in the vector retrieved is equal
 *         to sum(i=1..n)(i) = n(n+1)/2 where n =
 *         GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT property value.
 *
 */
class GeometryShaderMaxImageUniformsTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderMaxImageUniformsTest(Context& context, const ExtParameters& extParams, const char* name,
									   const char* description);

	virtual ~GeometryShaderMaxImageUniformsTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */
	std::string getGSCode();

	/* Private variables */
	glw::GLuint  m_fs_id;
	glw::GLint   m_gl_max_geometry_image_uniforms_ext_value;
	glw::GLuint  m_gs_id;
	glw::GLuint  m_po_id;
	glw::GLuint* m_texture_ids;
	glw::GLuint  m_tfbo_id;
	glw::GLuint  m_vao_id;
	glw::GLuint  m_vs_id;
};

/* Implementation of "Group 23", test 2 from CTS_EXT_geometry_shader. Description follows:
 *
 *  2. It should be possible to use as many shader storage blocks in a geometry
 *     shader as reported for GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT property.
 *
 *     Category: API;
 *             Functional Test.0
 *
 *     1. Create a program object consisting of a fragment, geometry and vertex
 *        shader:
 *     1a. The shaders can have boilerplate implementation but should be
 *         compatible with each other.
 *     1b. Geometry shader should take points on input and output a maximum of
 *         1 point.
 *     1c. Geometry shader should define exactly as many shader storage blocks
 *         as reported for the property, each using subsequent shader storage
 *         buffer binding points. Each storage block should take a single int
 *         value.
 *     1d. Geometry shader should read values from all bound SSBOs, write
 *         *incremented* values. It should also store a summed-up result (along
 *         the lines of test case 23.1) calculated from values *prior* to
 *         incrementation in X component of output vertex.
 *     2. Configure transform feedback to capture gl_Position output from the
 *        program.
 *     3. All shaders should compile successfully and the program object should
 *        link without problems.
 *     4. Initialize a buffer object filled with subsequent int values (starting
 *        from 1). Bind corresponding ranges (of size sizeof(int) ) to subsequent
 *        SSBO binding points.
 *     5. Generate, bind a vertex array object, do a single point draw call. The
 *        test succeeds if the value retrieved is equal to
 *        sum(i=1..n)(i) = n(n+1)/2 where n =
 *        GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT property value AND if the
 *        buffer object storing input data is now filled with increasing values,
 *        assuming a delta of 1 and a start value of 2.
 *
 */
class GeometryShaderMaxShaderStorageBlocksTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderMaxShaderStorageBlocksTest(Context& context, const ExtParameters& extParams, const char* name,
											 const char* description);

	virtual ~GeometryShaderMaxShaderStorageBlocksTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */
	std::string getGSCode();

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLint  m_gl_max_geometry_shader_storage_blocks_ext_value;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_ssbo_id;
	glw::GLuint m_tfbo_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

/* Implementation of "Group 23", test 3 from CTS_EXT_geometry_shader. Description follows:
 *
 *  3. Make sure writing up to GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT atomic
 *     counters from within a geometry shader works correctly.
 *
 *     Category: API.
 *
 *     Create a program object. Define a boilerplate fragment shader object,
 *     a vertex and a geometry shader.
 *
 *     The vertex shader should:
 *
 *     - pass gl_VertexID information to the geometry shader using an output
 *       int variable called vertex_id.
 *
 *     The geometry shader should:
 *
 *     - define exactly GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT atomic counters.
 *     - take points on input and output a maximum of 1 point;
 *     - use only one invocation.
 *     - the shader should increment all atomic counters, for which
 *       (vertex_id % counter_id) == 0, where counter_id stands for "id" of
 *       a shader atomic counter, assuming first shader atomic counter has an
 *       "id" of 1.
 *
 *     A single buffer object should be used to back up the storage. It should
 *     be filled with zeros on start-up.
 *
 *     The test should draw 128*GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT points. It then
 *     should read the buffer object's contents and make sure the values read
 *     are valid.
 *
 */
class GeometryShaderMaxAtomicCountersTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderMaxAtomicCountersTest(Context& context, const ExtParameters& extParams, const char* name,
										const char* description);

	virtual ~GeometryShaderMaxAtomicCountersTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */
	std::string getGSCode();

	/* Private variables */
	glw::GLuint m_acbo_id;
	glw::GLuint m_fs_id;
	glw::GLint  m_gl_max_geometry_atomic_counters_ext_value;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

/* Implementation of "Group 23", test 5 from CTS_EXT_geometry_shader. Description follows:
 *
 *  5. Make sure writing up to GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT atomic
 *     counter buffers from within a geometry shader works correctly.
 *
 *     Category: API.
 *
 *     Create a program object. Define a boilerplate fragment shader object,
 *     a vertex and a geometry shader.
 *
 *     The vertex shader should:
 *
 *     - pass gl_VertexID information to the geometry shader using an output
 *     int variable called vertex_id.
 *
 *     The geometry shader should:
 *
 *     - define exactly GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT atomic
 *       counters.
 *     - take points on input and output a maximum of 1 point;
 *     - use only one invocation.
 *     - the shader should increment all atomic counter if
 *       (vertex_id % counter_id) == 0, where counter_id stands for "id" of
 *       a shader atomic counter, assuming first shader atomic counter has an
 *       "id" of 1.
 *
 *     Each atomic counter should use a separate buffer object binding to back
 *     up the storage. They should be filled with zeros on start-up.
 *
 *     The test should draw 128*GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT
 *     points. It then should read the buffer objects' contents and make sure
 *     the values read are valid.
 *
 */
class GeometryShaderMaxAtomicCounterBuffersTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderMaxAtomicCounterBuffersTest(Context& context, const ExtParameters& extParams, const char* name,
											  const char* description);

	virtual ~GeometryShaderMaxAtomicCounterBuffersTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */
	std::string getGSCode();

	/* Private variables */
	glw::GLuint* m_acbo_ids;
	glw::GLuint  m_fs_id;
	glw::GLint   m_gl_max_atomic_counter_buffer_bindings_value;
	glw::GLint   m_gl_max_geometry_atomic_counter_buffers_ext_value;
	glw::GLuint  m_gs_id;
	glw::GLuint  m_po_id;
	glw::GLuint  m_vao_id;
	glw::GLuint  m_vs_id;
};

/* Implementation of "Group 24", test 2 from CTS_EXT_geometry_shader. Description follows:
 *
 *  2. Make sure that draw calls results in an error if the bound pipeline
 *     program object has a configured geometry stage but has no active program
 *     with an executable vertex shader.
 *
 *     Category: API;
 *
 *     Generate a pipeline object.
 *
 *     Create stand-alone fragment and geometry programs. Both of the programs
 *     should be provided valid but boilerplate implementation. Use these
 *     programs to define fragment and geometry stages for the pipeline object.
 *
 *     Generate and bind a vertex array object. Bind the pipeline object and
 *     try to draw a single point. Test succeeds if GL_INVALID_OPERATION error
 *     was generated.
 *
 */
class GeometryShaderPiplineProgramObjectWithoutActiveVSProgramTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderPiplineProgramObjectWithoutActiveVSProgramTest(Context& context, const ExtParameters& extParams,
																 const char* name, const char* description);

	virtual ~GeometryShaderPiplineProgramObjectWithoutActiveVSProgramTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_fs_id;
	glw::GLuint m_fs_po_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_gs_po_id;
	glw::GLuint m_ppo_id;
	glw::GLuint m_vao_id;
};

/* Implementation of "Group 24", test 3 from CTS_EXT_geometry_shader. Description follows:
 *
 *  3. Verify that doing a draw call using a mode that is incompatible with
 *     input type of geometry shader active in current pipeline results in
 *     GL_INVALID_OPERATION error.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Create 5 program objects and a fragment & vertex shader objects. These
 *     shaders are free to use boilerplate code. Create 5 geometry shaders, each
 *     using a different input primitive type allowed for geometry shaders.
 *     Each program object should be assigned a set of fragment, geometry and
 *     vertex shaders, assuming each program is set a different geometry shader.
 *     Compile all shaders and link the program objects.
 *
 *     Generate a vertex array object and bind it.
 *
 *     For each valid draw call mode, iterate through all program objects. For
 *     program objects that accept an input primitive geometry type deemed
 *     incompatible for the draw call mode considered, try executing the draw
 *     call. Test fails if any of these draw calls does not result in
 *     a GL_INVALID_OPERATION.
 *
 *     Invalid primitive types for all available draw call modes:
 *
 *     * GL_LINE_LOOP draw call mode:
 *     1) lines_adjacency;
 *     2) points;
 *     3) triangles;
 *     4) triangles with adjacency;
 *
 *     * GL_LINE_STRIP draw call mode:
 *     1) lines with adjacency;
 *     2) points;
 *     3) triangles;
 *     4) triangles with adajcency;
 *
 *     * GL_LINE_STRIP_ADJACENCY_EXT draw call mode:
 *     1) lines;
 *     2) points;
 *     3) triangles;
 *     4) triangles with adjacency;
 *
 *     * GL_LINES draw call mode:
 *     1) lines with adjacency;
 *     2) points;
 *     3) triangles;
 *     4) triangles with adjacency;
 *
 *     * GL_LINES_ADJACENCY_EXT draw call mode:
 *     1) lines;
 *     2) points;
 *     3) triangles;
 *     4) triangles with adjacency;
 *
 *     * GL_POINTS draw call mode:
 *     1) lines;
 *     2) lines with adjacency;
 *     3) triangles;
 *     4) triangles with adjacency;
 *
 *     * GL_TRIANGLE_FAN draw call mode:
 *     1) lines;
 *     2) lines with adjacency;
 *     3) points;
 *     4) triangles with adjacency;
 *
 *     * GL_TRIANGLE_STRIP draw call mode:
 *     1) lines;
 *     2) lines with adjacency;
 *     3) points;
 *     4) triangles with adjacency;
 *
 *     * GL_TRIANGLES draw call mode:
 *     1) lines;
 *     2) lines with adjacency;
 *     3) points;
 *     4) triangles with adjacency;
 *
 *     * GL_TRIANGLES_ADJACENCY_EXT draw call mode:
 *     1) lines;
 *     2) lines with adjacency;
 *     3) points;
 *     4) triangles;
 *
 *     * GL_TRIANGLE_STRIP_ADJACENCY_EXT draw call mdoe:
 *     1) lines;
 *     2) lines with adjacency;
 *     3) points;
 *     4) triangles;
 *
 */
class GeometryShaderIncompatibleDrawCallModeTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderIncompatibleDrawCallModeTest(Context& context, const ExtParameters& extParams, const char* name,
											   const char* description);

	virtual ~GeometryShaderIncompatibleDrawCallModeTest()
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

/* Implementation of "Group 24", test 5 from CTS_EXT_geometry_shader. Description follows:
 *
 *  5. Make sure that nothing is drawn if the number of vertices emitted by
 *     a geometry shader is insufficient to produce a single primitive.
 *
 *     Category: API;
 *               Functional/Negative Test.
 *
 *     For each output primitive type from the following list:
 *
 *     - Line strip;
 *     - Triangle strip;
 *
 *     Create a geometry shader that accepts a single point and emits exactly
 *     (N-1) vertices, coordinates of which are located within <-1,1>x<-1,1>
 *     region, Z set to 0 and W to 1, where N corresponds to exact amount of
 *     vertices needed to output a single primitive for output primitive type
 *     considered.
 *
 *     Create a vertex shader object with boilerplate but valid implementation.
 *     Create a fragment shader object setting the only output variable to
 *     (1, 0, 0, 0).
 *
 *     Create 2 program objects, compile all the shaders. For each program
 *     object, attach vertex and fragment shader objects discussed, as well as
 *     one of the geometry shaders discussed. All program objects together
 *     should use all geometry shaders discussed in the second paragraph.
 *     Link all program objects.
 *
 *     Create a FBO and a texture object using a GL_RGBA8 internalformat and
 *     of 16x16 resolution. Attach the texture object to color attachment 0 of
 *     the FBO, bind the FBO to both framebuffer targets.
 *
 *     Create a vertex array object and bind it.
 *
 *     Set clear color to (0, 1, 0, 0).
 *
 *     Iterate through all program objects. For each iteration:
 *     - Before doing actual draw call, clear the color buffer.;
 *     - Activate program object specific for current iteration;
 *     - Draw a single point.
 *     - Read rendered contents (glReadPixels() call with GL_RGBA format and
 *     GL_UNSIGNED_BYTE type) and make sure all pixels are set to (0, 255, 0, 0).
 *
 */
class GeometryShaderInsufficientEmittedVerticesTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderInsufficientEmittedVerticesTest(Context& context, const ExtParameters& extParams, const char* name,
												  const char* description);

	virtual ~GeometryShaderInsufficientEmittedVerticesTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLubyte* m_pixels;

	glw::GLuint		  m_fbo_id;
	glw::GLuint		  m_fs_id;
	glw::GLuint*	  m_gs_ids;
	const glw::GLuint m_number_of_color_components;
	const glw::GLuint m_number_of_gs;
	glw::GLuint*	  m_po_ids;
	const glw::GLuint m_texture_height;
	glw::GLuint		  m_texture_id;
	const glw::GLuint m_texture_width;
	glw::GLuint		  m_vs_id;
	glw::GLuint		  m_vao_id;
};

/* Implementation of "Group 25", test 2 from CTS_EXT_geometry_shader. Description follows:
 *
 *  2. Transform feedback which captures data from two variables where,
 *     configured separately for two separable program objects where:
 *
 *     - the first one defines vertex shader stage (as in test case 25.1); is set
 *       by a program activated for vertex shader stage;
 *     - the second one defines geometry shader stage (as in test case 25.1);
 *       should correctly capture output variables from either geometry shader
 *       stage (if both stages are active) or vertex shader stage (if geometry
 *       shader stage is inactive) in a result buffer object.
 *
 *     Category: API;
 *               Functional Test.
 *
 *     Modify test case 25.1 to use pipeline objects.
 *
 *     First, a pipeline object consisting of both stages, should be used for
 *     the test. Next, the geometry shader stage should be detached from the
 *     geometry shader.
 *     Transform feedback should be paused before the pipeline object's geometry
 *     shader stage is deactivated.
 *
 *     Test succeeds if:
 *
 *     - out_gs_1 variable values are correctly captured if both shader stages
 *       are being used for the pipeline object;
 *     - out_vs_1 variable values are correctly captured if only vertex shader
 *       stage is being used for the pipeline object;
 *
 */
class GeometryShaderPipelineObjectTransformFeedbackVertexAndGeometryShaderCaptureTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderPipelineObjectTransformFeedbackVertexAndGeometryShaderCaptureTest(Context&			 context,
																					const ExtParameters& extParams,
																					const char*			 name,
																					const char*			 description);

	virtual ~GeometryShaderPipelineObjectTransformFeedbackVertexAndGeometryShaderCaptureTest()
	{
	}

	virtual void		  deinit();
	virtual IterateResult iterate();

private:
	/* Private type definition */

	/* Private methods */

	/* Private variables */
	glw::GLuint m_gs_id;
	glw::GLuint m_gs_po_id;
	glw::GLuint m_ppo_id;
	glw::GLuint m_tfbo_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
	glw::GLuint m_vs_po_id;
};

/* Implementation of "Group 25", test 3 from CTS_EXT_geometry_shader. Description follows:
 *
 *  3. Make sure that, while transform feedback is active, attempts to draw
 *     primitives that do not match output primitive type of the geometry shader
 *     are rejected with GL_INVALID_OPERATION error.
 *
 *     Category: API;
 *               Negative Test.
 *
 *     Consider a program object, for which fragment, geometry and vertex
 *     shaders have been defined and attached. The geometry shader meets the
 *     following requirements:
 *
 *     - accepts lines input;
 *     - outputs triangles (a maximum of 3 vertices);
 *     - defines and assigns a value to an output variable that can be used for
 *       transform feedback process.
 *
 *     Fragment & vertex shader stages are boilerplate. Program object should
 *     capture data stored in the geometry shader's output variable described.
 *
 *     Using the program object and GL_LINES transform feedback primitive mode,
 *     doing a draw call in GL_TRIANGLES mode should result in
 *     GL_INVALID_OPERATION error.
 *
 */
class GeometryShaderDrawPrimitivesDoNotMatchOutputPrimitives : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderDrawPrimitivesDoNotMatchOutputPrimitives(Context& context, const ExtParameters& extParams,
														   const char* name, const char* description);

	virtual ~GeometryShaderDrawPrimitivesDoNotMatchOutputPrimitives()
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
	glw::GLuint m_tfbo_id;
	glw::GLuint m_vs_id;
	glw::GLuint m_vao_id;
};

/* Implementation of "Group 25", test 4 from CTS_EXT_geometry_shader. Description follows:
 *
 *  4. Make sure that, while transform feedback is paused, all draw calls
 *     executed with an active program object, which includes a geometry shader,
 *     are valid. All input primitive type+output primitive type configurations
 *     should be considered.
 *
 *     Category: API.
 *
 *     The test should run through all three transform feedback primitive modes,
 *     but should pause transform feedback before doing a check draw call. All
 *     permutations of valid input and output primitive types should be
 *     considered, actual implementation can be boilerplate.
 *
 *     Note: in order to keep the execution times and implementation complexity
 *           level in control, this test is not focused on verifying visual
 *           outcome of the draw calls. Its aim is to only verify that the
 *           driver correctly handles use cases where the transform feedback is
 *           paused and draw calls are issued, meaning no error is reported.
 *
 */
class GeometryShaderDrawCallsWhileTFPaused : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderDrawCallsWhileTFPaused(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~GeometryShaderDrawCallsWhileTFPaused()
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
	glw::GLuint m_po_ids[15] /* All combinations of possible inputs and outputs in GS */;
	glw::GLuint m_tfbo_id;
	glw::GLuint m_vs_id;
	glw::GLuint m_vao_id;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERAPI_HPP
