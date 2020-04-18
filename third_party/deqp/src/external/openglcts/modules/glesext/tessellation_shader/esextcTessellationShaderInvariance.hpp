#ifndef _ESEXTCTESSELLATIONSHADERINVARIANCE_HPP
#define _ESEXTCTESSELLATIONSHADERINVARIANCE_HPP
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
#include "glwEnums.hpp"
#include "tcuDefs.hpp"

namespace glcts
{

/** A DEQP CTS test group that collects all tests that verify invariance
 *  conformance.
 */
class TessellationShaderInvarianceTests : public glcts::TestCaseGroupBase
{
public:
	/* Public methods */
	TessellationShaderInvarianceTests(glcts::Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderInvarianceTests(void)
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	TessellationShaderInvarianceTests(const TessellationShaderInvarianceTests& other);
	TessellationShaderInvarianceTests& operator=(const TessellationShaderInvarianceTests& other);
};

/** Base class that provides shared invariance test implementation. Invariance
 *  rule test need only to implement the abstract methods.
 **/
class TessellationShaderInvarianceBaseTest : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderInvarianceBaseTest(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~TessellationShaderInvarianceBaseTest(void)
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	/* Protected variables */
	TessellationShaderUtils* m_utils_ptr;

	virtual void executeDrawCall(unsigned int n_iteration);
	virtual unsigned int getAmountOfIterations() = 0;
	virtual unsigned int getDrawCallCountArgument();
	virtual std::string getFSCode(unsigned int n_iteration);
	virtual const char* getInnerTessLevelUniformName();
	virtual const char* getOuterTessLevelUniformName();

	virtual void getIterationProperties(unsigned int n_iteration, float* out_inner_tess_levels,
										float* out_outer_tess_levels, bool* out_point_mode,
										_tessellation_primitive_mode*		  out_primitive_mode,
										_tessellation_shader_vertex_ordering* out_vertex_ordering,
										unsigned int*						  out_result_buffer_size) = 0;

	virtual std::string getTCCode(unsigned int n_iteration);
	virtual std::string getTECode(unsigned int n_iteration) = 0;
	virtual std::string getVSCode(unsigned int n_iteration);

	virtual void getXFBProperties(unsigned int n_iteration, unsigned int* out_n_names, const char*** out_names);

	virtual void verifyResultDataForIteration(unsigned int n_iteration, const void* data);

	virtual void verifyResultData(const void** all_iterations_data);

private:
	/* Private type definitions */

	/* Private methods */
	void initTest();

	/* Private variables */
	typedef struct _test_program
	{
		glw::GLuint po_id;

		glw::GLuint inner_tess_level_uniform_location;
		glw::GLuint outer_tess_level_uniform_location;

	} _test_program;

	/* Defines a vector of program objects. Index corresponds to iteration index */
	typedef std::vector<_test_program> _programs;
	typedef _programs::const_iterator  _programs_const_iterator;
	typedef _programs::iterator		   _programs_iterator;

	glw::GLuint m_bo_id;
	_programs   m_programs;
	glw::GLuint m_qo_tfpw_id;
	glw::GLuint m_vao_id;
};

/** Implementation of Test Case 42
 *
 *  Make sure that invariance rule 1 is adhered to. Using a program object
 *  consisting of a fragment/tessellation control/tessellation evaluation/
 *  vertex shaders, render three points/lines/triangles (A, B, C) and
 *  store vertices output by the tessellation evaluation shader.  Then render
 *  the geometry in (B, C, A) order, using the same program object. Test
 *  passes if vertices stored in two different iterations for the same
 *  triangle are identical. Owing to rule 8, assume zero epsilon.
 **/
class TessellationShaderInvarianceRule1Test : public TessellationShaderInvarianceBaseTest
{
public:
	/* Public methods */
	TessellationShaderInvarianceRule1Test(Context& context, const ExtParameters& extParams);
	virtual ~TessellationShaderInvarianceRule1Test();

protected:
	/* Protected methods */
	unsigned int getAmountOfIterations();
	unsigned int getDrawCallCountArgument();

