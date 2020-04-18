/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2014 The Android Open Source Project
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Tessellation User Defined IO Tests
 *//*--------------------------------------------------------------------*/

#include "vktTessellationUserDefinedIO.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTessellationUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuImageCompare.hpp"
#include "tcuImageIO.hpp"

#include "gluVarType.hpp"
#include "gluVarTypeUtil.hpp"

#include "vkDefs.hpp"
#include "vkQueryUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkTypeUtil.hpp"

#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"

namespace vkt
{
namespace tessellation
{

using namespace vk;

namespace
{

enum Constants
{
	NUM_PER_PATCH_BLOCKS		= 2,
	NUM_PER_PATCH_ARRAY_ELEMS	= 3,
	NUM_OUTPUT_VERTICES			= 5,
	NUM_TESS_LEVELS				= 6,
	MAX_TESSELLATION_PATCH_SIZE = 32,
	RENDER_SIZE					= 256,
};

enum IOType
{
	IO_TYPE_PER_PATCH = 0,
	IO_TYPE_PER_PATCH_ARRAY,
	IO_TYPE_PER_PATCH_BLOCK,
	IO_TYPE_PER_PATCH_BLOCK_ARRAY,
	IO_TYPE_PER_VERTEX,
	IO_TYPE_PER_VERTEX_BLOCK,

	IO_TYPE_LAST
};

enum VertexIOArraySize
{
	VERTEX_IO_ARRAY_SIZE_IMPLICIT = 0,
	VERTEX_IO_ARRAY_SIZE_EXPLICIT_SHADER_BUILTIN,		//!< Use gl_MaxPatchVertices as size for per-vertex input array.
	VERTEX_IO_ARRAY_SIZE_EXPLICIT_SPEC_MIN,				//!< Minimum maxTessellationPatchSize required by the spec.

	VERTEX_IO_ARRAY_SIZE_LAST
};

struct CaseDefinition
{
	TessPrimitiveType	primitiveType;
	IOType				ioType;
	VertexIOArraySize	vertexIOArraySize;
	std::string			referenceImagePath;
};

typedef std::string (*BasicTypeVisitFunc)(const std::string& name, glu::DataType type, int indentationDepth); //!< See glslTraverseBasicTypes below.

class TopLevelObject
{
public:
	virtual					~TopLevelObject					(void) {}

	virtual std::string		name							(void) const = 0;
	virtual std::string		declare							(void) const = 0;
	virtual std::string		declareArray					(const std::string& arraySizeExpr) const = 0;
	virtual std::string		glslTraverseBasicTypeArray		(const int numArrayElements, //!< If negative, traverse just array[gl_InvocationID], not all indices.
															 const int indentationDepth,
															 BasicTypeVisitFunc) const = 0;
	virtual std::string		glslTraverseBasicType			(const int indentationDepth,
															 BasicTypeVisitFunc) const = 0;
	virtual int				numBasicSubobjectsInElementType	(void) const = 0;
	virtual std::string		basicSubobjectAtIndex			(const int index, const int arraySize) const = 0;
};

std::string glslTraverseBasicTypes (const std::string&			rootName,
									const glu::VarType&			rootType,
									const int					arrayNestingDepth,
									const int					indentationDepth,
									const BasicTypeVisitFunc	visit)
{
	if (rootType.isBasicType())
		return visit(rootName, rootType.getBasicType(), indentationDepth);
	else if (rootType.isArrayType())
	{
		const std::string indentation	= std::string(indentationDepth, '\t');
		const std::string loopIndexName	= "i" + de::toString(arrayNestingDepth);
		const std::string arrayLength	= de::toString(rootType.getArraySize());
		return indentation + "for (int " + loopIndexName + " = 0; " + loopIndexName + " < " + de::toString(rootType.getArraySize()) + "; ++" + loopIndexName + ")\n" +
			   indentation + "{\n" +
			   glslTraverseBasicTypes(rootName + "[" + loopIndexName + "]", rootType.getElementType(), arrayNestingDepth+1, indentationDepth+1, visit) +
			   indentation + "}\n";
	}
	else if (rootType.isStructType())
	{
		const glu::StructType&	structType = *rootType.getStructPtr();
		const int				numMembers = structType.getNumMembers();
		std::string				result;

		for (int membNdx = 0; membNdx < numMembers; ++membNdx)
		{
			const glu::StructMember& member = structType.getMember(membNdx);
			result += glslTraverseBasicTypes(rootName + "." + member.getName(), member.getType(), arrayNestingDepth, indentationDepth, visit);
		}

		return result;
	}
	else
	{
		DE_ASSERT(false);
		return DE_NULL;
	}
}

//! Used as the 'visit' argument for glslTraverseBasicTypes.
std::string glslAssignBasicTypeObject (const std::string& name, const glu::DataType type, const int indentationDepth)
{
	const int			scalarSize	= glu::getDataTypeScalarSize(type);
	const std::string	indentation	= std::string(indentationDepth, '\t');
	std::ostringstream	result;

	result << indentation << name << " = ";

	if (type != glu::TYPE_FLOAT)
		result << std::string() << glu::getDataTypeName(type) << "(";
	for (int i = 0; i < scalarSize; ++i)
		result << (i > 0 ? ", v+" + de::floatToString(0.8f*(float)i, 1) : "v");
	if (type != glu::TYPE_FLOAT)
		result << ")";
	result << ";\n"
		   << indentation << "v += 0.4;\n";
	return result.str();
}

//! Used as the 'visit' argument for glslTraverseBasicTypes.
std::string glslCheckBasicTypeObject (const std::string& name, const glu::DataType type, const int indentationDepth)
{
	const int			scalarSize	= glu::getDataTypeScalarSize(type);
	const std::string	indentation	= std::string(indentationDepth, '\t');
	std::ostringstream	result;

	result << indentation << "allOk = allOk && compare_" << glu::getDataTypeName(type) << "(" << name << ", ";

	if (type != glu::TYPE_FLOAT)
		result << std::string() << glu::getDataTypeName(type) << "(";
	for (int i = 0; i < scalarSize; ++i)
		result << (i > 0 ? ", v+" + de::floatToString(0.8f*(float)i, 1) : "v");
	if (type != glu::TYPE_FLOAT)
		result << ")";
	result << ");\n"
		   << indentation << "v += 0.4;\n"
		   << indentation << "if (allOk) ++firstFailedInputIndex;\n";

	return result.str();
}

int numBasicSubobjectsInElementType (const std::vector<de::SharedPtr<TopLevelObject> >& objects)
{
	int result = 0;
	for (int i = 0; i < static_cast<int>(objects.size()); ++i)
		result += objects[i]->numBasicSubobjectsInElementType();
	return result;
}

std::string basicSubobjectAtIndex (const int subobjectIndex, const std::vector<de::SharedPtr<TopLevelObject> >& objects, const int topLevelArraySize)
{
	int currentIndex = 0;
	int objectIndex  = 0;

	for (; currentIndex < subobjectIndex; ++objectIndex)
		currentIndex += objects[objectIndex]->numBasicSubobjectsInElementType() * topLevelArraySize;

	if (currentIndex > subobjectIndex)
	{
		--objectIndex;
		currentIndex -= objects[objectIndex]->numBasicSubobjectsInElementType() * topLevelArraySize;
	}

	return objects[objectIndex]->basicSubobjectAtIndex(subobjectIndex - currentIndex, topLevelArraySize);
}

int numBasicSubobjects (const glu::VarType& type)
{
	if (type.isBasicType())
		return 1;
	else if (type.isArrayType())
		return type.getArraySize()*numBasicSubobjects(type.getElementType());
	else if (type.isStructType())
	{
		const glu::StructType&	structType	= *type.getStructPtr();
		int						result		= 0;
		for (int i = 0; i < structType.getNumMembers(); ++i)
			result += numBasicSubobjects(structType.getMember(i).getType());
		return result;
	}
	else
	{
		DE_ASSERT(false);
		return -1;
	}
}

class Variable : public TopLevelObject
{
public:
	Variable (const std::string& name_, const glu::VarType& type, const bool isArray)
		: m_name		(name_)
		, m_type		(type)
		, m_isArray		(isArray)
	{
		DE_ASSERT(!type.isArrayType());
	}

