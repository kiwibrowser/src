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

#include "localintermediate.h"
#include "SymbolTable.h"

//
// Two purposes:
// 1.  Show an example of how to iterate tree.  Functions can
//     also directly call Traverse() on children themselves to
//     have finer grained control over the process than shown here.
//     See the last function for how to get started.
// 2.  Print out a text based description of the tree.
//

//
// Use this class to carry along data from node to node in
// the traversal
//
class TOutputTraverser : public TIntermTraverser {
public:
	TOutputTraverser(TInfoSinkBase& i) : sink(i) { }
	TInfoSinkBase& sink;

protected:
	void visitSymbol(TIntermSymbol*);
	void visitConstantUnion(TIntermConstantUnion*);
	bool visitBinary(Visit visit, TIntermBinary*);
	bool visitUnary(Visit visit, TIntermUnary*);
	bool visitSelection(Visit visit, TIntermSelection*);
	bool visitAggregate(Visit visit, TIntermAggregate*);
	bool visitLoop(Visit visit, TIntermLoop*);
	bool visitBranch(Visit visit, TIntermBranch*);
};

TString TType::getCompleteString() const
{
	TStringStream stream;

	if (qualifier != EvqTemporary && qualifier != EvqGlobal)
		stream << getQualifierString() << " " << getPrecisionString() << " ";
	if (array)
		stream << "array of ";
	if (isMatrix())
		stream << static_cast<int>(primarySize) << "X" << static_cast<int>(secondarySize) << " matrix of ";
	else if(primarySize > 1)
		stream << static_cast<int>(primarySize) << "-component vector of ";

	stream << getBasicString();
	return stream.str();
}

//
// Helper functions for printing, not part of traversing.
//

void OutputTreeText(TInfoSinkBase& sink, TIntermNode* node, const int depth)
{
	int i;

	sink.location(node->getLine());

	for (i = 0; i < depth; ++i)
		sink << "  ";
}

//
// The rest of the file are the traversal functions.  The last one
// is the one that starts the traversal.
//
// Return true from interior nodes to have the external traversal
// continue on to children.  If you process children yourself,
// return false.
//

void TOutputTraverser::visitSymbol(TIntermSymbol* node)
{
	OutputTreeText(sink, node, mDepth);

	sink << "'" << node->getSymbol() << "' ";
	sink << "(" << node->getCompleteString() << ")\n";
}

bool TOutputTraverser::visitBinary(Visit visit, TIntermBinary* node)
{
	TInfoSinkBase& out = sink;

	OutputTreeText(out, node, mDepth);

	switch (node->getOp()) {
	case EOpAssign:                   out << "move second child to first child";           break;
	case EOpInitialize:               out << "initialize first child with second child";   break;
	case EOpAddAssign:                out << "add second child into first child";          break;
	case EOpSubAssign:                out << "subtract second child into first child";     break;
	case EOpMulAssign:                out << "multiply second child into first child";     break;
	case EOpVectorTimesMatrixAssign:  out << "matrix mult second child into first child";  break;
	case EOpVectorTimesScalarAssign:  out << "vector scale second child into first child"; break;
	case EOpMatrixTimesScalarAssign:  out << "matrix scale second child into first child"; break;
	case EOpMatrixTimesMatrixAssign:  out << "matrix mult second child into first child";  break;
	case EOpDivAssign:                out << "divide second child into first child";       break;
	case EOpIModAssign:               out << "modulo second child into first child";       break;
	case EOpBitShiftLeftAssign:       out << "bit-wise shift first child left by second child";  break;
	case EOpBitShiftRightAssign:      out << "bit-wise shift first child right by second child"; break;
	case EOpBitwiseAndAssign:         out << "bit-wise and second child into first child"; break;
	case EOpBitwiseXorAssign:         out << "bit-wise xor second child into first child"; break;
	case EOpBitwiseOrAssign:          out << "bit-wise or second child into first child";  break;
	case EOpIndexDirect:   out << "direct index";   break;
	case EOpIndexIndirect: out << "indirect index"; break;
	case EOpIndexDirectStruct:   out << "direct index for structure";   break;
	case EOpVectorSwizzle: out << "vector swizzle"; break;

	case EOpAdd:    out << "add";                     break;
	case EOpSub:    out << "subtract";                break;
	case EOpMul:    out << "component-wise multiply"; break;
	case EOpDiv:    out << "divide";                  break;
	case EOpIMod:   out << "modulo";                  break;
	case EOpBitShiftLeft:     out << "bit-wise shift left";           break;
	case EOpBitShiftRight:    out << "bit-wise shift right";          break;
	case EOpBitwiseAnd:       out << "bit-wise and";                  break;
	case EOpBitwiseXor:       out << "bit-wise xor";                  break;
	case EOpBitwiseOr:        out << "bit-wise or";                   break;
	case EOpEqual:            out << "Compare Equal";                 break;
	case EOpNotEqual:         out << "Compare Not Equal";             break;
	case EOpLessThan:         out << "Compare Less Than";             break;
	case EOpGreaterThan:      out << "Compare Greater Than";          break;
	case EOpLessThanEqual:    out << "Compare Less Than or Equal";    break;
	case EOpGreaterThanEqual: out << "Compare Greater Than or Equal"; break;

	case EOpVectorTimesScalar: out << "vector-scale";          break;
	case EOpVectorTimesMatrix: out << "vector-times-matrix";   break;
	case EOpMatrixTimesVector: out << "matrix-times-vector";   break;
	case EOpMatrixTimesScalar: out << "matrix-scale";          break;
	case EOpMatrixTimesMatrix: out << "matrix-multiply";       break;

	case EOpLogicalOr:  out << "logical-or";   break;
	case EOpLogicalXor: out << "logical-xor"; break;
	case EOpLogicalAnd: out << "logical-and"; break;
	default: out << "<unknown op>";
	}

	out << " (" << node->getCompleteString() << ")";

	out << "\n";

	return true;
}

