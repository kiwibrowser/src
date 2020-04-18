/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.0 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Negative GL State API tests.
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeStateApiTests.hpp"

#include "gluCallLogWrapper.hpp"
#include "gluContextInfo.hpp"
#include "gluShaderProgram.hpp"

#include "glwDefs.hpp"
#include "glwEnums.hpp"

#include "tcuStringTemplate.hpp"

#include "deMemory.h"

#include <string>
#include <map>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace NegativeTestShared
{

using tcu::TestLog;
using glu::CallLogWrapper;
using namespace glw;

static const char* uniformTestVertSource	=	"${GLSL_VERSION_DECL}\n"
												"uniform mediump vec4 vUnif_vec4;\n"
												"in mediump vec4 attr;"
												"layout(shared) uniform Block { mediump vec4 blockVar; };\n"
												"void main (void)\n"
												"{\n"
												"	gl_Position = vUnif_vec4 + blockVar + attr;\n"
												"}\n\0";

static const char* uniformTestFragSource	=	"${GLSL_VERSION_DECL}\n"
												"uniform mediump ivec4 fUnif_ivec4;\n"
												"uniform mediump uvec4 fUnif_uvec4;\n"
												"layout(location = 0) out mediump vec4 fragColor;"
												"void main (void)\n"
												"{\n"
												"	fragColor = vec4(vec4(fUnif_ivec4) + vec4(fUnif_uvec4));\n"
												"}\n\0";

static std::string getVtxFragVersionSources (const std::string source, NegativeTestContext& ctx)
{
	const bool supportsES32 = glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));

	std::map<std::string, std::string> args;

	args["GLSL_VERSION_DECL"] = supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_300_ES);

	return tcu::StringTemplate(source).specialize(args);
}

