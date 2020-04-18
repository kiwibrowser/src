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

/**
 */ /*!
 * \file  es31cTextureStorageMultisampleGetActiveUniformTests.cpp
 * \brief Implements conformance tests that check whether glGetActiveUniform()
 *        works correctly with multisample texture samplers. (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleGetActiveUniformTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <string.h>
#include <string>
#include <vector>

namespace glcts
{

/* Constants */
const char* MultisampleTextureGetActiveUniformSamplersTest::fs_body =
	"#version 310 es\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"uniform highp sampler2DMS  fs_sampler_2d_multisample;\n"
	"uniform highp usampler2DMS fs_sampler_2d_multisample_uint;\n"
	"uniform highp isampler2DMS fs_sampler_2d_multisample_int;\n"
	"\n"
	"out vec4 result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    vec4  sampler2DMS_value  = texelFetch(fs_sampler_2d_multisample,      ivec2(0), 0);\n"
	"    uvec4 usampler2DMS_value = texelFetch(fs_sampler_2d_multisample_uint, ivec2(0), 0);\n"
	"    ivec4 isampler2DMS_value = texelFetch(fs_sampler_2d_multisample_int,  ivec2(0), 0);\n"
	"\n"
	"    result  =       sampler2DMS_value  +\n"
	"              vec4(usampler2DMS_value) +\n"
	"              vec4(isampler2DMS_value);\n"
	"}\n";

const char* MultisampleTextureGetActiveUniformSamplersTest::fs_body_oes =
	"#version 310 es\n"
	"\n"
	"#extension GL_OES_texture_storage_multisample_2d_array : enable\n"
	"precision highp float;\n"
	"\n"
	"uniform highp sampler2DMS       fs_sampler_2d_multisample;\n"
	"uniform highp sampler2DMSArray  fs_sampler_2d_multisample_array;\n"
	"uniform highp usampler2DMS      fs_sampler_2d_multisample_uint;\n"
	"uniform highp usampler2DMSArray fs_sampler_2d_multisample_array_uint;\n"
	"uniform highp isampler2DMS      fs_sampler_2d_multisample_int;\n"
	"uniform highp isampler2DMSArray fs_sampler_2d_multisample_array_int;\n"
	"\n"
	"out vec4 result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    vec4  sampler2DMS_value       = texelFetch(fs_sampler_2d_multisample,            ivec2(0), 0);\n"
	"    vec4  sampler2DMSArray_value  = texelFetch(fs_sampler_2d_multisample_array,      ivec3(0), 0);\n"
	"    uvec4 usampler2DMS_value      = texelFetch(fs_sampler_2d_multisample_uint,       ivec2(0), 0);\n"
	"    uvec4 usampler2DMSArray_value = texelFetch(fs_sampler_2d_multisample_array_uint, ivec3(0), 0);\n"
	"    ivec4 isampler2DMS_value      = texelFetch(fs_sampler_2d_multisample_int,        ivec2(0), 0);\n"
	"    ivec4 isampler2DMSArray_value = texelFetch(fs_sampler_2d_multisample_array_int,  ivec3(0), 0);\n"
	"\n"
	"    result  =       sampler2DMS_value  +       sampler2DMSArray_value  +\n"
	"              vec4(usampler2DMS_value) + vec4(usampler2DMSArray_value) +\n"
	"              vec4(isampler2DMS_value) + vec4(isampler2DMSArray_value);\n"
	"}\n";

const char* MultisampleTextureGetActiveUniformSamplersTest::vs_body =
	"#version 310 es\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"uniform highp sampler2DMS  vs_sampler_2d_multisample;\n"
	"uniform highp usampler2DMS vs_sampler_2d_multisample_uint;\n"
	"uniform highp isampler2DMS vs_sampler_2d_multisample_int;\n"
	"\n"
	"out vec4 result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    vec4  sampler2DMS_value  = texelFetch(vs_sampler_2d_multisample,      ivec2(0), 0);\n"
	"    uvec4 usampler2DMS_value = texelFetch(vs_sampler_2d_multisample_uint, ivec2(0), 0);\n"
	"    ivec4 isampler2DMS_value = texelFetch(vs_sampler_2d_multisample_int,  ivec2(0), 0);\n"
	"\n"
	"    gl_Position =       sampler2DMS_value  +\n"
	"                  vec4(usampler2DMS_value) +\n"
	"                  vec4(isampler2DMS_value);\n"
	"}\n";

