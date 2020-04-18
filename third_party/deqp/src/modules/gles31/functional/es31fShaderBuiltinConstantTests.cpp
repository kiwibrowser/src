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
 * \brief Shader built-in constant tests.
 *//*--------------------------------------------------------------------*/

#include "es31fShaderBuiltinConstantTests.hpp"
#include "glsShaderExecUtil.hpp"
#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"
#include "tcuTestLog.hpp"
#include "gluStrUtil.hpp"
#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

using std::string;
using std::vector;
using tcu::TestLog;

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

static int getInteger (const glw::Functions& gl, deUint32 pname)
{
	int value = -1;
	gl.getIntegerv(pname, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), ("glGetIntegerv(" + glu::getGettableStateStr((int)pname).toString() + ")").c_str());
	return value;
}

template<deUint32 Pname>
static int getInteger (const glw::Functions& gl)
{
	return getInteger(gl, Pname);
}

static int getVectorsFromComps (const glw::Functions& gl, deUint32 pname)
{
	int value = -1;
	gl.getIntegerv(pname, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), ("glGetIntegerv(" + glu::getGettableStateStr((int)pname).toString() + ")").c_str());
	TCU_CHECK_MSG(value%4 == 0, ("Expected " + glu::getGettableStateStr((int)pname).toString() + " to be divisible by 4").c_str());
	return value/4;
}

template<deUint32 Pname>
static int getVectorsFromComps (const glw::Functions& gl)
{
	return getVectorsFromComps(gl, Pname);
}

static tcu::IVec3 getIVec3 (const glw::Functions& gl, deUint32 pname)
{
	tcu::IVec3 value(-1);
	for (int ndx = 0; ndx < 3; ndx++)
		gl.getIntegeri_v(pname, (glw::GLuint)ndx, &value[ndx]);
	GLU_EXPECT_NO_ERROR(gl.getError(), ("glGetIntegeri_v(" + glu::getGettableStateStr((int)pname).toString() + ")").c_str());
	return value;
}

template<deUint32 Pname>
static tcu::IVec3 getIVec3 (const glw::Functions& gl)
{
	return getIVec3(gl, Pname);
}

static std::string makeCaseName (const std::string& varName)
{
	DE_ASSERT(varName.length() > 3);
	DE_ASSERT(varName.substr(0,3) == "gl_");

	std::ostringstream name;
	name << de::toLower(char(varName[3]));

	for (size_t ndx = 4; ndx < varName.length(); ndx++)
	{
		const char c = char(varName[ndx]);
		if (de::isUpper(c))
			name << '_' << de::toLower(c);
		else
			name << c;
	}

	return name.str();
}

enum
{
	VS = (1<<glu::SHADERTYPE_VERTEX),
	TC = (1<<glu::SHADERTYPE_TESSELLATION_CONTROL),
	TE = (1<<glu::SHADERTYPE_TESSELLATION_EVALUATION),
	GS = (1<<glu::SHADERTYPE_GEOMETRY),
	FS = (1<<glu::SHADERTYPE_FRAGMENT),
	CS = (1<<glu::SHADERTYPE_COMPUTE),

	SHADER_TYPES = VS|TC|TE|GS|FS|CS
};

template<typename DataType>
class ShaderBuiltinConstantCase : public TestCase
{
public:
	typedef DataType (*GetConstantValueFunc) (const glw::Functions& gl);

								ShaderBuiltinConstantCase	(Context& context, const char* varName, GetConstantValueFunc getValue, const char* requiredExt);
								~ShaderBuiltinConstantCase	(void);

	void						init						(void);
	IterateResult				iterate						(void);

private:
	bool						verifyInShaderType			(glu::ShaderType shaderType, DataType reference);

	const std::string			m_varName;
	const GetConstantValueFunc	m_getValue;
	const std::string			m_requiredExt;
};

