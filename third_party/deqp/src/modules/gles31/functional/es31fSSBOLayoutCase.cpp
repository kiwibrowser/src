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
 * \brief SSBO layout case.
 *//*--------------------------------------------------------------------*/

#include "es31fSSBOLayoutCase.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"
#include "gluPixelTransfer.hpp"
#include "gluContextInfo.hpp"
#include "gluRenderContext.hpp"
#include "gluProgramInterfaceQuery.hpp"
#include "gluObjectWrapper.hpp"
#include "gluVarTypeUtil.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "tcuTestLog.hpp"
#include "tcuSurface.hpp"
#include "tcuRenderTarget.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deMemory.h"
#include "deString.h"
#include "deMath.h"

#include <algorithm>
#include <map>

using tcu::TestLog;
using std::string;
using std::vector;
using std::map;

namespace deqp
{
namespace gles31
{

using glu::VarType;
using glu::StructType;
using glu::StructMember;

namespace bb
{

struct LayoutFlagsFmt
{
	deUint32 flags;
	LayoutFlagsFmt (deUint32 flags_) : flags(flags_) {}
};

std::ostream& operator<< (std::ostream& str, const LayoutFlagsFmt& fmt)
{
	static const struct
	{
		deUint32	bit;
		const char*	token;
	} bitDesc[] =
	{
		{ LAYOUT_SHARED,		"shared"		},
		{ LAYOUT_PACKED,		"packed"		},
		{ LAYOUT_STD140,		"std140"		},
		{ LAYOUT_STD430,		"std430"		},
		{ LAYOUT_ROW_MAJOR,		"row_major"		},
		{ LAYOUT_COLUMN_MAJOR,	"column_major"	}
	};

	deUint32 remBits = fmt.flags;
	for (int descNdx = 0; descNdx < DE_LENGTH_OF_ARRAY(bitDesc); descNdx++)
	{
		if (remBits & bitDesc[descNdx].bit)
		{
			if (remBits != fmt.flags)
				str << ", ";
			str << bitDesc[descNdx].token;
			remBits &= ~bitDesc[descNdx].bit;
		}
	}
	DE_ASSERT(remBits == 0);
	return str;
}

// BufferVar implementation.

BufferVar::BufferVar (const char* name, const VarType& type, deUint32 flags)
	: m_name	(name)
	, m_type	(type)
	, m_flags	(flags)
{
}

// BufferBlock implementation.

BufferBlock::BufferBlock (const char* blockName)
	: m_blockName	(blockName)
	, m_arraySize	(-1)
	, m_flags		(0)
{
	setArraySize(0);
}

void BufferBlock::setArraySize (int arraySize)
{
	DE_ASSERT(arraySize >= 0);
	m_lastUnsizedArraySizes.resize(arraySize == 0 ? 1 : arraySize, 0);
	m_arraySize = arraySize;
}

struct BlockLayoutEntry
{
	BlockLayoutEntry (void)
		: size(0)
	{
	}

	std::string			name;
	int					size;
	std::vector<int>	activeVarIndices;
};

std::ostream& operator<< (std::ostream& stream, const BlockLayoutEntry& entry)
{
	stream << entry.name << " { name = " << entry.name
		   << ", size = " << entry.size
		   << ", activeVarIndices = [";

	for (vector<int>::const_iterator i = entry.activeVarIndices.begin(); i != entry.activeVarIndices.end(); i++)
	{
		if (i != entry.activeVarIndices.begin())
			stream << ", ";
		stream << *i;
	}

	stream << "] }";
	return stream;
}

struct BufferVarLayoutEntry
{
	BufferVarLayoutEntry (void)
		: type					(glu::TYPE_LAST)
		, blockNdx				(-1)
		, offset				(-1)
		, arraySize				(-1)
		, arrayStride			(-1)
		, matrixStride			(-1)
		, topLevelArraySize		(-1)
		, topLevelArrayStride	(-1)
		, isRowMajor			(false)
	{
	}

	std::string			name;
	glu::DataType		type;
	int					blockNdx;
	int					offset;
	int					arraySize;
	int					arrayStride;
	int					matrixStride;
	int					topLevelArraySize;
	int					topLevelArrayStride;
	bool				isRowMajor;
};

static bool isUnsizedArray (const BufferVarLayoutEntry& entry)
{
	DE_ASSERT(entry.arraySize != 0 || entry.topLevelArraySize != 0);
	return entry.arraySize == 0 || entry.topLevelArraySize == 0;
}

std::ostream& operator<< (std::ostream& stream, const BufferVarLayoutEntry& entry)
{
	stream << entry.name << " { type = " << glu::getDataTypeName(entry.type)
		   << ", blockNdx = " << entry.blockNdx
		   << ", offset = " << entry.offset
		   << ", arraySize = " << entry.arraySize
		   << ", arrayStride = " << entry.arrayStride
		   << ", matrixStride = " << entry.matrixStride
		   << ", topLevelArraySize = " << entry.topLevelArraySize
		   << ", topLevelArrayStride = " << entry.topLevelArrayStride
		   << ", isRowMajor = " << (entry.isRowMajor ? "true" : "false")
		   << " }";
	return stream;
}

class BufferLayout
{
public:
	std::vector<BlockLayoutEntry>		blocks;
	std::vector<BufferVarLayoutEntry>	bufferVars;

