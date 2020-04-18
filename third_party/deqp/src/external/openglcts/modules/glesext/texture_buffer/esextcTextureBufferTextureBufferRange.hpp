#ifndef _ESEXTCTEXTUREBUFFERTEXTUREBUFFERRANGE_HPP
#define _ESEXTCTEXTUREBUFFERTEXTUREBUFFERRANGE_HPP
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
 * \file  esextcTextureBufferTextureBufferRange.hpp
 * \brief Texture Buffer Range Test (Test 3)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "glwEnums.hpp"
#include <map>

namespace glcts
{

/**  Implementation of (Test 3) from CTS_EXT_texture_buffer. Description follows
 *
 *   Test whether using the function TexBufferRangeEXT sub-ranges of
 *   the buffer's object data store can be correctly attached to
 *   texture buffers. The test should take into account that the offset value
 *   should be an integer multiple of TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT value.
 *
 *   Category: API, Functional Test.
 *
 *   The test should create a texture object and bind it to TEXTURE_BUFFER_EXT
 *   texture target at texture unit 0.
 *
 *   It should create a buffer object and bind it to TEXTURE_BUFFER_EXT target.
 *
 *   It should then query for the value of TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT
 *   using GetIntegerv function.
 *
 *   The test should allocate a memory block of the size equal to the sum of
 *   aligned texel sizes calculated for each format supported by texture buffer
 *   listed in table texbo.1
 *
 *   Texel size for each format supported by texture buffer should be aligned
 *   taking into consideration the value of TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT.
 *
 *   The memory block should then be filled with values up to its size.
 *
 *   Use glBufferData to initialize a buffer object's data store.
 *   glBufferData should be given a pointer to allocated memory that will be
 *   copied into the data store for initialization.
 *
 *   The test should iterate over all formats supported by texture buffer and
 *   use the buffer object as texture buffer's data store by calling
 *
 *   TexBufferRangeEXT(TEXTURE_BUFFER_EXT, format_name, buffer_id, offset,
 *       not_aligned_texel_size_for_format );
 *
 *   offset += aligned_texel_size_for_format;
 *
 *   Write a vertex shader that defines texture sampler of the type compatible
 *   with used format. Bind the sampler location to texture unit 0.
 *
 *   The vertex shader should also define an output variable outValue of the type
 *   compatible with used format and int outTextureSize output variable.
 *   Configure transform feedback to capture the value of outValue and
 *   outTextureSize.
 *
 *   In the shader execute:
 *
 *   outValue        = texelFetch( sampler_buffer, 0 );
 *   outTextureSize  = textureSize( sampler_buffer );
 *
 *   Create a program from the above vertex shader and a boilerplate fragment
 *   shader and use it.
 *
 *   Execute a draw call and copy captured outValue and outTextureSize from the
 *   buffer objects bound to transform feedback binding points to client's
 *   address space.
 *
 *   This phase of the test is successful if for each format supported by texture
 *   buffer outValue is equal to the value stored originally in the
 *   buffer object that is being used as texture buffer's data store and
 *   the value of outTextureSize is equal to 1.
 *
 *   Write a compute shader that defines image sampler of the type compatible
 *   with used format. Bind the texture buffer to image unit 0.
 *
 *   Work group size should be equal to 1 x 1 x 1.
 *
 *   The shader should also define shader storage buffer objects
 *
 *   layout(binding = 0) buffer ComputeSSBOSize
 *   {
 *       int outImageSize;
 *   } computeSSBOSize;
 *
 *   layout(binding = 1) buffer ComputeSSBOValue
 *   {
 *       compatible_type outValue;
 *   } computeSSBOValue;

 *   Initialize two buffer objects to be assigned as ssbos' data stores.
 *
 *   In the compute shader execute:
 *
 *   computeSSBOSize.outImageSize   = imageSize( image_buffer );
 *   computeSSBOValue.outValue      = imageLoad( image_buffer, 0 );
 *
 *   Call:
 *
 *   glDispatchCompute(1, 1, 1);
 *   glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
 *
 *   Map the ssbos' buffer objects' data stores to client's address space and
 *   fetch the values of outImageSize and outValue.
 *
 *   This phase of the test is successful if for each format supported by texture
 *   buffer outValue is equal to the value stored originally in the
 *   buffer object that is being used as texture buffer's data store and
 *   the value of outImageSize is equal to 1.
 */

/* Helper class containing test configuration for specific internal format */
class FormatInfo
{
public:
	/* Public methods */
	FormatInfo();
	FormatInfo(glw::GLenum internalFormat, glw::GLuint offsetAlignment);

