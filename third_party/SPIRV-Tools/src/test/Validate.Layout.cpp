// Copyright (c) 2015-2016 The Khronos Group Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

// Validation tests for Logical Layout

#include <functional>
#include <sstream>
#include <string>
#include <utility>

#include "gmock/gmock.h"

#include "UnitSPIRV.h"
#include "ValidateFixtures.h"
#include "source/diagnostic.h"

using std::function;
using std::ostream;
using std::ostream_iterator;
using std::pair;
using std::stringstream;
using std::string;
using std::tie;
using std::tuple;
using std::vector;

using ::testing::HasSubstr;

using libspirv::spvResultToString;

using pred_type = function<spv_result_t(int)>;
using ValidateLayout =
    spvtest::ValidateBase<tuple<int, tuple<string, pred_type, pred_type>>>;

namespace {

// returns true if order is equal to VAL
template <int VAL, spv_result_t RET = SPV_ERROR_INVALID_LAYOUT>
spv_result_t Equals(int order) {
  return order == VAL ? SPV_SUCCESS : RET;
}

// returns true if order is between MIN and MAX(inclusive)
template <int MIN, int MAX, spv_result_t RET = SPV_ERROR_INVALID_LAYOUT>
struct Range {
  explicit Range(bool inverse = false) : inverse_(inverse) {}
  spv_result_t operator()(int order) {
    return (inverse_ ^ (order >= MIN && order <= MAX)) ? SPV_SUCCESS : RET;
  }

