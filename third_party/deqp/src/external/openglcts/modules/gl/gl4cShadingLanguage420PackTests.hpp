#ifndef _GL4CSHADINGLANGUAGE420PACKTESTS_HPP
#define _GL4CSHADINGLANGUAGE420PACKTESTS_HPP
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
 * \file  gl4cShadingLanguage420PackTests.hpp
 * \brief Declares test classes for "Shading Language 420Pack" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"

namespace tcu
{
class MessageBuilder;
} /* namespace tcu */

namespace gl4cts
{

namespace GLSL420Pack
{
class Utils
{
public:
	/* Public enums */
	enum TEXTURE_TYPES
	{
		TEX_BUFFER,
		TEX_2D,
		TEX_2D_RECT,
		TEX_2D_ARRAY,
		TEX_3D,
		TEX_CUBE,
		TEX_1D,
		TEX_1D_ARRAY,

		/* */
		TEXTURE_TYPES_MAX
	};

	enum SHADER_STAGES
	{
		COMPUTE_SHADER = 0,
		VERTEX_SHADER,
		TESS_CTRL_SHADER,
		TESS_EVAL_SHADER,
		GEOMETRY_SHADER,
		FRAGMENT_SHADER,

		/* */
		SHADER_STAGES_MAX
	};

	enum UTF8_CHARACTERS
	{
		TWO_BYTES,
		THREE_BYTES,
		FOUR_BYTES,
		FIVE_BYTES,
		SIX_BYTES,
		REDUNDANT_ASCII,

		/* */
		EMPTY
	};

	enum TYPES
	{
		FLOAT,
		DOUBLE,
		INT,
		UINT,

		/* */
		TYPES_MAX
	};

	enum QUALIFIER_CLASSES
	{
		QUAL_CLS_INVARIANCE = 0,
		QUAL_CLS_INTERPOLATION,
		QUAL_CLS_LAYOUT,
		QUAL_CLS_AUXILARY_STORAGE,
		QUAL_CLS_STORAGE,
		QUAL_CLS_PRECISION,

		/* */
		QUAL_CLS_MAX
	};

	enum QUALIFIERS
	{
		QUAL_NONE,

		/* CONSTNESS */
		QUAL_CONST,

		/* STORAGE */
		QUAL_IN,
		QUAL_OUT,
		QUAL_INOUT,
		QUAL_UNIFORM,

		/* AUXILARY */
		QUAL_PATCH,
		QUAL_CENTROID,
		QUAL_SAMPLE,

		/* INTERPOLATION */
		QUAL_FLAT,
		QUAL_NOPERSPECTIVE,
		QUAL_SMOOTH,

		/* LAYOUT */
		QUAL_LOCATION,

		/* PRECISION */
		QUAL_LOWP,
		QUAL_MEDIUMP,
		QUAL_HIGHP,
		QUAL_PRECISE,

		/* INVARIANCE */
		QUAL_INVARIANT,

		/* */
		QUAL_MAX
	};

	enum VARIABLE_STORAGE
	{
		INPUT,
		OUTPUT,
		UNIFORM,

		/* */
		STORAGE_MAX
	};

	enum VARIABLE_FLAVOUR
	{
		BASIC,
		ARRAY,
		INDEXED_BY_INVOCATION_ID
	};

	/* Public types */
	struct buffer
	{
		buffer(deqp::Context& context);
		~buffer();

		void bind() const;

		void bindRange(glw::GLuint index, glw::GLintptr offset, glw::GLsizeiptr size);

		void generate(glw::GLenum target);
		void* map(glw::GLenum access) const;
		void unmap() const;

		void update(glw::GLsizeiptr size, glw::GLvoid* data, glw::GLenum usage);

		void		release();
		glw::GLuint m_id;

	private:
		deqp::Context& m_context;
		glw::GLenum	m_target;
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

	struct shaderSource
	{
		shaderSource();
		shaderSource(const shaderSource& source);
		shaderSource(const glw::GLchar* source_code);

		struct shaderPart
		{
			std::string m_code;
			glw::GLint  m_length;
		};

		std::vector<shaderPart> m_parts;
		bool					m_use_lengths;
	};

	class shaderCompilationException : public std::exception
	{
	public:
		shaderCompilationException(const shaderSource& source, const glw::GLchar* message);

		virtual ~shaderCompilationException() throw()
		{
		}

		virtual const char* what() const throw();

		shaderSource m_shader_source;
		std::string  m_error_message;
	};

	class programLinkageException : public std::exception
	{
	public:
		programLinkageException(const glw::GLchar* error_message);

		virtual ~programLinkageException() throw()
		{
		}

		virtual const char* what() const throw();

		std::string m_error_message;
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

		void build(const shaderSource& compute_shader, const shaderSource& fragment_shader,
				   const shaderSource& geometry_shader, const shaderSource& tesselation_control_shader,
				   const shaderSource& tesselation_evaluation_shader, const shaderSource& vertex_shader,
				   const glw::GLchar* const* varying_names, glw::GLuint n_varying_names, bool is_separable = false);

		void compile(glw::GLuint shader_id, const shaderSource& source) const;

		void createFromBinary(const std::vector<glw::GLubyte>& binary, glw::GLenum binary_format);

		glw::GLint getAttribLocation(const glw::GLchar* name) const;

		void getBinary(std::vector<glw::GLubyte>& binary, glw::GLenum& binary_format) const;

		glw::GLuint getSubroutineIndex(const glw::GLchar* subroutine_name, glw::GLenum shader_stage) const;

		glw::GLint getSubroutineUniformLocation(const glw::GLchar* uniform_name, glw::GLenum shader_stage) const;

		glw::GLint getUniform1i(glw::GLuint location) const;
		glw::GLint getUniformLocation(const glw::GLchar* uniform_name) const;

		void link() const;
		void remove();

		void uniform(const glw::GLchar* uniform_name, TYPES type, glw::GLuint n_columns, glw::GLuint n_rows,
					 const void* data) const;

		void use() const;

		/* */
		static void printShaderSource(const shaderSource& source, tcu::MessageBuilder& log);

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

		void bind() const;

		void create(glw::GLuint width, glw::GLuint height, glw::GLenum internal_format);

		void create(glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLenum internal_format,
					TEXTURE_TYPES texture_type);

		void createBuffer(glw::GLenum internal_format, glw::GLuint buffer_id);

		void get(glw::GLenum format, glw::GLenum type, glw::GLvoid* out_data) const;

		void release();

		void update(glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLenum format, glw::GLenum type,
					glw::GLvoid* data);

		glw::GLuint m_id;

	private:
		glw::GLuint	m_buffer_id;
		deqp::Context& m_context;
		TEXTURE_TYPES  m_texture_type;
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

	/* Public typedefs */
	typedef std::vector<Utils::QUALIFIERS> qualifierSet;

	/* UniformN*v prototypes */
	typedef GLW_APICALL void(GLW_APIENTRY* uniformNdv)(glw::GLint, glw::GLsizei, const glw::GLdouble*);
	typedef GLW_APICALL void(GLW_APIENTRY* uniformNfv)(glw::GLint, glw::GLsizei, const glw::GLfloat*);
	typedef GLW_APICALL void(GLW_APIENTRY* uniformNiv)(glw::GLint, glw::GLsizei, const glw::GLint*);
	typedef GLW_APICALL void(GLW_APIENTRY* uniformNuiv)(glw::GLint, glw::GLsizei, const glw::GLuint*);
	typedef GLW_APICALL void(GLW_APIENTRY* uniformMatrixNdv)(glw::GLint, glw::GLsizei, glw::GLboolean,
															 const glw::GLdouble*);
	typedef GLW_APICALL void(GLW_APIENTRY* uniformMatrixNfv)(glw::GLint, glw::GLsizei, glw::GLboolean,
															 const glw::GLfloat*);

	/* Public static methods */
	/* UniformN*v routine getters */
	static uniformNdv getUniformNdv(const glw::Functions& gl, glw::GLuint n_rows);

	static uniformNfv getUniformNfv(const glw::Functions& gl, glw::GLuint n_rows);

	static uniformNiv getUniformNiv(const glw::Functions& gl, glw::GLuint n_rows);

	static uniformNuiv getUniformNuiv(const glw::Functions& gl, glw::GLuint n_rows);

	static uniformMatrixNdv getUniformMatrixNdv(const glw::Functions& gl, glw::GLuint n_columns, glw::GLuint n_rows);

	static uniformMatrixNfv getUniformMatrixNfv(const glw::Functions& gl, glw::GLuint n_columns, glw::GLuint n_rows);

	/* GLSL qualifiers */
	static bool doesContainQualifier(QUALIFIERS qualifier, const qualifierSet& qualifiers);

	static bool doesStageSupportQualifier(SHADER_STAGES stage, VARIABLE_STORAGE storage, QUALIFIERS qualifier);

	static const glw::GLchar* getQualifierString(QUALIFIERS qualifier);
	static std::string getQualifiersListString(const qualifierSet& qualifiers);

	static qualifierSet prepareQualifiersSet(const qualifierSet& in_qualifiers, SHADER_STAGES stage,
											 VARIABLE_STORAGE storage);

	/* Variable name */
	static std::string getBlockVariableDefinition(const Utils::qualifierSet& qualifiers, const glw::GLchar* type_name,
												  const glw::GLchar* variable_name);

