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
 * \file esextcTextureBorderClampSamplingTexture.cpp
 * \brief  Verify that sampling a texture with GL_CLAMP_TO_BORDER_EXT
 * wrap mode enabled gives correct results (Test 7)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampSamplingTexture.hpp"
#include "esextcTextureBorderClampCompressedResources.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

template <typename InputType, typename OutputType>
const glw::GLuint TextureBorderClampSamplingTexture<InputType, OutputType>::m_texture_unit = 0;

/** Constructor
 *
 *  @param nComponents           number of components
 *  @param target                texture target
 *  @param inputInternalFormat   input texture internal format
 *  @param outputInternalFormat  output texture internal format
 *  @param filtering             contains parameters for GL_TEXTURE_MAG_FILTER and GL_TEXTURE_MIN_FILTER - in our case can be GL_NEAREST or GL_LINEAR
 *  @param inputFormat           input texture format
 *  @param outputFormat          output texture format
 *  @param width                 texture/viewport width
 *  @param height                texture/viewport height
 *  @param initValue             value used for input texture to fill all texels with
 *  @param initBorderColor       value of border color for input texture
 *  @param expectedValue         expected value for texture texels for points taken from inside of input texture
 *  @param expectedBorderColor   expected value for texture texels for points taken from outside of input texture
 *  @param inputType             enum representing data type for input texture
 *  @param outputType            enum representing data type for output texture
 **/
template <typename InputType, typename OutputType>
TestConfiguration<InputType, OutputType>::TestConfiguration(
	glw::GLsizei nInComponents, glw::GLsizei nOutComponents, glw::GLenum target, glw::GLenum inputInternalFormat,
	glw::GLenum outputInternalFormat, glw::GLenum filtering, glw::GLenum inputFormat, glw::GLenum outputFormat,
	glw::GLuint width, glw::GLuint height, glw::GLuint depth, InputType initValue, InputType initBorderColor,
	OutputType expectedValue, OutputType expectedBorderColor, glw::GLenum inputType, glw::GLenum outputType)
	: m_n_in_components(nInComponents)
	, m_n_out_components(nOutComponents)
	, m_target(target)
	, m_input_internal_format(inputInternalFormat)
	, m_output_internal_format(outputInternalFormat)
	, m_filtering(filtering)
	, m_input_format(inputFormat)
	, m_output_format(outputFormat)
	, m_width(width)
	, m_height(height)
	, m_depth(depth)
	, m_init_value(initValue)
	, m_init_border_color(initBorderColor)
	, m_expected_value(expectedValue)
	, m_expected_border_color(expectedBorderColor)
	, m_input_type(inputType)
	, m_output_type(outputType)
{
	/* Nothing to be done here */
}

/** Copy Contructor
 *
 * @param configuration  const reference to the configuration which will be copied
 */
