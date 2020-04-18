// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _TYPES_INCLUDED
#define _TYPES_INCLUDED

#include "BaseTypes.h"
#include "Common.h"
#include "debug.h"

#include <algorithm>

class TType;
struct TPublicType;

class TField
{
public:
	POOL_ALLOCATOR_NEW_DELETE();
	TField(TType *type, TString *name, const TSourceLoc &line)
		: mType(type),
		mName(name),
		mLine(line)
	{
	}

	// TODO(alokp): We should only return const type.
	// Fix it by tweaking grammar.
	TType *type()
	{
		return mType;
	}
	const TType *type() const
	{
		return mType;
	}

	const TString &name() const
	{
		return *mName;
	}
	const TSourceLoc &line() const
	{
		return mLine;
	}

private:
	TType *mType;
	TString *mName;
	TSourceLoc mLine;
};

typedef TVector<TField *> TFieldList;
inline TFieldList *NewPoolTFieldList()
{
	void *memory = GetGlobalPoolAllocator()->allocate(sizeof(TFieldList));
	return new(memory)TFieldList;
}

class TFieldListCollection
{
public:
	virtual ~TFieldListCollection() { }
	const TString &name() const
	{
		return *mName;
	}
	const TFieldList &fields() const
	{
		return *mFields;
	}

	const TString &mangledName() const
	{
		if(mMangledName.empty())
			mMangledName = buildMangledName();
		return mMangledName;
	}
	size_t objectSize() const
	{
		if(mObjectSize == 0)
			mObjectSize = calculateObjectSize();
		return mObjectSize;
	};

protected:
	TFieldListCollection(const TString *name, TFieldList *fields)
		: mName(name),
		mFields(fields),
		mObjectSize(0)
	{
	}
	TString buildMangledName() const;
	size_t calculateObjectSize() const;
	virtual TString mangledNamePrefix() const = 0;

	const TString *mName;
	TFieldList *mFields;

	mutable TString mMangledName;
	mutable size_t mObjectSize;
};

// May also represent interface blocks
class TStructure : public TFieldListCollection
{
public:
	POOL_ALLOCATOR_NEW_DELETE();
	TStructure(const TString *name, TFieldList *fields)
		: TFieldListCollection(name, fields),
		mDeepestNesting(0),
		mUniqueId(0),
		mAtGlobalScope(false)
	{
	}

	int deepestNesting() const
	{
		if(mDeepestNesting == 0)
			mDeepestNesting = calculateDeepestNesting();
		return mDeepestNesting;
	}
	bool containsArrays() const;
	bool containsType(TBasicType type) const;
	bool containsSamplers() const;

	bool equals(const TStructure &other) const;

	void setMatrixPackingIfUnspecified(TLayoutMatrixPacking matrixPacking);

	void setUniqueId(int uniqueId)
	{
		mUniqueId = uniqueId;
	}

	int uniqueId() const
	{
		ASSERT(mUniqueId != 0);
		return mUniqueId;
	}

	void setAtGlobalScope(bool atGlobalScope)
	{
		mAtGlobalScope = atGlobalScope;
	}

	bool atGlobalScope() const
	{
		return mAtGlobalScope;
	}

private:
	// TODO(zmo): Find a way to get rid of the const_cast in function
	// setName().  At the moment keep this function private so only
	// friend class RegenerateStructNames may call it.
	friend class RegenerateStructNames;
	void setName(const TString &name)
	{
		TString *mutableName = const_cast<TString *>(mName);
		*mutableName = name;
	}

	virtual TString mangledNamePrefix() const
	{
		return "struct-";
	}
	int calculateDeepestNesting() const;

	mutable int mDeepestNesting;
	int mUniqueId;
	bool mAtGlobalScope;
};

class TInterfaceBlock : public TFieldListCollection
{
public:
	POOL_ALLOCATOR_NEW_DELETE();
	TInterfaceBlock(const TString *name, TFieldList *fields, const TString *instanceName,
		int arraySize, const TLayoutQualifier &layoutQualifier)
		: TFieldListCollection(name, fields),
		mInstanceName(instanceName),
		mArraySize(arraySize),
		mBlockStorage(layoutQualifier.blockStorage),
		mMatrixPacking(layoutQualifier.matrixPacking)
	{
	}

