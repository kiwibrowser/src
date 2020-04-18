#ifndef _ESEXTCGPUSHADER5ATOMICCOUNTERSARRAYINDEXING_HPP
#define _ESEXTCGPUSHADER5ATOMICCOUNTERSARRAYINDEXING_HPP
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
 * \file esextcGPUShader5AtomicCountersArrayIndexing.hpp
 * \brief GPUShader5 Atomic Counters Array Indexing (Test 3)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
/** "Test 3" from CTS_EXT_gpu_shader5. Description follows
 *
 *   Test whether indexing into an array of atomic counters using dynamically
 *   uniform integer expressions works as expected.
 *
 *   Category:   API,
 *               Functional Test.
 *
 *   Write a compute shader with an array of four atomic_uint counters.
 *   The work group size should be 2x2.
 *
 *   Set up a buffer object of enough size to fit all four shader atomic
 *   counters. Fill it with zeroes.
 *
 *   In the compute shader perform operations:
 *
 *   for(uint index = 0; index <= 3; ++index)
 *   {
 *       for(uint iteration = 0; iteration <= gl_LocalInvocationID.x *
 *           local_size_y + gl_LocalInvocationID.y; ++iteration)
 *       {
 *           atomicCounterIncrement ( counters[index] );
 *       }
 *   }
 *
 *   Create a program from the above compute shader, use it and call
 *   glDispatchCompute(10, 10, 1);
 *
 *   The test is successful if the values of all counters are equal to 1000.
 */
class GPUShader5AtomicCountersArrayIndexing : public TestCaseBase
{
public:
	/* Public methods */
	GPUShader5AtomicCountersArrayIndexing(Context& context, const ExtParameters& extParams, const char* name,
										  const char* description);

	virtual ~GPUShader5AtomicCountersArrayIndexing()
	{
	}

	virtual IterateResult iterate(void);
	virtual void		  deinit(void);

private:
	/* Private methods */
	void initTest(void);

	/* Private variables */
	static const char*		 m_compute_shader_code;
	static const glw::GLuint m_atomic_counters_array_size;

	glw::GLuint m_buffer_id;
	glw::GLuint m_compute_shader_id;
	glw::GLuint m_program_id;
};
}

#endif // _ESEXTCGPUSHADER5ATOMICCOUNTERSARRAYINDEXING_HPP