	static std::string getBlockVariableReference(VARIABLE_FLAVOUR flavour, const glw::GLchar* variable_name,
												 const glw::GLchar* block_name);

	static std::string getVariableDefinition(VARIABLE_FLAVOUR flavour, const Utils::qualifierSet& qualifiers,
											 const glw::GLchar* type_name, const glw::GLchar* variable_name);

	static VARIABLE_FLAVOUR getVariableFlavour(Utils::SHADER_STAGES stage, Utils::VARIABLE_STORAGE storage,
											   const Utils::qualifierSet& qualifiers);

	static std::string getVariableName(Utils::SHADER_STAGES stage, Utils::VARIABLE_STORAGE storage,
									   const glw::GLchar* variable_name);

	static std::string getVariableReference(VARIABLE_FLAVOUR flavour, const glw::GLchar* variable_name);

	static void prepareBlockVariableStrings(Utils::SHADER_STAGES in_stage, Utils::VARIABLE_STORAGE in_storage,
											const Utils::qualifierSet& in_qualifiers, const glw::GLchar* in_type_name,
											const glw::GLchar* in_variable_name, const glw::GLchar* in_block_name,
											std::string& out_definition, std::string& out_reference);

	static void prepareVariableStrings(Utils::SHADER_STAGES in_stage, Utils::VARIABLE_STORAGE in_storage,
									   const Utils::qualifierSet& in_qualifiers, const glw::GLchar* in_type_name,
									   const glw::GLchar* in_variable_name, std::string& out_definition,
									   std::string& out_reference);

	/* Textures */
	static const glw::GLchar* getImageType(TEXTURE_TYPES type);
	static glw::GLuint getNumberOfCoordinates(TEXTURE_TYPES type);
	static const glw::GLchar* getSamplerType(TEXTURE_TYPES type);
	static glw::GLenum getTextureTartet(TEXTURE_TYPES type);
	static const glw::GLchar* getTextureTypeName(TEXTURE_TYPES type);

	/* Stuff */
	static bool checkUniformBinding(Utils::program& program, const glw::GLchar* name, glw::GLint expected_binding);
	static bool checkUniformArrayBinding(Utils::program& program, const glw::GLchar* name, glw::GLuint index,
										 glw::GLint expected_binding);
	static bool doesTypeSupportMatrix(TYPES type);
	static const glw::GLchar* getShaderStageName(SHADER_STAGES stage);
	static const glw::GLchar* getTypeName(TYPES type, glw::GLuint n_columns, glw::GLuint n_rows);
	static const glw::GLchar* getUtf8Character(UTF8_CHARACTERS character);
	static bool isExtensionSupported(deqp::Context& context, const glw::GLchar* extension_name);
	static bool isGLVersionAtLeast(const glw::Functions& gl, glw::GLint required_major, glw::GLint required_minor);
	static void replaceToken(const glw::GLchar* token, size_t& search_position, const glw::GLchar* text,
							 std::string& string);
	static void replaceAllTokens(const glw::GLchar* token, const glw::GLchar* text, std::string& string);
};

/** Base class for tests **/
class TestBase : public deqp::TestCase
{
public:
	/* Public methods */
	TestBase(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description);
	virtual ~TestBase()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

protected:
	/* Methods to be implemented by child class */
	virtual void getShaderSourceConfig(glw::GLuint& out_n_parts, bool& out_use_lengths);
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source) = 0;
	virtual void prepareUniforms(Utils::program& program);
	virtual bool testInit();
	virtual bool testCompute()						 = 0;
	virtual bool testDrawArray(bool use_version_400) = 0;

	/* Methods available to child classes */
	const glw::GLchar* getStageSpecificLayout(Utils::SHADER_STAGES stage) const;
	const glw::GLchar* getVersionString(Utils::SHADER_STAGES stage, bool use_version_400) const;
	void initShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400, Utils::shaderSource& out_source);
	int maxImageUniforms(Utils::SHADER_STAGES stage) const;

	/* Protected variables */
	bool m_is_compute_shader_supported;
	bool m_is_explicit_uniform_location;
	bool m_is_shader_language_420pack;

private:
	/* Private methods */
	bool test();
};

/** Base class for API tests */
class APITestBase : public TestBase
{
public:
	/* Public methods */
	APITestBase(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description);

	virtual ~APITestBase()
	{
	}

protected:
	/* Protected methods inherited from TestBase */
	virtual bool testCompute();
	virtual bool testDrawArray(bool use_version_400);

	/* Protected methods to be implemented by child class */
	virtual bool checkResults(Utils::program& program) = 0;

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source) = 0;
};

/** Base class for GLSL tests **/
class GLSLTestBase : public TestBase
{
public:
	/* Public methods */
	GLSLTestBase(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description);

	virtual ~GLSLTestBase()
	{
	}

protected:
	/* Protected methods inherited from TestBase */
	virtual bool testCompute();
	virtual bool testDrawArray(bool use_version_400);

	/* Protected methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source) = 0;

	virtual const glw::GLchar* prepareSourceTexture(Utils::texture& texture);

	virtual void prepareVertexBuffer(const Utils::program& program, Utils::buffer& buffer, Utils::vertexArray& vao);

	virtual bool verifyAdditionalResults() const;
	virtual void releaseResource();

private:
	/* Private methods */
	void bindTextureToimage(Utils::program& program, Utils::texture& texture, const glw::GLchar* uniform_name) const;

	void bindTextureToSampler(Utils::program& program, Utils::texture& texture, const glw::GLchar* uniform_name) const;

	bool checkResults(Utils::texture& color_texture) const;

	void prepareFramebuffer(Utils::framebuffer& framebuffer, Utils::texture& color_texture) const;

	void prepareImage(Utils::program& program, Utils::texture& color_texture) const;

	/* Private constants */
	static const glw::GLenum m_color_texture_internal_format;
	static const glw::GLenum m_color_texture_format;
	static const glw::GLenum m_color_texture_type;
	static const glw::GLuint m_color_texture_width;
	static const glw::GLuint m_color_texture_height;
};

/** Base class for negative tests **/
class NegativeTestBase : public TestBase
{
public:
	/* Public methods */
	NegativeTestBase(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description);

	virtual ~NegativeTestBase()
	{
	}

protected:
	/* Protected methods inherited from TestBase */
	virtual bool testCompute();
	virtual bool testDrawArray(bool use_version_400);

	/* Protected methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source) = 0;
};

/** Base class for "binding image" tests **/
class BindingImageTest : public GLSLTestBase
{
public:
	/* Public methods */
	BindingImageTest(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description);

	virtual ~BindingImageTest()
	{
	}

protected:
	/* Protected methods */
	void prepareBuffer(Utils::buffer& buffer, glw::GLuint color);

	void prepareTexture(Utils::texture& texture, const Utils::buffer& buffer, Utils::TEXTURE_TYPES texture_type,
						glw::GLuint color, glw::GLuint unit);

	bool verifyBuffer(const Utils::buffer& buffer) const;
	bool verifyTexture(const Utils::texture& texture) const;

	/* Protected constants */
	static const glw::GLuint m_width;
	static const glw::GLuint m_green_color;
	static const glw::GLuint m_height;
	static const glw::GLuint m_depth;
};

/** Test implementation, description follows:
 *
 * GLSL Tests:
 *
 *   * Unix-style new line continuation:
 *
 *    Run test with shader that contains line continuation and Unix-style (LF)
 *    new line characters inside:
 *
 *     - assignment expression (after and before '=' operator)
 *
 *     - vector variable initializer (after ',' in contructor)
 *
 *     - tokens (inside function name, inside type name, inside variable name).
 *       These tokens are later used to generate some color value, that
 *       is later verifed.
 *
 *     - preprocessor (#define) syntax  - inside and in between preprocessor
 *       tokens. These tokens are later used to generate some color value,
 *       that is later verifed.
 *
 *     - comments
 *
 *    For example test for line continuation inside preprocessor tokens may use
 *    following GLSL code:
 *
 *        #define ADD_ONE(XX) (X\\
         *        X + 1.0)
 *        vec4 getColor(float f) {
 *            return vec4(0.0, ADD_ONE(0.0), 0.0, 1.0);
 *        }
 *
 *     Where returned color is verified agains some reference value.
 *
 *
 *   * DOS-style line continuation:
 *
 *    Run same test with line continuation sign before DOS-style (CR+LF) new
 *    line character.
 *
 *
 *   * Multiple line continuations in same GLSL token:
 *
 *    Run test with shader that contains multiple (3 or more) line
 *    continuation and newlines inside same GLSL tokens (function or variable
 *    names).
 *
 *
 *   * Line continuation near GLSL shader source null-termination:
 *
 *    Run test with shader that contains line continuation character as the
 *    last character in null terminated shader string.
 *
 *
 *   * Line continuation near GLSL shader source end:
 *
 *    Run test with shader that contains line continuation character as the
 *    last character in not null terminated shader string (shader source length
 *    parameter is specified in glShaderSource call).
 *
 *
 *   * Line continuation near end of GLSL shader source string:
 *
 *    Run test with shader constructed by multple strings passed to
 *    glShaderSource. New line continuation characters placed as:
 *
 *     - last character of passed null terminated string
 *     - next-to-last character of passed null terminated string,
 *       followed by newline
 *     - last character of passed not null terminated string
 *     - next-to-last character of passed not null terminated string,
 *       followed by newline
 *
 *     Each string with line continuation should be followed by a next,
 *     non-empty string.
 **/
class LineContinuationTest : public GLSLTestBase
{
public:
	/* Public methods */
	LineContinuationTest(deqp::Context&);

