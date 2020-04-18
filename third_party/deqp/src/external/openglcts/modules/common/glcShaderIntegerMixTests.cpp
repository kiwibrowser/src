/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014 Intel Corporation
 * Copyright (c) 2016 The Khronos Group Inc.
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

#include "glcShaderIntegerMixTests.hpp"
#include "deMath.h"
#include "deRandom.hpp"
#include "deString.h"
#include "deStringUtil.hpp"
#include "gluContextInfo.hpp"
#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "glw.h"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

namespace deqp
{

using tcu::TestLog;

class ShaderIntegerMixCase : public TestCase
{
public:
	ShaderIntegerMixCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion)
		: TestCase(context, name, description), m_glslVersion(glslVersion)
	{
	}

	~ShaderIntegerMixCase()
	{
		// empty
	}

	IterateResult iterate()
	{
		qpTestResult result = test();

		m_testCtx.setTestResult(result, qpGetTestResultName(result));

		return STOP;
	}

protected:
	glu::GLSLVersion m_glslVersion;

	virtual qpTestResult test() = 0;
};

class ShaderIntegerMixDefineCase : public ShaderIntegerMixCase
{
public:
	ShaderIntegerMixDefineCase(Context& context, const char* name, const char* description,
							   glu::GLSLVersion glslVersion)
		: ShaderIntegerMixCase(context, name, description, glslVersion)
	{
		// empty
	}

	~ShaderIntegerMixDefineCase()
	{
		// empty
	}

protected:
	virtual qpTestResult test()
	{
		const glw::Functions& gl   = m_context.getRenderContext().getFunctions();
		bool				  pass = true;

		static const char source_template[] = "${VERSION_DECL}\n"
											  "#extension GL_EXT_shader_integer_mix: require\n"
											  "\n"
											  "#if !defined GL_EXT_shader_integer_mix\n"
											  "#  error GL_EXT_shader_integer_mix is not defined\n"
											  "#elif GL_EXT_shader_integer_mix != 1\n"
											  "#  error GL_EXT_shader_integer_mix is not equal to 1\n"
											  "#endif\n"
											  "\n"
											  "void main(void) { ${BODY} }\n";

		static const struct
		{
			GLenum		target;
			const char* body;
		} shader_targets[] = {
			{ GL_VERTEX_SHADER, "gl_Position = vec4(0);" }, { GL_FRAGMENT_SHADER, "" },
		};

		const glu::GLSLVersion v = glslVersionIsES(m_glslVersion) ? glu::GLSL_VERSION_300_ES : glu::GLSL_VERSION_330;

		if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_shader_integer_mix"))
			return QP_TEST_RESULT_NOT_SUPPORTED;

		for (int i = 0; i < DE_LENGTH_OF_ARRAY(shader_targets); i++)
		{
			std::map<std::string, std::string> args;

			args["VERSION_DECL"] = glu::getGLSLVersionDeclaration(v);
			args["BODY"]		 = shader_targets[i].body;

			std::string code = tcu::StringTemplate(source_template).specialize(args);

			GLuint		shader	 = gl.createShader(shader_targets[i].target);
			char const* strings[1] = { code.c_str() };
			gl.shaderSource(shader, 1, strings, 0);
			gl.compileShader(shader);

			GLint compileSuccess = 0;
			gl.getShaderiv(shader, GL_COMPILE_STATUS, &compileSuccess);
			gl.deleteShader(shader);

			if (!compileSuccess)
				pass = false;
		}

		return pass ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL;
	}
};

class ShaderIntegerMixPrototypesCase : public ShaderIntegerMixCase
{
public:
	ShaderIntegerMixPrototypesCase(Context& context, const char* name, const char* description,
								   glu::GLSLVersion glslVersion, bool _use_extension, bool _is_negative_testing)
		: ShaderIntegerMixCase(context, name, description, glslVersion)
		, use_extension(_use_extension)
		, is_negative_testing(_is_negative_testing)
	{
		// empty
	}