	std::string		name								(void) const { return m_name; }
	std::string		declare								(void) const;
	std::string		declareArray						(const std::string& arraySizeExpr) const;
	std::string		glslTraverseBasicTypeArray			(const int numArrayElements, const int indentationDepth, BasicTypeVisitFunc) const;
	std::string		glslTraverseBasicType				(const int indentationDepth, BasicTypeVisitFunc) const;
	int				numBasicSubobjectsInElementType		(void) const;
	std::string		basicSubobjectAtIndex				(const int index, const int arraySize) const;

private:
	std::string		m_name;
	glu::VarType	m_type; //!< If this Variable is an array element, m_type is the element type; otherwise just the variable type.
	const bool		m_isArray;
};

std::string Variable::declare (void) const
{
	DE_ASSERT(!m_isArray);
	return de::toString(glu::declare(m_type, m_name)) + ";\n";
}

std::string Variable::declareArray (const std::string& sizeExpr) const
{
	DE_ASSERT(m_isArray);
	return de::toString(glu::declare(m_type, m_name)) + "[" + sizeExpr + "];\n";
}

std::string Variable::glslTraverseBasicTypeArray (const int numArrayElements, const int indentationDepth, BasicTypeVisitFunc visit) const
{
	DE_ASSERT(m_isArray);

	const bool			traverseAsArray	= numArrayElements >= 0;
	const std::string	traversedName	= m_name + (!traverseAsArray ? "[gl_InvocationID]" : "");
	const glu::VarType	type			= traverseAsArray ? glu::VarType(m_type, numArrayElements) : m_type;

	return glslTraverseBasicTypes(traversedName, type, 0, indentationDepth, visit);
}

std::string Variable::glslTraverseBasicType (const int indentationDepth, BasicTypeVisitFunc visit) const
{
	DE_ASSERT(!m_isArray);
	return glslTraverseBasicTypes(m_name, m_type, 0, indentationDepth, visit);
}

int Variable::numBasicSubobjectsInElementType (void) const
{
	return numBasicSubobjects(m_type);
}

std::string Variable::basicSubobjectAtIndex (const int subobjectIndex, const int arraySize) const
{
	const glu::VarType	type		 = m_isArray ? glu::VarType(m_type, arraySize) : m_type;
	int					currentIndex = 0;

	for (glu::BasicTypeIterator basicIt = glu::BasicTypeIterator::begin(&type); basicIt != glu::BasicTypeIterator::end(&type); ++basicIt)
	{
		if (currentIndex == subobjectIndex)
			return m_name + de::toString(glu::TypeAccessFormat(type, basicIt.getPath()));
		++currentIndex;
	}
	DE_ASSERT(false);
	return DE_NULL;
}

class IOBlock : public TopLevelObject
{
public:
	struct Member
	{
		std::string		name;
		glu::VarType	type;