	const TString &instanceName() const
	{
		return *mInstanceName;
	}
	bool hasInstanceName() const
	{
		return mInstanceName != nullptr;
	}
	bool isArray() const
	{
		return mArraySize > 0;
	}
	int arraySize() const
	{
		return mArraySize;
	}
	TLayoutBlockStorage blockStorage() const
	{
		return mBlockStorage;
	}
	TLayoutMatrixPacking matrixPacking() const
	{
		return mMatrixPacking;
	}

private:
	virtual TString mangledNamePrefix() const
	{
		return "iblock-";
	}

	const TString *mInstanceName; // for interface block instance names
	int mArraySize; // 0 if not an array
	TLayoutBlockStorage mBlockStorage;
	TLayoutMatrixPacking mMatrixPacking;
};

//
// Base class for things that have a type.
//
class TType
{
public:
	POOL_ALLOCATOR_NEW_DELETE();

	TType(TBasicType t, int s0 = 1, int s1 = 1) :
		type(t), precision(EbpUndefined), qualifier(EvqGlobal),
		primarySize(s0), secondarySize(s1), array(false), arraySize(0), maxArraySize(0), arrayInformationType(0), interfaceBlock(0), layoutQualifier(TLayoutQualifier::create()),
		structure(0), mangled(0)
	{
	}

	TType(TBasicType t, TPrecision p, TQualifier q = EvqTemporary, int s0 = 1, int s1 = 1, bool a = false) :
		type(t), precision(p), qualifier(q),
		primarySize(s0), secondarySize(s1), array(a), arraySize(0), maxArraySize(0), arrayInformationType(0), interfaceBlock(0), layoutQualifier(TLayoutQualifier::create()),
		structure(0), mangled(0)
	{
	}

	TType(TStructure* userDef, TPrecision p = EbpUndefined) :
		type(EbtStruct), precision(p), qualifier(EvqTemporary),
		primarySize(1), secondarySize(1), array(false), arraySize(0), maxArraySize(0), arrayInformationType(0), interfaceBlock(0), layoutQualifier(TLayoutQualifier::create()),
		structure(userDef), mangled(0)
	{
	}

	TType(TInterfaceBlock *interfaceBlockIn, TQualifier qualifierIn,
		TLayoutQualifier layoutQualifierIn, int arraySizeIn)
		: type(EbtInterfaceBlock), precision(EbpUndefined), qualifier(qualifierIn),
		primarySize(1), secondarySize(1), array(arraySizeIn > 0), arraySize(arraySizeIn), maxArraySize(0), arrayInformationType(0),
		interfaceBlock(interfaceBlockIn), layoutQualifier(layoutQualifierIn), structure(0), mangled(0)
	{
	}

	explicit TType(const TPublicType &p);

	TBasicType getBasicType() const { return type; }
	void setBasicType(TBasicType t) { type = t; }

	TPrecision getPrecision() const { return precision; }
	void setPrecision(TPrecision p) { precision = p; }

	TQualifier getQualifier() const { return qualifier; }
	void setQualifier(TQualifier q) { qualifier = q; }

	TLayoutQualifier getLayoutQualifier() const { return layoutQualifier; }
	void setLayoutQualifier(TLayoutQualifier lq) { layoutQualifier = lq; }

	void setMatrixPackingIfUnspecified(TLayoutMatrixPacking matrixPacking)
	{
		if(isStruct())
		{
			// If the structure's matrix packing is specified, it overrules the block's matrix packing
			structure->setMatrixPackingIfUnspecified((layoutQualifier.matrixPacking == EmpUnspecified) ?
			                                         matrixPacking : layoutQualifier.matrixPacking);
		}
		// If the member's matrix packing is specified, it overrules any higher level matrix packing
		if(layoutQualifier.matrixPacking == EmpUnspecified)
		{
			layoutQualifier.matrixPacking = matrixPacking;
		}
	}

	// One-dimensional size of single instance type
	int getNominalSize() const { return primarySize; }
	void setNominalSize(int s) { primarySize = s; }
	// Full size of single instance of type
	size_t getObjectSize() const
	{
		if(isArray())
		{
			return getElementSize() * std::max(getArraySize(), getMaxArraySize());
		}
		else
		{
			return getElementSize();
		}
	}

	size_t getElementSize() const
	{
		if(getBasicType() == EbtStruct)
		{
			return getStructSize();
		}
		else if(isInterfaceBlock())
		{
			return interfaceBlock->objectSize();
		}
		else if(isMatrix())
		{
			return primarySize * secondarySize;
		}
		else   // Vector or scalar
		{
			return primarySize;
		}
	}

