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

#include "AnalyzeCallDepth.h"

static TIntermSequence::iterator
traverseCaseBody(AnalyzeCallDepth* analysis,
				 TIntermSequence::iterator& start,
				 const TIntermSequence::iterator& end) {
	TIntermSequence::iterator current = start;
	for (++current; current != end; ++current)
	{
		(*current)->traverse(analysis);
		if((*current)->getAsBranchNode()) // Kill, Break, Continue or Return
		{
			break;
		}
	}
	return current;
}


AnalyzeCallDepth::FunctionNode::FunctionNode(TIntermAggregate *node) : node(node)
{
	visit = PreVisit;
	callDepth = 0;
}

const TString &AnalyzeCallDepth::FunctionNode::getName() const
{
	return node->getName();
}

void AnalyzeCallDepth::FunctionNode::addCallee(AnalyzeCallDepth::FunctionNode *callee)
{
	for(size_t i = 0; i < callees.size(); i++)
	{
		if(callees[i] == callee)
		{
			return;
		}
	}

	callees.push_back(callee);
}

unsigned int AnalyzeCallDepth::FunctionNode::analyzeCallDepth(AnalyzeCallDepth *analyzeCallDepth)
{
	ASSERT(visit == PreVisit);
	ASSERT(analyzeCallDepth);

	callDepth = 0;
	visit = InVisit;

	for(size_t i = 0; i < callees.size(); i++)
	{
		unsigned int calleeDepth = 0;
		switch(callees[i]->visit)
		{
		case InVisit:
			// Cycle detected (recursion)
			return UINT_MAX;
		case PostVisit:
			calleeDepth = callees[i]->getLastDepth();
			break;
		case PreVisit:
			calleeDepth = callees[i]->analyzeCallDepth(analyzeCallDepth);
			break;
		default:
			UNREACHABLE(callees[i]->visit);
			break;
		}
		if(calleeDepth != UINT_MAX) ++calleeDepth;
		callDepth = std::max(callDepth, calleeDepth);
	}

	visit = PostVisit;
	return callDepth;
}

unsigned int AnalyzeCallDepth::FunctionNode::getLastDepth() const
{
	return callDepth;
}

void AnalyzeCallDepth::FunctionNode::removeIfUnreachable()
{
	if(visit == PreVisit)
	{
		node->setOp(EOpPrototype);
		node->getSequence().resize(1);   // Remove function body
	}
}

AnalyzeCallDepth::AnalyzeCallDepth(TIntermNode *root)
	: TIntermTraverser(true, false, true, false),
	  currentFunction(0)
{
	root->traverse(this);
}

AnalyzeCallDepth::~AnalyzeCallDepth()
{
	for(size_t i = 0; i < functions.size(); i++)
	{
		delete functions[i];
	}
}

bool AnalyzeCallDepth::visitSwitch(Visit visit, TIntermSwitch *node)
{
	TIntermTyped* switchValue = node->getInit();
	TIntermAggregate* opList = node->getStatementList();

	if(!switchValue || !opList)
	{
		return false;
	}

	// TODO: We need to dig into switch statement cases from
	// visitSwitch for all traversers. Is there a way to
	// preserve existing functionality while moving the iteration
	// to the general traverser?
	TIntermSequence& sequence = opList->getSequence();
	TIntermSequence::iterator it = sequence.begin();
	TIntermSequence::iterator defaultIt = sequence.end();
	for(; it != sequence.end(); ++it)
	{
		TIntermCase* currentCase = (*it)->getAsCaseNode();
		if(currentCase)
		{
			TIntermSequence::iterator caseIt = it;
			TIntermTyped* condition = currentCase->getCondition();
			if(condition) // non default case
			{
				condition->traverse(this);
				traverseCaseBody(this, caseIt, sequence.end());
			}
			else
			{
				defaultIt = it; // The default case might not be the last case, keep it for last
			}
		}
	}

	// If there's a default case, traverse it here
	if(defaultIt != sequence.end())
	{
		traverseCaseBody(this, defaultIt, sequence.end());
	}
	return false;
}

bool AnalyzeCallDepth::visitAggregate(Visit visit, TIntermAggregate *node)
{
	switch(node->getOp())
	{
	case EOpFunction:   // Function definition
		{
			if(visit == PreVisit)
			{
				currentFunction = findFunctionByName(node->getName());

				if(!currentFunction)
				{
					currentFunction = new FunctionNode(node);
					functions.push_back(currentFunction);
				}
			}
			else if(visit == PostVisit)
			{
				currentFunction = 0;
			}
		}
		break;
	case EOpFunctionCall:
		{
			if(!node->isUserDefined())
			{
				return true;   // Check the arguments for function calls
			}

			if(visit == PreVisit)
			{
				FunctionNode *function = findFunctionByName(node->getName());

				if(!function)
				{
					function = new FunctionNode(node);
					functions.push_back(function);
				}

				if(currentFunction)
				{
					currentFunction->addCallee(function);
				}
				else
				{
					globalFunctionCalls.insert(function);
				}
			}
		}
		break;
	default:
		break;
	}

	return true;
}

unsigned int AnalyzeCallDepth::analyzeCallDepth()
{
	FunctionNode *main = findFunctionByName("main(");

	if(!main)
	{
		return 0;
	}

	unsigned int depth = main->analyzeCallDepth(this);
	if(depth != UINT_MAX) ++depth;

	for(FunctionSet::iterator globalCall = globalFunctionCalls.begin(); globalCall != globalFunctionCalls.end(); globalCall++)
	{
		unsigned int globalDepth = (*globalCall)->analyzeCallDepth(this);
		if(globalDepth != UINT_MAX) ++globalDepth;

		if(globalDepth > depth)
		{
			depth = globalDepth;
		}
	}

	for(size_t i = 0; i < functions.size(); i++)
	{
		functions[i]->removeIfUnreachable();
	}

	return depth;
}

AnalyzeCallDepth::FunctionNode *AnalyzeCallDepth::findFunctionByName(const TString &name)
{
	for(size_t i = 0; i < functions.size(); i++)
	{
		if(functions[i]->getName() == name)
		{
			return functions[i];
		}
	}

	return 0;
}

