#ifndef _ESEXTCTEXTUREBUFFERACTIVEUNIFORMVALIDATION_HPP
#define _ESEXTCTEXTUREBUFFERACTIVEUNIFORMVALIDATION_HPP
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
 * \file  esextcTextureBufferActiveUniformValidation.hpp
 * \brief Texture Buffer -  Active Uniform Value Validation (Test 8)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include <map>

namespace glcts
{

/** Implementation of (Test 8) from CTS_EXT_texture_buffer. Description follows
 *
 *   Check whether glGetActiveUniform, glGetActiveUniformsiv and
 *   glGetProgramResourceiv functions return correct information about active
 *   uniform variables of the type:
 *
 *   SAMPLER_BUFFER_EXT
 *   INT_SAMPLER_BUFFER_EXT
 *   UNSIGNED_INT_SAMPLER_BUFFER_EXT
 *   IMAGE_BUFFER_EXT
 *   INT_IMAGE_BUFFER_EXT
 *   UNSIGNED_INT_IMAGE_BUFFER_EXT
 *
 *   Category: API.
 *
 *   Write a fragment shader that defines the following uniform variables:
 *
 *   uniform highp samplerBuffer   sampler_buffer;
 *   uniform highp isamplerBuffer  isampler_buffer;
 *   uniform highp usamplerBuffer  usampler_buffer;
 *
 *   Make sure each of the uniform variables will be considered active.
 *   A uniform variable is considered active if it is determined during the link
 *   operation that it may be accessed during program execution. The easiest way
 *   to make sure the above variables are active is to call texelFetch on each
 *   texture sampler and imageLoad on each image sampler and use the returned
 *   values to determine the output color.
 *
 *   Pair the fragment shader with a boilerplate vertex shader.
 *
 *   Compile vertex and fragment shader, attach them to a program object and link
 *   the program object.
 *
 *   Get the number of active uniforms in the program object by calling
 *   glGetProgramiv with the value GL_ACTIVE_UNIFORMS and store the result in
 *   n_active_uniforms variable.
 *
 *   For index values ranging from zero to n_active_uniforms - 1 get the information
 *   about the uniform variable by calling glGetActiveUniform.
 *
 *   This phase of the test passes if among the returned information
 *   about active uniforms for the specified program object we can identify:
 *
 *   Name:               Type:
 *
 *   "sampler_buffer"    SAMPLER_BUFFER_EXT
 *   "isampler_buffer"   INT_SAMPLER_BUFFER_EXT
 *   "usampler_buffer"   UNSIGNED_INT_SAMPLER_BUFFER_EXT
 *
 *   Store which index corresponds to which variable type.
 *
 *   Create an array holding uniform variables' indices ranging from
 *   zero to n_active_uniforms - 1.
 *
 *   Call
 *
 *   glGetActiveUniformsiv( po_id, n_active_uniforms, indices_array,
 *       GL_UNIFORM_TYPE, types_array );
 *
 *   This phase of the test passes if the resulting types array holds for each
 *   index value in indices_array the same uniform type as returned previously by
 *   glGetActiveUniform called for this index.
 *
 *   For index values ranging from zero to n_active_uniforms - 1
 *   get the type information about the uniform variable by calling
 *   glGetProgramResourceiv with GL_UNIFORM program interface and GL_TYPE property.
 *
 *   This phase of the test passes if for each index value the returned type is
 *   equal to the one previously returned by glGetActiveUniform.
 *
 *   Repeat the test for a compute shader that defines the following uniform
 *   variables:
 *
 *   uniform highp   imageBuffer     image_buffer;
 *   uniform highp   iimageBuffer    iimage_buffer;
 *   uniform highp   uimageBuffer    uimage_buffer;
 *
 *   The corresponding types for the above uniform variables are:
 *
 *   IMAGE_BUFFER_EXT
 *   INT_IMAGE_BUFFER_EXT
 *   UNSIGNED_INT_IMAGE_BUFFER_EXT
 *
 */

/* Helper Sctructure storing texture confituration parameters */
class TextureParameters
{
public:
	TextureParameters();
	TextureParameters(glw::GLuint textureBufferSize, glw::GLenum textureFormat, glw::GLenum textureUniformType,
					  const char* uniformName);

	glw::GLuint get_texture_buffer_size() const
	{
		return m_texture_buffer_size;
	}

	glw::GLenum get_texture_format() const
	{
		return m_texture_format;
	}

	glw::GLenum get_texture_uniform_type() const
	{
		return m_texture_uniform_type;
	}

	std::string get_uniform_name() const
	{
		return m_uniform_name;
	}

private:
	glw::GLenum m_texture_buffer_size;
	glw::GLenum m_texture_format;
	glw::GLenum m_texture_uniform_type;
	std::string m_uniform_name;
};

/* Base Class */
class TextureBufferActiveUniformValidation : public TestCaseBase
{
public:
	/* Public methods */
	TextureBufferActiveUniformValidation(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~TextureBufferActiveUniformValidation()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	/* Protected methods */
	void addTextureParam(glw::GLenum uniformType, glw::GLenum format, glw::GLuint size, const char* name,
						 std::vector<TextureParameters>* params);

	/* Protected variables */
	glw::GLuint m_po_id;

	static const glw::GLuint m_param_value_size;

private:
	/* Private methods */
	virtual void configureParams(std::vector<TextureParameters>* params) = 0;
	virtual void configureProgram(std::vector<TextureParameters>* params, glw::GLuint* texIds) = 0;
	virtual void createProgram(void) = 0;

	virtual void initTest(void);
	const char* getUniformTypeName(glw::GLenum uniformType);
	const TextureParameters* getParamsForType(glw::GLenum uniformType) const;

	/* Variables for general usage */
	glw::GLuint*				   m_tbo_ids;
	glw::GLuint*				   m_tbo_tex_ids;
	std::vector<TextureParameters> m_texture_params;
};

/* Vertex/Fragment Shader (Case 1)*/
class TextureBufferActiveUniformValidationVSFS : public TextureBufferActiveUniformValidation
{
public:
	/* Public methods */
	TextureBufferActiveUniformValidationVSFS(Context& context, const ExtParameters& extParams, const char* name,
											 const char* description);

	virtual ~TextureBufferActiveUniformValidationVSFS()
	{
	}

	virtual void deinit(void);

private:
	/* Private methods */
	virtual void configureParams(std::vector<TextureParameters>* params);
	virtual void configureProgram(std::vector<TextureParameters>* params, glw::GLuint* texIds);
	virtual void createProgram(void);

	const char* getFragmentShaderCode(void) const;
	const char* getVertexShaderCode(void) const;

	/* Variables for general usage */
	glw::GLuint m_fs_id;
	glw::GLuint m_vs_id;
};

/* Compute Shader (Case 2)*/
class TextureBufferActiveUniformValidationCS : public TextureBufferActiveUniformValidation
{
public:
	/* Public methods */
	TextureBufferActiveUniformValidationCS(Context& context, const ExtParameters& extParams, const char* name,
										   const char* description);

	virtual ~TextureBufferActiveUniformValidationCS()
	{
	}

	virtual void deinit(void);

private:
	/* Private methods */
	virtual void configureParams(std::vector<TextureParameters>* params);
	virtual void configureProgram(std::vector<TextureParameters>* params, glw::GLuint* texIds);
	virtual void createProgram(void);

	const char* getComputeShaderCode(void) const;

	/* Variables for general usage */
	glw::GLuint m_cs_id;
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBUFFERACTIVEUNIFORMVALIDATION_HPP
