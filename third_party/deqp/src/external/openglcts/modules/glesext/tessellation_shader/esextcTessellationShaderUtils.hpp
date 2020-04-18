#ifndef _ESEXTCTESSELLATIONSHADERUTILS_HPP
#define _ESEXTCTESSELLATIONSHADERUTILS_HPP
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
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwFunctions.hpp"
#include "tcuDefs.hpp"
#include <cstring>
#include <vector>

namespace glcts
{

/** Stores an ivec4 representation */
typedef struct _ivec4
{
	int x;
	int y;
	int z;
	int w;

	/** Constructor.
	 *
	 *  @param in_x Value to use for X component;
	 *  @param in_y Value to use for Y component;
	 *  @param in_z Value to use for Z component;
	 *  @param in_w Value to use for W component.
	 */
	_ivec4(int in_x, int in_y, int in_z, int in_w)
	{
		x = in_x;
		y = in_y;
		z = in_z;
		w = in_w;
	}

	/** Compares all components of _ivec4 instance with
	 *  another instance.
	 *
	 *  @return true if all components match, false otherwise.
	 **/
	bool operator==(const _ivec4& in) const
	{
		return (x == in.x) && (y == in.y) && (z == in.z) && (w == in.w);
	}

	/** Implements inequality operator.
	 *
	 *  @return true if any of the compared components
	 *          do not match, false otherwise.
	 **/
	bool operator!=(const _ivec4& in) const
	{
		return !(*this == in);
	}
} _ivec4;

/* Stores a vec2 representation */
typedef struct _vec2
{
	float x;
	float y;

	/** Constructor.
	 *
	 *  @param in_x Value to use for X component;
	 *  @param in_y Value to use for Y component;
	 */
	_vec2(float in_x, float in_y)
	{
		x = in_x;
		y = in_y;
	}

	/** Compares all components of _vec2 instance with
	 *  another instance, using == operator.
	 *
	 *  @return true if all components match, false otherwise.
	 **/
	bool operator==(const _vec2& in) const
	{
		return (x == in.x) && (y == in.y);
	}

	/** Implements inequality operator.
	 *
	 *  @return true if any of the compared components
	 *          do not match, false otherwise.
	 **/
	bool operator!=(const _vec2& in) const
	{
		return !(*this == in);
	}
} _vec2;

/* Stores a vec4 representation */
typedef struct _vec4
{
	float x;
	float y;
	float z;
	float w;

	/** Constructor.
	 *
	 *  @param in_x Value to use for X component;
	 *  @param in_y Value to use for Y component;
	 *  @param in_z Value to use for Z component;
	 *  @param in_w Value to use for W component.
	 */
	_vec4(float in_x, float in_y, float in_z, float in_w)
	{
		x = in_x;
		y = in_y;
		z = in_z;
		w = in_w;
	}

	/** Compares all components of _vec4 instance with
	 *  another instance, using == operator.
	 *
	 *  @return true if all components match, false otherwise.
	 **/
	bool operator==(const _vec4& in) const
	{
		return (x == in.x) && (y == in.y) && (z == in.z) && (w == in.w);
	}

	/** Implements inequality operator.
	 *
	 *  @return true if any of the compared components
	 *          do not match, false otherwise.
	 **/
	bool operator!=(const _vec4& in) const
	{
		return !(*this == in);
	}
} _vec4;

/** Defines a set of tessellation inner+outer levels */
typedef struct _tessellation_levels
{
	float inner[2];
	float outer[4];

	_tessellation_levels()
	{
		memset(inner, 0, sizeof(inner));
		memset(outer, 0, sizeof(outer));
	}
} _tessellation_levels;

/* Defines a vector of tessellation levels */
typedef std::vector<_tessellation_levels>		 _tessellation_levels_set;
typedef _tessellation_levels_set::const_iterator _tessellation_levels_set_const_iterator;
typedef _tessellation_levels_set::iterator		 _tessellation_levels_set_iterator;

/* Determines condition that returned level sets should meet in order to be returned
 * by TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode() .
 */
typedef enum {
	/********* General modes: do not use these values in conjugation  *********/

	/* All combinations of values from the set {-1, 1, GL_MAX_TESS_GEN_LEVEL_EXT / 2,
	 * GL_MAX_TESS_GEN_LEVEL_EXT} will be used for inner/outer tesselelation
	 * levels relevant to user-specified primitive mode.
	 * An important exception is that the negative value will be SKIPPED for
	 * outer tessellation levels (because otherwise no geometry will be generated
	 * by the tessellator)
	 **/
	TESSELLATION_LEVEL_SET_FILTER_ALL_COMBINATIONS = 0x1,

	/* Only combinations where:
	 *
	 * - inner tessellation levels use different values (inner[0] != inner[1])
	 * - outer tessellation levels use different values (outer[0] != outer[1] !=
	 *   != outer[2] != outer[3]);
	 *
	 * are allowed. */
	TESSELLATION_LEVEL_SET_FILTER_INNER_AND_OUTER_LEVELS_USE_DIFFERENT_VALUES = 0x2,

	/* All inner/outer tessellation level use the same base value */
	TESSELLATION_LEVEL_SET_FILTER_ALL_LEVELS_USE_THE_SAME_VALUE = 0x4,

	/********* Flags: can be combined with above general mode values  *********/
	TESSELLATION_LEVEL_SET_FILTER_EXCLUDE_NEGATIVE_BASE_VALUE = 0x8
} _tessellation_level_set_filter;

/* Represents primitive modes supported by GL_EXT_tessellation_shader */
typedef enum {
	TESSELLATION_SHADER_PRIMITIVE_MODE_FIRST = 0,

	TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES = TESSELLATION_SHADER_PRIMITIVE_MODE_FIRST,
	TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES,
	TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS,

	TESSELLATION_SHADER_PRIMITIVE_MODE_COUNT,
	TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN = TESSELLATION_SHADER_PRIMITIVE_MODE_COUNT
} _tessellation_primitive_mode;

/** Represents vertex ordering modes supported by GL_EXT_tessellation_shader */
typedef enum {
	TESSELLATION_SHADER_VERTEX_ORDERING_CCW,
	TESSELLATION_SHADER_VERTEX_ORDERING_CW,
	TESSELLATION_SHADER_VERTEX_ORDERING_DEFAULT,
	TESSELLATION_SHADER_VERTEX_ORDERING_UNKNOWN
} _tessellation_shader_vertex_ordering;

/** Represents vertex spacing modes supported by GL_EXT_tessellation_shader */
typedef enum {
	TESSELLATION_SHADER_VERTEX_SPACING_FIRST,

	TESSELLATION_SHADER_VERTEX_SPACING_EQUAL = TESSELLATION_SHADER_VERTEX_SPACING_FIRST,
	TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_EVEN,
	TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD,
	TESSELLATION_SHADER_VERTEX_SPACING_DEFAULT,

	TESSELLATION_SHADER_VERTEX_SPACING_COUNT,
	TESSELLATION_SHADER_VERTEX_SPACING_UNKNOWN = TESSELLATION_SHADER_VERTEX_SPACING_COUNT
} _tessellation_shader_vertex_spacing;

/** Defines what tesellation stages should be tested for a given test pass. */
typedef enum {
	TESSELLATION_TEST_TYPE_FIRST,

	TESSELLATION_TEST_TYPE_TCS_TES = TESSELLATION_TEST_TYPE_FIRST, /* tcs + tes stages defined */
	TESSELLATION_TEST_TYPE_TES,									   /* only tes stage defined */

	/* Always last */
	TESSELLATION_TEST_TYPE_COUNT,
	TESSELLATION_TEST_TYPE_UNKNOWN = TESSELLATION_TEST_TYPE_COUNT
} _tessellation_test_type;

/* Stores various helper functions used across multiple tessellation shader tests */
class TessellationShaderUtils
{
public:
	/* Public methods */
	TessellationShaderUtils(const glw::Functions& gl, glcts::TestCaseBase* parentTest);
	~TessellationShaderUtils();

