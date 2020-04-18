#ifndef _ESEXTCTEXTUREBUFFERBUFFERPARAMETERS_HPP
#define _ESEXTCTEXTUREBUFFERBUFFERPARAMETERS_HPP
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
 * \file  esextcTextureBufferBufferParameters.hpp
 * \brief GetBufferParameteriv and GetBufferPointerv test (Test 9)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

namespace glcts
{

/**   Implementation of (Test 9) from CTS_EXT_texture_buffer. Description follows:
 *
 *    Test whether for buffer object bound to TEXTURE_BUFFER_EXT target
 *    GetBufferParameteriv and GetBufferPointerv functions return correct
 *    parameters of the buffer object.
 *
 *    Category: API.
 *
 *    The test should create buffer object and bind it to TEXTURE_BUFFER_EXT target.
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_SIZE parameter name
 *    should return 0.
 *
 *    Use glBufferData to initialize the buffer object's data store.
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_SIZE parameter name
 *    should return the size specified in the above glBufferData call.
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_USAGE parameter name
 *    should return the usage specified in the above glBufferData call.
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_MAPPED parameter name
 *    should return GL_FALSE, because the buffer object's data store is not mapped.
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_MAP_OFFSET parameter
 *    name should return 0, because the buffer object's data store is not mapped.
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_MAP_LENGTH parameter
 *    name should return 0, because the buffer object's data store is not mapped.
 *
 *    The function glGetBufferPointerv called with GL_BUFFER_MAP_POINTER parameter
 *    name should return NULL, because the buffer object's data store is not mapped.
 *
 *    Map the buffer object's data store to client's address space by using
 *    glMapBufferRange function (map the whole data store).
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_MAPPED parameter name
 *    should return GL_TRUE, because whole buffer object's data store is mapped.
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_MAP_OFFSET parameter
 *    name should return 0, because the whole data store is mapped.
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_MAP_LENGTH parameter
 *    name should return the size of the data store.
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_ACCESS_FLAGS parameter
 *    name should return the access policy set while mapping the buffer object.
 *
 *    The function glGetBufferPointerv called with GL_BUFFER_MAP_POINTER parameter
 *    name should the pointer to which the buffer object's data store is currently
 *    mapped.
 *
 *    Map the buffer object's data store to client's address space by using
 *    glMapBufferRange function (map only a range of the data store).
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_MAPPED parameter name
 *    should return GL_TRUE, because a range of buffer object's data store is mapped.
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_MAP_OFFSET parameter
 *    name should return the offset specified in the above glMapBufferRange call.
 *
 *    The function GetBufferParameteriv called with GL_BUFFER_MAP_LENGTH parameter
 *    name should return the length specified in the above glMapBufferRange call.
 *
 *    The data store should be unmapped using glUnmapBuffer function.
 */
class TextureBufferBufferParameters : public TestCaseBase
{
public:
	/* Public methods */
	TextureBufferBufferParameters(Context& context, const ExtParameters& extParams, const char* name,
								  const char* description);

	virtual ~TextureBufferBufferParameters()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void initTest(void);

	glw::GLboolean queryBufferParameteriv(glw::GLenum target, glw::GLenum pname, glw::GLint expected_data);
	glw::GLboolean queryBufferParameteri64v(glw::GLenum target, glw::GLenum pname, glw::GLint64 expected_data);
	glw::GLboolean queryBufferPointerv(glw::GLenum target, glw::GLenum pname, glw::GLvoid* expected_params);

	/* Private variables */
	glw::GLuint   m_bo_id;			/* Buffer Object */
	glw::GLubyte* m_buffer_pointer; /* Pointer to mapped buffer */

	/* Private static constants */
	static const glw::GLint m_bo_size; /* Buffer object size */
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBUFFERBUFFERPARAMETERS_HPP