	virtual ~LineContinuationTest()
	{
	}

protected:
	/* Protected methods inherited from GLSLTestBase */
	virtual void getShaderSourceConfig(glw::GLuint& out_n_parts, bool& out_use_lengths);

	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual const glw::GLchar* prepareSourceTexture(Utils::texture& texture);

	virtual void prepareVertexBuffer(const Utils::program& program, Utils::buffer& buffer, Utils::vertexArray& vao);

private:
	/* Private enums */
	enum CASES
	{
		ASSIGNMENT_BEFORE_OPERATOR = 0,
		ASSIGNMENT_AFTER_OPERATOR,
		VECTOR_VARIABLE_INITIALIZER,
		TOKEN_INSIDE_FUNCTION_NAME,
		TOKEN_INSIDE_TYPE_NAME,
		TOKEN_INSIDE_VARIABLE_NAME,
		PREPROCESSOR_TOKEN_INSIDE,
		PREPROCESSOR_TOKEN_BETWEEN,
		COMMENT,
		SOURCE_TERMINATION_NULL,
		SOURCE_TERMINATION_NON_NULL,
		PART_TERMINATION_NULL,
		PART_NEXT_TO_TERMINATION_NULL,
		PART_TERMINATION_NON_NULL,
		PART_NEXT_TO_TERMINATION_NON_NULL,

		/* DEBUG: there will be no line continuations at all */
		DEBUG_CASE
	};

	enum REPETITIONS
	{
		ONCE = 0,
		MULTIPLE_TIMES,
	};

	enum LINE_ENDINGS
	{
		UNIX = 0,
		DOS,
	};

	/* Private types */
	/** Declare test case
	 *
	 **/
	struct testCase
	{
		glw::GLuint m_case;
		glw::GLuint m_repetitions;
		glw::GLuint m_line_endings;
	};

	/* Private methods */
	const glw::GLchar* casesToStr(CASES cases) const;
	const glw::GLchar* getExpectedValueString() const;
	std::string		   getLineContinuationString() const;
	bool			   isShaderMultipart() const;
	const glw::GLchar* lineEndingsToStr(LINE_ENDINGS line_ending) const;
	void prepareComputShaderSource(Utils::shaderSource& shaderSource);

	void prepareShaderSourceForDraw(Utils::SHADER_STAGES stage, bool use_version_400, Utils::shaderSource& source);

	const glw::GLchar* repetitionsToStr(REPETITIONS repetitions) const;
	void replaceAllCaseTokens(std::string& source) const;
	bool useSourceLengths() const;

	/* Private constants */
	static const glw::GLuint  m_n_repetitions;
	static const glw::GLchar* m_texture_coordinates_name;

	/* Private variables */
	testCase m_test_case;
};

/** Test implementation, description follows:
 *
 * Correct numbering of lines with line continuations:
 *
 * Try to compile shader with line continuation schemes, followed
 * by __LINE__ macro capturing the current line number.
 * The value of __LINE__ is than validated against expected
 * constant. Expected value must account for continued lines,
 * for example in code below, they are two line continuations,
 * so the expected value is N - 2 (where N is the "raw" line number).
 *
 *     ivec4 glsl\\
         *     Test\\
         *     Func(float f) {
 *         obvious = error;
 *         return vec4(__LINE__, 0, 0, 1);
 *     }
 **/
class LineNumberingTest : public GLSLTestBase
{
public:
	/* Public methods */
	LineNumberingTest(deqp::Context&);

	virtual ~LineNumberingTest()
	{
	}

protected:
	/* Protected methods inherited from GLSLTestBase */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);
};

/** Test implementation, description follows:
 *
 * * UTF-8 characters in comments:
 *
 *   Run test with shader that contains non-ASCII UTF-8 characters in comments.
 *   Use 2-byte UTF-8 characters (Latin-1 supplements, Greek, Hebrew, Cyril),
 *   3-byte (Chinese/Japanese/Korean), 4-byte (less common CJK).
 *   Also test 5 and 6 byte codes.
 *   Also test base plane ASCII characters encoded with redundant bytes,
 *   such as 'a' or <whitespace> encoded by 4 bytes.
 *
 *   Encode UTF-8 strings manually in test, either by c-array or '\0xNN' escape
 *   sequences.
 *
 *
 *  * UTF-8 characters in preprocessor:
 *
 *   Run test with shader that contains non-ASCII UTF-8 characters (arbitrary
 *   from above) in preprocessor tokens. Use preprocessor to strip these UTF-8
 *   characters, so they does not occur in preprocessed GLSL shader source.
 *
 *
 *  * Incomplete UTF-8 near GLSL shader source null-termination:
 *
 *   Run test with shader that contains comment with incomplete UTF-8
 *   character as the last character in null terminated shader string.
 *
 *
 *  * Incomplete UTF-8 near GLSL shader source end:
 *
 *   Run test with shader that contains comment with incomplete UTF-8
 *   character as the last character in not-null terminated shader string.
 *   Shader source length parameter is specified in glShaderSource call.
 **/
class UTF8CharactersTest : public GLSLTestBase
{
public:
	/* Public methods */
	UTF8CharactersTest(deqp::Context&);

	virtual ~UTF8CharactersTest()
	{
	}

	/* Protected methods inherited from GLSLTestBase */
	virtual void getShaderSourceConfig(glw::GLuint& out_n_parts, bool& out_use_lengths);

	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual const glw::GLchar* prepareSourceTexture(Utils::texture& texture);

private:
	/* Private enums */
	enum CASES
	{
		IN_COMMENT = 0,
		IN_PREPROCESSOR,
		AS_LAST_CHARACTER_NULL_TERMINATED,
		AS_LAST_CHARACTER_NON_NULL_TERMINATED,

		DEBUG_CASE
	};

	/* Private types */
	struct testCase
	{
		CASES				   m_case;
		Utils::UTF8_CHARACTERS m_character;
	};

	/* Private methods */
	const glw::GLchar* casesToStr() const;

	/* Private variables */
	testCase m_test_case;
};

/** Test implementation, description follows:
 *
 * * UTF-8 in after preprocessor, in GLSL syntax:
 *
 *   Try to compile shader that contains non-ASCII UTF-8 character after
 *   preprocessing. Expect compilation error.
 **/
class UTF8InSourceTest : public NegativeTestBase
{
public:
	/* Public methods */
	UTF8InSourceTest(deqp::Context&);

	virtual ~UTF8InSourceTest()
	{
	}

	/* Protected methods inherited from GLSLTestBase */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

private:
	/* Private variables */
	Utils::UTF8_CHARACTERS m_character;
};

/** Test implementation, description follows:
 *
 * * Check all implicit conversions on function return:
 *
 *    Run test with shader that verifies value being return by following
 *    function:
 *
 *        T1 f(T2 x, T2 y) { return x + y; }'
 *
 *   By substituting T1 and T2 typenames check following conversions:
 *    - int to uint
 *    - int to float
 *    - uint to float
 *    - int to double
 *    - uint to double
 *    - float to double
 *   Use scalars and vector types (all vector sizes). For conversions not
 *   involving ints or uints test also matrix types (all matrix sizes)
 *
 *   Call this function on literals, constant expressions and variables
 *   (variables should contain values that cannot be constant folded during
 *   compilation).
 **/
class ImplicitConversionsValidTest : public GLSLTestBase
{
public:
	/* Public methods */
	ImplicitConversionsValidTest(deqp::Context&);

	virtual ~ImplicitConversionsValidTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual bool testInit();

private:
	/* Private types */
	struct typesPair
	{
		Utils::TYPES m_t1;
		Utils::TYPES m_t2;
	};

	struct testCase
	{
		typesPair   m_types;
		glw::GLuint m_n_cols;
		glw::GLuint m_n_rows;
	};

	/* Private methods */
	const testCase& getCurrentTestCase();

	std::string getValueList(glw::GLuint n_columns, glw::GLuint n_rows);

	/* Private variables */
	testCase			  m_debug_test_case;
	std::vector<testCase> m_test_cases;
	glw::GLuint			  m_current_test_case_index;
};

/** Test implementation, description follows:
 *
 * * Check if uint to int conversion is forbidden:
 *
 *   Try to compile shader that returns uint value from function returning int.
 *   Expect shader compilation error. Use scalars and vector types.
 **/
class ImplicitConversionsInvalidTest : public NegativeTestBase
{
public:
	/* Public methods */
	ImplicitConversionsInvalidTest(deqp::Context&);

	virtual ~ImplicitConversionsInvalidTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

private:
	/* Private methods */
	std::string getValueList(glw::GLuint n_rows);

