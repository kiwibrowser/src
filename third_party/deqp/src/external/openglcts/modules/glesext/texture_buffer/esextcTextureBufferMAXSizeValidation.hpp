#ifndef _ESEXTCTEXTUREBUFFERMAXSIZEVALIDATION_HPP
#define _ESEXTCTEXTUREBUFFERMAXSIZEVALIDATION_HPP
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
 * \file  esextcTextureBufferMAXSizeValidation.hpp
 * \brief Texture Buffer Max Size Validation (Test 2)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{

/** Implementation of (Test 2) from CTS_EXT_texture_buffer. Description follows:
 *
 *   Test whether texture buffer can have the maximum texture size specified for
 *   this type of texture.
 *
 *   Category: API, Functional Test.
 *
 *   The test should create a texture object and bind it to TEXTURE_BUFFER_EXT
 *   texture target at texture unit 0. It should also create buffer object and
 *   bind it to TEXTURE_BUFFER_EXT target.
 *
 *   Query for the value of MAX_TEXTURE_BUFFER_SIZE_EXT using GetIntegerv function.
 *   Call glBufferData with NULL data pointer to allocate storage for the buffer
 *   object. The size of the storage should be equal to value of
 *   value(MAX_TEXTURE_BUFFER_SIZE_EXT) * sizeof(GLubyte).
 *
 *   Map the buffer object's data store to client's address space by using
 *   glMapBufferRange function (map the whole data store). The data store should
 *   then be filled with unsigned byte values up to the size of the data store
 *   using the algorithm:
 *
 *   data[index] = (GLubyte)(index % 256);
 *
 *   The data store should be unmapped using glUnmapBuffer function.
 *
 *   The buffer object should be used as texture buffer's data store by calling
 *
 *   TexBufferEXT(TEXTURE_BUFFER_EXT, GL_R8UI, buffer_id );
 *
 *   Query the value of TEXTURE_BUFFER_OFFSET_EXT and TEXTURE_BUFFER_SIZE_EXT by
 *   calling the GetTexLevelParameter function.
 *
 *   The value of TEXTURE_BUFFER_OFFSET_EXT should be equal to 0 and the value of
 *   TEXTURE_BUFFER_SIZE_EXT should be equal to value(MAX_TEXTURE_BUFFER_SIZE_EXT)
 *   * sizeof(GLubyte).
 *
 *   Write a compute shader that defines
 *
 *   uniform highp usamplerBuffer sampler_buffer;
 *
 *   The shader should also define a shader storage buffer object
 *
 *   buffer ComputeSSBO
 *   {
 *     uvec3 outValue;
 *
 *   } computeSSBO;
 *
 *   Work group size should be 1 x 1 x 1.
 *
 *   Initialize a buffer object to be assigned as ssbo's data store.
 *   Bind the sampler_buffer location to texture unit 0.
 *
 *   Perform the following calculations:
 *
 *   computeSSBO.outValue.x = uint( textureSize(sampler_buffer) );
 *   computeSSBO.outValue.y = uint( texelFetch( sampler_buffer, 0 ).x );
 *   computeSSBO.outValue.z =
 *       uint( texelFetch( sampler_buffer, computeSSBO.outValue.x - 1 ).x );
 *
 *   Call:
 *
 *   glDispatchCompute(1, 1, 1);
 *   glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
 *
 *   The test passes if
 *
 *   outValue.x ==  value(MAX_TEXTURE_BUFFER_SIZE_EXT)
 *   outValue.y ==  0
 *   outValue.z ==  ((value(MAX_TEXTURE_BUFFER_SIZE_EXT)-1) % 256).
 */
class TextureBufferMAXSizeValidation : public TestCaseBase
{
public:
	/* Public methods */
	TextureBufferMAXSizeValidation(Context& context, const ExtParameters& extParams, const char* name,
								   const char* description);

	virtual ~TextureBufferMAXSizeValidation()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void		initTest(void);
	const char* getComputeShaderCode(void);

	/* Static constant variables */
	static const glw::GLuint m_n_vec_components;

	/* Variables for general usage */
	glw::GLuint m_cs_id;
	glw::GLint  m_max_tex_buffer_size;
	glw::GLuint m_po_id;
	glw::GLuint m_ssbo_id;
	glw::GLuint m_tbo_id;
	glw::GLuint m_tbo_tex_id;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBUFFERMAXSIZEVALIDATION_HPP
