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
 * \brief Program interface query test case
 *//*--------------------------------------------------------------------*/

#include "es31fProgramInterfaceQueryTestCase.hpp"
#include "es31fProgramInterfaceDefinitionUtil.hpp"
#include "tcuTestLog.hpp"
#include "gluVarTypeUtil.hpp"
#include "gluStrUtil.hpp"
#include "gluContextInfo.hpp"
#include "gluShaderProgram.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deString.h"
#include "deStringUtil.hpp"
#include "deSTLUtil.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using ProgramInterfaceDefinition::VariablePathComponent;
using ProgramInterfaceDefinition::VariableSearchFilter;

static glw::GLenum getProgramDefaultBlockInterfaceFromStorage (glu::Storage storage)
{
	switch (storage)
	{
		case glu::STORAGE_IN:
		case glu::STORAGE_PATCH_IN:
			return GL_PROGRAM_INPUT;

		case glu::STORAGE_OUT:
		case glu::STORAGE_PATCH_OUT:
			return GL_PROGRAM_OUTPUT;

		case glu::STORAGE_UNIFORM:
			return GL_UNIFORM;

		default:
			DE_ASSERT(false);
			return 0;
	}
}

static bool isBufferBackedInterfaceBlockStorage (glu::Storage storage)
{
	return storage == glu::STORAGE_BUFFER || storage == glu::STORAGE_UNIFORM;
}

