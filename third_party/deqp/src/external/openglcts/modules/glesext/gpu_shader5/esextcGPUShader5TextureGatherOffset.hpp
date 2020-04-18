#ifndef _ESEXTCGPUSHADER5TEXTUREGATHEROFFSET_HPP
#define _ESEXTCGPUSHADER5TEXTUREGATHEROFFSET_HPP
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
 * \file esextcGPUShader5TextureGatherOffset.hpp
 * \brief gpu_shader5 extension - texture gather offset tests (Test 9 and 10)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

#include <string>
#include <vector>

namespace glcts
{
/** Base class for texture gather offset tests (9, 10 and 11)
 *
 **/
class GPUShader5TextureGatherOffsetTestBase : public TestCaseBase
{
public:
	/* Public methods */
	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	/* Protected types */
	struct VertexBufferInfo
	{
		const glw::GLchar* attribute_name;
		glw::GLuint		   n_components;
		glw::GLenum		   type;
		glw::GLvoid*	   data;
		glw::GLuint		   data_size;
	};

	/* Protected methods */
	GPUShader5TextureGatherOffsetTestBase(Context& context, const ExtParameters& extParams, const char* name,
										  const char* description);

	virtual ~GPUShader5TextureGatherOffsetTestBase(void)
	{
	}

	virtual void initTest(void);

	/* To be implemented by inheriting classes */
	virtual void getBorderColor(glw::GLfloat out_color[4])								  = 0;
	virtual void getShaderParts(std::vector<const glw::GLchar*>& out_vertex_shader_parts) = 0;

	virtual void getTextureInfo(glw::GLuint& out_width, glw::GLenum& out_texture_internal_format,
								glw::GLenum& out_texture_format, glw::GLenum& out_texture_type,
								glw::GLuint& out_bytes_per_pixel) = 0;

	virtual void getTextureWrapMode(glw::GLenum& out_wrap_mode) = 0;

	virtual void getTransformFeedBackDetails(glw::GLuint&					  buffer_size,
											 std::vector<const glw::GLchar*>& captured_varyings) = 0;

	virtual void isTextureArray(bool& out_is_texture_array)									  = 0;
	virtual void prepareTextureData(glw::GLubyte* data)										  = 0;
	virtual void prepareVertexBuffersData(std::vector<VertexBufferInfo>& vertex_buffer_infos) = 0;
	virtual bool verifyResult(const void* result_data)										  = 0;

	/* Utilities */
	void logArray(const glw::GLint* data, unsigned int length, const char* description);

	void logCoordinates(unsigned int index);

	/* Protected fields */
	/* GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET and GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET pname values */
	glw::GLint m_min_texture_gather_offset;
	glw::GLint m_max_texture_gather_offset;

	/* Number of drawn vertices */
	static const unsigned int m_n_vertices;

	/* Number of components in captured varyings */
	static const unsigned int m_n_components_per_varying;

private:
	/* Private methods */
	void prepareProgramInput();
	void prepareTexture();
	void prepareVertexBufferInfoForCoordinates();
	void setCoordinatesData(glw::GLfloat x, glw::GLfloat y, unsigned int index);
	void setCoordinatesData(glw::GLfloat x, glw::GLfloat y, glw::GLfloat z, unsigned int index);

	/* Private fields */
	/* Program and shader ids */
	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_program_object_id;
	glw::GLuint m_vertex_shader_id;

	/* Vertex array object */
	glw::GLuint m_vertex_array_object_id;

	/* Texture id */
	glw::GLuint m_texture_object_id;

	/* Sampler id */
	glw::GLuint m_sampler_object_id;

	/* Shaders' code */
	static const glw::GLchar* const m_fragment_shader_code;
	std::vector<const glw::GLchar*> m_vertex_shader_parts;

	/* Name of uniforms */
	static const glw::GLchar* const m_sampler_uniform_name;
	static const glw::GLchar* const m_reference_sampler_uniform_name;

	/* Vertex buffer infos */
	std::vector<VertexBufferInfo> m_vertex_buffer_infos;
	std::vector<glw::GLuint>	  m_vertex_buffer_ids;

	/* Texture info */
	bool		m_is_texture_array;
	glw::GLuint m_texture_bytes_per_pixel;
	glw::GLenum m_texture_format;
	glw::GLenum m_texture_internal_format;
	glw::GLenum m_texture_type;
	glw::GLuint m_texture_size;
	glw::GLenum m_texture_wrap_mode;

	/* Texture array length */
	static const unsigned int m_n_texture_array_length;

	/* Name of varyings */
	std::vector<const glw::GLchar*> m_captured_varying_names;

	/* Size of buffer used for transform feedback */
	glw::GLuint m_transform_feedback_buffer_size;

	/* Buffer object used in transform feedback */
	glw::GLuint m_transform_feedback_buffer_object_id;

	/* Storage for texture coordinates */
	std::vector<glw::GLfloat> m_coordinates_buffer_data;