	void compileShaders(glw::GLint n_shaders, const glw::GLuint* shaders, bool should_succeed);

	static void convertBarycentricCoordinatesToCartesian(const float* barycentric_coordinates,
														 float*		  out_cartesian_coordinates);

	static void convertCartesianCoordinatesToBarycentric(const float* cartesian_coordinates,
														 float*		  out_barycentric_coordinates);

	unsigned int getAmountOfVerticesGeneratedByTessellator(_tessellation_primitive_mode		   primitive_mode,
														   const float*						   inner_tessellation_level,
														   const float*						   outer_tessellation_level,
														   _tessellation_shader_vertex_spacing vertex_spacing,
														   bool								   is_point_mode_enabled);

	std::vector<char> getDataGeneratedByTessellator(const float* inner, bool point_mode,
													_tessellation_primitive_mode		 primitive_mode,
													_tessellation_shader_vertex_ordering vertex_ordering,
													_tessellation_shader_vertex_spacing  vertex_spacing,
													const float*						 outer);

	static std::string getESTokenForPrimitiveMode(_tessellation_primitive_mode primitive_mode);
	static std::string getESTokenForVertexOrderingMode(_tessellation_shader_vertex_ordering vertex_ordering);
	static std::string getESTokenForVertexSpacingMode(_tessellation_shader_vertex_spacing vertex_spacing);

