/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file es32cCopyImageTests.cpp
 * \brief Implements CopyImageSubData functional tests.
 */ /*-------------------------------------------------------------------*/

#include "es32cCopyImageTests.hpp"

#include "gluDefs.hpp"
#include "gluDrawUtil.hpp"
#include "gluShaderProgram.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuFloat.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <vector>

#include "deMath.h"

using namespace glw;

#define TEXTURE_WIDTH 16
#define TEXTURE_HEIGHT 16

namespace glcts
{

/** Implements functional test. Description follows:
 *
 * 1. Create a single level integer texture, with BASE_LEVEL and MAX_LEVEL set to 0.
 * 2. Leave the mipmap filters at the default of GL_NEAREST_MIPMAP_LINEAR and GL_LINEAR.
 * 3. Do glCopyImageSubData to or from that texture.
 * 4. Make sure it succeeds and does not raise GL_INVALID_OPERATION.
 **/
class IntegerTexTest : public deqp::TestCase
{
public:
	IntegerTexTest(deqp::Context& context, const char* name, glw::GLint internal_format, glw::GLuint type);

	virtual ~IntegerTexTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	GLuint createTexture(const GLvoid* data, GLint minFilter, GLint magFilter);
	bool verify(glw::GLuint texture_name, glw::GLuint sampler_type);
	void clean();

