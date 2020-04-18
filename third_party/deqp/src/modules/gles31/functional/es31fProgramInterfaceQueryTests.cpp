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
 * \brief Program interface query tests.
 *//*--------------------------------------------------------------------*/

#include "es31fProgramInterfaceQueryTests.hpp"
#include "es31fProgramInterfaceQueryTestCase.hpp"
#include "es31fProgramInterfaceDefinition.hpp"
#include "es31fProgramInterfaceDefinitionUtil.hpp"
#include "tcuTestLog.hpp"
#include "tcuStringTemplate.hpp"
#include "gluShaderProgram.hpp"
#include "gluVarTypeUtil.hpp"
#include "gluStrUtil.hpp"
#include "gluContextInfo.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deRandom.hpp"
#include "deString.h"
#include "deStringUtil.hpp"
#include "deSharedPtr.hpp"
#include "deUniquePtr.hpp"
#include "deSTLUtil.hpp"
#include "deArrayUtil.hpp"

#include <set>
#include <map>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

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
		return glu::getDataTypeScalarSize(type.getBasicType()) * getTypeSize(glu::getDataTypeScalarType(type.getBasicType()));
	else if (type.isStructType())
	{
		int size = 0;
		for (int ndx = 0; ndx < type.getStructPtr()->getNumMembers(); ++ndx)
			size += getVarTypeSize(type.getStructPtr()->getMember(ndx).getType());
		return size;
	}
	else if (type.isArrayType())
	{
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

static std::string convertGLTypeNameToTestName (const char* glName)
{
	// vectors and matrices are fine as is
	{
		if (deStringBeginsWith(glName, "vec")  == DE_TRUE ||
			deStringBeginsWith(glName, "ivec") == DE_TRUE ||
			deStringBeginsWith(glName, "uvec") == DE_TRUE ||
			deStringBeginsWith(glName, "bvec") == DE_TRUE ||
			deStringBeginsWith(glName, "mat")  == DE_TRUE)
			return std::string(glName);
	}

	// convert camel case to use underscore
	{
		std::ostringstream	buf;
		std::istringstream	name					(glName);
		bool				mergeNextToken			= false;
		bool				previousTokenWasDigit	= false;

		while (!name.eof())
		{
			std::ostringstream token;

			while (name.peek() != EOF)
			{
				if ((de::isDigit((char)name.peek()) || de::isUpper((char)name.peek())) && token.tellp())
					break;

				token << de::toLower((char)name.get());
			}

			if (buf.str().empty() || mergeNextToken)
				buf << token.str();
			else
				buf << '_' << token.str();

			// Single char causes next char to be merged (don't split initialisms or acronyms) unless it is 'D' after a number (split to ..._2d_acronym_aa
			mergeNextToken = false;
			if (token.tellp() == (std::streamoff)1)
			{
				if (!previousTokenWasDigit || token.str()[0] != 'd')
					mergeNextToken = true;

				previousTokenWasDigit = de::isDigit(token.str()[0]);
			}
			else
				previousTokenWasDigit = false;
		}

		return buf.str();
	}
}

static glw::GLenum getProgramInterfaceGLEnum (ProgramInterface interface)
{
	static const glw::GLenum s_enums[] =
	{
		GL_UNIFORM,						// PROGRAMINTERFACE_UNIFORM
		GL_UNIFORM_BLOCK,				// PROGRAMINTERFACE_UNIFORM_BLOCK
		GL_ATOMIC_COUNTER_BUFFER,		// PROGRAMINTERFACE_ATOMIC_COUNTER_BUFFER
		GL_PROGRAM_INPUT,				// PROGRAMINTERFACE_PROGRAM_INPUT
		GL_PROGRAM_OUTPUT,				// PROGRAMINTERFACE_PROGRAM_OUTPUT
		GL_TRANSFORM_FEEDBACK_VARYING,	// PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING
		GL_BUFFER_VARIABLE,				// PROGRAMINTERFACE_BUFFER_VARIABLE
		GL_SHADER_STORAGE_BLOCK,		// PROGRAMINTERFACE_SHADER_STORAGE_BLOCK
	};

	return de::getSizedArrayElement<PROGRAMINTERFACE_LAST>(s_enums, interface);
}

static glu::ShaderType getShaderMaskFirstStage (deUint32 mask)
{
	if (mask & (1u << glu::SHADERTYPE_COMPUTE))
		return glu::SHADERTYPE_COMPUTE;

	if (mask & (1u << glu::SHADERTYPE_VERTEX))
		return glu::SHADERTYPE_VERTEX;

	if (mask & (1u << glu::SHADERTYPE_TESSELLATION_CONTROL))
		return glu::SHADERTYPE_TESSELLATION_CONTROL;

	if (mask & (1u << glu::SHADERTYPE_TESSELLATION_EVALUATION))
		return glu::SHADERTYPE_TESSELLATION_EVALUATION;

	if (mask & (1u << glu::SHADERTYPE_GEOMETRY))
		return glu::SHADERTYPE_GEOMETRY;

	if (mask & (1u << glu::SHADERTYPE_FRAGMENT))
		return glu::SHADERTYPE_FRAGMENT;

	DE_ASSERT(false);
	return glu::SHADERTYPE_LAST;
}

static glu::ShaderType getShaderMaskLastStage (deUint32 mask)
{
	if (mask & (1u << glu::SHADERTYPE_FRAGMENT))
		return glu::SHADERTYPE_FRAGMENT;

	if (mask & (1u << glu::SHADERTYPE_GEOMETRY))
		return glu::SHADERTYPE_GEOMETRY;

	if (mask & (1u << glu::SHADERTYPE_TESSELLATION_EVALUATION))
		return glu::SHADERTYPE_TESSELLATION_EVALUATION;

	if (mask & (1u << glu::SHADERTYPE_TESSELLATION_CONTROL))
		return glu::SHADERTYPE_TESSELLATION_CONTROL;

	if (mask & (1u << glu::SHADERTYPE_VERTEX))
		return glu::SHADERTYPE_VERTEX;

	if (mask & (1u << glu::SHADERTYPE_COMPUTE))
		return glu::SHADERTYPE_COMPUTE;

	DE_ASSERT(false);
	return glu::SHADERTYPE_LAST;
}

static std::string specializeShader(Context& context, const char* code)
{
	const glu::GLSLVersion				glslVersion			= glu::getContextTypeGLSLVersion(context.getRenderContext().getType());
	std::map<std::string, std::string>	specializationMap;

	specializationMap["GLSL_VERSION_DECL"] = glu::getGLSLVersionDeclaration(glslVersion);

	return tcu::StringTemplate(code).specialize(specializationMap);
}

namespace ResourceDefinition
{

class Node
{
public:
	enum NodeType
	{
		TYPE_PROGRAM = 0,
		TYPE_SHADER,
		TYPE_DEFAULT_BLOCK,
		TYPE_VARIABLE,
		TYPE_INTERFACE_BLOCK,
		TYPE_ARRAY_ELEMENT,
		TYPE_STRUCT_MEMBER,
		TYPE_STORAGE_QUALIFIER,
		TYPE_LAYOUT_QUALIFIER,
		TYPE_SHADER_SET,
		TYPE_INTERPOLATION_QUALIFIER,
		TYPE_TRANSFORM_FEEDBACK_TARGET,

		TYPE_LAST
	};

	typedef de::SharedPtr<const Node> SharedPtr;

							Node				(NodeType type, const SharedPtr& enclosingNode) : m_type(type), m_enclosingNode(enclosingNode) { DE_ASSERT(type < TYPE_LAST); }
	virtual					~Node				(void) { }

	inline const Node*		getEnclosingNode	(void) const					{ return m_enclosingNode.get();	}
	inline NodeType			getType				(void) const					{ return m_type;				}

private:
	const NodeType			m_type;
	const SharedPtr			m_enclosingNode;
};

class Program : public Node
{
public:
	Program (bool separable = false)
		: Node			(TYPE_PROGRAM, SharedPtr())
		, m_separable	(separable)
	{
	}

	const bool m_separable;
};

class Shader : public Node
{
public:
	Shader (const SharedPtr& enclosingNode, glu::ShaderType type, glu::GLSLVersion version)
		: Node		(TYPE_SHADER, enclosingNode)
		, m_type	(type)
		, m_version	(version)
	{
		DE_ASSERT(enclosingNode->getType() == TYPE_PROGRAM);
		DE_ASSERT(type < glu::SHADERTYPE_LAST);
	}

	const glu::ShaderType	m_type;
	const glu::GLSLVersion	m_version;
};

class DefaultBlock : public Node
{
public:
	DefaultBlock (const SharedPtr& enclosing)
		: Node(TYPE_DEFAULT_BLOCK, enclosing)
	{
		// enclosed by the shader
		DE_ASSERT(enclosing->getType() == TYPE_SHADER		||
				  enclosing->getType() == TYPE_SHADER_SET);
	}
};

class StorageQualifier : public Node
{
public:
	StorageQualifier (const SharedPtr& enclosing, glu::Storage storage)
		: Node		(TYPE_STORAGE_QUALIFIER, enclosing)
		, m_storage	(storage)
	{
		// not a part of any block
		DE_ASSERT(enclosing->getType() == TYPE_DEFAULT_BLOCK);
	}

	const glu::Storage	m_storage;
};

class Variable : public Node
{
public:
	Variable (const SharedPtr& enclosing, glu::DataType dataType)
		: Node			(TYPE_VARIABLE, enclosing)
		, m_dataType	(dataType)
	{
		DE_ASSERT(enclosing->getType() == TYPE_STORAGE_QUALIFIER		||
				  enclosing->getType() == TYPE_LAYOUT_QUALIFIER			||
				  enclosing->getType() == TYPE_INTERPOLATION_QUALIFIER	||
				  enclosing->getType() == TYPE_INTERFACE_BLOCK			||
				  enclosing->getType() == TYPE_ARRAY_ELEMENT			||
				  enclosing->getType() == TYPE_STRUCT_MEMBER			||
				  enclosing->getType() == TYPE_TRANSFORM_FEEDBACK_TARGET);
	}

	const glu::DataType	m_dataType;
};

class InterfaceBlock : public Node
{
public:
	InterfaceBlock (const SharedPtr& enclosing, bool named)
		: Node		(TYPE_INTERFACE_BLOCK, enclosing)
		, m_named	(named)
	{
		// Must be storage qualified
		const Node* storageNode = enclosing.get();
		while (storageNode->getType() == TYPE_ARRAY_ELEMENT ||
			   storageNode->getType() == TYPE_LAYOUT_QUALIFIER)
		{
			storageNode = storageNode->getEnclosingNode();
		}

		DE_ASSERT(storageNode->getType() == TYPE_STORAGE_QUALIFIER);
		DE_UNREF(storageNode);
	}

	const bool	m_named;
};

class ArrayElement : public Node
{
public:
	ArrayElement (const SharedPtr& enclosing, int arraySize = DEFAULT_SIZE)
		: Node			(TYPE_ARRAY_ELEMENT, enclosing)
		, m_arraySize	(arraySize)
	{
		DE_ASSERT(enclosing->getType() == TYPE_STORAGE_QUALIFIER		||
				  enclosing->getType() == TYPE_LAYOUT_QUALIFIER			||
				  enclosing->getType() == TYPE_INTERPOLATION_QUALIFIER	||
				  enclosing->getType() == TYPE_INTERFACE_BLOCK			||
				  enclosing->getType() == TYPE_ARRAY_ELEMENT			||
				  enclosing->getType() == TYPE_STRUCT_MEMBER			||
				  enclosing->getType() == TYPE_TRANSFORM_FEEDBACK_TARGET);
	}

	const int m_arraySize;

	enum
	{
		DEFAULT_SIZE	= -1,
		UNSIZED_ARRAY	= -2,
	};
};

class StructMember : public Node
{
public:
	StructMember (const SharedPtr& enclosing)
		: Node(TYPE_STRUCT_MEMBER, enclosing)
	{
		DE_ASSERT(enclosing->getType() == TYPE_STORAGE_QUALIFIER		||
				  enclosing->getType() == TYPE_LAYOUT_QUALIFIER			||
				  enclosing->getType() == TYPE_INTERPOLATION_QUALIFIER	||
				  enclosing->getType() == TYPE_INTERFACE_BLOCK			||
				  enclosing->getType() == TYPE_ARRAY_ELEMENT			||
				  enclosing->getType() == TYPE_STRUCT_MEMBER			||
				  enclosing->getType() == TYPE_TRANSFORM_FEEDBACK_TARGET);
	}
};

class LayoutQualifier : public Node
{
public:
	LayoutQualifier (const SharedPtr& enclosing, const glu::Layout& layout)
		: Node		(TYPE_LAYOUT_QUALIFIER, enclosing)
		, m_layout	(layout)
	{
		DE_ASSERT(enclosing->getType() == TYPE_STORAGE_QUALIFIER		||
				  enclosing->getType() == TYPE_LAYOUT_QUALIFIER			||
				  enclosing->getType() == TYPE_INTERPOLATION_QUALIFIER	||
				  enclosing->getType() == TYPE_DEFAULT_BLOCK			||
				  enclosing->getType() == TYPE_INTERFACE_BLOCK);
	}

	const glu::Layout m_layout;
};

class InterpolationQualifier : public Node
{
public:
	InterpolationQualifier (const SharedPtr& enclosing, const glu::Interpolation& interpolation)
		: Node				(TYPE_INTERPOLATION_QUALIFIER, enclosing)
		, m_interpolation	(interpolation)
	{
		DE_ASSERT(enclosing->getType() == TYPE_STORAGE_QUALIFIER		||
				  enclosing->getType() == TYPE_LAYOUT_QUALIFIER			||
				  enclosing->getType() == TYPE_INTERPOLATION_QUALIFIER	||
				  enclosing->getType() == TYPE_DEFAULT_BLOCK			||
				  enclosing->getType() == TYPE_INTERFACE_BLOCK);
	}

	const glu::Interpolation m_interpolation;
};

class ShaderSet : public Node
{
public:
				ShaderSet			(const SharedPtr& enclosing, glu::GLSLVersion version);
				ShaderSet			(const SharedPtr& enclosing, glu::GLSLVersion version, deUint32 stagesPresentBits, deUint32 stagesReferencingBits);

	void		setStage			(glu::ShaderType type, bool referencing);
	bool		isStagePresent		(glu::ShaderType stage) const;
	bool		isStageReferencing	(glu::ShaderType stage) const;

	deUint32	getReferencingMask	(void) const;

	const glu::GLSLVersion	m_version;
private:
	bool		m_stagePresent[glu::SHADERTYPE_LAST];
	bool		m_stageReferencing[glu::SHADERTYPE_LAST];
};

ShaderSet::ShaderSet (const SharedPtr& enclosing, glu::GLSLVersion version)
	: Node		(TYPE_SHADER_SET, enclosing)
	, m_version	(version)
{
	DE_ASSERT(enclosing->getType() == TYPE_PROGRAM);

	deMemset(m_stagePresent, 0, sizeof(m_stagePresent));
	deMemset(m_stageReferencing, 0, sizeof(m_stageReferencing));
}

ShaderSet::ShaderSet (const SharedPtr&	enclosing,
					  glu::GLSLVersion	version,
					  deUint32			stagesPresentBits,
					  deUint32			stagesReferencingBits)
	: Node		(TYPE_SHADER_SET, enclosing)
	, m_version	(version)
{
	for (deUint32 stageNdx = 0; stageNdx < glu::SHADERTYPE_LAST; ++stageNdx)
	{
		const deUint32	stageMask			= (1u << stageNdx);
		const bool		stagePresent		= (stagesPresentBits & stageMask) != 0;
		const bool		stageReferencing	= (stagesReferencingBits & stageMask) != 0;

		DE_ASSERT(stagePresent || !stageReferencing);

		m_stagePresent[stageNdx]		= stagePresent;
		m_stageReferencing[stageNdx]	= stageReferencing;
	}
}

void ShaderSet::setStage (glu::ShaderType type, bool referencing)
{
	DE_ASSERT(type < glu::SHADERTYPE_LAST);
	m_stagePresent[type] = true;
	m_stageReferencing[type] = referencing;
}

bool ShaderSet::isStagePresent (glu::ShaderType stage) const
{
	DE_ASSERT(stage < glu::SHADERTYPE_LAST);
	return m_stagePresent[stage];
}

bool ShaderSet::isStageReferencing (glu::ShaderType stage) const
{
	DE_ASSERT(stage < glu::SHADERTYPE_LAST);
	return m_stageReferencing[stage];
}

deUint32 ShaderSet::getReferencingMask (void) const
{
	deUint32 mask = 0;
	for (deUint32 stage = 0; stage < glu::SHADERTYPE_LAST; ++stage)
	{
		if (m_stageReferencing[stage])
			mask |= (1u << stage);
	}
	return mask;
}

class TransformFeedbackTarget : public Node
{
public:
	TransformFeedbackTarget (const SharedPtr& enclosing, const char* builtinVarName = DE_NULL)
		: Node				(TYPE_TRANSFORM_FEEDBACK_TARGET, enclosing)
		, m_builtinVarName	(builtinVarName)
	{
	}

	const char* const m_builtinVarName;
};

} // ResourceDefinition

static glu::Precision getDataTypeDefaultPrecision (const glu::DataType& type)
{
	if (glu::isDataTypeBoolOrBVec(type))
		return glu::PRECISION_LAST;
	else if (glu::isDataTypeScalarOrVector(type) || glu::isDataTypeMatrix(type))
		return glu::PRECISION_HIGHP;
	else if (glu::isDataTypeSampler(type))
		return glu::PRECISION_HIGHP;
	else if (glu::isDataTypeImage(type))
		return glu::PRECISION_HIGHP;
	else if (type == glu::TYPE_UINT_ATOMIC_COUNTER)
		return glu::PRECISION_HIGHP;

	DE_ASSERT(false);
	return glu::PRECISION_LAST;
}

static de::MovePtr<ProgramInterfaceDefinition::Program>	generateProgramDefinitionFromResource (const ResourceDefinition::Node* resource)
{
	de::MovePtr<ProgramInterfaceDefinition::Program>	program	(new ProgramInterfaceDefinition::Program());
	const ResourceDefinition::Node*						head	= resource;

	if (head->getType() == ResourceDefinition::Node::TYPE_VARIABLE)
	{
		DE_ASSERT(dynamic_cast<const ResourceDefinition::Variable*>(resource));

		enum BindingType
		{
			BINDING_VARIABLE,
			BINDING_INTERFACE_BLOCK,
			BINDING_DEFAULT_BLOCK
		};

		int											structNdx				= 0;
		int											autoAssignArraySize		= 0;
		const glu::DataType							basicType				= static_cast<const ResourceDefinition::Variable*>(resource)->m_dataType;
		BindingType									boundObject				= BINDING_VARIABLE;
		glu::VariableDeclaration					variable				(glu::VarType(basicType, getDataTypeDefaultPrecision(basicType)), "target");
		glu::InterfaceBlock							interfaceBlock;
		ProgramInterfaceDefinition::DefaultBlock	defaultBlock;
		std::vector<std::string>					feedbackTargetVaryingPath;
		bool										feedbackTargetSet		= false;

		// image specific
		if (glu::isDataTypeImage(basicType))
		{
			variable.memoryAccessQualifierBits |= glu::MEMORYACCESSQUALIFIER_READONLY_BIT;
			variable.layout.binding = 1;

			if (basicType >= glu::TYPE_IMAGE_2D && basicType <= glu::TYPE_IMAGE_3D)
				variable.layout.format = glu::FORMATLAYOUT_RGBA8;
			else if (basicType >= glu::TYPE_INT_IMAGE_2D && basicType <= glu::TYPE_INT_IMAGE_3D)
				variable.layout.format = glu::FORMATLAYOUT_RGBA8I;
			else if (basicType >= glu::TYPE_UINT_IMAGE_2D && basicType <= glu::TYPE_UINT_IMAGE_3D)
				variable.layout.format = glu::FORMATLAYOUT_RGBA8UI;
			else
				DE_ASSERT(false);
		}

		// atomic counter specific
		if (basicType == glu::TYPE_UINT_ATOMIC_COUNTER)
			variable.layout.binding = 1;

		for (head = head->getEnclosingNode(); head; head = head->getEnclosingNode())
		{
			if (head->getType() == ResourceDefinition::Node::TYPE_STORAGE_QUALIFIER)
			{
				const ResourceDefinition::StorageQualifier* qualifier = static_cast<const ResourceDefinition::StorageQualifier*>(head);

				DE_ASSERT(dynamic_cast<const ResourceDefinition::StorageQualifier*>(head));

				if (boundObject == BINDING_VARIABLE)
				{
					DE_ASSERT(variable.storage == glu::STORAGE_LAST);
					variable.storage = qualifier->m_storage;
				}
				else if (boundObject == BINDING_INTERFACE_BLOCK)
				{
					DE_ASSERT(interfaceBlock.storage == glu::STORAGE_LAST);
					interfaceBlock.storage = qualifier->m_storage;
				}
				else
					DE_ASSERT(false);
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_LAYOUT_QUALIFIER)
			{
				const ResourceDefinition::LayoutQualifier*	qualifier		= static_cast<const ResourceDefinition::LayoutQualifier*>(head);
				glu::Layout*								targetLayout	= DE_NULL;

				DE_ASSERT(dynamic_cast<const ResourceDefinition::LayoutQualifier*>(head));

				if (boundObject == BINDING_VARIABLE)
					targetLayout = &variable.layout;
				else if (boundObject == BINDING_INTERFACE_BLOCK)
					targetLayout = &interfaceBlock.layout;
				else
					DE_ASSERT(false);

				if (qualifier->m_layout.location != -1)
					targetLayout->location = qualifier->m_layout.location;

				if (qualifier->m_layout.binding != -1)
					targetLayout->binding = qualifier->m_layout.binding;

				if (qualifier->m_layout.offset != -1)
					targetLayout->offset = qualifier->m_layout.offset;

				if (qualifier->m_layout.format != glu::FORMATLAYOUT_LAST)
					targetLayout->format = qualifier->m_layout.format;

				if (qualifier->m_layout.matrixOrder != glu::MATRIXORDER_LAST)
					targetLayout->matrixOrder = qualifier->m_layout.matrixOrder;
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_INTERPOLATION_QUALIFIER)
			{
				const ResourceDefinition::InterpolationQualifier* qualifier = static_cast<const ResourceDefinition::InterpolationQualifier*>(head);

				DE_ASSERT(dynamic_cast<const ResourceDefinition::InterpolationQualifier*>(head));

				if (boundObject == BINDING_VARIABLE)
					variable.interpolation = qualifier->m_interpolation;
				else
					DE_ASSERT(false);
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_ARRAY_ELEMENT)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::ArrayElement*>(head));

				const ResourceDefinition::ArrayElement*	arrayElement = static_cast<const ResourceDefinition::ArrayElement*>(head);
				int										arraySize;

				// Vary array size per level
				if (arrayElement->m_arraySize == ResourceDefinition::ArrayElement::DEFAULT_SIZE)
				{
					if (--autoAssignArraySize <= 1)
						autoAssignArraySize = 3;

					arraySize = autoAssignArraySize;
				}
				else if (arrayElement->m_arraySize == ResourceDefinition::ArrayElement::UNSIZED_ARRAY)
					arraySize = glu::VarType::UNSIZED_ARRAY;
				else
					arraySize = arrayElement->m_arraySize;

				if (boundObject == BINDING_VARIABLE)
					variable.varType = glu::VarType(variable.varType, arraySize);
				else if (boundObject == BINDING_INTERFACE_BLOCK)
					interfaceBlock.dimensions.push_back(arraySize);
				else
					DE_ASSERT(false);

				if (feedbackTargetSet)
					feedbackTargetVaryingPath.back().append("[0]");
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_STRUCT_MEMBER)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::StructMember*>(head));
				DE_ASSERT(boundObject == BINDING_VARIABLE);

				// Struct members cannot contain any qualifiers except precision
				DE_ASSERT(variable.interpolation == glu::INTERPOLATION_LAST);
				DE_ASSERT(variable.layout == glu::Layout());
				DE_ASSERT(variable.memoryAccessQualifierBits == 0);
				DE_ASSERT(variable.storage == glu::STORAGE_LAST);

				{
					glu::StructType* structPtr = new glu::StructType(("StructType" + de::toString(structNdx++)).c_str());
					structPtr->addMember(variable.name.c_str(), variable.varType);

					variable = glu::VariableDeclaration(glu::VarType(structPtr), "target");
				}

				if (feedbackTargetSet)
					feedbackTargetVaryingPath.push_back("target");
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_INTERFACE_BLOCK)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::InterfaceBlock*>(head));
				DE_ASSERT(boundObject == BINDING_VARIABLE);

				const bool named = static_cast<const ResourceDefinition::InterfaceBlock*>(head)->m_named;

				boundObject = BINDING_INTERFACE_BLOCK;

				interfaceBlock.interfaceName = "TargetInterface";
				interfaceBlock.instanceName = (named) ? ("targetInstance") : ("");
				interfaceBlock.variables.push_back(variable);

				if (feedbackTargetSet && !interfaceBlock.instanceName.empty())
					feedbackTargetVaryingPath.push_back(interfaceBlock.interfaceName);
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_DEFAULT_BLOCK)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::DefaultBlock*>(head));
				DE_ASSERT(boundObject == BINDING_VARIABLE || boundObject == BINDING_INTERFACE_BLOCK);

				if (boundObject == BINDING_VARIABLE)
					defaultBlock.variables.push_back(variable);
				else if (boundObject == BINDING_INTERFACE_BLOCK)
					defaultBlock.interfaceBlocks.push_back(interfaceBlock);
				else
					DE_ASSERT(false);

				boundObject = BINDING_DEFAULT_BLOCK;
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_SHADER)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::Shader*>(head));

				const ResourceDefinition::Shader*	shaderDef	= static_cast<const ResourceDefinition::Shader*>(head);
				ProgramInterfaceDefinition::Shader* shader		= program->addShader(shaderDef->m_type, shaderDef->m_version);

				shader->getDefaultBlock() = defaultBlock;
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_SHADER_SET)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::ShaderSet*>(head));

				const ResourceDefinition::ShaderSet* shaderDef = static_cast<const ResourceDefinition::ShaderSet*>(head);

				for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; ++shaderType)
				{
					if (shaderDef->isStagePresent((glu::ShaderType)shaderType))
					{
						ProgramInterfaceDefinition::Shader* shader = program->addShader((glu::ShaderType)shaderType, shaderDef->m_version);

						if (shaderDef->isStageReferencing((glu::ShaderType)shaderType))
							shader->getDefaultBlock() = defaultBlock;
					}
				}
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_PROGRAM)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::Program*>(head));

				const ResourceDefinition::Program* programDef = static_cast<const ResourceDefinition::Program*>(head);

				program->setSeparable(programDef->m_separable);

				DE_ASSERT(feedbackTargetSet == !feedbackTargetVaryingPath.empty());
				if (!feedbackTargetVaryingPath.empty())
				{
					std::ostringstream buf;

					for (std::vector<std::string>::reverse_iterator it = feedbackTargetVaryingPath.rbegin(); it != feedbackTargetVaryingPath.rend(); ++it)
					{
						if (it != feedbackTargetVaryingPath.rbegin())
							buf << ".";
						buf << *it;
					}

					program->addTransformFeedbackVarying(buf.str());
					program->setTransformFeedbackMode(GL_INTERLEAVED_ATTRIBS);
				}
				break;
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_TRANSFORM_FEEDBACK_TARGET)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::TransformFeedbackTarget*>(head));

				const ResourceDefinition::TransformFeedbackTarget* feedbackTarget = static_cast<const ResourceDefinition::TransformFeedbackTarget*>(head);

				DE_ASSERT(feedbackTarget->m_builtinVarName == DE_NULL);
				DE_UNREF(feedbackTarget);

				feedbackTargetSet = true;
				feedbackTargetVaryingPath.push_back(variable.name);
			}
			else
			{
				DE_ASSERT(DE_FALSE);
				break;
			}
		}
	}
	else if (head->getType() == ResourceDefinition::Node::TYPE_DEFAULT_BLOCK ||
			 head->getType() == ResourceDefinition::Node::TYPE_TRANSFORM_FEEDBACK_TARGET)
	{
		const char* feedbackTargetVaryingName = DE_NULL;

		// empty default block

		for (; head; head = head->getEnclosingNode())
		{
			if (head->getType() == ResourceDefinition::Node::TYPE_SHADER)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::Shader*>(head));

				const ResourceDefinition::Shader* shaderDef = static_cast<const ResourceDefinition::Shader*>(head);

				program->addShader(shaderDef->m_type, shaderDef->m_version);
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_SHADER_SET)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::ShaderSet*>(head));

				const ResourceDefinition::ShaderSet* shaderDef = static_cast<const ResourceDefinition::ShaderSet*>(head);

				for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; ++shaderType)
					if (shaderDef->isStagePresent((glu::ShaderType)shaderType))
						program->addShader((glu::ShaderType)shaderType, shaderDef->m_version);
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_PROGRAM)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::Program*>(head));

				const ResourceDefinition::Program* programDef = static_cast<const ResourceDefinition::Program*>(head);

				program->setSeparable(programDef->m_separable);
				if (feedbackTargetVaryingName)
				{
					program->addTransformFeedbackVarying(std::string(feedbackTargetVaryingName));
					program->setTransformFeedbackMode(GL_INTERLEAVED_ATTRIBS);
				}
				break;
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_TRANSFORM_FEEDBACK_TARGET)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::TransformFeedbackTarget*>(head));

				const ResourceDefinition::TransformFeedbackTarget* feedbackTarget = static_cast<const ResourceDefinition::TransformFeedbackTarget*>(head);

				DE_ASSERT(feedbackTarget->m_builtinVarName != DE_NULL);

				feedbackTargetVaryingName = feedbackTarget->m_builtinVarName;
			}
			else if (head->getType() == ResourceDefinition::Node::TYPE_DEFAULT_BLOCK)
			{
			}
			else
			{
				DE_ASSERT(DE_FALSE);
				break;
			}
		}
	}

	if (program->hasStage(glu::SHADERTYPE_GEOMETRY))
		program->setGeometryNumOutputVertices(1);
	if (program->hasStage(glu::SHADERTYPE_TESSELLATION_CONTROL) || program->hasStage(glu::SHADERTYPE_TESSELLATION_EVALUATION))
		program->setTessellationNumOutputPatchVertices(1);

	return program;
}

static void checkAndLogProgram (const glu::ShaderProgram& program, const ProgramInterfaceDefinition::Program* programDefinition, const glw::Functions& gl, tcu::TestLog& log)
{
	const tcu::ScopedLogSection section(log, "Program", "Program");

	log << program;
	if (!program.isOk())
	{
		log << tcu::TestLog::Message << "Program build failed, checking if program exceeded implementation limits" << tcu::TestLog::EndMessage;
		checkProgramResourceUsage(programDefinition, gl, log);

		// within limits
		throw tcu::TestError("could not build program");
	}
}

// Resource list query case

class ResourceListTestCase : public TestCase
{
public:
												ResourceListTestCase		(Context& context, const ResourceDefinition::Node::SharedPtr& targetResource, ProgramInterface interface, const char* name = DE_NULL);
												~ResourceListTestCase		(void);

protected:
	void										init						(void);
	void										deinit						(void);
	IterateResult								iterate						(void);

	void										queryResourceList			(std::vector<std::string>& dst, glw::GLuint program);
	bool										verifyResourceList			(const std::vector<std::string>& resourceList, const std::vector<std::string>& expectedResources);
	bool										verifyResourceIndexQuery	(const std::vector<std::string>& resourceList, const std::vector<std::string>& referenceResources, glw::GLuint program);
	bool										verifyMaxNameLength			(const std::vector<std::string>& referenceResourceList, glw::GLuint program);

	static std::string							genTestCaseName				(ProgramInterface interface, const ResourceDefinition::Node*);
	static bool									isArrayedInterface			(ProgramInterface interface, deUint32 stageBits);

	const ProgramInterface						m_programInterface;
	ResourceDefinition::Node::SharedPtr			m_targetResource;
	ProgramInterfaceDefinition::Program*		m_programDefinition;
};

ResourceListTestCase::ResourceListTestCase (Context& context, const ResourceDefinition::Node::SharedPtr& targetResource, ProgramInterface interface, const char* name)
	: TestCase				(context, (name == DE_NULL) ? (genTestCaseName(interface, targetResource.get()).c_str()) : (name), "")
	, m_programInterface	(interface)
	, m_targetResource		(targetResource)
	, m_programDefinition	(DE_NULL)
{
	// GL_ATOMIC_COUNTER_BUFFER: no resource names
	DE_ASSERT(m_programInterface != PROGRAMINTERFACE_ATOMIC_COUNTER_BUFFER);
}

ResourceListTestCase::~ResourceListTestCase (void)
{
	deinit();
}

void ResourceListTestCase::init (void)
{
	m_programDefinition	= generateProgramDefinitionFromResource(m_targetResource.get()).release();
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if ((m_programDefinition->hasStage(glu::SHADERTYPE_TESSELLATION_CONTROL) || m_programDefinition->hasStage(glu::SHADERTYPE_TESSELLATION_EVALUATION)) &&
		!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
	{
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader extension");
	}
	if (m_programDefinition->hasStage(glu::SHADERTYPE_GEOMETRY) &&
		!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
	{
		throw tcu::NotSupportedError("Test requires GL_EXT_geometry_shader extension");
	}
	if (programContainsIOBlocks(m_programDefinition) &&
		!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_shader_io_blocks"))
	{
		throw tcu::NotSupportedError("Test requires GL_EXT_shader_io_blocks extension");
	}
}

void ResourceListTestCase::deinit (void)
{
	m_targetResource.clear();

	delete m_programDefinition;
	m_programDefinition = DE_NULL;
}

ResourceListTestCase::IterateResult ResourceListTestCase::iterate (void)
{
	const glu::ShaderProgram program(m_context.getRenderContext(), generateProgramInterfaceProgramSources(m_programDefinition));

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	checkAndLogProgram(program, m_programDefinition, m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	// Check resource list
	{
		const tcu::ScopedLogSection	section				(m_testCtx.getLog(), "ResourceList", "Resource list");
		std::vector<std::string>	resourceList;
		std::vector<std::string>	expectedResources;

		queryResourceList(resourceList, program.getProgram());
		expectedResources = getProgramInterfaceResourceList(m_programDefinition, m_programInterface);

		// verify the list and the expected list match

		if (!verifyResourceList(resourceList, expectedResources))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "invalid resource list");

		// verify GetProgramResourceIndex() matches the indices of the list

		if (!verifyResourceIndexQuery(resourceList, expectedResources, program.getProgram()))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "GetProgramResourceIndex returned unexpected values");

		// Verify MAX_NAME_LENGTH
		if (!verifyMaxNameLength(resourceList, program.getProgram()))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "MAX_NAME_LENGTH invalid");
	}

	return STOP;
}

void ResourceListTestCase::queryResourceList (std::vector<std::string>& dst, glw::GLuint program)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const glw::GLenum		programInterface	= getProgramInterfaceGLEnum(m_programInterface);
	glw::GLint				numActiveResources	= 0;
	glw::GLint				maxNameLength		= 0;
	std::vector<char>		buffer;

	m_testCtx.getLog() << tcu::TestLog::Message << "Querying " << glu::getProgramInterfaceName(programInterface) << " interface:" << tcu::TestLog::EndMessage;

	gl.getProgramInterfaceiv(program, programInterface, GL_ACTIVE_RESOURCES, &numActiveResources);
	gl.getProgramInterfaceiv(program, programInterface, GL_MAX_NAME_LENGTH, &maxNameLength);
	GLU_EXPECT_NO_ERROR(gl.getError(), "query interface");

	m_testCtx.getLog()	<< tcu::TestLog::Message
						<< "\tGL_ACTIVE_RESOURCES = " << numActiveResources << "\n"
						<< "\tGL_MAX_NAME_LENGTH = " << maxNameLength
						<< tcu::TestLog::EndMessage;

	m_testCtx.getLog() << tcu::TestLog::Message << "Querying all active resources" << tcu::TestLog::EndMessage;

	buffer.resize(maxNameLength+1, '\0');

	for (int resourceNdx = 0; resourceNdx < numActiveResources; ++resourceNdx)
	{
		glw::GLint written = 0;

		gl.getProgramResourceName(program, programInterface, resourceNdx, maxNameLength, &written, &buffer[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "query resource name");

		dst.push_back(std::string(&buffer[0], written));
	}
}

bool ResourceListTestCase::verifyResourceList (const std::vector<std::string>& resourceList, const std::vector<std::string>& expectedResources)
{
	bool error = false;

	// Log and compare resource lists

	m_testCtx.getLog() << tcu::TestLog::Message << "GL returned resources:" << tcu::TestLog::EndMessage;

	for (int ndx = 0; ndx < (int)resourceList.size(); ++ndx)
	{
		// dummyZero is a uniform that may be added by
		// generateProgramInterfaceProgramSources.  Omit it here to avoid
		// confusion about the output.
		if (resourceList[ndx] != getDummyZeroUniformName())
			m_testCtx.getLog() << tcu::TestLog::Message << "\t" << ndx << ": " << resourceList[ndx] << tcu::TestLog::EndMessage;
	}

	m_testCtx.getLog() << tcu::TestLog::Message << "Expected list of resources:" << tcu::TestLog::EndMessage;

	for (int ndx = 0; ndx < (int)expectedResources.size(); ++ndx)
		m_testCtx.getLog() << tcu::TestLog::Message << "\t" << ndx << ": " << expectedResources[ndx] << tcu::TestLog::EndMessage;

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying resource list contents." << tcu::TestLog::EndMessage;

	for (int ndx = 0; ndx < (int)expectedResources.size(); ++ndx)
	{
		if (!de::contains(resourceList.begin(), resourceList.end(), expectedResources[ndx]))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, resource list did not contain active resource " << expectedResources[ndx] << tcu::TestLog::EndMessage;
			error = true;
		}
	}

	for (int ndx = 0; ndx < (int)resourceList.size(); ++ndx)
	{
		if (!de::contains(expectedResources.begin(), expectedResources.end(), resourceList[ndx]))
		{
			// Ignore all builtin variables or the variable dummyZero,
			// mismatch causes errors otherwise.  dummyZero is a uniform that
			// may be added by generateProgramInterfaceProgramSources.
			if (deStringBeginsWith(resourceList[ndx].c_str(), "gl_") == DE_FALSE &&
				resourceList[ndx] != getDummyZeroUniformName())
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, resource list contains unexpected resource name " << resourceList[ndx] << tcu::TestLog::EndMessage;
				error = true;
			}
			else
				m_testCtx.getLog() << tcu::TestLog::Message << "Note, resource list contains unknown built-in " << resourceList[ndx] << ". This variable is ignored." << tcu::TestLog::EndMessage;
		}
	}

	return !error;
}

