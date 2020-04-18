// Copyright 2016 The Shaderc Authors. All rights reserved.
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

// The program demonstrates basic shader compilation using the Shaderc C++ API.
// For clarity, each method is deliberately self-contained.
//
// Techniques demonstrated:
//  - Preprocessing GLSL source text
//  - Compiling a shader to SPIR-V assembly text
//  - Compliing a shader to a SPIR-V binary module
//  - Setting basic options: setting a preprocessor symbol.
//  - Checking compilation status and extracting an error message.

#include <iostream>
#include <string>
#include <vector>

#include <shaderc/shaderc.hpp>

// Returns GLSL shader source text after preprocessing.
std::string preprocess_shader(const std::string& source_name,
                              shaderc_shader_kind kind,
                              const std::string& source) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;

  // Like -DMY_DEFINE=1
  options.AddMacroDefinition("MY_DEFINE", "1");

  shaderc::PreprocessedSourceCompilationResult result = compiler.PreprocessGlsl(
      source.c_str(), source.size(), kind, source_name.c_str(), options);

  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    std::cerr << result.GetErrorMessage();
    return "";
  }

  return std::string(result.cbegin(), result.cend());
}

// Compiles a shader to SPIR-V assembly. Returns the assembly text
// as a string.
std::string compile_file_to_assembly(const std::string& source_name,
                                     shaderc_shader_kind kind,
                                     const std::string& source) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;

  // Like -DMY_DEFINE=1
  options.AddMacroDefinition("MY_DEFINE", "1");

  shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(
      source.c_str(), source.size(), kind, source_name.c_str(), options);

  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    std::cerr << result.GetErrorMessage();
    return "";
  }

  return std::string(result.cbegin(), result.cend());
}

// Compiles a shader to a SPIR-V binary. Returns the binary as
// a vector of 32-bit words.
std::vector<uint32_t> compile_file(const std::string& source_name,
                                   shaderc_shader_kind kind,
                                   const std::string& source) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;

  // Like -DMY_DEFINE=1
  options.AddMacroDefinition("MY_DEFINE", "1");

  shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
      source.c_str(), source.size(), kind, source_name.c_str(), options);

  if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
    std::cerr << module.GetErrorMessage();
    return std::vector<uint32_t>();
  }

  std::vector<uint32_t> result(module.cbegin(), module.cend());
  return result;
}

int main() {
  const char kShaderSource[] =
      "#version 310 es\nvoid main() {int x = MY_DEFINE; }\n";

  auto preprocessed = preprocess_shader(
      "shader_src", shaderc_glsl_vertex_shader, kShaderSource);
  std::cout << "Compiled a vertex shader resulting in preprocessed text:"
            << std::endl
            << preprocessed << std::endl;

  auto assembly = compile_file_to_assembly(
      "shader_src", shaderc_glsl_vertex_shader, kShaderSource);
  std::cout << "SPIR-V assembly:" << std::endl << assembly << std::endl;

  auto spirv =
      compile_file("shader_src", shaderc_glsl_vertex_shader, kShaderSource);
  std::cout << "Compiled to a binary module with " << spirv.size() << " words."
            << std::endl;

  const char kBadShaderSource[] =
      "#version 310 es\nint main() { int main_should_be_void; }\n";

  std::cout << std::endl << "Compiling a bad shader:" << std::endl;
  compile_file("bad_src", shaderc_glsl_vertex_shader, kBadShaderSource);

  return 0;
}