	int									getVariableIndex		(const string& name) const;
	int									getBlockIndex			(const string& name) const;
};

// \todo [2012-01-24 pyry] Speed up lookups using hash.

int BufferLayout::getVariableIndex (const string& name) const
{
	for (int ndx = 0; ndx < (int)bufferVars.size(); ndx++)
	{
		if (bufferVars[ndx].name == name)
			return ndx;
	}
	return -1;
}

int BufferLayout::getBlockIndex (const string& name) const
{
	for (int ndx = 0; ndx < (int)blocks.size(); ndx++)
	{
		if (blocks[ndx].name == name)
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
	for (std::vector<StructType*>::iterator i = m_structs.begin(); i != m_structs.end(); i++)
		delete *i;

	for (std::vector<BufferBlock*>::iterator i = m_bufferBlocks.begin(); i != m_bufferBlocks.end(); i++)
		delete *i;
}

StructType& ShaderInterface::allocStruct (const char* name)
{
	m_structs.reserve(m_structs.size()+1);
	m_structs.push_back(new StructType(name));
	return *m_structs.back();
}

struct StructNameEquals
{
	std::string name;

	StructNameEquals (const char* name_) : name(name_) {}

	bool operator() (const StructType* type) const
	{
		return type->getTypeName() && name == type->getTypeName();
	}
};

const StructType* ShaderInterface::findStruct (const char* name) const
{
	std::vector<StructType*>::const_iterator pos = std::find_if(m_structs.begin(), m_structs.end(), StructNameEquals(name));
	return pos != m_structs.end() ? *pos : DE_NULL;
}

void ShaderInterface::getNamedStructs (std::vector<const StructType*>& structs) const
{
	for (std::vector<StructType*>::const_iterator i = m_structs.begin(); i != m_structs.end(); i++)
	{
		if ((*i)->getTypeName() != DE_NULL)
			structs.push_back(*i);
	}
}

BufferBlock& ShaderInterface::allocBlock (const char* name)
{
	m_bufferBlocks.reserve(m_bufferBlocks.size()+1);
	m_bufferBlocks.push_back(new BufferBlock(name));
	return *m_bufferBlocks.back();
}

// BlockDataPtr

struct BlockDataPtr
{
	void*		ptr;
	int			size;						//!< Redundant, for debugging purposes.
	int			lastUnsizedArraySize;

	BlockDataPtr (void* ptr_, int size_, int lastUnsizedArraySize_)
		: ptr					(ptr_)
		, size					(size_)
		, lastUnsizedArraySize	(lastUnsizedArraySize_)
	{
	}

	BlockDataPtr (void)
		: ptr					(DE_NULL)
		, size					(0)
		, lastUnsizedArraySize	(0)
	{
	}
};

namespace // Utilities
{

int findBlockIndex (const BufferLayout& layout, const string& name)
{
	for (int ndx = 0; ndx < (int)layout.blocks.size(); ndx++)
	{
		if (layout.blocks[ndx].name == name)
			return ndx;
	}
	return -1;
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

static inline int deRoundUp32 (int a, int b)
{
	int d = a/b;
	return d*b == a ? a : (d+1)*b;
}

int computeStd140BaseAlignment (const VarType& type, deUint32 layoutFlags)
{
	const int vec4Alignment = (int)sizeof(deUint32)*4;

	if (type.isBasicType())
	{
		glu::DataType basicType = type.getBasicType();

		if (glu::isDataTypeMatrix(basicType))
		{
			const bool	isRowMajor	= !!(layoutFlags & LAYOUT_ROW_MAJOR);
			const int	vecSize		= isRowMajor ? glu::getDataTypeMatrixNumColumns(basicType)
												 : glu::getDataTypeMatrixNumRows(basicType);
			const int	vecAlign	= deAlign32(getDataTypeByteAlignment(glu::getDataTypeFloatVec(vecSize)), vec4Alignment);

			return vecAlign;
		}
		else
			return getDataTypeByteAlignment(basicType);
	}
	else if (type.isArrayType())
	{
		int elemAlignment = computeStd140BaseAlignment(type.getElementType(), layoutFlags);

		// Round up to alignment of vec4
		return deAlign32(elemAlignment, vec4Alignment);
	}
	else
	{
		DE_ASSERT(type.isStructType());

		int maxBaseAlignment = 0;

		for (StructType::ConstIterator memberIter = type.getStructPtr()->begin(); memberIter != type.getStructPtr()->end(); memberIter++)
			maxBaseAlignment = de::max(maxBaseAlignment, computeStd140BaseAlignment(memberIter->getType(), layoutFlags));

		return deAlign32(maxBaseAlignment, vec4Alignment);
	}
}

int computeStd430BaseAlignment (const VarType& type, deUint32 layoutFlags)
{
	// Otherwise identical to std140 except that alignment of structures and arrays
	// are not rounded up to alignment of vec4.

	if (type.isBasicType())
	{
		glu::DataType basicType = type.getBasicType();

		if (glu::isDataTypeMatrix(basicType))
		{
			const bool	isRowMajor	= !!(layoutFlags & LAYOUT_ROW_MAJOR);
			const int	vecSize		= isRowMajor ? glu::getDataTypeMatrixNumColumns(basicType)
												 : glu::getDataTypeMatrixNumRows(basicType);
			const int	vecAlign	= getDataTypeByteAlignment(glu::getDataTypeFloatVec(vecSize));

			return vecAlign;
		}
		else
			return getDataTypeByteAlignment(basicType);
	}
	else if (type.isArrayType())
	{
		return computeStd430BaseAlignment(type.getElementType(), layoutFlags);
	}
	else
	{
		DE_ASSERT(type.isStructType());

		int maxBaseAlignment = 0;

		for (StructType::ConstIterator memberIter = type.getStructPtr()->begin(); memberIter != type.getStructPtr()->end(); memberIter++)
			maxBaseAlignment = de::max(maxBaseAlignment, computeStd430BaseAlignment(memberIter->getType(), layoutFlags));

		return maxBaseAlignment;
	}
}

inline deUint32 mergeLayoutFlags (deUint32 prevFlags, deUint32 newFlags)
{
	const deUint32	packingMask		= LAYOUT_PACKED|LAYOUT_SHARED|LAYOUT_STD140|LAYOUT_STD430;
	const deUint32	matrixMask		= LAYOUT_ROW_MAJOR|LAYOUT_COLUMN_MAJOR;

	deUint32 mergedFlags = 0;

	mergedFlags |= ((newFlags & packingMask)	? newFlags : prevFlags) & packingMask;
	mergedFlags |= ((newFlags & matrixMask)		? newFlags : prevFlags) & matrixMask;

	return mergedFlags;
}

//! Appends all child elements to layout, returns value that should be appended to offset.
int computeReferenceLayout (
	BufferLayout&		layout,
	int					curBlockNdx,
	int					baseOffset,
	const std::string&	curPrefix,
	const VarType&		type,
	deUint32			layoutFlags)
{
	// Reference layout uses std430 rules by default. std140 rules are
	// choosen only for blocks that have std140 layout.
	const bool	isStd140			= (layoutFlags & LAYOUT_STD140) != 0;
	const int	baseAlignment		= isStd140 ? computeStd140BaseAlignment(type, layoutFlags)
											   : computeStd430BaseAlignment(type, layoutFlags);
	int			curOffset			= deAlign32(baseOffset, baseAlignment);
	const int	topLevelArraySize	= 1; // Default values
	const int	topLevelArrayStride	= 0;

	if (type.isBasicType())
	{
		const glu::DataType		basicType	= type.getBasicType();
		BufferVarLayoutEntry	entry;

		entry.name					= curPrefix;
		entry.type					= basicType;
		entry.arraySize				= 1;
		entry.arrayStride			= 0;
		entry.matrixStride			= 0;
		entry.topLevelArraySize		= topLevelArraySize;
		entry.topLevelArrayStride	= topLevelArrayStride;
		entry.blockNdx				= curBlockNdx;

		if (glu::isDataTypeMatrix(basicType))
		{
			// Array of vectors as specified in rules 5 & 7.
			const bool	isRowMajor			= !!(layoutFlags & LAYOUT_ROW_MAJOR);
			const int	numVecs				= isRowMajor ? glu::getDataTypeMatrixNumRows(basicType)
														 : glu::getDataTypeMatrixNumColumns(basicType);

			entry.offset		= curOffset;
			entry.matrixStride	= baseAlignment;
			entry.isRowMajor	= isRowMajor;

			curOffset += numVecs*baseAlignment;
		}
		else
		{
			// Scalar or vector.
			entry.offset = curOffset;

			curOffset += getDataTypeByteSize(basicType);
		}

		layout.bufferVars.push_back(entry);
	}
	else if (type.isArrayType())
	{
		const VarType&	elemType	= type.getElementType();

		if (elemType.isBasicType() && !glu::isDataTypeMatrix(elemType.getBasicType()))
		{
			// Array of scalars or vectors.
			const glu::DataType		elemBasicType	= elemType.getBasicType();
			const int				stride			= baseAlignment;
			BufferVarLayoutEntry	entry;

			entry.name					= curPrefix + "[0]"; // Array variables are always postfixed with [0]
			entry.type					= elemBasicType;
			entry.blockNdx				= curBlockNdx;
			entry.offset				= curOffset;
			entry.arraySize				= type.getArraySize();
			entry.arrayStride			= stride;
			entry.matrixStride			= 0;
			entry.topLevelArraySize		= topLevelArraySize;
			entry.topLevelArrayStride	= topLevelArrayStride;

			curOffset += stride*type.getArraySize();

			layout.bufferVars.push_back(entry);
		}
		else if (elemType.isBasicType() && glu::isDataTypeMatrix(elemType.getBasicType()))
		{
			// Array of matrices.
			const glu::DataType			elemBasicType	= elemType.getBasicType();
			const bool					isRowMajor		= !!(layoutFlags & LAYOUT_ROW_MAJOR);
			const int					numVecs			= isRowMajor ? glu::getDataTypeMatrixNumRows(elemBasicType)
																	 : glu::getDataTypeMatrixNumColumns(elemBasicType);
			const int					vecStride		= baseAlignment;
			BufferVarLayoutEntry		entry;

			entry.name					= curPrefix + "[0]"; // Array variables are always postfixed with [0]
			entry.type					= elemBasicType;
			entry.blockNdx				= curBlockNdx;
			entry.offset				= curOffset;
			entry.arraySize				= type.getArraySize();
			entry.arrayStride			= vecStride*numVecs;
			entry.matrixStride			= vecStride;
			entry.isRowMajor			= isRowMajor;
			entry.topLevelArraySize		= topLevelArraySize;
			entry.topLevelArrayStride	= topLevelArrayStride;

			curOffset += numVecs*vecStride*type.getArraySize();

			layout.bufferVars.push_back(entry);
		}
		else
		{
			DE_ASSERT(elemType.isStructType() || elemType.isArrayType());

			for (int elemNdx = 0; elemNdx < type.getArraySize(); elemNdx++)
				curOffset += computeReferenceLayout(layout, curBlockNdx, curOffset, curPrefix + "[" + de::toString(elemNdx) + "]", type.getElementType(), layoutFlags);
		}
	}
	else
	{
		DE_ASSERT(type.isStructType());

		for (StructType::ConstIterator memberIter = type.getStructPtr()->begin(); memberIter != type.getStructPtr()->end(); memberIter++)
			curOffset += computeReferenceLayout(layout, curBlockNdx, curOffset, curPrefix + "." + memberIter->getName(), memberIter->getType(), layoutFlags);

		curOffset = deAlign32(curOffset, baseAlignment);
	}

	return curOffset-baseOffset;
}

//! Appends all child elements to layout, returns offset increment.
int computeReferenceLayout (BufferLayout& layout, int curBlockNdx, const std::string& blockPrefix, int baseOffset, const BufferVar& bufVar, deUint32 blockLayoutFlags)
{
	const VarType&	varType			= bufVar.getType();
	const deUint32	combinedFlags	= mergeLayoutFlags(blockLayoutFlags, bufVar.getFlags());

	if (varType.isArrayType())
	{
		// Top-level arrays need special care.
		const int		topLevelArraySize	= varType.getArraySize() == VarType::UNSIZED_ARRAY ? 0 : varType.getArraySize();
		const string	prefix				= blockPrefix + bufVar.getName() + "[0]";
		const bool		isStd140			= (blockLayoutFlags & LAYOUT_STD140) != 0;
		const int		vec4Align			= (int)sizeof(deUint32)*4;
		const int		baseAlignment		= isStd140 ? computeStd140BaseAlignment(varType, combinedFlags)
													   : computeStd430BaseAlignment(varType, combinedFlags);
		int				curOffset			= deAlign32(baseOffset, baseAlignment);
		const VarType&	elemType			= varType.getElementType();

		if (elemType.isBasicType() && !glu::isDataTypeMatrix(elemType.getBasicType()))
		{
			// Array of scalars or vectors.
			const glu::DataType		elemBasicType	= elemType.getBasicType();
			const int				elemBaseAlign	= getDataTypeByteAlignment(elemBasicType);
			const int				stride			= isStd140 ? deAlign32(elemBaseAlign, vec4Align) : elemBaseAlign;
			BufferVarLayoutEntry	entry;

			entry.name					= prefix;
			entry.topLevelArraySize		= 1;
			entry.topLevelArrayStride	= 0;
			entry.type					= elemBasicType;
			entry.blockNdx				= curBlockNdx;
			entry.offset				= curOffset;
			entry.arraySize				= topLevelArraySize;
			entry.arrayStride			= stride;
			entry.matrixStride			= 0;

			layout.bufferVars.push_back(entry);

			curOffset += stride*topLevelArraySize;
		}
		else if (elemType.isBasicType() && glu::isDataTypeMatrix(elemType.getBasicType()))
		{
			// Array of matrices.
			const glu::DataType		elemBasicType	= elemType.getBasicType();
			const bool				isRowMajor		= !!(combinedFlags & LAYOUT_ROW_MAJOR);
			const int				vecSize			= isRowMajor ? glu::getDataTypeMatrixNumColumns(elemBasicType)
																 : glu::getDataTypeMatrixNumRows(elemBasicType);
			const int				numVecs			= isRowMajor ? glu::getDataTypeMatrixNumRows(elemBasicType)
																 : glu::getDataTypeMatrixNumColumns(elemBasicType);
			const glu::DataType		vecType			= glu::getDataTypeFloatVec(vecSize);
			const int				vecBaseAlign	= getDataTypeByteAlignment(vecType);
			const int				stride			= isStd140 ? deAlign32(vecBaseAlign, vec4Align) : vecBaseAlign;
			BufferVarLayoutEntry	entry;

			entry.name					= prefix;
			entry.topLevelArraySize		= 1;
			entry.topLevelArrayStride	= 0;
			entry.type					= elemBasicType;
			entry.blockNdx				= curBlockNdx;
			entry.offset				= curOffset;
			entry.arraySize				= topLevelArraySize;
			entry.arrayStride			= stride*numVecs;
			entry.matrixStride			= stride;
			entry.isRowMajor			= isRowMajor;

			layout.bufferVars.push_back(entry);

			curOffset += stride*numVecs*topLevelArraySize;
		}
		else
		{
			DE_ASSERT(elemType.isStructType() || elemType.isArrayType());

			// Struct base alignment is not added multiple times as curOffset supplied to computeReferenceLayout
			// was already aligned correctly. Thus computeReferenceLayout should not add any extra padding
			// before struct. Padding after struct will be added as it should.
			//
			// Stride could be computed prior to creating child elements, but it would essentially require running
			// the layout computation twice. Instead we fix stride to child elements afterwards.

			const int	firstChildNdx	= (int)layout.bufferVars.size();
			const int	stride			= computeReferenceLayout(layout, curBlockNdx, curOffset, prefix, varType.getElementType(), combinedFlags);

			for (int childNdx = firstChildNdx; childNdx < (int)layout.bufferVars.size(); childNdx++)
			{
				layout.bufferVars[childNdx].topLevelArraySize	= topLevelArraySize;
				layout.bufferVars[childNdx].topLevelArrayStride	= stride;
			}

			curOffset += stride*topLevelArraySize;
		}

		return curOffset-baseOffset;
	}
	else
		return computeReferenceLayout(layout, curBlockNdx, baseOffset, blockPrefix + bufVar.getName(), varType, combinedFlags);
}

void computeReferenceLayout (BufferLayout& layout, const ShaderInterface& interface)
{
	int numBlocks = interface.getNumBlocks();

	for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
	{
		const BufferBlock&	block			= interface.getBlock(blockNdx);
		bool				hasInstanceName	= block.getInstanceName() != DE_NULL;
		std::string			blockPrefix		= hasInstanceName ? (std::string(block.getBlockName()) + ".") : std::string("");
		int					curOffset		= 0;
		int					activeBlockNdx	= (int)layout.blocks.size();
		int					firstVarNdx		= (int)layout.bufferVars.size();

		for (BufferBlock::const_iterator varIter = block.begin(); varIter != block.end(); varIter++)
		{
			const BufferVar& bufVar = *varIter;
			curOffset += computeReferenceLayout(layout, activeBlockNdx,  blockPrefix, curOffset, bufVar, block.getFlags());
		}

		int	varIndicesEnd	= (int)layout.bufferVars.size();
		int	blockSize		= curOffset;
		int	numInstances	= block.isArray() ? block.getArraySize() : 1;

		// Create block layout entries for each instance.
		for (int instanceNdx = 0; instanceNdx < numInstances; instanceNdx++)
		{
			// Allocate entry for instance.
			layout.blocks.push_back(BlockLayoutEntry());
			BlockLayoutEntry& blockEntry = layout.blocks.back();

			blockEntry.name = block.getBlockName();
			blockEntry.size = blockSize;

			// Compute active variable set for block.
			for (int varNdx = firstVarNdx; varNdx < varIndicesEnd; varNdx++)
				blockEntry.activeVarIndices.push_back(varNdx);

			if (block.isArray())
				blockEntry.name += "[" + de::toString(instanceNdx) + "]";
		}
	}
}

// Value generator.

void generateValue (const BufferVarLayoutEntry& entry, int unsizedArraySize, void* basePtr, de::Random& rnd)
{
	const glu::DataType	scalarType		= glu::getDataTypeScalarType(entry.type);
	const int			scalarSize		= glu::getDataTypeScalarSize(entry.type);
	const int			arraySize		= entry.arraySize == 0 ? unsizedArraySize : entry.arraySize;
	const int			arrayStride		= entry.arrayStride;
	const int			topLevelSize	= entry.topLevelArraySize == 0 ? unsizedArraySize : entry.topLevelArraySize;
	const int			topLevelStride	= entry.topLevelArrayStride;
	const bool			isMatrix		= glu::isDataTypeMatrix(entry.type);
	const int			numVecs			= isMatrix ? (entry.isRowMajor ? glu::getDataTypeMatrixNumRows(entry.type) : glu::getDataTypeMatrixNumColumns(entry.type)) : 1;
	const int			vecSize			= scalarSize / numVecs;
	const int			compSize		= sizeof(deUint32);

	DE_ASSERT(scalarSize%numVecs == 0);
	DE_ASSERT(topLevelSize >= 0);
	DE_ASSERT(arraySize >= 0);

	for (int topElemNdx = 0; topElemNdx < topLevelSize; topElemNdx++)
	{
		deUint8* const topElemPtr = (deUint8*)basePtr + entry.offset + topElemNdx*topLevelStride;

		for (int elemNdx = 0; elemNdx < arraySize; elemNdx++)
		{
			deUint8* const elemPtr = topElemPtr + elemNdx*arrayStride;

			for (int vecNdx = 0; vecNdx < numVecs; vecNdx++)
			{
				deUint8* const vecPtr = elemPtr + (isMatrix ? vecNdx*entry.matrixStride : 0);

				for (int compNdx = 0; compNdx < vecSize; compNdx++)
				{
					deUint8* const compPtr = vecPtr + compSize*compNdx;

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
}

void generateValues (const BufferLayout& layout, const vector<BlockDataPtr>& blockPointers, deUint32 seed)
{
	de::Random	rnd			(seed);
	const int	numBlocks	= (int)layout.blocks.size();

	DE_ASSERT(numBlocks == (int)blockPointers.size());

	for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
	{
		const BlockLayoutEntry&	blockLayout	= layout.blocks[blockNdx];
		const BlockDataPtr&		blockPtr	= blockPointers[blockNdx];
		const int				numEntries	= (int)layout.blocks[blockNdx].activeVarIndices.size();

		for (int entryNdx = 0; entryNdx < numEntries; entryNdx++)
		{
			const int					varNdx		= blockLayout.activeVarIndices[entryNdx];
			const BufferVarLayoutEntry&	varEntry	= layout.bufferVars[varNdx];

			generateValue(varEntry, blockPtr.lastUnsizedArraySize, blockPtr.ptr, rnd);
		}
	}
}

// Shader generator.

const char* getCompareFuncForType (glu::DataType type)
{
	switch (type)
	{
		case glu::TYPE_FLOAT:			return "bool compare_float    (highp float a, highp float b)  { return abs(a - b) < 0.05; }\n";
		case glu::TYPE_FLOAT_VEC2:		return "bool compare_vec2     (highp vec2 a, highp vec2 b)    { return compare_float(a.x, b.x)&&compare_float(a.y, b.y); }\n";
		case glu::TYPE_FLOAT_VEC3:		return "bool compare_vec3     (highp vec3 a, highp vec3 b)    { return compare_float(a.x, b.x)&&compare_float(a.y, b.y)&&compare_float(a.z, b.z); }\n";
		case glu::TYPE_FLOAT_VEC4:		return "bool compare_vec4     (highp vec4 a, highp vec4 b)    { return compare_float(a.x, b.x)&&compare_float(a.y, b.y)&&compare_float(a.z, b.z)&&compare_float(a.w, b.w); }\n";
		case glu::TYPE_FLOAT_MAT2:		return "bool compare_mat2     (highp mat2 a, highp mat2 b)    { return compare_vec2(a[0], b[0])&&compare_vec2(a[1], b[1]); }\n";
		case glu::TYPE_FLOAT_MAT2X3:	return "bool compare_mat2x3   (highp mat2x3 a, highp mat2x3 b){ return compare_vec3(a[0], b[0])&&compare_vec3(a[1], b[1]); }\n";
		case glu::TYPE_FLOAT_MAT2X4:	return "bool compare_mat2x4   (highp mat2x4 a, highp mat2x4 b){ return compare_vec4(a[0], b[0])&&compare_vec4(a[1], b[1]); }\n";
		case glu::TYPE_FLOAT_MAT3X2:	return "bool compare_mat3x2   (highp mat3x2 a, highp mat3x2 b){ return compare_vec2(a[0], b[0])&&compare_vec2(a[1], b[1])&&compare_vec2(a[2], b[2]); }\n";
		case glu::TYPE_FLOAT_MAT3:		return "bool compare_mat3     (highp mat3 a, highp mat3 b)    { return compare_vec3(a[0], b[0])&&compare_vec3(a[1], b[1])&&compare_vec3(a[2], b[2]); }\n";
		case glu::TYPE_FLOAT_MAT3X4:	return "bool compare_mat3x4   (highp mat3x4 a, highp mat3x4 b){ return compare_vec4(a[0], b[0])&&compare_vec4(a[1], b[1])&&compare_vec4(a[2], b[2]); }\n";
		case glu::TYPE_FLOAT_MAT4X2:	return "bool compare_mat4x2   (highp mat4x2 a, highp mat4x2 b){ return compare_vec2(a[0], b[0])&&compare_vec2(a[1], b[1])&&compare_vec2(a[2], b[2])&&compare_vec2(a[3], b[3]); }\n";
		case glu::TYPE_FLOAT_MAT4X3:	return "bool compare_mat4x3   (highp mat4x3 a, highp mat4x3 b){ return compare_vec3(a[0], b[0])&&compare_vec3(a[1], b[1])&&compare_vec3(a[2], b[2])&&compare_vec3(a[3], b[3]); }\n";
		case glu::TYPE_FLOAT_MAT4:		return "bool compare_mat4     (highp mat4 a, highp mat4 b)    { return compare_vec4(a[0], b[0])&&compare_vec4(a[1], b[1])&&compare_vec4(a[2], b[2])&&compare_vec4(a[3], b[3]); }\n";
		case glu::TYPE_INT:				return "bool compare_int      (highp int a, highp int b)      { return a == b; }\n";
		case glu::TYPE_INT_VEC2:		return "bool compare_ivec2    (highp ivec2 a, highp ivec2 b)  { return a == b; }\n";
		case glu::TYPE_INT_VEC3:		return "bool compare_ivec3    (highp ivec3 a, highp ivec3 b)  { return a == b; }\n";
		case glu::TYPE_INT_VEC4:		return "bool compare_ivec4    (highp ivec4 a, highp ivec4 b)  { return a == b; }\n";
		case glu::TYPE_UINT:			return "bool compare_uint     (highp uint a, highp uint b)    { return a == b; }\n";
		case glu::TYPE_UINT_VEC2:		return "bool compare_uvec2    (highp uvec2 a, highp uvec2 b)  { return a == b; }\n";
		case glu::TYPE_UINT_VEC3:		return "bool compare_uvec3    (highp uvec3 a, highp uvec3 b)  { return a == b; }\n";
		case glu::TYPE_UINT_VEC4:		return "bool compare_uvec4    (highp uvec4 a, highp uvec4 b)  { return a == b; }\n";
		case glu::TYPE_BOOL:			return "bool compare_bool     (bool a, bool b)                { return a == b; }\n";
		case glu::TYPE_BOOL_VEC2:		return "bool compare_bvec2    (bvec2 a, bvec2 b)              { return a == b; }\n";
		case glu::TYPE_BOOL_VEC3:		return "bool compare_bvec3    (bvec3 a, bvec3 b)              { return a == b; }\n";
		case glu::TYPE_BOOL_VEC4:		return "bool compare_bvec4    (bvec4 a, bvec4 b)              { return a == b; }\n";
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
		for (StructType::ConstIterator iter = type.getStructPtr()->begin(); iter != type.getStructPtr()->end(); ++iter)
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

void collectUniqueBasicTypes (std::set<glu::DataType>& basicTypes, const BufferBlock& bufferBlock)
{
	for (BufferBlock::const_iterator iter = bufferBlock.begin(); iter != bufferBlock.end(); ++iter)
		collectUniqueBasicTypes(basicTypes, iter->getType());
}

void collectUniqueBasicTypes (std::set<glu::DataType>& basicTypes, const ShaderInterface& interface)
{
	for (int ndx = 0; ndx < interface.getNumBlocks(); ++ndx)
		collectUniqueBasicTypes(basicTypes, interface.getBlock(ndx));
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

void generateDeclaration (std::ostream& src, const BufferVar& bufferVar, int indentLevel)
{
	// \todo [pyry] Qualifiers

	if ((bufferVar.getFlags() & LAYOUT_MASK) != 0)
		src << "layout(" << LayoutFlagsFmt(bufferVar.getFlags() & LAYOUT_MASK) << ") ";

	src << glu::declare(bufferVar.getType(), bufferVar.getName(), indentLevel);
}

void generateDeclaration (std::ostream& src, const BufferBlock& block, int bindingPoint)
{
	src << "layout(";

	if ((block.getFlags() & LAYOUT_MASK) != 0)
		src << LayoutFlagsFmt(block.getFlags() & LAYOUT_MASK) << ", ";

	src << "binding = " << bindingPoint;

	src << ") ";

	src << "buffer " << block.getBlockName();
	src << "\n{\n";

	for (BufferBlock::const_iterator varIter = block.begin(); varIter != block.end(); varIter++)
	{
		src << Indent(1);
		generateDeclaration(src, *varIter, 1 /* indent level */);
		src << ";\n";
	}

	src << "}";

	if (block.getInstanceName() != DE_NULL)
	{
		src << " " << block.getInstanceName();
		if (block.isArray())
			src << "[" << block.getArraySize() << "]";
	}
	else
		DE_ASSERT(!block.isArray());

	src << ";\n";
}

void generateImmMatrixSrc (std::ostream& src, glu::DataType basicType, int matrixStride, bool isRowMajor, const void* valuePtr)
{
	DE_ASSERT(glu::isDataTypeMatrix(basicType));

	const int		compSize		= sizeof(deUint32);
	const int		numRows			= glu::getDataTypeMatrixNumRows(basicType);
	const int		numCols			= glu::getDataTypeMatrixNumColumns(basicType);

	src << glu::getDataTypeName(basicType) << "(";

	// Constructed in column-wise order.
	for (int colNdx = 0; colNdx < numCols; colNdx++)
	{
		for (int rowNdx = 0; rowNdx < numRows; rowNdx++)
		{
			const deUint8*	compPtr	= (const deUint8*)valuePtr + (isRowMajor ? rowNdx*matrixStride + colNdx*compSize
																				: colNdx*matrixStride + rowNdx*compSize);

			if (colNdx > 0 || rowNdx > 0)
				src << ", ";

			src << de::floatToString(*((const float*)compPtr), 1);
		}
	}

	src << ")";
}

void generateImmScalarVectorSrc (std::ostream& src, glu::DataType basicType, const void* valuePtr)
{
	DE_ASSERT(glu::isDataTypeFloatOrVec(basicType)	||
			  glu::isDataTypeIntOrIVec(basicType)	||
			  glu::isDataTypeUintOrUVec(basicType)	||
			  glu::isDataTypeBoolOrBVec(basicType));

	const glu::DataType		scalarType		= glu::getDataTypeScalarType(basicType);
	const int				scalarSize		= glu::getDataTypeScalarSize(basicType);
	const int				compSize		= sizeof(deUint32);

	if (scalarSize > 1)
		src << glu::getDataTypeName(basicType) << "(";

	for (int scalarNdx = 0; scalarNdx < scalarSize; scalarNdx++)
	{
		const deUint8* compPtr = (const deUint8*)valuePtr + scalarNdx*compSize;

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

	if (scalarSize > 1)
		src << ")";
}

string getAPIName (const BufferBlock& block, const BufferVar& var, const glu::TypeComponentVector& accessPath)
{
	std::ostringstream name;

	if (block.getInstanceName())
		name << block.getBlockName() << ".";

	name << var.getName();

	for (glu::TypeComponentVector::const_iterator pathComp = accessPath.begin(); pathComp != accessPath.end(); pathComp++)
	{
		if (pathComp->type == glu::VarTypeComponent::STRUCT_MEMBER)
		{
			const VarType		curType		= glu::getVarType(var.getType(), accessPath.begin(), pathComp);
			const StructType*	structPtr	= curType.getStructPtr();

			name << "." << structPtr->getMember(pathComp->index).getName();
		}
		else if (pathComp->type == glu::VarTypeComponent::ARRAY_ELEMENT)
		{
			if (pathComp == accessPath.begin() || (pathComp+1) == accessPath.end())
				name << "[0]"; // Top- / bottom-level array
			else
				name << "[" << pathComp->index << "]";
		}
		else
			DE_ASSERT(false);
	}

	return name.str();
}

string getShaderName (const BufferBlock& block, int instanceNdx, const BufferVar& var, const glu::TypeComponentVector& accessPath)
{
	std::ostringstream name;

	if (block.getInstanceName())
	{
		name << block.getInstanceName();

		if (block.isArray())
			name << "[" << instanceNdx << "]";

		name << ".";
	}
	else
		DE_ASSERT(instanceNdx == 0);

	name << var.getName();

	for (glu::TypeComponentVector::const_iterator pathComp = accessPath.begin(); pathComp != accessPath.end(); pathComp++)
	{
		if (pathComp->type == glu::VarTypeComponent::STRUCT_MEMBER)
		{
			const VarType		curType		= glu::getVarType(var.getType(), accessPath.begin(), pathComp);
			const StructType*	structPtr	= curType.getStructPtr();

			name << "." << structPtr->getMember(pathComp->index).getName();
		}
		else if (pathComp->type == glu::VarTypeComponent::ARRAY_ELEMENT)
			name << "[" << pathComp->index << "]";
		else
			DE_ASSERT(false);
	}

	return name.str();
}

int computeOffset (const BufferVarLayoutEntry& varLayout, const glu::TypeComponentVector& accessPath)
{
	const int	topLevelNdx		= (accessPath.size() > 1 && accessPath.front().type == glu::VarTypeComponent::ARRAY_ELEMENT) ? accessPath.front().index : 0;
	const int	bottomLevelNdx	= (!accessPath.empty() && accessPath.back().type == glu::VarTypeComponent::ARRAY_ELEMENT) ? accessPath.back().index : 0;

	return varLayout.offset + varLayout.topLevelArrayStride*topLevelNdx + varLayout.arrayStride*bottomLevelNdx;
}

void generateCompareSrc (
	std::ostream&				src,
	const char*					resultVar,
	const BufferLayout&			bufferLayout,
	const BufferBlock&			block,
	int							instanceNdx,
	const BlockDataPtr&			blockPtr,
	const BufferVar&			bufVar,
	const glu::SubTypeAccess&	accessPath)
{
	const VarType curType = accessPath.getType();

	if (curType.isArrayType())
	{
		const int arraySize = curType.getArraySize() == VarType::UNSIZED_ARRAY ? block.getLastUnsizedArraySize(instanceNdx) : curType.getArraySize();

		for (int elemNdx = 0; elemNdx < arraySize; elemNdx++)
			generateCompareSrc(src, resultVar, bufferLayout, block, instanceNdx, blockPtr, bufVar, accessPath.element(elemNdx));
	}
	else if (curType.isStructType())
	{
		const int numMembers = curType.getStructPtr()->getNumMembers();

		for (int memberNdx = 0; memberNdx < numMembers; memberNdx++)
			generateCompareSrc(src, resultVar, bufferLayout, block, instanceNdx, blockPtr, bufVar, accessPath.member(memberNdx));
	}
	else
	{
		DE_ASSERT(curType.isBasicType());

		const string	apiName	= getAPIName(block, bufVar, accessPath.getPath());
		const int		varNdx	= bufferLayout.getVariableIndex(apiName);

		DE_ASSERT(varNdx >= 0);
		{
			const BufferVarLayoutEntry&	varLayout		= bufferLayout.bufferVars[varNdx];
			const string				shaderName		= getShaderName(block, instanceNdx, bufVar, accessPath.getPath());
			const glu::DataType			basicType		= curType.getBasicType();
			const bool					isMatrix		= glu::isDataTypeMatrix(basicType);
			const char*					typeName		= glu::getDataTypeName(basicType);
			const void*					valuePtr		= (const deUint8*)blockPtr.ptr + computeOffset(varLayout, accessPath.getPath());

			src << "\t" << resultVar << " = " << resultVar << " && compare_" << typeName << "(" << shaderName << ", ";

			if (isMatrix)
				generateImmMatrixSrc(src, basicType, varLayout.matrixStride, varLayout.isRowMajor, valuePtr);
			else
				generateImmScalarVectorSrc(src, basicType, valuePtr);

			src << ");\n";
		}
	}
}

void generateCompareSrc (std::ostream& src, const char* resultVar, const ShaderInterface& interface, const BufferLayout& layout, const vector<BlockDataPtr>& blockPointers)
{
	for (int declNdx = 0; declNdx < interface.getNumBlocks(); declNdx++)
	{
		const BufferBlock&	block			= interface.getBlock(declNdx);
		const bool			isArray			= block.isArray();
		const int			numInstances	= isArray ? block.getArraySize() : 1;

		DE_ASSERT(!isArray || block.getInstanceName());

		for (int instanceNdx = 0; instanceNdx < numInstances; instanceNdx++)
		{
			const string		instanceName	= block.getBlockName() + (isArray ? "[" + de::toString(instanceNdx) + "]" : string(""));
			const int			blockNdx		= layout.getBlockIndex(instanceName);
			const BlockDataPtr&	blockPtr		= blockPointers[blockNdx];

			for (BufferBlock::const_iterator varIter = block.begin(); varIter != block.end(); varIter++)
			{
				const BufferVar& bufVar = *varIter;

				if ((bufVar.getFlags() & ACCESS_READ) == 0)
					continue; // Don't read from that variable.

				generateCompareSrc(src, resultVar, layout, block, instanceNdx, blockPtr, bufVar, glu::SubTypeAccess(bufVar.getType()));
			}
		}
	}
}

// \todo [2013-10-14 pyry] Almost identical to generateCompareSrc - unify?

void generateWriteSrc (
	std::ostream&				src,
	const BufferLayout&			bufferLayout,
	const BufferBlock&			block,
	int							instanceNdx,
	const BlockDataPtr&			blockPtr,
	const BufferVar&			bufVar,
	const glu::SubTypeAccess&	accessPath)
{
	const VarType curType = accessPath.getType();

	if (curType.isArrayType())
	{
		const int arraySize = curType.getArraySize() == VarType::UNSIZED_ARRAY ? block.getLastUnsizedArraySize(instanceNdx) : curType.getArraySize();

		for (int elemNdx = 0; elemNdx < arraySize; elemNdx++)
			generateWriteSrc(src, bufferLayout, block, instanceNdx, blockPtr, bufVar, accessPath.element(elemNdx));
	}
	else if (curType.isStructType())
	{
		const int numMembers = curType.getStructPtr()->getNumMembers();

		for (int memberNdx = 0; memberNdx < numMembers; memberNdx++)
			generateWriteSrc(src, bufferLayout, block, instanceNdx, blockPtr, bufVar, accessPath.member(memberNdx));
	}
	else
	{
		DE_ASSERT(curType.isBasicType());

		const string	apiName	= getAPIName(block, bufVar, accessPath.getPath());
		const int		varNdx	= bufferLayout.getVariableIndex(apiName);

		DE_ASSERT(varNdx >= 0);
		{
			const BufferVarLayoutEntry&	varLayout		= bufferLayout.bufferVars[varNdx];
			const string				shaderName		= getShaderName(block, instanceNdx, bufVar, accessPath.getPath());
			const glu::DataType			basicType		= curType.getBasicType();
			const bool					isMatrix		= glu::isDataTypeMatrix(basicType);
			const void*					valuePtr		= (const deUint8*)blockPtr.ptr + computeOffset(varLayout, accessPath.getPath());

			src << "\t" << shaderName << " = ";

			if (isMatrix)
				generateImmMatrixSrc(src, basicType, varLayout.matrixStride, varLayout.isRowMajor, valuePtr);
			else
				generateImmScalarVectorSrc(src, basicType, valuePtr);

			src << ";\n";
		}
	}
}

void generateWriteSrc (std::ostream& src, const ShaderInterface& interface, const BufferLayout& layout, const vector<BlockDataPtr>& blockPointers)
{
	for (int declNdx = 0; declNdx < interface.getNumBlocks(); declNdx++)
	{
		const BufferBlock&	block			= interface.getBlock(declNdx);
		const bool			isArray			= block.isArray();
		const int			numInstances	= isArray ? block.getArraySize() : 1;

		DE_ASSERT(!isArray || block.getInstanceName());

		for (int instanceNdx = 0; instanceNdx < numInstances; instanceNdx++)
		{
			const string		instanceName	= block.getBlockName() + (isArray ? "[" + de::toString(instanceNdx) + "]" : string(""));
			const int			blockNdx		= layout.getBlockIndex(instanceName);
			const BlockDataPtr&	blockPtr		= blockPointers[blockNdx];

			for (BufferBlock::const_iterator varIter = block.begin(); varIter != block.end(); varIter++)
			{
				const BufferVar& bufVar = *varIter;

				if ((bufVar.getFlags() & ACCESS_WRITE) == 0)
					continue; // Don't write to that variable.

				generateWriteSrc(src, layout, block, instanceNdx, blockPtr, bufVar, glu::SubTypeAccess(bufVar.getType()));
			}
		}
	}
}

string generateComputeShader (glu::GLSLVersion glslVersion, const ShaderInterface& interface, const BufferLayout& layout, const vector<BlockDataPtr>& comparePtrs, const vector<BlockDataPtr>& writePtrs)
{
	std::ostringstream src;

	DE_ASSERT(glslVersion == glu::GLSL_VERSION_310_ES || glslVersion == glu::GLSL_VERSION_430);

	src << glu::getGLSLVersionDeclaration(glslVersion) << "\n";
	src << "layout(local_size_x = 1) in;\n";
	src << "\n";

	std::vector<const StructType*> namedStructs;
	interface.getNamedStructs(namedStructs);
	for (std::vector<const StructType*>::const_iterator structIter = namedStructs.begin(); structIter != namedStructs.end(); structIter++)
		src << glu::declare(*structIter) << ";\n";

	{
		int bindingPoint = 0;

		for (int blockNdx = 0; blockNdx < interface.getNumBlocks(); blockNdx++)
		{
			const BufferBlock& block = interface.getBlock(blockNdx);
			generateDeclaration(src, block, bindingPoint);

			bindingPoint += block.isArray() ? block.getArraySize() : 1;
		}
	}

	// Atomic counter for counting passed invocations.
	src << "\nlayout(binding = 0) uniform atomic_uint ac_numPassed;\n";

	// Comparison utilities.
	src << "\n";
	generateCompareFuncs(src, interface);

	src << "\n"
		   "void main (void)\n"
		   "{\n"
		   "	bool allOk = true;\n";

	// Value compare.
	generateCompareSrc(src, "allOk", interface, layout, comparePtrs);

	src << "	if (allOk)\n"
		<< "		atomicCounterIncrement(ac_numPassed);\n"
		<< "\n";

	// Value write.
	generateWriteSrc(src, interface, layout, writePtrs);

	src << "}\n";

	return src.str();
}

void getGLBufferLayout (const glw::Functions& gl, BufferLayout& layout, deUint32 program)
{
	int		numActiveBufferVars	= 0;
	int		numActiveBlocks		= 0;

	gl.getProgramInterfaceiv(program, GL_BUFFER_VARIABLE,		GL_ACTIVE_RESOURCES,	&numActiveBufferVars);
	gl.getProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK,	GL_ACTIVE_RESOURCES,	&numActiveBlocks);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to get number of buffer variables and buffer blocks");

	// Block entries.
	layout.blocks.resize(numActiveBlocks);
	for (int blockNdx = 0; blockNdx < numActiveBlocks; blockNdx++)
	{
		BlockLayoutEntry&	entry				= layout.blocks[blockNdx];
		const deUint32		queryParams[]		= { GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES, GL_NAME_LENGTH };
		int					returnValues[]		= { 0, 0, 0 };

		DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(queryParams) == DE_LENGTH_OF_ARRAY(returnValues));

		{
			int returnLength = 0;
			gl.getProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, (deUint32)blockNdx, DE_LENGTH_OF_ARRAY(queryParams), &queryParams[0], DE_LENGTH_OF_ARRAY(returnValues), &returnLength, &returnValues[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceiv(GL_SHADER_STORAGE_BLOCK) failed");

			if (returnLength != DE_LENGTH_OF_ARRAY(returnValues))
				throw tcu::TestError("glGetProgramResourceiv(GL_SHADER_STORAGE_BLOCK) returned wrong number of values");
		}

		entry.size = returnValues[0];

		// Query active variables
		if (returnValues[1] > 0)
		{
			const int		numBlockVars	= returnValues[1];
			const deUint32	queryArg		= GL_ACTIVE_VARIABLES;
			int				retLength		= 0;

			entry.activeVarIndices.resize(numBlockVars);
			gl.getProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, (deUint32)blockNdx, 1, &queryArg, numBlockVars, &retLength, &entry.activeVarIndices[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceiv(GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_VARIABLES) failed");

			if (retLength != numBlockVars)
				throw tcu::TestError("glGetProgramResourceiv(GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_VARIABLES) returned wrong number of values");
		}

		// Query name
		if (returnValues[2] > 0)
		{
			const int		nameLen		= returnValues[2];
			int				retLen		= 0;
			vector<char>	name		(nameLen);

			gl.getProgramResourceName(program, GL_SHADER_STORAGE_BLOCK, (deUint32)blockNdx, (glw::GLsizei)name.size(), &retLen, &name[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceName(GL_SHADER_STORAGE_BLOCK) failed");

			if (retLen+1 != nameLen)
				throw tcu::TestError("glGetProgramResourceName(GL_SHADER_STORAGE_BLOCK) returned invalid name. Number of characters written is inconsistent with NAME_LENGTH property.");
			if (name[nameLen-1] != 0)
				throw tcu::TestError("glGetProgramResourceName(GL_SHADER_STORAGE_BLOCK) returned invalid name. Expected null terminator at index " + de::toString(nameLen-1));

			entry.name = &name[0];
		}
		else
			throw tcu::TestError("glGetProgramResourceiv() returned invalid GL_NAME_LENGTH");
	}

	layout.bufferVars.resize(numActiveBufferVars);
	for (int bufVarNdx = 0; bufVarNdx < numActiveBufferVars; bufVarNdx++)
	{
		BufferVarLayoutEntry&	entry				= layout.bufferVars[bufVarNdx];
		const deUint32			queryParams[] =
		{
			GL_BLOCK_INDEX,					// 0
			GL_TYPE,						// 1
			GL_OFFSET,						// 2
			GL_ARRAY_SIZE,					// 3
			GL_ARRAY_STRIDE,				// 4
			GL_MATRIX_STRIDE,				// 5
			GL_TOP_LEVEL_ARRAY_SIZE,		// 6
			GL_TOP_LEVEL_ARRAY_STRIDE,		// 7
			GL_IS_ROW_MAJOR,				// 8
			GL_NAME_LENGTH					// 9
		};
		int returnValues[DE_LENGTH_OF_ARRAY(queryParams)];

		DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(queryParams) == DE_LENGTH_OF_ARRAY(returnValues));

		{
			int returnLength = 0;
			gl.getProgramResourceiv(program, GL_BUFFER_VARIABLE, (deUint32)bufVarNdx, DE_LENGTH_OF_ARRAY(queryParams), &queryParams[0], DE_LENGTH_OF_ARRAY(returnValues), &returnLength, &returnValues[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceiv(GL_BUFFER_VARIABLE) failed");

			if (returnLength != DE_LENGTH_OF_ARRAY(returnValues))
				throw tcu::TestError("glGetProgramResourceiv(GL_BUFFER_VARIABLE) returned wrong number of values");
		}

		// Map values
		entry.blockNdx				= returnValues[0];
		entry.type					= glu::getDataTypeFromGLType(returnValues[1]);
		entry.offset				= returnValues[2];
		entry.arraySize				= returnValues[3];
		entry.arrayStride			= returnValues[4];
		entry.matrixStride			= returnValues[5];
		entry.topLevelArraySize		= returnValues[6];
		entry.topLevelArrayStride	= returnValues[7];
		entry.isRowMajor			= returnValues[8] != 0;

		// Query name
		DE_ASSERT(queryParams[9] == GL_NAME_LENGTH);
		if (returnValues[9] > 0)
		{
			const int		nameLen		= returnValues[9];
			int				retLen		= 0;
			vector<char>	name		(nameLen);

			gl.getProgramResourceName(program, GL_BUFFER_VARIABLE, (deUint32)bufVarNdx, (glw::GLsizei)name.size(), &retLen, &name[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceName(GL_BUFFER_VARIABLE) failed");

			if (retLen+1 != nameLen)
				throw tcu::TestError("glGetProgramResourceName(GL_BUFFER_VARIABLE) returned invalid name. Number of characters written is inconsistent with NAME_LENGTH property.");
			if (name[nameLen-1] != 0)
				throw tcu::TestError("glGetProgramResourceName(GL_BUFFER_VARIABLE) returned invalid name. Expected null terminator at index " + de::toString(nameLen-1));

			entry.name = &name[0];
		}
		else
			throw tcu::TestError("glGetProgramResourceiv() returned invalid GL_NAME_LENGTH");
	}
}

void copyBufferVarData (const BufferVarLayoutEntry& dstEntry, const BlockDataPtr& dstBlockPtr, const BufferVarLayoutEntry& srcEntry, const BlockDataPtr& srcBlockPtr)
{
	DE_ASSERT(dstEntry.arraySize <= srcEntry.arraySize);
	DE_ASSERT(dstEntry.topLevelArraySize <= srcEntry.topLevelArraySize);
	DE_ASSERT(dstBlockPtr.lastUnsizedArraySize <= srcBlockPtr.lastUnsizedArraySize);
	DE_ASSERT(dstEntry.type == srcEntry.type);

	deUint8* const			dstBasePtr			= (deUint8*)dstBlockPtr.ptr + dstEntry.offset;
	const deUint8* const	srcBasePtr			= (const deUint8*)srcBlockPtr.ptr + srcEntry.offset;
	const int				scalarSize			= glu::getDataTypeScalarSize(dstEntry.type);
	const bool				isMatrix			= glu::isDataTypeMatrix(dstEntry.type);
	const int				compSize			= sizeof(deUint32);
	const int				dstArraySize		= dstEntry.arraySize == 0 ? dstBlockPtr.lastUnsizedArraySize : dstEntry.arraySize;
	const int				dstArrayStride		= dstEntry.arrayStride;
	const int				dstTopLevelSize		= dstEntry.topLevelArraySize == 0 ? dstBlockPtr.lastUnsizedArraySize : dstEntry.topLevelArraySize;
	const int				dstTopLevelStride	= dstEntry.topLevelArrayStride;
	const int				srcArraySize		= srcEntry.arraySize == 0 ? srcBlockPtr.lastUnsizedArraySize : srcEntry.arraySize;
	const int				srcArrayStride		= srcEntry.arrayStride;
	const int				srcTopLevelSize		= srcEntry.topLevelArraySize == 0 ? srcBlockPtr.lastUnsizedArraySize : srcEntry.topLevelArraySize;
	const int				srcTopLevelStride	= srcEntry.topLevelArrayStride;

	DE_ASSERT(dstArraySize <= srcArraySize && dstTopLevelSize <= srcTopLevelSize);
	DE_UNREF(srcArraySize && srcTopLevelSize);

	for (int topElemNdx = 0; topElemNdx < dstTopLevelSize; topElemNdx++)
	{
		deUint8* const			dstTopPtr	= dstBasePtr + topElemNdx*dstTopLevelStride;
		const deUint8* const	srcTopPtr	= srcBasePtr + topElemNdx*srcTopLevelStride;

		for (int elementNdx = 0; elementNdx < dstArraySize; elementNdx++)
		{
			deUint8* const			dstElemPtr	= dstTopPtr + elementNdx*dstArrayStride;
			const deUint8* const	srcElemPtr	= srcTopPtr + elementNdx*srcArrayStride;

			if (isMatrix)
			{
				const int	numRows	= glu::getDataTypeMatrixNumRows(dstEntry.type);
				const int	numCols	= glu::getDataTypeMatrixNumColumns(dstEntry.type);

				for (int colNdx = 0; colNdx < numCols; colNdx++)
				{
					for (int rowNdx = 0; rowNdx < numRows; rowNdx++)
					{
						deUint8*		dstCompPtr	= dstElemPtr + (dstEntry.isRowMajor ? rowNdx*dstEntry.matrixStride + colNdx*compSize
																						: colNdx*dstEntry.matrixStride + rowNdx*compSize);
						const deUint8*	srcCompPtr	= srcElemPtr + (srcEntry.isRowMajor ? rowNdx*srcEntry.matrixStride + colNdx*compSize
																						: colNdx*srcEntry.matrixStride + rowNdx*compSize);

						DE_ASSERT((deIntptr)(srcCompPtr + compSize) - (deIntptr)srcBlockPtr.ptr <= (deIntptr)srcBlockPtr.size);
						DE_ASSERT((deIntptr)(dstCompPtr + compSize) - (deIntptr)dstBlockPtr.ptr <= (deIntptr)dstBlockPtr.size);
						deMemcpy(dstCompPtr, srcCompPtr, compSize);
					}
				}
			}
			else
			{
				DE_ASSERT((deIntptr)(srcElemPtr + scalarSize*compSize) - (deIntptr)srcBlockPtr.ptr <= (deIntptr)srcBlockPtr.size);
				DE_ASSERT((deIntptr)(dstElemPtr + scalarSize*compSize) - (deIntptr)dstBlockPtr.ptr <= (deIntptr)dstBlockPtr.size);
				deMemcpy(dstElemPtr, srcElemPtr, scalarSize*compSize);
			}
		}
	}
}

void copyData (const BufferLayout& dstLayout, const vector<BlockDataPtr>& dstBlockPointers, const BufferLayout& srcLayout, const vector<BlockDataPtr>& srcBlockPointers)
{
	// \note Src layout is used as reference in case of activeVarIndices happens to be incorrect in dstLayout blocks.
	int numBlocks = (int)srcLayout.blocks.size();

	for (int srcBlockNdx = 0; srcBlockNdx < numBlocks; srcBlockNdx++)
	{
		const BlockLayoutEntry&		srcBlock	= srcLayout.blocks[srcBlockNdx];
		const BlockDataPtr&			srcBlockPtr	= srcBlockPointers[srcBlockNdx];
		int							dstBlockNdx	= dstLayout.getBlockIndex(srcBlock.name.c_str());

		if (dstBlockNdx >= 0)
		{
			DE_ASSERT(de::inBounds(dstBlockNdx, 0, (int)dstBlockPointers.size()));

			const BlockDataPtr& dstBlockPtr = dstBlockPointers[dstBlockNdx];

			for (vector<int>::const_iterator srcVarNdxIter = srcBlock.activeVarIndices.begin(); srcVarNdxIter != srcBlock.activeVarIndices.end(); srcVarNdxIter++)
			{
				const BufferVarLayoutEntry&	srcEntry	= srcLayout.bufferVars[*srcVarNdxIter];
				int							dstVarNdx	= dstLayout.getVariableIndex(srcEntry.name.c_str());

				if (dstVarNdx >= 0)
					copyBufferVarData(dstLayout.bufferVars[dstVarNdx], dstBlockPtr, srcEntry, srcBlockPtr);
			}
		}
	}
}

void copyNonWrittenData (
	const BufferLayout&			layout,
	const BufferBlock&			block,
	int							instanceNdx,
	const BlockDataPtr&			srcBlockPtr,
	const BlockDataPtr&			dstBlockPtr,
	const BufferVar&			bufVar,
	const glu::SubTypeAccess&	accessPath)
{
	const VarType curType = accessPath.getType();

	if (curType.isArrayType())
	{
		const int arraySize = curType.getArraySize() == VarType::UNSIZED_ARRAY ? block.getLastUnsizedArraySize(instanceNdx) : curType.getArraySize();

		for (int elemNdx = 0; elemNdx < arraySize; elemNdx++)
			copyNonWrittenData(layout, block, instanceNdx, srcBlockPtr, dstBlockPtr, bufVar, accessPath.element(elemNdx));
	}
	else if (curType.isStructType())
	{
		const int numMembers = curType.getStructPtr()->getNumMembers();

		for (int memberNdx = 0; memberNdx < numMembers; memberNdx++)
			copyNonWrittenData(layout, block, instanceNdx, srcBlockPtr, dstBlockPtr, bufVar, accessPath.member(memberNdx));
	}
	else
	{
		DE_ASSERT(curType.isBasicType());

		const string	apiName	= getAPIName(block, bufVar, accessPath.getPath());
		const int		varNdx	= layout.getVariableIndex(apiName);

		DE_ASSERT(varNdx >= 0);
		{
			const BufferVarLayoutEntry& varLayout = layout.bufferVars[varNdx];
			copyBufferVarData(varLayout, dstBlockPtr, varLayout, srcBlockPtr);
		}
	}
}

void copyNonWrittenData (const ShaderInterface& interface, const BufferLayout& layout, const vector<BlockDataPtr>& srcPtrs, const vector<BlockDataPtr>& dstPtrs)
{
	for (int declNdx = 0; declNdx < interface.getNumBlocks(); declNdx++)
	{
		const BufferBlock&	block			= interface.getBlock(declNdx);
		const bool			isArray			= block.isArray();
		const int			numInstances	= isArray ? block.getArraySize() : 1;

		DE_ASSERT(!isArray || block.getInstanceName());

		for (int instanceNdx = 0; instanceNdx < numInstances; instanceNdx++)
		{
			const string		instanceName	= block.getBlockName() + (isArray ? "[" + de::toString(instanceNdx) + "]" : string(""));
			const int			blockNdx		= layout.getBlockIndex(instanceName);
			const BlockDataPtr&	srcBlockPtr		= srcPtrs[blockNdx];
			const BlockDataPtr&	dstBlockPtr		= dstPtrs[blockNdx];

			for (BufferBlock::const_iterator varIter = block.begin(); varIter != block.end(); varIter++)
			{
				const BufferVar& bufVar = *varIter;

				if (bufVar.getFlags() & ACCESS_WRITE)
					continue;

				copyNonWrittenData(layout, block, instanceNdx, srcBlockPtr, dstBlockPtr, bufVar, glu::SubTypeAccess(bufVar.getType()));
			}
		}
	}
}

bool compareComponents (glu::DataType scalarType, const void* ref, const void* res, int numComps)
{
	if (scalarType == glu::TYPE_FLOAT)
	{
		const float threshold = 0.05f; // Same as used in shaders - should be fine for values being used.

		for (int ndx = 0; ndx < numComps; ndx++)
		{
			const float		refVal		= *((const float*)ref + ndx);
			const float		resVal		= *((const float*)res + ndx);

			if (!(deFloatAbs(resVal - refVal) <= threshold))
				return false;
		}
	}
	else if (scalarType == glu::TYPE_BOOL)
	{
		for (int ndx = 0; ndx < numComps; ndx++)
		{
			const deUint32	refVal		= *((const deUint32*)ref + ndx);
			const deUint32	resVal		= *((const deUint32*)res + ndx);

			if ((refVal != 0) != (resVal != 0))
				return false;
		}
	}
	else
	{
		DE_ASSERT(scalarType == glu::TYPE_INT || scalarType == glu::TYPE_UINT);

		for (int ndx = 0; ndx < numComps; ndx++)
		{
			const deUint32	refVal		= *((const deUint32*)ref + ndx);
			const deUint32	resVal		= *((const deUint32*)res + ndx);

			if (refVal != resVal)
				return false;
		}
	}

	return true;
}

bool compareBufferVarData (tcu::TestLog& log, const BufferVarLayoutEntry& refEntry, const BlockDataPtr& refBlockPtr, const BufferVarLayoutEntry& resEntry, const BlockDataPtr& resBlockPtr)
{
	DE_ASSERT(resEntry.arraySize <= refEntry.arraySize);
	DE_ASSERT(resEntry.topLevelArraySize <= refEntry.topLevelArraySize);
	DE_ASSERT(resBlockPtr.lastUnsizedArraySize <= refBlockPtr.lastUnsizedArraySize);
	DE_ASSERT(resEntry.type == refEntry.type);

	deUint8* const			resBasePtr			= (deUint8*)resBlockPtr.ptr + resEntry.offset;
	const deUint8* const	refBasePtr			= (const deUint8*)refBlockPtr.ptr + refEntry.offset;
	const glu::DataType		scalarType			= glu::getDataTypeScalarType(refEntry.type);
	const int				scalarSize			= glu::getDataTypeScalarSize(resEntry.type);
	const bool				isMatrix			= glu::isDataTypeMatrix(resEntry.type);
	const int				compSize			= sizeof(deUint32);
	const int				maxPrints			= 3;
	int						numFailed			= 0;

	const int				resArraySize		= resEntry.arraySize == 0 ? resBlockPtr.lastUnsizedArraySize : resEntry.arraySize;
	const int				resArrayStride		= resEntry.arrayStride;
	const int				resTopLevelSize		= resEntry.topLevelArraySize == 0 ? resBlockPtr.lastUnsizedArraySize : resEntry.topLevelArraySize;
	const int				resTopLevelStride	= resEntry.topLevelArrayStride;
	const int				refArraySize		= refEntry.arraySize == 0 ? refBlockPtr.lastUnsizedArraySize : refEntry.arraySize;
	const int				refArrayStride		= refEntry.arrayStride;
	const int				refTopLevelSize		= refEntry.topLevelArraySize == 0 ? refBlockPtr.lastUnsizedArraySize : refEntry.topLevelArraySize;
	const int				refTopLevelStride	= refEntry.topLevelArrayStride;

	DE_ASSERT(resArraySize <= refArraySize && resTopLevelSize <= refTopLevelSize);
	DE_UNREF(refArraySize && refTopLevelSize);

	for (int topElemNdx = 0; topElemNdx < resTopLevelSize; topElemNdx++)
	{
		deUint8* const			resTopPtr	= resBasePtr + topElemNdx*resTopLevelStride;
		const deUint8* const	refTopPtr	= refBasePtr + topElemNdx*refTopLevelStride;

		for (int elementNdx = 0; elementNdx < resArraySize; elementNdx++)
		{
			deUint8* const			resElemPtr	= resTopPtr + elementNdx*resArrayStride;
			const deUint8* const	refElemPtr	= refTopPtr + elementNdx*refArrayStride;

			if (isMatrix)
			{
				const int	numRows	= glu::getDataTypeMatrixNumRows(resEntry.type);
				const int	numCols	= glu::getDataTypeMatrixNumColumns(resEntry.type);
				bool		isOk	= true;

				for (int colNdx = 0; colNdx < numCols; colNdx++)
				{
					for (int rowNdx = 0; rowNdx < numRows; rowNdx++)
					{
						deUint8*		resCompPtr	= resElemPtr + (resEntry.isRowMajor ? rowNdx*resEntry.matrixStride + colNdx*compSize
																						: colNdx*resEntry.matrixStride + rowNdx*compSize);
						const deUint8*	refCompPtr	= refElemPtr + (refEntry.isRowMajor ? rowNdx*refEntry.matrixStride + colNdx*compSize
																						: colNdx*refEntry.matrixStride + rowNdx*compSize);

						DE_ASSERT((deIntptr)(refCompPtr + compSize) - (deIntptr)refBlockPtr.ptr <= (deIntptr)refBlockPtr.size);
						DE_ASSERT((deIntptr)(resCompPtr + compSize) - (deIntptr)resBlockPtr.ptr <= (deIntptr)resBlockPtr.size);

						isOk = isOk && compareComponents(scalarType, resCompPtr, refCompPtr, 1);
					}
				}

				if (!isOk)
				{
					numFailed += 1;
					if (numFailed < maxPrints)
					{
						std::ostringstream expected, got;
						generateImmMatrixSrc(expected, refEntry.type, refEntry.matrixStride, refEntry.isRowMajor, refElemPtr);
						generateImmMatrixSrc(got, resEntry.type, resEntry.matrixStride, resEntry.isRowMajor, resElemPtr);
						log << TestLog::Message << "ERROR: mismatch in " << refEntry.name << ", top-level ndx " << topElemNdx << ", bottom-level ndx " << elementNdx << ":\n"
												<< "  expected " << expected.str() << "\n"
												<< "  got " << got.str()
							<< TestLog::EndMessage;
					}
				}
			}
			else
			{
				DE_ASSERT((deIntptr)(refElemPtr + scalarSize*compSize) - (deIntptr)refBlockPtr.ptr <= (deIntptr)refBlockPtr.size);
				DE_ASSERT((deIntptr)(resElemPtr + scalarSize*compSize) - (deIntptr)resBlockPtr.ptr <= (deIntptr)resBlockPtr.size);

				const bool isOk = compareComponents(scalarType, resElemPtr, refElemPtr, scalarSize);

				if (!isOk)
				{
					numFailed += 1;
					if (numFailed < maxPrints)
					{
						std::ostringstream expected, got;
						generateImmScalarVectorSrc(expected, refEntry.type, refElemPtr);
						generateImmScalarVectorSrc(got, resEntry.type, resElemPtr);
						log << TestLog::Message << "ERROR: mismatch in " << refEntry.name << ", top-level ndx " << topElemNdx << ", bottom-level ndx " << elementNdx << ":\n"
												<< "  expected " << expected.str() << "\n"
												<< "  got " << got.str()
							<< TestLog::EndMessage;
					}
				}
			}
		}
	}

	if (numFailed >= maxPrints)
		log << TestLog::Message << "... (" << numFailed << " failures for " << refEntry.name << " in total)" << TestLog::EndMessage;

	return numFailed == 0;
}

bool compareData (tcu::TestLog& log, const BufferLayout& refLayout, const vector<BlockDataPtr>& refBlockPointers, const BufferLayout& resLayout, const vector<BlockDataPtr>& resBlockPointers)
{
	const int	numBlocks	= (int)refLayout.blocks.size();
	bool		allOk		= true;

	for (int refBlockNdx = 0; refBlockNdx < numBlocks; refBlockNdx++)
	{
		const BlockLayoutEntry&		refBlock	= refLayout.blocks[refBlockNdx];
		const BlockDataPtr&			refBlockPtr	= refBlockPointers[refBlockNdx];
		int							resBlockNdx	= resLayout.getBlockIndex(refBlock.name.c_str());

		if (resBlockNdx >= 0)
		{
			DE_ASSERT(de::inBounds(resBlockNdx, 0, (int)resBlockPointers.size()));

			const BlockDataPtr& resBlockPtr = resBlockPointers[resBlockNdx];

			for (vector<int>::const_iterator refVarNdxIter = refBlock.activeVarIndices.begin(); refVarNdxIter != refBlock.activeVarIndices.end(); refVarNdxIter++)
			{
				const BufferVarLayoutEntry&	refEntry	= refLayout.bufferVars[*refVarNdxIter];
				int							resVarNdx	= resLayout.getVariableIndex(refEntry.name.c_str());

				if (resVarNdx >= 0)
				{
					const BufferVarLayoutEntry& resEntry = resLayout.bufferVars[resVarNdx];
					allOk = compareBufferVarData(log, refEntry, refBlockPtr, resEntry, resBlockPtr) && allOk;
				}
			}
		}
	}

	return allOk;
}

string getBlockAPIName (const BufferBlock& block, int instanceNdx)
{
	DE_ASSERT(block.isArray() || instanceNdx == 0);
	return block.getBlockName() + (block.isArray() ? ("[" + de::toString(instanceNdx) + "]") : string());
}

// \note Some implementations don't report block members in the order they are declared.
//		 For checking whether size has to be adjusted by some top-level array actual size,
//		 we only need to know a) whether there is a unsized top-level array, and b)
//		 what is stride of that array.

static bool hasUnsizedArray (const BufferLayout& layout, const BlockLayoutEntry& entry)
{
	for (vector<int>::const_iterator varNdx = entry.activeVarIndices.begin(); varNdx != entry.activeVarIndices.end(); ++varNdx)
	{
		if (isUnsizedArray(layout.bufferVars[*varNdx]))
			return true;
	}

	return false;
}

static int getUnsizedArrayStride (const BufferLayout& layout, const BlockLayoutEntry& entry)
{
	for (vector<int>::const_iterator varNdx = entry.activeVarIndices.begin(); varNdx != entry.activeVarIndices.end(); ++varNdx)
	{
		const BufferVarLayoutEntry& varEntry = layout.bufferVars[*varNdx];

		if (varEntry.arraySize == 0)
			return varEntry.arrayStride;
		else if (varEntry.topLevelArraySize == 0)
			return varEntry.topLevelArrayStride;
	}

	return 0;
}

vector<int> computeBufferSizes (const ShaderInterface& interface, const BufferLayout& layout)
{
	vector<int> sizes(layout.blocks.size());

	for (int declNdx = 0; declNdx < interface.getNumBlocks(); declNdx++)
	{
		const BufferBlock&	block			= interface.getBlock(declNdx);
		const bool			isArray			= block.isArray();
		const int			numInstances	= isArray ? block.getArraySize() : 1;

		for (int instanceNdx = 0; instanceNdx < numInstances; instanceNdx++)
		{
			const string	apiName		= getBlockAPIName(block, instanceNdx);
			const int		blockNdx	= layout.getBlockIndex(apiName);

			if (blockNdx >= 0)
			{
				const BlockLayoutEntry&		blockLayout		= layout.blocks[blockNdx];
				const int					baseSize		= blockLayout.size;
				const bool					isLastUnsized	= hasUnsizedArray(layout, blockLayout);
				const int					lastArraySize	= isLastUnsized ? block.getLastUnsizedArraySize(instanceNdx) : 0;
				const int					stride			= isLastUnsized ? getUnsizedArrayStride(layout, blockLayout) : 0;

				sizes[blockNdx] = baseSize + lastArraySize*stride;
			}
		}
	}

	return sizes;
}

BlockDataPtr getBlockDataPtr (const BufferLayout& layout, const BlockLayoutEntry& blockLayout, void* ptr, int bufferSize)
{
	const bool	isLastUnsized	= hasUnsizedArray(layout, blockLayout);
	const int	baseSize		= blockLayout.size;

	if (isLastUnsized)
	{
		const int		lastArrayStride	= getUnsizedArrayStride(layout, blockLayout);
		const int		lastArraySize	= (bufferSize-baseSize) / (lastArrayStride ? lastArrayStride : 1);

		DE_ASSERT(baseSize + lastArraySize*lastArrayStride == bufferSize);

		return BlockDataPtr(ptr, bufferSize, lastArraySize);
	}
	else
		return BlockDataPtr(ptr, bufferSize, 0);
}

struct RefDataStorage
{
	vector<deUint8>			data;
	vector<BlockDataPtr>	pointers;
};

struct Buffer
{
	deUint32				buffer;
	int						size;

	Buffer (deUint32 buffer_, int size_) : buffer(buffer_), size(size_) {}
	Buffer (void) : buffer(0), size(0) {}
};

struct BlockLocation
{
	int						index;
	int						offset;
	int						size;

	BlockLocation (int index_, int offset_, int size_) : index(index_), offset(offset_), size(size_) {}
	BlockLocation (void) : index(0), offset(0), size(0) {}
};

void initRefDataStorage (const ShaderInterface& interface, const BufferLayout& layout, RefDataStorage& storage)
{
	DE_ASSERT(storage.data.empty() && storage.pointers.empty());

	const vector<int>	bufferSizes = computeBufferSizes(interface, layout);
	int					totalSize	= 0;

	for (vector<int>::const_iterator sizeIter = bufferSizes.begin(); sizeIter != bufferSizes.end(); ++sizeIter)
		totalSize += *sizeIter;

	storage.data.resize(totalSize);

	// Pointers for each block.
	{
		deUint8*	basePtr		= storage.data.empty() ? DE_NULL : &storage.data[0];
		int			curOffset	= 0;

		DE_ASSERT(bufferSizes.size() == layout.blocks.size());
		DE_ASSERT(totalSize == 0 || basePtr);

		storage.pointers.resize(layout.blocks.size());

		for (int blockNdx = 0; blockNdx < (int)layout.blocks.size(); blockNdx++)
		{
			const BlockLayoutEntry&	blockLayout		= layout.blocks[blockNdx];
			const int				bufferSize		= bufferSizes[blockNdx];

			storage.pointers[blockNdx] = getBlockDataPtr(layout, blockLayout, basePtr + curOffset, bufferSize);

			curOffset += bufferSize;
		}
	}
}

vector<BlockDataPtr> blockLocationsToPtrs (const BufferLayout& layout, const vector<BlockLocation>& blockLocations, const vector<void*>& bufPtrs)
{
	vector<BlockDataPtr> blockPtrs(blockLocations.size());

	DE_ASSERT(layout.blocks.size() == blockLocations.size());

	for (int blockNdx = 0; blockNdx < (int)layout.blocks.size(); blockNdx++)
	{
		const BlockLayoutEntry&	blockLayout		= layout.blocks[blockNdx];
		const BlockLocation&	location		= blockLocations[blockNdx];

		blockPtrs[blockNdx] = getBlockDataPtr(layout, blockLayout, (deUint8*)bufPtrs[location.index] + location.offset, location.size);
	}

	return blockPtrs;
}

vector<void*> mapBuffers (const glw::Functions& gl, const vector<Buffer>& buffers, deUint32 access)
{
	vector<void*> mapPtrs(buffers.size(), DE_NULL);

	try
	{
		for (int ndx = 0; ndx < (int)buffers.size(); ndx++)
		{
			if (buffers[ndx].size > 0)
			{
				gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[ndx].buffer);
				mapPtrs[ndx] = gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, buffers[ndx].size, access);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to map buffer");
				TCU_CHECK(mapPtrs[ndx]);
			}
			else
				mapPtrs[ndx] = DE_NULL;
		}

		return mapPtrs;
	}
	catch (...)
	{
		for (int ndx = 0; ndx < (int)buffers.size(); ndx++)
		{
			if (mapPtrs[ndx])
			{
				gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[ndx].buffer);
				gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
			}
		}

		throw;
	}
}

void unmapBuffers (const glw::Functions& gl, const vector<Buffer>& buffers)
{
	for (int ndx = 0; ndx < (int)buffers.size(); ndx++)
	{
		if (buffers[ndx].size > 0)
		{
			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[ndx].buffer);
			gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to unmap buffer");
}

} // anonymous (utilities)

class BufferManager
{
public:
								BufferManager	(const glu::RenderContext& renderCtx);
								~BufferManager	(void);

	deUint32					allocBuffer		(void);

private:
								BufferManager	(const BufferManager& other);
	BufferManager&				operator=		(const BufferManager& other);

	const glu::RenderContext&	m_renderCtx;
	std::vector<deUint32>		m_buffers;
};

BufferManager::BufferManager (const glu::RenderContext& renderCtx)
	: m_renderCtx(renderCtx)
{
}

BufferManager::~BufferManager (void)
{
	if (!m_buffers.empty())
		m_renderCtx.getFunctions().deleteBuffers((glw::GLsizei)m_buffers.size(), &m_buffers[0]);
}

deUint32 BufferManager::allocBuffer (void)
{
	deUint32 buf = 0;

	m_buffers.reserve(m_buffers.size()+1);
	m_renderCtx.getFunctions().genBuffers(1, &buf);
	GLU_EXPECT_NO_ERROR(m_renderCtx.getFunctions().getError(), "Failed to allocate buffer");
	m_buffers.push_back(buf);

	return buf;
}

} // bb

using namespace bb;

// SSBOLayoutCase.

SSBOLayoutCase::SSBOLayoutCase (tcu::TestContext& testCtx, glu::RenderContext& renderCtx, const char* name, const char* description, glu::GLSLVersion glslVersion, BufferMode bufferMode)
	: TestCase		(testCtx, name, description)
	, m_renderCtx	(renderCtx)
	, m_glslVersion	(glslVersion)
	, m_bufferMode	(bufferMode)
{
	DE_ASSERT(glslVersion == glu::GLSL_VERSION_310_ES || glslVersion == glu::GLSL_VERSION_430);
}

SSBOLayoutCase::~SSBOLayoutCase (void)
{
}

SSBOLayoutCase::IterateResult SSBOLayoutCase::iterate (void)
{
	TestLog&					log				= m_testCtx.getLog();
	const glw::Functions&		gl				= m_renderCtx.getFunctions();

	BufferLayout				refLayout;		// std140 / std430 layout.
	BufferLayout				glLayout;		// Layout reported by GL.
	RefDataStorage				initialData;	// Initial data stored in buffer.
	RefDataStorage				writeData;		// Data written by compute shader.

	BufferManager				bufferManager	(m_renderCtx);
	vector<Buffer>				buffers;		// Buffers allocated for storage
	vector<BlockLocation>		blockLocations;	// Block locations in storage (index, offset)

	// Initialize result to pass.
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	computeReferenceLayout	(refLayout, m_interface);
	initRefDataStorage		(m_interface, refLayout, initialData);
	initRefDataStorage		(m_interface, refLayout, writeData);
	generateValues			(refLayout, initialData.pointers, deStringHash(getName()) ^ 0xad2f7214);
	generateValues			(refLayout, writeData.pointers, deStringHash(getName()) ^ 0x25ca4e7);
	copyNonWrittenData		(m_interface, refLayout, initialData.pointers, writeData.pointers);

	const glu::ShaderProgram program(m_renderCtx, glu::ProgramSources() << glu::ComputeSource(generateComputeShader(m_glslVersion, m_interface, refLayout, initialData.pointers, writeData.pointers)));
	log << program;

	if (!program.isOk())
	{
		// Compile failed.
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Compile failed");
		return STOP;
	}

	// Query layout from GL.
	getGLBufferLayout(gl, glLayout, program.getProgram());

	// Print layout to log.
	{
		tcu::ScopedLogSection section(log, "ActiveBufferBlocks", "Active Buffer Blocks");
		for (int blockNdx = 0; blockNdx < (int)glLayout.blocks.size(); blockNdx++)
			log << TestLog::Message << blockNdx << ": " << glLayout.blocks[blockNdx] << TestLog::EndMessage;
	}

	{
		tcu::ScopedLogSection section(log, "ActiveBufferVars", "Active Buffer Variables");
		for (int varNdx = 0; varNdx < (int)glLayout.bufferVars.size(); varNdx++)
			log << TestLog::Message << varNdx << ": " << glLayout.bufferVars[varNdx] << TestLog::EndMessage;
	}

	// Verify layouts.
	{
		if (!checkLayoutIndices(glLayout) || !checkLayoutBounds(glLayout) || !compareTypes(refLayout, glLayout))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid layout");
			return STOP; // It is not safe to use the given layout.
		}

		if (!compareStdBlocks(refLayout, glLayout))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid std140 or std430 layout");

		if (!compareSharedBlocks(refLayout, glLayout))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid shared layout");

		if (!checkIndexQueries(program.getProgram(), glLayout))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Inconsintent block index query results");
	}

	// Allocate GL buffers & compute placement.
	{
		const int			numBlocks		= (int)glLayout.blocks.size();
		const vector<int>	bufferSizes		= computeBufferSizes(m_interface, glLayout);

		DE_ASSERT(bufferSizes.size() == glLayout.blocks.size());

		blockLocations.resize(numBlocks);

		if (m_bufferMode == BUFFERMODE_PER_BLOCK)
		{
			buffers.resize(numBlocks);

			for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
			{
				const int bufferSize = bufferSizes[blockNdx];

				buffers[blockNdx].size = bufferSize;
				blockLocations[blockNdx] = BlockLocation(blockNdx, 0, bufferSize);
			}
		}
		else
		{
			DE_ASSERT(m_bufferMode == BUFFERMODE_SINGLE);

			int		bindingAlignment	= 0;
			int		totalSize			= 0;

			gl.getIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &bindingAlignment);

			{
				int curOffset = 0;
				DE_ASSERT(bufferSizes.size() == glLayout.blocks.size());
				for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
				{
					const int bufferSize = bufferSizes[blockNdx];

					if (bindingAlignment > 0)
						curOffset = deRoundUp32(curOffset, bindingAlignment);

					blockLocations[blockNdx] = BlockLocation(0, curOffset, bufferSize);
					curOffset += bufferSize;
				}
				totalSize = curOffset;
			}

			buffers.resize(1);
			buffers[0].size = totalSize;
		}

		for (int bufNdx = 0; bufNdx < (int)buffers.size(); bufNdx++)
		{
			const int		bufferSize	= buffers[bufNdx].size;
			const deUint32	buffer		= bufferManager.allocBuffer();

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, DE_NULL, GL_STATIC_DRAW);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to allocate buffer");

			buffers[bufNdx].buffer = buffer;
		}
	}

	{
		const vector<void*>			mapPtrs			= mapBuffers(gl, buffers, GL_MAP_WRITE_BIT);
		const vector<BlockDataPtr>	mappedBlockPtrs	= blockLocationsToPtrs(glLayout, blockLocations, mapPtrs);

		copyData(glLayout, mappedBlockPtrs, refLayout, initialData.pointers);

		unmapBuffers(gl, buffers);
	}

	{
		int bindingPoint = 0;

		for (int blockDeclNdx = 0; blockDeclNdx < m_interface.getNumBlocks(); blockDeclNdx++)
		{
			const BufferBlock&	block		= m_interface.getBlock(blockDeclNdx);
			const int			numInst		= block.isArray() ? block.getArraySize() : 1;

			for (int instNdx = 0; instNdx < numInst; instNdx++)
			{
				const string	instName	= getBlockAPIName(block, instNdx);
				const int		layoutNdx	= findBlockIndex(glLayout, instName);

				if (layoutNdx >= 0)
				{
					const BlockLocation& blockLoc = blockLocations[layoutNdx];

					if (blockLoc.size > 0)
						gl.bindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingPoint, buffers[blockLoc.index].buffer, blockLoc.offset, blockLoc.size);
				}

				bindingPoint += 1;
			}
		}
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to bind buffers");

	{
		const bool execOk = execute(program.getProgram());

		if (execOk)
		{
			const vector<void*>			mapPtrs			= mapBuffers(gl, buffers, GL_MAP_READ_BIT);
			const vector<BlockDataPtr>	mappedBlockPtrs	= blockLocationsToPtrs(glLayout, blockLocations, mapPtrs);

			const bool					compareOk		= compareData(m_testCtx.getLog(), refLayout, writeData.pointers, glLayout, mappedBlockPtrs);

			unmapBuffers(gl, buffers);

			if (!compareOk)
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Result comparison failed");
		}
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Shader execution failed");
	}

	return STOP;
}

bool SSBOLayoutCase::compareStdBlocks (const BufferLayout& refLayout, const BufferLayout& cmpLayout) const
{
	TestLog&	log			= m_testCtx.getLog();
	bool		isOk		= true;
	int			numBlocks	= m_interface.getNumBlocks();

	for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
	{
		const BufferBlock&		block			= m_interface.getBlock(blockNdx);
		bool					isArray			= block.isArray();
		std::string				instanceName	= string(block.getBlockName()) + (isArray ? "[0]" : "");
		int						refBlockNdx		= refLayout.getBlockIndex(instanceName.c_str());
		int						cmpBlockNdx		= cmpLayout.getBlockIndex(instanceName.c_str());

		if ((block.getFlags() & (LAYOUT_STD140|LAYOUT_STD430)) == 0)
			continue; // Not std* layout.

		DE_ASSERT(refBlockNdx >= 0);

		if (cmpBlockNdx < 0)
		{
			// Not found.
			log << TestLog::Message << "Error: Buffer block '" << instanceName << "' not found" << TestLog::EndMessage;
			isOk = false;
			continue;
		}

		const BlockLayoutEntry&		refBlockLayout	= refLayout.blocks[refBlockNdx];
		const BlockLayoutEntry&		cmpBlockLayout	= cmpLayout.blocks[cmpBlockNdx];

		// \todo [2012-01-24 pyry] Verify that activeVarIndices is correct.
		// \todo [2012-01-24 pyry] Verify all instances.
		if (refBlockLayout.activeVarIndices.size() != cmpBlockLayout.activeVarIndices.size())
		{
			log << TestLog::Message << "Error: Number of active variables differ in block '" << instanceName
				<< "' (expected " << refBlockLayout.activeVarIndices.size()
				<< ", got " << cmpBlockLayout.activeVarIndices.size()
				<< ")" << TestLog::EndMessage;
			isOk = false;
		}

		for (vector<int>::const_iterator ndxIter = refBlockLayout.activeVarIndices.begin(); ndxIter != refBlockLayout.activeVarIndices.end(); ndxIter++)
		{
			const BufferVarLayoutEntry&	refEntry	= refLayout.bufferVars[*ndxIter];
			int							cmpEntryNdx	= cmpLayout.getVariableIndex(refEntry.name.c_str());

			if (cmpEntryNdx < 0)
			{
				log << TestLog::Message << "Error: Buffer variable '" << refEntry.name << "' not found" << TestLog::EndMessage;
				isOk = false;
				continue;
			}

			const BufferVarLayoutEntry&	cmpEntry	= cmpLayout.bufferVars[cmpEntryNdx];

			if (refEntry.type					!= cmpEntry.type				||
				refEntry.arraySize				!= cmpEntry.arraySize			||
				refEntry.offset					!= cmpEntry.offset				||
				refEntry.arrayStride			!= cmpEntry.arrayStride			||
				refEntry.matrixStride			!= cmpEntry.matrixStride		||
				refEntry.topLevelArraySize		!= cmpEntry.topLevelArraySize	||
				refEntry.topLevelArrayStride	!= cmpEntry.topLevelArrayStride	||
				refEntry.isRowMajor				!= cmpEntry.isRowMajor)
			{
				log << TestLog::Message << "Error: Layout mismatch in '" << refEntry.name << "':\n"
					<< "  expected: " << refEntry << "\n"
					<< "  got: " << cmpEntry
					<< TestLog::EndMessage;
				isOk = false;
			}
		}
	}

	return isOk;
}

bool SSBOLayoutCase::compareSharedBlocks (const BufferLayout& refLayout, const BufferLayout& cmpLayout) const
{
	TestLog&	log			= m_testCtx.getLog();
	bool		isOk		= true;
	int			numBlocks	= m_interface.getNumBlocks();

	for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
	{
		const BufferBlock&		block			= m_interface.getBlock(blockNdx);
		bool					isArray			= block.isArray();
		std::string				instanceName	= string(block.getBlockName()) + (isArray ? "[0]" : "");
		int						refBlockNdx		= refLayout.getBlockIndex(instanceName.c_str());
		int						cmpBlockNdx		= cmpLayout.getBlockIndex(instanceName.c_str());

		if ((block.getFlags() & LAYOUT_SHARED) == 0)
			continue; // Not shared layout.

		DE_ASSERT(refBlockNdx >= 0);

		if (cmpBlockNdx < 0)
		{
			// Not found, should it?
			log << TestLog::Message << "Error: Buffer block '" << instanceName << "' not found" << TestLog::EndMessage;
			isOk = false;
			continue;
		}

		const BlockLayoutEntry&		refBlockLayout	= refLayout.blocks[refBlockNdx];
		const BlockLayoutEntry&		cmpBlockLayout	= cmpLayout.blocks[cmpBlockNdx];

		if (refBlockLayout.activeVarIndices.size() != cmpBlockLayout.activeVarIndices.size())
		{
			log << TestLog::Message << "Error: Number of active variables differ in block '" << instanceName
				<< "' (expected " << refBlockLayout.activeVarIndices.size()
				<< ", got " << cmpBlockLayout.activeVarIndices.size()
				<< ")" << TestLog::EndMessage;
			isOk = false;
		}

		for (vector<int>::const_iterator ndxIter = refBlockLayout.activeVarIndices.begin(); ndxIter != refBlockLayout.activeVarIndices.end(); ndxIter++)
		{
			const BufferVarLayoutEntry&	refEntry	= refLayout.bufferVars[*ndxIter];
			int							cmpEntryNdx	= cmpLayout.getVariableIndex(refEntry.name.c_str());

			if (cmpEntryNdx < 0)
			{
				log << TestLog::Message << "Error: Buffer variable '" << refEntry.name << "' not found" << TestLog::EndMessage;
				isOk = false;
				continue;
			}

			const BufferVarLayoutEntry&	cmpEntry	= cmpLayout.bufferVars[cmpEntryNdx];

			if (refEntry.type				!= cmpEntry.type				||
				refEntry.arraySize			!= cmpEntry.arraySize			||
				refEntry.topLevelArraySize	!= cmpEntry.topLevelArraySize	||
				refEntry.isRowMajor	!= cmpEntry.isRowMajor)
			{
				log << TestLog::Message << "Error: Type / array size mismatch in '" << refEntry.name << "':\n"
					<< "  expected: " << refEntry << "\n"
					<< "  got: " << cmpEntry
					<< TestLog::EndMessage;
				isOk = false;
			}
		}
	}

	return isOk;
}

bool SSBOLayoutCase::compareTypes (const BufferLayout& refLayout, const BufferLayout& cmpLayout) const
{
	TestLog&	log			= m_testCtx.getLog();
	bool		isOk		= true;
	int			numBlocks	= m_interface.getNumBlocks();

	for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
	{
		const BufferBlock&		block			= m_interface.getBlock(blockNdx);
		bool					isArray			= block.isArray();
		int						numInstances	= isArray ? block.getArraySize() : 1;

		for (int instanceNdx = 0; instanceNdx < numInstances; instanceNdx++)
		{
			std::ostringstream instanceName;

			instanceName << block.getBlockName();
			if (isArray)
				instanceName << "[" << instanceNdx << "]";

			int cmpBlockNdx = cmpLayout.getBlockIndex(instanceName.str().c_str());

			if (cmpBlockNdx < 0)
				continue;

			const BlockLayoutEntry& cmpBlockLayout = cmpLayout.blocks[cmpBlockNdx];

			for (vector<int>::const_iterator ndxIter = cmpBlockLayout.activeVarIndices.begin(); ndxIter != cmpBlockLayout.activeVarIndices.end(); ndxIter++)
			{
				const BufferVarLayoutEntry&	cmpEntry	= cmpLayout.bufferVars[*ndxIter];
				int							refEntryNdx	= refLayout.getVariableIndex(cmpEntry.name.c_str());

				if (refEntryNdx < 0)
				{
					log << TestLog::Message << "Error: Buffer variable '" << cmpEntry.name << "' not found in reference layout" << TestLog::EndMessage;
					isOk = false;
					continue;
				}

				const BufferVarLayoutEntry&	refEntry	= refLayout.bufferVars[refEntryNdx];

				if (refEntry.type != cmpEntry.type)
				{
					log << TestLog::Message << "Error: Buffer variable type mismatch in '" << refEntry.name << "':\n"
						<< "  expected: " << glu::getDataTypeName(refEntry.type) << "\n"
						<< "  got: " << glu::getDataTypeName(cmpEntry.type)
						<< TestLog::EndMessage;
					isOk = false;
				}

				if (refEntry.arraySize < cmpEntry.arraySize)
				{
					log << TestLog::Message << "Error: Invalid array size in '" << refEntry.name << "': expected <= " << refEntry.arraySize << TestLog::EndMessage;
					isOk = false;
				}

				if (refEntry.topLevelArraySize < cmpEntry.topLevelArraySize)
				{
					log << TestLog::Message << "Error: Invalid top-level array size in '" << refEntry.name << "': expected <= " << refEntry.topLevelArraySize << TestLog::EndMessage;
					isOk = false;
				}
			}
		}
	}

	return isOk;
}

bool SSBOLayoutCase::checkLayoutIndices (const BufferLayout& layout) const
{
	TestLog&	log			= m_testCtx.getLog();
	int			numVars		= (int)layout.bufferVars.size();
	int			numBlocks	= (int)layout.blocks.size();
	bool		isOk		= true;

	// Check variable block indices.
	for (int varNdx = 0; varNdx < numVars; varNdx++)
	{
		const BufferVarLayoutEntry& bufVar = layout.bufferVars[varNdx];

		if (bufVar.blockNdx < 0 || !deInBounds32(bufVar.blockNdx, 0, numBlocks))
		{
			log << TestLog::Message << "Error: Invalid block index in buffer variable '" << bufVar.name << "'" << TestLog::EndMessage;
			isOk = false;
		}
	}

	// Check active variables.
	for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
	{
		const BlockLayoutEntry& block = layout.blocks[blockNdx];

		for (vector<int>::const_iterator varNdxIter = block.activeVarIndices.begin(); varNdxIter != block.activeVarIndices.end(); varNdxIter++)
		{
			if (!deInBounds32(*varNdxIter, 0, numVars))
			{
				log << TestLog::Message << "Error: Invalid active variable index " << *varNdxIter << " in block '" << block.name << "'" << TestLog::EndMessage;
				isOk = false;
			}
		}
	}

	return isOk;
}

bool SSBOLayoutCase::checkLayoutBounds (const BufferLayout& layout) const
{
	TestLog&	log			= m_testCtx.getLog();
	const int	numVars		= (int)layout.bufferVars.size();
	bool		isOk		= true;

	for (int varNdx = 0; varNdx < numVars; varNdx++)
	{
		const BufferVarLayoutEntry& var = layout.bufferVars[varNdx];

		if (var.blockNdx < 0 || isUnsizedArray(var))
			continue;

		const BlockLayoutEntry&		block			= layout.blocks[var.blockNdx];
		const bool					isMatrix		= glu::isDataTypeMatrix(var.type);
		const int					numVecs			= isMatrix ? (var.isRowMajor ? glu::getDataTypeMatrixNumRows(var.type) : glu::getDataTypeMatrixNumColumns(var.type)) : 1;
		const int					numComps		= isMatrix ? (var.isRowMajor ? glu::getDataTypeMatrixNumColumns(var.type) : glu::getDataTypeMatrixNumRows(var.type)) : glu::getDataTypeScalarSize(var.type);
		const int					numElements		= var.arraySize;
		const int					topLevelSize	= var.topLevelArraySize;
		const int					arrayStride		= var.arrayStride;
		const int					topLevelStride	= var.topLevelArrayStride;
		const int					compSize		= sizeof(deUint32);
		const int					vecSize			= numComps*compSize;

		int							minOffset		= 0;
		int							maxOffset		= 0;

		// For negative strides.
		minOffset	= de::min(minOffset, (numVecs-1)*var.matrixStride);
		minOffset	= de::min(minOffset, (numElements-1)*arrayStride);
		minOffset	= de::min(minOffset, (topLevelSize-1)*topLevelStride + (numElements-1)*arrayStride + (numVecs-1)*var.matrixStride);

		maxOffset	= de::max(maxOffset, vecSize);
		maxOffset	= de::max(maxOffset, (numVecs-1)*var.matrixStride + vecSize);
		maxOffset	= de::max(maxOffset, (numElements-1)*arrayStride + vecSize);
		maxOffset	= de::max(maxOffset, (topLevelSize-1)*topLevelStride + (numElements-1)*arrayStride + vecSize);
		maxOffset	= de::max(maxOffset, (topLevelSize-1)*topLevelStride + (numElements-1)*arrayStride + (numVecs-1)*var.matrixStride + vecSize);

		if (var.offset+minOffset < 0 || var.offset+maxOffset > block.size)
		{
			log << TestLog::Message << "Error: Variable '" << var.name << "' out of block bounds" << TestLog::EndMessage;
			isOk = false;
		}
	}

	return isOk;
}

bool SSBOLayoutCase::checkIndexQueries (deUint32 program, const BufferLayout& layout) const
{
	tcu::TestLog&				log			= m_testCtx.getLog();
	const glw::Functions&		gl			= m_renderCtx.getFunctions();
	bool						allOk		= true;

	// \note Spec mandates that buffer blocks are assigned consecutive locations from 0.
	//		 BlockLayoutEntries are stored in that order in UniformLayout.
	for (int blockNdx = 0; blockNdx < (int)layout.blocks.size(); blockNdx++)
	{
		const BlockLayoutEntry&		block		= layout.blocks[blockNdx];
		const int					queriedNdx	= gl.getProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, block.name.c_str());

		if (queriedNdx != blockNdx)
		{
			log << TestLog::Message << "ERROR: glGetProgramResourceIndex(" << block.name << ") returned " << queriedNdx << ", expected " << blockNdx << "!" << TestLog::EndMessage;
			allOk = false;
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformBlockIndex()");
	}

	return allOk;
}

bool SSBOLayoutCase::execute (deUint32 program)
{
	const glw::Functions&				gl				= m_renderCtx.getFunctions();
	const deUint32						numPassedLoc	= gl.getProgramResourceIndex(program, GL_UNIFORM, "ac_numPassed");
	const glu::InterfaceVariableInfo	acVarInfo		= numPassedLoc != GL_INVALID_INDEX ? glu::getProgramInterfaceVariableInfo(gl, program, GL_UNIFORM, numPassedLoc)
																						   : glu::InterfaceVariableInfo();
	const glu::InterfaceBlockInfo		acBufferInfo	= acVarInfo.atomicCounterBufferIndex != GL_INVALID_INDEX ? glu::getProgramInterfaceBlockInfo(gl, program, GL_ATOMIC_COUNTER_BUFFER, acVarInfo.atomicCounterBufferIndex)
																												 : glu::InterfaceBlockInfo();
	const glu::Buffer					acBuffer		(m_renderCtx);
	bool								isOk			= true;

	if (numPassedLoc == GL_INVALID_INDEX)
		throw tcu::TestError("No location for ac_numPassed found");

	if (acBufferInfo.index == GL_INVALID_INDEX)
		throw tcu::TestError("ac_numPassed buffer index is GL_INVALID_INDEX");

	if (acBufferInfo.dataSize == 0)
		throw tcu::TestError("ac_numPassed buffer size = 0");

	// Initialize atomic counter buffer.
	{
		vector<deUint8> emptyData(acBufferInfo.dataSize, 0);

		gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, *acBuffer);
		gl.bufferData(GL_ATOMIC_COUNTER_BUFFER, (glw::GLsizeiptr)emptyData.size(), &emptyData[0], GL_STATIC_READ);
		gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, acBufferInfo.index, *acBuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Setting up buffer for ac_numPassed failed");
	}

	gl.useProgram(program);
	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute() failed");

	// Read back ac_numPassed data.
	{
		const void*	mapPtr		= gl.mapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, acBufferInfo.dataSize, GL_MAP_READ_BIT);
		const int	refCount	= 1;
		int			resCount	= 0;

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER) failed");
		TCU_CHECK(mapPtr);

		resCount = *(const int*)((const deUint8*)mapPtr + acVarInfo.offset);

		gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER) failed");

		if (refCount != resCount)
		{
			m_testCtx.getLog() << TestLog::Message << "ERROR: ac_numPassed = " << resCount << ", expected " << refCount << TestLog::EndMessage;
			isOk = false;
		}
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Shader execution failed");

	return isOk;
}

} // gles31
} // deqp