bool ResourceListTestCase::verifyResourceIndexQuery (const std::vector<std::string>& resourceList, const std::vector<std::string>& referenceResources, glw::GLuint program)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const glw::GLenum		programInterface	= getProgramInterfaceGLEnum(m_programInterface);
	bool					error				= false;

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying GetProgramResourceIndex returns correct indices for resource names." << tcu::TestLog::EndMessage;

	for (int ndx = 0; ndx < (int)referenceResources.size(); ++ndx)
	{
		const glw::GLuint index = gl.getProgramResourceIndex(program, programInterface, referenceResources[ndx].c_str());
		GLU_EXPECT_NO_ERROR(gl.getError(), "query resource index");

		if (index == GL_INVALID_INDEX)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, for active resource \"" << referenceResources[ndx] << "\" got index GL_INVALID_INDEX." << tcu::TestLog::EndMessage;
			error = true;
		}
		else if ((int)index >= (int)resourceList.size())
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, for active resource \"" << referenceResources[ndx] << "\" got index " << index << " (larger or equal to GL_ACTIVE_RESOURCES)." << tcu::TestLog::EndMessage;
			error = true;
		}
		else if (resourceList[index] != referenceResources[ndx])
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, for active resource \"" << referenceResources[ndx] << "\" got index (index = " << index << ") of another resource (" << resourceList[index] << ")." << tcu::TestLog::EndMessage;
			error = true;
		}
	}

	// Query for "name" should match "name[0]" except for XFB

	if (m_programInterface != PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING)
	{
		for (int ndx = 0; ndx < (int)referenceResources.size(); ++ndx)
		{
			if (de::endsWith(referenceResources[ndx], "[0]"))
			{
				const std::string	queryString	= referenceResources[ndx].substr(0, referenceResources[ndx].length()-3);
				const glw::GLuint	index		= gl.getProgramResourceIndex(program, programInterface, queryString.c_str());
				GLU_EXPECT_NO_ERROR(gl.getError(), "query resource index");

				if (index == GL_INVALID_INDEX)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Error, query for \"" << queryString << "\" resulted in index GL_INVALID_INDEX." << tcu::TestLog::EndMessage;
					error = true;
				}
				else if ((int)index >= (int)resourceList.size())
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Error, query for \"" << queryString << "\" resulted in index " << index << " (larger or equal to GL_ACTIVE_RESOURCES)." << tcu::TestLog::EndMessage;
					error = true;
				}
				else if (resourceList[index] != queryString + "[0]")
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Error, query for \"" << queryString << "\" got index (index = " << index << ") of another resource (\"" << resourceList[index] << "\")." << tcu::TestLog::EndMessage;
					error = true;
				}
			}
		}
	}

	return !error;
}

bool ResourceListTestCase::verifyMaxNameLength (const std::vector<std::string>& resourceList, glw::GLuint program)
{
	const glw::Functions&	gl						= m_context.getRenderContext().getFunctions();
	const glw::GLenum		programInterface		= getProgramInterfaceGLEnum(m_programInterface);
	glw::GLint				maxNameLength			= 0;
	glw::GLint				expectedMaxNameLength	= 0;

	gl.getProgramInterfaceiv(program, programInterface, GL_MAX_NAME_LENGTH, &maxNameLength);
	GLU_EXPECT_NO_ERROR(gl.getError(), "query interface");

	for (int ndx = 0; ndx < (int)resourceList.size(); ++ndx)
		expectedMaxNameLength = de::max(expectedMaxNameLength, (int)resourceList[ndx].size() + 1);

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying MAX_NAME_LENGTH, expecting " << expectedMaxNameLength << " (i.e. consistent with the queried resource list)" << tcu::TestLog::EndMessage;

	if (expectedMaxNameLength != maxNameLength)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Error, got " << maxNameLength << tcu::TestLog::EndMessage;
		return false;
	}

	return true;
}

std::string ResourceListTestCase::genTestCaseName (ProgramInterface interface, const ResourceDefinition::Node* root)
{
	bool				isImplicitlySizedArray	= false;
	bool				hasVariable				= false;
	bool				accumulateName			= true;
	std::string			buf						= "var";
	std::string			prefix;

	for (const ResourceDefinition::Node* node = root; node; node = node->getEnclosingNode())
	{
		switch (node->getType())
		{
			case ResourceDefinition::Node::TYPE_VARIABLE:
			{
				hasVariable = true;
				break;
			}

			case ResourceDefinition::Node::TYPE_STRUCT_MEMBER:
			{
				if (accumulateName)
					buf += "_struct";
				break;
			}

			case ResourceDefinition::Node::TYPE_ARRAY_ELEMENT:
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::ArrayElement*>(node));
				const ResourceDefinition::ArrayElement* arrayElement = static_cast<const ResourceDefinition::ArrayElement*>(node);

				isImplicitlySizedArray = (arrayElement->m_arraySize == ResourceDefinition::ArrayElement::UNSIZED_ARRAY);

				if (accumulateName)
					buf += "_array";
				break;
			}

			case ResourceDefinition::Node::TYPE_STORAGE_QUALIFIER:
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::StorageQualifier*>(node));
				const ResourceDefinition::StorageQualifier* storageDef = static_cast<const ResourceDefinition::StorageQualifier*>(node);

				if (storageDef->m_storage == glu::STORAGE_PATCH_IN ||
					storageDef->m_storage == glu::STORAGE_PATCH_OUT)
				{
					if (accumulateName)
						prefix += "patch_";
				}
				break;
			}

			case ResourceDefinition::Node::TYPE_SHADER:
			case ResourceDefinition::Node::TYPE_SHADER_SET:
			{
				bool arrayedInterface;

				if (node->getType() == ResourceDefinition::Node::TYPE_SHADER)
				{
					DE_ASSERT(dynamic_cast<const ResourceDefinition::Shader*>(node));
					const ResourceDefinition::Shader* shaderDef = static_cast<const ResourceDefinition::Shader*>(node);

					arrayedInterface = isArrayedInterface(interface, (1u << shaderDef->m_type));
				}
				else
				{
					DE_ASSERT(node->getType() == ResourceDefinition::Node::TYPE_SHADER_SET);
					DE_ASSERT(dynamic_cast<const ResourceDefinition::ShaderSet*>(node));
					const ResourceDefinition::ShaderSet* shaderDef = static_cast<const ResourceDefinition::ShaderSet*>(node);

					arrayedInterface = isArrayedInterface(interface, shaderDef->getReferencingMask());
				}

				if (arrayedInterface && isImplicitlySizedArray)
				{
					// omit implicit arrayness from name, i.e. remove trailing "_array"
					DE_ASSERT(de::endsWith(buf, "_array"));
					buf = buf.substr(0, buf.length() - 6);
				}

				break;
			}

			case ResourceDefinition::Node::TYPE_INTERFACE_BLOCK:
			{
				accumulateName = false;
				break;
			}

			default:
				break;
		}
	}

	if (!hasVariable)
		return prefix + "empty";
	else
		return prefix + buf;
}

bool ResourceListTestCase::isArrayedInterface (ProgramInterface interface, deUint32 stageBits)
{
	if (interface == PROGRAMINTERFACE_PROGRAM_INPUT)
	{
		const glu::ShaderType firstStage = getShaderMaskFirstStage(stageBits);
		return	firstStage == glu::SHADERTYPE_TESSELLATION_CONTROL		||
				firstStage == glu::SHADERTYPE_TESSELLATION_EVALUATION	||
				firstStage == glu::SHADERTYPE_GEOMETRY;
	}
	else if (interface == PROGRAMINTERFACE_PROGRAM_OUTPUT)
	{
		const glu::ShaderType lastStage = getShaderMaskLastStage(stageBits);
		return	lastStage == glu::SHADERTYPE_TESSELLATION_CONTROL;
	}
	return false;
}

// Resouce property query case

class ResourceTestCase : public ProgramInterfaceQueryTestCase
{
public:
															ResourceTestCase			(Context& context, const ResourceDefinition::Node::SharedPtr& targetResource, const ProgramResourceQueryTestTarget& queryTarget, const char* name = DE_NULL);
															~ResourceTestCase			(void);

private:
	void													init						(void);
	void													deinit						(void);
	const ProgramInterfaceDefinition::Program*				getProgramDefinition		(void) const;
	std::vector<std::string>								getQueryTargetResources		(void) const;

	static std::string										genTestCaseName				(const ResourceDefinition::Node*);
	static std::string										genMultilineDescription		(const ResourceDefinition::Node*);

	ResourceDefinition::Node::SharedPtr						m_targetResource;
	ProgramInterfaceDefinition::Program*					m_program;
	std::vector<std::string>								m_targetResources;
};

ResourceTestCase::ResourceTestCase (Context& context, const ResourceDefinition::Node::SharedPtr& targetResource, const ProgramResourceQueryTestTarget& queryTarget, const char* name)
	: ProgramInterfaceQueryTestCase	(context, (name == DE_NULL) ? (genTestCaseName(targetResource.get()).c_str()) : (name), "", queryTarget)
	, m_targetResource				(targetResource)
	, m_program						(DE_NULL)
{
}

ResourceTestCase::~ResourceTestCase (void)
{
	deinit();
}

void ResourceTestCase::init (void)
{
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< genMultilineDescription(m_targetResource.get())
		<< tcu::TestLog::EndMessage;

	// Program
	{
		// Generate interface with target resource
		m_program = generateProgramDefinitionFromResource(m_targetResource.get()).release();
		m_targetResources = getProgramInterfaceResourceList(m_program, getTargetInterface());
	}
}

void ResourceTestCase::deinit (void)
{
	m_targetResource.clear();

	delete m_program;
	m_program = DE_NULL;

	m_targetResources = std::vector<std::string>();
}

const ProgramInterfaceDefinition::Program* ResourceTestCase::getProgramDefinition (void) const
{
	return m_program;
}

std::vector<std::string> ResourceTestCase::getQueryTargetResources (void) const
{
	return m_targetResources;
}

std::string ResourceTestCase::genTestCaseName (const ResourceDefinition::Node* resource)
{
	if (resource->getType() == ResourceDefinition::Node::TYPE_VARIABLE)
	{
		DE_ASSERT(dynamic_cast<const ResourceDefinition::Variable*>(resource));

		const ResourceDefinition::Variable* variable = static_cast<const ResourceDefinition::Variable*>(resource);

		return convertGLTypeNameToTestName(glu::getDataTypeName(variable->m_dataType));
	}

	DE_ASSERT(false);
	return "";
}

std::string ResourceTestCase::genMultilineDescription (const ResourceDefinition::Node* resource)
{
	if (resource->getType() == ResourceDefinition::Node::TYPE_VARIABLE)
	{
		DE_ASSERT(dynamic_cast<const ResourceDefinition::Variable*>(resource));

		const ResourceDefinition::Variable*	varDef				= static_cast<const ResourceDefinition::Variable*>(resource);
		std::ostringstream					buf;
		std::ostringstream					structureDescriptor;
		std::string							uniformType;

		for (const ResourceDefinition::Node* node = resource; node; node = node->getEnclosingNode())
		{
			if (node->getType() == ResourceDefinition::Node::TYPE_STORAGE_QUALIFIER)
			{
				DE_ASSERT(dynamic_cast<const ResourceDefinition::StorageQualifier*>(node));

				const ResourceDefinition::StorageQualifier*	storageDef = static_cast<const ResourceDefinition::StorageQualifier*>(node);

				uniformType = std::string(" ") + glu::getStorageName(storageDef->m_storage);
				structureDescriptor << "\n\tdeclared as \"" << glu::getStorageName(storageDef->m_storage) << "\"";
			}

			if (node->getType() == ResourceDefinition::Node::TYPE_ARRAY_ELEMENT)
				structureDescriptor << "\n\tarray";

			if (node->getType() == ResourceDefinition::Node::TYPE_STRUCT_MEMBER)
				structureDescriptor << "\n\tin a struct";

			if (node->getType() == ResourceDefinition::Node::TYPE_DEFAULT_BLOCK)
				structureDescriptor << "\n\tin the default block";

			if (node->getType() == ResourceDefinition::Node::TYPE_INTERFACE_BLOCK)
				structureDescriptor << "\n\tin an interface block";
		}

		buf	<< "Querying properties of " << glu::getDataTypeName(varDef->m_dataType) << uniformType << " variable.\n"
			<< "Variable is:\n"
			<< "\t" << glu::getDataTypeName(varDef->m_dataType)
			<< structureDescriptor.str();

		return buf.str();
	}
	else if (resource->getType() == ResourceDefinition::Node::TYPE_TRANSFORM_FEEDBACK_TARGET)
	{
		DE_ASSERT(dynamic_cast<const ResourceDefinition::TransformFeedbackTarget*>(resource));

		const ResourceDefinition::TransformFeedbackTarget* xfbDef = static_cast<const ResourceDefinition::TransformFeedbackTarget*>(resource);

		DE_ASSERT(xfbDef->m_builtinVarName);

		return std::string("Querying properties of a builtin variable ") + xfbDef->m_builtinVarName;
	}

	DE_ASSERT(false);
	return DE_NULL;
}

class ResourceNameBufferLimitCase : public TestCase
{
public:
					ResourceNameBufferLimitCase		(Context& context, const char* name, const char* description);
					~ResourceNameBufferLimitCase	(void);

private:
	IterateResult	iterate							(void);
};

ResourceNameBufferLimitCase::ResourceNameBufferLimitCase (Context& context, const char* name, const char* description)
	: TestCase(context, name, description)
{
}

ResourceNameBufferLimitCase::~ResourceNameBufferLimitCase (void)
{
}

ResourceNameBufferLimitCase::IterateResult ResourceNameBufferLimitCase::iterate (void)
{
	static const char* const computeSource =	"${GLSL_VERSION_DECL}\n"
												"layout(local_size_x = 1) in;\n"
												"uniform highp int u_uniformWithALongName;\n"
												"writeonly buffer OutputBufferBlock { highp int b_output_int; };\n"
												"void main ()\n"
												"{\n"
												"	b_output_int = u_uniformWithALongName;\n"
												"}\n";

	const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
	const glu::ShaderProgram	program			(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, computeSource)));
	glw::GLuint					uniformIndex;

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// Log program
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Program", "Program");

		m_testCtx.getLog() << program;
		if (!program.isOk())
			throw tcu::TestError("could not build program");
	}

	uniformIndex = gl.getProgramResourceIndex(program.getProgram(), GL_UNIFORM, "u_uniformWithALongName");
	GLU_EXPECT_NO_ERROR(gl.getError(), "query resource index");

	if (uniformIndex == GL_INVALID_INDEX)
		throw tcu::TestError("Uniform u_uniformWithALongName resource index was GL_INVALID_INDEX");

	// Query with different sized buffers, len("u_uniformWithALongName") == 22

	{
		static const struct
		{
			const char*	description;
			int			querySize;
			bool		returnLength;
		} querySizes[] =
		{
			{ "Query to larger buffer",										24,		true	},
			{ "Query to buffer the same size",								23,		true	},
			{ "Query to one byte too small buffer",							22,		true	},
			{ "Query to one byte buffer",									1,		true	},
			{ "Query to zero sized buffer",									0,		true	},
			{ "Query to one byte too small buffer, null length argument",	22,		false	},
			{ "Query to one byte buffer, null length argument",				1,		false	},
			{ "Query to zero sized buffer, null length argument",			0,		false	},
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(querySizes); ++ndx)
		{
			const tcu::ScopedLogSection			section				(m_testCtx.getLog(), "Query", querySizes[ndx].description);
			const int							uniformNameLen		= 22;
			const int							expectedWriteLen	= (querySizes[ndx].querySize != 0) ? (de::min(uniformNameLen, (querySizes[ndx].querySize - 1))) : (0);
			char								buffer				[26];
			glw::GLsizei						written				= -1;

			// One byte for guard
			DE_ASSERT((int)sizeof(buffer) > querySizes[ndx].querySize);

			deMemset(buffer, 'x', sizeof(buffer));

			if (querySizes[ndx].querySize)
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Querying uniform name to a buffer of size " << querySizes[ndx].querySize
					<< ", expecting query to write " << expectedWriteLen << " bytes followed by a null terminator"
					<< tcu::TestLog::EndMessage;
			else
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Querying uniform name to a buffer of size " << querySizes[ndx].querySize
					<< ", expecting query to write 0 bytes"
					<< tcu::TestLog::EndMessage;

			gl.getProgramResourceName(program.getProgram(), GL_UNIFORM, uniformIndex, querySizes[ndx].querySize, (querySizes[ndx].returnLength) ? (&written) : (DE_NULL), buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "query resource name");

			if (querySizes[ndx].returnLength && written != expectedWriteLen)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, expected write length of " << expectedWriteLen << ", got " << written << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Unexpected write lenght");
			}
			else if (querySizes[ndx].querySize != 0 && buffer[expectedWriteLen] != 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, expected null terminator at " << expectedWriteLen << ", got dec=" << (int)buffer[expectedWriteLen] << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Missing null terminator");
			}
			else if (querySizes[ndx].querySize != 0 && buffer[expectedWriteLen+1] != 'x')
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, guard at index " << (expectedWriteLen+1) << " was modified, got dec=" << (int)buffer[expectedWriteLen+1] << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Wrote over buffer size");
			}
			else if (querySizes[ndx].querySize == 0 && buffer[0] != 'x')
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, buffer size was 0 but buffer contents were modified. At index 0 got dec=" << (int)buffer[0] << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Buffer contents were modified");
			}
		}
	}

	return STOP;
}

class ResourceQueryBufferLimitCase : public TestCase
{
public:
					ResourceQueryBufferLimitCase	(Context& context, const char* name, const char* description);
					~ResourceQueryBufferLimitCase	(void);

private:
	IterateResult	iterate							(void);
};

ResourceQueryBufferLimitCase::ResourceQueryBufferLimitCase (Context& context, const char* name, const char* description)
	: TestCase(context, name, description)
{
}

ResourceQueryBufferLimitCase::~ResourceQueryBufferLimitCase (void)
{
}

ResourceQueryBufferLimitCase::IterateResult ResourceQueryBufferLimitCase::iterate (void)
{
	static const char* const computeSource =	"${GLSL_VERSION_DECL}\n"
												"layout(local_size_x = 1) in;\n"
												"uniform highp int u_uniform;\n"
												"writeonly buffer OutputBufferBlock { highp int b_output_int; };\n"
												"void main ()\n"
												"{\n"
												"	b_output_int = u_uniform;\n"
												"}\n";

	const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
	const glu::ShaderProgram	program			(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, computeSource)));
	glw::GLuint					uniformIndex;

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// Log program
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Program", "Program");

		m_testCtx.getLog() << program;
		if (!program.isOk())
			throw tcu::TestError("could not build program");
	}

	uniformIndex = gl.getProgramResourceIndex(program.getProgram(), GL_UNIFORM, "u_uniform");
	GLU_EXPECT_NO_ERROR(gl.getError(), "query resource index");

	if (uniformIndex == GL_INVALID_INDEX)
		throw tcu::TestError("Uniform u_uniform resource index was GL_INVALID_INDEX");

	// Query uniform properties

	{
		static const struct
		{
			const char*	description;
			int			numProps;
			int			bufferSize;
			bool		returnLength;
		} querySizes[] =
		{
			{ "Query to a larger buffer",							2, 3, true	},
			{ "Query to too small a buffer",						3, 2, true	},
			{ "Query to zero sized buffer",							3, 0, true	},
			{ "Query to a larger buffer, null length argument",		2, 3, false	},
			{ "Query to too small a buffer, null length argument",	3, 2, false	},
			{ "Query to zero sized buffer, null length argument",	3, 0, false	},
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(querySizes); ++ndx)
		{
			const tcu::ScopedLogSection		section				(m_testCtx.getLog(), "QueryToLarger", querySizes[ndx].description);
			const glw::GLenum				props[]				= { GL_LOCATION, GL_LOCATION, GL_LOCATION };
			const int						expectedWriteLen	= de::min(querySizes[ndx].bufferSize, querySizes[ndx].numProps);
			int								params[]			= { 255, 255, 255, 255 };
			glw::GLsizei					written				= -1;

			DE_ASSERT(querySizes[ndx].numProps <= DE_LENGTH_OF_ARRAY(props));
			DE_ASSERT(querySizes[ndx].bufferSize < DE_LENGTH_OF_ARRAY(params)); // leave at least one element for overflow detection

			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "Querying " << querySizes[ndx].numProps << " uniform prop(s) to a buffer with size " << querySizes[ndx].bufferSize << ". Expecting query to return " << expectedWriteLen << " prop(s)"
				<< tcu::TestLog::EndMessage;

			gl.getProgramResourceiv(program.getProgram(), GL_UNIFORM, uniformIndex, querySizes[ndx].numProps, props, querySizes[ndx].bufferSize, (querySizes[ndx].returnLength) ? (&written) : (DE_NULL), params);
			GLU_EXPECT_NO_ERROR(gl.getError(), "query program resources");

			if (querySizes[ndx].returnLength && written != expectedWriteLen)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, expected write length of " << expectedWriteLen << ", got " << written << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Unexpected write lenght");
			}
			else if (params[expectedWriteLen] != 255)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, guard at index " << (expectedWriteLen) << " was modified. Was 255 before call, got dec=" << params[expectedWriteLen] << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Wrote over buffer size");
			}
		}
	}

	return STOP;
}

class InterfaceBlockBaseCase : public TestCase
{
public:
	enum CaseType
	{
		CASE_NAMED_BLOCK = 0,
		CASE_UNNAMED_BLOCK,
		CASE_BLOCK_ARRAY,

		CASE_LAST
	};

											InterfaceBlockBaseCase		(Context& context, const char* name, const char* description, glu::Storage storage, CaseType caseType);
											~InterfaceBlockBaseCase		(void);

private:
	void									init						(void);
	void									deinit						(void);

protected:
	const glu::Storage						m_storage;
	const CaseType							m_caseType;
	ProgramInterfaceDefinition::Program*	m_program;
};

InterfaceBlockBaseCase::InterfaceBlockBaseCase (Context& context, const char* name, const char* description, glu::Storage storage, CaseType caseType)
	: TestCase		(context, name, description)
	, m_storage		(storage)
	, m_caseType	(caseType)
	, m_program		(DE_NULL)
{
	DE_ASSERT(storage == glu::STORAGE_UNIFORM || storage == glu::STORAGE_BUFFER);
}

InterfaceBlockBaseCase::~InterfaceBlockBaseCase (void)
{
	deinit();
}

void InterfaceBlockBaseCase::init (void)
{
	const glu::GLSLVersion				glslVersion	= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
	ProgramInterfaceDefinition::Shader*	shader;

	m_program = new ProgramInterfaceDefinition::Program();
	shader = m_program->addShader(glu::SHADERTYPE_COMPUTE, glslVersion);

	// PrecedingInterface
	{
		glu::InterfaceBlock precedingInterfaceBlock;

		precedingInterfaceBlock.interfaceName	= "PrecedingInterface";
		precedingInterfaceBlock.layout.binding	= 0;
		precedingInterfaceBlock.storage			= m_storage;
		precedingInterfaceBlock.instanceName	= "precedingInstance";

		precedingInterfaceBlock.variables.push_back(glu::VariableDeclaration(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP), "precedingMember"));

		// Unsized array type
		if (m_storage == glu::STORAGE_BUFFER)
			precedingInterfaceBlock.variables.push_back(glu::VariableDeclaration(glu::VarType(glu::VarType(glu::TYPE_FLOAT, glu::PRECISION_HIGHP), glu::VarType::UNSIZED_ARRAY), "precedingMemberUnsizedArray"));
		else
			precedingInterfaceBlock.variables.push_back(glu::VariableDeclaration(glu::VarType(glu::VarType(glu::TYPE_FLOAT, glu::PRECISION_HIGHP), 2), "precedingMemberArray"));

		shader->getDefaultBlock().interfaceBlocks.push_back(precedingInterfaceBlock);
	}

	// TargetInterface
	{
		glu::InterfaceBlock targetInterfaceBlock;

		targetInterfaceBlock.interfaceName	= "TargetInterface";
		targetInterfaceBlock.layout.binding	= 1;
		targetInterfaceBlock.storage		= m_storage;

		if (m_caseType == CASE_UNNAMED_BLOCK)
			targetInterfaceBlock.instanceName = "";
		else
			targetInterfaceBlock.instanceName = "targetInstance";

		if (m_caseType == CASE_BLOCK_ARRAY)
			targetInterfaceBlock.dimensions.push_back(2);

		// Basic type
		{
			targetInterfaceBlock.variables.push_back(glu::VariableDeclaration(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP), "blockMemberBasic"));
		}

		// Array type
		{
			targetInterfaceBlock.variables.push_back(glu::VariableDeclaration(glu::VarType(glu::VarType(glu::TYPE_FLOAT, glu::PRECISION_HIGHP), 3), "blockMemberArray"));
		}

		// Struct type
		{
			glu::StructType* structPtr = new glu::StructType("StructType");
			structPtr->addMember("structMemberBasic", glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP));
			structPtr->addMember("structMemberArray", glu::VarType(glu::VarType(glu::TYPE_FLOAT, glu::PRECISION_HIGHP), 2));

			targetInterfaceBlock.variables.push_back(glu::VariableDeclaration(glu::VarType(glu::VarType(structPtr), 2), "blockMemberStruct"));
		}

		// Unsized array type
		if (m_storage == glu::STORAGE_BUFFER)
			targetInterfaceBlock.variables.push_back(glu::VariableDeclaration(glu::VarType(glu::VarType(glu::TYPE_FLOAT, glu::PRECISION_HIGHP), glu::VarType::UNSIZED_ARRAY), "blockMemberUnsizedArray"));

		shader->getDefaultBlock().interfaceBlocks.push_back(targetInterfaceBlock);
	}

	// TrailingInterface
	{
		glu::InterfaceBlock trailingInterfaceBlock;

		trailingInterfaceBlock.interfaceName	= "TrailingInterface";
		trailingInterfaceBlock.layout.binding	= 3;
		trailingInterfaceBlock.storage			= m_storage;
		trailingInterfaceBlock.instanceName		= "trailingInstance";
		trailingInterfaceBlock.variables.push_back(glu::VariableDeclaration(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP), "trailingMember"));

		shader->getDefaultBlock().interfaceBlocks.push_back(trailingInterfaceBlock);
	}

	DE_ASSERT(m_program->isValid());
}

void InterfaceBlockBaseCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;
}

class InterfaceBlockActiveVariablesTestCase : public InterfaceBlockBaseCase
{
public:
											InterfaceBlockActiveVariablesTestCase	(Context& context, const char* name, const char* description, glu::Storage storage, CaseType caseType);

private:
	IterateResult							iterate									(void);
};

InterfaceBlockActiveVariablesTestCase::InterfaceBlockActiveVariablesTestCase (Context& context, const char* name, const char* description, glu::Storage storage, CaseType caseType)
	: InterfaceBlockBaseCase(context, name, description, storage, caseType)
{
}

InterfaceBlockActiveVariablesTestCase::IterateResult InterfaceBlockActiveVariablesTestCase::iterate (void)
{
	const ProgramInterface			programInterface				= (m_storage == glu::STORAGE_UNIFORM) ? (PROGRAMINTERFACE_UNIFORM_BLOCK) :
																	  (m_storage == glu::STORAGE_BUFFER) ? (PROGRAMINTERFACE_SHADER_STORAGE_BLOCK) :
																	  (PROGRAMINTERFACE_LAST);
	const glw::GLenum				programGLInterfaceValue			= getProgramInterfaceGLEnum(programInterface);
	const glw::GLenum				programMemberInterfaceValue		= (m_storage == glu::STORAGE_UNIFORM) ? (GL_UNIFORM) :
																	  (m_storage == glu::STORAGE_BUFFER) ? (GL_BUFFER_VARIABLE) :
																	  (0);
	const std::vector<std::string>	blockNames						= getProgramInterfaceResourceList(m_program, programInterface);
	glu::ShaderProgram				program							(m_context.getRenderContext(), generateProgramInterfaceProgramSources(m_program));
	int								expectedMaxNumActiveVariables	= 0;

	DE_ASSERT(programInterface != PROGRAMINTERFACE_LAST);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	checkAndLogProgram(program, m_program, m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	// Verify all blocks

	for (int blockNdx = 0; blockNdx < (int)blockNames.size(); ++blockNdx)
	{
		const tcu::ScopedLogSection section				(m_testCtx.getLog(), "Block", "Block \"" + blockNames[blockNdx] + "\"");
		const glw::Functions&		gl					= m_context.getRenderContext().getFunctions();
		const glw::GLuint			resourceNdx			= gl.getProgramResourceIndex(program.getProgram(), programGLInterfaceValue, blockNames[blockNdx].c_str());
		glw::GLint					numActiveResources;
		std::vector<std::string>	activeResourceNames;

		GLU_EXPECT_NO_ERROR(gl.getError(), "query resource index");

		if (resourceNdx == GL_INVALID_INDEX)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, getProgramResourceIndex returned GL_INVALID_INDEX for \"" << blockNames[blockNdx] << "\"" << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Resource not found");
			continue;
		}

		// query block information

		{
			const glw::GLenum	props[]			= { GL_NUM_ACTIVE_VARIABLES };
			glw::GLint			retBuffer[2]	= { -1, -1 };
			glw::GLint			written			= -1;

			gl.getProgramResourceiv(program.getProgram(), programGLInterfaceValue, resourceNdx, DE_LENGTH_OF_ARRAY(props), props, 1, &written, retBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "query GL_NUM_ACTIVE_VARIABLES");

			numActiveResources = retBuffer[0];
			expectedMaxNumActiveVariables = de::max(expectedMaxNumActiveVariables, numActiveResources);
			m_testCtx.getLog() << tcu::TestLog::Message << "NUM_ACTIVE_VARIABLES = " << numActiveResources << tcu::TestLog::EndMessage;

			if (written == -1 || retBuffer[0] == -1)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, Query for NUM_ACTIVE_VARIABLES did not return a value" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Query for NUM_ACTIVE_VARIABLES failed");
				continue;
			}
			else if (retBuffer[1] != -1)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, Query for NUM_ACTIVE_VARIABLES returned too many values" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Query for NUM_ACTIVE_VARIABLES returned too many values");
				continue;
			}
			else if (retBuffer[0] < 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, NUM_ACTIVE_VARIABLES < 0" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "NUM_ACTIVE_VARIABLES < 0");
				continue;
			}
		}

		// query block variable information

		{
			const glw::GLenum			props[]					= { GL_ACTIVE_VARIABLES };
			std::vector<glw::GLint>		activeVariableIndices	(numActiveResources + 1, -1);	// Allocate one extra trailing to detect wrong write lengths
			glw::GLint					written					= -1;

			gl.getProgramResourceiv(program.getProgram(), programGLInterfaceValue, resourceNdx, DE_LENGTH_OF_ARRAY(props), props, (glw::GLsizei)activeVariableIndices.size(), &written, &activeVariableIndices[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "query GL_ACTIVE_VARIABLES");

			if (written == -1)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, Query for GL_ACTIVE_VARIABLES did not return any values" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Query for GL_ACTIVE_VARIABLES failed");
				continue;
			}
			else if (written != numActiveResources)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, Query for GL_ACTIVE_VARIABLES did not return NUM_ACTIVE_VARIABLES values" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Query for GL_ACTIVE_VARIABLES returned invalid number of values");
				continue;
			}
			else if (activeVariableIndices.back() != -1)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, GL_ACTIVE_VARIABLES query return buffer trailing guard value was modified, getProgramResourceiv returned more than NUM_ACTIVE_VARIABLES values" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Query for GL_ACTIVE_VARIABLES returned too many values");
				continue;
			}

			// log indices
			{
				tcu::MessageBuilder builder(&m_testCtx.getLog());

				builder << "Active variable indices: {";
				for (int varNdx = 0; varNdx < numActiveResources; ++varNdx)
				{
					if (varNdx)
						builder << ", ";
					builder << activeVariableIndices[varNdx];
				}
				builder << "}" << tcu::TestLog::EndMessage;
			}

			// collect names

			activeResourceNames.resize(numActiveResources);

			for (int varNdx = 0; varNdx < numActiveResources; ++varNdx)
			{
				const glw::GLenum	nameProp	= GL_NAME_LENGTH;
				glw::GLint			nameLength	= -1;
				std::vector<char>	nameBuffer;

				written = -1;
				gl.getProgramResourceiv(program.getProgram(), programMemberInterfaceValue, activeVariableIndices[varNdx], 1, &nameProp, 1, &written, &nameLength);
				GLU_EXPECT_NO_ERROR(gl.getError(), "query GL_NAME_LENGTH");

				if (nameLength <= 0 || written <= 0)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Error, GL_NAME_LENGTH query failed" << tcu::TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "GL_NAME_LENGTH query failed");
					continue;
				}

				nameBuffer.resize(nameLength + 2, 'X'); // allocate more than required
				written = -1;
				gl.getProgramResourceName(program.getProgram(), programMemberInterfaceValue, activeVariableIndices[varNdx], nameLength+1, &written, &nameBuffer[0]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramResourceName");

				if (written <= 0)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Error, name query failed, no data written" << tcu::TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "name query failed");
					continue;
				}
				else if (written > nameLength)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Error, name query failed, query returned too much data" << tcu::TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "name query failed");
					continue;
				}

				activeResourceNames[varNdx] = std::string(&nameBuffer[0], written);
			}

			// log collected names
			{
				tcu::MessageBuilder builder(&m_testCtx.getLog());

				builder << "Active variables:\n";
				for (int varNdx = 0; varNdx < numActiveResources; ++varNdx)
					builder << "\t" << activeResourceNames[varNdx] << "\n";
				builder << tcu::TestLog::EndMessage;
			}
		}

		// verify names
		{
			glu::InterfaceBlock*		block		= DE_NULL;
			const std::string			blockName	= glu::parseVariableName(blockNames[blockNdx].c_str());
			std::vector<std::string>	referenceList;

			for (int interfaceNdx = 0; interfaceNdx < (int)m_program->getShaders()[0]->getDefaultBlock().interfaceBlocks.size(); ++interfaceNdx)
			{
				if (m_program->getShaders()[0]->getDefaultBlock().interfaceBlocks[interfaceNdx].interfaceName == blockName)
				{
					block = &m_program->getShaders()[0]->getDefaultBlock().interfaceBlocks[interfaceNdx];
					break;
				}
			}

			if (!block)
				throw tcu::InternalError("could not find block referenced in the reference resource list");

			// generate reference list

			referenceList = getProgramInterfaceBlockMemberResourceList(*block);
			{
				tcu::MessageBuilder builder(&m_testCtx.getLog());

				builder << "Expected variable names:\n";
				for (int varNdx = 0; varNdx < (int)referenceList.size(); ++varNdx)
					builder << "\t" << referenceList[varNdx] << "\n";
				builder << tcu::TestLog::EndMessage;
			}

			// compare lists
			{
				bool listsIdentical = true;

				for (int ndx = 0; ndx < (int)referenceList.size(); ++ndx)
				{
					if (!de::contains(activeResourceNames.begin(), activeResourceNames.end(), referenceList[ndx]))
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Error, variable name list did not contain active variable " << referenceList[ndx] << tcu::TestLog::EndMessage;
						listsIdentical = false;
					}
				}

				for (int ndx = 0; ndx < (int)activeResourceNames.size(); ++ndx)
				{
					if (!de::contains(referenceList.begin(), referenceList.end(), activeResourceNames[ndx]))
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Error, variable name list contains unexpected resource \"" << activeResourceNames[ndx] << "\"" << tcu::TestLog::EndMessage;
						listsIdentical = false;
					}
				}

				if (listsIdentical)
					m_testCtx.getLog() << tcu::TestLog::Message << "Lists identical" << tcu::TestLog::EndMessage;
				else
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Error, invalid active variable list" << tcu::TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "invalid active variable list");
					continue;
				}
			}
		}
	}

	// Max num active variables
	{
		const tcu::ScopedLogSection	section					(m_testCtx.getLog(), "MaxNumActiveVariables", "MAX_NUM_ACTIVE_VARIABLES");
		const glw::Functions&		gl						= m_context.getRenderContext().getFunctions();
		glw::GLint					maxNumActiveVariables	= -1;

		gl.getProgramInterfaceiv(program.getProgram(), programGLInterfaceValue, GL_MAX_NUM_ACTIVE_VARIABLES, &maxNumActiveVariables);
		GLU_EXPECT_NO_ERROR(gl.getError(), "query MAX_NUM_ACTIVE_VARIABLES");

		m_testCtx.getLog() << tcu::TestLog::Message << "MAX_NUM_ACTIVE_VARIABLES = " << maxNumActiveVariables << tcu::TestLog::EndMessage;

		if (expectedMaxNumActiveVariables != maxNumActiveVariables)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, got unexpected MAX_NUM_ACTIVE_VARIABLES" << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "unexpected MAX_NUM_ACTIVE_VARIABLES");
		}
		else
			m_testCtx.getLog() << tcu::TestLog::Message << "MAX_NUM_ACTIVE_VARIABLES valid" << tcu::TestLog::EndMessage;
	}

	return STOP;
}

