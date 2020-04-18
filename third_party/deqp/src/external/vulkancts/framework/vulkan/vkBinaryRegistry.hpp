#ifndef _VKBINARYREGISTRY_HPP
#define _VKBINARYREGISTRY_HPP
/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2015 Google Inc.
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
 * \brief Program binary registry.
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkPrograms.hpp"
#include "tcuResource.hpp"
#include "deMemPool.hpp"
#include "dePoolHash.h"
#include "deUniquePtr.hpp"

#include <map>
#include <vector>
#include <stdexcept>

namespace vk
{
namespace BinaryRegistryDetail
{

struct ProgramIdentifier
{
	std::string		testCasePath;
	std::string		programName;

	ProgramIdentifier (const std::string& testCasePath_, const std::string& programName_)
		: testCasePath	(testCasePath_)
		, programName	(programName_)
	{
	}
};

inline bool operator< (const ProgramIdentifier& a, const ProgramIdentifier& b)
{
	return (a.testCasePath < b.testCasePath) || ((a.testCasePath == b.testCasePath) && (a.programName < b.programName));
}

class ProgramNotFoundException : public tcu::ResourceError
{
public:
	ProgramNotFoundException (const ProgramIdentifier& id, const std::string& reason)
		: tcu::ResourceError("Program " + id.testCasePath + " / '" + id.programName + "' not found: " + reason)
	{
	}
};

// Program Binary Index
// --------------------
//
// When SPIR-V binaries are stored on disk, duplicate binaries are eliminated
// to save a significant amount of space. Many tests use identical binaries and
// just storing each compiled binary without de-duplication would be incredibly
// wasteful.
//
// To locate binary that corresponds given ProgramIdentifier, a program binary
// index is needed. Since that index is accessed every time a test requests shader
// binary, it must be fast to load (to reduce statup cost), and fast to access.
//
// Simple trie is used to store binary indices. It is laid out as an array of
// BinaryIndexNodes. Nodes store 4-byte pieces (words) of search string, rather
// than just a single character. This gives more regular memory layout in exchange
// of a little wasted storage.
//
// Search strings are created by splitting original string into 4-byte words and
// appending one or more terminating 0 bytes.
//
// For each node where word doesn't have trailing 0 bytes (not terminated), the
// index points into a offset of its child list. Children for each node are stored
// consecutively, and the list is terminated by child with word = 0.
//
// If word contains one or more trailing 0 bytes, index denotes the binary index
// instead of index of the child list.

struct BinaryIndexNode
{
	deUint32	word;		//!< 4 bytes of search string.
	deUint32	index;		//!< Binary index if word ends with 0 bytes, or index of first child node otherwise.
};

template<typename Element>
class LazyResource
{
public:
									LazyResource		(de::MovePtr<tcu::Resource> resource);

	const Element&					operator[]			(size_t ndx);
	size_t							size				(void) const { return m_elements.size();	}

private:
	enum
	{
		ELEMENTS_PER_PAGE_LOG2	= 10
	};

	inline size_t					getPageForElement	(size_t elemNdx) const { return elemNdx >> ELEMENTS_PER_PAGE_LOG2;	}
	inline bool						isPageResident		(size_t pageNdx) const { return m_isPageResident[pageNdx];			}

	void							makePageResident	(size_t pageNdx);

	de::UniquePtr<tcu::Resource>	m_resource;

