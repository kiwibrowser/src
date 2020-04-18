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

#ifndef _SYMBOL_TABLE_INCLUDED_
#define _SYMBOL_TABLE_INCLUDED_

//
// Symbol table for parsing.  Has these design characteristics:
//
// * Same symbol table can be used to compile many shaders, to preserve
//   effort of creating and loading with the large numbers of built-in
//   symbols.
//
// * Name mangling will be used to give each function a unique name
//   so that symbol table lookups are never ambiguous.  This allows
//   a simpler symbol table structure.
//
// * Pushing and popping of scope, so symbol table will really be a stack
//   of symbol tables.  Searched from the top, with new inserts going into
//   the top.
//
// * Constants:  Compile time constant symbols will keep their values
//   in the symbol table.  The parser can substitute constants at parse
//   time, including doing constant folding and constant propagation.
//
// * No temporaries:  Temporaries made from operations (+, --, .xy, etc.)
//   are tracked in the intermediate representation, not the symbol table.
//

#ifndef __ANDROID__
#include <assert.h>
#else
#include "../../Common/DebugAndroid.hpp"
#endif

#include "InfoSink.h"
#include "intermediate.h"
#include <set>

//
// Symbol base class.  (Can build functions or variables out of these...)
//
class TSymbol
{
public:
	POOL_ALLOCATOR_NEW_DELETE();
	TSymbol(const TString *n) :  name(n) { }
	virtual ~TSymbol() { /* don't delete name, it's from the pool */ }

	const TString& getName() const { return *name; }
	virtual const TString& getMangledName() const { return getName(); }
	virtual bool isFunction() const { return false; }
	virtual bool isVariable() const { return false; }
	void setUniqueId(int id) { uniqueId = id; }
	int getUniqueId() const { return uniqueId; }
	TSymbol(const TSymbol&);

protected:
	const TString *name;
	unsigned int uniqueId;      // For real comparing during code generation
};

//
// Variable class, meaning a symbol that's not a function.
//
// There could be a separate class heirarchy for Constant variables;
// Only one of int, bool, or float, (or none) is correct for
// any particular use, but it's easy to do this way, and doesn't
// seem worth having separate classes, and "getConst" can't simply return
// different values for different types polymorphically, so this is
// just simple and pragmatic.
//
class TVariable : public TSymbol
{
public:
	TVariable(const TString *name, const TType& t, bool uT = false ) : TSymbol(name), type(t), userType(uT), unionArray(0), arrayInformationType(0) { }
	virtual ~TVariable() { }
	virtual bool isVariable() const { return true; }
	TType& getType() { return type; }
	const TType& getType() const { return type; }
	bool isUserType() const { return userType; }
	void setQualifier(TQualifier qualifier) { type.setQualifier(qualifier); }
	void updateArrayInformationType(TType *t) { arrayInformationType = t; }
	TType* getArrayInformationType() { return arrayInformationType; }

	ConstantUnion* getConstPointer()
	{
		if (!unionArray)
			unionArray = new ConstantUnion[type.getObjectSize()];

		return unionArray;
	}

	ConstantUnion* getConstPointer() const { return unionArray; }
	bool isConstant() const { return unionArray != nullptr; }

	void shareConstPointer( ConstantUnion *constArray)
	{
		if (unionArray == constArray)
			return;

		delete[] unionArray;
		unionArray = constArray;
	}

protected:
	TType type;
	bool userType;
	// we are assuming that Pool Allocator will free the memory allocated to unionArray
	// when this object is destroyed
	ConstantUnion *unionArray;
	TType *arrayInformationType;  // this is used for updating maxArraySize in all the references to a given symbol
};

//
// The function sub-class of symbols and the parser will need to
// share this definition of a function parameter.
//
struct TParameter
{
	TString *name;
	TType *type;
};

//
// The function sub-class of a symbol.
//
class TFunction : public TSymbol
{
public:
	TFunction(TOperator o) :
		TSymbol(0),
		returnType(TType(EbtVoid, EbpUndefined)),
		op(o),
		defined(false),
		prototypeDeclaration(false) { }
	TFunction(const TString *name, const TType& retType, TOperator tOp = EOpNull, const char *ext = "") :
		TSymbol(name),
		returnType(retType),
		mangledName(TFunction::mangleName(*name)),
		op(tOp),
		extension(ext),
		defined(false),
		prototypeDeclaration(false) { }
	virtual ~TFunction();
	virtual bool isFunction() const { return true; }

	static TString mangleName(const TString& name) { return name + '('; }
	static TString unmangleName(const TString& mangledName)
	{
		return TString(mangledName.c_str(), mangledName.find_first_of('('));
	}

	void addParameter(TParameter& p)
	{
		parameters.push_back(p);
		mangledName = mangledName + p.type->getMangledName();
	}

