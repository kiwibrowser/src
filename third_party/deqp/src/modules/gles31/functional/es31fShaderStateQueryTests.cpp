/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
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
 * \brief Shader state query tests
 *//*--------------------------------------------------------------------*/

#include "es31fShaderStateQueryTests.hpp"
#include "es31fInfoLogQueryShared.hpp"
#include "glsStateQueryUtil.hpp"
#include "tcuTestLog.hpp"
#include "tcuStringTemplate.hpp"
#include "gluShaderProgram.hpp"
#include "gluRenderContext.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluContextInfo.hpp"
#include "gluStrUtil.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

static inline std::string brokenShaderSource (const glu::ContextType &contextType)
{
	const std::string glslVersionDecl = glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(contextType));

	return	glslVersionDecl + "\n"
			"broken, this should not compile,\n"
			"{";
}

class BaseTypeCase : public TestCase
{
public:
	struct TestTypeInfo
	{
		glw::GLenum	glType;
		const char*	declarationStr;
		const char*	accessStr;
	};

										BaseTypeCase		(Context& ctx, const char* name, const char* desc, const char* extension);

private:
	IterateResult						iterate				(void);
	virtual std::vector<TestTypeInfo>	getInfos			(void) const = 0;
	virtual void						checkRequirements	(void) const;

	const char* const					m_extension;
};

BaseTypeCase::BaseTypeCase (Context& ctx, const char* name, const char* desc, const char* extension)
	: TestCase		(ctx, name, desc)
	, m_extension	(extension)
{
}

BaseTypeCase::IterateResult BaseTypeCase::iterate (void)
{
	static const char* const	vertexSourceTemplate	=	"${VERSIONDECL}\n"
															"in highp vec4 a_position;\n"
															"void main(void)\n"
															"{\n"
															"	gl_Position = a_position;\n"
															"}\n";
	static const char* const	fragmentSourceTemplate	=	"${VERSIONDECL}\n"
															"${EXTENSIONSTATEMENT}"
															"${DECLARATIONSTR};\n"
															"layout(location = 0) out highp vec4 dEQP_FragColor;\n"
															"void main(void)\n"
															"{\n"
															"	dEQP_FragColor = vec4(${ACCESSSTR});\n"
															"}\n";

	tcu::ResultCollector		result			(m_testCtx.getLog());
	std::vector<TestTypeInfo>	samplerTypes	= getInfos();
	const bool					supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (m_extension && !m_context.getContextInfo().isExtensionSupported(m_extension) && !supportsES32)
		throw tcu::NotSupportedError("Test requires " + std::string(m_extension));
	checkRequirements();

	for (int typeNdx = 0; typeNdx < (int)samplerTypes.size(); ++typeNdx)
	{
		const tcu::ScopedLogSection			section	(m_testCtx.getLog(),
													 std::string(glu::getShaderVarTypeStr(samplerTypes[typeNdx].glType).toString()),
													 "Uniform type " + glu::getShaderVarTypeStr(samplerTypes[typeNdx].glType).toString());

		std::map<std::string, std::string>	shaderArgs;
		shaderArgs["DECLARATIONSTR"]		= samplerTypes[typeNdx].declarationStr;
		shaderArgs["ACCESSSTR"]				= samplerTypes[typeNdx].accessStr;
		shaderArgs["EXTENSIONSTATEMENT"]	= (m_extension && !supportsES32) ? (std::string() + "#extension " + m_extension + " : require\n") : ("");
		shaderArgs["VERSIONDECL"]			= glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType()));

		const std::string					fragmentSource	= tcu::StringTemplate(fragmentSourceTemplate).specialize(shaderArgs);
		const std::string					vertexSource	= tcu::StringTemplate(vertexSourceTemplate).specialize(shaderArgs);
		const glw::Functions&				gl				= m_context.getRenderContext().getFunctions();
		glu::ShaderProgram					program			(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(vertexSource) << glu::FragmentSource(fragmentSource));

		m_testCtx.getLog() << tcu::TestLog::Message << "Building program with uniform sampler of type " << glu::getShaderVarTypeStr(samplerTypes[typeNdx].glType) << tcu::TestLog::EndMessage;

		if (!program.isOk())
		{
			m_testCtx.getLog() << program;
			result.fail("could not build shader");
		}
		else
		{
			// only one uniform -- uniform at index 0
			int uniforms = 0;
			gl.getProgramiv(program.getProgram(), GL_ACTIVE_UNIFORMS, &uniforms);

			if (uniforms != 1)
				result.fail("Unexpected GL_ACTIVE_UNIFORMS, expected 1");
			else
			{
				// check type
				const glw::GLuint	uniformIndex	= 0;
				glw::GLint			type			= 0;

				m_testCtx.getLog() << tcu::TestLog::Message << "Verifying uniform type." << tcu::TestLog::EndMessage;
				gl.getActiveUniformsiv(program.getProgram(), 1, &uniformIndex, GL_UNIFORM_TYPE, &type);

				if (type != (glw::GLint)samplerTypes[typeNdx].glType)
				{
					std::ostringstream buf;
					buf << "Invalid type, expected " << samplerTypes[typeNdx].glType << ", got " << type;
					result.fail(buf.str());
				}
			}
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "");
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