	/* Private variables */
	glw::GLuint m_current_test_case_index;
};

/** Test implementation, description follows:
 *
 * * Read-only variables:
 *
 *   Run shader which contains and uses following read-only variables:
 *    const float c1 = X1;
 *    const vec4 c2 = X2;
 *    const mat2 c3 = X3;
 *    const S c4 = X4;
 *    const vec4 c5[15] = X5;
 *
 *   Where X1..X5 are non-constant initializer expressions (expressions which
 *   cannot be constant folded).  S is a struct of scalar, vector and matrix
 *   transparent types. Verify value of each read-only variable.
 **/
class ConstDynamicValueTest : public GLSLTestBase
{
public:
	/* Public methods */
	ConstDynamicValueTest(deqp::Context&);

	virtual ~ConstDynamicValueTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
};

/** Test implementation, description follows:
 *
 * * Override value of read-only variable:
 *
 *   Try to compile shaders, that override value of constant variable.
 *   Use constant variable defined as:
 *     const float c1 = X1;
 *
 *   Where X1 is once a literal initializer and in another shader is a
 *   non-const-foldable non-constant variable.
 *
 *   Variable is non-const-foldable when it's value cannot be deduced during
 *   shader compilation. (As an example uniforms and varyings are non const
 *   foldable).
 *
 *   Expect compilation errors on any assignment to such variable.
 **/
class ConstAssignmentTest : public NegativeTestBase
{
public:
	/* Public methods */
	ConstAssignmentTest(deqp::Context&);

	virtual ~ConstAssignmentTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

private:
	/* Private variables */
	glw::GLuint m_current_test_case_index;
};

/** Test implementation, description follows:
 *
 * * Read-only variable use in place of constant expression:
 *
 *   Try to compile shader, that tries to force constant folding on const
 *   variable, when constant variable was initialized with non-constant,
 *   non const foldable expression. For example:
 *
 *   vec4 glslTestFunc(float f) {
 *        const float fConst1 = f;
 *        float a[f]; //force constant folding of f.
 *        return vec4(a[0]);
 *    }
 *    ...
 *    glslTestFunc(gl_FragCoord.x);
 **/
class ConstDynamicValueAsConstExprTest : public NegativeTestBase
{
public:
	/* Public methods */
	ConstDynamicValueAsConstExprTest(deqp::Context&);

	virtual ~ConstDynamicValueAsConstExprTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);
};

/** Test implementation, description follows:
 *
 * * Input variable qualifier order:
 *
 *   Run shader which uses input variable, that is declared with all
 *   permutations of following qualifiers:
 *
 *    storage qualifiers: in
 *    interpolation qualifiers: (none), flat, noperespective, smooth
 *    auxaliary qualifiers: (none), patch, sample, centroid
 *    precision qualifiers: (none), precise
 *    invariance qualifiers: (none), invariant
 *    layout qualifiers: (none), layout(location = 0)
 *
 *   Test fragment, tessellation evaluation, tessellation control and geometry
 *   shader inputs.  Skip illegal permutations: flat interpolation qualifier
 *   used with non empty auxaliary qualifier, patch qualifier outside
 *   tessellation shaders. Also skip non-flat interpolation qualifiers for
 *   vertex, tessellation and geometry shaders.
 *
 * * Input variable qualifers used multiple times:
 *
 *   Same as above, but use some qualifiers multiple times.
 *
 * * Output variable qualifier order:
 *   Run shader which uses output variable, that is declared with all
 *    permutations of following qualifiers:
 *
 *     storage qualifiers: out
 *     interpolation qualifiers: (none), flat, noperespective, smooth
 *     auxaliary qualifiers: (none), patch, sample, centroid
 *     precision qualifiers: (none), precise
 *     invariance qualifiers: (none), invariant
 *     layout qualifiers: (none), layout(location = 0)
 *
 *    All permutations above following sets should be used (so all combinations
 *    of qualifiers are tested and all orderings of such combinations are tested).
 *    Used shader input must match output from earlier shader stage.
 *
 *    Test tessellation evaluation, tessellation control, geometry and vertex
 *    shader inputs. Skip illegal permutations: flat interpolation qualifier used
 *    with non empty auxaliary qualifier, patch qualifier outside tessellation
 *    shaders.
 *
 *
 * * Output variable qualifers used multiple times:
 *
 *   Same as above, but use some qualifiers multiple times.
 **/
class QualifierOrderTest : public GLSLTestBase
{
public:
	/* Public methods */
	QualifierOrderTest(deqp::Context&);

	virtual ~QualifierOrderTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareVertexBuffer(const Utils::program& program, Utils::buffer& buffer, Utils::vertexArray& vao);

	virtual bool testInit();

private:
	/* Private methods */
	const Utils::qualifierSet& getCurrentTestCase();

	/* Private varaibles */
	std::vector<Utils::qualifierSet> m_test_cases;
	glw::GLuint						 m_current_test_case_index;
};

/** Test implementation, description follows:
 *
 * * Input block interface qualifier order:
 *
 *   Run shaders with same variable qualifications as above used for input
 *   interface block member.
 *
 *    Use following block declaration:
 *      in BLOCK {
 *          vec4 color;
 *      };
 *
 *   Test fragment shader, tessellation evaluation, tessellation control and
 *   geometry shader inputs. Skip illegal permutations, same as in previous
 *   test cases.
 *
 *
 * * Input block interface qualifers used multiple times:
 *
 *   Same as above, but use some qualifiers multiple times.
 *
 * * Output block interface qualifier order:
 *   Run shaders with same variable qualifications as above used for output
 *   interface block member.
 *
 *   Use following block declaration:
 *     out BLOCK {
 *         vec4 color;
 *     };
 *
 *   Test tessellation evaluation, tessellation control, geometry and vertex
 *   shader outputs. Skip illegal permutations, same as in previous test case.
 *
 *
 * * Output block interface qualifers used multiple times:
 *
 *   Same as above, but use some qualifiers multiple times.
 **/
class QualifierOrderBlockTest : public GLSLTestBase
{
public:
	/* Public methods */
	QualifierOrderBlockTest(deqp::Context&);

	virtual ~QualifierOrderBlockTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareVertexBuffer(const Utils::program& program, Utils::buffer& buffer, Utils::vertexArray& vao);

	virtual bool testInit();

private:
	/* Private methods */
	const Utils::qualifierSet& getCurrentTestCase();

	/* Private varaibles */
	std::vector<Utils::qualifierSet> m_test_cases;
	glw::GLuint						 m_current_test_case_index;
};

/** Test implementation, description follows:
 *
 * * Uniform variable qualifier order:
 *
 *   Run shaders which use uniform, that is declared with all permutations of
 *   'precise', 'uniform', and 'layout(...)' qualifiers.
 *
 *
 * * Uniform qualifers used multiple times:
 *
 *   Same as above, but use some qualifiers multiple times.
 **/
class QualifierOrderUniformTest : public GLSLTestBase
{
public:
	/* Public methods */
	QualifierOrderUniformTest(deqp::Context&);

	virtual ~QualifierOrderUniformTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);

	virtual bool testInit();

private:
	/* Private methods */
	const Utils::qualifierSet& getCurrentTestCase();

	/* Private varaibles */
	std::vector<Utils::qualifierSet> m_test_cases;
	glw::GLuint						 m_current_test_case_index;
};

/** Test implementation, description follows:
 *
 * * Function inout parameter qualifier order:
 *
 *   Run shaders which use function, that has inout parameter declared with all
 *   permutations of 'lowp' or 'mediump' or 'highp', 'precise' qualifiers.
 *
 *   Also run with some qualifiers used multiple times.
 **/
class QualifierOrderFunctionInoutTest : public GLSLTestBase
{
public:
	/* Public methods */
	QualifierOrderFunctionInoutTest(deqp::Context&);

	virtual ~QualifierOrderFunctionInoutTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual bool testInit();

private:
	/* Private methods */
	const Utils::qualifierSet& getCurrentTestCase();

	/* Private varaibles */
	std::vector<Utils::qualifierSet> m_test_cases;
	glw::GLuint						 m_current_test_case_index;
};

/** Test implementation, description follows:
 *
 * * Function input parameter qualifier order:
 *
 *   Run shaders which use function, that has 'in' parameter declared with all
 *   permutations of 'in', 'lowp' or 'mediump' or 'highp', 'precise', 'const'
 *   qualifiers.
 *
 *   Also run with some qualifiers used multiple times.
 **/
class QualifierOrderFunctionInputTest : public GLSLTestBase
{
public:
	/* Public methods */
	QualifierOrderFunctionInputTest(deqp::Context&);

	virtual ~QualifierOrderFunctionInputTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual bool testInit();

private:
	/* Private methods */
	const Utils::qualifierSet& getCurrentTestCase();

	/* Private varaibles */
	std::vector<Utils::qualifierSet> m_test_cases;
	glw::GLuint						 m_current_test_case_index;
};

/** Test implementation, description follows:
 *
 * * Function output parameter qualifier order:
 *
 *   Run shaders which use function, that has out parameter declared with all
 *   permutations of  'lowp' or 'mediump' or 'highp', 'precise' qualifiers.
 *
 *   Also run with some qualifiers used multiple times.
 **/
class QualifierOrderFunctionOutputTest : public GLSLTestBase
{
public:
	/* Public methods */
	QualifierOrderFunctionOutputTest(deqp::Context&);

	virtual ~QualifierOrderFunctionOutputTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual bool testInit();

private:
	/* Private methods */
	const Utils::qualifierSet& getCurrentTestCase();

