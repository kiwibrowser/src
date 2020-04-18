/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 * Copyright (c) 2016 The Android Open Source Project
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
 * \brief Uniform block case.
 *//*--------------------------------------------------------------------*/

#include "vktUniformBlockCase.hpp"

#include "vkPrograms.hpp"

#include "gluVarType.hpp"
#include "tcuTestLog.hpp"
#include "tcuSurface.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"

#include "tcuTextureUtil.hpp"
#include "deSharedPtr.hpp"

#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkBuilderUtil.hpp"

#include <map>
#include <set>

namespace vkt
{
namespace ubo
{

using namespace vk;

// VarType implementation.

VarType::VarType (void)
	: m_type	(TYPE_LAST)
	, m_flags	(0)
{
}

VarType::VarType (const VarType& other)
	: m_type	(TYPE_LAST)
	, m_flags	(0)
{
	*this = other;
}

VarType::VarType (glu::DataType basicType, deUint32 flags)
	: m_type	(TYPE_BASIC)
	, m_flags	(flags)
{
	m_data.basicType = basicType;
}

VarType::VarType (const VarType& elementType, int arraySize)
	: m_type	(TYPE_ARRAY)
	, m_flags	(0)
{
	m_data.array.size			= arraySize;
	m_data.array.elementType	= new VarType(elementType);
}

VarType::VarType (const StructType* structPtr, deUint32 flags)
	: m_type	(TYPE_STRUCT)
	, m_flags	(flags)
{
	m_data.structPtr = structPtr;
}

VarType::~VarType (void)
{
	if (m_type == TYPE_ARRAY)
		delete m_data.array.elementType;
}

VarType& VarType::operator= (const VarType& other)
{
	if (this == &other)
		return *this; // Self-assignment.

	if (m_type == TYPE_ARRAY)
		delete m_data.array.elementType;

	m_type	= other.m_type;
	m_flags	= other.m_flags;
	m_data	= Data();

	if (m_type == TYPE_ARRAY)
	{
		m_data.array.elementType	= new VarType(*other.m_data.array.elementType);
		m_data.array.size			= other.m_data.array.size;
	}
	else
		m_data = other.m_data;

	return *this;
}

// StructType implementation.

void StructType::addMember (const std::string& name, const VarType& type, deUint32 flags)
{
	m_members.push_back(StructMember(name, type, flags));
}

// Uniform implementation.

Uniform::Uniform (const std::string& name, const VarType& type, deUint32 flags)
	: m_name	(name)
	, m_type	(type)
	, m_flags	(flags)
{
}

// UniformBlock implementation.

UniformBlock::UniformBlock (const std::string& blockName)
	: m_blockName	(blockName)
	, m_arraySize	(0)
	, m_flags		(0)
{
}

std::ostream& operator<< (std::ostream& stream, const BlockLayoutEntry& entry)
{
	stream << entry.name << " { name = " << entry.name
		   << ", size = " << entry.size
		   << ", activeUniformIndices = [";

	for (std::vector<int>::const_iterator i = entry.activeUniformIndices.begin(); i != entry.activeUniformIndices.end(); i++)
	{
		if (i != entry.activeUniformIndices.begin())
			stream << ", ";
		stream << *i;
	}

	stream << "] }";
	return stream;
}

std::ostream& operator<< (std::ostream& stream, const UniformLayoutEntry& entry)
{
	stream << entry.name << " { type = " << glu::getDataTypeName(entry.type)
		   << ", size = " << entry.size
		   << ", blockNdx = " << entry.blockLayoutNdx
		   << ", offset = " << entry.offset
		   << ", arrayStride = " << entry.arrayStride
		   << ", matrixStride = " << entry.matrixStride
		   << ", isRowMajor = " << (entry.isRowMajor ? "true" : "false")
		   << " }";
	return stream;
}

int UniformLayout::getUniformLayoutIndex (int blockNdx, const std::string& name) const
{
	for (int ndx = 0; ndx < (int)uniforms.size(); ndx++)
	{
		if (blocks[uniforms[ndx].blockLayoutNdx].blockDeclarationNdx == blockNdx &&
			uniforms[ndx].name == name)
			return ndx;
	}

	return -1;
}

int UniformLayout::getBlockLayoutIndex (int blockNdx, int instanceNdx) const
{
	for (int ndx = 0; ndx < (int)blocks.size(); ndx++)
	{
		if (blocks[ndx].blockDeclarationNdx == blockNdx &&
			blocks[ndx].instanceNdx == instanceNdx)
			return ndx;
	}

	return -1;
}

// ShaderInterface implementation.

ShaderInterface::ShaderInterface (void)
{
}

ShaderInterface::~ShaderInterface (void)
{
}

StructType& ShaderInterface::allocStruct (const std::string& name)
{
	m_structs.push_back(StructTypeSP(new StructType(name)));
	return *m_structs.back();
}

struct StructNameEquals
{
	std::string name;

	StructNameEquals (const std::string& name_) : name(name_) {}

