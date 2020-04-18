#ifndef _GLCUNIFORMBLOCKCASE_HPP
#define _GLCUNIFORMBLOCKCASE_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
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
 */ /*!
 * \file
 * \brief Uniform block tests.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "gluShaderProgram.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

namespace deqp
{

// Uniform block details.
namespace ub
{

enum UniformFlags
{
	PRECISION_LOW	= (1 << 0),
	PRECISION_MEDIUM = (1 << 1),
	PRECISION_HIGH   = (1 << 2),
	PRECISION_MASK   = PRECISION_LOW | PRECISION_MEDIUM | PRECISION_HIGH,

	LAYOUT_SHARED		= (1 << 3),
	LAYOUT_PACKED		= (1 << 4),
	LAYOUT_STD140		= (1 << 5),
	LAYOUT_ROW_MAJOR	= (1 << 6),
	LAYOUT_COLUMN_MAJOR = (1 << 7), //!< \note Lack of both flags means column-major matrix.
	LAYOUT_MASK			= LAYOUT_SHARED | LAYOUT_PACKED | LAYOUT_STD140 | LAYOUT_ROW_MAJOR | LAYOUT_COLUMN_MAJOR,

	DECLARE_VERTEX   = (1 << 8),
	DECLARE_FRAGMENT = (1 << 9),
	DECLARE_BOTH	 = DECLARE_VERTEX | DECLARE_FRAGMENT,

	UNUSED_VERTEX   = (1 << 10), //!< Uniform or struct member is not read in vertex shader.
	UNUSED_FRAGMENT = (1 << 11), //!< Uniform or struct member is not read in fragment shader.
	UNUSED_BOTH		= UNUSED_VERTEX | UNUSED_FRAGMENT
};

// \todo [2012-07-25 pyry] Use glu::VarType.

class StructType;

class VarType
{
public:
	VarType(void);
	VarType(const VarType& other);
	VarType(glu::DataType basicType, deUint32 flags);
	VarType(const VarType& elementType, int arraySize);
	explicit VarType(const StructType* structPtr);
	~VarType(void);

	bool isBasicType(void) const
	{
		return m_type == TYPE_BASIC;
	}
	bool isArrayType(void) const
	{
		return m_type == TYPE_ARRAY;
	}
	bool isStructType(void) const
	{
		return m_type == TYPE_STRUCT;
	}

	deUint32 getFlags(void) const
	{
		return m_flags;
	}
	glu::DataType getBasicType(void) const
	{
		return m_data.basicType;
	}

	const VarType& getElementType(void) const
	{
		return *m_data.array.elementType;
	}
	int getArraySize(void) const
	{
		return m_data.array.size;
	}

	const StructType& getStruct(void) const
	{
		return *m_data.structPtr;
	}

	VarType& operator=(const VarType& other);

private:
	enum Type
	{
		TYPE_BASIC,
		TYPE_ARRAY,
		TYPE_STRUCT,

		TYPE_LAST
	};

	Type	 m_type;
	deUint32 m_flags;
	union Data {
		glu::DataType basicType;
		struct
		{
			VarType* elementType;
			int		 size;
		} array;
		const StructType* structPtr;

		Data(void)
		{
			array.elementType = DE_NULL;
			array.size		  = 0;
		};
	} m_data;
};

class StructMember
{
public:
	StructMember(const char* name, const VarType& type, deUint32 flags) : m_name(name), m_type(type), m_flags(flags)
	{
	}
	StructMember(void) : m_flags(0)
	{
	}

	const char* getName(void) const
	{
		return m_name.c_str();
	}
	const VarType& getType(void) const
	{
		return m_type;
	}
	deUint32 getFlags(void) const
	{
		return m_flags;
	}

private:
	std::string m_name;
	VarType		m_type;
	deUint32	m_flags;
};

class StructType
{
public:
	typedef std::vector<StructMember>::iterator		  Iterator;
	typedef std::vector<StructMember>::const_iterator ConstIterator;

	StructType(const char* typeName) : m_typeName(typeName)
	{
	}
	~StructType(void)
	{
	}

	const char* getTypeName(void) const
	{
		return m_typeName.empty() ? DE_NULL : m_typeName.c_str();
	}

	inline Iterator begin(void)
	{
		return m_members.begin();
	}
	inline ConstIterator begin(void) const
	{
		return m_members.begin();
	}
	inline Iterator end(void)
	{
		return m_members.end();
	}
	inline ConstIterator end(void) const
	{
		return m_members.end();
	}

	void addMember(const char* name, const VarType& type, deUint32 flags = 0);

private:
	std::string				  m_typeName;
	std::vector<StructMember> m_members;
};

class Uniform
{
public:
	Uniform(const char* name, const VarType& type, deUint32 flags = 0);

	const char* getName(void) const
	{
		return m_name.c_str();
	}
	const VarType& getType(void) const
	{
		return m_type;
	}
	deUint32 getFlags(void) const
	{
		return m_flags;
	}

private:
	std::string m_name;
	VarType		m_type;
	deUint32	m_flags;
};

class UniformBlock
{
public:
	typedef std::vector<Uniform>::iterator		 Iterator;
	typedef std::vector<Uniform>::const_iterator ConstIterator;

	UniformBlock(const char* blockName);

	const char* getBlockName(void) const
	{
		return m_blockName.c_str();
	}
	const char* getInstanceName(void) const
	{
		return m_instanceName.empty() ? DE_NULL : m_instanceName.c_str();
	}
	bool isArray(void) const
	{
		return m_arraySize > 0;
	}
	int getArraySize(void) const
	{
		return m_arraySize;
	}
	deUint32 getFlags(void) const
	{
		return m_flags;
	}

	void setInstanceName(const char* name)
	{
		m_instanceName = name;
	}
	void setFlags(deUint32 flags)
	{
		m_flags = flags;
	}
	void setArraySize(int arraySize)
	{
		m_arraySize = arraySize;
	}
	void addUniform(const Uniform& uniform)
	{
		m_uniforms.push_back(uniform);
	}

	inline Iterator begin(void)
	{
		return m_uniforms.begin();
	}
	inline ConstIterator begin(void) const
	{
		return m_uniforms.begin();
	}
	inline Iterator end(void)
	{
		return m_uniforms.end();
	}
	inline ConstIterator end(void) const
	{
		return m_uniforms.end();
	}

private:
	std::string			 m_blockName;
	std::string			 m_instanceName;
	std::vector<Uniform> m_uniforms;
	int					 m_arraySize; //!< Array size or 0 if not interface block array.
	deUint32			 m_flags;
};

class ShaderInterface
{
public:
	ShaderInterface(void);
	~ShaderInterface(void);

	StructType& allocStruct(const char* name);
	const StructType* findStruct(const char* name) const;
	void getNamedStructs(std::vector<const StructType*>& structs) const;

	UniformBlock& allocBlock(const char* name);

	int getNumUniformBlocks(void) const
	{
		return (int)m_uniformBlocks.size();
	}
	const UniformBlock& getUniformBlock(int ndx) const
	{
		return *m_uniformBlocks[ndx];
	}

private:
	std::vector<StructType*>   m_structs;
	std::vector<UniformBlock*> m_uniformBlocks;
};

class UniformLayout;

} // ub

class UniformBlockCase : public TestCase
{
public:
	enum BufferMode
	{
		BUFFERMODE_SINGLE = 0, //!< Single buffer shared between uniform blocks.
		BUFFERMODE_PER_BLOCK,  //!< Per-block buffers

		BUFFERMODE_LAST
	};

	UniformBlockCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion,
					 BufferMode bufferMode);
	~UniformBlockCase(void);

	IterateResult iterate(void);

protected:
	bool compareStd140Blocks(const ub::UniformLayout& refLayout, const ub::UniformLayout& cmpLayout) const;
	bool compareSharedBlocks(const ub::UniformLayout& refLayout, const ub::UniformLayout& cmpLayout) const;
	bool compareTypes(const ub::UniformLayout& refLayout, const ub::UniformLayout& cmpLayout) const;
	bool checkLayoutIndices(const ub::UniformLayout& layout) const;
	bool checkLayoutBounds(const ub::UniformLayout& layout) const;
	bool checkIndexQueries(deUint32 program, const ub::UniformLayout& layout) const;

	bool render(glu::ShaderProgram& program) const;

	glu::GLSLVersion	m_glslVersion;
	BufferMode			m_bufferMode;
	ub::ShaderInterface m_interface;
};

} // deqp

#endif // _GLCUNIFORMBLOCKCASE_HPP