	/* Number of texture coordinates per vertex */
	unsigned int m_n_coordinates_components;

	/* Name of texture coordinate attribute */
	static const glw::GLchar* const m_coordinates_attribute_name;

	/* Configuration of texture coordinate values generation */
	static const int m_max_coordinate_value;
	static const int m_min_coordinate_value;
	static const int m_coordinate_resolution;
};

/** Base class for test 9 and 11
 *
 **/
class GPUShader5TextureGatherOffsetColorTestBase : public GPUShader5TextureGatherOffsetTestBase
{
protected:
	/* Protected types */
	struct CapturedVaryings
	{
		glw::GLint without_offset_0[4];
		glw::GLint without_offset_1[4];
		glw::GLint without_offset_2[4];
		glw::GLint without_offset_3[4];

		glw::GLint with_offset_0[4];
		glw::GLint with_offset_1[4];
		glw::GLint with_offset_2[4];
		glw::GLint with_offset_3[4];
	};

	/* Protected methods */
	GPUShader5TextureGatherOffsetColorTestBase(Context& context, const ExtParameters& extParams, const char* name,
											   const char* description);

	virtual ~GPUShader5TextureGatherOffsetColorTestBase(void)
	{
	}

	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getBorderColor(glw::GLfloat out_color[4]);

	virtual void getTextureInfo(glw::GLuint& out_size, glw::GLenum& out_texture_internal_format,
								glw::GLenum& out_texture_format, glw::GLenum& out_texture_type,
								glw::GLuint& out_bytes_per_pixel);

	virtual void getTextureWrapMode(glw::GLenum& out_wrap_mode);

	virtual void getTransformFeedBackDetails(glw::GLuint&					  out_buffer_size,
											 std::vector<const glw::GLchar*>& out_captured_varyings);

	virtual void isTextureArray(bool& out_is_texture_array);
	virtual void prepareTextureData(glw::GLubyte* data);
	virtual bool verifyResult(const void* result_data);

	/* Methods to be implemented by child classes */
	virtual bool checkResult(const CapturedVaryings& captured_data, unsigned int index,
							 unsigned int m_texture_size) = 0;

	/* Utilities */
	void logVaryings(const CapturedVaryings& varyings);

private:
	/* Texture size */
	static const glw::GLuint m_texture_size;

	/* Number of varyings captured per vertex */
	unsigned int m_n_varyings_per_vertex;
};

/** Base class for test 10 and 11
 *
 **/
class GPUShader5TextureGatherOffsetDepthTestBase : public GPUShader5TextureGatherOffsetTestBase
{
protected:
	/* Protected types */
	struct CapturedVaryings
	{
		glw::GLint floor_tex_coord[4];
		glw::GLint without_offset[4];
		glw::GLint with_offset[4];
	};

	/* Protected methods */
	GPUShader5TextureGatherOffsetDepthTestBase(Context& context, const ExtParameters& extParams, const char* name,
											   const char* description);

	virtual ~GPUShader5TextureGatherOffsetDepthTestBase(void)
	{
	}

	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getBorderColor(glw::GLfloat out_color[4]);

	virtual void getTextureInfo(glw::GLuint& out_width, glw::GLenum& out_texture_internal_format,
								glw::GLenum& out_texture_format, glw::GLenum& out_texture_type,
								glw::GLuint& out_bytes_per_pixel);

	virtual void getTextureWrapMode(glw::GLenum& out_wrap_mode);

	virtual void getTransformFeedBackDetails(glw::GLuint&					  out_buffer_size,
											 std::vector<const glw::GLchar*>& out_captured_varyings);

	virtual void isTextureArray(bool& out_is_texture_array);
	virtual void prepareTextureData(glw::GLubyte* data);
	virtual bool verifyResult(const void* result_data);

	/* Methods to be implemented by child classes */
	virtual bool checkResult(const CapturedVaryings& captured_data, unsigned int index, unsigned int texture_size) = 0;

	virtual void getVaryings(std::vector<const glw::GLchar*>& out_captured_varyings);

	/* Utilities */
	void logVaryings(const CapturedVaryings& varyings);

private:
	/* Texture size */
	static const glw::GLuint m_texture_size;