class InterfaceBlockDataSizeTestCase : public InterfaceBlockBaseCase
{
public:
											InterfaceBlockDataSizeTestCase	(Context& context, const char* name, const char* description, glu::Storage storage, CaseType caseType);

private:
	IterateResult							iterate							(void);
	int										getBlockMinDataSize				(const std::string& blockName) const;
	int										getBlockMinDataSize				(const glu::InterfaceBlock& block) const;
};

InterfaceBlockDataSizeTestCase::InterfaceBlockDataSizeTestCase (Context& context, const char* name, const char* description, glu::Storage storage, CaseType caseType)
	: InterfaceBlockBaseCase(context, name, description, storage, caseType)
{
}

InterfaceBlockDataSizeTestCase::IterateResult InterfaceBlockDataSizeTestCase::iterate (void)
{
	const ProgramInterface			programInterface		= (m_storage == glu::STORAGE_UNIFORM) ? (PROGRAMINTERFACE_UNIFORM_BLOCK) :
															  (m_storage == glu::STORAGE_BUFFER) ? (PROGRAMINTERFACE_SHADER_STORAGE_BLOCK) :
															  (PROGRAMINTERFACE_LAST);
	const glw::GLenum				programGLInterfaceValue	= getProgramInterfaceGLEnum(programInterface);
	const std::vector<std::string>	blockNames				= getProgramInterfaceResourceList(m_program, programInterface);
	glu::ShaderProgram				program					(m_context.getRenderContext(), generateProgramInterfaceProgramSources(m_program));

	DE_ASSERT(programInterface != PROGRAMINTERFACE_LAST);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	checkAndLogProgram(program, m_program, m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	// Verify all blocks
	for (int blockNdx = 0; blockNdx < (int)blockNames.size(); ++blockNdx)
	{
		const tcu::ScopedLogSection section				(m_testCtx.getLog(), "Block", "Block \"" + blockNames[blockNdx] + "\"");
		const glw::Functions&		gl					= m_context.getRenderContext().getFunctions();
		const glw::GLuint			resourceNdx			= gl.getProgramResourceIndex(program.getProgram(), programGLInterfaceValue, blockNames[blockNdx].c_str());
		const int					expectedMinDataSize	= getBlockMinDataSize(blockNames[blockNdx]);
		glw::GLint					queryDataSize		= -1;

		GLU_EXPECT_NO_ERROR(gl.getError(), "query resource index");

		if (resourceNdx == GL_INVALID_INDEX)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, getProgramResourceIndex returned GL_INVALID_INDEX for \"" << blockNames[blockNdx] << "\"" << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Resource not found");
			continue;
		}

		// query
		{
			const glw::GLenum prop = GL_BUFFER_DATA_SIZE;

			gl.getProgramResourceiv(program.getProgram(), programGLInterfaceValue, resourceNdx, 1, &prop, 1, DE_NULL, &queryDataSize);
			GLU_EXPECT_NO_ERROR(gl.getError(), "query resource BUFFER_DATA_SIZE");
		}

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "BUFFER_DATA_SIZE = " << queryDataSize << "\n"
			<< "Buffer data size with tight packing: " << expectedMinDataSize
			<< tcu::TestLog::EndMessage;

		if (queryDataSize < expectedMinDataSize)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, buffer size was less than minimum buffer data size" << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Buffer data size invalid");
			continue;
		}
		else
			m_testCtx.getLog() << tcu::TestLog::Message << "Buffer size valid" << tcu::TestLog::EndMessage;
	}

	return STOP;
}

int InterfaceBlockDataSizeTestCase::getBlockMinDataSize (const std::string& blockFullName) const
{
	const std::string blockName = glu::parseVariableName(blockFullName.c_str());

	for (int interfaceNdx = 0; interfaceNdx < (int)m_program->getShaders()[0]->getDefaultBlock().interfaceBlocks.size(); ++interfaceNdx)
	{
		if (m_program->getShaders()[0]->getDefaultBlock().interfaceBlocks[interfaceNdx].interfaceName == blockName &&
			m_program->getShaders()[0]->getDefaultBlock().interfaceBlocks[interfaceNdx].storage == m_storage)
			return getBlockMinDataSize(m_program->getShaders()[0]->getDefaultBlock().interfaceBlocks[interfaceNdx]);
	}

	DE_ASSERT(false);
	return -1;
}

class AtomicCounterCase : public TestCase
{
public:
											AtomicCounterCase			(Context& context, const char* name, const char* description);
											~AtomicCounterCase			(void);

private:
	void									init						(void);
	void									deinit						(void);

protected:
	int										getNumAtomicCounterBuffers	(void) const;
	int										getMaxNumActiveVariables	(void) const;
	int										getBufferVariableCount		(int binding) const;
	int										getBufferMinimumDataSize	(int binding) const;

	ProgramInterfaceDefinition::Program*	m_program;
};

AtomicCounterCase::AtomicCounterCase (Context& context, const char* name, const char* description)
	: TestCase	(context, name, description)
	, m_program	(DE_NULL)
{
}

AtomicCounterCase::~AtomicCounterCase (void)
{
	deinit();
}

void AtomicCounterCase::init (void)
{
	ProgramInterfaceDefinition::Shader* shader;
	glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());

	m_program = new ProgramInterfaceDefinition::Program();
	shader = m_program->addShader(glu::SHADERTYPE_COMPUTE, glslVersion);

	{
		glu::VariableDeclaration decl(glu::VarType(glu::TYPE_UINT_ATOMIC_COUNTER, glu::PRECISION_LAST), "binding1_counter1", glu::STORAGE_UNIFORM);
		decl.layout.binding = 1;
		shader->getDefaultBlock().variables.push_back(decl);
	}
	{
		glu::VariableDeclaration decl(glu::VarType(glu::TYPE_UINT_ATOMIC_COUNTER, glu::PRECISION_LAST), "binding1_counter2", glu::STORAGE_UNIFORM);
		decl.layout.binding = 1;
		decl.layout.offset = 8;

		shader->getDefaultBlock().variables.push_back(decl);
	}
	{
		glu::VariableDeclaration decl(glu::VarType(glu::TYPE_UINT_ATOMIC_COUNTER, glu::PRECISION_LAST), "binding2_counter1", glu::STORAGE_UNIFORM);
		decl.layout.binding = 2;
		shader->getDefaultBlock().variables.push_back(decl);
	}

	DE_ASSERT(m_program->isValid());
}

void AtomicCounterCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;
}

int AtomicCounterCase::getNumAtomicCounterBuffers (void) const
{
	std::set<int> buffers;

	for (int ndx = 0; ndx < (int)m_program->getShaders()[0]->getDefaultBlock().variables.size(); ++ndx)
	{
		if (m_program->getShaders()[0]->getDefaultBlock().variables[ndx].varType.isBasicType() &&
			glu::isDataTypeAtomicCounter(m_program->getShaders()[0]->getDefaultBlock().variables[ndx].varType.getBasicType()))
		{
			buffers.insert(m_program->getShaders()[0]->getDefaultBlock().variables[ndx].layout.binding);
		}
	}

	return (int)buffers.size();
}

int AtomicCounterCase::getMaxNumActiveVariables (void) const
{
	int					maxVars			= 0;
	std::map<int,int>	numBufferVars;

	for (int ndx = 0; ndx < (int)m_program->getShaders()[0]->getDefaultBlock().variables.size(); ++ndx)
	{
		if (m_program->getShaders()[0]->getDefaultBlock().variables[ndx].varType.isBasicType() &&
			glu::isDataTypeAtomicCounter(m_program->getShaders()[0]->getDefaultBlock().variables[ndx].varType.getBasicType()))
		{
			const int binding = m_program->getShaders()[0]->getDefaultBlock().variables[ndx].layout.binding;

			if (numBufferVars.find(binding) == numBufferVars.end())
				numBufferVars[binding] = 1;
			else
				++numBufferVars[binding];
		}
	}

	for (std::map<int,int>::const_iterator it = numBufferVars.begin(); it != numBufferVars.end(); ++it)
		maxVars = de::max(maxVars, it->second);

	return maxVars;
}

int AtomicCounterCase::getBufferVariableCount (int binding) const
{
	int numVars = 0;

	for (int ndx = 0; ndx < (int)m_program->getShaders()[0]->getDefaultBlock().variables.size(); ++ndx)
	{
		if (m_program->getShaders()[0]->getDefaultBlock().variables[ndx].varType.isBasicType() &&
			glu::isDataTypeAtomicCounter(m_program->getShaders()[0]->getDefaultBlock().variables[ndx].varType.getBasicType()) &&
			m_program->getShaders()[0]->getDefaultBlock().variables[ndx].layout.binding == binding)
			++numVars;
	}

	return numVars;
}

int AtomicCounterCase::getBufferMinimumDataSize (int binding) const
{
	int minSize			= -1;
	int currentOffset	= 0;

	for (int ndx = 0; ndx < (int)m_program->getShaders()[0]->getDefaultBlock().variables.size(); ++ndx)
	{
		if (m_program->getShaders()[0]->getDefaultBlock().variables[ndx].varType.isBasicType() &&
			glu::isDataTypeAtomicCounter(m_program->getShaders()[0]->getDefaultBlock().variables[ndx].varType.getBasicType()) &&
			m_program->getShaders()[0]->getDefaultBlock().variables[ndx].layout.binding == binding)
		{
			const int thisOffset = (m_program->getShaders()[0]->getDefaultBlock().variables[ndx].layout.offset != -1) ? (m_program->getShaders()[0]->getDefaultBlock().variables[ndx].layout.offset) : (currentOffset);
			currentOffset = thisOffset + 4;

			minSize = de::max(minSize, thisOffset + 4);
		}
	}

	return minSize;
}

class AtomicCounterResourceListCase : public AtomicCounterCase
{
public:
						AtomicCounterResourceListCase	(Context& context, const char* name, const char* description);

private:
	IterateResult		iterate							(void);
};

AtomicCounterResourceListCase::AtomicCounterResourceListCase (Context& context, const char* name, const char* description)
	: AtomicCounterCase(context, name, description)
{
}

AtomicCounterResourceListCase::IterateResult AtomicCounterResourceListCase::iterate (void)
{
	const glu::ShaderProgram program(m_context.getRenderContext(), generateProgramInterfaceProgramSources(m_program));

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	checkAndLogProgram(program, m_program, m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	{
		const tcu::ScopedLogSection		section						(m_testCtx.getLog(), "ActiveResources", "ACTIVE_RESOURCES");
		const glw::Functions&			gl							= m_context.getRenderContext().getFunctions();
		glw::GLint						numActiveResources			= -1;
		const int						numExpectedActiveResources	= 2; // 2 buffer bindings

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying ACTIVE_RESOURCES, expecting " << numExpectedActiveResources << tcu::TestLog::EndMessage;

		gl.getProgramInterfaceiv(program.getProgram(), GL_ATOMIC_COUNTER_BUFFER, GL_ACTIVE_RESOURCES, &numActiveResources);
		GLU_EXPECT_NO_ERROR(gl.getError(), "query GL_ACTIVE_RESOURCES");

		m_testCtx.getLog() << tcu::TestLog::Message << "ACTIVE_RESOURCES = " << numActiveResources << tcu::TestLog::EndMessage;

		if (numActiveResources != numExpectedActiveResources)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, got unexpected ACTIVE_RESOURCES" << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected ACTIVE_RESOURCES");
		}
		else
			m_testCtx.getLog() << tcu::TestLog::Message << "ACTIVE_RESOURCES valid" << tcu::TestLog::EndMessage;
	}

	return STOP;
}

class AtomicCounterActiveVariablesCase : public AtomicCounterCase
{
public:
					AtomicCounterActiveVariablesCase	(Context& context, const char* name, const char* description);

private:
	IterateResult	iterate								(void);
};

AtomicCounterActiveVariablesCase::AtomicCounterActiveVariablesCase (Context& context, const char* name, const char* description)
	: AtomicCounterCase(context, name, description)
{
}

AtomicCounterActiveVariablesCase::IterateResult AtomicCounterActiveVariablesCase::iterate (void)
{
	const glw::Functions&		gl								= m_context.getRenderContext().getFunctions();
	const glu::ShaderProgram	program							(m_context.getRenderContext(), generateProgramInterfaceProgramSources(m_program));
	const int					numAtomicBuffers				= getNumAtomicCounterBuffers();
	const int					expectedMaxNumActiveVariables	= getMaxNumActiveVariables();

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	checkAndLogProgram(program, m_program, m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	// check active variables
	{
		const tcu::ScopedLogSection	section						(m_testCtx.getLog(), "Interface", "ATOMIC_COUNTER_BUFFER interface");
		glw::GLint					queryActiveResources		= -1;
		glw::GLint					queryMaxNumActiveVariables	= -1;

		gl.getProgramInterfaceiv(program.getProgram(), GL_ATOMIC_COUNTER_BUFFER, GL_ACTIVE_RESOURCES, &queryActiveResources);
		gl.getProgramInterfaceiv(program.getProgram(), GL_ATOMIC_COUNTER_BUFFER, GL_MAX_NUM_ACTIVE_VARIABLES, &queryMaxNumActiveVariables);
		GLU_EXPECT_NO_ERROR(gl.getError(), "query interface");

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "GL_ACTIVE_RESOURCES = " << queryActiveResources << "\n"
			<< "GL_MAX_NUM_ACTIVE_VARIABLES = " << queryMaxNumActiveVariables << "\n"
			<< tcu::TestLog::EndMessage;

		if (queryActiveResources != numAtomicBuffers)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, got unexpected GL_ACTIVE_RESOURCES, expected " << numAtomicBuffers << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected GL_ACTIVE_RESOURCES");
		}

		if (queryMaxNumActiveVariables != expectedMaxNumActiveVariables)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, got unexpected GL_MAX_NUM_ACTIVE_VARIABLES, expected " << expectedMaxNumActiveVariables << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected GL_MAX_NUM_ACTIVE_VARIABLES");
		}
	}

	// Check each buffer
	for (int bufferNdx = 0; bufferNdx < numAtomicBuffers; ++bufferNdx)
	{
		const tcu::ScopedLogSection	section				(m_testCtx.getLog(), "Resource", "Resource index " + de::toString(bufferNdx));
		std::vector<glw::GLint>		activeVariables;
		std::vector<std::string>	memberNames;

		// Find active variables
		{
			const glw::GLenum	numActiveVariablesProp	= GL_NUM_ACTIVE_VARIABLES;
			const glw::GLenum	activeVariablesProp		= GL_ACTIVE_VARIABLES;
			glw::GLint			numActiveVariables		= -2;
			glw::GLint			written					= -1;

			gl.getProgramResourceiv(program.getProgram(), GL_ATOMIC_COUNTER_BUFFER, bufferNdx, 1, &numActiveVariablesProp, 1, &written, &numActiveVariables);
			GLU_EXPECT_NO_ERROR(gl.getError(), "query num active variables");

			if (numActiveVariables <= 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, got unexpected NUM_ACTIVE_VARIABLES: " << numActiveVariables  << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected NUM_ACTIVE_VARIABLES");
				continue;
			}

			if (written <= 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, query for NUM_ACTIVE_VARIABLES returned no values" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "NUM_ACTIVE_VARIABLES query failed");
				continue;
			}

			m_testCtx.getLog() << tcu::TestLog::Message << "GL_NUM_ACTIVE_VARIABLES = " << numActiveVariables << tcu::TestLog::EndMessage;

			written = -1;
			activeVariables.resize(numActiveVariables + 1, -2);

			gl.getProgramResourceiv(program.getProgram(), GL_ATOMIC_COUNTER_BUFFER, bufferNdx, 1, &activeVariablesProp, numActiveVariables, &written, &activeVariables[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "query active variables");

			if (written != numActiveVariables)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, unexpected number of ACTIVE_VARIABLES, NUM_ACTIVE_VARIABLES = " << numActiveVariables << ", query returned " << written << " values" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected ACTIVE_VARIABLES");
				continue;
			}

			if (activeVariables.back() != -2)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, query for ACTIVE_VARIABLES wrote over target buffer bounds" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "ACTIVE_VARIABLES query failed");
				continue;
			}

			activeVariables.pop_back();
		}

		// log indices
		{
			tcu::MessageBuilder builder(&m_testCtx.getLog());

			builder << "Active variable indices: {";
			for (int varNdx = 0; varNdx < (int)activeVariables.size(); ++varNdx)
			{
				if (varNdx)
					builder << ", ";
				builder << activeVariables[varNdx];
			}
			builder << "}" << tcu::TestLog::EndMessage;
		}

		// collect member names
		for (int ndx = 0; ndx < (int)activeVariables.size(); ++ndx)
		{
			const glw::GLenum	nameLengthProp	= GL_NAME_LENGTH;
			glw::GLint			nameLength		= -1;
			glw::GLint			written			= -1;
			std::vector<char>	nameBuf;

			gl.getProgramResourceiv(program.getProgram(), GL_UNIFORM, activeVariables[ndx], 1, &nameLengthProp, 1, &written, &nameLength);
			GLU_EXPECT_NO_ERROR(gl.getError(), "query buffer variable name length");

			if (written <= 0 || nameLength == -1)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, query for GL_NAME_LENGTH returned no values" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "GL_NAME_LENGTH query failed");
				continue;
			}

			nameBuf.resize(nameLength + 2, 'X'); // +2 to tolerate potential off-by-ones in some implementations, name queries will check these cases better
			written = -1;

			gl.getProgramResourceName(program.getProgram(), GL_UNIFORM, activeVariables[ndx], (int)nameBuf.size(), &written, &nameBuf[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "query buffer variable name");

			if (written <= 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, query for resource name returned no name" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Name query failed");
				continue;
			}

			memberNames.push_back(std::string(&nameBuf[0], written));
		}

		// log names
		{
			tcu::MessageBuilder builder(&m_testCtx.getLog());

			builder << "Active variables:\n";
			for (int varNdx = 0; varNdx < (int)memberNames.size(); ++varNdx)
			{
				builder << "\t" << memberNames[varNdx] << "\n";
			}
			builder << tcu::TestLog::EndMessage;
		}

		// check names are all in the same buffer
		{
			bool bindingsValid = true;

			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying names" << tcu::TestLog::EndMessage;

			for (int nameNdx = 0; nameNdx < (int)memberNames.size(); ++nameNdx)
			{
				int prevBinding = -1;

				for (int varNdx = 0; varNdx < (int)m_program->getShaders()[0]->getDefaultBlock().variables.size(); ++varNdx)
				{
					if (m_program->getShaders()[0]->getDefaultBlock().variables[varNdx].name == memberNames[nameNdx])
					{
						const int varBinding = m_program->getShaders()[0]->getDefaultBlock().variables[varNdx].layout.binding;

						if (prevBinding == -1 || prevBinding == varBinding)
							prevBinding = varBinding;
						else
							bindingsValid = false;
					}
				}

				if (prevBinding == -1)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Error, could not find variable with name \"" << memberNames[nameNdx] << "\"" << tcu::TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Variable name invalid");
				}
				else if (getBufferVariableCount(prevBinding) != (int)memberNames.size())
				{
					m_testCtx.getLog()
						<< tcu::TestLog::Message
						<< "Error, unexpected variable count for binding " << prevBinding
						<< ". Expected " << getBufferVariableCount(prevBinding) << ", got " << (int)memberNames.size()
						<< tcu::TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Variable names invalid");
				}
			}

			if (!bindingsValid)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, all resource do not share the same buffer" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Active variables invalid");
				continue;
			}
		}
	}

	return STOP;
}

class AtomicCounterBufferBindingCase : public AtomicCounterCase
{
public:
					AtomicCounterBufferBindingCase		(Context& context, const char* name, const char* description);

private:
	IterateResult	iterate								(void);
};

AtomicCounterBufferBindingCase::AtomicCounterBufferBindingCase (Context& context, const char* name, const char* description)
	: AtomicCounterCase(context, name, description)
{
}

AtomicCounterBufferBindingCase::IterateResult AtomicCounterBufferBindingCase::iterate (void)
{
	const glw::Functions&		gl								= m_context.getRenderContext().getFunctions();
	const glu::ShaderProgram	program							(m_context.getRenderContext(), generateProgramInterfaceProgramSources(m_program));
	const int					numAtomicBuffers				= getNumAtomicCounterBuffers();

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	checkAndLogProgram(program, m_program, m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	// check every buffer
	for (int bufferNdx = 0; bufferNdx < numAtomicBuffers; ++bufferNdx)
	{
		const tcu::ScopedLogSection	section				(m_testCtx.getLog(), "Resource", "Resource index " + de::toString(bufferNdx));
		const glw::GLenum			bufferBindingProp	= GL_BUFFER_BINDING;
		glw::GLint					bufferBinding		= -1;
		glw::GLint					written				= -1;

		gl.getProgramResourceiv(program.getProgram(), GL_ATOMIC_COUNTER_BUFFER, bufferNdx, 1, &bufferBindingProp, 1, &written, &bufferBinding);
		GLU_EXPECT_NO_ERROR(gl.getError(), "query buffer binding");

		if (written <= 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, query for BUFFER_BINDING returned no values." << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "BUFFER_BINDING query failed");
		}

		m_testCtx.getLog() << tcu::TestLog::Message << "GL_BUFFER_BINDING = " << bufferBinding << tcu::TestLog::EndMessage;

		// no such buffer binding?
		if (getBufferVariableCount(bufferBinding) == 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, got buffer with BUFFER_BINDING = " << bufferBinding << ", but such buffer does not exist." << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected BUFFER_BINDING");
		}
	}

	return STOP;
}

class AtomicCounterBufferDataSizeCase : public AtomicCounterCase
{
public:
					AtomicCounterBufferDataSizeCase		(Context& context, const char* name, const char* description);

private:
	IterateResult	iterate								(void);
};

AtomicCounterBufferDataSizeCase::AtomicCounterBufferDataSizeCase (Context& context, const char* name, const char* description)
	: AtomicCounterCase(context, name, description)
{
}

AtomicCounterBufferDataSizeCase::IterateResult AtomicCounterBufferDataSizeCase::iterate (void)
{
	const glw::Functions&		gl								= m_context.getRenderContext().getFunctions();
	const glu::ShaderProgram	program							(m_context.getRenderContext(), generateProgramInterfaceProgramSources(m_program));
	const int					numAtomicBuffers				= getNumAtomicCounterBuffers();

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	checkAndLogProgram(program, m_program, m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	// check every buffer
	for (int bufferNdx = 0; bufferNdx < numAtomicBuffers; ++bufferNdx)
	{
		const tcu::ScopedLogSection	section				(m_testCtx.getLog(), "Resource", "Resource index " + de::toString(bufferNdx));
		const glw::GLenum			props[]				= { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE };
		glw::GLint					values[]			= { -1, -1 };
		glw::GLint					written				= -1;
		int							bufferMinDataSize;

		gl.getProgramResourceiv(program.getProgram(), GL_ATOMIC_COUNTER_BUFFER, bufferNdx, DE_LENGTH_OF_ARRAY(props), props, DE_LENGTH_OF_ARRAY(values), &written, values);
		GLU_EXPECT_NO_ERROR(gl.getError(), "query buffer binding");

		if (written != 2)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, query for (BUFFER_BINDING, BUFFER_DATA_SIZE) returned " << written << " value(s)." << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "property query failed");
			continue;
		}

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "GL_BUFFER_BINDING = " << values[0] << "\n"
			<< "GL_BUFFER_DATA_SIZE = " << values[1]
			<< tcu::TestLog::EndMessage;

		bufferMinDataSize = getBufferMinimumDataSize(values[0]);
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying data size, expected greater than or equal to " << bufferMinDataSize << tcu::TestLog::EndMessage;

		// no such buffer binding?
		if (bufferMinDataSize == -1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, got buffer with BUFFER_BINDING = " << values[0] << ", but such buffer does not exist." << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected BUFFER_BINDING");
		}
		else if (values[1] < bufferMinDataSize)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, got buffer with BUFFER_DATA_SIZE = " << values[1] << ", expected greater than or equal to " << bufferMinDataSize << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected BUFFER_BINDING");
		}
		else
			m_testCtx.getLog() << tcu::TestLog::Message << "Data size valid" << tcu::TestLog::EndMessage;
	}

	return STOP;
}

class AtomicCounterReferencedByCase : public TestCase
{
public:
											AtomicCounterReferencedByCase	(Context&		context,
																			 const char*	name,
																			 const char*	description,
																			 bool			separable,
																			 deUint32		presentStagesMask,
																			 deUint32		activeStagesMask);
											~AtomicCounterReferencedByCase	(void);

private:
	void									init							(void);
	void									deinit							(void);
	IterateResult							iterate							(void);

	const bool								m_separable;
	const deUint32							m_presentStagesMask;
	const deUint32							m_activeStagesMask;
	ProgramInterfaceDefinition::Program*	m_program;
};

AtomicCounterReferencedByCase::AtomicCounterReferencedByCase (Context&		context,
															  const char*	name,
															  const char*	description,
															  bool			separable,
															  deUint32		presentStagesMask,
															  deUint32		activeStagesMask)
	: TestCase				(context, name, description)
	, m_separable			(separable)
	, m_presentStagesMask	(presentStagesMask)
	, m_activeStagesMask	(activeStagesMask)
	, m_program				(DE_NULL)
{
	DE_ASSERT((activeStagesMask & presentStagesMask) == activeStagesMask);
}

AtomicCounterReferencedByCase::~AtomicCounterReferencedByCase (void)
{
	deinit();
}

void AtomicCounterReferencedByCase::init (void)
{
	const deUint32				geometryMask		= (1 << glu::SHADERTYPE_GEOMETRY);
	const deUint32				tessellationMask	= (1 << glu::SHADERTYPE_TESSELLATION_CONTROL) | (1 << glu::SHADERTYPE_TESSELLATION_EVALUATION);
	glu::VariableDeclaration	atomicVar			(glu::VarType(glu::TYPE_UINT_ATOMIC_COUNTER, glu::PRECISION_LAST), "targetCounter", glu::STORAGE_UNIFORM);
	const glu::GLSLVersion		glslVersion			= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
	const bool					supportsES32		= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if ((m_presentStagesMask & tessellationMask) != 0 && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader extension");
	if ((m_presentStagesMask & geometryMask) != 0 && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		throw tcu::NotSupportedError("Test requires GL_EXT_geometry_shader extension");

	atomicVar.layout.binding = 1;

	m_program = new ProgramInterfaceDefinition::Program();
	m_program->setSeparable(m_separable);

	for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; ++shaderType)
	{
		if (m_activeStagesMask & (1 << shaderType))
			m_program->addShader((glu::ShaderType)shaderType, glslVersion)->getDefaultBlock().variables.push_back(atomicVar);
		else if (m_presentStagesMask & (1 << shaderType))
			m_program->addShader((glu::ShaderType)shaderType, glslVersion);
	}

	if (m_program->hasStage(glu::SHADERTYPE_GEOMETRY))
		m_program->setGeometryNumOutputVertices(1);
	if (m_program->hasStage(glu::SHADERTYPE_TESSELLATION_CONTROL) || m_program->hasStage(glu::SHADERTYPE_TESSELLATION_EVALUATION))
		m_program->setTessellationNumOutputPatchVertices(1);

	DE_ASSERT(m_program->isValid());
}

void AtomicCounterReferencedByCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;
}

AtomicCounterReferencedByCase::IterateResult AtomicCounterReferencedByCase::iterate (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	const struct
	{
		glw::GLenum		propName;
		glu::ShaderType	shaderType;
		const char*		extension;
	} targetProps[] =
	{
		{ GL_REFERENCED_BY_VERTEX_SHADER,			glu::SHADERTYPE_VERTEX,						DE_NULL												},
		{ GL_REFERENCED_BY_FRAGMENT_SHADER,			glu::SHADERTYPE_FRAGMENT,					DE_NULL												},
		{ GL_REFERENCED_BY_COMPUTE_SHADER,			glu::SHADERTYPE_COMPUTE,					DE_NULL												},
		{ GL_REFERENCED_BY_TESS_CONTROL_SHADER,		glu::SHADERTYPE_TESSELLATION_CONTROL,		(supportsES32 ? DE_NULL : "GL_EXT_tessellation_shader")	},
		{ GL_REFERENCED_BY_TESS_EVALUATION_SHADER,	glu::SHADERTYPE_TESSELLATION_EVALUATION,	(supportsES32 ? DE_NULL : "GL_EXT_tessellation_shader")	},
		{ GL_REFERENCED_BY_GEOMETRY_SHADER,			glu::SHADERTYPE_GEOMETRY,					(supportsES32 ? DE_NULL : "GL_EXT_geometry_shader")		},
	};

	const glw::Functions&		gl			= m_context.getRenderContext().getFunctions();
	const glu::ShaderProgram	program		(m_context.getRenderContext(), generateProgramInterfaceProgramSources(m_program));

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	checkAndLogProgram(program, m_program, m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	// check props
	for (int propNdx = 0; propNdx < DE_LENGTH_OF_ARRAY(targetProps); ++propNdx)
	{
		if (targetProps[propNdx].extension == DE_NULL || m_context.getContextInfo().isExtensionSupported(targetProps[propNdx].extension))
		{
			const glw::GLenum	prop		= targetProps[propNdx].propName;
			const glw::GLint	expected	= ((m_activeStagesMask & (1 << targetProps[propNdx].shaderType)) != 0) ? (GL_TRUE) : (GL_FALSE);
			glw::GLint			value		= -1;
			glw::GLint			written		= -1;

			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying " << glu::getProgramResourcePropertyName(prop) << ", expecting " << glu::getBooleanName(expected) << tcu::TestLog::EndMessage;

			gl.getProgramResourceiv(program.getProgram(), GL_ATOMIC_COUNTER_BUFFER, 0, 1, &prop, 1, &written, &value);
			GLU_EXPECT_NO_ERROR(gl.getError(), "query buffer binding");

			if (written != 1)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, query for referenced_by_* returned invalid number of values." << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "property query failed");
				continue;
			}

			m_testCtx.getLog() << tcu::TestLog::Message << glu::getProgramResourcePropertyName(prop) << " = " << glu::getBooleanStr(value) << tcu::TestLog::EndMessage;

			if (value != expected)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, got unexpected value" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "unexpected property value");
				continue;
			}
		}
	}

	return STOP;
}

class ProgramInputOutputReferencedByCase : public TestCase
{
public:
	enum CaseType
	{
		CASE_VERTEX_FRAGMENT = 0,
		CASE_VERTEX_GEO_FRAGMENT,
		CASE_VERTEX_TESS_FRAGMENT,
		CASE_VERTEX_TESS_GEO_FRAGMENT,

		CASE_SEPARABLE_VERTEX,
		CASE_SEPARABLE_FRAGMENT,
		CASE_SEPARABLE_GEOMETRY,
		CASE_SEPARABLE_TESS_CTRL,
		CASE_SEPARABLE_TESS_EVAL,

		CASE_LAST
	};
											ProgramInputOutputReferencedByCase	(Context& context, const char* name, const char* description, glu::Storage targetStorage, CaseType caseType);
											~ProgramInputOutputReferencedByCase	(void);

private:
	void									init								(void);
	void									deinit								(void);
	IterateResult							iterate								(void);

	const CaseType							m_caseType;
	const glu::Storage						m_targetStorage;
	ProgramInterfaceDefinition::Program*	m_program;
};

ProgramInputOutputReferencedByCase::ProgramInputOutputReferencedByCase (Context& context, const char* name, const char* description, glu::Storage targetStorage, CaseType caseType)
	: TestCase				(context, name, description)
	, m_caseType			(caseType)
	, m_targetStorage		(targetStorage)
	, m_program				(DE_NULL)
{
	DE_ASSERT(caseType < CASE_LAST);
}

ProgramInputOutputReferencedByCase::~ProgramInputOutputReferencedByCase (void)
{
	deinit();
}

