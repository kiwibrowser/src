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
 * \brief Tests for separate shader objects
 *//*--------------------------------------------------------------------*/

#include "es31fSeparateShaderTests.hpp"

#include "deInt32.h"
#include "deString.h"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "deRandom.hpp"
#include "deSTLUtil.hpp"
#include "tcuCommandLine.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuResultCollector.hpp"
#include "tcuRGBA.hpp"
#include "tcuSurface.hpp"
#include "tcuStringTemplate.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluPixelTransfer.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"
#include "gluVarType.hpp"
#include "glsShaderLibrary.hpp"
#include "glwFunctions.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"

#include <cstdarg>
#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <set>
#include <vector>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using std::map;
using std::set;
using std::ostringstream;
using std::string;
using std::vector;
using de::MovePtr;
using de::Random;
using de::UniquePtr;
using tcu::MessageBuilder;
using tcu::RenderTarget;
using tcu::StringTemplate;
using tcu::Surface;
using tcu::TestLog;
using tcu::ResultCollector;
using glu::CallLogWrapper;
using glu::DataType;
using glu::VariableDeclaration;
using glu::Precision;
using glu::Program;
using glu::ProgramPipeline;
using glu::ProgramSources;
using glu::RenderContext;
using glu::ShaderProgram;
using glu::ShaderType;
using glu::Storage;
using glu::VarType;
using glu::VertexSource;
using glu::FragmentSource;
using glu::ProgramSeparable;

using namespace glw;

#define LOG_CALL(CALL) do	\
{							\
	enableLogging(true);	\
	CALL;					\
	enableLogging(false);	\
} while (deGetFalse())

enum
{
	VIEWPORT_SIZE = 128
};

enum VaryingInterpolation
{
	VARYINGINTERPOLATION_SMOOTH		= 0,
	VARYINGINTERPOLATION_FLAT,
	VARYINGINTERPOLATION_CENTROID,
	VARYINGINTERPOLATION_DEFAULT,
	VARYINGINTERPOLATION_RANDOM,

	VARYINGINTERPOLATION_LAST
};

DataType randomType (Random& rnd)
{
	using namespace glu;

	if (rnd.getInt(0, 7) == 0)
	{
		const int numCols = rnd.getInt(2, 4), numRows = rnd.getInt(2, 4);

		return getDataTypeMatrix(numCols, numRows);
	}
	else
	{
		static const DataType	s_types[]	= { TYPE_FLOAT,	TYPE_INT,	TYPE_UINT	};
		static const float		s_weights[] = { 3.0,		1.0,		1.0			};
		const int				size		= rnd.getInt(1, 4);
		const DataType			scalarType	= rnd.chooseWeighted<DataType>(
			DE_ARRAY_BEGIN(s_types), DE_ARRAY_END(s_types), DE_ARRAY_BEGIN(s_weights));
		return getDataTypeVector(scalarType, size);
	}

	DE_FATAL("Impossible");
	return TYPE_INVALID;
}

VaryingInterpolation randomInterpolation (Random& rnd)
{
	static const VaryingInterpolation s_validInterpolations[] =
	{
		VARYINGINTERPOLATION_SMOOTH,
		VARYINGINTERPOLATION_FLAT,
		VARYINGINTERPOLATION_CENTROID,
		VARYINGINTERPOLATION_DEFAULT,
	};
	return s_validInterpolations[rnd.getInt(0, DE_LENGTH_OF_ARRAY(s_validInterpolations)-1)];
}

glu::Interpolation getGluInterpolation (VaryingInterpolation interpolation)
{
	switch (interpolation)
	{
		case VARYINGINTERPOLATION_SMOOTH:	return glu::INTERPOLATION_SMOOTH;
		case VARYINGINTERPOLATION_FLAT:		return glu::INTERPOLATION_FLAT;
		case VARYINGINTERPOLATION_CENTROID:	return glu::INTERPOLATION_CENTROID;
		case VARYINGINTERPOLATION_DEFAULT:	return glu::INTERPOLATION_LAST;		//!< Last means no qualifier, i.e. default
		default:
			DE_FATAL("Invalid interpolation");
			return glu::INTERPOLATION_LAST;
	}
}

// used only for debug sanity checks
#if defined(DE_DEBUG)
VaryingInterpolation getVaryingInterpolation (glu::Interpolation interpolation)
{
	switch (interpolation)
	{
		case glu::INTERPOLATION_SMOOTH:		return VARYINGINTERPOLATION_SMOOTH;
		case glu::INTERPOLATION_FLAT:		return VARYINGINTERPOLATION_FLAT;
		case glu::INTERPOLATION_CENTROID:	return VARYINGINTERPOLATION_CENTROID;
		case glu::INTERPOLATION_LAST:		return VARYINGINTERPOLATION_DEFAULT;		//!< Last means no qualifier, i.e. default
		default:
			DE_FATAL("Invalid interpolation");
			return VARYINGINTERPOLATION_LAST;
	}
}
#endif

enum BindingKind
{
	BINDING_NAME,
	BINDING_LOCATION,
	BINDING_LAST
};

BindingKind randomBinding (Random& rnd)
{
	return rnd.getBool() ? BINDING_LOCATION : BINDING_NAME;
}

void printInputColor (ostringstream& oss, const VariableDeclaration& input)
{
	using namespace glu;

	const DataType	basicType	= input.varType.getBasicType();
	string			exp			= input.name;

	switch (getDataTypeScalarType(basicType))
	{
		case TYPE_FLOAT:
			break;

		case TYPE_INT:
		case TYPE_UINT:
		{
			DataType floatType = getDataTypeFloatScalars(basicType);
			exp = string() + "(" + getDataTypeName(floatType) + "(" + exp + ") / 255.0" + ")";
			break;
		}

		default:
			DE_FATAL("Impossible");
	}

	if (isDataTypeScalarOrVector(basicType))
	{
		switch (getDataTypeScalarSize(basicType))
		{
			case 1:
				oss << "hsv(vec3(" << exp << ", 1.0, 1.0))";
				break;
			case 2:
				oss << "hsv(vec3(" << exp << ", 1.0))";
				break;
			case 3:
				oss << "vec4(" << exp << ", 1.0)";
				break;
			case 4:
				oss << exp;
				break;
			default:
				DE_FATAL("Impossible");
		}
	}
	else if (isDataTypeMatrix(basicType))
	{
		int	rows	= getDataTypeMatrixNumRows(basicType);
		int	columns	= getDataTypeMatrixNumColumns(basicType);

		if (rows == columns)
			oss << "hsv(vec3(determinant(" << exp << ")))";
		else
		{
			if (rows != 3 && columns >= 3)
			{
				exp = "transpose(" + exp + ")";
				std::swap(rows, columns);
			}
			exp = exp + "[0]";
			if (rows > 3)
				exp = exp + ".xyz";
			oss << "hsv(" << exp << ")";
		}
	}
	else
		DE_FATAL("Impossible");
}

// Representation for the varyings between vertex and fragment shaders

struct VaryingParams
{
	VaryingParams			(void)
		: count				(0)
		, type				(glu::TYPE_LAST)
		, binding			(BINDING_LAST)
		, vtxInterp			(VARYINGINTERPOLATION_LAST)
		, frgInterp			(VARYINGINTERPOLATION_LAST) {}

	int						count;
	DataType				type;
	BindingKind				binding;
	VaryingInterpolation	vtxInterp;
	VaryingInterpolation	frgInterp;
};

struct VaryingInterface
{
	vector<VariableDeclaration>	vtxOutputs;
	vector<VariableDeclaration>	frgInputs;
};

// Generate corresponding input and output variable declarations that may vary
// in compatible ways.

VaryingInterpolation chooseInterpolation (VaryingInterpolation param, DataType type, Random& rnd)
{
	if (glu::getDataTypeScalarType(type) != glu::TYPE_FLOAT)
		return VARYINGINTERPOLATION_FLAT;

	if (param == VARYINGINTERPOLATION_RANDOM)
		return randomInterpolation(rnd);

	return param;
}