	int samplerRegisterCount() const
	{
		if(structure)
		{
			int registerCount = 0;

			const TFieldList& fields = isInterfaceBlock() ? interfaceBlock->fields() : structure->fields();
			for(size_t i = 0; i < fields.size(); i++)
			{
				registerCount += fields[i]->type()->totalSamplerRegisterCount();
			}

			return registerCount;
		}

		return IsSampler(getBasicType()) ? 1 : 0;
	}

	int elementRegisterCount() const
	{
		if(structure || isInterfaceBlock())
		{
			int registerCount = 0;

			const TFieldList& fields = isInterfaceBlock() ? interfaceBlock->fields() : structure->fields();
			for(size_t i = 0; i < fields.size(); i++)
			{
				registerCount += fields[i]->type()->totalRegisterCount();
			}

			return registerCount;
		}
		else if(isMatrix())
		{
			return getNominalSize();
		}
		else
		{
			return 1;
		}
	}

	int blockRegisterCount() const
	{
		// If this TType object is a block member, return the register count of the parent block
		// Otherwise, return the register count of the current TType object
		if(interfaceBlock && !isInterfaceBlock())
		{
			int registerCount = 0;
			const TFieldList& fieldList = interfaceBlock->fields();
			for(size_t i = 0; i < fieldList.size(); i++)
			{
				const TType &fieldType = *(fieldList[i]->type());
				registerCount += fieldType.totalRegisterCount();
			}
			return registerCount;
		}
		return totalRegisterCount();
	}

	int totalSamplerRegisterCount() const
	{
		if(array)
		{
			return arraySize * samplerRegisterCount();
		}
		else
		{
			return samplerRegisterCount();
		}
	}

	int totalRegisterCount() const
	{
		if(array)
		{
			return arraySize * elementRegisterCount();
		}
		else
		{
			return elementRegisterCount();
		}
	}

	int registerSize() const
	{
		return isMatrix() ? secondarySize : primarySize;
	}

	bool isMatrix() const { return secondarySize > 1; }
	void setSecondarySize(int s1) { secondarySize = s1; }
	int getSecondarySize() const { return secondarySize; }

	bool isArray() const  { return array ? true : false; }
	bool isUnsizedArray() const { return array && arraySize == 0; }
	int getArraySize() const { return arraySize; }
	void setArraySize(int s) { array = true; arraySize = s; }
	int getMaxArraySize () const { return maxArraySize; }
	void setMaxArraySize (int s) { maxArraySize = s; }
	void clearArrayness() { array = false; arraySize = 0; maxArraySize = 0; }
	void setArrayInformationType(TType* t) { arrayInformationType = t; }
	TType* getArrayInformationType() const { return arrayInformationType; }

	TInterfaceBlock *getInterfaceBlock() const { return interfaceBlock; }
	void setInterfaceBlock(TInterfaceBlock *interfaceBlockIn) { interfaceBlock = interfaceBlockIn; }
	bool isInterfaceBlock() const { return type == EbtInterfaceBlock; }
	TInterfaceBlock *getAsInterfaceBlock() const { return isInterfaceBlock() ? getInterfaceBlock() : nullptr; }

	bool isVector() const { return primarySize > 1 && !isMatrix(); }
	bool isScalar() const { return primarySize == 1 && !isMatrix() && !structure && !isInterfaceBlock(); }
	bool isRegister() const { return !isMatrix() && !structure && !array && !isInterfaceBlock(); }   // Fits in a 4-element register
	bool isStruct() const { return structure != 0; }
	bool isScalarInt() const { return isScalar() && IsInteger(type); }

	TStructure* getStruct() const { return structure; }
	void setStruct(TStructure* s) { structure = s; }

	TString& getMangledName() {
		if (!mangled) {
			mangled = NewPoolTString("");
			buildMangledName(*mangled);
			*mangled += ';' ;
		}

		return *mangled;
	}

	bool sameElementType(const TType& right) const {
		return      type == right.type   &&
		     primarySize == right.primarySize &&
		   secondarySize == right.secondarySize &&
		       structure == right.structure;
	}
	bool operator==(const TType& right) const {
		return      type == right.type   &&
		     primarySize == right.primarySize &&
		   secondarySize == right.secondarySize &&
			       array == right.array && (!array || arraySize == right.arraySize) &&
		       structure == right.structure;
		// don't check the qualifier, it's not ever what's being sought after
	}
	bool operator!=(const TType& right) const {
		return !operator==(right);
	}
	bool operator<(const TType& right) const {
		if (type != right.type) return type < right.type;
		if(primarySize != right.primarySize) return (primarySize * secondarySize) < (right.primarySize * right.secondarySize);
		if(secondarySize != right.secondarySize) return secondarySize < right.secondarySize;
		if (array != right.array) return array < right.array;
		if (arraySize != right.arraySize) return arraySize < right.arraySize;
		if (structure != right.structure) return structure < right.structure;

		return false;
	}

