#ifndef _ESEXTCDRAWELEMENTSBASEVERTEXTESTS_HPP
#define _ESEXTCDRAWELEMENTSBASEVERTEXTESTS_HPP
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

/**
 */ /*!
 * \file  esextcDrawElementsBaseVertexTests.hpp
 * \brief Declares test classes that verify conformance of the
 *        "draw elements base vertex" functionality for both
 *        ES and GL.
 */ /*-------------------------------------------------------------------*/
#include <memory.h>

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"
#include <string.h>

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
using deqp::Context;
using deqp::TestCaseGroup;

/** Base test class implementation for "draw elements base vertex" functionality
 *  conformance tests.
 *
 *  Functional tests fill m_test_cases instance with test case descriptors,
 *  and then call base class' executeTestCases() method to process these
 *  test cases. The base class also initializes all required function
 *  pointers and reassures that no test cases which are dependent on other
 *  ES extensions are not executed, if any of these extensions are not
 *  supported.
 */
class DrawElementsBaseVertexTestBase : public TestCaseBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexTestBase(glcts::Context& context, const ExtParameters& extParams, const char* name,
								   const char* description);

protected:
	/* Protected type definitions */
	/** Defines a draw call type that should be used for a single
	 *  test case iteration.
	 */
	enum _function_type
	{
		FUNCTION_GL_DRAW_ELEMENTS_BASE_VERTEX,
		FUNCTION_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX,
		FUNCTION_GL_DRAW_RANGE_ELEMENTS_BASE_VERTEX,
		FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX,

		FUNCTION_COUNT
	};

	/** Defines a single test case. Each functional test fills the
	 *  m_test_cases vector with test case descriptors, which are
	 *  then traversed by base class'executeTestCases() method.
	 */
	typedef struct _test_case
	{
		glw::GLint basevertex; /* Tells the value of <basevertex> argument for the tested
		 * basevertex draw call. */
		_function_type function_type; /* Tells the type of the basevertex draw call that should
		 * be used for the described iteration. */
		const glw::GLuint* index_offset; /* Tells the value of <indices> argument for both basevertex
		 * and regular draw calls */
		glw::GLenum  index_type;					   /* GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT or GL_UNSIGNED_INT */
		glw::GLuint  range_start;					   /* The range start for DrawRangeElements */
		glw::GLuint  range_end;						   /* The range end for DrawRangeElements */
		glw::GLsizei multi_draw_call_count_array[3];   /* an array of three elements storing "count" arguments
		 * for multi draw calls used to generate contents of both
		 * textures. */
		glw::GLuint* multi_draw_call_indices_array[3]; /* an array of three elements holding "indices" arguments
		 * for multi draw call used rto generate contents of base
		 * texture */
		glw::GLenum primitive_mode; /* Tells the primitive type that should be used for both
		 * types of draw calls. */
		glw::GLuint   regular_draw_call_offset;					   /* offset to be used for non-basevertex draw calls made
		 * to generate reference texture contents. This value will
		 * be added to test_case.index_offset for non-basevertex
		 * draw calls. */
		glw::GLuint*  regular_draw_call_offset2;				   /* offset to be used for non-basevertex draw calls made
		 * to generate reference texture contents. This value will
		 * NOT be added to test_case.index_offset for non-basevertex
		 * draw calls. */
		glw::GLenum   regular_draw_call_index_type;				   /* index type to be used for non-basevertex draw calls made
		 * to generate reference texture contents. The index type
		 is differenet with base draw call in overflow test.*/
		glw::GLuint** regular_multi_draw_call_offseted_array[3];   /* an array of three elements storing offsets for the
		 * multi draw call used to generate reference texture
		 * contents */
		bool		  should_base_texture_match_reference_texture; /* Tells if the iteration passes if the base and the reference
		 * texture are a match (true), or if it should only pass if
		 * contents of the textures are different. */
		bool use_clientside_index_data; /* Tells if the index data should be taken from a client-side
		 * buffer, or from a VBO */
		bool use_clientside_vertex_data; /* Tells if the vertex (color & position) data should be taken
		 * from a client-side buffer, or from a VBO */
		bool use_geometry_shader_stage; /* Tells if the program object used for the iteration should
		 * include geometry shader */
		bool use_tessellation_shader_stage; /* Tells if the program object used for the iteration should
		 * include tessellation control & evaluation shaders. */
		bool use_vertex_attrib_binding; /* Tells if the iteration should use vertex attribute bindings
		 * instead of setting vertex attribute arrays with
		 * glVertexAttribPointer() call(s) */
		bool use_overflow_test_vertices;

		/** Constructor */
		_test_case()
			: basevertex(0)
			, function_type(FUNCTION_COUNT)
			, index_offset(NULL)
			, index_type(0)
			, range_start(0)
			, range_end(0)
			, primitive_mode((glw::GLenum)-1)
			, regular_draw_call_offset(0)
			, regular_draw_call_offset2(NULL)
			, regular_draw_call_index_type(0)
			, should_base_texture_match_reference_texture(false)
			, use_clientside_index_data(false)
			, use_clientside_vertex_data(false)
			, use_geometry_shader_stage(false)
			, use_tessellation_shader_stage(false)
			, use_vertex_attrib_binding(false)
			, use_overflow_test_vertices(false)
		{
			memset(multi_draw_call_count_array, 0, sizeof(multi_draw_call_count_array));
			memset(multi_draw_call_indices_array, 0, sizeof(multi_draw_call_indices_array));
			memset(regular_multi_draw_call_offseted_array, 0, sizeof(regular_multi_draw_call_offseted_array));
		}
	} _test_case;

	/** Type definitions for test case container */
	typedef std::vector<_test_case>		_test_cases;
	typedef _test_cases::const_iterator _test_cases_const_iterator;
	typedef _test_cases::iterator		_test_cases_iterator;

	/* Protected methods */
	void compareBaseAndReferenceTextures(bool should_be_equal);

	void computeVBODataOffsets(bool use_clientside_index_data, bool use_clientside_vertex_data);

	virtual void deinit();

	void executeTestCases();

	std::string getFunctionName(_function_type function_type);

	virtual void init();

	void setUpFunctionalTestObjects(bool use_clientside_vertex_data, bool use_clientside_index_data,
									bool use_tessellation_shader_stage, bool use_geometry_shader_stage,
									bool use_vertex_attrib_binding, bool use_overflow_test_vertices);

	virtual void deinitPerTestObjects();

	void setUpNegativeTestObjects(bool use_clientside_vertex_data, bool use_clientside_index_data);

	/* Protected variables */
	bool m_is_draw_elements_base_vertex_supported; /* Corresponds to GL_EXT_draw_elements_base_vertex availability
	 * under ES contexts and to GL_ARB_draw_elements_base_vertex
	 * availability under GL contexts.
	 */
	bool m_is_ext_multi_draw_arrays_supported; /* Corresponds to GL_EXT_multi_draw_arrays availability under
	 * both ES and GL contexts.
	 */
	bool m_is_geometry_shader_supported;	   /* Corresponds to GL_EXT_geometry_shader availability under
	 * ES contexts and to GL_ARB_geometry_shader4 availability under
	 * GL contexts.
	 */
	bool m_is_tessellation_shader_supported;   /* Corresponds to GL_EXT_tessellation_shader availability under
	 * ES contexts and to GL_ARB_tessellation_shader availability
	 * under GL contexts.
	 */
	bool m_is_vertex_attrib_binding_supported; /* Corresponds to GL_ARB_vertex_attrib_binding availability under
	 * GL contexts. Under ES always set to true, since the conformance
	 * tests are only run for >= ES 3.1 contexts, where VAA bindings
	 * are core functionality.
	 */

	glw::GLuint m_bo_id;
	glw::GLuint m_bo_id_2;
	glw::GLuint m_fbo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLint  m_po_color_attribute_location;
	bool		m_po_uses_gs_stage;
	bool		m_po_uses_tc_te_stages;
	bool		m_po_uses_vertex_attrib_binding;
	glw::GLint  m_po_vertex_attribute_location;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_to_base_id;
	glw::GLuint m_to_ref_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;

	const glw::GLuint*   m_bo_functional2_data_index;		/* Holds functional second index data set data */
	unsigned int		 m_bo_functional2_data_index_size;  /* Holds size of functional second index data set */
	const glw::GLfloat*  m_bo_functional2_data_vertex;		/* Holds functional second vertex data set data */
	unsigned int		 m_bo_functional2_data_vertex_size; /* Holds size of functional second vertex data set */
	const glw::GLubyte*  m_bo_functional3_data_index;		/* Holds functional third index data set data */
	unsigned int		 m_bo_functional3_data_index_size;  /* Holds size of functional third index data set */
	const glw::GLushort* m_bo_functional4_data_index;		/* Holds functional fourth index data set data */
	unsigned int		 m_bo_functional4_data_index_size;  /* Holds size of functional fourth index data set */
	const glw::GLuint*   m_bo_functional5_data_index;		/* Holds functional fifth index data set data */
	unsigned int		 m_bo_functional5_data_index_size;  /* Holds size of functional fifth index data set */
	const glw::GLfloat*  m_bo_functional_data_color;		/* Holds functional first color data set data */
	unsigned int		 m_bo_functional_data_color_size;   /* Holds size of functional first color data set */
	const glw::GLuint*   m_bo_functional_data_index;		/* Holds functional first index data set data */
	unsigned int		 m_bo_functional_data_index_size;   /* Holds size of functional first index data set */
	const glw::GLfloat*  m_bo_functional_data_vertex;		/* Holds functional first vertex data set data */
	unsigned int		 m_bo_functional_data_vertex_size;  /* Holds size of functional first vertex data set */
	const glw::GLuint*   m_bo_negative_data_index;			/* Holds negative index data set data */
	unsigned int		 m_bo_negative_data_index_size;		/* Holds size of negative index data set */
	const glw::GLfloat*  m_bo_negative_data_vertex;			/* Holds negative vertex data set data */
	unsigned int		 m_bo_negative_data_vertex_size;	/* Holds size of negative vertex data set */
	const glw::GLfloat*  m_draw_call_color_offset;			/* Either holds buffer object storage offset to color data set OR
	 * is a raw pointer to the color data store. Actual contents
	 * is iteration-dependent */
	const glw::GLuint*   m_draw_call_index_offset;			/* Either holds buffer object storage offset to first index data set OR
	 * is a raw pointer to the index data store. Actual contents
	 * is iteration-dependent */
	const glw::GLuint*
		m_draw_call_index2_offset; /* Either holds buffer object storage offset to second index data set OR
	 * is a raw pointer to the index data store. Actual contents
	 * is iteration-dependent */
	const glw::GLubyte*
		m_draw_call_index3_offset; /* Either holds buffer object storage offset to third index data set OR
	 * is a raw pointer to the index data store. Actual contents
	 * is iteration-dependent */
	const glw::GLushort*
		m_draw_call_index4_offset; /* Either holds buffer object storage offset to fourth index data set OR
	 * is a raw pointer to the index data store. Actual contents
	 * is iteration-dependent */
	const glw::GLuint*
		m_draw_call_index5_offset; /* Either holds buffer object storage offset to fifth index data set OR
	 * is a raw pointer to the index data store. Actual contents
	 * is iteration-dependent */
	const glw::GLfloat*
		m_draw_call_vertex_offset; /* Either holds buffer object storage offset to first vertex data set OR
	 * is a raw pointer to the first vertex data store. Actual contents
	 * is iteration-dependent */
	const glw::GLfloat*
		m_draw_call_vertex2_offset; /* Either holds buffer object storage offset to second vertex data set OR
	 * is a raw pointer to the second vertex data store. Actual contents
	 * is iteration-dependent */

	_test_cases m_test_cases; /* Holds all test cases */

