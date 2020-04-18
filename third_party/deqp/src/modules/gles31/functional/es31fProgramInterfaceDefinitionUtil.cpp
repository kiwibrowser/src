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
 * \brief Program interface utilities
 *//*--------------------------------------------------------------------*/

#include "es31fProgramInterfaceDefinitionUtil.hpp"
#include "es31fProgramInterfaceDefinition.hpp"
#include "gluVarType.hpp"
#include "gluVarTypeUtil.hpp"
#include "gluShaderUtil.hpp"
#include "deString.h"
#include "deStringUtil.hpp"
#include "glwEnums.hpp"

#include <set>
#include <map>
#include <sstream>
#include <vector>
#include <algorithm>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace ProgramInterfaceDefinition
{

VariableSearchFilter::VariableSearchFilter (void)
	: m_shaderTypeBits	(0xFFFFFFFFul)
	, m_storageBits		(0xFFFFFFFFul)
{
}

VariableSearchFilter VariableSearchFilter::createShaderTypeFilter (glu::ShaderType type)
{
	DE_ASSERT(type < glu::SHADERTYPE_LAST);

	VariableSearchFilter filter;
	filter.m_shaderTypeBits = (1u << type);
	return filter;
}

VariableSearchFilter VariableSearchFilter::createStorageFilter (glu::Storage storage)
{
	DE_ASSERT(storage < glu::STORAGE_LAST);

	VariableSearchFilter filter;
	filter.m_storageBits = (1u << storage);
	return filter;
}

VariableSearchFilter VariableSearchFilter::createShaderTypeStorageFilter (glu::ShaderType type, glu::Storage storage)
{
	return logicalAnd(createShaderTypeFilter(type), createStorageFilter(storage));
}

VariableSearchFilter VariableSearchFilter::logicalOr (const VariableSearchFilter& a, const VariableSearchFilter& b)
{
	VariableSearchFilter filter;
	filter.m_shaderTypeBits	= a.m_shaderTypeBits | b.m_shaderTypeBits;
	filter.m_storageBits	= a.m_storageBits | b.m_storageBits;
	return filter;
}

VariableSearchFilter VariableSearchFilter::logicalAnd (const VariableSearchFilter& a, const VariableSearchFilter& b)
{
	VariableSearchFilter filter;
	filter.m_shaderTypeBits	= a.m_shaderTypeBits & b.m_shaderTypeBits;
	filter.m_storageBits	= a.m_storageBits & b.m_storageBits;
	return filter;
}

bool VariableSearchFilter::matchesFilter (const ProgramInterfaceDefinition::Shader* shader) const
{
	DE_ASSERT(shader->getType() < glu::SHADERTYPE_LAST);
	return (m_shaderTypeBits & (1u << shader->getType())) != 0;
}

bool VariableSearchFilter::matchesFilter (const glu::VariableDeclaration& variable) const
{
	DE_ASSERT(variable.storage < glu::STORAGE_LAST);
	return (m_storageBits & (1u << variable.storage)) != 0;
}

bool VariableSearchFilter::matchesFilter (const glu::InterfaceBlock& block) const
{
	DE_ASSERT(block.storage < glu::STORAGE_LAST);
	return (m_storageBits & (1u << block.storage)) != 0;
}

} // ProgramInterfaceDefinition

static bool incrementMultiDimensionIndex (std::vector<int>& index, const std::vector<int>& dimensions)
{
	int incrementDimensionNdx = (int)(index.size() - 1);

	while (incrementDimensionNdx >= 0)
	{
		if (++index[incrementDimensionNdx] == dimensions[incrementDimensionNdx])
			index[incrementDimensionNdx--] = 0;
		else
			break;
	}

	return (incrementDimensionNdx != -1);
}

bool programContainsIOBlocks (const ProgramInterfaceDefinition::Program* program)
{
	for (int shaderNdx = 0; shaderNdx < (int)program->getShaders().size(); ++shaderNdx)
	{
		if (shaderContainsIOBlocks(program->getShaders()[shaderNdx]))
			return true;
	}

	return false;
}

bool shaderContainsIOBlocks (const ProgramInterfaceDefinition::Shader* shader)
{
	for (int ndx = 0; ndx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++ndx)
	{
		const glu::Storage storage = shader->getDefaultBlock().interfaceBlocks[ndx].storage;
		if (storage == glu::STORAGE_IN			||
			storage == glu::STORAGE_OUT			||
			storage == glu::STORAGE_PATCH_IN	||
			storage == glu::STORAGE_PATCH_OUT)
		{
			return true;
		}
	}
	return false;
}

glu::ShaderType getProgramTransformFeedbackStage (const ProgramInterfaceDefinition::Program* program)
{
	if (program->hasStage(glu::SHADERTYPE_GEOMETRY))
		return glu::SHADERTYPE_GEOMETRY;

	if (program->hasStage(glu::SHADERTYPE_TESSELLATION_EVALUATION))
		return glu::SHADERTYPE_TESSELLATION_EVALUATION;

	if (program->hasStage(glu::SHADERTYPE_VERTEX))
		return glu::SHADERTYPE_VERTEX;

	DE_ASSERT(false);
	return glu::SHADERTYPE_LAST;
}

void generateVariableTypeResourceNames (std::vector<std::string>& resources, const std::string& name, const glu::VarType& type, deUint32 resourceNameGenerationFlags)
{
	DE_ASSERT((resourceNameGenerationFlags & (~RESOURCE_NAME_GENERATION_FLAG_MASK)) == 0);

	// remove top-level flag from children
	const deUint32 childFlags = resourceNameGenerationFlags & ~((deUint32)RESOURCE_NAME_GENERATION_FLAG_TOP_LEVEL_BUFFER_VARIABLE);

	if (type.isBasicType())
		resources.push_back(name);
	else if (type.isStructType())
	{
		const glu::StructType* structType = type.getStructPtr();
		for (int ndx = 0; ndx < structType->getNumMembers(); ++ndx)
			generateVariableTypeResourceNames(resources, name + "." + structType->getMember(ndx).getName(), structType->getMember(ndx).getType(), childFlags);
	}
	else if (type.isArrayType())
	{
		// Bottom-level arrays of basic types of a transform feedback variable will produce only the first
		// element but without the trailing "[0]"
		if (type.getElementType().isBasicType() &&
			(resourceNameGenerationFlags & RESOURCE_NAME_GENERATION_FLAG_TRANSFORM_FEEDBACK_VARIABLE) != 0)
		{
			resources.push_back(name);
		}
		// Bottom-level arrays of basic types and SSBO top-level arrays of any type procude only first element
		else if (type.getElementType().isBasicType() ||
				 (resourceNameGenerationFlags & RESOURCE_NAME_GENERATION_FLAG_TOP_LEVEL_BUFFER_VARIABLE) != 0)
		{
			generateVariableTypeResourceNames(resources, name + "[0]", type.getElementType(), childFlags);
		}
		// Other arrays of aggregate types are expanded
		else
		{
			for (int ndx = 0; ndx < type.getArraySize(); ++ndx)
				generateVariableTypeResourceNames(resources, name + "[" + de::toString(ndx) + "]", type.getElementType(), childFlags);
		}
	}
	else
		DE_ASSERT(false);
}

// Program source generation