	~ShaderIntegerMixPrototypesCase()
	{
		// empty
	}

protected:
	bool use_extension;
	bool is_negative_testing;

	virtual qpTestResult test()
	{
		TestLog&			  log  = m_testCtx.getLog();
		const glw::Functions& gl   = m_context.getRenderContext().getFunctions();
		bool				  pass = true;

		static const char source_template[] = "${VERSION_DECL}\n"
											  "${EXTENSION_ENABLE}\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "	mix(ivec2(1), ivec2(2), bvec2(true));\n"
											  "	mix(ivec3(1), ivec3(2), bvec3(true));\n"
											  "	mix(ivec4(1), ivec4(2), bvec4(true));\n"
											  "	mix(uvec2(1), uvec2(2), bvec2(true));\n"
											  "	mix(uvec3(1), uvec3(2), bvec3(true));\n"
											  "	mix(uvec4(1), uvec4(2), bvec4(true));\n"
											  "	mix(bvec2(1), bvec2(0), bvec2(true));\n"
											  "	mix(bvec3(1), bvec3(0), bvec3(true));\n"
											  "	mix(bvec4(1), bvec4(0), bvec4(true));\n"
											  "	${BODY}\n"
											  "}\n";

		static const struct
		{
			GLenum		target;
			const char* body;
		} shader_targets[] = {
			{ GL_VERTEX_SHADER, "gl_Position = vec4(0);" }, { GL_FRAGMENT_SHADER, "" },
		};

		glu::GLSLVersion v;
		const char*		 extension_enable;

		if (use_extension)
		{
			v				 = glslVersionIsES(m_glslVersion) ? glu::GLSL_VERSION_300_ES : glu::GLSL_VERSION_330;
			extension_enable = "#extension GL_EXT_shader_integer_mix: enable";

			if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_shader_integer_mix"))
				return QP_TEST_RESULT_NOT_SUPPORTED;
		}
		else if (is_negative_testing)
		{
			v				 = glslVersionIsES(m_glslVersion) ? glu::GLSL_VERSION_300_ES : glu::GLSL_VERSION_330;
			extension_enable = "";
		}
		else
		{
			v				 = m_glslVersion;
			extension_enable = "";
			if (glslVersionIsES(m_glslVersion))
			{
				if (m_glslVersion < glu::GLSL_VERSION_310_ES)
					return QP_TEST_RESULT_NOT_SUPPORTED;
			}
			else
			{
				if (m_glslVersion < glu::GLSL_VERSION_450)
					return QP_TEST_RESULT_NOT_SUPPORTED;
			}
		}

		for (int i = 0; i < DE_LENGTH_OF_ARRAY(shader_targets); i++)
		{
			std::map<std::string, std::string> args;

			args["VERSION_DECL"]	 = glu::getGLSLVersionDeclaration(v);
			args["EXTENSION_ENABLE"] = extension_enable;
			args["BODY"]			 = shader_targets[i].body;

			std::string code = tcu::StringTemplate(source_template).specialize(args);

			GLuint		shader	 = gl.createShader(shader_targets[i].target);
			char const* strings[1] = { code.c_str() };
			gl.shaderSource(shader, 1, strings, 0);
			gl.compileShader(shader);

			GLint compileSuccess = 0;
			gl.getShaderiv(shader, GL_COMPILE_STATUS, &compileSuccess);

			if (is_negative_testing)
			{
				if (compileSuccess)
				{
					TCU_FAIL("The shader compilation was expected to fail, but it was successful.");
					pass = false;
				}
			}
			else if (!compileSuccess)
			{
				GLchar infoLog[1000];

				gl.getShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
				log.writeKernelSource(strings[0]);
				log.writeCompileInfo("shader", "", false, infoLog);

				pass = false;
			}

			gl.deleteShader(shader);
		}

		return pass ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL;
	}
};

class ShaderIntegerMixRenderCase : public ShaderIntegerMixCase
{
public:
	ShaderIntegerMixRenderCase(Context& context, const char* name, const char* description,
							   glu::GLSLVersion glslVersion, const char* _type)
		: ShaderIntegerMixCase(context, name, description, glslVersion), type(_type)
	{
		// empty
	}

