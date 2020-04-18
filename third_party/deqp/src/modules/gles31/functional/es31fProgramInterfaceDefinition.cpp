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
 * \brief Program interface
 *//*--------------------------------------------------------------------*/

#include "es31fProgramInterfaceDefinition.hpp"
#include "es31fProgramInterfaceDefinitionUtil.hpp"
#include "gluVarType.hpp"
#include "gluShaderProgram.hpp"
#include "deSTLUtil.hpp"
#include "deStringUtil.hpp"
#include "glwEnums.hpp"

#include <set>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace ProgramInterfaceDefinition
{
namespace
{

static const glu::ShaderType s_shaderStageOrder[] =
{
	glu::SHADERTYPE_COMPUTE,

	glu::SHADERTYPE_VERTEX,
	glu::SHADERTYPE_TESSELLATION_CONTROL,
	glu::SHADERTYPE_TESSELLATION_EVALUATION,
	glu::SHADERTYPE_GEOMETRY,
	glu::SHADERTYPE_FRAGMENT
};

// s_shaderStageOrder does not contain ShaderType_LAST
DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(s_shaderStageOrder) == glu::SHADERTYPE_LAST);

static bool containsMatchingSubtype (const glu::VarType& varType, bool (*predicate)(glu::DataType))
{
	if (varType.isBasicType() && predicate(varType.getBasicType()))
		return true;

	if (varType.isArrayType())
		return containsMatchingSubtype(varType.getElementType(), predicate);

	if (varType.isStructType())
		for (int memberNdx = 0; memberNdx < varType.getStructPtr()->getNumMembers(); ++memberNdx)
			if (containsMatchingSubtype(varType.getStructPtr()->getMember(memberNdx).getType(), predicate))
				return true;

	return false;
}

static bool containsMatchingSubtype (const std::vector<glu::VariableDeclaration>& decls, bool (*predicate)(glu::DataType))
{
	for (int varNdx = 0; varNdx < (int)decls.size(); ++varNdx)
		if (containsMatchingSubtype(decls[varNdx].varType, predicate))
			return true;
	return false;
}

static bool isOpaqueType (glu::DataType type)
{
	return	glu::isDataTypeAtomicCounter(type)	||
			glu::isDataTypeImage(type)			||
			glu::isDataTypeSampler(type);
}

static int getShaderStageIndex (glu::ShaderType stage)
{
	const glu::ShaderType* const it = std::find(DE_ARRAY_BEGIN(s_shaderStageOrder), DE_ARRAY_END(s_shaderStageOrder), stage);

	if (it == DE_ARRAY_END(s_shaderStageOrder))
		return -1;
	else
	{
		const int index = (int)(it - DE_ARRAY_BEGIN(s_shaderStageOrder));
		return index;
	}
}

} // anonymous

Shader::Shader (glu::ShaderType type, glu::GLSLVersion version)
	: m_shaderType	(type)
	, m_version		(version)
{
}

Shader::~Shader (void)
{
}

static bool isIllegalVertexInput (const glu::VarType& varType)
{
	// booleans, opaque types, arrays, structs are not allowed as inputs
	if (!varType.isBasicType())
		return true;
	if (glu::isDataTypeBoolOrBVec(varType.getBasicType()))
		return true;
	return false;
}

static bool isIllegalVertexOutput (const glu::VarType& varType, bool insideAStruct = false, bool insideAnArray = false)
{
	// booleans, opaque types, arrays of arrays, arrays of structs, array in struct, struct struct are not allowed as vertex outputs

	if (varType.isBasicType())
	{
		const bool isOpaqueType = !glu::isDataTypeScalar(varType.getBasicType()) && !glu::isDataTypeVector(varType.getBasicType()) && !glu::isDataTypeMatrix(varType.getBasicType());

		if (glu::isDataTypeBoolOrBVec(varType.getBasicType()))
			return true;

		if (isOpaqueType)
			return true;

		return false;
	}
	else if (varType.isArrayType())
	{
		if (insideAnArray || insideAStruct)
			return true;

		return isIllegalVertexOutput(varType.getElementType(), insideAStruct, true);
	}
	else if (varType.isStructType())
	{
		if (insideAnArray || insideAStruct)
			return true;

		for (int ndx = 0; ndx < varType.getStructPtr()->getNumMembers(); ++ndx)
			if (isIllegalVertexOutput(varType.getStructPtr()->getMember(ndx).getType(), true, insideAnArray))
				return true;

		return false;
	}
	else
	{
		DE_ASSERT(false);
		return true;
	}
}

