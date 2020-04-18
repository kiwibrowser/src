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

//
// Symbol table for parsing.  Most functionaliy and main ideas
// are documented in the header file.
//

#if defined(_MSC_VER)
#pragma warning(disable: 4718)
#endif

#include "SymbolTable.h"

#include <stdio.h>
#include <limits.h>
#include <algorithm>

#if defined(_MSC_VER) && MSC_VER < 1900
#define snprintf _snprintf
#endif

int TSymbolTableLevel::uniqueId = 0;

TType::TType(const TPublicType &p) :
	type(p.type), precision(p.precision), qualifier(p.qualifier),
	primarySize(p.primarySize), secondarySize(p.secondarySize), array(p.array), arraySize(p.arraySize), maxArraySize(0),
	arrayInformationType(0), interfaceBlock(0), layoutQualifier(p.layoutQualifier), structure(0), mangled(0)
{
	if (p.userDef)
	{
		structure = p.userDef->getStruct();
	}
}

//
// Recursively generate mangled names.
//
void TType::buildMangledName(TString& mangledName)
{
	if (isMatrix())
		mangledName += 'm';
	else if (isVector())
		mangledName += 'v';

	switch (type) {
	case EbtFloat:              mangledName += 'f';      break;
	case EbtInt:                mangledName += 'i';      break;
	case EbtUInt:               mangledName += 'u';      break;
	case EbtBool:               mangledName += 'b';      break;
	case EbtSampler2D:          mangledName += "s2";     break;
	case EbtSampler3D:          mangledName += "s3";     break;
	case EbtSamplerCube:        mangledName += "sC";     break;
	case EbtSampler2DArray:		mangledName += "s2a";    break;
	case EbtSampler2DRect:      mangledName += "s2r";    break;
	case EbtSamplerExternalOES: mangledName += "sext";   break;
	case EbtISampler2D:  		mangledName += "is2";    break;
	case EbtISampler3D: 		mangledName += "is3";    break;
	case EbtISamplerCube:		mangledName += "isC";    break;
	case EbtISampler2DArray:	mangledName += "is2a";   break;
	case EbtUSampler2D: 		mangledName += "us2";    break;
	case EbtUSampler3D:  		mangledName += "us3";    break;
	case EbtUSamplerCube:		mangledName += "usC";    break;
	case EbtUSampler2DArray:	mangledName += "us2a";   break;
	case EbtSampler2DShadow:	mangledName += "s2s";    break;
	case EbtSamplerCubeShadow:  mangledName += "sCs";    break;
	case EbtSampler2DArrayShadow: mangledName += "s2as"; break;
	case EbtStruct:             mangledName += structure->mangledName(); break;
	case EbtInterfaceBlock:	    mangledName += interfaceBlock->mangledName(); break;
	default:
		break;
	}

	mangledName += static_cast<char>('0' + getNominalSize());
	if(isMatrix()) {
		mangledName += static_cast<char>('0' + getSecondarySize());
	}
	if (isArray()) {
		char buf[20];
		snprintf(buf, sizeof(buf), "%d", arraySize);
		mangledName += '[';
		mangledName += buf;
		mangledName += ']';
	}
}

size_t TType::getStructSize() const
{
	if (!getStruct()) {
		assert(false && "Not a struct");
		return 0;
	}

	return getStruct()->objectSize();
}

bool TStructure::containsArrays() const
{
	for(const auto& field : *mFields)
	{
		const TType *fieldType = field->type();
		if(fieldType->isArray() || fieldType->isStructureContainingArrays())
			return true;
	}
	return false;
}

bool TStructure::containsType(TBasicType type) const
{
	for(const auto& field : *mFields)
	{
		const TType *fieldType = field->type();
		if(fieldType->getBasicType() == type || fieldType->isStructureContainingType(type))
			return true;
	}
	return false;
}

