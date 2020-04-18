/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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

/**
 * \file  gl3cTextureSizePromotionTests.hpp
 * \brief Implements test classes for testing of texture internal format
 promotion mechanism.
 */ /*-------------------------------------------------------------------*/

#include "gl3cTextureSizePromotion.hpp"

#include "deMath.h"
#include "gluContextInfo.hpp"
#include "gluStrUtil.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

/* Stringify macro. */
#define _STR(s) STR(s)
#define STR(s) #s

namespace gl3cts
{
namespace TextureSizePromotion
{
Tests::Tests(deqp::Context& context)
	: TestCaseGroup(context, "texture_size_promotion", "Verifies texture internal format size promotion mechanism.")
{
	/* Left blank on purpose */
}

void Tests::init(void)
{
	addChild(new TextureSizePromotion::FunctionalTest(m_context));
}

/*===========================================================================================================*/

FunctionalTest::FunctionalTest(deqp::Context& context)
	: TestCase(context, "functional", "Verifies that texture internal format size promotion mechanism can be used.")
	, m_vao(0)
	, m_source_texture(0)
	, m_destination_texture(0)
	, m_framebuffer(0)
	, m_program(0)
	, m_max_samples(0)
{
	/* Left blank on purpose */
}

tcu::TestNode::IterateResult FunctionalTest::iterate()
{
	/* Get context setup. */
	glu::ContextType context_type = m_context.getRenderContext().getType();
	bool			 is_arb_texture_storage_multisample =
		m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_storage_multisample");

	/* Prepare initial results. */
	bool is_ok	 = true;
	bool was_error = false;

	/* Iterate over test cases. */
	try
	{
		/* Only for OpenGL 3.1 context. */
		if (glu::contextSupports(context_type, glu::ApiType::core(3, 1)))
		{
			/* Generate and bind VAO. */
			prepareVertexArrayObject();

			/* For every required format */
			for (glw::GLuint i = 0; i < s_formats_size; ++i)
			{
				/* Test only if format is required by context. */
				if (glu::contextSupports(context_type, s_formats[i].required_by_context.getAPI()))
				{
					/* For every target. */
					for (glw::GLuint j = 0; j < s_source_texture_targets_count; ++j)
					{
						/* Test if it is supported by context or internal format. */
						if (isTargetMultisampled(s_source_texture_targets[j]))
						{
							if ((!is_arb_texture_storage_multisample) &&
								(!glu::contextSupports(context_type, glu::ApiType::core(4, 3))))
							{
								continue;
							}

							if (!s_formats[i]
									 .is_color_renderable) /* Multisampled textures need to be set using rendering. */
							{
								continue;
							}

							if (isDepthType(s_formats[i]) || isStencilType(s_formats[i]))
							{
								continue;
							}
						}

						if ((isDepthType(s_formats[i]) || isStencilType(s_formats[i])) &&
							(GL_TEXTURE_3D == s_source_texture_targets[j]))
						{
							continue;
						}

						/* Prepare source texture to be tested. */
						try
						{
							prepareSourceTexture(s_formats[i], s_source_texture_targets[j]);
						}
						catch (tcu::NotSupportedError e)
						{
							continue;
						}

						/* Check basic API queries for source texture. */
						is_ok = is_ok & checkSourceTextureSizeAndType(s_formats[i], s_source_texture_targets[j]);

						/* For every [R, G, B, A] component. */
						for (glw::GLuint k = 0; k < COMPONENTS_COUNT; ++k)
						{
							/* Prepare destination texture. */
							prepareDestinationTextureAndFramebuffer(s_formats[i], GL_TEXTURE_2D);

							/* Building program (throws on failure). */
							m_program =
								prepareProgram(s_source_texture_targets[j], s_formats[i], ColorChannelSelector(k));

							/* Setup GL and draw. */
							makeProgramAndSourceTextureActive(s_source_texture_targets[j]);

							drawQuad();

							/* Check results. */
							is_ok = is_ok & checkDestinationTexture(s_formats[i], ColorChannelSelector(k),
																	s_source_texture_targets[j],
																	s_source_texture_targets_names[j]);

							/* Cleanup. */
							cleanDestinationTexture();
							cleanFramebuffer();
							cleanProgram();
						}

						cleanSourceTexture();
					}
				}
			}
		}
	}
	catch (...)
	{
		/* Error have occured. */
		is_ok	 = false;
		was_error = true;
	}

	/* Clean all. */

	cleanSourceTexture();
	cleanDestinationTexture();
	cleanFramebuffer();
	cleanProgram();
	cleanVertexArrayObject();

	/* Result's setup. */
	if (is_ok)
	{
		/* Log success. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Texture Size Promotion Test have passed."
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (was_error)
		{
			/* Log error. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Texture Size Promotion Test have approached error."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			/* Log fail. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Texture Size Promotion Test have failed."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	/* End test. */
	return tcu::TestNode::STOP;
}

void FunctionalTest::prepareVertexArrayObject()
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays have failed");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray have failed");
}

void FunctionalTest::prepareSourceTexture(TextureInternalFormatDescriptor descriptor, glw::GLenum target)
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create and bind texture object. */
	gl.genTextures(1, &m_source_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");

	gl.bindTexture(target, m_source_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");

	if (!isTargetMultisampled(target))
	{
		gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");
	}

	/* Select proper data set. */
	glw::GLvoid* source_data   = DE_NULL;
	glw::GLenum  source_type   = GL_NONE;
	glw::GLenum  source_format = GL_RGBA;

	if (isFloatType(descriptor)) /* For floating type. */
	{
		source_data   = (glw::GLvoid*)s_source_texture_data_f;
		source_type   = GL_FLOAT;
		source_format = GL_RGBA;
	}
	else
	{
		if (isFixedSignedType(descriptor)) /* For fixed signed type. */
		{
			source_data   = (glw::GLvoid*)s_source_texture_data_sn;
			source_type   = GL_FLOAT;
			source_format = GL_RGBA;
		}
		else
		{
			if (isFixedUnsignedType(descriptor)) /* For fixed unsigned type. */
			{
				source_data   = (glw::GLvoid*)s_source_texture_data_n;
				source_type   = GL_FLOAT;
				source_format = GL_RGBA;
			}
			else
			{
				if (isIntegerSignedType(descriptor)) /* For integral signed type. */
				{
					source_data   = (glw::GLvoid*)s_source_texture_data_i;
					source_type   = GL_INT;
					source_format = GL_RGBA_INTEGER;
				}
				else
				{
					if (isIntegerUnsignedType(descriptor)) /* For integral unsigned type. */
					{
						source_data   = (glw::GLvoid*)s_source_texture_data_ui;
						source_type   = GL_UNSIGNED_INT;
						source_format = GL_RGBA_INTEGER;
					}
					else
					{
						if (isDepthType(descriptor)) /* For depth type. */
						{
							source_data   = (glw::GLvoid*)s_source_texture_data_f;
							source_type   = GL_FLOAT;
							source_format = GL_DEPTH_COMPONENT;
						}
						else
						{
							if (isStencilType(descriptor)) /* For stencil type. */
							{
								source_data   = (glw::GLvoid*)s_source_texture_data_ui;
								source_type   = GL_UNSIGNED_INT;
								source_format = GL_STENCIL_INDEX;
							}
						}
					}
				}
			}
		}
	}

	/* Prepare texture storage depending on the target. */
	switch (target)
	{
	case GL_TEXTURE_1D:
		gl.texImage1D(target, 0, descriptor.internal_format, s_source_texture_size, 0, source_format, source_type,
					  source_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");
		break;
	case GL_TEXTURE_1D_ARRAY:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE:
		gl.texImage2D(target, 0, descriptor.internal_format, s_source_texture_size, s_source_texture_size, 0,
					  source_format, source_type, source_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");
		break;
	case GL_TEXTURE_2D_ARRAY:
	case GL_TEXTURE_3D:
		gl.texImage3D(target, 0, descriptor.internal_format, s_source_texture_size, s_source_texture_size,
					  s_source_texture_size, 0, source_format, source_type, source_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");
		break;
	case GL_TEXTURE_2D_MULTISAMPLE:
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		renderDataIntoMultisampledTexture(descriptor, target);
		break;
	default:
		throw 0;
	}
}

void FunctionalTest::renderDataIntoMultisampledTexture(TextureInternalFormatDescriptor descriptor, glw::GLenum target)
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetch limits. */
	gl.getIntegerv(GL_MAX_SAMPLES, &m_max_samples);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv have failed");

	if (m_max_samples == 0)
	{
		m_max_samples = 1;
	}

	/* Setup target. */
	glw::GLenum non_ms_target = (target == GL_TEXTURE_2D_MULTISAMPLE) ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;

	/* Cleanup required by prepareSourceTexture(...). */
	cleanSourceTexture();

	/* Prepare textures and program. */
	prepareSourceTexture(descriptor, non_ms_target);

	prepareDestinationTextureAndFramebuffer(descriptor, target);

	m_program = prepareProgram(non_ms_target, descriptor, COMPONENTS_COUNT);

	/* Setup GL and render texture. */
	makeProgramAndSourceTextureActive(non_ms_target);

	drawQuad();

	/* Cleanup. */
	cleanFramebuffer();
	cleanSourceTexture();

	/* Swpaing destination texture to source texture. */
	m_source_texture = m_destination_texture;

	m_destination_texture = 0;

	/* Clean program. */
	cleanProgram();
}

void FunctionalTest::prepareDestinationTextureAndFramebuffer(TextureInternalFormatDescriptor descriptor,
															 glw::GLenum					 target)
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get internal format. */
	glw::GLenum internal_format = getDestinationFormatForChannel(descriptor);

	/* Create framebuffer object. */
	gl.genFramebuffers(1, &m_framebuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	/* Create framebuffer's destination texture. */
	gl.genTextures(1, &m_destination_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");

	gl.bindTexture(target, m_destination_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");

	/* Allocate texture storage and upload data for test rendering. */
	if (target == GL_TEXTURE_2D)
	{
		glw::GLenum  destination_format = GL_RED;
		glw::GLenum  destination_type   = GL_FLOAT;
		glw::GLvoid* destination_data   = (glw::GLvoid*)s_destination_texture_data_f;

		if (isIntegerSignedType(descriptor))
		{
			destination_format = GL_RED_INTEGER;
			destination_type   = GL_INT;
			destination_data   = (glw::GLvoid*)s_destination_texture_data_i;
		}

		if (isIntegerUnsignedType(descriptor))
		{
			destination_format = GL_RED_INTEGER;
			destination_type   = GL_UNSIGNED_INT;
			destination_data   = (glw::GLvoid*)s_destination_texture_data_ui;
		}

		gl.texImage2D(target, 0, internal_format, s_source_texture_size, s_source_texture_size, 0, destination_format,
					  destination_type, destination_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");

		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_destination_texture, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D have failed");
	}
	else /* Allocate texture storage for uploading datat for multisampled targets (upload must be done via shader). */
	{
		if (target == GL_TEXTURE_2D_MULTISAMPLE)
		{
			gl.texImage2DMultisample(target, m_max_samples - 1, descriptor.internal_format, s_source_texture_size,
									 s_source_texture_size, GL_TRUE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");

			gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, m_destination_texture, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D have failed");
		}
		else
		{
			gl.texImage3DMultisample(target, m_max_samples - 1, descriptor.internal_format, s_source_texture_size,
									 s_source_texture_size, s_source_texture_size, GL_TRUE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");

			gl.framebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_destination_texture, 0, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D have failed");
		}
	}

	/* Check framebuffer completness. */
	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_UNSUPPORTED)
			throw tcu::NotSupportedError("unsupported framebuffer configuration");
		else
			throw 0;
	}

	/* Setup viewport. */
	gl.viewport(0, 0, s_source_texture_size, s_source_texture_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");
}

void FunctionalTest::makeProgramAndSourceTextureActive(glw::GLenum target)
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Use GLSL program. */
	gl.useProgram(m_program);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram have failed");

	/* Setup source texture. */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture have failed");

	gl.bindTexture(target, m_source_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");

	glw::GLint location = gl.getUniformLocation(m_program, "data");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation have failed");

	gl.uniform1i(location, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i have failed");
}

bool FunctionalTest::checkSourceTextureSizeAndType(TextureInternalFormatDescriptor descriptor, glw::GLenum target)
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* query result storage */
	glw::GLint red_size		= 0;
	glw::GLint blue_size	= 0;
	glw::GLint green_size   = 0;
	glw::GLint alpha_size   = 0;
	glw::GLint depth_size   = 0;
	glw::GLint stencil_size = 0;

	glw::GLint red_type   = 0;
	glw::GLint green_type = 0;
	glw::GLint blue_type  = 0;
	glw::GLint alpha_type = 0;
	glw::GLint depth_type = 0;

	/* Bind texture */
	gl.bindTexture(target, m_source_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");

	/* queries */
	gl.getTexLevelParameteriv(target, 0, GL_TEXTURE_RED_SIZE, &red_size);
	gl.getTexLevelParameteriv(target, 0, GL_TEXTURE_GREEN_SIZE, &green_size);
	gl.getTexLevelParameteriv(target, 0, GL_TEXTURE_BLUE_SIZE, &blue_size);
	gl.getTexLevelParameteriv(target, 0, GL_TEXTURE_ALPHA_SIZE, &alpha_size);
	gl.getTexLevelParameteriv(target, 0, GL_TEXTURE_DEPTH_SIZE, &depth_size);
	gl.getTexLevelParameteriv(target, 0, GL_TEXTURE_STENCIL_SIZE, &stencil_size);

	gl.getTexLevelParameteriv(target, 0, GL_TEXTURE_RED_TYPE, &red_type);
	gl.getTexLevelParameteriv(target, 0, GL_TEXTURE_GREEN_TYPE, &green_type);
	gl.getTexLevelParameteriv(target, 0, GL_TEXTURE_BLUE_TYPE, &blue_type);
	gl.getTexLevelParameteriv(target, 0, GL_TEXTURE_ALPHA_TYPE, &alpha_type);
	gl.getTexLevelParameteriv(target, 0, GL_TEXTURE_DEPTH_TYPE, &depth_type);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv have failed");

	/* check expected values */
	bool is_ok = true;

	is_ok = is_ok && (red_size >= descriptor.min_red_size);
	is_ok = is_ok && (green_size >= descriptor.min_green_size);
	is_ok = is_ok && (blue_size >= descriptor.min_blue_size);
	is_ok = is_ok && (alpha_size >= descriptor.min_alpha_size);
	is_ok = is_ok && (depth_size >= descriptor.min_depth_size);
	is_ok = is_ok && (stencil_size >= descriptor.min_stencil_size);

	is_ok = is_ok && ((glw::GLenum)red_type == descriptor.expected_red_type);
	is_ok = is_ok && ((glw::GLenum)green_type == descriptor.expected_green_type);
	is_ok = is_ok && ((glw::GLenum)blue_type == descriptor.expected_blue_type);
	is_ok = is_ok && ((glw::GLenum)alpha_type == descriptor.expected_alpha_type);
	is_ok = is_ok && ((glw::GLenum)depth_type == descriptor.expected_depth_type);

	/* Log on failure. */
	if (!is_ok)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Promotion from internal format " << descriptor.internal_format_name
			<< " have failed during glGetTexLevelParameteriv query. "
			<< "Expected red size = " << descriptor.min_red_size
			<< ", expected green size = " << descriptor.min_green_size
			<< ", expected blue size = " << descriptor.min_blue_size
			<< ", expected alpha size = " << descriptor.min_alpha_size
			<< ", expected depth size = " << descriptor.min_depth_size
			<< ", expected stencil size = " << descriptor.min_stencil_size << ". Queried red size = " << red_size
			<< ", queried green size = " << green_size << ", queried blue size = " << blue_size
			<< ", queried alpha size = " << alpha_size << ", queried depth size = " << depth_size
			<< ", queried stencil size = " << stencil_size << ". "
			<< "Expected red type = " << glu::getTypeStr(descriptor.expected_red_type)
			<< ", expected green type = " << glu::getTypeStr(descriptor.expected_green_type)
			<< ", expected blue type = " << glu::getTypeStr(descriptor.expected_blue_type)
			<< ", expected alpha type = " << glu::getTypeStr(descriptor.expected_alpha_type)
			<< ", expected depth type = " << glu::getTypeStr(descriptor.expected_depth_type)
			<< ". Queried red type = " << glu::getTypeStr(red_type)
			<< ", queried green type = " << glu::getTypeStr(green_type)
			<< ", queried blue type = " << glu::getTypeStr(blue_type)
			<< ", queried alpha type = " << glu::getTypeStr(alpha_type)
			<< ", queried depth type = " << glu::getTypeStr(depth_type) << "." << tcu::TestLog::EndMessage;
	}

	/* return results. */
	return is_ok;
}

glw::GLenum FunctionalTest::getDestinationFormatForChannel(TextureInternalFormatDescriptor descriptor)
{
	if (isFloatType(descriptor))
	{
		return GL_R32F;
	}

	if (isFixedUnsignedType(descriptor))
	{
		return GL_R16;
	}

	if (isFixedSignedType(descriptor))
	{
		return GL_R16_SNORM;
	}

	if (isIntegerUnsignedType(descriptor))
	{
		return GL_R32UI;
	}

	if (isIntegerSignedType(descriptor))
	{
		return GL_R32I;
	}

	return GL_R32F;
}

glw::GLuint FunctionalTest::prepareProgram(glw::GLenum target, TextureInternalFormatDescriptor descriptor,
										   ColorChannelSelector channel)
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Preparing sampler name and textelFetch arguments */
	std::string sampler_name			   = "";
	std::string texel_fetch_arguments_tail = "";

	switch (target)
	{
	case GL_TEXTURE_1D:
		sampler_name			   = "sampler1D";
		texel_fetch_arguments_tail = "0, 0";
		break;
	case GL_TEXTURE_2D:
		sampler_name			   = "sampler2D";
		texel_fetch_arguments_tail = "ivec2(0), 0";
		break;
	case GL_TEXTURE_1D_ARRAY:
		sampler_name			   = "sampler1DArray";
		texel_fetch_arguments_tail = "ivec2(0), 0";
		break;
	case GL_TEXTURE_RECTANGLE:
		sampler_name			   = "sampler2DRect";
		texel_fetch_arguments_tail = "ivec2(0)";
		break;
	case GL_TEXTURE_2D_ARRAY:
		sampler_name			   = "sampler2DArray";
		texel_fetch_arguments_tail = "ivec3(0), 0";
		break;
	case GL_TEXTURE_3D:
		sampler_name			   = "sampler3D";
		texel_fetch_arguments_tail = "ivec3(0), 0";
		break;
	case GL_TEXTURE_2D_MULTISAMPLE:
		sampler_name			   = "sampler2DMS";
		texel_fetch_arguments_tail = "ivec2(0), ";
		texel_fetch_arguments_tail.append(Utilities::itoa(m_max_samples - 1));
		break;
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		sampler_name			   = "sampler2DMSArray";
		texel_fetch_arguments_tail = "ivec3(0), ";
		texel_fetch_arguments_tail.append(Utilities::itoa(m_max_samples - 1));
		break;
	default:
		throw 0;
	}

	/* Preparing component selector name */
	std::string component_name = "";

	switch (channel)
	{
	case RED_COMPONENT:
		component_name = ".r";
		break;
	case GREEN_COMPONENT:
		component_name = ".g";
		break;
	case BLUE_COMPONENT:
		component_name = ".b";
		break;
	case ALPHA_COMPONENT:
		component_name = ".a";
		break;
	case COMPONENTS_COUNT:
		break;
	default:
		throw 0;
	}

	/* Preparing output type name and sampler prefix */
	std::string type_name	  = "";
	std::string sampler_prefix = "";

	if (isFloatType(descriptor) || isFixedSignedType(descriptor) || isFixedUnsignedType(descriptor) ||
		isDepthType(descriptor) || isStencilType(descriptor))
	{
		if (channel == COMPONENTS_COUNT)
		{
			type_name = "vec4";
		}
		else
		{
			type_name = "float";
		}
		sampler_prefix = "";
	}
	else
	{
		if (isIntegerSignedType(descriptor))
		{
			if (channel == COMPONENTS_COUNT)
			{
				type_name = "ivec4";
			}
			else
			{
				type_name = "int";
			}
			sampler_prefix = "i";
		}
		else
		{
			if (channel == COMPONENTS_COUNT)
			{
				type_name = "uvec4";
			}
			else
			{
				type_name = "uint";
			}
			sampler_prefix = "u";
		}
	}

	std::string template_verison = "#version 150";

	/* Preprocessing fragment shader source code. */
	std::string fragment_shader = s_fragment_shader_template;

	fragment_shader = Utilities::preprocessString(fragment_shader, "TEMPLATE_TYPE", type_name);
	fragment_shader =
		Utilities::preprocessString(fragment_shader, "TEMPLATE_SAMPLER", sampler_prefix.append(sampler_name));
	fragment_shader =
		Utilities::preprocessString(fragment_shader, "TEMPLATE_TEXEL_FETCH_ARGUMENTS", texel_fetch_arguments_tail);
	fragment_shader = Utilities::preprocessString(fragment_shader, "TEMPLATE_COMPONENT", component_name);

	/* Building program. */
	glw::GLuint program =
		Utilities::buildProgram(gl, m_context.getTestContext().getLog(), s_vertex_shader_code, fragment_shader.c_str());

	if (0 == program)
	{
		throw 0;
	}

	/* Return program name. */
	return program;
}

void FunctionalTest::drawQuad()
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Draw quad. */
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays have failed");
}

void FunctionalTest::cleanSourceTexture()
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete object. */
	if (m_source_texture)
	{
		gl.deleteTextures(1, &m_source_texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures have failed");

		m_source_texture = 0;
	}
}

void FunctionalTest::cleanDestinationTexture()
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete object. */
	if (m_destination_texture)
	{
		gl.deleteTextures(1, &m_destination_texture);

		m_destination_texture = 0;
	}
}

void FunctionalTest::cleanFramebuffer()
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete object. */
	if (m_framebuffer)
	{
		gl.deleteFramebuffers(1, &m_framebuffer);

		m_framebuffer = 0;
	}
}

void FunctionalTest::cleanProgram()
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete object. */
	if (m_program)
	{
		gl.useProgram(0);

		gl.deleteProgram(m_program);

		m_program = 0;
	}
}

void FunctionalTest::cleanVertexArrayObject()
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete object. */
	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}
}

bool FunctionalTest::checkDestinationTexture(TextureInternalFormatDescriptor descriptor, ColorChannelSelector channel,
											 glw::GLenum target, const glw::GLchar* target_name)
{
	/* GL functions object. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check depending on format. */
	if (isDepthType(descriptor) || isStencilType(descriptor))
	{
		/* Fetch results from destination texture (attached to current framebuffer). */
		glw::GLfloat pixel = 3.1415927f;
		gl.readPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &pixel);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels have failed");

		/* Setup expected value. */
		glw::GLfloat expected_value = (channel == RED_COMPONENT) ?
			                                                 s_source_texture_data_f[0] :
			                                                 (channel == ALPHA_COMPONENT) ? 1.f : 0.f;

		/* Compare expected and fetched values. */
		if (fabs(pixel - expected_value) <= getMinPrecision(descriptor, channel))
		{
			/* Test succeeded*/
			return true;
		}
		else
		{
			/* Log failure. */
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Promotion from internal format " << descriptor.internal_format_name
				<< " have failed during functional test of " << s_color_channel_names[channel]
				<< " channel with target " << target_name << ". Expected value = " << expected_value
				<< " read value = " << pixel << "." << tcu::TestLog::EndMessage;
		}
	}
	else
	{
		if (isFloatType(descriptor))
		{
			/* Fetch results from destination texture (attached to current framebuffer). */
			glw::GLfloat pixel = 3.1415927f;
			gl.readPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &pixel);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels have failed");

			/* Setup expected value. */
			glw::GLfloat expected_value = isChannelTypeNone(descriptor, channel) ?
											  ((channel == ALPHA_COMPONENT) ? 1.f : 0.f) :
											  s_source_texture_data_f[channel];

			/* Compare expected and fetched values. */
			if (fabs(pixel - expected_value) <= getMinPrecision(descriptor, channel))
			{
				/* Test succeeded*/
				return true;
			}
			else
			{
				/* Log failure. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Promotion from internal format " << descriptor.internal_format_name
					<< " have failed during functional test of " << s_color_channel_names[channel]
					<< " channel with target " << target_name << ". Expected value = " << expected_value
					<< " read value = " << pixel << "." << tcu::TestLog::EndMessage;
			}
		}
		else
		{
			if (isFixedSignedType(descriptor))
			{
				/* Fetch results from destination texture (attached to current framebuffer). */
				glw::GLfloat pixel = 3.1415927f;
				gl.readPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &pixel);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels have failed");

				/* Setup expected value. */
				/* Signed fixed-point read color are clamped to [0, 1] by default */
				glw::GLfloat expected_value = isChannelTypeNone(descriptor, channel) ?
												  ((channel == ALPHA_COMPONENT) ? 1.f : 0.f) :
												  deFloatClamp(s_source_texture_data_sn[channel], 0, 1);

				/* Compare expected and fetched values. */
				if (fabs(pixel - expected_value) <= getMinPrecision(descriptor, channel))
				{
					/* Test succeeded*/
					return true;
				}
				else
				{
					/* Log failure. */
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Promotion from internal format " << descriptor.internal_format_name
						<< " have failed during functional test of " << s_color_channel_names[channel]
						<< " channel with target " << target_name << ". Expected value = " << expected_value
						<< " read value = " << pixel << "." << tcu::TestLog::EndMessage;
				}
			}
			else
			{
				if (isFixedUnsignedType(descriptor))
				{
					/* Fetch results from destination texture (attached to current framebuffer). */
					glw::GLfloat pixel = 3.1415927f;
					gl.readPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &pixel);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels have failed");

					/* Setup expected value. */
					glw::GLfloat expected_value = isChannelTypeNone(descriptor, channel) ?
													  ((channel == ALPHA_COMPONENT) ? 1.f : 0.f) :
													  s_source_texture_data_n[channel];

					/* For sRGB internal formats convert value to linear space. */
					if (descriptor.is_sRGB && (channel < ALPHA_COMPONENT))
					{
						expected_value = convert_from_sRGB(expected_value);

						if (isTargetMultisampled(
								target)) /* In multisampled targets two conversions are made (in upload and in shader) */
						{
							expected_value = convert_from_sRGB(expected_value);
						}
					}

					/* Compare expected and fetched values. */
					if (fabs(pixel - expected_value) <= getMinPrecision(descriptor, channel))
					{
						/* Test succeeded*/
						return true;
					}
					else
					{
						/* Log failure. */
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Promotion from internal format " << descriptor.internal_format_name
							<< " have failed during functional test of " << s_color_channel_names[channel]
							<< " channel with target " << target_name << ". Expected value = " << expected_value
							<< " read value = " << pixel << "." << tcu::TestLog::EndMessage;
					}
				}
				else
				{
					if (isIntegerSignedType(descriptor))
					{
						/* Fetch results from destination texture (attached to current framebuffer). */
						glw::GLint pixel = 5;
						gl.readPixels(0, 0, 1, 1, GL_RED_INTEGER, GL_INT, &pixel);
						GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels have failed");

						/* Setup expected value. */
						glw::GLint expected_value = isChannelTypeNone(descriptor, channel) ?
														((channel == ALPHA_COMPONENT) ? 1 : 0) :
														s_source_texture_data_i[channel];

						/* Compare expected and fetched values. */
						if (pixel == expected_value)
						{
							/* Test succeeded*/
							return true;
						}
						else
						{
							/* Log failure. */
							m_context.getTestContext().getLog()
								<< tcu::TestLog::Message << "Promotion from internal format "
								<< descriptor.internal_format_name << " have failed during functional test of "
								<< s_color_channel_names[channel] << " channel with target " << target_name
								<< ". Expected value = " << expected_value << " read value = " << pixel << "."
								<< tcu::TestLog::EndMessage;
						}
					}
					else
					{
						if (isIntegerUnsignedType(descriptor))
						{
							/* Fetch results from destination texture (attached to current framebuffer). */
							glw::GLuint pixel = 5;
							gl.readPixels(0, 0, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &pixel);
							GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels have failed");

							/* Setup expected value. */
							glw::GLuint expected_value = isChannelTypeNone(descriptor, channel) ?
															 ((channel == ALPHA_COMPONENT) ? 1 : 0) :
															 s_source_texture_data_ui[channel];

							/* Compare expected and fetched values. */
							if (pixel == expected_value)
							{
								/* Test succeeded*/
								return true;
							}
							else
							{
								/* Log failure. */
								m_context.getTestContext().getLog()
									<< tcu::TestLog::Message << "Promotion from internal format "
									<< descriptor.internal_format_name << " have failed during functional test of "
									<< s_color_channel_names[channel] << " channel with target " << target_name
									<< ". Expected value = " << expected_value << " read value = " << pixel << "."
									<< tcu::TestLog::EndMessage;
							}
						}
					}
				}
			}
		}
	}

	/* Test failed. */
	return false;
}