	/* Number of varyings captured per vertex */
	unsigned int m_n_varyings_per_vertex;
};

/** Implementation of "Test 9" from CTS_EXT_gpu_shader5. Test description follows:
 *
 *  Test whether using non constant offsets in the textureGatherOffset
 *  and constant offsets in the textureGatherOffsets family of
 *  functions works as expected for sampler2D and sampler2DArray and
 *  GL_REPEAT wrap mode.
 *
 *  Category:   API,
 *              Functional Test.
 *
 *  Create a 64 x 64 texture with internal format GL_RGBA32I.
 *
 *  Set both GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER to GL_NEAREST.
 *
 *  Set both GL_TEXTURE_WRAP_S and GL_TEXTURE_WRAP_T to GL_REPEAT.
 *
 *  Fill the 4 components of each texel with values corresponding to texel
 *  row and column number (x,y) -> (x,y,x,y)
 *
 *  Write a vertex shader that defines:
 *
 *  uniform isampler2D sampler;
 *
 *  in ivec2 tgoOffset;
 *  in  vec2 texCoords;
 *
 *  out ivec4 withoutOffset0;
 *  out ivec4 withOffset0;
 *
 *  out ivec4 withoutOffset1;
 *  out ivec4 withOffset1;
 *
 *  out ivec4 withoutOffset2;
 *  out ivec4 withOffset2;
 *
 *  out ivec4 withoutOffset3;
 *  out ivec4 withOffset3;
 *
 *  Bind the sampler to texture unit the texture we created is assigned to.
 *
 *  Initialize a buffer object to be assigned as input attribute tgoOffset
 *  data source. The buffer object should hold 128 integer tuples. Fill it
 *  with some random integer values but falling into a range of
 *  (MIN_PROGRAM_TEXTURE_GATHER_OFFSET,
 *   MAX_PROGRAM_TEXTURE_GATHER_OFFSET).
 *
 *  Initialize a buffer object to be assigned as input attribute texCoords
 *  data source. The buffer object should hold 128 elements. Fill the first
 *  4 tuples with (0.0, 0.0) , (0.0, 1.0), (1.0, 0.0), (1.0, 1.0) and the
 *  rest with some random float values in the range [-8.0..8.0].
 *
 *  In the vertex shader perform the following operation:
 *
 *  withoutOffset0 = textureGather(sampler, texCoords, 0);
 *  withOffset0 = textureGatherOffset(sampler, texCoords, tgoOffset, 0);
 *
 *  withoutOffset1 = textureGather(sampler, texCoords, 1);
 *  withOffset1 = textureGatherOffset(sampler, texCoords, tgoOffset, 1);
 *
 *  withoutOffset2 = textureGather(sampler, texCoords, 2);
 *  withOffset2 = textureGatherOffset(sampler, texCoords, tgoOffset, 2);
 *
 *  withoutOffset3 = textureGather(sampler, texCoords, 3);
 *  withOffset3 = textureGatherOffset(sampler, texCoords, tgoOffset, 3);
 *
 *  Configure transform feedback to capture the values of withoutOffset*
 *  and withOffset*.
 *
 *  Write a boilerplate fragment shader.
 *
 *  Create a program from the above vertex shader and fragment shader
 *  and use it.
 *
 *  Execute a draw call glDrawArrays(GL_POINTS, 0, 128).
 *
 *  Copy the captured results from the buffer objects bound to transform
 *  feedback binding points.
 *
 *  Using the captured values for each of the 128 results compute
 *
 *  (Pseudocode)
 *
 *  i = 0...127
 *
 *  ivec2 referenceOffset = offsets[i];
 *
 *  if(referenceOffset[0] < 0 )
 *  {
 *      referenceOffset[0] = sizeX - (referenceOffset[0] % sizeX);
 *  }
 *  if(referenceOffset[1] < 0 )
 *  {
 *      referenceOffset[1] = sizeY - (referenceOffset[1] % sizeY);
 *  }
 *
 *  for(int tNr = 0 tNr < 4; ++tNr)
 *  {
 *      ivec2 referenceTexelValue01 = ivec2(
 *       ((tgoResults[i].withoutOffset0[tNr] + referenceOffset[0]) % sizeX),
 *       ((tgoResults[i].withoutOffset1[tNr] + referenceOffset[1]) % sizeY));
 *
 *      ivec2 referenceTexelValue23 = ivec2(
 *       ((tgoResults[i].withoutOffset2[tNr] + referenceOffset[0]) % sizeX),
 *       ((tgoResults[i].withoutOffset3[tNr] + referenceOffset[1]) % sizeY));
 *
 *      ivec2 texelValue01 = ivec2(
 *       tgoResults[i].withOffset0[tNr], tgoResults[i].withOffset1[tNr]);
 *
 *      ivec2 texelValue23 = ivec2(
 *       tgoResults[i].withOffset2[tNr], tgoResults[i].withOffset3[tNr]);
 *  }
 *
 *  The test passes if in all cases we have
 *  referenceTexelValue01 == texelValue01 and
 *  referenceTexelValue23 == texelValue23.
 *
 *  Repeat the same type of test for sampler2DArray.
 *
 *  Repeat the same type of test using textureGatherOffsets instead of
 *  textureGatherOffset inputing constant offsets
 *  offsets[4] = {
 *  (MIN_PROGRAM_TEXTURE_GATHER_OFFSET, MIN_PROGRAM_TEXTURE_GATHER_OFFSET),
 *  (MIN_PROGRAM_TEXTURE_GATHER_OFFSET, MAX_PROGRAM_TEXTURE_GATHER_OFFSET),
 *  (MAX_PROGRAM_TEXTURE_GATHER_OFFSET, MIN_PROGRAM_TEXTURE_GATHER_OFFSET),
 *  (MAX_PROGRAM_TEXTURE_GATHER_OFFSET, MAX_PROGRAM_TEXTURE_GATHER_OFFSET)
 *  };
 **/

/** Test configuration: wrap mode GL_REPEAT, sampler isampler2D, function textureGatherOffset
 */
class GPUShader5TextureGatherOffsetColor2DRepeatCaseTest : public GPUShader5TextureGatherOffsetColorTestBase
{
public:
	/* Public methods */
	GPUShader5TextureGatherOffsetColor2DRepeatCaseTest(Context& context, const ExtParameters& extParams,
													   const char* name, const char* description);