bool isSSOCompatibleInterpolation (VaryingInterpolation vertexInterpolation, VaryingInterpolation fragmentInterpolation)
{
	// interpolations must be fully specified
	DE_ASSERT(vertexInterpolation != VARYINGINTERPOLATION_RANDOM);
	DE_ASSERT(vertexInterpolation < VARYINGINTERPOLATION_LAST);
	DE_ASSERT(fragmentInterpolation != VARYINGINTERPOLATION_RANDOM);
	DE_ASSERT(fragmentInterpolation < VARYINGINTERPOLATION_LAST);

	// interpolation can only be either smooth or flat. Auxiliary storage does not matter.
	const bool isSmoothVtx =    (vertexInterpolation == VARYINGINTERPOLATION_SMOOTH)      || //!< trivial
	                            (vertexInterpolation == VARYINGINTERPOLATION_DEFAULT)     || //!< default to smooth
	                            (vertexInterpolation == VARYINGINTERPOLATION_CENTROID);      //!< default to smooth, ignore storage
	const bool isSmoothFrag =   (fragmentInterpolation == VARYINGINTERPOLATION_SMOOTH)    || //!< trivial
	                            (fragmentInterpolation == VARYINGINTERPOLATION_DEFAULT)   || //!< default to smooth
	                            (fragmentInterpolation == VARYINGINTERPOLATION_CENTROID);    //!< default to smooth, ignore storage
	// Khronos bug #12630: flat / smooth qualifiers must match in SSO
	return isSmoothVtx == isSmoothFrag;
}

VaryingInterface genVaryingInterface (const VaryingParams&		params,
									  Random&					rnd)
{
	using namespace	glu;

	VaryingInterface	ret;
	int					offset = 0;

	for (int varNdx = 0; varNdx < params.count; ++varNdx)
	{
		const BindingKind			binding			= ((params.binding == BINDING_LAST)
													   ? randomBinding(rnd) : params.binding);
		const DataType				type			= ((params.type == TYPE_LAST)
													   ? randomType(rnd) : params.type);
		const VaryingInterpolation	vtxInterp		= chooseInterpolation(params.vtxInterp, type, rnd);
		const VaryingInterpolation	frgInterp		= chooseInterpolation(params.frgInterp, type, rnd);
		const VaryingInterpolation	vtxCompatInterp	= (isSSOCompatibleInterpolation(vtxInterp, frgInterp))
													   ? (vtxInterp) : (frgInterp);
		const int					loc				= ((binding == BINDING_LOCATION) ? offset : -1);
		const string				ndxStr			= de::toString(varNdx);
		const string				vtxName			= ((binding == BINDING_NAME)
													   ? "var" + ndxStr : "vtxVar" + ndxStr);
		const string				frgName			= ((binding == BINDING_NAME)
													   ? "var" + ndxStr : "frgVar" + ndxStr);
		const VarType				varType			(type, PRECISION_HIGHP);

		offset += getDataTypeNumLocations(type);

		// Over 16 locations aren't necessarily supported, so halt here.
		if (offset > 16)
			break;

		ret.vtxOutputs.push_back(
			VariableDeclaration(varType, vtxName, STORAGE_OUT, getGluInterpolation(vtxCompatInterp), loc));
		ret.frgInputs.push_back(
			VariableDeclaration(varType, frgName, STORAGE_IN, getGluInterpolation(frgInterp), loc));
	}

	return ret;
}

// Create vertex output variable declarations that are maximally compatible
// with the fragment input variables.

vector<VariableDeclaration> varyingCompatVtxOutputs (const VaryingInterface& varyings)
{
	vector<VariableDeclaration> outputs = varyings.vtxOutputs;

	for (size_t i = 0; i < outputs.size(); ++i)
	{
		outputs[i].interpolation = varyings.frgInputs[i].interpolation;
		outputs[i].name = varyings.frgInputs[i].name;
	}

	return outputs;
}

// Shader source generation

void printFloat (ostringstream& oss, double d)
{
	oss.setf(oss.fixed | oss.internal);
	oss.precision(4);
	oss.width(7);
	oss << d;
}

void printFloatDeclaration (ostringstream&	oss,
							const string&	varName,
							bool			uniform,
							GLfloat			value		= 0.0)
{
	using namespace glu;

	const VarType	varType	(TYPE_FLOAT, PRECISION_HIGHP);

	if (uniform)
		oss << VariableDeclaration(varType, varName, STORAGE_UNIFORM) << ";\n";
	else
		oss << VariableDeclaration(varType, varName, STORAGE_CONST)
			<< " = " << de::floatToString(value, 6) << ";\n";
}

void printRandomInitializer (ostringstream& oss, DataType type, Random& rnd)
{
	using namespace glu;
	const int		size	= getDataTypeScalarSize(type);

	if (size > 0)
		oss << getDataTypeName(type) << "(";

	for (int i = 0; i < size; ++i)
	{
		oss << (i == 0 ? "" : ", ");
		switch (getDataTypeScalarType(type))
		{
			case TYPE_FLOAT:
				printFloat(oss, rnd.getInt(0, 16) / 16.0);
				break;

			case TYPE_INT:
			case TYPE_UINT:
				oss << rnd.getInt(0, 255);
				break;

			case TYPE_BOOL:
				oss << (rnd.getBool() ? "true" : "false");
				break;

			default:
				DE_FATAL("Impossible");
		}
	}

	if (size > 0)
		oss << ")";
}

string genVtxShaderSrc (deUint32							seed,
						const vector<VariableDeclaration>&	outputs,
						const string&						varName,
						bool								uniform,
						float								value = 0.0)
{
	ostringstream		oss;
	Random				rnd								(seed);
	enum {				NUM_COMPONENTS					= 2 };
	static const int	s_quadrants[][NUM_COMPONENTS]	= { {1, 1}, {-1, 1}, {1, -1} };

	oss << "#version 310 es\n";

	printFloatDeclaration(oss, varName, uniform, value);

	for (vector<VariableDeclaration>::const_iterator it = outputs.begin();
		 it != outputs.end(); ++it)
		oss << *it << ";\n";

	oss << "const vec2 triangle[3] = vec2[3](\n";

	for (int vertexNdx = 0; vertexNdx < DE_LENGTH_OF_ARRAY(s_quadrants); ++vertexNdx)
	{
		oss << "\tvec2(";

		for (int componentNdx = 0; componentNdx < NUM_COMPONENTS; ++componentNdx)
		{
			printFloat(oss, s_quadrants[vertexNdx][componentNdx] * rnd.getInt(4,16) / 16.0);
			oss << (componentNdx < 1 ? ", " : "");
		}

		oss << ")" << (vertexNdx < 2 ? "," : "") << "\n";
	}
	oss << ");\n";


	for (vector<VariableDeclaration>::const_iterator it = outputs.begin();
		 it != outputs.end(); ++it)
	{
		const DataType	type		= it->varType.getBasicType();
		const string	typeName	= glu::getDataTypeName(type);

		oss << "const " << typeName << " " << it->name << "Inits[3] = "
			<< typeName << "[3](\n";
		for (int i = 0; i < 3; ++i)
		{
			oss << (i == 0 ? "\t" : ",\n\t");
			printRandomInitializer(oss, type, rnd);
		}
		oss << ");\n";
	}

	oss << "void main (void)\n"
		<< "{\n"
		<< "\tgl_Position = vec4(" << varName << " * triangle[gl_VertexID], 0.0, 1.0);\n";

	for (vector<VariableDeclaration>::const_iterator it = outputs.begin();
		 it != outputs.end(); ++it)
		oss << "\t" << it->name << " = " << it->name << "Inits[gl_VertexID];\n";

	oss << "}\n";

	return oss.str();
}

