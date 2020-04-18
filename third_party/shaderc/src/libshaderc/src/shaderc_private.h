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

#ifndef LIBSHADERC_SRC_SHADERC_PRIVATE_H_
#define LIBSHADERC_SRC_SHADERC_PRIVATE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "shaderc/shaderc.h"

#include "spirv-tools/libspirv.h"

// Described in shaderc.h.
struct shaderc_compilation_result {
  virtual ~shaderc_compilation_result() {}

  // Returns the data from this compilation as a sequence of bytes.
  virtual const char* GetBytes() const = 0;

  // The size of the output data in term of bytes.
  size_t output_data_size = 0;
  // Compilation messages.
  std::string messages;
  // Number of errors.
  size_t num_errors = 0;
  // Number of warnings.
  size_t num_warnings = 0;
  // Compilation status.
  shaderc_compilation_status compilation_status =
      shaderc_compilation_status_null_result_object;
};

// Compilation result class using a vector for holding the compilation
// output data.
class shaderc_compilation_result_vector : public shaderc_compilation_result {
 public:
  ~shaderc_compilation_result_vector() = default;

  void SetOutputData(std::vector<uint32_t>&& data) {
    output_data_ = std::move(data);
  }

  const char* GetBytes() const override {
    return reinterpret_cast<const char*>(output_data_.data());
  }

 private:
  // Compilation output data. In normal compilation mode, it contains the
  // compiled SPIR-V binary code. In disassembly and preprocessing-only mode, it
  // contains a null-terminated string which is the text output. For text
  // output, extra bytes with value 0x00 might be appended to complete the last
  // uint32_t element.
  std::vector<uint32_t> output_data_;
};

// Compilation result class using a spv_binary for holding the compilation
// output data.
class shaderc_compilation_result_spv_binary
    : public shaderc_compilation_result {
 public:
  ~shaderc_compilation_result_spv_binary() { spvBinaryDestroy(output_data_); }

  void SetOutputData(spv_binary data) { output_data_ = data; }

  const char* GetBytes() const override {
    return reinterpret_cast<const char*>(output_data_->code);
  }

 private:
  spv_binary output_data_ = nullptr;
};

namespace shaderc_util {
class GlslInitializer;
}

struct shaderc_compiler {
  shaderc_util::GlslInitializer* initializer;
};

#endif  // LIBSHADERC_SRC_SHADERC_PRIVATE_H_
