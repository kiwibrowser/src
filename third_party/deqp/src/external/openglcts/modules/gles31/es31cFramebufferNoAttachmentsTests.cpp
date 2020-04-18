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
#include "es31cFramebufferNoAttachmentsTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glw.h"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

using tcu::TestLog;
using std::string;
using std::vector;
using glcts::Context;

// I tried to find something like this, but failed
void checkErrorEqualsExpected(GLenum err, GLenum expected, const char* msg, const char* file, int line)
{
	if (err != expected)
	{
		std::ostringstream msgStr;
		msgStr << "glGetError() returned " << glu::getErrorStr(err) << ", expected " << glu::getErrorStr(expected);

		if (msg)
			msgStr << " in '" << msg << "'";

		if (err == GL_OUT_OF_MEMORY)
			throw glu::OutOfMemoryError(msgStr.str().c_str(), "", file, line);
		else
			throw glu::Error(err, msgStr.str().c_str(), "", file, line);
	}
}

#define GLU_EXPECT_ERROR(ERR, EXPECTED, MSG) checkErrorEqualsExpected((ERR), EXPECTED, MSG, __FILE__, __LINE__)

// Contains expect_fbo_status()
class FramebufferNoAttachmentsBaseCase : public TestCase
{
public:
	FramebufferNoAttachmentsBaseCase(Context& context, const char* name, const char* description);
	~FramebufferNoAttachmentsBaseCase();

protected:
	void expect_fbo_status(GLenum target, GLenum expected_status, const char* fail_message);
};

FramebufferNoAttachmentsBaseCase::FramebufferNoAttachmentsBaseCase(Context& context, const char* name,
																   const char* description)
	: TestCase(context, name, description)
{
}

FramebufferNoAttachmentsBaseCase::~FramebufferNoAttachmentsBaseCase()
{
}

// API tests
class FramebufferNoAttachmentsApiCase : public FramebufferNoAttachmentsBaseCase
{
public:
	FramebufferNoAttachmentsApiCase(Context& context, const char* name, const char* description);
	~FramebufferNoAttachmentsApiCase();

	IterateResult iterate();

private:
	void begin_fbo_no_attachments(GLenum target);
	void begin_fbo_with_multisample_renderbuffer(GLenum target);
	void begin_fbo(GLenum target, unsigned test_case);
	void end_fbo(GLenum target);

private:
	GLuint m_fbo;
	GLuint m_renderbuffer;
	GLuint m_texture;
};

FramebufferNoAttachmentsApiCase::FramebufferNoAttachmentsApiCase(Context& context, const char* name,
																 const char* description)
	: FramebufferNoAttachmentsBaseCase(context, name, description), m_fbo(0), m_renderbuffer(0), m_texture(0)
{
}

FramebufferNoAttachmentsApiCase::~FramebufferNoAttachmentsApiCase()
{
}

void FramebufferNoAttachmentsBaseCase::expect_fbo_status(GLenum target, GLenum expected_status,
														 const char* fail_message)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	GLenum				  error;

	error = gl.getError();
	if (error != GL_NO_ERROR)
	{
		std::ostringstream msgStr;
		msgStr << "Error before glCheckFramebufferStatus() for '" << fail_message << "'\n";

		GLU_EXPECT_NO_ERROR(error, "Error before glCheckFramebufferStatus()");
	}

	TCU_CHECK_MSG(gl.checkFramebufferStatus(target) == expected_status, fail_message);

	error = gl.getError();
	if (error != GL_NO_ERROR)
	{
		std::ostringstream msgStr;
		msgStr << "Error after glCheckFramebufferStatus() for '" << fail_message << "'\n";

		GLU_EXPECT_NO_ERROR(error, "Error after glCheckFramebufferStatus()");
	}
}

void FramebufferNoAttachmentsApiCase::begin_fbo_no_attachments(GLenum target)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genFramebuffers(1, &m_fbo);
	gl.bindFramebuffer(target, m_fbo);

	// A freshly created framebuffer with no attachment is expected to be incomplete
	// until default width and height is set.
	expect_fbo_status(target, GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT,
					  "error setting up framebuffer with multisample attachment");
}

void FramebufferNoAttachmentsApiCase::begin_fbo_with_multisample_renderbuffer(GLenum target)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genFramebuffers(1, &m_fbo);
	gl.bindFramebuffer(target, m_fbo);
	gl.genRenderbuffers(1, &m_renderbuffer);
	gl.bindRenderbuffer(GL_RENDERBUFFER, m_renderbuffer);
	gl.renderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA8, 101, 102);
	gl.framebufferRenderbuffer(target, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderbuffer);
	expect_fbo_status(target, GL_FRAMEBUFFER_COMPLETE, "framebuffer with an attachment should be complete");
}