	virtual ~GPUShader5TextureGatherOffsetColor2DRepeatCaseTest(void)
	{
	}

protected:
	/* Protected methods */
	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getShaderParts(std::vector<const glw::GLchar*>& out_vertex_shader_parts);
	virtual void prepareVertexBuffersData(std::vector<VertexBufferInfo>& vertex_buffer_infos);

	/* Virtual methods from GPUShader5TextureGatherOffsetColorTestBase */
	virtual bool checkResult(const CapturedVaryings& captured_data, unsigned int index, unsigned int texture_size);

	/* Utilities */
	void getOffsets(glw::GLint& out_x_offset, glw::GLint& out_y_offset, unsigned int index);

private:
	/* Private fields */
	/* Vertex shader code */
	static const glw::GLchar* const m_vertex_shader_code;

	/* Number of offsets per vertex */
	static const unsigned int m_n_offsets_components;

	/* Name of offset attribute */
	static const glw::GLchar* const m_offsets_attribute_name;

	/* Storage for offsets */
	std::vector<glw::GLint> m_offsets_buffer_data;
};

/** Test configuration: wrap mode GL_REPEAT, sampler isampler2DArray, function textureGatherOffset
 */
class GPUShader5TextureGatherOffsetColor2DArrayCaseTest : public GPUShader5TextureGatherOffsetColor2DRepeatCaseTest
{
public:
	/* Public methods */
	GPUShader5TextureGatherOffsetColor2DArrayCaseTest(Context& context, const ExtParameters& extParams,
													  const char* name, const char* description);

	virtual ~GPUShader5TextureGatherOffsetColor2DArrayCaseTest(void)
	{
	}

protected:
	/* Protected methods */
	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getShaderParts(std::vector<const glw::GLchar*>& out_vertex_shader_parts);
	virtual void isTextureArray(bool& out_is_texture_array);

private:
	/* Private fields */
	/* Vertex shader code */
	static const glw::GLchar* const m_vertex_shader_code;
};

/** Test configuration: wrap mode GL_REPEAT, sampler isampler2D, function textureGatherOffsets
 */
class GPUShader5TextureGatherOffsetColor2DOffsetsCaseTest : public GPUShader5TextureGatherOffsetColorTestBase
{
public:
	/* Public methods */
	GPUShader5TextureGatherOffsetColor2DOffsetsCaseTest(Context& context, const ExtParameters& extParams,
														const char* name, const char* description);

	virtual ~GPUShader5TextureGatherOffsetColor2DOffsetsCaseTest(void)
	{
	}

protected:
	/* Protected methods */
	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getShaderParts(std::vector<const glw::GLchar*>& out_vertex_shader_parts);
	virtual void prepareVertexBuffersData(std::vector<VertexBufferInfo>& vertex_buffer_infos);

	/* Virtual methods from GPUShader5TextureGatherOffsetColorTestBase */
	virtual bool checkResult(const CapturedVaryings& captured_data, unsigned int index, unsigned int texture_size);

private:
	/* Private fields */
	/* Vertex shader code */
	static const glw::GLchar* const m_vertex_shader_code_preamble;
	static const glw::GLchar* const m_vertex_shader_code_body;