	const char* getBasicString() const { return ::getBasicString(type); }
	const char* getPrecisionString() const { return ::getPrecisionString(precision); }
	const char* getQualifierString() const { return ::getQualifierString(qualifier); }
	TString getCompleteString() const;

	// If this type is a struct, returns the deepest struct nesting of
	// any field in the struct. For example:
	//   struct nesting1 {
	//     vec4 position;
	//   };
	//   struct nesting2 {
	//     nesting1 field1;
	//     vec4 field2;
	//   };
	// For type "nesting2", this method would return 2 -- the number
	// of structures through which indirection must occur to reach the
	// deepest field (nesting2.field1.position).
	int getDeepestStructNesting() const
	{
		return structure ? structure->deepestNesting() : 0;
	}

	bool isStructureContainingArrays() const
	{
		return structure ? structure->containsArrays() : false;
	}

	bool isStructureContainingType(TBasicType t) const
	{
		return structure ? structure->containsType(t) : false;
	}

	bool isStructureContainingSamplers() const
	{
		return structure ? structure->containsSamplers() : false;
	}

protected:
	void buildMangledName(TString&);
	size_t getStructSize() const;

	TBasicType type = EbtVoid;
	TPrecision precision = EbpUndefined;
	TQualifier qualifier = EvqTemporary;
	unsigned char primarySize = 0;     // size of vector or matrix, not size of array
	unsigned char secondarySize = 0;   // 1 for vectors, > 1 for matrices
	bool array = false;
	int arraySize = 0;
	int maxArraySize = 0;
	TType *arrayInformationType = nullptr;

	// null unless this is an interface block, or interface block member variable
	TInterfaceBlock *interfaceBlock = nullptr;
	TLayoutQualifier layoutQualifier;

	TStructure *structure = nullptr;   // null unless this is a struct

	TString *mangled = nullptr;
};

//
// This is a workaround for a problem with the yacc stack,  It can't have
// types that it thinks have non-trivial constructors.  It should
// just be used while recognizing the grammar, not anything else.  Pointers
// could be used, but also trying to avoid lots of memory management overhead.
//
// Not as bad as it looks, there is no actual assumption that the fields
// match up or are name the same or anything like that.
//
struct TPublicType
{
	TBasicType type;
	TLayoutQualifier layoutQualifier;
	TQualifier qualifier;
	bool invariant;
	TPrecision precision;
	int primarySize;          // size of vector or matrix, not size of array
	int secondarySize;        // 1 for scalars/vectors, >1 for matrices
	bool array;
	int arraySize;
	TType* userDef;
	TSourceLoc line;

	void setBasic(TBasicType bt, TQualifier q, const TSourceLoc &ln)
	{
		type = bt;
		layoutQualifier = TLayoutQualifier::create();
		qualifier = q;
		invariant = false;
		precision = EbpUndefined;
		primarySize = 1;
		secondarySize = 1;
		array = false;
		arraySize = 0;
		userDef = 0;
		line = ln;
	}

	void setAggregate(int s)
	{
		primarySize = s;
		secondarySize = 1;
	}

	void setMatrix(int s0, int s1)
	{
		primarySize = s0;
		secondarySize = s1;
	}

	bool isUnsizedArray() const
	{
		return array && arraySize == 0;
	}

	void setArray(bool a, int s = 0)
	{
		array = a;
		arraySize = s;
	}

	void clearArrayness()
	{
		array = false;
		arraySize = 0;
	}

	bool isStructureContainingArrays() const
	{
		if (!userDef)
		{
			return false;
		}

		return userDef->isStructureContainingArrays();
	}

	bool isStructureContainingType(TBasicType t) const
	{
		if(!userDef)
		{
			return false;
		}

		return userDef->isStructureContainingType(t);
	}

	bool isMatrix() const
	{
		return primarySize > 1 && secondarySize > 1;
	}

	bool isVector() const
	{
		return primarySize > 1 && secondarySize == 1;
	}

	int getCols() const
	{
		ASSERT(isMatrix());
		return primarySize;
	}

	int getRows() const
	{
		ASSERT(isMatrix());
		return secondarySize;
	}

	int getNominalSize() const
	{
		return primarySize;
	}

	bool isAggregate() const
	{
		return array || isMatrix() || isVector();
	}
};

#endif // _TYPES_INCLUDED_