static bool isIllegalFragmentInput (const glu::VarType& varType)
{
	return isIllegalVertexOutput(varType);
}

static bool isIllegalFragmentOutput (const glu::VarType& varType, bool insideAnArray = false)
{
	// booleans, opaque types, matrices, structs, arrays of arrays are not allowed as outputs

	if (varType.isBasicType())
	{
		const bool isOpaqueType = !glu::isDataTypeScalar(varType.getBasicType()) && !glu::isDataTypeVector(varType.getBasicType()) && !glu::isDataTypeMatrix(varType.getBasicType());

		if (glu::isDataTypeBoolOrBVec(varType.getBasicType()) || isOpaqueType || glu::isDataTypeMatrix(varType.getBasicType()))
			return true;
		return false;
	}
	else if (varType.isArrayType())
	{
		if (insideAnArray)
			return true;
		return isIllegalFragmentOutput(varType.getElementType(), true);
	}
	else if (varType.isStructType())
		return true;
	else
	{
		DE_ASSERT(false);
		return true;
	}
}

static bool isTypeIntegerOrContainsIntegers (const glu::VarType& varType)
{
	if (varType.isBasicType())
		return glu::isDataTypeIntOrIVec(varType.getBasicType()) || glu::isDataTypeUintOrUVec(varType.getBasicType());
	else if (varType.isArrayType())
		return isTypeIntegerOrContainsIntegers(varType.getElementType());
	else if (varType.isStructType())
	{
		for (int ndx = 0; ndx < varType.getStructPtr()->getNumMembers(); ++ndx)
			if (isTypeIntegerOrContainsIntegers(varType.getStructPtr()->getMember(ndx).getType()))
				return true;
		return false;
	}
	else
	{
		DE_ASSERT(false);
		return true;
	}
}

