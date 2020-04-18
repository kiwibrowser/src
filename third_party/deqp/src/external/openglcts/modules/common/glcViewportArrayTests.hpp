#ifndef _GLCVIEWPORTARRAYTESTS_HPP
#define _GLCVIEWPORTARRAYTESTS_HPP
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
 * \file  glcViewportArrayTests.hpp
 * \brief Declares test classes for "Viewport Array" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"

#include "esextcTestCaseBase.hpp"

namespace tcu
{
class MessageBuilder;
} /* namespace tcu */

namespace glcts
{
namespace ViewportArray
{

class Utils
{
public:
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

	class shaderCompilationException : public std::exception
	{
	public:
		shaderCompilationException(const glw::GLchar* source, const glw::GLchar* message);

		virtual ~shaderCompilationException() throw()
		{
		}

		virtual const char* what() const throw();

		std::string m_shader_source;
		std::string m_error_message;
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

		void compile(glw::GLuint shader_id, const glw::GLchar* source) const;

		glw::GLint getAttribLocation(const glw::GLchar* name) const;

		glw::GLuint getSubroutineIndex(const glw::GLchar* subroutine_name, glw::GLenum shader_stage) const;

		glw::GLint getSubroutineUniformLocation(const glw::GLchar* uniform_name, glw::GLenum shader_stage) const;

		glw::GLint getUniformLocation(const glw::GLchar* uniform_name) const;

		void link() const;
		void remove();
		void use() const;

		/* */
		static void printShaderSource(const glw::GLchar* source, tcu::MessageBuilder& log);

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

		void create(glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLenum internal_format);

		void get(glw::GLenum format, glw::GLenum type, glw::GLvoid* out_data) const;

		void release();

		void update(glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLenum format, glw::GLenum type,
					glw::GLvoid* data);

		glw::GLuint m_id;
		glw::GLuint m_width;
		glw::GLuint m_height;
		glw::GLuint m_depth;

	private:
		deqp::Context& m_context;
		bool		   m_is_array;
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

	class DepthFuncWrapper
	{
	public:
		DepthFuncWrapper(deqp::Context& context) : m_gl(context.getRenderContext().getFunctions()){};
		~DepthFuncWrapper(){};

		void depthRangeArray(glw::GLuint first, glw::GLsizei count, const glw::GLfloat* v)
		{
			m_gl.depthRangeArrayfvOES(first, count, v);
		}

		void depthRangeArray(glw::GLuint first, glw::GLsizei count, const glw::GLdouble* v)
		{
			m_gl.depthRangeArrayv(first, count, v);
		}

		void depthRangeIndexed(glw::GLuint index, glw::GLfloat n, glw::GLfloat f)
		{
			m_gl.depthRangeIndexedfOES(index, n, f);
		}

		void depthRangeIndexed(glw::GLuint index, glw::GLdouble n, glw::GLdouble f)
		{
			m_gl.depthRangeIndexed(index, n, f);
		}

		void depthRange(glw::GLfloat near, glw::GLfloat far)
		{
			m_gl.depthRangef(near, far);
		}

		void depthRange(glw::GLdouble near, glw::GLdouble far)
		{
			m_gl.depthRange(near, far);
		}

		void getDepthi_v(glw::GLenum target, glw::GLuint index, glw::GLfloat* data)
		{
			m_gl.getFloati_v(target, index, data);
		}

		void getDepthi_v(glw::GLenum target, glw::GLuint index, glw::GLdouble* data)
		{
			m_gl.getDoublei_v(target, index, data);
		}

		const glw::Functions& getFunctions()
		{
			return m_gl;
		}

