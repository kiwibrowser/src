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

#ifndef LIBSHADERC_UTIL_INC_COMPILER_H
#define LIBSHADERC_UTIL_INC_COMPILER_H

#include <functional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>

#include "glslang/Public/ShaderLang.h"

#include "counting_includer.h"
#include "file_finder.h"
#include "mutex.h"
#include "string_piece.h"

namespace shaderc_util {

// Initializes glslang on creation, and destroys it on completion.
// This object is expected to be a singleton, so that internal
// glslang state can be correctly handled.
// TODO(awoloszyn): Once glslang no longer has static global mutable state
//                  remove this class.
class GlslInitializer {
 public:
  GlslInitializer() : last_messages_(EShMsgDefault) {
    glslang::InitializeProcess();
  }

  ~GlslInitializer() { glslang::FinalizeProcess(); }

  // Calls release on GlslangInitializer used to intialize this object
  // when it is destroyed.
  class InitializationToken {
   public:
    ~InitializationToken() {
      if (initializer_) {
        initializer_->Release();
      }
    }

    InitializationToken(InitializationToken&& other)
        : initializer_(other.initializer_) {
      other.initializer_ = nullptr;
    }

    InitializationToken(const InitializationToken&) = delete;

   private:
    InitializationToken(GlslInitializer* initializer)
        : initializer_(initializer) {}

    friend class GlslInitializer;
    GlslInitializer* initializer_;
  };

  // Obtains exclusive access to the glslang state. The state remains
  // exclusive until the Initialization Token has been destroyed.
  // Re-initializes glsl state iff the previous messages and the current
  // messages are incompatible.
  InitializationToken Acquire(EShMessages new_messages) {
    state_lock_.lock();

    if ((last_messages_ ^ new_messages) &
        (EShMsgVulkanRules | EShMsgSpvRules)) {
      glslang::FinalizeProcess();
      glslang::InitializeProcess();
    }
    last_messages_ = new_messages;
    return InitializationToken(this);
  }

 private:
  void Release() { state_lock_.unlock(); }

  friend class InitializationToken;

  EShMessages last_messages_;
  mutex state_lock_;
};

// Maps macro names to their definitions.  Stores string_pieces, so the
// underlying strings must outlive it.
using MacroDictionary = std::unordered_map<std::string, std::string>;

// Holds all of the state required to compile source GLSL into SPIR-V.
class Compiler {
 public:
  Compiler()
      // The default version for glsl is 110, or 100 if you are using an es
      // profile. But we want to default to a non-es profile.
      : default_version_(110),
        default_profile_(ENoProfile),
        force_version_profile_(false),
        warnings_as_errors_(false),
        suppress_warnings_(false),
        generate_debug_info_(false),
        message_rules_(GetDefaultRules()) {}


  // Requests that the compiler place debug information into the object code,
  // such as identifier names and line numbers.
  void SetGenerateDebugInfo();

  // When a warning is encountered it treat it as an error.
  void SetWarningsAsErrors();

  // Any warning message generated is suppressed before it is output.
  void SetSuppressWarnings();

  // Adds an implicit macro definition obeyed by subsequent CompileShader()
  // calls. The macro and definition should be passed in with their char*
  // pointer and their lengths. They can be modified or deleted after this
  // function has returned.
  void AddMacroDefinition(const char* macro, size_t macro_length,
                          const char* definition, size_t definition_length);

  // Sets message rules to be used when generating compiler warnings/errors
  void SetMessageRules(EShMessages rules);

  // Gets the message rules when generating compiler warnings/error.
  EShMessages GetMessageRules() const;

  // Forces (without any verification) the default version and profile for
  // subsequent CompileShader() calls.
  void SetForcedVersionProfile(int version, EProfile profile);

  enum class OutputType {
    SpirvBinary,  // A binary module, as defined by the SPIR-V specification.
    SpirvAssemblyText,  // Assembly syntax defined by the SPIRV-Tools project.
    PreprocessedText,   // Preprocessed source code.
  };
  // Compiles the shader source in the input_source_string parameter.
  //
  // If the forced_shader stage parameter is not EShLangCount then
  // the shader is assumed to be of the given stage.
  //
  // The stage_callback function will be called if a shader_stage has
  // not been forced and the stage can not be determined
  // from the shader text. Any #include directives are parsed with the given
  // includer.
  //
  // The initializer parameter must be a valid GlslInitializer object.
  // Acquire will be called on the initializer and the result will be
  // destoryed before the function ends.
  //
  // The output_type parameter determines what kind of output should be
  // produced.
  //
  // Any error messages are written as if the file name were error_tag.
  // Any errors are written to the error_stream parameter.
  // total_warnings and total_errors are incremented once for every
  // warning or error encountered respectively.
  //
  // Returns a tuple consisting of three fields. 1) a boolean which is true when
  // the compilation succeeded, and false otherwise; 2) a vector of 32-bit words
  // which contains the compilation output data, either compiled SPIR-V binary
  // code, or the text string generated in preprocessing-only or disassembly
  // mode; 3) the size of the output data in bytes. When the output is SPIR-V
  // binary code, the size is the number of bytes of valid data in the vector.
  // If the output is a text string, the size equals the length of that string.
  std::tuple<bool, std::vector<uint32_t>, size_t> Compile(
      const shaderc_util::string_piece& input_source_string,
      EShLanguage forced_shader_stage, const std::string& error_tag,
      const std::function<EShLanguage(
          std::ostream* error_stream,
          const shaderc_util::string_piece& error_tag)>& stage_callback,
      CountingIncluder& includer, OutputType output_type,
      std::ostream* error_stream, size_t* total_warnings, size_t* total_errors,
      GlslInitializer* initializer) const;