string genFrgShaderSrc (deUint32							seed,
						const vector<VariableDeclaration>&	inputs,
						const string&						varName,
						bool								uniform,
						float								value = 0.0)
{
	Random				rnd		(seed);
	ostringstream		oss;

	oss.precision(4);
	oss.width(7);
	oss << "#version 310 es\n";

	oss << "precision highp float;\n";

	oss << "out vec4 fragColor;\n";

	printFloatDeclaration(oss, varName, uniform, value);

	for (vector<VariableDeclaration>::const_iterator it = inputs.begin();
		 it != inputs.end(); ++it)
		oss << *it << ";\n";

	// glsl % isn't defined for negative numbers
	oss << "int imod (int n, int d)" << "\n"
		<< "{" << "\n"
		<< "\t" << "return (n < 0 ? d - 1 - (-1 - n) % d : n % d);" << "\n"
		<< "}" << "\n";

	oss << "vec4 hsv (vec3 hsv)"
		<< "{" << "\n"
		<< "\tfloat h = hsv.x * 3.0;\n"
		<< "\tfloat r = max(0.0, 1.0 - h) + max(0.0, h - 2.0);\n"
		<< "\tfloat g = max(0.0, 1.0 - abs(h - 1.0));\n"
		<< "\tfloat b = max(0.0, 1.0 - abs(h - 2.0));\n"
		<< "\tvec3 hs = mix(vec3(1.0), vec3(r, g, b), hsv.y);\n"
		<< "\treturn vec4(hsv.z * hs, 1.0);\n"
		<< "}\n";

	oss << "void main (void)\n"
		<< "{\n";

	oss << "\t" << "fragColor = vec4(vec3(" << varName << "), 1.0);" << "\n";

	if (inputs.size() > 0)
	{
		oss << "\t"
			<< "switch (imod(int(0.5 * (";

		printFloat(oss, rnd.getFloat(0.5f, 2.0f));
		oss << " * gl_FragCoord.x - ";

		printFloat(oss, rnd.getFloat(0.5f, 2.0f));
		oss << " * gl_FragCoord.y)), "
			<< inputs.size() << "))" << "\n"
			<< "\t" << "{" << "\n";

		for (size_t i = 0; i < inputs.size(); ++i)
		{
			oss << "\t\t" << "case " << i << ":" << "\n"
				<< "\t\t\t" << "fragColor *= ";

			printInputColor(oss, inputs[i]);

			oss << ";" << "\n"
				<< "\t\t\t" << "break;" << "\n";
		}

		oss << "\t\t" << "case " << inputs.size() << ":\n"
			<< "\t\t\t" << "fragColor = vec4(1.0, 0.0, 1.0, 1.0);" << "\n";
		oss << "\t\t\t" << "break;" << "\n";

		oss << "\t\t" << "case -1:\n"
			<< "\t\t\t" << "fragColor = vec4(1.0, 1.0, 0.0, 1.0);" << "\n";
		oss << "\t\t\t" << "break;" << "\n";

		oss << "\t\t" << "default:" << "\n"
			<< "\t\t\t" << "fragColor = vec4(1.0, 1.0, 0.0, 1.0);" << "\n";

		oss << "\t" << "}\n";

	}

	oss << "}\n";

	return oss.str();
}

// ProgramWrapper

class ProgramWrapper
{
public:
	virtual			~ProgramWrapper			(void) {}

	virtual GLuint	getProgramName			(void) = 0;
	virtual void	writeToLog				(TestLog& log) = 0;
};

class ShaderProgramWrapper : public ProgramWrapper
{
public:
					ShaderProgramWrapper	(const RenderContext&	renderCtx,
											 const ProgramSources&	sources)
						: m_shaderProgram	(renderCtx, sources) {}
					~ShaderProgramWrapper	(void) {}

	GLuint			getProgramName			(void) { return m_shaderProgram.getProgram(); }
	ShaderProgram&	getShaderProgram		(void) { return m_shaderProgram; }
	void			writeToLog				(TestLog& log) { log << m_shaderProgram; }

private:
	ShaderProgram	m_shaderProgram;
};

class RawProgramWrapper : public ProgramWrapper
{
public:
					RawProgramWrapper		(const RenderContext&	renderCtx,
											 GLuint					programName,
											 ShaderType				shaderType,
											 const string&			source)
						: m_program			(renderCtx, programName)
						, m_shaderType		(shaderType)
						, m_source			(source) {}
					~RawProgramWrapper		(void) {}

	GLuint			getProgramName			(void) { return m_program.getProgram(); }
	Program&		getProgram				(void) { return m_program; }
	void			writeToLog				(TestLog& log);

private:
	Program			m_program;
	ShaderType		m_shaderType;
	const string	m_source;
};

void RawProgramWrapper::writeToLog (TestLog& log)
{
	const string	info	= m_program.getInfoLog();
	qpShaderType	qpType	= glu::getLogShaderType(m_shaderType);

	log << TestLog::ShaderProgram(true, info)
		<< TestLog::Shader(qpType, m_source,
						   true, "[Shader created by glCreateShaderProgramv()]")
		<< TestLog::EndShaderProgram;
}

// ProgramParams

struct ProgramParams
{
	ProgramParams (deUint32 vtxSeed_, GLfloat vtxScale_, deUint32 frgSeed_, GLfloat frgScale_)
		: vtxSeed	(vtxSeed_)
		, vtxScale	(vtxScale_)
		, frgSeed	(frgSeed_)
		, frgScale	(frgScale_) {}
	deUint32	vtxSeed;
	GLfloat		vtxScale;
	deUint32	frgSeed;
	GLfloat		frgScale;
};

ProgramParams genProgramParams (Random& rnd)
{
	const deUint32	vtxSeed		= rnd.getUint32();
	const GLfloat	vtxScale	= (float)rnd.getInt(8, 16) / 16.0f;
	const deUint32	frgSeed		= rnd.getUint32();
	const GLfloat	frgScale	= (float)rnd.getInt(0, 16) / 16.0f;

	return ProgramParams(vtxSeed, vtxScale, frgSeed, frgScale);
}

// TestParams

struct TestParams
{
	bool					initSingle;
	bool					switchVtx;
	bool					switchFrg;
	bool					useUniform;
	bool					useSameName;
	bool					useCreateHelper;
	bool					useProgramUniform;
	VaryingParams			varyings;
};

deUint32 paramsSeed (const TestParams& params)
{
	deUint32 paramCode	= (params.initSingle			<< 0 |
						   params.switchVtx				<< 1 |
						   params.switchFrg				<< 2 |
						   params.useUniform			<< 3 |
						   params.useSameName			<< 4 |
						   params.useCreateHelper		<< 5 |
						   params.useProgramUniform		<< 6);

	paramCode = deUint32Hash(paramCode) + params.varyings.count;
	paramCode = deUint32Hash(paramCode) + params.varyings.type;
	paramCode = deUint32Hash(paramCode) + params.varyings.binding;
	paramCode = deUint32Hash(paramCode) + params.varyings.vtxInterp;
	paramCode = deUint32Hash(paramCode) + params.varyings.frgInterp;

	return deUint32Hash(paramCode);
}

string paramsCode (const TestParams& params)
{
	using namespace glu;

	ostringstream oss;

	oss << (params.initSingle ? "1" : "2")
		<< (params.switchVtx ? "v" : "")
		<< (params.switchFrg ? "f" : "")
		<< (params.useProgramUniform ? "p" : "")
		<< (params.useUniform ? "u" : "")
		<< (params.useSameName ? "s" : "")
		<< (params.useCreateHelper ? "c" : "")
		 << de::toString(params.varyings.count)
		 << (params.varyings.binding == BINDING_NAME ? "n" :
			 params.varyings.binding == BINDING_LOCATION ? "l" :
			 params.varyings.binding == BINDING_LAST ? "r" :
			"")
		 << (params.varyings.vtxInterp == VARYINGINTERPOLATION_SMOOTH ? "m" :
			 params.varyings.vtxInterp == VARYINGINTERPOLATION_CENTROID ? "e" :
			 params.varyings.vtxInterp == VARYINGINTERPOLATION_FLAT ? "a" :
			 params.varyings.vtxInterp == VARYINGINTERPOLATION_RANDOM ? "r" :
			"o")
		 << (params.varyings.frgInterp == VARYINGINTERPOLATION_SMOOTH ? "m" :
			 params.varyings.frgInterp == VARYINGINTERPOLATION_CENTROID ? "e" :
			 params.varyings.frgInterp == VARYINGINTERPOLATION_FLAT ? "a" :
			 params.varyings.frgInterp == VARYINGINTERPOLATION_RANDOM ? "r" :
			"o");
	return oss.str();
}

bool paramsValid (const TestParams& params)
{
	using namespace glu;

	// Final pipeline has a single program?
	if (params.initSingle)
	{
		// Cannot have conflicting names for uniforms or constants
		if (params.useSameName)
			return false;

		// CreateShaderProgram would never get called
		if (!params.switchVtx && !params.switchFrg && params.useCreateHelper)
			return false;

		// Must switch either all or nothing
		if (params.switchVtx != params.switchFrg)
			return false;
	}

	// ProgramUniform would never get called
	if (params.useProgramUniform && !params.useUniform)
		return false;

	// Interpolation is meaningless if we don't use an in/out variable.
	if (params.varyings.count == 0 &&
		!(params.varyings.vtxInterp == VARYINGINTERPOLATION_LAST &&
		  params.varyings.frgInterp == VARYINGINTERPOLATION_LAST))
		return false;

	// Mismatch by flat / smooth is not allowed. See Khronos bug #12630
	// \note: iterpolations might be RANDOM, causing generated varyings potentially match / mismatch anyway.
	//        This is checked later on. Here, we just make sure that we don't force the generator to generate
	//        only invalid varying configurations, i.e. there exists a valid varying configuration for this
	//        test param config.
	if ((params.varyings.vtxInterp != VARYINGINTERPOLATION_RANDOM) &&
		(params.varyings.frgInterp != VARYINGINTERPOLATION_RANDOM) &&
		(params.varyings.vtxInterp == VARYINGINTERPOLATION_FLAT) != (params.varyings.frgInterp == VARYINGINTERPOLATION_FLAT))
		return false;

	return true;
}