	const TString& getMangledName() const { return mangledName; }
	const TType& getReturnType() const { return returnType; }

	TOperator getBuiltInOp() const { return op; }
	const TString& getExtension() const { return extension; }

	void setDefined() { defined = true; }
	bool isDefined() { return defined; }
	void setHasPrototypeDeclaration() { prototypeDeclaration = true; }
	bool hasPrototypeDeclaration() const { return prototypeDeclaration; }

	size_t getParamCount() const { return parameters.size(); }
	const TParameter& getParam(int i) const { return parameters[i]; }

protected:
	typedef TVector<TParameter> TParamList;
	TParamList parameters;
	TType returnType;
	TString mangledName;
	TOperator op;
	TString extension;
	bool defined;
	bool prototypeDeclaration;
};


class TSymbolTableLevel
{
public:
	typedef TMap<TString, TSymbol*> tLevel;
	typedef tLevel::const_iterator const_iterator;
	typedef const tLevel::value_type tLevelPair;
	typedef std::pair<tLevel::iterator, bool> tInsertResult;

	POOL_ALLOCATOR_NEW_DELETE();
	TSymbolTableLevel() { }
	~TSymbolTableLevel();

	bool insert(TSymbol *symbol);

	// Insert a function using its unmangled name as the key.
	bool insertUnmangled(TFunction *function);

	TSymbol *find(const TString &name) const;

	static int nextUniqueId()
	{
		return ++uniqueId;
	}

protected:
	tLevel level;
	static int uniqueId;     // for unique identification in code generation
};

enum ESymbolLevel
{
	COMMON_BUILTINS,
	ESSL1_BUILTINS,
	ESSL3_BUILTINS,
	LAST_BUILTIN_LEVEL = ESSL3_BUILTINS,
	GLOBAL_LEVEL
};

inline bool IsGenType(const TType *type)
{
	if(type)
	{
		TBasicType basicType = type->getBasicType();
		return basicType == EbtGenType || basicType == EbtGenIType || basicType == EbtGenUType || basicType == EbtGenBType;
	}

	return false;
}

inline bool IsVecType(const TType *type)
{
	if(type)
	{
		TBasicType basicType = type->getBasicType();
		return basicType == EbtVec || basicType == EbtIVec || basicType == EbtUVec || basicType == EbtBVec;
	}

	return false;
}

inline TType *GenType(TType *type, int size)
{
	ASSERT(size >= 1 && size <= 4);

	if(!type)
	{
		return nullptr;
	}

	ASSERT(!IsVecType(type));

	switch(type->getBasicType())
	{
	case EbtGenType:  return new TType(EbtFloat, size);
	case EbtGenIType: return new TType(EbtInt, size);
	case EbtGenUType: return new TType(EbtUInt, size);
	case EbtGenBType: return new TType(EbtBool, size);
	default: return type;
	}
}

inline TType *VecType(TType *type, int size)
{
	ASSERT(size >= 2 && size <= 4);

	if(!type)
	{
		return nullptr;
	}

	ASSERT(!IsGenType(type));

	switch(type->getBasicType())
	{
	case EbtVec:  return new TType(EbtFloat, size);
	case EbtIVec: return new TType(EbtInt, size);
	case EbtUVec: return new TType(EbtUInt, size);
	case EbtBVec: return new TType(EbtBool, size);
	default: return type;
	}
}

class TSymbolTable
{
public:
	TSymbolTable()
		: mGlobalInvariant(false)
	{
		//
		// The symbol table cannot be used until push() is called, but
		// the lack of an initial call to push() can be used to detect
		// that the symbol table has not been preloaded with built-ins.
		//
	}

	~TSymbolTable()
	{
		while(currentLevel() > LAST_BUILTIN_LEVEL)
		{
			pop();
		}
	}

	bool isEmpty() { return table.empty(); }
	bool atBuiltInLevel() { return currentLevel() <= LAST_BUILTIN_LEVEL; }
	bool atGlobalLevel() { return currentLevel() <= GLOBAL_LEVEL; }
	void push()
	{
		table.push_back(new TSymbolTableLevel);
		precisionStack.push_back( PrecisionStackLevel() );
	}

	void pop()
	{
		delete table[currentLevel()];
		table.pop_back();
		precisionStack.pop_back();
	}

	bool declare(TSymbol *symbol)
	{
		return insert(currentLevel(), symbol);
	}

	bool insert(ESymbolLevel level, TSymbol *symbol)
	{
		return table[level]->insert(symbol);
	}

	bool insertConstInt(ESymbolLevel level, const char *name, int value)
	{
		TVariable *constant = new TVariable(NewPoolTString(name), TType(EbtInt, EbpUndefined, EvqConstExpr, 1));
		constant->getConstPointer()->setIConst(value);
		return insert(level, constant);
	}