	bool operator() (const StructTypeSP type) const
	{
		return type->hasTypeName() && name == type->getTypeName();
	}
};

void ShaderInterface::getNamedStructs (std::vector<const StructType*>& structs) const
{
	for (std::vector<StructTypeSP>::const_iterator i = m_structs.begin(); i != m_structs.end(); i++)
	{
		if ((*i)->hasTypeName())
			structs.push_back((*i).get());
	}
}

UniformBlock& ShaderInterface::allocBlock (const std::string& name)
{
	m_uniformBlocks.push_back(UniformBlockSP(new UniformBlock(name)));
	return *m_uniformBlocks.back();
}

namespace // Utilities
{

struct PrecisionFlagsFmt
{
	deUint32 flags;
	PrecisionFlagsFmt (deUint32 flags_) : flags(flags_) {}
};

std::ostream& operator<< (std::ostream& str, const PrecisionFlagsFmt& fmt)
{
	// Precision.
	DE_ASSERT(dePop32(fmt.flags & (PRECISION_LOW|PRECISION_MEDIUM|PRECISION_HIGH)) <= 1);
	str << (fmt.flags & PRECISION_LOW		? "lowp"	:
			fmt.flags & PRECISION_MEDIUM	? "mediump"	:
			fmt.flags & PRECISION_HIGH		? "highp"	: "");
	return str;
}

struct LayoutFlagsFmt
{
	deUint32 flags;
	deUint32 offset;
	LayoutFlagsFmt (deUint32 flags_, deUint32 offset_ = 0u) : flags(flags_), offset(offset_) {}
};

std::ostream& operator<< (std::ostream& str, const LayoutFlagsFmt& fmt)
{
	static const struct
	{
		deUint32	bit;
		const char*	token;
	} bitDesc[] =
	{
		{ LAYOUT_STD140,		"std140"		},
		{ LAYOUT_ROW_MAJOR,		"row_major"		},
		{ LAYOUT_COLUMN_MAJOR,	"column_major"	},
		{ LAYOUT_OFFSET,		"offset"		},
	};

	deUint32 remBits = fmt.flags;
	for (int descNdx = 0; descNdx < DE_LENGTH_OF_ARRAY(bitDesc); descNdx++)
	{
		if (remBits & bitDesc[descNdx].bit)
		{
			if (remBits != fmt.flags)
				str << ", ";
			str << bitDesc[descNdx].token;
			if (bitDesc[descNdx].bit == LAYOUT_OFFSET)
				str << " = " << fmt.offset;
			remBits &= ~bitDesc[descNdx].bit;
		}
	}
	DE_ASSERT(remBits == 0);
	return str;
}

// Layout computation.

int getDataTypeByteSize (glu::DataType type)
{
	return glu::getDataTypeScalarSize(type)*(int)sizeof(deUint32);
}

int getDataTypeByteAlignment (glu::DataType type)
{
	switch (type)
	{
		case glu::TYPE_FLOAT:
		case glu::TYPE_INT:
		case glu::TYPE_UINT:
		case glu::TYPE_BOOL:		return 1*(int)sizeof(deUint32);

		case glu::TYPE_FLOAT_VEC2:
		case glu::TYPE_INT_VEC2:
		case glu::TYPE_UINT_VEC2:
		case glu::TYPE_BOOL_VEC2:	return 2*(int)sizeof(deUint32);

		case glu::TYPE_FLOAT_VEC3:
		case glu::TYPE_INT_VEC3:
		case glu::TYPE_UINT_VEC3:
		case glu::TYPE_BOOL_VEC3:	// Fall-through to vec4

		case glu::TYPE_FLOAT_VEC4:
		case glu::TYPE_INT_VEC4:
		case glu::TYPE_UINT_VEC4:
		case glu::TYPE_BOOL_VEC4:	return 4*(int)sizeof(deUint32);

		default:
			DE_ASSERT(false);
			return 0;
	}
}

deInt32 getminUniformBufferOffsetAlignment (Context &ctx)
{
	VkPhysicalDeviceProperties properties;
	ctx.getInstanceInterface().getPhysicalDeviceProperties(ctx.getPhysicalDevice(), &properties);
	VkDeviceSize align = properties.limits.minUniformBufferOffsetAlignment;
	DE_ASSERT(align == (VkDeviceSize)(deInt32)align);
	return (deInt32)align;
}

int getDataTypeArrayStride (glu::DataType type)
{
	DE_ASSERT(!glu::isDataTypeMatrix(type));

	const int baseStride	= getDataTypeByteSize(type);
	const int vec4Alignment	= (int)sizeof(deUint32)*4;

	DE_ASSERT(baseStride <= vec4Alignment);
	return de::max(baseStride, vec4Alignment); // Really? See rule 4.
}

static inline int deRoundUp32 (int a, int b)
{
	int d = a/b;
	return d*b == a ? a : (d+1)*b;
}

int computeStd140BaseAlignment (const VarType& type)
{
	const int vec4Alignment = (int)sizeof(deUint32)*4;

	if (type.isBasicType())
	{
		glu::DataType basicType = type.getBasicType();

		if (glu::isDataTypeMatrix(basicType))
		{
			bool	isRowMajor	= !!(type.getFlags() & LAYOUT_ROW_MAJOR);
			int		vecSize		= isRowMajor ? glu::getDataTypeMatrixNumColumns(basicType)
											 : glu::getDataTypeMatrixNumRows(basicType);

			return getDataTypeArrayStride(glu::getDataTypeFloatVec(vecSize));
		}
		else
			return getDataTypeByteAlignment(basicType);
	}
	else if (type.isArrayType())
	{
		int elemAlignment = computeStd140BaseAlignment(type.getElementType());

		// Round up to alignment of vec4
		return deRoundUp32(elemAlignment, vec4Alignment);
	}
	else
	{
		DE_ASSERT(type.isStructType());

		int maxBaseAlignment = 0;

		for (StructType::ConstIterator memberIter = type.getStruct().begin(); memberIter != type.getStruct().end(); memberIter++)
			maxBaseAlignment = de::max(maxBaseAlignment, computeStd140BaseAlignment(memberIter->getType()));

		return deRoundUp32(maxBaseAlignment, vec4Alignment);
	}
}

inline deUint32 mergeLayoutFlags (deUint32 prevFlags, deUint32 newFlags)
{
	const deUint32	packingMask		= LAYOUT_STD140;
	const deUint32	matrixMask		= LAYOUT_ROW_MAJOR|LAYOUT_COLUMN_MAJOR;

	deUint32 mergedFlags = 0;

	mergedFlags |= ((newFlags & packingMask)	? newFlags : prevFlags) & packingMask;
	mergedFlags |= ((newFlags & matrixMask)		? newFlags : prevFlags) & matrixMask;

	return mergedFlags;
}

void computeStd140Layout (UniformLayout& layout, int& curOffset, int curBlockNdx, const std::string& curPrefix, const VarType& type, deUint32 layoutFlags)
{
	int baseAlignment = computeStd140BaseAlignment(type);

	curOffset = deAlign32(curOffset, baseAlignment);

	if (type.isBasicType())
	{
		glu::DataType		basicType	= type.getBasicType();
		UniformLayoutEntry	entry;

		entry.name			= curPrefix;
		entry.type			= basicType;
		entry.size			= 1;
		entry.arrayStride	= 0;
		entry.matrixStride	= 0;
		entry.blockLayoutNdx= curBlockNdx;

		if (glu::isDataTypeMatrix(basicType))
		{
			// Array of vectors as specified in rules 5 & 7.
			bool	isRowMajor	= !!(layoutFlags & LAYOUT_ROW_MAJOR);
			int		vecSize		= isRowMajor ? glu::getDataTypeMatrixNumColumns(basicType)
											 : glu::getDataTypeMatrixNumRows(basicType);
			int		numVecs		= isRowMajor ? glu::getDataTypeMatrixNumRows(basicType)
											 : glu::getDataTypeMatrixNumColumns(basicType);
			int		stride		= getDataTypeArrayStride(glu::getDataTypeFloatVec(vecSize));

			entry.offset		= curOffset;
			entry.matrixStride	= stride;
			entry.isRowMajor	= isRowMajor;

			curOffset += numVecs*stride;
		}
		else
		{
			// Scalar or vector.
			entry.offset = curOffset;

			curOffset += getDataTypeByteSize(basicType);
		}

		layout.uniforms.push_back(entry);
	}
	else if (type.isArrayType())
	{
		const VarType&	elemType	= type.getElementType();

		if (elemType.isBasicType() && !glu::isDataTypeMatrix(elemType.getBasicType()))
		{
			// Array of scalars or vectors.
			glu::DataType		elemBasicType	= elemType.getBasicType();
			UniformLayoutEntry	entry;
			int					stride			= getDataTypeArrayStride(elemBasicType);

			entry.name			= curPrefix + "[0]"; // Array uniforms are always postfixed with [0]
			entry.type			= elemBasicType;
			entry.blockLayoutNdx= curBlockNdx;
			entry.offset		= curOffset;
			entry.size			= type.getArraySize();
			entry.arrayStride	= stride;
			entry.matrixStride	= 0;

			curOffset += stride*type.getArraySize();

			layout.uniforms.push_back(entry);
		}
		else if (elemType.isBasicType() && glu::isDataTypeMatrix(elemType.getBasicType()))
		{
			// Array of matrices.
			glu::DataType		elemBasicType	= elemType.getBasicType();
			bool				isRowMajor		= !!(layoutFlags & LAYOUT_ROW_MAJOR);
			int					vecSize			= isRowMajor ? glu::getDataTypeMatrixNumColumns(elemBasicType)
															 : glu::getDataTypeMatrixNumRows(elemBasicType);
			int					numVecs			= isRowMajor ? glu::getDataTypeMatrixNumRows(elemBasicType)
															 : glu::getDataTypeMatrixNumColumns(elemBasicType);
			int					stride			= getDataTypeArrayStride(glu::getDataTypeFloatVec(vecSize));
			UniformLayoutEntry	entry;

			entry.name			= curPrefix + "[0]"; // Array uniforms are always postfixed with [0]
			entry.type			= elemBasicType;
			entry.blockLayoutNdx= curBlockNdx;
			entry.offset		= curOffset;
			entry.size			= type.getArraySize();
			entry.arrayStride	= stride*numVecs;
			entry.matrixStride	= stride;
			entry.isRowMajor	= isRowMajor;

			curOffset += numVecs*type.getArraySize()*stride;

			layout.uniforms.push_back(entry);
		}
		else
		{
			DE_ASSERT(elemType.isStructType() || elemType.isArrayType());

			for (int elemNdx = 0; elemNdx < type.getArraySize(); elemNdx++)
				computeStd140Layout(layout, curOffset, curBlockNdx, curPrefix + "[" + de::toString(elemNdx) + "]", type.getElementType(), layoutFlags);
		}
	}
	else
	{
		DE_ASSERT(type.isStructType());

		for (StructType::ConstIterator memberIter = type.getStruct().begin(); memberIter != type.getStruct().end(); memberIter++)
			computeStd140Layout(layout, curOffset, curBlockNdx, curPrefix + "." + memberIter->getName(), memberIter->getType(), layoutFlags);

		curOffset = deAlign32(curOffset, baseAlignment);
	}
}

void computeStd140Layout (UniformLayout& layout, const ShaderInterface& interface)
{
	int numUniformBlocks = interface.getNumUniformBlocks();

	for (int blockNdx = 0; blockNdx < numUniformBlocks; blockNdx++)
	{
		const UniformBlock&	block			= interface.getUniformBlock(blockNdx);
		bool				hasInstanceName	= block.hasInstanceName();
		std::string			blockPrefix		= hasInstanceName ? (block.getBlockName() + ".") : "";
		int					curOffset		= 0;
		int					activeBlockNdx	= (int)layout.blocks.size();
		int					firstUniformNdx	= (int)layout.uniforms.size();

		for (UniformBlock::ConstIterator uniformIter = block.begin(); uniformIter != block.end(); uniformIter++)
		{
			const Uniform& uniform = *uniformIter;
			computeStd140Layout(layout, curOffset, activeBlockNdx, blockPrefix + uniform.getName(), uniform.getType(), mergeLayoutFlags(block.getFlags(), uniform.getFlags()));
		}

		int	uniformIndicesEnd	= (int)layout.uniforms.size();
		int	blockSize			= curOffset;
		int	numInstances		= block.isArray() ? block.getArraySize() : 1;

		// Create block layout entries for each instance.
		for (int instanceNdx = 0; instanceNdx < numInstances; instanceNdx++)
		{
			// Allocate entry for instance.
			layout.blocks.push_back(BlockLayoutEntry());
			BlockLayoutEntry& blockEntry = layout.blocks.back();

			blockEntry.name = block.getBlockName();
			blockEntry.size = blockSize;
			blockEntry.bindingNdx = blockNdx;
			blockEntry.blockDeclarationNdx = blockNdx;
			blockEntry.instanceNdx = instanceNdx;

			// Compute active uniform set for block.
			for (int uniformNdx = firstUniformNdx; uniformNdx < uniformIndicesEnd; uniformNdx++)
				blockEntry.activeUniformIndices.push_back(uniformNdx);

			if (block.isArray())
				blockEntry.name += "[" + de::toString(instanceNdx) + "]";
		}
	}
}

// Value generator.

void generateValue (const UniformLayoutEntry& entry, void* basePtr, de::Random& rnd)
{
	glu::DataType	scalarType		= glu::getDataTypeScalarType(entry.type);
	int				scalarSize		= glu::getDataTypeScalarSize(entry.type);
	bool			isMatrix		= glu::isDataTypeMatrix(entry.type);
	int				numVecs			= isMatrix ? (entry.isRowMajor ? glu::getDataTypeMatrixNumRows(entry.type) : glu::getDataTypeMatrixNumColumns(entry.type)) : 1;
	int				vecSize			= scalarSize / numVecs;
	bool			isArray			= entry.size > 1;
	const int		compSize		= sizeof(deUint32);

	DE_ASSERT(scalarSize%numVecs == 0);

	for (int elemNdx = 0; elemNdx < entry.size; elemNdx++)
	{
		deUint8* elemPtr = (deUint8*)basePtr + entry.offset + (isArray ? elemNdx*entry.arrayStride : 0);

		for (int vecNdx = 0; vecNdx < numVecs; vecNdx++)
		{
			deUint8* vecPtr = elemPtr + (isMatrix ? vecNdx*entry.matrixStride : 0);

			for (int compNdx = 0; compNdx < vecSize; compNdx++)
			{
				deUint8* compPtr = vecPtr + compSize*compNdx;

				switch (scalarType)
				{
					case glu::TYPE_FLOAT:	*((float*)compPtr)		= (float)rnd.getInt(-9, 9);						break;
					case glu::TYPE_INT:		*((int*)compPtr)		= rnd.getInt(-9, 9);							break;
					case glu::TYPE_UINT:	*((deUint32*)compPtr)	= (deUint32)rnd.getInt(0, 9);					break;
					// \note Random bit pattern is used for true values. Spec states that all non-zero values are
					//       interpreted as true but some implementations fail this.
					case glu::TYPE_BOOL:	*((deUint32*)compPtr)	= rnd.getBool() ? rnd.getUint32()|1u : 0u;		break;
					default:
						DE_ASSERT(false);
				}
			}
		}
	}
}

void generateValues (const UniformLayout& layout, const std::map<int, void*>& blockPointers, deUint32 seed)
{
	de::Random	rnd			(seed);
	int			numBlocks	= (int)layout.blocks.size();

	for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
	{
		void*	basePtr		= blockPointers.find(blockNdx)->second;
		int		numEntries	= (int)layout.blocks[blockNdx].activeUniformIndices.size();

		for (int entryNdx = 0; entryNdx < numEntries; entryNdx++)
		{
			const UniformLayoutEntry& entry = layout.uniforms[layout.blocks[blockNdx].activeUniformIndices[entryNdx]];
			generateValue(entry, basePtr, rnd);
		}
	}
}

// Shader generator.

const char* getCompareFuncForType (glu::DataType type)
{
	switch (type)
	{
		case glu::TYPE_FLOAT:			return "mediump float compare_float    (highp float a, highp float b)  { return abs(a - b) < 0.05 ? 1.0 : 0.0; }\n";
		case glu::TYPE_FLOAT_VEC2:		return "mediump float compare_vec2     (highp vec2 a, highp vec2 b)    { return compare_float(a.x, b.x)*compare_float(a.y, b.y); }\n";
		case glu::TYPE_FLOAT_VEC3:		return "mediump float compare_vec3     (highp vec3 a, highp vec3 b)    { return compare_float(a.x, b.x)*compare_float(a.y, b.y)*compare_float(a.z, b.z); }\n";
		case glu::TYPE_FLOAT_VEC4:		return "mediump float compare_vec4     (highp vec4 a, highp vec4 b)    { return compare_float(a.x, b.x)*compare_float(a.y, b.y)*compare_float(a.z, b.z)*compare_float(a.w, b.w); }\n";
		case glu::TYPE_FLOAT_MAT2:		return "mediump float compare_mat2     (highp mat2 a, highp mat2 b)    { return compare_vec2(a[0], b[0])*compare_vec2(a[1], b[1]); }\n";
		case glu::TYPE_FLOAT_MAT2X3:	return "mediump float compare_mat2x3   (highp mat2x3 a, highp mat2x3 b){ return compare_vec3(a[0], b[0])*compare_vec3(a[1], b[1]); }\n";
		case glu::TYPE_FLOAT_MAT2X4:	return "mediump float compare_mat2x4   (highp mat2x4 a, highp mat2x4 b){ return compare_vec4(a[0], b[0])*compare_vec4(a[1], b[1]); }\n";
		case glu::TYPE_FLOAT_MAT3X2:	return "mediump float compare_mat3x2   (highp mat3x2 a, highp mat3x2 b){ return compare_vec2(a[0], b[0])*compare_vec2(a[1], b[1])*compare_vec2(a[2], b[2]); }\n";
		case glu::TYPE_FLOAT_MAT3:		return "mediump float compare_mat3     (highp mat3 a, highp mat3 b)    { return compare_vec3(a[0], b[0])*compare_vec3(a[1], b[1])*compare_vec3(a[2], b[2]); }\n";
		case glu::TYPE_FLOAT_MAT3X4:	return "mediump float compare_mat3x4   (highp mat3x4 a, highp mat3x4 b){ return compare_vec4(a[0], b[0])*compare_vec4(a[1], b[1])*compare_vec4(a[2], b[2]); }\n";
		case glu::TYPE_FLOAT_MAT4X2:	return "mediump float compare_mat4x2   (highp mat4x2 a, highp mat4x2 b){ return compare_vec2(a[0], b[0])*compare_vec2(a[1], b[1])*compare_vec2(a[2], b[2])*compare_vec2(a[3], b[3]); }\n";
		case glu::TYPE_FLOAT_MAT4X3:	return "mediump float compare_mat4x3   (highp mat4x3 a, highp mat4x3 b){ return compare_vec3(a[0], b[0])*compare_vec3(a[1], b[1])*compare_vec3(a[2], b[2])*compare_vec3(a[3], b[3]); }\n";
		case glu::TYPE_FLOAT_MAT4:		return "mediump float compare_mat4     (highp mat4 a, highp mat4 b)    { return compare_vec4(a[0], b[0])*compare_vec4(a[1], b[1])*compare_vec4(a[2], b[2])*compare_vec4(a[3], b[3]); }\n";
		case glu::TYPE_INT:				return "mediump float compare_int      (highp int a, highp int b)      { return a == b ? 1.0 : 0.0; }\n";
		case glu::TYPE_INT_VEC2:		return "mediump float compare_ivec2    (highp ivec2 a, highp ivec2 b)  { return a == b ? 1.0 : 0.0; }\n";
		case glu::TYPE_INT_VEC3:		return "mediump float compare_ivec3    (highp ivec3 a, highp ivec3 b)  { return a == b ? 1.0 : 0.0; }\n";
		case glu::TYPE_INT_VEC4:		return "mediump float compare_ivec4    (highp ivec4 a, highp ivec4 b)  { return a == b ? 1.0 : 0.0; }\n";
		case glu::TYPE_UINT:			return "mediump float compare_uint     (highp uint a, highp uint b)    { return a == b ? 1.0 : 0.0; }\n";
		case glu::TYPE_UINT_VEC2:		return "mediump float compare_uvec2    (highp uvec2 a, highp uvec2 b)  { return a == b ? 1.0 : 0.0; }\n";
		case glu::TYPE_UINT_VEC3:		return "mediump float compare_uvec3    (highp uvec3 a, highp uvec3 b)  { return a == b ? 1.0 : 0.0; }\n";
		case glu::TYPE_UINT_VEC4:		return "mediump float compare_uvec4    (highp uvec4 a, highp uvec4 b)  { return a == b ? 1.0 : 0.0; }\n";
		case glu::TYPE_BOOL:			return "mediump float compare_bool     (bool a, bool b)                { return a == b ? 1.0 : 0.0; }\n";
		case glu::TYPE_BOOL_VEC2:		return "mediump float compare_bvec2    (bvec2 a, bvec2 b)              { return a == b ? 1.0 : 0.0; }\n";
		case glu::TYPE_BOOL_VEC3:		return "mediump float compare_bvec3    (bvec3 a, bvec3 b)              { return a == b ? 1.0 : 0.0; }\n";
		case glu::TYPE_BOOL_VEC4:		return "mediump float compare_bvec4    (bvec4 a, bvec4 b)              { return a == b ? 1.0 : 0.0; }\n";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

void getCompareDependencies (std::set<glu::DataType>& compareFuncs, glu::DataType basicType)
{
	switch (basicType)
	{
		case glu::TYPE_FLOAT_VEC2:
		case glu::TYPE_FLOAT_VEC3:
		case glu::TYPE_FLOAT_VEC4:
			compareFuncs.insert(glu::TYPE_FLOAT);
			compareFuncs.insert(basicType);
			break;

		case glu::TYPE_FLOAT_MAT2:
		case glu::TYPE_FLOAT_MAT2X3:
		case glu::TYPE_FLOAT_MAT2X4:
		case glu::TYPE_FLOAT_MAT3X2:
		case glu::TYPE_FLOAT_MAT3:
		case glu::TYPE_FLOAT_MAT3X4:
		case glu::TYPE_FLOAT_MAT4X2:
		case glu::TYPE_FLOAT_MAT4X3:
		case glu::TYPE_FLOAT_MAT4:
			compareFuncs.insert(glu::TYPE_FLOAT);
			compareFuncs.insert(glu::getDataTypeFloatVec(glu::getDataTypeMatrixNumRows(basicType)));
			compareFuncs.insert(basicType);
			break;

		default:
			compareFuncs.insert(basicType);
			break;
	}
}

void collectUniqueBasicTypes (std::set<glu::DataType>& basicTypes, const VarType& type)
{
	if (type.isStructType())
	{
		for (StructType::ConstIterator iter = type.getStruct().begin(); iter != type.getStruct().end(); ++iter)
			collectUniqueBasicTypes(basicTypes, iter->getType());
	}
	else if (type.isArrayType())
		collectUniqueBasicTypes(basicTypes, type.getElementType());
	else
	{
		DE_ASSERT(type.isBasicType());
		basicTypes.insert(type.getBasicType());
	}
}

void collectUniqueBasicTypes (std::set<glu::DataType>& basicTypes, const UniformBlock& uniformBlock)
{
	for (UniformBlock::ConstIterator iter = uniformBlock.begin(); iter != uniformBlock.end(); ++iter)
		collectUniqueBasicTypes(basicTypes, iter->getType());
}

void collectUniqueBasicTypes (std::set<glu::DataType>& basicTypes, const ShaderInterface& interface)
{
	for (int ndx = 0; ndx < interface.getNumUniformBlocks(); ++ndx)
		collectUniqueBasicTypes(basicTypes, interface.getUniformBlock(ndx));
}

void generateCompareFuncs (std::ostream& str, const ShaderInterface& interface)
{
	std::set<glu::DataType> types;
	std::set<glu::DataType> compareFuncs;

	// Collect unique basic types
	collectUniqueBasicTypes(types, interface);

	// Set of compare functions required
	for (std::set<glu::DataType>::const_iterator iter = types.begin(); iter != types.end(); ++iter)
	{
		getCompareDependencies(compareFuncs, *iter);
	}

	for (int type = 0; type < glu::TYPE_LAST; ++type)
	{
		if (compareFuncs.find(glu::DataType(type)) != compareFuncs.end())
			str << getCompareFuncForType(glu::DataType(type));
	}
}

struct Indent
{
	int level;
	Indent (int level_) : level(level_) {}
};

std::ostream& operator<< (std::ostream& str, const Indent& indent)
{
	for (int i = 0; i < indent.level; i++)
		str << "\t";
	return str;
}

void		generateDeclaration			(std::ostringstream& src, const VarType& type, const std::string& name, int indentLevel, deUint32 unusedHints, deUint32 flagsMask, deUint32 offset);
void		generateDeclaration			(std::ostringstream& src, const Uniform& uniform, int indentLevel, deUint32 offset);
void		generateDeclaration			(std::ostringstream& src, const StructType& structType, int indentLevel);

void		generateLocalDeclaration	(std::ostringstream& src, const StructType& structType, int indentLevel);
void		generateFullDeclaration		(std::ostringstream& src, const StructType& structType, int indentLevel);

void generateDeclaration (std::ostringstream& src, const StructType& structType, int indentLevel)
{
	DE_ASSERT(structType.hasTypeName());
	generateFullDeclaration(src, structType, indentLevel);
	src << ";\n";
}

void generateFullDeclaration (std::ostringstream& src, const StructType& structType, int indentLevel)
{
	src << "struct";
	if (structType.hasTypeName())
		src << " " << structType.getTypeName();
	src << "\n" << Indent(indentLevel) << "{\n";

	for (StructType::ConstIterator memberIter = structType.begin(); memberIter != structType.end(); memberIter++)
	{
		src << Indent(indentLevel + 1);
		generateDeclaration(src, memberIter->getType(), memberIter->getName(), indentLevel + 1, memberIter->getFlags() & UNUSED_BOTH, ~LAYOUT_OFFSET, 0u);
	}

	src << Indent(indentLevel) << "}";
}

void generateLocalDeclaration (std::ostringstream& src, const StructType& structType, int /* indentLevel */)
{
	src << structType.getTypeName();
}

void generateLayoutAndPrecisionDeclaration (std::ostringstream& src, deUint32 flags, deUint32 offset)
{
	if ((flags & LAYOUT_MASK) != 0)
		src << "layout(" << LayoutFlagsFmt(flags & LAYOUT_MASK, offset) << ") ";

	if ((flags & PRECISION_MASK) != 0)
		src << PrecisionFlagsFmt(flags & PRECISION_MASK) << " ";
}

void generateDeclaration (std::ostringstream& src, const VarType& type, const std::string& name, int indentLevel, deUint32 unusedHints, deUint32 flagsMask, deUint32 offset)
{
	generateLayoutAndPrecisionDeclaration(src, type.getFlags() & flagsMask, offset);

	if (type.isBasicType())
		src << glu::getDataTypeName(type.getBasicType()) << " " << name;
	else if (type.isArrayType())
	{
		std::vector<int>	arraySizes;
		const VarType*		curType		= &type;
		while (curType->isArrayType())
		{
			arraySizes.push_back(curType->getArraySize());
			curType = &curType->getElementType();
		}

		generateLayoutAndPrecisionDeclaration(src, curType->getFlags() & flagsMask, offset);

		if (curType->isBasicType())
			src << glu::getDataTypeName(curType->getBasicType());
		else
		{
			DE_ASSERT(curType->isStructType());
			generateLocalDeclaration(src, curType->getStruct(), indentLevel+1);
		}

		src << " " << name;

		for (std::vector<int>::const_iterator sizeIter = arraySizes.begin(); sizeIter != arraySizes.end(); sizeIter++)
			src << "[" << *sizeIter << "]";
	}
	else
	{
		generateLocalDeclaration(src, type.getStruct(), indentLevel+1);
		src << " " << name;
	}

	src << ";";

	// Print out unused hints.
	if (unusedHints != 0)
		src << " // unused in " << (unusedHints == UNUSED_BOTH		? "both shaders"	:
									unusedHints == UNUSED_VERTEX	? "vertex shader"	:
									unusedHints == UNUSED_FRAGMENT	? "fragment shader" : "???");

	src << "\n";
}

void generateDeclaration (std::ostringstream& src, const Uniform& uniform, int indentLevel, deUint32 offset)
{
	if ((uniform.getFlags() & LAYOUT_MASK) != 0)
		src << "layout(" << LayoutFlagsFmt(uniform.getFlags() & LAYOUT_MASK) << ") ";

	generateDeclaration(src, uniform.getType(), uniform.getName(), indentLevel, uniform.getFlags() & UNUSED_BOTH, ~0u, offset);
}

deUint32 getBlockMemberOffset (int blockNdx, const UniformBlock& block, const Uniform& uniform, const UniformLayout& layout)
{
	std::ostringstream	name;
	const VarType*		curType = &uniform.getType();

	if (block.getInstanceName().length() != 0)
		name << block.getBlockName() << ".";	// \note UniformLayoutEntry uses block name rather than instance name

	name << uniform.getName();

	while (!curType->isBasicType())
	{
		if (curType->isArrayType())
		{
			name << "[0]";
			curType = &curType->getElementType();
		}

		if (curType->isStructType())
		{
			const StructType::ConstIterator firstMember = curType->getStruct().begin();
			name << "." << firstMember->getName();
			curType = &firstMember->getType();
		}
	}

	const int uniformNdx = layout.getUniformLayoutIndex(blockNdx, name.str());
	DE_ASSERT(uniformNdx >= 0);

	return layout.uniforms[uniformNdx].offset;
}

template<typename T>
void semiShuffle (std::vector<T>& v)
{
	const std::vector<T>	src	= v;
	int						i	= -1;
	int						n	= static_cast<int>(src.size());

	v.clear();

	while (n)
	{
		i += n;
		v.push_back(src[i]);
		n = (n > 0 ? 1 - n : -1 - n);
	}
}

template<typename T>
//! \note Stores pointers to original elements
class Traverser
{
public:
	template<typename Iter>
	Traverser (const Iter beg, const Iter end, const bool shuffled)
	{
		for (Iter it = beg; it != end; ++it)
			m_elements.push_back(&(*it));

		if (shuffled)
			semiShuffle(m_elements);

		m_next = m_elements.begin();
	}

	T* next (void)
	{
		if (m_next != m_elements.end())
			return *m_next++;
		else
			return DE_NULL;
	}

private:
	typename std::vector<T*>					m_elements;
	typename std::vector<T*>::const_iterator	m_next;
};

void generateDeclaration (std::ostringstream& src, int blockNdx, const UniformBlock& block, const UniformLayout& layout, bool shuffleUniformMembers)
{
	src << "layout(set = 0, binding = " << blockNdx;
	if ((block.getFlags() & LAYOUT_MASK) != 0)
		src << ", " << LayoutFlagsFmt(block.getFlags() & LAYOUT_MASK);
	src << ") ";

	src << "uniform " << block.getBlockName();
	src << "\n{\n";

	Traverser<const Uniform> uniforms(block.begin(), block.end(), shuffleUniformMembers);

	while (const Uniform* pUniform = uniforms.next())
	{
		src << Indent(1);
		generateDeclaration(src, *pUniform, 1 /* indent level */, getBlockMemberOffset(blockNdx, block, *pUniform, layout));
	}

	src << "}";

	if (block.hasInstanceName())
	{
		src << " " << block.getInstanceName();
		if (block.isArray())
			src << "[" << block.getArraySize() << "]";
	}
	else
		DE_ASSERT(!block.isArray());

	src << ";\n";
}

void generateValueSrc (std::ostringstream& src, const UniformLayoutEntry& entry, const void* basePtr, int elementNdx)
{
	glu::DataType	scalarType		= glu::getDataTypeScalarType(entry.type);
	int				scalarSize		= glu::getDataTypeScalarSize(entry.type);
	bool			isArray			= entry.size > 1;
	const deUint8*	elemPtr			= (const deUint8*)basePtr + entry.offset + (isArray ? elementNdx * entry.arrayStride : 0);
	const int		compSize		= sizeof(deUint32);

	if (scalarSize > 1)
		src << glu::getDataTypeName(entry.type) << "(";

	if (glu::isDataTypeMatrix(entry.type))
	{
		int	numRows	= glu::getDataTypeMatrixNumRows(entry.type);
		int	numCols	= glu::getDataTypeMatrixNumColumns(entry.type);

		DE_ASSERT(scalarType == glu::TYPE_FLOAT);

		// Constructed in column-wise order.
		for (int colNdx = 0; colNdx < numCols; colNdx++)
		{
			for (int rowNdx = 0; rowNdx < numRows; rowNdx++)
			{
				const deUint8*	compPtr	= elemPtr + (entry.isRowMajor ? (rowNdx * entry.matrixStride + colNdx * compSize)
																	  : (colNdx * entry.matrixStride + rowNdx * compSize));

				if (colNdx > 0 || rowNdx > 0)
					src << ", ";

				src << de::floatToString(*((const float*)compPtr), 1);
			}
		}
	}
	else
	{
		for (int scalarNdx = 0; scalarNdx < scalarSize; scalarNdx++)
		{
			const deUint8* compPtr = elemPtr + scalarNdx * compSize;

			if (scalarNdx > 0)
				src << ", ";

			switch (scalarType)
			{
				case glu::TYPE_FLOAT:	src << de::floatToString(*((const float*)compPtr), 1);			break;
				case glu::TYPE_INT:		src << *((const int*)compPtr);									break;
				case glu::TYPE_UINT:	src << *((const deUint32*)compPtr) << "u";						break;
				case glu::TYPE_BOOL:	src << (*((const deUint32*)compPtr) != 0u ? "true" : "false");	break;
				default:
					DE_ASSERT(false);
			}
		}
	}

	if (scalarSize > 1)
		src << ")";
}

bool isMatrix (glu::DataType elementType)
{
	return (elementType >= glu::TYPE_FLOAT_MAT2) && (elementType <= glu::TYPE_FLOAT_MAT4);
}

void writeMatrixTypeSrc (int						columnCount,
						 int						rowCount,
						 std::string				compare,
						 std::string				compareType,
						 std::ostringstream&		src,
						 const std::string&			srcName,
						 const void*				basePtr,
						 const UniformLayoutEntry&	entry,
						 bool						vector)
{
	if (vector)	// generateTestSrcMatrixPerVec
	{
		for (int colNdex = 0; colNdex < columnCount; colNdex++)
		{
			src << "\tresult *= " << compare + compareType << "(" << srcName << "[" << colNdex << "], ";

			if (glu::isDataTypeMatrix(entry.type))
			{
				int	scalarSize = glu::getDataTypeScalarSize(entry.type);
				const deUint8*	elemPtr			= (const deUint8*)basePtr + entry.offset;
				const int		compSize		= sizeof(deUint32);

				if (scalarSize > 1)
					src << compareType << "(";
				for (int rowNdex = 0; rowNdex < rowCount; rowNdex++)
				{
					const deUint8*	compPtr	= elemPtr + (entry.isRowMajor ? (rowNdex * entry.matrixStride + colNdex * compSize)
																		  : (colNdex * entry.matrixStride + rowNdex * compSize));
					src << de::floatToString(*((const float*)compPtr), 1);

					if (rowNdex < rowCount-1)
						src << ", ";
				}
				src << "));\n";
			}
			else
			{
				generateValueSrc(src, entry, basePtr, 0);
				src << "[" << colNdex << "]);\n";
			}
		}
	}
	else		// generateTestSrcMatrixPerElement
	{
		for (int colNdex = 0; colNdex < columnCount; colNdex++)
		{
			for (int rowNdex = 0; rowNdex < rowCount; rowNdex++)
			{
				src << "\tresult *= " << compare + compareType << "(" << srcName << "[" << colNdex << "][" << rowNdex << "], ";
				if (glu::isDataTypeMatrix(entry.type))
				{
					const deUint8*	elemPtr			= (const deUint8*)basePtr + entry.offset;
					const int		compSize		= sizeof(deUint32);
					const deUint8*	compPtr	= elemPtr + (entry.isRowMajor ? (rowNdex * entry.matrixStride + colNdex * compSize)
																		  : (colNdex * entry.matrixStride + rowNdex * compSize));

					src << de::floatToString(*((const float*)compPtr), 1) << ");\n";
				}
				else
				{
					generateValueSrc(src, entry, basePtr, 0);
					src << "[" << colNdex << "][" << rowNdex << "]);\n";
				}
			}
		}
	}
}

void generateTestSrcMatrixPerVec (glu::DataType				elementType,
								  std::ostringstream&		src,
								  const std::string&		srcName,
								  const void*				basePtr,
								  const UniformLayoutEntry&	entry,
								  bool						vector)
{
	std::string compare = "compare_";
	switch (elementType)
	{
		case glu::TYPE_FLOAT_MAT2:
			writeMatrixTypeSrc(2, 2, compare, "vec2", src, srcName, basePtr, entry, vector);
			break;

		case glu::TYPE_FLOAT_MAT2X3:
			writeMatrixTypeSrc(2, 3, compare, "vec3", src, srcName, basePtr, entry, vector);
			break;

		case glu::TYPE_FLOAT_MAT2X4:
			writeMatrixTypeSrc(2, 4, compare, "vec4", src, srcName, basePtr, entry, vector);
			break;

		case glu::TYPE_FLOAT_MAT3X4:
			writeMatrixTypeSrc(3, 4, compare, "vec4", src, srcName, basePtr, entry, vector);
			break;

		case glu::TYPE_FLOAT_MAT4:
			writeMatrixTypeSrc(4, 4, compare, "vec4", src, srcName, basePtr, entry, vector);
			break;

		case glu::TYPE_FLOAT_MAT4X2:
			writeMatrixTypeSrc(4, 2, compare, "vec2", src, srcName, basePtr, entry, vector);
			break;

		case glu::TYPE_FLOAT_MAT4X3:
			writeMatrixTypeSrc(4, 3, compare, "vec3", src, srcName, basePtr, entry, vector);
			break;

		default:
			break;
	}
}

void generateTestSrcMatrixPerElement (glu::DataType				elementType,
									  std::ostringstream&		src,
									  const std::string&		srcName,
									  const void*				basePtr,
									  const UniformLayoutEntry&	entry,
									  bool						vector)
{
	std::string compare = "compare_";
	std::string compareType = "float";
	switch (elementType)
	{
		case glu::TYPE_FLOAT_MAT2:
			writeMatrixTypeSrc(2, 2, compare, compareType, src, srcName, basePtr, entry, vector);
			break;

		case glu::TYPE_FLOAT_MAT2X3:
			writeMatrixTypeSrc(2, 3, compare, compareType, src, srcName, basePtr, entry, vector);
			break;

		case glu::TYPE_FLOAT_MAT2X4:
			writeMatrixTypeSrc(2, 4, compare, compareType, src, srcName, basePtr, entry, vector);
			break;

		case glu::TYPE_FLOAT_MAT3X4:
			writeMatrixTypeSrc(3, 4, compare, compareType, src, srcName, basePtr, entry, vector);
			break;

		case glu::TYPE_FLOAT_MAT4:
			writeMatrixTypeSrc(4, 4, compare, compareType, src, srcName, basePtr, entry, vector);
			break;

		case glu::TYPE_FLOAT_MAT4X2:
			writeMatrixTypeSrc(4, 2, compare, compareType, src, srcName, basePtr, entry, vector);
			break;

		case glu::TYPE_FLOAT_MAT4X3:
			writeMatrixTypeSrc(4, 3, compare, compareType, src, srcName, basePtr, entry, vector);
			break;

		default:
			break;
	}
}

void generateSingleCompare (std::ostringstream&			src,
							glu::DataType				elementType,
							const std::string&			srcName,
							const void*					basePtr,
							const UniformLayoutEntry&	entry,
							MatrixLoadFlags				matrixLoadFlag)
{
	if (matrixLoadFlag == LOAD_FULL_MATRIX)
	{
		const char* typeName = glu::getDataTypeName(elementType);

		src << "\tresult *= compare_" << typeName << "(" << srcName << ", ";
		generateValueSrc(src, entry, basePtr, 0);
		src << ");\n";
	}
	else
	{
		if (isMatrix(elementType))
		{
			generateTestSrcMatrixPerVec		(elementType, src, srcName, basePtr, entry, true);
			generateTestSrcMatrixPerElement	(elementType, src, srcName, basePtr, entry, false);
		}
	}
}

void generateCompareSrc (std::ostringstream&	src,
						 const char*			resultVar,
						 const VarType&			type,
						 const std::string&		srcName,
						 const std::string&		apiName,
						 const UniformLayout&	layout,
						 int					blockNdx,
						 const void*			basePtr,
						 deUint32				unusedMask,
						 MatrixLoadFlags		matrixLoadFlag)
{
	if (type.isBasicType() || (type.isArrayType() && type.getElementType().isBasicType()))
	{
		// Basic type or array of basic types.
		bool						isArray			= type.isArrayType();
		glu::DataType				elementType		= isArray ? type.getElementType().getBasicType() : type.getBasicType();
		const char*					typeName		= glu::getDataTypeName(elementType);
		std::string					fullApiName		= std::string(apiName) + (isArray ? "[0]" : ""); // Arrays are always postfixed with [0]
		int							uniformNdx		= layout.getUniformLayoutIndex(blockNdx, fullApiName);
		const UniformLayoutEntry&	entry			= layout.uniforms[uniformNdx];

		if (isArray)
		{
			for (int elemNdx = 0; elemNdx < type.getArraySize(); elemNdx++)
			{
				src << "\tresult *= compare_" << typeName << "(" << srcName << "[" << elemNdx << "], ";
				generateValueSrc(src, entry, basePtr, elemNdx);
				src << ");\n";
			}
		}
		else
		{
			generateSingleCompare(src, elementType, srcName, basePtr, entry, matrixLoadFlag);
		}
	}
	else if (type.isArrayType())
	{
		const VarType& elementType = type.getElementType();

		for (int elementNdx = 0; elementNdx < type.getArraySize(); elementNdx++)
		{
			std::string op = std::string("[") + de::toString(elementNdx) + "]";
			std::string elementSrcName = std::string(srcName) + op;
			std::string elementApiName = std::string(apiName) + op;
			generateCompareSrc(src, resultVar, elementType, elementSrcName, elementApiName, layout, blockNdx, basePtr, unusedMask, LOAD_FULL_MATRIX);
		}
	}
	else
	{
		DE_ASSERT(type.isStructType());

		for (StructType::ConstIterator memberIter = type.getStruct().begin(); memberIter != type.getStruct().end(); memberIter++)
		{
			if (memberIter->getFlags() & unusedMask)
				continue; // Skip member.

			std::string op = std::string(".") + memberIter->getName();
			std::string memberSrcName = std::string(srcName) + op;
			std::string memberApiName = std::string(apiName) + op;
			generateCompareSrc(src, resultVar, memberIter->getType(), memberSrcName, memberApiName, layout, blockNdx, basePtr, unusedMask, LOAD_FULL_MATRIX);
		}
	}
}

void generateCompareSrc (std::ostringstream& src,
						 const char* resultVar,
						 const ShaderInterface& interface,
						 const UniformLayout& layout,
						 const std::map<int,
						 void*>& blockPointers,
						 bool isVertex,
						 MatrixLoadFlags matrixLoadFlag)
{
	deUint32 unusedMask = isVertex ? UNUSED_VERTEX : UNUSED_FRAGMENT;

	for (int blockNdx = 0; blockNdx < interface.getNumUniformBlocks(); blockNdx++)
	{
		const UniformBlock& block = interface.getUniformBlock(blockNdx);

		if ((block.getFlags() & (isVertex ? DECLARE_VERTEX : DECLARE_FRAGMENT)) == 0)
			continue; // Skip.

		bool			hasInstanceName	= block.hasInstanceName();
		bool			isArray			= block.isArray();
		int				numInstances	= isArray ? block.getArraySize() : 1;
		std::string		apiPrefix		= hasInstanceName ? block.getBlockName() + "." : std::string("");

		DE_ASSERT(!isArray || hasInstanceName);

		for (int instanceNdx = 0; instanceNdx < numInstances; instanceNdx++)
		{
			std::string		instancePostfix		= isArray ? std::string("[") + de::toString(instanceNdx) + "]" : std::string("");
			std::string		blockInstanceName	= block.getBlockName() + instancePostfix;
			std::string		srcPrefix			= hasInstanceName ? block.getInstanceName() + instancePostfix + "." : std::string("");
			int				blockLayoutNdx		= layout.getBlockLayoutIndex(blockNdx, instanceNdx);
			void*			basePtr				= blockPointers.find(blockLayoutNdx)->second;

			for (UniformBlock::ConstIterator uniformIter = block.begin(); uniformIter != block.end(); uniformIter++)
			{
				const Uniform& uniform = *uniformIter;

				if (uniform.getFlags() & unusedMask)
					continue; // Don't read from that uniform.

				std::string srcName = srcPrefix + uniform.getName();
				std::string apiName = apiPrefix + uniform.getName();
				generateCompareSrc(src, resultVar, uniform.getType(), srcName, apiName, layout, blockNdx, basePtr, unusedMask, matrixLoadFlag);
			}
		}
	}
}

std::string generateVertexShader (const ShaderInterface& interface, const UniformLayout& layout, const std::map<int, void*>& blockPointers, MatrixLoadFlags matrixLoadFlag, bool shuffleUniformMembers)
{
	std::ostringstream src;
	src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n";

	src << "layout(location = 0) in highp vec4 a_position;\n";
	src << "layout(location = 0) out mediump float v_vtxResult;\n";
	src << "\n";

	std::vector<const StructType*> namedStructs;
	interface.getNamedStructs(namedStructs);
	for (std::vector<const StructType*>::const_iterator structIter = namedStructs.begin(); structIter != namedStructs.end(); structIter++)
		generateDeclaration(src, **structIter, 0);

	for (int blockNdx = 0; blockNdx < interface.getNumUniformBlocks(); blockNdx++)
	{
		const UniformBlock& block = interface.getUniformBlock(blockNdx);
		if (block.getFlags() & DECLARE_VERTEX)
			generateDeclaration(src, blockNdx, block, layout, shuffleUniformMembers);
	}

	// Comparison utilities.
	src << "\n";
	generateCompareFuncs(src, interface);

	src << "\n"
		   "void main (void)\n"
		   "{\n"
		   "	gl_Position = a_position;\n"
		   "	mediump float result = 1.0;\n";

	// Value compare.
	generateCompareSrc(src, "result", interface, layout, blockPointers, true, matrixLoadFlag);

	src << "	v_vtxResult = result;\n"
		   "}\n";

	return src.str();
}

std::string generateFragmentShader (const ShaderInterface& interface, const UniformLayout& layout, const std::map<int, void*>& blockPointers, MatrixLoadFlags matrixLoadFlag, bool shuffleUniformMembers)
{
	std::ostringstream src;
	src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n";

	src << "layout(location = 0) in mediump float v_vtxResult;\n";
	src << "layout(location = 0) out mediump vec4 dEQP_FragColor;\n";
	src << "\n";

	std::vector<const StructType*> namedStructs;
	interface.getNamedStructs(namedStructs);
	for (std::vector<const StructType*>::const_iterator structIter = namedStructs.begin(); structIter != namedStructs.end(); structIter++)
		generateDeclaration(src, **structIter, 0);

	for (int blockNdx = 0; blockNdx < interface.getNumUniformBlocks(); blockNdx++)
	{
		const UniformBlock& block = interface.getUniformBlock(blockNdx);
		if (block.getFlags() & DECLARE_FRAGMENT)
			generateDeclaration(src, blockNdx, block, layout, shuffleUniformMembers);
	}

	// Comparison utilities.
	src << "\n";
	generateCompareFuncs(src, interface);

	src << "\n"
		   "void main (void)\n"
		   "{\n"
		   "	mediump float result = 1.0;\n";

	// Value compare.
	generateCompareSrc(src, "result", interface, layout, blockPointers, false, matrixLoadFlag);

	src << "	dEQP_FragColor = vec4(1.0, v_vtxResult, result, 1.0);\n"
		   "}\n";

	return src.str();
}

Move<VkBuffer> createBuffer (Context& context, VkDeviceSize bufferSize, vk::VkBufferUsageFlags usageFlags)
{
	const VkDevice				vkDevice			= context.getDevice();
	const DeviceInterface&		vk					= context.getDeviceInterface();
	const deUint32				queueFamilyIndex	= context.getUniversalQueueFamilyIndex();

	const VkBufferCreateInfo	bufferInfo			=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType		sType;
		DE_NULL,								// const void*			pNext;
		0u,										// VkBufferCreateFlags	flags;
		bufferSize,								// VkDeviceSize			size;
		usageFlags,								// VkBufferUsageFlags	usage;
		VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode		sharingMode;
		1u,										// deUint32				queueFamilyIndexCount;
		&queueFamilyIndex						// const deUint32*		pQueueFamilyIndices;
	};

	return vk::createBuffer(vk, vkDevice, &bufferInfo);
}

Move<vk::VkImage> createImage2D (Context& context, deUint32 width, deUint32 height, vk::VkFormat format, vk::VkImageTiling tiling, vk::VkImageUsageFlags usageFlags)
{
	const deUint32				queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	const vk::VkImageCreateInfo	params				=
	{
		vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,	// VkStructureType			sType
		DE_NULL,									// const void*				pNext
		0u,											// VkImageCreateFlags		flags
		vk::VK_IMAGE_TYPE_2D,						// VkImageType				imageType
		format,										// VkFormat					format
		{ width, height, 1u },						// VkExtent3D				extent
		1u,											// deUint32					mipLevels
		1u,											// deUint32					arrayLayers
		VK_SAMPLE_COUNT_1_BIT,						// VkSampleCountFlagBits	samples
		tiling,										// VkImageTiling			tiling
		usageFlags,									// VkImageUsageFlags		usage
		vk::VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode			sharingMode
		1u,											// deUint32					queueFamilyIndexCount
		&queueFamilyIndex,							// const deUint32*			pQueueFamilyIndices
		vk::VK_IMAGE_LAYOUT_UNDEFINED,				// VkImageLayout			initialLayout
	};

	return vk::createImage(context.getDeviceInterface(), context.getDevice(), &params);
}

de::MovePtr<vk::Allocation> allocateAndBindMemory (Context& context, vk::VkBuffer buffer, vk::MemoryRequirement memReqs)
{
	const vk::DeviceInterface&		vkd		= context.getDeviceInterface();
	const vk::VkMemoryRequirements	bufReqs	= vk::getBufferMemoryRequirements(vkd, context.getDevice(), buffer);
	de::MovePtr<vk::Allocation>		memory	= context.getDefaultAllocator().allocate(bufReqs, memReqs);

	vkd.bindBufferMemory(context.getDevice(), buffer, memory->getMemory(), memory->getOffset());

	return memory;
}

de::MovePtr<vk::Allocation> allocateAndBindMemory (Context& context, vk::VkImage image, vk::MemoryRequirement memReqs)
{
	const vk::DeviceInterface&	  vkd	 = context.getDeviceInterface();
	const vk::VkMemoryRequirements  imgReqs = vk::getImageMemoryRequirements(vkd, context.getDevice(), image);
	de::MovePtr<vk::Allocation>		 memory  = context.getDefaultAllocator().allocate(imgReqs, memReqs);

	vkd.bindImageMemory(context.getDevice(), image, memory->getMemory(), memory->getOffset());

	return memory;
}

Move<vk::VkImageView> createAttachmentView (Context& context, vk::VkImage image, vk::VkFormat format)
{
	const vk::VkImageViewCreateInfo params =
	{
		vk::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// sType
		DE_NULL,											// pNext
		0u,													// flags
		image,												// image
		vk::VK_IMAGE_VIEW_TYPE_2D,							// viewType
		format,												// format
		vk::makeComponentMappingRGBA(),						// components
		{ vk::VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u,1u },	// subresourceRange
	};

	return vk::createImageView(context.getDeviceInterface(), context.getDevice(), &params);
}

Move<vk::VkPipelineLayout> createPipelineLayout (Context& context, vk::VkDescriptorSetLayout descriptorSetLayout)
{
	const vk::VkPipelineLayoutCreateInfo params =
	{
		vk::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,	// sType
		DE_NULL,											// pNext
		0u,													// flags
		1u,													// setLayoutCount
		&descriptorSetLayout,								// pSetLayouts
		0u,													// pushConstantRangeCount
		DE_NULL,											// pPushConstantRanges
	};

	return vk::createPipelineLayout(context.getDeviceInterface(), context.getDevice(), &params);
}

Move<vk::VkCommandPool> createCmdPool (Context& context)
{
	const deUint32	queueFamilyIndex	= context.getUniversalQueueFamilyIndex();

	return vk::createCommandPool(context.getDeviceInterface(), context.getDevice(), vk::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex);
}

Move<vk::VkCommandBuffer> createCmdBuffer (Context& context, vk::VkCommandPool cmdPool)
{
	return vk::allocateCommandBuffer(context.getDeviceInterface(), context.getDevice(), cmdPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

// UniformBlockCaseInstance

class UniformBlockCaseInstance : public vkt::TestInstance
{
public:
									UniformBlockCaseInstance	(Context&						context,
																 UniformBlockCase::BufferMode	bufferMode,
																 const UniformLayout&			layout,
																 const std::map<int, void*>&	blockPointers);
	virtual							~UniformBlockCaseInstance	(void);
	virtual tcu::TestStatus			iterate						(void);

private:
	enum
	{
		RENDER_WIDTH = 100,
		RENDER_HEIGHT = 100,
	};

	vk::Move<VkRenderPass>			createRenderPass			(vk::VkFormat format) const;
	vk::Move<VkFramebuffer>			createFramebuffer			(vk::VkRenderPass renderPass, vk::VkImageView colorImageView) const;
	vk::Move<VkDescriptorSetLayout>	createDescriptorSetLayout	(void) const;
	vk::Move<VkDescriptorPool>		createDescriptorPool		(void) const;
	vk::Move<VkPipeline>			createPipeline				(vk::VkShaderModule vtxShaderModule, vk::VkShaderModule fragShaderModule, vk::VkPipelineLayout pipelineLayout, vk::VkRenderPass renderPass) const;

	vk::VkDescriptorBufferInfo		addUniformData				(deUint32 size, const void* dataPtr);

	UniformBlockCase::BufferMode	m_bufferMode;
	const UniformLayout&			m_layout;
	const std::map<int, void*>&		m_blockPointers;

	typedef de::SharedPtr<vk::Unique<vk::VkBuffer> >	VkBufferSp;
	typedef de::SharedPtr<vk::Allocation>				AllocationSp;

	std::vector<VkBufferSp>			m_uniformBuffers;
	std::vector<AllocationSp>		m_uniformAllocs;
};

UniformBlockCaseInstance::UniformBlockCaseInstance (Context&						ctx,
													UniformBlockCase::BufferMode	bufferMode,
													const UniformLayout&			layout,
													const std::map<int, void*>&		blockPointers)
	: vkt::TestInstance (ctx)
	, m_bufferMode		(bufferMode)
	, m_layout			(layout)
	, m_blockPointers	(blockPointers)
{
}

UniformBlockCaseInstance::~UniformBlockCaseInstance (void)
{
}

tcu::TestStatus UniformBlockCaseInstance::iterate (void)
{
	const vk::DeviceInterface&		vk					= m_context.getDeviceInterface();
	const vk::VkDevice				device				= m_context.getDevice();
	const vk::VkQueue				queue				= m_context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();

	const float positions[] =
	{
		-1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f, +1.0f, 0.0f, 1.0f,
		+1.0f, -1.0f, 0.0f, 1.0f,
		+1.0f, +1.0f, 0.0f, 1.0f
	};

	const deUint32 indices[] = { 0, 1, 2, 2, 1, 3 };

	vk::Unique<VkBuffer>				positionsBuffer		(createBuffer(m_context, sizeof(positions), vk::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
	de::UniquePtr<Allocation>			positionsAlloc		(allocateAndBindMemory(m_context, *positionsBuffer, MemoryRequirement::HostVisible));
	vk::Unique<VkBuffer>				indicesBuffer		(createBuffer(m_context, sizeof(indices), vk::VK_BUFFER_USAGE_INDEX_BUFFER_BIT|vk::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
	de::UniquePtr<Allocation>			indicesAlloc		(allocateAndBindMemory(m_context, *indicesBuffer, MemoryRequirement::HostVisible));

	int minUniformBufferOffsetAlignment = getminUniformBufferOffsetAlignment(m_context);

	// Upload attrbiutes data
	{
		deMemcpy(positionsAlloc->getHostPtr(), positions, sizeof(positions));
		flushMappedMemoryRange(vk, device, positionsAlloc->getMemory(), positionsAlloc->getOffset(), sizeof(positions));

		deMemcpy(indicesAlloc->getHostPtr(), indices, sizeof(indices));
		flushMappedMemoryRange(vk, device, indicesAlloc->getMemory(), indicesAlloc->getOffset(), sizeof(indices));
	}

	vk::Unique<VkImage>					colorImage			(createImage2D(m_context,
																			RENDER_WIDTH,
																			RENDER_HEIGHT,
																			vk::VK_FORMAT_R8G8B8A8_UNORM,
																			vk::VK_IMAGE_TILING_OPTIMAL,
																			vk::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
	de::UniquePtr<Allocation>			colorImageAlloc		(allocateAndBindMemory(m_context, *colorImage, MemoryRequirement::Any));
	vk::Unique<VkImageView>				colorImageView		(createAttachmentView(m_context, *colorImage, vk::VK_FORMAT_R8G8B8A8_UNORM));

	vk::Unique<VkDescriptorSetLayout>	descriptorSetLayout	(createDescriptorSetLayout());
	vk::Unique<VkDescriptorPool>		descriptorPool		(createDescriptorPool());

	const VkDescriptorSetAllocateInfo	descriptorSetAllocateInfo =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,		// VkStructureType				sType;
		DE_NULL,											// const void*					pNext;
		*descriptorPool,									// VkDescriptorPool				descriptorPool;
		1u,													// deUint32						setLayoutCount;
		&descriptorSetLayout.get()							// const VkDescriptorSetLayout*	pSetLayouts;
	};

	vk::Unique<VkDescriptorSet>			descriptorSet(vk::allocateDescriptorSet(vk, device, &descriptorSetAllocateInfo));
	int									numBlocks = (int)m_layout.blocks.size();
	std::vector<vk::VkDescriptorBufferInfo>	descriptors(numBlocks);

	// Upload uniform data
	{
		vk::DescriptorSetUpdateBuilder	descriptorSetUpdateBuilder;

		if (m_bufferMode == UniformBlockCase::BUFFERMODE_PER_BLOCK)
		{
			for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
			{
				const BlockLayoutEntry& block = m_layout.blocks[blockNdx];
				const void*	srcPtr = m_blockPointers.find(blockNdx)->second;

				descriptors[blockNdx] = addUniformData(block.size, srcPtr);
				descriptorSetUpdateBuilder.writeSingle(*descriptorSet, vk::DescriptorSetUpdateBuilder::Location::bindingArrayElement(block.bindingNdx, block.instanceNdx),
														VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &descriptors[blockNdx]);
			}
		}
		else
		{
			int currentOffset = 0;
			std::map<int, int> offsets;
			for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
			{
				if (minUniformBufferOffsetAlignment > 0)
					currentOffset = deAlign32(currentOffset, minUniformBufferOffsetAlignment);
				offsets[blockNdx] = currentOffset;
				currentOffset += m_layout.blocks[blockNdx].size;
			}

			deUint32 totalSize = currentOffset;

			// Make a copy of the data that satisfies the device's min uniform buffer alignment
			std::vector<deUint8> data;
			data.resize(totalSize);
			for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
			{
				deMemcpy(&data[offsets[blockNdx]], m_blockPointers.find(blockNdx)->second, m_layout.blocks[blockNdx].size);
			}

			vk::VkBuffer buffer = addUniformData(totalSize, &data[0]).buffer;

			for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
			{
				const BlockLayoutEntry& block = m_layout.blocks[blockNdx];
				deUint32 size = block.size;

				const VkDescriptorBufferInfo	descriptor =
				{
					buffer,							// VkBuffer		buffer;
					(deUint32)offsets[blockNdx],	// VkDeviceSize	offset;
					size,							// VkDeviceSize	range;
				};

				descriptors[blockNdx] = descriptor;
				descriptorSetUpdateBuilder.writeSingle(*descriptorSet,
														vk::DescriptorSetUpdateBuilder::Location::bindingArrayElement(block.bindingNdx, block.instanceNdx),
														VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
														&descriptors[blockNdx]);
			}
		}

		descriptorSetUpdateBuilder.update(vk, device);
	}

	vk::Unique<VkRenderPass>			renderPass			(createRenderPass(vk::VK_FORMAT_R8G8B8A8_UNORM));
	vk::Unique<VkFramebuffer>			framebuffer			(createFramebuffer(*renderPass, *colorImageView));
	vk::Unique<VkPipelineLayout>		pipelineLayout		(createPipelineLayout(m_context, *descriptorSetLayout));

	vk::Unique<VkShaderModule>			vtxShaderModule		(vk::createShaderModule(vk, device, m_context.getBinaryCollection().get("vert"), 0));
	vk::Unique<VkShaderModule>			fragShaderModule	(vk::createShaderModule(vk, device, m_context.getBinaryCollection().get("frag"), 0));
	vk::Unique<VkPipeline>				pipeline			(createPipeline(*vtxShaderModule, *fragShaderModule, *pipelineLayout, *renderPass));
	vk::Unique<VkCommandPool>			cmdPool				(createCmdPool(m_context));
	vk::Unique<VkCommandBuffer>			cmdBuffer			(createCmdBuffer(m_context, *cmdPool));
	vk::Unique<VkBuffer>				readImageBuffer		(createBuffer(m_context, (vk::VkDeviceSize)(RENDER_WIDTH * RENDER_HEIGHT * 4), vk::VK_BUFFER_USAGE_TRANSFER_DST_BIT));
	de::UniquePtr<Allocation>			readImageAlloc		(allocateAndBindMemory(m_context, *readImageBuffer, vk::MemoryRequirement::HostVisible));

	// Record command buffer
	const vk::VkCommandBufferBeginInfo beginInfo	=
	{
		vk::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType					sType;
		DE_NULL,											// const void*						pNext;
		0u,													// VkCommandBufferUsageFlags		flags;
		(const vk::VkCommandBufferInheritanceInfo*)DE_NULL,
	};
	VK_CHECK(vk.beginCommandBuffer(*cmdBuffer, &beginInfo));

	const vk::VkClearValue clearValue = vk::makeClearValueColorF32(0.125f, 0.25f, 0.75f, 1.0f);
	const vk::VkRenderPassBeginInfo passBeginInfo	=
	{
		vk::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// VkStructureType		sType;
		DE_NULL,										// const void*			pNext;
		*renderPass,									// VkRenderPass			renderPass;
		*framebuffer,									// VkFramebuffer		framebuffer;
		{ { 0, 0 }, { RENDER_WIDTH, RENDER_HEIGHT } },	// VkRect2D				renderArea;
		1u,												// deUint32				clearValueCount;
		&clearValue,									// const VkClearValue*	pClearValues;
	};

	// Add barrier for initializing image state
	{
		const vk::VkImageMemoryBarrier  initializeBarrier =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
			DE_NULL,										// const void*				pNext
			0,												// VVkAccessFlags			srcAccessMask;
			vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,		// VkAccessFlags			dstAccessMask;
			vk::VK_IMAGE_LAYOUT_UNDEFINED,					// VkImageLayout			oldLayout;
			vk::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// VkImageLayout			newLayout;
			queueFamilyIndex,								// deUint32					srcQueueFamilyIndex;
			queueFamilyIndex,								// deUint32					dstQueueFamilyIndex;
			*colorImage,									// VkImage					image;
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,			// VkImageAspectFlags	aspectMask;
				0u,										// deUint32				baseMipLevel;
				1u,										// deUint32				mipLevels;
				0u,										// deUint32				baseArraySlice;
				1u,										// deUint32				arraySize;
			}												// VkImageSubresourceRange	subresourceRange
		};

		vk.cmdPipelineBarrier(*cmdBuffer, vk::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, vk::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, (vk::VkDependencyFlags)0,
			0, (const vk::VkMemoryBarrier*)DE_NULL,
			0, (const vk::VkBufferMemoryBarrier*)DE_NULL,
			1, &initializeBarrier);
	}

	vk.cmdBeginRenderPass(*cmdBuffer, &passBeginInfo, vk::VK_SUBPASS_CONTENTS_INLINE);

	vk.cmdBindPipeline(*cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &*descriptorSet, 0u, DE_NULL);

	const vk::VkDeviceSize offsets[] = { 0u };
	vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &*positionsBuffer, offsets);
	vk.cmdBindIndexBuffer(*cmdBuffer, *indicesBuffer, (vk::VkDeviceSize)0, vk::VK_INDEX_TYPE_UINT32);

	vk.cmdDrawIndexed(*cmdBuffer, DE_LENGTH_OF_ARRAY(indices), 1u, 0u, 0u, 0u);
	vk.cmdEndRenderPass(*cmdBuffer);

	// Add render finish barrier
	{
		const vk::VkImageMemoryBarrier  renderFinishBarrier =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
			DE_NULL,										// const void*				pNext
			vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,		// VVkAccessFlags			srcAccessMask;
			vk::VK_ACCESS_TRANSFER_READ_BIT,				// VkAccessFlags			dstAccessMask;
			vk::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// VkImageLayout			oldLayout;
			vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,		// VkImageLayout			newLayout;
			queueFamilyIndex,								// deUint32					srcQueueFamilyIndex;
			queueFamilyIndex,								// deUint32					dstQueueFamilyIndex;
			*colorImage,									// VkImage					image;
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,			// VkImageAspectFlags	aspectMask;
				0u,										// deUint32				baseMipLevel;
				1u,										// deUint32				mipLevels;
				0u,										// deUint32				baseArraySlice;
				1u,										// deUint32				arraySize;
			}												// VkImageSubresourceRange	subresourceRange
		};

		vk.cmdPipelineBarrier(*cmdBuffer, vk::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0,
							  0, (const vk::VkMemoryBarrier*)DE_NULL,
							  0, (const vk::VkBufferMemoryBarrier*)DE_NULL,
							  1, &renderFinishBarrier);
	}

	// Add Image->Buffer copy command
	{
		const vk::VkBufferImageCopy copyParams =
		{
			(vk::VkDeviceSize)0u,					// VkDeviceSize				bufferOffset;
			(deUint32)RENDER_WIDTH,					// deUint32					bufferRowLength;
			(deUint32)RENDER_HEIGHT,				// deUint32					bufferImageHeight;
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,	// VkImageAspect	aspect;
				0u,								// deUint32			mipLevel;
				0u,								// deUint32			arrayLayer;
				1u,								// deUint32			arraySize;
			},										// VkImageSubresourceCopy	imageSubresource
			{ 0u, 0u, 0u },							// VkOffset3D				imageOffset;
			{ RENDER_WIDTH, RENDER_HEIGHT, 1u }		// VkExtent3D				imageExtent;
		};

		vk.cmdCopyImageToBuffer(*cmdBuffer, *colorImage, vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *readImageBuffer, 1u, &copyParams);
	}

	// Add copy finish barrier
	{
		const vk::VkBufferMemoryBarrier copyFinishBarrier	=
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,		// VkStructureType		sType;
			DE_NULL,											// const void*			pNext;
			VK_ACCESS_TRANSFER_WRITE_BIT,						// VkAccessFlags		srcAccessMask;
			VK_ACCESS_HOST_READ_BIT,							// VkAccessFlags		dstAccessMask;
			queueFamilyIndex,									// deUint32				srcQueueFamilyIndex;
			queueFamilyIndex,									// deUint32				destQueueFamilyIndex;
			*readImageBuffer,									// VkBuffer				buffer;
			0u,													// VkDeviceSize			offset;
			(vk::VkDeviceSize)(RENDER_WIDTH * RENDER_HEIGHT * 4)// VkDeviceSize			size;
		};

		vk.cmdPipelineBarrier(*cmdBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_HOST_BIT, (vk::VkDependencyFlags)0,
							  0, (const vk::VkMemoryBarrier*)DE_NULL,
							  1, &copyFinishBarrier,
							  0, (const vk::VkImageMemoryBarrier*)DE_NULL);
	}

	VK_CHECK(vk.endCommandBuffer(*cmdBuffer));

	// Submit the command buffer
	{
		const Unique<vk::VkFence> fence(vk::createFence(vk, device));

		const VkSubmitInfo			submitInfo	=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType			sType;
			DE_NULL,						// const void*				pNext;
			0u,								// deUint32					waitSemaphoreCount;
			DE_NULL,						// const VkSemaphore*		pWaitSemaphores;
			(const VkPipelineStageFlags*)DE_NULL,
			1u,								// deUint32					commandBufferCount;
			&cmdBuffer.get(),				// const VkCommandBuffer*	pCommandBuffers;
			0u,								// deUint32					signalSemaphoreCount;
			DE_NULL							// const VkSemaphore*		pSignalSemaphores;
		};

		VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));
		VK_CHECK(vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, ~0ull));
	}

