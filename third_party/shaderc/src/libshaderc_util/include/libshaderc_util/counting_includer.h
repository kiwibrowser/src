// Copyright 2015 The Shaderc Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef LIBSHADERC_UTIL_COUNTING_INCLUDER_H
#define LIBSHADERC_UTIL_COUNTING_INCLUDER_H

#include <atomic>

#include "glslang/Public/ShaderLang.h"

namespace shaderc_util {

// An Includer that counts how many #include directives it saw.
class CountingIncluder : public glslang::TShader::Includer {
 public:
  // Done as .store(0) instead of in the initializer list for the following
  // reasons:
  // Clang > 3.6 will complain about it if it is written as ({0}).
  // VS2013 fails if it is written as {0}.
  // G++-4.8 does not correctly support std::atomic_init.
  CountingIncluder() {
    num_include_directives_.store(0);
  }

  // Resolves an include request for a source by name, type, and name of the
  // requesting source.  For the semantics of the result, see the base class.
  // Also increments num_include_directives and returns the results of
  // include_delegate(filename).  Subclasses should override include_delegate()
  // instead of this method.
  glslang::TShader::Includer::IncludeResult* include(
      const char* requested_source, glslang::TShader::Includer::IncludeType type,
      const char* requesting_source,
      size_t include_depth) final {
    ++num_include_directives_;
    return include_delegate(requested_source, type, requesting_source,
                            include_depth);
  }

  // Releases the given IncludeResult.
  void releaseInclude(glslang::TShader::Includer::IncludeResult* result) final {
    release_delegate(result);
  }

  int num_include_directives() const { return num_include_directives_.load(); }

 private:
  // Invoked by this class to provide results to
  // glslang::TShader::Includer::include.
  virtual glslang::TShader::Includer::IncludeResult* include_delegate(
      const char* requested_source, glslang::TShader::Includer::IncludeType type,
      const char* requesting_source, size_t include_depth) = 0;

  // Release the given IncludeResult.
  virtual void release_delegate(
      glslang::TShader::Includer::IncludeResult* result) = 0;

  // The number of #include directive encountered.
  std::atomic_int num_include_directives_;
};
}

#endif  // LIBSHADERC_UTIL_COUNTING_INCLUDER_H