void BaseTypeCase::checkRequirements (void) const
{
}

class CoreSamplerTypeCase : public BaseTypeCase
{
public:
								CoreSamplerTypeCase	(Context& ctx, const char* name, const char* desc);

private:
	std::vector<TestTypeInfo>	getInfos			(void) const;
};

CoreSamplerTypeCase::CoreSamplerTypeCase (Context& ctx, const char* name, const char* desc)
	: BaseTypeCase(ctx, name, desc, DE_NULL)
{
}

std::vector<BaseTypeCase::TestTypeInfo> CoreSamplerTypeCase::getInfos (void) const
{
	static const TestTypeInfo samplerTypes[] =
	{
		{ GL_SAMPLER_2D_MULTISAMPLE,				"uniform highp sampler2DMS u_sampler",	"texelFetch(u_sampler, ivec2(gl_FragCoord.xy), 0)" },
		{ GL_INT_SAMPLER_2D_MULTISAMPLE,			"uniform highp isampler2DMS u_sampler",	"texelFetch(u_sampler, ivec2(gl_FragCoord.xy), 0)" },
		{ GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE,	"uniform highp usampler2DMS u_sampler",	"texelFetch(u_sampler, ivec2(gl_FragCoord.xy), 0)" },
	};

	std::vector<TestTypeInfo> infos;
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(samplerTypes); ++ndx)
		infos.push_back(samplerTypes[ndx]);

	return infos;
}

class MSArraySamplerTypeCase : public BaseTypeCase
{
public:
								MSArraySamplerTypeCase	(Context& ctx, const char* name, const char* desc);

private:
	std::vector<TestTypeInfo>	getInfos				(void) const;
};

MSArraySamplerTypeCase::MSArraySamplerTypeCase (Context& ctx, const char* name, const char* desc)
	: BaseTypeCase(ctx, name, desc, "GL_OES_texture_storage_multisample_2d_array")
{
}

std::vector<BaseTypeCase::TestTypeInfo> MSArraySamplerTypeCase::getInfos (void) const
{
	static const TestTypeInfo samplerTypes[] =
	{
		{ GL_SAMPLER_2D_MULTISAMPLE_ARRAY,				"uniform highp sampler2DMSArray u_sampler",		"texelFetch(u_sampler, ivec3(gl_FragCoord.xyz), 0)" },
		{ GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,			"uniform highp isampler2DMSArray u_sampler",	"texelFetch(u_sampler, ivec3(gl_FragCoord.xyz), 0)" },
		{ GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,	"uniform highp usampler2DMSArray u_sampler",	"texelFetch(u_sampler, ivec3(gl_FragCoord.xyz), 0)" },
	};

	std::vector<TestTypeInfo> infos;
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(samplerTypes); ++ndx)
		infos.push_back(samplerTypes[ndx]);

	return infos;
}