void ProgramInputOutputReferencedByCase::init (void)
{
	const bool hasTessellationShader =	(m_caseType == CASE_VERTEX_TESS_FRAGMENT)		||
										(m_caseType == CASE_VERTEX_TESS_GEO_FRAGMENT)	||
										(m_caseType == CASE_SEPARABLE_TESS_CTRL)		||
										(m_caseType == CASE_SEPARABLE_TESS_EVAL);
	const bool hasGeometryShader =		(m_caseType == CASE_VERTEX_GEO_FRAGMENT)		||
										(m_caseType == CASE_VERTEX_TESS_GEO_FRAGMENT)	||
										(m_caseType == CASE_SEPARABLE_GEOMETRY);
	const bool supportsES32 =			glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (hasTessellationShader && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader extension");
	if (hasGeometryShader && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		throw tcu::NotSupportedError("Test requires GL_EXT_geometry_shader extension");

	glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
	m_program = new ProgramInterfaceDefinition::Program();

	if (m_caseType == CASE_SEPARABLE_VERTEX		||
		m_caseType == CASE_SEPARABLE_FRAGMENT	||
		m_caseType == CASE_SEPARABLE_GEOMETRY	||
		m_caseType == CASE_SEPARABLE_TESS_CTRL	||
		m_caseType == CASE_SEPARABLE_TESS_EVAL)
	{
		const bool						isInputCase			= (m_targetStorage == glu::STORAGE_IN || m_targetStorage == glu::STORAGE_PATCH_IN);
		const bool						perPatchStorage		= (m_targetStorage == glu::STORAGE_PATCH_IN || m_targetStorage == glu::STORAGE_PATCH_OUT);
		const char*						varName				= (isInputCase) ? ("shaderInput") : ("shaderOutput");
		const glu::VariableDeclaration	targetDecl			(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP), varName, m_targetStorage);
		const glu::ShaderType			shaderType			= (m_caseType == CASE_SEPARABLE_VERTEX)		? (glu::SHADERTYPE_VERTEX)
															: (m_caseType == CASE_SEPARABLE_FRAGMENT)	? (glu::SHADERTYPE_FRAGMENT)
															: (m_caseType == CASE_SEPARABLE_GEOMETRY)	? (glu::SHADERTYPE_GEOMETRY)
															: (m_caseType == CASE_SEPARABLE_TESS_CTRL)	? (glu::SHADERTYPE_TESSELLATION_CONTROL)
															: (m_caseType == CASE_SEPARABLE_TESS_EVAL)	? (glu::SHADERTYPE_TESSELLATION_EVALUATION)
															:											  (glu::SHADERTYPE_LAST);
		const bool						arrayedInterface	= (isInputCase) ? ((shaderType == glu::SHADERTYPE_GEOMETRY)					||
																			   (shaderType == glu::SHADERTYPE_TESSELLATION_CONTROL)		||
																			   (shaderType == glu::SHADERTYPE_TESSELLATION_EVALUATION))
																			: (shaderType == glu::SHADERTYPE_TESSELLATION_CONTROL);

		m_program->setSeparable(true);

		if (arrayedInterface && !perPatchStorage)
		{
			const glu::VariableDeclaration targetDeclArr(glu::VarType(targetDecl.varType, glu::VarType::UNSIZED_ARRAY), varName, m_targetStorage);
			m_program->addShader(shaderType, glslVersion)->getDefaultBlock().variables.push_back(targetDeclArr);
		}
		else
		{
			m_program->addShader(shaderType, glslVersion)->getDefaultBlock().variables.push_back(targetDecl);
		}
	}
	else if (m_caseType == CASE_VERTEX_FRAGMENT			||
			 m_caseType == CASE_VERTEX_GEO_FRAGMENT		||
			 m_caseType == CASE_VERTEX_TESS_FRAGMENT	||
			 m_caseType == CASE_VERTEX_TESS_GEO_FRAGMENT)
	{
		ProgramInterfaceDefinition::Shader*	vertex		= m_program->addShader(glu::SHADERTYPE_VERTEX, glslVersion);
		ProgramInterfaceDefinition::Shader*	fragment	= m_program->addShader(glu::SHADERTYPE_FRAGMENT, glslVersion);

		m_program->setSeparable(false);

		vertex->getDefaultBlock().variables.push_back(glu::VariableDeclaration(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP),
																			   "shaderInput",
																			   glu::STORAGE_IN));
		vertex->getDefaultBlock().variables.push_back(glu::VariableDeclaration(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP),
																			   "shaderOutput",
																			   glu::STORAGE_OUT,
																			   glu::INTERPOLATION_LAST,
																			   glu::Layout(1)));

		fragment->getDefaultBlock().variables.push_back(glu::VariableDeclaration(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP),
																				 "shaderOutput",
																				 glu::STORAGE_OUT,
																				 glu::INTERPOLATION_LAST,
																				 glu::Layout(0)));
		fragment->getDefaultBlock().variables.push_back(glu::VariableDeclaration(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP),
																				 "shaderInput",
																				 glu::STORAGE_IN,
																				 glu::INTERPOLATION_LAST,
																				 glu::Layout(1)));

		if (m_caseType == CASE_VERTEX_TESS_FRAGMENT || m_caseType == CASE_VERTEX_TESS_GEO_FRAGMENT)
		{
			ProgramInterfaceDefinition::Shader* tessCtrl = m_program->addShader(glu::SHADERTYPE_TESSELLATION_CONTROL, glslVersion);
			ProgramInterfaceDefinition::Shader* tessEval = m_program->addShader(glu::SHADERTYPE_TESSELLATION_EVALUATION, glslVersion);

			tessCtrl->getDefaultBlock().variables.push_back(glu::VariableDeclaration(glu::VarType(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP), glu::VarType::UNSIZED_ARRAY),
																					 "shaderInput",
																					 glu::STORAGE_IN,
																					 glu::INTERPOLATION_LAST,
																					 glu::Layout(1)));
			tessCtrl->getDefaultBlock().variables.push_back(glu::VariableDeclaration(glu::VarType(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP), glu::VarType::UNSIZED_ARRAY),
																					 "shaderOutput",
																					 glu::STORAGE_OUT,
																					 glu::INTERPOLATION_LAST,
																					 glu::Layout(1)));

			tessEval->getDefaultBlock().variables.push_back(glu::VariableDeclaration(glu::VarType(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP), glu::VarType::UNSIZED_ARRAY),
																					 "shaderInput",
																					 glu::STORAGE_IN,
																					 glu::INTERPOLATION_LAST,
																					 glu::Layout(1)));
			tessEval->getDefaultBlock().variables.push_back(glu::VariableDeclaration(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP),
																					 "shaderOutput",
																					 glu::STORAGE_OUT,
																					 glu::INTERPOLATION_LAST,
																					 glu::Layout(1)));
		}

		if (m_caseType == CASE_VERTEX_GEO_FRAGMENT || m_caseType == CASE_VERTEX_TESS_GEO_FRAGMENT)
		{
			ProgramInterfaceDefinition::Shader* geometry = m_program->addShader(glu::SHADERTYPE_GEOMETRY, glslVersion);

			geometry->getDefaultBlock().variables.push_back(glu::VariableDeclaration(glu::VarType(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP), glu::VarType::UNSIZED_ARRAY),
																					 "shaderInput",
																					 glu::STORAGE_IN,
																					 glu::INTERPOLATION_LAST,
																					 glu::Layout(1)));
			geometry->getDefaultBlock().variables.push_back(glu::VariableDeclaration(glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP),
																					 "shaderOutput",
																					 glu::STORAGE_OUT,
																					 glu::INTERPOLATION_LAST,
																					 glu::Layout(1)));
		}
	}
	else
		DE_ASSERT(false);

	if (m_program->hasStage(glu::SHADERTYPE_GEOMETRY))
		m_program->setGeometryNumOutputVertices(1);
	if (m_program->hasStage(glu::SHADERTYPE_TESSELLATION_CONTROL) || m_program->hasStage(glu::SHADERTYPE_TESSELLATION_EVALUATION))
		m_program->setTessellationNumOutputPatchVertices(1);

	DE_ASSERT(m_program->isValid());
}

void ProgramInputOutputReferencedByCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;
}

ProgramInputOutputReferencedByCase::IterateResult ProgramInputOutputReferencedByCase::iterate (void)
{
	static const struct
	{
		glw::GLenum		propName;
		glu::ShaderType	shaderType;
		const char*		extension;
	} targetProps[] =
	{
		{ GL_REFERENCED_BY_VERTEX_SHADER,			glu::SHADERTYPE_VERTEX,						DE_NULL							},
		{ GL_REFERENCED_BY_FRAGMENT_SHADER,			glu::SHADERTYPE_FRAGMENT,					DE_NULL							},
		{ GL_REFERENCED_BY_COMPUTE_SHADER,			glu::SHADERTYPE_COMPUTE,					DE_NULL							},
		{ GL_REFERENCED_BY_TESS_CONTROL_SHADER,		glu::SHADERTYPE_TESSELLATION_CONTROL,		"GL_EXT_tessellation_shader"	},
		{ GL_REFERENCED_BY_TESS_EVALUATION_SHADER,	glu::SHADERTYPE_TESSELLATION_EVALUATION,	"GL_EXT_tessellation_shader"	},
		{ GL_REFERENCED_BY_GEOMETRY_SHADER,			glu::SHADERTYPE_GEOMETRY,					"GL_EXT_geometry_shader"		},
	};

	const bool					isInputCase						= (m_targetStorage == glu::STORAGE_IN || m_targetStorage == glu::STORAGE_PATCH_IN);
	const glw::Functions&		gl								= m_context.getRenderContext().getFunctions();
	const glu::ShaderProgram	program							(m_context.getRenderContext(), generateProgramInterfaceProgramSources(m_program));
	const std::string			targetResourceName				= (isInputCase) ? ("shaderInput") : ("shaderOutput");
	const glw::GLenum			programGLInterface				= (isInputCase) ? (GL_PROGRAM_INPUT) : (GL_PROGRAM_OUTPUT);
	glw::GLuint					resourceIndex;

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	checkAndLogProgram(program, m_program, m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	// find target resource index

	resourceIndex = gl.getProgramResourceIndex(program.getProgram(), programGLInterface, targetResourceName.c_str());
	GLU_EXPECT_NO_ERROR(gl.getError(), "query resource index");

	if (resourceIndex == GL_INVALID_INDEX)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Error, query for resource \"" << targetResourceName << "\" index returned invalid index." << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "could not find target resource");
		return STOP;
	}

	// check props
	for (int propNdx = 0; propNdx < DE_LENGTH_OF_ARRAY(targetProps); ++propNdx)
	{
		if (targetProps[propNdx].extension == DE_NULL || m_context.getContextInfo().isExtensionSupported(targetProps[propNdx].extension))
		{
			const glw::GLenum	prop			= targetProps[propNdx].propName;
			const bool			expected		= (isInputCase) ? (targetProps[propNdx].shaderType == m_program->getFirstStage()) : (targetProps[propNdx].shaderType == m_program->getLastStage());
			glw::GLint			value			= -1;
			glw::GLint			written			= -1;

			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying " << glu::getProgramResourcePropertyName(prop) << ", expecting " << ((expected) ? ("TRUE") : ("FALSE")) << tcu::TestLog::EndMessage;

			gl.getProgramResourceiv(program.getProgram(), programGLInterface, resourceIndex, 1, &prop, 1, &written, &value);
			GLU_EXPECT_NO_ERROR(gl.getError(), "query buffer binding");

			if (written != 1)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, query for referenced_by_* returned invalid number of values." << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "property query failed");
				continue;
			}

			m_testCtx.getLog() << tcu::TestLog::Message << glu::getProgramResourcePropertyName(prop) << " = " << glu::getBooleanStr(value) << tcu::TestLog::EndMessage;

			if (value != ((expected) ? (GL_TRUE) : (GL_FALSE)))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Error, got unexpected value" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "unexpected property value");
				continue;
			}
		}
	}

	return STOP;
}

class FeedbackResourceListTestCase : public ResourceListTestCase
{
public:
											FeedbackResourceListTestCase	(Context& context, const ResourceDefinition::Node::SharedPtr& resource, const char* name);
											~FeedbackResourceListTestCase	(void);

private:
	IterateResult							iterate							(void);
};

FeedbackResourceListTestCase::FeedbackResourceListTestCase (Context& context, const ResourceDefinition::Node::SharedPtr& resource, const char* name)
	: ResourceListTestCase(context, resource, PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING, name)
{
}

FeedbackResourceListTestCase::~FeedbackResourceListTestCase (void)
{
	deinit();
}

FeedbackResourceListTestCase::IterateResult FeedbackResourceListTestCase::iterate (void)
{
	const glu::ShaderProgram program(m_context.getRenderContext(), generateProgramInterfaceProgramSources(m_programDefinition));

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// Feedback varyings
	{
		tcu::MessageBuilder builder(&m_testCtx.getLog());
		builder << "Transform feedback varyings: {";
		for (int ndx = 0; ndx < (int)m_programDefinition->getTransformFeedbackVaryings().size(); ++ndx)
		{
			if (ndx)
				builder << ", ";
			builder << "\"" << m_programDefinition->getTransformFeedbackVaryings()[ndx] << "\"";
		}
		builder << "}" << tcu::TestLog::EndMessage;
	}

	checkAndLogProgram(program, m_programDefinition, m_context.getRenderContext().getFunctions(), m_testCtx.getLog());

	// Check resource list
	{
		const tcu::ScopedLogSection	section				(m_testCtx.getLog(), "ResourceList", "Resource list");
		std::vector<std::string>	resourceList;
		std::vector<std::string>	expectedResources;

		queryResourceList(resourceList, program.getProgram());
		expectedResources = getProgramInterfaceResourceList(m_programDefinition, m_programInterface);

		// verify the list and the expected list match

		if (!verifyResourceList(resourceList, expectedResources))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "invalid resource list");

		// verify GetProgramResourceIndex() matches the indices of the list

		if (!verifyResourceIndexQuery(resourceList, expectedResources, program.getProgram()))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "GetProgramResourceIndex returned unexpected values");

		// Verify MAX_NAME_LENGTH
		if (!verifyMaxNameLength(resourceList, program.getProgram()))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "MAX_NAME_LENGTH invalid");
	}

	return STOP;
}

int InterfaceBlockDataSizeTestCase::getBlockMinDataSize (const glu::InterfaceBlock& block) const
{
	int dataSize = 0;

	for (int ndx = 0; ndx < (int)block.variables.size(); ++ndx)
		dataSize += getVarTypeSize(block.variables[ndx].varType);

	return dataSize;
}

static bool isDataTypeLayoutQualified (glu::DataType type)
{
	return glu::isDataTypeImage(type) || glu::isDataTypeAtomicCounter(type);
}

static void generateVariableCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, const ProgramResourceQueryTestTarget& queryTarget, int expandLevel = 3, bool createTestGroup = true)
{
	static const struct
	{
		int				level;
		glu::DataType	dataType;
	} variableTypes[] =
	{
		{ 0,	glu::TYPE_FLOAT			},
		{ 1,	glu::TYPE_INT			},
		{ 1,	glu::TYPE_UINT			},
		{ 1,	glu::TYPE_BOOL			},

		{ 3,	glu::TYPE_FLOAT_VEC2	},
		{ 1,	glu::TYPE_FLOAT_VEC3	},
		{ 1,	glu::TYPE_FLOAT_VEC4	},

		{ 3,	glu::TYPE_INT_VEC2		},
		{ 2,	glu::TYPE_INT_VEC3		},
		{ 3,	glu::TYPE_INT_VEC4		},

		{ 3,	glu::TYPE_UINT_VEC2		},
		{ 2,	glu::TYPE_UINT_VEC3		},
		{ 3,	glu::TYPE_UINT_VEC4		},

		{ 3,	glu::TYPE_BOOL_VEC2		},
		{ 2,	glu::TYPE_BOOL_VEC3		},
		{ 3,	glu::TYPE_BOOL_VEC4		},

		{ 2,	glu::TYPE_FLOAT_MAT2	},
		{ 3,	glu::TYPE_FLOAT_MAT2X3	},
		{ 3,	glu::TYPE_FLOAT_MAT2X4	},
		{ 2,	glu::TYPE_FLOAT_MAT3X2	},
		{ 2,	glu::TYPE_FLOAT_MAT3	},
		{ 3,	glu::TYPE_FLOAT_MAT3X4	},
		{ 2,	glu::TYPE_FLOAT_MAT4X2	},
		{ 3,	glu::TYPE_FLOAT_MAT4X3	},
		{ 2,	glu::TYPE_FLOAT_MAT4	},
	};

	tcu::TestCaseGroup* group;

	if (createTestGroup)
	{
		group = new tcu::TestCaseGroup(context.getTestContext(), "basic_type", "Basic variable");
		targetGroup->addChild(group);
	}
	else
		group = targetGroup;

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(variableTypes); ++ndx)
	{
		if (variableTypes[ndx].level <= expandLevel)
		{
			const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(parentStructure, variableTypes[ndx].dataType));
			group->addChild(new ResourceTestCase(context, variable, queryTarget));
		}
	}
}

static void generateOpaqueTypeCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, const ProgramResourceQueryTestTarget& queryTarget, int expandLevel = 3, bool createTestGroup = true)
{
	static const struct
	{
		int				level;
		glu::DataType	dataType;
	} variableTypes[] =
	{
		{ 0,	glu::TYPE_SAMPLER_2D					},
		{ 2,	glu::TYPE_SAMPLER_CUBE					},
		{ 1,	glu::TYPE_SAMPLER_2D_ARRAY				},
		{ 1,	glu::TYPE_SAMPLER_3D					},
		{ 2,	glu::TYPE_SAMPLER_2D_SHADOW				},
		{ 3,	glu::TYPE_SAMPLER_CUBE_SHADOW			},
		{ 3,	glu::TYPE_SAMPLER_2D_ARRAY_SHADOW		},
		{ 1,	glu::TYPE_INT_SAMPLER_2D				},
		{ 3,	glu::TYPE_INT_SAMPLER_CUBE				},
		{ 3,	glu::TYPE_INT_SAMPLER_2D_ARRAY			},
		{ 3,	glu::TYPE_INT_SAMPLER_3D				},
		{ 2,	glu::TYPE_UINT_SAMPLER_2D				},
		{ 3,	glu::TYPE_UINT_SAMPLER_CUBE				},
		{ 3,	glu::TYPE_UINT_SAMPLER_2D_ARRAY			},
		{ 3,	glu::TYPE_UINT_SAMPLER_3D				},
		{ 2,	glu::TYPE_SAMPLER_2D_MULTISAMPLE		},
		{ 2,	glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE	},
		{ 3,	glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE	},
		{ 1,	glu::TYPE_IMAGE_2D						},
		{ 3,	glu::TYPE_IMAGE_CUBE					},
		{ 3,	glu::TYPE_IMAGE_2D_ARRAY				},
		{ 3,	glu::TYPE_IMAGE_3D						},
		{ 3,	glu::TYPE_INT_IMAGE_2D					},
		{ 3,	glu::TYPE_INT_IMAGE_CUBE				},
		{ 1,	glu::TYPE_INT_IMAGE_2D_ARRAY			},
		{ 3,	glu::TYPE_INT_IMAGE_3D					},
		{ 2,	glu::TYPE_UINT_IMAGE_2D					},
		{ 3,	glu::TYPE_UINT_IMAGE_CUBE				},
		{ 3,	glu::TYPE_UINT_IMAGE_2D_ARRAY			},
		{ 3,	glu::TYPE_UINT_IMAGE_3D					},
		{ 1,	glu::TYPE_UINT_ATOMIC_COUNTER			},
	};

	bool isStructMember = false;

	// Requirements
	for (const ResourceDefinition::Node* node = parentStructure.get(); node; node = node->getEnclosingNode())
	{
		// Don't insert inside a interface block
		if (node->getType() == ResourceDefinition::Node::TYPE_INTERFACE_BLOCK)
			return;

		isStructMember |= (node->getType() == ResourceDefinition::Node::TYPE_STRUCT_MEMBER);
	}

	// Add cases
	{
		tcu::TestCaseGroup* group;

		if (createTestGroup)
		{
			group = new tcu::TestCaseGroup(context.getTestContext(), "opaque_type", "Opaque types");
			targetGroup->addChild(group);
		}
		else
			group = targetGroup;

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(variableTypes); ++ndx)
		{
			if (variableTypes[ndx].level > expandLevel)
				continue;

			// Layout qualifiers are not allowed on struct members
			if (isDataTypeLayoutQualified(variableTypes[ndx].dataType) && isStructMember)
				continue;

			{
				const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(parentStructure, variableTypes[ndx].dataType));
				group->addChild(new ResourceTestCase(context, variable, queryTarget));
			}
		}
	}
}

static void generateCompoundVariableCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, const ProgramResourceQueryTestTarget& queryTarget, int expandLevel = 3);

static void generateVariableArrayCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, const ProgramResourceQueryTestTarget& queryTarget, int expandLevel = 3)
{
	if (expandLevel > 0)
	{
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(parentStructure));
		tcu::TestCaseGroup* const					blockGroup		= new tcu::TestCaseGroup(context.getTestContext(), "array", "Arrays");

		targetGroup->addChild(blockGroup);

		// Arrays of basic variables
		generateVariableCases(context, arrayElement, blockGroup, queryTarget, expandLevel, expandLevel != 1);

		// Arrays of opaque types
		generateOpaqueTypeCases(context, arrayElement, blockGroup, queryTarget, expandLevel, expandLevel != 1);

		// Arrays of arrays
		generateVariableArrayCases(context, arrayElement, blockGroup, queryTarget, expandLevel-1);

		// Arrays of structs
		generateCompoundVariableCases(context, arrayElement, blockGroup, queryTarget, expandLevel-1);
	}
}

static void generateCompoundVariableCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, const ProgramResourceQueryTestTarget& queryTarget, int expandLevel)
{
	if (expandLevel > 0)
	{
		const ResourceDefinition::Node::SharedPtr	structMember	(new ResourceDefinition::StructMember(parentStructure));
		tcu::TestCaseGroup* const					blockGroup		= new tcu::TestCaseGroup(context.getTestContext(), "struct", "Structs");

		targetGroup->addChild(blockGroup);

		// Struct containing basic variable
		generateVariableCases(context, structMember, blockGroup, queryTarget, expandLevel, expandLevel != 1);

		// Struct containing opaque types
		generateOpaqueTypeCases(context, structMember, blockGroup, queryTarget, expandLevel, expandLevel != 1);

		// Struct containing arrays
		generateVariableArrayCases(context, structMember, blockGroup, queryTarget, expandLevel-1);

		// Struct containing struct
		generateCompoundVariableCases(context, structMember, blockGroup, queryTarget, expandLevel-1);
	}
}

// Resource list cases

enum BlockFlags
{
	BLOCKFLAG_DEFAULT	= 0x01,
	BLOCKFLAG_NAMED		= 0x02,
	BLOCKFLAG_UNNAMED	= 0x04,
	BLOCKFLAG_ARRAY		= 0x08,

	BLOCKFLAG_ALL		= 0x0F
};

static void generateUniformCaseBlocks (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, deUint32 blockFlags, void (*blockContentGenerator)(Context&, const ResourceDefinition::Node::SharedPtr&, tcu::TestCaseGroup* const))
{
	const ResourceDefinition::Node::SharedPtr defaultBlock	(new ResourceDefinition::DefaultBlock(parentStructure));
	const ResourceDefinition::Node::SharedPtr uniform		(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_UNIFORM));

	// .default_block
	if (blockFlags & BLOCKFLAG_DEFAULT)
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "default_block", "Default block");
		targetGroup->addChild(blockGroup);

		blockContentGenerator(context, uniform, blockGroup);
	}

	// .named_block
	if (blockFlags & BLOCKFLAG_NAMED)
	{
		const ResourceDefinition::Node::SharedPtr block(new ResourceDefinition::InterfaceBlock(uniform, true));

		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "named_block", "Named uniform block");
		targetGroup->addChild(blockGroup);

		blockContentGenerator(context, block, blockGroup);
	}

	// .unnamed_block
	if (blockFlags & BLOCKFLAG_UNNAMED)
	{
		const ResourceDefinition::Node::SharedPtr block(new ResourceDefinition::InterfaceBlock(uniform, false));

		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "unnamed_block", "Unnamed uniform block");
		targetGroup->addChild(blockGroup);

		blockContentGenerator(context, block, blockGroup);
	}

	// .block_array
	if (blockFlags & BLOCKFLAG_ARRAY)
	{
		const ResourceDefinition::Node::SharedPtr arrayElement	(new ResourceDefinition::ArrayElement(uniform));
		const ResourceDefinition::Node::SharedPtr block			(new ResourceDefinition::InterfaceBlock(arrayElement, true));

		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "block_array", "Uniform block array");
		targetGroup->addChild(blockGroup);

		blockContentGenerator(context, block, blockGroup);
	}
}

static void generateBufferBackedResourceListBlockContentCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, ProgramInterface interface, int depth)
{
	// variable
	{
		const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(parentStructure, glu::TYPE_FLOAT_VEC4));
		targetGroup->addChild(new ResourceListTestCase(context, variable, interface));
	}

	// struct
	if (depth > 0)
	{
		const ResourceDefinition::Node::SharedPtr structMember(new ResourceDefinition::StructMember(parentStructure));
		generateBufferBackedResourceListBlockContentCases(context, structMember, targetGroup, interface, depth - 1);
	}

	// array
	if (depth > 0)
	{
		const ResourceDefinition::Node::SharedPtr arrayElement(new ResourceDefinition::ArrayElement(parentStructure));
		generateBufferBackedResourceListBlockContentCases(context, arrayElement, targetGroup, interface, depth - 1);
	}
}

static void generateBufferBackedVariableAggregateTypeCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, ProgramInterface interface, ProgramResourcePropFlags targetProp, glu::DataType dataType, const std::string& nameSuffix, int depth)
{
	// variable
	{
		const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(parentStructure, dataType));
		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(interface, targetProp), ("var" + nameSuffix).c_str()));
	}

	// struct
	if (depth > 0)
	{
		const ResourceDefinition::Node::SharedPtr structMember(new ResourceDefinition::StructMember(parentStructure));
		generateBufferBackedVariableAggregateTypeCases(context, structMember, targetGroup, interface, targetProp, dataType, "_struct" + nameSuffix, depth - 1);
	}

	// array
	if (depth > 0)
	{
		const ResourceDefinition::Node::SharedPtr arrayElement(new ResourceDefinition::ArrayElement(parentStructure));
		generateBufferBackedVariableAggregateTypeCases(context, arrayElement, targetGroup, interface, targetProp, dataType, "_array" + nameSuffix, depth - 1);
	}
}

static void generateUniformResourceListBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup)
{
	generateBufferBackedResourceListBlockContentCases(context, parentStructure, targetGroup, PROGRAMINTERFACE_UNIFORM, 4);
}

static void generateUniformBlockArraySizeContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup)
{
	const ProgramResourceQueryTestTarget	queryTarget			(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_ARRAY_SIZE);
	const bool								isInterfaceBlock	= (parentStructure->getType() == ResourceDefinition::Node::TYPE_INTERFACE_BLOCK);
	const bool								namedNonArrayBlock	= isInterfaceBlock																					&&
																  static_cast<const ResourceDefinition::InterfaceBlock*>(parentStructure.get())->m_named			&&
																  parentStructure->getEnclosingNode()->getType() != ResourceDefinition::Node::TYPE_ARRAY_ELEMENT;

	if (!isInterfaceBlock || namedNonArrayBlock)
	{
		// .types
		{
			tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "types", "Types");
			targetGroup->addChild(blockGroup);

			generateVariableCases(context, parentStructure, blockGroup, queryTarget, 2, false);
			generateOpaqueTypeCases(context, parentStructure, blockGroup, queryTarget, 2, false);
		}

		// aggregates
		{
			tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "aggregates", "Aggregate types");
			targetGroup->addChild(blockGroup);

			generateBufferBackedVariableAggregateTypeCases(context, parentStructure, blockGroup, queryTarget.interface, PROGRAMRESOURCEPROP_ARRAY_SIZE, glu::TYPE_FLOAT, "", 3);
		}
	}
	else
	{
		// aggregates
		generateBufferBackedVariableAggregateTypeCases(context, parentStructure, targetGroup, queryTarget.interface, PROGRAMRESOURCEPROP_ARRAY_SIZE, glu::TYPE_FLOAT, "", 2);
	}
}

static void generateBufferBackedArrayStrideTypeAggregateSubCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, const std::string& namePrefix, ProgramInterface interface, glu::DataType type, int expandLevel)
{
	// case
	{
		const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(parentStructure, type));
		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(interface, PROGRAMRESOURCEPROP_ARRAY_STRIDE), namePrefix.c_str()));
	}

	if (expandLevel > 0)
	{
		const ResourceDefinition::Node::SharedPtr	structMember	(new ResourceDefinition::StructMember(parentStructure));
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(parentStructure));

		// _struct
		generateBufferBackedArrayStrideTypeAggregateSubCases(context, structMember, targetGroup, namePrefix + "_struct", interface, type, expandLevel - 1);

		// _array
		generateBufferBackedArrayStrideTypeAggregateSubCases(context, arrayElement, targetGroup, namePrefix + "_array", interface, type, expandLevel - 1);
	}
}

static void generateBufferBackedArrayStrideTypeAggregateCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, ProgramInterface interface, glu::DataType type, int expandLevel, bool includeBaseCase)
{
	const ResourceDefinition::Node::SharedPtr	structMember	(new ResourceDefinition::StructMember(parentStructure));
	const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(parentStructure));
	const std::string							namePrefix		= glu::getDataTypeName(type);

	if (expandLevel == 0 || includeBaseCase)
	{
		const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(parentStructure, type));
		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(interface, PROGRAMRESOURCEPROP_ARRAY_STRIDE), namePrefix.c_str()));
	}
	if (expandLevel >= 1)
	{
		// _struct
		if (!glu::isDataTypeAtomicCounter(type))
			generateBufferBackedArrayStrideTypeAggregateSubCases(context, structMember, targetGroup, namePrefix + "_struct", interface, type, expandLevel - 1);

		// _array
		generateBufferBackedArrayStrideTypeAggregateSubCases(context, arrayElement, targetGroup, namePrefix + "_array", interface, type, expandLevel - 1);
	}
}

static void generateUniformBlockArrayStrideContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup)
{
	const ProgramResourceQueryTestTarget	queryTarget			(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_ARRAY_STRIDE);
	const bool								isInterfaceBlock	= (parentStructure->getType() == ResourceDefinition::Node::TYPE_INTERFACE_BLOCK);
	const bool								namedNonArrayBlock	= isInterfaceBlock																					&&
																  static_cast<const ResourceDefinition::InterfaceBlock*>(parentStructure.get())->m_named			&&
																  parentStructure->getEnclosingNode()->getType() != ResourceDefinition::Node::TYPE_ARRAY_ELEMENT;

	if (!isInterfaceBlock || namedNonArrayBlock)
	{
		// .types
		{
			tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "types", "Types");
			targetGroup->addChild(blockGroup);

			generateVariableCases(context, parentStructure, blockGroup, queryTarget, 2, false);
			generateOpaqueTypeCases(context, parentStructure, blockGroup, queryTarget, 2, false);
		}

		// .aggregates
		{
			tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "aggregates", "Aggregate types");
			targetGroup->addChild(blockGroup);

			// .sampler_2d_*
			if (!isInterfaceBlock)
				generateBufferBackedArrayStrideTypeAggregateCases(context, parentStructure, blockGroup, queryTarget.interface, glu::TYPE_SAMPLER_2D, 1, false);

			// .atomic_counter_*
			if (!isInterfaceBlock)
			{
				const ResourceDefinition::Node::SharedPtr layout(new ResourceDefinition::LayoutQualifier(parentStructure, glu::Layout(-1, 0)));
				generateBufferBackedArrayStrideTypeAggregateCases(context, layout, blockGroup, queryTarget.interface, glu::TYPE_UINT_ATOMIC_COUNTER, 1, false);
			}

			// .float_*
			generateBufferBackedArrayStrideTypeAggregateCases(context, parentStructure, blockGroup, queryTarget.interface, glu::TYPE_FLOAT, 2, false);

			// .bool_*
			generateBufferBackedArrayStrideTypeAggregateCases(context, parentStructure, blockGroup, queryTarget.interface, glu::TYPE_BOOL, 1, false);

			// .bvec3_*
			generateBufferBackedArrayStrideTypeAggregateCases(context, parentStructure, blockGroup, queryTarget.interface, glu::TYPE_BOOL_VEC3, 2, false);

			// .vec3_*
			generateBufferBackedArrayStrideTypeAggregateCases(context, parentStructure, blockGroup, queryTarget.interface, glu::TYPE_FLOAT_VEC3, 2, false);

			// .ivec2_*
			generateBufferBackedArrayStrideTypeAggregateCases(context, parentStructure, blockGroup, queryTarget.interface, glu::TYPE_INT_VEC3, 2, false);
		}
	}
	else
	{
		generateVariableCases(context, parentStructure, targetGroup, queryTarget, 1);
		generateVariableArrayCases(context, parentStructure, targetGroup, queryTarget, 1);
		generateCompoundVariableCases(context, parentStructure, targetGroup, queryTarget, 1);
	}
}

static void generateUniformBlockLocationContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup)
{
	const ProgramResourceQueryTestTarget	queryTarget			(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_LOCATION);
	const bool								isInterfaceBlock	= (parentStructure->getType() == ResourceDefinition::Node::TYPE_INTERFACE_BLOCK);

	if (!isInterfaceBlock)
	{
		generateVariableCases(context, parentStructure, targetGroup, queryTarget, 3);
		generateOpaqueTypeCases(context, parentStructure, targetGroup, queryTarget, 3);
		generateVariableArrayCases(context, parentStructure, targetGroup, queryTarget, 2);
		generateCompoundVariableCases(context, parentStructure, targetGroup, queryTarget, 2);
	}
	else
		generateVariableCases(context, parentStructure, targetGroup, queryTarget, 1, false);
}

static void generateUniformBlockBlockIndexContents (Context& context, tcu::TestCaseGroup* const targetGroup, glu::GLSLVersion glslVersion)
{
	const ResourceDefinition::Node::SharedPtr	program			(new ResourceDefinition::Program());
	const ResourceDefinition::Node::SharedPtr	shader			(new ResourceDefinition::Shader(program, glu::SHADERTYPE_COMPUTE, glslVersion));
	const ResourceDefinition::Node::SharedPtr	defaultBlock	(new ResourceDefinition::DefaultBlock(shader));
	const ResourceDefinition::Node::SharedPtr	uniform			(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_UNIFORM));
	const ResourceDefinition::Node::SharedPtr	binding			(new ResourceDefinition::LayoutQualifier(uniform, glu::Layout(-1, 0)));

	// .default_block
	{
		const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(uniform, glu::TYPE_FLOAT_VEC4));

		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_BLOCK_INDEX), "default_block"));
	}

	// .named_block
	{
		const ResourceDefinition::Node::SharedPtr	buffer		(new ResourceDefinition::InterfaceBlock(binding, true));
		const ResourceDefinition::Node::SharedPtr	variable	(new ResourceDefinition::Variable(buffer, glu::TYPE_FLOAT_VEC4));

		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_BLOCK_INDEX), "named_block"));
	}

	// .unnamed_block
	{
		const ResourceDefinition::Node::SharedPtr	buffer		(new ResourceDefinition::InterfaceBlock(binding, false));
		const ResourceDefinition::Node::SharedPtr	variable	(new ResourceDefinition::Variable(buffer, glu::TYPE_FLOAT_VEC4));

		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_BLOCK_INDEX), "unnamed_block"));
	}

	// .block_array
	{
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(binding));
		const ResourceDefinition::Node::SharedPtr	buffer			(new ResourceDefinition::InterfaceBlock(arrayElement, true));
		const ResourceDefinition::Node::SharedPtr	variable		(new ResourceDefinition::Variable(buffer, glu::TYPE_FLOAT_VEC4));

		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_BLOCK_INDEX), "block_array"));
	}
}

static void generateUniformBlockAtomicCounterBufferIndexContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup)
{
	const ProgramResourceQueryTestTarget	queryTarget			(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_ATOMIC_COUNTER_BUFFER_INDEX);
	const bool								isInterfaceBlock	= (parentStructure->getType() == ResourceDefinition::Node::TYPE_INTERFACE_BLOCK);

	if (!isInterfaceBlock)
	{
		generateVariableCases(context, parentStructure, targetGroup, queryTarget, 3);
		generateOpaqueTypeCases(context, parentStructure, targetGroup, queryTarget, 3);

		// .array
		{
			const ResourceDefinition::Node::SharedPtr	arrayElement		(new ResourceDefinition::ArrayElement(parentStructure));
			const ResourceDefinition::Node::SharedPtr	arrayArrayElement	(new ResourceDefinition::ArrayElement(arrayElement));
			const ResourceDefinition::Node::SharedPtr	variable			(new ResourceDefinition::Variable(arrayElement, glu::TYPE_UINT_ATOMIC_COUNTER));
			const ResourceDefinition::Node::SharedPtr	elementvariable		(new ResourceDefinition::Variable(arrayArrayElement, glu::TYPE_UINT_ATOMIC_COUNTER));
			tcu::TestCaseGroup* const					blockGroup			= new tcu::TestCaseGroup(context.getTestContext(), "array", "Arrays");

			targetGroup->addChild(blockGroup);

			blockGroup->addChild(new ResourceTestCase(context, variable, queryTarget, "var_array"));
			blockGroup->addChild(new ResourceTestCase(context, elementvariable, queryTarget, "var_array_array"));
		}
	}
	else
		generateVariableCases(context, parentStructure, targetGroup, queryTarget, 1, false);
}

static void generateUniformBlockNameLengthContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup)
{
	const bool	isInterfaceBlock	= (parentStructure->getType() == ResourceDefinition::Node::TYPE_INTERFACE_BLOCK);
	const bool	namedNonArrayBlock	= isInterfaceBlock																					&&
									  static_cast<const ResourceDefinition::InterfaceBlock*>(parentStructure.get())->m_named			&&
									  parentStructure->getEnclosingNode()->getType() != ResourceDefinition::Node::TYPE_ARRAY_ELEMENT;

	if (!isInterfaceBlock || namedNonArrayBlock)
		generateBufferBackedVariableAggregateTypeCases(context, parentStructure, targetGroup, PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_NAME_LENGTH, glu::TYPE_FLOAT, "", 2);
	else
		generateBufferBackedVariableAggregateTypeCases(context, parentStructure, targetGroup, PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_NAME_LENGTH, glu::TYPE_FLOAT, "", 1);
}