		Member (const std::string& n, const glu::VarType& t) : name(n), type(t) {}
	};

	IOBlock (const std::string& blockName, const std::string& interfaceName, const std::vector<Member>& members)
		: m_blockName		(blockName)
		, m_interfaceName	(interfaceName)
		, m_members			(members)
	{
	}

	std::string			name								(void) const { return m_interfaceName; }
	std::string			declare								(void) const;
	std::string			declareArray						(const std::string& arraySizeExpr) const;
	std::string			glslTraverseBasicTypeArray			(const int numArrayElements, const int indentationDepth, BasicTypeVisitFunc) const;
	std::string			glslTraverseBasicType				(const int indentationDepth, BasicTypeVisitFunc) const;
	int					numBasicSubobjectsInElementType		(void) const;
	std::string			basicSubobjectAtIndex				(const int index, const int arraySize) const;

private:
	std::string			m_blockName;
	std::string			m_interfaceName;
	std::vector<Member>	m_members;
};

std::string IOBlock::declare (void) const
{
	std::ostringstream buf;

	buf << m_blockName << "\n"
		<< "{\n";

	for (int i = 0; i < static_cast<int>(m_members.size()); ++i)
		buf << "\t" << glu::declare(m_members[i].type, m_members[i].name) << ";\n";

	buf << "} " << m_interfaceName << ";\n";
	return buf.str();
}

std::string IOBlock::declareArray (const std::string& sizeExpr) const
{
	std::ostringstream buf;

	buf << m_blockName << "\n"
		<< "{\n";

	for (int i = 0; i < static_cast<int>(m_members.size()); ++i)
		buf << "\t" << glu::declare(m_members[i].type, m_members[i].name) << ";\n";

	buf << "} " << m_interfaceName << "[" << sizeExpr << "];\n";
	return buf.str();
}

std::string IOBlock::glslTraverseBasicTypeArray (const int numArrayElements, const int indentationDepth, BasicTypeVisitFunc visit) const
{
	if (numArrayElements >= 0)
	{
		const std::string	indentation = std::string(indentationDepth, '\t');
		std::ostringstream	result;

		result << indentation << "for (int i0 = 0; i0 < " << numArrayElements << "; ++i0)\n"
			   << indentation << "{\n";
		for (int i = 0; i < static_cast<int>(m_members.size()); ++i)
			result << glslTraverseBasicTypes(m_interfaceName + "[i0]." + m_members[i].name, m_members[i].type, 1, indentationDepth + 1, visit);
		result << indentation + "}\n";
		return result.str();
	}
	else
	{
		std::ostringstream result;
		for (int i = 0; i < static_cast<int>(m_members.size()); ++i)
			result << glslTraverseBasicTypes(m_interfaceName + "[gl_InvocationID]." + m_members[i].name, m_members[i].type, 0, indentationDepth, visit);
		return result.str();
	}
}

std::string IOBlock::glslTraverseBasicType (const int indentationDepth, BasicTypeVisitFunc visit) const
{
	std::ostringstream result;
	for (int i = 0; i < static_cast<int>(m_members.size()); ++i)
		result << glslTraverseBasicTypes(m_interfaceName + "." + m_members[i].name, m_members[i].type, 0, indentationDepth, visit);
	return result.str();
}

int IOBlock::numBasicSubobjectsInElementType (void) const
{
	int result = 0;
	for (int i = 0; i < static_cast<int>(m_members.size()); ++i)
		result += numBasicSubobjects(m_members[i].type);
	return result;
}

std::string IOBlock::basicSubobjectAtIndex (const int subobjectIndex, const int arraySize) const
{
	int currentIndex = 0;
	for (int arrayNdx = 0; arrayNdx < arraySize; ++arrayNdx)
	for (int memberNdx = 0; memberNdx < static_cast<int>(m_members.size()); ++memberNdx)
	{
		const glu::VarType& membType = m_members[memberNdx].type;
		for (glu::BasicTypeIterator basicIt = glu::BasicTypeIterator::begin(&membType); basicIt != glu::BasicTypeIterator::end(&membType); ++basicIt)
		{
			if (currentIndex == subobjectIndex)
				return m_interfaceName + "[" + de::toString(arrayNdx) + "]." + m_members[memberNdx].name + de::toString(glu::TypeAccessFormat(membType, basicIt.getPath()));
			currentIndex++;
		}
	}
	DE_ASSERT(false);
	return DE_NULL;
}

class UserDefinedIOTest : public TestCase
{
public:
							UserDefinedIOTest	(tcu::TestContext& testCtx, const std::string& name, const std::string& description, const CaseDefinition caseDef);
	void					initPrograms		(vk::SourceCollections& programCollection) const;
	TestInstance*			createInstance		(Context& context) const;

private:
	const CaseDefinition						m_caseDef;
	std::vector<glu::StructType>				m_structTypes;
	std::vector<de::SharedPtr<TopLevelObject> >	m_tcsOutputs;
	std::vector<de::SharedPtr<TopLevelObject> >	m_tesInputs;
	std::string									m_tcsDeclarations;
	std::string									m_tcsStatements;
	std::string									m_tesDeclarations;
	std::string									m_tesStatements;
};

UserDefinedIOTest::UserDefinedIOTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const CaseDefinition caseDef)
	: TestCase	(testCtx, name, description)
	, m_caseDef	(caseDef)
{
	const bool			isPerPatchIO				= m_caseDef.ioType == IO_TYPE_PER_PATCH				||
													  m_caseDef.ioType == IO_TYPE_PER_PATCH_ARRAY		||
													  m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK		||
													  m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY;

	const bool			isExplicitVertexArraySize	= m_caseDef.vertexIOArraySize == VERTEX_IO_ARRAY_SIZE_EXPLICIT_SHADER_BUILTIN ||
													  m_caseDef.vertexIOArraySize == VERTEX_IO_ARRAY_SIZE_EXPLICIT_SPEC_MIN;

	const std::string	vertexAttrArrayInputSize	= m_caseDef.vertexIOArraySize == VERTEX_IO_ARRAY_SIZE_IMPLICIT					? ""
													: m_caseDef.vertexIOArraySize == VERTEX_IO_ARRAY_SIZE_EXPLICIT_SHADER_BUILTIN	? "gl_MaxPatchVertices"
													: m_caseDef.vertexIOArraySize == VERTEX_IO_ARRAY_SIZE_EXPLICIT_SPEC_MIN			? de::toString(MAX_TESSELLATION_PATCH_SIZE)
													: DE_NULL;

	const char* const	maybePatch					= isPerPatchIO ? "patch " : "";
	const std::string	outMaybePatch				= std::string() + maybePatch + "out ";
	const std::string	inMaybePatch				= std::string() + maybePatch + "in ";
	const bool			useBlock					= m_caseDef.ioType == IO_TYPE_PER_VERTEX_BLOCK		||
													  m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK		||
													  m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY;
	const int			wrongNumElements			= -2;

	std::ostringstream tcsDeclarations;
	std::ostringstream tcsStatements;
	std::ostringstream tesDeclarations;
	std::ostringstream tesStatements;

	// Indices 0 and 1 are taken, see initPrograms()
	int tcsNextOutputLocation = 2;
	int tesNextInputLocation  = 2;

	m_structTypes.push_back(glu::StructType("S"));

	const glu::VarType	highpFloat		(glu::TYPE_FLOAT, glu::PRECISION_HIGHP);
	glu::StructType&	structType		= m_structTypes.back();
	const glu::VarType	structVarType	(&structType);
	bool				usedStruct		= false;

	structType.addMember("x", glu::VarType(glu::TYPE_INT,		 glu::PRECISION_HIGHP));
	structType.addMember("y", glu::VarType(glu::TYPE_FLOAT_VEC4, glu::PRECISION_HIGHP));

	// It is illegal to have a structure containing an array as an output variable
	if (useBlock)
		structType.addMember("z", glu::VarType(highpFloat, 2));

	if (useBlock)
	{
		std::vector<IOBlock::Member> blockMembers;

		// use leaner block to make sure it is not larger than allowed (per-patch storage is very limited)
		const bool useLightweightBlock = (m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY);

		if (!useLightweightBlock)
			blockMembers.push_back(IOBlock::Member("blockS",	structVarType));

		blockMembers.push_back(IOBlock::Member("blockFa",	glu::VarType(highpFloat, 3)));
		blockMembers.push_back(IOBlock::Member("blockSa",	glu::VarType(structVarType, 2)));
		blockMembers.push_back(IOBlock::Member("blockF",	highpFloat));

		m_tcsOutputs.push_back	(de::SharedPtr<TopLevelObject>(new IOBlock("TheBlock", "tcBlock", blockMembers)));
		m_tesInputs.push_back	(de::SharedPtr<TopLevelObject>(new IOBlock("TheBlock", "teBlock", blockMembers)));

		usedStruct = true;
	}
	else
	{
		const Variable var0("in_te_s", structVarType,	m_caseDef.ioType != IO_TYPE_PER_PATCH);
		const Variable var1("in_te_f", highpFloat,		m_caseDef.ioType != IO_TYPE_PER_PATCH);

		if (m_caseDef.ioType != IO_TYPE_PER_PATCH_ARRAY)
		{
			// Arrays of structures are disallowed, add struct cases only if not arrayed variable
			m_tcsOutputs.push_back	(de::SharedPtr<TopLevelObject>(new Variable(var0)));
			m_tesInputs.push_back	(de::SharedPtr<TopLevelObject>(new Variable(var0)));

			usedStruct = true;
		}

		m_tcsOutputs.push_back	(de::SharedPtr<TopLevelObject>(new Variable(var1)));
		m_tesInputs.push_back	(de::SharedPtr<TopLevelObject>(new Variable(var1)));
	}

	if (usedStruct)
		tcsDeclarations << de::toString(glu::declare(structType)) + ";\n";

	tcsStatements << "\t{\n"
				  << "\t\thighp float v = 1.3;\n";

	for (int tcsOutputNdx = 0; tcsOutputNdx < static_cast<int>(m_tcsOutputs.size()); ++tcsOutputNdx)
	{
		const TopLevelObject&	output		= *m_tcsOutputs[tcsOutputNdx];
		const int				numElements	= !isPerPatchIO										? -1	//!< \note -1 means indexing with gl_InstanceID
											: m_caseDef.ioType == IO_TYPE_PER_PATCH				? 1
											: m_caseDef.ioType == IO_TYPE_PER_PATCH_ARRAY		? NUM_PER_PATCH_ARRAY_ELEMS
											: m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK		? 1
											: m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY	? NUM_PER_PATCH_BLOCKS
											: wrongNumElements;
		const bool				isArray		= (numElements != 1);

		DE_ASSERT(numElements != wrongNumElements);

		// \note: TCS output arrays are always implicitly-sized
		tcsDeclarations << "layout(location = " << tcsNextOutputLocation << ") ";
		if (isArray)
			tcsDeclarations << outMaybePatch << output.declareArray(m_caseDef.ioType == IO_TYPE_PER_PATCH_ARRAY			? de::toString(NUM_PER_PATCH_ARRAY_ELEMS)
																  : m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY	? de::toString(NUM_PER_PATCH_BLOCKS)
																  : "");
		else
			tcsDeclarations << outMaybePatch << output.declare();

		tcsNextOutputLocation += output.numBasicSubobjectsInElementType();

		if (!isPerPatchIO)
			tcsStatements << "\t\tv += float(gl_InvocationID)*" << de::floatToString(0.4f * (float)output.numBasicSubobjectsInElementType(), 1) << ";\n";

		tcsStatements << "\n\t\t// Assign values to output " << output.name() << "\n";
		if (isArray)
			tcsStatements << output.glslTraverseBasicTypeArray(numElements, 2, glslAssignBasicTypeObject);
		else
			tcsStatements << output.glslTraverseBasicType(2, glslAssignBasicTypeObject);

		if (!isPerPatchIO)
			tcsStatements << "\t\tv += float(" << de::toString(NUM_OUTPUT_VERTICES) << "-gl_InvocationID-1)*" << de::floatToString(0.4f * (float)output.numBasicSubobjectsInElementType(), 1) << ";\n";
	}
	tcsStatements << "\t}\n";

	tcsDeclarations << "\n"
					<< "layout(location = 0) in " + Variable("in_tc_attr", highpFloat, true).declareArray(vertexAttrArrayInputSize);

	if (usedStruct)
		tesDeclarations << de::toString(glu::declare(structType)) << ";\n";

	tesStatements << "\tbool allOk = true;\n"
				  << "\thighp uint firstFailedInputIndex = 0u;\n"
				  << "\t{\n"
				  << "\t\thighp float v = 1.3;\n";

	for (int tesInputNdx = 0; tesInputNdx < static_cast<int>(m_tesInputs.size()); ++tesInputNdx)
	{
		const TopLevelObject&	input		= *m_tesInputs[tesInputNdx];
		const int				numElements	= !isPerPatchIO										? NUM_OUTPUT_VERTICES
											: m_caseDef.ioType == IO_TYPE_PER_PATCH				? 1
											: m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK		? 1
											: m_caseDef.ioType == IO_TYPE_PER_PATCH_ARRAY		? NUM_PER_PATCH_ARRAY_ELEMS
											: m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY	? NUM_PER_PATCH_BLOCKS
											: wrongNumElements;
		const bool				isArray		= (numElements != 1);

		DE_ASSERT(numElements != wrongNumElements);

		tesDeclarations << "layout(location = " << tesNextInputLocation << ") ";
		if (isArray)
			tesDeclarations << inMaybePatch << input.declareArray(m_caseDef.ioType == IO_TYPE_PER_PATCH_ARRAY		? de::toString(NUM_PER_PATCH_ARRAY_ELEMS)
																: m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY	? de::toString(NUM_PER_PATCH_BLOCKS)
																: isExplicitVertexArraySize							? de::toString(vertexAttrArrayInputSize)
																: "");
		else
			tesDeclarations << inMaybePatch + input.declare();

		tesNextInputLocation += input.numBasicSubobjectsInElementType();

		tesStatements << "\n\t\t// Check values in input " << input.name() << "\n";
		if (isArray)
			tesStatements << input.glslTraverseBasicTypeArray(numElements, 2, glslCheckBasicTypeObject);
		else
			tesStatements << input.glslTraverseBasicType(2, glslCheckBasicTypeObject);
	}
	tesStatements << "\t}\n";

	m_tcsDeclarations = tcsDeclarations.str();
	m_tcsStatements   = tcsStatements.str();
	m_tesDeclarations = tesDeclarations.str();
	m_tesStatements   = tesStatements.str();
}

void UserDefinedIOTest::initPrograms (vk::SourceCollections& programCollection) const
{
	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "layout(location = 0) in  highp float in_v_attr;\n"
			<< "layout(location = 0) out highp float in_tc_attr;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "	in_tc_attr = in_v_attr;\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Tessellation control shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "\n"
			<< "layout(vertices = " << NUM_OUTPUT_VERTICES << ") out;\n"
			<< "\n"
			<< "layout(location = 0) patch out highp vec2 in_te_positionScale;\n"
			<< "layout(location = 1) patch out highp vec2 in_te_positionOffset;\n"
			<< "\n"
			<< m_tcsDeclarations
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< m_tcsStatements
			<< "\n"
			<< "	gl_TessLevelInner[0] = in_tc_attr[0];\n"
			<< "	gl_TessLevelInner[1] = in_tc_attr[1];\n"
			<< "\n"
			<< "	gl_TessLevelOuter[0] = in_tc_attr[2];\n"
			<< "	gl_TessLevelOuter[1] = in_tc_attr[3];\n"
			<< "	gl_TessLevelOuter[2] = in_tc_attr[4];\n"
			<< "	gl_TessLevelOuter[3] = in_tc_attr[5];\n"
			<< "\n"
			<< "	in_te_positionScale  = vec2(in_tc_attr[6], in_tc_attr[7]);\n"
			<< "	in_te_positionOffset = vec2(in_tc_attr[8], in_tc_attr[9]);\n"
			<< "}\n";

		programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
	}

