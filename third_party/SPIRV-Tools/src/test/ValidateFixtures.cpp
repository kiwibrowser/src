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

// Common validation fixtures for unit tests

#include "ValidateFixtures.h"

#include <functional>
#include <tuple>
#include <utility>

#include "TestFixture.h"

namespace spvtest {

template <typename T>
ValidateBase<T>::ValidateBase() : binary_(), diagnostic_() {}

template <typename T>
spv_const_binary ValidateBase<T>::get_const_binary() {
  return spv_const_binary(binary_);
}

template <typename T>
void ValidateBase<T>::TearDown() {
  if (diagnostic_) {
    spvDiagnosticPrint(diagnostic_);
  }
  spvDiagnosticDestroy(diagnostic_);
  spvBinaryDestroy(binary_);
}

template <typename T>
void ValidateBase<T>::CompileSuccessfully(std::string code,
                                          spv_target_env env) {
  spv_diagnostic diagnostic = nullptr;
  ASSERT_EQ(SPV_SUCCESS,
            spvTextToBinary(ScopedContext(env).context, code.c_str(),
                            code.size(), &binary_, &diagnostic))
      << "ERROR: " << diagnostic->error
      << "\nSPIR-V could not be compiled into binary:\n"
      << code;
}

template <typename T>
spv_result_t ValidateBase<T>::ValidateInstructions(spv_target_env env) {
  return spvValidate(ScopedContext(env).context, get_const_binary(),
                     &diagnostic_);
}

template <typename T>
std::string ValidateBase<T>::getDiagnosticString() {
  return std::string(diagnostic_->error);
}

template <typename T>
spv_position_t ValidateBase<T>::getErrorPosition() {
  return diagnostic_->position;
}

template class spvtest::ValidateBase<bool>;
template class spvtest::ValidateBase<int>;
template class spvtest::ValidateBase<std::string>;
template class spvtest::ValidateBase<std::pair<std::string, bool>>;
template class spvtest::ValidateBase<
    std::tuple<std::string, std::pair<std::string, std::vector<std::string>>>>;
template class spvtest::ValidateBase<
    std::tuple<int, std::tuple<std::string, std::function<spv_result_t(int)>,
                               std::function<spv_result_t(int)>>>>;
}