bool TOutputTraverser::visitUnary(Visit visit, TIntermUnary* node)
{
	TInfoSinkBase& out = sink;

	OutputTreeText(out, node, mDepth);

	switch (node->getOp()) {
	case EOpNegative:       out << "Negate value";         break;
	case EOpVectorLogicalNot:
	case EOpLogicalNot:     out << "Negate conditional";   break;
	case EOpBitwiseNot:     out << "bit-wise not";         break;

	case EOpPostIncrement:  out << "Post-Increment";       break;
	case EOpPostDecrement:  out << "Post-Decrement";       break;
	case EOpPreIncrement:   out << "Pre-Increment";        break;
	case EOpPreDecrement:   out << "Pre-Decrement";        break;

	case EOpRadians:        out << "radians";              break;
	case EOpDegrees:        out << "degrees";              break;
	case EOpSin:            out << "sine";                 break;
	case EOpCos:            out << "cosine";               break;
	case EOpTan:            out << "tangent";              break;
	case EOpAsin:           out << "arc sine";             break;
	case EOpAcos:           out << "arc cosine";           break;
	case EOpAtan:           out << "arc tangent";          break;
	case EOpSinh:           out << "hyperbolic sine";        break;
	case EOpCosh:           out << "hyperbolic cosine";      break;
	case EOpTanh:           out << "hyperbolic tangent";     break;
	case EOpAsinh:          out << "arc hyperbolic sine";    break;
	case EOpAcosh:          out << "arc hyperbolic cosine";  break;
	case EOpAtanh:          out << "arc hyperbolic tangent"; break;

	case EOpExp:            out << "exp";                  break;
	case EOpLog:            out << "log";                  break;
	case EOpExp2:           out << "exp2";                 break;
	case EOpLog2:           out << "log2";                 break;
	case EOpSqrt:           out << "sqrt";                 break;
	case EOpInverseSqrt:    out << "inverse sqrt";         break;

	case EOpAbs:            out << "Absolute value";       break;
	case EOpSign:           out << "Sign";                 break;
	case EOpFloor:          out << "Floor";                break;
	case EOpTrunc:          out << "Trunc";                break;
	case EOpRound:          out << "Round";                break;
	case EOpRoundEven:      out << "RoundEven";            break;
	case EOpCeil:           out << "Ceiling";              break;
	case EOpFract:          out << "Fraction";             break;
	case EOpIsNan:          out << "Is not a number";      break;
	case EOpIsInf:          out << "Is infinity";          break;

	case EOpFloatBitsToInt: out << "float bits to int";    break;
	case EOpFloatBitsToUint: out << "float bits to uint";  break;
	case EOpIntBitsToFloat: out << "int bits to float";    break;
	case EOpUintBitsToFloat: out << "uint bits to float";  break;

	case EOpPackSnorm2x16:  out << "pack Snorm 2x16";      break;
	case EOpPackUnorm2x16:  out << "pack Unorm 2x16";      break;
	case EOpPackHalf2x16:   out << "pack half 2x16";       break;

	case EOpUnpackSnorm2x16: out << "unpack Snorm 2x16";   break;
	case EOpUnpackUnorm2x16: out << "unpack Unorm 2x16";   break;
	case EOpUnpackHalf2x16:  out << "unpack half 2x16";    break;

	case EOpLength:         out << "length";               break;
	case EOpNormalize:      out << "normalize";            break;
		//	case EOpDPdx:           out << "dPdx";                 break;               
		//	case EOpDPdy:           out << "dPdy";                 break;   
		//	case EOpFwidth:         out << "fwidth";               break;                   

	case EOpDeterminant:    out << "determinant";          break;
	case EOpTranspose:      out << "transpose";            break;
	case EOpInverse:        out << "inverse";              break;

	case EOpAny:            out << "any";                  break;
	case EOpAll:            out << "all";                  break;

	default: out.message(EPrefixError, "Bad unary op");
	}

	out << " (" << node->getCompleteString() << ")";

	out << "\n";

	return true;
}