static void generateUniformBlockTypeContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup)
{
	const ProgramResourceQueryTestTarget	queryTarget			(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_TYPE);
	const bool								isInterfaceBlock	= (parentStructure->getType() == ResourceDefinition::Node::TYPE_INTERFACE_BLOCK);
	const bool								namedNonArrayBlock	= isInterfaceBlock																					&&
																  static_cast<const ResourceDefinition::InterfaceBlock*>(parentStructure.get())->m_named			&&
																  parentStructure->getEnclosingNode()->getType() != ResourceDefinition::Node::TYPE_ARRAY_ELEMENT;

	if (!isInterfaceBlock || namedNonArrayBlock)
	{
		// .types
		{
			tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "types", "Types");
			targetGroup->addChild(blockGroup);

			generateVariableCases(context, parentStructure, blockGroup, queryTarget, 3, false);
			generateOpaqueTypeCases(context, parentStructure, blockGroup, queryTarget, 3, false);
		}

		generateVariableArrayCases(context, parentStructure, targetGroup, queryTarget, 1);
		generateCompoundVariableCases(context, parentStructure, targetGroup, queryTarget, 1);

	}
	else
	{
		generateVariableCases(context, parentStructure, targetGroup, queryTarget, 1);
		generateVariableArrayCases(context, parentStructure, targetGroup, queryTarget, 1);
		generateCompoundVariableCases(context, parentStructure, targetGroup, queryTarget, 1);
	}
}

static void generateUniformBlockOffsetContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup)
{
	const ProgramResourceQueryTestTarget	queryTarget			(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_OFFSET);
	const bool								isInterfaceBlock	= (parentStructure->getType() == ResourceDefinition::Node::TYPE_INTERFACE_BLOCK);
	const bool								namedNonArrayBlock	= isInterfaceBlock																					&&
																  static_cast<const ResourceDefinition::InterfaceBlock*>(parentStructure.get())->m_named			&&
																  parentStructure->getEnclosingNode()->getType() != ResourceDefinition::Node::TYPE_ARRAY_ELEMENT;

	if (!isInterfaceBlock)
	{
		// .types
		{
			tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "types", "Types");
			targetGroup->addChild(blockGroup);

			generateVariableCases(context, parentStructure, blockGroup, queryTarget, 3, false);
			generateOpaqueTypeCases(context, parentStructure, blockGroup, queryTarget, 3, false);
		}

		// .aggregates
		{
			tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "aggregates", "Aggregate types");
			targetGroup->addChild(blockGroup);

			// .atomic_uint_struct
			// .atomic_uint_array
			{
				const ResourceDefinition::Node::SharedPtr offset			(new ResourceDefinition::LayoutQualifier(parentStructure, glu::Layout(-1, -1, 4)));
				const ResourceDefinition::Node::SharedPtr arrayElement		(new ResourceDefinition::ArrayElement(offset));
				const ResourceDefinition::Node::SharedPtr elementVariable	(new ResourceDefinition::Variable(arrayElement, glu::TYPE_UINT_ATOMIC_COUNTER));

				blockGroup->addChild(new ResourceTestCase(context, elementVariable, queryTarget, "atomic_uint_array"));
			}

			// .float_array
			// .float_struct
			{
				const ResourceDefinition::Node::SharedPtr structMember		(new ResourceDefinition::StructMember(parentStructure));
				const ResourceDefinition::Node::SharedPtr arrayElement		(new ResourceDefinition::ArrayElement(parentStructure));
				const ResourceDefinition::Node::SharedPtr memberVariable	(new ResourceDefinition::Variable(structMember, glu::TYPE_FLOAT));
				const ResourceDefinition::Node::SharedPtr elementVariable	(new ResourceDefinition::Variable(arrayElement, glu::TYPE_FLOAT));

				blockGroup->addChild(new ResourceTestCase(context, memberVariable, queryTarget, "float_struct"));
				blockGroup->addChild(new ResourceTestCase(context, elementVariable, queryTarget, "float_array"));
			}
		}
	}
	else if (namedNonArrayBlock)
	{
		// .types
		{
			tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "types", "Types");
			targetGroup->addChild(blockGroup);

			generateVariableCases(context, parentStructure, blockGroup, queryTarget, 3, false);
			generateOpaqueTypeCases(context, parentStructure, blockGroup, queryTarget, 3, false);
		}

		// .aggregates
		{
			tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "aggregates", "Aggregate types");
			targetGroup->addChild(blockGroup);

			// .float_array
			// .float_struct
			{
				const ResourceDefinition::Node::SharedPtr structMember		(new ResourceDefinition::StructMember(parentStructure));
				const ResourceDefinition::Node::SharedPtr arrayElement		(new ResourceDefinition::StructMember(parentStructure));
				const ResourceDefinition::Node::SharedPtr memberVariable	(new ResourceDefinition::Variable(structMember, glu::TYPE_FLOAT));
				const ResourceDefinition::Node::SharedPtr elementVariable	(new ResourceDefinition::Variable(arrayElement, glu::TYPE_FLOAT));

				blockGroup->addChild(new ResourceTestCase(context, memberVariable, queryTarget, "float_struct"));
				blockGroup->addChild(new ResourceTestCase(context, elementVariable, queryTarget, "float_array"));
			}
		}
	}
	else
	{
		generateVariableCases(context, parentStructure, targetGroup, queryTarget, 1);
		generateVariableArrayCases(context, parentStructure, targetGroup, queryTarget, 1);
		generateCompoundVariableCases(context, parentStructure, targetGroup, queryTarget, 1);
	}
}

static void generateMatrixVariableCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, const ProgramResourceQueryTestTarget& queryTarget, bool createTestGroup = true, int expandLevel = 2)
{
	static const struct
	{
		int				priority;
		glu::DataType	type;
	} variableTypes[] =
	{
		{ 0,	glu::TYPE_FLOAT_MAT2	},
		{ 1,	glu::TYPE_FLOAT_MAT2X3	},
		{ 2,	glu::TYPE_FLOAT_MAT2X4	},
		{ 2,	glu::TYPE_FLOAT_MAT3X2	},
		{ 1,	glu::TYPE_FLOAT_MAT3	},
		{ 0,	glu::TYPE_FLOAT_MAT3X4	},
		{ 2,	glu::TYPE_FLOAT_MAT4X2	},
		{ 1,	glu::TYPE_FLOAT_MAT4X3	},
		{ 0,	glu::TYPE_FLOAT_MAT4	},
	};

	tcu::TestCaseGroup* group;

	if (createTestGroup)
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "matrix", "Basic matrix type");
		targetGroup->addChild(blockGroup);
		group = blockGroup;
	}
	else
		group = targetGroup;

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(variableTypes); ++ndx)
	{
		if (variableTypes[ndx].priority < expandLevel)
		{
			const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(parentStructure, variableTypes[ndx].type));
			group->addChild(new ResourceTestCase(context, variable, queryTarget));
		}
	}
}

static void generateMatrixStructCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, const ProgramResourceQueryTestTarget& queryTarget, int expandLevel);

static void generateMatrixArrayCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, const ProgramResourceQueryTestTarget& queryTarget, int expandLevel)
{
	if (expandLevel > 0)
	{
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(parentStructure));
		tcu::TestCaseGroup* const					blockGroup		= new tcu::TestCaseGroup(context.getTestContext(), "array", "Arrays");

		targetGroup->addChild(blockGroup);

		// Arrays of basic variables
		generateMatrixVariableCases(context, arrayElement, blockGroup, queryTarget, expandLevel != 1, expandLevel);

		// Arrays of arrays
		generateMatrixArrayCases(context, arrayElement, blockGroup, queryTarget, expandLevel-1);

		// Arrays of structs
		generateMatrixStructCases(context, arrayElement, blockGroup, queryTarget, expandLevel-1);
	}
}

static void generateMatrixStructCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, const ProgramResourceQueryTestTarget& queryTarget, int expandLevel)
{
	if (expandLevel > 0)
	{
		const ResourceDefinition::Node::SharedPtr	structMember	(new ResourceDefinition::StructMember(parentStructure));
		tcu::TestCaseGroup* const					blockGroup		= new tcu::TestCaseGroup(context.getTestContext(), "struct", "Structs");

		targetGroup->addChild(blockGroup);

		// Struct containing basic variable
		generateMatrixVariableCases(context, structMember, blockGroup, queryTarget, expandLevel != 1, expandLevel);

		// Struct containing arrays
		generateMatrixArrayCases(context, structMember, blockGroup, queryTarget, expandLevel-1);

		// Struct containing struct
		generateMatrixStructCases(context, structMember, blockGroup, queryTarget, expandLevel-1);
	}
}

static void generateUniformMatrixOrderCaseBlockContentCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, bool extendedBasicTypeCases, bool opaqueCases)
{
	static const struct
	{
		const char*			name;
		glu::MatrixOrder	order;
	} qualifiers[] =
	{
		{ "no_qualifier",	glu::MATRIXORDER_LAST			},
		{ "row_major",		glu::MATRIXORDER_ROW_MAJOR		},
		{ "column_major",	glu::MATRIXORDER_COLUMN_MAJOR	},
	};

	const ProgramResourceQueryTestTarget queryTarget(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_MATRIX_ROW_MAJOR);

	for (int qualifierNdx = 0; qualifierNdx < DE_LENGTH_OF_ARRAY(qualifiers); ++qualifierNdx)
	{
		// Add layout qualifiers only for block members
		if (qualifiers[qualifierNdx].order == glu::MATRIXORDER_LAST || parentStructure->getType() == ResourceDefinition::Node::TYPE_INTERFACE_BLOCK)
		{
			ResourceDefinition::Node::SharedPtr	subStructure	= parentStructure;
			tcu::TestCaseGroup* const			qualifierGroup	= new tcu::TestCaseGroup(context.getTestContext(), qualifiers[qualifierNdx].name, "");

			targetGroup->addChild(qualifierGroup);

			if (qualifiers[qualifierNdx].order != glu::MATRIXORDER_LAST)
			{
				glu::Layout layout;
				layout.matrixOrder = qualifiers[qualifierNdx].order;
				subStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::LayoutQualifier(subStructure, layout));
			}

			if (extendedBasicTypeCases && qualifiers[qualifierNdx].order == glu::MATRIXORDER_LAST)
			{
				// .types
				{
					tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "types", "");
					qualifierGroup->addChild(blockGroup);

					generateVariableCases(context, subStructure, blockGroup, queryTarget, 1, false);
					generateMatrixVariableCases(context, subStructure, blockGroup, queryTarget, false);
					if (opaqueCases)
						generateOpaqueTypeCases(context, subStructure, blockGroup, queryTarget, 2, false);
				}

				// .aggregates
				{
					tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "aggregates", "");
					qualifierGroup->addChild(blockGroup);

					generateBufferBackedVariableAggregateTypeCases(context, subStructure, blockGroup, queryTarget.interface, PROGRAMRESOURCEPROP_MATRIX_ROW_MAJOR, glu::TYPE_FLOAT_MAT3X2, "", 1);
				}
			}
			else
			{
				generateBufferBackedVariableAggregateTypeCases(context, subStructure, qualifierGroup, queryTarget.interface, PROGRAMRESOURCEPROP_MATRIX_ROW_MAJOR, glu::TYPE_FLOAT_MAT3X2, "", 1);
			}
		}
	}
}

static void generateUniformMatrixStrideCaseBlockContentCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, bool extendedBasicTypeCases, bool opaqueCases)
{
	static const struct
	{
		const char*			name;
		glu::MatrixOrder	order;
	} qualifiers[] =
	{
		{ "no_qualifier",	glu::MATRIXORDER_LAST			},
		{ "row_major",		glu::MATRIXORDER_ROW_MAJOR		},
		{ "column_major",	glu::MATRIXORDER_COLUMN_MAJOR	},
	};

	const ProgramResourceQueryTestTarget queryTarget(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_MATRIX_STRIDE);

	for (int qualifierNdx = 0; qualifierNdx < DE_LENGTH_OF_ARRAY(qualifiers); ++qualifierNdx)
	{
		// Add layout qualifiers only for block members
		if (qualifiers[qualifierNdx].order == glu::MATRIXORDER_LAST || parentStructure->getType() == ResourceDefinition::Node::TYPE_INTERFACE_BLOCK)
		{
			ResourceDefinition::Node::SharedPtr	subStructure	= parentStructure;
			tcu::TestCaseGroup* const			qualifierGroup	= new tcu::TestCaseGroup(context.getTestContext(), qualifiers[qualifierNdx].name, "");

			targetGroup->addChild(qualifierGroup);

			if (qualifiers[qualifierNdx].order != glu::MATRIXORDER_LAST)
			{
				glu::Layout layout;
				layout.matrixOrder = qualifiers[qualifierNdx].order;
				subStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::LayoutQualifier(subStructure, layout));
			}

			if (extendedBasicTypeCases)
			{
				// .types
				// .matrix
				if (qualifiers[qualifierNdx].order == glu::MATRIXORDER_LAST)
				{
					tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "types", "");
					qualifierGroup->addChild(blockGroup);

					generateVariableCases(context, subStructure, blockGroup, queryTarget, 1, false);
					generateMatrixVariableCases(context, subStructure, blockGroup, queryTarget, false);
					if (opaqueCases)
						generateOpaqueTypeCases(context, subStructure, blockGroup, queryTarget, 2, false);
				}
				else
					generateMatrixVariableCases(context, subStructure, qualifierGroup, queryTarget);

				// .aggregates
				{
					tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "aggregates", "");
					qualifierGroup->addChild(blockGroup);

					generateBufferBackedVariableAggregateTypeCases(context, subStructure, blockGroup, queryTarget.interface, PROGRAMRESOURCEPROP_MATRIX_ROW_MAJOR, glu::TYPE_FLOAT_MAT3X2, "", 1);
				}
			}
			else
				generateBufferBackedVariableAggregateTypeCases(context, subStructure, qualifierGroup, queryTarget.interface, PROGRAMRESOURCEPROP_MATRIX_ROW_MAJOR, glu::TYPE_FLOAT_MAT3X2, "", 1);
		}
	}
}

static void generateUniformMatrixCaseBlocks (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, void (*blockContentGenerator)(Context&, const ResourceDefinition::Node::SharedPtr&, tcu::TestCaseGroup* const, bool, bool))
{
	static const struct
	{
		const char*			name;
		const char*			description;
		bool				block;
		bool				namedBlock;
		bool				extendedBasicTypeCases;
		glu::MatrixOrder	order;
	} children[] =
	{
		{ "default_block",				"Default block",			false,	true,	true,	glu::MATRIXORDER_LAST			},
		{ "named_block",				"Named uniform block",		true,	true,	true,	glu::MATRIXORDER_LAST			},
		{ "named_block_row_major",		"Named uniform block",		true,	true,	false,	glu::MATRIXORDER_ROW_MAJOR		},
		{ "named_block_col_major",		"Named uniform block",		true,	true,	false,	glu::MATRIXORDER_COLUMN_MAJOR	},
		{ "unnamed_block",				"Unnamed uniform block",	true,	false,	false,	glu::MATRIXORDER_LAST			},
		{ "unnamed_block_row_major",	"Unnamed uniform block",	true,	false,	false,	glu::MATRIXORDER_ROW_MAJOR		},
		{ "unnamed_block_col_major",	"Unnamed uniform block",	true,	false,	false,	glu::MATRIXORDER_COLUMN_MAJOR	},
	};

	const ResourceDefinition::Node::SharedPtr defaultBlock	(new ResourceDefinition::DefaultBlock(parentStructure));
	const ResourceDefinition::Node::SharedPtr uniform		(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_UNIFORM));

	for (int childNdx = 0; childNdx < (int)DE_LENGTH_OF_ARRAY(children); ++childNdx)
	{
		ResourceDefinition::Node::SharedPtr	subStructure	= uniform;
		tcu::TestCaseGroup* const			blockGroup		= new tcu::TestCaseGroup(context.getTestContext(), children[childNdx].name, children[childNdx].description);
		const bool							addOpaqueCases	= children[childNdx].extendedBasicTypeCases && !children[childNdx].block;

		targetGroup->addChild(blockGroup);

		if (children[childNdx].order != glu::MATRIXORDER_LAST)
		{
			glu::Layout layout;
			layout.matrixOrder = children[childNdx].order;
			subStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::LayoutQualifier(subStructure, layout));
		}

		if (children[childNdx].block)
			subStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::InterfaceBlock(subStructure, children[childNdx].namedBlock));

		blockContentGenerator(context, subStructure, blockGroup, children[childNdx].extendedBasicTypeCases, addOpaqueCases);
	}
}

static void generateBufferReferencedByShaderInterfaceBlockCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, const ProgramResourceQueryTestTarget& queryTarget, bool extendedCases)
{
	const bool isDefaultBlock = (parentStructure->getType() != ResourceDefinition::Node::TYPE_INTERFACE_BLOCK);

	// .float
	// .float_array
	// .float_struct
	{
		const ResourceDefinition::Node::SharedPtr	variable		(new ResourceDefinition::Variable(parentStructure, glu::TYPE_FLOAT));
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(parentStructure));
		const ResourceDefinition::Node::SharedPtr	structMember	(new ResourceDefinition::StructMember(parentStructure));
		const ResourceDefinition::Node::SharedPtr	variableArray	(new ResourceDefinition::Variable(arrayElement, glu::TYPE_FLOAT));
		const ResourceDefinition::Node::SharedPtr	variableStruct	(new ResourceDefinition::Variable(structMember, glu::TYPE_FLOAT));

		targetGroup->addChild(new ResourceTestCase(context, variable, queryTarget, "float"));
		targetGroup->addChild(new ResourceTestCase(context, variableArray, queryTarget, "float_array"));
		targetGroup->addChild(new ResourceTestCase(context, variableStruct, queryTarget, "float_struct"));
	}

	// .sampler
	// .sampler_array
	// .sampler_struct
	if (isDefaultBlock)
	{
		const ResourceDefinition::Node::SharedPtr	layout			(new ResourceDefinition::LayoutQualifier(parentStructure, glu::Layout(-1, 0)));
		const ResourceDefinition::Node::SharedPtr	variable		(new ResourceDefinition::Variable(layout, glu::TYPE_SAMPLER_2D));
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(layout));
		const ResourceDefinition::Node::SharedPtr	structMember	(new ResourceDefinition::StructMember(parentStructure));
		const ResourceDefinition::Node::SharedPtr	variableArray	(new ResourceDefinition::Variable(arrayElement, glu::TYPE_SAMPLER_2D));
		const ResourceDefinition::Node::SharedPtr	variableStruct	(new ResourceDefinition::Variable(structMember, glu::TYPE_SAMPLER_2D));

		targetGroup->addChild(new ResourceTestCase(context, variable, queryTarget, "sampler"));
		targetGroup->addChild(new ResourceTestCase(context, variableArray, queryTarget, "sampler_array"));
		targetGroup->addChild(new ResourceTestCase(context, variableStruct, queryTarget, "sampler_struct"));
	}

	// .atomic_uint
	// .atomic_uint_array
	if (isDefaultBlock)
	{
		const ResourceDefinition::Node::SharedPtr	layout			(new ResourceDefinition::LayoutQualifier(parentStructure, glu::Layout(-1, 0)));
		const ResourceDefinition::Node::SharedPtr	variable		(new ResourceDefinition::Variable(layout, glu::TYPE_UINT_ATOMIC_COUNTER));
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(layout));
		const ResourceDefinition::Node::SharedPtr	variableArray	(new ResourceDefinition::Variable(arrayElement, glu::TYPE_UINT_ATOMIC_COUNTER));

		targetGroup->addChild(new ResourceTestCase(context, variable, queryTarget, "atomic_uint"));
		targetGroup->addChild(new ResourceTestCase(context, variableArray, queryTarget, "atomic_uint_array"));
	}

	if (extendedCases)
	{
		// .float_array_struct
		{
			const ResourceDefinition::Node::SharedPtr	structMember		(new ResourceDefinition::StructMember(parentStructure));
			const ResourceDefinition::Node::SharedPtr	arrayElement		(new ResourceDefinition::ArrayElement(structMember));
			const ResourceDefinition::Node::SharedPtr	variableArrayStruct	(new ResourceDefinition::Variable(arrayElement, glu::TYPE_FLOAT));

			targetGroup->addChild(new ResourceTestCase(context, variableArrayStruct, queryTarget, "float_array_struct"));
		}

		// .float_struct_array
		{
			const ResourceDefinition::Node::SharedPtr	arrayElement		(new ResourceDefinition::ArrayElement(parentStructure));
			const ResourceDefinition::Node::SharedPtr	arrayStructMember	(new ResourceDefinition::StructMember(arrayElement));
			const ResourceDefinition::Node::SharedPtr	variableArrayStruct	(new ResourceDefinition::Variable(arrayStructMember, glu::TYPE_FLOAT));

			targetGroup->addChild(new ResourceTestCase(context, variableArrayStruct, queryTarget, "float_struct_array"));
		}

		// .float_array_array
		{
			const ResourceDefinition::Node::SharedPtr	arrayElement		(new ResourceDefinition::ArrayElement(parentStructure));
			const ResourceDefinition::Node::SharedPtr	subArrayElement		(new ResourceDefinition::ArrayElement(arrayElement));
			const ResourceDefinition::Node::SharedPtr	variableArrayStruct	(new ResourceDefinition::Variable(subArrayElement, glu::TYPE_FLOAT));

			targetGroup->addChild(new ResourceTestCase(context, variableArrayStruct, queryTarget, "float_array_array"));
		}

		// .float_struct_struct
		{
			const ResourceDefinition::Node::SharedPtr	structMember		(new ResourceDefinition::StructMember(parentStructure));
			const ResourceDefinition::Node::SharedPtr	subStructMember		(new ResourceDefinition::StructMember(structMember));
			const ResourceDefinition::Node::SharedPtr	variableArrayStruct	(new ResourceDefinition::Variable(subStructMember, glu::TYPE_FLOAT));

			targetGroup->addChild(new ResourceTestCase(context, variableArrayStruct, queryTarget, "float_struct_struct"));
		}

		if (queryTarget.interface == PROGRAMINTERFACE_BUFFER_VARIABLE)
		{
			const ResourceDefinition::Node::SharedPtr arrayElement(new ResourceDefinition::ArrayElement(parentStructure, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));

			// .float_unsized_array
			{
				const ResourceDefinition::Node::SharedPtr	variableArray	(new ResourceDefinition::Variable(arrayElement, glu::TYPE_FLOAT));

				targetGroup->addChild(new ResourceTestCase(context, variableArray, queryTarget, "float_unsized_array"));
			}

			// .float_unsized_struct_array
			{
				const ResourceDefinition::Node::SharedPtr	structMember	(new ResourceDefinition::StructMember(arrayElement));
				const ResourceDefinition::Node::SharedPtr	variableArray	(new ResourceDefinition::Variable(structMember, glu::TYPE_FLOAT));

				targetGroup->addChild(new ResourceTestCase(context, variableArray, queryTarget, "float_unsized_struct_array"));
			}
		}
	}
}

static void generateUniformReferencedByShaderSingleBlockContentCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, int expandLevel)
{
	DE_UNREF(expandLevel);

	const ResourceDefinition::Node::SharedPtr	defaultBlock		(new ResourceDefinition::DefaultBlock(parentStructure));
	const ResourceDefinition::Node::SharedPtr	uniform				(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_UNIFORM));
	const ProgramResourceQueryTestTarget		queryTarget			(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER);
	const bool									singleShaderCase	= parentStructure->getType() == ResourceDefinition::Node::TYPE_SHADER;

	// .default_block
	{
		TestCaseGroup* const blockGroup = new TestCaseGroup(context, "default_block", "");
		targetGroup->addChild(blockGroup);

		generateBufferReferencedByShaderInterfaceBlockCases(context, uniform, blockGroup, queryTarget, singleShaderCase);
	}

	// .named_block
	{
		const ResourceDefinition::Node::SharedPtr	block		(new ResourceDefinition::InterfaceBlock(uniform, true));
		TestCaseGroup* const						blockGroup	= new TestCaseGroup(context, "uniform_block", "");

		targetGroup->addChild(blockGroup);

		generateBufferReferencedByShaderInterfaceBlockCases(context, block, blockGroup, queryTarget, singleShaderCase);
	}

	// .unnamed_block
	{
		const ResourceDefinition::Node::SharedPtr	block		(new ResourceDefinition::InterfaceBlock(uniform, false));
		TestCaseGroup* const						blockGroup	= new TestCaseGroup(context, "unnamed_block", "");

		targetGroup->addChild(blockGroup);

		generateBufferReferencedByShaderInterfaceBlockCases(context, block, blockGroup, queryTarget, false);
	}

	// .block_array
	{
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(uniform));
		const ResourceDefinition::Node::SharedPtr	block			(new ResourceDefinition::InterfaceBlock(arrayElement, true));
		TestCaseGroup* const						blockGroup		= new TestCaseGroup(context, "block_array", "");

		targetGroup->addChild(blockGroup);

		generateBufferReferencedByShaderInterfaceBlockCases(context, block, blockGroup, queryTarget, false);
	}
}

static void generateReferencedByShaderCaseBlocks (Context& context, tcu::TestCaseGroup* const targetGroup, glu::GLSLVersion glslVersion, void (*generateBlockContent)(Context&, const ResourceDefinition::Node::SharedPtr&, tcu::TestCaseGroup*, int expandLevel))
{
	static const struct
	{
		const char*		name;
		glu::ShaderType	stage;
		int				expandLevel;
	} singleStageCases[] =
	{
		{ "compute",				glu::SHADERTYPE_COMPUTE,					3	},
		{ "separable_vertex",		glu::SHADERTYPE_VERTEX,						2	},
		{ "separable_fragment",		glu::SHADERTYPE_FRAGMENT,					2	},
		{ "separable_tess_ctrl",	glu::SHADERTYPE_TESSELLATION_CONTROL,		2	},
		{ "separable_tess_eval",	glu::SHADERTYPE_TESSELLATION_EVALUATION,	2	},
		{ "separable_geometry",		glu::SHADERTYPE_GEOMETRY,					2	},
	};
	static const struct
	{
		const char*	name;
		deUint32	flags;
		int			expandLevel;
		int			subExpandLevel;
	} pipelines[] =
	{
		{
			"vertex_fragment",
			(1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT),
			3,
			2,
		},
		{
			"vertex_tess_fragment",
			(1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT) | (1 << glu::SHADERTYPE_TESSELLATION_CONTROL) | (1 << glu::SHADERTYPE_TESSELLATION_EVALUATION),
			2,
			2,
		},
		{
			"vertex_geo_fragment",
			(1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT) | (1 << glu::SHADERTYPE_GEOMETRY),
			2,
			2,
		},
		{
			"vertex_tess_geo_fragment",
			(1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT) | (1 << glu::SHADERTYPE_TESSELLATION_CONTROL) | (1 << glu::SHADERTYPE_TESSELLATION_EVALUATION) | (1 << glu::SHADERTYPE_GEOMETRY),
			2,
			1,
		},
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(singleStageCases); ++ndx)
	{
		TestCaseGroup* const						blockGroup			= new TestCaseGroup(context, singleStageCases[ndx].name, "");
		const bool									programSeparable	= (singleStageCases[ndx].stage != glu::SHADERTYPE_COMPUTE);
		const ResourceDefinition::Node::SharedPtr	program				(new ResourceDefinition::Program(programSeparable));
		const ResourceDefinition::Node::SharedPtr	stage				(new ResourceDefinition::Shader(program, singleStageCases[ndx].stage, glslVersion));

		targetGroup->addChild(blockGroup);

		generateBlockContent(context, stage, blockGroup, singleStageCases[ndx].expandLevel);
	}

	for (int pipelineNdx = 0; pipelineNdx < DE_LENGTH_OF_ARRAY(pipelines); ++pipelineNdx)
	{
		// whole pipeline
		{
			TestCaseGroup* const						blockGroup			= new TestCaseGroup(context, pipelines[pipelineNdx].name, "");
			const ResourceDefinition::Node::SharedPtr	program				(new ResourceDefinition::Program());
			ResourceDefinition::ShaderSet*				shaderSet			= new ResourceDefinition::ShaderSet(program,
																												glslVersion,
																												pipelines[pipelineNdx].flags,
																												pipelines[pipelineNdx].flags);
			targetGroup->addChild(blockGroup);

			{
				const ResourceDefinition::Node::SharedPtr shaders(shaderSet);
				generateBlockContent(context, shaders, blockGroup, pipelines[pipelineNdx].expandLevel);
			}
		}

		// only one stage
		for (int selectedStageBit = 0; selectedStageBit < glu::SHADERTYPE_LAST; ++selectedStageBit)
		{
			if (pipelines[pipelineNdx].flags & (1 << selectedStageBit))
			{
				const ResourceDefinition::Node::SharedPtr	program		(new ResourceDefinition::Program());
				ResourceDefinition::ShaderSet*				shaderSet	= new ResourceDefinition::ShaderSet(program,
																											glslVersion,
																											pipelines[pipelineNdx].flags,
																											(1u << selectedStageBit));
				const char*									stageName	= (selectedStageBit == glu::SHADERTYPE_VERTEX)					? ("vertex")
																		: (selectedStageBit == glu::SHADERTYPE_FRAGMENT)				? ("fragment")
																		: (selectedStageBit == glu::SHADERTYPE_GEOMETRY)				? ("geo")
																		: (selectedStageBit == glu::SHADERTYPE_TESSELLATION_CONTROL)	? ("tess_ctrl")
																		: (selectedStageBit == glu::SHADERTYPE_TESSELLATION_EVALUATION)	? ("tess_eval")
																		: (DE_NULL);
				const std::string							setName		= std::string() + pipelines[pipelineNdx].name + "_only_" + stageName;
				TestCaseGroup* const						blockGroup	= new TestCaseGroup(context, setName.c_str(), "");
				const ResourceDefinition::Node::SharedPtr	shaders		(shaderSet);

				generateBlockContent(context, shaders, blockGroup, pipelines[pipelineNdx].subExpandLevel);
				targetGroup->addChild(blockGroup);
			}
		}
	}
}

static glu::DataType generateRandomDataType (de::Random& rnd, bool excludeOpaqueTypes)
{
	static const glu::DataType s_types[] =
	{
		glu::TYPE_FLOAT,
		glu::TYPE_INT,
		glu::TYPE_UINT,
		glu::TYPE_BOOL,
		glu::TYPE_FLOAT_VEC2,
		glu::TYPE_FLOAT_VEC3,
		glu::TYPE_FLOAT_VEC4,
		glu::TYPE_INT_VEC2,
		glu::TYPE_INT_VEC3,
		glu::TYPE_INT_VEC4,
		glu::TYPE_UINT_VEC2,
		glu::TYPE_UINT_VEC3,
		glu::TYPE_UINT_VEC4,
		glu::TYPE_BOOL_VEC2,
		glu::TYPE_BOOL_VEC3,
		glu::TYPE_BOOL_VEC4,
		glu::TYPE_FLOAT_MAT2,
		glu::TYPE_FLOAT_MAT2X3,
		glu::TYPE_FLOAT_MAT2X4,
		glu::TYPE_FLOAT_MAT3X2,
		glu::TYPE_FLOAT_MAT3,
		glu::TYPE_FLOAT_MAT3X4,
		glu::TYPE_FLOAT_MAT4X2,
		glu::TYPE_FLOAT_MAT4X3,
		glu::TYPE_FLOAT_MAT4,

		glu::TYPE_SAMPLER_2D,
		glu::TYPE_SAMPLER_CUBE,
		glu::TYPE_SAMPLER_2D_ARRAY,
		glu::TYPE_SAMPLER_3D,
		glu::TYPE_SAMPLER_2D_SHADOW,
		glu::TYPE_SAMPLER_CUBE_SHADOW,
		glu::TYPE_SAMPLER_2D_ARRAY_SHADOW,
		glu::TYPE_INT_SAMPLER_2D,
		glu::TYPE_INT_SAMPLER_CUBE,
		glu::TYPE_INT_SAMPLER_2D_ARRAY,
		glu::TYPE_INT_SAMPLER_3D,
		glu::TYPE_UINT_SAMPLER_2D,
		glu::TYPE_UINT_SAMPLER_CUBE,
		glu::TYPE_UINT_SAMPLER_2D_ARRAY,
		glu::TYPE_UINT_SAMPLER_3D,
		glu::TYPE_SAMPLER_2D_MULTISAMPLE,
		glu::TYPE_INT_SAMPLER_2D_MULTISAMPLE,
		glu::TYPE_UINT_SAMPLER_2D_MULTISAMPLE,
		glu::TYPE_IMAGE_2D,
		glu::TYPE_IMAGE_CUBE,
		glu::TYPE_IMAGE_2D_ARRAY,
		glu::TYPE_IMAGE_3D,
		glu::TYPE_INT_IMAGE_2D,
		glu::TYPE_INT_IMAGE_CUBE,
		glu::TYPE_INT_IMAGE_2D_ARRAY,
		glu::TYPE_INT_IMAGE_3D,
		glu::TYPE_UINT_IMAGE_2D,
		glu::TYPE_UINT_IMAGE_CUBE,
		glu::TYPE_UINT_IMAGE_2D_ARRAY,
		glu::TYPE_UINT_IMAGE_3D,
		glu::TYPE_UINT_ATOMIC_COUNTER
	};

	for (;;)
	{
		const glu::DataType type = s_types[rnd.getInt(0, DE_LENGTH_OF_ARRAY(s_types)-1)];

		if (!excludeOpaqueTypes					||
			glu::isDataTypeScalarOrVector(type)	||
			glu::isDataTypeMatrix(type))
			return type;
	}
}

static ResourceDefinition::Node::SharedPtr generateRandomVariableDefinition (de::Random&								rnd,
																			 const ResourceDefinition::Node::SharedPtr&	parentStructure,
																			 glu::DataType								baseType,
																			 const glu::Layout&							layout,
																			 bool										allowUnsized)
{
	const int							maxNesting			= 4;
	ResourceDefinition::Node::SharedPtr	currentStructure	= parentStructure;
	const bool							canBeInsideAStruct	= layout.binding == -1 && !isDataTypeLayoutQualified(baseType);

	for (int nestNdx = 0; nestNdx < maxNesting; ++nestNdx)
	{
		if (allowUnsized && nestNdx == 0 && rnd.getFloat() < 0.2)
			currentStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::ArrayElement(currentStructure, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
		else if (rnd.getFloat() < 0.3 && canBeInsideAStruct)
			currentStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::StructMember(currentStructure));
		else if (rnd.getFloat() < 0.3)
			currentStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::ArrayElement(currentStructure));
		else
			break;
	}

	return ResourceDefinition::Node::SharedPtr(new ResourceDefinition::Variable(currentStructure, baseType));
}

static ResourceDefinition::Node::SharedPtr generateRandomCoreShaderSet (de::Random& rnd, glu::GLSLVersion glslVersion)
{
	if (rnd.getFloat() < 0.5f)
	{
		// compute only
		const ResourceDefinition::Node::SharedPtr program(new ResourceDefinition::Program());
		return ResourceDefinition::Node::SharedPtr(new ResourceDefinition::Shader(program, glu::SHADERTYPE_COMPUTE, glslVersion));
	}
	else if (rnd.getFloat() < 0.5f)
	{
		// vertex and fragment
		const ResourceDefinition::Node::SharedPtr	program		(new ResourceDefinition::Program());
		ResourceDefinition::ShaderSet*				shaderSet	= new ResourceDefinition::ShaderSet(program, glslVersion);

		if (rnd.getBool())
		{
			shaderSet->setStage(glu::SHADERTYPE_VERTEX, true);
			shaderSet->setStage(glu::SHADERTYPE_FRAGMENT, rnd.getBool());
		}
		else
		{
			shaderSet->setStage(glu::SHADERTYPE_VERTEX, rnd.getBool());
			shaderSet->setStage(glu::SHADERTYPE_FRAGMENT, true);
		}

		return ResourceDefinition::Node::SharedPtr(shaderSet);
	}
	else
	{
		// separate vertex or fragment
		const ResourceDefinition::Node::SharedPtr	program		(new ResourceDefinition::Program(true));
		const glu::ShaderType						shaderType	= (rnd.getBool()) ? (glu::SHADERTYPE_VERTEX) : (glu::SHADERTYPE_FRAGMENT);

		return ResourceDefinition::Node::SharedPtr(new ResourceDefinition::Shader(program, shaderType, glslVersion));
	}
}