bool Shader::isValid (void) const
{
	// Default block variables
	{
		for (int varNdx = 0; varNdx < (int)m_defaultBlock.variables.size(); ++varNdx)
		{
			// atomic declaration in the default block without binding
			if (m_defaultBlock.variables[varNdx].layout.binding == -1 &&
				containsMatchingSubtype(m_defaultBlock.variables[varNdx].varType, glu::isDataTypeAtomicCounter))
				return false;

			// atomic declaration in a struct
			if (m_defaultBlock.variables[varNdx].varType.isStructType() &&
				containsMatchingSubtype(m_defaultBlock.variables[varNdx].varType, glu::isDataTypeAtomicCounter))
				return false;

			// Unsupported layout qualifiers

			if (m_defaultBlock.variables[varNdx].layout.matrixOrder != glu::MATRIXORDER_LAST)
				return false;

			if (containsMatchingSubtype(m_defaultBlock.variables[varNdx].varType, glu::isDataTypeSampler))
			{
				const glu::Layout layoutWithLocationAndBinding(m_defaultBlock.variables[varNdx].layout.location, m_defaultBlock.variables[varNdx].layout.binding);

				if (m_defaultBlock.variables[varNdx].layout != layoutWithLocationAndBinding)
					return false;
			}
		}
	}

	// Interface blocks
	{
		for (int interfaceNdx = 0; interfaceNdx < (int)m_defaultBlock.interfaceBlocks.size(); ++interfaceNdx)
		{
			// ES31 disallows interface block array arrays
			if (m_defaultBlock.interfaceBlocks[interfaceNdx].dimensions.size() > 1)
				return false;

			// Interface block arrays must have instance name
			if (!m_defaultBlock.interfaceBlocks[interfaceNdx].dimensions.empty() && m_defaultBlock.interfaceBlocks[interfaceNdx].instanceName.empty())
				return false;

			// Opaque types in interface block
			if (containsMatchingSubtype(m_defaultBlock.interfaceBlocks[interfaceNdx].variables, isOpaqueType))
				return false;
		}
	}

	// Shader type specific

	if (m_shaderType == glu::SHADERTYPE_VERTEX)
	{
		for (int varNdx = 0; varNdx < (int)m_defaultBlock.variables.size(); ++varNdx)
		{
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_IN && isIllegalVertexInput(m_defaultBlock.variables[varNdx].varType))
				return false;
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_OUT && isIllegalVertexOutput(m_defaultBlock.variables[varNdx].varType))
				return false;
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_OUT && m_defaultBlock.variables[varNdx].interpolation != glu::INTERPOLATION_FLAT && isTypeIntegerOrContainsIntegers(m_defaultBlock.variables[varNdx].varType))
				return false;
		}
		for (int interfaceNdx = 0; interfaceNdx < (int)m_defaultBlock.interfaceBlocks.size(); ++interfaceNdx)
		{
			if (m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_IN			||
				m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_PATCH_IN	||
				m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_PATCH_OUT)
			{
				return false;
			}
		}
	}
	else if (m_shaderType == glu::SHADERTYPE_FRAGMENT)
	{
		for (int varNdx = 0; varNdx < (int)m_defaultBlock.variables.size(); ++varNdx)
		{
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_IN && isIllegalFragmentInput(m_defaultBlock.variables[varNdx].varType))
				return false;
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_IN && m_defaultBlock.variables[varNdx].interpolation != glu::INTERPOLATION_FLAT && isTypeIntegerOrContainsIntegers(m_defaultBlock.variables[varNdx].varType))
				return false;
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_OUT && isIllegalFragmentOutput(m_defaultBlock.variables[varNdx].varType))
				return false;
		}
		for (int interfaceNdx = 0; interfaceNdx < (int)m_defaultBlock.interfaceBlocks.size(); ++interfaceNdx)
		{
			if (m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_PATCH_IN	||
				m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_OUT		||
				m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_PATCH_OUT)
			{
				return false;
			}
		}
	}
	else if (m_shaderType == glu::SHADERTYPE_COMPUTE)
	{
		for (int varNdx = 0; varNdx < (int)m_defaultBlock.variables.size(); ++varNdx)
		{
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_IN			||
				m_defaultBlock.variables[varNdx].storage == glu::STORAGE_PATCH_IN	||
				m_defaultBlock.variables[varNdx].storage == glu::STORAGE_OUT		||
				m_defaultBlock.variables[varNdx].storage == glu::STORAGE_PATCH_OUT)
			{
				return false;
			}
		}
		for (int interfaceNdx = 0; interfaceNdx < (int)m_defaultBlock.interfaceBlocks.size(); ++interfaceNdx)
		{
			if (m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_IN			||
				m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_PATCH_IN	||
				m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_OUT		||
				m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_PATCH_OUT)
			{
				return false;
			}
		}
	}
	else if (m_shaderType == glu::SHADERTYPE_GEOMETRY)
	{
		for (int varNdx = 0; varNdx < (int)m_defaultBlock.variables.size(); ++varNdx)
		{
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_PATCH_IN	||
				m_defaultBlock.variables[varNdx].storage == glu::STORAGE_PATCH_OUT)
			{
				return false;
			}
			// arrayed input
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_IN && !m_defaultBlock.variables[varNdx].varType.isArrayType())
				return false;
		}
		for (int interfaceNdx = 0; interfaceNdx < (int)m_defaultBlock.interfaceBlocks.size(); ++interfaceNdx)
		{
			if (m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_PATCH_IN	||
				m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_PATCH_OUT)
			{
				return false;
			}
			// arrayed input
			if (m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_IN && m_defaultBlock.interfaceBlocks[interfaceNdx].dimensions.empty())
				return false;
		}
	}
	else if (m_shaderType == glu::SHADERTYPE_TESSELLATION_CONTROL)
	{
		for (int varNdx = 0; varNdx < (int)m_defaultBlock.variables.size(); ++varNdx)
		{
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_PATCH_IN)
				return false;
			// arrayed input
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_IN && !m_defaultBlock.variables[varNdx].varType.isArrayType())
				return false;
			// arrayed output
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_OUT && !m_defaultBlock.variables[varNdx].varType.isArrayType())
				return false;
		}
		for (int interfaceNdx = 0; interfaceNdx < (int)m_defaultBlock.interfaceBlocks.size(); ++interfaceNdx)
		{
			if (m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_PATCH_IN)
				return false;
			// arrayed input
			if (m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_IN && m_defaultBlock.interfaceBlocks[interfaceNdx].dimensions.empty())
				return false;
			// arrayed output
			if (m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_OUT && m_defaultBlock.interfaceBlocks[interfaceNdx].dimensions.empty())
				return false;
		}
	}
	else if (m_shaderType == glu::SHADERTYPE_TESSELLATION_EVALUATION)
	{
		for (int varNdx = 0; varNdx < (int)m_defaultBlock.variables.size(); ++varNdx)
		{
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_PATCH_OUT)
				return false;
			// arrayed input
			if (m_defaultBlock.variables[varNdx].storage == glu::STORAGE_IN && !m_defaultBlock.variables[varNdx].varType.isArrayType())
				return false;
		}
		for (int interfaceNdx = 0; interfaceNdx < (int)m_defaultBlock.interfaceBlocks.size(); ++interfaceNdx)
		{
			if (m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_PATCH_OUT)
				return false;
			// arrayed input
			if (m_defaultBlock.interfaceBlocks[interfaceNdx].storage == glu::STORAGE_IN && m_defaultBlock.interfaceBlocks[interfaceNdx].dimensions.empty())
				return false;
		}
	}
	else
		DE_ASSERT(false);

	return true;
}

