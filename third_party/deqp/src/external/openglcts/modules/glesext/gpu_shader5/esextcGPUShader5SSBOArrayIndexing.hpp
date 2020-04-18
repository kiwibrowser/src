#ifndef _ESEXTCGPUSHADER5SSBOARRAYINDEXING_HPP
#define _ESEXTCGPUSHADER5SSBOARRAYINDEXING_HPP
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
 * \file esextcGPUShader5SSBOArrayIndexing.hpp
 * \brief GPUShader5 SSBO Array Indexing (Test 5)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
/**  Implementation of Test 5 from CTS_EXT_gpu_shader5. Description follows
 *
 *   Test whether indexing into an array of SSBOs using dynamically uniform
 *   integer expressions works as expected.
 *
 *   Category:   API,
 *               Functional Test,
 *
 *   Write a compute shader with an array of four SSBOs.
 *   The work group size should be 3x3
 *
 *   shared Buffer ComputeSSBO {
 *       uint value;
 *   } computeSSBO[4];
 *
 *   Initialize a set of buffer objects to be assigned as SSBO data sources.
 *   Set the buffer objects' data to zeros.
 *
 *   Add a uniform variable:
 *
 *   uniform uint index;
 *
 *   In the compute shader perform operations:
 *
 *   uint id = gl_LocalInvocationID.x * local_size_y + gl_LocalInvocationID.y;
 *
 *   for(uint i = 0; i < local_size_x * local_size_y; ++i)
 *   {
 *       if(id == i)
 *       {
 *           computeSSBO[index].value += id;
 *       }
 *       memoryBarrier();
 *   }
 *
 *   Create a program from the above compute shader and use it.
 *
 *   Execute:
 *
 *   glUniform1ui( indexLocation, 1);
 *   glDispatchCompute(1, 1, 1);
 *   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
 *
 *   glUniform1ui( indexLocation, 3);
 *   glDispatchCompute(1, 1, 1);
 *   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
 *
 *   Map the buffer object's data storage to program memory using glMapBuffer
 *   for all four buffer objects.
 *
 *   The test is successful if the values read from mapped buffers
 *   corresponding to computeSSBO[1].value and computeSSBO[3].value and equal
 *   to 36 and the values corresponding to computeSSBO[0].value and
 *   computeSSBO[2].value are equal to 0.
 */
class GPUShader5SSBOArrayIndexing : public TestCaseBase
{
public:
	/* Public functions */
	GPUShader5SSBOArrayIndexing(Context& context, const ExtParameters& extParams, const char* name,
								const char* description);

	virtual ~GPUShader5SSBOArrayIndexing()
	{
	}

	virtual IterateResult iterate(void);
	virtual void		  deinit(void);

private:
	/* Private variables */
	static const char*		 m_compute_shader_code;
	static const glw::GLuint m_n_arrays;

	glw::GLuint  m_compute_shader_id;
	glw::GLuint  m_program_id;
	glw::GLuint* m_ssbo_buffer_ids;

	/* Private functions */
	void initTest(void);
};

} // namespace glcts

#endif // _ESEXTCGPUSHADER5SSBOARRAYINDEXING_HPP