private:
	/* Private methods */
	void buildProgram(const char* fs_code, const char* vs_code, const char* tc_code, const char* te_code,
					  const char* gs_code);
	void deinitProgramAndShaderObjects();

	/* Private data */
	const unsigned int m_to_height;
	const unsigned int m_to_width;

	unsigned char* m_to_base_data;
	unsigned char* m_to_ref_data;
};

/** Implements functional Tests I, II, III and IV. For clarity, the description
 *  of these functional tests is included below.
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *  Functional Test I:
 *
 *  I.1.   Define vertex coordinates and corresponding indices array (as shown
 *         above).
 *  I.1.a. Render a triangle to texture using glDrawElementsBaseVertexEXT()
 *         call. Use '10' for the 'basevertex' argument value. Use process-side
 *         memory to store vertex coordinates and index data. Assert that
 *         data read from the result texture is the same as for reference texture.
 *  I.1.b. Repeat the test (I.1.a.) for glDrawRangeElementsBaseVertexEXT().
 *  I.1.c. Repeat the test (I.1.a.) for glDrawElementsInstancedBaseVertexEXT():
 *         draw 3 instances of triangle object, compare with a corresponding
 *         texture.
 *  I.1.d. If EXT_multi_draw_arrays extension is supported, repeat the test
 *         (I.1.a.) for glMultiDrawElementsBaseVertexEXT().
 *
 *  Functional Test II:
 *
 *  II.1. Repeat the tests (I.1) using buffer object storing vertex coordinates
 *        attached to GL_ARRAY_BUFFER buffer binding point and client-side
 *        memory to store indices.
 *  II.2. Repeat the tests (I.1) using client-side memory to store vertex
 *        coordinates and a buffer object holding index data attached to
 *        GL_ELEMENT_ARRAY_BUFFER buffer binding point.
 *  II.3. Repeat the tests (I.1) using buffer object storing vertex coordinates
 *        attached to GL_ARRAY_BUFFER buffer binding point and
 *        a buffer object holding index data attached to GL_ELEMENT_ARRAY_BUFFER
 *        buffer binding point.
 *
 *  Functional Test III:
 *
 *   III.1. Repeat the tests (I.1 - II.3) using '0' for the 'basevertex' argument
 *          value. Assert that data read from the result texture differs from
 *          the reference texture.
 *
 *  Functional Test IV:
 *
 *  IV.1.  Repeat the tests (I.1 - II.3) using '10' for 'basevertex'
 *         argument value, but this time use different indices array (values
 *         that should be used for element indices are: 10, 11, 12). Assert
 *         that data read from the result texture differs from the reference
 *         texture.
 *
 *
 **/
class DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior(Context& context, const ExtParameters& extParams);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void setUpTestCases();
};

/** Implements Functional Test V. For clarity, the description is included below:
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *   V.1.   Define new vertex coordinates array so that the triangle coordinates
 *          are placed at the beginning of the array (starting at element of index
 *          0). The rest of the array should be filled with random values, but
 *          must not duplicate the requested triangle coordinates (as presented
 *          in the new_vertices array).
 *   V.1.a. Render a triangle to texture using glDrawElementsBaseVertexEXT()
 *          call. Use '5' for the 'basevertex' argument value. The client-side
 *          memory should be used to store vertex coordinates and client-side
 *          memory to store indices. Assert that data read from the result
 *          texture differs from the reference texture.
 *   V.1.b. Repeat the test (V.1.a.) for glDrawRangeElementsBaseVertexEXT().
 *   V.1.c. Repeat the test (V.1.a.) for glDrawElementsInstancedBaseVertexEXT():
 *          draw 3 instances of triangle object, compare with a corresponding
 *          texture.
 *   V.1.d. If EXT_multi_draw_arrays extension is supported, repeat the test
 *          (V.1.a.) for glMultiDrawElementsBaseVertexEXT().
 *   V.2.   Repeat the tests (V.1) using buffer object holding vertex coordinates
 *          data attached to GL_ARRAY_BUFFER buffer binding point and client-side
 *          memory to store indices.
 *   V.3.   Repeat the tests (V.1) using client-side memory to store vertex
 *          coordinates and a buffer object holding index data attached to
 *          GL_ELEMENT_ARRAY_BUFFER buffer binding point.
 *   V.4.   Repeat the tests (V.1) using buffer object holding vertex coordinates
 *          attached to GL_ARRAY_BUFFER buffer binding point and
 *          a buffer object holding index data attached to GL_ELEMENT_ARRAY_BUFFER
 *          buffer binding point.
 *
 */
class DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior2 : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior2(Context& context, const ExtParameters& extParams);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void setUpTestCases();
};

/** Implements Functional Test VIII. For clarity, the description is included below:
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *  VIII.1. Test whether glDraw*BaseVertexEXT() works correctly when
 *          'basevertex' parameter value is negative (however the condition
 *          "indices[i] + basevertex >= 0" must always be met, otherwise the
 *          behaviour is undefined and should not be tested).
 *          Use vertices array (as declared above) to store vertex coordinates
 *          data: triangle vertex coordinates are declared at the beginning
 *          of the array, the rest of the array is filled with random values.
 *          Index array should store 3 values: 10, 11, 12.
 *  VIII.1.a. Execute glDrawElementsBaseVertexEXT() using '-10' for the
 *           'basevertex' argument value. Use data described in the example.
 *           Use client-side memory to store the index data.
 *           Assert that the result texture does not differ from a reference
 *           texture object (which we get by calling a simple glDrawArrays()
 *           command with the same settings as described for this test case).
 *  VIII.1.b. Repeat the tests (VIII.1.a.) for
 *           glDrawRangeElementsBaseVertexEXT().
 *  VIII.1.c. Repeat the tests (VIII.1.a.) for
 *           glDrawElementsInstancedBaseVertexEXT():
 *           draw 3 instances of triangle object, compare with a corresponding
 *           texture.
 *  VIII.1.d. If EXT_multi_draw_arrays extension is supported, repeat the test
 *           (VIII.1.a.) for glMultiDrawElementsBaseVertexEXT().
 *  VIII.2.  Repeat the tests (VIII.1) using a buffer object holding index data
 *           attached to GL_ELEMENT_ARRAY_BUFFER buffer binding point.
 *
 */
class DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorUnderflow : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorUnderflow(Context&				context,
																	   const ExtParameters& extParams);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void setUpTestCases();
};

/** Implements Functional Test IX. For clarity, the description is included below:
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *  IX.1.   Test whether glDraw*BaseVertexEXT() works correctly when
 *          ("indices[i]" + "basevertex") is larger than the maximum value
 *          representable by "type".
 *          Use vertices array (as declared above) to store vertex coordinates
 *          data: triangle vertex coordinates are declared at the beginning
 *          of the array, the rest of the array is filled with random values.
 *          Index array should store 3 values: 0, 1, 2.
 *          Use "basevertex" argument value that equals to (maximum value
 *          representable by a type + 1).
 *  IX.1.a. Execute glDrawElementsBaseVertexEXT() using 'basevertex' argument
 *          that equals to (maximum value representable by a type + 1).
 *          Use data described in the example. Use client-side memory to store
 *          the index data.
 *          Assert that the result texture does not differ from a reference
 *          texture object (which we get by calling a simple glDrawArrays()
 *          command with the same settings as described for this test case).
 *          The test should be executed for all of the types listed below:
 *              - GL_UNSIGNED_BYTE,
 *              - GL_UNSIGNED_SHORT.
 *          GL_UNSIGNED_INT is excluded from the test.
 *  IX.1.b. Repeat the tests (IX.1.a.) for
 *          glDrawRangeElementsBaseVertexEXT(). "start" and "end" should be
 *          equal to 0 and 3 accordingly.
 *  IX.1.c. Repeat the tests (IX.1.a.) for
 *          glDrawElementsInstancedBaseVertexEXT():
 *          draw 3 instances of triangle object, compare with a corresponding
 *          texture.
 *  IX.1.d. If EXT_multi_draw_arrays extension is supported, repeat the test
 *          (IX.1.a.) for glMultiDrawElementsBaseVertexEXT().
 *  IX.2.   Repeat the tests (IX.1) using a buffer object holding index data
 *          attached to GL_ELEMENT_ARRAY_BUFFER buffer binding point.
 *  IX.3.   Repeat the above tests (IX.1 - IX.2), but this time indices array
 *          should store 4 values: 0, 1, 2, 3. 'basevertex' argument should be
 *          equal to (maximum value representable by a type + 2).
 *          Assert that the result texture differs from a reference one.
 *
 */
class DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorOverflow : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorOverflow(Context& context, const ExtParameters& extParams);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void setUpTestCases();
};