const char* MultisampleTextureGetActiveUniformSamplersTest::vs_body_oes =
	"#version 310 es\n"
	"\n"
	"#extension GL_OES_texture_storage_multisample_2d_array : enable\n"
	"precision highp float;\n"
	"\n"
	"uniform highp sampler2DMS       vs_sampler_2d_multisample;\n"
	"uniform highp sampler2DMSArray  vs_sampler_2d_multisample_array;\n"
	"uniform highp usampler2DMS      vs_sampler_2d_multisample_uint;\n"
	"uniform highp usampler2DMSArray vs_sampler_2d_multisample_array_uint;\n"
	"uniform highp isampler2DMS      vs_sampler_2d_multisample_int;\n"
	"uniform highp isampler2DMSArray vs_sampler_2d_multisample_array_int;\n"
	"\n"
	"out vec4 result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    vec4  sampler2DMS_value       = texelFetch(vs_sampler_2d_multisample,            ivec2(0), 0);\n"
	"    vec4  sampler2DMSArray_value  = texelFetch(vs_sampler_2d_multisample_array,      ivec3(0), 0);\n"
	"    uvec4 usampler2DMS_value      = texelFetch(vs_sampler_2d_multisample_uint,       ivec2(0), 0);\n"
	"    uvec4 usampler2DMSArray_value = texelFetch(vs_sampler_2d_multisample_array_uint, ivec3(0), 0);\n"
	"    ivec4 isampler2DMS_value      = texelFetch(vs_sampler_2d_multisample_int,        ivec2(0), 0);\n"
	"    ivec4 isampler2DMSArray_value = texelFetch(vs_sampler_2d_multisample_array_int,  ivec3(0), 0);\n"
	"\n"
	"    gl_Position =       sampler2DMS_value  +       sampler2DMSArray_value  +\n"
	"                  vec4(usampler2DMS_value) + vec4(usampler2DMSArray_value) +\n"
	"                  vec4(isampler2DMS_value) + vec4(isampler2DMSArray_value);\n"
	"}\n";

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureGetActiveUniformSamplersTest::MultisampleTextureGetActiveUniformSamplersTest(Context& context)
	: TestCase(context, "multisample_texture_samplers", "Verifies multisample texture samplers are reported correctly")
	, fs_id(0)
	, gl_oes_texture_storage_multisample_2d_array_supported(GL_FALSE)
	, po_id(0)
	, vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureGetActiveUniformSamplersTest::deinit()
{
	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes test-specific ES objects */
void MultisampleTextureGetActiveUniformSamplersTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	po_id = gl.createProgram();
	vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Shader or program creation failed");
}

