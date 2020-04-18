#ifndef _VKTSSBOLAYOUTCASE_HPP
#define _VKTSSBOLAYOUTCASE_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
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
 * \brief SSBO layout tests.
 *//*--------------------------------------------------------------------*/

#include "vktTestCase.hpp"
#include "tcuDefs.hpp"
#include "gluShaderUtil.hpp"
#include "gluVarType.hpp"

namespace vkt
{

namespace ssbo
{

enum BufferVarFlags
{
	LAYOUT_STD140		= (1<<0),
	LAYOUT_STD430		= (1<<1),
	LAYOUT_ROW_MAJOR	= (1<<2),
	LAYOUT_COLUMN_MAJOR	= (1<<3),	//!< \note Lack of both flags means column-major matrix.
	LAYOUT_MASK			= LAYOUT_STD430|LAYOUT_STD140|LAYOUT_ROW_MAJOR|LAYOUT_COLUMN_MAJOR,

	// \todo [2013-10-14 pyry] Investigate adding these.
/*	QUALIFIER_COHERENT	= (1<<4),
	QUALIFIER_VOLATILE	= (1<<5),
	QUALIFIER_RESTRICT	= (1<<6),
	QUALIFIER_READONLY	= (1<<7),
	QUALIFIER_WRITEONLY	= (1<<8),*/
	ACCESS_READ			= (1<<9),	//!< Buffer variable is read in the shader.
	ACCESS_WRITE		= (1<<10),	//!< Buffer variable is written in the shader.
	LAYOUT_RELAXED		= (1<<11),	//!< Support VK_KHR_relaxed_block_layout extension
};

enum MatrixLoadFlags
{
	LOAD_FULL_MATRIX		= 0,
	LOAD_MATRIX_COMPONENTS	= 1,
};

class BufferVar
{
public:
						BufferVar		(const char* name, const glu::VarType& type, deUint32 flags);

	const char*			getName			(void) const		{ return m_name.c_str();	}
	const glu::VarType&	getType			(void) const		{ return m_type;			}
	deUint32			getFlags		(void) const		{ return m_flags;			}
	deUint32			getOffset		(void) const		{ return m_offset;			}

	void				setOffset		(deUint32 offset)	{ m_offset = offset;		}

private:
	std::string			m_name;
	glu::VarType		m_type;
	deUint32			m_flags;
	deUint32			m_offset;
};

class BufferBlock
{
public:
	typedef std::vector<BufferVar>::iterator		iterator;
	typedef std::vector<BufferVar>::const_iterator	const_iterator;

							BufferBlock				(const char* blockName);

	const char*				getBlockName			(void) const				{ return m_blockName.c_str();										}
	const char*				getInstanceName			(void) const				{ return m_instanceName.empty() ? DE_NULL : m_instanceName.c_str();	}
	bool					isArray					(void) const				{ return m_arraySize > 0;											}
	int						getArraySize			(void) const				{ return m_arraySize;												}
	deUint32				getFlags				(void) const				{ return m_flags;													}

	void					setInstanceName			(const char* name)			{ m_instanceName = name;											}
	void					setFlags				(deUint32 flags)			{ m_flags = flags;													}
	void					addMember				(const BufferVar& var)		{ m_variables.push_back(var);										}
	void					setArraySize			(int arraySize);

	int						getLastUnsizedArraySize	(int instanceNdx) const		{ return m_lastUnsizedArraySizes[instanceNdx];						}
	void					setLastUnsizedArraySize	(int instanceNdx, int size)	{ m_lastUnsizedArraySizes[instanceNdx] = size;						}

	inline iterator			begin					(void)						{ return m_variables.begin();										}
	inline const_iterator	begin					(void) const				{ return m_variables.begin();										}
	inline iterator			end						(void)						{ return m_variables.end();											}
	inline const_iterator	end						(void) const				{ return m_variables.end();											}

private:
	std::string				m_blockName;
	std::string				m_instanceName;
	std::vector<BufferVar>	m_variables;
	int						m_arraySize;				//!< Array size or 0 if not interface block array.
	std::vector<int>		m_lastUnsizedArraySizes;	//!< Sizes of last unsized array element, can be different per instance.
	deUint32				m_flags;
};

class ShaderInterface
{
public:
									ShaderInterface			(void);
									~ShaderInterface		(void);

	glu::StructType&				allocStruct				(const char* name);
	const glu::StructType*			findStruct				(const char* name) const;
	void							getNamedStructs			(std::vector<const glu::StructType*>& structs) const;

	BufferBlock&					allocBlock				(const char* name);

	int								getNumBlocks			(void) const	{ return (int)m_bufferBlocks.size();	}
	const BufferBlock&				getBlock				(int ndx) const	{ return *m_bufferBlocks[ndx];			}
	BufferBlock&					getBlock				(int ndx)		{ return *m_bufferBlocks[ndx];			}

private:
									ShaderInterface			(const ShaderInterface&);
	ShaderInterface&				operator=				(const ShaderInterface&);

	std::vector<glu::StructType*>	m_structs;
	std::vector<BufferBlock*>		m_bufferBlocks;
};

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

class BufferLayout
{
public:
	std::vector<BlockLayoutEntry>		blocks;
	std::vector<BufferVarLayoutEntry>	bufferVars;

	int									getVariableIndex		(const std::string& name) const;
	int									getBlockIndex			(const std::string& name) const;
};

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

struct RefDataStorage
{
	std::vector<deUint8>			data;
	std::vector<BlockDataPtr>	pointers;
};

class SSBOLayoutCase : public vkt::TestCase
{
public:
	enum BufferMode
	{
		BUFFERMODE_SINGLE = 0,	//!< Single buffer shared between uniform blocks.
		BUFFERMODE_PER_BLOCK,	//!< Per-block buffers

		BUFFERMODE_LAST
	};

								SSBOLayoutCase				(tcu::TestContext& testCtx, const char* name, const char* description, BufferMode bufferMode, MatrixLoadFlags matrixLoadFlag);
	virtual						~SSBOLayoutCase				(void);

	virtual void				initPrograms				(vk::SourceCollections& programCollection) const;
	virtual TestInstance*		createInstance				(Context& context) const;

protected:
	void						init						(void);

	BufferMode					m_bufferMode;
	ShaderInterface				m_interface;
	MatrixLoadFlags				m_matrixLoadFlag;
	std::string					m_computeShaderSrc;

private:
								SSBOLayoutCase				(const SSBOLayoutCase&);
	SSBOLayoutCase&				operator=					(const SSBOLayoutCase&);

	BufferLayout				m_refLayout;
	RefDataStorage				m_initialData;	// Initial data stored in buffer.
	RefDataStorage				m_writeData;		// Data written by compute shader.
};

} // ssbo
} // vkt

#endif // _VKTSSBOLAYOUTCASE_HPP
