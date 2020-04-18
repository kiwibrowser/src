#ifndef _ESEXTCTEXTUREBUFFERERRORS_HPP
#define _ESEXTCTEXTUREBUFFERERRORS_HPP
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
 * \file  esextcTextureBufferErrors.hpp
 * \brief TexBufferEXT and TexBufferRangeEXT errors (Test 7)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

#include <vector>

namespace glcts
{

/*    Implementation of (Test 7) from CTS_EXT_texture_buffer. Description follows:
 *
 *    Test whether functions TexBufferEXT and TexBufferRangeEXT generate errors
 *    as specified in the extension specification.
 *
 *    Category: API.
 *              Dependency with OES_texture_storage_multisample_2d_array
 *
 *    Check if for all targets apart from TEXTURE_BUFFER_EXT functions
 *    TexBufferEXT and TexBufferRangeEXT report INVALID_ENUM error.
 *
 *    Check if for GL_DEPTH_COMPONENT32F internal format functions
 *    TexBufferEXT and TexBufferRangeEXT report INVALID_ENUM error.
 *
 *    Check if for buffer id not previously returned by genBuffers and
 *    not equal to zero functions TexBufferEXT and TexBufferRangeEXT report
 *    INVALID_OPERATION error.
 *
 *    Check if for offset parameter that is negative, size parameter that is less
 *    than or equal to zero or offset + size that is greater than the value of
 *    GL_BUFFER_SIZE for the buffer object bound to TEXTURE_BUFFER_EXT target
 *    function TexBufferRangeEXT reports INVALID_VALUE error.
 *
 *    Check if in case of offset parameter not being an integer multiple of
 *    value(TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT) function TexBufferRangeEXT
 *    reports INVALID_VALUE error.
 */
class TextureBufferErrors : public TestCaseBase
{
public:
	/* Public methods */
	TextureBufferErrors(Context& context, const ExtParameters& extParams, const char* name, const char* description);

	virtual ~TextureBufferErrors()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void		   initTest(void);
	glw::GLboolean verifyError(const glw::GLenum expected_error, const char* description);

	typedef std::vector<glw::GLenum> TargetsVector;

	/* Private variables */
	glw::GLuint   m_bo_id;			 /* Buffer  Object */
	glw::GLuint   m_tex_id;			 /* Texture Buffer Object Texture ID */
	TargetsVector m_texture_targets; /* Texture Targets */

	/* Private static constants */
	static const glw::GLint m_bo_size; /* Buffer object size */
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBUFFERERRORS_HPP