template <typename InputType, typename OutputType>
TestConfiguration<InputType, OutputType>::TestConfiguration(const TestConfiguration& configuration)
{
	m_n_in_components		 = configuration.get_n_in_components();
	m_n_out_components		 = configuration.get_n_out_components();
	m_target				 = configuration.get_target();
	m_input_internal_format  = configuration.get_input_internal_format();
	m_output_internal_format = configuration.get_output_internal_format();
	m_filtering				 = configuration.get_filtering();
	m_input_format			 = configuration.get_input_format();
	m_output_format			 = configuration.get_output_format();
	m_width					 = configuration.get_width();
	m_height				 = configuration.get_height();
	m_depth					 = configuration.get_depth();
	m_init_value			 = configuration.get_init_value();
	m_init_border_color		 = configuration.get_init_border_color();
	m_expected_value		 = configuration.get_expected_value();
	m_expected_border_color  = configuration.get_expected_border_color();
	m_input_type			 = configuration.get_input_type();
	m_output_type			 = configuration.get_output_type();
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
template <typename InputType, typename OutputType>
TextureBorderClampSamplingTexture<InputType, OutputType>::TextureBorderClampSamplingTexture(
	Context& context, const ExtParameters& extParams, const char* name, const char* description,
	const TestConfiguration<InputType, OutputType>& configuration)
	: TestCaseBase(context, extParams, name, description)
	, m_attr_position_location(-1)
	, m_attr_texcoord_location(-1)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_po_id(0)
	, m_sampler_id(0)
	, m_test_configuration(configuration)
	, m_input_to_id(0)
	, m_output_to_id(0)
	, m_position_vbo_id(0)
	, m_text_coord_vbo_id(0)
	, m_vs_id(0)
	, m_vao_id(0)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 **/
template <typename InputType, typename OutputType>
void TextureBorderClampSamplingTexture<InputType, OutputType>::initTest(void)
{
	if (!m_is_texture_border_clamp_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BORDER_CLAMP_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate and bind VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");
	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Generate sampler object */
	gl.genSamplers(1, &m_sampler_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating sampler object!");

	/* Create framebuffer object */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating framebuffer object!");

	/* Set up clear color */
	gl.clearColor(0.5 /* red */, 0.5 /* green */, 0.5 /* blue */, 1 /* alpha */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting clear color value!");

	/* Input attributes for vertex shader */

	/* Full screen quad */
	glw::GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
								1.0f,  -1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 0.0f, 1.0f };

	/* Texture coords */
	glw::GLfloat coords[] = {
		-1.0f, -1.0f, /* for bottom-left corner of the viewport */
		-1.0f, 2.0f,  /* for top-left corner of the viewport */
		2.0f,  -1.0f, /* for bottom-right corner of the viewport */
		2.0f,  2.0f   /* for top-right corner of the viewport */
	};

	/* Generate buffer object */
	gl.genBuffers(1, &m_position_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");

	/* Bind buffer object */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_position_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");

	/* Set data for buffer object */
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting data for buffer object!");

	/* Generate buffer object */
	gl.genBuffers(1, &m_text_coord_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");

	/* Bind buffer object */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_text_coord_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");

	/* Set data for buffer object */
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error seting data for buffer object!");

	/* Create program object */
	m_po_id = gl.createProgram();

	/* Create shader objects */
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);

	/* Get vertex shader code */
	std::string vsCode	= getVertexShaderCode();
	const char* vsCodePtr = (const char*)vsCode.c_str();

	/* Get fragment shader code */
	std::string fshaderCode	= getFragmentShaderCode();
	const char* fshaderCodePtr = (const char*)fshaderCode.c_str();

	/* Build program */
	if (!buildProgram(m_po_id, m_fs_id, 1, &fshaderCodePtr, m_vs_id, 1, &vsCodePtr))
	{
		TCU_FAIL("Program could not have been created sucessfully from a valid vertex/fragment shader!");
	}

	createTextures();
}

/** Set data for input texture
 *
 * @param buffer  reference to buffer where initial data will be stored
 */
template <typename InputType, typename OutputType>
void TextureBorderClampSamplingTexture<InputType, OutputType>::setInitData(std::vector<InputType>& buffer)
{
	const InputType initDataTexel = m_test_configuration.get_init_value();

	glw::GLuint size = m_test_configuration.get_width() * m_test_configuration.get_height() *
					   m_test_configuration.get_depth() * m_test_configuration.get_n_in_components();

	for (glw::GLuint i = 0; i < size; ++i)
	{
		buffer[i] = initDataTexel;
	}
}

/** Create input and output textures
 *
 */
template <typename InputType, typename OutputType>
void TextureBorderClampSamplingTexture<InputType, OutputType>::createTextures(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate input texture */
	gl.genTextures(1, &m_input_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");

	/* Bind input texture */
	gl.bindTexture(m_test_configuration.get_target(), m_input_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	glw::GLsizei components = m_test_configuration.get_n_in_components();
	glw::GLsizei texelsNumber =
		m_test_configuration.get_width() * m_test_configuration.get_height() * m_test_configuration.get_depth();

	/* Allocate storage for input texture and fill it with data */
	{
		switch (m_test_configuration.get_target())
		{
		case GL_TEXTURE_2D:
		{
			gl.texStorage2D(m_test_configuration.get_target(),				  /* target */
							1,												  /* levels */
							m_test_configuration.get_input_internal_format(), /* internalformat */
							m_test_configuration.get_width(),				  /* width */
							m_test_configuration.get_height());				  /* height */
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating storage for texture object!");

			if (m_test_configuration.get_input_internal_format() == GL_COMPRESSED_RGBA8_ETC2_EAC)
			{
				gl.compressedTexSubImage2D(m_test_configuration.get_target(),				 /* target */
										   0,												 /* level */
										   0,												 /* xoffset */
										   0,												 /* yoffset */
										   m_test_configuration.get_width(),				 /* width */
										   m_test_configuration.get_height(),				 /* height */
										   m_test_configuration.get_input_internal_format(), /* internalformat */
										   sizeof(compressed_image_data_2D),				 /* image size */
										   compressed_image_data_2D);						 /* data */
				GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling texture with compressed data!");
			}
			else
			{
				std::vector<InputType> inputData(components * texelsNumber);
				setInitData(inputData);

				gl.texSubImage2D(m_test_configuration.get_target(),		  /* target */
								 0,										  /* level */
								 0,										  /* xoffset */
								 0,										  /* yoffset */
								 m_test_configuration.get_width(),		  /* width */
								 m_test_configuration.get_height(),		  /* height */
								 m_test_configuration.get_input_format(), /* format */
								 m_test_configuration.get_input_type(),   /* type */
								 &inputData[0]);						  /* data */
				GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling texture with data!");
			}
			break;
		}
		case GL_TEXTURE_2D_ARRAY:
		case GL_TEXTURE_3D:
		{
			gl.texStorage3D(m_test_configuration.get_target(),				  /* target */
							1,												  /* levels */
							m_test_configuration.get_input_internal_format(), /* internalformat*/
							m_test_configuration.get_width(),				  /* width */
							m_test_configuration.get_height(),				  /* height */
							m_test_configuration.get_depth());				  /* depth */
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating storage for texture object!");

			if (m_test_configuration.get_input_internal_format() == GL_COMPRESSED_RGBA8_ETC2_EAC)
			{
				gl.compressedTexSubImage3D(m_test_configuration.get_target(),				 /* target */
										   0,												 /* level */
										   0,												 /* xoffset */
										   0,												 /* yoffset */
										   0,												 /* zoffset */
										   m_test_configuration.get_width(),				 /* width */
										   m_test_configuration.get_height(),				 /* height */
										   m_test_configuration.get_depth(),				 /* depth */
										   m_test_configuration.get_input_internal_format(), /* internalformat */
										   sizeof(compressed_image_data_2D_array),			 /* image size */
										   compressed_image_data_2D_array);					 /* data */
				GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling texture with compressed data!");
			}
			else
			{
				std::vector<InputType> inputData(components * texelsNumber);
				setInitData(inputData);

				gl.texSubImage3D(m_test_configuration.get_target(),		  /* target */
								 0,										  /* level */
								 0,										  /* xoffset */
								 0,										  /* yoffset */
								 0,										  /* zoffset */
								 m_test_configuration.get_width(),		  /* width */
								 m_test_configuration.get_height(),		  /* height */
								 m_test_configuration.get_depth(),		  /* depth */
								 m_test_configuration.get_input_format(), /* format */
								 m_test_configuration.get_input_type(),   /* type */
								 &inputData[0]);						  /* data */
				GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling texture with data!");
			}
			break;
		}
		default:
			TCU_FAIL("Test parameters can contain only following targets: GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, "
					 "GL_TEXTURE_3D!");
		}
	}

	/* Generate output texture */
	gl.genTextures(1, &m_output_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");

	/* Bind output texture */
	gl.bindTexture(GL_TEXTURE_2D, m_output_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	/* Allocate storage for output texture */
	gl.texStorage2D(GL_TEXTURE_2D, 1, m_test_configuration.get_output_internal_format(),
					m_test_configuration.get_width(), m_test_configuration.get_height());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating storage for texture object!");
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
template <typename InputType, typename OutputType>
tcu::TestNode::IterateResult TextureBorderClampSamplingTexture<InputType, OutputType>::iterate(void)
{
	/* Initialize test case */
	initTest();

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool testResult = true;

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error using program!");

	/* Configure vertices position attribute */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_position_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object!");

	m_attr_position_location = gl.getAttribLocation(m_po_id, "vertex_position_in");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get attribute location!");

	gl.vertexAttribPointer(m_attr_position_location, 4, GL_FLOAT, GL_FALSE, 0, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set vertex attribute pointer!");

	gl.enableVertexAttribArray(m_attr_position_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not enable vertex attribute array!");

	/* Configure texture coordinates attribute */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_text_coord_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object!");

	m_attr_texcoord_location = gl.getAttribLocation(m_po_id, "texture_coords_in");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get attribute location!");

	gl.vertexAttribPointer(m_attr_texcoord_location, 2, GL_FLOAT, GL_FALSE, 0, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set vertex attribute pointer!");

	gl.enableVertexAttribArray(m_attr_texcoord_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not enable vertex attribute array!");

	/* Configure and bind sampler to texture unit */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active texture unit!");

	gl.bindTexture(m_test_configuration.get_target(), m_input_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture!");

	glw::GLint samplerLocation = gl.getUniformLocation(m_po_id, "test_sampler");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Erros getting sampler location!");

	gl.uniform1i(samplerLocation, m_texture_unit);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind sampler location to texture unit!");

	gl.bindSampler(m_texture_unit, m_sampler_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind sampler object to texture unit!");

	/* Set GL_TEXTURE_BORDER_COLOR_EXT for sampler object */
	switch (m_test_configuration.get_input_internal_format())
	{
	case GL_RGBA32F:
	case GL_RGBA8:
	case GL_DEPTH_COMPONENT32F:
	case GL_DEPTH_COMPONENT16:
	case GL_COMPRESSED_RGBA8_ETC2_EAC:
	{
		glw::GLfloat val			= (glw::GLfloat)m_test_configuration.get_init_border_color();
		glw::GLfloat border_color[] = { val, val, val, val };
		gl.samplerParameterfv(m_sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, border_color);
		break;
	}
	case GL_R32UI:
	{
		glw::GLuint val			   = (glw::GLuint)m_test_configuration.get_init_border_color();
		glw::GLuint border_color[] = { val, val, val, val };
		gl.samplerParameterIuiv(m_sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, border_color);
		break;
	}
	case GL_R32I:
	{
		glw::GLint val			  = (glw::GLint)m_test_configuration.get_init_border_color();
		glw::GLint border_color[] = { val, val, val, val };
		gl.samplerParameterIiv(m_sampler_id, m_glExtTokens.TEXTURE_BORDER_COLOR, border_color);
		break;
	}

	default:
		throw tcu::TestError("Unsupported sized internal format. Should never happen!", "", __FILE__, __LINE__);
		break;
	};
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting border color parameter!");

	/* Set sampler's GL_TEXTURE_WRAP_* parameters values to GL_CLAMP_TO_BORDER_EXT */
	gl.samplerParameteri(m_sampler_id, GL_TEXTURE_WRAP_S, m_glExtTokens.CLAMP_TO_BORDER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting GL_TEXTURE_WRAP_S parameter!");
	gl.samplerParameteri(m_sampler_id, GL_TEXTURE_WRAP_R, m_glExtTokens.CLAMP_TO_BORDER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting GL_TEXTURE_WRAP_R parameter!");
	gl.samplerParameteri(m_sampler_id, GL_TEXTURE_WRAP_T, m_glExtTokens.CLAMP_TO_BORDER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting GL_TEXTURE_WRAP_T parameter!");

	/* Set GL_TEXTURE_MAG_FILTER and GL_TEXTURE_MIN_FILTER parameters values */
	gl.samplerParameteri(m_sampler_id, GL_TEXTURE_MAG_FILTER, m_test_configuration.get_filtering());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting value for GL_TEXTURE_MAG_FILTER parameter!");
	gl.samplerParameteri(m_sampler_id, GL_TEXTURE_MIN_FILTER, m_test_configuration.get_filtering());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting value for GL_TEXTURE_MIN_FILTER parameter!");

	for (glw::GLint i = getStartingLayerIndex(); i < getLastLayerIndex(); ++i)
	{
		/* Configure layer (third texture coordinate) */
		glw::GLint layerLocation = gl.getUniformLocation(m_po_id, "layer");
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting layer uniform location!");

		gl.uniform1f(layerLocation, getCoordinateValue(i));
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting layer uniform variable!");

		/* Bind framebuffer object */
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding framebuffer object!");

		/* Set view port */
		gl.viewport(0,									/* x */
					0,									/* y */
					m_test_configuration.get_width(),   /* width */
					m_test_configuration.get_height()); /* height */
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting view port!");

		/* Attach texture to framebuffer */
		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER,  /* target */
								GL_COLOR_ATTACHMENT0, /* attachment */
								GL_TEXTURE_2D,		  /* textarget */
								m_output_to_id,		  /* texture */
								0);					  /* level */
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not attach texture object to GL_COLOR_ATTACHMENT0!");

		/* Check framebuffer status */
		checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

		/* Clear the color buffer with (0.5, 0.5, 0.5, 1) color */
		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error clearing color buffer");

		/* Render */
		gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

		/* Get data from framebuffer's color attachment and compare with expected values.
		 * For GL_NEAREST filtering and GL_TEXTURE_3D texture target and Layer equal to
		 * -1 or Depth the whole texture is expected to be filled with border color
		 */
		OutputType expectedColor;

		switch (m_test_configuration.get_target())
		{
		case GL_TEXTURE_2D:
		case GL_TEXTURE_2D_ARRAY:
			expectedColor = m_test_configuration.get_expected_value();
			break;

		case GL_TEXTURE_3D:
			if (i > -1 && i < (glw::GLint)m_test_configuration.get_depth())
				expectedColor = m_test_configuration.get_expected_value();
			else
				expectedColor = m_test_configuration.get_expected_border_color();
			break;
		default:
			TCU_FAIL("Not allowed texture target - should be one of GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_3D");
		}

		if (!checkResult(expectedColor, m_test_configuration.get_expected_border_color(), i))
		{
			testResult = false;
		}
	}

	if (testResult)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}
	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
template <typename InputType, typename OutputType>
void TextureBorderClampSamplingTexture<InputType, OutputType>::deinit(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset Gl state */
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, 0);
	gl.bindTexture(GL_TEXTURE_3D, 0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindSampler(m_texture_unit, 0);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	gl.bindVertexArray(0);

	if (m_attr_position_location != -1)
	{
		gl.disableVertexAttribArray(m_attr_position_location);
		m_attr_position_location = -1;
	}

	if (m_attr_texcoord_location != -1)
	{
		gl.disableVertexAttribArray(m_attr_texcoord_location);
		m_attr_texcoord_location = -1;
	}

	gl.useProgram(0);

	/* Delete Gl objects */
	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
		m_fbo_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
		m_po_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
		m_fs_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
		m_vs_id = 0;
	}

	if (m_input_to_id != 0)
	{
		gl.deleteTextures(1, &m_input_to_id);
		m_input_to_id = 0;
	}

	if (m_output_to_id != 0)
	{
		gl.deleteTextures(1, &m_output_to_id);
		m_output_to_id = 0;
	}

	if (m_sampler_id != 0)
	{
		gl.deleteSamplers(1, &m_sampler_id);
		m_sampler_id = 0;
	}

	if (m_position_vbo_id != 0)
	{
		gl.deleteBuffers(1, &m_position_vbo_id);
		m_position_vbo_id = 0;
	}

	if (m_text_coord_vbo_id != 0)
	{
		gl.deleteBuffers(1, &m_text_coord_vbo_id);
		m_text_coord_vbo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Check Framebuffer Status - throw exception if status is different than GL_FRAMEBUFFER_COMPLETE
 *
 * @param framebuffer  - GL_DRAW_FRAMEBUFFER, GL_READ_FRAMEBUFFER or GL_FRAMEBUFFER
 *
 */
template <typename InputType, typename OutputType>
void TextureBorderClampSamplingTexture<InputType, OutputType>::checkFramebufferStatus(glw::GLenum framebuffer)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check framebuffer status */
	glw::GLenum framebufferStatus = gl.checkFramebufferStatus(framebuffer);

	if (GL_FRAMEBUFFER_COMPLETE != framebufferStatus)
	{
		switch (framebufferStatus)
		{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		{
			TCU_FAIL("Framebuffer incomplete, status: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
		}

		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		{
			TCU_FAIL("Framebuffer incomplete, status: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
		}

		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		{
			TCU_FAIL("Framebuffer incomplete, status: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
		}

		case GL_FRAMEBUFFER_UNSUPPORTED:
		{
			TCU_FAIL("Framebuffer incomplete, status: GL_FRAMEBUFFER_UNSUPPORTED");
		}

		default:
		{
			TCU_FAIL("Framebuffer incomplete, status not recognized");
		}
		}

	} /* if (GL_FRAMEBUFFER_COMPLETE != framebuffer_status) */
}

/** Get result data and check if it is as expected
 *
 * @return   returns true if result data is as expected, otherwise returns false
 */
template <typename InputType, typename OutputType>
bool TextureBorderClampSamplingTexture<InputType, OutputType>::checkResult(OutputType expectedValue,
																		   OutputType expectedBorderColor,
																		   glw::GLint layer)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind draw framebuffer to read framebuffer */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding framebuffer object!");

	std::vector<OutputType> resultData(m_test_configuration.get_width() * m_test_configuration.get_height() *
									   m_test_configuration.get_n_out_components());

	/* Read data from framebuffer */
	gl.readPixels(0,										/* x */
				  0,										/* y */
				  m_test_configuration.get_width(),			/* width */
				  m_test_configuration.get_height(),		/* height */
				  m_test_configuration.get_output_format(), /* format */
				  m_test_configuration.get_output_type(),   /* type */
				  &resultData[0]);							/* data */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error reading pixels from color buffer");

	/* Choose comparision method depending on filtering mode */
	if (m_test_configuration.get_filtering() == GL_NEAREST)
	{
		return checkNearest(resultData, expectedValue, expectedBorderColor, layer);
	}
	else
	{
		return checkLinear(resultData, layer);
	}
}

/** Create fragment shader code
 *
 * @return string with fragment shader code
 */
template <typename InputType, typename OutputType>
std::string TextureBorderClampSamplingTexture<InputType, OutputType>::getFragmentShaderCode(void)
{
	std::stringstream result;
	std::string		  coordType;
	std::string		  samplerType;
	std::string		  outType;
	std::string		  outCommand;

	/* Check input texture format and prepare sampler prefix */
	switch (m_test_configuration.get_input_internal_format())
	{
	case GL_RGBA32F:
	case GL_RGBA8:
	case GL_DEPTH_COMPONENT32F:
	case GL_DEPTH_COMPONENT16:
	case GL_COMPRESSED_RGBA8_ETC2_EAC:
		samplerType = "";
		break;

	case GL_R32UI:
		samplerType = "u";
		break;

	case GL_R32I:
		samplerType = "i";
		break;

	default:
		throw tcu::TestError("Not allowed internal format", "", __FILE__, __LINE__);
	}

	/* Check input texture target and prepare approperiate texture coordinate type and sampler type */
	switch (m_test_configuration.get_target())
	{
	case GL_TEXTURE_2D:
		coordType = "vec2";
		samplerType += "sampler2D";
		break;

	case GL_TEXTURE_2D_ARRAY:
		coordType = "vec3";
		samplerType += "sampler2DArray";
		break;

	case GL_TEXTURE_3D:
		coordType = "vec3";
		samplerType += "sampler3D";
		break;

	default:
		throw tcu::TestError("Not allowed texture target!", "", __FILE__, __LINE__);
	}

	/* Check output texture format and prepare approperiate texel fetching method and output type */
	switch (m_test_configuration.get_output_internal_format())
	{
	case GL_RGBA8:
		outType	= "vec4";
		outCommand = "texture(test_sampler, texture_coords_out)";
		break;

	case GL_R8:
		outType	= "float";
		outCommand = "texture(test_sampler, texture_coords_out).x";
		break;

	case GL_R32UI:
		outType	= "uint";
		outCommand = "uint(texture(test_sampler, texture_coords_out).x)";
		break;

	case GL_R32I:
		outType	= "int";
		outCommand = "int(texture(test_sampler, texture_coords_out).x)";
		break;

	default:
		throw tcu::TestError("Not allowed internal format!", "", __FILE__, __LINE__);
	}

	result << "${VERSION}\n"
			  "\n"
			  "precision highp float;\n"
			  "precision highp "
		   << samplerType << ";\n"
							 "\n"
							 "uniform "
		   << samplerType << " test_sampler;\n"
							 "in  "
		   << coordType << " texture_coords_out;\n"
						   "layout(location = 0) out "
		   << outType << " color;\n"
						 "\n"
						 "void main()\n"
						 "{\n"
						 "   color = "
		   << outCommand << ";\n"
							"}\n";

	return result.str();
}

/** Create vertex shader code
 *
 * @return string with vertex shader code
 */
template <typename InputType, typename OutputType>
std::string TextureBorderClampSamplingTexture<InputType, OutputType>::getVertexShaderCode(void)
{
	std::stringstream result;
	std::string		  coordType;
	std::string		  coordAssignment;

	/* Check input texture target and prepare approperiate coordinate type and coordinate assignment method */
	switch (m_test_configuration.get_target())
	{
	case GL_TEXTURE_2D:
		coordType		= "vec2";
		coordAssignment = "texture_coords_in";
		break;

	case GL_TEXTURE_2D_ARRAY:
	case GL_TEXTURE_3D:
		coordType		= "vec3";
		coordAssignment = "vec3(texture_coords_in, layer)";
		break;

	default:
		throw tcu::TestError("Not allowed texture target!", "", __FILE__, __LINE__);
	}

	result << "${VERSION}\n"
			  "\n"
			  "precision highp float;\n"
			  "\n"
			  "layout (location = 0) in vec4 vertex_position_in;\n"
			  "layout (location = 1) in vec2 texture_coords_in;\n"
			  "out "
		   << coordType << " texture_coords_out;\n"
						   "uniform float layer;\n"
						   "\n"
						   "void main()\n"
						   "{\n"
						   "    gl_Position = vertex_position_in;\n"
						   "    texture_coords_out = "
		   << coordAssignment << ";\n"
								 "}\n";

	return result.str();
}

/** Check if result data is the same as expected data when GL_NEAREST filtering is set
 * @param buffer              reference to the buffer with result data
 * @param expectedValue       it is the value which should be read from texture if coordinates are inside of [0,1]
 * @param expectedBorderColor it is the value which should be read from texture if coordinates are outside of [0,1]
 * @return                    returns true if result data is as expected, otherwise returns false
 */
template <typename InputType, typename OutputType>
bool TextureBorderClampSamplingTexture<InputType, OutputType>::checkNearest(std::vector<OutputType>& buffer,
																			OutputType expectedValue,
																			OutputType expectedBorderColor,
																			glw::GLint layer)
{
	glw::GLuint width		   = m_test_configuration.get_width();
	glw::GLuint height		   = m_test_configuration.get_height();
	glw::GLuint in_components  = m_test_configuration.get_n_in_components();
	glw::GLuint out_components = m_test_configuration.get_n_out_components();
	glw::GLuint outRowWidth	= m_test_configuration.get_width() * out_components;
	glw::GLuint index		   = 0;

	/* Check if center point is equal to expectedValue */
	std::pair<glw::GLuint, glw::GLuint> centerPoint(width / 2, height / 2);

	for (glw::GLuint i = 0; i < deMinu32(out_components, in_components); ++i)
	{
		index = centerPoint.second * outRowWidth + centerPoint.first * out_components + i;
		if (buffer[index] != expectedValue)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Wrong value for layer (" << layer << ") at point (x,y)  = ("
							   << centerPoint.first << "," << centerPoint.second << ") , component (" << i << ")\n"
							   << "Expected value [" << (glw::GLint)expectedValue << "]\n"
							   << "Result   value [" << (glw::GLint)buffer[index] << "]\n"
							   << tcu::TestLog::EndMessage;
			return false;
		}
	}

	/* Check if following points (marked as BC) contain values equal border color
	 *
	 *                 (-1, -1)    (0, -1)  (1,  -1)     (2, -1)
	 *                         *-------+-------+-------*
	 *                         |       |       |       |
	 *                         |  BC   |  BC   |   BC  |
	 *                         |       |       |       |
	 *                 (-1, 0) +-------+-------+-------+ (2, 0)
	 *                         |       |       |       |
	 *                         |  BC   |   0   |   BC  |
	 *                         |       |       |       |
	 *                 (-1, 1) +-------+-------+-------+ (2, 1)
	 *                         |       |       |       |
	 *                         |  BC   |  BC   |   BC  |
	 *                         |       |       |       |
	 *                         *-------+-------+-------*
	 *                 (-1, 2)      (0, 2)  (1, 2)       (2, 2)
	 */

	std::vector<std::pair<glw::GLuint, glw::GLuint> > borderPoints;

	borderPoints.push_back(std::pair<glw::GLuint, glw::GLuint>(width / 6, height / 6));
	borderPoints.push_back(std::pair<glw::GLuint, glw::GLuint>(width / 2, height / 6));
	borderPoints.push_back(std::pair<glw::GLuint, glw::GLuint>((glw::GLuint)(width / 6.0 * 5), height / 6));
	borderPoints.push_back(std::pair<glw::GLuint, glw::GLuint>(width / 6, height / 2));
	borderPoints.push_back(std::pair<glw::GLuint, glw::GLuint>((glw::GLuint)(width / 6.0 * 5), height / 2));
	borderPoints.push_back(std::pair<glw::GLuint, glw::GLuint>(width / 6, (glw::GLuint)(height / 6.0 * 5)));
	borderPoints.push_back(std::pair<glw::GLuint, glw::GLuint>(width / 2, (glw::GLuint)(height / 6.0 * 5)));
	borderPoints.push_back(
		std::pair<glw::GLuint, glw::GLuint>((glw::GLuint)(width / 6.0 * 5), (glw::GLuint)(height / 6.0 * 5)));

	for (glw::GLuint j = 0; j < borderPoints.size(); ++j)
	{
		for (glw::GLuint i = 0; i < deMinu32(out_components, in_components); ++i)
		{
			index = borderPoints[j].second * outRowWidth + borderPoints[j].first * out_components + i;
			if (buffer[index] != expectedBorderColor)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Wrong value for layer (" << layer
								   << ") at point (x,y)  = (" << borderPoints[j].first << "," << borderPoints[j].second
								   << ") , component (" << i << ")\n"
								   << "Expected value [" << (glw::GLint)expectedBorderColor << "]\n"
								   << "Result   value [" << (glw::GLint)buffer[index] << "]\n"
								   << tcu::TestLog::EndMessage;
				return false;
			}
		}
	}

	return true;
}

/** Check if result data is as expected when GL_LINEAR filtering is set
 *
 * @param buffer reference to the buffer with result data
 * @return       returns true if result data is as expected, otherwise return false
 */
template <typename InputType, typename OutputType>
bool TextureBorderClampSamplingTexture<InputType, OutputType>::checkLinear(std::vector<OutputType>& buffer,
																		   glw::GLint layer)
{
	glw::GLuint centerX = m_test_configuration.get_width() / 2;
	glw::GLuint centerY = m_test_configuration.get_height() / 2;
	glw::GLuint stepX   = m_test_configuration.get_width() / 3;
	glw::GLuint stepY   = m_test_configuration.get_height() / 3;

	glw::GLuint index = 0;

	glw::GLuint in_components  = m_test_configuration.get_n_in_components();
	glw::GLuint out_components = m_test_configuration.get_n_out_components();
	glw::GLuint outRowWidth	= m_test_configuration.get_width() * out_components;

	/* Check values from center to the bottom */
	for (glw::GLuint y = centerY; y < centerY + stepY; ++y)
	{
		for (glw::GLuint c = 0; c < deMinu32(out_components, in_components); ++c)
		{
			index = y * outRowWidth + centerX * out_components + c;
			if (buffer[index + outRowWidth] - buffer[index] < 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "For layer (" << layer
								   << ") when moving from center point  (x, y)  = (" << centerX << "," << centerY
								   << ") to the bottom\n"
								   << "at point (x, y)  = (" << centerX << "," << y
								   << ") - texel values stopped to be monotonically increasing\n"
								   << tcu::TestLog::EndMessage;
				return false;
			}
		}
	}

	/* Check values from center to the top */
	for (glw::GLuint y = centerY; y > centerY - stepY; --y)
	{
		for (glw::GLuint c = 0; c < deMinu32(out_components, in_components); ++c)
		{
			index = y * outRowWidth + centerX * out_components + c;
			if (buffer[index - outRowWidth] - buffer[index] < 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "For layer (" << layer
								   << ") when moving from center point  (x, y)  = (" << centerX << "," << centerY
								   << ") to the top\n"
								   << "at point (x, y)  = (" << centerX << "," << y
								   << ")- texel values stopped to be monotonically increasing\n"
								   << tcu::TestLog::EndMessage;
				return false;
			}
		}
	}

	/* Check values from center to the right */
	for (glw::GLuint x = centerX; x < centerX + stepX; ++x)
	{
		for (glw::GLuint c = 0; c < deMinu32(out_components, in_components); ++c)
		{
			index = centerY + x * out_components + c;
			if (buffer[index + out_components] - buffer[index] < 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "For layer (" << layer
								   << ") when moving from center point  (x, y) = (" << centerX << "," << centerY
								   << ") to the right\n"
								   << "at point (x, y)  = (" << x << "," << centerY
								   << ")- texel values stopped to be monotonically increasing\n"
								   << tcu::TestLog::EndMessage;
				return false;
			}
		}
	}

	/* Check values from center to the left */
	for (glw::GLuint x = centerY; x > centerX - stepX; --x)
	{
		for (glw::GLuint c = 0; c < deMinu32(out_components, in_components); ++c)
		{
			index = centerY + x * out_components + c;
			if (buffer[index - out_components] - buffer[index] < 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "For layer (" << layer
								   << ") when moving from center point  (x, y) = (" << centerX << "," << centerY
								   << ") to the left\n"
								   << "at point (x, y)  = (" << x << "," << centerY
								   << ")- texel values stopped to be monotonically increasing\n"
								   << tcu::TestLog::EndMessage;
				return false;
			}
		}
	}

	return true;
}

/** Returns start layer index
 *
 * @return returns start layer index (0 for GL_TEXTURE_2D target , otherwise -1)
 */
template <typename InputType, typename OutputType>
glw::GLint TextureBorderClampSamplingTexture<InputType, OutputType>::getStartingLayerIndex()
{
	switch (m_test_configuration.get_target())
	{
	case GL_TEXTURE_2D:
	case GL_TEXTURE_2D_ARRAY:
		return 0;

	case GL_TEXTURE_3D:
		return -1;

	default:
		TCU_FAIL("Not allowed texture target - should be one of GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_3D");
	}
}

/** Returns last layer index
 *
 * @return returns last layer index (1 for GL_TEXTURE_2D target , otherwise depth + 1)
 */
template <typename InputType, typename OutputType>
glw::GLint TextureBorderClampSamplingTexture<InputType, OutputType>::getLastLayerIndex()
{
	switch (m_test_configuration.get_target())
	{
	case GL_TEXTURE_2D:
	case GL_TEXTURE_2D_ARRAY:
		return m_test_configuration.get_depth();

	case GL_TEXTURE_3D:
		return m_test_configuration.get_depth() + 1;

	default:
		TCU_FAIL("Not allowed texture target - should be one of GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_3D");
	}
}

/** Returns third texture coordinate to access a particular layer in a texture
 *
 * @return returns third texture coordinate
 */
template <typename InputType, typename OutputType>
glw::GLfloat TextureBorderClampSamplingTexture<InputType, OutputType>::getCoordinateValue(glw::GLint index)
{
	switch (m_test_configuration.get_target())
	{
	case GL_TEXTURE_2D:
		return 0.0f;
		break;

	case GL_TEXTURE_2D_ARRAY:
		return (glw::GLfloat)index;

	case GL_TEXTURE_3D:
		return static_cast<glw::GLfloat>(index) / static_cast<glw::GLfloat>(m_test_configuration.get_depth());

	default:
		TCU_FAIL("Not allowed texture target - should be one of GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_3D");
	}
}

} // namespace glcts
