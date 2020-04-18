/*-------------------------------------------------------------------------
* drawElements Quality Program OpenGL ES 3.1 Module
* -------------------------------------------------
*
* Copyright 2016 The Android Open Source Project
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
* \brief Negative Shader Storage Buffer Object (SSBO) tests.
*//*--------------------------------------------------------------------*/
#include "es31fNegativeSSBOBlockTests.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuStringTemplate.hpp"
#include "gluShaderProgram.hpp"
#include <map>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace NegativeTestShared
{
namespace
{
using tcu::TestLog;
using glu::CallLogWrapper;
using namespace glw;
namespace args
{
enum ArgMember
{
	ARGMEMBER_FORMAT			=	0,
	ARGMEMBER_BINDING_POINT,
	ARGMEMBER_MATRIX_ORDER,
	ARGMEMBER_MEMBER_TYPE,
	ARGMEMBER_NAME,
	ARGMEMBER_FIXED_ARRAY,
	ARGMEMBER_VARIABLE_ARRAY,
	ARGMEMBER_REORDER
};

// key pair ssbo arg data
struct SsboArgData
{
	ArgMember	member;
	std::string	data;

	SsboArgData(const ArgMember& member_, const std::string& data_)
	{
		member	=	member_;
		data	=	data_;
	}
};

// class which manages string based argument used to build varying ssbo interface blocks and members
class SsboArgs
{
public:
					SsboArgs(const std::string version, tcu::TestLog& log);

	void			setSingleValue						(const SsboArgData argData);
	bool			setAllValues						(const std::vector<SsboArgData> argDataList);

	bool				getMemberReorder				(void) const;

	void				resetValues						(void);

	std::map<std::string, std::string>	populateArgsMap	(void) const;

private:
	std::string		m_negativeContextVersion;
	std::string		m_stdFormat;
	std::string		m_bindingPoint;
	std::string		m_matrixOrder;
	std::string		m_memberType;
	std::string		m_memberName;
	std::string		m_memberFixedArrayerName;
	std::string		m_memberVariableArray;
	bool			m_memberReorder;
	int				m_numberMembers;
	tcu::TestLog&	m_testLog;

	void			setDefaultValues					(void);
};

//constructor which ensure a proper context is passed into the struct
SsboArgs::SsboArgs(const std::string version, tcu::TestLog& log)
	: m_negativeContextVersion	(version)
	, m_numberMembers			(8)
	, m_testLog					(log)
{
	setDefaultValues();
}

void SsboArgs::setSingleValue (const SsboArgData argData)
{
	std::string message;

	switch (argData.member)
	{
		case ARGMEMBER_FORMAT:
			m_stdFormat					=	argData.data;
			return;
		case ARGMEMBER_BINDING_POINT:
			m_bindingPoint				=	argData.data;
			return;
		case ARGMEMBER_MATRIX_ORDER:
			m_matrixOrder				=	argData.data;
			return;
		case ARGMEMBER_MEMBER_TYPE:
			m_memberType				=	argData.data;
			return;
		case ARGMEMBER_NAME:
			m_memberName				=	argData.data;
			return;
		case ARGMEMBER_FIXED_ARRAY:
			m_memberFixedArrayerName	=	argData.data;
			return;
		case ARGMEMBER_VARIABLE_ARRAY:
			m_memberVariableArray		=	argData.data;
			return;
		case ARGMEMBER_REORDER:
			if (argData.data == "true")
			{
				m_memberReorder			=	true;
			}
			return;
		default:
			message = "auto loop argument data member not recognised.";
			m_testLog << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;
	}
}

bool SsboArgs::setAllValues (const std::vector<SsboArgData> argDataList)
{
	std::string	message;

	if ((argDataList.size() == 0) || (argDataList.size() > (size_t)m_numberMembers))
	{
		message = "set of args does not match the number of args struct changeable members.";
		m_testLog << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		for (unsigned int idx = 0; idx < argDataList.size(); idx++)
		{
			setSingleValue(argDataList[idx]);
		}
	}

	return true;
}

bool SsboArgs::getMemberReorder (void) const
{
	return m_memberReorder;
}

void SsboArgs::resetValues (void)
{
	setDefaultValues();
}

//converts SsboArgs member variable into a map object to be used by tcu::StringTemplate
std::map<std::string, std::string> SsboArgs::populateArgsMap (void) const
{
	std::map<std::string, std::string> argsMap;

	// key placeholders located at specific points in the ssbo block
	argsMap["NEGATIVE_CONTEXT_VERSION"]	=	m_negativeContextVersion;
	argsMap["STD_FORMAT"]				=	m_stdFormat;
	argsMap["BINDING_POINT"]			=	m_bindingPoint;
	argsMap["MATRIX_ORDER"]				=	m_matrixOrder;
	argsMap["MEMBER_TYPE"]				=	m_memberType;
	argsMap["MEMBER_NAME"]				=	m_memberName;
	argsMap["MEMBER_FIXED_ARRAY"]		=	m_memberFixedArrayerName;
	argsMap["MEMBER_VARIABLE_ARRAY"]	=	m_memberVariableArray;

	return argsMap;
}

// default values i.e. same shader template
void SsboArgs::setDefaultValues (void)
{
	m_stdFormat					=	"std430";
	m_bindingPoint				=	"0";
	m_matrixOrder				=	"column_major";
	m_memberType				=	"int";
	m_memberName				=	"matrix";
	m_memberFixedArrayerName	=	"10";
	m_memberVariableArray		=	"";
	m_memberReorder				=	false;
}
} // args

std::string generateVaryingSSBOShader(const glw::GLenum shaderType, const args::SsboArgs& args, tcu::TestLog& log)
{
	std::map<std::string, std::string>	argsMap;
	std::ostringstream					source;
	std::string							sourceString;
	std::stringstream					ssboString;
	std::string							message;

	if (args.getMemberReorder())
	{
		ssboString	<< "	mediump vec4 array_1[${MEMBER_FIXED_ARRAY}];\n"
					<< "	highp mat4 ${MEMBER_NAME};\n"
					<< "	lowp ${MEMBER_TYPE} data;\n"
					<< "	mediump float array_2[${MEMBER_VARIABLE_ARRAY}];\n";
	}
	else
	{
		ssboString	<< "	lowp ${MEMBER_TYPE} data;\n"
					<< "	highp mat4 ${MEMBER_NAME};\n"
					<< "	mediump vec4 array_1[${MEMBER_FIXED_ARRAY}];\n"
					<< "	mediump float array_2[${MEMBER_VARIABLE_ARRAY}];\n";
	}

	argsMap = args.populateArgsMap();

	switch (shaderType)
	{
		case GL_VERTEX_SHADER:
		{
			source	<< "${NEGATIVE_CONTEXT_VERSION}\n"
					<< "layout (location = 0) in highp vec4 position;\n"
					<< "layout (location = 1) in mediump vec4 colour;\n"
					<< "out mediump vec4 vertex_colour;\n"
					<< "layout (${STD_FORMAT}, binding = ${BINDING_POINT}, ${MATRIX_ORDER}) buffer ssbo_block\n"
					<< "{\n";

			source << ssboString.str();

			source	<< "} ssbo;\n"
					<< "void main()\n"
					<< "{\n"
					<< "	mediump vec4 variable;\n"
					<< "	gl_Position = ssbo.${MEMBER_NAME} * position;\n"
					<< "	for (int idx = 0; idx < ${MEMBER_FIXED_ARRAY}; idx++)\n"
					<< "	{\n"
					<< "		variable += ssbo.array_1[idx];\n"
					<< "	}\n"
					<< "	vertex_colour = colour + variable;\n"
					<< "}\n";

			sourceString = source.str();
			sourceString = tcu::StringTemplate(sourceString).specialize(argsMap);

			return sourceString;
		}

		case GL_FRAGMENT_SHADER:
		{
			source	<< "${NEGATIVE_CONTEXT_VERSION}\n"
					<< "in mediump vec4 vertex_colour;\n"
					<< "layout (location = 0) out mediump vec4 fragment_colour;\n"
					<< "layout (${STD_FORMAT}, binding = ${BINDING_POINT}, ${MATRIX_ORDER}) buffer ssbo_block\n"
					<< "{\n";

			source << ssboString.str();

			source	<< "} ssbo;\n"
					<< "void main()\n"
					<< "{\n"
					<< "	mediump vec4 variable;\n"
					<< "	variable * ssbo.${MEMBER_NAME};\n"
					<< "	for (int idx = 0; idx < ${MEMBER_FIXED_ARRAY}; idx++)\n"
					<< "	{\n"
					<< "		variable += ssbo.array_1[idx];\n"
					<< "	}\n"
					<< "	fragment_colour = vertex_colour + variable;\n"
					<< "}\n";

			sourceString = source.str();
			sourceString = tcu::StringTemplate(sourceString).specialize(argsMap);

			return sourceString;
		}

		case GL_GEOMETRY_SHADER:
		{
			// TODO:
			return sourceString;
		}

		case GL_TESS_CONTROL_SHADER:
		{
			// TODO:
			return sourceString;
		}

		case GL_TESS_EVALUATION_SHADER:
		{
			// TODO:
			return sourceString;
		}

		case GL_COMPUTE_SHADER:
		{
			// TODO:
			return sourceString;
		}

		default:
		{
			message = "shader type not recognised.";
			log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;
		}
	}

	return std::string();
}

void logProgramInfo(NegativeTestContext& ctx, GLint program)
{
	GLint			maxLength	=	0;
	std::string		message;
	tcu::TestLog&	log			=	ctx.getLog();

	ctx.glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

	message = "Program log:";
	log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;

	if (maxLength == 0)
	{
		message = "No available info log.";
		log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;
		return;
	}

	std::vector<GLchar> infoLog(maxLength);
	ctx.glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

	std::string programLogMessage(&infoLog[0], maxLength);
	log << tcu::TestLog::Message << programLogMessage << tcu::TestLog::EndMessage;
}

void ssbo_block_matching(NegativeTestContext& ctx)
{
	const bool				isES32													=	contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const glu::GLSLVersion	version													=	isES32 ? glu::GLSL_VERSION_320_ES : glu::GLSL_VERSION_310_ES;
	tcu::TestLog&			log														=	ctx.getLog();
	std::string				message;
	std::string				versionString(glu::getGLSLVersionDeclaration(version));
	args::SsboArgs			ssboArgs(versionString, log);
	GLint					shaderVertexGL;
	std::string				shaderVertexString;
	const char*				shaderVertexCharPtr;

	// List of arguments used to create varying ssbo objects in the fragment shader
	const args::SsboArgData argDataArrayFrag[] = {	args::SsboArgData(args::ARGMEMBER_FORMAT,			"std140"),
													args::SsboArgData(args::ARGMEMBER_BINDING_POINT,	"10"),
													args::SsboArgData(args::ARGMEMBER_MATRIX_ORDER,		"row_major"),
													args::SsboArgData(args::ARGMEMBER_MEMBER_TYPE,		"vec2"),
													args::SsboArgData(args::ARGMEMBER_NAME,				"name_changed"),
													args::SsboArgData(args::ARGMEMBER_FIXED_ARRAY,		"20"),
													args::SsboArgData(args::ARGMEMBER_VARIABLE_ARRAY,	"5"),
													args::SsboArgData(args::ARGMEMBER_REORDER,			"true") };
	std::vector<args::SsboArgData> argDataVectorFrag(argDataArrayFrag, argDataArrayFrag + sizeof(argDataArrayFrag) / sizeof(argDataArrayFrag[0]));

	// create default vertex shader
	shaderVertexString = generateVaryingSSBOShader(GL_VERTEX_SHADER, ssboArgs, log);
	shaderVertexCharPtr = shaderVertexString.c_str();
	shaderVertexGL = ctx.glCreateShader(GL_VERTEX_SHADER);

	// log
	message = shaderVertexString;
	log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;

	// compile
	ctx.glShaderSource(shaderVertexGL, 1, &shaderVertexCharPtr, DE_NULL);
	ctx.glCompileShader(shaderVertexGL);

	for (std::size_t idx = 0; idx < argDataVectorFrag.size(); ++idx)
	{
		GLint			linkStatus				=	-1;
		GLint			program;
		GLint			shaderFragmentGL;
		std::string		shaderFragmentString;
		const char*		shaderFragmentCharPtr;

		ctx.beginSection("Multiple shaders created using SSBO's sharing the same name but not matching layouts");

		program = ctx.glCreateProgram();

		// reset args to default and make a single change
		ssboArgs.resetValues();
		ssboArgs.setSingleValue(argDataVectorFrag[idx]);

		// create fragment shader
		shaderFragmentString = generateVaryingSSBOShader(GL_FRAGMENT_SHADER, ssboArgs, log);
		shaderFragmentCharPtr = shaderFragmentString.c_str();
		shaderFragmentGL = ctx.glCreateShader(GL_FRAGMENT_SHADER);

		// log
		message = shaderFragmentString;
		log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;

		// compile
		ctx.glShaderSource(shaderFragmentGL, 1, &shaderFragmentCharPtr, DE_NULL);
		ctx.glCompileShader(shaderFragmentGL);

		// attach shaders to program and attempt to link
		ctx.glAttachShader(program, shaderVertexGL);
		ctx.glAttachShader(program, shaderFragmentGL);
		ctx.glLinkProgram(program);
		ctx.glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

		logProgramInfo(ctx, program);

		if (linkStatus == GL_TRUE)
		{
			ctx.fail("Program should not have linked");
		}

		// clean up resources
		ctx.glDeleteShader(shaderFragmentGL);
		ctx.glDeleteProgram(program);

		ctx.endSection();
	}

	// clean up default resources
	ctx.glDeleteShader(shaderVertexGL);
}

void ssbo_block_shared_qualifier(NegativeTestContext& ctx)
{
	const bool				isES32													=	contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const glu::GLSLVersion	version													=	isES32 ? glu::GLSL_VERSION_320_ES : glu::GLSL_VERSION_310_ES;
	tcu::TestLog&			log														=	ctx.getLog();
	std::string				message;
	std::string				versionString(glu::getGLSLVersionDeclaration(version));
	args::SsboArgs			ssboArgs(versionString, log);
	bool					result;
	GLint					shaderVertexGL;
	std::string				shaderVertexString;
	const char*				shaderVertexCharPtr;

	// default args used in vertex shader ssbo
	const args::SsboArgData argDataArrayVert[] = {	args::SsboArgData(args::ARGMEMBER_FORMAT,			"shared"),
													args::SsboArgData(args::ARGMEMBER_BINDING_POINT,	"0"),
													args::SsboArgData(args::ARGMEMBER_MATRIX_ORDER,		"column_major"),
													args::SsboArgData(args::ARGMEMBER_FIXED_ARRAY,		"10"),
													args::SsboArgData(args::ARGMEMBER_VARIABLE_ARRAY,	"10"),
													args::SsboArgData(args::ARGMEMBER_REORDER,			"false") };
	std::vector<args::SsboArgData> argDataVectorVert(argDataArrayVert, argDataArrayVert + sizeof(argDataArrayVert) / sizeof(argDataArrayVert[0]));

	// args changed in fragment shader ssbo
	const args::SsboArgData argDataArrayFrag[] = {	args::SsboArgData(args::ARGMEMBER_MATRIX_ORDER,		"row_major"),
													args::SsboArgData(args::ARGMEMBER_VARIABLE_ARRAY,	""),
													args::SsboArgData(args::ARGMEMBER_FIXED_ARRAY,		"20") };
	std::vector<args::SsboArgData> argDataVectorFrag(argDataArrayFrag, argDataArrayFrag + sizeof(argDataArrayFrag) / sizeof(argDataArrayFrag[0]));

	// set default vertex ssbo args
	result = ssboArgs.setAllValues(argDataVectorVert);

	if (result == false)
	{
		message = "Invalid use of args.setAllValues()";
		log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;
		return;
	}

	// create default vertex shader
	shaderVertexString = generateVaryingSSBOShader(GL_VERTEX_SHADER, ssboArgs, log);
	shaderVertexCharPtr = shaderVertexString.c_str();
	shaderVertexGL = ctx.glCreateShader(GL_VERTEX_SHADER);

	// log
	message = shaderVertexString;
	log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;

	// compile
	ctx.glShaderSource(shaderVertexGL, 1, &shaderVertexCharPtr, DE_NULL);
	ctx.glCompileShader(shaderVertexGL);

	for (std::size_t idx = 0; idx < argDataVectorFrag.size(); idx++)
	{
		GLint		linkStatus				=	-1;
		GLint		program;
		GLint		shaderFragmentGL;
		std::string	shaderFragmentString;
		const char*	shaderFragmentCharPtr;

		ctx.beginSection("Multiple shaders created using SSBO's sharing the same name but not matching layouts");

		program = ctx.glCreateProgram();

		// reset args to default and make a single change
		ssboArgs.setAllValues(argDataVectorVert);
		ssboArgs.setSingleValue(argDataVectorFrag[idx]);

		// create fragment shader
		shaderFragmentString = generateVaryingSSBOShader(GL_FRAGMENT_SHADER, ssboArgs, log);
		shaderFragmentCharPtr = shaderFragmentString.c_str();
		shaderFragmentGL = ctx.glCreateShader(GL_FRAGMENT_SHADER);

		// log
		message = shaderFragmentString;
		log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;

		// compile
		ctx.glShaderSource(shaderFragmentGL, 1, &shaderFragmentCharPtr, DE_NULL);
		ctx.glCompileShader(shaderFragmentGL);

		// attach shaders to the program and attempt to link
		ctx.glAttachShader(program, shaderVertexGL);
		ctx.glAttachShader(program, shaderFragmentGL);
		ctx.glLinkProgram(program);
		ctx.glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

		logProgramInfo(ctx, program);

		if (linkStatus == GL_TRUE)
		{
			ctx.fail("Program should not have linked");
		}

		// clean up resources
		ctx.glDeleteShader(shaderFragmentGL);
		ctx.glDeleteProgram(program);

		ctx.endSection();
	}

	// clean up default resources
	ctx.glDeleteShader(shaderVertexGL);
}
} // anonymous

std::vector<FunctionContainer> getNegativeSSBOBlockTestFunctions (void)
{
	const FunctionContainer funcs[] =
	{
		{ ssbo_block_matching,			"ssbo_block_interface_matching_tests",	"Invalid Shader Linkage" },
		{ ssbo_block_shared_qualifier,	"ssbo_using_shared_qualifier_tests",	"Invalid Shader Linkage" },
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}
} // NegativeTestShared
} //Functional
} //gles31
} //deqp
