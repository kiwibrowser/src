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
// Build the intermediate representation.
//

#include <float.h>
#include <limits.h>
#include <algorithm>

#include "localintermediate.h"
#include "SymbolTable.h"
#include "Common/Math.hpp"

bool CompareStructure(const TType& leftNodeType, ConstantUnion* rightUnionArray, ConstantUnion* leftUnionArray);

static TPrecision GetHigherPrecision( TPrecision left, TPrecision right ){
	return left > right ? left : right;
}

static bool ValidateMultiplication(TOperator op, const TType &left, const TType &right)
{
	switch(op)
	{
	case EOpMul:
	case EOpMulAssign:
		return left.getNominalSize() == right.getNominalSize() &&
		       left.getSecondarySize() == right.getSecondarySize();
	case EOpVectorTimesScalar:
	case EOpVectorTimesScalarAssign:
		return true;
	case EOpVectorTimesMatrix:
		return left.getNominalSize() == right.getSecondarySize();
	case EOpVectorTimesMatrixAssign:
		return left.getNominalSize() == right.getSecondarySize() &&
		       left.getNominalSize() == right.getNominalSize();
	case EOpMatrixTimesVector:
		return left.getNominalSize() == right.getNominalSize();
	case EOpMatrixTimesScalar:
	case EOpMatrixTimesScalarAssign:
		return true;
	case EOpMatrixTimesMatrix:
		return left.getNominalSize() == right.getSecondarySize();
	case EOpMatrixTimesMatrixAssign:
		return left.getNominalSize() == right.getNominalSize() &&
		       left.getSecondarySize() == right.getSecondarySize();
	default:
		UNREACHABLE(op);
		return false;
	}
}

TOperator TypeToConstructorOperator(const TType &type)
{
	switch(type.getBasicType())
	{
	case EbtFloat:
		if(type.isMatrix())
		{
			switch(type.getNominalSize())
			{
			case 2:
				switch(type.getSecondarySize())
				{
				case 2:
					return EOpConstructMat2;
				case 3:
					return EOpConstructMat2x3;
				case 4:
					return EOpConstructMat2x4;
				default:
					break;
				}
				break;

			case 3:
				switch(type.getSecondarySize())
				{
				case 2:
					return EOpConstructMat3x2;
				case 3:
					return EOpConstructMat3;
				case 4:
					return EOpConstructMat3x4;
				default:
					break;
				}
				break;

			case 4:
				switch(type.getSecondarySize())
				{
				case 2:
					return EOpConstructMat4x2;
				case 3:
					return EOpConstructMat4x3;
				case 4:
					return EOpConstructMat4;
				default:
					break;
				}
				break;
			}
		}
		else
		{
			switch(type.getNominalSize())
			{
			case 1:
				return EOpConstructFloat;
			case 2:
				return EOpConstructVec2;
			case 3:
				return EOpConstructVec3;
			case 4:
				return EOpConstructVec4;
			default:
				break;
			}
		}
		break;

	case EbtInt:
		switch(type.getNominalSize())
		{
		case 1:
			return EOpConstructInt;
		case 2:
			return EOpConstructIVec2;
		case 3:
			return EOpConstructIVec3;
		case 4:
			return EOpConstructIVec4;
		default:
			break;
		}
		break;

	case EbtUInt:
		switch(type.getNominalSize())
		{
		case 1:
			return EOpConstructUInt;
		case 2:
			return EOpConstructUVec2;
		case 3:
			return EOpConstructUVec3;
		case 4:
			return EOpConstructUVec4;
		default:
			break;
		}
		break;

	case EbtBool:
		switch(type.getNominalSize())
		{
		case 1:
			return EOpConstructBool;
		case 2:
			return EOpConstructBVec2;
		case 3:
			return EOpConstructBVec3;
		case 4:
			return EOpConstructBVec4;
		default:
			break;
		}
		break;

	case EbtStruct:
		return EOpConstructStruct;

	default:
		break;
	}

	return EOpNull;
}

const char* getOperatorString(TOperator op) {
	switch (op) {
	case EOpInitialize: return "=";
	case EOpAssign: return "=";
	case EOpAddAssign: return "+=";
	case EOpSubAssign: return "-=";
	case EOpDivAssign: return "/=";
	case EOpIModAssign: return "%=";
	case EOpBitShiftLeftAssign: return "<<=";
	case EOpBitShiftRightAssign: return ">>=";
	case EOpBitwiseAndAssign: return "&=";
	case EOpBitwiseXorAssign: return "^=";
	case EOpBitwiseOrAssign: return "|=";

	// Fall-through.
	case EOpMulAssign:
	case EOpVectorTimesMatrixAssign:
	case EOpVectorTimesScalarAssign:
	case EOpMatrixTimesScalarAssign:
	case EOpMatrixTimesMatrixAssign: return "*=";

	// Fall-through.
	case EOpIndexDirect:
	case EOpIndexIndirect: return "[]";

	case EOpIndexDirectStruct: return ".";
	case EOpVectorSwizzle: return ".";
	case EOpAdd: return "+";
	case EOpSub: return "-";
	case EOpMul: return "*";
	case EOpDiv: return "/";
	case EOpMod: UNIMPLEMENTED(); break;
	case EOpEqual: return "==";
	case EOpNotEqual: return "!=";
	case EOpLessThan: return "<";
	case EOpGreaterThan: return ">";
	case EOpLessThanEqual: return "<=";
	case EOpGreaterThanEqual: return ">=";

	// Fall-through.
	case EOpVectorTimesScalar:
	case EOpVectorTimesMatrix:
	case EOpMatrixTimesVector:
	case EOpMatrixTimesScalar:
	case EOpMatrixTimesMatrix: return "*";

	case EOpLogicalOr: return "||";
	case EOpLogicalXor: return "^^";
	case EOpLogicalAnd: return "&&";
	case EOpIMod: return "%";
	case EOpBitShiftLeft: return "<<";
	case EOpBitShiftRight: return ">>";
	case EOpBitwiseAnd: return "&";
	case EOpBitwiseXor: return "^";
	case EOpBitwiseOr: return "|";
	case EOpNegative: return "-";
	case EOpVectorLogicalNot: return "not";
	case EOpLogicalNot: return "!";
	case EOpBitwiseNot: return "~";
	case EOpPostIncrement: return "++";
	case EOpPostDecrement: return "--";
	case EOpPreIncrement: return "++";
	case EOpPreDecrement: return "--";

	case EOpRadians: return "radians";
	case EOpDegrees: return "degrees";
	case EOpSin: return "sin";
	case EOpCos: return "cos";
	case EOpTan: return "tan";
	case EOpAsin: return "asin";
	case EOpAcos: return "acos";
	case EOpAtan: return "atan";
	case EOpSinh: return "sinh";
	case EOpCosh: return "cosh";
	case EOpTanh: return "tanh";
	case EOpAsinh: return "asinh";
	case EOpAcosh: return "acosh";
	case EOpAtanh: return "atanh";
	case EOpExp: return "exp";
	case EOpLog: return "log";
	case EOpExp2: return "exp2";
	case EOpLog2: return "log2";
	case EOpSqrt: return "sqrt";
	case EOpInverseSqrt: return "inversesqrt";
	case EOpAbs: return "abs";
	case EOpSign: return "sign";
	case EOpFloor: return "floor";
	case EOpTrunc: return "trunc";
	case EOpRound: return "round";
	case EOpRoundEven: return "roundEven";
	case EOpCeil: return "ceil";
	case EOpFract: return "fract";
	case EOpLength: return "length";
	case EOpNormalize: return "normalize";
	case EOpDFdx: return "dFdx";
	case EOpDFdy: return "dFdy";
	case EOpFwidth: return "fwidth";
	case EOpAny: return "any";
	case EOpAll: return "all";
	case EOpIsNan: return "isnan";
	case EOpIsInf: return "isinf";
	case EOpOuterProduct: return "outerProduct";
	case EOpTranspose: return "transpose";
	case EOpDeterminant: return "determinant";
	case EOpInverse: return "inverse";

	default: break;
	}
	return "";
}

////////////////////////////////////////////////////////////////////////////
//
// First set of functions are to help build the intermediate representation.
// These functions are not member functions of the nodes.
// They are called from parser productions.
//
/////////////////////////////////////////////////////////////////////////////

