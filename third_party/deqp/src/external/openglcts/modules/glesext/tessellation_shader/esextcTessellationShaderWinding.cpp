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
 * \file esextcTessellationShaderWinding.cpp
 * \brief Test winding order with tessellation shaders
 */ /*-------------------------------------------------------------------*/

#include "esextcTessellationShaderWinding.hpp"
#include "deSharedPtr.hpp"
#include "esextcTessellationShaderUtils.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRGBA.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include <string>

namespace glcts
{

class WindingCase : public TestCaseBase
{
public:
	WindingCase(glcts::Context& context, const ExtParameters& extParams, std::string name, std::string primitiveType,
				std::string winding);

	void		  init(void);
	void		  deinit(void);
	IterateResult iterate(void);
	void prepareFramebuffer();

private:
	static const int RENDER_SIZE = 64;

	de::SharedPtr<const glu::ShaderProgram> m_program;
	glw::GLuint m_rbo;
	glw::GLuint m_fbo;
};

WindingCase::WindingCase(glcts::Context& context, const ExtParameters& extParams, std::string name,
						 std::string primitiveType, std::string winding)
	: TestCaseBase(context, extParams, name.c_str(), "")
{
	DE_ASSERT((primitiveType.compare("triangles") == 0) || (primitiveType.compare("quads") == 0));
	DE_ASSERT((winding.compare("cw") == 0) || (winding.compare("ccw") == 0));

	m_specializationMap["PRIMITIVE_TYPE"] = primitiveType;
	m_specializationMap["WINDING"]        = winding;
	m_rbo                                 = 0;
	m_fbo                                 = 0;
}

void WindingCase::init(void)
{
	TestCaseBase::init();
	if (!m_is_tessellation_shader_supported)
	{
		TCU_THROW(NotSupportedError, TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	TestCaseBase::init();

	const char* vs("${VERSION}\n"
				   "void main (void)\n"
				   "{\n"
				   "}\n");
	const char* tcs("${VERSION}\n"
					"${TESSELLATION_SHADER_REQUIRE}\n"
					"layout (vertices = 1) out;\n"
					"void main (void)\n"
					"{\n"
					"	gl_TessLevelInner[0] = 5.0;\n"
					"	gl_TessLevelInner[1] = 5.0;\n"
					"\n"
					"	gl_TessLevelOuter[0] = 5.0;\n"
					"	gl_TessLevelOuter[1] = 5.0;\n"
					"	gl_TessLevelOuter[2] = 5.0;\n"
					"	gl_TessLevelOuter[3] = 5.0;\n"
					"}\n");
	const char* tes("${VERSION}\n"
					"${TESSELLATION_SHADER_REQUIRE}\n"
					"layout (${PRIMITIVE_TYPE}, ${WINDING}) in;\n"
					"void main (void)\n"
					"{\n"
					"	gl_Position = vec4(gl_TessCoord.xy*2.0 - 1.0, 0.0, 1.0);\n"
					"}\n");
	const char* fs("${VERSION}\n"
				   "layout (location = 0) out mediump vec4 o_color;\n"
				   "void main (void)\n"
				   "{\n"
				   "	o_color = vec4(1.0);\n"
				   "}\n");

	m_program = de::SharedPtr<const glu::ShaderProgram>(
		new glu::ShaderProgram(m_context.getRenderContext(),
							   glu::ProgramSources() << glu::VertexSource(specializeShader(1, &vs))
													 << glu::TessellationControlSource(specializeShader(1, &tcs))
													 << glu::TessellationEvaluationSource(specializeShader(1, &tes))
													 << glu::FragmentSource(specializeShader(1, &fs))));

	m_testCtx.getLog() << *m_program;
	if (!m_program->isOk())
		TCU_FAIL("Program compilation failed");
}

void WindingCase::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);
		m_fbo = 0;
	}

	if (m_rbo)
	{
		gl.deleteRenderbuffers(1, &m_rbo);
		m_rbo = 0;
	}

	m_program.clear();
}

