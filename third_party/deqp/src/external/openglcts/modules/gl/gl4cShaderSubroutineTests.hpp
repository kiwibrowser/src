#ifndef _GL4CSHADERSUBROUTINETESTS_HPP
#define _GL4CSHADERSUBROUTINETESTS_HPP
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
 * \file  gl4cShaderSubroutineTests.hpp
 * \brief Declares test classes for "Shader Subroutine" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include <queue>

#include "tcuTestLog.hpp"

namespace gl4cts
{
namespace ShaderSubroutine
{
class Utils
{
public:
	/* Public type definitions */

	struct buffer
	{
		buffer(deqp::Context& context);
		~buffer();

		void bindRange(glw::GLenum target, glw::GLuint index, glw::GLintptr offset, glw::GLsizeiptr size);

		void generate();

		void update(glw::GLenum target, glw::GLsizeiptr size, glw::GLvoid* data, glw::GLenum usage);

		glw::GLuint m_id;

	private:
		deqp::Context& m_context;
	};

	struct framebuffer
	{
		framebuffer(deqp::Context& context);
		~framebuffer();

		void attachTexture(glw::GLenum attachment, glw::GLuint texture_id, glw::GLuint width, glw::GLuint height);

		void bind();
		void clear(glw::GLenum mask);

		void clearColor(glw::GLfloat red, glw::GLfloat green, glw::GLfloat blue, glw::GLfloat alpha);
		void generate();

		glw::GLuint m_id;

	private:
		deqp::Context& m_context;
	};

	/** Store information about program object
	 *
	 **/
	struct program
	{
		program(deqp::Context& context);
		~program();

		void build(const glw::GLchar* compute_shader_code, const glw::GLchar* fragment_shader_code,
				   const glw::GLchar* geometry_shader_code, const glw::GLchar* tesselation_control_shader_code,
				   const glw::GLchar* tesselation_evaluation_shader_code, const glw::GLchar* vertex_shader_code,
				   const glw::GLchar* const* varying_names, glw::GLuint n_varying_names, bool is_separable = false);

		void compile(glw::GLuint shader_id, const glw::GLchar* shader_code) const;

		bool isProgramBinarySupported() const;

		void createFromBinary(const std::vector<glw::GLubyte>& binary, glw::GLenum binary_format);

		void getBinary(std::vector<glw::GLubyte>& binary, glw::GLenum& binary_format) const;

		glw::GLuint getSubroutineIndex(const glw::GLchar* subroutine_name, glw::GLenum shader_stage) const;

		glw::GLint getSubroutineUniformLocation(const glw::GLchar* uniform_name, glw::GLenum shader_stage) const;

		glw::GLint getUniformLocation(const glw::GLchar* uniform_name) const;
		void link() const;
		void remove();
		void use() const;

		static const glw::GLenum ARB_COMPUTE_SHADER;

		glw::GLuint m_compute_shader_id;
		glw::GLuint m_fragment_shader_id;
		glw::GLuint m_geometry_shader_id;
		glw::GLuint m_program_object_id;
		glw::GLuint m_tesselation_control_shader_id;
		glw::GLuint m_tesselation_evaluation_shader_id;
		glw::GLuint m_vertex_shader_id;

	private:
		deqp::Context& m_context;
	};

	struct texture
	{
		texture(deqp::Context& context);
		~texture();

		void bind();

		void create(glw::GLuint width, glw::GLuint height, glw::GLenum internal_format);

		void get(glw::GLenum format, glw::GLenum type, glw::GLvoid* out_data);

		void update(glw::GLuint width, glw::GLuint height, glw::GLenum format, glw::GLenum type, glw::GLvoid* data);

		glw::GLuint m_id;

	private:
		deqp::Context& m_context;
	};

	struct vertexArray
	{
		vertexArray(deqp::Context& Context);
		~vertexArray();

		void generate();
		void bind();

		glw::GLuint m_id;

	private:
		deqp::Context& m_context;
	};

	/** Storage for 4 element vector of T
	 *
	 **/
	template <typename T>
	struct vec4
	{
		vec4()
		{
		}

		vec4(T x, T y, T z, T w) : m_x(x), m_y(y), m_z(z), m_w(w)
		{
		}

		bool operator==(const vec4& val) const
		{
			bool result = true;

			result = result && compare(m_x, val.m_x);
			result = result && compare(m_y, val.m_y);
			result = result && compare(m_z, val.m_z);
			result = result && compare(m_w, val.m_w);

			return result;
		}

		void log(tcu::MessageBuilder& message) const
		{
			message << "[ " << m_x << ", " << m_y << ", " << m_z << ", " << m_w << " ]";
		}

		T m_x;
		T m_y;
		T m_z;
		T m_w;
	};

	enum _shader_stage
	{
		SHADER_STAGE_FIRST,

		SHADER_STAGE_VERTEX = SHADER_STAGE_FIRST,
		SHADER_STAGE_TESSELLATION_CONTROL,
		SHADER_STAGE_TESSELLATION_EVALUATION,
		SHADER_STAGE_GEOMETRY,
		SHADER_STAGE_FRAGMENT,

		SHADER_STAGE_COUNT
	};

