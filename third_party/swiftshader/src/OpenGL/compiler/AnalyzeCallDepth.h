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

#ifndef COMPILER_ANALYZE_CALL_DEPTH_H_
#define COMPILER_ANALYZE_CALL_DEPTH_H_

#include "intermediate.h"

#include <set>
#include <limits.h>

// Traverses intermediate tree to analyze call depth or detect function recursion
class AnalyzeCallDepth : public TIntermTraverser
{
public:
	AnalyzeCallDepth(TIntermNode *root);
	~AnalyzeCallDepth();

	virtual bool visitSwitch(Visit, TIntermSwitch*);
	virtual bool visitAggregate(Visit, TIntermAggregate*);

	unsigned int analyzeCallDepth();

private:
	class FunctionNode
	{
	public:
		FunctionNode(TIntermAggregate *node);

		const TString &getName() const;
		void addCallee(FunctionNode *callee);
		unsigned int analyzeCallDepth(AnalyzeCallDepth *analyzeCallDepth);
		unsigned int getLastDepth() const;

		void removeIfUnreachable();

	private:
		TIntermAggregate *const node;
		TVector<FunctionNode*> callees;

		Visit visit;
		unsigned int callDepth;
	};

	FunctionNode *findFunctionByName(const TString &name);

	std::vector<FunctionNode*> functions;
	typedef std::set<FunctionNode*> FunctionSet;
	FunctionSet globalFunctionCalls;
	FunctionNode *currentFunction;
};

#endif  // COMPILER_ANALYZE_CALL_DEPTH_H_
