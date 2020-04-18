#ifndef _ESEXTCGPUSHADER5UNIFORMBLOCKSARRAYINDEXING_HPP
#define _ESEXTCGPUSHADER5UNIFORMBLOCKSARRAYINDEXING_HPP
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
 * \file esextcGPUShader5UniformBlocksArrayIndexing.hpp
 * \brief GPUShader5 Uniform Blocks Array Indexing Test (Test Group 4)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{

/** "Test 4" from CTS_EXT_gpu_shader5. Description follows
 *
 *   Test whether indexing into an array of uniform blocks using dynamically
 *   uniform integer expressions works as expected.
 *
 *   Category:  API,
 *              Functional Test.
 *
 *   Write a vertex shader with an array of four uniform blocks.
 *
 *   The blocks should be as specified below:
 *   uniform PositionBlock {
 *       vec4 position;
 *   } positionBlocks[4];
 *
 *   Initialize a set of buffer objects to be assigned as uniform block data
 *   sources. Subsequent buffer objects should use: (-1.0, -1.0, 0.0, 1.0),
 *   (-1.0, 1.0, 0.0, 1.0), (1.0, -1.0, 0.0, 1.0), (1.0, 1.0, 0.0, 1.0)
 *   as position vector variable values.
 *
 *   Add a uniform variable:
 *
 *   uniform uint index;
 *
 *   In the vertex shader perform operation:
 *
 *   gl_Position =  positionBlocks[index].position;
 *
 *   Write a fragment shader that assigns a (1.0,1.0,1.0,1.0) color to
 *   fragment output color.
 *
 *   Configure transform feedback to capture the value of gl_Position.
 *
 *   Create a program from the above vertex shader and fragment shader
 *   and use it.
 *
 *   Execute four draw calls:
 *
 *   glUniform1ui( indexLocation, 0);
 *   glDrawArrays(GL_POINTS, 0, 1);
 *
 *   glUniform1ui( indexLocation, 1);
 *   glDrawArrays(GL_POINTS, 0, 1);
 *
 *   glUniform1ui( indexLocation, 2);
 *   glDrawArrays(GL_POINTS, 0, 1);
 *
 *   glUniform1ui( indexLocation, 3);
 *   glDrawArrays(GL_POINTS, 0, 1);
 *
 *   After each draw call copy captured result from the buffer object bound
 *   to transform feedback binding point to outPositions[drawCallNr]
 *   array member.
 *
 *   The test is successful if the value of outPosition[0] is equal to
 *   (-1.0,-1.0,0.0,1.0), outPosition [1] is equal to (-1.0,1.0,0.0,1.0),
 *   outPosition [2] is equal to (1.0,-1.0,0.0,1.0) and outPosition[3]
 *   is equal to (1.0,1.0,0.0,1.0) - all equalities with GTF tolerance.
 */
class GPUShader5UniformBlocksArrayIndexing : public TestCaseBase
{
public:
	/* Public methods */
	GPUShader5UniformBlocksArrayIndexing(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~GPUShader5UniformBlocksArrayIndexing()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void initTest(void);

	/* Private variables */
	static const char*		  m_fragment_shader_code;
	static const glw::GLuint  m_n_array_size;
	static const glw::GLuint  m_n_position_components;
	static const glw::GLfloat m_position_data[];
	static const char*		  m_vertex_shader_code;

	glw::GLuint  m_fragment_shader_id;
	glw::GLuint  m_program_id;
	glw::GLuint  m_tf_buffer_id;
	glw::GLuint* m_uniform_buffer_ids;
	glw::GLuint  m_vertex_shader_id;
	glw::GLuint  m_vao_id;

	/* Private functions */
	bool drawAndCheckResult(glw::GLuint index_location, glw::GLuint index_value);
};

} // namespace glcts

#endif // _ESEXTCGPUSHADER5UNIFORMBLOCKSARRAYINDEXING_HPP