	enum _variable_type
	{
		VARIABLE_TYPE_BOOL,
		VARIABLE_TYPE_BVEC2,
		VARIABLE_TYPE_BVEC3,
		VARIABLE_TYPE_BVEC4,
		VARIABLE_TYPE_DOUBLE,
		VARIABLE_TYPE_DVEC2,
		VARIABLE_TYPE_DVEC3,
		VARIABLE_TYPE_DVEC4,
		VARIABLE_TYPE_FLOAT,
		VARIABLE_TYPE_INT,
		VARIABLE_TYPE_IVEC2,
		VARIABLE_TYPE_IVEC3,
		VARIABLE_TYPE_IVEC4,
		VARIABLE_TYPE_MAT2,
		VARIABLE_TYPE_MAT2X3,
		VARIABLE_TYPE_MAT2X4,
		VARIABLE_TYPE_MAT3,
		VARIABLE_TYPE_MAT3X2,
		VARIABLE_TYPE_MAT3X4,
		VARIABLE_TYPE_MAT4,
		VARIABLE_TYPE_MAT4X2,
		VARIABLE_TYPE_MAT4X3,
		VARIABLE_TYPE_UINT,
		VARIABLE_TYPE_UVEC2,
		VARIABLE_TYPE_UVEC3,
		VARIABLE_TYPE_UVEC4,
		VARIABLE_TYPE_VEC2,
		VARIABLE_TYPE_VEC3,
		VARIABLE_TYPE_VEC4,

		/* Always last */
		VARIABLE_TYPE_UNKNOWN
	};

	/* Public methods */
	static bool buildProgram(const glw::Functions& gl, const std::string& vs_body, const std::string& tc_body,
							 const std::string& te_body, const std::string& gs_body, const std::string& fs_body,
							 const glw::GLchar** xfb_varyings, const unsigned int& n_xfb_varyings,
							 glw::GLuint* out_vs_id, glw::GLuint* out_tc_id, glw::GLuint* out_te_id,
							 glw::GLuint* out_gs_id, glw::GLuint* out_fs_id, glw::GLuint* out_po_id);

	static _variable_type getBaseVariableType(const _variable_type& variable_type);

	static unsigned int getComponentSizeForVariableType(const _variable_type& variable_type);

	static glw::GLenum getGLenumForShaderStage(const _shader_stage& shader_stage);

	static unsigned int getNumberOfComponentsForVariableType(const _variable_type& variable_type);

	static std::string getShaderStageString(const _shader_stage& shader_stage);

	static std::string getShaderStageStringFromGLEnum(const glw::GLenum shader_stage_glenum);

	static _variable_type getVariableTypeFromProperties(const _variable_type& base_variable_type,
														const unsigned int&   n_components);

	static std::string getVariableTypeGLSLString(const _variable_type& variable_type);

	static const glw::GLchar* programInterfaceToStr(glw::GLenum program_interface);
	static const glw::GLchar* pnameToStr(glw::GLenum pname);

private:
	/* Private methods */
	template <typename T>
	static bool compare(const T& left, const T& right)
	{
		return left == right;
	}