void FramebufferNoAttachmentsApiCase::begin_fbo(GLenum target, unsigned test_case)
{
	switch (test_case)
	{
	case 0:
		begin_fbo_no_attachments(target);
		break;
	case 1:
		begin_fbo_with_multisample_renderbuffer(target);
		break;
	}
}

void FramebufferNoAttachmentsApiCase::end_fbo(GLenum target)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindFramebuffer(target, 0);
	gl.deleteFramebuffers(1, &m_fbo);
	gl.deleteRenderbuffers(1, &m_renderbuffer);
	gl.deleteTextures(1, &m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "error deleting framebuffer / renderbuffer / texture");
	m_fbo		   = 0;
	m_renderbuffer = 0;
	m_texture	  = 0;
}

FramebufferNoAttachmentsApiCase::IterateResult FramebufferNoAttachmentsApiCase::iterate()
{
	const glw::Functions& gl   = m_context.getRenderContext().getFunctions();
	bool				  isOk = true;
	GLint				  binding;

	GLenum targets[] = {
		GL_DRAW_FRAMEBUFFER, GL_READ_FRAMEBUFFER,
		GL_FRAMEBUFFER // equivalent to DRAW_FRAMEBUFFER
	};
	GLenum bindings[] = {
		GL_DRAW_FRAMEBUFFER_BINDING, GL_READ_FRAMEBUFFER_BINDING,
		GL_FRAMEBUFFER_BINDING // equivalent to DRAW_FRAMEBUFFER_BINDING
	};
	GLenum pnames[] = { GL_FRAMEBUFFER_DEFAULT_WIDTH, GL_FRAMEBUFFER_DEFAULT_HEIGHT, GL_FRAMEBUFFER_DEFAULT_SAMPLES,
						GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS };
	GLenum enums_invalid_list[] = { GL_NOTEQUAL,
									GL_FRONT_FACE,
									GL_PACK_ROW_LENGTH,
									GL_FIXED,
									GL_LINEAR_MIPMAP_NEAREST,
									GL_RGBA4,
									GL_TEXTURE_MAX_LOD,
									GL_RG32F,
									GL_ALIASED_POINT_SIZE_RANGE,
									GL_VERTEX_ATTRIB_ARRAY_TYPE,
									GL_DRAW_BUFFER7,
									GL_MAX_COMBINED_UNIFORM_BLOCKS,
									GL_MAX_VARYING_COMPONENTS,
									GL_SRGB,
									GL_RGB8UI,
									GL_IMAGE_BINDING_NAME,
									GL_TEXTURE_2D_MULTISAMPLE,
									GL_COMPRESSED_R11_EAC,
									GL_BUFFER_DATA_SIZE };

	GLint default_values[] = { 0, 0, 0, GL_FALSE };
	GLint valid_values[]   = { 103, 104, 4, GL_TRUE };
	GLint min_values[]	 = { 0, 0, 0, -1 }; // Skip min_value test for boolean
	GLint max_values[]	 = { 0, 0, 0, -1 }; // Skip max_value test for boolean.

	unsigned num_targets			= sizeof(targets) / sizeof(GLenum);
	unsigned num_pnames				= sizeof(pnames) / sizeof(GLenum);
	unsigned num_enums_invalid_list = sizeof(enums_invalid_list) / sizeof(GLenum);

	// Check for extra pnames allowed from supported extensions.
	vector<GLenum> pnames_ext;
	if (m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader") ||
		m_context.getContextInfo().isExtensionSupported("GL_OES_geometry_shader"))
	{
		pnames_ext.push_back(GL_FRAMEBUFFER_DEFAULT_LAYERS);
	}

	// "Random" invalid enums distributed roughly evenly throughout 16bit enum number range.
	vector<GLenum> enums_invalid;
	if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader") &&
		!m_context.getContextInfo().isExtensionSupported("GL_OES_geometry_shader"))
	{
		enums_invalid.push_back(GL_FRAMEBUFFER_DEFAULT_LAYERS);
	}
	for (unsigned i = 0; i < num_enums_invalid_list; ++i)
	{
		enums_invalid.push_back(enums_invalid_list[i]);
	}

	gl.getIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &max_values[0]);
	gl.getIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &max_values[1]);
	gl.getIntegerv(GL_MAX_FRAMEBUFFER_SAMPLES, &max_values[2]);
	GLU_EXPECT_NO_ERROR(
		gl.getError(),
		"Querying GL_MAX_FRAMEBUFFER_WIDTH / GL_MAX_FRAMEBUFFER_HEIGHT / GL_MAX_FRAMEBUFFER_SAMPLES failed");

	TCU_CHECK_MSG(max_values[0] >= 2048, "GL_MAX_FRAMEBUFFER_WIDTH does not meet minimum requirements");

	TCU_CHECK_MSG(max_values[1] >= 2048, "GL_MAX_FRAMEBUFFER_HEIGHT does not meet minimum requirements");

	TCU_CHECK_MSG(max_values[2] >= 4, "GL_MAX_FRAMEBUFFER_SAMPLES does not meet minimum requirements");

	// It is valid to ask for number of samples > 0 and get
	// implementation defined value which is above the requested
	// value. We can use simple equality comparison by using
	// reported maximum number of samples in our valid value
	// set and get test.
	valid_values[2] = max_values[2];

	// Invalid target
	for (unsigned i = 0; i < enums_invalid.size(); ++i)
	{
		GLenum target   = enums_invalid[i];
		bool   is_valid = false;
		for (unsigned j = 0; j < num_targets; ++j)
		{
			if (target == targets[j])
			{
				is_valid = true;
				break;
			}
		}

		if (is_valid)
			continue;

		for (unsigned j = 0; j < num_pnames; ++j)
		{
			gl.framebufferParameteri(target, pnames[j], valid_values[j]);
			GLU_EXPECT_ERROR(gl.getError(), GL_INVALID_ENUM,
							 "Using glFramebufferParameteri() with invalid target should set GL_INVALID_ENUM");
		}
	}

	// For all valid targets
	for (unsigned i = 0; i < num_targets; ++i)
	{
		GLenum target = targets[i];

		glGetIntegerv(bindings[i], &binding);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Valid call to glGetIntegerv() "
										   "should not set GL error");

		// Using default framebuffer - GL_INVALID_OPERATION
		for (unsigned j = 0; j < num_pnames; ++j)
		{
			GLint  get_value = ~0;
			GLenum pname	 = pnames[j];

			gl.framebufferParameteri(target, pname, valid_values[j]);
			if (binding == 0)
			{
				GLU_EXPECT_ERROR(gl.getError(), GL_INVALID_OPERATION,
								 "Using glFramebufferParameteri() on default framebuffer "
								 "should set GL_INVALID_OPERATION");
			}
			else
			{
				GLU_EXPECT_NO_ERROR(gl.getError(), "Valid call to glGetFramebufferParameteriv() "
												   "should not set GL error");
			}

			gl.getFramebufferParameteriv(target, pname, &get_value);

			if (binding == 0)
			{
				GLU_EXPECT_ERROR(gl.getError(), GL_INVALID_OPERATION,
								 "Using glGetFramebufferParameteriv() on default framebuffer "
								 "should set GL_INVALID_OPERATION");
				TCU_CHECK_MSG(get_value == ~0, "failed call to glGetFramebufferParameteriv() "
											   "should not modify params");
			}
			else
			{
				GLU_EXPECT_NO_ERROR(gl.getError(), "Valid call to glGetFramebufferParameteriv() "
												   "should not set GL error");
			}
		}

		// j == 0 : fbo without attachments
		// j == 1 : fbo with a multisample attachment
		for (unsigned j = 0; j < 2; ++j)
		{
			glGetIntegerv(bindings[i], &binding);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Valid call to glGetIntegerv() "
											   "should not set GL error");

			if (binding == 0)
			{
				//  Check FBO status of default framebuffer
				//  TODO Check presence of default framebuffer - default framebuffer is complete
				//       only if it exists
				expect_fbo_status(target, GL_FRAMEBUFFER_COMPLETE, "Default framebuffer should be complete");
			}

			//  Invalid pname - GL_INVALID_VALUE
			begin_fbo(target, j);
			for (unsigned k = 0; k < enums_invalid.size(); ++k)
			{
				GLenum pname	= enums_invalid[k];
				bool   is_valid = false;
				for (unsigned m = 0; m < num_pnames; ++m)
				{
					if (pname == pnames[m])
					{
						is_valid = true;
						break;
					}
				}

				// Ignore any pnames that are added by extensions.
				for (unsigned m = 0; m < pnames_ext.size(); ++m)
				{
					if (pname == pnames_ext[m])
					{
						is_valid = true;
						break;
					}
				}

				if (is_valid)
					continue;

				GLint get_value = ~0;

				gl.framebufferParameteri(target, pname, 0);
				GLU_EXPECT_ERROR(gl.getError(), GL_INVALID_ENUM, "Calling glFramebufferParameteri() with invalid pname "
																 "should set GL_INVALID_ENUM");

				gl.getFramebufferParameteriv(target, pname, &get_value);
				GLU_EXPECT_ERROR(gl.getError(), GL_INVALID_ENUM,
								 "Calling glGetFramebufferParameteriv() with invalid pname "
								 "should set GL_INVALID_ENUM");

				TCU_CHECK_MSG(get_value == ~0, "Calling glGetFramebufferParameteriv() with invalid pname "
											   "should not modify params");
			}
			end_fbo(target);

			//  Valid set and get
			begin_fbo(target, j);
			{
				for (unsigned k = 0; k < num_pnames; ++k)
				{
					GLint get_value = ~0;

					gl.framebufferParameteri(target, pnames[k], valid_values[k]);
					GLU_EXPECT_NO_ERROR(gl.getError(), "Valid call to glFramebufferParameteri() "
													   "should not set GL error");

					gl.getFramebufferParameteriv(target, pnames[k], &get_value);
					GLU_EXPECT_NO_ERROR(gl.getError(), "Valid call to glGetFramebufferParameteriv() "
													   "should not set GL error");

					TCU_CHECK_MSG(get_value == valid_values[k],
								  "glGetFramebufferParameteriv() "
								  "should have returned the value set with glFramebufferParameteri()");
				}

				//  After valid set, check FBO status of user FBO
				expect_fbo_status(target, GL_FRAMEBUFFER_COMPLETE,
								  "Framebuffer should be complete after setting valid valid");
			}
			end_fbo(target);

			//  Negative or too large values - GL_INVALID_VALUE
			//  Also check for correct default values
			begin_fbo(target, j);
			for (unsigned k = 0; k < num_pnames; ++k)
			{
				GLint  get_value = ~0;
				GLenum pname	 = pnames[k];

				if (min_values[k] >= 0)
				{
					gl.framebufferParameteri(target, pname, min_values[k] - 1);
					GLU_EXPECT_ERROR(gl.getError(), GL_INVALID_VALUE,
									 "Calling glFramebufferParameteri() with negative value "
									 "should set GL_INVALID_VALUE");
				}

				gl.getFramebufferParameteriv(target, pname, &get_value);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Valid call to glGetFramebufferParameteriv() "
												   "should not set GL error");

				TCU_CHECK_MSG(get_value == default_values[k], "glGetFramebufferParameteriv() "
															  "did not return a valid default value");

				get_value = ~0;
				if (max_values[k] >= 0)
				{
					gl.framebufferParameteri(target, pname, max_values[k] + 1);
					GLU_EXPECT_ERROR(gl.getError(), GL_INVALID_VALUE,
									 "Calling glFramebufferParameteri() too large value "
									 "should set GL_INVALID_VALUE");
				}

				gl.getFramebufferParameteriv(target, pname, &get_value);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Valid call to glGetFramebufferParameteriv() "
												   "should not set GL error");

				TCU_CHECK_MSG(get_value == default_values[k], "glGetFramebufferParameteriv() "
															  "did not return a valid default value");
			}
			end_fbo(target);
		}
	}

	m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, isOk ? "Pass" : "Fail");
	return STOP;
}

