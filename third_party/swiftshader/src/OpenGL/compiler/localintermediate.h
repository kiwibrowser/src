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

#ifndef _LOCAL_INTERMEDIATE_INCLUDED_
#define _LOCAL_INTERMEDIATE_INCLUDED_

#include "intermediate.h"

struct TVectorFields {
	int offsets[4];
	int num;
};

//
// Set of helper functions to help parse and build the tree.
//
class TInfoSink;
class TIntermediate {
public:
	POOL_ALLOCATOR_NEW_DELETE();

	TIntermediate(TInfoSink& i) : infoSink(i) { }
	TIntermSymbol* addSymbol(int Id, const TString&, const TType&, const TSourceLoc&);
	TIntermTyped* addBinaryMath(TOperator op, TIntermTyped* left, TIntermTyped* right, const TSourceLoc&);
	TIntermTyped* addAssign(TOperator op, TIntermTyped* left, TIntermTyped* right, const TSourceLoc&);
	TIntermTyped* addIndex(TOperator op, TIntermTyped* base, TIntermTyped* index, const TSourceLoc&);
	TIntermTyped* addUnaryMath(TOperator op, TIntermTyped* child, const TSourceLoc&, const TType*);
	TIntermAggregate* growAggregate(TIntermNode* left, TIntermNode* right, const TSourceLoc&);
	TIntermAggregate* makeAggregate(TIntermNode* node, const TSourceLoc&);
	TIntermAggregate* setAggregateOperator(TIntermNode*, TOperator, const TSourceLoc&);
	TIntermNode*  addSelection(TIntermTyped* cond, TIntermNodePair code, const TSourceLoc&);
	TIntermTyped* addSelection(TIntermTyped* cond, TIntermTyped* trueBlock, TIntermTyped* falseBlock, const TSourceLoc&);
	TIntermSwitch *addSwitch(TIntermTyped *init, TIntermAggregate *statementList, const TSourceLoc &line);
	TIntermCase *addCase(TIntermTyped *condition, const TSourceLoc &line);
	TIntermTyped* addComma(TIntermTyped* left, TIntermTyped* right, const TSourceLoc&);
	TIntermConstantUnion* addConstantUnion(ConstantUnion*, const TType&, const TSourceLoc&);
	TIntermTyped* promoteConstantUnion(TBasicType, TIntermConstantUnion*);
	bool parseConstTree(const TSourceLoc&, TIntermNode*, ConstantUnion*, TOperator, TType, bool singleConstantParam = false);
	TIntermNode* addLoop(TLoopType, TIntermNode*, TIntermTyped*, TIntermTyped*, TIntermNode*, const TSourceLoc&);
	TIntermBranch* addBranch(TOperator, const TSourceLoc&);
	TIntermBranch* addBranch(TOperator, TIntermTyped*, const TSourceLoc&);
	TIntermTyped* addSwizzle(TVectorFields&, const TSourceLoc&);
	bool postProcess(TIntermNode*);
	void outputTree(TIntermNode*);

protected:
	TInfoSink& infoSink;

private:
	void operator=(TIntermediate&); // prevent assignments
};

#endif // _LOCAL_INTERMEDIATE_INCLUDED_