	void getIterationProperties(unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels,
								bool* out_point_mode, _tessellation_primitive_mode* out_primitive_mode,
								_tessellation_shader_vertex_ordering* out_vertex_ordering,
								unsigned int*						  out_result_buffer_size);

	std::string getTECode(unsigned int n_iteration);
	void verifyResultData(const void** all_iterations_data);
};

/** Implementation of Test Case 43
 *
 *  Make sure that invariance rule 2 is adhered to. Using a program object
 *  consisting of a fragment/tessellation control/tessellation evaluation/
 *  vertex shaders, render a number of full-screen triangles/quads, each
 *  instance rendered with different inner tessellation level but identical
 *  outer tessellation level and spacing input layout qualifiers. Test passes
 *  if outer edge's vertices are the same for both types of geometry
 *  (each type considered separately).
 **/
class TessellationShaderInvarianceRule2Test : public TessellationShaderInvarianceBaseTest
{
public:
	/* Public methods */
	TessellationShaderInvarianceRule2Test(Context& context, const ExtParameters& extParams);
	virtual ~TessellationShaderInvarianceRule2Test();

protected:
	/* Protected methods */
	unsigned int getAmountOfIterations();

	void getIterationProperties(unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels,
								bool* out_point_mode, _tessellation_primitive_mode* out_primitive_mode,
								_tessellation_shader_vertex_ordering* out_vertex_ordering,
								unsigned int*						  out_result_buffer_size);

	std::string getTECode(unsigned int n_iteration);
	void verifyResultData(const void** all_iterations_data);

private:
	/* Private variables */
	unsigned int m_n_tessellated_vertices[4 /* iterations in total */];
};

/** Implementation of Test Case 44
 *
 *  Make sure that invariance rule 3 is adhered to. Using a program object
 *  consisting of a fragment/tessellation control/tessellation evaluation/
 *  vertex shaders, tessellate a number of triangles/quads/isolines geometry
 *  with different inner/outer/vertex spacing input layout qualifiers.
 *  Capture vertices generated by tessellation evaluation stage and make sure
 *  that generated vertices are symmetrical. Owing to rule 8, assume zero
 *  epsilon.
 **/
class TessellationShaderInvarianceRule3Test : public TessellationShaderInvarianceBaseTest
{
public:
	/* Public methods */
	TessellationShaderInvarianceRule3Test(Context& context, const ExtParameters& extParams);
	virtual ~TessellationShaderInvarianceRule3Test();

protected:
	/* Protected methods */
	unsigned int getAmountOfIterations();

	void getIterationProperties(unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels,
								bool* out_point_mode, _tessellation_primitive_mode* out_primitive_mode,
								_tessellation_shader_vertex_ordering* out_vertex_ordering,
								unsigned int*						  out_result_buffer_size);

	std::string getTECode(unsigned int n_iteration);

	void verifyResultDataForIteration(unsigned int n_iteration, const void* data);

private:
	/* Private type definitions */
	typedef struct _test_iteration
	{
		glw::GLfloat						inner_tess_levels[2];
		glw::GLfloat						outer_tess_levels[4];
		_tessellation_primitive_mode		primitive_mode;
		_tessellation_shader_vertex_spacing vertex_spacing;

		unsigned int n_vertices;

		_test_iteration()
		{
			memset(inner_tess_levels, 0, sizeof(inner_tess_levels));
			memset(outer_tess_levels, 0, sizeof(outer_tess_levels));

			primitive_mode = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
			vertex_spacing = TESSELLATION_SHADER_VERTEX_SPACING_UNKNOWN;

			n_vertices = 0;
		}
	} _test_iteration;

	typedef std::vector<_test_iteration>	 _test_iterations;
	typedef _test_iterations::const_iterator _test_iterations_const_iterator;