bool FunctionalTest::isFloatType(TextureInternalFormatDescriptor descriptor)
{
	return (GL_FLOAT == descriptor.expected_red_type) || (GL_FLOAT == descriptor.expected_green_type) ||
		   (GL_FLOAT == descriptor.expected_blue_type) || (GL_FLOAT == descriptor.expected_alpha_type);
}

bool FunctionalTest::isFixedSignedType(TextureInternalFormatDescriptor descriptor)
{
	return (GL_SIGNED_NORMALIZED == descriptor.expected_red_type) ||
		   (GL_SIGNED_NORMALIZED == descriptor.expected_green_type) ||
		   (GL_SIGNED_NORMALIZED == descriptor.expected_blue_type) ||
		   (GL_SIGNED_NORMALIZED == descriptor.expected_alpha_type);
}

bool FunctionalTest::isFixedUnsignedType(TextureInternalFormatDescriptor descriptor)
{
	return (GL_UNSIGNED_NORMALIZED == descriptor.expected_red_type) ||
		   (GL_UNSIGNED_NORMALIZED == descriptor.expected_green_type) ||
		   (GL_UNSIGNED_NORMALIZED == descriptor.expected_blue_type) ||
		   (GL_UNSIGNED_NORMALIZED == descriptor.expected_alpha_type);
}