	~ShaderIntegerMixRenderCase()
	{
		// empty
	}

protected:
	// Type used for mix() parameters in this test case.
	const char* type;

	static const unsigned width  = 8 * 8;
	static const unsigned height = 8 * 8;

	virtual qpTestResult test()
	{
		static const char vs_template[] = "${VERSION_DECL}\n"
										  "${EXTENSION_ENABLE}\n"
										  "\n"
										  "in vec2 vertex;\n"
										  "in ivec4 vs_in_a;\n"
										  "in ivec4 vs_in_b;\n"
										  "in ivec4 vs_in_sel;\n"
										  "\n"
										  "flat out ivec4 fs_in_a;\n"
										  "flat out ivec4 fs_in_b;\n"
										  "flat out ivec4 fs_in_sel;\n"
										  "flat out ivec4 fs_in_result;\n"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    fs_in_a = vs_in_a;\n"
										  "    fs_in_b = vs_in_b;\n"
										  "    fs_in_sel = vs_in_sel;\n"
										  "\n"
										  "    ${TYPE} a = ${TYPE}(vs_in_a);\n"
										  "    ${TYPE} b = ${TYPE}(vs_in_b);\n"
										  "    bvec4 sel = bvec4(vs_in_sel);\n"
										  "    fs_in_result = ivec4(mix(a, b, sel));\n"
										  "\n"
										  "    gl_Position = vec4(vertex, 0, 1);\n"
										  "    gl_PointSize = 4.;\n"
										  "}\n";

		static const char fs_template[] = "${VERSION_DECL}\n"
										  "${EXTENSION_ENABLE}\n"
										  "\n"
										  "out ivec4 o;\n"
										  "\n"
										  "flat in ivec4 fs_in_a;\n"
										  "flat in ivec4 fs_in_b;\n"
										  "flat in ivec4 fs_in_sel;\n"
										  "flat in ivec4 fs_in_result;\n"
										  "\n"
										  "uniform bool use_vs_data;\n"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    if (use_vs_data)\n"
										  "        o = fs_in_result;\n"
										  "    else {\n"
										  "        ${TYPE} a = ${TYPE}(fs_in_a);\n"
										  "        ${TYPE} b = ${TYPE}(fs_in_b);\n"
										  "        bvec4 sel = bvec4(fs_in_sel);\n"
										  "        o = ivec4(mix(a, b, sel));\n"
										  "    }\n"
										  "}\n";

		TestLog&			  log  = m_testCtx.getLog();
		const glw::Functions& gl   = m_context.getRenderContext().getFunctions();
		bool				  pass = true;
		const char*			  extension_enable;
		bool				  is_es = glslVersionIsES(m_glslVersion);

		if ((is_es && (m_glslVersion < glu::GLSL_VERSION_310_ES)) || !is_es)
		{
			/* For versions that do not support this feature in Core it must be exposed via an extension. */
			if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_shader_integer_mix"))
			{
				return QP_TEST_RESULT_NOT_SUPPORTED;
			}

			extension_enable = "#extension GL_EXT_shader_integer_mix: enable";
		}
		else
		{
			extension_enable = "";
		}

		/* Generate the specialization of the shader for the specific
		 * type being tested.
		 */
		std::map<std::string, std::string> args;

		args["VERSION_DECL"]	 = glu::getGLSLVersionDeclaration(m_glslVersion);
		args["EXTENSION_ENABLE"] = extension_enable;
		args["TYPE"]			 = type;

		std::string vs_code = tcu::StringTemplate(vs_template).specialize(args);

		std::string fs_code = tcu::StringTemplate(fs_template).specialize(args);

		glu::ShaderProgram prog(m_context.getRenderContext(),
								glu::makeVtxFragSources(vs_code.c_str(), fs_code.c_str()));

		if (!prog.isOk())
		{
			log << prog;
			TCU_FAIL("Compile failed");
		}

		if (!glslVersionIsES(m_glslVersion))
			glEnable(GL_PROGRAM_POINT_SIZE);

		/* Generate an integer FBO for rendering.
		 */
		GLuint fbo;
		GLuint tex;

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0 /* level */, GL_RGBA32I, width, height, 0 /* border */, GL_RGBA_INTEGER, GL_INT,
					 NULL /* data */);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0 /* level */);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Creation of rendering FBO failed.");

		if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			TCU_FAIL("Framebuffer not complete.");

		glViewport(0, 0, width, height);

		/* Fill a VBO with some vertex data.
		 */
		deUint32   pointIndices[256];
		float	  vertex[DE_LENGTH_OF_ARRAY(pointIndices) * 2];
		deInt32	a[DE_LENGTH_OF_ARRAY(pointIndices) * 4];
		deInt32	b[DE_LENGTH_OF_ARRAY(a)];
		deInt32	sel[DE_LENGTH_OF_ARRAY(a)];
		tcu::IVec4 expected[DE_LENGTH_OF_ARRAY(pointIndices)];

		for (int i = 0; i < DE_LENGTH_OF_ARRAY(pointIndices); i++)
		{
			pointIndices[i] = deUint16(i);

			const int x = (i / 16) * (width / 16) + (4 / 2);
			const int y = (i % 16) * (height / 16) + (4 / 2);

			vertex[(i * 2) + 0] = float(x) * 2.0f / float(width) - 1.0f;
			vertex[(i * 2) + 1] = float(y) * 2.0f / float(height) - 1.0f;

			a[(i * 4) + 0] = i;
			a[(i * 4) + 1] = i * 5;
			a[(i * 4) + 2] = i * 7;
			a[(i * 4) + 3] = i * 11;

			b[(i * 4) + 0] = ~a[(i * 4) + 3];
			b[(i * 4) + 1] = ~a[(i * 4) + 2];
			b[(i * 4) + 2] = ~a[(i * 4) + 1];
			b[(i * 4) + 3] = ~a[(i * 4) + 0];

			sel[(i * 4) + 0] = (i >> 0) & 1;
			sel[(i * 4) + 1] = (i >> 1) & 1;
			sel[(i * 4) + 2] = (i >> 2) & 1;
			sel[(i * 4) + 3] = (i >> 3) & 1;

			expected[i] = tcu::IVec4(
				sel[(i * 4) + 0] ? b[(i * 4) + 0] : a[(i * 4) + 0], sel[(i * 4) + 1] ? b[(i * 4) + 1] : a[(i * 4) + 1],
				sel[(i * 4) + 2] ? b[(i * 4) + 2] : a[(i * 4) + 2], sel[(i * 4) + 3] ? b[(i * 4) + 3] : a[(i * 4) + 3]);
		}

		/* Mask off all but the least significant bit for boolean
		 * types.
		 */
		if (type[0] == 'b')
		{
			for (int i = 0; i < DE_LENGTH_OF_ARRAY(a); i++)
			{
				a[i] &= 1;
				b[i] &= 1;

				expected[i / 4][0] &= 1;
				expected[i / 4][1] &= 1;
				expected[i / 4][2] &= 1;
				expected[i / 4][3] &= 1;
			}
		}

		glu::VertexArrayBinding vertexArrays[] = {
			glu::va::Float("vertex", 2, DE_LENGTH_OF_ARRAY(pointIndices), 0, vertex),
			glu::va::Int32("vs_in_a", 4, DE_LENGTH_OF_ARRAY(pointIndices), 0, a),
			glu::va::Int32("vs_in_b", 4, DE_LENGTH_OF_ARRAY(pointIndices), 0, b),
			glu::va::Int32("vs_in_sel", 4, DE_LENGTH_OF_ARRAY(pointIndices), 0, sel)
		};

		/* Render and verify the results.  Rendering happens twice.
		 * The first time, use_vs_data is false, and the mix() result
		 * from the fragment shader is used.  The second time,
		 * use_vs_data is true, and the mix() result from the vertex
		 * shader is used.
		 */
		const GLint loc = gl.getUniformLocation(prog.getProgram(), "use_vs_data");
		gl.useProgram(prog.getProgram());

		static const GLint clear[] = { 1, 2, 3, 4 };
		glClearBufferiv(GL_COLOR, 0, clear);

		gl.uniform1i(loc, 0);
		glu::draw(m_context.getRenderContext(), prog.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
				  glu::pr::Points(DE_LENGTH_OF_ARRAY(pointIndices), pointIndices));

		for (int i = 0; i < DE_LENGTH_OF_ARRAY(pointIndices); i++)
		{
			const int x = int((vertex[(i * 2) + 0] + 1.0f) * float(width) / 2.0f);
			const int y = int((vertex[(i * 2) + 1] + 1.0f) * float(height) / 2.0f);

			pass = probe_pixel(log, "Fragment", x, y, expected[i]) && pass;
		}

		gl.uniform1i(loc, 1);
		glu::draw(m_context.getRenderContext(), prog.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
				  glu::pr::Points(DE_LENGTH_OF_ARRAY(pointIndices), pointIndices));

		for (int i = 0; i < DE_LENGTH_OF_ARRAY(pointIndices); i++)
		{
			const int x = int((vertex[(i * 2) + 0] + 1.0f) * float(width) / 2.0f);
			const int y = int((vertex[(i * 2) + 1] + 1.0f) * float(height) / 2.0f);

			pass = probe_pixel(log, "Vertex", x, y, expected[i]) && pass;
		}

		glDeleteFramebuffers(1, &fbo);
		glDeleteTextures(1, &tex);

		return pass ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL;
	}

	bool probe_pixel(TestLog& log, const char* stage, int x, int y, const tcu::IVec4& expected)
	{
		tcu::IVec4 pixel;

		glReadPixels(x, y, 1, 1, GL_RGBA_INTEGER, GL_INT, &pixel);

		if (expected != pixel)
		{
			log << TestLog::Message << stage << " shader failed at pixel (" << x << ", " << y << ").  "
				<< "Got " << pixel << ", expected " << expected << ")." << TestLog::EndMessage;
			return false;
		}

		return true;
	}
};