//
// Add a terminal node for an identifier in an expression.
//
// Returns the added node.
//
TIntermSymbol* TIntermediate::addSymbol(int id, const TString& name, const TType& type, const TSourceLoc &line)
{
	TIntermSymbol* node = new TIntermSymbol(id, name, type);
	node->setLine(line);

	return node;
}

//
// Connect two nodes with a new parent that does a binary operation on the nodes.
//
// Returns the added node.
//
TIntermTyped* TIntermediate::addBinaryMath(TOperator op, TIntermTyped* left, TIntermTyped* right, const TSourceLoc &line)
{
	bool isBitShift = false;
	switch (op) {
	case EOpEqual:
	case EOpNotEqual:
		if (left->isArray())
			return 0;
		break;
	case EOpLessThan:
	case EOpGreaterThan:
	case EOpLessThanEqual:
	case EOpGreaterThanEqual:
		if (left->isMatrix() || left->isArray() || left->isVector() || left->getBasicType() == EbtStruct) {
			return 0;
		}
		break;
	case EOpLogicalOr:
	case EOpLogicalXor:
	case EOpLogicalAnd:
		if (left->getBasicType() != EbtBool || left->isMatrix() || left->isArray() || left->isVector()) {
			return 0;
		}
		break;
	case EOpBitwiseOr:
	case EOpBitwiseXor:
	case EOpBitwiseAnd:
		if (!IsInteger(left->getBasicType()) || left->isMatrix() || left->isArray()) {
			return 0;
		}
		break;
	case EOpAdd:
	case EOpSub:
	case EOpDiv:
	case EOpMul:
		if (left->getBasicType() == EbtStruct || left->getBasicType() == EbtBool) {
			return 0;
		}
		break;
	case EOpIMod:
		// Note that this is only for the % operator, not for mod()
		if (left->getBasicType() == EbtStruct || left->getBasicType() == EbtBool || left->getBasicType() == EbtFloat) {
			return 0;
		}
		break;
	case EOpBitShiftLeft:
	case EOpBitShiftRight:
	case EOpBitShiftLeftAssign:
	case EOpBitShiftRightAssign:
		// Unsigned can be bit-shifted by signed and vice versa, but we need to
		// check that the basic type is an integer type.
		isBitShift = true;
		if(!IsInteger(left->getBasicType()) || !IsInteger(right->getBasicType()))
		{
			return 0;
		}
		break;
	default: break;
	}

	if(!isBitShift && left->getBasicType() != right->getBasicType())
	{
		return 0;
	}

	//
	// Need a new node holding things together then.  Make
	// one and promote it to the right type.
	//
	TIntermBinary* node = new TIntermBinary(op);
	node->setLine(line);

	node->setLeft(left);
	node->setRight(right);
	if (!node->promote(infoSink))
	{
		delete node;
		return 0;
	}

	//
	// See if we can fold constants.
	//
	TIntermConstantUnion *leftTempConstant = left->getAsConstantUnion();
	TIntermConstantUnion *rightTempConstant = right->getAsConstantUnion();
	if (leftTempConstant && rightTempConstant) {
		TIntermTyped *typedReturnNode = leftTempConstant->fold(node->getOp(), rightTempConstant, infoSink);

		if (typedReturnNode)
			return typedReturnNode;
	}

	return node;
}

//
// Connect two nodes through an assignment.
//
// Returns the added node.
//
TIntermTyped* TIntermediate::addAssign(TOperator op, TIntermTyped* left, TIntermTyped* right, const TSourceLoc &line)
{
	if (left->getType().getStruct() || right->getType().getStruct())
	{
		if (left->getType() != right->getType())
		{
			return 0;
		}
	}

	TIntermBinary* node = new TIntermBinary(op);
	node->setLine(line);

	node->setLeft(left);
	node->setRight(right);
	if (! node->promote(infoSink))
		return 0;

	return node;
}

//
// Connect two nodes through an index operator, where the left node is the base
// of an array or struct, and the right node is a direct or indirect offset.
//
// Returns the added node.
// The caller should set the type of the returned node.
//
TIntermTyped* TIntermediate::addIndex(TOperator op, TIntermTyped* base, TIntermTyped* index, const TSourceLoc &line)
{
	TIntermBinary* node = new TIntermBinary(op);
	node->setLine(line);
	node->setLeft(base);
	node->setRight(index);

	// caller should set the type

	return node;
}

//
// Add one node as the parent of another that it operates on.
//
// Returns the added node.
//
TIntermTyped* TIntermediate::addUnaryMath(TOperator op, TIntermTyped* child, const TSourceLoc &line, const TType *funcReturnType)
{
	if (child == 0) {
		infoSink.info.message(EPrefixInternalError, "Bad type in AddUnaryMath", line);
		return 0;
	}

	switch (op) {
	case EOpBitwiseNot:
		if (!IsInteger(child->getType().getBasicType()) || child->getType().isMatrix() || child->getType().isArray()) {
			return 0;
		}
		break;

	case EOpLogicalNot:
		if (child->getType().getBasicType() != EbtBool || child->getType().isMatrix() || child->getType().isArray() || child->getType().isVector()) {
			return 0;
		}
		break;

	case EOpPostIncrement:
	case EOpPreIncrement:
	case EOpPostDecrement:
	case EOpPreDecrement:
	case EOpNegative:
		if (child->getType().getBasicType() == EbtStruct || child->getType().isArray())
			return 0;
	default: break;
	}

	TIntermConstantUnion *childTempConstant = 0;
	if (child->getAsConstantUnion())
		childTempConstant = child->getAsConstantUnion();

	//
	// Make a new node for the operator.
	//
	TIntermUnary *node = new TIntermUnary(op);
	node->setLine(line);
	node->setOperand(child);

	if (! node->promote(infoSink, funcReturnType))
		return 0;

	if (childTempConstant)  {
		TIntermTyped* newChild = childTempConstant->fold(op, 0, infoSink);

		if (newChild)
			return newChild;
	}

	return node;
}

//
// This is the safe way to change the operator on an aggregate, as it
// does lots of error checking and fixing.  Especially for establishing
// a function call's operation on it's set of parameters.  Sequences
// of instructions are also aggregates, but they just direnctly set
// their operator to EOpSequence.
//
// Returns an aggregate node, which could be the one passed in if
// it was already an aggregate but no operator was set.
//
TIntermAggregate* TIntermediate::setAggregateOperator(TIntermNode* node, TOperator op, const TSourceLoc &line)
{
	TIntermAggregate* aggNode;

	//
	// Make sure we have an aggregate.  If not turn it into one.
	//
	if (node) {
		aggNode = node->getAsAggregate();
		if (aggNode == 0 || aggNode->getOp() != EOpNull) {
			//
			// Make an aggregate containing this node.
			//
			aggNode = new TIntermAggregate();
			aggNode->getSequence().push_back(node);
		}
	} else
		aggNode = new TIntermAggregate();

	//
	// Set the operator.
	//
	aggNode->setOp(op);

	return aggNode;
}

//
// Safe way to combine two nodes into an aggregate.  Works with null pointers,
// a node that's not a aggregate yet, etc.
//
// Returns the resulting aggregate, unless 0 was passed in for
// both existing nodes.
//
TIntermAggregate* TIntermediate::growAggregate(TIntermNode* left, TIntermNode* right, const TSourceLoc &line)
{
	if (left == 0 && right == 0)
		return 0;

	TIntermAggregate* aggNode = 0;
	if (left)
		aggNode = left->getAsAggregate();
	if (!aggNode || aggNode->getOp() != EOpNull) {
		aggNode = new TIntermAggregate;
		if (left)
			aggNode->getSequence().push_back(left);
	}

	if (right)
		aggNode->getSequence().push_back(right);

	aggNode->setLine(line);

	return aggNode;
}

//
// Turn an existing node into an aggregate.
//
// Returns an aggregate, unless 0 was passed in for the existing node.
//
TIntermAggregate* TIntermediate::makeAggregate(TIntermNode* node, const TSourceLoc &line)
{
	if (node == 0)
		return 0;

	TIntermAggregate* aggNode = new TIntermAggregate;
	aggNode->getSequence().push_back(node);

	aggNode->setLine(line);

	return aggNode;
}

