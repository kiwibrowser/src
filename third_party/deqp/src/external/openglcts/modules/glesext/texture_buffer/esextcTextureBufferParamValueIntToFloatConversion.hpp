#ifndef _ESEXTCTEXTUREBUFFERPARAMVALUEINTTOFLOATCONVERSION_HPP
#define _ESEXTCTEXTUREBUFFERPARAMVALUEINTTOFLOATCONVERSION_HPP
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
 * \file  esextcTextureBufferParamValueIntToFloatConversion.hpp
 * \brief Texture Buffer - Integer To Float Conversion (Test 4)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
/** Implementation of (Test 4) from CTS_EXT_texture_buffer. Description follows
 *
 *   Test whether for texture buffer formats with unsigned normalized
 *   fixed-point components, the extracted values in the shader are correctly
 *   converted to floating-point values using equation 2.1 from OpenGL ES
 *   specification.
 *
 *   Category: API, Functional Test.
 *
 *   The test should create a texture object and bind it to TEXTURE_BUFFER_EXT
 *   texture target at texture unit 0. It should also create buffer object and
 *   bind it to TEXTURE_BUFFER_EXT target.
 *
 *   It should then allocate memory block of size 256 bytes. The memory should be
 *   initialized with subsequent unsigned byte values from the range [0 .. 255].
 *
 *   Use glBufferData to initialize a buffer object's data store.
 *   glBufferData should be given a pointer to allocated memory that will be
 *   copied into the data store for initialization.
 *
 *   The buffer object should be used as texture buffer's data store by calling
 *
 *   TexBufferEXT(TEXTURE_BUFFER_EXT, GL_R8, buffer_id );
 *
 *   Write a compute shader that defines
 *
 *   uniform highp samplerBuffer sampler_buffer;
 *
 *   Bind the sampler_buffer location to texture unit 0.
 *
 *   Work group size should be equal to 16 x 16 x 1.
 *
 *   The shader should also define a shader storage buffer object
 *
 *   buffer ComputeSSBO
 *   {
 *     float value[];
 *
 *   } computeSSBO;
 *
 *   Initialize a buffer object to be assigned as ssbo's data store. The size of
 *   this buffer object's data store should be equal to 256 * sizeof(GLfloat).
 *
 *   In the compute shader execute:
 *
 *   int index = int(gl_LocalInvocationID.x * gl_WorkGroupSize.y + gl_LocalInvocationID.y);
 *
 *   computeSSBO.value[index] = texelFetch( sampler_buffer, index ).x;
 *
 *   Call:
 *
 *   glDispatchCompute(1, 1, 1);
 *   glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
 *
 *   Map the ssbo's buffer object's data store to client's address space.
 *
 *   The test passes if for each of 256 float values read from the mapped
 *   ssbo's buffer object's data store the value of
 *
 *   fabs( computeSSBO.value[index] - (index / 255.0f) )
 *
 *   is smaller than 1.0f/256.0f epsilon tolerance.
 *
 */
class TextureBufferParamValueIntToFloatConversion : public TestCaseBase
{
public:
	/* Public methods */
	TextureBufferParamValueIntToFloatConversion(Context& context, const ExtParameters& extParams, const char* name,
												const char* description);

	virtual ~TextureBufferParamValueIntToFloatConversion()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void		initTest(void);
	std::string getComputeShaderCode(void);

	/* Variables for general usage */
	glw::GLuint m_cs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_ssbo_id;
	glw::GLuint m_tbo_id;
	glw::GLuint m_tbo_tex_id;

	/* Static constant variables */
	static glw::GLuint		  m_texture_buffer_size;
	static glw::GLuint		  m_work_group_x_size;
	static glw::GLuint		  m_work_group_y_size;
	static const glw::GLfloat m_epsilon;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBUFFERPARAMVALUEINTTOFLOATCONVERSION_HPP