	/* Private varaibles */
	std::vector<Utils::qualifierSet> m_test_cases;
	glw::GLuint						 m_current_test_case_index;
};

/** Test implementation, description follows:
 *
 * * Input variable layout qualifiers override:
 *
 *   Run shaders which use input variable, qualified with multiple layout
 *   qualifiers. For example:
 *
 *       layout(location = 3) layout(location = 2) out vec4 gColor
 *
 *
 * * Geometry shader layout qualifiers override:
 *
 *   Run shader which use multiple global geometry shader qualifiers.
 *   For example:
 *
 *       layout( triangle_strip, max_vertices = 2 ) layout( max_vertices = 3) out;'
 *
 *
 * * Tesselation shader layout qualifiers override:
 *
 *   Run shader which use multiple tesselation shader qualifiers, for example:
 *
 *       layout(vertices = 2) layout(vertices = 4) out;
 **/
class QualifierOverrideLayoutTest : public GLSLTestBase
{
public:
	/* Public methods */
	QualifierOverrideLayoutTest(deqp::Context&);

	virtual ~QualifierOverrideLayoutTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareVertexBuffer(const Utils::program& program, Utils::buffer& buffer, Utils::vertexArray& vao);
};

/** Test implementation, description follows:
 *
 * * 'binding' qualified used for uniform block:
 *
 *   Create shader program which uses uniform block declaration
 *   with 'binding' layout qualifier specified. For example:
 *
 *        layout(std140, binding = 2) uniform BLOCK {
 *           vec4 color;
 *        } block;
 *
 *   Bind filled uniform buffer object to binding point 2.
 *
 *   Run shader program, validate uniform buffer contents in shader.
 *
 *
 * * 'binding' layout qualifier used for multiple uniform blocks in same shader:
 *
 *   Same as above, but use multiple uniform block declarations, each with
 *   different 'layout(binding = X)' qualifier. Validate contents of all
 *   uniform buffers in shader.
 *
 *
 * * 'binding' layout qualifier used for uniform block in different shader
 *    stages:
 *
 *   Link multiple shaders of different stage that use same uniform block.
 *   All uniform block declarations use same 'binding' layout qualifier.
 *
 *   Validate contents of uniform buffer in all shader stages.
 **/
class BindingUniformBlocksTest : public GLSLTestBase
{
public:
	/* Public methods */
	BindingUniformBlocksTest(deqp::Context&);

	virtual ~BindingUniformBlocksTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual void releaseResource();

private:
	/* Private variables */
	Utils::buffer m_goku_buffer;
	Utils::buffer m_vegeta_buffer;
	Utils::buffer m_children_buffer;
};

/** Test implementation, description follows:
 *
 * * 'binding' layout qualifier used only once for same uniform block in
 *   different shader stages:

 *   Link multiple shaders of different stage that use same uniform block.
 *   'binding' layout qualifier is used only in one shader stage, other shader
 *   stages does not specify layout qualifier.
 *
 *   Validate contents of uniform buffer in all shader stages.
 **/
class BindingUniformSingleBlockTest : public GLSLTestBase
{
public:
	/* Public methods */
	BindingUniformSingleBlockTest(deqp::Context&);

	virtual ~BindingUniformSingleBlockTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual void releaseResource();

private:
	/* Private variables */
	Utils::buffer		 m_goku_buffer;
	Utils::SHADER_STAGES m_test_stage;
};

/** Test implementation, description follows:
 *
 * * 'binding' layout qualifier used with uniform block array.
 *
 *   Create shader program which uses uniform block array, with 'binding'
 *   layout qualifier specified, example:
 *
 *       layout(std140, binding = 2) uniform BLOCK {
 *          vec4 color;
 *       } block[14];
 *
 *   Bind filled uniform buffer objects to binding points 2..16. Validate
 *   contents of all uniform buffers in shader.
 *
 * * bindings of array of uniform blocks:
 *
 *   Check if uniform buffer array elements automatically get subsequent
 *   binding values, when their interface is specified using 'binding'
 *   layout qualifier. Use glGetActiveUniformBlockiv.
 **/
class BindingUniformBlockArrayTest : public GLSLTestBase
{
public:
	/* Public methods */
	BindingUniformBlockArrayTest(deqp::Context&);

	virtual ~BindingUniformBlockArrayTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual void releaseResource();

private:
	/* Private methods */
	void checkBinding(Utils::program& program, glw::GLuint index, glw::GLint expected_binding);

	/* Private variables */
	Utils::buffer m_goku_00_buffer;
	Utils::buffer m_goku_01_buffer;
	Utils::buffer m_goku_02_buffer;
	Utils::buffer m_goku_03_buffer;
	Utils::buffer m_goku_04_buffer;
	Utils::buffer m_goku_05_buffer;
	Utils::buffer m_goku_06_buffer;
	Utils::buffer m_goku_07_buffer;
	Utils::buffer m_goku_08_buffer;
	Utils::buffer m_goku_09_buffer;
	Utils::buffer m_goku_10_buffer;
	Utils::buffer m_goku_11_buffer;
	Utils::buffer m_goku_12_buffer;
	Utils::buffer m_goku_13_buffer;
};

/** Test implementation, description follows:
 *
 * * Default binding value:
 *
 *   Create shader program, with uniform buffer interface declared without
 *   'binding' layout qualifier. Use glGetActiveUniformBlockiv to test if
 *   default 'binding' value is 0.
 **/
class BindingUniformDefaultTest : public APITestBase
{
public:
	/* Public methods */
	BindingUniformDefaultTest(deqp::Context&);

	virtual ~BindingUniformDefaultTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool checkResults(Utils::program& program);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);
};

/** Test implementation, description follows:
 *
 * * Override binding value from API:
 *
 *   Create a shader program with uniform buffer interface declared with
 *   'layout(..., binding = 3)'. Use glUniformBlockBinding to change binding
 *   value to 11. Test if binding point 11 is now used during rendering.
 *   Test if binding point 11 is returned when enumerating interface with
 *   glGetActiveUniformBlockiv.
 **/
class BindingUniformAPIOverirdeTest : public GLSLTestBase
{
public:
	/* Public methods */
	BindingUniformAPIOverirdeTest(deqp::Context&);

	virtual ~BindingUniformAPIOverirdeTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual void releaseResource();

private:
	/* Private variables */
	Utils::buffer m_goku_buffer;
};

/** Test implementation, description follows:
 *
 * * 'binding' layout qualifier used with global uniform
 *
 *   Use 'binding' layout qualifier on global (default block) uniform.
 *   Expect shader compilation error.
 **/
class BindingUniformGlobalBlockTest : public NegativeTestBase
{
public:
	/* Public methods */
	BindingUniformGlobalBlockTest(deqp::Context&);

	virtual ~BindingUniformGlobalBlockTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);
};

/** Test implementation, description follows:
 *
 * * Wrong value for 'binding' layout qualifier.
 *
 *   Use -1, variable name, 'std140' as binding value.
 *   Expect shader compilation error in each case.
 *
 * * Missing value for 'binding' layout qualifier.
 *
 *   Expect shader compilation error in following declaration:
 *
 *       layout(std140, binding) uniform BLOCK {
 *          vec4 color;
 *       } block[14];
 **/
class BindingUniformInvalidTest : public NegativeTestBase
{
public:
	/* Public methods */
	BindingUniformInvalidTest(deqp::Context&);

	virtual ~BindingUniformInvalidTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

private:
	/* Private enums */
	enum TESTCASES
	{
		NEGATIVE_VALUE,
		VARIABLE_NAME,
		STD140,
		MISSING,

		/* */
		TEST_CASES_MAX
	};

	/* Private methods */
	const glw::GLchar* getCaseString(TESTCASES test_case);

	/* Provate variables */
	TESTCASES m_case;
};

/** Test implementation, description follows:
 *
 * * 'binding' qualified used for sampler uniform:
 *
 *   Create shader program which uses sampler uniform declaration with
 *   'binding' layout qualifier specified. For example:
 *
 *        layout(binding = 2) uniform sampler2D s;
 *
 *   Bind 2D texture to texture unit GL_TEXTURE2.
 *
 *   Run shader program, validate binding by sampling from texture in shader.
 *
 *
 * * 'binding' layout qualifier used for multiple sampler uniforms in same
 *   shader:
 *
 *   Same as above, but use multiple sampler uniform declarations, each with
 *   different 'layout(binding = X)' qualifier. Validate bindings of all
 *   samplers by sampling textures in shader.
 *
 *
 * * 'binding' layout qualifier used for sampler uniform in different shader
 *   stages:
 *
 *
 *   Link multiple shaders of different stages that use same sampler uniform.
 *   All sampler uniform declarations use same 'binding' layout qualifier.
 *
 *   Validate binding of sampler by sampling texture in shader.
 *
 * * 'binding layout qualifier used with sampler uniforms of various types.
 *
 *   Create shader program which uses samplers of type: samplerBuffer,
 *   sampler2D, sampler2DRect, sampler2DArray, sampler3D, samplerCubeMap,
 *   sampler1D, sampler1DArray.
 *
 *   Each sampler declaration uses 'binding' qualifier with different value.
 *
 *   Validate bindings of all samplers by sampling bound textures in shader.
 **/
class BindingSamplersTest : public GLSLTestBase
{
public:
	/* Public methods */
	BindingSamplersTest(deqp::Context&);

