#ifndef _ESEXTCTEXTUREBUFFEROPERATIONS_HPP
#define _ESEXTCTEXTUREBUFFEROPERATIONS_HPP
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

/*!
 * \file  esextcTextureBufferOperations.hpp
 * \brief Texture Buffer Operations (Test 1)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{

/** Implementation of (Test 1) from CTS_EXT_texture_buffer. Description follows
 *
 *   Check whether texture data for the texture buffer can be specified in a number
 *   of different ways:
 *
 *   1. via buffer object loads
 *   2. via direct CPU writes
 *   3. via framebuffer readbacks to pixel buffer objects
 *   4. via transform feedback
 *   5. via image store
 *   6. via ssbo writes
 *
 *   Category: API, Functional Test.
 *
 *   The test should create a texture object and bind it to GL_TEXTURE_BUFFER_EXT
 *   texture target at texture unit 0. It should also create buffer object
 *   and bind it to TEXTURE_BUFFER_EXT target. Call glBufferData with NULL data
 *   pointer to allocate storage for the buffer object. The size of the storage
 *   should be equal to value(GL_MAX_COMPUTE_WORK_GROUP_SIZE, X axis) *
 *   sizeof(GLint) * 4.
 *
 *   The buffer object should be used as texture buffer's data store by calling
 *
 *   TexBufferExt(TEXTURE_BUFFER_EXT, GL_RGBA32I, buffer_id );
 *
 *   The data for the buffer object should be specified in the following ways:
 *
 *   ---------------------------------------------------------------------------
 *
 *   1. Use glBufferSubData to fill buffer object's data store.
 *   glBufferSubData should be given a pointer to data that will be copied into
 *   the data store for initialization.
 *
 *   ---------------------------------------------------------------------------
 *
 *   2. Map the buffer object's data store to client's address space by using
 *   glMapBufferRange function (map the whole data store). The data store should
 *   then be filled with values up to the size of the data store.
 *   The data store should be unmapped using glUnmapBuffer function.
 *
 *   ---------------------------------------------------------------------------
 *
 *   3. Create a framebuffer object and bind it to GL_DRAW_FRAMEBUFFER target.
 *   Create a 2d texture object with size
 *   value(GL_MAX_COMPUTE_WORK_GROUP_SIZE, X axis) x 1 and RGBA32I internal
 *   format. Attach the texture object to framebuffer's GL_COLOR_ATTACHMENT0
 *   attachment. Render to texture filling it completely with some predefined
 *   integer values.
 *
 *   Bind the framebuffer object to GL_READ_FRAMEBUFFER target.
 *
 *   Bind the buffer object to GL_PIXEL_PACK_BUFFER target.
 *
 *   Set the alignment requirements by calling glPixelStorei(GL_UNPACK_ALIGNMENT,1)
 *   and glPixelStorei(GL_PACK_ALIGNMENT,1).
 *
 *   Call glReadPixels(0, 0, value(GL_MAX_COMPUTE_WORK_GROUP_SIZE, X axis),
 *                     1, GL_RGBA, GL_INT, 0).
 *
 *   Bind the buffer object once again to TEXTURE_BUFFER_EXT target.
 *
 *   ---------------------------------------------------------------------------
 *
 *   4. Write a vertex shader that declares in ivec4 inPosition and
 *   out ivec4 outPosition variables and a matching fragment shader. The vertex
 *   shader should assign the value of inPosition to outPosition. The buffer object
 *   being a data source for the inPosition attribute should be filled with some
 *   integer values. Configure transform feedback to capture value of outPosition.
 *   Bind our texture buffer's buffer object as destination buffer for transform
 *   feedback operations.
 *
 *   Execute a draw call.
 *
 *   ---------------------------------------------------------------------------
 *
 *   5. Write a compute shader that defines
 *
 *   layout(rgba32i, binding = 0) uniform highp iimageBuffer image_buffer;
 *
 *   The work group size should be equal to
 *   value(GL_MAX_COMPUTE_WORK_GROUP_SIZE, X axis) x 1 x 1.
 *
 *   Bind the texture buffer to image unit 0.
 *
 *   In the compute shader execute:
 *
 *   imageStore(image_buffer, gl_LocalInvocationID.x, ivec4(gl_LocalInvocationID.x) );
 *
 *   Call:
 *
 *   glDispatchCompute(1, 1, 1);
 *   glMemoryBarrier(TEXTURE_FETCH_BARRIER_BIT);
 *
 *   ---------------------------------------------------------------------------
 *
 *   6. Write a compute shader that defines
 *
 *   buffer ComputeSSBO
 *   {
 *     ivec4 value[];
 *
 *   } computeSSBO;
 *
 *   Work group size should be equal to
 *   value(GL_MAX_COMPUTE_WORK_GROUP_SIZE, X axis) x 1 x 1.
 *
 *   Bind the buffer object to GL_SHADER_STORAGE_BUFFER target.
 *
 *   In the compute shader execute:
 *
 *   computeSSBO.value[gl_LocalInvocationID.x] = vec4(gl_LocalInvocationID.x);
 *
 *   Call:
 *
 *   glDispatchCompute(1, 1, 1);
 *   glMemoryBarrier(TEXTURE_FETCH_BARRIER_BIT);
 *
 *   Bind the buffer object once again to TEXTURE_BUFFER_EXT target.
 *
 *   ---------------------------------------------------------------------------
 *
 *   The test checks whether data for the texture buffer has been correctly
 *   specified in the above 6 different ways and if it can be accessed via imageLoad
 *   and texelFetch. In the first phase we try to access the data via imageLoad in
 *   a compute shader and in the second phase we try to access the same data
 *   via texelFetch in a vertex shader.
 *
 *   For the first phase write a compute shader that defines
 *
 *   layout(rgba32i, binding = 0) uniform highp iimageBuffer image_buffer;
 *
 *   Bind the texture buffer to image unit 0.
 *
 *   Work group size should be equal to
 *   value(GL_MAX_COMPUTE_WORK_GROUP_SIZE, X axis) x 1 x 1.
 *
 *   The shader should also define a shader storage buffer object
 *
 *   buffer ComputeSSBO
 *   {
 *     ivec4 value[];
 *
 *   } computeSSBO;
 *
 *   Initialize a buffer object to be assigned as ssbo data store. The size of
 *   this buffer object's data store should be equal to the size of
 *   the data store of the buffer object associated with texture buffer.
 *
 *   In the compute shader execute:
 *
 *   int index = int(gl_LocalInvocationID.x);
 *
 *   computeSSBO.value[index] = imageLoad( image_buffer, index);
 *
 *   Call:
 *
 *   glDispatchCompute(1, 1, 1);
 *   glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
 *
 *   For each case compare the contents of the data store of ssbo's buffer object
 *   with the values that this case should have written to the data store of the
 *   texture buffer. This phase of the test passes if for each case we get equality.
 *
 *   For the second phase of the test write a vertex shader that defines
 *
 *   in  vec4  vs_position;
 *   in  float vs_index;
 *   out float fs_index;
 *
 *   The shader should execute the following operations:
 *
 *   gl_Position = vs_position;
 *   fs_index    = vs_index;
 *
 *   Configure buffer object as data source for the vs_position attribute.
 *   The buffer object should be filled with data:
 *   (-1,-1,0,1), (1,-1,0,1), (-1,1,0,1), (1,1,0,1).
 *
 *   Configure buffer object as data source for the vs_index attribute.
 *   The buffer object should be filled with data:
 *   0.0f , 0.0f , value(GL_MAX_COMPUTE_WORK_GROUP_SIZE, X axis),
 *   value(GL_MAX_COMPUTE_WORK_GROUP_SIZE, X axis).
 *
 *   Write a fragment shader that defines;
 *
 *   in float  fs_index;
 *
 *   uniform highp isamplerBuffer sampler_buffer;
 *
 *   layout(location = 0) out ivec4 color;
 *
 *   The shader should execute the following operations:
 *
 *   color = texelFetch( sampler_buffer, int(fs_index) );
 *
 *   Create a program from the above vertex shader and fragment shader and use it.
 *
 *   Bind the sampler_buffer location to texture unit 0.
 *
 *   The test should set up a FBO. A 2D texture of
 *   value(GL_MAX_COMPUTE_WORK_GROUP_SIZE, X axis) x 1 resolution using
 *   GL_RGBA32I internal format and GL_NEAREST magnification and
 *   minification filter should be attached to its color attachment 0.
 *
 *   Execute a draw call glDrawArrays(GL_TRIANGLE_STRIP, 0, 4).
 *
 *   Use glReadPixels to read the texel data from the texture attached to color
 *   attachment 0.
 *
 *   For each case compare the contents of retrieved texel data with the values
 *   that this case should have written to the data store of the texture buffer.
 *   This phase of the test passes if for each case we get equality.
 */

