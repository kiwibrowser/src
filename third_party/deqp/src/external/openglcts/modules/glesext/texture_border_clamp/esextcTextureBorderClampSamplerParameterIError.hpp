#ifndef _ESEXTCTEXTUREBORDERCLAMPSAMPLERPARAMETERIERROR_HPP
#define _ESEXTCTEXTUREBORDERCLAMPSAMPLERPARAMETERIERROR_HPP
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
 * \file  esextcTextureBorderClampSamplerParameterIError.hpp
 * \brief Verifies glGetSamplerParameterI*() and glSamplerParameterI*()
 *        entry-points generate errors for non-generated sampler objects
 *        as per extension specification. (Test 4)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampBase.hpp"

namespace glcts
{

/** Implementation of Test 4 from CTS_EXT_texture_border_clamp. Description follows
 *
 *  Verify glGetSamplerParameterIivEXT(), glGetSamplerParameterIuivEXT(),
 *  glSamplerParameterIivEXT() and glSamplerParameterIuivEXT() generate
 *  GL_INVALID_OPERATION error if called for a non-generated sampler object.
 *
 *  Category: Negative tests.
 *  Priority: Must-have.
 */
class TextureBorderClampSamplerParameterIErrorTest : public TextureBorderClampBase
{
public:
	/* Public methods */
	TextureBorderClampSamplerParameterIErrorTest(Context& context, const ExtParameters& extParams, const char* name,
												 const char* description);

	virtual ~TextureBorderClampSamplerParameterIErrorTest()
	{
	}

	virtual IterateResult iterate(void);
	virtual void		  deinit(void);

private:
	/* Private type definitions */
	struct PnameParams
	{
		/* Constructor */
		PnameParams() : pname(0)
		{
			params[0] = 0;
			params[1] = 0;
			params[2] = 0;
			params[3] = 0;
		}

		/* Constructor.
		 *
		 * @param paramname parameter name
		 * @param param0    parameter value nr. 0
		 * @param param1    parameter value nr. 1
		 * @param param2    parameter value nr. 2
		 * @param param3    parameter value nr. 3
		 */
		PnameParams(glw::GLenum paramname, glw::GLint param0, glw::GLint param1 = 0, glw::GLint param2 = 0,
					glw::GLint param3 = 0)
			: pname(paramname)
		{
			params[0] = param0;
			params[1] = param1;
			params[2] = param2;
			params[3] = param3;
		}

		glw::GLenum pname;
		glw::GLint  params[4];
	};

	/* Private methods */
	void initTest(void);

	void VerifyGLGetCallsForAllPNames(glw::GLuint sampler_id, glw::GLenum expected_error);

	void VerifyGLGetSamplerParameterIiv(glw::GLuint sampler_id, glw::GLenum pname, glw::GLenum expected_error);

	void VerifyGLGetSamplerParameterIuiv(glw::GLuint sampler_id, glw::GLenum pname, glw::GLenum expected_error);

	void VerifyGLSamplerCallsForAllPNames(glw::GLuint sampler_id, glw::GLenum expected_error);

	void VerifyGLSamplerParameterIiv(glw::GLuint sampler_id, glw::GLenum pname, glw::GLint* params,
									 glw::GLenum expected_error);

	void VerifyGLSamplerParameterIuiv(glw::GLuint sampler_id, glw::GLenum pname, glw::GLuint* params,
									  glw::GLenum expected_error);

	/* Private variables */
	glw::GLuint	m_sampler_id;
	glw::GLboolean m_test_passed;

	std::vector<PnameParams> m_pnames_list;

	/* Private static constants */
	static const glw::GLuint m_buffer_size;
	static const glw::GLuint m_texture_unit_index;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBORDERCLAMPSAMPLERPARAMETERIERROR_HPP