	virtual ~BindingSamplersTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual void releaseResource();

private:
	/* Private methods */
	void prepareTexture(Utils::texture& texture, Utils::TEXTURE_TYPES texture_type, glw::GLuint color);

	/* Private variables */
	Utils::texture		 m_goku_texture;
	Utils::texture		 m_vegeta_texture;
	Utils::texture		 m_trunks_texture;
	Utils::buffer		 m_buffer;
	Utils::TEXTURE_TYPES m_test_case;
};

/** Test implementation, description follows:
 *
 * * 'binding' layout qualifier used only once for same sampler uniform in
 *   different shader stages:
 *
 *   Link multiple shaders of different stages that use same sampler uniform.
 *   'binding' layout qualifier is used only in one shader stage, other shader
 *   stages does not specify layout qualifier.
 *
 *   Validate binding of sampler by sampling texture in all shader stages.
 **/
class BindingSamplerSingleTest : public GLSLTestBase
{
public:
	/* Public methods */
	BindingSamplerSingleTest(deqp::Context&);

	virtual ~BindingSamplerSingleTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual void releaseResource();

private:
	/* Private variables */
	Utils::texture		 m_goku_texture;
	Utils::SHADER_STAGES m_test_stage;
};

/** Test implementation, description follows:
 *
 * * 'binding' layout qualifier used with sampler uniform array.
 *
 *   Create shader program which uses sampler uniform array, with 'binding'
 *   layout qualifier specified, example:
 *
 *       layout(binding = 2) uniform sampler2D s[7];
 *
 *   Bind textures to texture units 2..9. Validate bindings of all samplers
 *   by sampling bound textures in shader.
 *
 * * bindings of array of sampler uniforms
 *
 *   Check if sampler uniform array elements automatically get subsequent
 *   binding values, when their interface is specified using 'binding'
 *   layout qualifier. Use glGetUniformiv.
 **/
class BindingSamplerArrayTest : public GLSLTestBase
{
public:
	/* Public methods */
	BindingSamplerArrayTest(deqp::Context&);

	virtual ~BindingSamplerArrayTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual void releaseResource();

private:
	/* Private methods */
	void checkBinding(Utils::program& program, glw::GLuint index, glw::GLint expected_binding);

	/* Private variables */
	Utils::texture m_goku_00_texture;
	Utils::texture m_goku_01_texture;
	Utils::texture m_goku_02_texture;
	Utils::texture m_goku_03_texture;
	Utils::texture m_goku_04_texture;
	Utils::texture m_goku_05_texture;
	Utils::texture m_goku_06_texture;
};

/** Test implementation, description follows:
 *
 * * Default binding value:
 *
 *   Create shader program, with sampler uniform declared without 'binding'
 *   layout qualifier. Use glGetUniformiv to test, if default 'binding' value
 *   is 0.
 **/
class BindingSamplerDefaultTest : public APITestBase
{
public:
	/* Public methods */
	BindingSamplerDefaultTest(deqp::Context&);

	virtual ~BindingSamplerDefaultTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool checkResults(Utils::program& program);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);
};

/** Test implementation, description follows:
 *
 * * Override binding value from API:
 *
 *   Create a shader program with sampler uniform buffer declared with
 *   'layout(binding = 3)'. Use glUniform1i to change binding value to 11.
 *   Test if binding point 11 is now used during rendering.
 *   Test if binding point 11 is returned querying interface with
 *   glGetUniformiv.
 **/
class BindingSamplerAPIOverrideTest : public GLSLTestBase
{
public:
	/* Public methods */
	BindingSamplerAPIOverrideTest(deqp::Context&);

	virtual ~BindingSamplerAPIOverrideTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual void releaseResource();

private:
	/* Private variables */
	Utils::texture m_goku_texture;
};

/** Test implementation, description follows:
 *
 * * Wrong value for 'binding' layout qualifier.
 *
 *   Use -1 or variable name as binding value. Expect shader compilation
 *   error in each case.
 *
 *
 * * Missing value for 'binding' layout qualifier.
 *
 *   Expect shader compilation error in following declaration:
 *
 *       layout(binding) uniform sampler2D s;
 **/
class BindingSamplerInvalidTest : public NegativeTestBase
{
public:
	/* Public methods */
	BindingSamplerInvalidTest(deqp::Context&);
	virtual ~BindingSamplerInvalidTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

private:
	/* Private enums */
	enum TESTCASES
	{
		NEGATIVE_VALUE,
		VARIABLE_NAME,
		STD140,
		MISSING,

		/* */
		TEST_CASES_MAX
	};

	/* Private methods */
	const glw::GLchar* getCaseString(TESTCASES test_case);

	/* Provate variables */
	TESTCASES m_case;
};

/** Test implementation, description follows:
 *
 * * 'binding' qualified used for image uniform:
 *
 *   Create shader program which uses image uniform declaration with
 *   'binding' layout qualifier specified. For example:
 *
 *       layout(rgba32f, binding = 2) image2D i;
 *
 *   Bind 2D texture to image unit 2.
 *
 *   Run shader program, validate binding by storing values to image in shader.
 *
 *
 * * 'binding' layout qualifier used for multiple image uniforms in same
 *   shader:
 *
 *   Same as above, but use multiple image uniform declarations, each with
 *   different 'layout(binding = X)' qualifier. Validate bindings of all
 *   samplers by storing values to textures in shader.
 *
 *
 * * 'binding' layout qualifier used for image uniform in different shader
 *   stages:
 *
 *   Link multiple shaders of different stages that use same image uniform.
 *   All uniform uniform declarations use same 'binding' layout qualifier.
 *
 *   Validate binding of image uniform by storing values to image in shader.
 *
 *
 * * 'binding' layout qualifier used with image uniforms of various types.
 *
 *   Create shader program which uses samplers of type: imageBuffer,
 *   image2D, image2DRect, image2DArray, image3D, imageCubeMap,
 *   image1D, image1DArray.
 *
 *   Each image declaration uses 'binding' qualifier with different value.
 *
 *   Validate bindings of all image uniforms by storing values to textures
 *   in shader.
 **/
class BindingImagesTest : public BindingImageTest
{
public:
	/* Public methods */
	BindingImagesTest(deqp::Context&);

	virtual ~BindingImagesTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual bool verifyAdditionalResults() const;
	virtual void releaseResource();

private:
	/* Private variables */
	Utils::texture		 m_goku_texture;
	Utils::texture		 m_vegeta_texture;
	Utils::texture		 m_trunks_texture;
	Utils::buffer		 m_goku_buffer;
	Utils::buffer		 m_vegeta_buffer;
	Utils::buffer		 m_trunks_buffer;
	Utils::TEXTURE_TYPES m_test_case;

	/* Private constant */
	static const glw::GLuint m_goku_data;
	static const glw::GLuint m_vegeta_data;
	static const glw::GLuint m_trunks_data;
};

/** Test implementation, description follows:
 *
 * * 'binding' layout qualifier used only once for same image uniform in
 different shader stages:
 *
 *   Link multiple shaders of different stages that use same image uniform.
 *   'binding' layout qualifier is used only in one shader stage, other shader
 *   stages does not specify layout qualifier.
 *
 *   Validate binding of image uniform by storing values to image in shader.
 **/
class BindingImageSingleTest : public BindingImageTest
{
public:
	/* Public methods */
	BindingImageSingleTest(deqp::Context&);

	virtual ~BindingImageSingleTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual bool verifyAdditionalResults() const;
	virtual void releaseResource();

private:
	/* Private variables */
	Utils::texture		 m_goku_texture;
	Utils::SHADER_STAGES m_test_stage;
};

/** Test implementation, description follows:
 *
 * * 'binding' layout qualifier used with image uniform array.
 *
 *   Create shader program which uses image uniform array, with 'binding'
 *   layout qualifier specified, example:
 *
 *       layout(rgba32f, binding = 2) uniform image2D i[7];
 *
 *   Bind textures to image units 2..9. Validate bindings of all
 *   image uniforms by storing values to textures in shader.
 *
 *
 * * Bindings of array of image uniforms
 *
 *   Check if image uniform array elements automatically get subsequent
 *   binding values, when their interface is specified using 'binding'
 *   layout qualifier. Use glGetUniformiv.
 **/
class BindingImageArrayTest : public BindingImageTest
{
public:
	/* Public methods */
	BindingImageArrayTest(deqp::Context&);

	virtual ~BindingImageArrayTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual void releaseResource();

private:
	/* Private methods */
	void checkBinding(Utils::program& program, glw::GLuint index, glw::GLint expected_binding);

	/* Private variables */
	Utils::texture m_goku_00_texture;
	Utils::texture m_goku_01_texture;
	Utils::texture m_goku_02_texture;
	Utils::texture m_goku_03_texture;
	Utils::texture m_goku_04_texture;
	Utils::texture m_goku_05_texture;
	Utils::texture m_goku_06_texture;
};

/** Test implementation, description follows:
 *
 * * Default binding value:
 *
 *   Create shader program, with image uniform declared without 'binding'
 *   layout qualifier. Use glGetUniformiv to test if default 'binding' value
 *   is 0.
 **/
class BindingImageDefaultTest : public APITestBase
{
public:
	/* Public methods */
	BindingImageDefaultTest(deqp::Context&);