	// Tessellation evaluation shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "\n"
			<< "layout(" << getTessPrimitiveTypeShaderName(m_caseDef.primitiveType) << ") in;\n"
			<< "\n"
			<< "layout(location = 0) patch in highp vec2 in_te_positionScale;\n"
			<< "layout(location = 1) patch in highp vec2 in_te_positionOffset;\n"
			<< "\n"
			<< m_tesDeclarations
			<< "\n"
			<< "layout(location = 0) out highp vec4 in_f_color;\n"
			<< "\n"
			<< "// Will contain the index of the first incorrect input,\n"
			<< "// or the number of inputs if all are correct\n"
			<< "layout (set = 0, binding = 0, std430) coherent restrict buffer Output {\n"
			<< "    int  numInvocations;\n"
			<< "    uint firstFailedInputIndex[];\n"
			<< "} sb_out;\n"
			<< "\n"
			<< "bool compare_int   (int   a, int   b) { return a == b; }\n"
			<< "bool compare_float (float a, float b) { return abs(a - b) < 0.01f; }\n"
			<< "bool compare_vec4  (vec4  a, vec4  b) { return all(lessThan(abs(a - b), vec4(0.01f))); }\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< m_tesStatements
			<< "\n"
			<< "	gl_Position = vec4(gl_TessCoord.xy*in_te_positionScale + in_te_positionOffset, 0.0, 1.0);\n"
			<< "	in_f_color  = allOk ? vec4(0.0, 1.0, 0.0, 1.0)\n"
			<< "	                    : vec4(1.0, 0.0, 0.0, 1.0);\n"
			<< "\n"
			<< "	int index = atomicAdd(sb_out.numInvocations, 1);\n"
			<< "	sb_out.firstFailedInputIndex[index] = firstFailedInputIndex;\n"
			<< "}\n";