static ResourceDefinition::Node::SharedPtr generateRandomExtShaderSet (de::Random& rnd, glu::GLSLVersion glslVersion)
{
	if (rnd.getFloat() < 0.5f)
	{
		// whole pipeline
		const ResourceDefinition::Node::SharedPtr	program		(new ResourceDefinition::Program());
		ResourceDefinition::ShaderSet*				shaderSet	= new ResourceDefinition::ShaderSet(program, glslVersion);

		shaderSet->setStage(glu::SHADERTYPE_VERTEX, rnd.getBool());
		shaderSet->setStage(glu::SHADERTYPE_FRAGMENT, rnd.getBool());

		// tess shader are either both or neither present. Make cases interesting
		// by forcing one extended shader to always have reference
		if (rnd.getBool())
		{
			shaderSet->setStage(glu::SHADERTYPE_GEOMETRY, true);

			if (rnd.getBool())
			{
				shaderSet->setStage(glu::SHADERTYPE_TESSELLATION_CONTROL, rnd.getBool());
				shaderSet->setStage(glu::SHADERTYPE_TESSELLATION_EVALUATION, rnd.getBool());
			}
		}
		else
		{
			shaderSet->setStage(glu::SHADERTYPE_GEOMETRY, rnd.getBool());

			if (rnd.getBool())
			{
				shaderSet->setStage(glu::SHADERTYPE_TESSELLATION_CONTROL, true);
				shaderSet->setStage(glu::SHADERTYPE_TESSELLATION_EVALUATION, rnd.getBool());
			}
			else
			{
				shaderSet->setStage(glu::SHADERTYPE_TESSELLATION_CONTROL, rnd.getBool());
				shaderSet->setStage(glu::SHADERTYPE_TESSELLATION_EVALUATION, true);
			}
		}

		return ResourceDefinition::Node::SharedPtr(shaderSet);
	}
	else
	{
		// separate
		const ResourceDefinition::Node::SharedPtr	program		(new ResourceDefinition::Program(true));
		const int									selector	= rnd.getInt(0, 2);
		const glu::ShaderType						shaderType	= (selector == 0) ? (glu::SHADERTYPE_GEOMETRY)
																: (selector == 1) ? (glu::SHADERTYPE_TESSELLATION_CONTROL)
																: (selector == 2) ? (glu::SHADERTYPE_TESSELLATION_EVALUATION)
																:					(glu::SHADERTYPE_LAST);

		return ResourceDefinition::Node::SharedPtr(new ResourceDefinition::Shader(program, shaderType, glslVersion));
	}
}

static ResourceDefinition::Node::SharedPtr generateRandomShaderSet (de::Random& rnd, glu::GLSLVersion glslVersion, bool onlyExtensionStages)
{
	if (!onlyExtensionStages)
		return generateRandomCoreShaderSet(rnd, glslVersion);
	else
		return generateRandomExtShaderSet(rnd, glslVersion);
}

static glu::Layout generateRandomUniformBlockLayout (de::Random& rnd)
{
	glu::Layout layout;

	if (rnd.getBool())
		layout.binding = rnd.getInt(0, 5);

	if (rnd.getBool())
		layout.matrixOrder = (rnd.getBool()) ? (glu::MATRIXORDER_COLUMN_MAJOR) : (glu::MATRIXORDER_ROW_MAJOR);

	return layout;
}

static glu::Layout generateRandomBufferBlockLayout (de::Random& rnd)
{
	return generateRandomUniformBlockLayout(rnd);
}

static glu::Layout generateRandomVariableLayout (de::Random& rnd, glu::DataType type, bool interfaceBlockMember)
{
	glu::Layout layout;

	if ((glu::isDataTypeAtomicCounter(type) || glu::isDataTypeImage(type) || glu::isDataTypeSampler(type)) && rnd.getBool())
		layout.binding = rnd.getInt(0, 5);

	if (glu::isDataTypeAtomicCounter(type) && rnd.getBool())
		layout.offset = rnd.getInt(0, 3) * 4;

	if (glu::isDataTypeMatrix(type) && interfaceBlockMember && rnd.getBool())
		layout.matrixOrder = (rnd.getBool()) ? (glu::MATRIXORDER_COLUMN_MAJOR) : (glu::MATRIXORDER_ROW_MAJOR);

	return layout;
}

static void generateUniformRandomCase (Context& context, tcu::TestCaseGroup* const targetGroup, glu::GLSLVersion glslVersion, int index, bool onlyExtensionStages)
{
	de::Random									rnd					(index * 0x12345);
	const ResourceDefinition::Node::SharedPtr	shader				= generateRandomShaderSet(rnd, glslVersion, onlyExtensionStages);
	const bool									interfaceBlock		= rnd.getBool();
	const glu::DataType							type				= generateRandomDataType(rnd, interfaceBlock);
	const glu::Layout							layout				= generateRandomVariableLayout(rnd, type, interfaceBlock);
	const ResourceDefinition::Node::SharedPtr	defaultBlock		(new ResourceDefinition::DefaultBlock(shader));
	const ResourceDefinition::Node::SharedPtr	uniform				(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_UNIFORM));
	ResourceDefinition::Node::SharedPtr			currentStructure	= uniform;

	if (interfaceBlock)
	{
		const bool namedBlock = rnd.getBool();

		currentStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::LayoutQualifier(currentStructure, generateRandomUniformBlockLayout(rnd)));

		if (namedBlock && rnd.getBool())
			currentStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::ArrayElement(currentStructure));

		currentStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::InterfaceBlock(currentStructure, namedBlock));
	}

	currentStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::LayoutQualifier(currentStructure, layout));
	currentStructure = generateRandomVariableDefinition(rnd, currentStructure, type, layout, false);

	targetGroup->addChild(new ResourceTestCase(context, currentStructure, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_UNIFORM, PROGRAMRESOURCEPROP_UNIFORM_INTERFACE_MASK), de::toString(index).c_str()));
}

static void generateUniformCaseRandomCases (Context& context, tcu::TestCaseGroup* const targetGroup, glu::GLSLVersion glslVersion)
{
	const int numBasicCases		= 40;
	const int numTessGeoCases	= 40;

	for (int ndx = 0; ndx < numBasicCases; ++ndx)
		generateUniformRandomCase(context, targetGroup, glslVersion, ndx, false);
	for (int ndx = 0; ndx < numTessGeoCases; ++ndx)
		generateUniformRandomCase(context, targetGroup, glslVersion, numBasicCases + ndx, true);
}

class UniformInterfaceTestGroup : public TestCaseGroup
{
public:
			UniformInterfaceTestGroup	(Context& context);
	void	init						(void);
};

UniformInterfaceTestGroup::UniformInterfaceTestGroup (Context& context)
	: TestCaseGroup(context, "uniform", "Uniform interace")
{
}

void UniformInterfaceTestGroup::init (void)
{
	glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
	const ResourceDefinition::Node::SharedPtr	program			(new ResourceDefinition::Program());
	const ResourceDefinition::Node::SharedPtr	computeShader	(new ResourceDefinition::Shader(program, glu::SHADERTYPE_COMPUTE, glslVersion));

	// .resource_list
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "resource_list", "Resource list");
		addChild(blockGroup);
		generateUniformCaseBlocks(m_context, computeShader, blockGroup, BLOCKFLAG_ALL, generateUniformResourceListBlockContents);
	}

	// .array_size
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "array_size", "Query array size");
		addChild(blockGroup);
		generateUniformCaseBlocks(m_context, computeShader, blockGroup, BLOCKFLAG_ALL, generateUniformBlockArraySizeContents);
	}

	// .array_stride
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "array_stride", "Query array stride");
		addChild(blockGroup);
		generateUniformCaseBlocks(m_context, computeShader, blockGroup, BLOCKFLAG_ALL, generateUniformBlockArrayStrideContents);
	}

	// .atomic_counter_buffer_index
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "atomic_counter_buffer_index", "Query atomic counter buffer index");
		addChild(blockGroup);
		generateUniformCaseBlocks(m_context, computeShader, blockGroup, BLOCKFLAG_DEFAULT | BLOCKFLAG_NAMED, generateUniformBlockAtomicCounterBufferIndexContents);
	}

	// .block_index
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "block_index", "Query block index");
		addChild(blockGroup);
		generateUniformBlockBlockIndexContents(m_context, blockGroup, glslVersion);
	}

	// .location
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "location", "Query location");
		addChild(blockGroup);
		generateUniformCaseBlocks(m_context, computeShader, blockGroup, BLOCKFLAG_DEFAULT | BLOCKFLAG_NAMED | BLOCKFLAG_UNNAMED, generateUniformBlockLocationContents);
	}

	// .matrix_row_major
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "matrix_row_major", "Query matrix row_major");
		addChild(blockGroup);
		generateUniformMatrixCaseBlocks(m_context, computeShader, blockGroup, generateUniformMatrixOrderCaseBlockContentCases);
	}

	// .matrix_stride
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "matrix_stride", "Query matrix stride");
		addChild(blockGroup);
		generateUniformMatrixCaseBlocks(m_context, computeShader, blockGroup, generateUniformMatrixStrideCaseBlockContentCases);
	}

	// .name_length
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "name_length", "Query name length");
		addChild(blockGroup);
		generateUniformCaseBlocks(m_context, computeShader, blockGroup, BLOCKFLAG_ALL, generateUniformBlockNameLengthContents);
	}

	// .offset
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "offset", "Query offset");
		addChild(blockGroup);
		generateUniformCaseBlocks(m_context, computeShader, blockGroup, BLOCKFLAG_ALL, generateUniformBlockOffsetContents);
	}

	// .referenced_by_shader
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "referenced_by_shader", "Query referenced by shader");
		addChild(blockGroup);
		generateReferencedByShaderCaseBlocks(m_context, blockGroup, glslVersion, generateUniformReferencedByShaderSingleBlockContentCases);
	}

	// .type
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "type", "Query type");
		addChild(blockGroup);
		generateUniformCaseBlocks(m_context, computeShader, blockGroup, BLOCKFLAG_ALL, generateUniformBlockTypeContents);
	}

	// .random
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "random", "Random");
		addChild(blockGroup);
		generateUniformCaseRandomCases(m_context, blockGroup, glslVersion);
	}
}

static void generateBufferBackedInterfaceResourceListCase (Context& context, const ResourceDefinition::Node::SharedPtr& targetResource, tcu::TestCaseGroup* const targetGroup, ProgramInterface interface, const char* blockName)
{
	targetGroup->addChild(new ResourceListTestCase(context, targetResource, interface, blockName));
}

static void generateBufferBackedInterfaceNameLengthCase (Context& context, const ResourceDefinition::Node::SharedPtr& targetResource, tcu::TestCaseGroup* const targetGroup, ProgramInterface interface, const char* blockName)
{
	targetGroup->addChild(new ResourceTestCase(context, targetResource, ProgramResourceQueryTestTarget(interface, PROGRAMRESOURCEPROP_NAME_LENGTH), blockName));
}

static void generateBufferBackedInterfaceResourceBasicBlockTypes (Context& context, tcu::TestCaseGroup* targetGroup, glu::GLSLVersion glslVersion, glu::Storage storage, void (*blockContentGenerator)(Context&, const ResourceDefinition::Node::SharedPtr&, tcu::TestCaseGroup* const, ProgramInterface interface, const char* blockName))
{
	const ResourceDefinition::Node::SharedPtr	program				(new ResourceDefinition::Program());
	const ResourceDefinition::Node::SharedPtr	shader				(new ResourceDefinition::Shader(program, glu::SHADERTYPE_COMPUTE, glslVersion));
	const ResourceDefinition::Node::SharedPtr	defaultBlock		(new ResourceDefinition::DefaultBlock(shader));
	const ResourceDefinition::Node::SharedPtr	storageQualifier	(new ResourceDefinition::StorageQualifier(defaultBlock, storage));
	const ResourceDefinition::Node::SharedPtr	binding				(new ResourceDefinition::LayoutQualifier(storageQualifier, glu::Layout(-1, 1)));
	const ProgramInterface						programInterface	= (storage == glu::STORAGE_UNIFORM) ? (PROGRAMINTERFACE_UNIFORM_BLOCK) : (PROGRAMINTERFACE_SHADER_STORAGE_BLOCK);

	// .named_block
	{
		const ResourceDefinition::Node::SharedPtr block			(new ResourceDefinition::InterfaceBlock(binding, true));
		const ResourceDefinition::Node::SharedPtr dummyVariable	(new ResourceDefinition::Variable(block, glu::TYPE_BOOL_VEC3));

		blockContentGenerator(context, dummyVariable, targetGroup, programInterface, "named_block");
	}

	// .unnamed_block
	{
		const ResourceDefinition::Node::SharedPtr block			(new ResourceDefinition::InterfaceBlock(binding, false));
		const ResourceDefinition::Node::SharedPtr dummyVariable	(new ResourceDefinition::Variable(block, glu::TYPE_BOOL_VEC3));

		blockContentGenerator(context, dummyVariable, targetGroup, programInterface, "unnamed_block");
	}

	// .block_array
	{
		const ResourceDefinition::Node::SharedPtr arrayElement	(new ResourceDefinition::ArrayElement(binding, 3));
		const ResourceDefinition::Node::SharedPtr block			(new ResourceDefinition::InterfaceBlock(arrayElement, true));
		const ResourceDefinition::Node::SharedPtr dummyVariable	(new ResourceDefinition::Variable(block, glu::TYPE_BOOL_VEC3));

		blockContentGenerator(context, dummyVariable, targetGroup, programInterface, "block_array");
	}

	// .block_array_single_element
	{
		const ResourceDefinition::Node::SharedPtr arrayElement	(new ResourceDefinition::ArrayElement(binding, 1));
		const ResourceDefinition::Node::SharedPtr block			(new ResourceDefinition::InterfaceBlock(arrayElement, true));
		const ResourceDefinition::Node::SharedPtr dummyVariable	(new ResourceDefinition::Variable(block, glu::TYPE_BOOL_VEC3));

		blockContentGenerator(context, dummyVariable, targetGroup, programInterface, "block_array_single_element");
	}
}

static void generateBufferBackedInterfaceResourceBufferBindingCases (Context& context, tcu::TestCaseGroup* targetGroup, glu::GLSLVersion glslVersion, glu::Storage storage)
{
	const ResourceDefinition::Node::SharedPtr	program				(new ResourceDefinition::Program());
	const ResourceDefinition::Node::SharedPtr	shader				(new ResourceDefinition::Shader(program, glu::SHADERTYPE_COMPUTE, glslVersion));
	const ResourceDefinition::Node::SharedPtr	defaultBlock		(new ResourceDefinition::DefaultBlock(shader));
	const ResourceDefinition::Node::SharedPtr	storageQualifier	(new ResourceDefinition::StorageQualifier(defaultBlock, storage));

	for (int ndx = 0; ndx < 2; ++ndx)
	{
		const bool									explicitBinding		= (ndx == 1);
		const int									bindingNdx			= (explicitBinding) ? (1) : (-1);
		const std::string							nameSuffix			= (explicitBinding) ? ("_explicit_binding") : ("");
		const ResourceDefinition::Node::SharedPtr	binding				(new ResourceDefinition::LayoutQualifier(storageQualifier, glu::Layout(-1, bindingNdx)));
		const ProgramInterface						programInterface	= (storage == glu::STORAGE_UNIFORM) ? (PROGRAMINTERFACE_UNIFORM_BLOCK) : (PROGRAMINTERFACE_SHADER_STORAGE_BLOCK);

		// .named_block*
		{
			const ResourceDefinition::Node::SharedPtr block			(new ResourceDefinition::InterfaceBlock(binding, true));
			const ResourceDefinition::Node::SharedPtr dummyVariable	(new ResourceDefinition::Variable(block, glu::TYPE_BOOL_VEC3));

			targetGroup->addChild(new ResourceTestCase(context, dummyVariable, ProgramResourceQueryTestTarget(programInterface, PROGRAMRESOURCEPROP_BUFFER_BINDING), ("named_block" + nameSuffix).c_str()));
		}

		// .unnamed_block*
		{
			const ResourceDefinition::Node::SharedPtr block			(new ResourceDefinition::InterfaceBlock(binding, false));
			const ResourceDefinition::Node::SharedPtr dummyVariable	(new ResourceDefinition::Variable(block, glu::TYPE_BOOL_VEC3));

			targetGroup->addChild(new ResourceTestCase(context, dummyVariable, ProgramResourceQueryTestTarget(programInterface, PROGRAMRESOURCEPROP_BUFFER_BINDING), ("unnamed_block" + nameSuffix).c_str()));
		}

		// .block_array*
		{
			const ResourceDefinition::Node::SharedPtr arrayElement	(new ResourceDefinition::ArrayElement(binding, 3));
			const ResourceDefinition::Node::SharedPtr block			(new ResourceDefinition::InterfaceBlock(arrayElement, true));
			const ResourceDefinition::Node::SharedPtr dummyVariable	(new ResourceDefinition::Variable(block, glu::TYPE_BOOL_VEC3));

			targetGroup->addChild(new ResourceTestCase(context, dummyVariable, ProgramResourceQueryTestTarget(programInterface, PROGRAMRESOURCEPROP_BUFFER_BINDING), ("block_array" + nameSuffix).c_str()));
		}
	}
}

template <glu::Storage Storage>
static void generateBufferBlockReferencedByShaderSingleBlockContentCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, int expandLevel)
{
	const ProgramInterface						programInterface	= (Storage == glu::STORAGE_UNIFORM) ? (PROGRAMINTERFACE_UNIFORM_BLOCK) :
																      (Storage == glu::STORAGE_BUFFER) ? (PROGRAMINTERFACE_SHADER_STORAGE_BLOCK) :
																      (PROGRAMINTERFACE_LAST);
	const ResourceDefinition::Node::SharedPtr	defaultBlock		(new ResourceDefinition::DefaultBlock(parentStructure));
	const ResourceDefinition::Node::SharedPtr	storage				(new ResourceDefinition::StorageQualifier(defaultBlock, Storage));

	DE_UNREF(expandLevel);

	DE_ASSERT(programInterface != PROGRAMINTERFACE_LAST);

	// .named_block
	{
		const ResourceDefinition::Node::SharedPtr block			(new ResourceDefinition::InterfaceBlock(storage, true));
		const ResourceDefinition::Node::SharedPtr dummyVariable	(new ResourceDefinition::Variable(block, glu::TYPE_BOOL_VEC3));

		targetGroup->addChild(new ResourceTestCase(context, dummyVariable, ProgramResourceQueryTestTarget(programInterface, PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER), "named_block"));
	}

	// .unnamed_block
	{
		const ResourceDefinition::Node::SharedPtr block			(new ResourceDefinition::InterfaceBlock(storage, false));
		const ResourceDefinition::Node::SharedPtr dummyVariable	(new ResourceDefinition::Variable(block, glu::TYPE_BOOL_VEC3));

		targetGroup->addChild(new ResourceTestCase(context, dummyVariable, ProgramResourceQueryTestTarget(programInterface, PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER), "unnamed_block"));
	}

	// .block_array
	{
		const ResourceDefinition::Node::SharedPtr arrayElement	(new ResourceDefinition::ArrayElement(storage, 3));
		const ResourceDefinition::Node::SharedPtr block			(new ResourceDefinition::InterfaceBlock(arrayElement, true));
		const ResourceDefinition::Node::SharedPtr dummyVariable	(new ResourceDefinition::Variable(block, glu::TYPE_BOOL_VEC3));

		targetGroup->addChild(new ResourceTestCase(context, dummyVariable, ProgramResourceQueryTestTarget(programInterface, PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER), "block_array"));
	}
}

static void generateBufferBackedInterfaceResourceActiveVariablesCase (Context& context, tcu::TestCaseGroup* targetGroup, glu::Storage storage)
{
	targetGroup->addChild(new InterfaceBlockActiveVariablesTestCase(context, "named_block",		"Named block",		storage,	InterfaceBlockActiveVariablesTestCase::CASE_NAMED_BLOCK));
	targetGroup->addChild(new InterfaceBlockActiveVariablesTestCase(context, "unnamed_block",	"Unnamed block",	storage,	InterfaceBlockActiveVariablesTestCase::CASE_UNNAMED_BLOCK));
	targetGroup->addChild(new InterfaceBlockActiveVariablesTestCase(context, "block_array",		"Block array",		storage,	InterfaceBlockActiveVariablesTestCase::CASE_BLOCK_ARRAY));
}

static void generateBufferBackedInterfaceResourceBufferDataSizeCases (Context& context, tcu::TestCaseGroup* targetGroup, glu::Storage storage)
{
	targetGroup->addChild(new InterfaceBlockDataSizeTestCase(context, "named_block",	"Named block",		storage,	InterfaceBlockDataSizeTestCase::CASE_NAMED_BLOCK));
	targetGroup->addChild(new InterfaceBlockDataSizeTestCase(context, "unnamed_block",	"Unnamed block",	storage,	InterfaceBlockDataSizeTestCase::CASE_UNNAMED_BLOCK));
	targetGroup->addChild(new InterfaceBlockDataSizeTestCase(context, "block_array",	"Block array",		storage,	InterfaceBlockDataSizeTestCase::CASE_BLOCK_ARRAY));
}

class BufferBackedBlockInterfaceTestGroup : public TestCaseGroup
{
public:
						BufferBackedBlockInterfaceTestGroup	(Context& context, glu::Storage interfaceBlockStorage);
	void				init								(void);

private:
	static const char*	getGroupName						(glu::Storage storage);
	static const char*	getGroupDescription					(glu::Storage storage);

	const glu::Storage	m_storage;
};

BufferBackedBlockInterfaceTestGroup::BufferBackedBlockInterfaceTestGroup(Context& context, glu::Storage storage)
	: TestCaseGroup	(context, getGroupName(storage), getGroupDescription(storage))
	, m_storage		(storage)
{
	DE_ASSERT(storage == glu::STORAGE_BUFFER || storage == glu::STORAGE_UNIFORM);
}

void BufferBackedBlockInterfaceTestGroup::init (void)
{
	const glu::GLSLVersion	glslVersion	= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());

	// .resource_list
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "resource_list", "Resource list");
		addChild(blockGroup);
		generateBufferBackedInterfaceResourceBasicBlockTypes(m_context, blockGroup, glslVersion, m_storage, generateBufferBackedInterfaceResourceListCase);
	}

	// .active_variables
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "active_variables", "Active variables");
		addChild(blockGroup);
		generateBufferBackedInterfaceResourceActiveVariablesCase(m_context, blockGroup, m_storage);
	}

	// .buffer_binding
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "buffer_binding", "Buffer binding");
		addChild(blockGroup);
		generateBufferBackedInterfaceResourceBufferBindingCases(m_context, blockGroup, glslVersion, m_storage);
	}

	// .buffer_data_size
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "buffer_data_size", "Buffer data size");
		addChild(blockGroup);
		generateBufferBackedInterfaceResourceBufferDataSizeCases(m_context, blockGroup, m_storage);
	}

	// .name_length
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "name_length", "Name length");
		addChild(blockGroup);
		generateBufferBackedInterfaceResourceBasicBlockTypes(m_context, blockGroup, glslVersion, m_storage, generateBufferBackedInterfaceNameLengthCase);
	}

	// .referenced_by
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "referenced_by", "Referenced by shader");
		addChild(blockGroup);

		if (m_storage == glu::STORAGE_UNIFORM)
			generateReferencedByShaderCaseBlocks(m_context, blockGroup, glslVersion, generateBufferBlockReferencedByShaderSingleBlockContentCases<glu::STORAGE_UNIFORM>);
		else if (m_storage == glu::STORAGE_BUFFER)
			generateReferencedByShaderCaseBlocks(m_context, blockGroup, glslVersion, generateBufferBlockReferencedByShaderSingleBlockContentCases<glu::STORAGE_BUFFER>);
		else
			DE_ASSERT(false);
	}
}

const char* BufferBackedBlockInterfaceTestGroup::getGroupName (glu::Storage storage)
{
	switch (storage)
	{
		case glu::STORAGE_UNIFORM:	return "uniform_block";
		case glu::STORAGE_BUFFER:	return "shader_storage_block";
		default:
			DE_ASSERT("false");
			return DE_NULL;
	}
}

const char* BufferBackedBlockInterfaceTestGroup::getGroupDescription (glu::Storage storage)
{
	switch (storage)
	{
		case glu::STORAGE_UNIFORM:	return "Uniform block interface";
		case glu::STORAGE_BUFFER:	return "Shader storage block interface";
		default:
			DE_ASSERT("false");
			return DE_NULL;
	}
}

class AtomicCounterTestGroup : public TestCaseGroup
{
public:
			AtomicCounterTestGroup	(Context& context);
	void	init					(void);
};

AtomicCounterTestGroup::AtomicCounterTestGroup (Context& context)
	: TestCaseGroup(context, "atomic_counter_buffer", "Atomic counter buffer")
{
}

void AtomicCounterTestGroup::init (void)
{
	static const struct
	{
		const char*	name;
		deUint32	flags;
	} pipelines[] =
	{
		{
			"vertex_fragment",
			(1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT)
		},
		{
			"vertex_tess_fragment",
			(1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT) | (1 << glu::SHADERTYPE_TESSELLATION_CONTROL) | (1 << glu::SHADERTYPE_TESSELLATION_EVALUATION)
		},
		{
			"vertex_geo_fragment",
			(1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT) | (1 << glu::SHADERTYPE_GEOMETRY)
		},
		{
			"vertex_tess_geo_fragment",
			(1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT) | (1 << glu::SHADERTYPE_TESSELLATION_CONTROL) | (1 << glu::SHADERTYPE_TESSELLATION_EVALUATION) | (1 << glu::SHADERTYPE_GEOMETRY),
		},
	};

	// .resource_list
	addChild(new AtomicCounterResourceListCase(m_context, "resource_list", "Resource list"));

	// .active_variables
	addChild(new AtomicCounterActiveVariablesCase(m_context, "active_variables", "Active variables"));

	// .buffer_binding
	addChild(new AtomicCounterBufferBindingCase(m_context, "buffer_binding", "Buffer binding"));

	// .buffer_data_size
	addChild(new AtomicCounterBufferDataSizeCase(m_context, "buffer_data_size", "Buffer binding"));

	// .referenced_by
	addChild(new AtomicCounterReferencedByCase(m_context, "referenced_by_compute",				"",	false,	(1 << glu::SHADERTYPE_COMPUTE),										(1 << glu::SHADERTYPE_COMPUTE)));
	addChild(new AtomicCounterReferencedByCase(m_context, "referenced_by_separable_vertex",		"",	true,	(1 << glu::SHADERTYPE_VERTEX),										(1 << glu::SHADERTYPE_VERTEX)));
	addChild(new AtomicCounterReferencedByCase(m_context, "referenced_by_separable_fragment",	"",	true,	(1 << glu::SHADERTYPE_FRAGMENT),									(1 << glu::SHADERTYPE_FRAGMENT)));
	addChild(new AtomicCounterReferencedByCase(m_context, "referenced_by_separable_geometry",	"",	true,	(1 << glu::SHADERTYPE_GEOMETRY),									(1 << glu::SHADERTYPE_GEOMETRY)));
	addChild(new AtomicCounterReferencedByCase(m_context, "referenced_by_separable_tess_ctrl",	"",	true,	(1 << glu::SHADERTYPE_TESSELLATION_CONTROL),						(1 << glu::SHADERTYPE_TESSELLATION_CONTROL)));
	addChild(new AtomicCounterReferencedByCase(m_context, "referenced_by_separable_tess_eval",	"",	true,	(1 << glu::SHADERTYPE_TESSELLATION_EVALUATION),						(1 << glu::SHADERTYPE_TESSELLATION_EVALUATION)));

	for (int pipelineNdx = 0; pipelineNdx < DE_LENGTH_OF_ARRAY(pipelines); ++pipelineNdx)
	{
		addChild(new AtomicCounterReferencedByCase(m_context, (std::string() + "referenced_by_" + pipelines[pipelineNdx].name).c_str(), "", false, pipelines[pipelineNdx].flags, pipelines[pipelineNdx].flags));

		for (deUint32 stageNdx = 0; stageNdx < glu::SHADERTYPE_LAST; ++stageNdx)
		{
			const deUint32 currentBit = (1u << stageNdx);
			if (currentBit > pipelines[pipelineNdx].flags)
				break;
			if (currentBit & pipelines[pipelineNdx].flags)
			{
				const char*			stageName	= (stageNdx == glu::SHADERTYPE_VERTEX)					? ("vertex")
												: (stageNdx == glu::SHADERTYPE_FRAGMENT)				? ("fragment")
												: (stageNdx == glu::SHADERTYPE_GEOMETRY)				? ("geo")
												: (stageNdx == glu::SHADERTYPE_TESSELLATION_CONTROL)	? ("tess_ctrl")
												: (stageNdx == glu::SHADERTYPE_TESSELLATION_EVALUATION)	? ("tess_eval")
												: (DE_NULL);
				const std::string	name		= std::string() + "referenced_by_" + pipelines[pipelineNdx].name + "_only_" + stageName;

				addChild(new AtomicCounterReferencedByCase(m_context, name.c_str(), "", false, pipelines[pipelineNdx].flags, currentBit));
			}
		}
	}
}

static void generateProgramInputOutputShaderCaseBlocks (Context& context, tcu::TestCaseGroup* targetGroup, glu::GLSLVersion glslVersion, bool withCompute, bool inputCase, void (*blockContentGenerator)(Context&, const ResourceDefinition::Node::SharedPtr&, tcu::TestCaseGroup*, deUint32))
{
	static const struct
	{
		const char*		name;
		glu::ShaderType	stage;
	} singleStageCases[] =
	{
		{ "separable_vertex",		glu::SHADERTYPE_VERTEX					},
		{ "separable_fragment",		glu::SHADERTYPE_FRAGMENT				},
		{ "separable_tess_ctrl",	glu::SHADERTYPE_TESSELLATION_CONTROL	},
		{ "separable_tess_eval",	glu::SHADERTYPE_TESSELLATION_EVALUATION	},
		{ "separable_geometry",		glu::SHADERTYPE_GEOMETRY				},
	};

	// .vertex_fragment
	{
		tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "vertex_fragment", "Vertex and fragment");
		const ResourceDefinition::Node::SharedPtr	program			(new ResourceDefinition::Program(false));
		ResourceDefinition::ShaderSet*				shaderSetPtr	= new ResourceDefinition::ShaderSet(program, glslVersion);
		const ResourceDefinition::Node::SharedPtr	shaderSet		(shaderSetPtr);
		const ResourceDefinition::Node::SharedPtr	defaultBlock	(new ResourceDefinition::DefaultBlock(shaderSet));

		shaderSetPtr->setStage(glu::SHADERTYPE_VERTEX, inputCase);
		shaderSetPtr->setStage(glu::SHADERTYPE_FRAGMENT, !inputCase);

		targetGroup->addChild(blockGroup);

		blockContentGenerator(context, defaultBlock, blockGroup, (1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT));
	}

	// .separable_*
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(singleStageCases); ++ndx)
	{
		TestCaseGroup* const						blockGroup			= new TestCaseGroup(context, singleStageCases[ndx].name, "");
		const ResourceDefinition::Node::SharedPtr	program				(new ResourceDefinition::Program(true));
		const ResourceDefinition::Node::SharedPtr	shader				(new ResourceDefinition::Shader(program, singleStageCases[ndx].stage, glslVersion));
		const ResourceDefinition::Node::SharedPtr	defaultBlock		(new ResourceDefinition::DefaultBlock(shader));

		targetGroup->addChild(blockGroup);
		blockContentGenerator(context, defaultBlock, blockGroup, (1 << singleStageCases[ndx].stage));
	}

	// .compute
	if (withCompute)
	{
		tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "compute", "Compute");
		const ResourceDefinition::Node::SharedPtr	program			(new ResourceDefinition::Program(true));
		const ResourceDefinition::Node::SharedPtr	shader			(new ResourceDefinition::Shader(program, glu::SHADERTYPE_COMPUTE, glslVersion));
		const ResourceDefinition::Node::SharedPtr	defaultBlock	(new ResourceDefinition::DefaultBlock(shader));

		targetGroup->addChild(blockGroup);

		blockContentGenerator(context, defaultBlock, blockGroup, (1 << glu::SHADERTYPE_COMPUTE));
	}

	// .interface_blocks
	{
		static const struct
		{
			const char*			inputName;
			glu::ShaderType		inputStage;
			glu::Storage		inputStorage;
			const char*			outputName;
			glu::ShaderType		outputStage;
			glu::Storage		outputStorage;
		} ioBlockTypes[] =
		{
			{
				"in",
				glu::SHADERTYPE_FRAGMENT,
				glu::STORAGE_IN,
				"out",
				glu::SHADERTYPE_VERTEX,
				glu::STORAGE_OUT,
			},
			{
				"patch_in",
				glu::SHADERTYPE_TESSELLATION_EVALUATION,
				glu::STORAGE_PATCH_IN,
				"patch_out",
				glu::SHADERTYPE_TESSELLATION_CONTROL,
				glu::STORAGE_PATCH_OUT,
			},
		};

		tcu::TestCaseGroup* const ioBlocksGroup = new TestCaseGroup(context, "interface_blocks", "Interface blocks");
		targetGroup->addChild(ioBlocksGroup);

		// .in/out
		// .sample in/out
		// .patch in/out
		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(ioBlockTypes); ++ndx)
		{
			const char* const							name			= (inputCase) ? (ioBlockTypes[ndx].inputName) : (ioBlockTypes[ndx].outputName);
			const glu::ShaderType						shaderType		= (inputCase) ? (ioBlockTypes[ndx].inputStage) : (ioBlockTypes[ndx].outputStage);
			const glu::Storage							storageType		= (inputCase) ? (ioBlockTypes[ndx].inputStorage) : (ioBlockTypes[ndx].outputStorage);
			tcu::TestCaseGroup* const					ioBlockGroup	= new TestCaseGroup(context, name, "");
			const ResourceDefinition::Node::SharedPtr	program			(new ResourceDefinition::Program(true));
			const ResourceDefinition::Node::SharedPtr	shader			(new ResourceDefinition::Shader(program, shaderType, glslVersion));
			const ResourceDefinition::Node::SharedPtr	defaultBlock	(new ResourceDefinition::DefaultBlock(shader));
			const ResourceDefinition::Node::SharedPtr	storage			(new ResourceDefinition::StorageQualifier(defaultBlock, storageType));

			ioBlocksGroup->addChild(ioBlockGroup);

			// .named_block
			{
				const ResourceDefinition::Node::SharedPtr	block		(new ResourceDefinition::InterfaceBlock(storage, true));
				tcu::TestCaseGroup* const					blockGroup	= new TestCaseGroup(context, "named_block", "Named block");

				ioBlockGroup->addChild(blockGroup);

				blockContentGenerator(context, block, blockGroup, (1 << shaderType));
			}

			// .named_block_explicit_location
			{
				const ResourceDefinition::Node::SharedPtr	layout		(new ResourceDefinition::LayoutQualifier(storage, glu::Layout(3)));
				const ResourceDefinition::Node::SharedPtr	block		(new ResourceDefinition::InterfaceBlock(layout, true));
				tcu::TestCaseGroup* const					blockGroup	= new TestCaseGroup(context, "named_block_explicit_location", "Named block with explicit location");

				ioBlockGroup->addChild(blockGroup);

				blockContentGenerator(context, block, blockGroup, (1 << shaderType));
			}

			// .unnamed_block
			{
				const ResourceDefinition::Node::SharedPtr	block		(new ResourceDefinition::InterfaceBlock(storage, false));
				tcu::TestCaseGroup* const					blockGroup	= new TestCaseGroup(context, "unnamed_block", "Unnamed block");

				ioBlockGroup->addChild(blockGroup);

				blockContentGenerator(context, block, blockGroup, (1 << shaderType));
			}

			// .block_array
			{
				const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(storage));
				const ResourceDefinition::Node::SharedPtr	block			(new ResourceDefinition::InterfaceBlock(arrayElement, true));
				tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "block_array", "Block array");

				ioBlockGroup->addChild(blockGroup);

				blockContentGenerator(context, block, blockGroup, (1 << shaderType));
			}
		}
	}
}

static void generateProgramInputBlockContents (Context&										context,
											   const ResourceDefinition::Node::SharedPtr&	parentStructure,
											   tcu::TestCaseGroup*							targetGroup,
											   deUint32										presentShadersMask,
											   bool											includeEmpty,
											   void											(*genCase)(Context&										context,
																									   const ResourceDefinition::Node::SharedPtr&	parentStructure,
																									   tcu::TestCaseGroup*							targetGroup,
																									   ProgramInterface								interface,
																									   const char*									name))
{
	const bool									inDefaultBlock	= parentStructure->getType() == ResourceDefinition::Node::TYPE_DEFAULT_BLOCK;
	const ResourceDefinition::Node::SharedPtr	input			= (inDefaultBlock)
																	? (ResourceDefinition::Node::SharedPtr(new ResourceDefinition::StorageQualifier(parentStructure, glu::STORAGE_IN)))
																	: (parentStructure);
	const glu::ShaderType						firstStage		= getShaderMaskFirstStage(presentShadersMask);

	// .empty
	if (includeEmpty && inDefaultBlock)
		genCase(context, parentStructure, targetGroup, PROGRAMINTERFACE_PROGRAM_INPUT, "empty");

	if (firstStage == glu::SHADERTYPE_VERTEX)
	{
		// .var
		const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(input, glu::TYPE_FLOAT_VEC4));
		genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_INPUT, "var");
	}
	else if (firstStage == glu::SHADERTYPE_FRAGMENT || !inDefaultBlock)
	{
		// .var
		{
			const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(input, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_INPUT, "var");
		}
		// .var_struct
		{
			const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(input));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(structMbr, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_INPUT, "var_struct");
		}
		// .var_array
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(input));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_INPUT, "var_array");
		}
	}
	else if (firstStage == glu::SHADERTYPE_TESSELLATION_CONTROL ||
			 firstStage == glu::SHADERTYPE_GEOMETRY)
	{
		// arrayed interface

		// .var
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(input, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_INPUT, "var");
		}
		// extension forbids use arrays of structs
		// extension forbids use arrays of arrays
	}
	else if (firstStage == glu::SHADERTYPE_TESSELLATION_EVALUATION)
	{
		// arrayed interface
		const ResourceDefinition::Node::SharedPtr patchInput(new ResourceDefinition::StorageQualifier(parentStructure, glu::STORAGE_PATCH_IN));

		// .var
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(input, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_INPUT, "var");
		}
		// extension forbids use arrays of structs
		// extension forbids use arrays of arrays

		// .patch_var
		{
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(patchInput, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_INPUT, "patch_var");
		}
		// .patch_var_struct
		{
			const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(patchInput));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(structMbr, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_INPUT, "patch_var_struct");
		}
		// .patch_var_array
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(patchInput));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_INPUT, "patch_var_array");
		}
	}
	else if (firstStage == glu::SHADERTYPE_COMPUTE)
	{
		// nada
	}
	else
		DE_ASSERT(false);
}