bool FunctionalTest::isIntegerSignedType(TextureInternalFormatDescriptor descriptor)
{
	return (GL_INT == descriptor.expected_red_type) || (GL_INT == descriptor.expected_green_type) ||
		   (GL_INT == descriptor.expected_blue_type) || (GL_INT == descriptor.expected_alpha_type);
}

bool FunctionalTest::isIntegerUnsignedType(TextureInternalFormatDescriptor descriptor)
{
	return (GL_UNSIGNED_INT == descriptor.expected_red_type) || (GL_UNSIGNED_INT == descriptor.expected_green_type) ||
		   (GL_UNSIGNED_INT == descriptor.expected_blue_type) || (GL_UNSIGNED_INT == descriptor.expected_alpha_type);
}

bool FunctionalTest::isDepthType(TextureInternalFormatDescriptor descriptor)
{
	return (GL_NONE != descriptor.expected_depth_type);
}

bool FunctionalTest::isStencilType(TextureInternalFormatDescriptor descriptor)
{
	return (descriptor.min_stencil_size > 0);
}

bool FunctionalTest::isChannelTypeNone(TextureInternalFormatDescriptor descriptor, ColorChannelSelector channel)
{
	switch (channel)
	{
	case RED_COMPONENT:
		return (GL_NONE == descriptor.expected_red_type);
	case GREEN_COMPONENT:
		return (GL_NONE == descriptor.expected_green_type);
	case BLUE_COMPONENT:
		return (GL_NONE == descriptor.expected_blue_type);
	case ALPHA_COMPONENT:
		return (GL_NONE == descriptor.expected_alpha_type);
	default:
		throw 0;
	}

	return false;
}

