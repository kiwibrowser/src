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

#include "Compiler.h"
#include "intermediate.h"

class TInfoSinkBase;

struct TLoopInfo {
	struct TIndex {
		int id;  // symbol id.
	} index;
	TIntermLoop* loop;
};
typedef TVector<TLoopInfo> TLoopStack;

// Traverses intermediate tree to ensure that the shader does not exceed the
// minimum functionality mandated in GLSL 1.0 spec, Appendix A.
class ValidateLimitations : public TIntermTraverser {
public:
	ValidateLimitations(GLenum shaderType, TInfoSinkBase& sink);

	int numErrors() const { return mNumErrors; }

	virtual bool visitBinary(Visit, TIntermBinary*);
	virtual bool visitUnary(Visit, TIntermUnary*);
	virtual bool visitAggregate(Visit, TIntermAggregate*);
	virtual bool visitLoop(Visit, TIntermLoop*);

private:
	void error(TSourceLoc loc, const char *reason, const char* token);

	bool withinLoopBody() const;
	bool isLoopIndex(const TIntermSymbol* symbol) const;
	bool validateLoopType(TIntermLoop* node);
	bool validateForLoopHeader(TIntermLoop* node, TLoopInfo* info);
	bool validateForLoopInit(TIntermLoop* node, TLoopInfo* info);
	bool validateForLoopCond(TIntermLoop* node, TLoopInfo* info);
	bool validateForLoopExpr(TIntermLoop* node, TLoopInfo* info);
	// Returns true if none of the loop indices is used as the argument to
	// the given function out or inout parameter.
	bool validateFunctionCall(TIntermAggregate* node);
	bool validateOperation(TIntermOperator* node, TIntermNode* operand);

	// Returns true if indexing does not exceed the minimum functionality
	// mandated in GLSL 1.0 spec, Appendix A, Section 5.
	bool isConstExpr(TIntermNode* node);
	bool isConstIndexExpr(TIntermNode* node);
	bool validateIndexing(TIntermBinary* node);

	GLenum mShaderType;
	TInfoSinkBase& mSink;
	int mNumErrors;
	TLoopStack mLoopStack;
};

