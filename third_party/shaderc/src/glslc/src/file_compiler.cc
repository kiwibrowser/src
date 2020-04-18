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

#include "file_compiler.h"

#include <fstream>
#include <iostream>

#include "file.h"
#include "file_includer.h"
#include "shader_stage.h"

#include "libshaderc_util/io.h"
#include "libshaderc_util/message.h"

namespace {
using shaderc_util::string_piece;

}  // anonymous namespace

namespace glslc {
bool FileCompiler::CompileShaderFile(const std::string& input_file,
                                     shaderc_shader_kind shader_stage) {
  std::vector<char> input_data;
  std::string path = input_file;
  if (!shaderc_util::ReadFile(path, &input_data)) {
    return false;
  }

  std::string output_name = GetOutputFileName(input_file);

  std::ofstream potential_file_stream;
  std::ostream* output_stream =
      shaderc_util::GetOutputStream(output_name, &potential_file_stream);
  if (!output_stream) {
    // An error message has already been emitted to the stderr stream.
    return false;
  }
  string_piece error_file_name = input_file;

  if (error_file_name == "-") {
    // If the input file was stdin, we want to output errors as <stdin>.
    error_file_name = "<stdin>";
  }

  string_piece source_string = "";
  if (!input_data.empty()) {
    source_string = {&input_data.front(),
                     &input_data.front() + input_data.size()};
  }

  std::unique_ptr<FileIncluder> includer(
      new FileIncluder(&include_file_finder_));
  // Get a reference to the dependency trace before we pass the ownership to
  // shaderc::CompileOptions.
  const auto& used_source_files = includer->file_path_trace();
  options_.SetIncluder(std::move(includer));

  if (shader_stage == shaderc_spirv_assembly) {
    // Only act if the requested target is SPIR-V binary.
    if (output_type_ == OutputType::SpirvBinary) {
      const auto result =
          compiler_.AssembleToSpv(source_string.data(), source_string.size());
      return EmitCompiledResult(result, input_file, error_file_name,
                                used_source_files, output_stream);
    } else {
      return true;
    }
  }

  switch (output_type_) {
    case OutputType::SpirvBinary: {
      const auto result = compiler_.CompileGlslToSpv(
          source_string.data(), source_string.size(), shader_stage,
          error_file_name.data(), options_);
      return EmitCompiledResult(result, input_file, error_file_name,
                                used_source_files, output_stream);
    }
    case OutputType::SpirvAssemblyText: {
      const auto result = compiler_.CompileGlslToSpvAssembly(
          source_string.data(), source_string.size(), shader_stage,
          error_file_name.data(), options_);
      return EmitCompiledResult(result, input_file, error_file_name,
                                used_source_files, output_stream);
    }
    case OutputType::PreprocessedText: {
      const auto result = compiler_.PreprocessGlsl(
          source_string.data(), source_string.size(), shader_stage,
          error_file_name.data(), options_);
      return EmitCompiledResult(result, input_file, error_file_name,
                                used_source_files, output_stream);
    }
  }
  return false;
}

template <typename CompilationResultType>
bool FileCompiler::EmitCompiledResult(
    const CompilationResultType& result, const std::string& input_file,
    string_piece error_file_name,
    const std::unordered_set<std::string>& used_source_files,
    std::ostream* out) {
  total_errors_ += result.GetNumErrors();
  total_warnings_ += result.GetNumWarnings();

  bool compilation_success =
      result.GetCompilationStatus() == shaderc_compilation_status_success;

  // Handle the error message for failing to deduce the shader kind.
  if (result.GetCompilationStatus() ==
      shaderc_compilation_status_invalid_stage) {
    if (IsGlslFile(error_file_name)) {
      std::cerr << "glslc: error: "
                << "'" << error_file_name << "': "
                << ".glsl file encountered but no -fshader-stage "
                   "specified ahead";
    } else if (error_file_name == "<stdin>") {
      std::cerr
          << "glslc: error: '-': -fshader-stage required when input is from "
             "standard "
             "input \"-\"";
    } else {
      std::cerr << "glslc: error: "
                << "'" << error_file_name << "': "
                << "file not recognized: File format not recognized";
    }
    std::cerr << "\n";

    return false;
  }

  // Get a string_piece which refers to the normal compilation output for now.
  // This string_piece might be redirected to the dependency info to be dumped
  // later, if the handler is instantiated to dump as normal compilation output,
  // and the original compilation output should be blocked. Otherwise it won't
  // be touched. The main output stream dumps this string_piece later.
  string_piece compilation_output(
      reinterpret_cast<const char*>(result.cbegin()),
      reinterpret_cast<const char*>(result.cend()));

  // If we have dependency info dumping handler instantiated, we should dump
  // dependency info first. This may redirect the compilation output
  // string_piece to dependency info.
  std::string potential_dependency_info_output;
  if (dependency_info_dumping_handler_) {
    if (!dependency_info_dumping_handler_->DumpDependencyInfo(
            GetCandidateOutputFileName(input_file), error_file_name.data(),
            &potential_dependency_info_output, used_source_files)) {
      return false;
    }
    if (!potential_dependency_info_output.empty()) {
      // If the potential_dependency_info_output string is not empty, it means
      // we should dump dependency info as normal compilation output. Redirect
      // the compilation output string_piece to the dependency info stored in
      // potential_dependency_info_output to make it happen.
      compilation_output = potential_dependency_info_output;
    }
  }

  // Write compilation output to output file.
  out->write(compilation_output.data(), compilation_output.size());
  // Write error message to std::cerr.
  std::cerr << result.GetErrorMessage();
  if (out->fail()) {
    // Something wrong happened on output.
    if (out == &std::cout) {
      std::cerr << "glslc: error: error writing to standard output"
                << std::endl;
    } else {
      std::cerr << "glslc: error: error writing to output file: '"
                << output_file_name_ << "'" << std::endl;
    }
    return false;
  }

  return compilation_success;
}

void FileCompiler::AddIncludeDirectory(const std::string& path) {
  include_file_finder_.search_path().push_back(path);
}

void FileCompiler::SetIndividualCompilationFlag() {
  if (output_type_ != OutputType::SpirvAssemblyText) {
    needs_linking_ = false;
    file_extension_ = ".spv";
  }
}

void FileCompiler::SetDisassemblyFlag() {
  if (!PreprocessingOnly()) {
    output_type_ = OutputType::SpirvAssemblyText;
    needs_linking_ = false;
    file_extension_ = ".spvasm";
  }
}

void FileCompiler::SetPreprocessingOnlyFlag() {
  output_type_ = OutputType::PreprocessedText;
  needs_linking_ = false;
  if (output_file_name_.empty()) {
    output_file_name_ = "-";
  }
}

bool FileCompiler::ValidateOptions(size_t num_files) {
  if (num_files == 0) {
    std::cerr << "glslc: error: no input files" << std::endl;
    return false;
  }

  if (num_files > 1 && needs_linking_) {
    std::cerr << "glslc: error: linking multiple files is not supported yet. "
                 "Use -c to compile files individually."
              << std::endl;
    return false;
  }

  // If we are outputting many object files, we cannot specify -o. Also
  // if we are preprocessing multiple files they must be to stdout.
  if (num_files > 1 && ((!PreprocessingOnly() && !needs_linking_ &&
                         !output_file_name_.empty()) ||
                        (PreprocessingOnly() && output_file_name_ != "-"))) {
    std::cerr << "glslc: error: cannot specify -o when generating multiple"
                 " output files"
              << std::endl;
    return false;
  }

  // If we have dependency info dumping handler instantiated, we should check
  // its validity.
  if (dependency_info_dumping_handler_) {
    std::string dependency_info_dumping_hander_error_msg;
    if (!dependency_info_dumping_handler_->IsValid(
            &dependency_info_dumping_hander_error_msg, num_files)) {
      std::cerr << "glslc: error: " << dependency_info_dumping_hander_error_msg
                << std::endl;
      return false;
    }
  }

  return true;
}

void FileCompiler::OutputMessages() {
  shaderc_util::OutputMessages(&std::cerr, total_warnings_, total_errors_);
}

std::string FileCompiler::GetOutputFileName(std::string input_filename) {
  if (output_file_name_.empty()) {
    return needs_linking_ ? std::string("a.spv")
                          : GetCandidateOutputFileName(input_filename);
  } else {
    return output_file_name_.str();
  }
}

std::string FileCompiler::GetCandidateOutputFileName(
    std::string input_filename) {
  if (!output_file_name_.empty() && !PreprocessingOnly()) {
    return output_file_name_.str();
  }

  std::string extension = file_extension_;
  if (PreprocessingOnly() || needs_linking_) {
    extension = ".spv";
  }

  std::string candidate_output_file_name =
      IsStageFile(input_filename)
          ? shaderc_util::GetBaseFileName(input_filename) + extension
          : shaderc_util::GetBaseFileName(
                input_filename.substr(0, input_filename.find_last_of('.')) +
                extension);
  return candidate_output_file_name;
}
}  // namesapce glslc