	// Read back the results
	tcu::Surface surface(RENDER_WIDTH, RENDER_HEIGHT);
	{
		const tcu::TextureFormat textureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8);
		const tcu::ConstPixelBufferAccess imgAccess(textureFormat, RENDER_WIDTH, RENDER_HEIGHT, 1, readImageAlloc->getHostPtr());
		const vk::VkDeviceSize bufferSize = RENDER_WIDTH * RENDER_HEIGHT * 4;
		invalidateMappedMemoryRange(vk, device, readImageAlloc->getMemory(), readImageAlloc->getOffset(), bufferSize);

		tcu::copy(surface.getAccess(), imgAccess);
	}

	// Check if the result image is all white
	tcu::RGBA white(tcu::RGBA::white());
	int numFailedPixels = 0;

	for (int y = 0; y < surface.getHeight(); y++)
	{
		for (int x = 0; x < surface.getWidth(); x++)
		{
			if (surface.getPixel(x, y) != white)
				numFailedPixels += 1;
		}
	}

	if (numFailedPixels > 0)
	{
		tcu::TestLog& log = m_context.getTestContext().getLog();
		log << tcu::TestLog::Image("Image", "Rendered image", surface);
		log << tcu::TestLog::Message << "Image comparison failed, got " << numFailedPixels << " non-white pixels" << tcu::TestLog::EndMessage;

		for (size_t blockNdx = 0; blockNdx < m_layout.blocks.size(); blockNdx++)
		{
			const BlockLayoutEntry& block = m_layout.blocks[blockNdx];
			log << tcu::TestLog::Message << "Block index: " << blockNdx << " infos: " << block << tcu::TestLog::EndMessage;
		}

		for (size_t uniformNdx = 0; uniformNdx < m_layout.uniforms.size(); uniformNdx++)
		{
			log << tcu::TestLog::Message << "Uniform index: " << uniformNdx << " infos: " << m_layout.uniforms[uniformNdx] << tcu::TestLog::EndMessage;
		}

		return tcu::TestStatus::fail("Detected non-white pixels");
	}
	else
		return tcu::TestStatus::pass("Full white image ok");
}

