#ifndef _GL3CCULLDISTANCETESTS_HPP
#define _GL3CCULLDISTANCETESTS_HPP
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

/**
 */ /*!
 * \file  gl3cCullDistanceTests.hpp
 * \brief  Cull Distance Test Suite Interface
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

namespace glcts
{
namespace CullDistance
{
/** @brief Cull Distance Test utility class
 *
 *  This class contains utility static function members
 *  helpful to OpenGL shader template based construction
 *  and building process.
 */
class Utilities
{
public:
	/* Public static methods */
	static void buildProgram(const glw::Functions& gl, tcu::TestContext& testCtx, const glw::GLchar* cs_body,
							 const glw::GLchar* fs_body, const glw::GLchar* gs_body, const glw::GLchar* tc_body,
							 const glw::GLchar* te_body, const glw::GLchar* vs_body, const glw::GLuint& n_tf_varyings,
							 const glw::GLchar** tf_varyings, glw::GLuint* out_program);

	static void replaceAll(std::string& str, const std::string& from, const std::string& to);

	static std::string intToString(glw::GLint integer);
};

/** @brief Cull Distance API Coverage Test class
 *
 *  This class contains basic API coverage test,
 *  which check if the implementation provides
 *  basic cull distance structures:
 *
 *   * Checks that calling GetIntegerv with MAX_CULL_DISTANCES doesn't generate
 *    any errors and returns a value at least 8.
 *
 *   * Checks that calling GetIntegerv with MAX_COMBINED_CLIP_AND_CULL_DISTANCES
 *     doesn't generate any errors and returns a value at least 8.
 *
 *   * Checks that using the GLSL built-in constant gl_MaxCullDistance in any
 *     shader stage (including compute shader) compiles and links successfully
 *     and that the value of the built-in constant is at least 8.
 *
 *   * Checks that using the GLSL built-in constant gl_MaxCombinedClipAndCull-
 *     Distances in any shader stage (including compute shader) compiles and
 *     links successfully and that the value of the built-in constant is at
 *     least 8.
 */
class APICoverageTest : public deqp::TestCase
{
public:
	/* Public methods */
	APICoverageTest(deqp::Context& context);

protected:
	/* Protected methods */
	void						 deinit();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private fields */
	glw::GLuint m_bo_id;
	glw::GLuint m_cs_id;
	glw::GLuint m_cs_to_id;
	glw::GLuint m_fbo_draw_id;
	glw::GLuint m_fbo_draw_to_id;
	glw::GLuint m_fbo_read_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

/** @brief Cull Distance Functional Test class
 *
 *  This class contains functional test cases,
 *  which check if the implementation works
 *  in specified way. For each functional test:
 *    * Use the basic outline to test the basic functionality of cull distances.
 *    * Use the basic outline but don't redeclare gl_ClipDistance with a size.
 *    * Use the basic outline but don't redeclare gl_CullDistance with a size.
 *    * Use the basic outline but don't redeclare either gl_ClipDistance or
 *      gl_CullDistance with a size.
 *    * Use the basic outline but use dynamic indexing when writing the elements
 *      of the gl_ClipDistance and gl_CullDistance arrays.
 *    * Use the basic outline but add a geometry shader to the program that
 *      simply passes through all written clip and cull distances.
 *    * Use the basic outline but add a tessellation control and tessellation
 *      evaluation shader to the program which simply pass through all written
 *      clip and cull distances.
 *    * Test that using #extension with GL_ARB_cull_distance allows using the
 *      feature even with an earlier version of GLSL. Also test that the
 *      extension name is available as preprocessor #define.
 *  a basic outline is used to check the implementation:
 *    * Enable disjunct cull distances using Enable with CLIP_DISTANCE<i>.
 *    * Use a program that has only a vertex shader and a fragment shader.
 *      The vertex shader should redeclare gl_ClipDistance with a size that
 *      fits all enabled cull distances. Also redeclare gl_CullDistance with a
 *      size. The sum of the two sizes should not be more than MAX_COMBINED_-
 *      CLIP_AND_CULL_DISTANCES. The fragment shader should output the cull
 *      distances written by the vertex shader by reading them from the built-in
 *      array gl_CullDistance.
 *    * Write different positive and negative values for all the enabled clip
 *      distances to gl_ClipDistance in the vertex shader. Also write different
 *      positive and negative values for all the elements of gl_CullDistance.
 *      Use constant indices when writing to gl_ClipDistance and gl_CullDistance.
 *    * Render point, line and triangle primitives. Expect primitives that for
 *      a given index <i> all of their vertices have a negative value set for
 *      gl_CullDistance[i] to be discarded. Otherwise, they should be clipped
 *      according to the enabled clip distances as without this extension.
 *      Check the output image to make sure that the color output for each
 *      fragment matches the expected interpolated values of the written cull
 *      distances.
 * */
class FunctionalTest : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest(deqp::Context& context);

protected:
	/* Protected methods */
	void						 deinit();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	enum _primitive_mode
	{
		PRIMITIVE_MODE_LINES,
		PRIMITIVE_MODE_POINTS,
		PRIMITIVE_MODE_TRIANGLES,

		PRIMITIVE_MODE_COUNT
	};

