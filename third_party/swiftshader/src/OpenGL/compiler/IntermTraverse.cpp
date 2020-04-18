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

#include "intermediate.h"

//
// Traverse the intermediate representation tree, and
// call a node type specific function for each node.
// Done recursively through the member function Traverse().
// Node types can be skipped if their function to call is 0,
// but their subtree will still be traversed.
// Nodes with children can have their whole subtree skipped
// if preVisit is turned on and the type specific function
// returns false.
//
// preVisit, postVisit, and rightToLeft control what order
// nodes are visited in.
//

//
// Traversal functions for terminals are straighforward....
//
void TIntermSymbol::traverse(TIntermTraverser* it)
{
	it->visitSymbol(this);
}

void TIntermConstantUnion::traverse(TIntermTraverser* it)
{
	it->visitConstantUnion(this);
}

//
// Traverse a binary node.
//
void TIntermBinary::traverse(TIntermTraverser* it)
{
	bool visit = true;

	//
	// visit the node before children if pre-visiting.
	//
	if(it->preVisit)
	{
		visit = it->visitBinary(PreVisit, this);
	}

	//
	// Visit the children, in the right order.
	//
	if(visit)
	{
		it->incrementDepth(this);

		if(it->rightToLeft)
		{
			if(right)
			{
				right->traverse(it);
			}

			if(it->inVisit)
			{
				visit = it->visitBinary(InVisit, this);
			}

			if(visit && left)
			{
				left->traverse(it);
			}
		}
		else
		{
			if(left)
			{
				left->traverse(it);
			}

			if(it->inVisit)
			{
				visit = it->visitBinary(InVisit, this);
			}

			if(visit && right)
			{
				right->traverse(it);
			}
		}

		it->decrementDepth();
	}

	//
	// Visit the node after the children, if requested and the traversal
	// hasn't been cancelled yet.
	//
	if(visit && it->postVisit)
	{
		it->visitBinary(PostVisit, this);
	}
}

//
// Traverse a unary node.  Same comments in binary node apply here.
//
void TIntermUnary::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if (it->preVisit)
		visit = it->visitUnary(PreVisit, this);

	if (visit) {
		it->incrementDepth(this);
		operand->traverse(it);
		it->decrementDepth();
	}

	if (visit && it->postVisit)
		it->visitUnary(PostVisit, this);
}

//
// Traverse an aggregate node.  Same comments in binary node apply here.
//
void TIntermAggregate::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if(it->preVisit)
	{
		visit = it->visitAggregate(PreVisit, this);
	}

	if(visit)
	{
		it->incrementDepth(this);

		if(it->rightToLeft)
		{
			for(TIntermSequence::reverse_iterator sit = sequence.rbegin(); sit != sequence.rend(); sit++)
			{
				(*sit)->traverse(it);

				if(visit && it->inVisit)
				{
					if(*sit != sequence.front())
					{
						visit = it->visitAggregate(InVisit, this);
					}
				}
			}
		}
		else
		{
			for(TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); sit++)
			{
				(*sit)->traverse(it);

				if(visit && it->inVisit)
				{
					if(*sit != sequence.back())
					{
						visit = it->visitAggregate(InVisit, this);
					}
				}
			}
		}

		it->decrementDepth();
	}

	if(visit && it->postVisit)
	{
		it->visitAggregate(PostVisit, this);
	}
}

//
// Traverse a selection node.  Same comments in binary node apply here.
//
void TIntermSelection::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if (it->preVisit)
		visit = it->visitSelection(PreVisit, this);

	if (visit) {
		it->incrementDepth(this);
		if (it->rightToLeft) {
			if (falseBlock)
				falseBlock->traverse(it);
			if (trueBlock)
				trueBlock->traverse(it);
			condition->traverse(it);
		} else {
			condition->traverse(it);
			if (trueBlock)
				trueBlock->traverse(it);
			if (falseBlock)
				falseBlock->traverse(it);
		}
		it->decrementDepth();
	}

	if (visit && it->postVisit)
		it->visitSelection(PostVisit, this);
}

//
// Traverse a switch node.  Same comments in binary node apply here.
//
void TIntermSwitch::traverse(TIntermTraverser *it)
{
	bool visit = true;

	if(it->preVisit)
		visit = it->visitSwitch(PreVisit, this);

	if(visit)
	{
		it->incrementDepth(this);
		if(it->inVisit)
			visit = it->visitSwitch(InVisit, this);
		it->decrementDepth();
	}

	if(visit && it->postVisit)
		it->visitSwitch(PostVisit, this);
}

//
// Traverse a switch node.  Same comments in binary node apply here.
//
void TIntermCase::traverse(TIntermTraverser *it)
{
	bool visit = true;

	if(it->preVisit)
		visit = it->visitCase(PreVisit, this);

	if(visit && mCondition)
		mCondition->traverse(it);

	if(visit && it->postVisit)
		it->visitCase(PostVisit, this);
}

//
// Traverse a loop node.  Same comments in binary node apply here.
//
void TIntermLoop::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if(it->preVisit)
	{
		visit = it->visitLoop(PreVisit, this);
	}

	if(visit)
	{
		it->incrementDepth(this);

		if(it->rightToLeft)
		{
			if(expr)
			{
				expr->traverse(it);
			}

			if(body)
			{
				body->traverse(it);
			}

			if(cond)
			{
				cond->traverse(it);
			}
		}
		else
		{
			if(cond)
			{
				cond->traverse(it);
			}

			if(body)
			{
				body->traverse(it);
			}

			if(expr)
			{
				expr->traverse(it);
			}
		}

		it->decrementDepth();
	}

	if(visit && it->postVisit)
	{
		it->visitLoop(PostVisit, this);
	}
}

//
// Traverse a branch node.  Same comments in binary node apply here.
//
void TIntermBranch::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if (it->preVisit)
		visit = it->visitBranch(PreVisit, this);

	if (visit && expression) {
		it->incrementDepth(this);
		expression->traverse(it);
		it->decrementDepth();
	}

	if (visit && it->postVisit)
		it->visitBranch(PostVisit, this);
}