bool TOutputTraverser::visitAggregate(Visit visit, TIntermAggregate* node)
{
	TInfoSinkBase& out = sink;

	if (node->getOp() == EOpNull) {
		out.message(EPrefixError, "node is still EOpNull!");
		return true;
	}

	OutputTreeText(out, node, mDepth);

	switch (node->getOp()) {
	case EOpSequence:      out << "Sequence\n"; return true;
	case EOpComma:         out << "Comma\n"; return true;
	case EOpFunction:      out << "Function Definition: " << node->getName(); break;
	case EOpFunctionCall:  out << "Function Call: " << node->getName(); break;
	case EOpParameters:    out << "Function Parameters: ";              break;

	case EOpConstructFloat: out << "Construct float"; break;
	case EOpConstructVec2:  out << "Construct vec2";  break;
	case EOpConstructVec3:  out << "Construct vec3";  break;
	case EOpConstructVec4:  out << "Construct vec4";  break;
	case EOpConstructBool:  out << "Construct bool";  break;
	case EOpConstructBVec2: out << "Construct bvec2"; break;
	case EOpConstructBVec3: out << "Construct bvec3"; break;
	case EOpConstructBVec4: out << "Construct bvec4"; break;
	case EOpConstructInt:   out << "Construct int";   break;
	case EOpConstructIVec2: out << "Construct ivec2"; break;
	case EOpConstructIVec3: out << "Construct ivec3"; break;
	case EOpConstructIVec4: out << "Construct ivec4"; break;
	case EOpConstructUInt:  out << "Construct uint";  break;
	case EOpConstructUVec2: out << "Construct uvec2"; break;
	case EOpConstructUVec3: out << "Construct uvec3"; break;
	case EOpConstructUVec4: out << "Construct uvec4"; break;
	case EOpConstructMat2:  out << "Construct mat2";  break;
	case EOpConstructMat2x3:  out << "Construct mat2x3";  break;
	case EOpConstructMat2x4:  out << "Construct mat2x4";  break;
	case EOpConstructMat3x2:  out << "Construct mat3x2";  break;
	case EOpConstructMat3:  out << "Construct mat3";  break;
	case EOpConstructMat3x4:  out << "Construct mat3x4";  break;
	case EOpConstructMat4x2:  out << "Construct mat4x2";  break;
	case EOpConstructMat4x3:  out << "Construct mat4x3";  break;
	case EOpConstructMat4:  out << "Construct mat4";  break;
	case EOpConstructStruct:  out << "Construct structure";  break;

	case EOpLessThan:         out << "Compare Less Than";             break;
	case EOpGreaterThan:      out << "Compare Greater Than";          break;
	case EOpLessThanEqual:    out << "Compare Less Than or Equal";    break;
	case EOpGreaterThanEqual: out << "Compare Greater Than or Equal"; break;
	case EOpVectorEqual:      out << "Equal";                         break;
	case EOpVectorNotEqual:   out << "NotEqual";                      break;

	case EOpMod:           out << "mod";         break;
	case EOpModf:          out << "modf";        break;
	case EOpPow:           out << "pow";         break;

	case EOpAtan:          out << "arc tangent"; break;

	case EOpMin:           out << "min";         break;
	case EOpMax:           out << "max";         break;
	case EOpClamp:         out << "clamp";       break;
	case EOpMix:           out << "mix";         break;
	case EOpStep:          out << "step";        break;
	case EOpSmoothStep:    out << "smoothstep";  break;

	case EOpFloatBitsToInt:  out << "floatBitsToInt";  break;
	case EOpFloatBitsToUint: out << "floatBitsToUint"; break;
	case EOpIntBitsToFloat:  out << "intBitsToFloat";  break;
	case EOpUintBitsToFloat: out << "uintBitsToFloat"; break;

	case EOpDistance:      out << "distance";                break;
	case EOpDot:           out << "dot-product";             break;
	case EOpCross:         out << "cross-product";           break;
	case EOpFaceForward:   out << "face-forward";            break;
	case EOpReflect:       out << "reflect";                 break;
	case EOpRefract:       out << "refract";                 break;
	case EOpMul:           out << "component-wise multiply"; break;
	case EOpOuterProduct:  out << "outer product";           break;

	default: out.message(EPrefixError, "Bad aggregation op");
	}

	if (node->getOp() != EOpSequence && node->getOp() != EOpParameters)
		out << " (" << node->getCompleteString() << ")";

	out << "\n";

	return true;
}