ShaderIntegerMixTests::ShaderIntegerMixTests(Context& context, glu::GLSLVersion glslVersion)
	: TestCaseGroup(context, "shader_integer_mix", "Shader Integer Mix tests"), m_glslVersion(glslVersion)
{
	// empty
}

ShaderIntegerMixTests::~ShaderIntegerMixTests()
{
	// empty
}

void ShaderIntegerMixTests::init(void)
{
	addChild(new ShaderIntegerMixDefineCase(m_context, "define", "Verify GL_EXT_shader_integer_mix is defined to 1.",
											m_glslVersion));
	addChild(new ShaderIntegerMixPrototypesCase(m_context, "prototypes-extension",
												"Verify availability of all function signatures with the extension.",
												m_glslVersion, true, false));
	addChild(new ShaderIntegerMixPrototypesCase(
		m_context, "prototypes", "Verify availability of all function signatures with the proper GLSL version.",
		m_glslVersion, false, false));
	addChild(new ShaderIntegerMixPrototypesCase(
		m_context, "prototypes-negative",
		"Verify compilation fails if the GLSL version does not support shader_integer_mix", m_glslVersion, false,
		true));

	static const char* types_to_test[] = { "ivec4", "uvec4", "bvec4" };

	for (int i = 0; i < DE_LENGTH_OF_ARRAY(types_to_test); i++)
	{
		std::stringstream name;

		name << "mix-" << types_to_test[i];

		std::stringstream description;

		description << "Verify functionality of mix() with " << types_to_test[i] << " parameters.";

		addChild(new ShaderIntegerMixRenderCase(m_context, name.str().c_str(), description.str().c_str(), m_glslVersion,
												types_to_test[i]));
	}
}

} // namespace deqp