glw::GLfloat FunctionalTest::getMinPrecision(TextureInternalFormatDescriptor descriptor, ColorChannelSelector channel)
{
	/* Select channel data. */
	glw::GLenum type = GL_NONE;
	glw::GLuint size = 0;

	switch (channel)
	{
	case RED_COMPONENT:
		type = descriptor.expected_red_type;
		size = descriptor.min_red_size;
		break;
	case GREEN_COMPONENT:
		type = descriptor.expected_green_type;
		size = descriptor.min_green_size;
		break;
	case BLUE_COMPONENT:
		type = descriptor.expected_blue_type;
		size = descriptor.min_blue_size;
		break;
	case ALPHA_COMPONENT:
		type = descriptor.expected_alpha_type;
		size = descriptor.min_alpha_size;
		break;
	default:
		throw 0;
	}

	/* If it is empty channel. */
	if ((type == GL_NONE) || (size == 0))
	{
		return 0.1f;
	}

	/* If float type. */
	if (isFloatType(descriptor))
	{
		switch (size)
		{
		case 32:
			return 0.00001f; /* specification GL4.5 core constant */
		case 16:
			return 1.f / 1024.f; /* specification GL4.5 core 10 bit mantisa constant */
		case 11:
			return 1.f / 64.f; /* specification GL4.5 core 6 bit mantisa constant */
		case 10:
			return 1.f / 32.f; /* specification GL4.5 core 5 bit mantisa constant */
		default:
			return 0.00001f;
		}
	}

	/* Fixed types precision */
	if (isFixedSignedType(descriptor))
	{
		return (float)(2.0 / pow(2.0, (double)(size - 1 /* sign bit */)));
	}

	if (isFixedUnsignedType(descriptor))
	{
		return (float)(2.0 / pow(2.0, (double)(size)));
	}

	/* other aka (unsigned) integer */
	return 1;
}