vk::VkDescriptorBufferInfo UniformBlockCaseInstance::addUniformData (deUint32 size, const void* dataPtr)
{
	const VkDevice					vkDevice			= m_context.getDevice();
	const DeviceInterface&			vk					= m_context.getDeviceInterface();

	Move<VkBuffer>					buffer	= createBuffer(m_context, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	de::MovePtr<Allocation>			alloc	= allocateAndBindMemory(m_context, *buffer, vk::MemoryRequirement::HostVisible);

	deMemcpy(alloc->getHostPtr(), dataPtr, size);
	flushMappedMemoryRange(vk, vkDevice, alloc->getMemory(), alloc->getOffset(), size);

	const VkDescriptorBufferInfo			descriptor			=
	{
		*buffer,				// VkBuffer		buffer;
		0u,						// VkDeviceSize	offset;
		size,					// VkDeviceSize	range;

	};

	m_uniformBuffers.push_back(VkBufferSp(new vk::Unique<vk::VkBuffer>(buffer)));
	m_uniformAllocs.push_back(AllocationSp(alloc.release()));

	return descriptor;
}

vk::Move<VkRenderPass> UniformBlockCaseInstance::createRenderPass (vk::VkFormat format) const
{
	const VkDevice					vkDevice				= m_context.getDevice();
	const DeviceInterface&			vk						= m_context.getDeviceInterface();

	const VkAttachmentDescription	attachmentDescription	=
	{
		0u,												// VkAttachmentDescriptorFlags	flags;
		format,											// VkFormat						format;
		VK_SAMPLE_COUNT_1_BIT,							// VkSampleCountFlagBits		samples;
		VK_ATTACHMENT_LOAD_OP_CLEAR,					// VkAttachmentLoadOp			loadOp;
		VK_ATTACHMENT_STORE_OP_STORE,					// VkAttachmentStoreOp			storeOp;
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,				// VkAttachmentLoadOp			stencilLoadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,				// VkAttachmentStoreOp			stencilStoreOp;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		// VkImageLayout				initialLayout;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		// VkImageLayout				finalLayout;
	};

	const VkAttachmentReference		attachmentReference		=
	{
		0u,											// deUint32			attachment;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout	layout;
	};


	const VkSubpassDescription		subpassDescription		=
	{
		0u,												// VkSubpassDescriptionFlags	flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,				// VkPipelineBindPoint			pipelineBindPoint;
		0u,												// deUint32						inputAttachmentCount;
		DE_NULL,										// const VkAttachmentReference*	pInputAttachments;
		1u,												// deUint32						colorAttachmentCount;
		&attachmentReference,							// const VkAttachmentReference*	pColorAttachments;
		DE_NULL,										// const VkAttachmentReference*	pResolveAttachments;
		DE_NULL,										// const VkAttachmentReference*	pDepthStencilAttachment;
		0u,												// deUint32						preserveAttachmentCount;
		DE_NULL											// const VkAttachmentReference*	pPreserveAttachments;
	};

	const VkRenderPassCreateInfo	renderPassParams		=
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,		// VkStructureType					sType;
		DE_NULL,										// const void*						pNext;
		0u,												// VkRenderPassCreateFlags			flags;
		1u,												// deUint32							attachmentCount;
		&attachmentDescription,							// const VkAttachmentDescription*	pAttachments;
		1u,												// deUint32							subpassCount;
		&subpassDescription,							// const VkSubpassDescription*		pSubpasses;
		0u,												// deUint32							dependencyCount;
		DE_NULL											// const VkSubpassDependency*		pDependencies;
	};

	return vk::createRenderPass(vk, vkDevice, &renderPassParams);
}