namespace
{

using ProgramInterfaceDefinition::VariablePathComponent;
using ProgramInterfaceDefinition::VariableSearchFilter;

static std::string getShaderExtensionDeclarations (const ProgramInterfaceDefinition::Shader* shader)
{
	std::vector<std::string>	extensions;
	std::ostringstream			buf;

	if (shader->getType() == glu::SHADERTYPE_GEOMETRY)
	{
		extensions.push_back("GL_EXT_geometry_shader");
	}
	else if (shader->getType() == glu::SHADERTYPE_TESSELLATION_CONTROL ||
			 shader->getType() == glu::SHADERTYPE_TESSELLATION_EVALUATION)
	{
		extensions.push_back("GL_EXT_tessellation_shader");
	}

	if (shaderContainsIOBlocks(shader))
		extensions.push_back("GL_EXT_shader_io_blocks");

	for (int ndx = 0; ndx < (int)extensions.size(); ++ndx)
		buf << "#extension " << extensions[ndx] << " : require\n";
	return buf.str();
}

static std::string getShaderTypeDeclarations (const ProgramInterfaceDefinition::Program* program, glu::ShaderType type)
{
	switch (type)
	{
		case glu::SHADERTYPE_VERTEX:
			return "";

		case glu::SHADERTYPE_FRAGMENT:
			return "";

		case glu::SHADERTYPE_GEOMETRY:
		{
			std::ostringstream buf;
			buf <<	"layout(points) in;\n"
					"layout(points, max_vertices=" << program->getGeometryNumOutputVertices() << ") out;\n";
			return buf.str();
		}

		case glu::SHADERTYPE_TESSELLATION_CONTROL:
		{
			std::ostringstream buf;
			buf << "layout(vertices=" << program->getTessellationNumOutputPatchVertices() << ") out;\n";
			return buf.str();
		}

		case glu::SHADERTYPE_TESSELLATION_EVALUATION:
			return "layout(triangles, point_mode) in;\n";

		case glu::SHADERTYPE_COMPUTE:
			return "layout(local_size_x=1) in;\n";

		default:
			DE_ASSERT(false);
			return "";
	}
}

class StructNameEqualPredicate
{
public:
				StructNameEqualPredicate	(const char* name) : m_name(name) { }
	bool		operator()					(const glu::StructType* type) { return type->hasTypeName() && (deStringEqual(m_name, type->getTypeName()) == DE_TRUE); }
private:
	const char*	m_name;
};

static void collectNamedStructureDefinitions (std::vector<const glu::StructType*>& dst, const glu::VarType& type)
{
	if (type.isBasicType())
		return;
	else if (type.isArrayType())
		return collectNamedStructureDefinitions(dst, type.getElementType());
	else if (type.isStructType())
	{
		if (type.getStructPtr()->hasTypeName())
		{
			// must be unique (may share the the same struct)
			std::vector<const glu::StructType*>::iterator where = std::find_if(dst.begin(), dst.end(), StructNameEqualPredicate(type.getStructPtr()->getTypeName()));
			if (where != dst.end())
			{
				DE_ASSERT(**where == *type.getStructPtr());

				// identical type has been added already, types of members must be added too
				return;
			}
		}

		// Add types of members first
		for (int ndx = 0; ndx < type.getStructPtr()->getNumMembers(); ++ndx)
			collectNamedStructureDefinitions(dst, type.getStructPtr()->getMember(ndx).getType());

		dst.push_back(type.getStructPtr());
	}
	else
		DE_ASSERT(false);
}

static void writeStructureDefinitions (std::ostringstream& buf, const ProgramInterfaceDefinition::DefaultBlock& defaultBlock)
{
	std::vector<const glu::StructType*> namedStructs;

	// Collect all structs in post order

	for (int ndx = 0; ndx < (int)defaultBlock.variables.size(); ++ndx)
		collectNamedStructureDefinitions(namedStructs, defaultBlock.variables[ndx].varType);

	for (int blockNdx = 0; blockNdx < (int)defaultBlock.interfaceBlocks.size(); ++blockNdx)
		for (int ndx = 0; ndx < (int)defaultBlock.interfaceBlocks[blockNdx].variables.size(); ++ndx)
			collectNamedStructureDefinitions(namedStructs, defaultBlock.interfaceBlocks[blockNdx].variables[ndx].varType);

	// Write

	for (int structNdx = 0; structNdx < (int)namedStructs.size(); ++structNdx)
	{
		buf <<	"struct " << namedStructs[structNdx]->getTypeName() << "\n"
				"{\n";

		for (int memberNdx = 0; memberNdx < namedStructs[structNdx]->getNumMembers(); ++memberNdx)
			buf << glu::indent(1) << glu::declare(namedStructs[structNdx]->getMember(memberNdx).getType(), namedStructs[structNdx]->getMember(memberNdx).getName(), 1) << ";\n";

		buf <<	"};\n";
	}

	if (!namedStructs.empty())
		buf << "\n";
}

static void writeInterfaceBlock (std::ostringstream& buf, const glu::InterfaceBlock& interfaceBlock)
{
	buf << interfaceBlock.layout;

	if (interfaceBlock.layout != glu::Layout())
		buf << " ";

	buf	<< glu::getStorageName(interfaceBlock.storage) << " " << interfaceBlock.interfaceName << "\n"
		<< "{\n";

	for (int ndx = 0; ndx < (int)interfaceBlock.variables.size(); ++ndx)
		buf << glu::indent(1) << interfaceBlock.variables[ndx] << ";\n";

	buf << "}";

	if (!interfaceBlock.instanceName.empty())
		buf << " " << interfaceBlock.instanceName;

	for (int dimensionNdx = 0; dimensionNdx < (int)interfaceBlock.dimensions.size(); ++dimensionNdx)
		buf << "[" << interfaceBlock.dimensions[dimensionNdx] << "]";

	buf << ";\n\n";
}

static bool isReadableInterface (const glu::InterfaceBlock& interface)
{
	return	interface.storage == glu::STORAGE_UNIFORM	||
			interface.storage == glu::STORAGE_IN		||
			interface.storage == glu::STORAGE_PATCH_IN	||
			(interface.storage == glu::STORAGE_BUFFER && (interface.memoryAccessQualifierFlags & glu::MEMORYACCESSQUALIFIER_WRITEONLY_BIT) == 0);
}

static bool isWritableInterface (const glu::InterfaceBlock& interface)
{
	return	interface.storage == glu::STORAGE_OUT		||
			interface.storage == glu::STORAGE_PATCH_OUT	||
			(interface.storage == glu::STORAGE_BUFFER && (interface.memoryAccessQualifierFlags & glu::MEMORYACCESSQUALIFIER_READONLY_BIT) == 0);
}


static void writeVariableReadAccumulateExpression (std::ostringstream&							buf,
												   const std::string&							accumulatorName,
												   const std::string&							name,
												   glu::ShaderType								shaderType,
												   glu::Storage									storage,
												   const ProgramInterfaceDefinition::Program*	program,
												   const glu::VarType&							varType)
{
	if (varType.isBasicType())
	{
		buf << "\t" << accumulatorName << " += ";

		if (glu::isDataTypeScalar(varType.getBasicType()))
			buf << "vec4(float(" << name << "))";
		else if (glu::isDataTypeVector(varType.getBasicType()))
			buf << "vec4(" << name << ".xyxy)";
		else if (glu::isDataTypeMatrix(varType.getBasicType()))
			buf << "vec4(float(" << name << "[0][0]))";
		else if (glu::isDataTypeSamplerMultisample(varType.getBasicType()))
			buf << "vec4(float(textureSize(" << name << ").x))";
		else if (glu::isDataTypeSampler(varType.getBasicType()))
			buf << "vec4(float(textureSize(" << name << ", 0).x))";
		else if (glu::isDataTypeImage(varType.getBasicType()))
			buf << "vec4(float(imageSize(" << name << ").x))";
		else if (varType.getBasicType() == glu::TYPE_UINT_ATOMIC_COUNTER)
			buf << "vec4(float(atomicCounterIncrement(" << name << ")))";
		else
			DE_ASSERT(false);

		buf << ";\n";
	}
	else if (varType.isStructType())
	{
		for (int ndx = 0; ndx < varType.getStructPtr()->getNumMembers(); ++ndx)
			writeVariableReadAccumulateExpression(buf,
												  accumulatorName,
												  name + "." + varType.getStructPtr()->getMember(ndx).getName(),
												  shaderType,
												  storage,
												  program,
												  varType.getStructPtr()->getMember(ndx).getType());
	}
	else if (varType.isArrayType())
	{
		if (varType.getArraySize() != glu::VarType::UNSIZED_ARRAY)
		{
			for (int ndx = 0; ndx < varType.getArraySize(); ++ndx)
				writeVariableReadAccumulateExpression(buf,
													  accumulatorName,
													  name + "[" + de::toString(ndx) + "]",
													  shaderType,
													  storage,
													  program,
													  varType.getElementType());
		}
		else if (storage == glu::STORAGE_BUFFER)
		{
			// run-time sized array, read arbitrary
			writeVariableReadAccumulateExpression(buf,
												  accumulatorName,
												  name + "[8]",
												  shaderType,
												  storage,
												  program,
												  varType.getElementType());
		}
		else
		{
			DE_ASSERT(storage == glu::STORAGE_IN);

			if (shaderType == glu::SHADERTYPE_GEOMETRY)
			{
				// implicit sized geometry input array, size = primitive size. Just reading first is enough
				writeVariableReadAccumulateExpression(buf,
													  accumulatorName,
													  name + "[0]",
													  shaderType,
													  storage,
													  program,
													  varType.getElementType());
			}
			else if (shaderType == glu::SHADERTYPE_TESSELLATION_CONTROL)
			{
				// implicit sized tessellation input array, size = input patch max size. Just reading current is enough
				writeVariableReadAccumulateExpression(buf,
													  accumulatorName,
													  name + "[gl_InvocationID]",
													  shaderType,
													  storage,
													  program,
													  varType.getElementType());
			}
			else if (shaderType == glu::SHADERTYPE_TESSELLATION_EVALUATION)
			{
				// implicit sized tessellation input array, size = output patch max size. Read all to prevent optimizations
				DE_ASSERT(program->getTessellationNumOutputPatchVertices() > 0);
				for (int ndx = 0; ndx < (int)program->getTessellationNumOutputPatchVertices(); ++ndx)
				{
					writeVariableReadAccumulateExpression(buf,
														  accumulatorName,
														  name + "[" + de::toString(ndx) + "]",
														  shaderType,
														  storage,
														  program,
														  varType.getElementType());
				}
			}
			else
				DE_ASSERT(false);
		}
	}
	else
		DE_ASSERT(false);
}

static void writeInterfaceReadAccumulateExpression (std::ostringstream&							buf,
													const std::string&							accumulatorName,
													const glu::InterfaceBlock&					block,
													glu::ShaderType								shaderType,
													const ProgramInterfaceDefinition::Program*	program)
{
	if (block.dimensions.empty())
	{
		const std::string prefix = (block.instanceName.empty()) ? ("") : (block.instanceName + ".");

		for (int ndx = 0; ndx < (int)block.variables.size(); ++ndx)
		{
			writeVariableReadAccumulateExpression(buf,
												  accumulatorName,
												  prefix + block.variables[ndx].name,
												  shaderType,
												  block.storage,
												  program,
												  block.variables[ndx].varType);
		}
	}
	else
	{
		std::vector<int> index(block.dimensions.size(), 0);

		for (;;)
		{
			// access element
			{
				std::ostringstream name;
				name << block.instanceName;

				for (int dimensionNdx = 0; dimensionNdx < (int)block.dimensions.size(); ++dimensionNdx)
					name << "[" << index[dimensionNdx] << "]";

				for (int ndx = 0; ndx < (int)block.variables.size(); ++ndx)
				{
					writeVariableReadAccumulateExpression(buf,
														  accumulatorName,
														  name.str() + "." + block.variables[ndx].name,
														  shaderType,
														  block.storage,
														  program,
														  block.variables[ndx].varType);
				}
			}

			// increment index
			if (!incrementMultiDimensionIndex(index, block.dimensions))
				break;
		}
	}
}

static void writeVariableWriteExpression (std::ostringstream&							buf,
										  const std::string&							sourceVec4Name,
										  const std::string&							name,
										  glu::ShaderType								shaderType,
										  glu::Storage									storage,
										  const ProgramInterfaceDefinition::Program*	program,
										  const glu::VarType&							varType)
{
	if (varType.isBasicType())
	{
		buf << "\t" << name << " = ";

		if (glu::isDataTypeScalar(varType.getBasicType()))
			buf << glu::getDataTypeName(varType.getBasicType()) << "(" << sourceVec4Name << ".y)";
		else if (glu::isDataTypeVector(varType.getBasicType()) || glu::isDataTypeMatrix(varType.getBasicType()))
			buf << glu::getDataTypeName(varType.getBasicType()) << "(" << glu::getDataTypeName(glu::getDataTypeScalarType(varType.getBasicType())) << "(" << sourceVec4Name << ".y))";
		else
			DE_ASSERT(false);

		buf << ";\n";
	}
	else if (varType.isStructType())
	{
		for (int ndx = 0; ndx < varType.getStructPtr()->getNumMembers(); ++ndx)
			writeVariableWriteExpression(buf,
										 sourceVec4Name,
										 name + "." + varType.getStructPtr()->getMember(ndx).getName(),
										 shaderType,
										 storage,
										 program,
										 varType.getStructPtr()->getMember(ndx).getType());
	}
	else if (varType.isArrayType())
	{
		if (varType.getArraySize() != glu::VarType::UNSIZED_ARRAY)
		{
			for (int ndx = 0; ndx < varType.getArraySize(); ++ndx)
				writeVariableWriteExpression(buf,
											 sourceVec4Name,
											 name + "[" + de::toString(ndx) + "]",
											 shaderType,
											 storage,
											 program,
											 varType.getElementType());
		}
		else if (storage == glu::STORAGE_BUFFER)
		{
			// run-time sized array, write arbitrary
			writeVariableWriteExpression(buf,
										 sourceVec4Name,
										 name + "[9]",
										 shaderType,
										 storage,
										 program,
										 varType.getElementType());
		}
		else
		{
			DE_ASSERT(storage == glu::STORAGE_OUT);

			if (shaderType == glu::SHADERTYPE_TESSELLATION_CONTROL)
			{
				// implicit sized tessellation onput array, size = output patch max size. Can only write to gl_InvocationID
				writeVariableWriteExpression(buf,
											 sourceVec4Name,
											 name + "[gl_InvocationID]",
											 shaderType,
											 storage,
											 program,
											 varType.getElementType());
			}
			else
				DE_ASSERT(false);
		}
	}
	else
		DE_ASSERT(false);
}

static void writeInterfaceWriteExpression (std::ostringstream&							buf,
										   const std::string&							sourceVec4Name,
										   const glu::InterfaceBlock&					block,
										   glu::ShaderType								shaderType,
										   const ProgramInterfaceDefinition::Program*	program)
{
	if (block.dimensions.empty())
	{
		const std::string prefix = (block.instanceName.empty()) ? ("") : (block.instanceName + ".");

		for (int ndx = 0; ndx < (int)block.variables.size(); ++ndx)
		{
			writeVariableWriteExpression(buf,
										 sourceVec4Name,
										 prefix + block.variables[ndx].name,
										 shaderType,
										 block.storage,
										 program,
										 block.variables[ndx].varType);
		}
	}
	else
	{
		std::vector<int> index(block.dimensions.size(), 0);

		for (;;)
		{
			// access element
			{
				std::ostringstream name;
				name << block.instanceName;

				for (int dimensionNdx = 0; dimensionNdx < (int)block.dimensions.size(); ++dimensionNdx)
					name << "[" << index[dimensionNdx] << "]";

				for (int ndx = 0; ndx < (int)block.variables.size(); ++ndx)
				{
					writeVariableWriteExpression(buf,
												 sourceVec4Name,
												 name.str() + "." + block.variables[ndx].name,
												 shaderType,
												 block.storage,
												 program,
												 block.variables[ndx].varType);
				}
			}

			// increment index
			if (!incrementMultiDimensionIndex(index, block.dimensions))
				break;
		}
	}
}

static bool traverseVariablePath (std::vector<VariablePathComponent>& typePath, const char* subPath, const glu::VarType& type)
{
	glu::VarTokenizer tokenizer(subPath);

	typePath.push_back(VariablePathComponent(&type));

	if (tokenizer.getToken() == glu::VarTokenizer::TOKEN_END)
		return true;

	if (type.isStructType() && tokenizer.getToken() == glu::VarTokenizer::TOKEN_PERIOD)
	{
		tokenizer.advance();

		// malformed path
		if (tokenizer.getToken() != glu::VarTokenizer::TOKEN_IDENTIFIER)
			return false;

		for (int memberNdx = 0; memberNdx < type.getStructPtr()->getNumMembers(); ++memberNdx)
			if (type.getStructPtr()->getMember(memberNdx).getName() == tokenizer.getIdentifier())
				return traverseVariablePath(typePath, subPath + tokenizer.getCurrentTokenEndLocation(), type.getStructPtr()->getMember(memberNdx).getType());

		// malformed path, no such member
		return false;
	}
	else if (type.isArrayType() && tokenizer.getToken() == glu::VarTokenizer::TOKEN_LEFT_BRACKET)
	{
		tokenizer.advance();

		// malformed path
		if (tokenizer.getToken() != glu::VarTokenizer::TOKEN_NUMBER)
			return false;

		tokenizer.advance();
		if (tokenizer.getToken() != glu::VarTokenizer::TOKEN_RIGHT_BRACKET)
			return false;

		return traverseVariablePath(typePath, subPath + tokenizer.getCurrentTokenEndLocation(), type.getElementType());
	}

	return false;
}

static bool traverseVariablePath (std::vector<VariablePathComponent>& typePath, const std::string& path, const glu::VariableDeclaration& var)
{
	if (glu::parseVariableName(path.c_str()) != var.name)
		return false;

	typePath.push_back(VariablePathComponent(&var));
	return traverseVariablePath(typePath, path.c_str() + var.name.length(), var.varType);
}

static bool traverseShaderVariablePath (std::vector<VariablePathComponent>& typePath, const ProgramInterfaceDefinition::Shader* shader, const std::string& path, const VariableSearchFilter& filter)
{
	// Default block variable?
	for (int varNdx = 0; varNdx < (int)shader->getDefaultBlock().variables.size(); ++varNdx)
		if (filter.matchesFilter(shader->getDefaultBlock().variables[varNdx]))
			if (traverseVariablePath(typePath, path, shader->getDefaultBlock().variables[varNdx]))
				return true;

	// is variable an interface block variable?
	{
		const std::string blockName = glu::parseVariableName(path.c_str());

		for (int interfaceNdx = 0; interfaceNdx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++interfaceNdx)
		{
			if (!filter.matchesFilter(shader->getDefaultBlock().interfaceBlocks[interfaceNdx]))
				continue;

			if (shader->getDefaultBlock().interfaceBlocks[interfaceNdx].interfaceName == blockName)
			{
				// resource is a member of a named interface block
				// \note there is no array index specifier even if the interface is declared as an array of instances
				const std::string blockMemberPath = path.substr(blockName.size() + 1);
				const std::string blockMemeberName = glu::parseVariableName(blockMemberPath.c_str());

				for (int varNdx = 0; varNdx < (int)shader->getDefaultBlock().interfaceBlocks[interfaceNdx].variables.size(); ++varNdx)
				{
					if (shader->getDefaultBlock().interfaceBlocks[interfaceNdx].variables[varNdx].name == blockMemeberName)
					{
						typePath.push_back(VariablePathComponent(&shader->getDefaultBlock().interfaceBlocks[interfaceNdx]));
						return traverseVariablePath(typePath, blockMemberPath, shader->getDefaultBlock().interfaceBlocks[interfaceNdx].variables[varNdx]);
					}
				}

				// terminate search
				return false;
			}
			else if (shader->getDefaultBlock().interfaceBlocks[interfaceNdx].instanceName.empty())
			{
				const std::string blockMemeberName = glu::parseVariableName(path.c_str());

				// unnamed block contains such variable?
				for (int varNdx = 0; varNdx < (int)shader->getDefaultBlock().interfaceBlocks[interfaceNdx].variables.size(); ++varNdx)
				{
					if (shader->getDefaultBlock().interfaceBlocks[interfaceNdx].variables[varNdx].name == blockMemeberName)
					{
						typePath.push_back(VariablePathComponent(&shader->getDefaultBlock().interfaceBlocks[interfaceNdx]));
						return traverseVariablePath(typePath, path, shader->getDefaultBlock().interfaceBlocks[interfaceNdx].variables[varNdx]);
					}
				}

				// continue search
			}
		}
	}

	return false;
}

static bool traverseProgramVariablePath (std::vector<VariablePathComponent>& typePath, const ProgramInterfaceDefinition::Program* program, const std::string& path, const VariableSearchFilter& filter)
{
	for (int shaderNdx = 0; shaderNdx < (int)program->getShaders().size(); ++shaderNdx)
	{
		const ProgramInterfaceDefinition::Shader* shader = program->getShaders()[shaderNdx];

		if (filter.matchesFilter(shader))
		{
			// \note modifying output variable even when returning false
			typePath.clear();
			if (traverseShaderVariablePath(typePath, shader, path, filter))
				return true;
		}
	}

	return false;
}

static bool containsSubType (const glu::VarType& complexType, glu::DataType basicType)
{
	if (complexType.isBasicType())
	{
		return complexType.getBasicType() == basicType;
	}
	else if (complexType.isArrayType())
	{
		return containsSubType(complexType.getElementType(), basicType);
	}
	else if (complexType.isStructType())
	{
		for (int ndx = 0; ndx < complexType.getStructPtr()->getNumMembers(); ++ndx)
			if (containsSubType(complexType.getStructPtr()->getMember(ndx).getType(), basicType))
				return true;
		return false;
	}
	else
	{
		DE_ASSERT(false);
		return false;
	}
}

static int getNumShaderBlocks (const ProgramInterfaceDefinition::Shader* shader, glu::Storage storage)
{
	int retVal = 0;

	for (int ndx = 0; ndx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++ndx)
	{
		if (shader->getDefaultBlock().interfaceBlocks[ndx].storage == storage)
		{
			int numInstances = 1;

			for (int dimensionNdx = 0; dimensionNdx < (int)shader->getDefaultBlock().interfaceBlocks[ndx].dimensions.size(); ++dimensionNdx)
				numInstances *= shader->getDefaultBlock().interfaceBlocks[ndx].dimensions[dimensionNdx];

			retVal += numInstances;
		}
	}

	return retVal;
}

static int getNumAtomicCounterBuffers (const ProgramInterfaceDefinition::Shader* shader)
{
	std::set<int> buffers;

	for (int ndx = 0; ndx < (int)shader->getDefaultBlock().variables.size(); ++ndx)
	{
		if (containsSubType(shader->getDefaultBlock().variables[ndx].varType, glu::TYPE_UINT_ATOMIC_COUNTER))
		{
			DE_ASSERT(shader->getDefaultBlock().variables[ndx].layout.binding != -1);
			buffers.insert(shader->getDefaultBlock().variables[ndx].layout.binding);
		}
	}

	return (int)buffers.size();
}

template <typename DataTypeMap>
static int accumulateComplexType (const glu::VarType& complexType, const DataTypeMap& dTypeMap)
{
	if (complexType.isBasicType())
		return dTypeMap(complexType.getBasicType());
	else if (complexType.isArrayType())
	{
		const int arraySize = (complexType.getArraySize() == glu::VarType::UNSIZED_ARRAY) ? (1) : (complexType.getArraySize());
		return arraySize * accumulateComplexType(complexType.getElementType(), dTypeMap);
	}
	else if (complexType.isStructType())
	{
		int sum = 0;
		for (int ndx = 0; ndx < complexType.getStructPtr()->getNumMembers(); ++ndx)
			sum += accumulateComplexType(complexType.getStructPtr()->getMember(ndx).getType(), dTypeMap);
		return sum;
	}
	else
	{
		DE_ASSERT(false);
		return false;
	}
}

template <typename InterfaceBlockFilter, typename VarDeclFilter, typename DataTypeMap>
static int accumulateShader (const ProgramInterfaceDefinition::Shader* shader,
							 const InterfaceBlockFilter& ibFilter,
							 const VarDeclFilter& vdFilter,
							 const DataTypeMap& dMap)
{
	int retVal = 0;

	for (int ndx = 0; ndx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++ndx)
	{
		if (ibFilter(shader->getDefaultBlock().interfaceBlocks[ndx]))
		{
			int numInstances = 1;

			for (int dimensionNdx = 0; dimensionNdx < (int)shader->getDefaultBlock().interfaceBlocks[ndx].dimensions.size(); ++dimensionNdx)
				numInstances *= shader->getDefaultBlock().interfaceBlocks[ndx].dimensions[dimensionNdx];

			for (int varNdx = 0; varNdx < (int)shader->getDefaultBlock().interfaceBlocks[ndx].variables.size(); ++varNdx)
				retVal += numInstances * accumulateComplexType(shader->getDefaultBlock().interfaceBlocks[ndx].variables[varNdx].varType, dMap);
		}
	}

	for (int varNdx = 0; varNdx < (int)shader->getDefaultBlock().variables.size(); ++varNdx)
		if (vdFilter(shader->getDefaultBlock().variables[varNdx]))
			retVal += accumulateComplexType(shader->getDefaultBlock().variables[varNdx].varType, dMap);

	return retVal;
}

static bool dummyTrueConstantTypeFilter (glu::DataType d)
{
	DE_UNREF(d);
	return true;
}

class InstanceCounter
{
public:
	InstanceCounter (bool (*predicate)(glu::DataType))
		: m_predicate(predicate)
	{
	}