bool FunctionalTest::isTargetMultisampled(glw::GLenum target)
{
	return (GL_TEXTURE_2D_MULTISAMPLE == target) || (GL_TEXTURE_2D_MULTISAMPLE_ARRAY == target);
}

float FunctionalTest::convert_from_sRGB(float value)
{
	/* For reference check OpenGL specification (eg. OpenGL 4.5 core profile specification chapter 8.24 */
	if (value > 0.04045f)
	{
		return deFloatPow((value + 0.055f) / 1.055f, 2.4f);
	}

	return value / 12.92f;
}

const glw::GLfloat FunctionalTest::s_source_texture_data_f[] = { 0.125f, 0.25f, 0.5f, 0.75f };

const glw::GLfloat FunctionalTest::s_source_texture_data_n[] = { 0.125f, 0.25f, 0.5f, 0.75f };

const glw::GLfloat FunctionalTest::s_source_texture_data_sn[] = { -0.125f, 0.25f, -0.5f, 0.75f };

const glw::GLint FunctionalTest::s_source_texture_data_i[] = { -1, 2, -3, 4 };

const glw::GLuint FunctionalTest::s_source_texture_data_ui[] = { 4, 3, 2, 1 };

const glw::GLfloat FunctionalTest::s_destination_texture_data_f[] = {
	5.f
}; /* False data for destination texture to be overwriten. */

