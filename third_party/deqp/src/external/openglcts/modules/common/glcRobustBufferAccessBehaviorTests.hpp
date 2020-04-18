#ifndef _GLCROBUSTBUFFERACCESSBEHAVIORTESTS_HPP
#define _GLCROBUSTBUFFERACCESSBEHAVIORTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \file  glcRobustBufferAccessBehaviorTests.hpp
 * \brief Declares test classes for "Robust Buffer Access Behavior" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcRobustnessTests.hpp"
#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"

#include <map>

namespace glcts
{
namespace RobustBufferAccessBehavior
{
/** Replace first occurance of <token> with <text> in <string> starting at <search_posistion>
 **/
void replaceToken(const glw::GLchar* token, size_t& search_position, const glw::GLchar* text, std::string& string);

/** Represents buffer instance
 * Provides basic buffer functionality
 **/
class Buffer
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	Buffer(const glw::Functions& gl);
	~Buffer();

	/* Init & Release */
	void InitData(glw::GLenum target, glw::GLenum usage, glw::GLsizeiptr size, const glw::GLvoid* data);
	void Release();

	/* Functionality */
	void Bind() const;
	void BindBase(glw::GLuint index) const;

	/* Public static routines */
	/* Functionality */
	static void Bind(const glw::Functions& gl, glw::GLuint id, glw::GLenum target);
	static void BindBase(const glw::Functions& gl, glw::GLuint id, glw::GLenum target, glw::GLuint index);
	static void Data(const glw::Functions& gl, glw::GLenum target, glw::GLenum usage, glw::GLsizeiptr size,
					 const glw::GLvoid* data);
	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);
	static void SubData(const glw::Functions& gl, glw::GLenum target, glw::GLintptr offset, glw::GLsizeiptr size,
						glw::GLvoid* data);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;
	static const glw::GLuint m_n_targets = 13;
	static const glw::GLenum m_targets[m_n_targets];

private:
	/* Private enums */

	/* Private fields */
	const glw::Functions& m_gl;
	glw::GLenum	m_target;
};

/** Represents framebuffer
 * Provides basic functionality
 **/
class Framebuffer
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	Framebuffer(const glw::Functions& gl);
	~Framebuffer();

	/* Init & Release */
	void Release();

	/* Public static routines */
	static void AttachTexture(const glw::Functions& gl, glw::GLenum target, glw::GLenum attachment,
							  glw::GLuint texture_id, glw::GLint level, glw::GLuint width, glw::GLuint height);

	static void Bind(const glw::Functions& gl, glw::GLenum target, glw::GLuint id);
	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	const glw::Functions& m_gl;
};

/** Represents shader instance.
 * Provides basic functionality for shaders.
 **/
class Shader
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	Shader(const glw::Functions& gl);
	~Shader();

	/* Init & Realese */
	void Init(glw::GLenum stage, const std::string& source);
	void Release();

	/* Public static routines */
	/* Functionality */
	static void Compile(const glw::Functions& gl, glw::GLuint id);
	static void Create(const glw::Functions& gl, glw::GLenum stage, glw::GLuint& out_id);
	static void Source(const glw::Functions& gl, glw::GLuint id, const std::string& source);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	const glw::Functions& m_gl;
};

/** Represents program instance.
 * Provides basic functionality
 **/
class Program
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	Program(const glw::Functions& gl);
	~Program();

	/* Init & Release */
	void Init(const std::string& compute_shader, const std::string& fragment_shader, const std::string& geometry_shader,
			  const std::string& tesselation_control_shader, const std::string& tesselation_evaluation_shader,
			  const std::string& vertex_shader);

	void Release();

	/* Functionality */
	void Use() const;

	/* Public static routines */
	/* Functionality */
	static void Attach(const glw::Functions& gl, glw::GLuint program_id, glw::GLuint shader_id);
	static void Create(const glw::Functions& gl, glw::GLuint& out_id);
	static void Link(const glw::Functions& gl, glw::GLuint id);
	static void Use(const glw::Functions& gl, glw::GLuint id);

	/* Public fields */
	glw::GLuint m_id;

	Shader m_compute;
	Shader m_fragment;
	Shader m_geometry;
	Shader m_tess_ctrl;
	Shader m_tess_eval;
	Shader m_vertex;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	const glw::Functions& m_gl;
};