vk::Move<VkFramebuffer> UniformBlockCaseInstance::createFramebuffer (vk::VkRenderPass renderPass, vk::VkImageView colorImageView) const
{
	const VkDevice					vkDevice			= m_context.getDevice();
	const DeviceInterface&			vk					= m_context.getDeviceInterface();

	const VkFramebufferCreateInfo	framebufferParams	=
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,		// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		0u,												// VkFramebufferCreateFlags	flags;
		renderPass,										// VkRenderPass				renderPass;
		1u,												// deUint32					attachmentCount;
		&colorImageView,								// const VkImageView*		pAttachments;
		RENDER_WIDTH,									// deUint32					width;
		RENDER_HEIGHT,									// deUint32					height;
		1u												// deUint32					layers;
	};

	return vk::createFramebuffer(vk, vkDevice, &framebufferParams);
}

vk::Move<VkDescriptorSetLayout> UniformBlockCaseInstance::createDescriptorSetLayout (void) const
{
	int numBlocks = (int)m_layout.blocks.size();
	int lastBindingNdx = -1;
	std::vector<int> lengths;

	for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
	{
		const BlockLayoutEntry& block = m_layout.blocks[blockNdx];

		if (block.bindingNdx == lastBindingNdx)
		{
			lengths.back()++;
		}
		else
		{
			lengths.push_back(1);
			lastBindingNdx = block.bindingNdx;
		}
	}

	vk::DescriptorSetLayoutBuilder layoutBuilder;
	for (size_t i = 0; i < lengths.size(); i++)
	{
		if (lengths[i] > 0)
		{
			layoutBuilder.addArrayBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, lengths[i], vk::VK_SHADER_STAGE_ALL);
		}
		else
		{
			layoutBuilder.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, vk::VK_SHADER_STAGE_ALL);
		}
	}

	return layoutBuilder.build(m_context.getDeviceInterface(), m_context.getDevice());
}