	private:
		const glw::Functions& m_gl;
	};
};

/** Implements test APIErrors, description follows:
 *
 *   Verify that API generate errors as specified. Check that:
 *   * DepthRangeArrayv generates INVALID_VALUE when <first> + <count> is greater
 *   than or equal to the value of MAX_VIEWPORTS;
 *   * DepthRangeIndexed generates INVALID_VALUE when <index> is greater than or
 *   equal to the value of MAX_VIEWPORTS;
 *   * ViewportArrayv generates INVALID_VALUE when <first> + <count> is greater
 *   than or equal to the value of MAX_VIEWPORTS;
 *   * ViewportIndexedf and ViewportIndexedfv generate INVALID_VALUE when <index>
 *   is greater than or equal to the value of MAX_VIEWPORTS;
 *   * ViewportArrayv, Viewport, ViewportIndexedf and ViewportIndexedfv generate
 *   INVALID_VALUE when <w> or <h> values are negative;
 *   * ScissorArrayv generates INVALID_VALUE when <first> + <count> is greater
 *   than or equal to the value of MAX_VIEWPORTS;
 *   * ScissorIndexed and ScissorIndexedv generate INVALID_VALUE when <index> is
 *   greater than or equal to the value of MAX_VIEWPORTS;
 *   * ScissorArrayv, ScissorIndexed, ScissorIndexedv and Scissor generate
 *   INVALID_VALUE when <width> or <height> values are negative;
 *   * Disablei, Enablei and IsEnabledi generate INVALID_VALUE when <cap> is
 *   SCISSOR_TEST and <index> is greater than or equal to the
 *   value of MAX_VIEWPORTS;
 *   * GetIntegeri_v generates INVALID_VALUE when <target> is SCISSOR_BOX and
 *   <index> is greater than or equal to the value of MAX_VIEWPORTS;
 *   * GetFloati_v generates INVALID_VALUE when <target> is VIEWPORT and <index>
 *   is greater than or equal to the value of MAX_VIEWPORTS;
 *   * GetDoublei_v generates INVALID_VALUE when <target> is DEPTH_RANGE and
 *   <index> is greater than or equal to the value of MAX_VIEWPORTS;
 **/
class APIErrors : public glcts::TestCaseBase
{
public:
	/* Public methods */
	APIErrors(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~APIErrors()
	{
	}

	/* Public methods inherited from TestCaseBase */
	virtual IterateResult iterate(void);

private:
	template <typename T>
	void depthRangeArrayHelper(Utils::DepthFuncWrapper& depthFunc, glw::GLint max_viewports, bool& test_result,
							   T* data = NULL);

	template <typename T>
	void depthRangeIndexedHelper(Utils::DepthFuncWrapper& depthFunc, glw::GLint max_viewports, bool& test_result,
								 T* data = NULL);

	template <typename T>
	void getDepthHelper(Utils::DepthFuncWrapper& depthFunc, glw::GLint max_viewports, bool& test_result,
						T* data = NULL);

	void checkGLError(glw::GLenum expected_error, const glw::GLchar* description, bool& out_result);
};

/** Implements test Queries, description follows:
 *
 *   Verify that:
 *   * Initial dimensions of VIEWPORT returned by GetFloati_v match dimensions of
 *   the window into which GL is rendering;
 *   * Initial values of DEPTH_RANGE returned by GetDoublei_v are [0, 1];
 *   * Initial state of SCISSOR_TEST returned by IsEnabledi is FALSE;
 *   * Initial dimensions of SCISSOR_BOX returned by GetIntegeri_v are either
 *   zeros or match dimensions of the window into which GL is rendering;
 *   * Dimensions of MAX_VIEWPORT_DIMS returned by GetFloati_v are at least
 *   as big as supported dimensions of render buffers, see MAX_RENDERBUFFER_SIZE;
 *   * Value of MAX_VIEWPORTS returned by GetIntegeri_v is at least 16;
 *   * Value of VIEWPORT_SUBPIXEL_BITS returned by GetIntegeri_v is at least 0;
 *   * Values of VIEWPORT_BOUNDS_RANGE returned by GetFloatv are
 *   at least [-32768, 32767];
 *   * Values of LAYER_PROVOKING_VERTEX and VIEWPORT_INDEX_PROVOKING_VERTEX
 *   returned by GetIntegerv are located in the following set
 *   { FIRST_VERTEX_CONVENTION, LAST_VERTEX_CONVENTION, PROVOKING_VERTEX,
 *   UNDEFINED_VERTEX };
 **/
class Queries : public glcts::TestCaseBase
{
public:
	/* Public methods */
	Queries(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~Queries()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	template <typename T>
	void depthRangeInitialValuesHelper(Utils::DepthFuncWrapper& depthFunc, glw::GLint max_viewports, bool& test_result,
									   T* data = NULL);
};

/** Implements test ViewportAPI, description follows:
 *
 *   Verify that VIEWPORT can be set and queried.
 *   Steps:
 *   - get initial dimensions of VIEWPORT for all MAX_VIEWPORTS indices;
 *   - change location and dimensions of all indices at once with
 *   ViewportArrayv;
 *   - get VIEWPORT for all MAX_VIEWPORTS indices and verify results;
 *   - for each index:
 *     * modify with ViewportIndexedf,
 *     * get VIEWPORT for all MAX_VIEWPORTS indices and verify results;
 *     * modify with ViewportIndexedfv,
 *     * get VIEWPORT for all MAX_VIEWPORTS indices and verify results;
 *   - for each index:
 *     * modify all indices before and after current one with ViewportArrayv,
 *     * get VIEWPORT for all MAX_VIEWPORTS indices and verify results;
 *   - change location and dimensions of all indices at once with Viewport;
 *   - get VIEWPORT for all MAX_VIEWPORTS indices and verify results;
 **/
class ViewportAPI : public glcts::TestCaseBase
{
public:
	/* Public methods */
	ViewportAPI(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~ViewportAPI()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	/* Private methods */
	void compareViewports(std::vector<glw::GLfloat>& left, std::vector<glw::GLfloat>& right,
						  const glw::GLchar* description, bool& out_result);

	void getViewports(glw::GLint max_viewports, std::vector<glw::GLfloat>& out_data);

	/* Private constants */
	static const glw::GLuint m_n_elements;
};

/** Implements test ScissorAPI, description follows:
 *
 *   Verify that SCISSOR_BOX can be set and queried.
 *   Steps:
 *   - get initial dimensions of SCISSOR_BOX for all MAX_VIEWPORTS indices;
 *   - change location and dimensions of all indices at once with
 *   ScissorArrayv;
 *   - get SCISSOR_BOX for all MAX_VIEWPORTS indices and verify results;
 *   - for each index:
 *     * modify with ScissorIndexed,
 *     * get SCISSOR_BOX for all MAX_VIEWPORTS indices and verify results;
 *     * modify with ScissorIndexedv,
 *     * get SCISSOR_BOX for all MAX_VIEWPORTS indices and verify results;
 *   - for each index:
 *     * modify all indices before and after current one with ScissorArrayv,
 *     * get SCISSOR_BOX for all MAX_VIEWPORTS indices and verify results;
 *   - change location and dimensions of all indices at once with Scissor;
 *   - get SCISSOR_BOX for all MAX_VIEWPORTS indices and verify results;
 **/
class ScissorAPI : public glcts::TestCaseBase
{
public:
	/* Public methods */
	ScissorAPI(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~ScissorAPI()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	/* Private methods */
	void compareScissorBoxes(std::vector<glw::GLint>& left, std::vector<glw::GLint>& right,
							 const glw::GLchar* description, bool& out_result);

	void getScissorBoxes(glw::GLint max_viewports, std::vector<glw::GLint>& out_data);

	/* Private constants */
	static const glw::GLuint m_n_elements;
};

/** Implements test DepthRangeAPI, description follows:
 *
 *   Verify that DEPTH_RANGE can be set and queried.
 *   Steps:
 *   - get initial values of DEPTH_RANGE for all MAX_VIEWPORTS indices;
 *   - change values of all indices at once with DepthRangeArrayv;
 *   - get DEPTH_RANGE for all MAX_VIEWPORTS indices and verify results;
 *   - for each index:
 *     * modify with DepthRangeIndexed,
 *     * get DEPTH_RANGE for all MAX_VIEWPORTS indices and verify results;
 *   - for each index:
 *     * modify all indices before and after current one with DepthRangeArrayv,
 *     * get DEPTH_RANGE for all MAX_VIEWPORTS indices and verify results;
 *   - change values of all indices at once with DepthRange;
 *   - get DEPTH_RANGE for all MAX_VIEWPORTS indices and verify results;
 **/
class DepthRangeAPI : public glcts::TestCaseBase
{
public:
	/* Public methods */
	DepthRangeAPI(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~DepthRangeAPI()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	/* Private methods */
	template <typename T>
	void compareDepthRanges(std::vector<T>& left, std::vector<T>& right, const glw::GLchar* description,
							bool& out_result);

	template <typename T>
	void getDepthRanges(Utils::DepthFuncWrapper& depthFunc, glw::GLint max_viewports, std::vector<T>& out_data);

	template <typename T>
	bool iterateHelper(T* data = NULL);

	/* Private constants */
	static const glw::GLuint m_n_elements;
};

/** Implements test ScissorTestStateAPI, description follows:
 *
 *   Verify that state of SCISSOR_TEST can be set and queried.
 *   Steps:
 *   - get initial state of SCISSOR_TEST for all MAX_VIEWPORTS indices;
 *   - for each index:
 *     * toggle SCISSOR_TEST,
 *     * get state of SCISSOR_TEST for all MAX_VIEWPORTS indices and verify;
 *   - for each index:
 *     * toggle SCISSOR_TEST,
 *     * get state of SCISSOR_TEST for all MAX_VIEWPORTS indices and verify;
 *   - enable SCISSOR_TEST for all indices at once with Enable;
 *   - get state of SCISSOR_TEST for all MAX_VIEWPORTS indices and verify;
 *   - disable SCISSOR_TEST for all indices at once with Disable;
 *   - get state of SCISSOR_TEST for all MAX_VIEWPORTS indices and verify;
 *   - enable SCISSOR_TEST for all indices at once with Enable;
 *   - get state of SCISSOR_TEST for all MAX_VIEWPORTS indices and verify;
 **/
class ScissorTestStateAPI : public glcts::TestCaseBase
{
public:
	/* Public methods */
	ScissorTestStateAPI(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~ScissorTestStateAPI()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	/* Private methods */
	void compareScissorTestStates(std::vector<glw::GLboolean>& left, std::vector<glw::GLboolean>& right,
								  const glw::GLchar* description, bool& out_result);

	void getScissorTestStates(glw::GLint max_viewports, std::vector<glw::GLboolean>& out_data);
};

class DrawTestBase : public glcts::TestCaseBase
{
public:
	/* Public methods */
	DrawTestBase(deqp::Context& context, const glcts::ExtParameters& extParams, const glw::GLchar* test_name,
				 const glw::GLchar* test_description);