bool TStructure::containsSamplers() const
{
	for(const auto& field : *mFields)
	{
		const TType *fieldType = field->type();
		if(IsSampler(fieldType->getBasicType()) || fieldType->isStructureContainingSamplers())
			return true;
	}
	return false;
}

void TStructure::setMatrixPackingIfUnspecified(TLayoutMatrixPacking matrixPacking)
{
	for(auto& field : *mFields)
	{
		field->type()->setMatrixPackingIfUnspecified(matrixPacking);
	}
}

TString TFieldListCollection::buildMangledName() const
{
	TString mangledName(mangledNamePrefix());
	mangledName += *mName;
	for(const auto& field : *mFields)
	{
		mangledName += '-';
		mangledName += field->type()->getMangledName();
	}
	return mangledName;
}

size_t TFieldListCollection::calculateObjectSize() const
{
	size_t size = 0;
	for(const auto& field : *mFields)
	{
		size_t fieldSize = field->type()->getObjectSize();
		if(fieldSize > INT_MAX - size)
			size = INT_MAX;
		else
			size += fieldSize;
	}
	return size;
}

int TStructure::calculateDeepestNesting() const
{
	int maxNesting = 0;
	for(const auto& field : *mFields)
		maxNesting = std::max(maxNesting, field->type()->getDeepestStructNesting());
	return 1 + maxNesting;
}

//
// Functions have buried pointers to delete.
//
TFunction::~TFunction()
{
	for (TParamList::iterator i = parameters.begin(); i != parameters.end(); ++i)
		delete (*i).type;
}

//
// Symbol table levels are a map of pointers to symbols that have to be deleted.
//
TSymbolTableLevel::~TSymbolTableLevel()
{
	for (tLevel::iterator it = level.begin(); it != level.end(); ++it)
		delete (*it).second;
}

bool TSymbolTableLevel::insert(TSymbol *symbol)
{
	symbol->setUniqueId(nextUniqueId());

	// returning true means symbol was added to the table
	tInsertResult result = level.insert(tLevelPair(symbol->getMangledName(), symbol));

	return result.second;
}

bool TSymbolTableLevel::insertUnmangled(TFunction *function)
{
	function->setUniqueId(nextUniqueId());

	// returning true means symbol was added to the table
	tInsertResult result = level.insert(tLevelPair(function->getName(), function));

	return result.second;
}

TSymbol *TSymbolTableLevel::find(const TString &name) const
{
	tLevel::const_iterator it = level.find(name);
	if (it == level.end())
		return 0;
	else
		return (*it).second;
}

TSymbol *TSymbolTable::find(const TString &name, int shaderVersion, bool *builtIn, bool *sameScope) const
{
	int level = currentLevel();
	TSymbol *symbol = nullptr;

	do
	{
		while((level == ESSL3_BUILTINS && shaderVersion != 300) ||
		      (level == ESSL1_BUILTINS && shaderVersion != 100))   // Skip version specific levels
		{
			--level;
		}

		symbol = table[level]->find(name);
	}
	while(!symbol && --level >= 0);   // Doesn't decrement level when a symbol was found

	if(builtIn)
	{
		*builtIn = (level <= LAST_BUILTIN_LEVEL);
	}

	if(sameScope)
	{
		*sameScope = (level == currentLevel());
	}

	return symbol;
}

TSymbol *TSymbolTable::findBuiltIn(const TString &name, int shaderVersion) const
{
	for(int level = LAST_BUILTIN_LEVEL; level >= 0; --level)
	{
		while((level == ESSL3_BUILTINS && shaderVersion != 300) ||
		      (level == ESSL1_BUILTINS && shaderVersion != 100))   // Skip version specific levels
		{
			--level;
		}

		TSymbol *symbol = table[level]->find(name);

		if(symbol)
		{
			return symbol;
		}
	}

	return 0;
}

TSymbol::TSymbol(const TSymbol& copyOf)
{
	name = NewPoolTString(copyOf.name->c_str());
}