  static EShMessages GetDefaultRules() {
    return static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
  }

 protected:
  // Preprocesses a shader whose filename is filename and content is
  // shader_source. If preprocessing is successful, returns true, the
  // preprocessed shader, and any warning message as a tuple. Otherwise,
  // returns false, an empty string, and error messages as a tuple.
  //
  // The error_tag parameter is the name to use for outputting errors.
  // The shader_source parameter is the input shader's source text.
  // The shader_preamble parameter is a context-specific preamble internally
  // prepended to shader_text without affecting the validity of its #version
  // position.
  //
  // Any #include directives are processed with the given includer.
  //
  // If force_version_profile_ is set, the shader's version/profile is forced
  // to be default_version_/default_profile_ regardless of the #version
  // directive in the source code.
  std::tuple<bool, std::string, std::string> PreprocessShader(
      const std::string& error_tag,
      const shaderc_util::string_piece& shader_source,
      const shaderc_util::string_piece& shader_preamble,
      CountingIncluder& includer) const;

  // Cleans up the preamble in a given preprocessed shader.
  //
  // The error_tag parameter is the name to be given for the main file.
  // The pound_extension parameter is the #extension directive we prepended to
  // the original shader source code via preamble.
  // The num_include_directives parameter is the number of #include directives
  // appearing in the original shader source code.
  // The is_for_next_line means whether the #line sets the line number for the
  // next line.
  //
  // If no #include directive is used in the shader source code, we can safely
  // delete the #extension directive we injected via preamble. Otherwise, we
  // need to adjust it if there exists a #version directive in the original
  // shader source code.
  std::string CleanupPreamble(
      const shaderc_util::string_piece& preprocessed_shader,
      const shaderc_util::string_piece& error_tag,
      const shaderc_util::string_piece& pound_extension,
      int num_include_directives, bool is_for_next_line) const;

  // Determines version and profile from command line, or the source code.
  // Returns the decoded version and profile pair on success. Otherwise,
  // returns (0, ENoProfile).
  std::pair<int, EProfile> DeduceVersionProfile(
      const std::string& preprocessed_shader) const;

  // Determines the shader stage from pragmas embedded in the source text if
  // possible. In the returned pair, the glslang EShLanguage is the shader
  // stage deduced. If no #pragma directives for shader stage exist, it's
  // EShLangCount.  If errors occur, the second element in the pair is the
  // error message.  Otherwise, it's an empty string.
  std::pair<EShLanguage, std::string> GetShaderStageFromSourceCode(
      shaderc_util::string_piece filename,
      const std::string& preprocessed_shader) const;

  // Determines version and profile from command line, or the source code.
  // Returns the decoded version and profile pair on success. Otherwise,
  // returns (0, ENoProfile).
  std::pair<int, EProfile> DeduceVersionProfile(
      const std::string& preprocessed_shader);

  // Gets version and profile specification from the given preprocessedshader.
  // Returns the decoded version and profile pair on success. Otherwise,
  // returns (0, ENoProfile).
  std::pair<int, EProfile> GetVersionProfileFromSourceCode(
      const std::string& preprocessed_shader) const;

  // Version to use when force_version_profile_ is true.
  int default_version_;
  // Profile to use when force_version_profile_ is true.
  EProfile default_profile_;
  // When true, use the default version and profile from eponymous data members.
  bool force_version_profile_;


  // Macro definitions that must be available to reference in the shader source.
  MacroDictionary predefined_macros_;

  // When true, treat warnings as errors.
  bool warnings_as_errors_;
  // Supress warnings when true.
  bool suppress_warnings_;

  // When true, compilation will generate debug info with the binary SPIR-V
  // output.
  bool generate_debug_info_;

  // Sets the glslang EshMessages bitmask for determining which dialect of GLSL
  // and which SPIR-V codegen semantics are used. This impacts the warning &
  // error
  // messages as well as the set of available builtins
  EShMessages message_rules_;
};

// Converts a string to a vector of uint32_t by copying the content of a given
// string to the vector and returns it. Appends '\0' at the end if extra bytes
// are required to complete the last element.
std::vector<uint32_t> ConvertStringToVector(const std::string& str);

}  // namespace shaderc_util
#endif  // LIBSHADERC_UTIL_INC_COMPILER_H