//
// For "if" test nodes.  There are three children; a condition,
// a true path, and a false path.  The two paths are in the
// nodePair.
//
// Returns the selection node created.
//
TIntermNode* TIntermediate::addSelection(TIntermTyped* cond, TIntermNodePair nodePair, const TSourceLoc &line)
{
	//
	// For compile time constant selections, prune the code and
	// test now.
	//

	if (cond->getAsTyped() && cond->getAsTyped()->getAsConstantUnion()) {
		if (cond->getAsConstantUnion()->getBConst(0) == true)
			return nodePair.node1 ? setAggregateOperator(nodePair.node1, EOpSequence, nodePair.node1->getLine()) : nullptr;
		else
			return nodePair.node2 ? setAggregateOperator(nodePair.node2, EOpSequence, nodePair.node2->getLine()) : nullptr;
	}

	TIntermSelection* node = new TIntermSelection(cond, nodePair.node1, nodePair.node2);
	node->setLine(line);

	return node;
}


TIntermTyped* TIntermediate::addComma(TIntermTyped* left, TIntermTyped* right, const TSourceLoc &line)
{
	if (left->getType().getQualifier() == EvqConstExpr && right->getType().getQualifier() == EvqConstExpr) {
		return right;
	} else {
		TIntermTyped *commaAggregate = growAggregate(left, right, line);
		commaAggregate->getAsAggregate()->setOp(EOpComma);
		commaAggregate->setType(right->getType());
		commaAggregate->getTypePointer()->setQualifier(EvqTemporary);
		return commaAggregate;
	}
}

//
// For "?:" test nodes.  There are three children; a condition,
// a true path, and a false path.  The two paths are specified
// as separate parameters.
//
// Returns the selection node created, or 0 if one could not be.
//
TIntermTyped* TIntermediate::addSelection(TIntermTyped* cond, TIntermTyped* trueBlock, TIntermTyped* falseBlock, const TSourceLoc &line)
{
	if (trueBlock->getType() != falseBlock->getType())
	{
		return 0;
	}

	//
	// See if all the operands are constant, then fold it otherwise not.
	//

	if (cond->getAsConstantUnion() && trueBlock->getAsConstantUnion() && falseBlock->getAsConstantUnion()) {
		if (cond->getAsConstantUnion()->getBConst(0))
			return trueBlock;
		else
			return falseBlock;
	}

	//
	// Make a selection node.
	//
	TIntermSelection* node = new TIntermSelection(cond, trueBlock, falseBlock, trueBlock->getType());
	node->getTypePointer()->setQualifier(EvqTemporary);
	node->setLine(line);

	return node;
}

TIntermSwitch *TIntermediate::addSwitch(TIntermTyped *init, TIntermAggregate *statementList, const TSourceLoc &line)
{
	TIntermSwitch *node = new TIntermSwitch(init, statementList);
	node->setLine(line);

	return node;
}

TIntermCase *TIntermediate::addCase(TIntermTyped *condition, const TSourceLoc &line)
{
	TIntermCase *node = new TIntermCase(condition);
	node->setLine(line);

	return node;
}

//
// Constant terminal nodes.  Has a union that contains bool, float or int constants
//
// Returns the constant union node created.
//

TIntermConstantUnion* TIntermediate::addConstantUnion(ConstantUnion* unionArrayPointer, const TType& t, const TSourceLoc &line)
{
	TIntermConstantUnion* node = new TIntermConstantUnion(unionArrayPointer, t);
	node->setLine(line);

	return node;
}

TIntermTyped* TIntermediate::addSwizzle(TVectorFields& fields, const TSourceLoc &line)
{

	TIntermAggregate* node = new TIntermAggregate(EOpSequence);

	node->setLine(line);
	TIntermConstantUnion* constIntNode;
	TIntermSequence &sequenceVector = node->getSequence();
	ConstantUnion* unionArray;

	for (int i = 0; i < fields.num; i++) {
		unionArray = new ConstantUnion[1];
		unionArray->setIConst(fields.offsets[i]);
		constIntNode = addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConstExpr), line);
		sequenceVector.push_back(constIntNode);
	}

	return node;
}

//
// Create loop nodes.
//
TIntermNode* TIntermediate::addLoop(TLoopType type, TIntermNode* init, TIntermTyped* cond, TIntermTyped* expr, TIntermNode* body, const TSourceLoc &line)
{
	TIntermNode* node = new TIntermLoop(type, init, cond, expr, body);
	node->setLine(line);

	return node;
}

//
// Add branches.
//
TIntermBranch* TIntermediate::addBranch(TOperator branchOp, const TSourceLoc &line)
{
	return addBranch(branchOp, 0, line);
}

TIntermBranch* TIntermediate::addBranch(TOperator branchOp, TIntermTyped* expression, const TSourceLoc &line)
{
	TIntermBranch* node = new TIntermBranch(branchOp, expression);
	node->setLine(line);

	return node;
}

//
// This is to be executed once the final root is put on top by the parsing
// process.
//
bool TIntermediate::postProcess(TIntermNode* root)
{
	if (root == 0)
		return true;

	//
	// First, finish off the top level sequence, if any
	//
	TIntermAggregate* aggRoot = root->getAsAggregate();
	if (aggRoot && aggRoot->getOp() == EOpNull)
		aggRoot->setOp(EOpSequence);

	return true;
}

////////////////////////////////////////////////////////////////
//
// Member functions of the nodes used for building the tree.
//
////////////////////////////////////////////////////////////////

// static
TIntermTyped *TIntermTyped::CreateIndexNode(int index)
{
	ConstantUnion *u = new ConstantUnion[1];
	u[0].setIConst(index);

	TType type(EbtInt, EbpUndefined, EvqConstExpr, 1);
	TIntermConstantUnion *node = new TIntermConstantUnion(u, type);
	return node;
}

//
// Say whether or not an operation node changes the value of a variable.
//
// Returns true if state is modified.
//
bool TIntermOperator::modifiesState() const
{
	switch (op) {
		case EOpPostIncrement:
		case EOpPostDecrement:
		case EOpPreIncrement:
		case EOpPreDecrement:
		case EOpAssign:
		case EOpAddAssign:
		case EOpSubAssign:
		case EOpMulAssign:
		case EOpVectorTimesMatrixAssign:
		case EOpVectorTimesScalarAssign:
		case EOpMatrixTimesScalarAssign:
		case EOpMatrixTimesMatrixAssign:
		case EOpDivAssign:
		case EOpIModAssign:
		case EOpBitShiftLeftAssign:
		case EOpBitShiftRightAssign:
		case EOpBitwiseAndAssign:
		case EOpBitwiseXorAssign:
		case EOpBitwiseOrAssign:
			return true;
		default:
			return false;
	}
}

//
// returns true if the operator is for one of the constructors
//
bool TIntermOperator::isConstructor() const
{
	switch (op) {
		case EOpConstructVec2:
		case EOpConstructVec3:
		case EOpConstructVec4:
		case EOpConstructMat2:
		case EOpConstructMat2x3:
		case EOpConstructMat2x4:
		case EOpConstructMat3x2:
		case EOpConstructMat3:
		case EOpConstructMat3x4:
		case EOpConstructMat4x2:
		case EOpConstructMat4x3:
		case EOpConstructMat4:
		case EOpConstructFloat:
		case EOpConstructIVec2:
		case EOpConstructIVec3:
		case EOpConstructIVec4:
		case EOpConstructInt:
		case EOpConstructUVec2:
		case EOpConstructUVec3:
		case EOpConstructUVec4:
		case EOpConstructUInt:
		case EOpConstructBVec2:
		case EOpConstructBVec3:
		case EOpConstructBVec4:
		case EOpConstructBool:
		case EOpConstructStruct:
			return true;
		default:
			return false;
	}
}