	/* Private methods */
	void buildPO(glw::GLuint clipdistances_array_size, glw::GLuint culldistances_array_size, bool dynamic_index_writes,
				 _primitive_mode primitive_mode, bool redeclare_clipdistances, bool redeclare_culldistances,
				 bool use_core_functionality, bool use_gs, bool use_ts, bool fetch_culldistance_from_fs);

	void configureVAO(glw::GLuint clipdistances_array_size, glw::GLuint culldistances_array_size,
					  _primitive_mode primitive_mode);

	void deinitPO();

	void executeRenderTest(glw::GLuint clipdistances_array_size, glw::GLuint culldistances_array_size,
						   _primitive_mode primitive_mode, bool use_tesselation, bool fetch_culldistance_from_fs);

	glw::GLint readRedPixelValue(glw::GLint x, glw::GLint y);

	void readTexturePixels();

	/* Private fields */
	std::vector<glw::GLfloat> m_bo_data;
	glw::GLuint				  m_bo_id;
	glw::GLuint				  m_fbo_id;
	glw::GLuint				  m_po_id;
	glw::GLsizei			  m_render_primitives;
	glw::GLsizei			  m_render_vertices;
	glw::GLint				  m_sub_grid_cell_size;
	glw::GLuint				  m_to_id;
	glw::GLuint				  m_vao_id;

	const glw::GLuint		   m_to_height;
	const glw::GLuint		   m_to_width;
	static const glw::GLuint   m_to_pixel_data_cache_color_components = 4;
	std::vector<glw::GLushort> m_to_pixel_data_cache;
};

/** @brief Cull Distance Negative Test class
 *
 *  This class contains negative test cases,
 *  which check if the implementation returns
 *  properly in case of unsupport state
 *  configuration. Following cases are checked:
 *    * Use the basic outline but redeclare gl_ClipDistance and gl_CullDistance
 *      with sizes whose sum is more than MAX_COMBINED_CLIP_AND_CULL_DISTANCES.
 *      Expect a compile-time or link-time error.
 *    * Use the basic outline but don't redeclare gl_ClipDistance and/or
 *      gl_CullDistance with a size and statically write values to such elements
 *      of gl_ClipDistance and gl_CullDistance that the sum of these element
 *      indices is greater than MAX_COMBINED_CLIP_AND_CULL_DISTANCES minus two
 *      (the "minus two" part is needed because the indices are zero-based).
 *      Expect a compile-time or link-time error.
 *    * Use the basic outline but don't redeclare gl_ClipDistance and/or
 *      gl_CullDistance with a size and use dynamic indexing when writing their
 *      elements. Expect a compile-time or link-time error.
 */
class NegativeTest : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTest(deqp::Context& context);

protected:
	/* Protected methods */
	void						 deinit();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	std::string getTestDescription(glw::GLint n_test_iteration, bool should_redeclare_output_variables,
								   bool use_dynamic_index_based_writes);

	/* Private fields */
	glw::GLuint  m_fs_id;
	glw::GLuint  m_po_id;
	glw::GLchar* m_temp_buffer;
	glw::GLuint  m_vs_id;
};

/** @brief Grouping class for Cull Distance Tests */
class Tests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	Tests(deqp::Context& context);

	void init(void);

private:
	Tests(const CullDistance::Tests& other);
	Tests& operator=(const CullDistance::Tests& other);
};
};
/* CullDistance namespace */
} /* glcts namespace */

#endif // _GL3CCULLDISTANCETESTS_HPP
