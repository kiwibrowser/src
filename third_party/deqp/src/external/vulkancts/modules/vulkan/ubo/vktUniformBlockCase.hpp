#ifndef _VKTUNIFORMBLOCKCASE_HPP
#define _VKTUNIFORMBLOCKCASE_HPP
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
 * \brief Uniform block tests.
 *//*--------------------------------------------------------------------*/

#include "deSharedPtr.hpp"
#include "vktTestCase.hpp"
#include "tcuDefs.hpp"
#include "gluShaderUtil.hpp"

#include <map>

namespace vkt
{
namespace ubo
{

// Uniform block details.

enum UniformFlags
{
	PRECISION_LOW		= (1<<0),
	PRECISION_MEDIUM	= (1<<1),
	PRECISION_HIGH		= (1<<2),
	PRECISION_MASK		= PRECISION_LOW|PRECISION_MEDIUM|PRECISION_HIGH,

	LAYOUT_SHARED		= (1<<3),
	LAYOUT_PACKED		= (1<<4),
	LAYOUT_STD140		= (1<<5),
	LAYOUT_ROW_MAJOR	= (1<<6),
	LAYOUT_COLUMN_MAJOR	= (1<<7),	//!< \note Lack of both flags means column-major matrix.
	LAYOUT_OFFSET		= (1<<8),
	LAYOUT_MASK			= LAYOUT_SHARED|LAYOUT_PACKED|LAYOUT_STD140|LAYOUT_ROW_MAJOR|LAYOUT_COLUMN_MAJOR|LAYOUT_OFFSET,

	DECLARE_VERTEX		= (1<<9),
	DECLARE_FRAGMENT	= (1<<10),
	DECLARE_BOTH		= DECLARE_VERTEX|DECLARE_FRAGMENT,

	UNUSED_VERTEX		= (1<<11),	//!< Uniform or struct member is not read in vertex shader.
	UNUSED_FRAGMENT		= (1<<12),	//!< Uniform or struct member is not read in fragment shader.
	UNUSED_BOTH			= UNUSED_VERTEX|UNUSED_FRAGMENT
};

enum MatrixLoadFlags
{
	LOAD_FULL_MATRIX		= 0,
	LOAD_MATRIX_COMPONENTS	= 1,
};

class StructType;

class VarType
{
public:
						VarType			(void);
						VarType			(const VarType& other);
						VarType			(glu::DataType basicType, deUint32 flags);
						VarType			(const VarType& elementType, int arraySize);
	explicit			VarType			(const StructType* structPtr, deUint32 flags = 0u);
						~VarType		(void);

	bool				isBasicType		(void) const	{ return m_type == TYPE_BASIC;	}
	bool				isArrayType		(void) const	{ return m_type == TYPE_ARRAY;	}
	bool				isStructType	(void) const	{ return m_type == TYPE_STRUCT;	}

	deUint32			getFlags		(void) const	{ return m_flags;					}
	glu::DataType		getBasicType	(void) const	{ return m_data.basicType;			}

	const VarType&		getElementType	(void) const	{ return *m_data.array.elementType;	}
	int					getArraySize	(void) const	{ return m_data.array.size;			}

	const StructType&	getStruct		(void) const	{ return *m_data.structPtr;			}

	VarType&			operator=		(const VarType& other);

private:
	enum Type
	{
		TYPE_BASIC,
		TYPE_ARRAY,
		TYPE_STRUCT,

		TYPE_LAST
	};

	Type				m_type;
	deUint32			m_flags;
	union Data
	{
		glu::DataType		basicType;
		struct
		{
			VarType*		elementType;
			int				size;
		} array;
		const StructType*	structPtr;

		Data (void)
		{
			array.elementType	= DE_NULL;
			array.size			= 0;
		};
	} m_data;
};

class StructMember
{
public:
						StructMember	(const std::string& name, const VarType& type, deUint32 flags)
							: m_name(name), m_type(type), m_flags(flags)
						{}
						StructMember	(void)
							: m_flags(0)
						{}

	const std::string&	getName			(void) const { return m_name;	}
	const VarType&		getType			(void) const { return m_type;	}
	deUint32			getFlags		(void) const { return m_flags;	}

private:
	std::string			m_name;
	VarType				m_type;
	deUint32			m_flags;
};

class StructType
{
public:
	typedef std::vector<StructMember>::iterator			Iterator;
	typedef std::vector<StructMember>::const_iterator	ConstIterator;

								StructType		(const std::string& typeName) : m_typeName(typeName) {}
								~StructType		(void) {}

	const std::string&			getTypeName		(void) const	{ return m_typeName;			}
	bool						hasTypeName		(void) const	{ return !m_typeName.empty();	}

	inline Iterator				begin			(void)			{ return m_members.begin();		}
	inline ConstIterator		begin			(void) const	{ return m_members.begin();		}
	inline Iterator				end				(void)			{ return m_members.end();		}
	inline ConstIterator		end				(void) const	{ return m_members.end();		}

	void						addMember		(const std::string& name, const VarType& type, deUint32 flags = 0);

private:
	std::string					m_typeName;
	std::vector<StructMember>	m_members;
};

class Uniform
{
public:
						Uniform			(const std::string& name, const VarType& type, deUint32 flags = 0);