	virtual ~DrawTestBase()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

protected:
	/* Protected enums */
	enum VIEWPORT_METHOD
	{
		VIEWPORTARRAYV = 0,
		VIEWPORTINDEXEDF,
		VIEWPORTINDEXEDF_V,
	};
	enum SCISSOR_METHOD
	{
		SCISSORARRAYV = 0,
		SCISSORINDEXEDF,
		SCISSORINDEXEDF_V,
	};
	enum DEPTH_RANGE_METHOD
	{
		DEPTHRANGEARRAYV = 0,
		DEPTHRANGEINDEXED,
	};
	enum PROVOKING_VERTEX
	{
		FIRST,
		LAST,
	};
	enum TEST_TYPE
	{
		VIEWPORT,
		SCISSOR,
		DEPTHRANGE,
		PROVOKING,
	};

	/* Protected methods to be implemented by child class */
	virtual bool checkResults(Utils::texture& texture_0, Utils::texture& texture_1, glw::GLuint draw_call_index);

	virtual void getClearSettings(bool& clear_depth_before_draw, glw::GLuint iteration_index,
								  glw::GLfloat& depth_value);

	virtual glw::GLuint getDrawCallsNumber();
	virtual std::string getFragmentShader() = 0;
	virtual std::string getGeometryShader() = 0;
	virtual TEST_TYPE   getTestType();
	virtual bool		isClearTest();