// used only for debug sanity checks
#if defined(DE_DEBUG)
bool varyingsValid (const VaryingInterface& varyings)
{
	for (int ndx = 0; ndx < (int)varyings.vtxOutputs.size(); ++ndx)
	{
		const VaryingInterpolation vertexInterpolation		= getVaryingInterpolation(varyings.vtxOutputs[ndx].interpolation);
		const VaryingInterpolation fragmentInterpolation	= getVaryingInterpolation(varyings.frgInputs[ndx].interpolation);

		if (!isSSOCompatibleInterpolation(vertexInterpolation, fragmentInterpolation))
			return false;
	}

	return true;
}
#endif

void logParams (TestLog& log, const TestParams& params)
{
	// We don't log operational details here since those are shown
	// in the log messages during execution.
	MessageBuilder msg = log.message();

	msg << "Pipeline configuration:\n";

	msg << "Vertex and fragment shaders have "
		<< (params.useUniform ? "uniform" : "constant") << "s with "
		<< (params.useSameName ? "the same name" : "different names") << ".\n";

	if (params.varyings.count == 0)
		msg << "There are no varyings.\n";
	else
	{
		if (params.varyings.count == 1)
			msg << "There is one varying.\n";
		else
			msg << "There are " << params.varyings.count << " varyings.\n";

		if (params.varyings.type == glu::TYPE_LAST)
			msg << "Varyings are of random types.\n";
		else
			msg << "Varyings are of type '"
				<< glu::getDataTypeName(params.varyings.type) << "'.\n";

		msg << "Varying outputs and inputs correspond ";
		switch (params.varyings.binding)
		{
			case BINDING_NAME:
				msg << "by name.\n";
				break;
			case BINDING_LOCATION:
				msg << "by location.\n";
				break;
			case BINDING_LAST:
				msg << "randomly either by name or by location.\n";
				break;
			default:
				DE_FATAL("Impossible");
		}

		msg << "In the vertex shader the varyings are qualified ";
		if (params.varyings.vtxInterp == VARYINGINTERPOLATION_DEFAULT)
			msg << "with no interpolation qualifiers.\n";
		else if (params.varyings.vtxInterp == VARYINGINTERPOLATION_RANDOM)
			msg << "with a random interpolation qualifier.\n";
		else
			msg << "'" << glu::getInterpolationName(getGluInterpolation(params.varyings.vtxInterp)) << "'.\n";

		msg << "In the fragment shader the varyings are qualified ";
		if (params.varyings.frgInterp == VARYINGINTERPOLATION_DEFAULT)
			msg << "with no interpolation qualifiers.\n";
		else if (params.varyings.frgInterp == VARYINGINTERPOLATION_RANDOM)
			msg << "with a random interpolation qualifier.\n";
		else
			msg << "'" << glu::getInterpolationName(getGluInterpolation(params.varyings.frgInterp)) << "'.\n";
	}

	msg << TestLog::EndMessage;

	log.writeMessage("");
}

TestParams genParams (deUint32 seed)
{
	Random		rnd		(seed);
	TestParams	params;
	int			tryNdx	= 0;

	do
	{
		params.initSingle			= rnd.getBool();
		params.switchVtx			= rnd.getBool();
		params.switchFrg			= rnd.getBool();
		params.useUniform			= rnd.getBool();
		params.useProgramUniform	= params.useUniform && rnd.getBool();
		params.useCreateHelper		= rnd.getBool();
		params.useSameName			= rnd.getBool();
		{
			int i = rnd.getInt(-1, 3);
			params.varyings.count = (i == -1 ? 0 : 1 << i);
		}
		if (params.varyings.count > 0)
		{
			params.varyings.type		= glu::TYPE_LAST;
			params.varyings.binding		= BINDING_LAST;
			params.varyings.vtxInterp	= VARYINGINTERPOLATION_RANDOM;
			params.varyings.frgInterp	= VARYINGINTERPOLATION_RANDOM;
		}
		else
		{
			params.varyings.type		= glu::TYPE_INVALID;
			params.varyings.binding		= BINDING_LAST;
			params.varyings.vtxInterp	= VARYINGINTERPOLATION_LAST;
			params.varyings.frgInterp	= VARYINGINTERPOLATION_LAST;
		}

		tryNdx += 1;
	} while (!paramsValid(params) && tryNdx < 16);

	DE_ASSERT(paramsValid(params));

	return params;
}

// Program pipeline wrapper that retains references to component programs.

struct Pipeline
{
								Pipeline			(MovePtr<ProgramPipeline>	pipeline_,
													 MovePtr<ProgramWrapper>	fullProg_,
													 MovePtr<ProgramWrapper>	vtxProg_,
													 MovePtr<ProgramWrapper>	frgProg_)
									: pipeline	(pipeline_)
									, fullProg	(fullProg_)
									, vtxProg	(vtxProg_)
									, frgProg	(frgProg_) {}

	ProgramWrapper&				getVertexProgram	(void) const
	{
		return vtxProg ? *vtxProg : *fullProg;
	}

	ProgramWrapper&				getFragmentProgram	(void) const
	{
		return frgProg ? *frgProg : *fullProg;
	}

	UniquePtr<ProgramPipeline>	pipeline;
	UniquePtr<ProgramWrapper>	fullProg;
	UniquePtr<ProgramWrapper>	vtxProg;
	UniquePtr<ProgramWrapper>	frgProg;
};

void logPipeline(TestLog& log, const Pipeline& pipeline)
{
	ProgramWrapper&	vtxProg	= pipeline.getVertexProgram();
	ProgramWrapper&	frgProg	= pipeline.getFragmentProgram();

	log.writeMessage("// Failed program pipeline:");
	if (&vtxProg == &frgProg)
	{
		log.writeMessage("// Common program for both vertex and fragment stages:");
		vtxProg.writeToLog(log);
	}
	else
	{
		log.writeMessage("// Vertex stage program:");
		vtxProg.writeToLog(log);
		log.writeMessage("// Fragment stage program:");
		frgProg.writeToLog(log);
	}
}

// Rectangle

struct Rectangle
{
			Rectangle	(int x_, int y_, int width_, int height_)
				: x			(x_)
				, y			(y_)
				, width		(width_)
				, height	(height_) {}
	int	x;
	int	y;
	int	width;
	int	height;
};

void setViewport (const RenderContext& renderCtx, const Rectangle& rect)
{
	renderCtx.getFunctions().viewport(rect.x, rect.y, rect.width, rect.height);
}

void readRectangle (const RenderContext& renderCtx, const Rectangle& rect, Surface& dst)
{
	dst.setSize(rect.width, rect.height);
	glu::readPixels(renderCtx, rect.x, rect.y, dst.getAccess());
}

Rectangle randomViewport (const RenderContext& ctx, Random& rnd,
						  GLint maxWidth, GLint maxHeight)
{
	const RenderTarget&	target	= ctx.getRenderTarget();
	GLint				width	= de::min(target.getWidth(), maxWidth);
	GLint				xOff	= rnd.getInt(0, target.getWidth() - width);
	GLint				height	= de::min(target.getHeight(), maxHeight);
	GLint				yOff	= rnd.getInt(0, target.getHeight() - height);

	return Rectangle(xOff, yOff, width, height);
}

// SeparateShaderTest

class SeparateShaderTest : public TestCase, private CallLogWrapper
{
public:
	typedef	void			(SeparateShaderTest::*TestFunc)
														(MovePtr<Pipeline>&		pipeOut);

							SeparateShaderTest			(Context&				ctx,
														 const string&			name,
														 const string&			description,
														 int					iterations,
														 const TestParams&		params,
														 TestFunc				testFunc);