	int operator() (glu::DataType t) const
	{
		return (m_predicate(t)) ? (1) : (0);
	}

private:
	bool (*const m_predicate)(glu::DataType);
};

class InterfaceBlockStorageFilter
{
public:
	InterfaceBlockStorageFilter (glu::Storage storage)
		: m_storage(storage)
	{
	}

	bool operator() (const glu::InterfaceBlock& b) const
	{
		return m_storage == b.storage;
	}

private:
	const glu::Storage m_storage;
};

class VariableDeclarationStorageFilter
{
public:
	VariableDeclarationStorageFilter (glu::Storage storage)
		: m_storage(storage)
	{
	}

	bool operator() (const glu::VariableDeclaration& d) const
	{
		return m_storage == d.storage;
	}

private:
	const glu::Storage m_storage;
};

static int getNumTypeInstances (const glu::VarType& complexType, bool (*predicate)(glu::DataType))
{
	return accumulateComplexType(complexType, InstanceCounter(predicate));
}

static int getNumTypeInstances (const ProgramInterfaceDefinition::Shader* shader, glu::Storage storage, bool (*predicate)(glu::DataType))
{
	return accumulateShader(shader, InterfaceBlockStorageFilter(storage), VariableDeclarationStorageFilter(storage), InstanceCounter(predicate));
}

static int getNumTypeInstances (const ProgramInterfaceDefinition::Shader* shader, glu::Storage storage)
{
	return getNumTypeInstances(shader, storage, dummyTrueConstantTypeFilter);
}

static int accumulateShaderStorage (const ProgramInterfaceDefinition::Shader* shader, glu::Storage storage, int (*typeMap)(glu::DataType))
{
	return accumulateShader(shader, InterfaceBlockStorageFilter(storage), VariableDeclarationStorageFilter(storage), typeMap);
}

static int getNumDataTypeComponents (glu::DataType type)
{
	if (glu::isDataTypeScalarOrVector(type) || glu::isDataTypeMatrix(type))
		return glu::getDataTypeScalarSize(type);
	else
		return 0;
}

static int getNumDataTypeVectors (glu::DataType type)
{
	if (glu::isDataTypeScalar(type))
		return 1;
	else if (glu::isDataTypeVector(type))
		return 1;
	else if (glu::isDataTypeMatrix(type))
		return glu::getDataTypeMatrixNumColumns(type);
	else
		return 0;
}

static int getNumComponents (const ProgramInterfaceDefinition::Shader* shader, glu::Storage storage)
{
	return accumulateShaderStorage(shader, storage, getNumDataTypeComponents);
}

static int getNumVectors (const ProgramInterfaceDefinition::Shader* shader, glu::Storage storage)
{
	return accumulateShaderStorage(shader, storage, getNumDataTypeVectors);
}

static int getNumDefaultBlockComponents (const ProgramInterfaceDefinition::Shader* shader, glu::Storage storage)
{
	int retVal = 0;

	for (int varNdx = 0; varNdx < (int)shader->getDefaultBlock().variables.size(); ++varNdx)
		if (shader->getDefaultBlock().variables[varNdx].storage == storage)
			retVal += accumulateComplexType(shader->getDefaultBlock().variables[varNdx].varType, getNumDataTypeComponents);

	return retVal;
}

static int getMaxBufferBinding (const ProgramInterfaceDefinition::Shader* shader, glu::Storage storage)
{
	int maxBinding = -1;

	for (int ndx = 0; ndx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++ndx)
	{
		if (shader->getDefaultBlock().interfaceBlocks[ndx].storage == storage)
		{
			const int	binding			= (shader->getDefaultBlock().interfaceBlocks[ndx].layout.binding == -1) ? (0) : (shader->getDefaultBlock().interfaceBlocks[ndx].layout.binding);
			int			numInstances	= 1;

			for (int dimensionNdx = 0; dimensionNdx < (int)shader->getDefaultBlock().interfaceBlocks[ndx].dimensions.size(); ++dimensionNdx)
				numInstances *= shader->getDefaultBlock().interfaceBlocks[ndx].dimensions[dimensionNdx];

			maxBinding = de::max(maxBinding, binding + numInstances - 1);
		}
	}

	return (int)maxBinding;
}

static int getBufferTypeSize (glu::DataType type, glu::MatrixOrder order)
{
	// assume vec4 alignments, should produce values greater than or equal to the actual resource usage
	int numVectors = 0;

	if (glu::isDataTypeScalarOrVector(type))
		numVectors = 1;
	else if (glu::isDataTypeMatrix(type) && order == glu::MATRIXORDER_ROW_MAJOR)
		numVectors = glu::getDataTypeMatrixNumRows(type);
	else if (glu::isDataTypeMatrix(type) && order != glu::MATRIXORDER_ROW_MAJOR)
		numVectors = glu::getDataTypeMatrixNumColumns(type);
	else
		DE_ASSERT(false);

	return 4 * numVectors;
}

static int getBufferVariableSize (const glu::VarType& type, glu::MatrixOrder order)
{
	if (type.isBasicType())
		return getBufferTypeSize(type.getBasicType(), order);
	else if (type.isArrayType())
	{
		const int arraySize = (type.getArraySize() == glu::VarType::UNSIZED_ARRAY) ? (1) : (type.getArraySize());
		return arraySize * getBufferVariableSize(type.getElementType(), order);
	}
	else if (type.isStructType())
	{
		int sum = 0;
		for (int ndx = 0; ndx < type.getStructPtr()->getNumMembers(); ++ndx)
			sum += getBufferVariableSize(type.getStructPtr()->getMember(ndx).getType(), order);
		return sum;
	}
	else
	{
		DE_ASSERT(false);
		return false;
	}
}

static int getBufferSize (const glu::InterfaceBlock& block, glu::MatrixOrder blockOrder)
{
	int size = 0;

	for (int ndx = 0; ndx < (int)block.variables.size(); ++ndx)
		size += getBufferVariableSize(block.variables[ndx].varType, (block.variables[ndx].layout.matrixOrder == glu::MATRIXORDER_LAST) ? (blockOrder) : (block.variables[ndx].layout.matrixOrder));

	return size;
}

static int getBufferMaxSize (const ProgramInterfaceDefinition::Shader* shader, glu::Storage storage)
{
	int maxSize = 0;

	for (int ndx = 0; ndx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++ndx)
		if (shader->getDefaultBlock().interfaceBlocks[ndx].storage == storage)
			maxSize = de::max(maxSize, getBufferSize(shader->getDefaultBlock().interfaceBlocks[ndx], shader->getDefaultBlock().interfaceBlocks[ndx].layout.matrixOrder));

	return (int)maxSize;
}

static int getAtomicCounterMaxBinding (const ProgramInterfaceDefinition::Shader* shader)
{
	int maxBinding = -1;

	for (int ndx = 0; ndx < (int)shader->getDefaultBlock().variables.size(); ++ndx)
	{
		if (containsSubType(shader->getDefaultBlock().variables[ndx].varType, glu::TYPE_UINT_ATOMIC_COUNTER))
		{
			DE_ASSERT(shader->getDefaultBlock().variables[ndx].layout.binding != -1);
			maxBinding = de::max(maxBinding, shader->getDefaultBlock().variables[ndx].layout.binding);
		}
	}

	return (int)maxBinding;
}

static int getUniformMaxBinding (const ProgramInterfaceDefinition::Shader* shader, bool (*predicate)(glu::DataType))
{
	int maxBinding = -1;

	for (int ndx = 0; ndx < (int)shader->getDefaultBlock().variables.size(); ++ndx)
	{
		const int binding		= (shader->getDefaultBlock().variables[ndx].layout.binding == -1) ? (0) : (shader->getDefaultBlock().variables[ndx].layout.binding);
		const int numInstances	= getNumTypeInstances(shader->getDefaultBlock().variables[ndx].varType, predicate);

		maxBinding = de::max(maxBinding, binding + numInstances - 1);
	}

	return maxBinding;
}

static int getAtomicCounterMaxBufferSize (const ProgramInterfaceDefinition::Shader* shader)
{
	std::map<int, int>	bufferSizes;
	int					maxSize			= 0;

	for (int ndx = 0; ndx < (int)shader->getDefaultBlock().variables.size(); ++ndx)
	{
		if (containsSubType(shader->getDefaultBlock().variables[ndx].varType, glu::TYPE_UINT_ATOMIC_COUNTER))
		{
			const int bufferBinding	= shader->getDefaultBlock().variables[ndx].layout.binding;
			const int offset		= (shader->getDefaultBlock().variables[ndx].layout.offset == -1) ? (0) : (shader->getDefaultBlock().variables[ndx].layout.offset);
			const int size			= offset + 4 * getNumTypeInstances(shader->getDefaultBlock().variables[ndx].varType, glu::isDataTypeAtomicCounter);

			DE_ASSERT(shader->getDefaultBlock().variables[ndx].layout.binding != -1);

			if (bufferSizes.find(bufferBinding) == bufferSizes.end())
				bufferSizes[bufferBinding] = size;
			else
				bufferSizes[bufferBinding] = de::max<int>(bufferSizes[bufferBinding], size);
		}
	}

	for (std::map<int, int>::iterator it = bufferSizes.begin(); it != bufferSizes.end(); ++it)
		maxSize = de::max<int>(maxSize, it->second);

	return maxSize;
}

static int getNumFeedbackVaryingComponents (const ProgramInterfaceDefinition::Program* program, const std::string& name)
{
	std::vector<VariablePathComponent> path;

	if (name == "gl_Position")
		return 4;

	DE_ASSERT(deStringBeginsWith(name.c_str(), "gl_") == DE_FALSE);

	if (!traverseProgramVariablePath(path, program, name, VariableSearchFilter::createShaderTypeStorageFilter(getProgramTransformFeedbackStage(program), glu::STORAGE_OUT)))
		DE_ASSERT(false); // Program failed validate, invalid operation

	return accumulateComplexType(*path.back().getVariableType(), getNumDataTypeComponents);
}

static int getNumXFBComponents (const ProgramInterfaceDefinition::Program* program)
{
	int numComponents = 0;

	for (int ndx = 0; ndx < (int)program->getTransformFeedbackVaryings().size(); ++ndx)
		numComponents += getNumFeedbackVaryingComponents(program, program->getTransformFeedbackVaryings()[ndx]);

	return numComponents;
}

static int getNumMaxXFBOutputComponents (const ProgramInterfaceDefinition::Program* program)
{
	int numComponents = 0;

	for (int ndx = 0; ndx < (int)program->getTransformFeedbackVaryings().size(); ++ndx)
		numComponents = de::max(numComponents, getNumFeedbackVaryingComponents(program, program->getTransformFeedbackVaryings()[ndx]));

	return numComponents;
}

static int getFragmentOutputMaxLocation (const ProgramInterfaceDefinition::Shader* shader)
{
	DE_ASSERT(shader->getType() == glu::SHADERTYPE_FRAGMENT);

	int maxOutputLocation = -1;

	for (int ndx = 0; ndx < (int)shader->getDefaultBlock().variables.size(); ++ndx)
	{
		if (shader->getDefaultBlock().variables[ndx].storage == glu::STORAGE_OUT)
		{
			// missing location qualifier means location == 0
			const int outputLocation		= (shader->getDefaultBlock().variables[ndx].layout.location == -1)
												? (0)
												: (shader->getDefaultBlock().variables[ndx].layout.location);

			// only basic types or arrays of basic types possible
			DE_ASSERT(!shader->getDefaultBlock().variables[ndx].varType.isStructType());

			const int locationSlotsTaken	= (shader->getDefaultBlock().variables[ndx].varType.isArrayType())
												? (shader->getDefaultBlock().variables[ndx].varType.getArraySize())
												: (1);

			maxOutputLocation = de::max(maxOutputLocation, outputLocation + locationSlotsTaken - 1);
		}
	}

	return maxOutputLocation;
}

} // anonymous