template<typename DataType>
ShaderBuiltinConstantCase<DataType>::ShaderBuiltinConstantCase (Context& context, const char* varName, GetConstantValueFunc getValue, const char* requiredExt)
	: TestCase		(context, makeCaseName(varName).c_str(), varName)
	, m_varName		(varName)
	, m_getValue	(getValue)
	, m_requiredExt	(requiredExt ? requiredExt : "")
{
	DE_ASSERT(!requiredExt == m_requiredExt.empty());
}

template<typename DataType>
ShaderBuiltinConstantCase<DataType>::~ShaderBuiltinConstantCase (void)
{
}

template<typename DataType>
void ShaderBuiltinConstantCase<DataType>::init (void)
{
	const bool supportsES32 = contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (m_requiredExt == "GL_OES_sample_variables" || m_requiredExt == "GL_EXT_geometry_shader" || m_requiredExt == "GL_EXT_tessellation_shader")
	{
		if(!supportsES32)
		{
			const std::string message = "The test requires a 3.2 context or support for the extension " + m_requiredExt + ".";
			TCU_CHECK_AND_THROW(NotSupportedError, m_context.getContextInfo().isExtensionSupported(m_requiredExt.c_str()), message.c_str());
		}
	}
	else if (!m_requiredExt.empty() && !m_context.getContextInfo().isExtensionSupported(m_requiredExt.c_str()))
			throw tcu::NotSupportedError(m_requiredExt + " not supported");

	if (!supportsES32 && (m_varName == "gl_MaxTessControlImageUniforms"	||
		m_varName == "gl_MaxTessEvaluationImageUniforms"			||
		m_varName == "gl_MaxTessControlAtomicCounters"				||
		m_varName == "gl_MaxTessEvaluationAtomicCounters"			||
		m_varName == "gl_MaxTessControlAtomicCounterBuffers"		||
		m_varName == "gl_MaxTessEvaluationAtomicCounterBuffers"))
	{
		std::string message = "The test requires a 3.2 context. The constant '" + m_varName + "' is not supported.";
		TCU_THROW(NotSupportedError, message.c_str());
	}
}

static gls::ShaderExecUtil::ShaderExecutor* createGetConstantExecutor (const glu::RenderContext&	renderCtx,
																	   glu::ShaderType				shaderType,
																	   glu::DataType				dataType,
																	   const std::string&			varName,
																	   const std::string&			extName)
{
	using namespace gls::ShaderExecUtil;

	const bool	supportsES32	= contextSupports(renderCtx.getType(), glu::ApiType::es(3, 2));
	ShaderSpec	shaderSpec;

	shaderSpec.version	= supportsES32 ? glu::GLSL_VERSION_320_ES : glu::GLSL_VERSION_310_ES;
	shaderSpec.source	= string("result = ") + varName + ";\n";

	shaderSpec.outputs.push_back(Symbol("result", glu::VarType(dataType, glu::PRECISION_HIGHP)));

	if (!extName.empty() && !(supportsES32 && (extName == "GL_OES_sample_variables" || extName == "GL_EXT_geometry_shader" || extName == "GL_EXT_tessellation_shader")))
		shaderSpec.globalDeclarations = "#extension " + extName + " : require\n";

	return createExecutor(renderCtx, shaderType, shaderSpec);
}

template<typename DataType>
static void logVarValue (tcu::TestLog& log, const std::string& varName, DataType value)
{
	log << TestLog::Message << varName << " = " << value << TestLog::EndMessage;
}

template<>
void logVarValue<int> (tcu::TestLog& log, const std::string& varName, int value)
{
	log << TestLog::Integer(varName, varName, "", QP_KEY_TAG_NONE, value);
}