	void insertBuiltIn(ESymbolLevel level, TOperator op, const char *ext, TType *rvalue, const char *name, TType *ptype1, TType *ptype2 = 0, TType *ptype3 = 0, TType *ptype4 = 0, TType *ptype5 = 0)
	{
		if(ptype1->getBasicType() == EbtGSampler2D)
		{
			insertUnmangledBuiltIn(name);
			bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
			insertBuiltIn(level, gvec4 ? new TType(EbtFloat, 4) : rvalue, name, new TType(EbtSampler2D), ptype2, ptype3, ptype4, ptype5);
			insertBuiltIn(level, gvec4 ? new TType(EbtInt, 4) : rvalue, name, new TType(EbtISampler2D), ptype2, ptype3, ptype4, ptype5);
			insertBuiltIn(level, gvec4 ? new TType(EbtUInt, 4) : rvalue, name, new TType(EbtUSampler2D), ptype2, ptype3, ptype4, ptype5);
		}
		else if(ptype1->getBasicType() == EbtGSampler3D)
		{
			insertUnmangledBuiltIn(name);
			bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
			insertBuiltIn(level, gvec4 ? new TType(EbtFloat, 4) : rvalue, name, new TType(EbtSampler3D), ptype2, ptype3, ptype4, ptype5);
			insertBuiltIn(level, gvec4 ? new TType(EbtInt, 4) : rvalue, name, new TType(EbtISampler3D), ptype2, ptype3, ptype4, ptype5);
			insertBuiltIn(level, gvec4 ? new TType(EbtUInt, 4) : rvalue, name, new TType(EbtUSampler3D), ptype2, ptype3, ptype4, ptype5);
		}
		else if(ptype1->getBasicType() == EbtGSamplerCube)
		{
			insertUnmangledBuiltIn(name);
			bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
			insertBuiltIn(level, gvec4 ? new TType(EbtFloat, 4) : rvalue, name, new TType(EbtSamplerCube), ptype2, ptype3, ptype4, ptype5);
			insertBuiltIn(level, gvec4 ? new TType(EbtInt, 4) : rvalue, name, new TType(EbtISamplerCube), ptype2, ptype3, ptype4, ptype5);
			insertBuiltIn(level, gvec4 ? new TType(EbtUInt, 4) : rvalue, name, new TType(EbtUSamplerCube), ptype2, ptype3, ptype4, ptype5);
		}
		else if(ptype1->getBasicType() == EbtGSampler2DArray)
		{
			insertUnmangledBuiltIn(name);
			bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
			insertBuiltIn(level, gvec4 ? new TType(EbtFloat, 4) : rvalue, name, new TType(EbtSampler2DArray), ptype2, ptype3, ptype4, ptype5);
			insertBuiltIn(level, gvec4 ? new TType(EbtInt, 4) : rvalue, name, new TType(EbtISampler2DArray), ptype2, ptype3, ptype4, ptype5);
			insertBuiltIn(level, gvec4 ? new TType(EbtUInt, 4) : rvalue, name, new TType(EbtUSampler2DArray), ptype2, ptype3, ptype4, ptype5);
		}
		else if(IsGenType(rvalue) || IsGenType(ptype1) || IsGenType(ptype2) || IsGenType(ptype3))
		{
			ASSERT(!ptype4);
			insertUnmangledBuiltIn(name);
			insertBuiltIn(level, op, ext, GenType(rvalue, 1), name, GenType(ptype1, 1), GenType(ptype2, 1), GenType(ptype3, 1));
			insertBuiltIn(level, op, ext, GenType(rvalue, 2), name, GenType(ptype1, 2), GenType(ptype2, 2), GenType(ptype3, 2));
			insertBuiltIn(level, op, ext, GenType(rvalue, 3), name, GenType(ptype1, 3), GenType(ptype2, 3), GenType(ptype3, 3));
			insertBuiltIn(level, op, ext, GenType(rvalue, 4), name, GenType(ptype1, 4), GenType(ptype2, 4), GenType(ptype3, 4));
		}
		else if(IsVecType(rvalue) || IsVecType(ptype1) || IsVecType(ptype2) || IsVecType(ptype3))
		{
			ASSERT(!ptype4);
			insertUnmangledBuiltIn(name);
			insertBuiltIn(level, op, ext, VecType(rvalue, 2), name, VecType(ptype1, 2), VecType(ptype2, 2), VecType(ptype3, 2));
			insertBuiltIn(level, op, ext, VecType(rvalue, 3), name, VecType(ptype1, 3), VecType(ptype2, 3), VecType(ptype3, 3));
			insertBuiltIn(level, op, ext, VecType(rvalue, 4), name, VecType(ptype1, 4), VecType(ptype2, 4), VecType(ptype3, 4));
		}
		else
		{
			TFunction *function = new TFunction(NewPoolTString(name), *rvalue, op, ext);

			TParameter param1 = {0, ptype1};
			function->addParameter(param1);

			if(ptype2)
			{
				TParameter param2 = {0, ptype2};
				function->addParameter(param2);
			}

			if(ptype3)
			{
				TParameter param3 = {0, ptype3};
				function->addParameter(param3);
			}

			if(ptype4)
			{
				TParameter param4 = {0, ptype4};
				function->addParameter(param4);
			}

			if(ptype5)
			{
				TParameter param5 = {0, ptype5};
				function->addParameter(param5);
			}

			ASSERT(hasUnmangledBuiltIn(name));
			insert(level, function);
		}
	}