std::vector<std::string> getProgramInterfaceBlockMemberResourceList (const glu::InterfaceBlock& interfaceBlock)
{
	const std::string			namePrefix					= (!interfaceBlock.instanceName.empty()) ? (interfaceBlock.interfaceName + ".") : ("");
	const bool					isTopLevelBufferVariable	= (interfaceBlock.storage == glu::STORAGE_BUFFER);
	std::vector<std::string>	resources;

	// \note this is defined in the GLSL spec, not in the GL spec
	for (int variableNdx = 0; variableNdx < (int)interfaceBlock.variables.size(); ++variableNdx)
		generateVariableTypeResourceNames(resources,
										  namePrefix + interfaceBlock.variables[variableNdx].name,
										  interfaceBlock.variables[variableNdx].varType,
										  (isTopLevelBufferVariable) ?
											(RESOURCE_NAME_GENERATION_FLAG_TOP_LEVEL_BUFFER_VARIABLE) :
											(RESOURCE_NAME_GENERATION_FLAG_DEFAULT));

	return resources;
}

std::vector<std::string> getProgramInterfaceResourceList (const ProgramInterfaceDefinition::Program* program, ProgramInterface interface)
{
	// The same {uniform (block), buffer (variable)} can exist in multiple shaders, remove duplicates but keep order
	const bool					removeDuplicated	= (interface == PROGRAMINTERFACE_UNIFORM)			||
													  (interface == PROGRAMINTERFACE_UNIFORM_BLOCK)		||
													  (interface == PROGRAMINTERFACE_BUFFER_VARIABLE)	||
													  (interface == PROGRAMINTERFACE_SHADER_STORAGE_BLOCK);
	std::vector<std::string>	resources;

	switch (interface)
	{
		case PROGRAMINTERFACE_UNIFORM:
		case PROGRAMINTERFACE_BUFFER_VARIABLE:
		{
			const glu::Storage storage = (interface == PROGRAMINTERFACE_UNIFORM) ? (glu::STORAGE_UNIFORM) : (glu::STORAGE_BUFFER);

			for (int shaderNdx = 0; shaderNdx < (int)program->getShaders().size(); ++shaderNdx)
			{
				const ProgramInterfaceDefinition::Shader* shader = program->getShaders()[shaderNdx];

				for (int variableNdx = 0; variableNdx < (int)shader->getDefaultBlock().variables.size(); ++variableNdx)
					if (shader->getDefaultBlock().variables[variableNdx].storage == storage)
						generateVariableTypeResourceNames(resources,
														  shader->getDefaultBlock().variables[variableNdx].name,
														  shader->getDefaultBlock().variables[variableNdx].varType,
														  RESOURCE_NAME_GENERATION_FLAG_DEFAULT);

				for (int interfaceNdx = 0; interfaceNdx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++interfaceNdx)
				{
					const glu::InterfaceBlock& interfaceBlock = shader->getDefaultBlock().interfaceBlocks[interfaceNdx];
					if (interfaceBlock.storage == storage)
					{
						const std::vector<std::string> blockResources = getProgramInterfaceBlockMemberResourceList(interfaceBlock);
						resources.insert(resources.end(), blockResources.begin(), blockResources.end());
					}
				}
			}
			break;
		}

		case PROGRAMINTERFACE_UNIFORM_BLOCK:
		case PROGRAMINTERFACE_SHADER_STORAGE_BLOCK:
		{
			const glu::Storage storage = (interface == PROGRAMINTERFACE_UNIFORM_BLOCK) ? (glu::STORAGE_UNIFORM) : (glu::STORAGE_BUFFER);

			for (int shaderNdx = 0; shaderNdx < (int)program->getShaders().size(); ++shaderNdx)
			{
				const ProgramInterfaceDefinition::Shader* shader = program->getShaders()[shaderNdx];
				for (int interfaceNdx = 0; interfaceNdx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++interfaceNdx)
				{
					const glu::InterfaceBlock& interfaceBlock = shader->getDefaultBlock().interfaceBlocks[interfaceNdx];
					if (interfaceBlock.storage == storage)
					{
						std::vector<int> index(interfaceBlock.dimensions.size(), 0);

						for (;;)
						{
							// add resource string for each element
							{
								std::ostringstream name;
								name << interfaceBlock.interfaceName;

								for (int dimensionNdx = 0; dimensionNdx < (int)interfaceBlock.dimensions.size(); ++dimensionNdx)
									name << "[" << index[dimensionNdx] << "]";

								resources.push_back(name.str());
							}

							// increment index
							if (!incrementMultiDimensionIndex(index, interfaceBlock.dimensions))
								break;
						}
					}
				}
			}
			break;
		}

		case PROGRAMINTERFACE_PROGRAM_INPUT:
		case PROGRAMINTERFACE_PROGRAM_OUTPUT:
		{
			const glu::Storage		queryStorage		= (interface == PROGRAMINTERFACE_PROGRAM_INPUT) ? (glu::STORAGE_IN) : (glu::STORAGE_OUT);
			const glu::Storage		queryPatchStorage	= (interface == PROGRAMINTERFACE_PROGRAM_INPUT) ? (glu::STORAGE_PATCH_IN) : (glu::STORAGE_PATCH_OUT);
			const glu::ShaderType	shaderType			= (interface == PROGRAMINTERFACE_PROGRAM_INPUT) ? (program->getFirstStage()) : (program->getLastStage());

			for (int shaderNdx = 0; shaderNdx < (int)program->getShaders().size(); ++shaderNdx)
			{
				const ProgramInterfaceDefinition::Shader* shader = program->getShaders()[shaderNdx];

				if (shader->getType() != shaderType)
					continue;

				for (int variableNdx = 0; variableNdx < (int)shader->getDefaultBlock().variables.size(); ++variableNdx)
				{
					const glu::Storage variableStorage = shader->getDefaultBlock().variables[variableNdx].storage;
					if (variableStorage == queryStorage || variableStorage == queryPatchStorage)
						generateVariableTypeResourceNames(resources,
														  shader->getDefaultBlock().variables[variableNdx].name,
														  shader->getDefaultBlock().variables[variableNdx].varType,
														  RESOURCE_NAME_GENERATION_FLAG_DEFAULT);
				}

				for (int interfaceNdx = 0; interfaceNdx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++interfaceNdx)
				{
					const glu::InterfaceBlock& interfaceBlock = shader->getDefaultBlock().interfaceBlocks[interfaceNdx];
					if (interfaceBlock.storage == queryStorage || interfaceBlock.storage == queryPatchStorage)
					{
						const std::vector<std::string> blockResources = getProgramInterfaceBlockMemberResourceList(interfaceBlock);
						resources.insert(resources.end(), blockResources.begin(), blockResources.end());
					}
				}
			}

			// built-ins
			if (interface == PROGRAMINTERFACE_PROGRAM_INPUT)
			{
				if (shaderType == glu::SHADERTYPE_VERTEX && resources.empty())
					resources.push_back("gl_VertexID"); // only read from when there are no other inputs
				else if (shaderType == glu::SHADERTYPE_FRAGMENT && resources.empty())
					resources.push_back("gl_FragCoord"); // only read from when there are no other inputs
				else if (shaderType == glu::SHADERTYPE_GEOMETRY)
					resources.push_back("gl_PerVertex.gl_Position");
				else if (shaderType == glu::SHADERTYPE_TESSELLATION_CONTROL)
				{
					resources.push_back("gl_InvocationID");
					resources.push_back("gl_PerVertex.gl_Position");
				}
				else if (shaderType == glu::SHADERTYPE_TESSELLATION_EVALUATION)
					resources.push_back("gl_PerVertex.gl_Position");
				else if (shaderType == glu::SHADERTYPE_COMPUTE && resources.empty())
					resources.push_back("gl_NumWorkGroups"); // only read from when there are no other inputs
			}
			else if (interface == PROGRAMINTERFACE_PROGRAM_OUTPUT)
			{
				if (shaderType == glu::SHADERTYPE_VERTEX)
					resources.push_back("gl_Position");
				else if (shaderType == glu::SHADERTYPE_FRAGMENT && resources.empty())
					resources.push_back("gl_FragDepth"); // only written to when there are no other outputs
				else if (shaderType == glu::SHADERTYPE_GEOMETRY)
					resources.push_back("gl_Position");
				else if (shaderType == glu::SHADERTYPE_TESSELLATION_CONTROL)
				{
					resources.push_back("gl_PerVertex.gl_Position");
					resources.push_back("gl_TessLevelOuter[0]");
					resources.push_back("gl_TessLevelInner[0]");
				}
				else if (shaderType == glu::SHADERTYPE_TESSELLATION_EVALUATION)
					resources.push_back("gl_Position");
			}

			break;
		}

		case PROGRAMINTERFACE_TRANSFORM_FEEDBACK_VARYING:
		{
			const glu::ShaderType xfbStage = getProgramTransformFeedbackStage(program);

			for (int varyingNdx = 0; varyingNdx < (int)program->getTransformFeedbackVaryings().size(); ++varyingNdx)
			{
				const std::string& varyingName = program->getTransformFeedbackVaryings()[varyingNdx];

				if (deStringBeginsWith(varyingName.c_str(), "gl_"))
					resources.push_back(varyingName); // builtin
				else
				{
					std::vector<VariablePathComponent> path;

					if (!traverseProgramVariablePath(path, program, varyingName, VariableSearchFilter::createShaderTypeStorageFilter(xfbStage, glu::STORAGE_OUT)))
						DE_ASSERT(false); // Program failed validate, invalid operation

					generateVariableTypeResourceNames(resources,
													  varyingName,
													  *path.back().getVariableType(),
													  RESOURCE_NAME_GENERATION_FLAG_TRANSFORM_FEEDBACK_VARIABLE);
				}
			}

			break;
		}

		default:
			DE_ASSERT(false);
	}

	if (removeDuplicated)
	{
		std::set<std::string>		addedVariables;
		std::vector<std::string>	uniqueResouces;

		for (int ndx = 0; ndx < (int)resources.size(); ++ndx)
		{
			if (addedVariables.find(resources[ndx]) == addedVariables.end())
			{
				addedVariables.insert(resources[ndx]);
				uniqueResouces.push_back(resources[ndx]);
			}
		}

		uniqueResouces.swap(resources);
	}

	return resources;
}

