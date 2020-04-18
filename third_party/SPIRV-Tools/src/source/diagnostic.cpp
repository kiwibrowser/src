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

#include "diagnostic.h"

#include <assert.h>
#include <string.h>

#include <iostream>

#include "spirv-tools/libspirv.h"

// Diagnostic API

spv_diagnostic spvDiagnosticCreate(const spv_position position,
                                   const char* message) {
  spv_diagnostic diagnostic = new spv_diagnostic_t;
  if (!diagnostic) return nullptr;
  size_t length = strlen(message) + 1;
  diagnostic->error = new char[length];
  if (!diagnostic->error) {
    delete diagnostic;
    return nullptr;
  }
  diagnostic->position = *position;
  diagnostic->isTextSource = false;
  memset(diagnostic->error, 0, length);
  strncpy(diagnostic->error, message, length);
  return diagnostic;
}

void spvDiagnosticDestroy(spv_diagnostic diagnostic) {
  if (!diagnostic) return;
  delete[] diagnostic->error;
  delete diagnostic;
}

spv_result_t spvDiagnosticPrint(const spv_diagnostic diagnostic) {
  if (!diagnostic) return SPV_ERROR_INVALID_DIAGNOSTIC;

  if (diagnostic->isTextSource) {
    // NOTE: This is a text position
    // NOTE: add 1 to the line as editors start at line 1, we are counting new
    // line characters to start at line 0
    std::cerr << "error: " << diagnostic->position.line + 1 << ": "
              << diagnostic->position.column + 1 << ": " << diagnostic->error
              << "\n";
    return SPV_SUCCESS;
  } else {
    // NOTE: Assume this is a binary position
    std::cerr << "error: " << diagnostic->position.index << ": "
              << diagnostic->error << "\n";
    return SPV_SUCCESS;
  }

  return SPV_ERROR_INVALID_VALUE;
}

namespace libspirv {

DiagnosticStream::~DiagnosticStream() {
  if (pDiagnostic_ && error_ != SPV_FAILED_MATCH) {
    *pDiagnostic_ = spvDiagnosticCreate(&position_, stream_.str().c_str());
  }
}
std::string
spvResultToString(spv_result_t res) {
  std::string out;
  switch (res) {
    case SPV_SUCCESS:
      out = "SPV_SUCCESS";
      break;
    case SPV_UNSUPPORTED:
      out = "SPV_UNSUPPORTED";
      break;
    case SPV_END_OF_STREAM:
      out = "SPV_END_OF_STREAM";
      break;
    case SPV_WARNING:
      out = "SPV_WARNING";
      break;
    case SPV_FAILED_MATCH:
      out = "SPV_FAILED_MATCH";
      break;
    case SPV_REQUESTED_TERMINATION:
      out = "SPV_REQUESTED_TERMINATION";
      break;
    case SPV_ERROR_INTERNAL:
      out = "SPV_ERROR_INTERNAL";
      break;
    case SPV_ERROR_OUT_OF_MEMORY:
      out = "SPV_ERROR_OUT_OF_MEMORY";
      break;
    case SPV_ERROR_INVALID_POINTER:
      out = "SPV_ERROR_INVALID_POINTER";
      break;
    case SPV_ERROR_INVALID_BINARY:
      out = "SPV_ERROR_INVALID_BINARY";
      break;
    case SPV_ERROR_INVALID_TEXT:
      out = "SPV_ERROR_INVALID_TEXT";
      break;
    case SPV_ERROR_INVALID_TABLE:
      out = "SPV_ERROR_INVALID_TABLE";
      break;
    case SPV_ERROR_INVALID_VALUE:
      out = "SPV_ERROR_INVALID_VALUE";
      break;
    case SPV_ERROR_INVALID_DIAGNOSTIC:
      out = "SPV_ERROR_INVALID_DIAGNOSTIC";
      break;
    case SPV_ERROR_INVALID_LOOKUP:
      out = "SPV_ERROR_INVALID_LOOKUP";
      break;
    case SPV_ERROR_INVALID_ID:
      out = "SPV_ERROR_INVALID_ID";
      break;
    case SPV_ERROR_INVALID_CFG:
      out = "SPV_ERROR_INVALID_CFG";
      break;
    case SPV_ERROR_INVALID_LAYOUT:
      out = "SPV_ERROR_INVALID_LAYOUT";
      break;
    default:
      out = "Unknown Error";
  }
  return out;
}

}  // namespace libspirv