vk::Move<VkDescriptorPool> UniformBlockCaseInstance::createDescriptorPool (void) const
{
	vk::DescriptorPoolBuilder poolBuilder;

	return poolBuilder
		.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (int)m_layout.blocks.size())
		.build(m_context.getDeviceInterface(), m_context.getDevice(), VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);
}

vk::Move<VkPipeline> UniformBlockCaseInstance::createPipeline (vk::VkShaderModule vtxShaderModule, vk::VkShaderModule fragShaderModule, vk::VkPipelineLayout pipelineLayout, vk::VkRenderPass renderPass) const
{
	const VkDevice									vkDevice				= m_context.getDevice();
	const DeviceInterface&							vk						= m_context.getDeviceInterface();

	const VkVertexInputBindingDescription			vertexBinding			=
	{
		0,									// deUint32					binding;
		(deUint32)sizeof(float) * 4,		// deUint32					strideInBytes;
		VK_VERTEX_INPUT_RATE_VERTEX			// VkVertexInputStepRate	inputRate;
	};

	const VkVertexInputAttributeDescription			vertexAttribute			=
	{
		0,									// deUint32		location;
		0,									// deUint32		binding;
		VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat		format;
		0u									// deUint32		offset;
	};

	const VkPipelineShaderStageCreateInfo			shaderStages[2]	=
	{
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType					sType;
			DE_NULL,												// const void*						pNext;
			0u,														// VkPipelineShaderStageCreateFlags	flags;
			VK_SHADER_STAGE_VERTEX_BIT,								// VkShaderStageFlagBits			stage;
			vtxShaderModule,										// VkShaderModule					module;
			"main",													// const char*						pName;
			DE_NULL													// const VkSpecializationInfo*		pSpecializationInfo;
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType					sType;
			DE_NULL,												// const void*						pNext;
			0u,														// VkPipelineShaderStageCreateFlags flags;
			VK_SHADER_STAGE_FRAGMENT_BIT,							// VkShaderStageFlagBits			stage;
			fragShaderModule,										// VkShaderModule					module;
			"main",													// const char*						pName;
			DE_NULL													// const VkSpecializationInfo*		pSpecializationInfo;
		}
	};

	const VkPipelineVertexInputStateCreateInfo		vertexInputStateParams		=
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		0u,															// VkPipelineVertexInputStateCreateFlags	flags;
		1u,															// deUint32									vertexBindingDescriptionCount;
		&vertexBinding,												// const VkVertexInputBindingDescription*	pVertexBindingDescriptions;
		1u,															// deUint32									vertexAttributeDescriptionCount;
		&vertexAttribute,											// const VkVertexInputAttributeDescription*	pVertexAttributeDescriptions;
	};

	const VkPipelineInputAssemblyStateCreateInfo	inputAssemblyStateParams	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		0u,															// VkPipelineInputAssemblyStateCreateFlags	flags;
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,						// VkPrimitiveTopology						topology;
		false														// VkBool32									primitiveRestartEnable;
	};

	const VkViewport								viewport					=
	{
		0.0f,					// float	originX;
		0.0f,					// float	originY;
		(float)RENDER_WIDTH,	// float	width;
		(float)RENDER_HEIGHT,	// float	height;
		0.0f,					// float	minDepth;
		1.0f					// float	maxDepth;
	};


	const VkRect2D									scissor						=
	{
		{
			0u,				// deUint32	x;
			0u,				// deUint32	y;
		},						// VkOffset2D	offset;
		{
			RENDER_WIDTH,	// deUint32	width;
			RENDER_HEIGHT,	// deUint32	height;
		},						// VkExtent2D	extent;
	};

	const VkPipelineViewportStateCreateInfo			viewportStateParams			=
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,		// VkStructureType						sType;
		DE_NULL,													// const void*							pNext;
		0u,															// VkPipelineViewportStateCreateFlags	flags;
		1u,															// deUint32								viewportCount;
		&viewport,													// const VkViewport*					pViewports;
		1u,															// deUint32								scissorsCount;
		&scissor,													// const VkRect2D*						pScissors;
	};

	const VkPipelineRasterizationStateCreateInfo	rasterStateParams			=
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		0u,															// VkPipelineRasterizationStateCreateFlags	flags;
		false,														// VkBool32									depthClampEnable;
		false,														// VkBool32									rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,										// VkPolygonMode							polygonMode;
		VK_CULL_MODE_NONE,											// VkCullModeFlags							cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,							// VkFrontFace								frontFace;
		false,														// VkBool32									depthBiasEnable;
		0.0f,														// float									depthBiasConstantFactor;
		0.0f,														// float									depthBiasClamp;
		0.0f,														// float									depthBiasSlopeFactor;
		1.0f,														// float									lineWidth;
	};

	const VkPipelineMultisampleStateCreateInfo		multisampleStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		0u,															// VkPipelineMultisampleStateCreateFlags	flags;
		VK_SAMPLE_COUNT_1_BIT,										// VkSampleCountFlagBits					rasterizationSamples;
		VK_FALSE,													// VkBool32									sampleShadingEnable;
		0.0f,														// float									minSampleShading;
		DE_NULL,													// const VkSampleMask*						pSampleMask;
		VK_FALSE,													// VkBool32									alphaToCoverageEnable;
		VK_FALSE													// VkBool32									alphaToOneEnable;
	 };

	const VkPipelineColorBlendAttachmentState		colorBlendAttachmentState	=
	{
		false,																		// VkBool32			blendEnable;
		VK_BLEND_FACTOR_ONE,														// VkBlend			srcBlendColor;
		VK_BLEND_FACTOR_ZERO,														// VkBlend			destBlendColor;
		VK_BLEND_OP_ADD,															// VkBlendOp		blendOpColor;
		VK_BLEND_FACTOR_ONE,														// VkBlend			srcBlendAlpha;
		VK_BLEND_FACTOR_ZERO,														// VkBlend			destBlendAlpha;
		VK_BLEND_OP_ADD,															// VkBlendOp		blendOpAlpha;
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |						// VkChannelFlags	channelWriteMask;
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	const VkPipelineColorBlendStateCreateInfo		colorBlendStateParams		=
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
		DE_NULL,													// const void*									pNext;
		0u,															// VkPipelineColorBlendStateCreateFlags			flags;
		false,														// VkBool32										logicOpEnable;
		VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
		1u,															// deUint32										attachmentCount;
		&colorBlendAttachmentState,									// const VkPipelineColorBlendAttachmentState*	pAttachments;
		{ 0.0f, 0.0f, 0.0f, 0.0f },									// float										blendConstants[4];
	};

	const VkGraphicsPipelineCreateInfo				graphicsPipelineParams		=
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
		DE_NULL,											// const void*										pNext;
		0u,													// VkPipelineCreateFlags							flags;
		2u,													// deUint32											stageCount;
		shaderStages,										// const VkPipelineShaderStageCreateInfo*			pStages;
		&vertexInputStateParams,							// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
		&inputAssemblyStateParams,							// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
		DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
		&viewportStateParams,								// const VkPipelineViewportStateCreateInfo*			pViewportState;
		&rasterStateParams,									// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
		&multisampleStateParams,							// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
		DE_NULL,											// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
		&colorBlendStateParams,								// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
		(const VkPipelineDynamicStateCreateInfo*)DE_NULL,	// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
		pipelineLayout,										// VkPipelineLayout									layout;
		renderPass,											// VkRenderPass										renderPass;
		0u,													// deUint32											subpass;
		0u,													// VkPipeline										basePipelineHandle;
		0u													// deInt32											basePipelineIndex;
	};

	return vk::createGraphicsPipeline(vk, vkDevice, DE_NULL, &graphicsPipelineParams);
}

} // anonymous (utilities)