/**
 * Name of the dummy uniform added by generateProgramInterfaceProgramSources
 *
 * A uniform named "dummyZero" is added by
 * generateProgramInterfaceProgramSources.  It is used in expressions to
 * prevent various program resources from being eliminated by the GLSL
 * compiler's optimizer.
 *
 * \sa deqp::gles31::Functional::ProgramInterfaceDefinition::generateProgramInterfaceProgramSources
 */
const char* getDummyZeroUniformName()
{
	return "dummyZero";
}

glu::ProgramSources generateProgramInterfaceProgramSources (const ProgramInterfaceDefinition::Program* program)
{
	glu::ProgramSources sources;

	DE_ASSERT(program->isValid());

	for (int shaderNdx = 0; shaderNdx < (int)program->getShaders().size(); ++shaderNdx)
	{
		const ProgramInterfaceDefinition::Shader*	shader						= program->getShaders()[shaderNdx];
		bool										containsUserDefinedOutputs	= false;
		bool										containsUserDefinedInputs	= false;
		std::ostringstream							sourceBuf;
		std::ostringstream							usageBuf;

		sourceBuf	<< glu::getGLSLVersionDeclaration(shader->getVersion()) << "\n"
					<< getShaderExtensionDeclarations(shader)
					<< getShaderTypeDeclarations(program, shader->getType())
					<< "\n";

		// Struct definitions

		writeStructureDefinitions(sourceBuf, shader->getDefaultBlock());

		// variables in the default scope

		for (int ndx = 0; ndx < (int)shader->getDefaultBlock().variables.size(); ++ndx)
			sourceBuf << shader->getDefaultBlock().variables[ndx] << ";\n";

		if (!shader->getDefaultBlock().variables.empty())
			sourceBuf << "\n";

		// Interface blocks

		for (int ndx = 0; ndx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++ndx)
			writeInterfaceBlock(sourceBuf, shader->getDefaultBlock().interfaceBlocks[ndx]);

		// Use inputs and outputs so that they won't be removed by the optimizer

		usageBuf <<	"highp uniform vec4 " << getDummyZeroUniformName() << "; // Default value is vec4(0.0).\n"
					"highp vec4 readInputs()\n"
					"{\n"
					"	highp vec4 retValue = " << getDummyZeroUniformName() << ";\n";

		// User-defined inputs

		for (int ndx = 0; ndx < (int)shader->getDefaultBlock().variables.size(); ++ndx)
		{
			if (shader->getDefaultBlock().variables[ndx].storage == glu::STORAGE_IN			||
				shader->getDefaultBlock().variables[ndx].storage == glu::STORAGE_PATCH_IN	||
				shader->getDefaultBlock().variables[ndx].storage == glu::STORAGE_UNIFORM)
			{
				writeVariableReadAccumulateExpression(usageBuf,
													  "retValue",
													  shader->getDefaultBlock().variables[ndx].name,
													  shader->getType(),
													  shader->getDefaultBlock().variables[ndx].storage,
													  program,
													  shader->getDefaultBlock().variables[ndx].varType);
				containsUserDefinedInputs = true;
			}
		}

		for (int interfaceNdx = 0; interfaceNdx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++interfaceNdx)
		{
			const glu::InterfaceBlock& interface = shader->getDefaultBlock().interfaceBlocks[interfaceNdx];
			if (isReadableInterface(interface))
			{
				writeInterfaceReadAccumulateExpression(usageBuf,
													   "retValue",
													   interface,
													   shader->getType(),
													   program);
				containsUserDefinedInputs = true;
			}
		}

		// Built-in-inputs

		switch (shader->getType())
		{
			case glu::SHADERTYPE_VERTEX:
				// make readInputs to never be compile time constant
				if (!containsUserDefinedInputs)
					usageBuf << "	retValue += vec4(float(gl_VertexID));\n";
				break;

			case glu::SHADERTYPE_FRAGMENT:
				// make readInputs to never be compile time constant
				if (!containsUserDefinedInputs)
					usageBuf << "	retValue += gl_FragCoord;\n";
				break;
			case glu::SHADERTYPE_GEOMETRY:
				// always use previous stage's output values so that previous stage won't be optimized out
				usageBuf << "	retValue += gl_in[0].gl_Position;\n";
				break;
			case glu::SHADERTYPE_TESSELLATION_CONTROL:
				// always use previous stage's output values so that previous stage won't be optimized out
				usageBuf << "	retValue += gl_in[0].gl_Position;\n";
				break;
			case glu::SHADERTYPE_TESSELLATION_EVALUATION:
				// always use previous stage's output values so that previous stage won't be optimized out
				usageBuf << "	retValue += gl_in[0].gl_Position;\n";
				break;

			case glu::SHADERTYPE_COMPUTE:
				// make readInputs to never be compile time constant
				if (!containsUserDefinedInputs)
					usageBuf << "	retValue += vec4(float(gl_NumWorkGroups.x));\n";
				break;
			default:
				DE_ASSERT(false);
		}

		usageBuf <<	"	return retValue;\n"
					"}\n\n";

		usageBuf <<	"void writeOutputs(in highp vec4 dummyValue)\n"
					"{\n";

		// User-defined outputs

		for (int ndx = 0; ndx < (int)shader->getDefaultBlock().variables.size(); ++ndx)
		{
			if (shader->getDefaultBlock().variables[ndx].storage == glu::STORAGE_OUT ||
				shader->getDefaultBlock().variables[ndx].storage == glu::STORAGE_PATCH_OUT)
			{
				writeVariableWriteExpression(usageBuf,
											 "dummyValue",
											 shader->getDefaultBlock().variables[ndx].name,
											 shader->getType(),
											 shader->getDefaultBlock().variables[ndx].storage,
											 program,
											 shader->getDefaultBlock().variables[ndx].varType);
				containsUserDefinedOutputs = true;
			}
		}

		for (int interfaceNdx = 0; interfaceNdx < (int)shader->getDefaultBlock().interfaceBlocks.size(); ++interfaceNdx)
		{
			const glu::InterfaceBlock& interface = shader->getDefaultBlock().interfaceBlocks[interfaceNdx];
			if (isWritableInterface(interface))
			{
				writeInterfaceWriteExpression(usageBuf, "dummyValue", interface, shader->getType(), program);
				containsUserDefinedOutputs = true;
			}
		}

		// Builtin-outputs that must be written to

		if (shader->getType() == glu::SHADERTYPE_VERTEX)
			usageBuf << "	gl_Position = dummyValue;\n";
		else if (shader->getType() == glu::SHADERTYPE_GEOMETRY)
			usageBuf << "	gl_Position = dummyValue;\n"
						 "	EmitVertex();\n";
		else if (shader->getType() == glu::SHADERTYPE_TESSELLATION_CONTROL)
			usageBuf << "	gl_out[gl_InvocationID].gl_Position = dummyValue;\n"
						"	gl_TessLevelOuter[0] = 2.8;\n"
						"	gl_TessLevelOuter[1] = 2.8;\n"
						"	gl_TessLevelOuter[2] = 2.8;\n"
						"	gl_TessLevelOuter[3] = 2.8;\n"
						"	gl_TessLevelInner[0] = 2.8;\n"
						"	gl_TessLevelInner[1] = 2.8;\n";
		else if (shader->getType() == glu::SHADERTYPE_TESSELLATION_EVALUATION)
			usageBuf << "	gl_Position = dummyValue;\n";

		// Output to sink input data to

		if (!containsUserDefinedOutputs)
		{
			if (shader->getType() == glu::SHADERTYPE_FRAGMENT)
				usageBuf << "	gl_FragDepth = dot(dummyValue.xy, dummyValue.xw);\n";
			else if (shader->getType() == glu::SHADERTYPE_COMPUTE)
				usageBuf << "	dummyOutputBlock.dummyValue = dummyValue;\n";
		}

		usageBuf <<	"}\n\n"
					"void main()\n"
					"{\n"
					"	writeOutputs(readInputs());\n"
					"}\n";

		// Interface for dummy output

		if (shader->getType() == glu::SHADERTYPE_COMPUTE && !containsUserDefinedOutputs)
		{
			sourceBuf	<< "writeonly buffer DummyOutputInterface\n"
						<< "{\n"
						<< "	highp vec4 dummyValue;\n"
						<< "} dummyOutputBlock;\n\n";
		}

		sources << glu::ShaderSource(shader->getType(), sourceBuf.str() + usageBuf.str());
	}

	if (program->isSeparable())
		sources << glu::ProgramSeparable(true);

	for (int ndx = 0; ndx < (int)program->getTransformFeedbackVaryings().size(); ++ndx)
		sources << glu::TransformFeedbackVarying(program->getTransformFeedbackVaryings()[ndx]);

	if (program->getTransformFeedbackMode())
		sources << glu::TransformFeedbackMode(program->getTransformFeedbackMode());

	return sources;
}