const glw::GLint FunctionalTest::s_destination_texture_data_i[] = {
	-5
}; /* False data for destination texture to be overwriten. */

const glw::GLuint FunctionalTest::s_destination_texture_data_ui[] = {
	5
}; /* False data for destination texture to be overwriten. */

const glw::GLenum FunctionalTest::s_source_texture_targets[] = {
	GL_TEXTURE_1D,		 GL_TEXTURE_2D, GL_TEXTURE_1D_ARRAY,	   GL_TEXTURE_RECTANGLE,
	GL_TEXTURE_2D_ARRAY, GL_TEXTURE_3D, GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE_ARRAY
};

const glw::GLchar* FunctionalTest::s_source_texture_targets_names[] = {
	STR(GL_TEXTURE_1D),		  STR(GL_TEXTURE_2D), STR(GL_TEXTURE_1D_ARRAY),		  STR(GL_TEXTURE_RECTANGLE),
	STR(GL_TEXTURE_2D_ARRAY), STR(GL_TEXTURE_3D), STR(GL_TEXTURE_2D_MULTISAMPLE), STR(GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
};

const glw::GLuint FunctionalTest::s_source_texture_targets_count =
	sizeof(s_source_texture_targets) / sizeof(s_source_texture_targets[0]);

const glw::GLuint FunctionalTest::s_source_texture_size = 1;

const glw::GLchar* FunctionalTest::s_color_channel_names[] = { "red", "green", "blue", "alpha", "all" };

const FunctionalTest::TextureInternalFormatDescriptor FunctionalTest::s_formats[] = {
	/*	context version,							internal format,		internal format name,	 is sRGB,	CR,size{R,	G,	B,	A,	D,	S},	type of R,				type of G,				type of B,				type of A,				type of depth			*/
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_R8, STR(GL_R8), false, true, 8, 0, 0, 0, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_NONE, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 1, glu::PROFILE_CORE), GL_R8_SNORM, STR(GL_R8_SNORM), false, true, 8, 0, 0, 0, 0, 0,
	  GL_SIGNED_NORMALIZED, GL_NONE, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_R16, STR(GL_R16), false, true, 16, 0, 0, 0, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_NONE, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 1, glu::PROFILE_CORE), GL_R16_SNORM, STR(GL_R16_SNORM), false, true, 16, 0, 0, 0, 0, 0,
	  GL_SIGNED_NORMALIZED, GL_NONE, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RG8, STR(GL_RG8), false, true, 8, 8, 0, 0, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 1, glu::PROFILE_CORE), GL_RG8_SNORM, STR(GL_RG8_SNORM), false, true, 8, 8, 0, 0, 0, 0,
	  GL_SIGNED_NORMALIZED, GL_SIGNED_NORMALIZED, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RG16, STR(GL_RG16), false, true, 16, 16, 0, 0, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 1, glu::PROFILE_CORE), GL_RG16_SNORM, STR(GL_RG16_SNORM), false, true, 16, 16, 0, 0, 0, 0,
	  GL_SIGNED_NORMALIZED, GL_SIGNED_NORMALIZED, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(4, 4, glu::PROFILE_CORE), GL_R3_G3_B2, STR(GL_R3_G3_B2), false, true, 3, 3, 2, 0, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE, GL_NONE },
	{ glu::ContextType(4, 4, glu::PROFILE_CORE), GL_RGB4, STR(GL_RGB4), false, true, 4, 4, 4, 0, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE, GL_NONE },
	{ glu::ContextType(4, 4, glu::PROFILE_CORE), GL_RGB5, STR(GL_RGB5), false, true, 5, 5, 5, 0, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGB8, STR(GL_RGB8), false, true, 8, 8, 8, 0, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 1, glu::PROFILE_CORE), GL_RGB8_SNORM, STR(GL_RGB8_SNORM), false, true, 8, 8, 8, 0, 0, 0,
	  GL_SIGNED_NORMALIZED, GL_SIGNED_NORMALIZED, GL_SIGNED_NORMALIZED, GL_NONE, GL_NONE },
	{ glu::ContextType(4, 4, glu::PROFILE_CORE), GL_RGB10, STR(GL_RGB10), false, true, 10, 10, 10, 0, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE, GL_NONE },
	{ glu::ContextType(4, 4, glu::PROFILE_CORE), GL_RGB12, STR(GL_RGB12), false, true, 12, 12, 12, 0, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGB16, STR(GL_RGB16), false, true, 16, 16, 16, 0, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 1, glu::PROFILE_CORE), GL_RGB16_SNORM, STR(GL_RGB16_SNORM), false, true, 16, 16, 16, 0, 0, 0,
	  GL_SIGNED_NORMALIZED, GL_SIGNED_NORMALIZED, GL_SIGNED_NORMALIZED, GL_NONE, GL_NONE },
	{ glu::ContextType(4, 4, glu::PROFILE_CORE), GL_RGBA2, STR(GL_RGBA2), false, true, 2, 2, 2, 2, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE },
	{ glu::ContextType(4, 2, glu::PROFILE_CORE), GL_RGBA4, STR(GL_RGBA4), false, true, 4, 4, 4, 4, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE },
	{ glu::ContextType(4, 2, glu::PROFILE_CORE), GL_RGB5_A1, STR(GL_RGB5_A1), false, true, 5, 5, 5, 1, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGBA8, STR(GL_RGBA8), false, true, 8, 8, 8, 8, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE },
	{ glu::ContextType(3, 1, glu::PROFILE_CORE), GL_RGBA8_SNORM, STR(GL_RGBA8_SNORM), false, true, 8, 8, 8, 8, 0, 0,
	  GL_SIGNED_NORMALIZED, GL_SIGNED_NORMALIZED, GL_SIGNED_NORMALIZED, GL_SIGNED_NORMALIZED, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGB10_A2, STR(GL_RGB10_A2), false, true, 10, 10, 10, 2, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE },
	{ glu::ContextType(3, 3, glu::PROFILE_CORE), GL_RGB10_A2UI, STR(GL_RGB10_A2UI), false, true, 10, 10, 10, 2, 0, 0,
	  GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_NONE },
	{ glu::ContextType(4, 4, glu::PROFILE_CORE), GL_RGBA12, STR(GL_RGBA12), false, true, 12, 12, 12, 12, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGBA16, STR(GL_RGBA16), false, true, 16, 16, 16, 16, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE },
	{ glu::ContextType(3, 1, glu::PROFILE_CORE), GL_RGBA16_SNORM, STR(GL_RGBA16_SNORM), false, true, 16, 16, 16, 16, 0,
	  0, GL_SIGNED_NORMALIZED, GL_SIGNED_NORMALIZED, GL_SIGNED_NORMALIZED, GL_SIGNED_NORMALIZED, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_SRGB8, STR(GL_SRGB8), true, true, 8, 8, 8, 0, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_SRGB8_ALPHA8, STR(GL_SRGB8_ALPHA8), true, true, 8, 8, 8, 8, 0, 0,
	  GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_R16F, STR(GL_R16F), false, true, 16, 0, 0, 0, 0, 0, GL_FLOAT,
	  GL_NONE, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RG16F, STR(GL_RG16F), false, true, 16, 16, 0, 0, 0, 0, GL_FLOAT,
	  GL_FLOAT, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGB16F, STR(GL_RGB16F), false, true, 16, 16, 16, 0, 0, 0, GL_FLOAT,
	  GL_FLOAT, GL_FLOAT, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGBA16F, STR(GL_RGBA16F), false, true, 16, 16, 16, 16, 0, 0,
	  GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_R32F, STR(GL_R32F), false, true, 32, 0, 0, 0, 0, 0, GL_FLOAT,
	  GL_NONE, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RG32F, STR(GL_RG32F), false, true, 32, 32, 0, 0, 0, 0, GL_FLOAT,
	  GL_FLOAT, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGB32F, STR(GL_RGB32F), false, true, 32, 32, 32, 0, 0, 0, GL_FLOAT,
	  GL_FLOAT, GL_FLOAT, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGBA32F, STR(GL_RGBA32F), false, true, 32, 32, 32, 32, 0, 0,
	  GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_R11F_G11F_B10F, STR(GL_R11F_G11F_B10F), false, true, 11, 11, 10, 0,
	  0, 0, GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGB9_E5, STR(GL_RGB9_E5), false, false, 9, 9, 9, 0, 0, 0, GL_FLOAT,
	  GL_FLOAT, GL_FLOAT, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_R8I, STR(GL_R8I), false, true, 8, 0, 0, 0, 0, 0, GL_INT, GL_NONE,
	  GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_R8UI, STR(GL_R8UI), false, true, 8, 0, 0, 0, 0, 0, GL_UNSIGNED_INT,
	  GL_NONE, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_R16I, STR(GL_R16I), false, true, 16, 0, 0, 0, 0, 0, GL_INT, GL_NONE,
	  GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_R16UI, STR(GL_R16UI), false, true, 16, 0, 0, 0, 0, 0,
	  GL_UNSIGNED_INT, GL_NONE, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_R32I, STR(GL_R32I), false, true, 32, 0, 0, 0, 0, 0, GL_INT, GL_NONE,
	  GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_R32UI, STR(GL_R32UI), false, true, 32, 0, 0, 0, 0, 0,
	  GL_UNSIGNED_INT, GL_NONE, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RG8I, STR(GL_RG8I), false, true, 8, 8, 0, 0, 0, 0, GL_INT, GL_INT,
	  GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RG8UI, STR(GL_RG8UI), false, true, 8, 8, 0, 0, 0, 0,
	  GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RG16I, STR(GL_RG16I), false, true, 16, 16, 0, 0, 0, 0, GL_INT,
	  GL_INT, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RG16UI, STR(GL_RG16UI), false, true, 16, 16, 0, 0, 0, 0,
	  GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RG32I, STR(GL_RG32I), false, true, 32, 32, 0, 0, 0, 0, GL_INT,
	  GL_INT, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RG32UI, STR(GL_RG32UI), false, true, 32, 32, 0, 0, 0, 0,
	  GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_NONE, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGB8I, STR(GL_RGB8I), false, true, 8, 8, 8, 0, 0, 0, GL_INT, GL_INT,
	  GL_INT, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGB8UI, STR(GL_RGB8UI), false, true, 8, 8, 8, 0, 0, 0,
	  GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGB16I, STR(GL_RGB16I), false, true, 16, 16, 16, 0, 0, 0, GL_INT,
	  GL_INT, GL_INT, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGB16UI, STR(GL_RGB16UI), false, true, 16, 16, 16, 0, 0, 0,
	  GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGB32I, STR(GL_RGB32I), false, true, 32, 32, 32, 0, 0, 0, GL_INT,
	  GL_INT, GL_INT, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGB32UI, STR(GL_RGB32UI), false, true, 32, 32, 32, 0, 0, 0,
	  GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_NONE, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGBA8I, STR(GL_RGBA8I), false, true, 8, 8, 8, 8, 0, 0, GL_INT,
	  GL_INT, GL_INT, GL_INT, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGBA8UI, STR(GL_RGBA8UI), false, true, 8, 8, 8, 8, 0, 0,
	  GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGBA16I, STR(GL_RGBA16I), false, true, 16, 16, 16, 16, 0, 0, GL_INT,
	  GL_INT, GL_INT, GL_INT, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGBA16UI, STR(GL_RGBA16UI), false, true, 16, 16, 16, 16, 0, 0,
	  GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGBA32I, STR(GL_RGBA32I), false, true, 32, 32, 32, 32, 0, 0, GL_INT,
	  GL_INT, GL_INT, GL_INT, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_RGBA32UI, STR(GL_RGBA32UI), false, true, 32, 32, 32, 32, 0, 0,
	  GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_UNSIGNED_INT, GL_NONE },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_DEPTH_COMPONENT16, STR(GL_DEPTH_COMPONENT16), false, true, 0, 0, 0,
	  0, 16, 0, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_UNSIGNED_NORMALIZED },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_DEPTH_COMPONENT24, STR(GL_DEPTH_COMPONENT24), false, true, 0, 0, 0,
	  0, 24, 0, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_UNSIGNED_NORMALIZED },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_DEPTH_COMPONENT32F, STR(GL_DEPTH_COMPONENT32F), false, true, 0, 0,
	  0, 0, 32, 0, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_FLOAT },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_DEPTH24_STENCIL8, STR(GL_DEPTH24_STENCIL8), false, true, 0, 0, 0, 0,
	  24, 8, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_UNSIGNED_NORMALIZED },
	{ glu::ContextType(3, 0, glu::PROFILE_CORE), GL_DEPTH32F_STENCIL8, STR(GL_DEPTH32F_STENCIL8), false, true, 0, 0, 0,
	  0, 32, 8, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_FLOAT }
};

