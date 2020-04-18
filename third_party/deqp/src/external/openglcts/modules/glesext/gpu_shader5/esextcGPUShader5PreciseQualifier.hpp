#ifndef _ESEXTCGPUSHADER5PRECISEQUALIFIER_HPP
#define _ESEXTCGPUSHADER5PRECISEQUALIFIER_HPP
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
 * \file esextcGPUShader5PreciseQualifier.hpp
 * \brief GPUShader5 Precise Float Test (Test Group 6)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
/**  Implementation of "Test 6" from CTS_EXT_gpu_shader5. Description follows:
 *
 *    Test whether the qualifier 'precise' prevents implementations from
 *    performing optimizations that produce slightly different results
 *    than unoptimized code. The optimizations may lead to cracks in position
 *    calculations during tessellation.
 *
 *            Category:   API,
 *                        Functional Test.
 *
 *    The test simulates computing a position of a vertex inside a patch
 *    using weighted sum of the patch vertices. To ensure that we have
 *    crack-free position calculation during tessellation, we should get
 *    with using 'precise' a bitwise accurate result regardless of the order
 *    in which the patch edges are traversed.
 *
 *    Create a vertex shader. Declare two input variables
 *
 *    in vec4 positions;
 *    in vec4 weights;
 *
 *    and one output variable
 *
 *    out vec4 weightedSum;
 *
 *    Declare functions:
 *
 *    void eval(in vec4 p, in vec4 w, precise out float result)
 *    {
 *         result = (p.x*w.x + p.y*w.y) + (p.z*w.z + p.w*w.w);
 *    }
 *
 *    float eval(in vec4 p, in vec4 w)
 *    {
 *         precise float result = (p.x*w.x + p.y*w.y) + (p.z*w.z + p.w*w.w);
 *         return result;
 *    }
 *
 *    In the vertex shader main function compute:
 *
 *    eval(positions, weights, weightedSum.x);
 *
 *    weightedSum.y = eval(positions, weights);
 *
 *    float result = 0;
 *    precise result;
 *    result =    (positions.x* weights.x + positions.y* weights.y) +
 *                (positions.z* weights.z + positions.w* weights.w);
 *    weightedSum.z = result;
 *
 *    weightedSum.w = (positions.x* weights.x + positions.y* weights.y) +
 *                    (positions.z* weights.z + positions.w* weights.w);
 *
 *    Create a boilerplate fragment shader.
 *
 *    Create a program from the above vertex shader and fragment shader
 *    and use it.
 *
 *    Configure transform feedback to capture the value of weightedSum.
 *
 *    Configure two buffer objects as data sources for the positions
 *    and weights attributes.
 *
 *    The buffer object being a data source for the positions attribute
 *    should be filled 4 random float values p1,p2,p3,p4 from range
 *    [-100.0,100.0] generated using a consistent seed.
 *
 *    The buffer object being a data source for the weights attribute
 *    should be filled with 4 random float values w1,w2,w3,w4 from range [0,1]
 *    generated using a consistent seed satisfying condition
 *    (w1 + w2 + w3 + w4) == 1.0
 *
 *    Execute a draw call glDrawArrays(GL_POINTS, 0, 1);
 *
 *    Copy the captured results from the buffer object bound to transform
 *    feedback binding point to float weightedSumForward[4] array.
 *
 *    Reverse the contents of the buffers that are fed into the shader.
 *
 *    Execute a draw call glDrawArrays(GL_POINTS, 0, 1);
 *
 *    Copy the captured results from the buffer object bound to transform
 *    feedback binding point to float weightedSumBackward[4] array.
 *
 *    The test is successful if values of
 *
 *    weightedSumForward[0], weightedSumForward[1], weightedSumForward[2],
 *    weightedSumBackward[0], weightedSumBackward[1], weightedSumBackward[2]
 *
 *    are all bitwise accurate.
 *
 *    On the other hand weightedSumForward[3] and weightedSumBackward[3]
 *    are not necessary bitwise accurate with any of the above values or even
 *    compared to each other. If precise is not used, it is likely that
 *    compiler optimizations will result in MAD or fma operations that
 *    are not exactly commutative and thus will not provide bitwise
 *    accurate results.
 *
 *    The test should be run in a loop at least 100 times, each time generating
 *    different values for positions and weights.
 */

union WeightedSum {
	float		 floatv;
	unsigned int intv;
};

class GPUShader5PreciseQualifier : public TestCaseBase
{
public:
	/* Public variables */
	GPUShader5PreciseQualifier(Context& context, const ExtParameters& extParams, const char* name,
							   const char* description);

	virtual ~GPUShader5PreciseQualifier()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private variables */
	static const char*		 m_fragment_shader_code;
	static const char*		 m_vertex_shader_code;
	static const glw::GLuint m_n_components;
	static const glw::GLuint m_n_iterations;
	static const glw::GLint  m_position_range;

	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_program_id;
	glw::GLuint m_tf_buffer_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vertex_shader_id;
	glw::GLuint m_vertex_positions_buffer_id;
	glw::GLuint m_vertex_weights_buffer_id;

	/* Private functions */
	void drawAndGetFeedbackResult(const glw::GLfloat* vertex_data_positions, const glw::GLfloat* vertex_data_weights,
								  WeightedSum* feedback_result);
	void initTest(void);
};

} // namespace glcts

#endif // _ESEXTCGPUSHADER5PRECISEQUALIFIER_HPP