	static std::string getGenericTCCode(unsigned int n_patch_vertices, bool should_use_glInvocationID_indexed_input);

	static std::string getGenericTECode(_tessellation_shader_vertex_spacing  vertex_spacing,
										_tessellation_primitive_mode		 primitive_mode,
										_tessellation_shader_vertex_ordering vertex_ordering, bool point_mode);

	static glw::GLint getPatchVerticesForPrimitiveMode(_tessellation_primitive_mode primitive_mode);

	static void getTessellationLevelAfterVertexSpacing(_tessellation_shader_vertex_spacing vertex_spacing, float level,
													   glw::GLint gl_max_tess_gen_level_value, float* out_clamped,
													   float* out_clamped_and_rounded);

	static _tessellation_levels_set getTessellationLevelSetForPrimitiveMode(_tessellation_primitive_mode primitive_mode,
																			glw::GLint gl_max_tess_gen_level_value,
																			_tessellation_level_set_filter filter);

	static glw::GLenum getTFModeForPrimitiveMode(_tessellation_primitive_mode primitive_mode, bool is_point_mode);

	static bool isOuterEdgeVertex(_tessellation_primitive_mode primitive_mode, const float* tessellated_vertex_data);

	static bool isTriangleDefined(const float* triangle_vertex_data, const float* vertex_data);

private:
	/* Private type definitions */
	/** Defines a single counter program */
	typedef struct _tessellation_vertex_counter_program
	{
		/* Properties */
		float								inner_tess_level[2];
		bool								is_point_mode_enabled;
		glw::GLint							n_patch_vertices;
		float								outer_tess_level[4];
		_tessellation_primitive_mode		primitive_mode;
		_tessellation_shader_vertex_spacing vertex_spacing;

		std::vector<char> m_data;
		unsigned int	  n_data_vertices;

		glw::GLint			  po_id;
		glw::GLint			  tc_id;
		glw::GLint			  te_id;
		glw::GLint			  tess_level_inner_uniform_location;
		glw::GLint			  tess_level_outer_uniform_location;
		const glw::Functions& m_gl;

		_tessellation_vertex_counter_program(const glw::Functions& gl) : m_gl(gl)
		{
			memset(inner_tess_level, 0, sizeof(inner_tess_level));
			memset(outer_tess_level, 0, sizeof(outer_tess_level));

			is_point_mode_enabled = false;
			n_patch_vertices	  = 0;
			po_id				  = 0;
			primitive_mode		  = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
			tc_id				  = 0;
			te_id				  = 0;
			vertex_spacing		  = TESSELLATION_SHADER_VERTEX_SPACING_UNKNOWN;
			n_data_vertices		  = 0;

			tess_level_inner_uniform_location = -1;
			tess_level_outer_uniform_location = -1;
		}

		~_tessellation_vertex_counter_program()
		{
			if (po_id != 0)
			{
				m_gl.deleteProgram(po_id);
				po_id = 0;
			}

			if (tc_id != 0)
			{
				m_gl.deleteShader(tc_id);
				tc_id = 0;
			}

			if (te_id != 0)
			{
				m_gl.deleteShader(te_id);
				te_id = 0;
			}
		}
	} _tessellation_vertex_counter_program;

	/* A vector of counter programs */
	typedef std::vector<_tessellation_vertex_counter_program> _programs;
	typedef _programs::const_iterator						  _programs_const_iterator;
	typedef _programs::iterator								  _programs_iterator;

	/* Private methods */
	void captureTessellationData(_tessellation_vertex_counter_program& program);
	void deinit();
	void init();

	void initTessellationVertexCounterProgram(const float* inner_tess_level, const float* outer_tess_level,
											  glw::GLint						  n_patch_vertices,
											  _tessellation_shader_vertex_spacing vertex_spacing,
											  _tessellation_primitive_mode primitive_mode, bool is_point_mode_enabled,
											  _tessellation_vertex_counter_program& result_descriptor);

	/* Private variables */
	const glw::Functions& m_gl;
	glw::GLuint			  m_bo_id;
	glw::GLuint			  m_fs_id;
	glw::GLuint			  m_qo_pg_id;
	glw::GLuint			  m_vs_id;

	glcts::TestCaseBase* m_parent_test;
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERUTILS_HPP