// UniformBlockCase.

UniformBlockCase::UniformBlockCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, BufferMode bufferMode, MatrixLoadFlags matrixLoadFlag, bool shuffleUniformMembers)
	: TestCase					(testCtx, name, description)
	, m_bufferMode				(bufferMode)
	, m_matrixLoadFlag			(matrixLoadFlag)
	, m_shuffleUniformMembers	(shuffleUniformMembers)
{
}

UniformBlockCase::~UniformBlockCase (void)
{
}

void UniformBlockCase::initPrograms (vk::SourceCollections& programCollection) const
{
	DE_ASSERT(!m_vertShaderSource.empty());
	DE_ASSERT(!m_fragShaderSource.empty());

	programCollection.glslSources.add("vert") << glu::VertexSource(m_vertShaderSource);
	programCollection.glslSources.add("frag") << glu::FragmentSource(m_fragShaderSource);
}

TestInstance* UniformBlockCase::createInstance (Context& context) const
{
	return new UniformBlockCaseInstance(context, m_bufferMode, m_uniformLayout, m_blockPointers);
}

void UniformBlockCase::init (void)
{
	// Compute reference layout.
	computeStd140Layout(m_uniformLayout, m_interface);

	// Assign storage for reference values.
	{
		int totalSize = 0;
		for (std::vector<BlockLayoutEntry>::const_iterator blockIter = m_uniformLayout.blocks.begin(); blockIter != m_uniformLayout.blocks.end(); blockIter++)
			totalSize += blockIter->size;
		m_data.resize(totalSize);

		// Pointers for each block.
		int curOffset = 0;
		for (int blockNdx = 0; blockNdx < (int)m_uniformLayout.blocks.size(); blockNdx++)
		{
			m_blockPointers[blockNdx] = &m_data[0] + curOffset;
			curOffset += m_uniformLayout.blocks[blockNdx].size;
		}
	}

	// Generate values.
	generateValues(m_uniformLayout, m_blockPointers, 1 /* seed */);

	// Generate shaders.
	m_vertShaderSource = generateVertexShader(m_interface, m_uniformLayout, m_blockPointers, m_matrixLoadFlag, m_shuffleUniformMembers);
	m_fragShaderSource = generateFragmentShader(m_interface, m_uniformLayout, m_blockPointers, m_matrixLoadFlag, m_shuffleUniformMembers);
}

} // ubo
} // vkt