/** Implements Functional Test VI. For clarity, the description is included below:
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *   VI.1.   If EXT_tessellation_shader extension is supported, test whether
 *           the tessellation works correctly when glDraw*BaseVertexEXT()
 *           entry-points are used.
 *           Draw a single patch consisting of 3 patch vertices. In Tessellation
 *           Control shader, set gl_TessLevelInner[0], gl_TessLevelOuter[1],
 *           gl_TessLevelOuter[2] to 3.
 *           In the Tessellation Evaluation shader, define
 *           layout(triangles, equal_spacing, cw) in;
 *           Coordinates generated by the tessellator should be stored in gl_Position,
 *           after having been adjusted to completely fit the screen space.
 *           Set different color for each vertex. As a result we expect
 *           a multi-colored triangle to be drawn into the result texture.
 *   VI.1.a. Execute glDrawElementsBaseVertexEXT() using '10' for the 'basevertex'
 *           argument value. Use data described in the example. Use client-side
 *           memory to store vertex coordinates and client-side memory to store
 *           indices. Assert that the result texture does not differ from reference
 *           texture (which we get by calling a simple glDrawArrays() command with
 *           the same settings as described for this test case).
 *   VI.1.b. Repeat the tests (VI.1.a.) for
 *           glDrawRangeElementsBaseVertexEXT().
 *   VI.1.c. Repeat the tests (VI.1.a.) for
 *           glDrawElementsInstancedBaseVertexEXT():
 *           draw 3 instances of triangle object, compare with a corresponding
 *           texture.
 *   VI.1.d. If EXT_multi_draw_arrays extension is supported, repeat the test
 *           (VI.1.a.) for glMultiDrawElementsBaseVertexEXT().
 *   VI.2.   Repeat the tests (VI.1) using buffer object holding vertex
 *           coordinates attached to GL_ARRAY_BUFFER buffer binding point and
 *           client-side memory to store indices.
 *   VI.3.   Repeat the tests (VI.1) using client-side memory to store vertex
 *           coordinates and a buffer object holding index data attached to
 *           GL_ELEMENT_ARRAY_BUFFER buffer binding point.
 *   VI.4.   Repeat the tests (VI.1) using buffer object holding vertex
 *           coordinates attached to GL_ARRAY_BUFFER buffer binding point and
 *           a buffer object holding index data attached to
 *           GL_ELEMENT_ARRAY_BUFFER buffer binding point.
 *   VI.5.   Repeat the tests (VI.1 - VI.4) using '0' for the 'basevertex'
 *           argument value. Assert that data read from the result texture
 *           differs from the reference texture.
 *   VI.6    Add Geometry Shader Execution step before the fragment operation
 *           are handled. The shader should output input triangles, with negated
 *           y component. Repeat tests (VI.1 - VI.5).
 *           The new reference texture should be prepared (a multi-colored triangle
 *           rendered based on the assumption described in the test case, with
 *           a simple glDrawArrays() call).
 *
 */
class DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorAEPShaderStages : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorAEPShaderStages(Context&			  context,
																			 const ExtParameters& extParams);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void setUpTestCases();
};

/** Implements Negative Test IV. For clarity, the description is included below:
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *   IV.1.   Check if proper error code is generated when transform feedback is
 *           active and not paused.
 *   IV.1.a. Generate, bind and begin transform feedback prepared to record
 *           GL_TRIANGLES type of primitives. Execute glDrawElementsBaseVertexEXT()
 *           as shown in the description. Expect GL_INVALID_OPERATION to be
 *           generated.
 *   IV.1.b. Repeat the test (IV.1.a.) for glDrawRangeElementsBaseVertexEXT().
 *   IV.1.c. Repeat the test (IV.1.a.) for glDrawElementsInstancedBaseVertexEXT().
 *
 */
class DrawElementsBaseVertexNegativeActiveTransformFeedbackTest : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexNegativeActiveTransformFeedbackTest(Context& context, const ExtParameters& extParams);

	virtual void						 deinit();
	virtual void						 init();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint m_bo_tf_result_id;

	/* Private methods */
	virtual void deinitPerTestObjects();
};