	/* Private fields */
	glw::GLuint m_dst_tex_name;
	glw::GLuint m_src_tex_name;
	glw::GLint  m_internal_format;
	glw::GLuint m_type;
};

/** Constructor
 *
 * @param context Text context
 **/
IntegerTexTest::IntegerTexTest(deqp::Context& context, const char* name, glw::GLint internal_format, glw::GLuint type)
	: TestCase(
		  context, name,
		  "Test verifies if INVALID_OPERATION is generated when texture provided to CopySubImageData is incomplete")
	, m_dst_tex_name(0)
	, m_src_tex_name(0)
	, m_internal_format(internal_format)
	, m_type(type)
{
}

/** Create texture
 *
 * @return Texture name
 **/
GLuint IntegerTexTest::createTexture(const GLvoid* data, GLint minFilter, GLint magFilter)
{
	const Functions& gl = m_context.getRenderContext().getFunctions();
	GLuint			 tex_name;

	gl.genTextures(1, &tex_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");
	gl.bindTexture(GL_TEXTURE_2D, tex_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
	gl.texImage2D(GL_TEXTURE_2D, 0, m_internal_format, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RED_INTEGER, m_type, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexImage2D");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	gl.bindTexture(GL_TEXTURE_2D, 0);

	return tex_name;
}

/** Execute test
 *
 * @return CONTINUE as long there are more test case, STOP otherwise
 **/
tcu::TestNode::IterateResult IntegerTexTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create destination and source textures */
	std::vector<int> data_buf(TEXTURE_WIDTH * TEXTURE_HEIGHT, 1);
	m_dst_tex_name = createTexture(&data_buf[0], GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR);
	std::fill(data_buf.begin(), data_buf.end(), 0);
	m_src_tex_name = createTexture(&data_buf[0], GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR);

	/* Execute CopyImageSubData */
	gl.copyImageSubData(m_src_tex_name, GL_TEXTURE_2D, 0 /* srcLevel */, 0 /* srcX */, 0 /* srcY */, 0 /* srcZ */,
						m_dst_tex_name, GL_TEXTURE_2D, 0 /* dstLevel */, 0 /* dstX */, 0 /* dstY */, 0 /* dstZ */,
						1 /* srcWidth */, 1 /* srcHeight */, 1 /* srcDepth */);

	GLenum error = gl.getError();
	if (error == GL_NO_ERROR)
	{
		/* Verify result */
		if (verify(m_dst_tex_name, m_type))
		{
			m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");
		}
		else
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Failure. Image data is not valid." << tcu::TestLog::EndMessage;

			m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}
	else
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Failure. Expected no error, got: " << glu::getErrorStr(error)
			<< ". Texture internal format: " << glu::getTextureFormatStr(m_internal_format) << tcu::TestLog::EndMessage;

		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Remove resources */
	clean();

	/* Done */
	return tcu::TestNode::STOP;
}

/** Verify result
 *
 **/
bool IntegerTexTest::verify(glw::GLuint texture_name, glw::GLuint sampler_type)
{
	static char const* vs = "${VERSION}\n"
							"in highp vec2 a_position;\n"
							"void main(void)\n"
							"{\n"
							"  gl_Position = vec4(a_position, 0.0, 1.0);\n"
							"}\n";

	static char const* fs = "${VERSION}\n"
							"uniform highp ${SAMPLER} u_texture;\n"
							"layout(location = 0) out highp vec4 o_color;\n"
							"void main(void)\n"
							"{\n"
							"   ivec2 coord = ivec2(gl_FragCoord.x, gl_FragCoord.y);\n"
							"   o_color = vec4(texelFetch(u_texture, coord, 0).r);\n"
							"}\n";

	glu::RenderContext& render_context = m_context.getRenderContext();
	glu::GLSLVersion	glsl_version   = glu::getContextTypeGLSLVersion(render_context.getType());
	const Functions&	gl			   = m_context.getRenderContext().getFunctions();

	std::map<std::string, std::string> specialization_map;
	specialization_map["VERSION"] = glu::getGLSLVersionDeclaration(glsl_version);
	specialization_map["SAMPLER"] = (sampler_type == GL_INT) ? "isampler2D" : "usampler2D";

	glu::ShaderProgram program(m_context.getRenderContext(),
							   glu::makeVtxFragSources(tcu::StringTemplate(vs).specialize(specialization_map).c_str(),
													   tcu::StringTemplate(fs).specialize(specialization_map).c_str()));

	if (!program.isOk())
	{
		m_testCtx.getLog() << program;
		TCU_FAIL("Compile failed");
	}

	static float const position[] = {
		-1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f,
	};

	gl.useProgram(program.getProgram());
	gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_texture"), 0);

	static const deUint16   quad_indices[]  = { 0, 1, 2, 2, 1, 3 };
	glu::VertexArrayBinding vertex_arrays[] = {
		glu::va::Float("a_position", 2, 4, 0, &position[0]),
	};

	GLuint rbo;
	gl.genRenderbuffers(1, &rbo);
	gl.bindRenderbuffer(GL_RENDERBUFFER, rbo);
	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT);

	GLuint fbo;
	gl.genFramebuffers(1, &fbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
	gl.viewport(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT);

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, texture_name);

	// make texture complete
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glu::draw(render_context, program.getProgram(), DE_LENGTH_OF_ARRAY(vertex_arrays), &vertex_arrays[0],
			  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(quad_indices), &quad_indices[0]));

	const unsigned int		   result_size = TEXTURE_WIDTH * TEXTURE_HEIGHT * 4;
	std::vector<unsigned char> result(result_size, 3);
	gl.readPixels(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, &result[0]);

	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.deleteFramebuffers(1, &fbo);

	gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
	gl.deleteRenderbuffers(1, &rbo);

	return ((std::count(result.begin(), result.begin() + 4, 0) == 4) &&
			(std::count(result.begin() + 4, result.end(), 255) == (result_size - 4)));
}

/** Cleans resources
 *
 **/
void IntegerTexTest::clean()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean textures and buffers. Errors ignored */
	gl.deleteTextures(1, &m_dst_tex_name);
	gl.deleteTextures(1, &m_src_tex_name);

	m_dst_tex_name = 0;
	m_src_tex_name = 0;
}

CopyImageTests::CopyImageTests(deqp::Context& context) : TestCaseGroup(context, "copy_image", "")
{
}

CopyImageTests::~CopyImageTests(void)
{
}

void CopyImageTests::init()
{
	addChild(new IntegerTexTest(m_context, "r32i_texture", GL_R32I, GL_INT));
	addChild(new IntegerTexTest(m_context, "r32ui_texture", GL_R32UI, GL_UNSIGNED_INT));
}

} /* namespace glcts */
