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

TEST(DiagnosticDestroy, DestroyNull) { spvDiagnosticDestroy(nullptr); }

// Returns a newly created diagnostic value.
spv_diagnostic MakeValidDiagnostic() {
  spv_position_t position = {};
  spv_diagnostic diagnostic = spvDiagnosticCreate(&position, "");
  EXPECT_NE(nullptr, diagnostic);
  return diagnostic;
}

TEST(DiagnosticDestroy, DestroyValidDiagnostic) {
  spv_diagnostic diagnostic = MakeValidDiagnostic();
  spvDiagnosticDestroy(diagnostic);
  // We aren't allowed to use the diagnostic pointer anymore.
  // So we can't test its behaviour.
}

TEST(DiagnosticDestroy, DestroyValidDiagnosticAfterReassignment) {
  spv_diagnostic diagnostic = MakeValidDiagnostic();
  spv_diagnostic second_diagnostic = MakeValidDiagnostic();
  EXPECT_TRUE(diagnostic != second_diagnostic);
  spvDiagnosticDestroy(diagnostic);
  diagnostic = second_diagnostic;
  spvDiagnosticDestroy(diagnostic);
}

}  // anonymous namespace