/** Implements Negative Test I.2 and II.1, II.2, II.3 (for the original test).
 *  For clarity, the description is included below:
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *  I.2.   Check if proper error code is generated when 'count' argument is
 *         invalid.
 *  I.2.a. Prepare the environment as described above. Execute
 *         glDrawElementsBaseVertexEXT() using '-1' as 'count' argument value.
 *         Expect GL_INVALID_VALUE error to be generated.
 *  I.2.b. Repeat the test (I.2.a.) for glDrawRangeElementsBaseVertexEXT().
 *  I.2.c. Repeat the test (I.2.a.) for glDrawElementsInstancedBaseVertexEXT().
 *  I.2.d. If EXT_multi_draw_arrays extension is supported, repeat the test
 *         (I.2.a.) for glMultiDrawElementsBaseVertexEXT().
 *
 *  II.1. Repeat the tests (I.1 - I.6) using client-side memory to store indices
 *        and buffer object storing vertex coordinates attached to
 *        GL_ARRAY_BUFFER buffer binding point.
 *
 *  II.2. Repeat the tests (I.1 - I.6) when a buffer object holding index data
 *        is attached to GL_ELEMENT_ARRAY_BUFFER buffer binding point and
 *        client-side memory is used to store vertex coordinates. Replace
 *        value of 'indices' argument from the functions described above
 *        with NULL.
 *
 *  II.3. Repeat the tests (I.1 - I.6) when a buffer object storing index data
 *        is attached to GL_ELEMENT_ARRAY_BUFFER buffer binding point and
 *        buffer object storing vertex coordinates data is attached to
 *        GL_ARRAY_BUFFER buffer binding point.
 *        Replace value of 'indices' argument from the functions described
 *        above with NULL.
 */
class DrawElementsBaseVertexNegativeInvalidCountArgumentTest : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexNegativeInvalidCountArgumentTest(Context& context, const ExtParameters& extParams);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Implements Negative Test I.6 and II.1, II.2, II.3 (for the original test).
 *  For clarity, the description for I.6 is included below. II.1, II.2 and II.3
 *  have already been cited above.
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *  I.6    Check if proper error code is generated when 'instancecount' argument
 *       value is invalid for glDrawElementsInstancedBaseVertexEXT() call.
 *       Prepare the environment as described above. The client-side memory
 *       should be used in the draw calls to store vertex coordinates and
 *       indices data. Execute glDrawElementsInstancedBaseVertexEXT() using
 *       '-1' for 'instancecount' argument value. Expect GL_INVALID_VALUE
 *       to be generated.
 *
 */
class DrawElementsBaseVertexNegativeInvalidInstanceCountArgumentTest : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexNegativeInvalidInstanceCountArgumentTest(Context& context, const ExtParameters& extParams);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Implements Negative Test I.1 and II.1, II.2, II.3 (for the original test).
 *  For clarity, the description for I.1 is included below. II.1, II.2 and II.3
 *  have already been cited above.
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *  I.1.   Check if proper error code is generated when 'mode' argument is
 *         invalid. The client-side memory should be used in the draw calls to
 *         store vertex coordinates and indices data.
 *  I.1.a. Prepare the environment as described above. Execute
 *         glDrawElementsBaseVertexEXT() using GL_NONE for 'mode' argument
 *         value. Expect GL_INVALID_ENUM error to be generated.
 *  I.1.b. Repeat the test (I.1.a.) for glDrawRangeElementsBaseVertexEXT().
 *  I.1.c. Repeat the test (I.1.a.) for glDrawElementsInstancedBaseVertexEXT().
 *  I.1.d. If EXT_multi_draw_arrays extension is supported, repeat the test
 *         (I.1.a.) for glMultiDrawElementsBaseVertexEXT().
 *
 */
class DrawElementsBaseVertexNegativeInvalidModeArgumentTest : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexNegativeInvalidModeArgumentTest(Context& context, const ExtParameters& extParams);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Implements Negative Test I.4 and II.1, II.2, II.3 (for the original test).
 *  For clarity, the description for I.4 is included below. II.1, II.2 and II.3
 *  have already been cited above.
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *  I.4    Check if proper error code is generated when 'primcount' argument is
 *         invalid. If EXT_multi_draw_arrays extension is supported, prepare
 *         the environment as described above, execute
 *         glMultiDrawElementsBaseVertexEXT() with negative 'primcount' value.
 *         The client-side memory should be used in the draw calls to store
 *         vertex coordinates and indices data. Expect GL_INVALID_VALUE error
 *         to be generated.
 *
 */