	/* Private methods */
	void deinitTestIterations();
	void initTestIterations();

	/* Private fields */
	_test_iterations m_test_iterations;
};

/** Implementation of Test Case 45
 *
 *  Make sure that invariance rule 4 is adhered to. Using a program object
 *  consisting of a fragment/tessellation control/tessellation evaluation/
 *  vertex shaders, tessellate a number of triangular and quad geometry with
 *  different inner tessellation level input layout qualifiers.
 *  Capture vertices generated by tessellation evaluation stage and make sure
 *  that all sets of vertices generated when subdividing outer edges are
 *  independent of the specific edge subdivided.
 *
 *  Technical details:
 *
 *  1. The test should use a number of different inner+outer
 *     tessellation levels+vertex spacing mode configuration
 *     combinations, each resulting in a different vertex set for
 *     the generator primitive type considered.
 *     In first iteration, it should draw a screen quad, and
 *     in the other a triangle should be rendered.
 *  2. The test should capture vertices output in TE stage. The
 *     rasterizer discard mode can be enabled, as the test is not
 *     expected to analyse visual output.
 *  3. For quad tessellation, the test should identify vertices
 *     generated for top outer edge and make sure that remaining
 *     outer edges of the quad are built of vertices that conform
 *     to the rule.
 *  4. For triangular tessellation, the test should identify vertices
 *     generated for one of the outer edges and then check if the other
 *     two outer edges have been generated in conformance to the rule.
 **/
class TessellationShaderInvarianceRule4Test : public TessellationShaderInvarianceBaseTest
{
public:
	/* Public methods */
	TessellationShaderInvarianceRule4Test(Context& context, const ExtParameters& extParams);
	virtual ~TessellationShaderInvarianceRule4Test();

protected:
	/* Protected methods */
	unsigned int getAmountOfIterations();

	void getIterationProperties(unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels,
								bool* out_point_mode, _tessellation_primitive_mode* out_primitive_mode,
								_tessellation_shader_vertex_ordering* out_vertex_ordering,
								unsigned int*						  out_result_buffer_size);

	std::string getTECode(unsigned int n_iteration);

	void verifyResultDataForIteration(unsigned int n_iteration, const void* data);

private:
	/* Private type definitions */
	typedef struct _test_iteration
	{
		glw::GLfloat						inner_tess_levels[2];
		glw::GLfloat						outer_tess_levels[4];
		_tessellation_primitive_mode		primitive_mode;
		_tessellation_shader_vertex_spacing vertex_spacing;

		unsigned int n_vertices;

		_test_iteration()
		{
			memset(inner_tess_levels, 0, sizeof(inner_tess_levels));
			memset(outer_tess_levels, 0, sizeof(outer_tess_levels));

			primitive_mode = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
			vertex_spacing = TESSELLATION_SHADER_VERTEX_SPACING_UNKNOWN;

			n_vertices = 0;
		}
	} _test_iteration;

	typedef std::vector<_test_iteration>	 _test_iterations;
	typedef _test_iterations::const_iterator _test_iterations_const_iterator;

	/* Private methods */
	void deinitTestIterations();
	void initTestIterations();

	bool isVertexDefined(const float* vertex_data, unsigned int n_vertices, const float* vertex_data_seeked,
						 unsigned int n_vertex_data_seeked_components);

	/* Private fields */
	_test_iterations m_test_iterations;
};

/** Implementation of Test Case 46
 *
 *  Make sure that Rule 5 is adhered to. Using a program object
 *  consisting of a fragment/tessellation control/tessellation evaluation/
 *  vertex shaders, tessellate a number of triangles/quads/isolines
 *  geometry with different vertex ordering input layout qualifiers. Capture
 *  vertices generated by tessellation evaluation stage and make sure that each
 *  iteration defines exactly the same set of vertices, although in different
 *  order.
 *
 **/
class TessellationShaderInvarianceRule5Test : public TessellationShaderInvarianceBaseTest
{
public:
	/* Public methods */
	TessellationShaderInvarianceRule5Test(Context& context, const ExtParameters& extParams);
	virtual ~TessellationShaderInvarianceRule5Test();

protected:
	/* Protected methods */
	unsigned int getAmountOfIterations();