/* Base Class */
class TextureBufferOperations : public TestCaseBase
{
public:
	/* Public methods */
	TextureBufferOperations(Context& context, const ExtParameters& extParams, const char* name,
							const char* description);

	virtual ~TextureBufferOperations()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	/* Protected methods */
	void checkFramebufferStatus(glw::GLenum framebuffer);
	virtual void initTest(void);
	virtual void initializeBufferObjectData(void) = 0;
	virtual void fillBufferWithData(glw::GLint* buffer, glw::GLuint bufferLength);

	/* Protected static constants */
	static const glw::GLuint m_n_vector_components;

	/* Protected variables */
	glw::GLint m_n_vectors_in_buffer_texture;

	glw::GLuint m_tb_bo_id;
	glw::GLuint m_texbuff_id;

private:
	std::string getComputeShaderCode(void) const;
	std::string getFragmentShaderCode(void) const;
	std::string getVertexShaderCode(void) const;

	void initFirstPhase(void);
	void initSecondPhase(void);
	void iterateFirstPhase(glw::GLint* result, glw::GLuint size);
	void iterateSecondPhase(glw::GLint* result);
	void deinitFirstPhase(void);
	void deinitSecondPhase(void);

	glw::GLboolean verifyResults(glw::GLint* reference, glw::GLint* result, glw::GLuint size, const char* message);

