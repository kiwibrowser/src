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

#ifndef COMPILER_TRANSLATOR_VALIDATESWITCH_H_
#define COMPILER_TRANSLATOR_VALIDATESWITCH_H_

#include "intermediate.h"
#include <set>

class TParseContext;

class ValidateSwitch : public TIntermTraverser
{
public:
	// Check for errors and output messages any remaining errors on the context.
	// Returns true if there are no errors.
	static bool validate(TBasicType switchType, TParseContext *context,
	                     TIntermAggregate *statementList, const TSourceLoc &loc);

	void visitSymbol(TIntermSymbol *) override;
	void visitConstantUnion(TIntermConstantUnion *) override;
	bool visitBinary(Visit, TIntermBinary *) override;
	bool visitUnary(Visit, TIntermUnary *) override;
	bool visitSelection(Visit visit, TIntermSelection *) override;
	bool visitSwitch(Visit, TIntermSwitch *) override;
	bool visitCase(Visit, TIntermCase *) override;
	bool visitAggregate(Visit, TIntermAggregate *) override;
	bool visitLoop(Visit visit, TIntermLoop *) override;
	bool visitBranch(Visit, TIntermBranch *) override;

private:
	ValidateSwitch(TBasicType switchType, TParseContext *context);

	bool validateInternal(const TSourceLoc &loc);

	TBasicType mSwitchType;
	TParseContext *mContext;
	bool mCaseTypeMismatch;
	bool mFirstCaseFound;
	bool mStatementBeforeCase;
	bool mLastStatementWasCase;
	int mControlFlowDepth;
	bool mCaseInsideControlFlow;
	int mDefaultCount;
	std::set<int> mCasesSigned;
	std::set<unsigned int> mCasesUnsigned;
	bool mDuplicateCases;
};

#endif // COMPILER_TRANSLATOR_VALIDATESWITCH_H_
