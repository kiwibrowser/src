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

#ifndef LIBSPIRV_PRINT_H_
#define LIBSPIRV_PRINT_H_

#include <iostream>
#include <sstream>

namespace libspirv {

// Wrapper for out stream selection.
class out_stream {
 public:
  out_stream() : pStream(nullptr) {}
  explicit out_stream(std::stringstream& stream) : pStream(&stream) {}

  std::ostream& get() {
    if (pStream) {
      return *pStream;
    }
    return std::cout;
  }

 private:
  std::stringstream* pStream;
};

namespace clr {
// Resets console color.
struct reset {
  operator const char*();
};
// Sets console color to grey.
struct grey {
  operator const char*();
};
// Sets console color to red.
struct red {
  operator const char*();
};
// Sets console color to green.
struct green {
  operator const char*();
};
// Sets console color to yellow.
struct yellow {
  operator const char*();
};
// Sets console color to blue.
struct blue {
  operator const char*();
};
}  // namespace clr

}  // namespace libspirv

#endif  // LIBSPIRV_PRINT_H_