		programCollection.glslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "layout(location = 0) in  highp   vec4 in_f_color;\n"
			<< "layout(location = 0) out mediump vec4 o_color;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    o_color = in_f_color;\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

class UserDefinedIOTestInstance : public TestInstance
{
public:
							UserDefinedIOTestInstance	(Context&											context,
														 const CaseDefinition								caseDef,
														 const std::vector<de::SharedPtr<TopLevelObject> >&	tesInputs);
	tcu::TestStatus			iterate						(void);

private:
	const CaseDefinition								m_caseDef;
	const std::vector<de::SharedPtr<TopLevelObject> >	m_tesInputs;
};

UserDefinedIOTestInstance::UserDefinedIOTestInstance (Context& context, const CaseDefinition caseDef, const std::vector<de::SharedPtr<TopLevelObject> >& tesInputs)
	: TestInstance		(context)
	, m_caseDef			(caseDef)
	, m_tesInputs		(tesInputs)
{
}

tcu::TestStatus UserDefinedIOTestInstance::iterate (void)
{
	requireFeatures(m_context.getInstanceInterface(), m_context.getPhysicalDevice(), FEATURE_TESSELLATION_SHADER | FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS);

	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const int				numAttributes				= NUM_TESS_LEVELS + 2 + 2;
	static const float		attributes[numAttributes]	= { /* inner */ 3.0f, 4.0f, /* outer */ 5.0f, 6.0f, 7.0f, 8.0f, /* pos. scale */ 1.2f, 1.3f, /* pos. offset */ -0.3f, -0.4f };
	const int				refNumVertices				= referenceVertexCount(m_caseDef.primitiveType, SPACINGMODE_EQUAL, false, &attributes[0], &attributes[2]);
	const int				refNumUniqueVertices		= referenceVertexCount(m_caseDef.primitiveType, SPACINGMODE_EQUAL, true, &attributes[0], &attributes[2]);

	// Vertex input attributes buffer: to pass tessellation levels

	const VkFormat     vertexFormat				= VK_FORMAT_R32_SFLOAT;
	const deUint32     vertexStride				= tcu::getPixelSize(mapVkFormat(vertexFormat));
	const VkDeviceSize vertexDataSizeBytes		= numAttributes * vertexStride;
	const Buffer       vertexBuffer				(vk, device, allocator, makeBufferCreateInfo(vertexDataSizeBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), MemoryRequirement::HostVisible);

	{
		const Allocation& alloc = vertexBuffer.getAllocation();
		deMemcpy(alloc.getHostPtr(), &attributes[0], static_cast<std::size_t>(vertexDataSizeBytes));
		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), vertexDataSizeBytes);
	}