Program::Program (void)
	: m_separable				(false)
	, m_xfbMode					(0)
	, m_geoNumOutputVertices	(0)
	, m_tessNumOutputVertices	(0)
{
}

static void collectStructPtrs (std::set<const glu::StructType*>& dst, const glu::VarType& type)
{
	if (type.isArrayType())
		collectStructPtrs(dst, type.getElementType());
	else if (type.isStructType())
	{
		dst.insert(type.getStructPtr());

		for (int memberNdx = 0; memberNdx < type.getStructPtr()->getNumMembers(); ++memberNdx)
			collectStructPtrs(dst, type.getStructPtr()->getMember(memberNdx).getType());
	}
}

Program::~Program (void)
{
	// delete shader struct types, need to be done by the program since shaders might share struct types
	{
		std::set<const glu::StructType*> structTypes;

		for (int shaderNdx = 0; shaderNdx < (int)m_shaders.size(); ++shaderNdx)
		{
			for (int varNdx = 0; varNdx < (int)m_shaders[shaderNdx]->m_defaultBlock.variables.size(); ++varNdx)
				collectStructPtrs(structTypes, m_shaders[shaderNdx]->m_defaultBlock.variables[varNdx].varType);

			for (int interfaceNdx = 0; interfaceNdx < (int)m_shaders[shaderNdx]->m_defaultBlock.interfaceBlocks.size(); ++interfaceNdx)
				for (int varNdx = 0; varNdx < (int)m_shaders[shaderNdx]->m_defaultBlock.interfaceBlocks[interfaceNdx].variables.size(); ++varNdx)
					collectStructPtrs(structTypes, m_shaders[shaderNdx]->m_defaultBlock.interfaceBlocks[interfaceNdx].variables[varNdx].varType);
		}

		for (std::set<const glu::StructType*>::iterator it = structTypes.begin(); it != structTypes.end(); ++it)
			delete *it;
	}

	for (int shaderNdx = 0; shaderNdx < (int)m_shaders.size(); ++shaderNdx)
		delete m_shaders[shaderNdx];
	m_shaders.clear();
}

Shader* Program::addShader (glu::ShaderType type, glu::GLSLVersion version)
{
	DE_ASSERT(type < glu::SHADERTYPE_LAST);

	Shader* shader;

	// make sure push_back() cannot throw
	m_shaders.reserve(m_shaders.size() + 1);

	shader = new Shader(type, version);
	m_shaders.push_back(shader);

	return shader;
}

void Program::setSeparable (bool separable)
{
	m_separable = separable;
}

bool Program::isSeparable (void) const
{
	return m_separable;
}

const std::vector<Shader*>& Program::getShaders (void) const
{
	return m_shaders;
}

glu::ShaderType Program::getFirstStage (void) const
{
	const int	nullValue	= DE_LENGTH_OF_ARRAY(s_shaderStageOrder);
	int			firstStage	= nullValue;

	for (int shaderNdx = 0; shaderNdx < (int)m_shaders.size(); ++shaderNdx)
	{
		const int index = getShaderStageIndex(m_shaders[shaderNdx]->getType());
		if (index != -1)
			firstStage = de::min(firstStage, index);
	}

	if (firstStage == nullValue)
		return glu::SHADERTYPE_LAST;
	else
		return s_shaderStageOrder[firstStage];
}

glu::ShaderType Program::getLastStage (void) const
{
	const int	nullValue	= -1;
	int			lastStage	= nullValue;

	for (int shaderNdx = 0; shaderNdx < (int)m_shaders.size(); ++shaderNdx)
	{
		const int index = getShaderStageIndex(m_shaders[shaderNdx]->getType());
		if (index != -1)
			lastStage = de::max(lastStage, index);
	}

	if (lastStage == nullValue)
		return glu::SHADERTYPE_LAST;
	else
		return s_shaderStageOrder[lastStage];
}