const char* getRequiredExtensionForStage (glu::ShaderType stage)
{
	switch (stage)
	{
		case glu::SHADERTYPE_COMPUTE:
		case glu::SHADERTYPE_VERTEX:
		case glu::SHADERTYPE_FRAGMENT:
			return DE_NULL;

		case glu::SHADERTYPE_GEOMETRY:
			return "GL_EXT_geometry_shader";

		case glu::SHADERTYPE_TESSELLATION_CONTROL:
		case glu::SHADERTYPE_TESSELLATION_EVALUATION:
			return "GL_EXT_tessellation_shader";

		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static int getTypeSize (glu::DataType type)
{
	if (type == glu::TYPE_FLOAT)
		return 4;
	else if (type == glu::TYPE_INT || type == glu::TYPE_UINT)
		return 4;
	else if (type == glu::TYPE_BOOL)
		return 4; // uint

	DE_ASSERT(false);
	return 0;
}

static int getVarTypeSize (const glu::VarType& type)
{
	if (type.isBasicType())
	{
		// return in basic machine units
		return glu::getDataTypeScalarSize(type.getBasicType()) * getTypeSize(glu::getDataTypeScalarType(type.getBasicType()));
	}
	else if (type.isStructType())
	{
		int size = 0;
		for (int ndx = 0; ndx < type.getStructPtr()->getNumMembers(); ++ndx)
			size += getVarTypeSize(type.getStructPtr()->getMember(ndx).getType());
		return size;
	}
	else if (type.isArrayType())
	{
		// unsized arrays are handled as if they had only one element
		if (type.getArraySize() == glu::VarType::UNSIZED_ARRAY)
			return getVarTypeSize(type.getElementType());
		else
			return type.getArraySize() * getVarTypeSize(type.getElementType());
	}
	else
	{
		DE_ASSERT(false);
		return 0;
	}
}

static glu::MatrixOrder getMatrixOrderFromPath (const std::vector<VariablePathComponent>& path)
{
	glu::MatrixOrder order = glu::MATRIXORDER_LAST;

	// inherit majority
	for (int pathNdx = 0; pathNdx < (int)path.size(); ++pathNdx)
	{
		glu::MatrixOrder matOrder;

		if (path[pathNdx].isInterfaceBlock())
			matOrder = path[pathNdx].getInterfaceBlock()->layout.matrixOrder;
		else if (path[pathNdx].isDeclaration())
			matOrder = path[pathNdx].getDeclaration()->layout.matrixOrder;
		else if (path[pathNdx].isVariableType())
			matOrder = glu::MATRIXORDER_LAST;
		else
		{
			DE_ASSERT(false);
			return glu::MATRIXORDER_LAST;
		}

		if (matOrder != glu::MATRIXORDER_LAST)
			order = matOrder;
	}

	return order;
}

class PropValidator
{
public:
									PropValidator					(Context& context, ProgramResourcePropFlags validationProp, const char* requiredExtension);

	virtual std::string				getHumanReadablePropertyString	(glw::GLint propVal) const;
	virtual void					validate						(const ProgramInterfaceDefinition::Program* program, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const = 0;

	bool							isSupported						(void) const;
	bool							isSelected						(deUint32 caseFlags) const;

protected:
	void							setError						(const std::string& err) const;

	tcu::TestContext&				m_testCtx;
	const glu::RenderContext&		m_renderContext;

private:
	const glu::ContextInfo&			m_contextInfo;
	const char*						m_extension;
	const ProgramResourcePropFlags	m_validationProp;
};

PropValidator::PropValidator (Context& context, ProgramResourcePropFlags validationProp, const char* requiredExtension)
	: m_testCtx			(context.getTestContext())
	, m_renderContext	(context.getRenderContext())
	, m_contextInfo		(context.getContextInfo())
	, m_extension		(requiredExtension)
	, m_validationProp	(validationProp)
{
}

std::string PropValidator::getHumanReadablePropertyString (glw::GLint propVal) const
{
	return de::toString(propVal);
}

bool PropValidator::isSupported (void) const
{
	if(glu::contextSupports(m_renderContext.getType(), glu::ApiType::es(3, 2)))
		return true;
	return m_extension == DE_NULL || m_contextInfo.isExtensionSupported(m_extension);
}

bool PropValidator::isSelected (deUint32 caseFlags) const
{
	return (caseFlags & (deUint32)m_validationProp) != 0;
}

void PropValidator::setError (const std::string& err) const
{
	// don't overwrite earlier errors
	if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, err.c_str());
}

class SingleVariableValidator : public PropValidator
{
public:
					SingleVariableValidator	(Context& context, ProgramResourcePropFlags validationProp, glw::GLuint programID, const VariableSearchFilter& filter, const char* requiredExtension);

	void			validate				(const ProgramInterfaceDefinition::Program* program, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
	virtual void	validateSingleVariable	(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const = 0;
	virtual void	validateBuiltinVariable	(const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;

protected:
	const VariableSearchFilter	m_filter;
	const glw::GLuint			m_programID;
};

SingleVariableValidator::SingleVariableValidator (Context& context, ProgramResourcePropFlags validationProp, glw::GLuint programID, const VariableSearchFilter& filter, const char* requiredExtension)
	: PropValidator	(context, validationProp, requiredExtension)
	, m_filter		(filter)
	, m_programID	(programID)
{
}

void SingleVariableValidator::validate (const ProgramInterfaceDefinition::Program* program, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	std::vector<VariablePathComponent> path;

	if (findProgramVariablePathByPathName(path, program, resource, m_filter))
	{
		const glu::VarType* variable = (path.back().isVariableType()) ? (path.back().getVariableType()) : (DE_NULL);

		if (!variable || !variable->isBasicType())
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, resource name \"" << resource << "\" refers to a non-basic type." << tcu::TestLog::EndMessage;
			setError("resource not basic type");
		}
		else
			validateSingleVariable(path, resource, propValue, implementationName);

		// finding matching variable in any shader is sufficient
		return;
	}
	else if (deStringBeginsWith(resource.c_str(), "gl_"))
	{
		// special case for builtins
		validateBuiltinVariable(resource, propValue, implementationName);
		return;
	}

	// we are only supplied good names, generated by ourselves
	DE_ASSERT(false);
	throw tcu::InternalError("Resource name consistency error");
}

void SingleVariableValidator::validateBuiltinVariable (const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(resource);
	DE_UNREF(propValue);
	DE_UNREF(implementationName);
	DE_ASSERT(false);
}

class SingleBlockValidator : public PropValidator
{
public:
								SingleBlockValidator	(Context& context, ProgramResourcePropFlags validationProp, glw::GLuint programID, const VariableSearchFilter& filter, const char* requiredExtension);

	void						validate				(const ProgramInterfaceDefinition::Program* program, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
	virtual void				validateSingleBlock		(const glu::InterfaceBlock& block, const std::vector<int>& instanceIndex, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const = 0;

protected:
	const VariableSearchFilter	m_filter;
	const glw::GLuint			m_programID;
};

SingleBlockValidator::SingleBlockValidator (Context& context, ProgramResourcePropFlags validationProp, glw::GLuint programID, const VariableSearchFilter& filter, const char* requiredExtension)
	: PropValidator	(context, validationProp, requiredExtension)
	, m_filter		(filter)
	, m_programID	(programID)
{
}

void SingleBlockValidator::validate (const ProgramInterfaceDefinition::Program* program, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	glu::VarTokenizer	tokenizer		(resource.c_str());
	const std::string	blockName		= tokenizer.getIdentifier();
	std::vector<int>	instanceIndex;

	tokenizer.advance();

	// array index
	while (tokenizer.getToken() == glu::VarTokenizer::TOKEN_LEFT_BRACKET)
	{
		tokenizer.advance();
		DE_ASSERT(tokenizer.getToken() == glu::VarTokenizer::TOKEN_NUMBER);

		instanceIndex.push_back(tokenizer.getNumber());

		tokenizer.advance();
		DE_ASSERT(tokenizer.getToken() == glu::VarTokenizer::TOKEN_RIGHT_BRACKET);

		tokenizer.advance();
	}

	// no trailing garbage
	DE_ASSERT(tokenizer.getToken() == glu::VarTokenizer::TOKEN_END);

	for (int shaderNdx = 0; shaderNdx < (int)program->getShaders().size(); ++shaderNdx)
	{
		const ProgramInterfaceDefinition::Shader* const shader = program->getShaders()[shaderNdx];
		if (!m_filter.matchesFilter(shader))
			continue;

		for (int blockNdx = 0; blockNdx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++blockNdx)
		{
			const glu::InterfaceBlock& block = shader->getDefaultBlock().interfaceBlocks[blockNdx];

			if (m_filter.matchesFilter(block) && block.interfaceName == blockName)
			{
				// dimensions match
				DE_ASSERT(instanceIndex.size() == block.dimensions.size());

				validateSingleBlock(block, instanceIndex, resource, propValue, implementationName);
				return;
			}
		}
	}

	// we are only supplied good names, generated by ourselves
	DE_ASSERT(false);
	throw tcu::InternalError("Resource name consistency error");
}

class TypeValidator : public SingleVariableValidator
{
public:
				TypeValidator					(Context& context, glw::GLuint programID, const VariableSearchFilter& filter);

	std::string	getHumanReadablePropertyString	(glw::GLint propVal) const;
	void		validateSingleVariable			(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
	void		validateBuiltinVariable			(const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

TypeValidator::TypeValidator (Context& context, glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleVariableValidator(context, PROGRAMRESOURCEPROP_TYPE, programID, filter, DE_NULL)
{
}

std::string TypeValidator::getHumanReadablePropertyString (glw::GLint propVal) const
{
	return de::toString(glu::getShaderVarTypeStr(propVal));
}

void TypeValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	const glu::VarType* variable = path.back().getVariableType();

	DE_UNREF(resource);
	DE_UNREF(implementationName);

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying type, expecting " << glu::getDataTypeName(variable->getBasicType()) << tcu::TestLog::EndMessage;

	if (variable->getBasicType() != glu::getDataTypeFromGLType(propValue))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << glu::getDataTypeName(glu::getDataTypeFromGLType(propValue)) << tcu::TestLog::EndMessage;
		setError("resource type invalid");
	}
}

void TypeValidator::validateBuiltinVariable (const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(implementationName);

	static const struct
	{
		const char*		name;
		glu::DataType	type;
	} builtins[] =
	{
		{ "gl_Position",				glu::TYPE_FLOAT_VEC4	},
		{ "gl_FragCoord",				glu::TYPE_FLOAT_VEC4	},
		{ "gl_PerVertex.gl_Position",	glu::TYPE_FLOAT_VEC4	},
		{ "gl_VertexID",				glu::TYPE_INT			},
		{ "gl_InvocationID",			glu::TYPE_INT			},
		{ "gl_NumWorkGroups",			glu::TYPE_UINT_VEC3		},
		{ "gl_FragDepth",				glu::TYPE_FLOAT			},
		{ "gl_TessLevelOuter[0]",		glu::TYPE_FLOAT			},
		{ "gl_TessLevelInner[0]",		glu::TYPE_FLOAT			},
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(builtins); ++ndx)
	{
		if (resource == builtins[ndx].name)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying type, expecting " << glu::getDataTypeName(builtins[ndx].type) << tcu::TestLog::EndMessage;

			if (glu::getDataTypeFromGLType(propValue) != builtins[ndx].type)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << glu::getDataTypeName(glu::getDataTypeFromGLType(propValue)) << tcu::TestLog::EndMessage;
				setError("resource type invalid");
			}
			return;
		}
	}

	DE_ASSERT(false);
}

class ArraySizeValidator : public SingleVariableValidator
{
public:
				ArraySizeValidator				(Context& context, glw::GLuint programID, int unsizedArraySize, const VariableSearchFilter& filter);

	void		validateSingleVariable			(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
	void		validateBuiltinVariable			(const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;

private:
	const int	m_unsizedArraySize;
};

ArraySizeValidator::ArraySizeValidator (Context& context, glw::GLuint programID, int unsizedArraySize, const VariableSearchFilter& filter)
	: SingleVariableValidator	(context, PROGRAMRESOURCEPROP_ARRAY_SIZE, programID, filter, DE_NULL)
	, m_unsizedArraySize		(unsizedArraySize)
{
}

void ArraySizeValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	const VariablePathComponent		nullComponent;
	const VariablePathComponent&	enclosingcomponent	= (path.size() > 1) ? (path[path.size()-2]) : (nullComponent);

	const bool						isArray				= enclosingcomponent.isVariableType() && enclosingcomponent.getVariableType()->isArrayType();
	const bool						inUnsizedArray		= isArray && (enclosingcomponent.getVariableType()->getArraySize() == glu::VarType::UNSIZED_ARRAY);
	const int						arraySize			= (!isArray) ? (1) : (inUnsizedArray) ? (m_unsizedArraySize) : (enclosingcomponent.getVariableType()->getArraySize());

	DE_ASSERT(arraySize >= 0);
	DE_UNREF(resource);
	DE_UNREF(implementationName);

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying array size, expecting " << arraySize << tcu::TestLog::EndMessage;

	if (arraySize != propValue)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
		setError("resource array size invalid");
	}
}

void ArraySizeValidator::validateBuiltinVariable (const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(implementationName);

	static const struct
	{
		const char*		name;
		int				arraySize;
	} builtins[] =
	{
		{ "gl_Position",				1	},
		{ "gl_VertexID",				1	},
		{ "gl_FragCoord",				1	},
		{ "gl_PerVertex.gl_Position",	1	},
		{ "gl_InvocationID",			1	},
		{ "gl_NumWorkGroups",			1	},
		{ "gl_FragDepth",				1	},
		{ "gl_TessLevelOuter[0]",		4	},
		{ "gl_TessLevelInner[0]",		2	},
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(builtins); ++ndx)
	{
		if (resource == builtins[ndx].name)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying array size, expecting " << builtins[ndx].arraySize << tcu::TestLog::EndMessage;

			if (propValue != builtins[ndx].arraySize)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
				setError("resource array size invalid");
			}
			return;
		}
	}

	DE_ASSERT(false);
}

class ArrayStrideValidator : public SingleVariableValidator
{
public:
				ArrayStrideValidator			(Context& context, glw::GLuint programID, const VariableSearchFilter& filter);

	void		validateSingleVariable			(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

ArrayStrideValidator::ArrayStrideValidator (Context& context, glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleVariableValidator(context, PROGRAMRESOURCEPROP_ARRAY_STRIDE, programID, filter, DE_NULL)
{
}

void ArrayStrideValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	const VariablePathComponent		nullComponent;
	const VariablePathComponent&	component			= path.back();
	const VariablePathComponent&	enclosingcomponent	= (path.size() > 1) ? (path[path.size()-2]) : (nullComponent);
	const VariablePathComponent&	firstComponent		= path.front();

	const bool						isBufferBlock		= firstComponent.isInterfaceBlock() && isBufferBackedInterfaceBlockStorage(firstComponent.getInterfaceBlock()->storage);
	const bool						isArray				= enclosingcomponent.isVariableType() && enclosingcomponent.getVariableType()->isArrayType();
	const bool						isAtomicCounter		= glu::isDataTypeAtomicCounter(component.getVariableType()->getBasicType()); // atomic counters are buffer backed with a stride of 4 basic machine units

	DE_UNREF(resource);
	DE_UNREF(implementationName);

	// Layout tests will verify layouts of buffer backed arrays properly. Here we just check values are greater or equal to the element size
	if (isBufferBlock && isArray)
	{
		const int elementSize = glu::getDataTypeScalarSize(component.getVariableType()->getBasicType()) * getTypeSize(glu::getDataTypeScalarType(component.getVariableType()->getBasicType()));
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying array stride, expecting greater or equal to " << elementSize << tcu::TestLog::EndMessage;

		if (propValue < elementSize)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
			setError("resource array stride invalid");
		}
	}
	else
	{
		// Atomics are buffer backed with stride of 4 even though they are not in an interface block
		const int arrayStride = (isAtomicCounter && isArray) ? (4) : (!isBufferBlock && !isAtomicCounter) ? (-1) : (0);

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying array stride, expecting " << arrayStride << tcu::TestLog::EndMessage;

		if (arrayStride != propValue)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
			setError("resource array stride invalid");
		}
	}
}

class BlockIndexValidator : public SingleVariableValidator
{
public:
				BlockIndexValidator				(Context& context, glw::GLuint programID, const VariableSearchFilter& filter);

	void		validateSingleVariable			(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

BlockIndexValidator::BlockIndexValidator (Context& context, glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleVariableValidator(context, PROGRAMRESOURCEPROP_BLOCK_INDEX, programID, filter, DE_NULL)
{
}

void BlockIndexValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	const VariablePathComponent& firstComponent = path.front();

	DE_UNREF(resource);
	DE_UNREF(implementationName);

	if (!firstComponent.isInterfaceBlock())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying block index, expecting -1" << tcu::TestLog::EndMessage;

		if (propValue != -1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
			setError("resource block index invalid");
		}
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying block index, expecting a valid block index" << tcu::TestLog::EndMessage;

		if (propValue == -1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
			setError("resource block index invalid");
		}
		else
		{
			const glw::Functions&	gl			= m_renderContext.getFunctions();
			const glw::GLenum		interface	= (firstComponent.getInterfaceBlock()->storage == glu::STORAGE_UNIFORM) ? (GL_UNIFORM_BLOCK) :
												  (firstComponent.getInterfaceBlock()->storage == glu::STORAGE_BUFFER) ? (GL_SHADER_STORAGE_BLOCK) :
												  (0);
			glw::GLint				written		= 0;
			std::vector<char>		nameBuffer	(firstComponent.getInterfaceBlock()->interfaceName.size() + 3 * firstComponent.getInterfaceBlock()->dimensions.size() + 2, '\0'); // +3 for appended "[N]", +1 for '\0' and +1 just for safety

			gl.getProgramResourceName(m_programID, interface, propValue, (int)nameBuffer.size() - 1, &written, &nameBuffer[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "query block name");
			TCU_CHECK(written < (int)nameBuffer.size());
			TCU_CHECK(nameBuffer.back() == '\0');

			{
				const std::string	blockName		(&nameBuffer[0], written);
				std::ostringstream	expectedName;

				expectedName << firstComponent.getInterfaceBlock()->interfaceName;
				for (int dimensionNdx = 0; dimensionNdx < (int)firstComponent.getInterfaceBlock()->dimensions.size(); ++dimensionNdx)
					expectedName << "[0]";

				m_testCtx.getLog() << tcu::TestLog::Message << "Block name with index " << propValue << " is \"" << blockName << "\"" << tcu::TestLog::EndMessage;
				if (blockName != expectedName.str())
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "\tError, expected " << expectedName.str() << tcu::TestLog::EndMessage;
					setError("resource block index invalid");
				}
			}
		}
	}
}

class IsRowMajorValidator : public SingleVariableValidator
{
public:
				IsRowMajorValidator				(Context& context, glw::GLuint programID, const VariableSearchFilter& filter);

	std::string getHumanReadablePropertyString	(glw::GLint propVal) const;
	void		validateSingleVariable			(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

IsRowMajorValidator::IsRowMajorValidator (Context& context, glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleVariableValidator(context, PROGRAMRESOURCEPROP_MATRIX_ROW_MAJOR, programID, filter, DE_NULL)
{
}

std::string IsRowMajorValidator::getHumanReadablePropertyString (glw::GLint propVal) const
{
	return de::toString(glu::getBooleanStr(propVal));
}

void IsRowMajorValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	const VariablePathComponent&	component			= path.back();
	const VariablePathComponent&	firstComponent		= path.front();

	const bool						isBufferBlock		= firstComponent.isInterfaceBlock() && isBufferBackedInterfaceBlockStorage(firstComponent.getInterfaceBlock()->storage);
	const bool						isMatrix			= glu::isDataTypeMatrix(component.getVariableType()->getBasicType());
	const int						expected			= (isBufferBlock && isMatrix && getMatrixOrderFromPath(path) == glu::MATRIXORDER_ROW_MAJOR) ? (1) : (0);

	DE_UNREF(resource);
	DE_UNREF(implementationName);

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying matrix order, expecting IS_ROW_MAJOR = " << expected << tcu::TestLog::EndMessage;

	if (propValue != expected)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
		setError("resource matrix order invalid");
	}
}

class MatrixStrideValidator : public SingleVariableValidator
{
public:
				MatrixStrideValidator			(Context& context, glw::GLuint programID, const VariableSearchFilter& filter);

	void		validateSingleVariable			(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

MatrixStrideValidator::MatrixStrideValidator (Context& context, glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleVariableValidator(context, PROGRAMRESOURCEPROP_MATRIX_STRIDE, programID, filter, DE_NULL)
{
}

void MatrixStrideValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	const VariablePathComponent&	component			= path.back();
	const VariablePathComponent&	firstComponent		= path.front();

	const bool						isBufferBlock		= firstComponent.isInterfaceBlock() && isBufferBackedInterfaceBlockStorage(firstComponent.getInterfaceBlock()->storage);
	const bool						isMatrix			= glu::isDataTypeMatrix(component.getVariableType()->getBasicType());

	DE_UNREF(resource);
	DE_UNREF(implementationName);

	// Layout tests will verify layouts of buffer backed arrays properly. Here we just check the stride is is greater or equal to the row/column size
	if (isBufferBlock && isMatrix)
	{
		const bool	columnMajor			= getMatrixOrderFromPath(path) != glu::MATRIXORDER_ROW_MAJOR;
		const int	numMajorElements	= (columnMajor) ? (glu::getDataTypeMatrixNumRows(component.getVariableType()->getBasicType())) : (glu::getDataTypeMatrixNumColumns(component.getVariableType()->getBasicType()));
		const int	majorSize			= numMajorElements * getTypeSize(glu::getDataTypeScalarType(component.getVariableType()->getBasicType()));

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying matrix stride, expecting greater or equal to " << majorSize << tcu::TestLog::EndMessage;

		if (propValue < majorSize)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
			setError("resource matrix stride invalid");
		}
	}
	else
	{
		const int matrixStride = (!isBufferBlock && !glu::isDataTypeAtomicCounter(component.getVariableType()->getBasicType())) ? (-1) : (0);

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying matrix stride, expecting " << matrixStride << tcu::TestLog::EndMessage;

		if (matrixStride != propValue)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
			setError("resource matrix stride invalid");
		}
	}
}

class AtomicCounterBufferIndexVerifier : public SingleVariableValidator
{
public:
				AtomicCounterBufferIndexVerifier	(Context& context, glw::GLuint programID, const VariableSearchFilter& filter);

	void		validateSingleVariable				(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

AtomicCounterBufferIndexVerifier::AtomicCounterBufferIndexVerifier (Context& context, glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleVariableValidator(context, PROGRAMRESOURCEPROP_ATOMIC_COUNTER_BUFFER_INDEX, programID, filter, DE_NULL)
{
}

void AtomicCounterBufferIndexVerifier::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(resource);
	DE_UNREF(implementationName);

	if (!glu::isDataTypeAtomicCounter(path.back().getVariableType()->getBasicType()))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying atomic counter buffer index, expecting -1" << tcu::TestLog::EndMessage;

		if (propValue != -1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
			setError("resource atomic counter buffer index invalid");
		}
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying atomic counter buffer index, expecting a valid index" << tcu::TestLog::EndMessage;

		if (propValue == -1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
			setError("resource atomic counter buffer index invalid");
		}
		else
		{
			const glw::Functions&	gl					= m_renderContext.getFunctions();
			glw::GLint				numActiveResources	= 0;

			gl.getProgramInterfaceiv(m_programID, GL_ATOMIC_COUNTER_BUFFER, GL_ACTIVE_RESOURCES, &numActiveResources);
			GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramInterfaceiv(..., GL_ATOMIC_COUNTER_BUFFER, GL_ACTIVE_RESOURCES, ...)");

			if (propValue >= numActiveResources)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << ", GL_ACTIVE_RESOURCES = " << numActiveResources << tcu::TestLog::EndMessage;
				setError("resource atomic counter buffer index invalid");
			}
		}
	}
}

class LocationValidator : public SingleVariableValidator
{
public:
				LocationValidator		(Context& context, glw::GLuint programID, const VariableSearchFilter& filter);

	void		validateSingleVariable	(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
	void		validateBuiltinVariable	(const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

LocationValidator::LocationValidator (Context& context, glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleVariableValidator(context, PROGRAMRESOURCEPROP_LOCATION, programID, filter, DE_NULL)
{
}

static int getVariableLocationLength (const glu::VarType& type)
{
	if (type.isBasicType())
	{
		if (glu::isDataTypeMatrix(type.getBasicType()))
			return glu::getDataTypeMatrixNumColumns(type.getBasicType());
		else
			return 1;
	}
	else if (type.isStructType())
	{
		int size = 0;
		for (int ndx = 0; ndx < type.getStructPtr()->getNumMembers(); ++ndx)
			size += getVariableLocationLength(type.getStructPtr()->getMember(ndx).getType());
		return size;
	}
	else if (type.isArrayType())
		return type.getArraySize() * getVariableLocationLength(type.getElementType());
	else
	{
		DE_ASSERT(false);
		return 0;
	}
}

static int getIOSubVariableLocation (const std::vector<VariablePathComponent>& path, int startNdx, int currentLocation)
{
	if (currentLocation == -1)
		return -1;

	if (path[startNdx].getVariableType()->isBasicType())
		return currentLocation;
	else if (path[startNdx].getVariableType()->isArrayType())
		return getIOSubVariableLocation(path, startNdx+1, currentLocation);
	else if (path[startNdx].getVariableType()->isStructType())
	{
		for (int ndx = 0; ndx < path[startNdx].getVariableType()->getStructPtr()->getNumMembers(); ++ndx)
		{
			if (&path[startNdx].getVariableType()->getStructPtr()->getMember(ndx).getType() == path[startNdx + 1].getVariableType())
				return getIOSubVariableLocation(path, startNdx + 1, currentLocation);

			if (currentLocation != -1)
				currentLocation += getVariableLocationLength(path[startNdx].getVariableType()->getStructPtr()->getMember(ndx).getType());
		}

		// could not find member, never happens
		DE_ASSERT(false);
		return -1;
	}
	else
	{
		DE_ASSERT(false);
		return -1;
	}
}

static int getIOBlockVariableLocation (const std::vector<VariablePathComponent>& path)
{
	const glu::InterfaceBlock*	block			= path.front().getInterfaceBlock();
	int							currentLocation	= block->layout.location;

	// Find the block member
	for (int memberNdx = 0; memberNdx < (int)block->variables.size(); ++memberNdx)
	{
		if (block->variables[memberNdx].layout.location != -1)
			currentLocation = block->variables[memberNdx].layout.location;

		if (&block->variables[memberNdx] == path[1].getDeclaration())
			break;

		// unspecified + unspecified = unspecified
		if (currentLocation != -1)
			currentLocation += getVariableLocationLength(block->variables[memberNdx].varType);
	}

	// Find subtype location in the complex type
	return getIOSubVariableLocation(path, 2, currentLocation);
}

static int getExplicitLocationFromPath (const std::vector<VariablePathComponent>& path)
{
	const glu::VariableDeclaration* varDecl = (path[0].isInterfaceBlock()) ? (path[1].getDeclaration()) : (path[0].getDeclaration());

	if (path.front().isInterfaceBlock() && path.front().getInterfaceBlock()->storage == glu::STORAGE_UNIFORM)
	{
		// inside uniform block
		return -1;
	}
	else if (path.front().isInterfaceBlock() && (path.front().getInterfaceBlock()->storage == glu::STORAGE_IN		||
												 path.front().getInterfaceBlock()->storage == glu::STORAGE_OUT		||
												 path.front().getInterfaceBlock()->storage == glu::STORAGE_PATCH_IN	||
												 path.front().getInterfaceBlock()->storage == glu::STORAGE_PATCH_OUT))
	{
		// inside ioblock
		return getIOBlockVariableLocation(path);
	}
	else if (varDecl->storage == glu::STORAGE_UNIFORM)
	{
		// default block uniform
		return varDecl->layout.location;
	}
	else if (varDecl->storage == glu::STORAGE_IN		||
			 varDecl->storage == glu::STORAGE_OUT		||
			 varDecl->storage == glu::STORAGE_PATCH_IN	||
			 varDecl->storage == glu::STORAGE_PATCH_OUT)
	{
		// default block input/output
		return getIOSubVariableLocation(path, 1, varDecl->layout.location);
	}
	else
	{
		DE_ASSERT(false);
		return -1;
	}
}

void LocationValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	const bool			isAtomicCounterUniform	= glu::isDataTypeAtomicCounter(path.back().getVariableType()->getBasicType());
	const bool			isUniformBlockVariable	= path.front().isInterfaceBlock() && path.front().getInterfaceBlock()->storage == glu::STORAGE_UNIFORM;
	const bool			isVertexShader			= m_filter.getShaderTypeBits() == (1u << glu::SHADERTYPE_VERTEX);
	const bool			isFragmentShader		= m_filter.getShaderTypeBits() == (1u << glu::SHADERTYPE_FRAGMENT);
	const glu::Storage	storage					= (path.front().isInterfaceBlock()) ? (path.front().getInterfaceBlock()->storage) : (path.front().getDeclaration()->storage);
	const bool			isInputVariable			= (storage == glu::STORAGE_IN || storage == glu::STORAGE_PATCH_IN);
	const bool			isOutputVariable		= (storage == glu::STORAGE_OUT || storage == glu::STORAGE_PATCH_OUT);
	const int			explicitLayoutLocation	= getExplicitLocationFromPath(path);

	bool				expectLocation;
	std::string			reasonStr;

	DE_UNREF(resource);

	if (isAtomicCounterUniform)
	{
		expectLocation = false;
		reasonStr = "Atomic counter uniforms have effective location of -1";
	}
	else if (isUniformBlockVariable)
	{
		expectLocation = false;
		reasonStr = "Uniform block variables have effective location of -1";
	}
	else if (isInputVariable && !isVertexShader && explicitLayoutLocation == -1)
	{
		expectLocation = false;
		reasonStr = "Inputs (except for vertex shader inputs) not declared with a location layout qualifier have effective location of -1";
	}
	else if (isOutputVariable && !isFragmentShader && explicitLayoutLocation == -1)
	{
		expectLocation = false;
		reasonStr = "Outputs (except for fragment shader outputs) not declared with a location layout qualifier have effective location of -1";
	}
	else
	{
		expectLocation = true;
	}

	if (!expectLocation)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying uniform location, expecting -1. (" << reasonStr << ")" << tcu::TestLog::EndMessage;

		if (propValue != -1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
			setError("resource location invalid");
		}
	}
	else
	{
		bool locationOk;

		if (explicitLayoutLocation == -1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying location, expecting a valid location" << tcu::TestLog::EndMessage;
			locationOk = (propValue != -1);
		}
		else
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying location, expecting " << explicitLayoutLocation << tcu::TestLog::EndMessage;
			locationOk = (propValue == explicitLayoutLocation);
		}

		if (!locationOk)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
			setError("resource location invalid");
		}
		else
		{
			const VariablePathComponent		nullComponent;
			const VariablePathComponent&	enclosingcomponent	= (path.size() > 1) ? (path[path.size()-2]) : (nullComponent);
			const bool						isArray				= enclosingcomponent.isVariableType() && enclosingcomponent.getVariableType()->isArrayType();

			const glw::Functions&			gl					= m_renderContext.getFunctions();
			const glw::GLenum				interface			= getProgramDefaultBlockInterfaceFromStorage(storage);

			m_testCtx.getLog() << tcu::TestLog::Message << "Comparing location to the values returned by GetProgramResourceLocation" << tcu::TestLog::EndMessage;

			// Test all bottom-level array elements
			if (isArray)
			{
				const std::string arrayResourceName = (implementationName.size() > 3) ? (implementationName.substr(0, implementationName.size() - 3)) : (""); // chop "[0]"

				for (int arrayElementNdx = 0; arrayElementNdx < enclosingcomponent.getVariableType()->getArraySize(); ++arrayElementNdx)
				{
					const std::string	elementResourceName	= arrayResourceName + "[" + de::toString(arrayElementNdx) + "]";
					const glw::GLint	location			= gl.getProgramResourceLocation(m_programID, interface, elementResourceName.c_str());

					if (location != propValue+arrayElementNdx)
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message
							<< "\tError, getProgramResourceLocation (resource=\"" << elementResourceName << "\") returned location " << location
							<< ", expected " << (propValue+arrayElementNdx)
							<< tcu::TestLog::EndMessage;
						setError("resource location invalid");
					}
					else
						m_testCtx.getLog() << tcu::TestLog::Message << "\tLocation of \"" << elementResourceName << "\":\t" << location << tcu::TestLog::EndMessage;
				}
			}
			else
			{
				const glw::GLint location = gl.getProgramResourceLocation(m_programID, interface, implementationName.c_str());

				if (location != propValue)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "\tError, getProgramResourceLocation returned location " << location << ", expected " << propValue << tcu::TestLog::EndMessage;
					setError("resource location invalid");
				}
			}

		}
	}
}

void LocationValidator::validateBuiltinVariable (const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(resource);
	DE_UNREF(implementationName);

	// built-ins have no location

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying location, expecting -1" << tcu::TestLog::EndMessage;

	if (propValue != -1)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
		setError("resource location invalid");
	}
}

class VariableNameLengthValidator : public SingleVariableValidator
{
public:
				VariableNameLengthValidator	(Context& context, glw::GLuint programID, const VariableSearchFilter& filter);

	void		validateSingleVariable		(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
	void		validateBuiltinVariable		(const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
	void		validateNameLength			(const std::string& implementationName, glw::GLint propValue) const;
};

VariableNameLengthValidator::VariableNameLengthValidator (Context& context, glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleVariableValidator(context, PROGRAMRESOURCEPROP_NAME_LENGTH, programID, filter, DE_NULL)
{
}

void VariableNameLengthValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(path);
	DE_UNREF(resource);
	validateNameLength(implementationName, propValue);
}

void VariableNameLengthValidator::validateBuiltinVariable (const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(resource);
	validateNameLength(implementationName, propValue);
}

void VariableNameLengthValidator::validateNameLength (const std::string& implementationName, glw::GLint propValue) const
{
	const int expected = (int)implementationName.length() + 1; // includes null byte
	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying name length, expecting " << expected << " (" << (int)implementationName.length() << " for \"" << implementationName << "\" + 1 byte for terminating null character)" << tcu::TestLog::EndMessage;

	if (propValue != expected)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, invalid name length, got " << propValue << tcu::TestLog::EndMessage;
		setError("name length invalid");
	}
}

class OffsetValidator : public SingleVariableValidator
{
public:
				OffsetValidator			(Context& context, glw::GLuint programID, const VariableSearchFilter& filter);

	void		validateSingleVariable	(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

OffsetValidator::OffsetValidator (Context& context, glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleVariableValidator(context, PROGRAMRESOURCEPROP_OFFSET, programID, filter, DE_NULL)
{
}

void OffsetValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	const bool isAtomicCounterUniform		= glu::isDataTypeAtomicCounter(path.back().getVariableType()->getBasicType());
	const bool isBufferBackedBlockStorage	= path.front().isInterfaceBlock() && isBufferBackedInterfaceBlockStorage(path.front().getInterfaceBlock()->storage);

	DE_UNREF(resource);
	DE_UNREF(implementationName);

	if (!isAtomicCounterUniform && !isBufferBackedBlockStorage)
	{
		// Not buffer backed
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying offset, expecting -1" << tcu::TestLog::EndMessage;

		if (propValue != -1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, invalid offset, got " << propValue << tcu::TestLog::EndMessage;
			setError("offset invalid");
		}
	}
	else
	{
		// Expect a valid offset
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying offset, expecting a valid offset" << tcu::TestLog::EndMessage;

		if (propValue < 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, invalid offset, got " << propValue << tcu::TestLog::EndMessage;
			setError("offset invalid");
		}
	}
}

class VariableReferencedByShaderValidator : public PropValidator
{
public:
								VariableReferencedByShaderValidator	(Context& context, glu::ShaderType shaderType, const VariableSearchFilter& searchFilter);

	std::string					getHumanReadablePropertyString		(glw::GLint propVal) const;
	void						validate							(const ProgramInterfaceDefinition::Program* program, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;

private:
	const VariableSearchFilter	m_filter;
	const glu::ShaderType		m_shaderType;
};

VariableReferencedByShaderValidator::VariableReferencedByShaderValidator (Context& context, glu::ShaderType shaderType, const VariableSearchFilter& searchFilter)
	: PropValidator	(context, PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER, getRequiredExtensionForStage(shaderType))
	, m_filter		(VariableSearchFilter::logicalAnd(VariableSearchFilter::createShaderTypeFilter(shaderType), searchFilter))
	, m_shaderType	(shaderType)
{
	DE_ASSERT(m_shaderType < glu::SHADERTYPE_LAST);
}

std::string VariableReferencedByShaderValidator::getHumanReadablePropertyString (glw::GLint propVal) const
{
	return de::toString(glu::getBooleanStr(propVal));
}

void VariableReferencedByShaderValidator::validate (const ProgramInterfaceDefinition::Program* program, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(implementationName);

	std::vector<VariablePathComponent>	dummyPath;
	const bool							referencedByShader = findProgramVariablePathByPathName(dummyPath, program, resource, m_filter);

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying referenced by " << glu::getShaderTypeName(m_shaderType) << " shader, expecting "
		<< ((referencedByShader) ? ("GL_TRUE") : ("GL_FALSE"))
		<< tcu::TestLog::EndMessage;

	if (propValue != ((referencedByShader) ? (1) : (0)))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, invalid referenced_by_" << glu::getShaderTypeName(m_shaderType) << ", got " << propValue << tcu::TestLog::EndMessage;
		setError("referenced_by_" + std::string(glu::getShaderTypeName(m_shaderType)) + " invalid");
	}
}

class BlockNameLengthValidator : public SingleBlockValidator
{
public:
			BlockNameLengthValidator	(Context& context, const glw::GLuint programID, const VariableSearchFilter& filter);

	void	validateSingleBlock			(const glu::InterfaceBlock& block, const std::vector<int>& instanceIndex, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

BlockNameLengthValidator::BlockNameLengthValidator (Context& context, const glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleBlockValidator(context, PROGRAMRESOURCEPROP_NAME_LENGTH, programID, filter, DE_NULL)
{
}

void BlockNameLengthValidator::validateSingleBlock (const glu::InterfaceBlock& block, const std::vector<int>& instanceIndex, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(instanceIndex);
	DE_UNREF(block);
	DE_UNREF(resource);

	const int expected = (int)implementationName.length() + 1; // includes null byte
	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying name length, expecting " << expected << " (" << (int)implementationName.length() << " for \"" << implementationName << "\" + 1 byte for terminating null character)" << tcu::TestLog::EndMessage;

	if (propValue != expected)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, invalid name length, got " << propValue << tcu::TestLog::EndMessage;
		setError("name length invalid");
	}
}

class BufferBindingValidator : public SingleBlockValidator
{
public:
			BufferBindingValidator	(Context& context, const glw::GLuint programID, const VariableSearchFilter& filter);

	void	validateSingleBlock		(const glu::InterfaceBlock& block, const std::vector<int>& instanceIndex, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

BufferBindingValidator::BufferBindingValidator (Context& context, const glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleBlockValidator(context, PROGRAMRESOURCEPROP_BUFFER_BINDING, programID, filter, DE_NULL)
{
}

void BufferBindingValidator::validateSingleBlock (const glu::InterfaceBlock& block, const std::vector<int>& instanceIndex, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(resource);
	DE_UNREF(implementationName);

	if (block.layout.binding != -1)
	{
		int flatIndex		= 0;
		int dimensionSize	= 1;

		for (int dimensionNdx = (int)(block.dimensions.size()) - 1; dimensionNdx >= 0; --dimensionNdx)
		{
			flatIndex += dimensionSize * instanceIndex[dimensionNdx];
			dimensionSize *= block.dimensions[dimensionNdx];
		}

		const int expected = (block.dimensions.empty()) ? (block.layout.binding) : (block.layout.binding + flatIndex);
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying block binding, expecting " << expected << tcu::TestLog::EndMessage;

		if (propValue != expected)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, invalid buffer binding, got " << propValue << tcu::TestLog::EndMessage;
			setError("buffer binding invalid");
		}
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying buffer binding, expecting a valid binding" << tcu::TestLog::EndMessage;

		if (propValue < 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, invalid buffer binding, got " << propValue << tcu::TestLog::EndMessage;
			setError("buffer binding invalid");
		}
	}
}

class BlockReferencedByShaderValidator : public PropValidator
{
public:
								BlockReferencedByShaderValidator	(Context& context, glu::ShaderType shaderType, const VariableSearchFilter& searchFilter);

	std::string					getHumanReadablePropertyString		(glw::GLint propVal) const;
	void						validate							(const ProgramInterfaceDefinition::Program* program, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;

private:
	const VariableSearchFilter	m_filter;
	const glu::ShaderType		m_shaderType;
};

BlockReferencedByShaderValidator::BlockReferencedByShaderValidator (Context& context, glu::ShaderType shaderType, const VariableSearchFilter& searchFilter)
	: PropValidator	(context, PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER, getRequiredExtensionForStage(shaderType))
	, m_filter		(VariableSearchFilter::logicalAnd(VariableSearchFilter::createShaderTypeFilter(shaderType), searchFilter))
	, m_shaderType	(shaderType)
{
	DE_ASSERT(m_shaderType < glu::SHADERTYPE_LAST);
}

std::string BlockReferencedByShaderValidator::getHumanReadablePropertyString (glw::GLint propVal) const
{
	return de::toString(glu::getBooleanStr(propVal));
}

void BlockReferencedByShaderValidator::validate (const ProgramInterfaceDefinition::Program* program, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	const std::string	blockName			= glu::parseVariableName(resource.c_str());
	bool				referencedByShader	= false;

	DE_UNREF(implementationName);

	for (int shaderNdx = 0; shaderNdx < (int)program->getShaders().size(); ++shaderNdx)
	{
		const ProgramInterfaceDefinition::Shader* const shader = program->getShaders()[shaderNdx];
		if (!m_filter.matchesFilter(shader))
			continue;

		for (int blockNdx = 0; blockNdx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++blockNdx)
		{
			const glu::InterfaceBlock& block = shader->getDefaultBlock().interfaceBlocks[blockNdx];

			if (m_filter.matchesFilter(block) && block.interfaceName == blockName)
				referencedByShader = true;
		}
	}

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying referenced by " << glu::getShaderTypeName(m_shaderType) << " shader, expecting "
		<< ((referencedByShader) ? ("GL_TRUE") : ("GL_FALSE"))
		<< tcu::TestLog::EndMessage;

	if (propValue != ((referencedByShader) ? (1) : (0)))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, invalid referenced_by_" << glu::getShaderTypeName(m_shaderType) << ", got " << propValue << tcu::TestLog::EndMessage;
		setError("referenced_by_" + std::string(glu::getShaderTypeName(m_shaderType)) + " invalid");
	}
}

class TopLevelArraySizeValidator : public SingleVariableValidator
{
public:
				TopLevelArraySizeValidator	(Context& context, glw::GLuint programID, const VariableSearchFilter& filter);

	void		validateSingleVariable		(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

TopLevelArraySizeValidator::TopLevelArraySizeValidator (Context& context, glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleVariableValidator(context, PROGRAMRESOURCEPROP_TOP_LEVEL_ARRAY_SIZE, programID, filter, DE_NULL)
{
}

void TopLevelArraySizeValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	int			expected;
	std::string	reason;

	DE_ASSERT(path.front().isInterfaceBlock() && path.front().getInterfaceBlock()->storage == glu::STORAGE_BUFFER);
	DE_UNREF(resource);
	DE_UNREF(implementationName);

	if (!path[1].getDeclaration()->varType.isArrayType())
	{
		expected = 1;
		reason = "Top-level block member is not an array";
	}
	else if (path[1].getDeclaration()->varType.getElementType().isBasicType())
	{
		expected = 1;
		reason = "Top-level block member is not an array of an aggregate type";
	}
	else if (path[1].getDeclaration()->varType.getArraySize() == glu::VarType::UNSIZED_ARRAY)
	{
		expected = 0;
		reason = "Top-level block member is an unsized top-level array";
	}
	else
	{
		expected = path[1].getDeclaration()->varType.getArraySize();
		reason = "Top-level block member is a sized top-level array";
	}

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying top level array size, expecting " << expected << ". (" << reason << ")." << tcu::TestLog::EndMessage;

	if (propValue != expected)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, invalid top level array size, got " << propValue << tcu::TestLog::EndMessage;
		setError("top level array size invalid");
	}
}

class TopLevelArrayStrideValidator : public SingleVariableValidator
{
public:
				TopLevelArrayStrideValidator	(Context& context, glw::GLuint programID, const VariableSearchFilter& filter);

	void		validateSingleVariable			(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

TopLevelArrayStrideValidator::TopLevelArrayStrideValidator (Context& context, glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleVariableValidator(context, PROGRAMRESOURCEPROP_TOP_LEVEL_ARRAY_STRIDE, programID, filter, DE_NULL)
{
}

void TopLevelArrayStrideValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_ASSERT(path.front().isInterfaceBlock() && path.front().getInterfaceBlock()->storage == glu::STORAGE_BUFFER);
	DE_UNREF(resource);
	DE_UNREF(implementationName);

	if (!path[1].getDeclaration()->varType.isArrayType())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying top level array stride, expecting 0. (Top-level block member is not an array)." << tcu::TestLog::EndMessage;

		if (propValue != 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, top level array stride, got " << propValue << tcu::TestLog::EndMessage;
			setError("top level array stride invalid");
		}
	}
	else if (path[1].getDeclaration()->varType.getElementType().isBasicType())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying top level array stride, expecting 0. (Top-level block member is not an array of an aggregate type)." << tcu::TestLog::EndMessage;

		if (propValue != 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, top level array stride, got " << propValue << tcu::TestLog::EndMessage;
			setError("top level array stride invalid");
		}
	}
	else
	{
		const int minimumStride = getVarTypeSize(path[1].getDeclaration()->varType.getElementType());

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying top level array stride, expecting greater or equal to " << minimumStride << "." << tcu::TestLog::EndMessage;

		if (propValue < minimumStride)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "\tError, invalid top level array stride, got " << propValue << tcu::TestLog::EndMessage;
			setError("top level array stride invalid");
		}
	}
}

class TransformFeedbackResourceValidator : public PropValidator
{
public:
					TransformFeedbackResourceValidator	(Context& context, ProgramResourcePropFlags validationProp);

	void			validate							(const ProgramInterfaceDefinition::Program* program, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;

private:
	virtual void	validateBuiltinVariable				(const std::string& resource, glw::GLint propValue, const std::string& implementationName) const = 0;
	virtual void	validateSingleVariable				(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const = 0;
};


TransformFeedbackResourceValidator::TransformFeedbackResourceValidator (Context& context, ProgramResourcePropFlags validationProp)
	: PropValidator(context, validationProp, DE_NULL)
{
}

void TransformFeedbackResourceValidator::validate (const ProgramInterfaceDefinition::Program* program, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	if (deStringBeginsWith(resource.c_str(), "gl_"))
	{
		validateBuiltinVariable(resource, propValue, implementationName);
	}
	else
	{
		// Check resource name is a xfb output. (sanity check)
#if defined(DE_DEBUG)
		bool generatorFound = false;

		// Check the resource name is a valid transform feedback resource and find the name generating resource
		for (int varyingNdx = 0; varyingNdx < (int)program->getTransformFeedbackVaryings().size(); ++varyingNdx)
		{
			const std::string					varyingName = program->getTransformFeedbackVaryings()[varyingNdx];
			std::vector<VariablePathComponent>	path;
			std::vector<std::string>			resources;

			if (!findProgramVariablePathByPathName(path, program, varyingName, VariableSearchFilter::createShaderTypeStorageFilter(getProgramTransformFeedbackStage(program), glu::STORAGE_OUT)))
			{
				// program does not contain feedback varying, not valid program
				DE_ASSERT(false);
				return;
			}

			generateVariableTypeResourceNames(resources, varyingName, *path.back().getVariableType(), RESOURCE_NAME_GENERATION_FLAG_TRANSFORM_FEEDBACK_VARIABLE);

			if (de::contains(resources.begin(), resources.end(), resource))
			{
				generatorFound = true;
				break;
			}
		}

		// resource name was not found, should never happen
		DE_ASSERT(generatorFound);
		DE_UNREF(generatorFound);
#endif

		// verify resource
		{
			std::vector<VariablePathComponent> path;

			if (!findProgramVariablePathByPathName(path, program, resource, VariableSearchFilter::createShaderTypeStorageFilter(getProgramTransformFeedbackStage(program), glu::STORAGE_OUT)))
				DE_ASSERT(false);

			validateSingleVariable(path, resource, propValue, implementationName);
		}
	}
}

class TransformFeedbackArraySizeValidator : public TransformFeedbackResourceValidator
{
public:
				TransformFeedbackArraySizeValidator	(Context& context);

	void		validateBuiltinVariable				(const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
	void		validateSingleVariable				(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

TransformFeedbackArraySizeValidator::TransformFeedbackArraySizeValidator (Context& context)
	: TransformFeedbackResourceValidator(context, PROGRAMRESOURCEPROP_ARRAY_SIZE)
{
}

void TransformFeedbackArraySizeValidator::validateBuiltinVariable (const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(implementationName);

	int arraySize = 0;

	if (resource == "gl_Position")
		arraySize = 1;
	else
		DE_ASSERT(false);

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying array size, expecting " << arraySize << tcu::TestLog::EndMessage;
	if (arraySize != propValue)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
		setError("resource array size invalid");
	}
}

void TransformFeedbackArraySizeValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(resource);
	DE_UNREF(implementationName);

	const int arraySize = (path.back().getVariableType()->isArrayType()) ? (path.back().getVariableType()->getArraySize()) : (1);

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying array size, expecting " << arraySize << tcu::TestLog::EndMessage;
	if (arraySize != propValue)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
		setError("resource array size invalid");
	}
}

class TransformFeedbackNameLengthValidator : public TransformFeedbackResourceValidator
{
public:
				TransformFeedbackNameLengthValidator	(Context& context);

private:
	void		validateBuiltinVariable					(const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
	void		validateSingleVariable					(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
	void		validateVariable						(const std::string& implementationName, glw::GLint propValue) const;
};

TransformFeedbackNameLengthValidator::TransformFeedbackNameLengthValidator (Context& context)
	: TransformFeedbackResourceValidator(context, PROGRAMRESOURCEPROP_NAME_LENGTH)
{
}

void TransformFeedbackNameLengthValidator::validateBuiltinVariable (const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(resource);
	validateVariable(implementationName, propValue);
}

void TransformFeedbackNameLengthValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(path);
	DE_UNREF(resource);
	validateVariable(implementationName, propValue);
}

void TransformFeedbackNameLengthValidator::validateVariable (const std::string& implementationName, glw::GLint propValue) const
{
	const int expected = (int)implementationName.length() + 1; // includes null byte
	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying name length, expecting " << expected << " (" << (int)implementationName.length() << " for \"" << implementationName << "\" + 1 byte for terminating null character)" << tcu::TestLog::EndMessage;

	if (propValue != expected)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, invalid name length, got " << propValue << tcu::TestLog::EndMessage;
		setError("name length invalid");
	}
}

class TransformFeedbackTypeValidator : public TransformFeedbackResourceValidator
{
public:
				TransformFeedbackTypeValidator		(Context& context);

	void		validateBuiltinVariable				(const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
	void		validateSingleVariable				(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

TransformFeedbackTypeValidator::TransformFeedbackTypeValidator (Context& context)
	: TransformFeedbackResourceValidator(context, PROGRAMRESOURCEPROP_TYPE)
{
}

void TransformFeedbackTypeValidator::validateBuiltinVariable (const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(implementationName);

	glu::DataType varType = glu::TYPE_INVALID;

	if (resource == "gl_Position")
		varType = glu::TYPE_FLOAT_VEC4;
	else
		DE_ASSERT(false);

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying type, expecting " << glu::getDataTypeName(varType) << tcu::TestLog::EndMessage;
	if (glu::getDataTypeFromGLType(propValue) != varType)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << glu::getDataTypeName(glu::getDataTypeFromGLType(propValue)) << tcu::TestLog::EndMessage;
		setError("resource type invalid");
	}
	return;
}

void TransformFeedbackTypeValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(resource);
	DE_UNREF(implementationName);

	// Unlike other interfaces, xfb program interface uses just variable name to refer to arrays of basic types. (Others use "variable[0]")
	// Thus we might end up querying a type for an array. In this case, return the type of an array element.
	const glu::VarType& variable    = *path.back().getVariableType();
	const glu::VarType& elementType = (variable.isArrayType()) ? (variable.getElementType()) : (variable);

	DE_ASSERT(elementType.isBasicType());

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying type, expecting " << glu::getDataTypeName(elementType.getBasicType()) << tcu::TestLog::EndMessage;
	if (elementType.getBasicType() != glu::getDataTypeFromGLType(propValue))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << glu::getDataTypeName(glu::getDataTypeFromGLType(propValue)) << tcu::TestLog::EndMessage;
		setError("resource type invalid");
	}
}

class PerPatchValidator : public SingleVariableValidator
{
public:
				PerPatchValidator				(Context& context, glw::GLuint programID, const VariableSearchFilter& filter);

	std::string getHumanReadablePropertyString	(glw::GLint propVal) const;
	void		validateSingleVariable			(const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
	void		validateBuiltinVariable			(const std::string& resource, glw::GLint propValue, const std::string& implementationName) const;
};

PerPatchValidator::PerPatchValidator (Context& context, glw::GLuint programID, const VariableSearchFilter& filter)
	: SingleVariableValidator(context, PROGRAMRESOURCEPROP_IS_PER_PATCH, programID, filter, "GL_EXT_tessellation_shader")
{
}

std::string PerPatchValidator::getHumanReadablePropertyString (glw::GLint propVal) const
{
	return de::toString(glu::getBooleanStr(propVal));
}

void PerPatchValidator::validateSingleVariable (const std::vector<VariablePathComponent>& path, const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	const glu::Storage	storage		= (path.front().isInterfaceBlock()) ? (path.front().getInterfaceBlock()->storage) : (path.front().getDeclaration()->storage);
	const int			expected	= (storage == glu::STORAGE_PATCH_IN || storage == glu::STORAGE_PATCH_OUT) ? (1) : (0);

	DE_UNREF(resource);
	DE_UNREF(implementationName);

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying if is per patch, expecting IS_PER_PATCH = " << expected << tcu::TestLog::EndMessage;

	if (propValue != expected)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
		setError("resource is per patch invalid");
	}
}

void PerPatchValidator::validateBuiltinVariable (const std::string& resource, glw::GLint propValue, const std::string& implementationName) const
{
	DE_UNREF(implementationName);

	static const struct
	{
		const char*		name;
		int				isPerPatch;
	} builtins[] =
	{
		{ "gl_Position",				0	},
		{ "gl_PerVertex.gl_Position",	0	},
		{ "gl_InvocationID",			0	},
		{ "gl_TessLevelOuter[0]",		1	},
		{ "gl_TessLevelInner[0]",		1	},
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(builtins); ++ndx)
	{
		if (resource == builtins[ndx].name)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying if is per patch, expecting IS_PER_PATCH = " << builtins[ndx].isPerPatch << tcu::TestLog::EndMessage;

			if (propValue != builtins[ndx].isPerPatch)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "\tError, got " << propValue << tcu::TestLog::EndMessage;
				setError("resource is per patch invalid");
			}
			return;
		}
	}

	DE_ASSERT(false);
}

} // anonymous

ProgramResourceQueryTestTarget::ProgramResourceQueryTestTarget (ProgramInterface interface_, deUint32 propFlags_)
	: interface(interface_)
	, propFlags(propFlags_)
{
	switch (interface)
	{
		case PROGRAMINTERFACE_UNIFORM:						DE_ASSERT((propFlags & PROGRAMRESOURCEPROP_UNIFORM_INTERFACE_MASK)			== propFlags);	break;
		case PROGRAMINTERFACE_UNIFORM_BLOCK:				DE_ASSERT((propFlags & PROGRAMRESOURCEPROP_UNIFORM_BLOCK_INTERFACE_MASK)	== propFlags);	break;
		case PROGRAMINTERFACE_SHADER_STORAGE_BLOCK:			DE_ASSERT((propFlags & PROGRAMRESOURCEPROP_SHADER_STORAGE_BLOCK_MASK)		== propFlags);	break;
		case PROGRAMINTERFACE_PROGRAM_INPUT:				DE_ASSERT((propFlags & PROGRAMRESOURCEPROP_PROGRAM_INPUT_MASK)				== propFlags);	break;
		case PROGRAMINTERFACE_PROGRAM_OUTPUT:				DE_ASSERT((propFlags & PROGRAMRESOURCEPROP_PROGRAM_OUTPUT_MASK)				== propFlags);	break;
		case PROGRAMINTERFACE_BUFFER_VARIABLE:				DE_ASSERT((propFlags & PROGRAMRESOURCEPROP_BUFFER_VARIABLE_MASK)			== propFlags);	break;
		case PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING:	DE_ASSERT((propFlags & PROGRAMRESOURCEPROP_TRANSFORM_FEEDBACK_VARYING_MASK)	== propFlags);	break;

		default:
			DE_ASSERT(false);
	}
}

ProgramInterfaceQueryTestCase::ProgramInterfaceQueryTestCase (Context& context, const char* name, const char* description, ProgramResourceQueryTestTarget queryTarget)
	: TestCase		(context, name, description)
	, m_queryTarget	(queryTarget)
{
}

ProgramInterfaceQueryTestCase::~ProgramInterfaceQueryTestCase (void)
{
}

ProgramInterface ProgramInterfaceQueryTestCase::getTargetInterface (void) const
{
	return m_queryTarget.interface;
}

static glw::GLenum getGLInterfaceEnumValue (ProgramInterface interface)
{
	switch (interface)
	{
		case PROGRAMINTERFACE_UNIFORM:						return GL_UNIFORM;
		case PROGRAMINTERFACE_UNIFORM_BLOCK:				return GL_UNIFORM_BLOCK;
		case PROGRAMINTERFACE_ATOMIC_COUNTER_BUFFER:		return GL_ATOMIC_COUNTER_BUFFER;
		case PROGRAMINTERFACE_PROGRAM_INPUT:				return GL_PROGRAM_INPUT;
		case PROGRAMINTERFACE_PROGRAM_OUTPUT:				return GL_PROGRAM_OUTPUT;
		case PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING:	return GL_TRANSFORM_FEEDBACK_VARYING;
		case PROGRAMINTERFACE_BUFFER_VARIABLE:				return GL_BUFFER_VARIABLE;
		case PROGRAMINTERFACE_SHADER_STORAGE_BLOCK:			return GL_SHADER_STORAGE_BLOCK;
		default:
			DE_ASSERT(false);
			return 0;
	};
}

static bool isInterfaceBlockInterfaceName (const ProgramInterfaceDefinition::Program* program, ProgramInterface interface, const std::string& blockInterfaceName)
{
	deUint32 validStorageBits;
	deUint32 searchStageBits;

	DE_STATIC_ASSERT(glu::STORAGE_LAST < 32);
	DE_STATIC_ASSERT(glu::SHADERTYPE_LAST < 32);

	switch (interface)
	{
		case PROGRAMINTERFACE_UNIFORM_BLOCK:
		case PROGRAMINTERFACE_SHADER_STORAGE_BLOCK:
		case PROGRAMINTERFACE_ATOMIC_COUNTER_BUFFER:
			return false;

		case PROGRAMINTERFACE_PROGRAM_INPUT:
			validStorageBits = (1u << glu::STORAGE_IN) | (1u << glu::STORAGE_PATCH_IN);
			searchStageBits = (1u << program->getFirstStage());
			break;

		case PROGRAMINTERFACE_PROGRAM_OUTPUT:
			validStorageBits = (1u << glu::STORAGE_OUT) | (1u << glu::STORAGE_PATCH_OUT);
			searchStageBits = (1u << program->getLastStage());
			break;

		case PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING:
			validStorageBits = (1u << glu::STORAGE_OUT);
			searchStageBits = (1u << getProgramTransformFeedbackStage(program));
			break;

		case PROGRAMINTERFACE_UNIFORM:
			validStorageBits = (1u << glu::STORAGE_UNIFORM);
			searchStageBits = 0xFFFFFFFFu;
			break;

		case PROGRAMINTERFACE_BUFFER_VARIABLE:
			validStorageBits = (1u << glu::STORAGE_BUFFER);
			searchStageBits = 0xFFFFFFFFu;
			break;

		default:
			DE_ASSERT(false);
			return false;
	}

	for (int shaderNdx = 0; shaderNdx < (int)program->getShaders().size(); ++shaderNdx)
	{
		const ProgramInterfaceDefinition::Shader* const shader = program->getShaders()[shaderNdx];
		if (((1u << shader->getType()) & searchStageBits) == 0)
			continue;

		for (int blockNdx = 0; blockNdx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++blockNdx)
		{
			const glu::InterfaceBlock& block = shader->getDefaultBlock().interfaceBlocks[blockNdx];

			if (((1u << block.storage) & validStorageBits) == 0)
				continue;

			if (block.interfaceName == blockInterfaceName)
				return true;
		}
	}
	return false;
}

static std::string getInterfaceBlockInteraceNameByMember (const ProgramInterfaceDefinition::Program* program, ProgramInterface interface, const std::string& memberName)
{
	deUint32 validStorageBits;
	deUint32 searchStageBits;

	DE_STATIC_ASSERT(glu::STORAGE_LAST < 32);
	DE_STATIC_ASSERT(glu::SHADERTYPE_LAST < 32);

	switch (interface)
	{
		case PROGRAMINTERFACE_UNIFORM_BLOCK:
		case PROGRAMINTERFACE_SHADER_STORAGE_BLOCK:
		case PROGRAMINTERFACE_ATOMIC_COUNTER_BUFFER:
			return "";

		case PROGRAMINTERFACE_PROGRAM_INPUT:
			validStorageBits = (1u << glu::STORAGE_IN) | (1u << glu::STORAGE_PATCH_IN);
			searchStageBits = (1u << program->getFirstStage());
			break;

		case PROGRAMINTERFACE_PROGRAM_OUTPUT:
			validStorageBits = (1u << glu::STORAGE_OUT) | (1u << glu::STORAGE_PATCH_OUT);
			searchStageBits = (1u << program->getLastStage());
			break;

		case PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING:
			validStorageBits = (1u << glu::STORAGE_OUT);
			searchStageBits = (1u << getProgramTransformFeedbackStage(program));
			break;

		case PROGRAMINTERFACE_UNIFORM:
			validStorageBits = (1u << glu::STORAGE_UNIFORM);
			searchStageBits = 0xFFFFFFFFu;
			break;

		case PROGRAMINTERFACE_BUFFER_VARIABLE:
			validStorageBits = (1u << glu::STORAGE_BUFFER);
			searchStageBits = 0xFFFFFFFFu;
			break;

		default:
			DE_ASSERT(false);
			return "";
	}

	for (int shaderNdx = 0; shaderNdx < (int)program->getShaders().size(); ++shaderNdx)
	{
		const ProgramInterfaceDefinition::Shader* const shader = program->getShaders()[shaderNdx];
		if (((1u << shader->getType()) & searchStageBits) == 0)
			continue;

		for (int blockNdx = 0; blockNdx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++blockNdx)
		{
			const glu::InterfaceBlock& block = shader->getDefaultBlock().interfaceBlocks[blockNdx];

			if (((1u << block.storage) & validStorageBits) == 0)
				continue;

			for (int varNdx = 0; varNdx < (int)block.variables.size(); ++varNdx)
			{
				if (block.variables[varNdx].name == memberName)
					return block.interfaceName;
			}
		}
	}
	return "";
}

static void queryAndValidateProps (tcu::TestContext&							testCtx,
								   const glw::Functions&						gl,
								   glw::GLuint									programID,
								   ProgramInterface								interface,
								   const char*									targetResourceName,
								   const ProgramInterfaceDefinition::Program*	programDefinition,
								   const std::vector<glw::GLenum>&				props,
								   const std::vector<const PropValidator*>&		validators)
{
	const glw::GLenum			glInterface					= getGLInterfaceEnumValue(interface);
	std::string					implementationResourceName	= targetResourceName;
	glw::GLuint					resourceNdx;
	glw::GLint					written						= -1;

	// prefill result buffer with an invalid value. -1 might be valid sometimes, avoid it. Make buffer one larger
	// to allow detection of too many return values
	std::vector<glw::GLint>		propValues		(props.size() + 1, -2);

	DE_ASSERT(props.size() == validators.size());

	// query

	resourceNdx = gl.getProgramResourceIndex(programID, glInterface, targetResourceName);
	GLU_EXPECT_NO_ERROR(gl.getError(), "get resource index");

	if (resourceNdx == GL_INVALID_INDEX)
	{
		static const struct
		{
			bool removeTrailingArray;	// convert from "target[0]" -> "target"
			bool removeTrailingMember;	// convert from "target.member" -> "target"
			bool removeIOBlock;			// convert from "InterfaceName.target" -> "target"
			bool addIOBlock;			// convert from "target" -> "InterfaceName.target"
			bool addIOBlockArray;		// convert from "target" -> "InterfaceName[0].target"
		} recoveryStrategies[] =
		{
			// try one patch
			{ true,		false,	false,	false,	false	},
			{ false,	true,	false,	false,	false	},
			{ false,	false,	true,	false,	false	},
			{ false,	false,	false,	true,	false	},
			{ false,	false,	false,	false,	true	},
			// patch both ends
			{ true,		false,	true,	false,	false	},
			{ true,		false,	false,	true,	false	},
			{ true,		false,	false,	false,	true	},
			{ false,	true,	true,	false,	false	},
			{ false,	true,	false,	true,	false	},
			{ false,	true,	false,	false,	true	},
		};

		// The resource name generation in the GL implementations is very commonly broken. Try to
		// keep the tests producing useful data even in these cases by attempting to recover from
		// common naming bugs. Set test result to failure even if recovery succeeded to signal
		// incorrect name generation.

		testCtx.getLog() << tcu::TestLog::Message << "getProgramResourceIndex returned GL_INVALID_INDEX for \"" << targetResourceName << "\"" << tcu::TestLog::EndMessage;
		testCtx.setTestResult(QP_TEST_RESULT_FAIL, "could not find target resource");

		for (int strategyNdx = 0; strategyNdx < DE_LENGTH_OF_ARRAY(recoveryStrategies); ++strategyNdx)
		{
			const std::string	resourceName			= std::string(targetResourceName);
			const size_t		rootNameEnd				= resourceName.find_first_of(".[");
			const std::string	rootName				= resourceName.substr(0, rootNameEnd);
			std::string			simplifiedResourceName;

			if (recoveryStrategies[strategyNdx].removeTrailingArray)
			{
				if (de::endsWith(resourceName, "[0]"))
					simplifiedResourceName = resourceName.substr(0, resourceName.length() - 3);
				else
					continue;
			}

			if (recoveryStrategies[strategyNdx].removeTrailingMember)
			{
				const size_t lastMember = resourceName.find_last_of('.');
				if (lastMember != std::string::npos)
					simplifiedResourceName = resourceName.substr(0, lastMember);
				else
					continue;
			}

			if (recoveryStrategies[strategyNdx].removeIOBlock)
			{
				if (deStringBeginsWith(resourceName.c_str(), "gl_PerVertex."))
				{
					// builtin interface bock, remove block name
					simplifiedResourceName = resourceName.substr(13);
				}
				else if (isInterfaceBlockInterfaceName(programDefinition, interface, rootName))
				{
					// user-defined inteface block, remove name
					const size_t accessorEnd = resourceName.find('.'); // includes potential array accessor

					if (accessorEnd != std::string::npos)
						simplifiedResourceName = resourceName.substr(0, accessorEnd+1);
					else
						continue;
				}
				else
				{
					// recovery not applicable
					continue;
				}
			}

			if (recoveryStrategies[strategyNdx].addIOBlock || recoveryStrategies[strategyNdx].addIOBlockArray)
			{
				const std::string arrayAccessor = (recoveryStrategies[strategyNdx].addIOBlockArray) ? ("[0]") : ("");

				if (deStringBeginsWith(resourceName.c_str(), "gl_") && resourceName.find('.') == std::string::npos)
				{
					// free builtin variable, add block name
					simplifiedResourceName = "gl_PerVertex" + arrayAccessor + "." + resourceName;
				}
				else
				{
					const std::string interafaceName = getInterfaceBlockInteraceNameByMember(programDefinition, interface, rootName);

					if (!interafaceName.empty())
					{
						// free user variable, add block name
						simplifiedResourceName = interafaceName + arrayAccessor + "." + resourceName;
					}
					else
					{
						// recovery not applicable
						continue;
					}
				}
			}

			if (simplifiedResourceName.empty())
				continue;

			resourceNdx = gl.getProgramResourceIndex(programID, glInterface, simplifiedResourceName.c_str());
			GLU_EXPECT_NO_ERROR(gl.getError(), "get resource index");

			// recovery succeeded
			if (resourceNdx != GL_INVALID_INDEX)
			{
				implementationResourceName = simplifiedResourceName;
				testCtx.getLog() << tcu::TestLog::Message << "\tResource not found, continuing anyway using index obtained for resource \"" << simplifiedResourceName << "\"" << tcu::TestLog::EndMessage;
				break;
			}
		}

		if (resourceNdx == GL_INVALID_INDEX)
			return;
	}

	gl.getProgramResourceiv(programID, glInterface, resourceNdx, (int)props.size(), &props[0], (int)propValues.size(), &written, &propValues[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "get props");

	if (written != (int)props.size())
	{
		testCtx.getLog() << tcu::TestLog::Message << "getProgramResourceiv returned unexpected number of values, expected " << (int)props.size() << ", got " << written << tcu::TestLog::EndMessage;
		testCtx.setTestResult(QP_TEST_RESULT_FAIL, "getProgramResourceiv returned unexpected number of values");
		return;
	}

	if (propValues.back() != -2)
	{
		testCtx.getLog() << tcu::TestLog::Message << "getProgramResourceiv post write buffer guard value was modified, too many return values" << tcu::TestLog::EndMessage;
		testCtx.setTestResult(QP_TEST_RESULT_FAIL, "getProgramResourceiv returned unexpected number of values");
		return;
	}
	propValues.pop_back();
	DE_ASSERT(validators.size() == propValues.size());

	// log

	{
		tcu::MessageBuilder message(&testCtx.getLog());
		message << "For resource index " << resourceNdx << " (\"" << targetResourceName << "\") got following properties:\n";

		for (int propNdx = 0; propNdx < (int)propValues.size(); ++propNdx)
			message << "\t" << glu::getProgramResourcePropertyName(props[propNdx]) << ":\t" << validators[propNdx]->getHumanReadablePropertyString(propValues[propNdx]) << "\n";

		message << tcu::TestLog::EndMessage;
	}

	// validate

	for (int propNdx = 0; propNdx < (int)propValues.size(); ++propNdx)
		validators[propNdx]->validate(programDefinition, targetResourceName, propValues[propNdx], implementationResourceName);
}

const ProgramInterfaceDefinition::Program* ProgramInterfaceQueryTestCase::getAndCheckProgramDefinition (void)
{
	const ProgramInterfaceDefinition::Program* programDefinition = getProgramDefinition();
	DE_ASSERT(programDefinition->isValid());

	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (programDefinition->hasStage(glu::SHADERTYPE_TESSELLATION_CONTROL) ||
		programDefinition->hasStage(glu::SHADERTYPE_TESSELLATION_EVALUATION))
	{
		if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
			throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader extension");
	}

	// Testing IS_PER_PATCH as a part of a larger set is ok, since the extension is checked
	// before query. However, we don't want IS_PER_PATCH-specific tests to become noop and pass.
	if (m_queryTarget.propFlags == PROGRAMRESOURCEPROP_IS_PER_PATCH)
	{
		if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
			throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader extension");
	}

	if (programDefinition->hasStage(glu::SHADERTYPE_GEOMETRY))
	{
		if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
			throw tcu::NotSupportedError("Test requires GL_EXT_geometry_shader extension");
	}

	if (programContainsIOBlocks(programDefinition))
	{
		if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_shader_io_blocks"))
			throw tcu::NotSupportedError("Test requires GL_EXT_shader_io_blocks extension");
	}

	return programDefinition;
}

int ProgramInterfaceQueryTestCase::getMaxPatchVertices (void)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	glw::GLint				maxPatchVertices	= 0;

	gl.getIntegerv(GL_MAX_PATCH_VERTICES, &maxPatchVertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getIntegerv(GL_MAX_PATCH_VERTICES)");
	return maxPatchVertices;
}

ProgramInterfaceQueryTestCase::IterateResult ProgramInterfaceQueryTestCase::iterate (void)
{
	struct TestProperty
	{
		glw::GLenum				prop;
		const PropValidator*	validator;
	};

	const ProgramInterfaceDefinition::Program*	programDefinition	= getAndCheckProgramDefinition();
	const std::vector<std::string>				targetResources		= getQueryTargetResources();
	glu::ShaderProgram							program				(m_context.getRenderContext(), generateProgramInterfaceProgramSources(programDefinition));

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// Log program
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Program", "Program");

		// Feedback varyings
		if (!programDefinition->getTransformFeedbackVaryings().empty())
		{
			tcu::MessageBuilder builder(&m_testCtx.getLog());
			builder << "Transform feedback varyings: {";
			for (int ndx = 0; ndx < (int)programDefinition->getTransformFeedbackVaryings().size(); ++ndx)
			{
				if (ndx)
					builder << ", ";
				builder << "\"" << programDefinition->getTransformFeedbackVaryings()[ndx] << "\"";
			}
			builder << "}" << tcu::TestLog::EndMessage;
		}

		m_testCtx.getLog() << program;
		if (!program.isOk())
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Program build failed, checking if program exceeded implementation limits" << tcu::TestLog::EndMessage;
			checkProgramResourceUsage(programDefinition, m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

			// within limits
			throw tcu::TestError("could not build program");
		}
	}

	// Check interface props

	switch (m_queryTarget.interface)
	{
		case PROGRAMINTERFACE_UNIFORM:
		{
			const VariableSearchFilter					uniformFilter						= VariableSearchFilter::createStorageFilter(glu::STORAGE_UNIFORM);

			const TypeValidator							typeValidator						(m_context, program.getProgram(),						uniformFilter);
			const ArraySizeValidator					arraySizeValidator					(m_context, program.getProgram(),						-1,					uniformFilter);
			const ArrayStrideValidator					arrayStrideValidator				(m_context, program.getProgram(),						uniformFilter);
			const BlockIndexValidator					blockIndexValidator					(m_context, program.getProgram(),						uniformFilter);
			const IsRowMajorValidator					isRowMajorValidator					(m_context, program.getProgram(),						uniformFilter);
			const MatrixStrideValidator					matrixStrideValidator				(m_context, program.getProgram(),						uniformFilter);
			const AtomicCounterBufferIndexVerifier		atomicCounterBufferIndexVerifier	(m_context, program.getProgram(),						uniformFilter);
			const LocationValidator						locationValidator					(m_context, program.getProgram(),						uniformFilter);
			const VariableNameLengthValidator			nameLengthValidator					(m_context, program.getProgram(),						uniformFilter);
			const OffsetValidator						offsetVerifier						(m_context, program.getProgram(),						uniformFilter);
			const VariableReferencedByShaderValidator	referencedByVertexVerifier			(m_context, glu::SHADERTYPE_VERTEX,						uniformFilter);
			const VariableReferencedByShaderValidator	referencedByFragmentVerifier		(m_context, glu::SHADERTYPE_FRAGMENT,					uniformFilter);
			const VariableReferencedByShaderValidator	referencedByComputeVerifier			(m_context, glu::SHADERTYPE_COMPUTE,					uniformFilter);
			const VariableReferencedByShaderValidator	referencedByGeometryVerifier		(m_context, glu::SHADERTYPE_GEOMETRY,					uniformFilter);
			const VariableReferencedByShaderValidator	referencedByTessControlVerifier		(m_context, glu::SHADERTYPE_TESSELLATION_CONTROL,		uniformFilter);
			const VariableReferencedByShaderValidator	referencedByTessEvaluationVerifier	(m_context, glu::SHADERTYPE_TESSELLATION_EVALUATION,	uniformFilter);

			const TestProperty allProperties[] =
			{
				{ GL_ARRAY_SIZE,							&arraySizeValidator					},
				{ GL_ARRAY_STRIDE,							&arrayStrideValidator				},
				{ GL_ATOMIC_COUNTER_BUFFER_INDEX,			&atomicCounterBufferIndexVerifier	},
				{ GL_BLOCK_INDEX,							&blockIndexValidator				},
				{ GL_IS_ROW_MAJOR,							&isRowMajorValidator				},
				{ GL_LOCATION,								&locationValidator					},
				{ GL_MATRIX_STRIDE,							&matrixStrideValidator				},
				{ GL_NAME_LENGTH,							&nameLengthValidator				},
				{ GL_OFFSET,								&offsetVerifier						},
				{ GL_REFERENCED_BY_VERTEX_SHADER,			&referencedByVertexVerifier			},
				{ GL_REFERENCED_BY_FRAGMENT_SHADER,			&referencedByFragmentVerifier		},
				{ GL_REFERENCED_BY_COMPUTE_SHADER,			&referencedByComputeVerifier		},
				{ GL_REFERENCED_BY_GEOMETRY_SHADER,			&referencedByGeometryVerifier		},
				{ GL_REFERENCED_BY_TESS_CONTROL_SHADER,		&referencedByTessControlVerifier	},
				{ GL_REFERENCED_BY_TESS_EVALUATION_SHADER,	&referencedByTessEvaluationVerifier	},
				{ GL_TYPE,									&typeValidator						},
			};

			for (int targetResourceNdx = 0; targetResourceNdx < (int)targetResources.size(); ++targetResourceNdx)
			{
				const tcu::ScopedLogSection			section			(m_testCtx.getLog(), "UniformResource", "Uniform resource \"" +  targetResources[targetResourceNdx] + "\"");
				const glw::Functions&				gl				= m_context.getRenderContext().getFunctions();
				std::vector<glw::GLenum>			props;
				std::vector<const PropValidator*>	validators;

				for (int propNdx = 0; propNdx < DE_LENGTH_OF_ARRAY(allProperties); ++propNdx)
				{
					if (allProperties[propNdx].validator->isSelected(m_queryTarget.propFlags) &&
						allProperties[propNdx].validator->isSupported())
					{
						props.push_back(allProperties[propNdx].prop);
						validators.push_back(allProperties[propNdx].validator);
					}
				}

				DE_ASSERT(!props.empty());

				queryAndValidateProps(m_testCtx, gl, program.getProgram(), m_queryTarget.interface, targetResources[targetResourceNdx].c_str(), programDefinition, props, validators);
			}

			break;
		}

		case PROGRAMINTERFACE_UNIFORM_BLOCK:
		case PROGRAMINTERFACE_SHADER_STORAGE_BLOCK:
		{
			const glu::Storage						storage								= (m_queryTarget.interface == PROGRAMINTERFACE_UNIFORM_BLOCK) ? (glu::STORAGE_UNIFORM) : (glu::STORAGE_BUFFER);
			const VariableSearchFilter				blockFilter							= VariableSearchFilter::createStorageFilter(storage);

			const BlockNameLengthValidator			nameLengthValidator					(m_context, program.getProgram(),						blockFilter);
			const BlockReferencedByShaderValidator	referencedByVertexVerifier			(m_context, glu::SHADERTYPE_VERTEX,						blockFilter);
			const BlockReferencedByShaderValidator	referencedByFragmentVerifier		(m_context, glu::SHADERTYPE_FRAGMENT,					blockFilter);
			const BlockReferencedByShaderValidator	referencedByComputeVerifier			(m_context, glu::SHADERTYPE_COMPUTE,					blockFilter);
			const BlockReferencedByShaderValidator	referencedByGeometryVerifier		(m_context, glu::SHADERTYPE_GEOMETRY,					blockFilter);
			const BlockReferencedByShaderValidator	referencedByTessControlVerifier		(m_context, glu::SHADERTYPE_TESSELLATION_CONTROL,		blockFilter);
			const BlockReferencedByShaderValidator	referencedByTessEvaluationVerifier	(m_context, glu::SHADERTYPE_TESSELLATION_EVALUATION,	blockFilter);
			const BufferBindingValidator			bufferBindingValidator				(m_context, program.getProgram(),						blockFilter);

			const TestProperty allProperties[] =
			{
				{ GL_NAME_LENGTH,							&nameLengthValidator				},
				{ GL_REFERENCED_BY_VERTEX_SHADER,			&referencedByVertexVerifier			},
				{ GL_REFERENCED_BY_FRAGMENT_SHADER,			&referencedByFragmentVerifier		},
				{ GL_REFERENCED_BY_COMPUTE_SHADER,			&referencedByComputeVerifier		},
				{ GL_REFERENCED_BY_GEOMETRY_SHADER,			&referencedByGeometryVerifier		},
				{ GL_REFERENCED_BY_TESS_CONTROL_SHADER,		&referencedByTessControlVerifier	},
				{ GL_REFERENCED_BY_TESS_EVALUATION_SHADER,	&referencedByTessEvaluationVerifier	},
				{ GL_BUFFER_BINDING,						&bufferBindingValidator				},
			};

			for (int targetResourceNdx = 0; targetResourceNdx < (int)targetResources.size(); ++targetResourceNdx)
			{
				const tcu::ScopedLogSection			section			(m_testCtx.getLog(), "BlockResource", "Interface block \"" +  targetResources[targetResourceNdx] + "\"");
				const glw::Functions&				gl				= m_context.getRenderContext().getFunctions();
				std::vector<glw::GLenum>			props;
				std::vector<const PropValidator*>	validators;

				for (int propNdx = 0; propNdx < DE_LENGTH_OF_ARRAY(allProperties); ++propNdx)
				{
					if (allProperties[propNdx].validator->isSelected(m_queryTarget.propFlags) &&
						allProperties[propNdx].validator->isSupported())
					{
						props.push_back(allProperties[propNdx].prop);
						validators.push_back(allProperties[propNdx].validator);
					}
				}

				DE_ASSERT(!props.empty());

				queryAndValidateProps(m_testCtx, gl, program.getProgram(), m_queryTarget.interface, targetResources[targetResourceNdx].c_str(), programDefinition, props, validators);
			}

			break;
		}

		case PROGRAMINTERFACE_PROGRAM_INPUT:
		case PROGRAMINTERFACE_PROGRAM_OUTPUT:
		{
			const bool									isInputCase							= (m_queryTarget.interface == PROGRAMINTERFACE_PROGRAM_INPUT);
			const glu::Storage							varyingStorage						= (isInputCase) ? (glu::STORAGE_IN) : (glu::STORAGE_OUT);
			const glu::Storage							patchStorage						= (isInputCase) ? (glu::STORAGE_PATCH_IN) : (glu::STORAGE_PATCH_OUT);
			const glu::ShaderType						shaderType							= (isInputCase) ? (programDefinition->getFirstStage()) : (programDefinition->getLastStage());
			const int									unsizedArraySize					= (isInputCase && shaderType == glu::SHADERTYPE_GEOMETRY)					? (1)															// input points
																							: (isInputCase && shaderType == glu::SHADERTYPE_TESSELLATION_CONTROL)		? (getMaxPatchVertices())										// input batch size
																							: (!isInputCase && shaderType == glu::SHADERTYPE_TESSELLATION_CONTROL)		? (programDefinition->getTessellationNumOutputPatchVertices())	// output batch size
																							: (isInputCase && shaderType == glu::SHADERTYPE_TESSELLATION_EVALUATION)	? (getMaxPatchVertices())										// input batch size
																							: (-1);
			const VariableSearchFilter					variableFilter						= VariableSearchFilter::logicalAnd(VariableSearchFilter::createShaderTypeFilter(shaderType),
																															   VariableSearchFilter::logicalOr(VariableSearchFilter::createStorageFilter(varyingStorage),
																																							   VariableSearchFilter::createStorageFilter(patchStorage)));

			const TypeValidator							typeValidator						(m_context, program.getProgram(),						variableFilter);
			const ArraySizeValidator					arraySizeValidator					(m_context, program.getProgram(),						unsizedArraySize,		variableFilter);
			const LocationValidator						locationValidator					(m_context, program.getProgram(),						variableFilter);
			const VariableNameLengthValidator			nameLengthValidator					(m_context, program.getProgram(),						variableFilter);
			const VariableReferencedByShaderValidator	referencedByVertexVerifier			(m_context, glu::SHADERTYPE_VERTEX,						variableFilter);
			const VariableReferencedByShaderValidator	referencedByFragmentVerifier		(m_context, glu::SHADERTYPE_FRAGMENT,					variableFilter);
			const VariableReferencedByShaderValidator	referencedByComputeVerifier			(m_context, glu::SHADERTYPE_COMPUTE,					variableFilter);
			const VariableReferencedByShaderValidator	referencedByGeometryVerifier		(m_context, glu::SHADERTYPE_GEOMETRY,					variableFilter);
			const VariableReferencedByShaderValidator	referencedByTessControlVerifier		(m_context, glu::SHADERTYPE_TESSELLATION_CONTROL,		variableFilter);
			const VariableReferencedByShaderValidator	referencedByTessEvaluationVerifier	(m_context, glu::SHADERTYPE_TESSELLATION_EVALUATION,	variableFilter);
			const PerPatchValidator						perPatchValidator					(m_context, program.getProgram(),						variableFilter);

			const TestProperty allProperties[] =
			{
				{ GL_ARRAY_SIZE,							&arraySizeValidator					},
				{ GL_LOCATION,								&locationValidator					},
				{ GL_NAME_LENGTH,							&nameLengthValidator				},
				{ GL_REFERENCED_BY_VERTEX_SHADER,			&referencedByVertexVerifier			},
				{ GL_REFERENCED_BY_FRAGMENT_SHADER,			&referencedByFragmentVerifier		},
				{ GL_REFERENCED_BY_COMPUTE_SHADER,			&referencedByComputeVerifier		},
				{ GL_REFERENCED_BY_GEOMETRY_SHADER,			&referencedByGeometryVerifier		},
				{ GL_REFERENCED_BY_TESS_CONTROL_SHADER,		&referencedByTessControlVerifier	},
				{ GL_REFERENCED_BY_TESS_EVALUATION_SHADER,	&referencedByTessEvaluationVerifier	},
				{ GL_TYPE,									&typeValidator						},
				{ GL_IS_PER_PATCH,							&perPatchValidator					},
			};

			for (int targetResourceNdx = 0; targetResourceNdx < (int)targetResources.size(); ++targetResourceNdx)
			{
				const std::string					resourceInterfaceName	= (m_queryTarget.interface == PROGRAMINTERFACE_PROGRAM_INPUT) ? ("Input") : ("Output");
				const tcu::ScopedLogSection			section					(m_testCtx.getLog(), "BlockResource", resourceInterfaceName + " resource \"" +  targetResources[targetResourceNdx] + "\"");
				const glw::Functions&				gl						= m_context.getRenderContext().getFunctions();
				std::vector<glw::GLenum>			props;
				std::vector<const PropValidator*>	validators;

				for (int propNdx = 0; propNdx < DE_LENGTH_OF_ARRAY(allProperties); ++propNdx)
				{
					if (allProperties[propNdx].validator->isSelected(m_queryTarget.propFlags) &&
						allProperties[propNdx].validator->isSupported())
					{
						props.push_back(allProperties[propNdx].prop);
						validators.push_back(allProperties[propNdx].validator);
					}
				}

				DE_ASSERT(!props.empty());

				queryAndValidateProps(m_testCtx, gl, program.getProgram(), m_queryTarget.interface, targetResources[targetResourceNdx].c_str(), programDefinition, props, validators);
			}

			break;
		}

		case PROGRAMINTERFACE_BUFFER_VARIABLE:
		{
			const VariableSearchFilter					variableFilter						= VariableSearchFilter::createStorageFilter(glu::STORAGE_BUFFER);

			const TypeValidator							typeValidator						(m_context, program.getProgram(),						variableFilter);
			const ArraySizeValidator					arraySizeValidator					(m_context, program.getProgram(),						0,					variableFilter);
			const ArrayStrideValidator					arrayStrideValidator				(m_context, program.getProgram(),						variableFilter);
			const BlockIndexValidator					blockIndexValidator					(m_context, program.getProgram(),						variableFilter);
			const IsRowMajorValidator					isRowMajorValidator					(m_context, program.getProgram(),						variableFilter);
			const MatrixStrideValidator					matrixStrideValidator				(m_context, program.getProgram(),						variableFilter);
			const OffsetValidator						offsetValidator						(m_context, program.getProgram(),						variableFilter);
			const VariableNameLengthValidator			nameLengthValidator					(m_context, program.getProgram(),						variableFilter);
			const VariableReferencedByShaderValidator	referencedByVertexVerifier			(m_context, glu::SHADERTYPE_VERTEX,						variableFilter);
			const VariableReferencedByShaderValidator	referencedByFragmentVerifier		(m_context, glu::SHADERTYPE_FRAGMENT,					variableFilter);
			const VariableReferencedByShaderValidator	referencedByComputeVerifier			(m_context, glu::SHADERTYPE_COMPUTE,					variableFilter);
			const VariableReferencedByShaderValidator	referencedByGeometryVerifier		(m_context, glu::SHADERTYPE_GEOMETRY,					variableFilter);
			const VariableReferencedByShaderValidator	referencedByTessControlVerifier		(m_context, glu::SHADERTYPE_TESSELLATION_CONTROL,		variableFilter);
			const VariableReferencedByShaderValidator	referencedByTessEvaluationVerifier	(m_context, glu::SHADERTYPE_TESSELLATION_EVALUATION,	variableFilter);
			const TopLevelArraySizeValidator			topLevelArraySizeValidator			(m_context, program.getProgram(),						variableFilter);
			const TopLevelArrayStrideValidator			topLevelArrayStrideValidator		(m_context, program.getProgram(),						variableFilter);

			const TestProperty allProperties[] =
			{
				{ GL_ARRAY_SIZE,							&arraySizeValidator					},
				{ GL_ARRAY_STRIDE,							&arrayStrideValidator				},
				{ GL_BLOCK_INDEX,							&blockIndexValidator				},
				{ GL_IS_ROW_MAJOR,							&isRowMajorValidator				},
				{ GL_MATRIX_STRIDE,							&matrixStrideValidator				},
				{ GL_NAME_LENGTH,							&nameLengthValidator				},
				{ GL_OFFSET,								&offsetValidator					},
				{ GL_REFERENCED_BY_VERTEX_SHADER,			&referencedByVertexVerifier			},
				{ GL_REFERENCED_BY_FRAGMENT_SHADER,			&referencedByFragmentVerifier		},
				{ GL_REFERENCED_BY_COMPUTE_SHADER,			&referencedByComputeVerifier		},
				{ GL_REFERENCED_BY_GEOMETRY_SHADER,			&referencedByGeometryVerifier		},
				{ GL_REFERENCED_BY_TESS_CONTROL_SHADER,		&referencedByTessControlVerifier	},
				{ GL_REFERENCED_BY_TESS_EVALUATION_SHADER,	&referencedByTessEvaluationVerifier	},
				{ GL_TOP_LEVEL_ARRAY_SIZE,					&topLevelArraySizeValidator			},
				{ GL_TOP_LEVEL_ARRAY_STRIDE,				&topLevelArrayStrideValidator		},
				{ GL_TYPE,									&typeValidator						},
			};

			for (int targetResourceNdx = 0; targetResourceNdx < (int)targetResources.size(); ++targetResourceNdx)
			{
				const tcu::ScopedLogSection			section			(m_testCtx.getLog(), "BufferVariableResource", "Buffer variable \"" +  targetResources[targetResourceNdx] + "\"");
				const glw::Functions&				gl				= m_context.getRenderContext().getFunctions();
				std::vector<glw::GLenum>			props;
				std::vector<const PropValidator*>	validators;

				for (int propNdx = 0; propNdx < DE_LENGTH_OF_ARRAY(allProperties); ++propNdx)
				{
					if (allProperties[propNdx].validator->isSelected(m_queryTarget.propFlags) &&
						allProperties[propNdx].validator->isSupported())
					{
						props.push_back(allProperties[propNdx].prop);
						validators.push_back(allProperties[propNdx].validator);
					}
				}

				DE_ASSERT(!props.empty());

				queryAndValidateProps(m_testCtx, gl, program.getProgram(), m_queryTarget.interface, targetResources[targetResourceNdx].c_str(), programDefinition, props, validators);
			}

			break;
		}

		case PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING:
		{
			const TransformFeedbackTypeValidator		typeValidator			(m_context);
			const TransformFeedbackArraySizeValidator	arraySizeValidator		(m_context);
			const TransformFeedbackNameLengthValidator	nameLengthValidator		(m_context);

			const TestProperty allProperties[] =
			{
				{ GL_ARRAY_SIZE,					&arraySizeValidator				},
				{ GL_NAME_LENGTH,					&nameLengthValidator			},
				{ GL_TYPE,							&typeValidator					},
			};

			for (int targetResourceNdx = 0; targetResourceNdx < (int)targetResources.size(); ++targetResourceNdx)
			{
				const tcu::ScopedLogSection			section			(m_testCtx.getLog(), "XFBVariableResource", "Transform feedback varying \"" +  targetResources[targetResourceNdx] + "\"");
				const glw::Functions&				gl				= m_context.getRenderContext().getFunctions();
				std::vector<glw::GLenum>			props;
				std::vector<const PropValidator*>	validators;

				for (int propNdx = 0; propNdx < DE_LENGTH_OF_ARRAY(allProperties); ++propNdx)
				{
					if (allProperties[propNdx].validator->isSelected(m_queryTarget.propFlags) &&
						allProperties[propNdx].validator->isSupported())
					{
						props.push_back(allProperties[propNdx].prop);
						validators.push_back(allProperties[propNdx].validator);
					}
				}

				DE_ASSERT(!props.empty());

				queryAndValidateProps(m_testCtx, gl, program.getProgram(), m_queryTarget.interface, targetResources[targetResourceNdx].c_str(), programDefinition, props, validators);
			}

			break;
		}

		default:
			DE_ASSERT(false);
	}

	return STOP;
}

static bool checkLimit (glw::GLenum pname, int usage, const glw::Functions& gl, tcu::TestLog& log)
{
	if (usage > 0)
	{
		glw::GLint limit = 0;
		gl.getIntegerv(pname, &limit);
		GLU_EXPECT_NO_ERROR(gl.getError(), "query limits");

		log << tcu::TestLog::Message << "\t" << glu::getGettableStateStr(pname) << " = " << limit << ", test requires " << usage << tcu::TestLog::EndMessage;

		if (limit < usage)
		{
			log << tcu::TestLog::Message << "\t\tLimit exceeded" << tcu::TestLog::EndMessage;
			return false;
		}
	}

	return true;
}

static bool checkShaderResourceUsage (const ProgramInterfaceDefinition::Program* program, const ProgramInterfaceDefinition::Shader* shader, const glw::Functions& gl, tcu::TestLog& log)
{
	const ProgramInterfaceDefinition::ShaderResourceUsage usage = getShaderResourceUsage(program, shader);

	switch (shader->getType())
	{
		case glu::SHADERTYPE_VERTEX:
		{
			const struct
			{
				glw::GLenum	pname;
				int			usage;
			} restrictions[] =
			{
				{ GL_MAX_VERTEX_ATTRIBS,						usage.numInputVectors					},
				{ GL_MAX_VERTEX_UNIFORM_COMPONENTS,				usage.numDefaultBlockUniformComponents	},
				{ GL_MAX_VERTEX_UNIFORM_VECTORS,				usage.numUniformVectors					},
				{ GL_MAX_VERTEX_UNIFORM_BLOCKS,					usage.numUniformBlocks					},
				{ GL_MAX_VERTEX_OUTPUT_COMPONENTS,				usage.numOutputComponents				},
				{ GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,			usage.numSamplers						},
				{ GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS,			usage.numAtomicCounterBuffers			},
				{ GL_MAX_VERTEX_ATOMIC_COUNTERS,				usage.numAtomicCounters					},
				{ GL_MAX_VERTEX_IMAGE_UNIFORMS,					usage.numImages							},
				{ GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS,	usage.numCombinedUniformComponents		},
				{ GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,			usage.numShaderStorageBlocks			},
			};

			bool allOk = true;

			log << tcu::TestLog::Message << "Vertex shader:" << tcu::TestLog::EndMessage;
			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(restrictions); ++ndx)
				allOk &= checkLimit(restrictions[ndx].pname, restrictions[ndx].usage, gl, log);

			return allOk;
		}

		case glu::SHADERTYPE_FRAGMENT:
		{
			const struct
			{
				glw::GLenum	pname;
				int			usage;
			} restrictions[] =
			{
				{ GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,			usage.numDefaultBlockUniformComponents		},
				{ GL_MAX_FRAGMENT_UNIFORM_VECTORS,				usage.numUniformVectors						},
				{ GL_MAX_FRAGMENT_UNIFORM_BLOCKS,				usage.numUniformBlocks						},
				{ GL_MAX_FRAGMENT_INPUT_COMPONENTS,				usage.numInputComponents					},
				{ GL_MAX_TEXTURE_IMAGE_UNITS,					usage.numSamplers							},
				{ GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS,		usage.numAtomicCounterBuffers				},
				{ GL_MAX_FRAGMENT_ATOMIC_COUNTERS,				usage.numAtomicCounters						},
				{ GL_MAX_FRAGMENT_IMAGE_UNIFORMS,				usage.numImages								},
				{ GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS,	usage.numCombinedUniformComponents			},
				{ GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,		usage.numShaderStorageBlocks				},
			};

			bool allOk = true;

			log << tcu::TestLog::Message << "Fragment shader:" << tcu::TestLog::EndMessage;
			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(restrictions); ++ndx)
				allOk &= checkLimit(restrictions[ndx].pname, restrictions[ndx].usage, gl, log);

			return allOk;
		}

		case glu::SHADERTYPE_COMPUTE:
		{
			const struct
			{
				glw::GLenum	pname;
				int			usage;
			} restrictions[] =
			{
				{ GL_MAX_COMPUTE_UNIFORM_BLOCKS,				usage.numUniformBlocks					},
				{ GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS,			usage.numSamplers						},
				{ GL_MAX_COMPUTE_UNIFORM_COMPONENTS,			usage.numDefaultBlockUniformComponents	},
				{ GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS,		usage.numAtomicCounterBuffers			},
				{ GL_MAX_COMPUTE_ATOMIC_COUNTERS,				usage.numAtomicCounters					},
				{ GL_MAX_COMPUTE_IMAGE_UNIFORMS,				usage.numImages							},
				{ GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS,	usage.numCombinedUniformComponents		},
				{ GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,			usage.numShaderStorageBlocks			},
			};

			bool allOk = true;

			log << tcu::TestLog::Message << "Compute shader:" << tcu::TestLog::EndMessage;
			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(restrictions); ++ndx)
				allOk &= checkLimit(restrictions[ndx].pname, restrictions[ndx].usage, gl, log);

			return allOk;
		}

		case glu::SHADERTYPE_GEOMETRY:
		{
			const int totalOutputComponents = program->getGeometryNumOutputVertices() * usage.numOutputComponents;
			const struct
			{
				glw::GLenum	pname;
				int			usage;
			} restrictions[] =
			{
				{ GL_MAX_GEOMETRY_UNIFORM_COMPONENTS,				usage.numDefaultBlockUniformComponents			},
				{ GL_MAX_GEOMETRY_UNIFORM_BLOCKS,					usage.numUniformBlocks							},
				{ GL_MAX_GEOMETRY_INPUT_COMPONENTS,					usage.numInputComponents						},
				{ GL_MAX_GEOMETRY_OUTPUT_COMPONENTS,				usage.numOutputComponents						},
				{ GL_MAX_GEOMETRY_OUTPUT_VERTICES,					(int)program->getGeometryNumOutputVertices()	},
				{ GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS,			totalOutputComponents							},
				{ GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS,				usage.numSamplers								},
				{ GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS,			usage.numAtomicCounterBuffers					},
				{ GL_MAX_GEOMETRY_ATOMIC_COUNTERS,					usage.numAtomicCounters							},
				{ GL_MAX_GEOMETRY_IMAGE_UNIFORMS,					usage.numImages									},
				{ GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS,			usage.numShaderStorageBlocks					},
			};

			bool allOk = true;

			log << tcu::TestLog::Message << "Geometry shader:" << tcu::TestLog::EndMessage;
			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(restrictions); ++ndx)
				allOk &= checkLimit(restrictions[ndx].pname, restrictions[ndx].usage, gl, log);

			return allOk;
		}

		case glu::SHADERTYPE_TESSELLATION_CONTROL:
		{
			const int totalOutputComponents = program->getTessellationNumOutputPatchVertices() * usage.numOutputComponents + usage.numPatchOutputComponents;
			const struct
			{
				glw::GLenum	pname;
				int			usage;
			} restrictions[] =
			{
				{ GL_MAX_PATCH_VERTICES,								(int)program->getTessellationNumOutputPatchVertices()	},
				{ GL_MAX_TESS_PATCH_COMPONENTS,							usage.numPatchOutputComponents							},
				{ GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS,				usage.numDefaultBlockUniformComponents					},
				{ GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS,					usage.numUniformBlocks									},
				{ GL_MAX_TESS_CONTROL_INPUT_COMPONENTS,					usage.numInputComponents								},
				{ GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS,				usage.numOutputComponents								},
				{ GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS,			totalOutputComponents									},
				{ GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS,				usage.numSamplers										},
				{ GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS,			usage.numAtomicCounterBuffers							},
				{ GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS,					usage.numAtomicCounters									},
				{ GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS,					usage.numImages											},
				{ GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS,			usage.numShaderStorageBlocks							},
			};

			bool allOk = true;

			log << tcu::TestLog::Message << "Tessellation control shader:" << tcu::TestLog::EndMessage;
			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(restrictions); ++ndx)
				allOk &= checkLimit(restrictions[ndx].pname, restrictions[ndx].usage, gl, log);

			return allOk;
		}

		case glu::SHADERTYPE_TESSELLATION_EVALUATION:
		{
			const struct
			{
				glw::GLenum	pname;
				int			usage;
			} restrictions[] =
			{
				{ GL_MAX_PATCH_VERTICES,								(int)program->getTessellationNumOutputPatchVertices()	},
				{ GL_MAX_TESS_PATCH_COMPONENTS,							usage.numPatchInputComponents							},
				{ GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS,			usage.numDefaultBlockUniformComponents					},
				{ GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS,				usage.numUniformBlocks									},
				{ GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS,				usage.numInputComponents								},
				{ GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS,				usage.numOutputComponents								},
				{ GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS,			usage.numSamplers										},
				{ GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS,		usage.numAtomicCounterBuffers							},
				{ GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS,				usage.numAtomicCounters									},
				{ GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS,				usage.numImages											},
				{ GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS,			usage.numShaderStorageBlocks							},
			};

			bool allOk = true;

			log << tcu::TestLog::Message << "Tessellation evaluation shader:" << tcu::TestLog::EndMessage;
			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(restrictions); ++ndx)
				allOk &= checkLimit(restrictions[ndx].pname, restrictions[ndx].usage, gl, log);

			return allOk;
		}

		default:
			DE_ASSERT(false);
			return false;
	}
}

static bool checkProgramCombinedResourceUsage (const ProgramInterfaceDefinition::Program* program, const glw::Functions& gl, tcu::TestLog& log)
{
	const ProgramInterfaceDefinition::ProgramResourceUsage usage = getCombinedProgramResourceUsage(program);

	const struct
	{
		glw::GLenum	pname;
		int			usage;
	} restrictions[] =
	{
		{ GL_MAX_UNIFORM_BUFFER_BINDINGS,						usage.uniformBufferMaxBinding+1					},
		{ GL_MAX_UNIFORM_BLOCK_SIZE,							usage.uniformBufferMaxSize						},
		{ GL_MAX_COMBINED_UNIFORM_BLOCKS,						usage.numUniformBlocks							},
		{ GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS,			usage.numCombinedVertexUniformComponents		},
		{ GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS,			usage.numCombinedFragmentUniformComponents		},
		{ GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS,			usage.numCombinedGeometryUniformComponents		},
		{ GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS,		usage.numCombinedTessControlUniformComponents	},
		{ GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS,	usage.numCombinedTessEvalUniformComponents		},
		{ GL_MAX_VARYING_COMPONENTS,							usage.numVaryingComponents						},
		{ GL_MAX_VARYING_VECTORS,								usage.numVaryingVectors							},
		{ GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,					usage.numCombinedSamplers						},
		{ GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES,				usage.numCombinedOutputResources				},
		{ GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS,				usage.atomicCounterBufferMaxBinding+1			},
		{ GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE,					usage.atomicCounterBufferMaxSize				},
		{ GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS,				usage.numAtomicCounterBuffers					},
		{ GL_MAX_COMBINED_ATOMIC_COUNTERS,						usage.numAtomicCounters							},
		{ GL_MAX_IMAGE_UNITS,									usage.maxImageBinding+1							},
		{ GL_MAX_COMBINED_IMAGE_UNIFORMS,						usage.numCombinedImages							},
		{ GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,				usage.shaderStorageBufferMaxBinding+1			},
		{ GL_MAX_SHADER_STORAGE_BLOCK_SIZE,						usage.shaderStorageBufferMaxSize				},
		{ GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS,				usage.numShaderStorageBlocks					},
		{ GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS,		usage.numXFBInterleavedComponents				},
		{ GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS,			usage.numXFBSeparateAttribs						},
		{ GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS,		usage.numXFBSeparateComponents					},
		{ GL_MAX_DRAW_BUFFERS,									usage.fragmentOutputMaxBinding+1				},
	};

	bool allOk = true;

	log << tcu::TestLog::Message << "Program combined:" << tcu::TestLog::EndMessage;
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(restrictions); ++ndx)
		allOk &= checkLimit(restrictions[ndx].pname, restrictions[ndx].usage, gl, log);

	return allOk;
}

void checkProgramResourceUsage (const ProgramInterfaceDefinition::Program* program, const glw::Functions& gl, tcu::TestLog& log)
{
	bool limitExceeded = false;

	for (int shaderNdx = 0; shaderNdx < (int)program->getShaders().size(); ++shaderNdx)
		limitExceeded |= !checkShaderResourceUsage(program, program->getShaders()[shaderNdx], gl, log);

	limitExceeded |= !checkProgramCombinedResourceUsage(program, gl, log);

	if (limitExceeded)
	{
		log << tcu::TestLog::Message << "One or more resource limits exceeded" << tcu::TestLog::EndMessage;
		throw tcu::NotSupportedError("one or more resource limits exceeded");
	}
}

} // Functional
} // gles31
} // deqp