	IterateResult			iterate						(void);

	void					testPipelineRendering		(MovePtr<Pipeline>&		pipeOut);
	void					testCurrentProgPriority		(MovePtr<Pipeline>&		pipeOut);
	void					testActiveProgramUniform	(MovePtr<Pipeline>&		pipeOut);
	void					testPipelineQueryActive		(MovePtr<Pipeline>&		pipeOut);
	void					testPipelineQueryPrograms	(MovePtr<Pipeline>&		pipeOut);

private:
	TestLog&				log							(void);
	const RenderContext&	getRenderContext			(void);

	void					setUniform					(ProgramWrapper&		program,
														 const string&			uniformName,
														 GLfloat				value,
														 bool					useProgramUni);

	void					drawSurface					(Surface&				dst,
														 deUint32				seed = 0);

	MovePtr<ProgramWrapper>	createShaderProgram			(const string*			vtxSource,
														 const string*			frgSource,
														 bool					separable);

	MovePtr<ProgramWrapper>	createSingleShaderProgram	(ShaderType			shaderType,
														 const string&			src);

	MovePtr<Pipeline>		createPipeline				(const ProgramParams&	pp);

	MovePtr<ProgramWrapper>	createReferenceProgram		(const ProgramParams&	pp);

	int						m_iterations;
	int						m_currentIteration;
	TestParams				m_params;
	TestFunc				m_testFunc;
	Random					m_rnd;
	ResultCollector			m_status;
	VaryingInterface		m_varyings;

	// Per-iteration state required for logging on exception
	MovePtr<ProgramWrapper>	m_fullProg;
	MovePtr<ProgramWrapper>	m_vtxProg;
	MovePtr<ProgramWrapper>	m_frgProg;

};

const RenderContext& SeparateShaderTest::getRenderContext (void)
{
	return m_context.getRenderContext();
}

TestLog& SeparateShaderTest::log (void)
{
	return m_testCtx.getLog();
}

SeparateShaderTest::SeparateShaderTest (Context&			ctx,
										const string&		name,
										const string&		description,
										int					iterations,
										const TestParams&	params,
										TestFunc			testFunc)
	: TestCase			(ctx, name.c_str(), description.c_str())
	, CallLogWrapper	(ctx.getRenderContext().getFunctions(), log())
	, m_iterations		(iterations)
	, m_currentIteration(0)
	, m_params			(params)
	, m_testFunc		(testFunc)
	, m_rnd				(paramsSeed(params))
	, m_status			(log(), "// ")
	, m_varyings		(genVaryingInterface(params.varyings, m_rnd))
{
	DE_ASSERT(paramsValid(params));
	DE_ASSERT(varyingsValid(m_varyings));
}

MovePtr<ProgramWrapper> SeparateShaderTest::createShaderProgram (const string*	vtxSource,
																 const string*	frgSource,
																 bool			separable)
{
	ProgramSources sources;

	if (vtxSource != DE_NULL)
		sources << VertexSource(*vtxSource);
	if (frgSource != DE_NULL)
		sources << FragmentSource(*frgSource);
	sources << ProgramSeparable(separable);

	MovePtr<ShaderProgramWrapper> wrapper (new ShaderProgramWrapper(getRenderContext(),
																	sources));
	if (!wrapper->getShaderProgram().isOk())
	{
		log().writeMessage("Couldn't create shader program");
		wrapper->writeToLog(log());
		TCU_FAIL("Couldn't create shader program");
	}

	return MovePtr<ProgramWrapper>(wrapper.release());
}

MovePtr<ProgramWrapper> SeparateShaderTest::createSingleShaderProgram (ShaderType shaderType,
																	   const string& src)
{
	const RenderContext&	renderCtx	= getRenderContext();

	if (m_params.useCreateHelper)
	{
		const char*	const	srcStr		= src.c_str();
		const GLenum		glType		= glu::getGLShaderType(shaderType);
		const GLuint		programName	= glCreateShaderProgramv(glType, 1, &srcStr);

		if (glGetError() != GL_NO_ERROR || programName == 0)
		{
			qpShaderType qpType = glu::getLogShaderType(shaderType);

			log() << TestLog::Message << "glCreateShaderProgramv() failed"
				  << TestLog::EndMessage
				  << TestLog::ShaderProgram(false, "[glCreateShaderProgramv() failed]")
				  << TestLog::Shader(qpType, src,
									 false, "[glCreateShaderProgramv() failed]")
				  << TestLog::EndShaderProgram;
			TCU_FAIL("glCreateShaderProgramv() failed");
		}

		RawProgramWrapper* const	wrapper	= new RawProgramWrapper(renderCtx, programName,
																	shaderType, src);
		MovePtr<ProgramWrapper>		wrapperPtr(wrapper);
		Program&					program = wrapper->getProgram();

		if (!program.getLinkStatus())
		{
			log().writeMessage("glCreateShaderProgramv() failed at linking");
			wrapper->writeToLog(log());
			TCU_FAIL("glCreateShaderProgram() failed at linking");
		}
		return wrapperPtr;
	}
	else
	{
		switch (shaderType)
		{
			case glu::SHADERTYPE_VERTEX:
				return createShaderProgram(&src, DE_NULL, true);
			case glu::SHADERTYPE_FRAGMENT:
				return createShaderProgram(DE_NULL, &src, true);
			default:
				DE_FATAL("Impossible case");
		}
	}
	return MovePtr<ProgramWrapper>(); // Shut up compiler warnings.
}

void SeparateShaderTest::setUniform (ProgramWrapper&	program,
									 const string&		uniformName,
									 GLfloat			value,
									 bool				useProgramUniform)
{
	const GLuint		progName	= program.getProgramName();
	const GLint			location	= glGetUniformLocation(progName, uniformName.c_str());
	MessageBuilder		msg			= log().message();

	msg << "// Set program " << progName << "'s uniform '" << uniformName << "' to " << value;
	if (useProgramUniform)
	{
		msg << " using glProgramUniform1f";
		glProgramUniform1f(progName, location, value);
	}
	else
	{
		msg << " using glUseProgram and glUniform1f";
		glUseProgram(progName);
		glUniform1f(location, value);
		glUseProgram(0);
	}
	msg << TestLog::EndMessage;
}