	virtual void prepareTextures(Utils::texture& texture_0, Utils::texture& texture_1);

	virtual void prepareUniforms(Utils::program& program, glw::GLuint draw_call_index);

	virtual void setupFramebuffer(Utils::framebuffer& framebuffer, Utils::texture& texture_0,
								  Utils::texture& texture_1);

	virtual void setupViewports(TEST_TYPE type, glw::GLuint iteration_index);

	/* Methods available for child class */
	bool checkRegionR32I(glw::GLuint x, glw::GLuint y, glw::GLint expected_value, glw::GLint* data);

	bool checkRegionR32I(glw::GLuint x, glw::GLuint y, glw::GLuint width, glw::GLuint height, glw::GLint expected_value,
						 glw::GLint* data);

	void prepareTextureR32I(Utils::texture& texture);
	void prepareTextureR32Ix4(Utils::texture& texture);
	void prepareTextureArrayR32I(Utils::texture& texture);
	void prepareTextureR32F(Utils::texture& texture);
	void prepareTextureD32F(Utils::texture& texture);
	void setup16x2Depths(DEPTH_RANGE_METHOD method);
	void setup4x4Scissor(SCISSOR_METHOD method, bool set_zeros);
	void setup4x4Viewport(VIEWPORT_METHOD method);
	void setup2x2Viewport(PROVOKING_VERTEX provoking);