//
// Make sure the type of a unary operator is appropriate for its
// combination of operation and operand type.
//
// Returns false in nothing makes sense.
//
bool TIntermUnary::promote(TInfoSink&, const TType *funcReturnType)
{
	setType(funcReturnType ? *funcReturnType : operand->getType());

	// Unary operations result in temporary variables unless const.
	if(type.getQualifier() != EvqConstExpr)
	{
		type.setQualifier(EvqTemporary);
	}

	switch (op) {
		case EOpLogicalNot:
			if (operand->getBasicType() != EbtBool)
				return false;
			break;
		case EOpBitwiseNot:
			if (!IsInteger(operand->getBasicType()))
				return false;
			break;
		case EOpNegative:
		case EOpPostIncrement:
		case EOpPostDecrement:
		case EOpPreIncrement:
		case EOpPreDecrement:
			if (operand->getBasicType() == EbtBool)
				return false;
			break;

			// operators for built-ins are already type checked against their prototype
		case EOpAny:
		case EOpAll:
		case EOpVectorLogicalNot:
		case EOpAbs:
		case EOpSign:
		case EOpIsNan:
		case EOpIsInf:
		case EOpFloatBitsToInt:
		case EOpFloatBitsToUint:
		case EOpIntBitsToFloat:
		case EOpUintBitsToFloat:
		case EOpPackSnorm2x16:
		case EOpPackUnorm2x16:
		case EOpPackHalf2x16:
		case EOpUnpackSnorm2x16:
		case EOpUnpackUnorm2x16:
		case EOpUnpackHalf2x16:
			return true;

		default:
			if (operand->getBasicType() != EbtFloat)
				return false;
	}

	return true;
}

//
// Establishes the type of the resultant operation, as well as
// makes the operator the correct one for the operands.
//
// Returns false if operator can't work on operands.
//
bool TIntermBinary::promote(TInfoSink& infoSink)
{
	ASSERT(left->isArray() == right->isArray());

	// GLSL ES 2.0 does not support implicit type casting.
	// So the basic type should always match.
	// GLSL ES 3.0 supports integer shift operands of different signedness.
	if(op != EOpBitShiftLeft &&
	   op != EOpBitShiftRight &&
	   op != EOpBitShiftLeftAssign &&
	   op != EOpBitShiftRightAssign &&
	   left->getBasicType() != right->getBasicType())
	{
		return false;
	}

	//
	// Base assumption:  just make the type the same as the left
	// operand.  Then only deviations from this need be coded.
	//
	setType(left->getType());

	// The result gets promoted to the highest precision.
	TPrecision higherPrecision = GetHigherPrecision(left->getPrecision(), right->getPrecision());
	getTypePointer()->setPrecision(higherPrecision);

	// Binary operations results in temporary variables unless both
	// operands are const.
	if (left->getQualifier() != EvqConstExpr || right->getQualifier() != EvqConstExpr) {
		getTypePointer()->setQualifier(EvqTemporary);
	}

	int primarySize = std::max(left->getNominalSize(), right->getNominalSize());

	//
	// All scalars. Code after this test assumes this case is removed!
	//
	if (primarySize == 1) {
		switch (op) {
			//
			// Promote to conditional
			//
			case EOpEqual:
			case EOpNotEqual:
			case EOpLessThan:
			case EOpGreaterThan:
			case EOpLessThanEqual:
			case EOpGreaterThanEqual:
				setType(TType(EbtBool, EbpUndefined));
				break;

			//
			// And and Or operate on conditionals
			//
			case EOpLogicalAnd:
			case EOpLogicalOr:
			case EOpLogicalXor:
				// Both operands must be of type bool.
				if (left->getBasicType() != EbtBool || right->getBasicType() != EbtBool)
					return false;
				setType(TType(EbtBool, EbpUndefined));
				break;

			default:
				break;
		}
		return true;
	}

	// If we reach here, at least one of the operands is vector or matrix.
	// The other operand could be a scalar, vector, or matrix.
	// Can these two operands be combined?
	//
	TBasicType basicType = left->getBasicType();
	switch (op) {
		case EOpMul:
			if (!left->isMatrix() && right->isMatrix()) {
				if (left->isVector())
				{
					op = EOpVectorTimesMatrix;
					setType(TType(basicType, higherPrecision, EvqTemporary,
						static_cast<unsigned char>(right->getNominalSize()), 1));
				}
				else {
					op = EOpMatrixTimesScalar;
					setType(TType(basicType, higherPrecision, EvqTemporary,
						static_cast<unsigned char>(right->getNominalSize()), static_cast<unsigned char>(right->getSecondarySize())));
				}
			} else if (left->isMatrix() && !right->isMatrix()) {
				if (right->isVector()) {
					op = EOpMatrixTimesVector;
					setType(TType(basicType, higherPrecision, EvqTemporary,
						static_cast<unsigned char>(left->getSecondarySize()), 1));
				} else {
					op = EOpMatrixTimesScalar;
				}
			} else if (left->isMatrix() && right->isMatrix()) {
				op = EOpMatrixTimesMatrix;
				setType(TType(basicType, higherPrecision, EvqTemporary,
					static_cast<unsigned char>(right->getNominalSize()), static_cast<unsigned char>(left->getSecondarySize())));
			} else if (!left->isMatrix() && !right->isMatrix()) {
				if (left->isVector() && right->isVector()) {
					// leave as component product
				} else if (left->isVector() || right->isVector()) {
					op = EOpVectorTimesScalar;
					setType(TType(basicType, higherPrecision, EvqTemporary,
						static_cast<unsigned char>(primarySize), 1));
				}
			} else {
				infoSink.info.message(EPrefixInternalError, "Missing elses", getLine());
				return false;
			}

			if(!ValidateMultiplication(op, left->getType(), right->getType()))
			{
				return false;
			}
			break;
		case EOpMulAssign:
			if (!left->isMatrix() && right->isMatrix()) {
				if (left->isVector())
					op = EOpVectorTimesMatrixAssign;
				else {
					return false;
				}
			} else if (left->isMatrix() && !right->isMatrix()) {
				if (right->isVector()) {
					return false;
				} else {
					op = EOpMatrixTimesScalarAssign;
				}
			} else if (left->isMatrix() && right->isMatrix()) {
				op = EOpMatrixTimesMatrixAssign;
				setType(TType(basicType, higherPrecision, EvqTemporary,
					static_cast<unsigned char>(right->getNominalSize()), static_cast<unsigned char>(left->getSecondarySize())));
			} else if (!left->isMatrix() && !right->isMatrix()) {
				if (left->isVector() && right->isVector()) {
					// leave as component product
				} else if (left->isVector() || right->isVector()) {
					if (! left->isVector())
						return false;
					op = EOpVectorTimesScalarAssign;
					setType(TType(basicType, higherPrecision, EvqTemporary,
						static_cast<unsigned char>(left->getNominalSize()), 1));
				}
			} else {
				infoSink.info.message(EPrefixInternalError, "Missing elses", getLine());
				return false;
			}

			if(!ValidateMultiplication(op, left->getType(), right->getType()))
			{
				return false;
			}
			break;

		case EOpAssign:
		case EOpInitialize:
			// No more additional checks are needed.
			if ((left->getNominalSize() != right->getNominalSize()) ||
				(left->getSecondarySize() != right->getSecondarySize()))
				return false;
			break;
		case EOpAdd:
		case EOpSub:
		case EOpDiv:
		case EOpIMod:
		case EOpBitShiftLeft:
		case EOpBitShiftRight:
		case EOpBitwiseAnd:
		case EOpBitwiseXor:
		case EOpBitwiseOr:
		case EOpAddAssign:
		case EOpSubAssign:
		case EOpDivAssign:
		case EOpIModAssign:
		case EOpBitShiftLeftAssign:
		case EOpBitShiftRightAssign:
		case EOpBitwiseAndAssign:
		case EOpBitwiseXorAssign:
		case EOpBitwiseOrAssign:
			if ((left->isMatrix() && right->isVector()) ||
				(left->isVector() && right->isMatrix()))
				return false;

			// Are the sizes compatible?
			if(left->getNominalSize() != right->getNominalSize() ||
			   left->getSecondarySize() != right->getSecondarySize())
			{
				// If the nominal sizes of operands do not match:
				// One of them must be a scalar.
				if(!left->isScalar() && !right->isScalar())
					return false;

				// In the case of compound assignment other than multiply-assign,
				// the right side needs to be a scalar. Otherwise a vector/matrix
				// would be assigned to a scalar. A scalar can't be shifted by a
				// vector either.
				if(!right->isScalar() && (modifiesState() || op == EOpBitShiftLeft || op == EOpBitShiftRight))
					return false;
			}

			{
				const int secondarySize = std::max(
					left->getSecondarySize(), right->getSecondarySize());
				setType(TType(basicType, higherPrecision, EvqTemporary,
					static_cast<unsigned char>(primarySize), static_cast<unsigned char>(secondarySize)));
				if(left->isArray())
				{
					ASSERT(left->getArraySize() == right->getArraySize());
					type.setArraySize(left->getArraySize());
				}
			}
			break;

		case EOpEqual:
		case EOpNotEqual:
		case EOpLessThan:
		case EOpGreaterThan:
		case EOpLessThanEqual:
		case EOpGreaterThanEqual:
			if ((left->getNominalSize() != right->getNominalSize()) ||
				(left->getSecondarySize() != right->getSecondarySize()))
				return false;
			setType(TType(EbtBool, EbpUndefined));
			break;

		case EOpOuterProduct:
			if(!left->isVector() || !right->isVector())
				return false;
			setType(TType(EbtFloat, right->getNominalSize(), left->getNominalSize()));
			break;

		case EOpTranspose:
			if(!right->isMatrix())
				return false;
			setType(TType(EbtFloat, right->getSecondarySize(), right->getNominalSize()));
			break;

		case EOpDeterminant:
			if(!right->isMatrix())
				return false;
			setType(TType(EbtFloat));
			break;

		case EOpInverse:
			if(!right->isMatrix() || right->getNominalSize() != right->getSecondarySize())
				return false;
			setType(right->getType());
			break;

		default:
			return false;
	}

	return true;
}