MovePtr<Pipeline> SeparateShaderTest::createPipeline (const ProgramParams& pp)
{
	const bool		useUniform	= m_params.useUniform;
	const string	vtxName		= m_params.useSameName ? "scale" : "vtxScale";
	const deUint32	initVtxSeed	= m_params.switchVtx ? m_rnd.getUint32() : pp.vtxSeed;

	const string	frgName		= m_params.useSameName ? "scale" : "frgScale";
	const deUint32	initFrgSeed	= m_params.switchFrg ? m_rnd.getUint32() : pp.frgSeed;
	const string	frgSource	= genFrgShaderSrc(initFrgSeed, m_varyings.frgInputs,
												  frgName, useUniform, pp.frgScale);

	const RenderContext&		renderCtx	= getRenderContext();
	MovePtr<ProgramPipeline>	pipeline	(new ProgramPipeline(renderCtx));
	MovePtr<ProgramWrapper>		fullProg;
	MovePtr<ProgramWrapper>		vtxProg;
	MovePtr<ProgramWrapper>		frgProg;

	// We cannot allow a situation where we have a single program with a
	// single uniform, because then the vertex and fragment shader uniforms
	// would not be distinct in the final pipeline, and we are going to test
	// that altering one uniform will not affect the other.
	DE_ASSERT(!(m_params.initSingle	&& m_params.useSameName &&
				!m_params.switchVtx && !m_params.switchFrg));

	if (m_params.initSingle)
	{
		string vtxSource = genVtxShaderSrc(initVtxSeed,
										   varyingCompatVtxOutputs(m_varyings),
										   vtxName, useUniform, pp.vtxScale);
		fullProg = createShaderProgram(&vtxSource, &frgSource, true);
		pipeline->useProgramStages(GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT,
								   fullProg->getProgramName());
		log() << TestLog::Message
			  << "// Created pipeline " << pipeline->getPipeline()
			  << " with two-shader program " << fullProg->getProgramName()
			  << TestLog::EndMessage;
	}
	else
	{
		string vtxSource = genVtxShaderSrc(initVtxSeed, m_varyings.vtxOutputs,
										   vtxName, useUniform, pp.vtxScale);
		vtxProg = createSingleShaderProgram(glu::SHADERTYPE_VERTEX, vtxSource);
		pipeline->useProgramStages(GL_VERTEX_SHADER_BIT, vtxProg->getProgramName());

		frgProg = createSingleShaderProgram(glu::SHADERTYPE_FRAGMENT, frgSource);
		pipeline->useProgramStages(GL_FRAGMENT_SHADER_BIT, frgProg->getProgramName());

		log() << TestLog::Message
			  << "// Created pipeline " << pipeline->getPipeline()
			  << " with vertex program " << vtxProg->getProgramName()
			  << " and fragment program " << frgProg->getProgramName()
			  << TestLog::EndMessage;
	}

	m_status.check(pipeline->isValid(),
				   "Pipeline is invalid after initialization");

	if (m_params.switchVtx)
	{
		string newSource = genVtxShaderSrc(pp.vtxSeed, m_varyings.vtxOutputs,
										   vtxName, useUniform, pp.vtxScale);
		vtxProg = createSingleShaderProgram(glu::SHADERTYPE_VERTEX, newSource);
		pipeline->useProgramStages(GL_VERTEX_SHADER_BIT, vtxProg->getProgramName());
		log() << TestLog::Message
			  << "// Switched pipeline " << pipeline->getPipeline()
			  << "'s vertex stage to single-shader program " << vtxProg->getProgramName()
			  << TestLog::EndMessage;
	}
	if (m_params.switchFrg)
	{
		string newSource = genFrgShaderSrc(pp.frgSeed, m_varyings.frgInputs,
										   frgName, useUniform, pp.frgScale);
		frgProg = createSingleShaderProgram(glu::SHADERTYPE_FRAGMENT, newSource);
		pipeline->useProgramStages(GL_FRAGMENT_SHADER_BIT, frgProg->getProgramName());
		log() << TestLog::Message
			  << "// Switched pipeline " << pipeline->getPipeline()
			  << "'s fragment stage to single-shader program " << frgProg->getProgramName()
			  << TestLog::EndMessage;
	}

	if (m_params.switchVtx || m_params.switchFrg)
		m_status.check(pipeline->isValid(),
					   "Pipeline became invalid after changing a stage's program");

	if (m_params.useUniform)
	{
		ProgramWrapper& vtxStage = *(vtxProg ? vtxProg : fullProg);
		ProgramWrapper& frgStage = *(frgProg ? frgProg : fullProg);

		setUniform(vtxStage, vtxName, pp.vtxScale, m_params.useProgramUniform);
		setUniform(frgStage, frgName, pp.frgScale, m_params.useProgramUniform);
	}
	else
		log().writeMessage("// Programs use constants instead of uniforms");

	return MovePtr<Pipeline>(new Pipeline(pipeline, fullProg, vtxProg, frgProg));
}

MovePtr<ProgramWrapper> SeparateShaderTest::createReferenceProgram (const ProgramParams& pp)
{
	bool					useUniform	= m_params.useUniform;
	const string			vtxSrc		= genVtxShaderSrc(pp.vtxSeed,
														  varyingCompatVtxOutputs(m_varyings),
														  "vtxScale", useUniform, pp.vtxScale);
	const string			frgSrc		= genFrgShaderSrc(pp.frgSeed, m_varyings.frgInputs,
														  "frgScale", useUniform, pp.frgScale);
	MovePtr<ProgramWrapper>	program		= createShaderProgram(&vtxSrc, &frgSrc, false);
	GLuint					progName	= program->getProgramName();

	log() << TestLog::Message
		  << "// Created monolithic shader program " << progName
		  << TestLog::EndMessage;

	if (useUniform)
	{
		setUniform(*program, "vtxScale", pp.vtxScale, false);
		setUniform(*program, "frgScale", pp.frgScale, false);
	}

	return program;
}

void SeparateShaderTest::drawSurface (Surface& dst, deUint32 seed)
{
	const RenderContext&	renderCtx	= getRenderContext();
	Random					rnd			(seed > 0 ? seed : m_rnd.getUint32());
	Rectangle				viewport	= randomViewport(renderCtx, rnd,
														 VIEWPORT_SIZE, VIEWPORT_SIZE);
	glClearColor(0.125f, 0.25f, 0.5f, 1.f);
	setViewport(renderCtx, viewport);
	glClear(GL_COLOR_BUFFER_BIT);
	GLU_CHECK_CALL(glDrawArrays(GL_TRIANGLES, 0, 3));
	readRectangle(renderCtx, viewport, dst);
	log().writeMessage("// Drew a triangle");
}

void SeparateShaderTest::testPipelineRendering (MovePtr<Pipeline>& pipeOut)
{
	ProgramParams				pp			= genProgramParams(m_rnd);
	Pipeline&					pipeline	= *(pipeOut = createPipeline(pp));
	GLuint						pipeName	= pipeline.pipeline->getPipeline();
	UniquePtr<ProgramWrapper>	refProgram	(createReferenceProgram(pp));
	GLuint						refProgName	= refProgram->getProgramName();
	Surface						refSurface;
	Surface						pipelineSurface;
	GLuint						drawSeed	= m_rnd.getUint32();

	glUseProgram(refProgName);
	log() << TestLog::Message << "// Use program " << refProgName << TestLog::EndMessage;
	drawSurface(refSurface, drawSeed);
	glUseProgram(0);

	glBindProgramPipeline(pipeName);
	log() << TestLog::Message << "// Bind pipeline " << pipeName << TestLog::EndMessage;
	drawSurface(pipelineSurface, drawSeed);
	glBindProgramPipeline(0);

	{
		const bool result = tcu::fuzzyCompare(
			m_testCtx.getLog(), "Program pipeline result",
			"Result of comparing a program pipeline with a monolithic program",
			refSurface, pipelineSurface, 0.05f, tcu::COMPARE_LOG_RESULT);

		m_status.check(result, "Pipeline rendering differs from equivalent monolithic program");
	}
}

void SeparateShaderTest::testCurrentProgPriority (MovePtr<Pipeline>& pipeOut)
{
	ProgramParams				pipePp		= genProgramParams(m_rnd);
	ProgramParams				programPp	= genProgramParams(m_rnd);
	Pipeline&					pipeline	= *(pipeOut = createPipeline(pipePp));
	GLuint						pipeName	= pipeline.pipeline->getPipeline();
	UniquePtr<ProgramWrapper>	program		(createReferenceProgram(programPp));
	Surface						pipelineSurface;
	Surface						refSurface;
	Surface						resultSurface;
	deUint32					drawSeed	= m_rnd.getUint32();

	LOG_CALL(glBindProgramPipeline(pipeName));
	drawSurface(pipelineSurface, drawSeed);
	LOG_CALL(glBindProgramPipeline(0));

	LOG_CALL(glUseProgram(program->getProgramName()));
	drawSurface(refSurface, drawSeed);
	LOG_CALL(glUseProgram(0));

	LOG_CALL(glUseProgram(program->getProgramName()));
	LOG_CALL(glBindProgramPipeline(pipeName));
	drawSurface(resultSurface, drawSeed);
	LOG_CALL(glBindProgramPipeline(0));
	LOG_CALL(glUseProgram(0));

	bool result = tcu::pixelThresholdCompare(
		m_testCtx.getLog(), "Active program rendering result",
		"Active program rendering result",
		refSurface, resultSurface, tcu::RGBA(0, 0, 0, 0), tcu::COMPARE_LOG_RESULT);

	m_status.check(result, "glBindProgramPipeline() affects glUseProgram()");
	if (!result)
		log() << TestLog::Image("Pipeline image", "Image produced by pipeline",
								pipelineSurface);
}