	/* Constants available to child class */
	static const glw::GLuint m_depth;
	static const glw::GLuint m_height;
	static const glw::GLuint m_width;
	static const glw::GLuint m_r32f_height;
	static const glw::GLuint m_r32f_width;
	static const glw::GLuint m_r32ix4_depth;

private:
	/* Private methods */
	std::string getVertexShader();

	template <typename T>
	void setup16x2DepthsHelper(DEPTH_RANGE_METHOD method, T* data = NULL);
};

/** Implements test DrawToSingleLayerWithMultipleViewports, description follows:
 *
 *   Verify that multiple viewports can be used to draw to single image.
 *   Steps:
 *   - prepare 2D R32I 128x128 texture filled with value -1 and set it up as
 *   COLOR_ATTACHMENT_0;
 *   - prepare program that consist of:
 *     * boilerplate vertex shader,
 *     * geometry shader,
 *     * fragment shaders;
 *   Geometry shader should output a quad (-1,-1 : 1,1) made of
 *   triangle_strip; gl_ViewportIndex and declared integer varying "color"
 *   should be assigned the value of gl_InvocationID; Amount of geometry shader
 *   invocations should be set to 16; Fragment shader should output value of
 *   varying "color" to attachment 0.
 *   - set up first 16 viewports with following code snippet:
 *
 *       index = 0;
 *       for (y = 0; y < 4; ++y)
 *         for (x = 0; x < 4; ++x)
 *           ViewportIndexedf(index++,
 *                            x * 32 x offset,
 *                            y * 32 y offset,
 *                            32     width   ,
 *                            32     height  );
 *   - draw single vertex;
 *   - inspect contents of COLOR_ATTACHMENT_0;
 *   - test pass if image is filled with the following pattern:
 *
 *       0  1  2  3
 *       4  5  6  7
 *       8  9  10 11
 *       12 13 14 15;
 *
 *   Each area should be 32x32 pixels rectangle;
 *   - repeat test with functions ViewportIndexedf_v and ViewportArrayv;
 **/
class DrawToSingleLayerWithMultipleViewports : public DrawTestBase
{
public:
	/* Public methods */
	DrawToSingleLayerWithMultipleViewports(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~DrawToSingleLayerWithMultipleViewports()
	{
	}

protected:
	/* Protected methods inherited from DrawTestBase */
	virtual std::string getFragmentShader();
	virtual std::string getGeometryShader();
};

/** Implements test DynamicViewportIndex, description follows:
 *
 *   Verify that gl_ViewportIndex can be set in dynamic manner.
 *   Modify DrawToSingleLayerWithMultipleViewports in the following aspects:
 *   - geometry shader should declare unsigned integer uniform "index";
 *   - geometry shader should assign a value of "index" to gl_ViewportIndex and
 *   "color";
 *   - amount of geometry shader invocations should be set to 1;
 *   - 16 times:
 *     * set "index" to unique value from range <0:15>;
 *     * draw single vertex;
 *     * verify that only area of viewport at "index" has been updated;
 *   - test pass if correct pixels were modified in each draw;
 **/
class DynamicViewportIndex : public DrawTestBase
{
public:
	/* Public methods */
	DynamicViewportIndex(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~DynamicViewportIndex()
	{
	}

protected:
	/* Protected methods inherited from DrawTestBase */
	virtual bool checkResults(Utils::texture& texture_0, Utils::texture& texture_1, glw::GLuint draw_call_index);

	virtual std::string getFragmentShader();
	virtual std::string getGeometryShader();
	virtual glw::GLuint getDrawCallsNumber();

	virtual void prepareUniforms(Utils::program& program, glw::GLuint draw_call_index);
};

/** Implements test DrawMulitpleViewportsWithSingleInvocation, description follows:
 *
 *   Verify that multiple viewports can be affected by single invocation of
 *   geometry shader.
 *   Modify DrawToSingleLayerWithMultipleViewports in the following aspects:
 *   - geometry shader should output 16 quads, as separate primitives;
 *   - instead of gl_InvocationID, geometry shader should use predefined values
 *   from range <0:15>, unique per quad;
 *   - amount of geometry shader invocations should be set to 1;
 **/
class DrawMulitpleViewportsWithSingleInvocation : public DrawTestBase
{
public:
	/* Public methods */
	DrawMulitpleViewportsWithSingleInvocation(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~DrawMulitpleViewportsWithSingleInvocation()
	{
	}

protected:
	/* Protected methods inherited from DrawTestBase */
	virtual std::string getFragmentShader();
	virtual std::string getGeometryShader();
};

/** Implements test ViewportIndexSubroutine, description follows:
 *
 *   Verify that gl_ViewportIndex can be assigned by subroutine.
 *   Depends on: ARB_shader_subroutine.
 *   Modify DynamicViewportIndex in the following aspects:
 *   - geometry shader should define two subroutines and single subroutine
 *   uniform; First subroutine should assign value 4 to gl_ViewportIndex and
 *   "color"; Second subroutine should assign value 5 to gl_ViewportIndex and
 *   "color"; subroutine should be called once per emitted vertex;
 *   - uniform "index" should be removed;
 *   - viewport 4 should be configured to span over left half of image; viewport
 *   5 should span over right half of image;
 *   - set up first subroutine and draw single vertex;
 *   - set up second subroutine and draw single vertex;
 *   - test pass if left half of image is filled with value 4 and right one with
 *   5;
 **/
class ViewportIndexSubroutine : public DrawTestBase
{
public:
	/* Public methods */
	ViewportIndexSubroutine(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~ViewportIndexSubroutine()
	{
	}

	/* Public methods inherited from TestCase/DrawTestBase */
	virtual tcu::TestNode::IterateResult iterate(void);

protected:
	/* Protected methods inherited from DrawTestBase */
	virtual bool checkResults(Utils::texture& texture_0, Utils::texture& texture_1, glw::GLuint draw_call_index);

	virtual std::string getFragmentShader();
	virtual std::string getGeometryShader();
	virtual glw::GLuint getDrawCallsNumber();

	virtual void prepareUniforms(Utils::program& program, glw::GLuint draw_call_index);

	virtual void setupViewports(TEST_TYPE type, glw::GLuint iteration_index);
};

/** Implements test DrawMultipleLayers, description follows:
 *
 *   Verify that single viewport affects multiple layers in the same way.
 *   Modify DynamicViewportIndex in the following aspects:
 *   - texture should be 2D array with 16 layers;
 *   - geometry shader should assign a value of gl_InvocationId to gl_Layer;
 *   - amount of geometry shader invocations should be set to 16;
 *   - verification should be applied to all 16 layers;
 **/
class DrawMultipleLayers : public DrawTestBase
{
public:
	/* Public methods */
	DrawMultipleLayers(deqp::Context& context, const glcts::ExtParameters& extParams);