// Draw with imageStore, validate that framebuffer
// default width and height is respected.
class FramebufferNoAttachmentsRenderCase : public FramebufferNoAttachmentsBaseCase
{
public:
	FramebufferNoAttachmentsRenderCase(Context& context, const char* name, const char* description);

	IterateResult iterate();
	void		  deinit(void);

private:
	GLuint m_program;
	GLuint m_vertex_shader;
	GLuint m_fragment_shader;
	GLuint m_vao;
	GLuint m_framebuffer;
	GLuint m_texture;
};

FramebufferNoAttachmentsRenderCase::FramebufferNoAttachmentsRenderCase(Context& context, const char* name,
																	   const char* description)
	: FramebufferNoAttachmentsBaseCase(context, name, description)
	, m_program(0)
	, m_vertex_shader(0)
	, m_fragment_shader(0)
	, m_vao(0)
	, m_framebuffer(0)
	, m_texture(0)
{
}

void FramebufferNoAttachmentsRenderCase::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	gl.deleteShader(m_vertex_shader);
	gl.deleteShader(m_fragment_shader);
	gl.deleteProgram(m_program);
	gl.deleteVertexArrays(1, &m_vao);
	gl.deleteTextures(1, &m_texture);
	gl.deleteFramebuffers(1, &m_framebuffer);
}