static void generateProgramOutputBlockContents (Context&										context,
												const ResourceDefinition::Node::SharedPtr&		parentStructure,
												tcu::TestCaseGroup*								targetGroup,
												deUint32										presentShadersMask,
												bool											includeEmpty,
												void											(*genCase)(Context&										context,
																										   const ResourceDefinition::Node::SharedPtr&	parentStructure,
																										   tcu::TestCaseGroup*							targetGroup,
																										   ProgramInterface								interface,
																										   const char*									name))
{
	const bool									inDefaultBlock	= parentStructure->getType() == ResourceDefinition::Node::TYPE_DEFAULT_BLOCK;
	const ResourceDefinition::Node::SharedPtr	output			= (inDefaultBlock)
																	? (ResourceDefinition::Node::SharedPtr(new ResourceDefinition::StorageQualifier(parentStructure, glu::STORAGE_OUT)))
																	: (parentStructure);
	const glu::ShaderType						lastStage		= getShaderMaskLastStage(presentShadersMask);

	// .empty
	if (includeEmpty && inDefaultBlock)
		genCase(context, parentStructure, targetGroup, PROGRAMINTERFACE_PROGRAM_OUTPUT, "empty");

	if (lastStage == glu::SHADERTYPE_VERTEX						||
		lastStage == glu::SHADERTYPE_GEOMETRY					||
		lastStage == glu::SHADERTYPE_TESSELLATION_EVALUATION	||
		!inDefaultBlock)
	{
		// .var
		{
			const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(output, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_OUTPUT, "var");
		}
		// .var_struct
		{
			const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(output));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(structMbr, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_OUTPUT, "var_struct");
		}
		// .var_array
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(output));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_OUTPUT, "var_array");
		}
	}
	else if (lastStage == glu::SHADERTYPE_FRAGMENT)
	{
		// .var
		{
			const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(output, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_OUTPUT, "var");
		}
		// .var_array
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(output));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_OUTPUT, "var_array");
		}
	}
	else if (lastStage == glu::SHADERTYPE_TESSELLATION_CONTROL)
	{
		// arrayed interface
		const ResourceDefinition::Node::SharedPtr patchOutput(new ResourceDefinition::StorageQualifier(parentStructure, glu::STORAGE_PATCH_OUT));

		// .var
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(output, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_OUTPUT, "var");
		}
		// extension forbids use arrays of structs
		// extension forbids use array of arrays

		// .patch_var
		{
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(patchOutput, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_OUTPUT, "patch_var");
		}
		// .patch_var_struct
		{
			const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(patchOutput));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(structMbr, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_OUTPUT, "patch_var_struct");
		}
		// .patch_var_array
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(patchOutput));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			genCase(context, variable, targetGroup, PROGRAMINTERFACE_PROGRAM_OUTPUT, "patch_var_array");
		}
	}
	else if (lastStage == glu::SHADERTYPE_COMPUTE)
	{
		// nada
	}
	else
		DE_ASSERT(false);
}

static void addProgramInputOutputResourceListCase (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, ProgramInterface programInterface, const char* name)
{
	ResourceListTestCase* const resourceListCase = new ResourceListTestCase(context, parentStructure, programInterface);

	DE_ASSERT(deStringEqual(name, resourceListCase->getName()));
	DE_UNREF(name);
	targetGroup->addChild(resourceListCase);
}

static void generateProgramInputResourceListBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, deUint32 presentShadersMask)
{
	generateProgramInputBlockContents(context, parentStructure, targetGroup, presentShadersMask, true, addProgramInputOutputResourceListCase);
}

static void generateProgramOutputResourceListBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, deUint32 presentShadersMask)
{
	generateProgramOutputBlockContents(context, parentStructure, targetGroup, presentShadersMask, true, addProgramInputOutputResourceListCase);
}

template <ProgramResourcePropFlags TargetProp>
static void addProgramInputOutputResourceTestCase (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, ProgramInterface programInterface, const char* name)
{
	ResourceTestCase* const resourceTestCase = new ResourceTestCase(context, parentStructure, ProgramResourceQueryTestTarget(programInterface, TargetProp), name);
	targetGroup->addChild(resourceTestCase);
}

template <ProgramResourcePropFlags TargetProp>
static void generateProgramInputBasicBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, deUint32 presentShadersMask)
{
	generateProgramInputBlockContents(context, parentStructure, targetGroup, presentShadersMask, false, addProgramInputOutputResourceTestCase<TargetProp>);
}

template <ProgramResourcePropFlags TargetProp>
static void generateProgramOutputBasicBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, deUint32 presentShadersMask)
{
	generateProgramOutputBlockContents(context, parentStructure, targetGroup, presentShadersMask, false, addProgramInputOutputResourceTestCase<TargetProp>);
}

static void generateProgramInputLocationBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, deUint32 presentShadersMask)
{
	const bool									inDefaultBlock	= parentStructure->getType() == ResourceDefinition::Node::TYPE_DEFAULT_BLOCK;
	const ResourceDefinition::Node::SharedPtr	input			= (inDefaultBlock)
																	? (ResourceDefinition::Node::SharedPtr(new ResourceDefinition::StorageQualifier(parentStructure, glu::STORAGE_IN)))
																	: (parentStructure);
	const glu::ShaderType						firstStage		= getShaderMaskFirstStage(presentShadersMask);

	if (firstStage == glu::SHADERTYPE_VERTEX)
	{
		// .var
		{
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(input, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "var"));
		}
		// .var_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(input, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(layout, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "var_explicit_location"));
		}
	}
	else if (firstStage == glu::SHADERTYPE_FRAGMENT || !inDefaultBlock)
	{
		// .var
		{
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(input, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "var"));
		}
		// .var_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(input, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(layout, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "var_explicit_location"));
		}
		// .var_struct
		{
			const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(input));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(structMbr, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "var_struct"));
		}
		// .var_struct_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(input, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(layout));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(structMbr, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "var_struct_explicit_location"));
		}
		// .var_array
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(input));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "var_array"));
		}
		// .var_array_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(input, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(layout));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "var_array_explicit_location"));
		}
	}
	else if (firstStage == glu::SHADERTYPE_TESSELLATION_CONTROL ||
			 firstStage == glu::SHADERTYPE_GEOMETRY)
	{
		// arrayed interface

		// .var
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(input, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "var"));
		}
		// .var_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(input, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(layout, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "var_explicit_location"));
		}
		// extension forbids use arrays of structs
		// extension forbids use arrays of arrays
	}
	else if (firstStage == glu::SHADERTYPE_TESSELLATION_EVALUATION)
	{
		// arrayed interface
		const ResourceDefinition::Node::SharedPtr patchInput(new ResourceDefinition::StorageQualifier(parentStructure, glu::STORAGE_PATCH_IN));

		// .var
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(input, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "var"));
		}
		// .var_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(input, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(layout, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "var_explicit_location"));
		}
		// extension forbids use arrays of structs
		// extension forbids use arrays of arrays

		// .patch_var
		{
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(patchInput, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "patch_var"));
		}
		// .patch_var_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(patchInput, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(layout, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "patch_var_explicit_location"));
		}
		// .patch_var_struct
		{
			const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(patchInput));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(structMbr, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "patch_var_struct"));
		}
		// .patch_var_struct_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(patchInput, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(layout));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(structMbr, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "patch_var_struct_explicit_location"));
		}
		// .patch_var_array
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(patchInput));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "patch_var_array"));
		}
		// .patch_var_array_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(patchInput, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(layout));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_INPUT, PROGRAMRESOURCEPROP_LOCATION), "patch_var_array_explicit_location"));
		}
	}
	else if (firstStage == glu::SHADERTYPE_COMPUTE)
	{
		// nada
	}
	else
		DE_ASSERT(false);
}

static void generateProgramOutputLocationBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, deUint32 presentShadersMask)
{
	const bool									inDefaultBlock	= parentStructure->getType() == ResourceDefinition::Node::TYPE_DEFAULT_BLOCK;
	const ResourceDefinition::Node::SharedPtr	output			= (inDefaultBlock)
																	? (ResourceDefinition::Node::SharedPtr(new ResourceDefinition::StorageQualifier(parentStructure, glu::STORAGE_OUT)))
																	: (parentStructure);
	const glu::ShaderType						lastStage		= getShaderMaskLastStage(presentShadersMask);

	if (lastStage == glu::SHADERTYPE_VERTEX						||
		lastStage == glu::SHADERTYPE_GEOMETRY					||
		lastStage == glu::SHADERTYPE_TESSELLATION_EVALUATION	||
		!inDefaultBlock)
	{
		// .var
		{
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(output, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "var"));
		}
		// .var_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(output, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(layout, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "var_explicit_location"));
		}
		// .var_struct
		{
			const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(output));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(structMbr, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "var_struct"));
		}
		// .var_struct_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(output, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(layout));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(structMbr, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "var_struct_explicit_location"));
		}
		// .var_array
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(output));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "var_array"));
		}
		// .var_array_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(output, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(layout));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "var_array_explicit_location"));
		}
	}
	else if (lastStage == glu::SHADERTYPE_FRAGMENT)
	{
		// .var
		{
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(output, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "var"));
		}
		// .var_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(output, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(layout, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "var_explicit_location"));
		}
		// .var_array
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(output));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "var_array"));
		}
		// .var_array_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(output, glu::Layout(1)));
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(layout));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "var_array_explicit_location"));
		}
	}
	else if (lastStage == glu::SHADERTYPE_TESSELLATION_CONTROL)
	{
		// arrayed interface
		const ResourceDefinition::Node::SharedPtr patchOutput(new ResourceDefinition::StorageQualifier(parentStructure, glu::STORAGE_PATCH_OUT));

		// .var
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(output, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "var"));
		}
		// .var_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(output, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(layout, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "var_explicit_location"));
		}
		// extension forbids use arrays of structs
		// extension forbids use array of arrays

		// .patch_var
		{
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(patchOutput, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "patch_var"));
		}
		// .patch_var_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(patchOutput, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(layout, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "patch_var_explicit_location"));
		}
		// .patch_var_struct
		{
			const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(patchOutput));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(structMbr, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "patch_var_struct"));
		}
		// .patch_var_struct_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(patchOutput, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(layout));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(structMbr, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "patch_var_struct_explicit_location"));
		}
		// .patch_var_array
		{
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(patchOutput));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "patch_var_array"));
		}
		// .patch_var_array_explicit_location
		{
			const ResourceDefinition::Node::SharedPtr layout	(new ResourceDefinition::LayoutQualifier(patchOutput, glu::Layout(2)));
			const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(layout));
			const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_PROGRAM_OUTPUT, PROGRAMRESOURCEPROP_LOCATION), "patch_var_array_explicit_location"));
		}
	}
	else if (lastStage == glu::SHADERTYPE_COMPUTE)
	{
		// nada
	}
	else
		DE_ASSERT(false);
}

static void generateProgramInputOutputReferencedByCases (Context& context, tcu::TestCaseGroup* targetGroup, glu::Storage storage)
{
	// all whole pipelines
	targetGroup->addChild(new ProgramInputOutputReferencedByCase(context, "referenced_by_vertex_fragment",			"",	storage,	ProgramInputOutputReferencedByCase::CASE_VERTEX_FRAGMENT));
	targetGroup->addChild(new ProgramInputOutputReferencedByCase(context, "referenced_by_vertex_tess_fragment",		"",	storage,	ProgramInputOutputReferencedByCase::CASE_VERTEX_TESS_FRAGMENT));
	targetGroup->addChild(new ProgramInputOutputReferencedByCase(context, "referenced_by_vertex_geo_fragment",		"",	storage,	ProgramInputOutputReferencedByCase::CASE_VERTEX_GEO_FRAGMENT));
	targetGroup->addChild(new ProgramInputOutputReferencedByCase(context, "referenced_by_vertex_tess_geo_fragment",	"",	storage,	ProgramInputOutputReferencedByCase::CASE_VERTEX_TESS_GEO_FRAGMENT));

	// all partial pipelines
	targetGroup->addChild(new ProgramInputOutputReferencedByCase(context, "referenced_by_separable_vertex",		"",	storage,	ProgramInputOutputReferencedByCase::CASE_SEPARABLE_VERTEX));
	targetGroup->addChild(new ProgramInputOutputReferencedByCase(context, "referenced_by_separable_fragment",	"",	storage,	ProgramInputOutputReferencedByCase::CASE_SEPARABLE_FRAGMENT));
	targetGroup->addChild(new ProgramInputOutputReferencedByCase(context, "referenced_by_separable_geometry",	"",	storage,	ProgramInputOutputReferencedByCase::CASE_SEPARABLE_GEOMETRY));
	targetGroup->addChild(new ProgramInputOutputReferencedByCase(context, "referenced_by_separable_tess_eval",	"",	storage,	ProgramInputOutputReferencedByCase::CASE_SEPARABLE_TESS_EVAL));
	targetGroup->addChild(new ProgramInputOutputReferencedByCase(context, "referenced_by_separable_tess_ctrl",	"",	storage,	ProgramInputOutputReferencedByCase::CASE_SEPARABLE_TESS_CTRL));

	// patch
	if (storage == glu::STORAGE_IN)
		targetGroup->addChild(new ProgramInputOutputReferencedByCase(context, "referenced_by_separable_tess_eval_patch_in", "", glu::STORAGE_PATCH_IN, ProgramInputOutputReferencedByCase::CASE_SEPARABLE_TESS_EVAL));
	else if (storage == glu::STORAGE_OUT)
		targetGroup->addChild(new ProgramInputOutputReferencedByCase(context, "referenced_by_separable_tess_ctrl_patch_out", "", glu::STORAGE_PATCH_OUT, ProgramInputOutputReferencedByCase::CASE_SEPARABLE_TESS_CTRL));
	else
		DE_ASSERT(false);
}

template <ProgramInterface interface>
static void generateProgramInputOutputTypeBasicTypeCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, bool allowMatrixCases, int expandLevel)
{
	static const struct
	{
		glu::DataType	type;
		bool			isMatrix;
		int				level;
	} variableTypes[] =
	{
		{ glu::TYPE_FLOAT,			false,		0	},
		{ glu::TYPE_INT,			false,		1	},
		{ glu::TYPE_UINT,			false,		1	},
		{ glu::TYPE_FLOAT_VEC2,		false,		2	},
		{ glu::TYPE_FLOAT_VEC3,		false,		1	},
		{ glu::TYPE_FLOAT_VEC4,		false,		2	},
		{ glu::TYPE_INT_VEC2,		false,		0	},
		{ glu::TYPE_INT_VEC3,		false,		2	},
		{ glu::TYPE_INT_VEC4,		false,		2	},
		{ glu::TYPE_UINT_VEC2,		false,		2	},
		{ glu::TYPE_UINT_VEC3,		false,		2	},
		{ glu::TYPE_UINT_VEC4,		false,		0	},
		{ glu::TYPE_FLOAT_MAT2,		true,		2	},
		{ glu::TYPE_FLOAT_MAT2X3,	true,		2	},
		{ glu::TYPE_FLOAT_MAT2X4,	true,		2	},
		{ glu::TYPE_FLOAT_MAT3X2,	true,		0	},
		{ glu::TYPE_FLOAT_MAT3,		true,		2	},
		{ glu::TYPE_FLOAT_MAT3X4,	true,		2	},
		{ glu::TYPE_FLOAT_MAT4X2,	true,		2	},
		{ glu::TYPE_FLOAT_MAT4X3,	true,		2	},
		{ glu::TYPE_FLOAT_MAT4,		true,		2	},
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(variableTypes); ++ndx)
	{
		if (!allowMatrixCases && variableTypes[ndx].isMatrix)
			continue;

		if (variableTypes[ndx].level <= expandLevel)
		{
			const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(parentStructure, variableTypes[ndx].type));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(interface, PROGRAMRESOURCEPROP_TYPE)));
		}
	}
}

static void generateProgramInputTypeBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, deUint32 presentShadersMask)
{
	const bool									inDefaultBlock						= parentStructure->getType() == ResourceDefinition::Node::TYPE_DEFAULT_BLOCK;
	const ResourceDefinition::Node::SharedPtr	input								= (inDefaultBlock)
																						? (ResourceDefinition::Node::SharedPtr(new ResourceDefinition::StorageQualifier(parentStructure, glu::STORAGE_IN)))
																						: (parentStructure);
	const glu::ShaderType						firstStage							= getShaderMaskFirstStage(presentShadersMask);
	const int									interfaceBlockExpansionReducement	= (!inDefaultBlock) ? (1) : (0); // lesser expansions on block members to keep test counts reasonable

	if (firstStage == glu::SHADERTYPE_VERTEX)
	{
		// Only basic types (and no booleans)
		generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_INPUT>(context, input, targetGroup, true, 2 - interfaceBlockExpansionReducement);
	}
	else if (firstStage == glu::SHADERTYPE_FRAGMENT || !inDefaultBlock)
	{
		const ResourceDefinition::Node::SharedPtr flatShading(new ResourceDefinition::InterpolationQualifier(input, glu::INTERPOLATION_FLAT));

		// Only basic types, arrays of basic types, struct of basic types (and no booleans)
		{
			tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(context, "basic_type", "Basic types");
			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_INPUT>(context, flatShading, blockGroup, true, 2 - interfaceBlockExpansionReducement);
		}
		{
			const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(flatShading));
			tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "array", "Array types");

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_INPUT>(context, arrayElement, blockGroup, true, 2 - interfaceBlockExpansionReducement);
		}
		{
			const ResourceDefinition::Node::SharedPtr	structMember	(new ResourceDefinition::StructMember(flatShading));
			tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "struct", "Struct types");

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_INPUT>(context, structMember, blockGroup, true, 2 - interfaceBlockExpansionReducement);
		}
	}
	else if (firstStage == glu::SHADERTYPE_TESSELLATION_CONTROL ||
			 firstStage == glu::SHADERTYPE_GEOMETRY)
	{
		// arrayed interface

		// Only basic types (and no booleans)
		const ResourceDefinition::Node::SharedPtr arrayElement(new ResourceDefinition::ArrayElement(input, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
		generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_INPUT>(context, arrayElement, targetGroup, true, 2);
	}
	else if (firstStage == glu::SHADERTYPE_TESSELLATION_EVALUATION)
	{
		// arrayed interface
		const ResourceDefinition::Node::SharedPtr patchInput(new ResourceDefinition::StorageQualifier(parentStructure, glu::STORAGE_PATCH_IN));

		// .var
		{
			const ResourceDefinition::Node::SharedPtr	arrayElem		(new ResourceDefinition::ArrayElement(input, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
			tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "basic_type", "Basic types");

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_INPUT>(context, arrayElem, blockGroup, true, 2);
		}
		// extension forbids use arrays of structs
		// extension forbids use arrays of arrays

		// .patch_var
		{
			tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "patch_var", "Basic types, per-patch");

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_INPUT>(context, patchInput, blockGroup, true, 1);
		}
		// .patch_var_struct
		{
			const ResourceDefinition::Node::SharedPtr	structMbr		(new ResourceDefinition::StructMember(patchInput));
			tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "patch_var_struct", "Struct types, per-patch");

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_INPUT>(context, structMbr, blockGroup, true, 1);
		}
		// .patch_var_array
		{
			const ResourceDefinition::Node::SharedPtr	arrayElem		(new ResourceDefinition::ArrayElement(patchInput));
			tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "patch_var_array", "Array types, per-patch");

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_INPUT>(context, arrayElem, blockGroup, true, 1);
		}
	}
	else if (firstStage == glu::SHADERTYPE_COMPUTE)
	{
		// nada
	}
	else
		DE_ASSERT(false);
}

static void generateProgramOutputTypeBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, deUint32 presentShadersMask)
{
	const bool									inDefaultBlock						= parentStructure->getType() == ResourceDefinition::Node::TYPE_DEFAULT_BLOCK;
	const ResourceDefinition::Node::SharedPtr	output								= (inDefaultBlock)
																						? (ResourceDefinition::Node::SharedPtr(new ResourceDefinition::StorageQualifier(parentStructure, glu::STORAGE_OUT)))
																						: (parentStructure);
	const glu::ShaderType						lastStage							= getShaderMaskLastStage(presentShadersMask);
	const int									interfaceBlockExpansionReducement	= (!inDefaultBlock) ? (1) : (0); // lesser expansions on block members to keep test counts reasonable

	if (lastStage == glu::SHADERTYPE_VERTEX						||
		lastStage == glu::SHADERTYPE_GEOMETRY					||
		lastStage == glu::SHADERTYPE_TESSELLATION_EVALUATION	||
		!inDefaultBlock)
	{
		const ResourceDefinition::Node::SharedPtr flatShading(new ResourceDefinition::InterpolationQualifier(output, glu::INTERPOLATION_FLAT));

		// Only basic types, arrays of basic types, struct of basic types (and no booleans)
		{
			tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(context, "basic_type", "Basic types");
			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_OUTPUT>(context, flatShading, blockGroup, true, 2 - interfaceBlockExpansionReducement);
		}
		{
			const ResourceDefinition::Node::SharedPtr	arrayElement			(new ResourceDefinition::ArrayElement(flatShading));
			tcu::TestCaseGroup* const					blockGroup				= new TestCaseGroup(context, "array", "Array types");
			const int									typeExpansionReducement	= (lastStage != glu::SHADERTYPE_VERTEX) ? (1) : (0); // lesser expansions on other stages
			const int									expansionLevel			= 2 - interfaceBlockExpansionReducement - typeExpansionReducement;

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_OUTPUT>(context, arrayElement, blockGroup, true, expansionLevel);
		}
		{
			const ResourceDefinition::Node::SharedPtr	structMember			(new ResourceDefinition::StructMember(flatShading));
			tcu::TestCaseGroup* const					blockGroup				= new TestCaseGroup(context, "struct", "Struct types");
			const int									typeExpansionReducement	= (lastStage != glu::SHADERTYPE_VERTEX) ? (1) : (0); // lesser expansions on other stages
			const int									expansionLevel			= 2 - interfaceBlockExpansionReducement - typeExpansionReducement;

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_OUTPUT>(context, structMember, blockGroup, true, expansionLevel);
		}
	}
	else if (lastStage == glu::SHADERTYPE_FRAGMENT)
	{
		// only basic type and basic type array (and no booleans or matrices)
		{
			tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(context, "basic_type", "Basic types");
			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_OUTPUT>(context, output, blockGroup, false, 2);
		}
		{
			const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(output));
			tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "array", "Array types");

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_OUTPUT>(context, arrayElement, blockGroup, false, 2);
		}
	}
	else if (lastStage == glu::SHADERTYPE_TESSELLATION_CONTROL)
	{
		// arrayed interface
		const ResourceDefinition::Node::SharedPtr patchOutput(new ResourceDefinition::StorageQualifier(parentStructure, glu::STORAGE_PATCH_OUT));

		// .var
		{
			const ResourceDefinition::Node::SharedPtr	arrayElem		(new ResourceDefinition::ArrayElement(output, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
			tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "basic_type", "Basic types");

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_OUTPUT>(context, arrayElem, blockGroup, true, 2);
		}
		// extension forbids use arrays of structs
		// extension forbids use arrays of arrays

		// .patch_var
		{
			tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "patch_var", "Basic types, per-patch");

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_OUTPUT>(context, patchOutput, blockGroup, true, 1);
		}
		// .patch_var_struct
		{
			const ResourceDefinition::Node::SharedPtr	structMbr		(new ResourceDefinition::StructMember(patchOutput));
			tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "patch_var_struct", "Struct types, per-patch");

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_OUTPUT>(context, structMbr, blockGroup, true, 1);
		}
		// .patch_var_array
		{
			const ResourceDefinition::Node::SharedPtr	arrayElem		(new ResourceDefinition::ArrayElement(patchOutput));
			tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "patch_var_array", "Array types, per-patch");

			targetGroup->addChild(blockGroup);
			generateProgramInputOutputTypeBasicTypeCases<PROGRAMINTERFACE_PROGRAM_OUTPUT>(context, arrayElem, blockGroup, true, 1);
		}
	}
	else if (lastStage == glu::SHADERTYPE_COMPUTE)
	{
		// nada
	}
	else
		DE_ASSERT(false);
}

class ProgramInputTestGroup : public TestCaseGroup
{
public:
			ProgramInputTestGroup	(Context& context);
	void	init					(void);
};

ProgramInputTestGroup::ProgramInputTestGroup (Context& context)
	: TestCaseGroup(context, "program_input", "Program input")
{
}

void ProgramInputTestGroup::init (void)
{
	const glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());

	// .resource_list
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "resource_list", "Resource list");
		addChild(blockGroup);
		generateProgramInputOutputShaderCaseBlocks(m_context, blockGroup, glslVersion, true, true, generateProgramInputResourceListBlockContents);
	}

	// .array_size
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "array_size", "Array size");
		addChild(blockGroup);
		generateProgramInputOutputShaderCaseBlocks(m_context, blockGroup, glslVersion, false, true, generateProgramInputBasicBlockContents<PROGRAMRESOURCEPROP_ARRAY_SIZE>);
	}

	// .location
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "location", "Location");
		addChild(blockGroup);
		generateProgramInputOutputShaderCaseBlocks(m_context, blockGroup, glslVersion, false, true, generateProgramInputLocationBlockContents);
	}

	// .name_length
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "name_length", "Name length");
		addChild(blockGroup);
		generateProgramInputOutputShaderCaseBlocks(m_context, blockGroup, glslVersion, false, true, generateProgramInputBasicBlockContents<PROGRAMRESOURCEPROP_NAME_LENGTH>);
	}

	// .referenced_by
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "referenced_by", "Reference by shader");
		addChild(blockGroup);
		generateProgramInputOutputReferencedByCases(m_context, blockGroup, glu::STORAGE_IN);
	}

	// .type
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "type", "Type");
		addChild(blockGroup);
		generateProgramInputOutputShaderCaseBlocks(m_context, blockGroup, glslVersion, false, true, generateProgramInputTypeBlockContents);
	}

	// .is_per_patch
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "is_per_patch", "Is per patch");
		addChild(blockGroup);
		generateProgramInputOutputShaderCaseBlocks(m_context, blockGroup, glslVersion, false, true, generateProgramInputBasicBlockContents<PROGRAMRESOURCEPROP_IS_PER_PATCH>);
	}
}

class ProgramOutputTestGroup : public TestCaseGroup
{
public:
			ProgramOutputTestGroup	(Context& context);
	void	init					(void);
};

ProgramOutputTestGroup::ProgramOutputTestGroup (Context& context)
	: TestCaseGroup(context, "program_output", "Program output")
{
}

void ProgramOutputTestGroup::init (void)
{
	const glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());

	// .resource_list
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "resource_list", "Resource list");
		addChild(blockGroup);
		generateProgramInputOutputShaderCaseBlocks(m_context, blockGroup, glslVersion, true, false, generateProgramOutputResourceListBlockContents);
	}

	// .array_size
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "array_size", "Array size");
		addChild(blockGroup);
		generateProgramInputOutputShaderCaseBlocks(m_context, blockGroup, glslVersion, false, false, generateProgramOutputBasicBlockContents<PROGRAMRESOURCEPROP_ARRAY_SIZE>);
	}

	// .location
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "location", "Location");
		addChild(blockGroup);
		generateProgramInputOutputShaderCaseBlocks(m_context, blockGroup, glslVersion, false, false, generateProgramOutputLocationBlockContents);
	}

	// .name_length
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "name_length", "Name length");
		addChild(blockGroup);
		generateProgramInputOutputShaderCaseBlocks(m_context, blockGroup, glslVersion, false, false, generateProgramOutputBasicBlockContents<PROGRAMRESOURCEPROP_NAME_LENGTH>);
	}

	// .referenced_by
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "referenced_by", "Reference by shader");
		addChild(blockGroup);
		generateProgramInputOutputReferencedByCases(m_context, blockGroup, glu::STORAGE_OUT);
	}

	// .type
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "type", "Type");
		addChild(blockGroup);
		generateProgramInputOutputShaderCaseBlocks(m_context, blockGroup, glslVersion, false, false, generateProgramOutputTypeBlockContents);
	}

	// .is_per_patch
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(m_testCtx, "is_per_patch", "Is per patch");
		addChild(blockGroup);
		generateProgramInputOutputShaderCaseBlocks(m_context, blockGroup, glslVersion, false, false, generateProgramOutputBasicBlockContents<PROGRAMRESOURCEPROP_IS_PER_PATCH>);
	}
}

static void generateTransformFeedbackShaderCaseBlocks (Context& context, tcu::TestCaseGroup* targetGroup, glu::GLSLVersion glslVersion, void (*blockContentGenerator)(Context&, const ResourceDefinition::Node::SharedPtr&, tcu::TestCaseGroup*, bool))
{
	static const struct
	{
		const char*	name;
		deUint32	stageBits;
		deUint32	lastStageBit;
		bool		reducedSet;
	} pipelines[] =
	{
		{
			"vertex_fragment",
			(1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT),
			(1 << glu::SHADERTYPE_VERTEX),
			false
		},
		{
			"vertex_tess_fragment",
			(1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT) | (1 << glu::SHADERTYPE_TESSELLATION_CONTROL) | (1 << glu::SHADERTYPE_TESSELLATION_EVALUATION),
			(1 << glu::SHADERTYPE_TESSELLATION_EVALUATION),
			true
		},
		{
			"vertex_geo_fragment",
			(1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT) | (1 << glu::SHADERTYPE_GEOMETRY),
			(1 << glu::SHADERTYPE_GEOMETRY),
			true
		},
		{
			"vertex_tess_geo_fragment",
			(1 << glu::SHADERTYPE_VERTEX) | (1 << glu::SHADERTYPE_FRAGMENT) | (1 << glu::SHADERTYPE_TESSELLATION_CONTROL) | (1 << glu::SHADERTYPE_TESSELLATION_EVALUATION) | (1 << glu::SHADERTYPE_GEOMETRY),
			(1 << glu::SHADERTYPE_GEOMETRY),
			true
		},
	};
	static const struct
	{
		const char*		name;
		glu::ShaderType	stage;
		bool			reducedSet;
	} singleStageCases[] =
	{
		{ "separable_vertex",		glu::SHADERTYPE_VERTEX,						false	},
		{ "separable_tess_eval",	glu::SHADERTYPE_TESSELLATION_EVALUATION,	true	},
		{ "separable_geometry",		glu::SHADERTYPE_GEOMETRY,					true	},
	};

	// monolithic pipeline
	for (int pipelineNdx = 0; pipelineNdx < DE_LENGTH_OF_ARRAY(pipelines); ++pipelineNdx)
	{
		TestCaseGroup* const						blockGroup		= new TestCaseGroup(context, pipelines[pipelineNdx].name, "");
		const ResourceDefinition::Node::SharedPtr	program			(new ResourceDefinition::Program());
		const ResourceDefinition::Node::SharedPtr	shaderSet		(new ResourceDefinition::ShaderSet(program,
																									   glslVersion,
																									   pipelines[pipelineNdx].stageBits,
																									   pipelines[pipelineNdx].lastStageBit));

		targetGroup->addChild(blockGroup);
		blockContentGenerator(context, shaderSet, blockGroup, pipelines[pipelineNdx].reducedSet);
	}

	// separable pipeline
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(singleStageCases); ++ndx)
	{
		TestCaseGroup* const						blockGroup			= new TestCaseGroup(context, singleStageCases[ndx].name, "");
		const ResourceDefinition::Node::SharedPtr	program				(new ResourceDefinition::Program(true));
		const ResourceDefinition::Node::SharedPtr	shader				(new ResourceDefinition::Shader(program, singleStageCases[ndx].stage, glslVersion));

		targetGroup->addChild(blockGroup);
		blockContentGenerator(context, shader, blockGroup, singleStageCases[ndx].reducedSet);
	}
}

static void generateTransformFeedbackResourceListBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, bool reducedSet)
{
	const ResourceDefinition::Node::SharedPtr	defaultBlock	(new ResourceDefinition::DefaultBlock(parentStructure));
	const ResourceDefinition::Node::SharedPtr	output			(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_OUT));

	DE_UNREF(reducedSet);

	// .builtin_gl_position
	{
		const ResourceDefinition::Node::SharedPtr xfbTarget(new ResourceDefinition::TransformFeedbackTarget(defaultBlock, "gl_Position"));
		targetGroup->addChild(new FeedbackResourceListTestCase(context, xfbTarget, "builtin_gl_position"));
	}
	// .default_block_basic_type
	{
		const ResourceDefinition::Node::SharedPtr xfbTarget	(new ResourceDefinition::TransformFeedbackTarget(output));
		const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(xfbTarget, glu::TYPE_FLOAT_VEC4));
		targetGroup->addChild(new FeedbackResourceListTestCase(context, variable, "default_block_basic_type"));
	}
	// .default_block_struct_member
	{
		const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(output));
		const ResourceDefinition::Node::SharedPtr xfbTarget	(new ResourceDefinition::TransformFeedbackTarget(structMbr));
		const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(xfbTarget, glu::TYPE_FLOAT_VEC4));
		targetGroup->addChild(new FeedbackResourceListTestCase(context, variable, "default_block_struct_member"));
	}
	// .default_block_array
	{
		const ResourceDefinition::Node::SharedPtr xfbTarget	(new ResourceDefinition::TransformFeedbackTarget(output));
		const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(xfbTarget));
		const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
		targetGroup->addChild(new FeedbackResourceListTestCase(context, variable, "default_block_array"));
	}
	// .default_block_array_element
	{
		const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(output));
		const ResourceDefinition::Node::SharedPtr xfbTarget	(new ResourceDefinition::TransformFeedbackTarget(arrayElem));
		const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(xfbTarget, glu::TYPE_FLOAT_VEC4));
		targetGroup->addChild(new FeedbackResourceListTestCase(context, variable, "default_block_array_element"));
	}
}

template <ProgramResourcePropFlags TargetProp>
static void generateTransformFeedbackVariableBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, bool reducedSet)
{
	const ResourceDefinition::Node::SharedPtr	defaultBlock	(new ResourceDefinition::DefaultBlock(parentStructure));
	const ResourceDefinition::Node::SharedPtr	output			(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_OUT));

	DE_UNREF(reducedSet);

	// .builtin_gl_position
	{
		const ResourceDefinition::Node::SharedPtr xfbTarget(new ResourceDefinition::TransformFeedbackTarget(defaultBlock, "gl_Position"));
		targetGroup->addChild(new ResourceTestCase(context, xfbTarget, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING, TargetProp), "builtin_gl_position"));
	}
	// .default_block_basic_type
	{
		const ResourceDefinition::Node::SharedPtr xfbTarget	(new ResourceDefinition::TransformFeedbackTarget(output));
		const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(xfbTarget, glu::TYPE_FLOAT_VEC4));
		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING, TargetProp), "default_block_basic_type"));
	}
	// .default_block_struct_member
	{
		const ResourceDefinition::Node::SharedPtr structMbr	(new ResourceDefinition::StructMember(output));
		const ResourceDefinition::Node::SharedPtr xfbTarget	(new ResourceDefinition::TransformFeedbackTarget(structMbr));
		const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(xfbTarget, glu::TYPE_FLOAT_VEC4));
		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING, TargetProp), "default_block_struct_member"));
	}
	// .default_block_array
	{
		const ResourceDefinition::Node::SharedPtr xfbTarget	(new ResourceDefinition::TransformFeedbackTarget(output));
		const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(xfbTarget));
		const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(arrayElem, glu::TYPE_FLOAT_VEC4));
		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING, TargetProp), "default_block_array"));
	}
	// .default_block_array_element
	{
		const ResourceDefinition::Node::SharedPtr arrayElem	(new ResourceDefinition::ArrayElement(output));
		const ResourceDefinition::Node::SharedPtr xfbTarget	(new ResourceDefinition::TransformFeedbackTarget(arrayElem));
		const ResourceDefinition::Node::SharedPtr variable	(new ResourceDefinition::Variable(xfbTarget, glu::TYPE_FLOAT_VEC4));
		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING, TargetProp), "default_block_array_element"));
	}
}