	std::vector<Element>			m_elements;
	std::vector<bool>				m_isPageResident;
};

template<typename Element>
LazyResource<Element>::LazyResource (de::MovePtr<tcu::Resource> resource)
	: m_resource(resource)
{
	const size_t	resSize		= m_resource->getSize();
	const size_t	numElements	= resSize/sizeof(Element);
	const size_t	numPages	= (numElements >> ELEMENTS_PER_PAGE_LOG2) + ((numElements & ((1u<<ELEMENTS_PER_PAGE_LOG2)-1u)) == 0 ? 0 : 1);

	TCU_CHECK_INTERNAL(numElements*sizeof(Element) == resSize);

	m_elements.resize(numElements);
	m_isPageResident.resize(numPages, false);
}

template<typename Element>
const Element& LazyResource<Element>::operator[] (size_t ndx)
{
	const size_t pageNdx = getPageForElement(ndx);

	if (ndx >= m_elements.size())
		throw std::out_of_range("");

	if (!isPageResident(pageNdx))
		makePageResident(pageNdx);

	return m_elements[ndx];
}

template<typename Element>
void LazyResource<Element>::makePageResident (size_t pageNdx)
{
	const size_t	pageSize		= (size_t)(1<<ELEMENTS_PER_PAGE_LOG2)*sizeof(Element);
	const size_t	pageOffset		= pageNdx*pageSize;
	const size_t	numBytesToRead	= de::min(m_elements.size()*sizeof(Element) - pageOffset, pageSize);

	DE_ASSERT(!isPageResident(pageNdx));

	if ((size_t)m_resource->getPosition() != pageOffset)
		m_resource->setPosition((int)pageOffset);

	m_resource->read((deUint8*)&m_elements[pageNdx << ELEMENTS_PER_PAGE_LOG2], (int)numBytesToRead);
	m_isPageResident[pageNdx] = true;
}

typedef LazyResource<BinaryIndexNode> BinaryIndexAccess;

class BinaryRegistryReader
{
public:
							BinaryRegistryReader	(const tcu::Archive& archive, const std::string& srcPath);
							~BinaryRegistryReader	(void);

	ProgramBinary*			loadProgram				(const ProgramIdentifier& id) const;

private:
	typedef de::MovePtr<BinaryIndexAccess> BinaryIndexPtr;

	const tcu::Archive&		m_archive;
	const std::string		m_srcPath;

	mutable BinaryIndexPtr	m_binaryIndex;
};

struct ProgramIdentifierIndex
{
	ProgramIdentifier	id;
	deUint32			index;

	ProgramIdentifierIndex (const ProgramIdentifier&	id_,
							deUint32					index_)
		: id	(id_)
		, index	(index_)
	{}
};

DE_DECLARE_POOL_HASH(BinaryIndexHashImpl, const ProgramBinary*, deUint32);

class BinaryIndexHash
{
public:
								BinaryIndexHash		(void);
								~BinaryIndexHash	(void);

	deUint32*					find				(const ProgramBinary* binary) const;
	void						insert				(const ProgramBinary* binary, deUint32 index);

private:
								BinaryIndexHash		(const BinaryIndexHash&);
	BinaryIndexHash&			operator=			(const BinaryIndexHash&);

	de::MemPool					m_memPool;
	BinaryIndexHashImpl* const	m_hash;
};

class BinaryRegistryWriter
{
public:
						BinaryRegistryWriter	(const std::string& dstPath);
						~BinaryRegistryWriter	(void);

	void				addProgram				(const ProgramIdentifier& id, const ProgramBinary& binary);
	void				write					(void) const;

private:
	void				initFromPath			(const std::string& srcPath);
	void				writeToPath				(const std::string& dstPath) const;

	deUint32*			findBinary				(const ProgramBinary& binary) const;
	deUint32			getNextSlot				(void) const;
	void				addBinary				(deUint32 index, const ProgramBinary& binary);

	struct BinarySlot
	{
		ProgramBinary*	binary;
		size_t			referenceCount;

		BinarySlot (ProgramBinary* binary_, size_t referenceCount_)
			: binary		(binary_)
			, referenceCount(referenceCount_)
		{}

		BinarySlot (void)
			: binary		(DE_NULL)
			, referenceCount(0)
		{}
	};

	typedef std::vector<BinarySlot>				BinaryVector;
	typedef std::vector<ProgramIdentifierIndex>	ProgIdIndexVector;

	const std::string&	m_dstPath;

	ProgIdIndexVector	m_binaryIndices;		//!< ProgramIdentifier -> slot in m_binaries
	BinaryIndexHash		m_binaryHash;			//!< ProgramBinary -> slot in m_binaries
	BinaryVector		m_binaries;
};

} // BinaryRegistryDetail

using BinaryRegistryDetail::BinaryRegistryReader;
using BinaryRegistryDetail::BinaryRegistryWriter;
using BinaryRegistryDetail::ProgramIdentifier;
using BinaryRegistryDetail::ProgramNotFoundException;

} // vk

#endif // _VKBINARYREGISTRY_HPP