	/* Private variables */
	glw::GLuint m_cs_id;
	glw::GLuint m_po_cs_id;
	glw::GLuint m_ssbo_bo_id;

	glw::GLuint m_fbo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_po_vs_fs_id;
	glw::GLuint m_to_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vbo_id;
	glw::GLuint m_vbo_indicies_id;
	glw::GLuint m_vs_id;
	glw::GLint  m_vertex_location;
	glw::GLint  m_index_location;
};

/* Test Case 1 - buffer object loads */
class TextureBufferOperationsViaBufferObjectLoad : public TextureBufferOperations
{
public:
	/* Public methods */
	TextureBufferOperationsViaBufferObjectLoad(Context& context, const ExtParameters& extParams, const char* name,
											   const char* description);

	virtual ~TextureBufferOperationsViaBufferObjectLoad()
	{
	}

protected:
	/* Protected methods */
	virtual void initializeBufferObjectData(void);
};

/* Test Case 2 - direct CPU writes */
class TextureBufferOperationsViaCPUWrites : public TextureBufferOperations
{
public:
	/* Public methods */
	TextureBufferOperationsViaCPUWrites(Context& context, const ExtParameters& extParams, const char* name,
										const char* description);

	virtual ~TextureBufferOperationsViaCPUWrites()
	{
	}

protected:
	/* Protected methods */
	virtual void initializeBufferObjectData(void);
};

/* Test Case 3 - framebuffer readbacks to pixel buffer objects */
class TextureBufferOperationsViaFrambufferReadBack : public TextureBufferOperations
{
public:
	/* Public methods */
	TextureBufferOperationsViaFrambufferReadBack(Context& context, const ExtParameters& extParams, const char* name,
												 const char* description);

	virtual ~TextureBufferOperationsViaFrambufferReadBack()
	{
	}

	virtual void deinit(void);

private:
	/* Private methods */
	virtual void initializeBufferObjectData(void);
	virtual void fillBufferWithData(glw::GLint* buffer, glw::GLuint bufferLength);

	std::string getFBFragmentShaderCode() const;
	std::string getFBVertexShaderCode() const;

	glw::GLuint m_fb_fbo_id;
	glw::GLuint m_fb_fs_id;
	glw::GLuint m_fb_po_id;
	glw::GLuint m_fb_to_id;
	glw::GLuint m_fb_vao_id;
	glw::GLuint m_fb_vbo_id;
	glw::GLuint m_fb_vs_id;
	glw::GLint  m_position_location;
};

/* Test Case 4 - transform feedback */
class TextureBufferOperationsViaTransformFeedback : public TextureBufferOperations
{
public:
	/* Public methods */
	TextureBufferOperationsViaTransformFeedback(Context& context, const ExtParameters& extParams, const char* name,
												const char* description);

	virtual ~TextureBufferOperationsViaTransformFeedback()
	{
	}

	virtual void deinit(void);

private:
	/* Private methods */
	virtual void initializeBufferObjectData(void);

	std::string getTFVertexShaderCode() const;
	std::string getTFFragmentShaderCode() const;

	/* Private variables */
	glw::GLuint m_tf_fs_id;
	glw::GLuint m_tf_po_id;
	glw::GLuint m_tf_vao_id;
	glw::GLuint m_tf_vbo_id;
	glw::GLuint m_tf_vs_id;
	glw::GLint  m_position_location;
};

/* Test Case 5 - image store */
class TextureBufferOperationsViaImageStore : public TextureBufferOperations
{
public:
	/* Public methods */
	TextureBufferOperationsViaImageStore(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~TextureBufferOperationsViaImageStore()
	{
	}

	virtual void deinit(void);

private:
	/* Private methods */
	virtual void initializeBufferObjectData(void);
	virtual void fillBufferWithData(glw::GLint* buffer, glw::GLuint bufferLength);

	std::string getISComputeShaderCode() const;

	/* Private Variables */
	glw::GLuint m_is_cs_id;
	glw::GLuint m_is_po_id;

	/* Private static constants */
	static const glw::GLuint m_image_unit;
};

/* Test Case 6 - ssbo writes */
class TextureBufferOperationsViaSSBOWrites : public TextureBufferOperations
{
public:
	/* Public methods */
	TextureBufferOperationsViaSSBOWrites(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~TextureBufferOperationsViaSSBOWrites()
	{
	}

	virtual void deinit(void);

private:
	/* Private methods */
	virtual void initializeBufferObjectData(void);
	virtual void fillBufferWithData(glw::GLint* buffer, glw::GLuint bufferLength);

	std::string getSSBOComputeShaderCode() const;

	/* Private variables */
	glw::GLuint m_ssbo_cs_id;
	glw::GLuint m_ssbo_po_id;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBUFFEROPERATIONS_HPP