	void getIterationProperties(unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels,
								bool* out_point_mode, _tessellation_primitive_mode* out_primitive_mode,
								_tessellation_shader_vertex_ordering* out_vertex_ordering,
								unsigned int*						  out_result_buffer_size);

	std::string getTECode(unsigned int n_iteration);
	void verifyResultData(const void** all_iterations_data);

private:
	/* Private type definitions */
	typedef struct _test_iteration
	{
		glw::GLfloat						 inner_tess_levels[2];
		glw::GLfloat						 outer_tess_levels[4];
		_tessellation_primitive_mode		 primitive_mode;
		_tessellation_shader_vertex_ordering vertex_ordering;

		unsigned int n_vertices;

		_test_iteration()
		{
			memset(inner_tess_levels, 0, sizeof(inner_tess_levels));
			memset(outer_tess_levels, 0, sizeof(outer_tess_levels));

			primitive_mode  = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
			vertex_ordering = TESSELLATION_SHADER_VERTEX_ORDERING_UNKNOWN;

			n_vertices = 0;
		}
	} _test_iteration;

	typedef std::vector<_test_iteration>	 _test_iterations;
	typedef _test_iterations::const_iterator _test_iterations_const_iterator;

	/* Private methods */
	void			 deinitTestIterations();
	_test_iteration& getTestForIteration(unsigned int n_iteration);
	void initTestIterations();
	bool isVertexDefined(const float* vertex_data, unsigned int n_vertices, const float* vertex_data_seeked,
						 unsigned int n_vertex_data_seeked_components);

	/* Private fields */
	_test_iterations m_test_triangles_iterations;
	_test_iterations m_test_quads_iterations;
};

/**  Implementation of Test Case 47
 *
 *   Make sure that invariance rule 6 is adhered to. Using a program object
 *   consisting of a fragment/tessellation control/tessellation evaluation/
 *   vertex shaders, tessellate a number of triangles/quads geometry
 *   with different inner tessellation levels/vertex spacing input layout qualifiers.
 *   Capture vertices generated by tessellation evaluation stage and make sure
 *   that all interior triangles generated during tessellation are identical
 *   except for vertex and triangle order.
 **/
class TessellationShaderInvarianceRule6Test : public TessellationShaderInvarianceBaseTest
{
public:
	/* Public methods */
	TessellationShaderInvarianceRule6Test(Context& context, const ExtParameters& extParams);
	virtual ~TessellationShaderInvarianceRule6Test();

protected:
	/* Protected methods */
	unsigned int getAmountOfIterations();

	void getIterationProperties(unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels,
								bool* out_point_mode, _tessellation_primitive_mode* out_primitive_mode,
								_tessellation_shader_vertex_ordering* out_vertex_ordering,
								unsigned int*						  out_result_buffer_size);

	std::string getTECode(unsigned int n_iteration);
	void verifyResultData(const void** all_iterations_data);

private:
	/* Private type definitions */
	typedef struct _test_iteration
	{
		glw::GLfloat						 inner_tess_levels[2];
		glw::GLfloat						 outer_tess_levels[4];
		_tessellation_primitive_mode		 primitive_mode;
		_tessellation_shader_vertex_ordering vertex_ordering;

		unsigned int n_vertices;

		_test_iteration()
		{
			memset(inner_tess_levels, 0, sizeof(inner_tess_levels));
			memset(outer_tess_levels, 0, sizeof(outer_tess_levels));

			primitive_mode  = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
			vertex_ordering = TESSELLATION_SHADER_VERTEX_ORDERING_UNKNOWN;

			n_vertices = 0;
		}
	} _test_iteration;

