#ifndef _ESEXTCGPUSHADER5FMAPRECISION_HPP
#define _ESEXTCGPUSHADER5FMAPRECISION_HPP
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
 * \file esextcGPUShader5FmaPrecision.hpp
 * \brief gpu_shader5 extension - fma precision Test (Test 8)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

#include "tcuVector.hpp"
#include <cstdlib>
#include <iostream>

namespace glcts
{
/** Implementation of "Test 8" from CTS_EXT_gpu_shader5. Description follows
 *
 *  Test whether the precision of fma() is conformant to the required
 *  precisions from Section 4.5.1 of the GLSL-ES 3.0 spec.
 *
 *  Category:   API,
 *              Functional Test.
 *
 *  Write a vertex shader that declares three input attributes and
 *  two output variables
 *
 *  in float a;
 *  in float b;
 *  in float c;
 *
 *  precise out float resultStd;
 *  out float resultFma;
 *
 *  In the vertex shader compute:
 *
 *  resultStd = a*b+c;
 *  resultFma = fma(a,b,c);
 *
 *  Write a boilerplate fragment shader.
 *
 *  Create a program from the vertex shader and fragment shader and use it. resultStd
 *  must use "precise" so that number of operations for a*b+c is well defined.
 *
 *  Initialize a set of buffer objects to be assigned as attributes data
 *  sources and fill each of them with 100 random float values from range
 *  [-100.0,100.0] generated using a consistent seed.
 *
 *  Configure transform feedback to capture the values of resultStd and
 *  resultFma.
 *
 *  Execute a draw call glDrawArrays(GL_POINTS, 0, 100).
 *
 *  Copy the captured results from the buffer objects bound to transform
 *  feedback binding points to unionFloatInt resultStd[100] and
 *  unionFloatInt resultFma[100].
 *
 *  Compute:
 *
 *  unionFloatInt resultCPU[100];
 *
 *  for(unsigned int i = 0; i < 100; ++i)
 *  {
 *      resultCPU[i].floatValue = data[i]*dataB[i] + dataC[i];
 *  }
 *
 *  The test is successful if
 *
 *  abs( resultCPU.intValue - resultStd.intValue ) <= 2 &&
 *  abs( resultCPU.intValue - resultFma.intValue ) <= 2 &&
 *  abs( resultStd.intValue - resultFma.intValue ) <= 2
 *
 *  for i = 0..99.
 *
 *  This test should be run against all genTypes applicable to fma.
 *  For integers the calculations should be exact.
 *
 **/

/* Define type of input data */
enum INPUT_DATA_TYPE
{
	IDT_FLOAT = 1,
	IDT_VEC2  = 2,
	IDT_VEC3  = 3,
	IDT_VEC4  = 4,
};

/* Helper for bitwise operation */
union FloatConverter {
	glw::GLfloat m_float;
	glw::GLint   m_int;
};

template <INPUT_DATA_TYPE S>
class GPUShader5FmaPrecision : public TestCaseBase
{
public:
	/* Public methods */
	GPUShader5FmaPrecision(Context& context, const ExtParameters& extParams, const char* name, const char* description);

	virtual ~GPUShader5FmaPrecision(void)
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	std::string generateVertexShaderCode();
	const char* getFragmentShaderCode();
	void		generateData();
	void		initTest(void);

	/* Static variables */
	static const glw::GLuint m_n_elements = 100;

	/* Variables for general usage */
	const glw::GLfloat m_amplitude;
	glw::GLfloat	   m_data_a[m_n_elements * S];
	glw::GLfloat	   m_data_b[m_n_elements * S];
	glw::GLfloat	   m_data_c[m_n_elements * S];
	glw::GLuint		   m_fs_id;
	glw::GLuint		   m_po_id;
	glw::GLuint		   m_vao_id;
	glw::GLuint		   m_vbo_a_id;
	glw::GLuint		   m_vbo_b_id;
	glw::GLuint		   m_vbo_c_id;
	glw::GLuint		   m_vbo_result_fma_id;
	glw::GLuint		   m_vbo_result_std_id;
	glw::GLuint		   m_vs_id;
};

} // namespace glcts

#endif // _ESEXTCGPUSHADER5FMAPRECISION_HPP