class TextureBufferSamplerTypeCase : public BaseTypeCase
{
public:
								TextureBufferSamplerTypeCase	(Context& ctx, const char* name, const char* desc);

private:
	std::vector<TestTypeInfo>	getInfos						(void) const;
};

TextureBufferSamplerTypeCase::TextureBufferSamplerTypeCase (Context& ctx, const char* name, const char* desc)
	: BaseTypeCase(ctx, name, desc, "GL_EXT_texture_buffer")
{
}

std::vector<BaseTypeCase::TestTypeInfo> TextureBufferSamplerTypeCase::getInfos (void) const
{
	static const TestTypeInfo samplerTypes[] =
	{
		{ GL_SAMPLER_BUFFER,				"uniform highp samplerBuffer u_sampler",	"texelFetch(u_sampler, int(gl_FragCoord.x))" },
		{ GL_INT_SAMPLER_BUFFER,			"uniform highp isamplerBuffer u_sampler",	"texelFetch(u_sampler, int(gl_FragCoord.x))" },
		{ GL_UNSIGNED_INT_SAMPLER_BUFFER,	"uniform highp usamplerBuffer u_sampler",	"texelFetch(u_sampler, int(gl_FragCoord.x))" },
	};

	std::vector<TestTypeInfo> infos;
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(samplerTypes); ++ndx)
		infos.push_back(samplerTypes[ndx]);

	return infos;
}

class TextureBufferImageTypeCase : public BaseTypeCase
{
public:
								TextureBufferImageTypeCase	(Context& ctx, const char* name, const char* desc);

private:
	std::vector<TestTypeInfo>	getInfos					(void) const;
	void						checkRequirements			(void) const;
};

TextureBufferImageTypeCase::TextureBufferImageTypeCase (Context& ctx, const char* name, const char* desc)
	: BaseTypeCase(ctx, name, desc, "GL_EXT_texture_buffer")
{
}

std::vector<BaseTypeCase::TestTypeInfo> TextureBufferImageTypeCase::getInfos (void) const
{
	static const TestTypeInfo samplerTypes[] =
	{
		{ GL_IMAGE_BUFFER,				"layout(binding=0, rgba8) readonly uniform highp imageBuffer u_image",	"imageLoad(u_image, int(gl_FragCoord.x))" },
		{ GL_INT_IMAGE_BUFFER,			"layout(binding=0, r32i) readonly uniform highp iimageBuffer u_image",	"imageLoad(u_image, int(gl_FragCoord.x))" },
		{ GL_UNSIGNED_INT_IMAGE_BUFFER,	"layout(binding=0, r32ui) readonly uniform highp uimageBuffer u_image",	"imageLoad(u_image, int(gl_FragCoord.x))" },
	};

	std::vector<TestTypeInfo> infos;
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(samplerTypes); ++ndx)
		infos.push_back(samplerTypes[ndx]);

	return infos;
}

void TextureBufferImageTypeCase::checkRequirements (void) const
{
	if (m_context.getContextInfo().getInt(GL_MAX_FRAGMENT_IMAGE_UNIFORMS) < 1)
		throw tcu::NotSupportedError("Test requires fragment images");
}

class CubeArraySamplerTypeCase : public BaseTypeCase
{
public:
								CubeArraySamplerTypeCase	(Context& ctx, const char* name, const char* desc);

private:
	std::vector<TestTypeInfo>	getInfos						(void) const;
};

CubeArraySamplerTypeCase::CubeArraySamplerTypeCase (Context& ctx, const char* name, const char* desc)
	: BaseTypeCase(ctx, name, desc, "GL_EXT_texture_cube_map_array")
{
}