void SeparateShaderTest::testActiveProgramUniform (MovePtr<Pipeline>& pipeOut)
{
	ProgramParams				refPp			= genProgramParams(m_rnd);
	Surface						refSurface;
	Surface						resultSurface;
	deUint32					drawSeed		= m_rnd.getUint32();

	DE_UNREF(pipeOut);
	{
		UniquePtr<ProgramWrapper>	refProg		(createReferenceProgram(refPp));
		GLuint						refProgName	= refProg->getProgramName();

		glUseProgram(refProgName);
		log() << TestLog::Message << "// Use reference program " << refProgName
			  << TestLog::EndMessage;
		drawSurface(refSurface, drawSeed);
		glUseProgram(0);
	}

	{
		ProgramParams				changePp	= genProgramParams(m_rnd);
		changePp.vtxSeed						= refPp.vtxSeed;
		changePp.frgSeed						= refPp.frgSeed;
		UniquePtr<ProgramWrapper>	changeProg	(createReferenceProgram(changePp));
		GLuint						changeName	= changeProg->getProgramName();
		ProgramPipeline				pipeline	(getRenderContext());
		GLint						vtxLoc		= glGetUniformLocation(changeName, "vtxScale");
		GLint						frgLoc		= glGetUniformLocation(changeName, "frgScale");

		LOG_CALL(glBindProgramPipeline(pipeline.getPipeline()));

		pipeline.activeShaderProgram(changeName);
		log() << TestLog::Message << "// Set active shader program to " << changeName
			  << TestLog::EndMessage;

		glUniform1f(vtxLoc, refPp.vtxScale);
		log() << TestLog::Message
			  << "// Set uniform 'vtxScale' to " << refPp.vtxScale << " using glUniform1f"
			  << TestLog::EndMessage;
		glUniform1f(frgLoc, refPp.frgScale);
		log() << TestLog::Message
			  << "// Set uniform 'frgScale' to " << refPp.frgScale << " using glUniform1f"
			  << TestLog::EndMessage;

		pipeline.activeShaderProgram(0);
		LOG_CALL(glBindProgramPipeline(0));

		LOG_CALL(glUseProgram(changeName));
		drawSurface(resultSurface, drawSeed);
		LOG_CALL(glUseProgram(0));
	}

	bool result = tcu::fuzzyCompare(
		m_testCtx.getLog(), "Active program uniform result",
		"Active program uniform result",
		refSurface, resultSurface, 0.05f, tcu::COMPARE_LOG_RESULT);

	m_status.check(result,
				   "glUniform() did not correctly modify "
				   "the active program of the bound pipeline");
}

void SeparateShaderTest::testPipelineQueryPrograms (MovePtr<Pipeline>& pipeOut)
{
	ProgramParams				pipePp		= genProgramParams(m_rnd);
	Pipeline&					pipeline	= *(pipeOut = createPipeline(pipePp));
	GLuint						pipeName	= pipeline.pipeline->getPipeline();
	GLint						queryVtx	= 0;
	GLint						queryFrg	= 0;

	LOG_CALL(GLU_CHECK_CALL(glGetProgramPipelineiv(pipeName, GL_VERTEX_SHADER, &queryVtx)));
	m_status.check(GLuint(queryVtx) == pipeline.getVertexProgram().getProgramName(),
				   "Program pipeline query reported wrong vertex shader program");

	LOG_CALL(GLU_CHECK_CALL(glGetProgramPipelineiv(pipeName, GL_FRAGMENT_SHADER, &queryFrg)));
	m_status.check(GLuint(queryFrg) == pipeline.getFragmentProgram().getProgramName(),
				   "Program pipeline query reported wrong fragment shader program");
}

void SeparateShaderTest::testPipelineQueryActive (MovePtr<Pipeline>& pipeOut)
{
	ProgramParams				pipePp		= genProgramParams(m_rnd);
	Pipeline&					pipeline	= *(pipeOut = createPipeline(pipePp));
	GLuint						pipeName	= pipeline.pipeline->getPipeline();
	GLuint						newActive	= pipeline.getVertexProgram().getProgramName();
	GLint						queryActive	= 0;

	LOG_CALL(GLU_CHECK_CALL(glGetProgramPipelineiv(pipeName, GL_ACTIVE_PROGRAM, &queryActive)));
	m_status.check(queryActive == 0,
				   "Program pipeline query reported non-zero initial active program");

	pipeline.pipeline->activeShaderProgram(newActive);
	log() << TestLog::Message
		  << "Set pipeline " << pipeName << "'s active shader program to " << newActive
		  << TestLog::EndMessage;

	LOG_CALL(GLU_CHECK_CALL(glGetProgramPipelineiv(pipeName, GL_ACTIVE_PROGRAM, &queryActive)));
	m_status.check(GLuint(queryActive) == newActive,
				   "Program pipeline query reported incorrect active program");

	pipeline.pipeline->activeShaderProgram(0);
}

TestCase::IterateResult SeparateShaderTest::iterate (void)
{
	MovePtr<Pipeline> pipeline;

	DE_ASSERT(m_iterations > 0);

	if (m_currentIteration == 0)
		logParams(log(), m_params);

	++m_currentIteration;

	try
	{
		(this->*m_testFunc)(pipeline);
		log().writeMessage("");
	}
	catch (const tcu::Exception&)
	{
		if (pipeline)
			logPipeline(log(), *pipeline);
		throw;
	}

	if (m_status.getResult() != QP_TEST_RESULT_PASS)
	{
		if (pipeline)
			logPipeline(log(), *pipeline);
	}
	else if (m_currentIteration < m_iterations)
		return CONTINUE;

	m_status.setTestContextResult(m_testCtx);
	return STOP;
}

// Group construction utilities

enum ParamFlags
{
	PARAMFLAGS_SWITCH_FRAGMENT	= 1 << 0,
	PARAMFLAGS_SWITCH_VERTEX	= 1 << 1,
	PARAMFLAGS_INIT_SINGLE		= 1 << 2,
	PARAMFLAGS_LAST				= 1 << 3,
	PARAMFLAGS_MASK				= PARAMFLAGS_LAST - 1
};

bool areCaseParamFlagsValid (ParamFlags flags)
{
	const ParamFlags switchAll = ParamFlags(PARAMFLAGS_SWITCH_VERTEX|PARAMFLAGS_SWITCH_FRAGMENT);

	if ((flags & PARAMFLAGS_INIT_SINGLE) != 0)
		return (flags & switchAll) == 0 ||
			   (flags & switchAll) == switchAll;
	else
		return true;
}

bool addRenderTest (TestCaseGroup& group, const string& namePrefix, const string& descPrefix,
					int numIterations, ParamFlags flags, TestParams params)
{
	ostringstream	name;
	ostringstream	desc;

	DE_ASSERT(areCaseParamFlagsValid(flags));

	name << namePrefix;
	desc << descPrefix;

	params.initSingle	= (flags & PARAMFLAGS_INIT_SINGLE) != 0;
	params.switchVtx	= (flags & PARAMFLAGS_SWITCH_VERTEX) != 0;
	params.switchFrg	= (flags & PARAMFLAGS_SWITCH_FRAGMENT) != 0;

	name << (flags & PARAMFLAGS_INIT_SINGLE ? "single_program" : "separate_programs");
	desc << (flags & PARAMFLAGS_INIT_SINGLE
			 ? "Single program with two shaders"
			 : "Separate programs for each shader");

	switch (flags & (PARAMFLAGS_SWITCH_FRAGMENT | PARAMFLAGS_SWITCH_VERTEX))
	{
		case 0:
			break;
		case PARAMFLAGS_SWITCH_FRAGMENT:
			name << "_add_fragment";
			desc << ", then add a fragment program";
			break;
		case PARAMFLAGS_SWITCH_VERTEX:
			name << "_add_vertex";
			desc << ", then add a vertex program";
			break;
		case PARAMFLAGS_SWITCH_FRAGMENT | PARAMFLAGS_SWITCH_VERTEX:
			name << "_add_both";
			desc << ", then add both vertex and shader programs";
			break;
	}

	if (!paramsValid(params))
		return false;

	group.addChild(new SeparateShaderTest(group.getContext(), name.str(), desc.str(),
										  numIterations, params,
										  &SeparateShaderTest::testPipelineRendering));

	return true;
}

void describeInterpolation (const string& stage, VaryingInterpolation qual,
							ostringstream& name, ostringstream& desc)
{
	DE_ASSERT(qual < VARYINGINTERPOLATION_RANDOM);

	if (qual == VARYINGINTERPOLATION_DEFAULT)
	{
		desc << ", unqualified in " << stage << " shader";
		return;
	}
	else
	{
		const string qualName = glu::getInterpolationName(getGluInterpolation(qual));

		name << "_" << stage << "_" << qualName;
		desc << ", qualified '" << qualName << "' in " << stage << " shader";
	}
}


} // anonymous