/** Removes test-specific ES objects */
void MultisampleTextureGetActiveUniformSamplersTest::deinitInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.deleteShader(fs_id);
	gl.deleteProgram(po_id);
	gl.deleteShader(vs_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Shader or program delete failed");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult MultisampleTextureGetActiveUniformSamplersTest::iterate()
{
	gl_oes_texture_storage_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Configure the test program object */
	gl.attachShader(po_id, fs_id);
	gl.attachShader(po_id, vs_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure the test program object");

	/* Compile the fragment shader */
	glw::GLint compile_status = GL_FALSE;

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		gl.shaderSource(fs_id, 1,			 /* count */
						&fs_body_oes, NULL); /* length */
	}
	else
	{
		gl.shaderSource(fs_id, 1,		 /* count */
						&fs_body, NULL); /* length */
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	gl.compileShader(fs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(fs_id, GL_COMPILE_STATUS, &compile_status);

	if (compile_status != GL_TRUE)
	{
		TCU_FAIL("Fragment shader compilation failed.");
	}

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		/* Compile the vertex shader */
		gl.shaderSource(vs_id, 1,			 /* count */
						&vs_body_oes, NULL); /* length */
	}
	else
	{
		/* Compile the vertex shader */
		gl.shaderSource(vs_id, 1,		 /* count */
						&vs_body, NULL); /* length */
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	gl.compileShader(vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(vs_id, GL_COMPILE_STATUS, &compile_status);

	if (compile_status != GL_TRUE)
	{
		char temp[1024];

		gl.getShaderInfoLog(vs_id, 1024, 0, temp);

		TCU_FAIL("Vertex shader compilation failed.");
	}

	/* Link the test program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(po_id, GL_LINK_STATUS, &link_status);

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed.");
	}

	/* Retrieve amount of active uniforms */
	glw::GLint n_active_uniforms = 0;

	gl.getProgramiv(po_id, GL_ACTIVE_UNIFORMS, &n_active_uniforms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call (GL_ACTIVE_UNIFORMS) failed.");

	/* Allocate a buffer that will hold uniform names */
	glw::GLint max_active_uniform_length = 0;
	char*	  uniform_name				 = NULL;

	gl.getProgramiv(po_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_active_uniform_length);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call (GL_ACTIVE_UNIFORM_MAX_LENGTH) failed.");

	uniform_name = new char[max_active_uniform_length];

	/* Prepare an array of booleans. Each cell, set to false by default, will tell
	 * whether a corresponding uniform has already been reported.
	 */
	enum
	{
		SHADER_UNIFORM_FS_MULTISAMPLE,
		SHADER_UNIFORM_FS_MULTISAMPLE_ARRAY,
		SHADER_UNIFORM_FS_MULTISAMPLE_UINT,
		SHADER_UNIFORM_FS_MULTISAMPLE_ARRAY_UINT,
		SHADER_UNIFORM_FS_MULTISAMPLE_INT,
		SHADER_UNIFORM_FS_MULTISAMPLE_ARRAY_INT,

		SHADER_UNIFORM_VS_MULTISAMPLE,
		SHADER_UNIFORM_VS_MULTISAMPLE_ARRAY,
		SHADER_UNIFORM_VS_MULTISAMPLE_UINT,
		SHADER_UNIFORM_VS_MULTISAMPLE_ARRAY_UINT,
		SHADER_UNIFORM_VS_MULTISAMPLE_INT,
		SHADER_UNIFORM_VS_MULTISAMPLE_ARRAY_INT,

		SHADER_UNIFORM_COUNT
	};

	bool shader_uniform_reported_status[SHADER_UNIFORM_COUNT];

	memset(shader_uniform_reported_status, 0, sizeof(shader_uniform_reported_status));

	/* Iterate through all active uniforms. */
	for (int n_uniform = 0; n_uniform < n_active_uniforms; ++n_uniform)
	{
		glw::GLint  uniform_size = 0;
		glw::GLenum uniform_type = GL_NONE;

		/* Retrieve uniform properties */
		gl.getActiveUniform(po_id, n_uniform, max_active_uniform_length, NULL, /* length */
							&uniform_size, &uniform_type, uniform_name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetActiveUniform() call failed");

		/* Check if the reported name is valid and that the type and size
		 * retrieved matches the uniform.
		 * Also verify that the uniform has not been already reported.
		 */
		if (strcmp(uniform_name, "fs_sampler_2d_multisample") == 0)
		{
			if (shader_uniform_reported_status[SHADER_UNIFORM_FS_MULTISAMPLE])
			{
				TCU_FAIL("fs_sampler_2d_multisample uniform is reported more than once");
			}

			if (uniform_type != GL_SAMPLER_2D_MULTISAMPLE)
			{
				TCU_FAIL("Invalid uniform type reported for fs_sampler_2d_multisample uniform");
			}

			shader_uniform_reported_status[SHADER_UNIFORM_FS_MULTISAMPLE] = true;
		}
		else if (strcmp(uniform_name, "fs_sampler_2d_multisample_array") == 0)
		{
			if (gl_oes_texture_storage_multisample_2d_array_supported)
			{
				if (shader_uniform_reported_status[SHADER_UNIFORM_FS_MULTISAMPLE_ARRAY])
				{
					TCU_FAIL("fs_sampler_2d_multisample_array uniform is reported more than once");
				}

				if (uniform_type != GL_SAMPLER_2D_MULTISAMPLE_ARRAY_OES)
				{
					TCU_FAIL("Invalid uniform type reported for fs_sampler_2d_multisample_array uniform");
				}

				shader_uniform_reported_status[SHADER_UNIFORM_FS_MULTISAMPLE_ARRAY] = true;
			}
			else
			{
				TCU_FAIL("Unsupported active uniform type reported.");
			}
		}
		else if (strcmp(uniform_name, "fs_sampler_2d_multisample_uint") == 0)
		{
			if (shader_uniform_reported_status[SHADER_UNIFORM_FS_MULTISAMPLE_UINT])
			{
				TCU_FAIL("fs_sampler_2d_multisample_uint uniform is reported more than once");
			}

			if (uniform_type != GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE)
			{
				TCU_FAIL("Invalid uniform type reported for fs_sampler_2d_multisample_uint uniform");
			}

			shader_uniform_reported_status[SHADER_UNIFORM_FS_MULTISAMPLE_UINT] = true;
		}
		else if (strcmp(uniform_name, "fs_sampler_2d_multisample_array_uint") == 0)
		{
			if (gl_oes_texture_storage_multisample_2d_array_supported)
			{
				if (shader_uniform_reported_status[SHADER_UNIFORM_FS_MULTISAMPLE_ARRAY_UINT])
				{
					TCU_FAIL("fs_sampler_2d_multisample_array_uint uniform is reported more than once");
				}

				if (uniform_type != GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY_OES)
				{
					TCU_FAIL("Invalid uniform type reported for fs_sampler_2d_multisample_array_uint uniform");
				}

				shader_uniform_reported_status[SHADER_UNIFORM_FS_MULTISAMPLE_ARRAY_UINT] = true;
			}
			else
			{
				TCU_FAIL("Unsupported active uniform type reported.");
			}
		}
		else if (strcmp(uniform_name, "fs_sampler_2d_multisample_int") == 0)
		{
			if (shader_uniform_reported_status[SHADER_UNIFORM_FS_MULTISAMPLE_INT])
			{
				TCU_FAIL("fs_sampler_2d_multisample_int uniform is reported more than once");
			}

			if (uniform_type != GL_INT_SAMPLER_2D_MULTISAMPLE)
			{
				TCU_FAIL("Invalid uniform type reported for fs_sampler_2d_multisample_int uniform");
			}

			shader_uniform_reported_status[SHADER_UNIFORM_FS_MULTISAMPLE_INT] = true;
		}
		else if (strcmp(uniform_name, "fs_sampler_2d_multisample_array_int") == 0)
		{
			if (gl_oes_texture_storage_multisample_2d_array_supported)
			{
				if (shader_uniform_reported_status[SHADER_UNIFORM_FS_MULTISAMPLE_ARRAY_INT])
				{
					TCU_FAIL("fs_sampler_2d_multisample_array_int uniform is reported more than once");
				}

				if (uniform_type != GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY_OES)
				{
					TCU_FAIL("Invalid uniform type reported for fs_sampler_2d_multisample_array_int uniform");
				}

				shader_uniform_reported_status[SHADER_UNIFORM_FS_MULTISAMPLE_ARRAY_INT] = true;
			}
			else
			{
				TCU_FAIL("Unsupported active uniform type reported.");
			}
		}
		else if (strcmp(uniform_name, "vs_sampler_2d_multisample") == 0)
		{
			if (shader_uniform_reported_status[SHADER_UNIFORM_VS_MULTISAMPLE])
			{
				TCU_FAIL("vs_sampler_2d_multisample uniform is reported more than once");
			}

			if (uniform_type != GL_SAMPLER_2D_MULTISAMPLE)
			{
				TCU_FAIL("Invalid uniform type reported for vs_sampler_2d_multisample uniform");
			}

			shader_uniform_reported_status[SHADER_UNIFORM_VS_MULTISAMPLE] = true;
		}
		else if (strcmp(uniform_name, "vs_sampler_2d_multisample_array") == 0)
		{
			if (gl_oes_texture_storage_multisample_2d_array_supported)
			{
				if (shader_uniform_reported_status[SHADER_UNIFORM_VS_MULTISAMPLE_ARRAY])
				{
					TCU_FAIL("vs_sampler_2d_multisample_array uniform is reported more than once");
				}

				if (uniform_type != GL_SAMPLER_2D_MULTISAMPLE_ARRAY_OES)
				{
					TCU_FAIL("Invalid uniform type reported for vs_sampler_2d_multisample_array uniform");
				}

				shader_uniform_reported_status[SHADER_UNIFORM_VS_MULTISAMPLE_ARRAY] = true;
			}
			else
			{
				TCU_FAIL("Unsupported active uniform type reported.");
			}
		}
		else if (strcmp(uniform_name, "vs_sampler_2d_multisample_uint") == 0)
		{
			if (shader_uniform_reported_status[SHADER_UNIFORM_VS_MULTISAMPLE_UINT])
			{
				TCU_FAIL("vs_sampler_2d_multisample_uint uniform is reported more than once");
			}

			if (uniform_type != GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE)
			{
				TCU_FAIL("Invalid uniform type reported for vs_sampler_2d_multisample_uint uniform");
			}

			shader_uniform_reported_status[SHADER_UNIFORM_VS_MULTISAMPLE_UINT] = true;
		}
		else if (strcmp(uniform_name, "vs_sampler_2d_multisample_array_uint") == 0)
		{
			if (gl_oes_texture_storage_multisample_2d_array_supported)
			{
				if (shader_uniform_reported_status[SHADER_UNIFORM_VS_MULTISAMPLE_ARRAY_UINT])
				{
					TCU_FAIL("vs_sampler_2d_multisample_array_uint uniform is reported more than once");
				}

				if (uniform_type != GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY_OES)
				{
					TCU_FAIL("Invalid uniform type reported for vs_sampler_2d_multisample_array_uint uniform");
				}

				shader_uniform_reported_status[SHADER_UNIFORM_VS_MULTISAMPLE_ARRAY_UINT] = true;
			}
			else
			{
				TCU_FAIL("Unsupported active uniform type reported.");
			}
		}
		else if (strcmp(uniform_name, "vs_sampler_2d_multisample_int") == 0)
		{
			if (shader_uniform_reported_status[SHADER_UNIFORM_VS_MULTISAMPLE_INT])
			{
				TCU_FAIL("vs_sampler_2d_multisample_int uniform is reported more than once");
			}

			if (uniform_type != GL_INT_SAMPLER_2D_MULTISAMPLE)
			{
				TCU_FAIL("Invalid uniform type reported for vs_sampler_2d_multisample_int uniform");
			}

			shader_uniform_reported_status[SHADER_UNIFORM_VS_MULTISAMPLE_INT] = true;
		}
		else if (strcmp(uniform_name, "vs_sampler_2d_multisample_array_int") == 0)
		{
			if (gl_oes_texture_storage_multisample_2d_array_supported)
			{
				if (shader_uniform_reported_status[SHADER_UNIFORM_VS_MULTISAMPLE_ARRAY_INT])
				{
					TCU_FAIL("vs_sampler_2d_multisample_array_int uniform is reported more than once");
				}

				if (uniform_type != GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY_OES)
				{
					TCU_FAIL("Invalid uniform type reported for vs_sampler_2d_multisample_array_int uniform");
				}

				shader_uniform_reported_status[SHADER_UNIFORM_VS_MULTISAMPLE_ARRAY_INT] = true;
			}
			else
			{
				TCU_FAIL("Unsupported active uniform type reported.");
			}
		}
		else
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Unrecognized active uniform [" << uniform_name
							   << "] of type [" << uniform_type << "] was reported." << tcu::TestLog::EndMessage;

			TCU_FAIL("Unrecognized active uniform type reported.");
		}
	} /* for (all active uniforms) */

	/* sampler2DMSArray, isampler2DMSArray and usampler2DMSArray are part of the
	 * OES_texture_storage_multisample_2d_array extension and should only be reported
	 * if the extension is supported */
	bool expected_result[SHADER_UNIFORM_COUNT]	 = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
	bool expected_result_oes[SHADER_UNIFORM_COUNT] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

	/* Make sure all sampler uniforms we were expecting have been reported */
	for (unsigned int n_sampler_type = 0; n_sampler_type < SHADER_UNIFORM_COUNT; ++n_sampler_type)
	{
		if (gl_oes_texture_storage_multisample_2d_array_supported)
		{
			if (shader_uniform_reported_status[n_sampler_type] != expected_result_oes[n_sampler_type])
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Sampler type [" << n_sampler_type
								   << "] has not been reported by glGetActiveUniform()." << tcu::TestLog::EndMessage;

				TCU_FAIL(
					"At least one expected multisample texture sampler has not been reported by glGetActiveUniform()");
			}
		}
		else
		{
			if (shader_uniform_reported_status[n_sampler_type] != expected_result[n_sampler_type])
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Sampler type [" << n_sampler_type
								   << "] has not been reported by glGetActiveUniform()." << tcu::TestLog::EndMessage;

				TCU_FAIL(
					"At least one expected multisample texture sampler has not been reported by glGetActiveUniform()");
			}
		}
	} /* for (all shader uniform types) */

	/* Done */
	deinitInternals();
	delete[] uniform_name;
	uniform_name = NULL;

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

} /* glcts namespace */