std::vector<BaseTypeCase::TestTypeInfo> CubeArraySamplerTypeCase::getInfos (void) const
{
	static const TestTypeInfo samplerTypes[] =
	{
		{ GL_SAMPLER_CUBE_MAP_ARRAY,				"uniform highp samplerCubeArray u_sampler",			"texture(u_sampler, gl_FragCoord.xxyz)"			},
		{ GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW,			"uniform highp samplerCubeArrayShadow u_sampler",	"texture(u_sampler, gl_FragCoord.xxyz, 0.5)"	},
		{ GL_INT_SAMPLER_CUBE_MAP_ARRAY,			"uniform highp isamplerCubeArray u_sampler",		"texture(u_sampler, gl_FragCoord.xxyz)"			},
		{ GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY,	"uniform highp usamplerCubeArray u_sampler",		"texture(u_sampler, gl_FragCoord.xxyz)"			},
	};

	std::vector<TestTypeInfo> infos;
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(samplerTypes); ++ndx)
		infos.push_back(samplerTypes[ndx]);

	return infos;
}

class CubeArrayImageTypeCase : public BaseTypeCase
{
public:
								CubeArrayImageTypeCase	(Context& ctx, const char* name, const char* desc);

private:
	std::vector<TestTypeInfo>	getInfos				(void) const;
	void						checkRequirements		(void) const;
};

CubeArrayImageTypeCase::CubeArrayImageTypeCase (Context& ctx, const char* name, const char* desc)
	: BaseTypeCase(ctx, name, desc, "GL_EXT_texture_cube_map_array")
{
}

std::vector<BaseTypeCase::TestTypeInfo> CubeArrayImageTypeCase::getInfos (void) const
{
	static const TestTypeInfo samplerTypes[] =
	{
		{ GL_IMAGE_CUBE_MAP_ARRAY,				"layout(binding=0, rgba8) readonly uniform highp imageCubeArray u_image",	"imageLoad(u_image, ivec3(gl_FragCoord.xyx))"	},
		{ GL_INT_IMAGE_CUBE_MAP_ARRAY,			"layout(binding=0, r32i) readonly uniform highp iimageCubeArray u_image",	"imageLoad(u_image, ivec3(gl_FragCoord.xyx))"	},
		{ GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY,	"layout(binding=0, r32ui) readonly uniform highp uimageCubeArray u_image",	"imageLoad(u_image, ivec3(gl_FragCoord.xyx))"	},
	};

	std::vector<TestTypeInfo> infos;
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(samplerTypes); ++ndx)
		infos.push_back(samplerTypes[ndx]);

	return infos;
}

void CubeArrayImageTypeCase::checkRequirements (void) const
{
	if (m_context.getContextInfo().getInt(GL_MAX_FRAGMENT_IMAGE_UNIFORMS) < 1)
		throw tcu::NotSupportedError("Test requires fragment images");
}

class ShaderLogCase : public TestCase
{
public:
							ShaderLogCase	(Context& ctx, const char* name, const char* desc, glu::ShaderType shaderType);

private:
	void					init			(void);
	IterateResult			iterate			(void);

	const glu::ShaderType	m_shaderType;
};

ShaderLogCase::ShaderLogCase (Context& ctx, const char* name, const char* desc, glu::ShaderType shaderType)
	: TestCase		(ctx, name, desc)
	, m_shaderType	(shaderType)
{
}