	// Output buffer: number of invocations and verification indices

	const int		   resultBufferMaxVertices	= refNumVertices;
	const VkDeviceSize resultBufferSizeBytes    = sizeof(deInt32) + resultBufferMaxVertices * sizeof(deUint32);
	const Buffer       resultBuffer             (vk, device, allocator, makeBufferCreateInfo(resultBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	{
		const Allocation& alloc = resultBuffer.getAllocation();
		deMemset(alloc.getHostPtr(), 0, static_cast<std::size_t>(resultBufferSizeBytes));
		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), resultBufferSizeBytes);
	}

	// Color attachment

	const tcu::IVec2			  renderSize				 = tcu::IVec2(RENDER_SIZE, RENDER_SIZE);
	const VkFormat				  colorFormat				 = VK_FORMAT_R8G8B8A8_UNORM;
	const VkImageSubresourceRange colorImageSubresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const Image					  colorAttachmentImage		 (vk, device, allocator,
															 makeImageCreateInfo(renderSize, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 1u),
															 MemoryRequirement::Any);

	// Color output buffer: image will be copied here for verification

	const VkDeviceSize	colorBufferSizeBytes	= renderSize.x()*renderSize.y() * tcu::getPixelSize(mapVkFormat(colorFormat));
	const Buffer		colorBuffer				(vk, device, allocator, makeBufferCreateInfo(colorBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);

	// Descriptors

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet    (makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));
	const VkDescriptorBufferInfo  resultBufferInfo = makeDescriptorBufferInfo(resultBuffer.get(), 0ull, resultBufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &resultBufferInfo)
		.update(vk, device);

