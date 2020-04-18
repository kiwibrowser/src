#ifndef _ESEXTCGPUSHADER5IMAGESARRAYINDEXING_HPP
#define _ESEXTCGPUSHADER5IMAGESARRAYINDEXING_HPP
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
 * \file esextcGPUShader5ImagesArrayIndexing.hpp
 * \brief GPUShader5 Images Array Indexing (Test 2)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
/** "Test 2" from CTS_EXT_gpu_shader5. Description follows
 *
 *   Test whether indexing into an array of images using constant
 *   expressions works as expected.
 *
 *   Category:  API,
 *              Functional Test,
 *
 *   Get the maximum size of work group in x and y direction by computing:
 *
 *   unsigned int max_wgs_x = 0;
 *   glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_wgs_x);
 *
 *   unsigned int max_wgs_y = 0;
 *   glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &max_wgs_y);
 *
 *   Write a compute shader with an array of 4 uimage2D image handlers.
 *   The work group size should be (max_wgs_x, max_wgs_y, 1).
 *
 *   For each of the uimage2D image handlers bind a (max_wgs_x, max_wgs_y)
 *   image texture with GL_RGBA32UI internal format. Data of the textures
 *   should be set to (1,1,1,1).
 *
 *   In the compute shader perform operations:
 *
 *   uvec4 texel0  = imageLoad(image[0], ivec2(gl_LocalInvocationID.x,
 *   gl_LocalInvocationID.y) );
 *
 *   uvec4 texel1  = imageLoad(image[1], ivec2(gl_LocalInvocationID.x,
 *   gl_LocalInvocationID.y) );
 *
 *   uvec4 texel2  = imageLoad(image[2], ivec2(gl_LocalInvocationID.x,
 *   gl_LocalInvocationID.y) );
 *
 *   uvec4 texel3  = imageLoad(image[3], ivec2(gl_LocalInvocationID.x,
 *   gl_LocalInvocationID.y) );
 *
 *   uvec4 addon = uvec4(gl_LocalInvocationID.x+gl_LocalInvocationID.y);
 *
 *   imageStore(image[0], ivec2( gl_LocalInvocationID.x,
 *   gl_LocalInvocationID.y), texel0  + addon);
 *
 *   imageStore(image[1], ivec2( gl_LocalInvocationID.x,
 *   gl_LocalInvocationID.y), texel1  + addon);
 *
 *   imageStore(image[2], ivec2( gl_LocalInvocationID.x,
 *   gl_LocalInvocationID.y), texel2  + addon);
 *
 *   imageStore(image[3], ivec2( gl_LocalInvocationID.x,
 *   gl_LocalInvocationID.y), texel3  + addon);
 *
 *   Create a program from the above compute shader, use it
 *   and call glDispatchCompute(1, 1, 1);
 *
 *   Read back the data from the textures corresponding to image units.
 *
 *   The test is successful if for each texture and for each texel
 *   in the texture we get (x,y) -> (x+y+1, x+y+1, x+y+1, x+y+1).
 */
class GPUShader5ImagesArrayIndexing : public TestCaseBase
{
public:
	/* Public methods */
	GPUShader5ImagesArrayIndexing(Context& context, const ExtParameters& extParams, const char* name,
								  const char* description);

	virtual ~GPUShader5ImagesArrayIndexing()
	{
	}

	virtual IterateResult iterate(void);
	virtual void		  deinit(void);

private:
	/* Private methods */
	void initTest(void);

	/* Private static variables */
	static const glw::GLuint m_array_size;
	static const glw::GLint  m_texture_n_components;

	/* Private variables */
	glw::GLuint  m_compute_shader_id;
	glw::GLuint* m_data_buffer;
	glw::GLuint  m_program_id;
	glw::GLint   m_texture_height;
	glw::GLint   m_texture_width;
	glw::GLuint* m_to_ids;
	glw::GLuint  m_fbo_id;

	/* Private functions */
	std::string getComputeShaderCode(const std::string& layout_size_x, const std::string& layout_size_y);
};

} // namespace glcts

#endif // _ESEXTCGPUSHADER5IMAGESARRAYINDEXING_HPP