void ShaderLogCase::init (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	switch (m_shaderType)
	{
		case glu::SHADERTYPE_VERTEX:
		case glu::SHADERTYPE_FRAGMENT:
		case glu::SHADERTYPE_COMPUTE:
			break;

		case glu::SHADERTYPE_GEOMETRY:
			if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader") && !supportsES32)
				throw tcu::NotSupportedError("Test requires GL_EXT_geometry_shader extension or an OpenGL ES 3.2 or higher context.");
			break;

		case glu::SHADERTYPE_TESSELLATION_CONTROL:
		case glu::SHADERTYPE_TESSELLATION_EVALUATION:
			if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader") && !supportsES32)
				throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader extension or an OpenGL ES 3.2 or higher context.");
			break;

		default:
			DE_ASSERT(false);
			break;
	}
}

ShaderLogCase::IterateResult ShaderLogCase::iterate (void)
{
	using gls::StateQueryUtil::StateQueryMemoryWriteGuard;

	tcu::ResultCollector					result			(m_testCtx.getLog());
	glu::CallLogWrapper						gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	deUint32								shader			= 0;
	const std::string						source			= brokenShaderSource(m_context.getRenderContext().getType());
	const char* const						brokenSource	= source.c_str();
	StateQueryMemoryWriteGuard<glw::GLint>	logLen;

	gl.enableLogging(true);

	m_testCtx.getLog() << tcu::TestLog::Message << "Trying to compile broken shader source." << tcu::TestLog::EndMessage;

	shader = gl.glCreateShader(glu::getGLShaderType(m_shaderType));
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "create shader");

	gl.glShaderSource(shader, 1, &brokenSource, DE_NULL);
	gl.glCompileShader(shader);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "compile");

	gl.glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
	logLen.verifyValidity(result);

	if (!logLen.isUndefined())
		verifyInfoLogQuery(result, gl, logLen, shader, &glu::CallLogWrapper::glGetShaderInfoLog, "glGetShaderInfoLog");

	gl.glDeleteShader(shader);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "delete");

	result.setTestContextResult(m_testCtx);
	return STOP;
}

} // anonymous

ShaderStateQueryTests::ShaderStateQueryTests (Context& context)
	: TestCaseGroup(context, "shader", "Shader state query tests")
{
}

ShaderStateQueryTests::~ShaderStateQueryTests (void)
{
}

void ShaderStateQueryTests::init (void)
{
	addChild(new CoreSamplerTypeCase			(m_context, "sampler_type",						"Sampler type cases"));
	addChild(new MSArraySamplerTypeCase			(m_context, "sampler_type_multisample_array",	"MSAA array sampler type cases"));
	addChild(new TextureBufferSamplerTypeCase	(m_context, "sampler_type_texture_buffer",		"Texture buffer sampler type cases"));
	addChild(new TextureBufferImageTypeCase		(m_context, "image_type_texture_buffer",		"Texture buffer image type cases"));
	addChild(new CubeArraySamplerTypeCase		(m_context, "sampler_type_cube_array",			"Cube array sampler type cases"));
	addChild(new CubeArrayImageTypeCase			(m_context, "image_type_cube_array",			"Cube array image type cases"));

	// shader info log tests
	// \note, there exists similar tests in gles3 module. However, the gles31 could use a different
	//        shader compiler with different INFO_LOG bugs.
	{
		static const struct
		{
			const char*		caseName;
			glu::ShaderType	caseType;
		} shaderTypes[] =
		{
			{ "info_log_vertex",		glu::SHADERTYPE_VERTEX					},
			{ "info_log_fragment",		glu::SHADERTYPE_FRAGMENT				},
			{ "info_log_geometry",		glu::SHADERTYPE_GEOMETRY				},
			{ "info_log_tess_ctrl",		glu::SHADERTYPE_TESSELLATION_CONTROL	},
			{ "info_log_tess_eval",		glu::SHADERTYPE_TESSELLATION_EVALUATION	},
			{ "info_log_compute",		glu::SHADERTYPE_COMPUTE					},
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(shaderTypes); ++ndx)
			addChild(new ShaderLogCase(m_context, shaderTypes[ndx].caseName, "", shaderTypes[ndx].caseType));
	}
}

} // Functional
} // gles31
} // deqp