bool CompareStruct(const TType& leftNodeType, ConstantUnion* rightUnionArray, ConstantUnion* leftUnionArray)
{
	const TFieldList& fields = leftNodeType.getStruct()->fields();

	size_t structSize = fields.size();
	int index = 0;

	for (size_t j = 0; j < structSize; j++) {
		size_t size = fields[j]->type()->getObjectSize();
		for(size_t i = 0; i < size; i++) {
			if (fields[j]->type()->getBasicType() == EbtStruct) {
				if (!CompareStructure(*(fields[j]->type()), &rightUnionArray[index], &leftUnionArray[index]))
					return false;
			} else {
				if (leftUnionArray[index] != rightUnionArray[index])
					return false;
				index++;
			}

		}
	}
	return true;
}

bool CompareStructure(const TType& leftNodeType, ConstantUnion* rightUnionArray, ConstantUnion* leftUnionArray)
{
	if (leftNodeType.isArray()) {
		TType typeWithoutArrayness = leftNodeType;
		typeWithoutArrayness.clearArrayness();

		int arraySize = leftNodeType.getArraySize();

		for (int i = 0; i < arraySize; ++i) {
			size_t offset = typeWithoutArrayness.getObjectSize() * i;
			if (!CompareStruct(typeWithoutArrayness, &rightUnionArray[offset], &leftUnionArray[offset]))
				return false;
		}
	} else
		return CompareStruct(leftNodeType, rightUnionArray, leftUnionArray);

	return true;
}

float determinant2(float m00, float m01, float m10, float m11)
{
	return m00 * m11 - m01 * m10;
}

float determinant3(float m00, float m01, float m02,
				   float m10, float m11, float m12,
				   float m20, float m21, float m22)
{
	return m00 * determinant2(m11, m12, m21, m22) -
		   m10 * determinant2(m01, m02, m21, m22) +
		   m20 * determinant2(m01, m02, m11, m12);
}

float determinant4(float m00, float m01, float m02, float m03,
				   float m10, float m11, float m12, float m13,
				   float m20, float m21, float m22, float m23,
				   float m30, float m31, float m32, float m33)
{
	return m00 * determinant3(m11, m12, m13, m21, m22, m23, m31, m32, m33) -
		   m10 * determinant3(m01, m02, m03, m21, m22, m23, m31, m32, m33) +
		   m20 * determinant3(m01, m02, m03, m11, m12, m13, m31, m32, m33) -
		   m30 * determinant3(m01, m02, m03, m11, m12, m13, m21, m22, m23);
}

float ComputeDeterminant(int size, ConstantUnion* unionArray)
{
	switch(size)
	{
	case 2:
		return determinant2(unionArray[0].getFConst(),
							unionArray[1].getFConst(),
							unionArray[2].getFConst(),
							unionArray[3].getFConst());
	case 3:
		return determinant3(unionArray[0].getFConst(),
							unionArray[1].getFConst(),
							unionArray[2].getFConst(),
							unionArray[3].getFConst(),
							unionArray[4].getFConst(),
							unionArray[5].getFConst(),
							unionArray[6].getFConst(),
							unionArray[7].getFConst(),
							unionArray[8].getFConst());
	case 4:
		return determinant4(unionArray[0].getFConst(),
							unionArray[1].getFConst(),
							unionArray[2].getFConst(),
							unionArray[3].getFConst(),
							unionArray[4].getFConst(),
							unionArray[5].getFConst(),
							unionArray[6].getFConst(),
							unionArray[7].getFConst(),
							unionArray[8].getFConst(),
							unionArray[9].getFConst(),
							unionArray[10].getFConst(),
							unionArray[11].getFConst(),
							unionArray[12].getFConst(),
							unionArray[13].getFConst(),
							unionArray[14].getFConst(),
							unionArray[15].getFConst());
	default:
		UNREACHABLE(size);
		return 0.0f;
	}
}

