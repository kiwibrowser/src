#ifndef _ESEXTCTEXTUREBUFFERPARAMETERS_HPP
#define _ESEXTCTEXTUREBUFFERPARAMETERS_HPP
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
 * \file  esextcTextureBufferParameters.hpp
 * \brief Texture Buffer GetTexLevelParameter and GetIntegerv test (Test 6)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"
#include <map>

namespace glcts
{

/**   Implementation of (Test 6) from CTS_EXT_texture_buffer. Description follows:
 *
 *    Test whether for correctly configured texture buffer with attached
 *    buffer object used as data store, GetTexLevelParameter and GetIntegerv
 *    functions return correct values for new tokens specified in the extension
 *    specification.
 *
 *    Category: API.
 *
 *    The test should create a texture object and bind it to TEXTURE_BUFFER_EXT
 *    texture target at texture unit 0. It should also create buffer object and
 *    bind it to TEXTURE_BUFFER_EXT target.
 *
 *    The function glGetIntegerv called with TEXTURE_BINDING_BUFFER_EXT parameter
 *    name should return the id of the texture object bound to TEXTURE_BUFFER_EXT
 *    binding point for the active texture image unit.
 *
 *    The function glGetIntegerv called with TEXTURE_BUFFER_BINDING_EXT parameter
 *    name should return the id of the buffer object bound to the TEXTURE_BUFFER_EXT
 *    binding point.
 *
 *    The test should iterate over all formats supported by texture buffer
 *    listed in table texbo.1
 *
 *    For each format it should allocate memory block of size 128 *
 *    texel_size_for_format.
 *
 *    Use glBufferData to initialize a buffer object's data store.
 *    glBufferData should be given a pointer to allocated memory that will be
 *    copied into the data store for initialization.
 *
 *    The buffer object should be used as texture buffer's data store by calling
 *
 *    TexBufferEXT(TEXTURE_BUFFER_EXT, format_name, buffer_id );
 *
 *    The function glGetTexLevelParameteriv called with
 *    TEXTURE_BUFFER_DATA_STORE_BINDING_EXT parameter name should return id of
 *    the buffer object whose data store is used by texture buffer.
 *
 *    The function glGetTexLevelParameteriv called with TEXTURE_INTERNAL_FORMAT
 *    parameter name should return the name of the used format.
 *
 *    The function glGetTexLevelParameteriv called with
 *    TEXTURE_BUFFER_OFFSET_EXT parameter name should return 0.
 *
 *    The function glGetTexLevelParameteriv called with TEXTURE_BUFFER_SIZE_EXT
 *    parameter name should return 128 * texel_size_for_format.
 *
 *    The function glGetTexLevelParameteriv called with one of the above parameter
 *    names and a non-zero lod should generate INVALID_VALUE error.
 *
 *    Call:
 *
 *    GLint offset_alignment = 0;
 *    GetIntegerv(TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT, &offset_alignment);
 *
 *    Resize the buffer's data store using glBufferData to
 *    256 * texel_size_for_format + offset_alignment while it's bound to the
 *    texture buffer.
 *
 *    The function glGetTexLevelParameteriv called with
 *    TEXTURE_BUFFER_OFFSET_EXT parameter name should return 0.
 *
 *    The function glGetTexLevelParameteriv called with TEXTURE_BUFFER_SIZE_EXT
 *    parameter name should return 256 * texel_size_for_format.
 *
 *    Call:
 *
 *    TexBufferRangeEXT(TEXTURE_BUFFER_EXT, format_name, buffer_id,
 *        offset_alignment, 256 * texel_size_for_format);
 *
 *    The function glGetTexLevelParameteriv called with
 *    TEXTURE_BUFFER_OFFSET_EXT parameter name should return offset_alignment.
 *
 *    The function glGetTexLevelParameteriv called with TEXTURE_BUFFER_SIZE_EXT
 *    parameter name should return 256 * texel_size_for_format.
 */
class TextureBufferParameters : public TestCaseBase
{
public:
	/* Public methods */
	TextureBufferParameters(Context& context, const ExtParameters& extParams, const char* name,
							const char* description);

	virtual ~TextureBufferParameters()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void initTest(void);

	glw::GLboolean queryTextureBindingBuffer(glw::GLint expected);
	glw::GLboolean queryTextureBufferBinding(glw::GLint expected);
	glw::GLboolean queryTextureBufferDataStoreBinding(glw::GLint expected);
	glw::GLboolean queryTextureBufferOffset(glw::GLint expected);
	glw::GLboolean queryTextureBufferSize(glw::GLint expected);
	glw::GLboolean queryTextureInternalFormat(glw::GLint expected);
	glw::GLboolean queryTextureInvalidLevel();

	static const glw::GLuint m_n_texels_phase_one;
	static const glw::GLuint m_n_texels_phase_two;

	typedef std::map<glw::GLint, glw::GLuint> InternalFormatsMap;

	/* Private variables */
	InternalFormatsMap m_internal_formats; /* Maps internal format to texel size for that format */
	glw::GLuint		   m_tbo_id;		   /* Texture Buffer Object*/
	glw::GLuint		   m_to_id;			   /* Texture Object*/
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBUFFERPARAMETERS_HPP