	glw::GLuint get_aligned_size()
	{
		return m_aligned_size;
	}
	glw::GLuint get_not_aligned_size()
	{
		return m_not_aligned_size;
	}
	glw::GLuint get_ssbo_value_size()
	{
		return m_ssbo_value_size;
	}
	glw::GLenum get_internal_format()
	{
		return m_internal_format;
	}
	glw::GLuint get_n_components()
	{
		return m_n_components;
	}
	glw::GLenum get_input_type()
	{
		return m_input_type;
	}
	glw::GLenum get_output_type()
	{
		return m_output_type;
	}
	glw::GLboolean get_is_image_supported()
	{
		return m_is_image_supported;
	}
	std::string get_image_format_name()
	{
		return m_image_format_name;
	}
	std::string get_image_type_name()
	{
		return m_image_type_name;
	}
	std::string get_sampler_type_name()
	{
		return m_sampler_type_name;
	}
	std::string get_output_type_name()
	{
		return m_output_type_name;
	}
	std::string get_value_selector()
	{
		return m_value_selector;
	}

private:
	/* Private methods */
	glw::GLuint countAlignedSize(glw::GLuint offsetAlignment, glw::GLuint totalSize);
	void configure(void);

	/* Private Variables */
	glw::GLuint	m_aligned_size;
	glw::GLuint	m_not_aligned_size;
	glw::GLuint	m_ssbo_value_size;
	glw::GLenum	m_internal_format;
	glw::GLuint	m_offset_alignment;
	glw::GLuint	m_n_components;
	glw::GLenum	m_input_type;
	glw::GLenum	m_output_type;
	glw::GLboolean m_is_image_supported;
	std::string	m_image_format_name;
	std::string	m_image_type_name;
	std::string	m_sampler_type_name;
	std::string	m_output_type_name;
	std::string	m_value_selector;
};

/* Test Case Class */
class TextureBufferTextureBufferRange : public TestCaseBase
{
public:
	/* Public methods */
	TextureBufferTextureBufferRange(Context& context, const ExtParameters& extParams, const char* name,
									const char* description);

	virtual ~TextureBufferTextureBufferRange()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	virtual void initTest(void);
	std::string getComputeShaderCode(FormatInfo& info) const;
	std::string getFragmentShaderCode(FormatInfo& info) const;
	std::string getVertexShaderCode(FormatInfo& info) const;

	void fillInputData(glw::GLubyte* buffer, glw::GLuint offset, FormatInfo& info);
	void fillOutputData(glw::GLubyte* buffer, FormatInfo& info);
	void cleanIteration();

	bool checkResult(FormatInfo& info, const char* phase, bool transformFeedback);
	void logError(const char* phase, const char* internalFormat, glw::GLuint component, const char* exptectedValue,
				  const char* resultValue);

	/* Variables for general usage */
	glw::GLuint m_cs_id;
	glw::GLuint m_cs_po_id;
	glw::GLuint m_ssbo_size_id;
	glw::GLuint m_ssbo_value_id;
	glw::GLuint m_tbo_id;
	glw::GLuint m_tbo_tex_id;
	glw::GLint  m_texture_buffer_offset_alignment;
	glw::GLuint m_vao_id;

	glw::GLuint m_vs_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_vsfs_po_id;
	glw::GLuint m_tf_size_buffer_id;
	glw::GLuint m_tf_value_buffer_id;

	std::map<glw::GLenum, FormatInfo> m_configurations;
	glw::GLuint m_buffer_total_size;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBUFFERTEXTUREBUFFERRANGE_HPP