class DrawElementsBaseVertexNegativeInvalidPrimcountArgumentTest : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexNegativeInvalidPrimcountArgumentTest(Context& context, const ExtParameters& extParams);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Implements Negative Test I.5 and II.1, II.2, II.3 (for the original test).
 *  For clarity, the description for I.5 is included below. II.1, II.2 and II.3
 *  have already been cited above.
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *  I.5    Check if proper error code is generated when 'start' and 'end'
 *         arguments combination is invalid for glDrawRangeElementsBaseVertexEXT()
 *         call. Prepare the environment as described above. Execute the
 *         glDrawRangeElementsBaseVertexEXT() using '3' for 'start' and '0'
 *         for 'end' argument values. The client-side memory should be used in the
 *         draw calls to store vertex coordinates and indices data. Expect
 *         GL_INVALID_VALUE to be generated.
 *
 */
class DrawElementsBaseVertexNegativeInvalidStartEndArgumentsTest : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexNegativeInvalidStartEndArgumentsTest(Context& context, const ExtParameters& extParams);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Implements Negative Test I.3 and II.1, II.2, II.3 (for the original test).
 *  For clarity, the description for I.3 is included below. II.1, II.2 and II.3
 *  have already been cited above.
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *  I.3.   Check if proper error code is generated when 'type' argument is
 *         invalid. The client-side memory should be used in the draw calls to
 *         store vertex coordinates and indices data.
 *  I.3.a. Prepare the environment as described above. Execute
 *         glDrawElementsBaseVertexEXT() using 'GL_NONE' as 'type' argument
 *         value. Expect GL_INVALID_ENUM error to be generated.
 *  I.3.b. Repeat the test (I.3.a.) for glDrawRangeElementsBaseVertexEXT().
 *  I.3.c. Repeat the test (I.3.a.) for glDrawElementsInstancedBaseVertexEXT().
 *  I.3.d. If EXT_multi_draw_arrays extension is supported, repeat the test
 *         (I.3.a.) for glMultiDrawElementsBaseVertexEXT().
 *
 */
class DrawElementsBaseVertexNegativeInvalidTypeArgumentTest : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexNegativeInvalidTypeArgumentTest(Context& context, const ExtParameters& extParams);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Implements Negative Test III. For clarity, the description is included below:
 *
 *  (note: the description below refers to ES entry-points, but these are
 *         replaced with GL equivalents under GL contexts)
 *
 *  III.1.   Check if proper error code is generated when element array buffer
 *           object is mapped for the draw call.
 *  III.1.a. Call glMapBufferRange() to map the element array buffer object
 *           that stores vertex indices used in the draw call. Execute
 *           glDrawElementsBaseVertexEXT() as shown in the description using
 *           element array buffer object as a vertex indices data source
 *           (the buffer object which is currently mapped). The client-side
 *           memory should be used to store vertex coordinates. Expect
 *           GL_INVALID_OPERATION to be generated.
 *  III.1.b. Repeat the test (III.1.a.) for glDrawRangeElementsBaseVertexEXT().
 *  III.1.c. Repeat the test (III.1.a.) for glDrawElementsInstancedBaseVertexEXT().
 *  III.1.d. Repeat the test (III.1.a - III.1.c) with buffer object holding
 *           vertex coordinates attached to GL_ARRAY_BUFFER buffer binding point.
 *
 */
class DrawElementsBaseVertexNegativeMappedBufferObjectsTest : public DrawElementsBaseVertexTestBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexNegativeMappedBufferObjectsTest(Context& context, const ExtParameters& extParams);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Test group which encapsulates all conformance tests for "draw elements base vertex"
 *  functionality.
 */
class DrawElementsBaseVertexTests : public glcts::TestCaseGroupBase
{
public:
	/* Public methods */
	DrawElementsBaseVertexTests(glcts::Context& context, const ExtParameters& extParams);

	void init(void);

private:
	DrawElementsBaseVertexTests(const DrawElementsBaseVertexTests& other);
	DrawElementsBaseVertexTests& operator=(const DrawElementsBaseVertexTests& other);
};

} /* glcts namespace */

#endif // _ESEXTCDRAWELEMENTSBASEVERTEXTESTS_HPP