bool TOutputTraverser::visitSelection(Visit visit, TIntermSelection* node)
{
	TInfoSinkBase& out = sink;

	OutputTreeText(out, node, mDepth);

	out << "Test condition and select";
	out << " (" << node->getCompleteString() << ")\n";

	++mDepth;

	OutputTreeText(sink, node, mDepth);
	out << "Condition\n";
	node->getCondition()->traverse(this);

	OutputTreeText(sink, node, mDepth);
	if (node->getTrueBlock()) {
		out << "true case\n";
		node->getTrueBlock()->traverse(this);
	} else
		out << "true case is null\n";

	if (node->getFalseBlock()) {
		OutputTreeText(sink, node, mDepth);
		out << "false case\n";
		node->getFalseBlock()->traverse(this);
	}

	--mDepth;

	return false;
}

void TOutputTraverser::visitConstantUnion(TIntermConstantUnion* node)
{
	TInfoSinkBase& out = sink;

	size_t size = node->getType().getObjectSize();

	for(size_t i = 0; i < size; i++) {
		OutputTreeText(out, node, mDepth);
		switch (node->getUnionArrayPointer()[i].getType()) {
		case EbtBool:
			if (node->getUnionArrayPointer()[i].getBConst())
				out << "true";
			else
				out << "false";

			out << " (" << "const bool" << ")";
			out << "\n";
			break;
		case EbtFloat:
			out << node->getUnionArrayPointer()[i].getFConst();
			out << " (const float)\n";
			break;
		case EbtInt:
			out << node->getUnionArrayPointer()[i].getIConst();
			out << " (const int)\n";
			break;
		case EbtUInt:
			out << node->getUnionArrayPointer()[i].getUConst();
			out << " (const uint)\n";
			break;
		default:
			out.message(EPrefixInternalError, "Unknown constant", node->getLine());
			break;
		}
	}
}

bool TOutputTraverser::visitLoop(Visit visit, TIntermLoop* node)
{
	TInfoSinkBase& out = sink;

	OutputTreeText(out, node, mDepth);

	out << "Loop with condition ";
	if (node->getType() == ELoopDoWhile)
		out << "not ";
	out << "tested first\n";

	++mDepth;

	OutputTreeText(sink, node, mDepth);
	if (node->getCondition()) {
		out << "Loop Condition\n";
		node->getCondition()->traverse(this);
	} else
		out << "No loop condition\n";

	OutputTreeText(sink, node, mDepth);
	if (node->getBody()) {
		out << "Loop Body\n";
		node->getBody()->traverse(this);
	} else
		out << "No loop body\n";

	if (node->getExpression()) {
		OutputTreeText(sink, node, mDepth);
		out << "Loop Terminal Expression\n";
		node->getExpression()->traverse(this);
	}

	--mDepth;

	return false;
}

bool TOutputTraverser::visitBranch(Visit visit, TIntermBranch* node)
{
	TInfoSinkBase& out = sink;

	OutputTreeText(out, node, mDepth);

	switch (node->getFlowOp()) {
	case EOpKill:      out << "Branch: Kill";           break;
	case EOpBreak:     out << "Branch: Break";          break;
	case EOpContinue:  out << "Branch: Continue";       break;
	case EOpReturn:    out << "Branch: Return";         break;
	default:           out << "Branch: Unknown Branch"; break;
	}

	if (node->getExpression()) {
		out << " with expression\n";
		++mDepth;
		node->getExpression()->traverse(this);
		--mDepth;
	} else
		out << "\n";

	return false;
}

//
// This function is the one to call externally to start the traversal.
// Individual functions can be initialized to 0 to skip processing of that
// type of node.  It's children will still be processed.
//
void TIntermediate::outputTree(TIntermNode* root)
{
	if (root == 0)
		return;

	TOutputTraverser it(infoSink.info);

	root->traverse(&it);
}