bool Program::hasStage (glu::ShaderType stage) const
{
	for (int shaderNdx = 0; shaderNdx < (int)m_shaders.size(); ++shaderNdx)
	{
		if (m_shaders[shaderNdx]->getType() == stage)
			return true;
	}
	return false;
}

void Program::addTransformFeedbackVarying (const std::string& varName)
{
	m_xfbVaryings.push_back(varName);
}

const std::vector<std::string>& Program::getTransformFeedbackVaryings (void) const
{
	return m_xfbVaryings;
}

void Program::setTransformFeedbackMode (deUint32 mode)
{
	m_xfbMode = mode;
}

deUint32 Program::getTransformFeedbackMode (void) const
{
	return m_xfbMode;
}

deUint32 Program::getGeometryNumOutputVertices (void) const
{
	return m_geoNumOutputVertices;
}

void Program::setGeometryNumOutputVertices (deUint32 vertices)
{
	m_geoNumOutputVertices = vertices;
}

deUint32 Program::getTessellationNumOutputPatchVertices (void) const
{
	return m_tessNumOutputVertices;
}

void Program::setTessellationNumOutputPatchVertices (deUint32 vertices)
{
	m_tessNumOutputVertices = vertices;
}

bool Program::isValid (void) const
{
	const bool	isOpenGLES			= (m_shaders.empty()) ? (false) : (glu::glslVersionIsES(m_shaders[0]->getVersion()));
	bool		computePresent		= false;
	bool		vertexPresent		= false;
	bool		fragmentPresent		= false;
	bool		tessControlPresent	= false;
	bool		tessEvalPresent		= false;
	bool		geometryPresent		= false;

	if (m_shaders.empty())
		return false;

	for (int ndx = 0; ndx < (int)m_shaders.size(); ++ndx)
		if (!m_shaders[ndx]->isValid())
			return false;

	// same version
	for (int ndx = 1; ndx < (int)m_shaders.size(); ++ndx)
		if (m_shaders[0]->getVersion() != m_shaders[ndx]->getVersion())
			return false;

	for (int ndx = 0; ndx < (int)m_shaders.size(); ++ndx)
	{
		switch (m_shaders[ndx]->getType())
		{
			case glu::SHADERTYPE_COMPUTE:					computePresent = true;		break;
			case glu::SHADERTYPE_VERTEX:					vertexPresent = true;		break;
			case glu::SHADERTYPE_FRAGMENT:					fragmentPresent = true;		break;
			case glu::SHADERTYPE_TESSELLATION_CONTROL:		tessControlPresent = true;	break;
			case glu::SHADERTYPE_TESSELLATION_EVALUATION:	tessEvalPresent = true;		break;
			case glu::SHADERTYPE_GEOMETRY:					geometryPresent = true;		break;
			default:
				DE_ASSERT(false);
				break;
		}
	}
	// compute present -> no other stages present
	{
		const bool nonComputePresent = vertexPresent || fragmentPresent || tessControlPresent || tessEvalPresent || geometryPresent;
		if (computePresent && nonComputePresent)
			return false;
	}

	// must contain both vertex and fragment shaders
	if (!computePresent && !m_separable)
	{
		if (!vertexPresent || !fragmentPresent)
			return false;
	}

	// tess.Eval present <=> tess.Control present
	if (!m_separable)
	{
		if (tessEvalPresent != tessControlPresent)
			return false;
	}

	if ((m_tessNumOutputVertices != 0) != (tessControlPresent || tessEvalPresent))
		return false;

	if ((m_geoNumOutputVertices != 0) != geometryPresent)
		return false;

	for (int ndx = 0; ndx < (int)m_xfbVaryings.size(); ++ndx)
	{
		// user-defined
		if (!de::beginsWith(m_xfbVaryings[ndx], "gl_"))
		{
			std::vector<ProgramInterfaceDefinition::VariablePathComponent> path;
			if (!findProgramVariablePathByPathName(path, this, m_xfbVaryings[ndx], VariableSearchFilter::createShaderTypeStorageFilter(getProgramTransformFeedbackStage(this), glu::STORAGE_OUT)))
				return false;
			if (!path.back().isVariableType())
				return false;

			// Khronos bug #12787 disallowed capturing whole structs in OpenGL ES.
			if (path.back().getVariableType()->isStructType() && isOpenGLES)
				return false;
		}
	}

	return true;
}

} // ProgramInterfaceDefinition
} // Functional
} // gles31
} // deqp
