#ifndef _ESEXTCTEXTUREBUFFERATOMICFUNCTIONS_HPP
#define _ESEXTCTEXTUREBUFFERATOMICFUNCTIONS_HPP
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
 * \file  esextcTextureBufferAtomicFunctions.hpp
 * \brief Texture Buffer Atomic Functions (Test 5)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{

/** Implementation of (Test 5) from CTS_EXT_texture_buffer. Description follows
 *
 *   Test whether image atomic functions work correctly with image uniforms
 *   having texture buffers bound to their image units.
 *
 *   Category: API, Functional Test.
 *             Dependency with OES_shader_image_atomic.
 *
 *   The test should create texture object and bind it to TEXTURE_BUFFER_EXT
 *   texture target at texture unit 0.
 *
 *   It should create a buffer object and bind it to TEXTURE_BUFFER_EXT target.
 *
 *   Let n_elements = value(GL_MAX_COMPUTE_WORK_GROUP_SIZE, X axis);
 *
 *   The test should allocate a memory block of the size
 *   (n_elements + 1) * sizeof(GLunit);
 *
 *   The memory block should then be filled with subsequent unsigned integer
 *   values from the range [0..n_elements]
 *
 *   Use glBufferData to initialize a buffer object's data store.
 *   glBufferData should be given a pointer to allocated memory that will be
 *   copied into the data store for initialization.
 *
 *   The buffer object should be used as texture buffer's data store by calling
 *
 *   TexBufferEXT(TEXTURE_BUFFER_EXT, GL_R32UI, buffer_id );
 *
 *   Write a compute shader that defines
 *
 *   layout(r32ui, binding = 0) uniform highp uimageBuffer uimage_buffer;
 *
 *   Work group size should be equal to n_elements x 1 x 1.
 *
 *   In the compute shader execute:
 *
 *   uint value = imageLoad( uimage_buffer, int(gl_LocalInvocationID.x) + 1 ).x;
 *   imageAtomicAdd( uimage_buffer, 0 , value );
 *
 *   memoryBarrier();
 *
 *   value = imageLoad( uimage_buffer, 0 ).x;
 *   imageAtomicCompSwap( uimage_buffer, int(gl_LocalInvocationID.x) + 1,
 *                                       gl_LocalInvocationID.x + 1u, value );
 *
 *   Call:
 *
 *   glDispatchCompute(1, 1, 1);
 *   glMemoryBarrier(BUFFER_UPDATE_BARRIER_BIT);
 *
 *   Map the buffer object's data store to client's address space by using
 *   glMapBufferRange function (map the whole data store). The test passes if
 *   each of the (n_elements + 1) unsigned integer values in the data store is
 *   equal to
 *
 *   ((1 + n_elements) * n_elements) / 2
 */
class TextureBufferAtomicFunctions : public TestCaseBase
{
public:
	/* Public methods */
	TextureBufferAtomicFunctions(Context& context, const ExtParameters& extParams, const char* name,
								 const char* description);

	virtual ~TextureBufferAtomicFunctions()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void		initTest(void);
	std::string getComputeShaderCode(glw::GLint work_group_size) const;

	/* Variables for general usage */
	glw::GLuint m_cs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_tbo_id;
	glw::GLuint m_tbo_tex_id;
	glw::GLuint m_n_texels_in_texture_buffer;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBUFFERATOMICFUNCTIONS_HPP
