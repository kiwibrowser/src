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

#include "UnitSPIRV.h"

namespace {

TEST(DiagnosticPrint, Default) {
  char message[] = "Test Diagnostic!";
  spv_diagnostic_t diagnostic = {{2, 3, 5}, message};
  // TODO: Redirect stderr
  ASSERT_EQ(SPV_SUCCESS, spvDiagnosticPrint(&diagnostic));
  // TODO: Validate the output of spvDiagnosticPrint()
  // TODO: Remove the redirection of stderr
}

TEST(DiagnosticPrint, InvalidDiagnostic) {
  ASSERT_EQ(SPV_ERROR_INVALID_DIAGNOSTIC, spvDiagnosticPrint(nullptr));
}

// TODO(dneto): We should be able to redirect the diagnostic printing.
// Once we do that, we can test diagnostic corner cases.

}  // anonymous namespace