/** Represents texture instance
 **/
class Texture
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	Texture(const glw::Functions& gl);
	~Texture();

	/* Init & Release */
	void Release();

	/* Public static routines */
	/* Functionality */
	static void Bind(const glw::Functions& gl, glw::GLuint id, glw::GLenum target);

	static void CompressedImage(const glw::Functions& gl, glw::GLenum target, glw::GLint level,
								glw::GLenum internal_format, glw::GLuint width, glw::GLuint height, glw::GLuint depth,
								glw::GLsizei image_size, const glw::GLvoid* data);

	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	static void GetData(const glw::Functions& gl, glw::GLint level, glw::GLenum target, glw::GLenum format,
						glw::GLenum type, glw::GLvoid* out_data);

	static void GetData(const glw::Functions& gl, glw::GLuint id, glw::GLint level, glw::GLuint width,
						glw::GLuint height, glw::GLenum format, glw::GLenum type, glw::GLvoid* out_data);

	static void GetLevelParameter(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLenum pname,
								  glw::GLint* param);

	static void Image(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLenum internal_format,
					  glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLenum format, glw::GLenum type,
					  const glw::GLvoid* data);

	static void Storage(const glw::Functions& gl, glw::GLenum target, glw::GLsizei levels, glw::GLenum internal_format,
						glw::GLuint width, glw::GLuint height, glw::GLuint depth);

	static void SubImage(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLint x, glw::GLint y,
						 glw::GLint z, glw::GLsizei width, glw::GLsizei height, glw::GLsizei depth, glw::GLenum format,
						 glw::GLenum type, const glw::GLvoid* pixels);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	const glw::Functions& m_gl;
};

/** Represents Vertex array object
 * Provides basic functionality
 **/
class VertexArray
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	VertexArray(const glw::Functions& gl);
	~VertexArray();

	/* Init & Release */
	void Release();

	/* Public static methods */
	static void Bind(const glw::Functions& gl, glw::GLuint id);
	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	const glw::Functions& m_gl;
};

class RobustnessBase : public tcu::TestCase
{
public:
	RobustnessBase(tcu::TestContext& testCtx, const char* name, const char* description, glu::ApiType apiType);

	glu::RenderContext* createRobustContext(
		glu::ResetNotificationStrategy reset = glu::RESET_NOTIFICATION_STRATEGY_NO_RESET_NOTIFICATION);

protected:
	glu::ApiType m_api_type;
	bool		 m_context_is_es;
	bool		 m_has_khr_robust_buffer_access;

	std::map<std::string, std::string> m_specializationMap;
};

/** Implementation of test VertexBufferObjects. Description follows:
 *
 * This test verifies that any "out-of-bound" read from vertex buffer result with abnormal program exit
 *
 * Steps:
 * - prepare vertex buffer with the following vertices:
 *   * 0 - [ 0,  0, 0],
 *   * 1 - [-1,  0, 0],
 *   * 2 - [-1,  1, 0],
 *   * 3 - [ 0,  1, 0],
 *   * 4 - [ 1,  1, 0],
 *   * 5 - [ 1,  0, 0],
 *   * 6 - [ 1, -1, 0],
 *   * 7 - [ 0, -1, 0],
 *   * 8 - [-1, -1, 0];
 * - prepare element buffer:
 *   * valid:
 *     0, 1, 2,
 *     0, 2, 3,
 *     0, 3, 4,
 *     0, 4, 5,
 *     0, 5, 6,
 *     0, 6, 7,
 *     0, 7, 8,
 *     0, 8, 1;
 *   * invalid:
 *      9, 1, 2,
 *     10, 2, 3,
 *     11, 3, 4,
 *     12, 4, 5,
 *     13, 5, 6,
 *     14, 6, 7,
 *     15, 7, 8,
 *     16, 8, 1;
 * - prepare program consisting of vertex and fragment shader that will output
 * value 1;
 * - prepare framebuffer with R8UI texture attached as color 0, filled with
 * value 128;
 * - execute draw call with invalid element buffer;
 * - inspect contents of framebuffer, it is expected that it is filled with
 * value 1;
 * - clean framebuffer to value 128;
 * - execute draw call with valid element buffer;
 * - inspect contents of framebuffer, it is expected that it is filled with
 * value 1.
 **/