	// Pipeline

	const Unique<VkImageView>      colorAttachmentView(makeImageView(vk, device, *colorAttachmentImage, VK_IMAGE_VIEW_TYPE_2D, colorFormat, colorImageSubresourceRange));
	const Unique<VkRenderPass>     renderPass         (makeRenderPass(vk, device, colorFormat));
	const Unique<VkFramebuffer>    framebuffer        (makeFramebuffer(vk, device, *renderPass, *colorAttachmentView, renderSize.x(), renderSize.y(), 1u));
	const Unique<VkPipelineLayout> pipelineLayout     (makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkCommandPool>    cmdPool            (makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer>  cmdBuffer          (allocateCommandBuffer (vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	const Unique<VkPipeline> pipeline(GraphicsPipelineBuilder()
		.setRenderSize                (renderSize)
		.setPatchControlPoints        (numAttributes)
		.setVertexInputSingleAttribute(vertexFormat, vertexStride)
		.setShader                    (vk, device, VK_SHADER_STAGE_VERTEX_BIT,					m_context.getBinaryCollection().get("vert"), DE_NULL)
		.setShader                    (vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	m_context.getBinaryCollection().get("tesc"), DE_NULL)
		.setShader                    (vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_context.getBinaryCollection().get("tese"), DE_NULL)
		.setShader                    (vk, device, VK_SHADER_STAGE_FRAGMENT_BIT,				m_context.getBinaryCollection().get("frag"), DE_NULL)
		.build                        (vk, device, *pipelineLayout, *renderPass));

	// Begin draw

	beginCommandBuffer(vk, *cmdBuffer);

	// Change color attachment image layout
	{
		const VkImageMemoryBarrier colorAttachmentLayoutBarrier = makeImageMemoryBarrier(
			(VkAccessFlags)0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			*colorAttachmentImage, colorImageSubresourceRange);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u,
			0u, DE_NULL, 0u, DE_NULL, 1u, &colorAttachmentLayoutBarrier);
	}

	{
		const VkRect2D renderArea = {
			makeOffset2D(0, 0),
			makeExtent2D(renderSize.x(), renderSize.y()),
		};
		const tcu::Vec4 clearColor(0.0f, 0.0f, 0.0f, 1.0f);

		beginRenderPass(vk, *cmdBuffer, *renderPass, *framebuffer, renderArea, clearColor);
	}

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
	{
		const VkDeviceSize vertexBufferOffset = 0ull;
		vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer.get(), &vertexBufferOffset);
	}

	vk.cmdDraw(*cmdBuffer, numAttributes, 1u, 0u, 0u);
	endRenderPass(vk, *cmdBuffer);

	// Copy render result to a host-visible buffer
	{
		const VkImageMemoryBarrier colorAttachmentPreCopyBarrier = makeImageMemoryBarrier(
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*colorAttachmentImage, colorImageSubresourceRange);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
			0u, DE_NULL, 0u, DE_NULL, 1u, &colorAttachmentPreCopyBarrier);
	}
	{
		const VkBufferImageCopy copyRegion = makeBufferImageCopy(makeExtent3D(renderSize.x(), renderSize.y(), 1), makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u));
		vk.cmdCopyImageToBuffer(*cmdBuffer, *colorAttachmentImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *colorBuffer, 1u, &copyRegion);
	}
	{
		const VkBufferMemoryBarrier postCopyBarrier = makeBufferMemoryBarrier(
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *colorBuffer, 0ull, colorBufferSizeBytes);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
			0u, DE_NULL, 1u, &postCopyBarrier, 0u, DE_NULL);
	}

	endCommandBuffer(vk, *cmdBuffer);
	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Verification

	bool isImageCompareOK = false;
	{
		const Allocation& colorBufferAlloc = colorBuffer.getAllocation();
		invalidateMappedMemoryRange(vk, device, colorBufferAlloc.getMemory(), colorBufferAlloc.getOffset(), colorBufferSizeBytes);

		// Load reference image
		tcu::TextureLevel referenceImage;
		tcu::ImageIO::loadPNG(referenceImage, m_context.getTestContext().getArchive(), m_caseDef.referenceImagePath.c_str());

		// Verify case result
		const tcu::ConstPixelBufferAccess resultImageAccess(mapVkFormat(colorFormat), renderSize.x(), renderSize.y(), 1, colorBufferAlloc.getHostPtr());
		isImageCompareOK = tcu::fuzzyCompare(m_context.getTestContext().getLog(), "ImageComparison", "Image Comparison",
											 referenceImage.getAccess(), resultImageAccess, 0.02f, tcu::COMPARE_LOG_RESULT);
	}
	{
		const Allocation& resultAlloc = resultBuffer.getAllocation();
		invalidateMappedMemoryRange(vk, device, resultAlloc.getMemory(), resultAlloc.getOffset(), resultBufferSizeBytes);

		const deInt32			numVertices = *static_cast<deInt32*>(resultAlloc.getHostPtr());
		const deUint32* const	vertices    = reinterpret_cast<deUint32*>(static_cast<deUint8*>(resultAlloc.getHostPtr()) + sizeof(deInt32));

		// If this fails then we didn't read all vertices from shader and test must be changed to allow more.
		DE_ASSERT(numVertices <= refNumVertices);

		if (numVertices < refNumUniqueVertices)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Failure: got " << numVertices << " vertices, but expected at least " << refNumUniqueVertices << tcu::TestLog::EndMessage;

			return tcu::TestStatus::fail("Wrong number of vertices");
		}
		else
		{
			tcu::TestLog&	log					= m_context.getTestContext().getLog();
			const int		topLevelArraySize	= (m_caseDef.ioType == IO_TYPE_PER_PATCH			? 1
												: m_caseDef.ioType == IO_TYPE_PER_PATCH_ARRAY		? NUM_PER_PATCH_ARRAY_ELEMS
												: m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK		? 1
												: m_caseDef.ioType == IO_TYPE_PER_PATCH_BLOCK_ARRAY	? NUM_PER_PATCH_BLOCKS
												: NUM_OUTPUT_VERTICES);
			const deUint32	numTEInputs			= numBasicSubobjectsInElementType(m_tesInputs) * topLevelArraySize;

			for (int vertexNdx = 0; vertexNdx < numVertices; ++vertexNdx)
				if (vertices[vertexNdx] > numTEInputs)
				{
					log << tcu::TestLog::Message
						<< "Failure: out_te_firstFailedInputIndex has value " << vertices[vertexNdx]
						<< ", but should be in range [0, " << numTEInputs << "]" << tcu::TestLog::EndMessage;

					return tcu::TestStatus::fail("Invalid values returned from shader");
				}
				else if (vertices[vertexNdx] != numTEInputs)
				{
					log << tcu::TestLog::Message << "Failure: in tessellation evaluation shader, check for input "
						<< basicSubobjectAtIndex(vertices[vertexNdx], m_tesInputs, topLevelArraySize) << " failed" << tcu::TestLog::EndMessage;

					return tcu::TestStatus::fail("Invalid input value in tessellation evaluation shader");
				}
		}
	}
	return (isImageCompareOK ? tcu::TestStatus::pass("OK") : tcu::TestStatus::fail("Image comparison failed"));
}