 private:
  bool inverse_;
};

template <typename... T>
spv_result_t InvalidSet(int order) {
  for (spv_result_t val : {T(true)(order)...})
    if (val != SPV_SUCCESS) return val;
  return SPV_SUCCESS;
}

// SPIRV source used to test the logical layout
const vector<string>& getInstructions() {
  // clang-format off
  static const vector<string> instructions = {
    "OpCapability Shader",
    "OpExtension \"TestExtension\"",
    "%inst = OpExtInstImport \"GLSL.std.450\"",
    "OpMemoryModel Logical GLSL450",
    "OpEntryPoint GLCompute %func \"\"",
    "OpExecutionMode %func LocalSize 1 1 1",
    "%str = OpString \"Test String\"",
    "%str2 = OpString \"blabla\"",
    "OpSource GLSL 450 %str \"uniform vec3 var = vec3(4.0);\"",
    "OpSourceContinued \"void main(){return;}\"",
    "OpSourceExtension \"Test extension\"",
    "OpName %func \"MyFunction\"",
    "OpMemberName %struct 1 \"my_member\"",
    "OpDecorate %dgrp RowMajor",
    "OpMemberDecorate %struct 1 RowMajor",
    "%dgrp   = OpDecorationGroup",
    "OpGroupDecorate %dgrp %mat33 %mat44",
    "%intt     = OpTypeInt 32 1",
    "%floatt   = OpTypeFloat 32",
    "%voidt    = OpTypeVoid",
    "%boolt    = OpTypeBool",
    "%vec4     = OpTypeVector %intt 4",
    "%vec3     = OpTypeVector %intt 3",
    "%mat33    = OpTypeMatrix %vec3 3",
    "%mat44    = OpTypeMatrix %vec4 4",
    "%struct   = OpTypeStruct %intt %mat33",
    "%vfunct   = OpTypeFunction %voidt",
    "%viifunct = OpTypeFunction %voidt %intt %intt",
    "%one      = OpConstant %intt 1",
    // TODO(umar): OpConstant fails because the type is not defined
    // TODO(umar): OpGroupMemberDecorate
    "OpLine %str 3 4",
    "OpNoLine",
    "%func     = OpFunction %voidt None %vfunct",
    "OpFunctionEnd",
    "%func2    = OpFunction %voidt None %viifunct",
    "%funcp1   = OpFunctionParameter %intt",
    "%funcp2   = OpFunctionParameter %intt",
    "%fLabel   = OpLabel",
    "            OpNop",
    "            OpReturn",
    "OpFunctionEnd"
  };
  return instructions;
}

static const int kRangeEnd = 1000;
pred_type All = Range<0, kRangeEnd>();

INSTANTIATE_TEST_CASE_P(InstructionsOrder,
    ValidateLayout,
    ::testing::Combine(::testing::Range((int)0, (int)getInstructions().size()),
    // Note: Because of ID dependencies between instructions, some instructions
    // are not free to be placed anywhere without triggering an non-layout
    // validation error. Therefore, "Lines to compile" for some instructions
    // are not "All" in the below.
    //
    //                                   | Instruction                | Line(s) valid          | Lines to compile
    ::testing::Values( make_tuple(string("OpCapability")              , Equals<0>              , Range<0, 2>())
                     , make_tuple(string("OpExtension")               , Equals<1>              , All)
                     , make_tuple(string("OpExtInstImport")           , Equals<2>              , All)
                     , make_tuple(string("OpMemoryModel")             , Equals<3>              , Range<1, kRangeEnd>())
                     , make_tuple(string("OpEntryPoint")              , Equals<4>              , All)
                     , make_tuple(string("OpExecutionMode")           , Equals<5>              , All)
                     , make_tuple(string("OpSource ")                 , Range<6, 10>()         , Range<7, kRangeEnd>())
                     , make_tuple(string("OpSourceContinued ")        , Range<6, 10>()         , All)
                     , make_tuple(string("OpSourceExtension ")        , Range<6, 10>()         , All)
                     , make_tuple(string("%str2 = OpString ")         , Range<6, 10>()         , All)
                     , make_tuple(string("OpName ")                   , Range<11, 12>()        , All)
                     , make_tuple(string("OpMemberName ")             , Range<11, 12>()        , All)
                     , make_tuple(string("OpDecorate ")               , Range<13, 16>()        , All)
                     , make_tuple(string("OpMemberDecorate ")         , Range<13, 16>()        , All)
                     , make_tuple(string("OpGroupDecorate ")          , Range<13, 16>()        , Range<16, kRangeEnd>())
                     , make_tuple(string("OpDecorationGroup")         , Range<13, 16>()        , Range<0, 15>())
                     , make_tuple(string("OpTypeBool")                , Range<17, 30>()        , All)
                     , make_tuple(string("OpTypeVoid")                , Range<17, 30>()        , Range<0, 25>())
                     , make_tuple(string("OpTypeFloat")               , Range<17, 30>()        , All)
                     , make_tuple(string("OpTypeInt")                 , Range<17, 30>()        , Range<0, 20>())
                     , make_tuple(string("OpTypeVector %intt 4")      , Range<17, 30>()        , Range<18, 23>())
                     , make_tuple(string("OpTypeMatrix %vec4 4")      , Range<17, 30>()        , Range<22, kRangeEnd>())
                     , make_tuple(string("OpTypeStruct")              , Range<17, 30>()        , Range<24, kRangeEnd>())
                     , make_tuple(string("%vfunct   = OpTypeFunction"), Range<17, 30>()        , Range<20, 30>())
                     , make_tuple(string("OpConstant")                , Range<17, 30>()        , Range<20, kRangeEnd>())
                     , make_tuple(string("OpLine ")                   , Range<17, kRangeEnd>() , Range<7, kRangeEnd>())
                     , make_tuple(string("OpNoLine")                  , Range<17, kRangeEnd>() , All)
                     , make_tuple(string("OpLabel")                   , Equals<36>             , All)
                     , make_tuple(string("OpNop")                     , Equals<37>             , All)
                     , make_tuple(string("OpReturn")                  , Equals<38>             , All)
    )),);
// clang-format on

// Creates a new vector which removes the string if the substr is found in the
// instructions vector and reinserts it in the location specified by order.
// NOTE: This will not work correctly if there are two instances of substr in
// instructions
vector<string> GenerateCode(string substr, int order) {
  vector<string> code(getInstructions().size());
  vector<string> inst(1);
  partition_copy(begin(getInstructions()), end(getInstructions()), begin(code),
                 begin(inst), [=](const string& str) {
                   return string::npos == str.find(substr);
                 });

  code.insert(begin(code) + order, inst.front());
  return code;
}

// This test will check the logical layout of a binary by removing each
// instruction in the pair of the INSTANTIATE_TEST_CASE_P call and moving it in
// the SPIRV source formed by combining the vector "instructions".
TEST_P(ValidateLayout, Layout) {
  int order;
  string instruction;
  pred_type pred;
  pred_type test_pred;  // Predicate to determine if the test should be build
  tuple<string, pred_type, pred_type> testCase;

  tie(order, testCase) = GetParam();
  tie(instruction, pred, test_pred) = testCase;

  // Skip test which break the code generation
  if (test_pred(order)) return;

  vector<string> code = GenerateCode(instruction, order);

  stringstream ss;
  copy(begin(code), end(code), ostream_iterator<string>(ss, "\n"));

  // printf("code: \n%s\n", ss.str().c_str());
  CompileSuccessfully(ss.str());
  spv_result_t result;
  // clang-format off
  ASSERT_EQ(pred(order), result = ValidateInstructions())
    << "Actual: "        << spvResultToString(result)
    << "\nExpected: "    << spvResultToString(pred(order))
    << "\nOrder: "       << order
    << "\nInstruction: " << instruction
    << "\nCode: \n"      << ss.str();
  // clang-format on
}

TEST_F(ValidateLayout, MemoryModelMissing) {
  string str = R"(
    OpCapability Matrix
    OpExtension "TestExtension"
    %inst = OpExtInstImport "GLSL.std.450"
    OpEntryPoint GLCompute %func ""
    OpExecutionMode %func LocalSize 1 1 1
    )";