	/* String used for definition of constant offsets */
	std::string m_vertex_shader_code_offsets;
};

/** Implementation of "Test 10" from CTS_EXT_gpu_shader5. Test description follows:
 *
 *  Test whether using non constant offsets in the textureGatherOffset
 *  and constant offsets in the textureGatherOffsets family of
 *  functions works as expected for sampler2DShadow
 *  and sampler2DArrayShadow and GL_REPEAT wrap mode.
 *
 *  Category:   API,
 *              Functional Test.
 *
 *  Create a 64 x 64 texture with internal format GL_DEPTH_COMPONENT and
 *  type GL_FLOAT.
 *
 *  Set both GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER to GL_NEAREST.
 *
 *  Set both GL_TEXTURE_WRAP_S and GL_TEXTURE_WRAP_T to GL_REPEAT.
 *
 *  Setup the texture parameters
 *
 *  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
 *                  COMPARE_REF_TO_TEXTURE);
 *
 *  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
 *
 *  Fill each texel with depth value corresponding to its position
 *  in the texture for the X axis: (x, y) -> (x / sizeX);
 *
 *  Write a vertex shader that defines:
 *
 *  uniform sampler2DShadow sampler;
 *
 *  in ivec2 tgoOffset;
 *  in  vec2 texCoords;
 *
 *  out ivec4 withoutOffset;
 *  out ivec4 withOffset;
 *
 *  Bind the sampler to texture unit the texture we created is assigned to.
 *
 *  Initialize a buffer object to be assigned as input attribute tgoOffset
 *  data source. The buffer object should hold 128 integer tuples. Fill it
 *  with some random integer values but falling into a range of
 *  (MIN_PROGRAM_TEXTURE_GATHER_OFFSET,
 *   MAX_PROGRAM_TEXTURE_GATHER_OFFSET).
 *
 *  Initialize a buffer object to be assigned as input attribute texCoords
 *  data source. The buffer object should hold 128 elements. Fill the first
 *  4 tuples with (0.0, 0.0) , (0.0, 1.0), (1.0, 0.0), (1.0, 1.0) and the
 *  rest with some random float values in the range [-8.0..8.0].
 *
 *  In the vertex shader perform the following operation:
 *
 *  withoutOffset = ivec4(0,0,0,0);
 *  withOffset    = ivec4(0,0,0,0);
 *
 *  ivec2 texSize = textureSize2D(sampler, 0);
 *
 *  for(int texelNr = 0 texelNr < texSize.x; ++texelNr)
 *  {
 *      float refZ =  float(texelNr) / float(texSize.x);
 *
 *      withoutOffset += textureGather( sampler, texCoords, refZ );
 *      withOffset += textureGatherOffset(sampler,texCoords,offset,refZ);
 *  }
 *
 *  Configure transform feedback to capture the values of withoutOffset
 *  and withOffset.
 *
 *  Write a boilerplate fragment shader.
 *
 *  Create a program from the above vertex shader and fragment shader
 *  and use it.
 *
 *  Execute a draw call glDrawArrays(GL_POINTS, 0, 128).
 *
 *  Copy the captured results from the buffer objects bound to transform
 *  feedback binding points.
 *
 *  Using the captured values for each of the 128 results compute
 *
 *  (Pseudocode)
 *
 *  i = 0...127
 *
 *  int referenceOffsetX = offsets[i].x;
 *
 *  if(referenceOffsetX < 0 )
 *  {
 *      referenceOffsetX = sizeX - (referenceOffsetX % sizeX);
 *  }
 *
 *  for(int tNr = 0 tNr < 4; ++tNr)
 *  {
 *      int referenceTexelValueX =
 *       (tgoResults[i].withoutOffset[tNr] + referenceOffsetX) % sizeX;
 *
 *      int texelValueX = tgoResults[i].withOffset[tNr];
 *  }
 *
 *  The test passes if in all cases we have
 *  referenceTexelValueX == texelValueX.
 *
 *  Repeat the same test for Y axis.
 *
 *  Repeat the same type of test for sampler2DArrayShadow.
 *
 *  Repeat the same type of test using textureGatherOffsets instead of
 *  textureGatherOffset inputting constant offsets
 *  offsets[4] = {
 *  (MIN_PROGRAM_TEXTURE_GATHER_OFFSET,MIN_PROGRAM_TEXTURE_GATHER_OFFSET),
 *  (MIN_PROGRAM_TEXTURE_GATHER_OFFSET,MAX_PROGRAM_TEXTURE_GATHER_OFFSET),
 *  (MAX_PROGRAM_TEXTURE_GATHER_OFFSET,MIN_PROGRAM_TEXTURE_GATHER_OFFSET),
 *  (MAX_PROGRAM_TEXTURE_GATHER_OFFSET,MAX_PROGRAM_TEXTURE_GATHER_OFFSET)
 *  };
 *
 **/

/** Test configuration: wrap mode GL_REPEAT, sampler isampler2DShadow, function textureGatherOffset, axis X
 */
class GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest : public GPUShader5TextureGatherOffsetDepthTestBase
{
public:
	/* Public methods */
	GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest(Context& context, const ExtParameters& extParams,
													   const char* name, const char* description);

	virtual ~GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest(void)
	{
	}

protected:
	/* Protected methods */
	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getShaderParts(std::vector<const glw::GLchar*>& out_vertex_shader_parts);
	virtual void prepareVertexBuffersData(std::vector<VertexBufferInfo>& vertex_buffer_infos);

	/* Virtual methods from GPUShader5TextureGatherOffsetDepthTestBase */
	virtual bool checkResult(const CapturedVaryings& captured_data, unsigned int index, unsigned int texture_size);

	/* Utilities */
	void getOffsets(glw::GLint& out_x_offset, glw::GLint& out_y_offset, unsigned int index);

	/* Number of offsets per vertex */
	static const unsigned int m_n_offsets_components;

	/* Name of offset attribute */
	static const glw::GLchar* const m_offsets_attribute_name;