	DrawMultipleLayers(deqp::Context& context, const glcts::ExtParameters& extParams, const glw::GLchar* test_name,
					   const glw::GLchar* test_description);

	virtual ~DrawMultipleLayers()
	{
	}

protected:
	/* Protected methods inherited from DrawTestBase */
	virtual bool checkResults(Utils::texture& texture_0, Utils::texture& texture_1, glw::GLuint draw_call_index);

	virtual std::string getFragmentShader();
	virtual std::string getGeometryShader();

	virtual void prepareTextures(Utils::texture& texture_0, Utils::texture& texture_1);
};

/** Implements test Scissor, description follows:
 *
 *   Verify that scissor test is applied as expected.
 *   Modify DrawMultipleLayers in the following aspects:
 *   - set all viewports to location 0,0 and dimensions 128x128;
 *   - set up first 16 scissor boxes with following code snippet:
 *
 *       index = 0;
 *       for (y = 0; y < 4; ++y)
 *         for (x = 0; x < 4; ++x)
 *           ScissorIndexed(index++,
 *                          x * 32 x offset,
 *                          y * 32 y offset,
 *                          32     width   ,
 *                          32     height  );
 *
 *   - enable SCISSORT_TEST for first 16 indices;
 *   - verification should be concerned with areas of the scissor boxes not
 *   viewports;
 *   - repeat test with functions ScissorIndexedv and ScissorArrayv;
 **/
class Scissor : public DrawMultipleLayers
{
public:
	/* Public methods */
	Scissor(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~Scissor()
	{
	}

protected:
	/* Protected methods inherited from DrawTestBase */
	virtual TEST_TYPE getTestType();
};

/** Implements test ScissorZeroDimension, description follows:
 *
 *   Verify that scissor test discard all fragments when width and height is set
 *   to zero.
 *   Modify Scissor to set up width and height of scissor boxes to 0.
 *   Test pass if no pixel is modified.
 **/
class ScissorZeroDimension : public DrawMultipleLayers
{
public:
	/* Public methods */
	ScissorZeroDimension(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~ScissorZeroDimension()
	{
	}

protected:
	/* Protected methods inherited from DrawTestBase */
	virtual bool checkResults(Utils::texture& texture_0, Utils::texture& texture_1, glw::GLuint draw_call_index);

	virtual TEST_TYPE getTestType();

	virtual void setupViewports(TEST_TYPE type, glw::GLuint iteration_index);
};

/** Implements test ScissorClear, description follows:
 *
 *   Verify that Clear is affected only by settings of scissor test in first
 *   viewport.
 *   Steps:
 *   - prepare 2D 128x128 R32I texture, filled with value -1 and set it as
 *   COLOR_ATTACHMENT_0;
 *   - configure first 16 viewports as in Scissor;
 *   - enable SCISSOR_TEST for first 16 indices;
 *   - clear framebuffer to (0, 0, 0, 0);
 *   - inspect image;
 *   - test pass if only area corresponding with first SCISSOR_BOX was filled
 *   with 0, while rest of image remain filled with value -1;
 **/
class ScissorClear : public DrawMultipleLayers
{
public:
	/* Public methods */
	ScissorClear(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~ScissorClear()
	{
	}

protected:
	/* Protected methods inherited from DrawTestBase */
	virtual bool checkResults(Utils::texture& texture_0, Utils::texture& texture_1, glw::GLuint draw_call_index);

	virtual TEST_TYPE getTestType();
	virtual bool	  isClearTest();
};

/** Implements test DepthRange, description follows:
 *
 *   Verify that depth range is applied as expected.
 *   Steps:
 *   - prepate 2D 16x2 R32F texture filled with value -1.0 and set it up as
 *   COLOR_ATTACHMENT_0;
 *   - prepare program that consist of:
 *     * boilerplate vertex shader,
 *     * geometry shader,
 *     * fragment shader;
 *   Geometry shader should emit two quads:
 *     * -1,0 : 1,1 with z equal -1.0,
 *     * -1,-1 : 1,0 with z equal 1.0,
 *   made of triangle_strip; gl_ViewportIndex should be assigned an value of
 *   gl_InvocationId; Amount of geometry shader invocations should be set to 16;
 *   Fragment shader should output value of gl_FragCoord.z to attachment 0.
 *   - set up first 16 viewports with the following code snippet:
 *
 *       const double step = 1.0 / 16.0;
 *       for (index = 0; index < 16; ++index)
 *       {
 *           const double near = ((double) i) * step;
 *           VieportIndexed   (i, (float) i, 0.0, 1.0, 2.0);
 *           DepthRangeIndexed(i, near,      1.0 - near);
 *       }
 *
 *   - draw single vertex;
 *   - inspect contents of COLOR_ATTACHMENT_0;
 *   - test pass if:
 *     * top row of image is filled with increasing values, starting at 0 with
 *     step 1/16;
 *     * bottom row of image is filled with decreasing values, starting at 1 with
 *     step 1/16;
 *   - repeat test with function DepthRangeArrayv;
 **/
class DepthRange : public DrawTestBase
{
public:
	/* Public methods */
	DepthRange(deqp::Context& context, const glcts::ExtParameters& extParams);
	virtual ~DepthRange()
	{
	}

protected:
	/* Protected methods inherited from DrawTestBase */
	virtual bool checkResults(Utils::texture& texture_0, Utils::texture& texture_1, glw::GLuint draw_call_index);