TestInstance* UserDefinedIOTest::createInstance (Context& context) const
{
	return new UserDefinedIOTestInstance(context, m_caseDef, m_tesInputs);
}

} // anonymous

//! These tests correspond roughly to dEQP-GLES31.functional.tessellation.user_defined_io.*
//! Original GLES test queried maxTessellationPatchSize, but this can't be done at the stage the shader source is prepared.
//! Instead, we use minimum supported value.
//! Negative tests weren't ported because vktShaderLibrary doesn't support tests that are expected to fail shader compilation.
tcu::TestCaseGroup* createUserDefinedIOTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "user_defined_io", "Test non-built-in per-patch and per-vertex inputs and outputs"));

	static const struct
	{
		const char*	name;
		const char*	description;
		IOType		ioType;
	} ioCases[] =
	{
		{ "per_patch",					"Per-patch TCS outputs",					IO_TYPE_PER_PATCH				},
		{ "per_patch_array",			"Per-patch array TCS outputs",				IO_TYPE_PER_PATCH_ARRAY			},
		{ "per_patch_block",			"Per-patch TCS outputs in IO block",		IO_TYPE_PER_PATCH_BLOCK			},
		{ "per_patch_block_array",		"Per-patch TCS outputs in IO block array",	IO_TYPE_PER_PATCH_BLOCK_ARRAY	},
		{ "per_vertex",					"Per-vertex TCS outputs",					IO_TYPE_PER_VERTEX				},
		{ "per_vertex_block",			"Per-vertex TCS outputs in IO block",		IO_TYPE_PER_VERTEX_BLOCK		},
	};

	static const struct
	{
		const char*			name;
		VertexIOArraySize	vertexIOArraySize;
	} vertexArraySizeCases[] =
	{
		{ "vertex_io_array_size_implicit",			VERTEX_IO_ARRAY_SIZE_IMPLICIT					},
		{ "vertex_io_array_size_shader_builtin",	VERTEX_IO_ARRAY_SIZE_EXPLICIT_SHADER_BUILTIN	},
		{ "vertex_io_array_size_spec_min",			VERTEX_IO_ARRAY_SIZE_EXPLICIT_SPEC_MIN			},
	};

	for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(ioCases); ++caseNdx)
	{
		de::MovePtr<tcu::TestCaseGroup> ioTypeGroup (new tcu::TestCaseGroup(testCtx, ioCases[caseNdx].name, ioCases[caseNdx].description));
		for (int arrayCaseNdx = 0; arrayCaseNdx < DE_LENGTH_OF_ARRAY(vertexArraySizeCases); ++arrayCaseNdx)
		{
			de::MovePtr<tcu::TestCaseGroup> vertexArraySizeGroup (new tcu::TestCaseGroup(testCtx, vertexArraySizeCases[arrayCaseNdx].name, ""));
			for (int primitiveTypeNdx = 0; primitiveTypeNdx < TESSPRIMITIVETYPE_LAST; ++primitiveTypeNdx)
			{
				const TessPrimitiveType primitiveType = static_cast<TessPrimitiveType>(primitiveTypeNdx);
				const std::string		primitiveName = getTessPrimitiveTypeShaderName(primitiveType);
				const CaseDefinition	caseDef		  = { primitiveType, ioCases[caseNdx].ioType, vertexArraySizeCases[arrayCaseNdx].vertexIOArraySize,
														  std::string() + "vulkan/data/tessellation/user_defined_io_" + primitiveName + "_ref.png" };

				vertexArraySizeGroup->addChild(new UserDefinedIOTest(testCtx, primitiveName, "", caseDef));
			}
			ioTypeGroup->addChild(vertexArraySizeGroup.release());
		}
		group->addChild(ioTypeGroup.release());
	}

	return group.release();
}

} // tessellation
} // vkt