  CompileSuccessfully(str);
  ASSERT_EQ(SPV_ERROR_INVALID_LAYOUT, ValidateInstructions());
}

TEST_F(ValidateLayout, FunctionDefinitionBeforeDeclarationBad) {
  char str[] = R"(
           OpCapability Shader
           OpMemoryModel Logical GLSL450
           OpDecorate %var Restrict
%intt    = OpTypeInt 32 1
%voidt   = OpTypeVoid
%vfunct  = OpTypeFunction %voidt
%vifunct = OpTypeFunction %voidt %intt
%ptrt    = OpTypePointer Function %intt
%func    = OpFunction %voidt None %vfunct
%funcl   = OpLabel
           OpNop
           OpReturn
           OpFunctionEnd
%func2   = OpFunction %voidt None %vifunct ; must appear before definition
%func2p  = OpFunctionParameter %intt
           OpFunctionEnd
)";

  CompileSuccessfully(str);
  ASSERT_EQ(SPV_ERROR_INVALID_LAYOUT, ValidateInstructions());
}

// TODO(umar): Passes but gives incorrect error message. Should be fixed after
// type checking
TEST_F(ValidateLayout, LabelBeforeFunctionParameterBad) {
  char str[] = R"(
           OpCapability Shader
           OpMemoryModel Logical GLSL450
           OpDecorate %var Restrict
%intt    = OpTypeInt 32 1
%voidt   = OpTypeVoid
%vfunct  = OpTypeFunction %voidt
%vifunct = OpTypeFunction %voidt %intt
%ptrt    = OpTypePointer Function %intt
%func    = OpFunction %voidt None %vifunct
%funcl   = OpLabel                    ; Label appears before function parameter
%func2p  = OpFunctionParameter %intt
           OpNop
           OpReturn
           OpFunctionEnd
)";

  CompileSuccessfully(str);
  ASSERT_EQ(SPV_ERROR_INVALID_LAYOUT, ValidateInstructions());
}

TEST_F(ValidateLayout, FuncParameterNotImmediatlyAfterFuncBad) {
  char str[] = R"(
           OpCapability Shader
           OpMemoryModel Logical GLSL450
           OpDecorate %var Restrict
%intt    = OpTypeInt 32 1
%voidt   = OpTypeVoid
%vfunct  = OpTypeFunction %voidt
%vifunct = OpTypeFunction %voidt %intt
%ptrt    = OpTypePointer Function %intt
%func    = OpFunction %voidt None %vifunct
%funcl   = OpLabel
           OpNop
           OpBranch %next
%func2p  = OpFunctionParameter %intt        ;FunctionParameter appears in a function but not immediately afterwards
%next    = OpLabel
           OpNop
           OpReturn
           OpFunctionEnd
)";

  CompileSuccessfully(str);
  ASSERT_EQ(SPV_ERROR_INVALID_LAYOUT, ValidateInstructions());
}

using ValidateOpFunctionParameter = spvtest::ValidateBase<int>;

TEST_F(ValidateOpFunctionParameter, OpLineBetweenParameters) {
  const auto s = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
%foo_frag = OpString "foo.frag"
%i32 = OpTypeInt 32 1
%tf = OpTypeFunction %i32 %i32 %i32
%c = OpConstant %i32 123
%f = OpFunction %i32 None %tf
OpLine %foo_frag 1 1
%p1 = OpFunctionParameter %i32
OpNoLine
%p2 = OpFunctionParameter %i32
%l = OpLabel
OpReturnValue %c
OpFunctionEnd
)";
  CompileSuccessfully(s);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateOpFunctionParameter, TooManyParameters) {
  const auto s = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
%i32 = OpTypeInt 32 1
%tf = OpTypeFunction %i32 %i32 %i32
%c = OpConstant %i32 123
%f = OpFunction %i32 None %tf
%p1 = OpFunctionParameter %i32
%p2 = OpFunctionParameter %i32
%xp3 = OpFunctionParameter %i32
%xp4 = OpFunctionParameter %i32
%xp5 = OpFunctionParameter %i32
%xp6 = OpFunctionParameter %i32
%xp7 = OpFunctionParameter %i32
%l = OpLabel
OpReturnValue %c
OpFunctionEnd
)";
  CompileSuccessfully(s);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
}
// TODO(umar): Test optional instructions
}