FramebufferNoAttachmentsRenderCase::IterateResult FramebufferNoAttachmentsRenderCase::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	int					  max_fragment_image_uniforms;
	bool				  isOk = true;

	// Check GL_MAX_FRAGMENT_IMAGE_UNIFORMS, we need at least one
	glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &max_fragment_image_uniforms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Querying GL_MAX_FRAGMENT_IMAGE_UNIFORMS");

	if (max_fragment_image_uniforms < 1)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_MAX_FRAGMENT_IMAGE_UNIFORMS<1");
		return STOP;
	}

	// Create program and VAO
	{
		const char* vs_source = "#version 310 es\n"
								"void main()\n"
								"{\n"
								"   if      (gl_VertexID == 0) gl_Position = vec4(-1, -1, 0, 1);\n"
								"   else if (gl_VertexID == 1) gl_Position = vec4(-1,  1, 0, 1);\n"
								"   else if (gl_VertexID == 2) gl_Position = vec4( 1,  1, 0, 1);\n"
								"   else                       gl_Position = vec4( 1, -1, 0, 1);\n"
								"}\n";

		const char* fs_source = "#version 310 es\n"
								"precision highp uimage2D;\n"
								"layout(r32ui) uniform uimage2D data;\n"
								"void main()\n"
								"{\n"
								"   ivec2 image_info = ivec2(gl_FragCoord.xy);\n"
								"   imageStore(data, image_info, uvec4(1, 2, 3, 4));\n"
								"}\n";

		m_program		  = gl.createProgram();
		m_vertex_shader   = gl.createShader(GL_VERTEX_SHADER);
		m_fragment_shader = gl.createShader(GL_FRAGMENT_SHADER);
		gl.shaderSource(m_vertex_shader, 1, &vs_source, NULL);
		gl.compileShader(m_vertex_shader);
		gl.attachShader(m_program, m_vertex_shader);
		gl.shaderSource(m_fragment_shader, 1, &fs_source, NULL);
		gl.compileShader(m_fragment_shader);
		gl.attachShader(m_program, m_fragment_shader);
		gl.linkProgram(m_program);
		gl.useProgram(m_program);
		gl.genVertexArrays(1, &m_vao);
		gl.bindVertexArray(m_vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Creating program and VAO");
	}

	// Create framebuffer with no attachments
	gl.genFramebuffers(1, &m_framebuffer);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer);
	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, 32);
	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, 32);
	expect_fbo_status(GL_READ_FRAMEBUFFER, GL_FRAMEBUFFER_COMPLETE, "Creating framebuffer with no attachments");

	// Create texture and clear it, temporarily attaching to FBO
	{
		GLuint zero = 0;
		gl.genTextures(1, &m_texture);
		gl.bindTexture(GL_TEXTURE_2D, m_texture);
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, 64, 64);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
		gl.viewport(0, 0, 64, 64);
		gl.clearBufferuiv(GL_COLOR, 0, &zero);
		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Creating and clearing texture");
	}

	// Draw using storeImage
	gl.drawBuffers(0, NULL);
	gl.viewport(0, 0, 64, 64);
	gl.bindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
	gl.drawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl.memoryBarrier(GL_ALL_BARRIER_BITS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Draw with imageStore");

	// Read and validate texture contents
	{
		GLuint pixels[64 * 64 * 4];
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
		gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
		expect_fbo_status(GL_READ_FRAMEBUFFER, GL_FRAMEBUFFER_COMPLETE, "ReadPixels to texture for validation");
		gl.readPixels(0, 0, 64, 64, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &pixels[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ReadPixels");

		for (unsigned y = 0; y < 64; ++y)
		{
			for (unsigned x = 0; x < 64; ++x)
			{
				GLuint expected_value = (x < 32) && (y < 32) ? 1 : 0;
				GLuint value		  = pixels[(y * 64 + x) * 4];
				TCU_CHECK_MSG(value == expected_value, "Validating draw with imageStore");
			}
		}
	}

	m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, isOk ? "Pass" : "Fail");
	return STOP;
}

FramebufferNoAttachmentsTests::FramebufferNoAttachmentsTests(Context& context)
	: TestCaseGroup(context, "framebuffer_no_attachments", "Framebuffer no attachments tests")
{
}

FramebufferNoAttachmentsTests::~FramebufferNoAttachmentsTests(void)
{
}

void FramebufferNoAttachmentsTests::init(void)
{
	addChild(new FramebufferNoAttachmentsApiCase(m_context, "api", "Basic API verification"));

	addChild(new FramebufferNoAttachmentsRenderCase(m_context, "render", "Rendering with imageStore"));
}

} // glcts