template<typename DataType>
bool ShaderBuiltinConstantCase<DataType>::verifyInShaderType (glu::ShaderType shaderType, DataType reference)
{
	using namespace gls::ShaderExecUtil;

	const de::UniquePtr<ShaderExecutor>	shaderExecutor	(createGetConstantExecutor(m_context.getRenderContext(), shaderType, glu::dataTypeOf<DataType>(), m_varName, m_requiredExt));
	DataType							result			= DataType(-1);
	void* const							outputs			= &result;

	if (!shaderExecutor->isOk())
	{
		shaderExecutor->log(m_testCtx.getLog());
		TCU_FAIL("Compile failed");
	}

	shaderExecutor->useProgram();
	shaderExecutor->execute(1, DE_NULL, &outputs);

	logVarValue(m_testCtx.getLog(), m_varName, result);

	if (result != reference)
	{
		m_testCtx.getLog() << TestLog::Message << "ERROR: Expected " << m_varName << " = " << reference << TestLog::EndMessage
						   << TestLog::Message << "Test shader:" << TestLog::EndMessage;
		shaderExecutor->log(m_testCtx.getLog());
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid builtin constant value");
		return false;
	}
	else
		return true;
}

template<typename DataType>
TestCase::IterateResult ShaderBuiltinConstantCase<DataType>::iterate (void)
{
	const DataType	reference	= m_getValue(m_context.getRenderContext().getFunctions());

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; shaderType++)
	{
		if ((SHADER_TYPES & (1<<shaderType)) != 0)
		{
			const char* const			shaderTypeName	= glu::getShaderTypeName(glu::ShaderType(shaderType));
			const tcu::ScopedLogSection	section			(m_testCtx.getLog(), shaderTypeName, shaderTypeName);

			try
			{
				const bool isOk = verifyInShaderType(glu::ShaderType(shaderType), reference);
				DE_ASSERT(isOk || m_testCtx.getTestResult() != QP_TEST_RESULT_PASS);
				DE_UNREF(isOk);
			}
			catch (const tcu::NotSupportedError& e)
			{
				m_testCtx.getLog() << TestLog::Message << "Not checking " << shaderTypeName << ": " << e.what() << TestLog::EndMessage;
			}
			catch (const tcu::TestError& e)
			{
				m_testCtx.getLog() << TestLog::Message << e.what() << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, e.getMessage());
			}
		}
	}

	return STOP;
}

} // anonymous

ShaderBuiltinConstantTests::ShaderBuiltinConstantTests (Context& context)
	: TestCaseGroup(context, "builtin_constants", "Built-in Constant Tests")
{
}

ShaderBuiltinConstantTests::~ShaderBuiltinConstantTests (void)
{
}