	void insertBuiltIn(ESymbolLevel level, TOperator op, TType *rvalue, const char *name, TType *ptype1, TType *ptype2 = 0, TType *ptype3 = 0, TType *ptype4 = 0, TType *ptype5 = 0)
	{
		insertUnmangledBuiltIn(name);
		insertBuiltIn(level, op, "", rvalue, name, ptype1, ptype2, ptype3, ptype4, ptype5);
	}

	void insertBuiltIn(ESymbolLevel level, TType *rvalue, const char *name, TType *ptype1, TType *ptype2 = 0, TType *ptype3 = 0, TType *ptype4 = 0, TType *ptype5 = 0)
	{
		insertUnmangledBuiltIn(name);
		insertBuiltIn(level, EOpNull, rvalue, name, ptype1, ptype2, ptype3, ptype4, ptype5);
	}

	TSymbol *find(const TString &name, int shaderVersion, bool *builtIn = nullptr, bool *sameScope = nullptr) const;
	TSymbol *findBuiltIn(const TString &name, int shaderVersion) const;

	TSymbolTableLevel *getOuterLevel() const
	{
		assert(currentLevel() >= 1);
		return table[currentLevel() - 1];
	}

	bool setDefaultPrecision(const TPublicType &type, TPrecision prec)
	{
		if (IsSampler(type.type))
			return true;  // Skip sampler types for the time being
		if (type.type != EbtFloat && type.type != EbtInt)
			return false; // Only set default precision for int/float
		if (type.primarySize > 1 || type.secondarySize > 1 || type.array)
			return false; // Not allowed to set for aggregate types
		int indexOfLastElement = static_cast<int>(precisionStack.size()) - 1;
		precisionStack[indexOfLastElement][type.type] = prec; // Uses map operator [], overwrites the current value
		return true;
	}

	// Searches down the precisionStack for a precision qualifier for the specified TBasicType
	TPrecision getDefaultPrecision( TBasicType type)
	{
		// unsigned integers use the same precision as signed
		if (type == EbtUInt) type = EbtInt;

		if( type != EbtFloat && type != EbtInt ) return EbpUndefined;
		int level = static_cast<int>(precisionStack.size()) - 1;
		assert( level >= 0); // Just to be safe. Should not happen.
		PrecisionStackLevel::iterator it;
		TPrecision prec = EbpUndefined; // If we dont find anything we return this. Should we error check this?
		while( level >= 0 ){
			it = precisionStack[level].find( type );
			if( it != precisionStack[level].end() ){
				prec = (*it).second;
				break;
			}
			level--;
		}
		return prec;
	}

	// This records invariant varyings declared through
	// "invariant varying_name;".
	void addInvariantVarying(const std::string &originalName)
	{
		mInvariantVaryings.insert(originalName);
	}
	// If this returns false, the varying could still be invariant
	// if it is set as invariant during the varying variable
	// declaration - this piece of information is stored in the
	// variable's type, not here.
	bool isVaryingInvariant(const std::string &originalName) const
	{
		return (mGlobalInvariant ||
			mInvariantVaryings.count(originalName) > 0);
	}

	void setGlobalInvariant() { mGlobalInvariant = true; }
	bool getGlobalInvariant() const { return mGlobalInvariant; }

	bool hasUnmangledBuiltIn(const char *name) { return mUnmangledBuiltinNames.count(std::string(name)) > 0; }

private:
	// Used to insert unmangled functions to check redeclaration of built-ins in ESSL 3.00.
	void insertUnmangledBuiltIn(const char *name) { mUnmangledBuiltinNames.insert(std::string(name)); }

protected:
	ESymbolLevel currentLevel() const { return static_cast<ESymbolLevel>(table.size() - 1); }

	std::vector<TSymbolTableLevel*> table;
	typedef std::map< TBasicType, TPrecision > PrecisionStackLevel;
	std::vector< PrecisionStackLevel > precisionStack;

	std::set<std::string> mUnmangledBuiltinNames;

	std::set<std::string> mInvariantVaryings;
	bool mGlobalInvariant;
};

#endif // _SYMBOL_TABLE_INCLUDED_