	static bool compare(const glw::GLfloat& left, const glw::GLfloat& right);
};

/** Verify that Get* commands accept MAX_SUBROUTINES and
 *  MAX_SUBROUTINE_UNIFORM_LOCATIONS tokens and that the returned values
 *  are not lower than required by the specification.
 **/
class APITest1 : public deqp::TestCase
{
public:
	/* Public methods */
	APITest1(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	/* Private methods */

	/* Private fields */
	bool m_has_test_passed;
};

/** Check if <bufsize> and <length> parameters behave correctly in
 *  GetActiveSubroutineName and GetActiveSubroutineUniformName functions.
 **/
class APITest2 : public deqp::TestCase
{
public:
	/* Public methods */
	APITest2(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */

	/* Private methods */
	std::string getVertexShaderBody();
	void		initTest();
	void		verifyGLGetActiveSubroutineNameFunctionality();
	void		verifyGLGetActiveSubroutineUniformNameFunctionality();

	/* Private fields */
	glw::GLchar* m_buffer;
	bool		 m_has_test_passed;
	glw::GLuint  m_po_id;
	const char*  m_subroutine_name1;
	const char*  m_subroutine_name2;
	const char*  m_subroutine_uniform_name;
	glw::GLuint  m_vs_id;
};

/** * Create program with 2 subroutines taking one parameter and 1 subroutine
 *    uniform. Select the first subroutine and make a draw. Verify the result
 *    and draw again with second subroutine selected then verify result again.
 *    Repeat for following subroutines return and argument types: bool, float,
 *    int, uint, double, *vec*, *mat*.
 *
 *  * Same as above, but with return and argument types as arrays.
 *
 ***/
class FunctionalTest1_2 : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest1_2(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	struct _test_case
	{
		unsigned int		  array_size;
		Utils::_variable_type variable_type;
	};

	typedef std::vector<_test_case>		_test_cases;
	typedef _test_cases::const_iterator _test_cases_const_iterator;

	/* Private methods */
	void deinitTestIteration();
	bool executeTestIteration(const _test_case& test_case);

	std::string getVertexShaderBody(const Utils::_variable_type& variable_type, unsigned int array_size);

	void initTest();

	bool verifyXFBData(const void* xfb_data, const Utils::_variable_type& variable_type);

	/* Private fields */
	bool		m_has_test_passed;
	glw::GLuint m_po_id;
	glw::GLuint m_po_getter0_subroutine_index;
	glw::GLuint m_po_getter1_subroutine_index;
	glw::GLint  m_po_subroutine_uniform_index;
	_test_cases m_test_cases;
	glw::GLuint m_xfb_bo_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

/** * Create a program with 4 subroutines and 2 subroutine uniforms and query
 *    it using: GetProgramStageiv, GetActiveSubroutineUniformiv,
 *    GetActiveSubroutineUniformName, GetActiveSubroutineName,
 *    GetUniformSubroutineuiv, GetSubroutineIndex and
 *    GetSubroutineUniformLocation. Verify the results and use them to select
 *    subroutines, then make a draw and select different set of subroutines.
 *    Draw again and verify the results.
 *
 *  OpenGL 4.3 or ARB_program_interface_query support required
 *  * Same as above, but query the program using calls introduced in
 *    ARB_program_interface_query extension.
 **/
class FunctionalTest3_4 : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest3_4(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private types */
	/** Connect pname with expected value. Used to check Get* API.
	 *
	 **/
	struct inspectionDetails
	{
		glw::GLenum pname;
		glw::GLint  expected_value;
	};

	/* Private types */
	/** Connect program_interface, pname and expected value. Used to check GetProgramInterface.
	 *
	 **/
	struct inspectionDetailsForProgramInterface
	{
		glw::GLenum program_interface;
		glw::GLenum pname;
		glw::GLint  expected_value;
	};

	/* Private methods */
	bool checkProgramStageiv(glw::GLuint program_id, glw::GLenum pname, glw::GLint expected) const;

	bool checkProgramResourceiv(glw::GLuint program_id, glw::GLenum program_interface, glw::GLenum prop,
								const glw::GLchar* resource_name, glw::GLint expected) const;

	bool checkProgramInterfaceiv(glw::GLuint program_id, glw::GLenum program_interface, glw::GLenum pname,
								 glw::GLint expected) const;

	bool checkActiveSubroutineUniformiv(glw::GLuint program_id, glw::GLuint index, glw::GLenum pname,
										glw::GLint expected) const;

	glw::GLuint getProgramResourceIndex(glw::GLuint program_id, glw::GLenum program_interface,
										const glw::GLchar* resource_name) const;

	glw::GLuint getSubroutineIndex(glw::GLuint program_id, const glw::GLchar* subroutine_name,
								   bool use_program_query) const;

	glw::GLint getSubroutineUniformLocation(glw::GLuint program_id, const glw::GLchar* uniform_name,
											bool use_program_query) const;

	bool inspectProgramStageiv(glw::GLuint program_id) const;
	bool inspectProgramInterfaceiv(glw::GLuint program_id) const;

	bool inspectProgramResourceiv(glw::GLuint program_id, const glw::GLchar** subroutine_names,
								  const glw::GLchar** uniform_names) const;

	bool inspectActiveSubroutineUniformiv(glw::GLuint program_id, const glw::GLchar** uniform_names) const;

	bool inspectActiveSubroutineUniformName(glw::GLuint program_id, const glw::GLchar** uniform_names) const;

	bool inspectActiveSubroutineName(glw::GLuint program_id, const glw::GLchar** subroutine_names) const;

	bool inspectSubroutineBinding(glw::GLuint program_id, const glw::GLchar** subroutine_names,
								  const glw::GLchar** uniform_names, bool use_program_query) const;

	bool testDraw(glw::GLuint program_id, const glw::GLchar* first_routine_name, const glw::GLchar* second_routine_name,
				  const glw::GLchar** uniform_names, const Utils::vec4<glw::GLfloat> data[5],
				  bool use_program_query) const;

	/* Private fields */
	glw::GLint m_n_active_subroutine_uniforms;
	glw::GLint m_n_active_subroutine_uniform_locations;
	glw::GLint m_n_active_subroutines;
	glw::GLint m_n_active_subroutine_uniform_name_length;
	glw::GLint m_n_active_subroutine_name_length;
	glw::GLint m_n_active_subroutine_uniform_size;
};

/**
 * * Create a program with 8 subroutines and 4 subroutine uniforms. Each
 *   subroutine uniform should have different signature that should match 2
 *   subroutines. Go through all possible combinations of subroutine uniforms
 *   values and for each combination verify that it works as expected.
 **/
class FunctionalTest5 : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest5(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void logError(const glw::GLchar* subroutine_names[4][2], const glw::GLuint subroutine_combination[4],
				  const Utils::vec4<glw::GLfloat> input_data[3], const Utils::vec4<glw::GLfloat>& first_routine_result,
				  const Utils::vec4<glw::GLfloat>& second_routine_result,
				  const Utils::vec4<glw::GLfloat>& third_routine_result,
				  const Utils::vec4<glw::GLuint>&  fourth_routine_result,
				  const Utils::vec4<glw::GLfloat>& first_routine_expected_result,
				  const Utils::vec4<glw::GLfloat>& second_routine_expected_result,
				  const Utils::vec4<glw::GLfloat>& third_routine_expected_result,
				  const Utils::vec4<glw::GLuint>&  fourth_routine_expected_result) const;

	void testDraw(const glw::GLuint subroutine_combination[4], const Utils::vec4<glw::GLfloat> input_data[3],
				  Utils::vec4<glw::GLfloat>& out_first_routine_result,
				  Utils::vec4<glw::GLfloat>& out_second_routine_result,
				  Utils::vec4<glw::GLfloat>& out_third_routine_result,
				  Utils::vec4<glw::GLuint>&  out_fourth_routine_result) const;

	bool verify(const Utils::vec4<glw::GLfloat>& first_routine_result,
				const Utils::vec4<glw::GLfloat>& second_routine_result,
				const Utils::vec4<glw::GLfloat>& third_routine_result,
				const Utils::vec4<glw::GLuint>&  fourth_routine_result,
				const Utils::vec4<glw::GLfloat>& first_routine_expected_result,
				const Utils::vec4<glw::GLfloat>& second_routine_expected_result,
				const Utils::vec4<glw::GLfloat>& third_routine_expected_result,
				const Utils::vec4<glw::GLuint>&  fourth_routine_expected_result) const;

	/* Private fields */
	glw::GLuint m_subroutine_uniform_locations[4][1];
	glw::GLuint m_subroutine_indices[4][2];
	glw::GLuint m_uniform_locations[3];
};

/**
 * * Create a program with a subroutine and a subroutine uniform. Verify that
 *   subroutine can be called directly with a static use of subroutine's
 *   function name, as is done with non-subroutine function declarations and
 *   calls.
 **/
class FunctionalTest6 : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest6(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();
};

/**
 * * Create a program with 2 subroutines and an array of 4 subroutine
 *   uniforms. Go through all possible combinations of subroutine uniforms
 *   values and for each combination verify that it works as expected by
 *   performing draw or dispatch call. Also verify that length() function
 *   works correctly when used on array of subroutine uniforms.
 *
 * * Verify that program which uses uniforms, constant expressions and
 *   dynamically uniform loop index to access subroutine uniform array
 *   compiles and works as expected.
 **/
class FunctionalTest7_8 : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest7_8(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void calculate(glw::GLuint function, const Utils::vec4<glw::GLfloat>& left, const Utils::vec4<glw::GLfloat>& right,
				   Utils::vec4<glw::GLfloat>& out) const;

	void calculate(const glw::GLuint combination[4], const Utils::vec4<glw::GLfloat>& left,
				   const Utils::vec4<glw::GLfloat>& right, const Utils::vec4<glw::GLuint>& indices,
				   Utils::vec4<glw::GLfloat>& out_combined, Utils::vec4<glw::GLfloat>& out_combined_inversed,
				   Utils::vec4<glw::GLfloat>& out_constant, Utils::vec4<glw::GLfloat>& out_constant_inversed,
				   Utils::vec4<glw::GLfloat>& out_dynamic, Utils::vec4<glw::GLfloat>& out_dynamic_inversed,
				   Utils::vec4<glw::GLfloat>& out_loop) const;

	void logError(const glw::GLuint combination[4], const Utils::vec4<glw::GLfloat>& left,
				  const Utils::vec4<glw::GLfloat>& right, const Utils::vec4<glw::GLuint>& indices,
				  const Utils::vec4<glw::GLfloat> vec4_expected[7], const Utils::vec4<glw::GLfloat> vec4_result[7],
				  glw::GLuint array_length, bool result[7]) const;

	bool testDraw(const glw::GLuint combination[4], const Utils::vec4<glw::GLfloat>& left,
				  const Utils::vec4<glw::GLfloat>& right, const Utils::vec4<glw::GLuint>& indices) const;

	/* Private fields */
	glw::GLuint m_subroutine_uniform_locations[4];
	glw::GLuint m_subroutine_indices[2];
	glw::GLuint m_uniform_locations[3];
};

/**
 *  Make sure that program with one function associated with 3 different
 *  subroutine types and 3 subroutine uniforms using that function compiles
 *  and works as expected.
 *
 **/
class FunctionalTest9 : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest9(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	std::string getVertexShaderBody() const;
	void		initTest();
	void verifyXFBData(const glw::GLvoid* data_ptr);

	/* Private fields */
	bool			   m_has_test_passed;
	const unsigned int m_n_points_to_draw;
	glw::GLuint		   m_po_id;
	glw::GLuint		   m_vao_id;
	glw::GLuint		   m_vs_id;
	glw::GLuint		   m_xfb_bo_id;
};

/**
 * OpenGL 4.3 or ARB_arrays_of_arrays support required
 * * Create a program that uses array of arrays for subroutine uniform type
 *   and verify that it works as expected.
 **/
class FunctionalTest10 : public deqp::TestCase
{
public:
	FunctionalTest10(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	glw::GLint testDraw(const glw::GLuint routine_indices[16]) const;

	/* Private fields */
	glw::GLuint m_subroutine_uniform_locations[16];
	glw::GLuint m_subroutine_indices[2];
};

/**
 * * Verify that following operations work correctly when performed inside
 *   subroutine body: setting global variable, texture sampling, writing
 *   to fragment shader outputs, using discard function (fragment shader
 *   only), calling other functions and subroutines.
 **/
class FunctionalTest11 : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest11(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private types */
	struct testConfiguration
	{
		testConfiguration(const glw::GLchar* description, const glw::GLubyte expected_color[4],
						  glw::GLuint discard_fragment, glw::GLuint set_global_colors, glw::GLuint sample_texture,
						  glw::GLuint compare, glw::GLuint test, glw::GLuint first_sampler, glw::GLuint second_sampler)
		{
			m_description = description;

			m_expected_color[0] = expected_color[0];
			m_expected_color[1] = expected_color[1];
			m_expected_color[2] = expected_color[2];
			m_expected_color[3] = expected_color[3];

			m_routines[0] = discard_fragment;
			m_routines[1] = set_global_colors;
			m_routines[2] = sample_texture;
			m_routines[3] = compare;
			m_routines[4] = test;

			m_samplers[0] = first_sampler;
			m_samplers[1] = second_sampler;
		}

		const glw::GLchar* m_description;
		glw::GLubyte	   m_expected_color[4];
		glw::GLuint		   m_routines[5];
		glw::GLuint		   m_samplers[2];
	};

	/* Private methods */
	void fillTexture(Utils::texture& texture, const glw::GLubyte color[4]) const;

	bool testDraw(const glw::GLuint routine_configuration[5], const glw::GLuint sampler_configuration[2],
				  const glw::GLubyte expected_color[4], Utils::texture& color_texture) const;

	/* Private fields */
	/* Constants */
	static const glw::GLuint m_texture_height;
	static const glw::GLuint m_texture_width;

	/* Locations and indices */
	glw::GLuint m_subroutine_uniform_locations[5];
	glw::GLuint m_subroutine_indices[5][2];
	glw::GLuint m_uniform_locations[2];
	glw::GLuint m_source_textures[2];
};

/**
 * OpenGL 4.3 or ARB_shader_storage_buffer_object, ARB_atomic_counters
 * and ARB_shader_image_load_store support required
 * * Same as above, but check writing/reading from storage buffer, performing
 *   operations on atomic counters and writing/reading from an image. This
 *   should be tested in a program stage which supports operations of these
 *   kind.
 **/
class FunctionalTest12 : public deqp::TestCase
{
public:
	FunctionalTest12(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void fillTexture(Utils::texture& texture, const glw::GLuint color[4]) const;

	bool verifyTexture(Utils::texture& texture, const glw::GLuint color[4]) const;

	bool testAtomic();

	bool testAtomicDraw(glw::GLuint subourinte_index, const glw::GLuint expected_results[3]) const;

	bool testImage();

	bool testImageDraw(glw::GLuint subroutine_index, Utils::texture& left, Utils::texture& right,
					   const glw::GLuint expected_left_color[4], const glw::GLuint expected_right_color[4]) const;

	bool testSSBO();

	bool testSSBODraw(glw::GLuint subourinte_index, const glw::GLuint expected_results[4]) const;

	/* Private fields */
	/* Constatnts */
	static const glw::GLuint m_texture_height;
	static const glw::GLuint m_texture_width;

	/* Locations */
	glw::GLuint m_left_image;
	glw::GLuint m_right_image;
};

/**
 *  Verify that subroutines work correctly when used in separate shader
 *  objects.
 *
 **/
class FunctionalTest13 : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest13(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	std::string getFragmentShaderBody(unsigned int n_id);
	std::string getGeometryShaderBody(unsigned int n_id);
	std::string getTessellationControlShaderBody(unsigned int n_id);
	std::string getTessellationEvaluationShaderBody(unsigned int n_id);
	std::string getVertexShaderBody(unsigned int n_id);
	void initTest();

	void verifyReadBuffer(unsigned int n_fs_id, unsigned int n_fs_subroutine, unsigned int n_gs_id,
						  unsigned int n_gs_subroutine, unsigned int n_tc_id, unsigned int n_tc_subroutine,
						  unsigned int n_te_id, unsigned int n_te_subroutine, unsigned int n_vs_id,
						  unsigned int n_vs_subroutine);

	/* Private fields */
	glw::GLuint		   m_fbo_id;
	glw::GLuint		   m_fs_po_ids[2];
	glw::GLuint		   m_gs_po_ids[2];
	glw::GLuint		   m_pipeline_id;
	unsigned char*	 m_read_buffer;
	glw::GLuint		   m_tc_po_ids[2];
	glw::GLuint		   m_te_po_ids[2];
	const unsigned int m_to_height;
	glw::GLuint		   m_to_id;
	const unsigned int m_to_width;
	glw::GLuint		   m_vao_id;
	glw::GLuint		   m_vs_po_ids[2];

	bool m_has_test_passed;
};

/**
 * * Create program with subroutines that use structures as parameters and
 *   make sure it works correctly.
 *
 * OpenGL 4.1 or ARB_get_program_binary support required
 * * Create a program with 4 subroutines and 2 subroutine uniforms. Query
 *   names and indices of all active subroutines and query names and
 *   locations of all active subroutine uniforms. Call GetProgramBinary on
 *   this program and delete it. Create new program from the binary using
 *   ProgramBinary. Verify that all active subroutine names and indices in
 *   this program match ones from the original program. Also verify that
 *   all active subroutine uniform names and locations match ones from
 *   original program. Make a draw, expect random but valid set of selected
 *   subroutines, then select arbitrary set of subroutines, make a draw and
 *   verify the results.
 **/
class FunctionalTest14_15 : public deqp::TestCase
{
public:
	FunctionalTest14_15(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	bool testDefaultSubroutineSet(const Utils::vec4<glw::GLuint>& uni_input,
								  const Utils::vec4<glw::GLuint>  expected_routine_1_result[2],
								  const Utils::vec4<glw::GLuint>  expected_routine_2_result[2]) const;

	bool testDraw(glw::GLuint routine_configuration, const Utils::vec4<glw::GLuint>& uni_input,
				  const Utils::vec4<glw::GLuint>& expected_routine_1_result,
				  const Utils::vec4<glw::GLuint>& expected_routine_2_result) const;

	bool testIndicesAndLocations() const;

	/* Private fields */
	/* Locations and indices */
	glw::GLuint m_subroutine_uniform_locations[2];
	glw::GLuint m_subroutine_indices[2][2];
	glw::GLuint m_uniform_location;
	glw::GLuint m_initial_subroutine_uniform_locations[2];
	glw::GLuint m_initial_subroutine_indices[2][2];
};

/**
 * Check that when the active program for a shader stage is re-linked or
 * changed by a call to UseProgram, BindProgramPipeline, or
 * UseProgramStages, subroutine uniforms for that stage are reset to
 * arbitrarily chosen default functions with compatible subroutine types.
 *
 **/
class FunctionalTest16 : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest16(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	/* Defines a specific use case that should be checked during particular
	 * test iteration.
	 */
	enum _test_case
	{
		TEST_CASE_FIRST,

		TEST_CASE_SWITCH_TO_DIFFERENT_PROGRAM_OBJECT = TEST_CASE_FIRST,
		TEST_CASE_SWITCH_TO_DIFFERENT_PROGRAM_PIPELINE_OBJECT,
		TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_FRAGMENT_STAGE,
		TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_GEOMETRY_STAGE,
		TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_TESS_CONTROL_STAGE,
		TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_TESS_EVALUATION_STAGE,
		TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_VERTEX_STAGE,

		/* Always last */
		TEST_CASE_COUNT
	};

	/** Defines a number of different subroutine-specific properties
	 *  for a single shader stage.
	 **/
	struct _shader_stage
	{
		glw::GLuint default_subroutine1_value;
		glw::GLuint default_subroutine2_value;
		glw::GLuint default_subroutine3_value;
		glw::GLuint default_subroutine4_value;
		glw::GLint  subroutine1_uniform_location;
		glw::GLint  subroutine2_uniform_location;
		glw::GLint  subroutine3_uniform_location;
		glw::GLint  subroutine4_uniform_location;
		glw::GLuint function1_index;
		glw::GLuint function2_index;
		glw::GLuint function3_index;
		glw::GLuint function4_index;

		glw::GLenum gl_stage;
	};

	/** Describes subroutine-specific properties for a program object */
	struct _program
	{
		_shader_stage fragment;
		_shader_stage geometry;
		_shader_stage tess_control;
		_shader_stage tess_evaluation;
		_shader_stage vertex;
	};

	/** Describes modes that verify*() functions can run in */
	enum _subroutine_uniform_value_verification
	{
		SUBROUTINE_UNIFORMS_SET_TO_DEFAULT_VALUES,
		SUBROUTINE_UNIFORMS_SET_TO_NONDEFAULT_VALUES,
		SUBROUTINE_UNIFORMS_SET_TO_VALID_VALUES,
	};

	/* Private methods */
	std::string getShaderBody(const Utils::_shader_stage& shader_stage, const unsigned int& n_id) const;

	void getShaderStages(bool retrieve_program_object_shader_ids, const unsigned int& n_id,
						 const _shader_stage** out_shader_stages) const;

	void initTest();

	void verifySubroutineUniformValues(const _test_case& test_case, const unsigned int& n_id,
									   const _subroutine_uniform_value_verification& verification);

	void verifySubroutineUniformValuesForShaderStage(const _shader_stage&						   shader_stage,
													 const _subroutine_uniform_value_verification& verification);

	/* Private fields */
	bool m_are_pipeline_objects_supported;
	bool m_has_test_passed;

	glw::GLuint m_fs_ids[2];
	glw::GLuint m_gs_ids[2];
	glw::GLuint m_po_ids[2];
	glw::GLuint m_tc_ids[2];
	glw::GLuint m_te_ids[2];
	glw::GLuint m_vs_ids[2];

	glw::GLuint m_fs_po_ids[2];
	glw::GLuint m_gs_po_ids[2];
	glw::GLuint m_pipeline_object_ids[2];
	glw::GLuint m_tc_po_ids[2];
	glw::GLuint m_te_po_ids[2];
	glw::GLuint m_vs_po_ids[2];

	_shader_stage m_fs_po_descriptors[2];
	_shader_stage m_gs_po_descriptors[2];
	_shader_stage m_tc_po_descriptors[2];
	_shader_stage m_te_po_descriptors[2];
	_shader_stage m_vs_po_descriptors[2];

	_program m_po_descriptors[2];
};

/**
 *  Create a program which uses the same subroutine and subroutine uniform
 *  names for every stage. Types of subroutines should be different in each
 *  stage. Make sure that such program compiles and works as expected.
 **/
class FunctionalTest17 : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest17(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	std::string getFragmentShaderBody() const;
	std::string getGeometryShaderBody() const;
	std::string getTessellationControlShaderBody() const;
	std::string getTessellationEvaluationShaderBody() const;
	std::string getVertexShaderBody() const;
	void		initTest();
	void		verifyRenderedData();

	/* Private fields */
	glw::GLuint		   m_fbo_id;
	glw::GLuint		   m_fs_id;
	glw::GLuint		   m_gs_id;
	bool			   m_has_test_passed;
	glw::GLuint		   m_po_id;
	glw::GLuint		   m_tc_id;
	glw::GLuint		   m_te_id;
	float*			   m_to_data;
	const unsigned int m_to_height;
	glw::GLuint		   m_to_id;
	const unsigned int m_to_width;
	glw::GLuint		   m_vao_id;
	glw::GLuint		   m_vs_id;
};

/**
 *  Make sure that calling a subroutine with argument value returned by
 *  another subroutine works correctly.
 *
 *  Verify that subroutines and subroutine uniforms work as expected when
 *  they are used in connection with control flow functions
 *  (if/else/case/while/for/break/continue).
 *
 **/
class FunctionalTest18_19 : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest18_19(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	typedef tcu::Vec4 (*PFNVEC4OPERATORPROC)(tcu::Vec4);

	/* Private methods */
	void executeTest(glw::GLuint bool_operator1_subroutine_location, glw::GLuint bool_operator2_subroutine_location,
					 glw::GLuint vec4_operator1_subroutine_location, glw::GLuint vec4_operator2_subroutine_location);

	std::string getVertexShaderBody() const;

	void initTest();
	void verifyXFBData(const glw::GLvoid* data, glw::GLuint bool_operator1_subroutine_location,
					   glw::GLuint bool_operator2_subroutine_location, glw::GLuint vec4_operator1_subroutine_location,
					   glw::GLuint vec4_operator2_subroutine_location);

	static tcu::Vec4 vec4operator_div2(tcu::Vec4 data);
	static tcu::Vec4 vec4operator_mul4(tcu::Vec4 data);

	/* Private fields */
	bool			   m_has_test_passed;
	const unsigned int m_n_points_to_draw;
	glw::GLuint		   m_po_id;
	glw::GLuint		   m_po_subroutine_divide_by_two_location;
	glw::GLuint		   m_po_subroutine_multiply_by_four_location;
	glw::GLuint		   m_po_subroutine_returns_false_location;
	glw::GLuint		   m_po_subroutine_returns_true_location;
	glw::GLint		   m_po_subroutine_uniform_bool_operator1;
	glw::GLint		   m_po_subroutine_uniform_bool_operator2;
	glw::GLint		   m_po_subroutine_uniform_vec4_processor1;
	glw::GLint		   m_po_subroutine_uniform_vec4_processor2;
	glw::GLuint		   m_xfb_bo_id;
	glw::GLuint		   m_vao_id;
	glw::GLuint		   m_vs_id;
};

/**
 *  Test whether all INVALID_OPERATION, INVALID_VALUE and INVALID_ENUM
 *  errors related to subroutines usage are properly generated.
 **/
class NegativeTest1 : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTest1(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initTest();

	/* Private fields */
	bool		m_has_test_passed;
	glw::GLint  m_po_active_subroutine_uniform_locations;
	glw::GLint  m_po_active_subroutine_uniforms;
	glw::GLint  m_po_active_subroutines;
	glw::GLint  m_po_subroutine_uniform_function_index;
	glw::GLint  m_po_subroutine_uniform_function2_index;
	glw::GLuint m_po_subroutine_test1_index;
	glw::GLuint m_po_subroutine_test2_index;
	glw::GLuint m_po_subroutine_test3_index;
	glw::GLuint m_po_not_linked_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/** Make sure that subroutine uniform variables are scoped to the shader
 *  execution stage the variable is declared in. Referencing subroutine
 *  uniform from different stage should cause compile or link error
 **/
class NegativeTest2 : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTest2(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */

	/* Private methods */
	void deinitGLObjects();
	void executeTestCase(const Utils::_shader_stage& referencing_stage);
	std::string getFragmentShaderBody(const Utils::_shader_stage& referencing_stage) const;
	std::string getGeometryShaderBody(const Utils::_shader_stage& referencing_stage) const;
	std::string getSubroutineUniformName(const Utils::_shader_stage& stage) const;
	std::string getTessellationControlShaderBody(const Utils::_shader_stage& referencing_stage) const;
	std::string getTessellationEvaluationShaderBody(const Utils::_shader_stage& referencing_stage) const;
	std::string getVertexShaderBody(const Utils::_shader_stage& referencing_stage) const;

	/* Private fields */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	bool		m_has_test_passed;
	glw::GLuint m_po_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vs_id;
};

/** Verify that "subroutine" keyword is necessary when declaring a
 *  subroutine uniform and a compilation error occurs without it.
 **/
class NegativeTest3 : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTest3(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void executeTest(const Utils::_shader_stage& shader_stage);
	std::string getFragmentShaderBody() const;
	std::string getGeometryShaderBody() const;
	std::string getTessellationControlShaderBody() const;
	std::string getTessellationEvaluationShaderBody() const;
	std::string getVertexShaderBody() const;

	/* Private fields */
	bool		m_has_test_passed;
	glw::GLuint m_so_id;
};

/**
 *  Verify that compile-time error is present when arguments and return type
 *  do not match between the function and each associated subroutine type.
 *  In particular make sure that applies when there are multiple associated
 *  subroutine types and only one of them does not match.
 *
 **/
class NegativeTest4 : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTest4(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type declarations */
	enum _test_case
	{
		TEST_CASE_FIRST,

		TEST_CASE_INCOMPATIBLE_RETURN_TYPE = TEST_CASE_FIRST,
		TEST_CASE_INCOMPATIBLE_ARGUMENT_LIST,

		/* Always last */
		TEST_CASE_COUNT
	};

	/* Private methods */
	std::string getShaderBody(const Utils::_shader_stage& shader_stage, const unsigned int& n_subroutine_types,
							  const _test_case& test_case) const;

	/* Private fields */
	bool	   m_has_test_passed;
	glw::GLint m_so_id;
};

/** Verify that link or compile error occurs when trying to link a program
 *  with no subroutine for subroutine uniform variable.
 **/
class NegativeTest5 : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTest5(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	/* Private methods */
	void deinitIteration();
	void executeIteration(const Utils::_shader_stage& shader_stage);
	std::string getFragmentShaderBody(bool include_invalid_subroutine_uniform_declaration) const;
	std::string getGeometryShaderBody(bool include_invalid_subroutine_uniform_declaration) const;
	std::string getTessellationControlShaderBody(bool include_invalid_subroutine_uniform_declaration) const;
	std::string getTessellationEvaluationShaderBody(bool include_invalid_subroutine_uniform_declaration) const;
	std::string getVertexShaderBody(bool include_invalid_subroutine_uniform_declaration) const;

	/* Private fields */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	bool		m_has_test_passed;
	glw::GLuint m_po_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vs_id;
};

/** Check that link or compile error occurs if any shader in a program
 *  includes two or more functions with the same name and at least one of
 *  which is associated with a subroutine type.
 **/
class NegativeTest6 : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTest6(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	/* Private methods */
	void deinitIteration();
	void executeIteration(const Utils::_shader_stage& shader_stage);
	std::string getFragmentShaderBody(bool include_invalid_declaration) const;
	std::string getGeometryShaderBody(bool include_invalid_declaration) const;
	std::string getTessellationControlShaderBody(bool include_invalid_declaration) const;
	std::string getTessellationEvaluationShaderBody(bool include_invalid_declaration) const;
	std::string getVertexShaderBody(bool include_invalid_declaration) const;

	/* Private fields */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	bool		m_has_test_passed;
	glw::GLuint m_po_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vs_id;
};

/**
 * * Verify that program fails to link if there is any possible combination
 *   of subroutine uniform values that would result in recursion.
 **/
class NegativeTest7 : public deqp::TestCase
{
public:
	NegativeTest7(deqp::Context& contex);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	bool test(const glw::GLchar* vertex_shader_code, const glw::GLchar* name_of_recursive_routine);

	/* Private fields */
	glw::GLuint m_program_id;
	glw::GLuint m_vertex_shader_id;
};

/** Test that either compile or link error occurs when function declared
 *  with subroutine does not include a body.
 **/
class NegativeTest8 : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTest8(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	/* Private methods */
	void deinitIteration();
	void executeIteration(const Utils::_shader_stage& shader_stage);
	std::string getFragmentShaderBody(bool include_invalid_declaration) const;
	std::string getGeometryShaderBody(bool include_invalid_declaration) const;
	std::string getTessellationControlShaderBody(bool include_invalid_declaration) const;
	std::string getTessellationEvaluationShaderBody(bool include_invalid_declaration) const;
	std::string getVertexShaderBody(bool include_invalid_declaration) const;

	/* Private fields */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	bool		m_has_test_passed;
	glw::GLuint m_po_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vs_id;
};

/**
 *   Make sure that it is not possible to assign float/int to subroutine
 *   uniform and that subroutine uniform values cannot be compared.
 *
 **/
class NegativeTest9 : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTest9(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	enum _test_case
	{
		TEST_CASE_FIRST,

		TEST_CASE_INVALID_FLOAT_TO_SUBROUTINE_UNIFORM_ASSIGNMENT = TEST_CASE_FIRST,
		TEST_CASE_INVALID_INT_TO_SUBROUTINE_UNIFORM_ASSIGNMENT,
		TEST_CASE_INVALID_SUBROUTINE_UNIFORM_VALUE_COMPARISON,

		/* Always last */
		TEST_CASE_COUNT
	};

	/* Private methods */
	std::string getTestCaseString(const _test_case& test_case);
	std::string getVertexShader(const _test_case& test_case);

	/* Private fields */
	bool		m_has_test_passed;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/**
 * Check that an overloaded function cannot be declared with subroutine and
 * a program will fail to compile or link if any shader or stage contains
 * two or more  functions with the same name if the name is associated with
 * a subroutine type.
 *
 **/
class NegativeTest10 : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTest10(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	struct _test_case
	{
		std::string name;

		std::string fs_body;
		std::string gs_body;
		std::string tc_body;
		std::string te_body;
		std::string vs_body;
	};

	typedef std::vector<_test_case>		_test_cases;
	typedef _test_cases::const_iterator _test_cases_const_iterator;

	/* Private methods */
	std::string getFragmentShader(bool include_duplicate_function);
	std::string getGeometryShader(bool include_duplicate_function);
	std::string getTessellationControlShader(bool include_duplicate_function);
	std::string getTessellationEvaluationShader(bool include_duplicate_function);
	std::string getVertexShader(bool include_duplicate_function);
	void initTestCases();

	/* Private fields */
	bool		m_has_test_passed;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	_test_cases m_test_cases;
	glw::GLuint m_vs_id;
};

/**
 *   Try to use subroutine uniform in invalid way in sampling, atomic and
 *   image functions. Verify that compile or link time error occurs.
 *
 **/
class NegativeTest11 : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTest11(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	enum _test_case
	{
		TEST_CASE_FIRST,

		TEST_CASE_INVALID_TEXTURE_SAMPLING_ATTEMPT = TEST_CASE_FIRST,
		TEST_CASE_INVALID_ATOMIC_COUNTER_USAGE_ATTEMPT,
		TEST_CASE_INVALID_IMAGE_FUNCTION_USAGE_ATTEMPT,

		/* Always last */
		TEST_CASE_COUNT
	};

	/* Private methods */
	std::string getTestCaseString(const _test_case& test_case);
	std::string getVertexShader(const _test_case& test_case);

	/* Private fields */
	bool		m_has_test_passed;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/**
 *  Verify that it is not allowed to use subroutine type for local/global
 *  variables, constructors or argument/return type.
 *
 **/
class NegativeTest12 : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTest12(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	enum _test_case
	{
		TEST_CASE_FIRST,

		TEST_CASE_INVALID_LOCAL_SUBROUTINE_VARIABLE = TEST_CASE_FIRST,
		TEST_CASE_INVALID_GLOBAL_SUBROUTINE_VARIABLE,
		TEST_CASE_SUBROUTINE_USED_AS_CONSTRUCTOR,
		TEST_CASE_SUBROUTINE_USED_AS_CONSTRUCTOR_ARGUMENT,
		TEST_CASE_SUBROUTINE_USED_AS_CONSTRUCTOR_RETURN_TYPE,

		/* Always last */
		TEST_CASE_COUNT
	};

	/* Private methods */
	std::string getTestCaseString(const _test_case& test_case);
	std::string getVertexShader(const _test_case& test_case);

	/* Private fields */
	bool		m_has_test_passed;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};
} /* ShaderSubroutine */

/** Group class for Shader Subroutine conformance tests */
class ShaderSubroutineTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	ShaderSubroutineTests(deqp::Context& context);
	virtual ~ShaderSubroutineTests()
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	ShaderSubroutineTests(const ShaderSubroutineTests&);
	ShaderSubroutineTests& operator=(const ShaderSubroutineTests&);
};

} /* gl4cts namespace */

#endif // _GL4CSHADERSUBROUTINETESTS_HPP