static void generateTransformFeedbackVariableBasicTypeCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, bool reducedSet)
{
	static const struct
	{
		glu::DataType	type;
		bool			important;
	} variableTypes[] =
	{
		{ glu::TYPE_FLOAT,			true	},
		{ glu::TYPE_INT,			true	},
		{ glu::TYPE_UINT,			true	},

		{ glu::TYPE_FLOAT_VEC2,		false	},
		{ glu::TYPE_FLOAT_VEC3,		true	},
		{ glu::TYPE_FLOAT_VEC4,		false	},

		{ glu::TYPE_INT_VEC2,		false	},
		{ glu::TYPE_INT_VEC3,		true	},
		{ glu::TYPE_INT_VEC4,		false	},

		{ glu::TYPE_UINT_VEC2,		true	},
		{ glu::TYPE_UINT_VEC3,		false	},
		{ glu::TYPE_UINT_VEC4,		false	},

		{ glu::TYPE_FLOAT_MAT2,		false	},
		{ glu::TYPE_FLOAT_MAT2X3,	false	},
		{ glu::TYPE_FLOAT_MAT2X4,	false	},
		{ glu::TYPE_FLOAT_MAT3X2,	false	},
		{ glu::TYPE_FLOAT_MAT3,		false	},
		{ glu::TYPE_FLOAT_MAT3X4,	true	},
		{ glu::TYPE_FLOAT_MAT4X2,	false	},
		{ glu::TYPE_FLOAT_MAT4X3,	false	},
		{ glu::TYPE_FLOAT_MAT4,		false	},
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(variableTypes); ++ndx)
	{
		if (variableTypes[ndx].important || !reducedSet)
		{
			const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(parentStructure, variableTypes[ndx].type));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING, PROGRAMRESOURCEPROP_TYPE)));
		}
	}
}

static void generateTransformFeedbackVariableTypeBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, bool reducedSet)
{
	const ResourceDefinition::Node::SharedPtr	defaultBlock	(new ResourceDefinition::DefaultBlock(parentStructure));
	const ResourceDefinition::Node::SharedPtr	output			(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_OUT));
	const ResourceDefinition::Node::SharedPtr	flatShading		(new ResourceDefinition::InterpolationQualifier(output, glu::INTERPOLATION_FLAT));

	// Only builtins, basic types, arrays of basic types, struct of basic types (and no booleans)
	{
		const ResourceDefinition::Node::SharedPtr	xfbTarget		(new ResourceDefinition::TransformFeedbackTarget(defaultBlock, "gl_Position"));
		tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "builtin", "Built-in outputs");

		targetGroup->addChild(blockGroup);
		blockGroup->addChild(new ResourceTestCase(context, xfbTarget, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING, PROGRAMRESOURCEPROP_TYPE), "gl_position"));
	}
	{
		const ResourceDefinition::Node::SharedPtr	xfbTarget		(new ResourceDefinition::TransformFeedbackTarget(flatShading));
		tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "basic_type", "Basic types");

		targetGroup->addChild(blockGroup);
		generateTransformFeedbackVariableBasicTypeCases(context, xfbTarget, blockGroup, reducedSet);
	}
	{
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(flatShading));
		const ResourceDefinition::Node::SharedPtr	xfbTarget		(new ResourceDefinition::TransformFeedbackTarget(arrayElement));
		tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "array", "Array types");

		targetGroup->addChild(blockGroup);
		generateTransformFeedbackVariableBasicTypeCases(context, xfbTarget, blockGroup, reducedSet);
	}
	{
		const ResourceDefinition::Node::SharedPtr	xfbTarget		(new ResourceDefinition::TransformFeedbackTarget(flatShading));
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(xfbTarget));
		tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "whole_array", "Whole array");

		targetGroup->addChild(blockGroup);
		generateTransformFeedbackVariableBasicTypeCases(context, arrayElement, blockGroup, reducedSet);
	}
	{
		const ResourceDefinition::Node::SharedPtr	structMember	(new ResourceDefinition::StructMember(flatShading));
		const ResourceDefinition::Node::SharedPtr	xfbTarget		(new ResourceDefinition::TransformFeedbackTarget(structMember));
		tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "struct", "Struct types");

		targetGroup->addChild(blockGroup);
		generateTransformFeedbackVariableBasicTypeCases(context, xfbTarget, blockGroup, reducedSet);
	}
}

class TransformFeedbackVaryingTestGroup : public TestCaseGroup
{
public:
			TransformFeedbackVaryingTestGroup	(Context& context);
	void	init								(void);
};

TransformFeedbackVaryingTestGroup::TransformFeedbackVaryingTestGroup (Context& context)
	: TestCaseGroup(context, "transform_feedback_varying", "Transform feedback varyings")
{
}

void TransformFeedbackVaryingTestGroup::init (void)
{
	const glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());

	// .resource_list
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "resource_list", "Resource list");
		addChild(blockGroup);
		generateTransformFeedbackShaderCaseBlocks(m_context, blockGroup, glslVersion, generateTransformFeedbackResourceListBlockContents);
	}

	// .array_size
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "array_size", "Array size");
		addChild(blockGroup);
		generateTransformFeedbackShaderCaseBlocks(m_context, blockGroup, glslVersion, generateTransformFeedbackVariableBlockContents<PROGRAMRESOURCEPROP_ARRAY_SIZE>);
	}

	// .name_length
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "name_length", "Name length");
		addChild(blockGroup);
		generateTransformFeedbackShaderCaseBlocks(m_context, blockGroup, glslVersion, generateTransformFeedbackVariableBlockContents<PROGRAMRESOURCEPROP_NAME_LENGTH>);
	}

	// .type
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "type", "Type");
		addChild(blockGroup);
		generateTransformFeedbackShaderCaseBlocks(m_context, blockGroup, glslVersion, generateTransformFeedbackVariableTypeBlockContents);
	}
}

static void generateBufferVariableBufferCaseBlocks (Context& context, tcu::TestCaseGroup* targetGroup, glu::GLSLVersion glslVersion, void (*blockContentGenerator)(Context&, const ResourceDefinition::Node::SharedPtr&, tcu::TestCaseGroup*))
{
	const ResourceDefinition::Node::SharedPtr	program			(new ResourceDefinition::Program());
	const ResourceDefinition::Node::SharedPtr	shader			(new ResourceDefinition::Shader(program, glu::SHADERTYPE_COMPUTE, glslVersion));
	const ResourceDefinition::Node::SharedPtr	defaultBlock	(new ResourceDefinition::DefaultBlock(shader));
	const ResourceDefinition::Node::SharedPtr	bufferStorage	(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_BUFFER));
	const ResourceDefinition::Node::SharedPtr	binding			(new ResourceDefinition::LayoutQualifier(bufferStorage, glu::Layout(-1, 0)));

	// .named_block
	{
		const ResourceDefinition::Node::SharedPtr	buffer		(new ResourceDefinition::InterfaceBlock(binding, true));
		tcu::TestCaseGroup* const					blockGroup	= new TestCaseGroup(context, "named_block", "Named block");

		targetGroup->addChild(blockGroup);

		blockContentGenerator(context, buffer, blockGroup);
	}

	// .unnamed_block
	{
		const ResourceDefinition::Node::SharedPtr	buffer		(new ResourceDefinition::InterfaceBlock(binding, false));
		tcu::TestCaseGroup* const					blockGroup	= new TestCaseGroup(context, "unnamed_block", "Unnamed block");

		targetGroup->addChild(blockGroup);

		blockContentGenerator(context, buffer, blockGroup);
	}

	// .block_array
	{
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(binding));
		const ResourceDefinition::Node::SharedPtr	buffer			(new ResourceDefinition::InterfaceBlock(arrayElement, true));
		tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "block_array", "Block array");

		targetGroup->addChild(blockGroup);

		blockContentGenerator(context, buffer, blockGroup);
	}
}

static void generateBufferVariableResourceListBlockContentsProxy (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup)
{
	generateBufferBackedResourceListBlockContentCases(context, parentStructure, targetGroup, PROGRAMINTERFACE_BUFFER_VARIABLE, 4);
}

static void generateBufferVariableArraySizeSubCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup, ProgramResourcePropFlags targetProp, bool sizedArray, bool extendedCases)
{
	const ProgramResourceQueryTestTarget	queryTarget		(PROGRAMINTERFACE_BUFFER_VARIABLE, targetProp);
	tcu::TestCaseGroup*						aggregateGroup;

	// .types
	if (extendedCases)
	{
		tcu::TestCaseGroup* const blockGroup = new tcu::TestCaseGroup(context.getTestContext(), "types", "Types");
		targetGroup->addChild(blockGroup);

		generateVariableCases(context, parentStructure, blockGroup, queryTarget, (sizedArray) ? (2) : (1), false);
	}

	// .aggregates
	if (extendedCases)
	{
		aggregateGroup = new tcu::TestCaseGroup(context.getTestContext(), "aggregates", "Aggregate types");
		targetGroup->addChild(aggregateGroup);
	}
	else
		aggregateGroup = targetGroup;

	// .float_*
	generateBufferBackedArrayStrideTypeAggregateCases(context, parentStructure, aggregateGroup, queryTarget.interface, glu::TYPE_FLOAT, (extendedCases && sizedArray) ? (2) : (1), !extendedCases);

	// .bool_*
	generateBufferBackedArrayStrideTypeAggregateCases(context, parentStructure, aggregateGroup, queryTarget.interface, glu::TYPE_BOOL, (extendedCases && sizedArray) ? (1) : (0), !extendedCases);

	// .bvec3_*
	generateBufferBackedArrayStrideTypeAggregateCases(context, parentStructure, aggregateGroup, queryTarget.interface, glu::TYPE_BOOL_VEC3, (extendedCases && sizedArray) ? (2) : (1), !extendedCases);

	// .vec4_*
	generateBufferBackedArrayStrideTypeAggregateCases(context, parentStructure, aggregateGroup, queryTarget.interface, glu::TYPE_FLOAT_VEC4, (extendedCases && sizedArray) ? (2) : (1), !extendedCases);

	// .ivec2_*
	generateBufferBackedArrayStrideTypeAggregateCases(context, parentStructure, aggregateGroup, queryTarget.interface, glu::TYPE_INT_VEC2, (extendedCases && sizedArray) ? (2) : (1), !extendedCases);
}

template <ProgramResourcePropFlags TargetProp>
static void generateBufferVariableArrayCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* const targetGroup)
{
	const ProgramResourceQueryTestTarget	queryTarget			(PROGRAMINTERFACE_BUFFER_VARIABLE, TargetProp);
	const bool								namedNonArrayBlock	= static_cast<const ResourceDefinition::InterfaceBlock*>(parentStructure.get())->m_named && parentStructure->getEnclosingNode()->getType() != ResourceDefinition::Node::TYPE_ARRAY_ELEMENT;

	// .non_array
	if (namedNonArrayBlock)
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(context, "non_array", "Non-array target");
		targetGroup->addChild(blockGroup);

		generateVariableCases(context, parentStructure, blockGroup, queryTarget, 1, false);
	}

	// .sized
	{
		const ResourceDefinition::Node::SharedPtr	sized		(new ResourceDefinition::ArrayElement(parentStructure));
		tcu::TestCaseGroup* const					blockGroup	= new TestCaseGroup(context, "sized", "Sized target");
		targetGroup->addChild(blockGroup);

		generateBufferVariableArraySizeSubCases(context, sized, blockGroup, TargetProp, true, namedNonArrayBlock);
	}

	// .unsized
	{
		const ResourceDefinition::Node::SharedPtr	unsized		(new ResourceDefinition::ArrayElement(parentStructure, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
		tcu::TestCaseGroup* const					blockGroup	= new TestCaseGroup(context, "unsized", "Unsized target");
		targetGroup->addChild(blockGroup);

		generateBufferVariableArraySizeSubCases(context, unsized, blockGroup, TargetProp, false, namedNonArrayBlock);
	}
}

static void generateBufferVariableBlockIndexCases (Context& context, glu::GLSLVersion glslVersion, tcu::TestCaseGroup* const targetGroup)
{
	const ResourceDefinition::Node::SharedPtr	program			(new ResourceDefinition::Program());
	const ResourceDefinition::Node::SharedPtr	shader			(new ResourceDefinition::Shader(program, glu::SHADERTYPE_COMPUTE, glslVersion));
	const ResourceDefinition::Node::SharedPtr	defaultBlock	(new ResourceDefinition::DefaultBlock(shader));
	const ResourceDefinition::Node::SharedPtr	bufferStorage	(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_BUFFER));
	const ResourceDefinition::Node::SharedPtr	binding			(new ResourceDefinition::LayoutQualifier(bufferStorage, glu::Layout(-1, 0)));

	// .named_block
	{
		const ResourceDefinition::Node::SharedPtr	buffer		(new ResourceDefinition::InterfaceBlock(binding, true));
		const ResourceDefinition::Node::SharedPtr	variable	(new ResourceDefinition::Variable(buffer, glu::TYPE_FLOAT_VEC4));

		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_BUFFER_VARIABLE, PROGRAMRESOURCEPROP_BLOCK_INDEX), "named_block"));
	}

	// .unnamed_block
	{
		const ResourceDefinition::Node::SharedPtr	buffer		(new ResourceDefinition::InterfaceBlock(binding, false));
		const ResourceDefinition::Node::SharedPtr	variable	(new ResourceDefinition::Variable(buffer, glu::TYPE_FLOAT_VEC4));

		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_BUFFER_VARIABLE, PROGRAMRESOURCEPROP_BLOCK_INDEX), "unnamed_block"));
	}

	// .block_array
	{
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(binding));
		const ResourceDefinition::Node::SharedPtr	buffer			(new ResourceDefinition::InterfaceBlock(arrayElement, true));
		const ResourceDefinition::Node::SharedPtr	variable		(new ResourceDefinition::Variable(buffer, glu::TYPE_FLOAT_VEC4));

		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_BUFFER_VARIABLE, PROGRAMRESOURCEPROP_BLOCK_INDEX), "block_array"));
	}
}

static void generateBufferVariableMatrixCaseBlocks (Context& context, tcu::TestCaseGroup* const targetGroup, glu::GLSLVersion glslVersion, void (*blockContentGenerator)(Context&, const ResourceDefinition::Node::SharedPtr&, tcu::TestCaseGroup*, bool))
{
	static const struct
	{
		const char*			name;
		const char*			description;
		bool				namedBlock;
		bool				extendedBasicTypeCases;
		glu::MatrixOrder	order;
	} children[] =
	{
		{ "named_block",				"Named uniform block",		true,	true,	glu::MATRIXORDER_LAST			},
		{ "named_block_row_major",		"Named uniform block",		true,	false,	glu::MATRIXORDER_ROW_MAJOR		},
		{ "named_block_col_major",		"Named uniform block",		true,	false,	glu::MATRIXORDER_COLUMN_MAJOR	},
		{ "unnamed_block",				"Unnamed uniform block",	false,	false,	glu::MATRIXORDER_LAST			},
		{ "unnamed_block_row_major",	"Unnamed uniform block",	false,	false,	glu::MATRIXORDER_ROW_MAJOR		},
		{ "unnamed_block_col_major",	"Unnamed uniform block",	false,	false,	glu::MATRIXORDER_COLUMN_MAJOR	},
	};

	const ResourceDefinition::Node::SharedPtr	program			(new ResourceDefinition::Program());
	const ResourceDefinition::Node::SharedPtr	shader			(new ResourceDefinition::Shader(program, glu::SHADERTYPE_COMPUTE, glslVersion));
	const ResourceDefinition::Node::SharedPtr	defaultBlock	(new ResourceDefinition::DefaultBlock(shader));
	const ResourceDefinition::Node::SharedPtr	buffer			(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_BUFFER));

	for (int childNdx = 0; childNdx < (int)DE_LENGTH_OF_ARRAY(children); ++childNdx)
	{
		ResourceDefinition::Node::SharedPtr	parentStructure	= buffer;
		tcu::TestCaseGroup* const			blockGroup		= new TestCaseGroup(context, children[childNdx].name, children[childNdx].description);

		targetGroup->addChild(blockGroup);

		if (children[childNdx].order != glu::MATRIXORDER_LAST)
		{
			glu::Layout layout;
			layout.matrixOrder = children[childNdx].order;
			parentStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::LayoutQualifier(parentStructure, layout));
		}

		parentStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::InterfaceBlock(parentStructure, children[childNdx].namedBlock));

		blockContentGenerator(context, parentStructure, blockGroup, children[childNdx].extendedBasicTypeCases);
	}
}

static void generateBufferVariableMatrixVariableBasicTypeCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, ProgramResourcePropFlags targetProp)
{
	// all matrix types and some non-matrix

	static const glu::DataType variableTypes[] =
	{
		glu::TYPE_FLOAT,
		glu::TYPE_INT_VEC3,
		glu::TYPE_FLOAT_MAT2,
		glu::TYPE_FLOAT_MAT2X3,
		glu::TYPE_FLOAT_MAT2X4,
		glu::TYPE_FLOAT_MAT3X2,
		glu::TYPE_FLOAT_MAT3,
		glu::TYPE_FLOAT_MAT3X4,
		glu::TYPE_FLOAT_MAT4X2,
		glu::TYPE_FLOAT_MAT4X3,
		glu::TYPE_FLOAT_MAT4,
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(variableTypes); ++ndx)
	{
		const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(parentStructure, variableTypes[ndx]));
		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_BUFFER_VARIABLE, targetProp)));
	}
}

static void generateBufferVariableMatrixVariableCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, ProgramResourcePropFlags targetProp)
{
	// Basic aggregates
	generateBufferBackedVariableAggregateTypeCases(context, parentStructure, targetGroup, PROGRAMINTERFACE_BUFFER_VARIABLE, targetProp, glu::TYPE_FLOAT_MAT3X2, "", 2);

	// Unsized array
	{
		const ResourceDefinition::Node::SharedPtr	unsized		(new ResourceDefinition::ArrayElement(parentStructure, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
		const ResourceDefinition::Node::SharedPtr	variable	(new ResourceDefinition::Variable(unsized, glu::TYPE_FLOAT_MAT3X2));

		targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_BUFFER_VARIABLE, targetProp), "var_unsized_array"));
	}
}

template <ProgramResourcePropFlags TargetProp>
static void generateBufferVariableMatrixCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, bool extendedTypeCases)
{
	// .types
	if (extendedTypeCases)
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(context, "types", "Types");
		targetGroup->addChild(blockGroup);
		generateBufferVariableMatrixVariableBasicTypeCases(context, parentStructure, blockGroup, TargetProp);
	}

	// .no_qualifier
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(context, "no_qualifier", "No qualifier");
		targetGroup->addChild(blockGroup);
		generateBufferVariableMatrixVariableCases(context, parentStructure, blockGroup, TargetProp);
	}

	// .column_major
	{
		const ResourceDefinition::Node::SharedPtr matrixOrder(new ResourceDefinition::LayoutQualifier(parentStructure, glu::Layout(-1, -1, -1, glu::FORMATLAYOUT_LAST, glu::MATRIXORDER_COLUMN_MAJOR)));

		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(context, "column_major", "Column major qualifier");
		targetGroup->addChild(blockGroup);
		generateBufferVariableMatrixVariableCases(context, matrixOrder, blockGroup, TargetProp);
	}

	// .row_major
	{
		const ResourceDefinition::Node::SharedPtr matrixOrder(new ResourceDefinition::LayoutQualifier(parentStructure, glu::Layout(-1, -1, -1, glu::FORMATLAYOUT_LAST, glu::MATRIXORDER_ROW_MAJOR)));

		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(context, "row_major", "Row major qualifier");
		targetGroup->addChild(blockGroup);
		generateBufferVariableMatrixVariableCases(context, matrixOrder, blockGroup, TargetProp);
	}
}

static void generateBufferVariableNameLengthCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup)
{
	// .sized
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(context, "sized", "Sized target");
		targetGroup->addChild(blockGroup);

		generateBufferBackedVariableAggregateTypeCases(context, parentStructure, blockGroup, PROGRAMINTERFACE_BUFFER_VARIABLE, PROGRAMRESOURCEPROP_NAME_LENGTH, glu::TYPE_FLOAT, "", 3);
	}

	// .unsized
	{
		const ResourceDefinition::Node::SharedPtr	unsized		(new ResourceDefinition::ArrayElement(parentStructure, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
		tcu::TestCaseGroup* const					blockGroup	= new TestCaseGroup(context, "unsized", "Unsized target");
		targetGroup->addChild(blockGroup);

		generateBufferBackedVariableAggregateTypeCases(context, unsized, blockGroup, PROGRAMINTERFACE_BUFFER_VARIABLE, PROGRAMRESOURCEPROP_NAME_LENGTH, glu::TYPE_FLOAT, "", 2);
	}
}

static void generateBufferVariableOffsetCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup)
{
	// .sized
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(context, "sized", "Sized target");
		targetGroup->addChild(blockGroup);

		generateBufferBackedVariableAggregateTypeCases(context, parentStructure, blockGroup, PROGRAMINTERFACE_BUFFER_VARIABLE, PROGRAMRESOURCEPROP_OFFSET, glu::TYPE_FLOAT, "", 3);
	}

	// .unsized
	{
		const ResourceDefinition::Node::SharedPtr	unsized		(new ResourceDefinition::ArrayElement(parentStructure, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));
		tcu::TestCaseGroup* const					blockGroup	= new TestCaseGroup(context, "unsized", "Unsized target");
		targetGroup->addChild(blockGroup);

		generateBufferBackedVariableAggregateTypeCases(context, unsized, blockGroup, PROGRAMINTERFACE_BUFFER_VARIABLE, PROGRAMRESOURCEPROP_OFFSET, glu::TYPE_FLOAT, "", 2);
	}
}

static void generateBufferVariableReferencedByBlockContents (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, int expandLevel)
{
	DE_UNREF(expandLevel);

	const ProgramResourceQueryTestTarget		queryTarget		(PROGRAMINTERFACE_BUFFER_VARIABLE, PROGRAMRESOURCEPROP_REFERENCED_BY_SHADER);
	const ResourceDefinition::Node::SharedPtr	defaultBlock	(new ResourceDefinition::DefaultBlock(parentStructure));
	const ResourceDefinition::Node::SharedPtr	storage			(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_BUFFER));
	const bool									singleShaderCase	= parentStructure->getType() == ResourceDefinition::Node::TYPE_SHADER;

	// .named_block
	{
		const ResourceDefinition::Node::SharedPtr	buffer		(new ResourceDefinition::InterfaceBlock(storage, true));
		tcu::TestCaseGroup* const					blockGroup	= new TestCaseGroup(context, "named_block", "Named block");

		targetGroup->addChild(blockGroup);

		generateBufferReferencedByShaderInterfaceBlockCases(context, buffer, blockGroup, queryTarget, singleShaderCase);
	}

	// .unnamed_block
	{
		const ResourceDefinition::Node::SharedPtr	buffer		(new ResourceDefinition::InterfaceBlock(storage, false));
		tcu::TestCaseGroup* const					blockGroup	= new TestCaseGroup(context, "unnamed_block", "Unnamed block");

		targetGroup->addChild(blockGroup);

		generateBufferReferencedByShaderInterfaceBlockCases(context, buffer, blockGroup, queryTarget, false);
	}

	// .block_array
	{
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(storage));
		const ResourceDefinition::Node::SharedPtr	buffer			(new ResourceDefinition::InterfaceBlock(arrayElement, true));
		tcu::TestCaseGroup* const					blockGroup	= new TestCaseGroup(context, "block_array", "Block array");

		targetGroup->addChild(blockGroup);

		generateBufferReferencedByShaderInterfaceBlockCases(context, buffer, blockGroup, queryTarget, false);
	}
}

template <ProgramResourcePropFlags TargetProp>
static void generateBufferVariableTopLevelCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup)
{
	// basic and aggregate types
	generateBufferBackedVariableAggregateTypeCases(context, parentStructure, targetGroup, PROGRAMINTERFACE_BUFFER_VARIABLE, TargetProp, glu::TYPE_FLOAT_VEC4, "", 3);

	// basic and aggregate types in an unsized array
	{
		const ResourceDefinition::Node::SharedPtr unsized(new ResourceDefinition::ArrayElement(parentStructure, ResourceDefinition::ArrayElement::UNSIZED_ARRAY));

		generateBufferBackedVariableAggregateTypeCases(context, unsized, targetGroup, PROGRAMINTERFACE_BUFFER_VARIABLE, TargetProp, glu::TYPE_FLOAT_VEC4, "_unsized_array", 2);
	}
}

static void generateBufferVariableTypeBasicTypeCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, int expandLevel)
{
	static const struct
	{
		int				level;
		glu::DataType	dataType;
	} variableTypes[] =
	{
		{ 0,	glu::TYPE_FLOAT			},
		{ 1,	glu::TYPE_INT			},
		{ 1,	glu::TYPE_UINT			},
		{ 1,	glu::TYPE_BOOL			},

		{ 3,	glu::TYPE_FLOAT_VEC2	},
		{ 1,	glu::TYPE_FLOAT_VEC3	},
		{ 1,	glu::TYPE_FLOAT_VEC4	},

		{ 3,	glu::TYPE_INT_VEC2		},
		{ 2,	glu::TYPE_INT_VEC3		},
		{ 3,	glu::TYPE_INT_VEC4		},

		{ 3,	glu::TYPE_UINT_VEC2		},
		{ 2,	glu::TYPE_UINT_VEC3		},
		{ 3,	glu::TYPE_UINT_VEC4		},

		{ 3,	glu::TYPE_BOOL_VEC2		},
		{ 2,	glu::TYPE_BOOL_VEC3		},
		{ 3,	glu::TYPE_BOOL_VEC4		},

		{ 2,	glu::TYPE_FLOAT_MAT2	},
		{ 3,	glu::TYPE_FLOAT_MAT2X3	},
		{ 3,	glu::TYPE_FLOAT_MAT2X4	},
		{ 2,	glu::TYPE_FLOAT_MAT3X2	},
		{ 2,	glu::TYPE_FLOAT_MAT3	},
		{ 3,	glu::TYPE_FLOAT_MAT3X4	},
		{ 2,	glu::TYPE_FLOAT_MAT4X2	},
		{ 3,	glu::TYPE_FLOAT_MAT4X3	},
		{ 2,	glu::TYPE_FLOAT_MAT4	},
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(variableTypes); ++ndx)
	{
		if (variableTypes[ndx].level <= expandLevel)
		{
			const ResourceDefinition::Node::SharedPtr variable(new ResourceDefinition::Variable(parentStructure, variableTypes[ndx].dataType));
			targetGroup->addChild(new ResourceTestCase(context, variable, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_BUFFER_VARIABLE, PROGRAMRESOURCEPROP_TYPE)));
		}
	}
}

static void generateBufferVariableTypeCases (Context& context, const ResourceDefinition::Node::SharedPtr& parentStructure, tcu::TestCaseGroup* targetGroup, int depth = 3)
{
	// .basic_type
	if (depth > 0)
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(context, "basic_type", "Basic type");
		targetGroup->addChild(blockGroup);
		generateBufferVariableTypeBasicTypeCases(context, parentStructure, blockGroup, depth);
	}
	else
	{
		// flatten bottom-level
		generateBufferVariableTypeBasicTypeCases(context, parentStructure, targetGroup, depth);
	}

	// .array
	if (depth > 0)
	{
		const ResourceDefinition::Node::SharedPtr	arrayElement	(new ResourceDefinition::ArrayElement(parentStructure));
		tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "array", "Arrays");

		targetGroup->addChild(blockGroup);
		generateBufferVariableTypeCases(context, arrayElement, blockGroup, depth-1);
	}

	// .struct
	if (depth > 0)
	{
		const ResourceDefinition::Node::SharedPtr	structMember	(new ResourceDefinition::StructMember(parentStructure));
		tcu::TestCaseGroup* const					blockGroup		= new TestCaseGroup(context, "struct", "Structs");

		targetGroup->addChild(blockGroup);
		generateBufferVariableTypeCases(context, structMember, blockGroup, depth-1);
	}
}

static void generateBufferVariableTypeBlock (Context& context, tcu::TestCaseGroup* targetGroup, glu::GLSLVersion glslVersion)
{
	const ResourceDefinition::Node::SharedPtr	program			(new ResourceDefinition::Program());
	const ResourceDefinition::Node::SharedPtr	shader			(new ResourceDefinition::Shader(program, glu::SHADERTYPE_COMPUTE, glslVersion));
	const ResourceDefinition::Node::SharedPtr	defaultBlock	(new ResourceDefinition::DefaultBlock(shader));
	const ResourceDefinition::Node::SharedPtr	buffer			(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_BUFFER));
	const ResourceDefinition::Node::SharedPtr	block			(new ResourceDefinition::InterfaceBlock(buffer, true));

	generateBufferVariableTypeCases(context, block, targetGroup);
}

static void generateBufferVariableRandomCase (Context& context, tcu::TestCaseGroup* const targetGroup, glu::GLSLVersion glslVersion, int index, bool onlyExtensionStages)
{
	de::Random									rnd					(index * 0x12345);
	const ResourceDefinition::Node::SharedPtr	shader				= generateRandomShaderSet(rnd, glslVersion, onlyExtensionStages);
	const glu::DataType							type				= generateRandomDataType(rnd, true);
	const glu::Layout							layout				= generateRandomVariableLayout(rnd, type, true);
	const bool									namedBlock			= rnd.getBool();
	const ResourceDefinition::Node::SharedPtr	defaultBlock		(new ResourceDefinition::DefaultBlock(shader));
	const ResourceDefinition::Node::SharedPtr	buffer				(new ResourceDefinition::StorageQualifier(defaultBlock, glu::STORAGE_BUFFER));
	ResourceDefinition::Node::SharedPtr			currentStructure	(new ResourceDefinition::LayoutQualifier(buffer, generateRandomBufferBlockLayout(rnd)));

	if (namedBlock && rnd.getBool())
		currentStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::ArrayElement(currentStructure));
	currentStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::InterfaceBlock(currentStructure, namedBlock));

	currentStructure = ResourceDefinition::Node::SharedPtr(new ResourceDefinition::LayoutQualifier(currentStructure, layout));
	currentStructure = generateRandomVariableDefinition(rnd, currentStructure, type, layout, true);

	targetGroup->addChild(new ResourceTestCase(context, currentStructure, ProgramResourceQueryTestTarget(PROGRAMINTERFACE_BUFFER_VARIABLE, PROGRAMRESOURCEPROP_BUFFER_VARIABLE_MASK), de::toString(index).c_str()));
}

static void generateBufferVariableRandomCases (Context& context, tcu::TestCaseGroup* const targetGroup, glu::GLSLVersion glslVersion)
{
	const int numBasicCases		= 40;
	const int numTessGeoCases	= 40;

	for (int ndx = 0; ndx < numBasicCases; ++ndx)
		generateBufferVariableRandomCase(context, targetGroup, glslVersion, ndx, false);
	for (int ndx = 0; ndx < numTessGeoCases; ++ndx)
		generateBufferVariableRandomCase(context, targetGroup, glslVersion, numBasicCases + ndx, true);
}

class BufferVariableTestGroup : public TestCaseGroup
{
public:
			BufferVariableTestGroup	(Context& context);
	void	init								(void);
};

BufferVariableTestGroup::BufferVariableTestGroup (Context& context)
	: TestCaseGroup(context, "buffer_variable", "Buffer variable")
{
}

void BufferVariableTestGroup::init (void)
{
	const glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());

	// .resource_list
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "resource_list", "Resource list");
		addChild(blockGroup);
		generateBufferVariableBufferCaseBlocks(m_context, blockGroup, glslVersion, generateBufferVariableResourceListBlockContentsProxy);
	}

	// .array_size
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "array_size", "Array size");
		addChild(blockGroup);
		generateBufferVariableBufferCaseBlocks(m_context, blockGroup, glslVersion, generateBufferVariableArrayCases<PROGRAMRESOURCEPROP_ARRAY_SIZE>);
	}

	// .array_stride
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "array_stride", "Array stride");
		addChild(blockGroup);
		generateBufferVariableBufferCaseBlocks(m_context, blockGroup, glslVersion, generateBufferVariableArrayCases<PROGRAMRESOURCEPROP_ARRAY_STRIDE>);
	}

	// .block_index
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "block_index", "Block index");
		addChild(blockGroup);
		generateBufferVariableBlockIndexCases(m_context, glslVersion, blockGroup);
	}

	// .is_row_major
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "is_row_major", "Is row major");
		addChild(blockGroup);
		generateBufferVariableMatrixCaseBlocks(m_context, blockGroup, glslVersion, generateBufferVariableMatrixCases<PROGRAMRESOURCEPROP_MATRIX_ROW_MAJOR>);
	}

	// .matrix_stride
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "matrix_stride", "Matrix stride");
		addChild(blockGroup);
		generateBufferVariableMatrixCaseBlocks(m_context, blockGroup, glslVersion, generateBufferVariableMatrixCases<PROGRAMRESOURCEPROP_MATRIX_STRIDE>);
	}

	// .name_length
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "name_length", "Name length");
		addChild(blockGroup);
		generateBufferVariableBufferCaseBlocks(m_context, blockGroup, glslVersion, generateBufferVariableNameLengthCases);
	}

	// .offset
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "offset", "Offset");
		addChild(blockGroup);
		generateBufferVariableBufferCaseBlocks(m_context, blockGroup, glslVersion, generateBufferVariableOffsetCases);
	}

	// .referenced_by
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "referenced_by", "Referenced by");
		addChild(blockGroup);
		generateReferencedByShaderCaseBlocks(m_context, blockGroup, glslVersion, generateBufferVariableReferencedByBlockContents);
	}

	// .top_level_array_size
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "top_level_array_size", "Top-level array size");
		addChild(blockGroup);
		generateBufferVariableBufferCaseBlocks(m_context, blockGroup, glslVersion, generateBufferVariableTopLevelCases<PROGRAMRESOURCEPROP_TOP_LEVEL_ARRAY_SIZE>);
	}

	// .top_level_array_stride
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "top_level_array_stride", "Top-level array stride");
		addChild(blockGroup);
		generateBufferVariableBufferCaseBlocks(m_context, blockGroup, glslVersion, generateBufferVariableTopLevelCases<PROGRAMRESOURCEPROP_TOP_LEVEL_ARRAY_STRIDE>);
	}

	// .type
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "type", "Type");
		addChild(blockGroup);
		generateBufferVariableTypeBlock(m_context, blockGroup, glslVersion);
	}

	// .random
	{
		tcu::TestCaseGroup* const blockGroup = new TestCaseGroup(m_context, "random", "Random");
		addChild(blockGroup);
		generateBufferVariableRandomCases(m_context, blockGroup, glslVersion);
	}
}

} // anonymous

ProgramInterfaceQueryTests::ProgramInterfaceQueryTests (Context& context)
	: TestCaseGroup(context, "program_interface_query", "Program interface query tests")
{
}

ProgramInterfaceQueryTests::~ProgramInterfaceQueryTests (void)
{
}

void ProgramInterfaceQueryTests::init (void)
{
	// Misc queries

	// .buffer_limited_query
	{
		tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "buffer_limited_query", "Queries limited by the buffer size");

		addChild(group);

		group->addChild(new ResourceNameBufferLimitCase(m_context, "resource_name_query", "Test GetProgramResourceName with too small a buffer"));
		group->addChild(new ResourceQueryBufferLimitCase(m_context, "resource_query", "Test GetProgramResourceiv with too small a buffer"));
	}

	// Interfaces

	// .uniform
	addChild(new UniformInterfaceTestGroup(m_context));

	// .uniform_block
	addChild(new BufferBackedBlockInterfaceTestGroup(m_context, glu::STORAGE_UNIFORM));

	// .atomic_counter_buffer
	addChild(new AtomicCounterTestGroup(m_context));

	// .program_input
	addChild(new ProgramInputTestGroup(m_context));

	// .program_output
	addChild(new ProgramOutputTestGroup(m_context));

	// .transform_feedback_varying
	addChild(new TransformFeedbackVaryingTestGroup(m_context));

	// .buffer_variable
	addChild(new BufferVariableTestGroup(m_context));

	// .shader_storage_block
	addChild(new BufferBackedBlockInterfaceTestGroup(m_context, glu::STORAGE_BUFFER));
}

} // Functional
} // gles31
} // deqp