bool findProgramVariablePathByPathName (std::vector<VariablePathComponent>& typePath, const ProgramInterfaceDefinition::Program* program, const std::string& pathName, const VariableSearchFilter& filter)
{
	std::vector<VariablePathComponent> modifiedPath;

	if (!traverseProgramVariablePath(modifiedPath, program, pathName, filter))
		return false;

	// modify param only on success
	typePath.swap(modifiedPath);
	return true;
}

ProgramInterfaceDefinition::ShaderResourceUsage getShaderResourceUsage (const ProgramInterfaceDefinition::Program* program, const ProgramInterfaceDefinition::Shader* shader)
{
	ProgramInterfaceDefinition::ShaderResourceUsage retVal;

	retVal.numInputs						= getNumTypeInstances(shader, glu::STORAGE_IN);
	retVal.numInputVectors					= getNumVectors(shader, glu::STORAGE_IN);
	retVal.numInputComponents				= getNumComponents(shader, glu::STORAGE_IN);

	retVal.numOutputs						= getNumTypeInstances(shader, glu::STORAGE_OUT);
	retVal.numOutputVectors					= getNumVectors(shader, glu::STORAGE_OUT);
	retVal.numOutputComponents				= getNumComponents(shader, glu::STORAGE_OUT);

	retVal.numPatchInputComponents			= getNumComponents(shader, glu::STORAGE_PATCH_IN);
	retVal.numPatchOutputComponents			= getNumComponents(shader, glu::STORAGE_PATCH_OUT);

	retVal.numDefaultBlockUniformComponents	= getNumDefaultBlockComponents(shader, glu::STORAGE_UNIFORM);
	retVal.numCombinedUniformComponents		= getNumComponents(shader, glu::STORAGE_UNIFORM);
	retVal.numUniformVectors				= getNumVectors(shader, glu::STORAGE_UNIFORM);

	retVal.numSamplers						= getNumTypeInstances(shader, glu::STORAGE_UNIFORM, glu::isDataTypeSampler);
	retVal.numImages						= getNumTypeInstances(shader, glu::STORAGE_UNIFORM, glu::isDataTypeImage);

	retVal.numAtomicCounterBuffers			= getNumAtomicCounterBuffers(shader);
	retVal.numAtomicCounters				= getNumTypeInstances(shader, glu::STORAGE_UNIFORM, glu::isDataTypeAtomicCounter);

	retVal.numUniformBlocks					= getNumShaderBlocks(shader, glu::STORAGE_UNIFORM);
	retVal.numShaderStorageBlocks			= getNumShaderBlocks(shader, glu::STORAGE_BUFFER);

	// add builtins
	switch (shader->getType())
	{
		case glu::SHADERTYPE_VERTEX:
			// gl_Position is not counted
			break;

		case glu::SHADERTYPE_FRAGMENT:
			// nada
			break;

		case glu::SHADERTYPE_GEOMETRY:
			// gl_Position in (point mode => size 1)
			retVal.numInputs			+= 1;
			retVal.numInputVectors		+= 1;
			retVal.numInputComponents	+= 4;

			// gl_Position out
			retVal.numOutputs			+= 1;
			retVal.numOutputVectors		+= 1;
			retVal.numOutputComponents	+= 4;
			break;

		case glu::SHADERTYPE_TESSELLATION_CONTROL:
			// gl_Position in is read up to gl_InstanceID
			retVal.numInputs			+= 1 * program->getTessellationNumOutputPatchVertices();
			retVal.numInputVectors		+= 1 * program->getTessellationNumOutputPatchVertices();
			retVal.numInputComponents	+= 4 * program->getTessellationNumOutputPatchVertices();

			// gl_Position out, size = num patch out vertices
			retVal.numOutputs			+= 1 * program->getTessellationNumOutputPatchVertices();
			retVal.numOutputVectors		+= 1 * program->getTessellationNumOutputPatchVertices();
			retVal.numOutputComponents	+= 4 * program->getTessellationNumOutputPatchVertices();
			break;

		case glu::SHADERTYPE_TESSELLATION_EVALUATION:
			// gl_Position in is read up to gl_InstanceID
			retVal.numInputs			+= 1 * program->getTessellationNumOutputPatchVertices();
			retVal.numInputVectors		+= 1 * program->getTessellationNumOutputPatchVertices();
			retVal.numInputComponents	+= 4 * program->getTessellationNumOutputPatchVertices();

			// gl_Position out
			retVal.numOutputs			+= 1;
			retVal.numOutputVectors		+= 1;
			retVal.numOutputComponents	+= 4;
			break;

		case glu::SHADERTYPE_COMPUTE:
			// nada
			break;

		default:
			DE_ASSERT(false);
			break;
	}
	return retVal;
}