class VertexBufferObjectsTest : public RobustnessBase
{
public:
	/* Public methods */
	VertexBufferObjectsTest(tcu::TestContext& testCtx, glu::ApiType apiType);
	virtual ~VertexBufferObjectsTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

protected:
	/* Protected methods */
	std::string getFragmentShader();
	std::string getVertexShader();
	void cleanTexture(const glw::Functions& gl, glw::GLuint texture_id);
	bool verifyInvalidResults(const glw::Functions& gl, glw::GLuint texture_id);
	bool verifyValidResults(const glw::Functions& gl, glw::GLuint texture_id);
	bool verifyResults(const glw::Functions& gl, glw::GLuint texture_id);
};

/** Implementation of test TexelFetch. Description follows:
 *
 * This test verifies that any "out-of-bound" fetch from texture result in
 * "zero".
 *
 * Steps:
 * - prepare program consisting of vertex, geometry and fragment shader that
 * will output full-screen quad; Each fragment should receive value of
 * corresponding texel from source texture; Use texelFetch function;
 * - prepare 16x16 2D R8UI source texture filled with unique values;
 * - prepare framebuffer with 16x16 R8UI texture as color attachment, filled
 * with value 0;
 * - execute draw call;
 * - inspect contents of framebuffer, it is expected to match source texture;
 * - modify program so it will fetch invalid texels;
 * - execute draw call;
 * - inspect contents of framebuffer, it is expected that it will be filled
 * with value 0 for RGB channels and with 0, 1 or the biggest representable
 * integral number for alpha channel.
 *
 * Repeat steps for:
 * - R8 texture;
 * - RG8_SNORM texture;
 * - RGBA32F texture;
 * - mipmap at level 1;
 * - a texture with 4 samples.
 **/
class TexelFetchTest : public RobustnessBase
{
public:
	/* Public methods */
	TexelFetchTest(tcu::TestContext& testCtx, glu::ApiType apiType);
	TexelFetchTest(tcu::TestContext& testCtx, const char* name, const char* description, glu::ApiType apiType);
	virtual ~TexelFetchTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

protected:
	/* Protected enums */
	enum TEST_CASES
	{
		R8,
		RG8_SNORM,
		R32UI_MULTISAMPLE,
		RGBA32F,
		R32UI_MIPMAP,
		/* */
		LAST
	};

	enum VERSION
	{
		VALID,
		SOURCE_INVALID,
		DESTINATION_INVALID,
	};

	/* Protected methods */
	const glw::GLchar* getTestCaseName() const;
	void prepareTexture(const glw::Functions& gl, bool is_source, glw::GLuint texture_id);

	/* Protected fields */
	TEST_CASES m_test_case;

protected:
	/* Protected methods */
	std::string getFragmentShader(const glu::ContextType& contextType, bool is_case_valid,
								  glw::GLuint fetch_offset = 0);
	std::string  getGeometryShader();
	std::string  getVertexShader();
	virtual bool verifyInvalidResults(const glw::Functions& gl, glw::GLuint texture_id);
	virtual bool verifyValidResults(const glw::Functions& gl, glw::GLuint texture_id);
};

/** Implementation of test ImageLoadStore. Description follows:
 *
 * This test verifies that any "out-of-bound" access to image result in "zero"
 * or is discarded.
 *
 * Modify TexelFetch test in the following aspects:
 * - use compute shader instead of "draw" pipeline;
 * - use imageLoad instead of texelFetch;
 * - use destination image instead of framebuffer; Store texel with imageStore;
 * - for each case from TexelFetch verify:
 *   * valid coordinates for source and destination images;
 *   * invalid coordinates for destination and valid ones for source image;
 *   * valid coordinates for destination and invalid ones for source image.
 **/
