#ifndef _ESEXTCGPUSHADER5FMAACCURACY_HPP
#define _ESEXTCGPUSHADER5FMAACCURACY_HPP
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
 * \file esextcGPUShader5FmaAccuracy.hpp
 * \brief gpu_shader5 extenstion - fma accuracy test (Test 7)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
/**  Implementation of "Test 7" from CTS_EXT_gpu_shader5. Test description follows:
 *
 *    Check the accuracy of the function fma() in comparison to a*b + c.
 *    The fma() should be at least as accurate as a*b+c.
 *
 *    Category:   API,
 *                Functional Test.
 *
 *    Write two vertex shaders that using Euler method compute the approximate
 *    value of y(1) for a differential equation y'(t) = y(t) with
 *    border case y(0) = 1. First shader should use the fma function and
 *    the second one standard "a*b + c" expression in each iteration of
 *    the Euler method. Both shaders should store the result in
 *    out float Result variable.
 *
 *    Explanation:
 *
 *    For a differential equation y'(t) = y(t) with edge case y(0) = 1
 *    the exact solution is y(t) = et . It means that for y(0) we get 1
 *    and for y(1) we get e ~ 2.71828.
 *
 *    Sometimes we can't solve differential equation for exact solution
 *    (get the actual y function), but we can still get an approximate value of
 *    y(t) for a chosen t, by solving the equation numerically. The simplest
 *    numerical method that can be applied to the problem is called Euler method.
 *
 *    This method start from the border case y(0) = 1 (t0 = 0, y0 = 1) and
 *    in n sub steps converges slowly to the value of y(1) (tn = 1, yn = ?).
 *    It does that by computing in a loop the equation
 *    y(tx+1) = y(tx) + 1/n * y(tx) for x = 0..n-1.
 *    After the last iteration the value of y(tn) is the approximate value of
 *    y(1) and should be close to e ~ 2.71828. The Euler method not always
 *    converges to the solution but for the differential equation y'(t) = y(t)
 *    it should converge without any problems.
 *
 *    The equation y(tx+1) = y(tx) + 1/n * y(tx) is ideal to be implemented
 *    using fma function in the following way:
 *
 *    y(tx+1) = fma(1/n, y(tx), y(tx)).
 *
 *    The shaders should be configurable by a uint uniform variable n
 *    in the number of subintervals the interval [0,1] is divided into
 *    while computing the value of y(1).
 *
 *    Write a boilerplate fragment shader.
 *
 *    Create a program from the first vertex shader and fragment shader and
 *    a second program from the second vertex shader and fragment shader.
 *
 *    Use the first program.
 *
 *    Configure transform feedback to capture the value of Result.
 *
 *    Execute a draw call glDrawArrays(GL_POINTS, 0, 1) five times in a row.
 *    Before each execution, double the value of n (n  should first be
 *    set to 10).
 *
 *    Copy the captured results from the buffer object bound to transform
 *    feedback binding point to resultsFmaArray.
 *
 *    Use the second program.
 *
 *    Configure transform feedback to capture the value of Result.
 *
 *    Execute a draw call glDrawArrays(GL_POINTS, 0, 1) 10 times in a row.
 *    Before each execution double the value of n (n should first be set
 *    to 10).
 *
 *    Copy the captured results from the buffer object bound to transform
 *    feedback to resultsNotFmaArray.
 *
 *    For each of the values stored in the array resultsFmaArray compute
 *    a relative error of the value with correspondence to a reference value
 *    of y(1) which is e ~ 2.71828. Sum up those relative errors to a variable
 *    precise float totalRelativeErrorFma.
 *
 *    Do the same for the array resultsNotFmaArray, this time storing the sum
 *    in precise float totalRelativeErrorNotFma.
 *
 *    The test passes if the absolute value of the totalRelativeErrorFma
 *    is smaller or equal to totalRelativeErrorNotFma.
 **/
class GPUShader5FmaAccuracyTest : public TestCaseBase
{
public:
	/* Public methods */
	GPUShader5FmaAccuracyTest(Context& context, const ExtParameters& extParams, const char* name,
							  const char* description);

	virtual ~GPUShader5FmaAccuracyTest(void)
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void calculateRelativeError(glw::GLfloat result, glw::GLfloat expected_result, glw::GLfloat& relative_error);
	void executePass(glw::GLuint program_object_id, glw::GLfloat* results);
	glw::GLuint getNumberOfStepsForIndex(glw::GLuint index);
	void initTest(void);
	void logArray(const char* description, glw::GLfloat* data, glw::GLuint length);

	/* Private variables */
	/* Program and shader ids */
	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_program_object_id_for_float_pass;
	glw::GLuint m_program_object_id_for_fma_pass;
	glw::GLuint m_vertex_shader_id_for_float_pass;
	glw::GLuint m_vertex_shader_id_for_fma_pass;

	/* Buffer object used for transform feedback */
	glw::GLuint m_buffer_object_id;

	/* Vertex array object */
	glw::GLuint m_vertex_array_object_id;

	/* Size of buffer used for transform feedback */
	static const glw::GLuint m_buffer_size;

	/* Expected solution */
	static const glw::GLfloat m_expected_result;

	/* Number of draw call executions */
	static const glw::GLuint m_n_draw_call_executions;

	/* Shaders' code */
	static const glw::GLchar* const m_fragment_shader_code;
	static const glw::GLchar* const m_vertex_shader_code_for_fma_pass;
	static const glw::GLchar* const m_vertex_shader_code_for_float_pass;
};

} /* glcts */

#endif // _ESEXTCGPUSHADER5FMAACCURACY_HPP