	virtual ~BindingImageDefaultTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool checkResults(Utils::program& program);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);
};

/** Test implementation, description follows:
 *
 * * Override binding value from API:
 *
 *   Create a shader program with image uniform buffer declared with
 *   'layout(binding = 3)'. Use glUniform1i to change binding value to 11.
 *   Test if binding point 11 is now used during rendering.
 *   Test if binding point 11 is returned querying interface with
 *   glGetUniformiv.
 **/
class BindingImageAPIOverrideTest : public BindingImageTest
{
public:
	/* Public methods */
	BindingImageAPIOverrideTest(deqp::Context&);

	virtual ~BindingImageAPIOverrideTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual void releaseResource();

private:
	/* Private variables */
	Utils::texture m_goku_texture;
};

/** Test implementation, description follows:
 *
 * * Wrong value for 'binding' layout qualifier.
 *
 *   Use -1, 'rgba32f' or variable name as binding value. Expect shader
 *   compilation error in each case.
 *
 *
 * * Missing value for 'binding' layout qualifier.
 *
 *   Expect shader compilation error in following declaration:
 *
 *      layout(rgba32f, binding) uniform image2D s;
 **/
class BindingImageInvalidTest : public NegativeTestBase
{
public:
	/* Public methods */
	BindingImageInvalidTest(deqp::Context&);

	virtual ~BindingImageInvalidTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

private:
	/* Private enums */
	enum TESTCASES
	{
		NEGATIVE_VALUE,
		VARIABLE_NAME,
		STD140,
		MISSING,

		/* */
		TEST_CASES_MAX
	};

	/* Private methods */
	const glw::GLchar* getCaseString(TESTCASES test_case);

	/* Provate variables */
	TESTCASES m_case;
};

/** Test implementation, description follows:
 *
 * * Vectors initialized using curly brace initializer lists:
 *
 *   Test expressions like:
 *       vec4 a = { 0.0, 1.0, 2.0, 3.0 };
 *
 *   Test all vector sizes.
 *   Verify if all components were set correctly.
 *
 *
 * * Matrices initialized using curly brace initializer lists:
 *
 *   Test expressions like:
 *       mat2   a = {{ 0.0, 1.0 }, { 2.0, 3.0 }};
 *       mat2x3 b = {{ 0.0, 1.0,  2.0 }, { 3.0, 4.0, 5.0 }};
 *
 *       Test all square matrix sizes. Check all non-square matrix sizes.
 *
 *       Verify if all components were set correctly.
 *
 *
 * * Matrix rows initialized using curly brace initializer lists:
 *
 *   Test expressions like:
 *       mat2   a = { vec2( 0.0, 1.0 ), vec2( 2.0, 3.0 ) };
 *       mat2x3 b = {vec3( 0.0, 1.0, 2.0), vec3( 3.0, 4.0, 5.0 )};
 *
 *       Test all square matrix sizes. Check all non-square matrix sizes.
 *
 *       Verify if all components were set correctly.
 *
 *
 * * Arrays initialized using curly brace initializer lists:
 *
 *   - Check arrays of scalars.
 *
 *   - Check arrays of vectors. Vectors initialized using *vec*(...) constructor.
 *
 *   - Check arrays of vectors. Vectors initialized initializer lists.
 *
 *   - Check arrays of matrices. Matrices initialized using *mat*(...) contructor.
 *
 *   - Check arrays of matrices. Matrices initialized initializer lists.
 *
 *   Verify if all components were set correctly.
 *
 *
 * * Structures of transparent types initialized using initializer lists:
 *
 *   Check arrays of structures also.
 *
 *   Test expressions like:
 *      struct S { float f; int i; uint u; }
 *      S a = {1.0, 2, -3};
 *      S b[3] = { S(1.0, 2, -3 ), { 3.0, 5, -6 }, { 7.0, 8, -9 } };
 *      S c[3] = { { 1.0, 2, -3 }, { 3.0, 5, -6 }, { 7.0, 8, -9 } };
 *
 *   Verify if all components were set correctly.
 *
 *
 * * Nested structures and arrays initialized using initializer lists:
 *
 *   - Check nested structures. Members initialized using <struct-type>(...)
 *     constructor.
 *
 *   - Check nested structures. Members initialized using initializer lists.\
         *
 *   - Check nested structures with multiple nesting levels.
 *
 *   - Check structures of arrays of structures. Initialize all members using
 *     initializer lists.
 *
 *   - Check structures of arrays of structures. Use mix of constructors and
 *     initializer lists to initialize members.
 *
 *   - Check arrays of structures, containing structures. Initialize all
 *     members using initializer lists.
 *
 *   - Check arrays of structures containing structures. Use mix of
 *     constructors and initializer lists to initialize members.
 *
 *   - Check structures containing structures, that contain arrays.
 *     Initialize all members using initializer lists.
 *
 *   - Check structures containing structures, that contain arrays. Use mix of
 *     constructors and initializer lists to initialize members.
 *
 *   Verify if all components were set correctly.
 *
 *
 * * Unsized arrays initialized with initialer lists:
 *
 *   Test expressions like:
 *       int i[] = { 1, 2, 3 };
 *       S b[] = { S(1.0, 2, -3 ), { 3.0, 5, -6 }, { 7.0, 8, -9 } };
 *       S c[] = { { 1.0, 2, -3 }, { 3.0, 5, -6 }, { 7.0, 8, -9 } };
 *
 *   Verify if all components were set correctly.
 **/
class InitializerListTest : public GLSLTestBase
{
public:
	/* Public methods */
	InitializerListTest(deqp::Context&);

	virtual ~InitializerListTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual bool testInit();

private:
	/* Private enums */
	enum TESTED_INITIALIZERS
	{
		VECTOR,
		MATRIX,
		MATRIX_ROWS,
		STRUCT,
		ARRAY_SCALAR,
		ARRAY_VECTOR_CTR,
		ARRAY_VECTOR_LIST,
		ARRAY_MATRIX_CTR,
		ARRAY_MATRIX_LIST,
		ARRAY_STRUCT,
		NESTED_STRUCT_CTR,
		NESTED_STRUCT_LIST,
		NESTED_STURCT_ARRAYS_STRUCT_LIST,
		NESTED_STURCT_ARRAYS_STRUCT_MIX,
		NESTED_ARRAY_STRUCT_STRUCT_LIST,
		NESTED_ARRAY_STRUCT_STRUCT_MIX,
		NESTED_STRUCT_STRUCT_ARRAY_LIST,
		NESTED_STRUCT_STRUCT_ARRAY_MIX,
		UNSIZED_ARRAY_SCALAR,
		UNSIZED_ARRAY_VECTOR,
		UNSIZED_ARRAY_MATRIX,
		UNSIZED_ARRAY_STRUCT,

		/* */
		TESTED_INITIALIZERS_MAX
	};

	/* Private types */
	struct testCase
	{
		TESTED_INITIALIZERS m_initializer;

		glw::GLuint m_n_cols;
		glw::GLuint m_n_rows;
	};

	/* Private methods */
	std::string getArrayDefinition();
	std::string getExpectedValue();
	std::string getInitialization();
	void		logTestCaseName();
	std::string getSum();
	std::string getTypeDefinition();
	std::string getTypeName();

	std::string getVectorArrayCtr(glw::GLuint columns, glw::GLuint size);

	std::string getVectorArrayList(glw::GLuint columns, glw::GLuint size);

	std::string getVectorConstructor(glw::GLuint column, glw::GLuint size);

	std::string getVectorInitializer(glw::GLuint column, glw::GLuint size);

	std::string getVectorArraySum(const glw::GLchar* array_name, glw::GLuint columns, glw::GLuint size);

	std::string getVectorSum(const glw::GLchar* vector_name, glw::GLuint size);

	std::string getVectorValues(glw::GLuint column, glw::GLuint size);

	/* Private variables */
	std::vector<testCase> m_test_cases;
	glw::GLint			  m_current_test_case_index;

	/* Private constants */
	static const glw::GLfloat m_value;
};

/** Test implementation, description follows:
 *
 * * Wrong type of component in initializer list.
 *
 *   Try to use wrong type of component. Expect compilation error. For example:
 *
 *       int a = { true };
 *
 *
 * * Wrong number of components in initializer list.
 *
 *   Try to wrong number of components. Expect compilation error. For example:
 *
 *       vec4 a = { 0.0, 0.0, 0.0 };
 *       vec3 a = { 0.0, 0.0, 0.0, 0.0 };
 *
 *
 * * Wrong matrix sizes in initializer lists:
 *
 *   Try to use wrong matrix row size or column count. Expect compilation error.
 *   For example:
 *
 *       mat2x3 b = {{ 0.0, 1.0,  2.0 }, { 3.0, 4.0}};
 *       mat2x3 b = {{ 0.0, 1.0}, { 2.0, 3.0}, { 4.0, 5.0 }};
 *       mat2x3 b = {{ 0.0, 1.0,  2.0 }, { 3.0, 4.0, 5.0 }};
 *       mat2x3 b = {{ 0.0, 1.0,  2.0 }, { 3.0, 4.0, 5.0 }};
 *
 *
 * * Initializer list inside constructor:
 *   Try to use initializer list inside constructor. Expect compilation error.
 *   For example:
 *
 *       struct S { vec2 v; };
 *       S s = S( {1.0, 2.0 } );
 *
 *
 * * Wrong struct layout in initializer list:
 *   Try to initialize struct with bad initializer list layout.
 *   Expect compilation error.
 *
 *   Check wrong member type, wrong member count and wrong member ordering.
 **/