ConstantUnion* CreateInverse(TIntermConstantUnion* node, ConstantUnion* unionArray)
{
	ConstantUnion* tempConstArray = 0;
	int size = node->getNominalSize();
	float determinant = ComputeDeterminant(size, unionArray);
	if(determinant != 0.0f)
	{
		float invDet = 1.0f / determinant;
		tempConstArray = new ConstantUnion[size*size];
		switch(size)
		{
		case 2:
			{
				float m00 = unionArray[0].getFConst();			// Matrix is:
				float m01 = unionArray[1].getFConst();			// (m00, m01)
				float m10 = unionArray[2].getFConst();			// (m10, m11)
				float m11 = unionArray[3].getFConst();
				tempConstArray[0].setFConst( invDet * m11);
				tempConstArray[1].setFConst(-invDet * m01);
				tempConstArray[2].setFConst(-invDet * m10);
				tempConstArray[3].setFConst( invDet * m00);
			}
			break;
		case 3:
			{
				float m00 = unionArray[0].getFConst();			// Matrix is:
				float m01 = unionArray[1].getFConst();			// (m00, m01, m02)
				float m02 = unionArray[2].getFConst();			// (m10, m11, m12)
				float m10 = unionArray[3].getFConst();			// (m20, m21, m22)
				float m11 = unionArray[4].getFConst();
				float m12 = unionArray[5].getFConst();
				float m20 = unionArray[6].getFConst();
				float m21 = unionArray[7].getFConst();
				float m22 = unionArray[8].getFConst();
				tempConstArray[0].setFConst(invDet * determinant2(m11, m12, m21, m22)); // m00 =  invDet * (m11 * m22 - m12 * m21)
				tempConstArray[1].setFConst(invDet * determinant2(m12, m10, m22, m20)); // m01 = -invDet * (m10 * m22 - m12 * m20)
				tempConstArray[2].setFConst(invDet * determinant2(m10, m11, m20, m21)); // m02 =  invDet * (m10 * m21 - m11 * m20)
				tempConstArray[3].setFConst(invDet * determinant2(m21, m22, m01, m02)); // m10 = -invDet * (m01 * m22 - m02 * m21)
				tempConstArray[4].setFConst(invDet * determinant2(m00, m02, m20, m22)); // m11 =  invDet * (m00 * m22 - m02 * m20)
				tempConstArray[5].setFConst(invDet * determinant2(m20, m21, m00, m01)); // m12 = -invDet * (m00 * m21 - m01 * m20)
				tempConstArray[6].setFConst(invDet * determinant2(m01, m02, m11, m12)); // m20 =  invDet * (m01 * m12 - m02 * m11)
				tempConstArray[7].setFConst(invDet * determinant2(m10, m12, m00, m02)); // m21 = -invDet * (m00 * m12 - m02 * m10)
				tempConstArray[8].setFConst(invDet * determinant2(m00, m01, m10, m11)); // m22 =  invDet * (m00 * m11 - m01 * m10)
			}
			break;
		case 4:
			{
				float m00 = unionArray[0].getFConst();			// Matrix is:
				float m01 = unionArray[1].getFConst();			// (m00, m01, m02, m03)
				float m02 = unionArray[2].getFConst();			// (m10, m11, m12, m13)
				float m03 = unionArray[3].getFConst();			// (m20, m21, m22, m23)
				float m10 = unionArray[4].getFConst();			// (m30, m31, m32, m33)
				float m11 = unionArray[5].getFConst();
				float m12 = unionArray[6].getFConst();
				float m13 = unionArray[7].getFConst();
				float m20 = unionArray[8].getFConst();
				float m21 = unionArray[9].getFConst();
				float m22 = unionArray[10].getFConst();
				float m23 = unionArray[11].getFConst();
				float m30 = unionArray[12].getFConst();
				float m31 = unionArray[13].getFConst();
				float m32 = unionArray[14].getFConst();
				float m33 = unionArray[15].getFConst();
				tempConstArray[ 0].setFConst( invDet * determinant3(m11, m12, m13, m21, m22, m23, m31, m32, m33)); // m00
				tempConstArray[ 1].setFConst(-invDet * determinant3(m10, m12, m13, m20, m22, m23, m30, m32, m33)); // m01
				tempConstArray[ 2].setFConst( invDet * determinant3(m10, m11, m13, m20, m21, m23, m30, m31, m33)); // m02
				tempConstArray[ 3].setFConst(-invDet * determinant3(m10, m11, m12, m20, m21, m22, m30, m31, m32)); // m03
				tempConstArray[ 4].setFConst( invDet * determinant3(m01, m02, m03, m21, m22, m23, m31, m32, m33)); // m10
				tempConstArray[ 5].setFConst(-invDet * determinant3(m00, m02, m03, m20, m22, m23, m30, m32, m33)); // m11
				tempConstArray[ 6].setFConst( invDet * determinant3(m00, m01, m03, m20, m21, m23, m30, m31, m33)); // m12
				tempConstArray[ 7].setFConst(-invDet * determinant3(m00, m01, m02, m20, m21, m22, m30, m31, m32)); // m13
				tempConstArray[ 8].setFConst( invDet * determinant3(m01, m02, m03, m11, m12, m13, m31, m32, m33)); // m20
				tempConstArray[ 9].setFConst(-invDet * determinant3(m00, m02, m03, m10, m12, m13, m30, m32, m33)); // m21
				tempConstArray[10].setFConst( invDet * determinant3(m00, m01, m03, m10, m11, m13, m30, m31, m33)); // m22
				tempConstArray[11].setFConst(-invDet * determinant3(m00, m01, m02, m10, m11, m12, m30, m31, m32)); // m23
				tempConstArray[12].setFConst( invDet * determinant3(m01, m02, m03, m11, m12, m13, m21, m22, m23)); // m30
				tempConstArray[13].setFConst(-invDet * determinant3(m00, m02, m03, m10, m12, m13, m20, m22, m23)); // m31
				tempConstArray[14].setFConst( invDet * determinant3(m00, m01, m03, m10, m11, m13, m20, m21, m23)); // m32
				tempConstArray[15].setFConst(-invDet * determinant3(m00, m01, m02, m10, m11, m12, m20, m21, m22)); // m33
			}
			break;
		default:
			UNREACHABLE(size);
		}
	}
	return tempConstArray;
}

//
// The fold functions see if an operation on a constant can be done in place,
// without generating run-time code.
//
// Returns the node to keep using, which may or may not be the node passed in.
//