// Enabling & disabling states
void enable (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if cap is not one of the allowed values.");
	ctx.glEnable(-1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

// Enabling & disabling states
void enablei (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError, contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)), "This test requires a higher context version.");

	ctx.beginSection("GL_INVALID_ENUM is generated if cap is not one of the allowed values.");
	ctx.glEnablei(-1, -1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated  if index is greater than or equal to the number of indexed capabilities for cap.");
	ctx.glEnablei(GL_BLEND, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void disable (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if cap is not one of the allowed values.");
	ctx.glDisable(-1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void disablei (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError, contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)), "This test requires a higher context version.");

	ctx.beginSection("GL_INVALID_ENUM is generated if cap is not one of the allowed values.");
	ctx.glDisablei(-1,-1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated  if index is greater than or equal to the number of indexed capabilities for cap.");
	ctx.glDisablei(GL_BLEND, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

// Simple state queries
void get_booleanv (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not one of the allowed values.");
	GLboolean params = GL_FALSE;
	ctx.glGetBooleanv(-1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void get_booleani_v (NegativeTestContext& ctx)
{
	GLboolean	data						= -1;
	GLint		maxUniformBufferBindings	= 0;

	ctx.beginSection("GL_INVALID_ENUM is generated if target is not indexed state queriable with these commands.");
	ctx.glGetBooleani_v(-1, 0, &data);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is outside of the valid range for the indexed state target.");
	ctx.glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformBufferBindings);
	ctx.expectError(GL_NO_ERROR);
	ctx.glGetBooleani_v(GL_UNIFORM_BUFFER_BINDING, maxUniformBufferBindings, &data);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void get_floatv (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not one of the allowed values.");
	GLfloat params = 0.0f;
	ctx.glGetFloatv(-1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void get_integerv (NegativeTestContext& ctx)
{
	GLint params = -1;
	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not one of the allowed values.");
	ctx.glGetIntegerv(-1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void get_integer64v (NegativeTestContext& ctx)
{
	GLint64 params = -1;
	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not one of the allowed values.");
	ctx.glGetInteger64v(-1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void get_integeri_v (NegativeTestContext& ctx)
{
	GLint data								= -1;
	GLint maxUniformBufferBindings			=  0;
	GLint maxShaderStorageBufferBindings	=  0;

	ctx.beginSection("GL_INVALID_ENUM is generated if name is not an accepted value.");
	ctx.glGetIntegeri_v(-1, 0, &data);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is outside of the valid range for the indexed state target.");
	ctx.glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformBufferBindings);
	ctx.expectError(GL_NO_ERROR);
	ctx.glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, maxUniformBufferBindings, &data);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is outside of the valid range for the indexed state target.");
	ctx.glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxShaderStorageBufferBindings);
	ctx.expectError(GL_NO_ERROR);
	ctx.glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, maxShaderStorageBufferBindings, &data);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void get_integer64i_v (NegativeTestContext& ctx)
{
	GLint64	data							= (GLint64)-1;
	GLint	maxUniformBufferBindings		= 0;
	GLint	maxShaderStorageBufferBindings	= 0;

	ctx.beginSection("GL_INVALID_ENUM is generated if name is not an accepted value.");
	ctx.glGetInteger64i_v(-1, 0, &data);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is outside of the valid range for the indexed state target.");
	ctx.glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformBufferBindings);
	ctx.expectError(GL_NO_ERROR);
	ctx.glGetInteger64i_v(GL_UNIFORM_BUFFER_START, maxUniformBufferBindings, &data);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is outside of the valid range for the indexed state target.");
	ctx.glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxShaderStorageBufferBindings);
	ctx.expectError(GL_NO_ERROR);
	ctx.glGetInteger64i_v(GL_SHADER_STORAGE_BUFFER_START, maxShaderStorageBufferBindings, &data);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.glGetInteger64i_v(GL_SHADER_STORAGE_BUFFER_SIZE, maxShaderStorageBufferBindings, &data);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void get_string (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if name is not an accepted value.");
	ctx.glGetString(-1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void get_stringi (NegativeTestContext& ctx)
{
	GLint numExtensions	= 0;

	ctx.beginSection("GL_INVALID_ENUM is generated if name is not an accepted value.");
	ctx.glGetStringi(-1, 0);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is outside the valid range for indexed state name.");
	ctx.glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
	ctx.glGetStringi(GL_EXTENSIONS, numExtensions);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

// Enumerated state queries: Shaders

void get_attached_shaders (NegativeTestContext& ctx)
{
	GLuint	shaders[1]		= { 0 };
	GLuint	shaderObject	= ctx.glCreateShader(GL_VERTEX_SHADER);
	GLuint	program			= ctx.glCreateProgram();
	GLsizei	count[1]		= { 0 };

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not a value generated by OpenGL.");
	ctx.glGetAttachedShaders(-1, 1, &count[0], &shaders[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is not a program object.");
	ctx.glGetAttachedShaders(shaderObject, 1, &count[0], &shaders[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if maxCount is less than 0.");
	ctx.glGetAttachedShaders(program, -1, &count[0], &shaders[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteShader(shaderObject);
	ctx.glDeleteProgram(program);
}

void get_shaderiv (NegativeTestContext& ctx)
{
	GLboolean	shaderCompilerSupported;
	GLuint		shader		= ctx.glCreateShader(GL_VERTEX_SHADER);
	GLuint		program		= ctx.glCreateProgram();
	GLint		param[1]	= { -1 };

	ctx.glGetBooleanv(GL_SHADER_COMPILER, &shaderCompilerSupported);
	ctx.getLog() << TestLog::Message << "// GL_SHADER_COMPILER = " << (shaderCompilerSupported ? "GL_TRUE" : "GL_FALSE") << TestLog::EndMessage;

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not an accepted value.");
	ctx.glGetShaderiv(shader, -1, &param[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if shader is not a value generated by OpenGL.");
	ctx.glGetShaderiv(-1, GL_SHADER_TYPE, &param[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if shader does not refer to a shader object.");
	ctx.glGetShaderiv(program, GL_SHADER_TYPE, &param[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteShader(shader);
	ctx.glDeleteProgram(program);
}

void get_shader_info_log (NegativeTestContext& ctx)
{
	GLuint shader		= ctx.glCreateShader(GL_VERTEX_SHADER);
	GLuint program		= ctx.glCreateProgram();
	GLsizei length[1]	= { -1 };
	char infoLog[128]	= { 0 };

	ctx.beginSection("GL_INVALID_VALUE is generated if shader is not a value generated by OpenGL.");
	ctx.glGetShaderInfoLog(-1, 128, &length[0], &infoLog[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if shader is not a shader object.");
	ctx.glGetShaderInfoLog(program, 128, &length[0], &infoLog[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if maxLength is less than 0.");
	ctx.glGetShaderInfoLog(shader, -1, &length[0], &infoLog[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteShader(shader);
	ctx.glDeleteProgram(program);
}

void get_shader_precision_format (NegativeTestContext& ctx)
{
	GLboolean	shaderCompilerSupported;
	GLint		range[2];
	GLint		precision[1];

	ctx.glGetBooleanv(GL_SHADER_COMPILER, &shaderCompilerSupported);
	ctx.getLog() << TestLog::Message << "// GL_SHADER_COMPILER = " << (shaderCompilerSupported ? "GL_TRUE" : "GL_FALSE") << TestLog::EndMessage;

	deMemset(&range[0], 0xcd, sizeof(range));
	deMemset(&precision[0], 0xcd, sizeof(precision));

	ctx.beginSection("GL_INVALID_ENUM is generated if shaderType or precisionType is not an accepted value.");
	ctx.glGetShaderPrecisionFormat (-1, GL_MEDIUM_FLOAT, &range[0], &precision[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetShaderPrecisionFormat (GL_VERTEX_SHADER, -1, &range[0], &precision[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetShaderPrecisionFormat (-1, -1, &range[0], &precision[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void get_shader_source (NegativeTestContext& ctx)
{
	GLsizei	length[1]	= { 0 };
	char	source[1]	= { 0 };
	GLuint	program		= ctx.glCreateProgram();
	GLuint	shader		= ctx.glCreateShader(GL_VERTEX_SHADER);

	ctx.beginSection("GL_INVALID_VALUE is generated if shader is not a value generated by OpenGL.");
	ctx.glGetShaderSource(-1, 1, &length[0], &source[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if shader is not a shader object.");
	ctx.glGetShaderSource(program, 1, &length[0], &source[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if bufSize is less than 0.");
	ctx.glGetShaderSource(shader, -1, &length[0], &source[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteProgram(program);
	ctx.glDeleteShader(shader);
}

// Enumerated state queries: Programs

void get_programiv (NegativeTestContext& ctx)
{
	GLuint	program		= ctx.glCreateProgram();
	GLuint	shader		= ctx.glCreateShader(GL_VERTEX_SHADER);
	GLint	params[1]	= { 0 };

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not an accepted value.");
	ctx.glGetProgramiv(program, -1, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not a value generated by OpenGL.");
	ctx.glGetProgramiv(-1, GL_LINK_STATUS, &params[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program does not refer to a program object.");
	ctx.glGetProgramiv(shader, GL_LINK_STATUS, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteProgram(program);
	ctx.glDeleteShader(shader);
}

void get_program_info_log (NegativeTestContext& ctx)
{
	GLuint	program		= ctx.glCreateProgram();
	GLuint	shader		= ctx.glCreateShader(GL_VERTEX_SHADER);
	GLsizei	length[1]	= { 0 };
	char	infoLog[1]	= { 'x' };

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not a value generated by OpenGL.");
	ctx.glGetProgramInfoLog (-1, 1, &length[0], &infoLog[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is not a program object.");
	ctx.glGetProgramInfoLog (shader, 1, &length[0], &infoLog[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if maxLength is less than 0.");
	ctx.glGetProgramInfoLog (program, -1, &length[0], &infoLog[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteProgram(program);
	ctx.glDeleteShader(shader);
}

// Enumerated state queries: Shader variables

void get_tex_parameterfv (NegativeTestContext& ctx)
{
	GLfloat params[1] = { 0 };

	ctx.beginSection("GL_INVALID_ENUM is generated if target or pname is not an accepted value.");
	ctx.glGetTexParameterfv (-1, GL_TEXTURE_MAG_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetTexParameterfv (GL_TEXTURE_2D, -1, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetTexParameterfv (-1, -1, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void get_tex_parameteriv (NegativeTestContext& ctx)
{
	GLint params[1] = { 0 };

	ctx.beginSection("GL_INVALID_ENUM is generated if target or pname is not an accepted value.");
	ctx.glGetTexParameteriv (-1, GL_TEXTURE_MAG_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetTexParameteriv (GL_TEXTURE_2D, -1, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetTexParameteriv (-1, -1, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void get_tex_parameteriiv (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError, contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)), "This test requires a higher context version.");

	GLint params[1] = { 0 };

	ctx.beginSection("GL_INVALID_ENUM is generated if target or pname is not an accepted value.");
	ctx.glGetTexParameterIiv(-1, GL_TEXTURE_MAG_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetTexParameterIiv(GL_TEXTURE_2D, -1, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetTexParameterIiv(-1, -1, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void get_tex_parameteriuiv (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError, contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)), "This test requires a higher context version.");

	GLuint params[1] = { 0 };

	ctx.beginSection("GL_INVALID_ENUM is generated if target or pname is not an accepted value.");
	ctx.glGetTexParameterIuiv(-1, GL_TEXTURE_MAG_FILTER, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetTexParameterIuiv(GL_TEXTURE_2D, -1, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetTexParameterIuiv(-1, -1, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void get_uniformfv (NegativeTestContext& ctx)
{
	glu::ShaderProgram	program		(ctx.getRenderContext(), glu::makeVtxFragSources(getVtxFragVersionSources(uniformTestVertSource, ctx), getVtxFragVersionSources(uniformTestFragSource, ctx)));
	GLfloat				params[4]	= { 0.f };
	GLuint				shader;
	GLuint				programEmpty;
	GLint				unif;

	ctx.glUseProgram(program.getProgram());

	unif = ctx.glGetUniformLocation(program.getProgram(), "vUnif_vec4");	// vec4
	if (unif == -1)
		ctx.fail("Failed to retrieve uniform location");

	shader = ctx.glCreateShader(GL_VERTEX_SHADER);
	programEmpty = ctx.glCreateProgram();

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not a value generated by OpenGL.");
	ctx.glGetUniformfv (-1, unif, &params[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is not a program object.");
	ctx.glGetUniformfv (shader, unif, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program has not been successfully linked.");
	ctx.glGetUniformfv (programEmpty, unif, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if location does not correspond to a valid uniform variable location for the specified program object.");
	ctx.glGetUniformfv (program.getProgram(), -1, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteShader(shader);
	ctx.glDeleteProgram(programEmpty);
}

void get_nuniformfv (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError, contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)), "This test requires a higher context version.");

	glu::ShaderProgram	program			(ctx.getRenderContext(), glu::makeVtxFragSources(getVtxFragVersionSources(uniformTestVertSource, ctx), getVtxFragVersionSources(uniformTestFragSource, ctx)));
	GLint				unif			= ctx.glGetUniformLocation(program.getProgram(), "vUnif_vec4");
	GLfloat				params[4]		= { 0.0f, 0.0f, 0.0f, 0.0f };
	GLuint				shader;
	GLuint				programEmpty;
	GLsizei				bufferSize;

	ctx.glUseProgram(program.getProgram());

	if (unif == -1)
		ctx.fail("Failed to retrieve uniform location");

	shader			= ctx.glCreateShader(GL_VERTEX_SHADER);
	programEmpty	= ctx.glCreateProgram();

	ctx.glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &bufferSize);

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not a value generated by OpenGL.");
	ctx.glGetnUniformfv(-1, unif, bufferSize, &params[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is not a program object.");
	ctx.glGetnUniformfv(shader, unif, bufferSize, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program has not been successfully linked.");
	ctx.glGetnUniformfv(programEmpty, unif, bufferSize, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if location does not correspond to a valid uniform variable location for the specified program object.");
	ctx.glGetnUniformfv(program.getProgram(), -1, bufferSize, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the buffer size required to store the requested data is greater than bufSize.");
	ctx.glGetnUniformfv(program.getProgram(), unif, 0, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteShader(shader);
	ctx.glDeleteProgram(programEmpty);
}

void get_uniformiv (NegativeTestContext& ctx)
{
	glu::ShaderProgram	program			(ctx.getRenderContext(), glu::makeVtxFragSources(getVtxFragVersionSources(uniformTestVertSource, ctx), getVtxFragVersionSources(uniformTestFragSource, ctx)));
	GLint				unif			= ctx.glGetUniformLocation(program.getProgram(), "fUnif_ivec4");
	GLint				params[4]		= { 0, 0, 0, 0 };
	GLuint				shader;
	GLuint				programEmpty;

	ctx.glUseProgram(program.getProgram());

	if (unif == -1)
		ctx.fail("Failed to retrieve uniform location");

	shader = ctx.glCreateShader(GL_VERTEX_SHADER);
	programEmpty = ctx.glCreateProgram();

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not a value generated by OpenGL.");
	ctx.glGetUniformiv (-1, unif, &params[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is not a program object.");
	ctx.glGetUniformiv (shader, unif, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program has not been successfully linked.");
	ctx.glGetUniformiv (programEmpty, unif, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if location does not correspond to a valid uniform variable location for the specified program object.");
	ctx.glGetUniformiv (program.getProgram(), -1, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteShader(shader);
	ctx.glDeleteProgram(programEmpty);
}

void get_nuniformiv (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError, contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)), "This test requires a higher context version.");

	glu::ShaderProgram	program			(ctx.getRenderContext(), glu::makeVtxFragSources(getVtxFragVersionSources(uniformTestVertSource, ctx), getVtxFragVersionSources(uniformTestFragSource, ctx)));
	GLint				unif			= ctx.glGetUniformLocation(program.getProgram(), "fUnif_ivec4");
	GLint				params[4]		= { 0, 0, 0, 0 };
	GLuint				shader;
	GLuint				programEmpty;
	GLsizei				bufferSize;

	ctx.glUseProgram(program.getProgram());

	if (unif == -1)
		ctx.fail("Failed to retrieve uniform location");

	shader = ctx.glCreateShader(GL_VERTEX_SHADER);
	programEmpty = ctx.glCreateProgram();

	ctx.glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &bufferSize);

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not a value generated by OpenGL.");
	ctx.glGetnUniformiv(-1, unif, bufferSize, &params[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is not a program object.");
	ctx.glGetnUniformiv(shader, unif, bufferSize, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program has not been successfully linked.");
	ctx.glGetnUniformiv(programEmpty, unif, bufferSize, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if location does not correspond to a valid uniform variable location for the specified program object.");
	ctx.glGetnUniformiv(program.getProgram(), -1, bufferSize, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the buffer size required to store the requested data is greater than bufSize.");
	ctx.glGetnUniformiv(program.getProgram(), unif, - 1, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteShader(shader);
	ctx.glDeleteProgram(programEmpty);
}

void get_uniformuiv (NegativeTestContext& ctx)
{
	glu::ShaderProgram	program			(ctx.getRenderContext(), glu::makeVtxFragSources(getVtxFragVersionSources(uniformTestVertSource, ctx), getVtxFragVersionSources(uniformTestFragSource, ctx)));
	GLint				unif			= ctx.glGetUniformLocation(program.getProgram(), "fUnif_uvec4");
	GLuint				params[4]		= { 0, 0, 0, 0 };
	GLuint				shader;
	GLuint				programEmpty;

	ctx.glUseProgram(program.getProgram());

	if (unif == -1)
		ctx.fail("Failed to retrieve uniform location");

	shader = ctx.glCreateShader(GL_VERTEX_SHADER);
	programEmpty = ctx.glCreateProgram();

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not a value generated by OpenGL.");
	ctx.glGetUniformuiv (-1, unif, &params[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is not a program object.");
	ctx.glGetUniformuiv (shader, unif, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program has not been successfully linked.");
	ctx.glGetUniformuiv (programEmpty, unif, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if location does not correspond to a valid uniform variable location for the specified program object.");
	ctx.glGetUniformuiv (program.getProgram(), -1, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteShader(shader);
	ctx.glDeleteProgram(programEmpty);
}

void get_nuniformuiv (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError, contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)), "This test requires a higher context version.");

	glu::ShaderProgram	program			(ctx.getRenderContext(), glu::makeVtxFragSources(getVtxFragVersionSources(uniformTestVertSource, ctx), getVtxFragVersionSources(uniformTestFragSource, ctx)));
	GLint				unif			= ctx.glGetUniformLocation(program.getProgram(), "fUnif_ivec4");
	GLuint				params[4]		= { 0, 0, 0, 0 };
	GLuint				shader;
	GLuint				programEmpty;
	GLsizei				bufferSize;

	ctx.glUseProgram(program.getProgram());

	if (unif == -1)
		ctx.fail("Failed to retrieve uniform location");

	shader = ctx.glCreateShader(GL_VERTEX_SHADER);
	programEmpty = ctx.glCreateProgram();

	ctx.glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &bufferSize);

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not a value generated by OpenGL.");
	ctx.glGetnUniformuiv(-1, unif, bufferSize, &params[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is not a program object.");
	ctx.glGetnUniformuiv(shader, unif, bufferSize, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program has not been successfully linked.");
	ctx.glGetnUniformuiv(programEmpty, unif, bufferSize, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if location does not correspond to a valid uniform variable location for the specified program object.");
	ctx.glGetnUniformuiv(program.getProgram(), -1, bufferSize, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the buffer size required to store the requested data is greater than bufSize.");
	ctx.glGetnUniformuiv(program.getProgram(), unif, -1, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteShader(shader);
	ctx.glDeleteProgram(programEmpty);
}

void get_active_uniform (NegativeTestContext& ctx)
{
	GLuint				shader				= ctx.glCreateShader(GL_VERTEX_SHADER);
	glu::ShaderProgram	program				(ctx.getRenderContext(), glu::makeVtxFragSources(getVtxFragVersionSources(uniformTestVertSource, ctx), getVtxFragVersionSources(uniformTestFragSource, ctx)));
	GLint				numActiveUniforms	= -1;

	ctx.glGetProgramiv	(program.getProgram(), GL_ACTIVE_UNIFORMS,	&numActiveUniforms);
	ctx.getLog() << TestLog::Message << "// GL_ACTIVE_UNIFORMS = " << numActiveUniforms << " (expected 4)." << TestLog::EndMessage;

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not a value generated by OpenGL.");
	ctx.glGetActiveUniform(-1, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is not a program object.");
	ctx.glGetActiveUniform(shader, 0, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is greater than or equal to the number of active uniform variables in program.");
	ctx.glUseProgram(program.getProgram());
	ctx.glGetActiveUniform(program.getProgram(), numActiveUniforms, 0, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if bufSize is less than 0.");
	ctx.glGetActiveUniform(program.getProgram(), 0, -1, 0, 0, 0, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glUseProgram(0);
	ctx.glDeleteShader(shader);
}

void get_active_uniformsiv (NegativeTestContext& ctx)
{
	GLuint					shader				= ctx.glCreateShader(GL_VERTEX_SHADER);
	glu::ShaderProgram		program				(ctx.getRenderContext(), glu::makeVtxFragSources(getVtxFragVersionSources(uniformTestVertSource, ctx), getVtxFragVersionSources(uniformTestFragSource, ctx)));
	GLuint					dummyUniformIndex	= 1;
	GLint					dummyParamDst		= -1;
	GLint					numActiveUniforms	= -1;

	ctx.glUseProgram(program.getProgram());

	ctx.glGetProgramiv	(program.getProgram(), GL_ACTIVE_UNIFORMS, &numActiveUniforms);
	ctx.getLog() << TestLog::Message << "// GL_ACTIVE_UNIFORMS = " << numActiveUniforms << " (expected 4)." << TestLog::EndMessage;

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not a value generated by OpenGL.");
	ctx.glGetActiveUniformsiv(-1, 1, &dummyUniformIndex, GL_UNIFORM_TYPE, &dummyParamDst);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is not a program object.");
	ctx.glGetActiveUniformsiv(shader, 1, &dummyUniformIndex, GL_UNIFORM_TYPE, &dummyParamDst);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if any value in uniformIndices is greater than or equal to the value of GL_ACTIVE_UNIFORMS for program.");
	for (int excess = 0; excess <= 2; excess++)
	{
		std::vector<GLuint> invalidUniformIndices;
		invalidUniformIndices.push_back(1);
		invalidUniformIndices.push_back(numActiveUniforms-1+excess);
		invalidUniformIndices.push_back(1);

		std::vector<GLint> dummyParamsDst(invalidUniformIndices.size());
		ctx.glGetActiveUniformsiv(program.getProgram(), (GLsizei)invalidUniformIndices.size(), &invalidUniformIndices[0], GL_UNIFORM_TYPE, &dummyParamsDst[0]);
		ctx.expectError(excess == 0 ? GL_NO_ERROR : GL_INVALID_VALUE);
	}
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not an accepted token.");
	ctx.glGetActiveUniformsiv(program.getProgram(), 1, &dummyUniformIndex, -1, &dummyParamDst);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.glUseProgram(0);
	ctx.glDeleteShader(shader);
}

void get_active_uniform_blockiv (NegativeTestContext& ctx)
{
	glu::ShaderProgram	program			(ctx.getRenderContext(), glu::makeVtxFragSources(getVtxFragVersionSources(uniformTestVertSource, ctx), getVtxFragVersionSources(uniformTestFragSource, ctx)));
	GLuint				shader			= ctx.glCreateShader(GL_VERTEX_SHADER);
	GLint				params			= -1;
	GLint				numActiveBlocks	= -1;

	ctx.glGetProgramiv(program.getProgram(), GL_ACTIVE_UNIFORM_BLOCKS, &numActiveBlocks);
	ctx.getLog() << TestLog::Message << "// GL_ACTIVE_UNIFORM_BLOCKS = " << numActiveBlocks << " (expected 1)." << TestLog::EndMessage;
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not the name of either a program or shader object.");
	ctx.glGetActiveUniformBlockiv(-1, 0, GL_UNIFORM_BLOCK_BINDING, &params);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is the name of a shader object");
	ctx.glGetActiveUniformBlockiv(shader, 0, GL_UNIFORM_BLOCK_BINDING, &params);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if uniformBlockIndex is greater than or equal to the value of GL_ACTIVE_UNIFORM_BLOCKS or is not the index of an active uniform block in program.");
	ctx.glUseProgram(program.getProgram());
	ctx.expectError(GL_NO_ERROR);
	ctx.glGetActiveUniformBlockiv(program.getProgram(), numActiveBlocks, GL_UNIFORM_BLOCK_BINDING, &params);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not one of the accepted tokens.");
	ctx.glGetActiveUniformBlockiv(program.getProgram(), 0, -1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.glUseProgram(0);
}

void get_active_uniform_block_name (NegativeTestContext& ctx)
{
	glu::ShaderProgram	program			(ctx.getRenderContext(), glu::makeVtxFragSources(getVtxFragVersionSources(uniformTestVertSource, ctx), getVtxFragVersionSources(uniformTestFragSource, ctx)));
	GLuint				shader			= ctx.glCreateShader(GL_VERTEX_SHADER);
	GLsizei				length			= -1;
	GLint				numActiveBlocks	= -1;
	GLchar				uniformBlockName[128];

	deMemset(&uniformBlockName[0], 0, sizeof(uniformBlockName));

	ctx.glGetProgramiv(program.getProgram(), GL_ACTIVE_UNIFORM_BLOCKS, &numActiveBlocks);
	ctx.getLog() << TestLog::Message << "// GL_ACTIVE_UNIFORM_BLOCKS = " << numActiveBlocks << " (expected 1)." << TestLog::EndMessage;
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is the name of a shader object.");
	ctx.glGetActiveUniformBlockName(shader, numActiveBlocks, GL_UNIFORM_BLOCK_BINDING, &length, &uniformBlockName[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not the name of either a program or shader object.");
	ctx.glGetActiveUniformBlockName(-1, numActiveBlocks, GL_UNIFORM_BLOCK_BINDING, &length, &uniformBlockName[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if uniformBlockIndex is greater than or equal to the value of GL_ACTIVE_UNIFORM_BLOCKS or is not the index of an active uniform block in program.");
	ctx.glUseProgram(program.getProgram());
	ctx.expectError(GL_NO_ERROR);
	ctx.glGetActiveUniformBlockName(program.getProgram(), numActiveBlocks, (int)sizeof(uniformBlockName), &length, &uniformBlockName[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glUseProgram(0);
}

void get_active_attrib (NegativeTestContext& ctx)
{
	GLuint				shader				= ctx.glCreateShader(GL_VERTEX_SHADER);
	glu::ShaderProgram	program				(ctx.getRenderContext(), glu::makeVtxFragSources(getVtxFragVersionSources(uniformTestVertSource, ctx), getVtxFragVersionSources(uniformTestFragSource, ctx)));
	GLint				numActiveAttributes	= -1;
	GLsizei				length				= -1;
	GLint				size				= -1;
	GLenum				type				= -1;
	GLchar				name[32];

	deMemset(&name[0], 0, sizeof(name));

	ctx.glGetProgramiv	(program.getProgram(), GL_ACTIVE_ATTRIBUTES,	&numActiveAttributes);
	ctx.getLog() << TestLog::Message << "// GL_ACTIVE_ATTRIBUTES = " << numActiveAttributes << " (expected 1)." << TestLog::EndMessage;

	ctx.glUseProgram(program.getProgram());

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not a value generated by OpenGL.");
	ctx.glGetActiveAttrib(-1, 0, 32, &length, &size, &type, &name[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is not a program object.");
	ctx.glGetActiveAttrib(shader, 0, 32, &length, &size, &type, &name[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is greater than or equal to GL_ACTIVE_ATTRIBUTES.");
	ctx.glGetActiveAttrib(program.getProgram(), numActiveAttributes, (int)sizeof(name), &length, &size, &type, &name[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if bufSize is less than 0.");
	ctx.glGetActiveAttrib(program.getProgram(), 0, -1, &length, &size, &type, &name[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glUseProgram(0);
	ctx.glDeleteShader(shader);
}

void get_uniform_indices (NegativeTestContext& ctx)
{
	GLuint				shader			= ctx.glCreateShader(GL_VERTEX_SHADER);
	glu::ShaderProgram	program			(ctx.getRenderContext(), glu::makeVtxFragSources(getVtxFragVersionSources(uniformTestVertSource, ctx), getVtxFragVersionSources(uniformTestFragSource, ctx)));
	GLint				numActiveBlocks	= -1;
	const GLchar*		uniformName		= "Block.blockVar";
	GLuint				uniformIndices	= -1;
	GLuint				invalid			= -1;

	ctx.glGetProgramiv(program.getProgram(), GL_ACTIVE_UNIFORM_BLOCKS,	&numActiveBlocks);
	ctx.getLog() << TestLog::Message << "// GL_ACTIVE_UNIFORM_BLOCKS = "		<< numActiveBlocks			<< TestLog::EndMessage;
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is a name of shader object.");
	ctx.glGetUniformIndices(shader, 1, &uniformName, &uniformIndices);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if program is not name of program or shader object.");
	ctx.glGetUniformIndices(invalid, 1, &uniformName, &uniformIndices);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glUseProgram(0);
	ctx.glDeleteShader(shader);
}

void get_vertex_attribfv (NegativeTestContext& ctx)
{
	GLfloat	params				= 0.0f;
	GLint	maxVertexAttribs;

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not an accepted value.");
	ctx.glGetVertexAttribfv(0, -1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is greater than or equal to GL_MAX_VERTEX_ATTRIBS.");
	ctx.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
	ctx.glGetVertexAttribfv(maxVertexAttribs, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &params);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void get_vertex_attribiv (NegativeTestContext& ctx)
{
	GLint	params				= -1;
	GLint	maxVertexAttribs;

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not an accepted value.");
	ctx.glGetVertexAttribiv(0, -1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is greater than or equal to GL_MAX_VERTEX_ATTRIBS.");
	ctx.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
	ctx.glGetVertexAttribiv(maxVertexAttribs, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &params);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void get_vertex_attribi_iv (NegativeTestContext& ctx)
{
	GLint	params				= -1;
	GLint	maxVertexAttribs;

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not an accepted value.");
	ctx.glGetVertexAttribIiv(0, -1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is greater than or equal to GL_MAX_VERTEX_ATTRIBS.");
	ctx.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
	ctx.glGetVertexAttribIiv(maxVertexAttribs, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &params);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void get_vertex_attribi_uiv (NegativeTestContext& ctx)
{
	GLuint	params				= (GLuint)-1;
	GLint	maxVertexAttribs;

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not an accepted value.");
	ctx.glGetVertexAttribIuiv(0, -1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is greater than or equal to GL_MAX_VERTEX_ATTRIBS.");
	ctx.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
	ctx.glGetVertexAttribIuiv(maxVertexAttribs, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &params);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void get_vertex_attrib_pointerv (NegativeTestContext& ctx)
{
	GLvoid*	ptr[1]				= { DE_NULL };
	GLint	maxVertexAttribs;

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not an accepted value.");
	ctx.glGetVertexAttribPointerv(0, -1, &ptr[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is greater than or equal to GL_MAX_VERTEX_ATTRIBS.");
	ctx.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
	ctx.glGetVertexAttribPointerv(maxVertexAttribs, GL_VERTEX_ATTRIB_ARRAY_POINTER, &ptr[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void get_frag_data_location (NegativeTestContext& ctx)
{
	GLuint shader	= ctx.glCreateShader(GL_VERTEX_SHADER);
	GLuint program	= ctx.glCreateProgram();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program is the name of a shader object.");
	ctx.glGetFragDataLocation(shader, "gl_FragColor");
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if program has not been linked.");
	ctx.glGetFragDataLocation(program, "gl_FragColor");
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteProgram(program);
	ctx.glDeleteShader(shader);
}

// Enumerated state queries: Buffers

void get_buffer_parameteriv (NegativeTestContext& ctx)
{
	GLint	params	= -1;
	GLuint	buf;
	ctx.glGenBuffers(1, &buf);
	ctx.glBindBuffer(GL_ARRAY_BUFFER, buf);

	ctx.beginSection("GL_INVALID_ENUM is generated if target or value is not an accepted value.");
	ctx.glGetBufferParameteriv(-1, GL_BUFFER_SIZE, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetBufferParameteriv(GL_ARRAY_BUFFER, -1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetBufferParameteriv(-1, -1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the reserved buffer object name 0 is bound to target.");
	ctx.glBindBuffer(GL_ARRAY_BUFFER, 0);
	ctx.glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &params);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteBuffers(1, &buf);
}

void get_buffer_parameteri64v (NegativeTestContext& ctx)
{
	GLint64	params	= -1;
	GLuint	buf;
	ctx.glGenBuffers(1, &buf);
	ctx.glBindBuffer(GL_ARRAY_BUFFER, buf);

	ctx.beginSection("GL_INVALID_ENUM is generated if target or value is not an accepted value.");
	ctx.glGetBufferParameteri64v(-1, GL_BUFFER_SIZE, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetBufferParameteri64v(GL_ARRAY_BUFFER , -1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetBufferParameteri64v(-1, -1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the reserved buffer object name 0 is bound to target.");
	ctx.glBindBuffer(GL_ARRAY_BUFFER, 0);
	ctx.glGetBufferParameteri64v(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &params);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteBuffers(1, &buf);
}

void get_buffer_pointerv (NegativeTestContext& ctx)
{
	GLvoid*	params	= DE_NULL;
	GLuint	buf;
	ctx.glGenBuffers(1, &buf);
	ctx.glBindBuffer(GL_ARRAY_BUFFER, buf);

	ctx.beginSection("GL_INVALID_ENUM is generated if target or pname is not an accepted value.");
	ctx.glGetBufferPointerv(GL_ARRAY_BUFFER, -1, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glGetBufferPointerv(-1, GL_BUFFER_MAP_POINTER, &params);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the reserved buffer object name 0 is bound to target.");
	ctx.glBindBuffer(GL_ARRAY_BUFFER, 0);
	ctx.glGetBufferPointerv(GL_ARRAY_BUFFER, GL_BUFFER_MAP_POINTER, &params);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteBuffers(1, &buf);
}

void get_framebuffer_attachment_parameteriv (NegativeTestContext& ctx)
{
	GLint	params[1]	= { -1 };
	GLuint	fbo;
	GLuint	rbo[2];

	ctx.glGenFramebuffers			(1, &fbo);
	ctx.glGenRenderbuffers			(2, rbo);

	ctx.glBindFramebuffer			(GL_FRAMEBUFFER,	fbo);
	ctx.glBindRenderbuffer			(GL_RENDERBUFFER,	rbo[0]);
	ctx.glRenderbufferStorage		(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 16, 16);
	ctx.glFramebufferRenderbuffer	(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo[0]);
	ctx.glBindRenderbuffer			(GL_RENDERBUFFER,	rbo[1]);
	ctx.glRenderbufferStorage		(GL_RENDERBUFFER, GL_STENCIL_INDEX8, 16, 16);
	ctx.glFramebufferRenderbuffer	(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);
	ctx.glCheckFramebufferStatus	(GL_FRAMEBUFFER);
	ctx.expectError					(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_ENUM is generated if target is not one of the accepted tokens.");
	ctx.glGetFramebufferAttachmentParameteriv(-1, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &params[0]);					// TYPE is GL_RENDERBUFFER
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not valid for the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE.");
	ctx.glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &params[0]);	// TYPE is GL_RENDERBUFFER
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ctx.glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &params[0]);					// TYPE is GL_FRAMEBUFFER_DEFAULT
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if attachment is GL_DEPTH_STENCIL_ATTACHMENT and different objects are bound to the depth and stencil attachment points of target.");
	ctx.glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE and pname is not GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME.");
	ctx.glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &params[0]);		// TYPE is GL_NONE
	ctx.expectError(GL_NO_ERROR);
	ctx.glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &params[0]);	// TYPE is GL_NONE
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION or GL_INVALID_ENUM is generated if attachment is not one of the accepted values for the current binding of target.");
	ctx.glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &params[0]);					// A FBO is bound so GL_BACK is invalid
	ctx.expectError(GL_INVALID_OPERATION, GL_INVALID_ENUM);
	ctx.glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ctx.glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &params[0]);		// Default framebuffer is bound so GL_COLOR_ATTACHMENT0 is invalid
	ctx.expectError(GL_INVALID_OPERATION, GL_INVALID_ENUM);
	ctx.endSection();

	ctx.glDeleteFramebuffers(1, &fbo);
}

void get_renderbuffer_parameteriv (NegativeTestContext& ctx)
{
	GLint	params[1] = { -1 };
	GLuint	rbo;
	ctx.glGenRenderbuffers(1, &rbo);
	ctx.glBindRenderbuffer(GL_RENDERBUFFER, rbo);

	ctx.beginSection("GL_INVALID_ENUM is generated if target is not GL_RENDERBUFFER.");
	ctx.glGetRenderbufferParameteriv(-1, GL_RENDERBUFFER_WIDTH, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not one of the accepted tokens.");
	ctx.glGetRenderbufferParameteriv(GL_RENDERBUFFER, -1, &params[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION  is generated if the renderbuffer currently bound to target is zero.");
	ctx.glBindRenderbuffer(GL_RENDERBUFFER, 0);
	ctx.expectError(GL_NO_ERROR);
	ctx.glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteRenderbuffers(1, &rbo);
	ctx.glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void get_internalformativ (NegativeTestContext& ctx)
{
	GLint params[16];

	deMemset(&params[0], 0xcd, sizeof(params));

	ctx.beginSection("GL_INVALID_VALUE is generated if bufSize is negative.");
	ctx.glGetInternalformativ	(GL_RENDERBUFFER, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, -1, &params[0]);
	ctx.expectError				(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not GL_SAMPLES or GL_NUM_SAMPLE_COUNTS.");
	ctx.glGetInternalformativ	(GL_RENDERBUFFER, GL_RGBA8, -1, 16, &params[0]);
	ctx.expectError				(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if internalformat is not color-, depth-, or stencil-renderable.");

	if (!ctx.getContextInfo().isExtensionSupported("GL_EXT_render_snorm"))
	{
		ctx.glGetInternalformativ	(GL_RENDERBUFFER, GL_RG8_SNORM, GL_NUM_SAMPLE_COUNTS, 16, &params[0]);
		ctx.expectError				(GL_INVALID_ENUM);
	}

	ctx.glGetInternalformativ	(GL_RENDERBUFFER, GL_COMPRESSED_RGB8_ETC2, GL_NUM_SAMPLE_COUNTS, 16, &params[0]);
	ctx.expectError				(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if target is not GL_RENDERBUFFER.");
	ctx.glGetInternalformativ	(-1, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 16, &params[0]);
	ctx.expectError				(GL_INVALID_ENUM);
	ctx.glGetInternalformativ	(GL_FRAMEBUFFER, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 16, &params[0]);
	ctx.expectError				(GL_INVALID_ENUM);

	if (!ctx.getContextInfo().isExtensionSupported("GL_EXT_sparse_texture"))
	{
		ctx.glGetInternalformativ	(GL_TEXTURE_2D, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 16, &params[0]);
		ctx.expectError				(GL_INVALID_ENUM);
	}

	ctx.endSection();
}

// Query object queries

void get_queryiv (NegativeTestContext& ctx)
{
	GLint params = -1;

	ctx.beginSection("GL_INVALID_ENUM is generated if target or pname is not an accepted value.");
	ctx.glGetQueryiv	(GL_ANY_SAMPLES_PASSED, -1, &params);
	ctx.expectError		(GL_INVALID_ENUM);
	ctx.glGetQueryiv	(-1, GL_CURRENT_QUERY, &params);
	ctx.expectError		(GL_INVALID_ENUM);
	ctx.glGetQueryiv	(-1, -1, &params);
	ctx.expectError		(GL_INVALID_ENUM);
	ctx.endSection();
}

void get_query_objectuiv (NegativeTestContext& ctx)
{
	GLuint params	= -1;
	GLuint id;
	ctx.glGenQueries		(1, &id);

	ctx.beginSection("GL_INVALID_OPERATION is generated if id is not the name of a query object.");
	ctx.glGetQueryObjectuiv	(-1, GL_QUERY_RESULT_AVAILABLE, &params);
	ctx.expectError			(GL_INVALID_OPERATION);
	ctx.getLog() << TestLog::Message << "// Note: " << id << " is not a query object yet, since it hasn't been used by glBeginQuery" << TestLog::EndMessage;
	ctx.glGetQueryObjectuiv	(id, GL_QUERY_RESULT_AVAILABLE, &params);
	ctx.expectError			(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glBeginQuery		(GL_ANY_SAMPLES_PASSED, id);
	ctx.glEndQuery			(GL_ANY_SAMPLES_PASSED);

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not an accepted value.");
	ctx.glGetQueryObjectuiv	(id, -1, &params);
	ctx.expectError			(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if id is the name of a currently active query object.");
	ctx.glBeginQuery		(GL_ANY_SAMPLES_PASSED, id);
	ctx.expectError			(GL_NO_ERROR);
	ctx.glGetQueryObjectuiv	(id, GL_QUERY_RESULT_AVAILABLE, &params);
	ctx.expectError			(GL_INVALID_OPERATION);
	ctx.glEndQuery			(GL_ANY_SAMPLES_PASSED);
	ctx.expectError			(GL_NO_ERROR);
	ctx.endSection();

	ctx.glDeleteQueries		(1, &id);
}

// Sync object queries

void get_synciv (NegativeTestContext& ctx)
{
	GLsizei	length		= -1;
	GLint	values[32];
	GLsync	sync;

	deMemset(&values[0], 0xcd, sizeof(values));

	ctx.beginSection("GL_INVALID_VALUE is generated if sync is not the name of a sync object.");
	ctx.glGetSynciv(0, GL_OBJECT_TYPE, 32, &length, &values[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not one of the accepted tokens.");
	sync = ctx.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	ctx.expectError(GL_NO_ERROR);
	ctx.glGetSynciv(sync, -1, 32, &length, &values[0]);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.glDeleteSync(sync);

	ctx.beginSection("GL_INVALID_VALUE is generated if bufSize is negative.");
	sync = ctx.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	ctx.expectError(GL_NO_ERROR);
	ctx.glGetSynciv(sync, GL_OBJECT_TYPE, -1, &length, &values[0]);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	ctx.glDeleteSync(sync);
}

// Enumerated boolean state queries

void is_enabled (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if cap is not an accepted value.");
	ctx.glIsEnabled(-1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glIsEnabled(GL_TRIANGLES);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

void is_enabledi (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError, contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)), "This test requires a higher context version.");

	ctx.beginSection("GL_INVALID_ENUM is generated if cap is not an accepted value.");
	ctx.glIsEnabledi(-1, 1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glIsEnabledi(GL_TRIANGLES, 1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if index is outside the valid range for the indexed state cap.");
	ctx.glIsEnabledi(GL_BLEND, -1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

// Hints

void hint (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_INVALID_ENUM is generated if either target or mode is not an accepted value.");
	ctx.glHint(GL_GENERATE_MIPMAP_HINT, -1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glHint(-1, GL_FASTEST);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.glHint(-1, -1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();
}

std::vector<FunctionContainer> getNegativeStateApiTestFunctions ()
{
	const FunctionContainer funcs[] =
	{
		{enable,									"enable",									"Invalid glEnable() usage"								},
		{disable,									"disable",									"Invalid glDisable() usage"								},
		{get_booleanv,								"get_booleanv",								"Invalid glGetBooleanv() usage"							},
		{get_floatv,								"get_floatv",								"Invalid glGetFloatv() usage"							},
		{get_integerv,								"get_integerv",								"Invalid glGetIntegerv() usage"							},
		{get_integer64v,							"get_integer64v",							"Invalid glGetInteger64v() usage"						},
		{get_integeri_v,							"get_integeri_v",							"Invalid glGetIntegeri_v() usage"						},
		{get_booleani_v,							"get_booleani_v",							"Invalid glGetBooleani_v() usage"						},
		{get_integer64i_v,							"get_integer64i_v",							"Invalid glGetInteger64i_v() usage"						},
		{get_string,								"get_string",								"Invalid glGetString() usage"							},
		{get_stringi,								"get_stringi",								"Invalid glGetStringi() usage"							},
		{get_attached_shaders,						"get_attached_shaders",						"Invalid glGetAttachedShaders() usage"					},
		{get_shaderiv,								"get_shaderiv",								"Invalid glGetShaderiv() usage"							},
		{get_shader_info_log,						"get_shader_info_log",						"Invalid glGetShaderInfoLog() usage"					},
		{get_shader_precision_format,				"get_shader_precision_format",				"Invalid glGetShaderPrecisionFormat() usage"			},
		{get_shader_source,							"get_shader_source",						"Invalid glGetShaderSource() usage"						},
		{get_programiv,								"get_programiv",							"Invalid glGetProgramiv() usage"						},
		{get_program_info_log,						"get_program_info_log",						"Invalid glGetProgramInfoLog() usage"					},
		{get_tex_parameterfv,						"get_tex_parameterfv",						"Invalid glGetTexParameterfv() usage"					},
		{get_tex_parameteriv,						"get_tex_parameteriv",						"Invalid glGetTexParameteriv() usage"					},
		{get_uniformfv,								"get_uniformfv",							"Invalid glGetUniformfv() usage"						},
		{get_uniformiv,								"get_uniformiv",							"Invalid glGetUniformiv() usage"						},
		{get_uniformuiv,							"get_uniformuiv",							"Invalid glGetUniformuiv() usage"						},
		{get_active_uniform,						"get_active_uniform",						"Invalid glGetActiveUniform() usage"					},
		{get_active_uniformsiv,						"get_active_uniformsiv",					"Invalid glGetActiveUniformsiv() usage"					},
		{get_active_uniform_blockiv,				"get_active_uniform_blockiv",				"Invalid glGetActiveUniformBlockiv() usage"				},
		{get_active_uniform_block_name,				"get_active_uniform_block_name",			"Invalid glGetActiveUniformBlockName() usage"			},
		{get_active_attrib,							"get_active_attrib",						"Invalid glGetActiveAttrib() usage"						},
		{get_uniform_indices,						"get_uniform_indices",						"Invalid glGetUniformIndices() usage"					},
		{get_vertex_attribfv,						"get_vertex_attribfv",						"Invalid glGetVertexAttribfv() usage"					},
		{get_vertex_attribiv,						"get_vertex_attribiv",						"Invalid glGetVertexAttribiv() usage"					},
		{get_vertex_attribi_iv,						"get_vertex_attribi_iv",					"Invalid glGetVertexAttribIiv() usage"					},
		{get_vertex_attribi_uiv,					"get_vertex_attribi_uiv",					"Invalid glGetVertexAttribIuiv() usage"					},
		{get_vertex_attrib_pointerv,				"get_vertex_attrib_pointerv",				"Invalid glGetVertexAttribPointerv() usage"				},
		{get_frag_data_location,					"get_frag_data_location",					"Invalid glGetFragDataLocation() usage"					},
		{get_buffer_parameteriv,					"get_buffer_parameteriv",					"Invalid glGetBufferParameteriv() usage"				},
		{get_buffer_parameteri64v,					"get_buffer_parameteri64v",					"Invalid glGetBufferParameteri64v() usage"				},
		{get_buffer_pointerv,						"get_buffer_pointerv",						"Invalid glGetBufferPointerv() usage"					},
		{get_framebuffer_attachment_parameteriv,	"get_framebuffer_attachment_parameteriv",	"Invalid glGetFramebufferAttachmentParameteriv() usage"	},
		{get_renderbuffer_parameteriv,				"get_renderbuffer_parameteriv",				"Invalid glGetRenderbufferParameteriv() usage"			},
		{get_internalformativ,						"get_internalformativ",						"Invalid glGetInternalformativ() usage"					},
		{get_queryiv,								"get_queryiv",								"Invalid glGetQueryiv() usage"							},
		{get_query_objectuiv,						"get_query_objectuiv",						"Invalid glGetQueryObjectuiv() usage"					},
		{get_synciv,								"get_synciv",								"Invalid glGetSynciv() usage"							},
		{is_enabled,								"is_enabled",								"Invalid glIsEnabled() usage"							},
		{hint,										"hint",										"Invalid glHint() usage"								},
		{enablei,									"enablei",									"Invalid glEnablei() usage"								},
		{disablei,									"disablei",									"Invalid glDisablei() usage"							},
		{get_tex_parameteriiv,						"get_tex_parameteriiv",						"Invalid glGetTexParameterIiv() usage"					},
		{get_tex_parameteriuiv,						"get_tex_parameteriuiv",					"Invalid glGetTexParameterIuiv() usage"					},
		{get_nuniformfv,							"get_nuniformfv",							"Invalid glGetnUniformfv() usage"						},
		{get_nuniformiv,							"get_nuniformiv",							"Invalid glGetnUniformiv() usage"						},
		{get_nuniformuiv,							"get_nuniformuiv",							"Invalid glGetnUniformuiv() usage"						},
		{is_enabledi,								"is_enabledi",								"Invalid glIsEnabledi() usage"							},
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles3
} // deqp