	/* Storage for offsets */
	std::vector<glw::GLint> m_offsets_buffer_data;

private:
	/* Private fields */
	/* Vertex shader code */
	static const glw::GLchar* const m_vertex_shader_code;
};

/** Test configuration: wrap mode GL_REPEAT, sampler isampler2DShadow, function textureGatherOffset, axis Y
 */
class GPUShader5TextureGatherOffsetDepth2DRepeatYCaseTest : public GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest
{
public:
	/* Public methods */
	GPUShader5TextureGatherOffsetDepth2DRepeatYCaseTest(Context& context, const ExtParameters& extParams,
														const char* name, const char* description);

	virtual ~GPUShader5TextureGatherOffsetDepth2DRepeatYCaseTest(void)
	{
	}

protected:
	/* Protected methods */
	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getShaderParts(std::vector<const glw::GLchar*>& out_vertex_shader_parts);
	virtual void prepareTextureData(glw::GLubyte* data);

	/* Virtual methods from GPUShader5TextureGatherOffsetDepthTestBase */
	virtual bool checkResult(const CapturedVaryings& captured_data, unsigned int index, unsigned int texture_size);

private:
	/* Private fields */
	/* Vertex shader code */
	static const glw::GLchar* const m_vertex_shader_code;
};

/** Test configuration: wrap mode GL_REPEAT, sampler isampler2DShadowArray, function textureGatherOffset, axis X
 */
class GPUShader5TextureGatherOffsetDepth2DArrayCaseTest : public GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest
{
public:
	/* Public methods */
	GPUShader5TextureGatherOffsetDepth2DArrayCaseTest(Context& context, const ExtParameters& extParams,
													  const char* name, const char* description);

	virtual ~GPUShader5TextureGatherOffsetDepth2DArrayCaseTest(void)
	{
	}

protected:
	/* Protected methods */
	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getShaderParts(std::vector<const glw::GLchar*>& out_vertex_shader_parts);
	virtual void isTextureArray(bool& out_is_texture_array);

private:
	/* Private fields */
	/* Vertex shader code */
	static const glw::GLchar* const m_vertex_shader_code;
};

/** Test configuration: wrap mode GL_REPEAT, sampler isampler2DShadow, function textureGatherOffsets, axis X
 */
class GPUShader5TextureGatherOffsetDepth2DOffsetsCaseTest : public GPUShader5TextureGatherOffsetDepthTestBase
{
public:
	/* Public methods */
	GPUShader5TextureGatherOffsetDepth2DOffsetsCaseTest(Context& context, const ExtParameters& extParams,
														const char* name, const char* description);

	virtual ~GPUShader5TextureGatherOffsetDepth2DOffsetsCaseTest(void)
	{
	}

protected:
	/* Protected methods */
	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getShaderParts(std::vector<const glw::GLchar*>& out_vertex_shader_parts);
	virtual void prepareVertexBuffersData(std::vector<VertexBufferInfo>& vertex_buffer_infos);

	/* Virtual methods from GPUShader5TextureGatherOffsetDepthTestBase */
	virtual bool checkResult(const CapturedVaryings& captured_data, unsigned int index, unsigned int texture_size);

private:
	/* Private fields */
	/* Vertex shader code */
	static const glw::GLchar* const m_vertex_shader_code_preamble;
	static const glw::GLchar* const m_vertex_shader_code_body;