TestCaseGroup* createSeparateShaderTests (Context& ctx)
{
	TestParams		defaultParams;
	int				numIterations	= 4;
	TestCaseGroup*	group			=
		new TestCaseGroup(ctx, "separate_shader", "Separate shader tests");

	defaultParams.useUniform			= false;
	defaultParams.initSingle			= false;
	defaultParams.switchVtx				= false;
	defaultParams.switchFrg				= false;
	defaultParams.useCreateHelper		= false;
	defaultParams.useProgramUniform		= false;
	defaultParams.useSameName			= false;
	defaultParams.varyings.count		= 0;
	defaultParams.varyings.type			= glu::TYPE_INVALID;
	defaultParams.varyings.binding		= BINDING_NAME;
	defaultParams.varyings.vtxInterp	= VARYINGINTERPOLATION_LAST;
	defaultParams.varyings.frgInterp	= VARYINGINTERPOLATION_LAST;

	TestCaseGroup* stagesGroup =
		new TestCaseGroup(ctx, "pipeline", "Pipeline configuration tests");
	group->addChild(stagesGroup);

	for (deUint32 flags = 0; flags < PARAMFLAGS_LAST << 2; ++flags)
	{
		TestParams		params			= defaultParams;
		ostringstream	name;
		ostringstream	desc;

		if (!areCaseParamFlagsValid(ParamFlags(flags & PARAMFLAGS_MASK)))
			continue;

		if (flags & (PARAMFLAGS_LAST << 1))
		{
			params.useSameName = true;
			name << "same_";
			desc << "Identically named ";
		}
		else
		{
			name << "different_";
			desc << "Differently named ";
		}

		if (flags & PARAMFLAGS_LAST)
		{
			params.useUniform = true;
			name << "uniform_";
			desc << "uniforms, ";
		}
		else
		{
			name << "constant_";
			desc << "constants, ";
		}

		addRenderTest(*stagesGroup, name.str(), desc.str(), numIterations,
					  ParamFlags(flags & PARAMFLAGS_MASK), params);
	}

	TestCaseGroup* programUniformGroup =
		new TestCaseGroup(ctx, "program_uniform", "ProgramUniform tests");
	group->addChild(programUniformGroup);

	for (deUint32 flags = 0; flags < PARAMFLAGS_LAST; ++flags)
	{
		TestParams		params			= defaultParams;

		if (!areCaseParamFlagsValid(ParamFlags(flags)))
			continue;

		params.useUniform = true;
		params.useProgramUniform = true;

		addRenderTest(*programUniformGroup, "", "", numIterations, ParamFlags(flags), params);
	}

	TestCaseGroup* createShaderProgramGroup =
		new TestCaseGroup(ctx, "create_shader_program", "CreateShaderProgram tests");
	group->addChild(createShaderProgramGroup);

	for (deUint32 flags = 0; flags < PARAMFLAGS_LAST; ++flags)
	{
		TestParams		params			= defaultParams;

		if (!areCaseParamFlagsValid(ParamFlags(flags)))
			continue;

		params.useCreateHelper = true;

		addRenderTest(*createShaderProgramGroup, "", "", numIterations,
					  ParamFlags(flags), params);
	}

	TestCaseGroup* interfaceGroup =
		new TestCaseGroup(ctx, "interface", "Shader interface compatibility tests");
	group->addChild(interfaceGroup);

	enum
	{
		NUM_INTERPOLATIONS	= VARYINGINTERPOLATION_RANDOM, // VARYINGINTERPOLATION_RANDOM is one after last fully specified interpolation
		INTERFACEFLAGS_LAST = BINDING_LAST * NUM_INTERPOLATIONS * NUM_INTERPOLATIONS
	};

	for (deUint32 flags = 0; flags < INTERFACEFLAGS_LAST; ++flags)
	{
		deUint32				tmpFlags	= flags;
		VaryingInterpolation	frgInterp	= VaryingInterpolation(tmpFlags % NUM_INTERPOLATIONS);
		VaryingInterpolation	vtxInterp	= VaryingInterpolation((tmpFlags /= NUM_INTERPOLATIONS)
																   % NUM_INTERPOLATIONS);
		BindingKind				binding		= BindingKind((tmpFlags /= NUM_INTERPOLATIONS)
														  % BINDING_LAST);
		TestParams				params		= defaultParams;
		ostringstream			name;
		ostringstream			desc;

		params.varyings.count		= 1;
		params.varyings.type		= glu::TYPE_FLOAT;
		params.varyings.binding		= binding;
		params.varyings.vtxInterp	= vtxInterp;
		params.varyings.frgInterp	= frgInterp;

		switch (binding)
		{
			case BINDING_LOCATION:
				name << "same_location";
				desc << "Varyings have same location, ";
				break;
			case BINDING_NAME:
				name << "same_name";
				desc << "Varyings have same name, ";
				break;
			default:
				DE_FATAL("Impossible");
		}

		describeInterpolation("vertex", vtxInterp, name, desc);
		describeInterpolation("fragment", frgInterp, name, desc);

		if (!paramsValid(params))
			continue;

		interfaceGroup->addChild(
			new SeparateShaderTest(ctx, name.str(), desc.str(), numIterations, params,
								   &SeparateShaderTest::testPipelineRendering));
	}

	deUint32		baseSeed	= ctx.getTestContext().getCommandLine().getBaseSeed();
	Random			rnd			(deStringHash("separate_shader.random") + baseSeed);
	set<string>		seen;
	TestCaseGroup*	randomGroup	= new TestCaseGroup(
		ctx, "random", "Random pipeline configuration tests");
	group->addChild(randomGroup);

	for (deUint32 i = 0; i < 128; ++i)
	{
		TestParams		params;
		string			code;
		deUint32		genIterations	= 4096;

		do
		{
			params	= genParams(rnd.getUint32());
			code	= paramsCode(params);
		} while (de::contains(seen, code) && --genIterations > 0);

		seen.insert(code);

		string name = de::toString(i); // Would be code but baseSeed can change

		randomGroup->addChild(new SeparateShaderTest(
								  ctx, name, name, numIterations, params,
								  &SeparateShaderTest::testPipelineRendering));
	}

	TestCaseGroup* apiGroup =
		new TestCaseGroup(ctx, "api", "Program pipeline API tests");
	group->addChild(apiGroup);

	{
		// More or less random parameters. These shouldn't have much effect, so just
		// do a single sample.
		TestParams params = defaultParams;
		params.useUniform = true;
		apiGroup->addChild(new SeparateShaderTest(
								  ctx,
								  "current_program_priority",
								  "Test priority between current program and pipeline binding",
								  1, params, &SeparateShaderTest::testCurrentProgPriority));
		apiGroup->addChild(new SeparateShaderTest(
								  ctx,
								  "active_program_uniform",
								  "Test that glUniform() affects a pipeline's active program",
								  1, params, &SeparateShaderTest::testActiveProgramUniform));

		apiGroup->addChild(new SeparateShaderTest(
								 ctx,
								 "pipeline_programs",
								 "Test queries for programs in program pipeline stages",
								 1, params, &SeparateShaderTest::testPipelineQueryPrograms));

		apiGroup->addChild(new SeparateShaderTest(
								 ctx,
								 "pipeline_active",
								 "Test query for active programs in a program pipeline",
								 1, params, &SeparateShaderTest::testPipelineQueryActive));
	}

	TestCaseGroup* interfaceMismatchGroup =
		new TestCaseGroup(ctx, "validation", "Negative program pipeline interface matching");
	group->addChild(interfaceMismatchGroup);

	{
		TestCaseGroup*						es31Group		= new TestCaseGroup(ctx, "es31", "GLSL ES 3.1 pipeline interface matching");
		gls::ShaderLibrary					shaderLibrary	(ctx.getTestContext(), ctx.getRenderContext(), ctx.getContextInfo());
		const std::vector<tcu::TestNode*>	children		= shaderLibrary.loadShaderFile("shaders/es31/separate_shader_validation.test");

		for (int i = 0; i < (int)children.size(); i++)
			es31Group->addChild(children[i]);

		interfaceMismatchGroup->addChild(es31Group);
	}

	{
		TestCaseGroup*						es32Group		= new TestCaseGroup(ctx, "es32", "GLSL ES 3.2 pipeline interface matching");
		gls::ShaderLibrary					shaderLibrary	(ctx.getTestContext(), ctx.getRenderContext(), ctx.getContextInfo());
		const std::vector<tcu::TestNode*>	children		= shaderLibrary.loadShaderFile("shaders/es32/separate_shader_validation.test");

		for (int i = 0; i < (int)children.size(); i++)
			es32Group->addChild(children[i]);

		interfaceMismatchGroup->addChild(es32Group);
	}

	return group;
}

} // Functional
} // gles31
} // deqp