	virtual std::string getFragmentShader();
	virtual std::string getGeometryShader();
	virtual TEST_TYPE   getTestType();

	virtual void prepareTextures(Utils::texture& texture_0, Utils::texture& texture_1);
};

/** Implements test DepthRangeDepthTest, description follows:
 *
 *   Verify that depth test work as expected with multiple viewports.
 *   Modify DepthRange test in the following aspect:
 *   - add second 2D 16x2 DEPTH_COMPONENT32F texture and set it up as
 *   DEPTH_ATTACHMENT;
 *   - enable DEPTH_TEST;
 *   - DepthFunc should be set to LESS (initial value);
 *   - 18 times:
 *     * set ClearDepth to "i" * 1/16, starting at 0 up to 17/16,
 *     * draw single vertex
 *     * verify contents of color attachment;
 *   - test pass when color attachment is filled only with values lower than
 *   current ClearDepth value;
 **/
class DepthRangeDepthTest : public DrawTestBase
{
public:
	/* Public methods */
	DepthRangeDepthTest(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~DepthRangeDepthTest()
	{
	}

protected:
	/* Protected methods inherited from DrawTestBase */
	virtual bool checkResults(Utils::texture& texture_0, Utils::texture& texture_1, glw::GLuint draw_call_index);

	virtual void getClearSettings(bool& clear_depth_before_draw, glw::GLuint iteration_index,
								  glw::GLfloat& depth_value);

	virtual glw::GLuint getDrawCallsNumber();
	virtual std::string getFragmentShader();
	virtual std::string getGeometryShader();
	virtual TEST_TYPE   getTestType();

	virtual void prepareTextures(Utils::texture& texture_0, Utils::texture& texture_1);

	virtual void setupFramebuffer(Utils::framebuffer& framebuffer, Utils::texture& texture_0,
								  Utils::texture& texture_1);

	virtual void setupViewports(TEST_TYPE type, glw::GLuint iteration_index);
};

/** Implements test ProvokingVertex, description follows:
 *
 *   Verify that provoking vertex work as expected.
 *   Steps:
 *   - prepare 2D array R32I 128x128x4 texture and configure it as
 *   COLOR_ATTACHMENT_0;
 *   - prepare program consisting of:
 *     * boilerplate vertex shader,
 *     * geometry shader,
 *     * fragment shader;
 *   Geometry shader should output a quad (-1,-1 : 1,1); Each vertex should
 *   receive different gl_ViewportIndex value, first vertex should be assigned an
 *   0, second 1, third 2, fourth 3; gl_Layer should be set in the same way as
 *   gl_ViewportIndex; Fragment shader should output integer of value 1;
 *   - configure first four viewports to form 2x2 grid, spanning whole image;
 *   - for each combination of LAYER_PROVOKING_VERTEX and
 *   VIEWPORT_INDEX_PROVOKING_VERTEX:
 *     * clear framebuffer to (0,0,0,0),
 *     * draw single vertex
 *     * inspect image;
 *   - test pass if correct area of correct layer is filled with value 1, while
 *   rest of image remains "clean";
 *   Notes:
 *   - for UNDEFINED_VERTEX any selection is correct;
 *   - for PROVOKING_VERTEX convention is selected by function ProvokingVertex;
 *   Test all possible combinations;
 **/
class ProvokingVertex : public DrawTestBase
{
public:
	/* Public methods */
	ProvokingVertex(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~ProvokingVertex()
	{
	}

protected:
	/* Protected methods inherited from DrawTestBase */
	virtual bool checkResults(Utils::texture& texture_0, Utils::texture& texture_1, glw::GLuint draw_call_index);

	virtual std::string getFragmentShader();
	virtual std::string getGeometryShader();
	virtual TEST_TYPE   getTestType();

	virtual void prepareTextures(Utils::texture& texture_0, Utils::texture& texture_1);
};
} /* ViewportArray namespace */

/** Group class for Shader Language 420Pack conformance tests */
class ViewportArrayTests : public glcts::TestCaseGroupBase
{
public:
	/* Public methods */
	ViewportArrayTests(deqp::Context& context, const glcts::ExtParameters& extParams);

	virtual ~ViewportArrayTests(void)
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	ViewportArrayTests(const ViewportArrayTests& other);
	ViewportArrayTests& operator=(const ViewportArrayTests& other);
};

} // glcts

#endif // _GLCVIEWPORTARRAYTESTS_HPP