ProgramInterfaceDefinition::ProgramResourceUsage getCombinedProgramResourceUsage (const ProgramInterfaceDefinition::Program* program)
{
	ProgramInterfaceDefinition::ProgramResourceUsage	retVal;
	int													numVertexOutputComponents	= 0;
	int													numFragmentInputComponents	= 0;
	int													numVertexOutputVectors		= 0;
	int													numFragmentInputVectors		= 0;

	retVal.uniformBufferMaxBinding					= -1; // max binding is inclusive upper bound. Allow 0 bindings by using negative value
	retVal.uniformBufferMaxSize						= 0;
	retVal.numUniformBlocks							= 0;
	retVal.numCombinedVertexUniformComponents		= 0;
	retVal.numCombinedFragmentUniformComponents		= 0;
	retVal.numCombinedGeometryUniformComponents		= 0;
	retVal.numCombinedTessControlUniformComponents	= 0;
	retVal.numCombinedTessEvalUniformComponents		= 0;
	retVal.shaderStorageBufferMaxBinding			= -1; // see above
	retVal.shaderStorageBufferMaxSize				= 0;
	retVal.numShaderStorageBlocks					= 0;
	retVal.numVaryingComponents						= 0;
	retVal.numVaryingVectors						= 0;
	retVal.numCombinedSamplers						= 0;
	retVal.atomicCounterBufferMaxBinding			= -1; // see above
	retVal.atomicCounterBufferMaxSize				= 0;
	retVal.numAtomicCounterBuffers					= 0;
	retVal.numAtomicCounters						= 0;
	retVal.maxImageBinding							= -1; // see above
	retVal.numCombinedImages						= 0;
	retVal.numCombinedOutputResources				= 0;
	retVal.numXFBInterleavedComponents				= 0;
	retVal.numXFBSeparateAttribs					= 0;
	retVal.numXFBSeparateComponents					= 0;
	retVal.fragmentOutputMaxBinding					= -1; // see above

	for (int shaderNdx = 0; shaderNdx < (int)program->getShaders().size(); ++shaderNdx)
	{
		const ProgramInterfaceDefinition::Shader* const shader = program->getShaders()[shaderNdx];

		retVal.uniformBufferMaxBinding		= de::max(retVal.uniformBufferMaxBinding, getMaxBufferBinding(shader, glu::STORAGE_UNIFORM));
		retVal.uniformBufferMaxSize			= de::max(retVal.uniformBufferMaxSize, getBufferMaxSize(shader, glu::STORAGE_UNIFORM));
		retVal.numUniformBlocks				+= getNumShaderBlocks(shader, glu::STORAGE_UNIFORM);

		switch (shader->getType())
		{
			case glu::SHADERTYPE_VERTEX:					retVal.numCombinedVertexUniformComponents		+= getNumComponents(shader, glu::STORAGE_UNIFORM); break;
			case glu::SHADERTYPE_FRAGMENT:					retVal.numCombinedFragmentUniformComponents		+= getNumComponents(shader, glu::STORAGE_UNIFORM); break;
			case glu::SHADERTYPE_GEOMETRY:					retVal.numCombinedGeometryUniformComponents		+= getNumComponents(shader, glu::STORAGE_UNIFORM); break;
			case glu::SHADERTYPE_TESSELLATION_CONTROL:		retVal.numCombinedTessControlUniformComponents	+= getNumComponents(shader, glu::STORAGE_UNIFORM); break;
			case glu::SHADERTYPE_TESSELLATION_EVALUATION:	retVal.numCombinedTessEvalUniformComponents		+= getNumComponents(shader, glu::STORAGE_UNIFORM); break;
			default: break;
		}

		retVal.shaderStorageBufferMaxBinding	= de::max(retVal.shaderStorageBufferMaxBinding, getMaxBufferBinding(shader, glu::STORAGE_BUFFER));
		retVal.shaderStorageBufferMaxSize		= de::max(retVal.shaderStorageBufferMaxSize, getBufferMaxSize(shader, glu::STORAGE_BUFFER));
		retVal.numShaderStorageBlocks			+= getNumShaderBlocks(shader, glu::STORAGE_BUFFER);

		if (shader->getType() == glu::SHADERTYPE_VERTEX)
		{
			numVertexOutputComponents	+= getNumComponents(shader, glu::STORAGE_OUT);
			numVertexOutputVectors		+= getNumVectors(shader, glu::STORAGE_OUT);
		}
		else if (shader->getType() == glu::SHADERTYPE_FRAGMENT)
		{
			numFragmentInputComponents	+= getNumComponents(shader, glu::STORAGE_IN);
			numFragmentInputVectors		+= getNumVectors(shader, glu::STORAGE_IN);
		}

		retVal.numCombinedSamplers	+= getNumTypeInstances(shader, glu::STORAGE_UNIFORM, glu::isDataTypeSampler);

		retVal.atomicCounterBufferMaxBinding	= de::max(retVal.atomicCounterBufferMaxBinding, getAtomicCounterMaxBinding(shader));
		retVal.atomicCounterBufferMaxSize		= de::max(retVal.atomicCounterBufferMaxSize, getAtomicCounterMaxBufferSize(shader));
		retVal.numAtomicCounterBuffers			+= getNumAtomicCounterBuffers(shader);
		retVal.numAtomicCounters				+= getNumTypeInstances(shader, glu::STORAGE_UNIFORM, glu::isDataTypeAtomicCounter);
		retVal.maxImageBinding					= de::max(retVal.maxImageBinding, getUniformMaxBinding(shader, glu::isDataTypeImage));
		retVal.numCombinedImages				+= getNumTypeInstances(shader, glu::STORAGE_UNIFORM, glu::isDataTypeImage);

		retVal.numCombinedOutputResources		+= getNumTypeInstances(shader, glu::STORAGE_UNIFORM, glu::isDataTypeImage);
		retVal.numCombinedOutputResources		+= getNumShaderBlocks(shader, glu::STORAGE_BUFFER);

		if (shader->getType() == glu::SHADERTYPE_FRAGMENT)
		{
			retVal.numCombinedOutputResources += getNumVectors(shader, glu::STORAGE_OUT);
			retVal.fragmentOutputMaxBinding = de::max(retVal.fragmentOutputMaxBinding, getFragmentOutputMaxLocation(shader));
		}
	}

	if (program->getTransformFeedbackMode() == GL_INTERLEAVED_ATTRIBS)
		retVal.numXFBInterleavedComponents = getNumXFBComponents(program);
	else if (program->getTransformFeedbackMode() == GL_SEPARATE_ATTRIBS)
	{
		retVal.numXFBSeparateAttribs	= (int)program->getTransformFeedbackVaryings().size();
		retVal.numXFBSeparateComponents	= getNumMaxXFBOutputComponents(program);
	}

	// legacy limits
	retVal.numVaryingComponents	= de::max(numVertexOutputComponents, numFragmentInputComponents);
	retVal.numVaryingVectors	= de::max(numVertexOutputVectors, numFragmentInputVectors);

	return retVal;
}

} // Functional
} // gles31
} // deqp