class InitializerListNegativeTest : public NegativeTestBase
{
public:
	/* Public methods */
	InitializerListNegativeTest(deqp::Context&);

	virtual ~InitializerListNegativeTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual bool testInit();

private:
	/* Private enums */
	enum TESTED_ERRORS
	{
		TYPE_UIVEC_BOOL,
		TYPE_IVEC_BOOL,
		TYPE_VEC_BOOL,
		TYPE_MAT_BOOL,
		COMPONENTS_VEC_LESS,
		COMPONENTS_VEC_MORE,
		COMPONENTS_MAT_LESS_ROWS,
		COMPONENTS_MAT_LESS_COLUMNS,
		COMPONENTS_MAT_MORE_ROWS,
		COMPONENTS_MAT_MORE_COLUMNS,
		LIST_IN_CONSTRUCTOR,
		STRUCT_LAYOUT_MEMBER_TYPE,
		STRUCT_LAYOUT_MEMBER_COUNT_MORE,
		STRUCT_LAYOUT_MEMBER_COUNT_LESS,
		STRUCT_LAYOUT_MEMBER_ORDER,

		/* */
		TESTED_ERRORS_MAX
	};

	/* Private methods */
	std::string getInitialization();
	void		logTestCaseName();
	std::string getSum();
	std::string getTypeDefinition();
	std::string getTypeName();

	/* Private variables */
	std::vector<TESTED_ERRORS> m_test_cases;
	glw::GLint				   m_current_test_case_index;
};

/** Test implementation, description follows:
 *
 * * Apply .length() to various types:
 *
 *   Check value returned by .length(), when applied to vectors of all types.
 *   Check value returned by .length(), when applied to matrices of all types.
 *
 *   Check float, int and uint base types of vectors and matrices.
 *   Check all vector sizes, check all matrix dimensions.
 *
 *   Also check value returned by .length() when applied to vector or matrix
 *   members of a structures or interface blocks.
 *
 *
 * * Constant folding of .length() expressions:
 *
 *   Use value of .length() to set array size. For example:
 *
 *       vec4 a;
 *       float b[a.length()];
 **/
class LengthOfVectorAndMatrixTest : public GLSLTestBase
{
public:
	/* Public methods */
	LengthOfVectorAndMatrixTest(deqp::Context&);

	virtual ~LengthOfVectorAndMatrixTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);

	virtual void prepareVertexBuffer(const Utils::program& program, Utils::buffer& buffer, Utils::vertexArray& vao);

	virtual bool testInit();

private:
	/* Private types */
	struct testCase
	{
		Utils::TYPES m_type;
		glw::GLuint  m_n_cols;
		glw::GLuint  m_n_rows;
	};

	/* Private methods */
	std::string getExpectedValue(Utils::SHADER_STAGES in_stage);
	std::string getInitialization();

	std::string getMatrixInitializer(glw::GLuint n_cols, glw::GLuint n_rows);

	std::string getVectorInitializer(Utils::TYPES type, glw::GLuint n_rows);

	void prepareComputeShaderSource(Utils::shaderSource& out_source);

	void prepareDrawShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
								 Utils::shaderSource& out_source);

	glw::GLuint			  m_current_test_case_index;
	bool				  m_is_compute_program;
	std::vector<testCase> m_test_cases;
};

/** Test implementation, description follows:
 *
 * * .length() called on compute type:
 *
 *   Check value returned by .length(), when applied to computed types:
 *
 *   - rows of matrices
 *        mat4x3 a;
 *        a[<variable>].length()
 *
 *   - computed types: matrix multiplication
 *       mat4x2 a;
 *       mat3x4 b;
 *       (a * b).length()
 *       (a * b)[<variable>].length()
 *
 *   - computed types: vector multiplication
 *       vec3 a;
 *       vec3 b;
 *       (a * b).length()
 *
 *
 * * Constant folding of .length() expressions using computed type.
 *
 *   Use value of .length() to set array size, when called on computed type.
 *   For example:
 *      mat4x2 a;
 *      mat3x4 b;
 *      float c[a(a * b).length()];
 *      float d[(a * b)[<variable>].length()];
 *
 *
 * * .length() called on build-in values.
 *
 *   Check value returned by .length when called on gl_Position,
 *   gl_PointCoord, gl_SamplePosition
 *
 *
 * * .length() called on build-in functions
 *
 *   Check value returned by .length() when called on values returned from
 *   build in functions. For example:
 *      outerProduct(vec4(0.0), vec3(0.0)).length()
 **/
class LengthOfComputeResultTest : public GLSLTestBase
{
public:
	/* Public methods */
	LengthOfComputeResultTest(deqp::Context&);

	virtual ~LengthOfComputeResultTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
};

/** Test implementation, description follows:
 *
 * * All sizes of scalar swizzle
 *
 *   Test value returned by all sizes of scalar swizzlers: .x, .xx, .xxx and
 *   .xxx, when called on a float variable.
 *
 * * Scalar swizzling of literals
 *
 *   Call scalar swizzler .xxx on literal, for example (0.0).xxx.
 *
 * * Scalar swizzling of constant expressions
 *
 *   Call scalar swizzler .xxx on constant, for example:
 *
 *       const float x = 0.0;
 *       x.xxx
 *
 * * Mixed scalar swizzling
 *
 *   Check combinations of 'x', 'r', 's' swizzlers: .xx, .rr, .ss, .xrs
 *
 * * Nested swizzlers
 *
 *   Check nested swizzlers. For example:
 *       const float x = 0.0;
 *       x.r.s.x.ss
 **/
class ScalarSwizzlersTest : public GLSLTestBase
{
public:
	/* Public methods */
	ScalarSwizzlersTest(deqp::Context&);

	virtual ~ScalarSwizzlersTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
};

/** Test implementation, description follows:
 *
 * * Wrong swizzlers for scalars:
 *
 *   Swizzlers not applicable for scalars like .z, .xz, .q should fail
 *   shader compilation.
 *
 * * Wrong swizzlers:
 *
 *   Wrong swizzlers, like .u  should fail shader compilation.
 *
 * * Wrong syntax:
 *
 *   Literal swizzlers without parenthesis, like 1.x, should fail shader
 *   compilation.
 **/
class ScalarSwizzlersInvalidTest : public NegativeTestBase
{
public:
	/* Public methods */
	ScalarSwizzlersInvalidTest(deqp::Context&);

	virtual ~ScalarSwizzlersInvalidTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

private:
	/* Private enums */
	enum TESTED_CASES
	{
		INVALID_Y,
		INVALID_B,
		INVALID_Q,
		INVALID_XY,
		INVALID_XRS,
		WRONG,
		MISSING_PARENTHESIS,
	};

	TESTED_CASES m_case;
};

/** Test implementation, description follows:
 *
 * * Value of gl_MinProgramTexelOffset:
 *
 *   Check that gl_MinProgramTexelOffset matches the value of
 *   GL_MIN_PROGRAM_TEXEL_OFFSET from API.
 *
 *   Check that both values satisfy minimal requirement from OpenGL
 *   specification.
 *
 * * Value of gl_MinProgramTexelOffset:
 *
 *   Check that gl_MinProgramTexelOffset matches the value of
 *   GL_MAX_PROGRAM_TEXEL_OFFSET from API.
 *
 *   Check that both values satisfy minimal requirement from OpenGL
 *   specification.
 **/
class BuiltInValuesTest : public GLSLTestBase
{
public:
	/* Public methods */
	BuiltInValuesTest(deqp::Context&);

	virtual ~BuiltInValuesTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

	virtual void prepareUniforms(Utils::program& program);
	virtual bool testInit();

private:
	/* Private constants */
	static const glw::GLint m_min_program_texel_offset_limit;
	static const glw::GLint m_max_program_texel_offset_limit;

	/* Private variables */
	glw::GLint m_min_program_texel_offset;
	glw::GLint m_max_program_texel_offset;
};

/** Test implementation, description follows:
 *
 **/
class BuiltInAssignmentTest : public NegativeTestBase
{
public:
	/* Public methods */
	BuiltInAssignmentTest(deqp::Context&);

	virtual ~BuiltInAssignmentTest()
	{
	}

protected:
	/* Methods to be implemented by child class */
	virtual bool prepareNextTestCase(glw::GLuint test_case_index);

	virtual void prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
									 Utils::shaderSource& out_source);

private:
	/* Private variables */
	glw::GLuint m_case;
};
} /* GLSL420Pack namespace */

/** Group class for Shader Language 420Pack conformance tests */
class ShadingLanguage420PackTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	ShadingLanguage420PackTests(deqp::Context& context);

	virtual ~ShadingLanguage420PackTests(void)
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	ShadingLanguage420PackTests(const ShadingLanguage420PackTests& other);
	ShadingLanguage420PackTests& operator=(const ShadingLanguage420PackTests& other);
};

} // gl4cts

#endif // _GL4CSHADINGLANGUAGE420PACKTESTS_HPP