	const std::string&	getName			(void) const { return m_name;	}
	const VarType&		getType			(void) const { return m_type;	}
	deUint32			getFlags		(void) const { return m_flags;	}

private:
	std::string			m_name;
	VarType				m_type;
	deUint32			m_flags;
};

class UniformBlock
{
public:
	typedef std::vector<Uniform>::iterator			Iterator;
	typedef std::vector<Uniform>::const_iterator	ConstIterator;

							UniformBlock		(const std::string& blockName);

	const std::string&		getBlockName		(void) const { return m_blockName;		}
	const std::string&		getInstanceName		(void) const { return m_instanceName;	}
	bool					hasInstanceName		(void) const { return !m_instanceName.empty();	}
	bool					isArray				(void) const { return m_arraySize > 0;			}
	int						getArraySize		(void) const { return m_arraySize;				}
	deUint32				getFlags			(void) const { return m_flags;					}

	void					setInstanceName		(const std::string& name)	{ m_instanceName = name;			}
	void					setFlags			(deUint32 flags)			{ m_flags = flags;					}
	void					setArraySize		(int arraySize)				{ m_arraySize = arraySize;			}
	void					addUniform			(const Uniform& uniform)	{ m_uniforms.push_back(uniform);	}

	inline Iterator			begin				(void)			{ return m_uniforms.begin();	}
	inline ConstIterator	begin				(void) const	{ return m_uniforms.begin();	}
	inline Iterator			end					(void)			{ return m_uniforms.end();		}
	inline ConstIterator	end					(void) const	{ return m_uniforms.end();		}

private:
	std::string				m_blockName;
	std::string				m_instanceName;
	std::vector<Uniform>	m_uniforms;
	int						m_arraySize;	//!< Array size or 0 if not interface block array.
	deUint32				m_flags;
};

typedef de::SharedPtr<StructType>	StructTypeSP;
typedef de::SharedPtr<UniformBlock>	UniformBlockSP;

class ShaderInterface
{
public:
								ShaderInterface			(void);
								~ShaderInterface		(void);

	StructType&					allocStruct				(const std::string& name);
	void						getNamedStructs			(std::vector<const StructType*>& structs) const;

	UniformBlock&				allocBlock				(const std::string& name);

	int							getNumUniformBlocks		(void) const	{ return (int)m_uniformBlocks.size();	}
	const UniformBlock&			getUniformBlock			(int ndx) const	{ return *m_uniformBlocks[ndx];			}

private:
	std::vector<StructTypeSP>		m_structs;
	std::vector<UniformBlockSP>		m_uniformBlocks;
};

struct BlockLayoutEntry
{
	BlockLayoutEntry (void)
		: size					(0)
		, blockDeclarationNdx	(-1)
		, bindingNdx			(-1)
		, instanceNdx			(-1)
	{
	}

	std::string			name;
	int					size;
	std::vector<int>	activeUniformIndices;
	int					blockDeclarationNdx;
	int					bindingNdx;
	int					instanceNdx;
};

struct UniformLayoutEntry
{
	UniformLayoutEntry (void)
		: type			(glu::TYPE_LAST)
		, size			(0)
		, blockLayoutNdx(-1)
		, offset		(-1)
		, arrayStride	(-1)
		, matrixStride	(-1)
		, isRowMajor	(false)
		, instanceNdx	(0)
	{
	}

	std::string			name;
	glu::DataType		type;
	int					size;
	int					blockLayoutNdx;
	int					offset;
	int					arrayStride;
	int					matrixStride;
	bool				isRowMajor;
	int					instanceNdx;
};

class UniformLayout
{
public:
	std::vector<BlockLayoutEntry>		blocks;
	std::vector<UniformLayoutEntry>		uniforms;

	int									getUniformLayoutIndex	(int blockDeclarationNdx, const std::string& name) const;
	int									getBlockLayoutIndex		(int blockDeclarationNdx, int instanceNdx) const;
};

class UniformBlockCase : public vkt::TestCase
{
public:
	enum BufferMode
	{
		BUFFERMODE_SINGLE = 0,	//!< Single buffer shared between uniform blocks.
		BUFFERMODE_PER_BLOCK,	//!< Per-block buffers

		BUFFERMODE_LAST
	};

								UniformBlockCase			(tcu::TestContext&	testCtx,
															 const std::string&	name,
															 const std::string&	description,
															 BufferMode			bufferMode,
															 MatrixLoadFlags	matrixLoadFlag,
															 bool				shuffleUniformMembers = false);
								~UniformBlockCase			(void);

	virtual	void				initPrograms				(vk::SourceCollections& programCollection) const;
	virtual TestInstance*		createInstance				(Context& context) const;

protected:
	void						init						(void);

	BufferMode					m_bufferMode;
	ShaderInterface				m_interface;
	MatrixLoadFlags				m_matrixLoadFlag;
	const bool					m_shuffleUniformMembers;	//!< Used with explicit offsets to test out of order member offsets

private:
	std::string					m_vertShaderSource;
	std::string					m_fragShaderSource;

	std::vector<deUint8>		m_data;						//!< Data.
	std::map<int, void*>		m_blockPointers;			//!< Reference block pointers.
	UniformLayout				m_uniformLayout;			//!< std140 layout.
};

} // ubo
} // vkt

#endif // _VKTUNIFORMBLOCKCASE_HPP