/** @brief Bind default framebuffer object.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void WindingCase::prepareFramebuffer()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genRenderbuffers(1, &m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, RENDER_SIZE, RENDER_SIZE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");
}

WindingCase::IterateResult WindingCase::iterate(void)
{
	const glu::RenderContext& renderCtx = m_context.getRenderContext();
	const deUint32			  programGL = m_program->getProgram();
	const glw::Functions&	 gl		= renderCtx.getFunctions();

	const unsigned int windingTaken[2]	 = { GL_CW, GL_CCW };
	const char*		   windingTakenName[2] = { "GL_CW", "GL_CCW" };

	const bool testPrimitiveTypeIsTriangles = (m_specializationMap["PRIMITIVE_TYPE"].compare("triangles") == 0);
	const bool testWindingIsCW				= (m_specializationMap["WINDING"].compare("cw") == 0);
	bool	   success						= true;

	prepareFramebuffer();
	gl.viewport(0, 0, RENDER_SIZE, RENDER_SIZE);
	gl.clearColor(1.0f, 0.0f, 0.0f, 1.0f);
	gl.useProgram(programGL);

	gl.patchParameteri(GL_PATCH_VERTICES, 1);

	gl.enable(GL_CULL_FACE);

	deUint32 vaoGL;
	gl.genVertexArrays(1, &vaoGL);
	gl.bindVertexArray(vaoGL);

	m_testCtx.getLog() << tcu::TestLog::Message << "Face culling enabled" << tcu::TestLog::EndMessage;

	for (int windingIndex = 0; windingIndex < 2; windingIndex++)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Setting glFrontFace(" << windingTakenName[windingIndex] << ")"
						   << tcu::TestLog::EndMessage;

		gl.frontFace(windingTaken[windingIndex]);

		gl.clear(GL_COLOR_BUFFER_BIT);
		gl.drawArrays(GL_PATCHES, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw failed");

		{
			tcu::Surface rendered(RENDER_SIZE, RENDER_SIZE);
			glu::readPixels(renderCtx, 0, 0, rendered.getAccess());
			m_testCtx.getLog() << tcu::TestLog::Image("RenderedImage", "Rendered Image", rendered);

			{
				const int badPixelTolerance =
					testPrimitiveTypeIsTriangles ? 5 * de::max(rendered.getWidth(), rendered.getHeight()) : 0;
				const int totalNumPixels = rendered.getWidth() * rendered.getHeight();

				int numWhitePixels = 0;
				int numRedPixels   = 0;
				for (int y = 0; y < rendered.getHeight(); y++)
					for (int x = 0; x < rendered.getWidth(); x++)
					{
						numWhitePixels += rendered.getPixel(x, y) == tcu::RGBA::white() ? 1 : 0;
						numRedPixels += rendered.getPixel(x, y) == tcu::RGBA::red() ? 1 : 0;
					}

				DE_ASSERT(numWhitePixels + numRedPixels <= totalNumPixels);

				m_testCtx.getLog() << tcu::TestLog::Message << "Note: got " << numWhitePixels << " white and "
								   << numRedPixels << " red pixels" << tcu::TestLog::EndMessage;

				if (totalNumPixels - numWhitePixels - numRedPixels > badPixelTolerance)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Failure: Got "
									   << totalNumPixels - numWhitePixels - numRedPixels
									   << " other than white or red pixels (maximum tolerance " << badPixelTolerance
									   << ")" << tcu::TestLog::EndMessage;
					success = false;
					break;
				}

				bool frontFaceWindingIsCW = (windingIndex == 0);
				if (frontFaceWindingIsCW == testWindingIsCW)
				{
					if (testPrimitiveTypeIsTriangles)
					{
						if (de::abs(numWhitePixels - totalNumPixels / 2) > badPixelTolerance)
						{
							m_testCtx.getLog() << tcu::TestLog::Message
											   << "Failure: wrong number of white pixels; expected approximately "
											   << totalNumPixels / 2 << tcu::TestLog::EndMessage;
							success = false;
							break;
						}
					}
					else // test primitive type is quads
					{
						if (numWhitePixels != totalNumPixels)
						{
							m_testCtx.getLog()
								<< tcu::TestLog::Message << "Failure: expected only white pixels (full-viewport quad)"
								<< tcu::TestLog::EndMessage;
							success = false;
							break;
						}
					}
				}
				else
				{
					if (numWhitePixels != 0)
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message << "Failure: expected only red pixels (everything culled)"
							<< tcu::TestLog::EndMessage;
						success = false;
						break;
					}
				}
			}
		}
	}

	gl.bindVertexArray(0);
	gl.deleteVertexArrays(1, &vaoGL);

	m_testCtx.setTestResult(success ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL,
							success ? "Pass" : "Image verification failed");
	return STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
TesselationShaderWindingTests::TesselationShaderWindingTests(glcts::Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "winding", "Verifies winding order with tessellation shaders")
{
}

/**
 * Initializes test groups for winding tests
 **/
void TesselationShaderWindingTests::init(void)
{
	addChild(new WindingCase(m_context, m_extParams, "triangles_ccw", "triangles", "ccw"));
	addChild(new WindingCase(m_context, m_extParams, "triangles_cw", "triangles", "cw"));
	addChild(new WindingCase(m_context, m_extParams, "quads_ccw", "quads", "ccw"));
	addChild(new WindingCase(m_context, m_extParams, "quads_cw", "quads", "cw"));
}

} /* namespace glcts */