void ShaderBuiltinConstantTests::init (void)
{
	// Core builtin constants
	{
		static const struct
		{
			const char*												varName;
			ShaderBuiltinConstantCase<int>::GetConstantValueFunc	getValue;
		} intConstants[] =
		{
			{ "gl_MaxVertexAttribs",					getInteger<GL_MAX_VERTEX_ATTRIBS>						},
			{ "gl_MaxVertexUniformVectors",				getInteger<GL_MAX_VERTEX_UNIFORM_VECTORS>				},
			{ "gl_MaxVertexOutputVectors",				getVectorsFromComps<GL_MAX_VERTEX_OUTPUT_COMPONENTS>	},
			{ "gl_MaxFragmentInputVectors",				getVectorsFromComps<GL_MAX_FRAGMENT_INPUT_COMPONENTS>	},
			{ "gl_MaxFragmentUniformVectors",			getInteger<GL_MAX_FRAGMENT_UNIFORM_VECTORS>				},
			{ "gl_MaxDrawBuffers",						getInteger<GL_MAX_DRAW_BUFFERS>							},

			{ "gl_MaxVertexTextureImageUnits",			getInteger<GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS>			},
			{ "gl_MaxCombinedTextureImageUnits",		getInteger<GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS>			},
			{ "gl_MaxTextureImageUnits",				getInteger<GL_MAX_TEXTURE_IMAGE_UNITS>					},

			{ "gl_MinProgramTexelOffset",				getInteger<GL_MIN_PROGRAM_TEXEL_OFFSET>					},
			{ "gl_MaxProgramTexelOffset",				getInteger<GL_MAX_PROGRAM_TEXEL_OFFSET>					},

			{ "gl_MaxImageUnits",						getInteger<GL_MAX_IMAGE_UNITS>							},
			{ "gl_MaxVertexImageUniforms",				getInteger<GL_MAX_VERTEX_IMAGE_UNIFORMS>				},
			{ "gl_MaxFragmentImageUniforms",			getInteger<GL_MAX_FRAGMENT_IMAGE_UNIFORMS>				},
			{ "gl_MaxComputeImageUniforms",				getInteger<GL_MAX_COMPUTE_IMAGE_UNIFORMS>				},
			{ "gl_MaxCombinedImageUniforms",			getInteger<GL_MAX_COMBINED_IMAGE_UNIFORMS>				},

			{ "gl_MaxCombinedShaderOutputResources",	getInteger<GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES>		},

			{ "gl_MaxComputeUniformComponents",			getInteger<GL_MAX_COMPUTE_UNIFORM_COMPONENTS>			},
			{ "gl_MaxComputeTextureImageUnits",			getInteger<GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS>			},

			{ "gl_MaxComputeAtomicCounters",			getInteger<GL_MAX_COMPUTE_ATOMIC_COUNTERS>				},
			{ "gl_MaxComputeAtomicCounterBuffers",		getInteger<GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS>		},

			{ "gl_MaxVertexAtomicCounters",				getInteger<GL_MAX_VERTEX_ATOMIC_COUNTERS>				},
			{ "gl_MaxFragmentAtomicCounters",			getInteger<GL_MAX_FRAGMENT_ATOMIC_COUNTERS>				},
			{ "gl_MaxCombinedAtomicCounters",			getInteger<GL_MAX_COMBINED_ATOMIC_COUNTERS>				},
			{ "gl_MaxAtomicCounterBindings",			getInteger<GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS>		},

			{ "gl_MaxVertexAtomicCounterBuffers",		getInteger<GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS>		},
			{ "gl_MaxFragmentAtomicCounterBuffers",		getInteger<GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS>		},
			{ "gl_MaxCombinedAtomicCounterBuffers",		getInteger<GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS>		},
			{ "gl_MaxAtomicCounterBufferSize",			getInteger<GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE>			},
		};

		static const struct
		{
			const char*													varName;
			ShaderBuiltinConstantCase<tcu::IVec3>::GetConstantValueFunc	getValue;
		} ivec3Constants[] =
		{
			{ "gl_MaxComputeWorkGroupCount",			getIVec3<GL_MAX_COMPUTE_WORK_GROUP_COUNT>				},
			{ "gl_MaxComputeWorkGroupSize",				getIVec3<GL_MAX_COMPUTE_WORK_GROUP_SIZE>				},
		};

		tcu::TestCaseGroup* const coreGroup = new tcu::TestCaseGroup(m_testCtx, "core", "Core Specification");
		addChild(coreGroup);

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(intConstants); ndx++)
			coreGroup->addChild(new ShaderBuiltinConstantCase<int>(m_context, intConstants[ndx].varName, intConstants[ndx].getValue, DE_NULL));

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(ivec3Constants); ndx++)
			coreGroup->addChild(new ShaderBuiltinConstantCase<tcu::IVec3>(m_context, ivec3Constants[ndx].varName, ivec3Constants[ndx].getValue, DE_NULL));
	}

	// OES_sample_variables
	{
		tcu::TestCaseGroup* const sampleVarGroup = new tcu::TestCaseGroup(m_testCtx, "sample_variables", "GL_OES_sample_variables");
		addChild(sampleVarGroup);
		sampleVarGroup->addChild(new ShaderBuiltinConstantCase<int>(m_context, "gl_MaxSamples", getInteger<GL_MAX_SAMPLES>, "GL_OES_sample_variables"));
	}

	// EXT_geometry_shader
	{
		static const struct
		{
			const char*												varName;
			ShaderBuiltinConstantCase<int>::GetConstantValueFunc	getValue;
		} intConstants[] =
		{
			{ "gl_MaxGeometryInputComponents",			getInteger<GL_MAX_GEOMETRY_INPUT_COMPONENTS>			},
			{ "gl_MaxGeometryOutputComponents",			getInteger<GL_MAX_GEOMETRY_OUTPUT_COMPONENTS>			},
			{ "gl_MaxGeometryImageUniforms",			getInteger<GL_MAX_GEOMETRY_IMAGE_UNIFORMS>				},
			{ "gl_MaxGeometryTextureImageUnits",		getInteger<GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS>			},
			{ "gl_MaxGeometryOutputVertices",			getInteger<GL_MAX_GEOMETRY_OUTPUT_VERTICES>				},
			{ "gl_MaxGeometryTotalOutputComponents",	getInteger<GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS>		},
			{ "gl_MaxGeometryUniformComponents",		getInteger<GL_MAX_GEOMETRY_UNIFORM_COMPONENTS>			},
			{ "gl_MaxGeometryAtomicCounters",			getInteger<GL_MAX_GEOMETRY_ATOMIC_COUNTERS>				},
			{ "gl_MaxGeometryAtomicCounterBuffers",		getInteger<GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS>		},
		};

		tcu::TestCaseGroup* const geomGroup = new tcu::TestCaseGroup(m_testCtx, "geometry_shader", "GL_EXT_geometry_shader");
		addChild(geomGroup);

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(intConstants); ndx++)
			geomGroup->addChild(new ShaderBuiltinConstantCase<int>(m_context, intConstants[ndx].varName, intConstants[ndx].getValue, "GL_EXT_geometry_shader"));
	}

	// EXT_tessellation_shader
	{
		static const struct
		{
			const char*												varName;
			ShaderBuiltinConstantCase<int>::GetConstantValueFunc	getValue;
		} intConstants[] =
		{
			{ "gl_MaxTessControlInputComponents",			getInteger<GL_MAX_TESS_CONTROL_INPUT_COMPONENTS>			},
			{ "gl_MaxTessControlOutputComponents",			getInteger<GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS>			},
			{ "gl_MaxTessControlTextureImageUnits",			getInteger<GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS>			},
			{ "gl_MaxTessControlUniformComponents",			getInteger<GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS>			},
			{ "gl_MaxTessControlTotalOutputComponents",		getInteger<GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS>		},

			{ "gl_MaxTessControlImageUniforms",				getInteger<GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS>				},
			{ "gl_MaxTessEvaluationImageUniforms",			getInteger<GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS>			},
			{ "gl_MaxTessControlAtomicCounters",			getInteger<GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS>				},
			{ "gl_MaxTessEvaluationAtomicCounters",			getInteger<GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS>			},
			{ "gl_MaxTessControlAtomicCounterBuffers",		getInteger<GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS>		},
			{ "gl_MaxTessEvaluationAtomicCounterBuffers",	getInteger<GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS>	},

			{ "gl_MaxTessEvaluationInputComponents",		getInteger<GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS>			},
			{ "gl_MaxTessEvaluationOutputComponents",		getInteger<GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS>		},
			{ "gl_MaxTessEvaluationTextureImageUnits",		getInteger<GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS>		},
			{ "gl_MaxTessEvaluationUniformComponents",		getInteger<GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS>		},

			{ "gl_MaxTessPatchComponents",					getInteger<GL_MAX_TESS_PATCH_COMPONENTS>					},

			{ "gl_MaxPatchVertices",						getInteger<GL_MAX_PATCH_VERTICES>							},
			{ "gl_MaxTessGenLevel",							getInteger<GL_MAX_TESS_GEN_LEVEL>							},
		};

		tcu::TestCaseGroup* const tessGroup = new tcu::TestCaseGroup(m_testCtx, "tessellation_shader", "GL_EXT_tessellation_shader");
		addChild(tessGroup);

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(intConstants); ndx++)
			tessGroup->addChild(new ShaderBuiltinConstantCase<int>(m_context, intConstants[ndx].varName, intConstants[ndx].getValue, "GL_EXT_tessellation_shader"));
	}
}

} // Functional
} // gles31
} // deqp
