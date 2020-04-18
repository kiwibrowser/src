#ifndef _ESEXTCGPUSHADER5SAMPLERARRAYINDEXING_HPP
#define _ESEXTCGPUSHADER5SAMPLERARRAYINDEXING_HPP
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
 * \file esextcGPUShader5SamplerArrayIndexing.hpp
 * \brief  gpu_shader5 extension - Sampler Array Indexing (Test 1)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
/** "Test 1" from CTS_EXT_gpu_shader5. Description follows
 *
 *   Test whether indexing into an array of samplers using dynamically
 *   uniform integer expressions works as expected.
 *
 *   Category:   API,
 *               Functional Test.
 *
 *   The test should set up a FBO. A 2D texture of 3x3 resolution using
 *   GL_RGBA8 internal format should be attached to its color attachment 0.
 *
 *   Write a vertex shader that declares input attribute
 *
 *   in vec4 position;
 *
 *   and assigns the value of position to gl_Position.
 *
 *   Write a fragment shader with an array of four sampler2D samplers.
 *   For each of the samplers in the array bind a 1x1 texture with
 *   GL_RGBA32F internal format. Fill the four textures with
 *   (1.0f,0,0,0), (0,1.0f,0,0), (0,0,1.0f,0), (0,0,0,1.0f) respectively.
 *
 *   Define in the fragment shader a variable
 *
 *   layout(location = 0) out vec4 outColor = vec4(0.0f,0.0f,0.0f,0.0f);
 *
 *   Write a loop that goes from 0 to 3 inclusive and for each iteration
 *   uses the loop index to index into the array of four sampler2d samples
 *   and each chosen sampler is used to read a texel from texture bound to
 *   it at coordinate (0.0f,0.0f).
 *
 *   A sum of vec4 values read from the textures should be stored
 *   in outColor.
 *
 *   Create a program from the above vertex shader and fragment shader
 *   and use it.
 *
 *   Configure buffer object as data source for the position attribute.
 *   The buffer object should be filled with data: (-1,-1,0,1),(1,-1,0,1),
 *   (-1,1,0,1),(1,1,0,1).
 *
 *   Execute a draw call glDrawArrays(GL_TRIANGLE_STRIP, 0, 4).
 *
 *   Call glReadPixels(1,1,1,1,GL_RGBA,GL_UNSIGNED_BYTE, pixel);
 *
 *   The test is successful if texel color is equal to (255,255,255,255)
 *   with GTF tolerance.
 */
class GPUShader5SamplerArrayIndexing : public TestCaseBase
{
public:
	/* Public methods */
	GPUShader5SamplerArrayIndexing(Context& context, const ExtParameters& extParams, const char* name,
								   const char* description);

	virtual ~GPUShader5SamplerArrayIndexing(void)
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	void		initTest(void);
	const char* getFragmentShaderCode();
	const char* getVertexShaderCode();

	/* Private static variables */
	static const int m_n_small_textures;
	static const int m_n_texture_components;
	static const int m_big_texture_height;
	static const int m_big_texture_width;
	static const int m_n_texture_levels;
	static const int m_small_texture_height;
	static const int m_small_texture_width;

	/* Private variables */
	glw::GLuint  m_big_to_id;
	glw::GLuint  m_fbo_id;
	glw::GLuint  m_fs_id;
	glw::GLuint  m_po_id;
	glw::GLuint* m_small_to_ids;
	glw::GLuint  m_vao_id;
	glw::GLuint  m_vbo_id;
	glw::GLuint  m_vs_id;
};

} // namespace glcts

#endif // _ESEXTCGPUSHADER5SAMPLERARRAYINDEXING_HPP