	/* String used for definition of constant offsets */
	std::string m_vertex_shader_code_offsets;
};

/** Implementation of "Test 11" from CTS_EXT_gpu_shader5. Test description follows:
 *
 * Test whether using non constant offsets in the textureGatherOffset
 * function works as expected for sampler2D and CLAMP_TO_BORDER_EXT
 * ( CLAMP_TO_EDGE ) wrap mode.
 *
 * Category:   API,
 *             Functional Test,
 *             dependency with EXT_texture_border_clamp.
 *
 * Create two (lets name them A and B) 64 (texWidth) x 64 (texHeight)
 * textures with internal format GL_RGBA32I.
 *
 * Bind texture A to texture unit 0.
 * Bind texture B to texture unit 1.
 *
 * Set for both textures GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER
 * to GL_NEAREST.
 *
 * For the A texture set GL_TEXTURE_WRAP_S and GL_TEXTURE_WRAP_T
 * to CLAMP_TO_BORDER_EXT.
 * For the B texture set GL_TEXTURE_WRAP_S and GL_TEXTURE_WRAP_T
 * to CLAMP_TO_BORDER_EXT ( CLAMP TO EDGE ).
 *
 * For both textures fill the 4 components of each texel with values
 * corresponding to texel row and column number (x,y) -> (x,y,x,y)
 *
 * Set the GL_TEXTURE_BORDER_COLOR_EXT to (-1,-1,-1,-1).
 *
 * Write a vertex shader that defines:
 *
 * uniform isampler2D samplerWithoutOffset;
 * uniform isampler2D samplerWithOffset;
 *
 * in ivec2 tgoOffset;
 * in  vec2 texCoords;
 *
 * out ivec4 withoutOffset0;
 * out ivec4 withoutOffset1;
 * out ivec2 intFloorTexCoords;
 *
 * out ivec4 withOffset0;
 * out ivec4 withOffset1;
 * out ivec4 withOffset2;
 * out ivec4 withOffset3;
 *
 * Bind samplerWithoutOffset to texture unit 0.
 * Bind samplerWithOffset to texture unit 1.
 *
 * Initialize a buffer object to be assigned as input attribute tgoOffset
 * data source. The buffer object should hold 128 integer tuples. Fill it
 * with some random integer values but falling into a range of
 * (MIN_PROGRAM_TEXTURE_GATHER_OFFSET,
 *  MAX_PROGRAM_TEXTURE_GATHER_OFFSET).
 *
 * Initialize a buffer object to be assigned as input attribute texCoords
 * data source. The buffer object should hold 128 elements. Fill the first
 * 4 tuples with (0.0, 0.0) , (0.0, 1.0), (1.0, 0.0), (1.0, 1.0) and the
 * rest with some random float values in the range [-8.0..8.0].
 *
 * In the vertex shader perform the following operation:
 *
 * vec2 floorTexCoords = floor(texCoords);
 * vec2 fractTexCoords = texCoords - floorTexCoords;
 *
 * withoutOffset0 = textureGather(samplerWithoutOffset,fractTexCoords, 0);
 * withoutOffset1 = textureGather(samplerWithoutOffset,fractTexCoords, 1);
 * intFloorTexCoords  = ivec2(int(floorTexCoords.x),int(floorTexCoords.y));
 *
 * withOffset0 = textureGatherOffset(samplerWithOffset,
 *                 texCoords, tgoOffset, 0);
 * withOffset1 = textureGatherOffset(samplerWithOffset,
 *                 texCoords, tgoOffset, 1);
 * withOffset2 = textureGatherOffset(samplerWithOffset,
 *                 texCoords, tgoOffset, 2);
 * withOffset3 = textureGatherOffset(samplerWithOffset,
 *                 texCoords, tgoOffset, 3);
 *
 * Configure transform feedback to capture the values of withoutOffset*,
 * withOffset* and intFloorTexCoords.
 *
 * Write a boilerplate fragment shader.
 *
 * Create a program from the above vertex shader and fragment shader
 * and use it.
 *
 * Execute a draw call glDrawArrays(GL_POINTS, 0, 128).
 *
 * Copy the captured results from the buffer objects bound to transform
 * feedback binding points.
 *
 * Using the captured values for each of the 128 results perform
 * the following algorithm:
 *
 * From the captured withoutOffset0 and withoutOffset1 variables extract
 * 4 texelPos values
 *
 * (Pseudocode)
 *
 * texelPos[i] = ivec2( withoutOffset0[i] , withoutOffset1[i] );
 *
 * Find a texel that has not been clamped. It's value should be
 * different than (texWidth, texHeight). Let's asume that the found
 * texel has index 'foundIndex'.
 *
 * If we can't find such texel we must throw an exception, because it means
 * that the sampling algorithm has failed.
 *
 * Extract the offset that has to be applied to the chosen texel to get it's
 * absolute position (the one we would get if we haven't clamped the texture
 * coordinates for textureGather to the range [0,1])
 *
 * (Pseudocode)
 *
 * ivec2 absoluteOffset = ivec2( intFloorTexCoords.x * texWidth ,
 *                               intFloorTexCoords.y * texHeight );
 *
 * Next apply the offset to the texel position
 *
 * (Pseudocode)
 *
 * texelAbsolutePos[foundIndex] = texelPos[foundIndex] + absoluteOffset;
 *
 * Now we have to set the absolute positions of the remaining 3 texels.
 * We can do this because the foundIndex gives us information which
 * texel we are dealing with in a 2x2 texel matrix returned by
 * textureGather. The remaining texels will have their absolute positions
 * computed by adding or substacting 1 to their x and y components
 * depending on their position in the matrix relative to foundIndex.
 *
 * In the next step we have to apply the offset used in
 * textureGatherOffset function to each of the absolute positions.
 *
 * (Pseudocode)
 *
 * texelAbsolutePos[i] += tgoOffset;
 *
 * We have to examine each absolute position in order to know if it should
 * been clamped or not.
 *
 * In case of CLAMP_TO_BORDER_EXT if the absolute position goes beyond the
 * texture bounds set it to value of GL_TEXTURE_BORDER_COLOR_EXT -> (-1,-1)
 *
 * In case of CLAMP_TO_EDGE if the absolute position goes beyond the
 * texture bounds we have to clamp the value of absolute position
 * to the edge that has been crossed.
 *
 * The test passes for each of the 4 absolute positions we have
 *
 * (Pseudocode)
 *
 * texelAbsolutePos[i] == ivec2( withOffset0[i], withOffset1[i] ) and
 * texelAbsolutePos[i] == ivec2( withOffset2[i], withOffset3[i] ).
 **/

/** Test configuration: wrap mode GL_CLAMP_TO_BORDER_EXT, sampler isampler2D, function textureGatherOffset
 */
class GPUShader5TextureGatherOffsetColor2DClampToBorderCaseTest
	: public GPUShader5TextureGatherOffsetColor2DRepeatCaseTest
{
public:
	/* Public methods */
	GPUShader5TextureGatherOffsetColor2DClampToBorderCaseTest(Context& context, const ExtParameters& extParams,
															  const char* name, const char* description);

	virtual ~GPUShader5TextureGatherOffsetColor2DClampToBorderCaseTest(void)
	{
	}

protected:
	/* Protected methods */
	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getTextureWrapMode(glw::GLenum& out_wrap_mode);
	virtual void getShaderParts(std::vector<const glw::GLchar*>& out_vertex_shader_parts);

	/* Virtual methods from GPUShader5TextureGatherOffsetColorTestBase */
	virtual bool checkResult(const CapturedVaryings& captured_data, unsigned int index, unsigned int texture_size);

	virtual void initTest(void);

private:
	/* Private fields */
	/* Vertex shader code */
	static const glw::GLchar* const m_vertex_shader_code;
};

/** Test configuration: wrap mode GL_CLAMP_TO_EDGE, sampler isampler2D, function textureGatherOffset
 */
class GPUShader5TextureGatherOffsetColor2DClampToEdgeCaseTest
	: public GPUShader5TextureGatherOffsetColor2DRepeatCaseTest
{
public:
	/* Public methods */
	GPUShader5TextureGatherOffsetColor2DClampToEdgeCaseTest(Context& context, const ExtParameters& extParams,
															const char* name, const char* description);

	virtual ~GPUShader5TextureGatherOffsetColor2DClampToEdgeCaseTest(void)
	{
	}

protected:
	/* Protected methods */
	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getTextureWrapMode(glw::GLenum& out_wrap_mode);
	virtual void getShaderParts(std::vector<const glw::GLchar*>& out_vertex_shader_parts);

	/* Virtual methods from GPUShader5TextureGatherOffsetColorTestBase */
	virtual bool checkResult(const CapturedVaryings& captured_data, unsigned int index, unsigned int texture_size);
	virtual void initTest(void);

private:
	/* Private fields */
	/* Vertex shader code */
	static const glw::GLchar* const m_vertex_shader_code;
};

/** Test configuration: wrap mode GL_CLAMP_TO_BORDER_EXT, sampler isampler2DShadow, function textureGatherOffset, axis X
 */
class GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest
	: public GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest
{
public:
	/* Public methods */
	GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest(Context& context, const ExtParameters& extParams,
															  const char* name, const char* description);

	virtual ~GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest(void)
	{
	}

protected:
	/* Protected methods */
	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getTextureWrapMode(glw::GLenum& out_wrap_mode);
	virtual void getShaderParts(std::vector<const glw::GLchar*>& out_vertex_shader_parts);
	virtual void prepareVertexBuffersData(std::vector<VertexBufferInfo>& vertex_buffer_infos);

	/* Virtual methods from GPUShader5TextureGatherOffsetDepthTestBase */
	virtual bool checkResult(const CapturedVaryings& captured_data, unsigned int index, unsigned int texture_size);

	virtual void getVaryings(std::vector<const glw::GLchar*>& out_captured_varyings);
	virtual void initTest(void);

private:
	/* Private fields */
	/* Vertex shader code */
	static const glw::GLchar* const m_vertex_shader_code;
};

/** Test configuration: wrap mode GL_CLAMP_TO_EDGE, sampler isampler2DShadow, function textureGatherOffset, axis X
 */
class GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest
	: public GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest
{
public:
	/* Public methods */
	GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest(Context& context, const ExtParameters& extParams,
															const char* name, const char* description);

	virtual ~GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest(void)
	{
	}

protected:
	/* Protected methods */
	/* Virtual methods from GPUShader5TextureGatherOffsetTestBase */
	virtual void getTextureWrapMode(glw::GLenum& out_wrap_mode);
	virtual void getShaderParts(std::vector<const glw::GLchar*>& out_vertex_shader_parts);

	/* Virtual methods from GPUShader5TextureGatherOffsetDepthTestBase */
	virtual bool checkResult(const CapturedVaryings& captured_data, unsigned int index, unsigned int texture_size);

	virtual void getVaryings(std::vector<const glw::GLchar*>& out_captured_varyings);
	virtual void initTest(void);

private:
	/* Private fields */
	/* Vertex shader code */
	static const glw::GLchar* const m_vertex_shader_code;
};

} /* glcts */

#endif // _ESEXTCGPUSHADER5TEXTUREGATHEROFFSET_HPP