	typedef std::vector<_test_iteration>	 _test_iterations;
	typedef _test_iterations::const_iterator _test_iterations_const_iterator;

	/* Private methods */
	void			 deinitTestIterations();
	_test_iteration& getTestForIteration(unsigned int n_iteration);
	void initTestIterations();

	/* Private fields */
	_test_iterations m_test_triangles_iterations;
	_test_iterations m_test_quads_iterations;
};

/**  Implementation of Test Case 48
 *
 *   Make sure that invariance rule 7 is adhered to. Using a program object
 *   consisting of a fragment/tessellation control/tessellation evaluation/
 *   vertex shaders, tessellate a number of triangles/quads geometry
 *   with different vertex spacing input layout qualifiers. For each such
 *   case, the test should verify that modification of a single outer tessellation
 *   level only affects tessellation coordinates generated for a corresponding
 *   edge. Verification should be carried out by capturing vertices generated for
 *   tessellation evaluation stage and making sure that each iteration defines
 *   exactly the same set of triangles connecting inner and outer edge of the
 *   tessellated geometry for all but the modified edge.
 **/
class TessellationShaderInvarianceRule7Test : public TessellationShaderInvarianceBaseTest
{
public:
	/* Public methods */
	TessellationShaderInvarianceRule7Test(Context& context, const ExtParameters& extParams);
	virtual ~TessellationShaderInvarianceRule7Test();

protected:
	/* Protected methods */
	unsigned int getAmountOfIterations();

	void getIterationProperties(unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels,
								bool* out_point_mode, _tessellation_primitive_mode* out_primitive_mode,
								_tessellation_shader_vertex_ordering* out_vertex_ordering,
								unsigned int*						  out_result_buffer_size);

	std::string getTECode(unsigned int n_iteration);
	void verifyResultData(const void** all_iterations_data);

private:
	/* Private type definitions */
	typedef struct _test_iteration
	{
		glw::GLfloat						 inner_tess_levels[2];
		glw::GLfloat						 outer_tess_levels[4];
		_tessellation_primitive_mode		 primitive_mode;
		_tessellation_shader_vertex_ordering vertex_ordering;

		bool		 is_base_iteration;
		unsigned int n_modified_outer_tess_level;
		unsigned int n_vertices;

		_test_iteration()
		{
			memset(inner_tess_levels, 0, sizeof(inner_tess_levels));
			memset(outer_tess_levels, 0, sizeof(outer_tess_levels));

			primitive_mode  = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
			vertex_ordering = TESSELLATION_SHADER_VERTEX_ORDERING_UNKNOWN;

			is_base_iteration			= false;
			n_modified_outer_tess_level = 0;
			n_vertices					= 0;
		}
	} _test_iteration;

	typedef std::vector<_test_iteration>	 _test_iterations;
	typedef _test_iterations::const_iterator _test_iterations_const_iterator;

	/* Private methods */
	void deinitTestIterations();

	unsigned int getTestIterationIndex(bool is_triangles_iteration, const float* inner_tess_levels,
									   const float*							outer_tess_levels,
									   _tessellation_shader_vertex_ordering vertex_ordering,
									   unsigned int							n_modified_outer_tess_level);

	_test_iteration& getTestForIteration(unsigned int n_iteration);
	void initTestIterations();

	bool isTriangleDefinedInVertexDataSet(const float* base_triangle_data, const float* vertex_data,
										  unsigned int vertex_data_n_vertices);

	bool isVertexDefined(const float* vertex_data, unsigned int n_vertices, const float* vertex_data_seeked,
						 unsigned int n_vertex_data_seeked_components);

	/* Private fields */
	_test_iterations m_test_triangles_iterations;
	_test_iterations m_test_quads_iterations;
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERINVARIANCE_HPP