TIntermTyped* TIntermConstantUnion::fold(TOperator op, TIntermTyped* constantNode, TInfoSink& infoSink)
{
	ConstantUnion *unionArray = getUnionArrayPointer();
	size_t objectSize = getType().getObjectSize();

	if (constantNode) {  // binary operations
		TIntermConstantUnion *node = constantNode->getAsConstantUnion();
		ConstantUnion *rightUnionArray = node->getUnionArrayPointer();
		TType returnType = getType();

		// for a case like float f = 1.2 + vec4(2,3,4,5);
		if (constantNode->getType().getObjectSize() == 1 && objectSize > 1) {
			rightUnionArray = new ConstantUnion[objectSize];
			for (size_t i = 0; i < objectSize; ++i)
				rightUnionArray[i] = *node->getUnionArrayPointer();
			returnType = getType();
		} else if (constantNode->getType().getObjectSize() > 1 && objectSize == 1) {
			// for a case like float f = vec4(2,3,4,5) + 1.2;
			unionArray = new ConstantUnion[constantNode->getType().getObjectSize()];
			for (size_t i = 0; i < constantNode->getType().getObjectSize(); ++i)
				unionArray[i] = *getUnionArrayPointer();
			returnType = node->getType();
			objectSize = constantNode->getType().getObjectSize();
		}

		ConstantUnion* tempConstArray = 0;
		TIntermConstantUnion *tempNode;

		switch(op) {
			case EOpAdd:
				tempConstArray = new ConstantUnion[objectSize];
				{// support MSVC++6.0
					for (size_t i = 0; i < objectSize; i++)
						tempConstArray[i] = unionArray[i] + rightUnionArray[i];
				}
				break;
			case EOpSub:
				tempConstArray = new ConstantUnion[objectSize];
				{// support MSVC++6.0
					for (size_t i = 0; i < objectSize; i++)
						tempConstArray[i] = unionArray[i] - rightUnionArray[i];
				}
				break;

			case EOpMul:
			case EOpVectorTimesScalar:
			case EOpMatrixTimesScalar:
				tempConstArray = new ConstantUnion[objectSize];
				{// support MSVC++6.0
					for (size_t i = 0; i < objectSize; i++)
						tempConstArray[i] = unionArray[i] * rightUnionArray[i];
				}
				break;
			case EOpMatrixTimesMatrix:
				if (getType().getBasicType() != EbtFloat || node->getBasicType() != EbtFloat) {
					infoSink.info.message(EPrefixInternalError, "Constant Folding cannot be done for matrix multiply", getLine());
					return 0;
				}
				{// support MSVC++6.0
					int leftNumCols = getNominalSize();
					int leftNumRows = getSecondarySize();
					int rightNumCols = node->getNominalSize();
					int rightNumRows = node->getSecondarySize();
					if(leftNumCols != rightNumRows) {
						infoSink.info.message(EPrefixInternalError, "Constant Folding cannot be done for matrix multiply", getLine());
						return 0;
					}
					int tempNumCols = rightNumCols;
					int tempNumRows = leftNumRows;
					int tempNumAdds = leftNumCols;
					tempConstArray = new ConstantUnion[tempNumCols*tempNumRows];
					for (int row = 0; row < tempNumRows; row++) {
						for (int column = 0; column < tempNumCols; column++) {
							tempConstArray[tempNumRows * column + row].setFConst(0.0f);
							for (int i = 0; i < tempNumAdds; i++) {
								tempConstArray[tempNumRows * column + row].setFConst(tempConstArray[tempNumRows * column + row].getFConst() + unionArray[i * leftNumRows + row].getFConst() * (rightUnionArray[column * rightNumRows + i].getFConst()));
							}
						}
					}
					// update return type for matrix product
					returnType.setNominalSize(static_cast<unsigned char>(tempNumCols));
					returnType.setSecondarySize(static_cast<unsigned char>(tempNumRows));
				}
				break;

			case EOpOuterProduct:
				{
					int leftSize = getNominalSize();
					int rightSize = node->getNominalSize();
					tempConstArray = new ConstantUnion[leftSize*rightSize];
					for(int row = 0; row < leftSize; row++) {
						for(int column = 0; column < rightSize; column++) {
							tempConstArray[leftSize * column + row].setFConst(unionArray[row].getFConst() * rightUnionArray[column].getFConst());
						}
					}
					// update return type for outer product
					returnType.setNominalSize(static_cast<unsigned char>(rightSize));
					returnType.setSecondarySize(static_cast<unsigned char>(leftSize));
				}
				break;

			case EOpTranspose:
				{
					int rightCol = node->getNominalSize();
					int rightRow = node->getSecondarySize();
					tempConstArray = new ConstantUnion[rightCol*rightRow];
					for(int row = 0; row < rightRow; row++) {
						for(int column = 0; column < rightCol; column++) {
							tempConstArray[rightRow * column + row].setFConst(rightUnionArray[rightCol * row + column].getFConst());
						}
					}
					// update return type for transpose
					returnType.setNominalSize(static_cast<unsigned char>(rightRow));
					returnType.setSecondarySize(static_cast<unsigned char>(rightCol));
				}
				break;

			case EOpDeterminant:
				{
					ASSERT(node->getNominalSize() == node->getSecondarySize());

					tempConstArray = new ConstantUnion[1];
					tempConstArray[0].setFConst(ComputeDeterminant(node->getNominalSize(), rightUnionArray));
					// update return type for determinant
					returnType.setNominalSize(1);
					returnType.setSecondarySize(1);
				}
				break;

			case EOpInverse:
				{
					ASSERT(node->getNominalSize() == node->getSecondarySize());

					tempConstArray = CreateInverse(node, rightUnionArray);
					if(!tempConstArray)
					{
						// Singular matrix, just copy
						tempConstArray = new ConstantUnion[objectSize];
						for(size_t i = 0; i < objectSize; i++)
							tempConstArray[i] = rightUnionArray[i];
					}
				}
				break;

			case EOpDiv:
			case EOpIMod:
				tempConstArray = new ConstantUnion[objectSize];
				{// support MSVC++6.0
					for (size_t i = 0; i < objectSize; i++) {
						switch (getType().getBasicType()) {
							case EbtFloat:
								if (rightUnionArray[i] == 0.0f) {
									infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", getLine());
									tempConstArray[i].setFConst(FLT_MAX);
								} else {
									ASSERT(op == EOpDiv);
									tempConstArray[i].setFConst(unionArray[i].getFConst() / rightUnionArray[i].getFConst());
								}
								break;

							case EbtInt:
								if (rightUnionArray[i] == 0) {
									infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", getLine());
									tempConstArray[i].setIConst(INT_MAX);
								} else {
									if(op == EOpDiv) {
										tempConstArray[i].setIConst(unionArray[i].getIConst() / rightUnionArray[i].getIConst());
									} else {
										ASSERT(op == EOpIMod);
										tempConstArray[i].setIConst(unionArray[i].getIConst() % rightUnionArray[i].getIConst());
									}
								}
								break;
							case EbtUInt:
								if (rightUnionArray[i] == 0) {
									infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", getLine());
									tempConstArray[i].setUConst(UINT_MAX);
								} else {
									if(op == EOpDiv) {
										tempConstArray[i].setUConst(unionArray[i].getUConst() / rightUnionArray[i].getUConst());
									} else {
										ASSERT(op == EOpIMod);
										tempConstArray[i].setUConst(unionArray[i].getUConst() % rightUnionArray[i].getUConst());
									}
								}
								break;
							default:
								infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"/\"", getLine());
								return 0;
						}
					}
				}
				break;

			case EOpMatrixTimesVector:
				if (node->getBasicType() != EbtFloat) {
					infoSink.info.message(EPrefixInternalError, "Constant Folding cannot be done for matrix times vector", getLine());
					return 0;
				}
				tempConstArray = new ConstantUnion[getNominalSize()];

				{// support MSVC++6.0
					for (int size = getNominalSize(), i = 0; i < size; i++) {
						tempConstArray[i].setFConst(0.0f);
						for (int j = 0; j < size; j++) {
							tempConstArray[i].setFConst(tempConstArray[i].getFConst() + ((unionArray[j*size + i].getFConst()) * rightUnionArray[j].getFConst()));
						}
					}
				}

				tempNode = new TIntermConstantUnion(tempConstArray, node->getType());
				tempNode->setLine(getLine());

				return tempNode;

			case EOpVectorTimesMatrix:
				if (getType().getBasicType() != EbtFloat) {
					infoSink.info.message(EPrefixInternalError, "Constant Folding cannot be done for vector times matrix", getLine());
					return 0;
				}

				tempConstArray = new ConstantUnion[getNominalSize()];
				{// support MSVC++6.0
					for (int size = getNominalSize(), i = 0; i < size; i++) {
						tempConstArray[i].setFConst(0.0f);
						for (int j = 0; j < size; j++) {
							tempConstArray[i].setFConst(tempConstArray[i].getFConst() + ((unionArray[j].getFConst()) * rightUnionArray[i*size + j].getFConst()));
						}
					}
				}
				break;

			case EOpLogicalAnd: // this code is written for possible future use, will not get executed currently
				tempConstArray = new ConstantUnion[objectSize];
				{// support MSVC++6.0
					for (size_t i = 0; i < objectSize; i++)
						tempConstArray[i] = unionArray[i] && rightUnionArray[i];
				}
				break;

			case EOpLogicalOr: // this code is written for possible future use, will not get executed currently
				tempConstArray = new ConstantUnion[objectSize];
				{// support MSVC++6.0
					for (size_t i = 0; i < objectSize; i++)
						tempConstArray[i] = unionArray[i] || rightUnionArray[i];
				}
				break;

			case EOpLogicalXor:
				tempConstArray = new ConstantUnion[objectSize];
				{// support MSVC++6.0
					for (size_t i = 0; i < objectSize; i++)
						switch (getType().getBasicType()) {
							case EbtBool: tempConstArray[i].setBConst((unionArray[i] == rightUnionArray[i]) ? false : true); break;
							default: assert(false && "Default missing");
					}
				}
				break;

			case EOpBitwiseAnd:
				tempConstArray = new ConstantUnion[objectSize];
				for(size_t i = 0; i < objectSize; i++)
					tempConstArray[i] = unionArray[i] & rightUnionArray[i];
				break;
			case EOpBitwiseXor:
				tempConstArray = new ConstantUnion[objectSize];
				for(size_t i = 0; i < objectSize; i++)
					tempConstArray[i] = unionArray[i] ^ rightUnionArray[i];
				break;
			case EOpBitwiseOr:
				tempConstArray = new ConstantUnion[objectSize];
				for(size_t i = 0; i < objectSize; i++)
					tempConstArray[i] = unionArray[i] | rightUnionArray[i];
				break;
			case EOpBitShiftLeft:
				tempConstArray = new ConstantUnion[objectSize];
				for(size_t i = 0; i < objectSize; i++)
					tempConstArray[i] = unionArray[i] << rightUnionArray[i];
				break;
			case EOpBitShiftRight:
				tempConstArray = new ConstantUnion[objectSize];
				for(size_t i = 0; i < objectSize; i++)
					tempConstArray[i] = unionArray[i] >> rightUnionArray[i];
				break;

			case EOpLessThan:
				tempConstArray = new ConstantUnion[objectSize];
				for(size_t i = 0; i < objectSize; i++)
					tempConstArray[i].setBConst(unionArray[i] < rightUnionArray[i]);
				returnType = TType(EbtBool, EbpUndefined, EvqConstExpr, objectSize);
				break;
			case EOpGreaterThan:
				tempConstArray = new ConstantUnion[objectSize];
				for(size_t i = 0; i < objectSize; i++)
					tempConstArray[i].setBConst(unionArray[i] > rightUnionArray[i]);
				returnType = TType(EbtBool, EbpUndefined, EvqConstExpr, objectSize);
				break;
			case EOpLessThanEqual:
				tempConstArray = new ConstantUnion[objectSize];
				for(size_t i = 0; i < objectSize; i++)
					tempConstArray[i].setBConst(unionArray[i] <= rightUnionArray[i]);
				returnType = TType(EbtBool, EbpUndefined, EvqConstExpr, objectSize);
				break;
			case EOpGreaterThanEqual:
				tempConstArray = new ConstantUnion[objectSize];
				for(size_t i = 0; i < objectSize; i++)
					tempConstArray[i].setBConst(unionArray[i] >= rightUnionArray[i]);
				returnType = TType(EbtBool, EbpUndefined, EvqConstExpr, objectSize);
				break;
			case EOpEqual:
				tempConstArray = new ConstantUnion[1];

				if(getType().getBasicType() == EbtStruct) {
					tempConstArray->setBConst(CompareStructure(node->getType(), node->getUnionArrayPointer(), unionArray));
				} else {
					bool boolNodeFlag = true;
					for (size_t i = 0; i < objectSize; i++) {
						if (unionArray[i] != rightUnionArray[i]) {
							boolNodeFlag = false;
							break;  // break out of for loop
						}
					}
					tempConstArray->setBConst(boolNodeFlag);
				}

				tempNode = new TIntermConstantUnion(tempConstArray, TType(EbtBool, EbpUndefined, EvqConstExpr));
				tempNode->setLine(getLine());

				return tempNode;

			case EOpNotEqual:
				tempConstArray = new ConstantUnion[1];

				if(getType().getBasicType() == EbtStruct) {
					tempConstArray->setBConst(!CompareStructure(node->getType(), node->getUnionArrayPointer(), unionArray));
				} else {
					bool boolNodeFlag = false;
					for (size_t i = 0; i < objectSize; i++) {
						if (unionArray[i] != rightUnionArray[i]) {
							boolNodeFlag = true;
							break;  // break out of for loop
						}
					}
					tempConstArray->setBConst(boolNodeFlag);
				}

				tempNode = new TIntermConstantUnion(tempConstArray, TType(EbtBool, EbpUndefined, EvqConstExpr));
				tempNode->setLine(getLine());

				return tempNode;
			case EOpMax:
				tempConstArray = new ConstantUnion[objectSize];
				{// support MSVC++6.0
					for (size_t i = 0; i < objectSize; i++)
						tempConstArray[i] = unionArray[i] > rightUnionArray[i] ? unionArray[i] : rightUnionArray[i];
				}
				break;
			case EOpMin:
				tempConstArray = new ConstantUnion[objectSize];
				{// support MSVC++6.0
					for (size_t i = 0; i < objectSize; i++)
						tempConstArray[i] = unionArray[i] < rightUnionArray[i] ? unionArray[i] : rightUnionArray[i];
				}
				break;
			default:
				return 0;
		}
		tempNode = new TIntermConstantUnion(tempConstArray, returnType);
		tempNode->setLine(getLine());

		return tempNode;
	} else {
		//
		// Do unary operations
		//
		TIntermConstantUnion *newNode = 0;
		ConstantUnion* tempConstArray = new ConstantUnion[objectSize];
		TType type = getType();
		TBasicType basicType = type.getBasicType();
		for (size_t i = 0; i < objectSize; i++) {
			switch(op) {
				case EOpNegative:
					switch (basicType) {
						case EbtFloat: tempConstArray[i].setFConst(-unionArray[i].getFConst()); break;
						case EbtInt:   tempConstArray[i].setIConst(-unionArray[i].getIConst()); break;
						default:
							infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
							return 0;
					}
					break;
				case EOpLogicalNot: // this code is written for possible future use, will not get executed currently
					switch (basicType) {
						case EbtBool:  tempConstArray[i].setBConst(!unionArray[i].getBConst()); break;
						default:
							infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
							return 0;
					}
					break;
				case EOpBitwiseNot:
					switch(basicType) {
						case EbtInt: tempConstArray[i].setIConst(~unionArray[i].getIConst()); break;
						case EbtUInt: tempConstArray[i].setUConst(~unionArray[i].getUConst()); break;
						default:
							infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
							return 0;
					}
					break;
				case EOpRadians:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(unionArray[i].getFConst() * 1.74532925e-2f); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpDegrees:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(unionArray[i].getFConst() * 5.72957795e+1f); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpSin:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(sinf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpCos:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(cosf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpTan:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(tanf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpAsin:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(asinf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpAcos:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(acosf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpAtan:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(atanf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpSinh:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(sinhf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpCosh:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(coshf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpTanh:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(tanhf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpAsinh:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(asinhf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpAcosh:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(acoshf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpAtanh:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(atanhf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpLog:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(logf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpLog2:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(sw::log2(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpExp:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(expf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpExp2:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(exp2f(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpSqrt:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(sqrtf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpInverseSqrt:
					switch(basicType) {
					case EbtFloat: tempConstArray[i].setFConst(1.0f / sqrtf(unionArray[i].getFConst())); break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpFloatBitsToInt:
					switch(basicType) {
					case EbtFloat:
						tempConstArray[i].setIConst(sw::bitCast<int>(unionArray[i].getFConst()));
						type.setBasicType(EbtInt);
						break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
					break;
				case EOpFloatBitsToUint:
					switch(basicType) {
					case EbtFloat:
						tempConstArray[i].setUConst(sw::bitCast<unsigned int>(unionArray[i].getFConst()));
						type.setBasicType(EbtUInt);
						break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpIntBitsToFloat:
					switch(basicType) {
					case EbtInt:
						tempConstArray[i].setFConst(sw::bitCast<float>(unionArray[i].getIConst()));
						type.setBasicType(EbtFloat);
						break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				case EOpUintBitsToFloat:
					switch(basicType) {
					case EbtUInt:
						tempConstArray[i].setFConst(sw::bitCast<float>(unionArray[i].getUConst()));
						type.setBasicType(EbtFloat);
						break;
					default:
						infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
						return 0;
					}
					break;
				default:
					return 0;
			}
		}
		newNode = new TIntermConstantUnion(tempConstArray, type);
		newNode->setLine(getLine());
		return newNode;
	}
}

TIntermTyped* TIntermediate::promoteConstantUnion(TBasicType promoteTo, TIntermConstantUnion* node)
{
	size_t size = node->getType().getObjectSize();

	ConstantUnion *leftUnionArray = new ConstantUnion[size];

	for(size_t i = 0; i < size; i++) {
		switch (promoteTo) {
			case EbtFloat:
				switch (node->getType().getBasicType()) {
					case EbtInt:
						leftUnionArray[i].setFConst(static_cast<float>(node->getIConst(i)));
						break;
					case EbtUInt:
						leftUnionArray[i].setFConst(static_cast<float>(node->getUConst(i)));
						break;
					case EbtBool:
						leftUnionArray[i].setFConst(static_cast<float>(node->getBConst(i)));
						break;
					case EbtFloat:
						leftUnionArray[i].setFConst(static_cast<float>(node->getFConst(i)));
						break;
					default:
						infoSink.info.message(EPrefixInternalError, "Cannot promote", node->getLine());
						return 0;
				}
				break;
			case EbtInt:
				switch (node->getType().getBasicType()) {
					case EbtInt:
						leftUnionArray[i].setIConst(static_cast<int>(node->getIConst(i)));
						break;
					case EbtUInt:
						leftUnionArray[i].setIConst(static_cast<int>(node->getUConst(i)));
						break;
					case EbtBool:
						leftUnionArray[i].setIConst(static_cast<int>(node->getBConst(i)));
						break;
					case EbtFloat:
						leftUnionArray[i].setIConst(static_cast<int>(node->getFConst(i)));
						break;
					default:
						infoSink.info.message(EPrefixInternalError, "Cannot promote", node->getLine());
						return 0;
				}
				break;
			case EbtUInt:
				switch (node->getType().getBasicType()) {
					case EbtInt:
						leftUnionArray[i].setUConst(static_cast<unsigned int>(node->getIConst(i)));
						break;
					case EbtUInt:
						leftUnionArray[i].setUConst(static_cast<unsigned int>(node->getUConst(i)));
						break;
					case EbtBool:
						leftUnionArray[i].setUConst(static_cast<unsigned int>(node->getBConst(i)));
						break;
					case EbtFloat:
						leftUnionArray[i].setUConst(static_cast<unsigned int>(node->getFConst(i)));
						break;
					default:
						infoSink.info.message(EPrefixInternalError, "Cannot promote", node->getLine());
						return 0;
				}
				break;
			case EbtBool:
				switch (node->getType().getBasicType()) {
					case EbtInt:
						leftUnionArray[i].setBConst(node->getIConst(i) != 0);
						break;
					case EbtUInt:
						leftUnionArray[i].setBConst(node->getUConst(i) != 0);
						break;
					case EbtBool:
						leftUnionArray[i].setBConst(node->getBConst(i));
						break;
					case EbtFloat:
						leftUnionArray[i].setBConst(node->getFConst(i) != 0.0f);
						break;
					default:
						infoSink.info.message(EPrefixInternalError, "Cannot promote", node->getLine());
						return 0;
				}

				break;
			default:
				infoSink.info.message(EPrefixInternalError, "Incorrect data type found", node->getLine());
				return 0;
		}

	}

	const TType& t = node->getType();

	return addConstantUnion(leftUnionArray, TType(promoteTo, t.getPrecision(), t.getQualifier(), t.getNominalSize(), t.getSecondarySize(), t.isArray()), node->getLine());
}