const glw::GLuint FunctionalTest::s_formats_size = sizeof(s_formats) / sizeof(s_formats[0]);

const glw::GLchar* FunctionalTest::s_vertex_shader_code = "#version 150\n"
														  "\n"
														  "void main()\n"
														  "{\n"
														  "    switch(gl_VertexID % 4)\n"
														  "    {\n"
														  "    case 0:\n"
														  "        gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
														  "        break;\n"
														  "    case 1:\n"
														  "        gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
														  "        break;\n"
														  "    case 2:\n"
														  "        gl_Position = vec4( 1.0, -1.0, 0.0, 1.0);\n"
														  "        break;\n"
														  "    case 3:\n"
														  "        gl_Position = vec4( 1.0,  1.0, 0.0, 1.0);\n"
														  "        break;\n"
														  "    }\n"
														  "}\n";

const glw::GLchar* FunctionalTest::s_fragment_shader_template =
	"#version 150\n"
	"\n"
	"out TEMPLATE_TYPE result;\n"
	"\n"
	"uniform TEMPLATE_SAMPLER data;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    result = texelFetch(data, TEMPLATE_TEXEL_FETCH_ARGUMENTS)TEMPLATE_COMPONENT;\n"
	"}\n";

/*===========================================================================================================*/

namespace Utilities
{

glw::GLuint buildProgram(glw::Functions const& gl, tcu::TestLog& log, glw::GLchar const* const vertex_shader_source,
						 glw::GLchar const* const fragment_shader_source)
{
	glw::GLuint program = 0;

	struct Shader
	{
		glw::GLchar const* const source;
		glw::GLenum const		 type;
		glw::GLuint				 id;
	} shader[] = { { vertex_shader_source, GL_VERTEX_SHADER, 0 }, { fragment_shader_source, GL_FRAGMENT_SHADER, 0 } };

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	try
	{
		/* Create program. */
		program = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			if (DE_NULL != shader[i].source)
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(program, shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

				gl.shaderSource(shader[i].id, 1, &(shader[i].source), NULL);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource call failed.");

				gl.compileShader(shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader call failed.");

				glw::GLint status = GL_FALSE;

				gl.getShaderiv(shader[i].id, GL_COMPILE_STATUS, &status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

				if (GL_FALSE == status)
				{
					glw::GLint log_size = 0;
					gl.getShaderiv(shader[i].id, GL_INFO_LOG_LENGTH, &log_size);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

					glw::GLchar* log_text = new glw::GLchar[log_size];

					gl.getShaderInfoLog(shader[i].id, log_size, NULL, &log_text[0]);

					log << tcu::TestLog::Message << "Shader compilation has failed.\n"
						<< "Shader type: " << glu::getShaderTypeStr(shader[i].type) << "\n"
						<< "Shader compilation error log:\n"
						<< log_text << "\n"
						<< "Shader source code:\n"
						<< shader[i].source << "\n"
						<< tcu::TestLog::EndMessage;

					delete[] log_text;

					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog call failed.");

					throw 0;
				}
			}
		}

		/* Link. */
		gl.linkProgram(program);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(program, GL_LINK_STATUS, &status);

		if (GL_TRUE == status)
		{
			for (glw::GLuint i = 0; i < shader_count; ++i)
			{
				if (shader[i].id)
				{
					gl.detachShader(program, shader[i].id);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glDetachShader call failed.");
				}
			}
		}
		else
		{
			glw::GLint log_size = 0;

			gl.getProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

			glw::GLchar* log_text = new glw::GLchar[log_size];

			gl.getProgramInfoLog(program, log_size, NULL, &log_text[0]);

			log << tcu::TestLog::Message << "Program linkage has failed due to:\n"
				<< log_text << "\n"
				<< tcu::TestLog::EndMessage;

			delete[] log_text;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog call failed.");

			throw 0;
		}
	}
	catch (...)
	{
		if (program)
		{
			gl.deleteProgram(program);

			program = 0;
		}
	}

	for (glw::GLuint i = 0; i < shader_count; ++i)
	{
		if (0 != shader[i].id)
		{
			gl.deleteShader(shader[i].id);

			shader[i].id = 0;
		}
	}

	return program;
}

std::string preprocessString(std::string source, std::string key, std::string value)
{
	std::string destination = source;

	while (true)
	{
		/* Find token in source code. */
		size_t position = destination.find(key, 0);

		/* No more occurences of this key. */
		if (position == std::string::npos)
		{
			break;
		}

		/* Replace token with sub_code. */
		destination.replace(position, key.size(), value);
	}

	return destination;
}

std::string itoa(glw::GLint i)
{
	std::stringstream stream;

	stream << i;

	return stream.str();
}

} // namespace Utilities
} // namespace TextureSizePromotion
} // namespace gl3cts