class ImageLoadStoreTest : public TexelFetchTest
{
public:
	/* Public methods */
	ImageLoadStoreTest(tcu::TestContext& testCtx, glu::ApiType apiType);
	virtual ~ImageLoadStoreTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

protected:
	/* Protected methods */
	std::string getComputeShader(VERSION version, glw::GLuint coord_offset = 0, glw::GLuint sample_offset = 0);
	void setTextures(const glw::Functions& gl, glw::GLuint id_destination, glw::GLuint id_source);
	bool verifyInvalidResults(const glw::Functions& gl, glw::GLuint texture_id);
	bool verifyValidResults(const glw::Functions& gl, glw::GLuint texture_id);
};

/** Implementation of test StorageBuffer. Description follows:
 *
 * This test verifies that any "out-of-bound" access to buffer result in zero
 * or is discarded.
 *
 * Steps:
 * - prepare compute shader based on the following code snippet:
 *
 *     uint dst_index         = gl_LocalInvocationID.x;
 *     uint src_index         = gl_LocalInvocationID.x;
 *     destination[dst_index] = source[src_index];
 *
 * where source and destination are storage buffers, defined as unsized arrays
 * of floats;
 * - prepare two buffers of 4 floats:
 *   * destination filled with value 1;
 *   * source filled with unique values;
 * - dispatch program to copy all 4 values;
 * - inspect program to verify that contents of source buffer were copied to
 * destination;
 * - repeat steps for the following cases:
 *   * value of dst_index is equal to gl_LocalInvocationID.x + 16; It is
 *   expected that destination buffer will not be modified;
 *   * value of src_index is equal to gl_LocalInvocationID.x + 16; It is
 *   expected that destination buffer will be filled with value 0.
 **/
class StorageBufferTest : public RobustnessBase
{
public:
	/* Public methods */
	StorageBufferTest(tcu::TestContext& testCtx, glu::ApiType apiType);
	virtual ~StorageBufferTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

protected:
	/* Protected enums */
	enum VERSION
	{
		VALID,
		SOURCE_INVALID,
		DESTINATION_INVALID,
		/* */
		LAST
	};

	/* Private methods */
	std::string getComputeShader(glw::GLuint offset);
	bool verifyResults(glw::GLfloat* buffer_data);

	/* Protected fields */
	VERSION m_test_case;

	/* Protected constants */
	static const glw::GLfloat m_destination_data[4];
	static const glw::GLfloat m_source_data[4];
};

/** Implementation of test UniformBuffer. Description follows:
 *
 * This test verifies that any "out-of-bound" read from uniform buffer result
 * in zero;
 *
 * Modify StorageBuffer test in the following aspects:
 * - use uniform buffer for source instead of storage buffer;
 * - ignore the case with invalid value of dst_index.
 **/
class UniformBufferTest : public RobustnessBase
{
public:
	/* Public methods */
	UniformBufferTest(tcu::TestContext& testCtx, glu::ApiType apiType);
	virtual ~UniformBufferTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

protected:
	/* Protected enums */
	enum VERSION
	{
		VALID,
		SOURCE_INVALID,
		/* */
		LAST
	};

	/* Protected methods */
	std::string getComputeShader(glw::GLuint offset);
	bool verifyResults(glw::GLfloat* buffer_data);

	/* Protected fields */
	VERSION m_test_case;
};
} /* RobustBufferAccessBehavior */

/** Group class for multi bind conformance tests */
class RobustBufferAccessBehaviorTests : public tcu::TestCaseGroup
{
public:
	/* Public methods */
	RobustBufferAccessBehaviorTests(tcu::TestContext& testCtx, glu::ApiType apiType);
	virtual ~RobustBufferAccessBehaviorTests(void)
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	RobustBufferAccessBehaviorTests(const RobustBufferAccessBehaviorTests& other);
	RobustBufferAccessBehaviorTests& operator=(const RobustBufferAccessBehaviorTests& other);

	glu::ApiType m_ApiType;
};

} /* glcts */

#endif // _GLCROBUSTBUFFERACCESSBEHAVIORTESTS_HPP
