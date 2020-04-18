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

#include "shaderc_private.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <sstream>
#include <vector>

#include "SPIRV/spirv.hpp"

#include "libshaderc_util/compiler.h"
#include "libshaderc_util/counting_includer.h"
#include "libshaderc_util/resources.h"
#include "libshaderc_util/spirv_tools_wrapper.h"
#include "libshaderc_util/version_profile.h"

#if (defined(_MSC_VER) && !defined(_CPPUNWIND)) || !defined(__EXCEPTIONS)
#define TRY_IF_EXCEPTIONS_ENABLED
#define CATCH_IF_EXCEPTIONS_ENABLED(X) if (0)
#else
#define TRY_IF_EXCEPTIONS_ENABLED try
#define CATCH_IF_EXCEPTIONS_ENABLED(X) catch (X)
#endif

namespace {

// Returns shader stage (ie: vertex, fragment, etc.) in response to forced
// shader kinds. If the shader kind is not a forced kind, returns EshLangCount
// to let #pragma annotation or shader stage deducer determine the stage to
// use.
EShLanguage GetForcedStage(shaderc_shader_kind kind) {
  switch (kind) {
    case shaderc_glsl_vertex_shader:
      return EShLangVertex;
    case shaderc_glsl_fragment_shader:
      return EShLangFragment;
    case shaderc_glsl_compute_shader:
      return EShLangCompute;
    case shaderc_glsl_geometry_shader:
      return EShLangGeometry;
    case shaderc_glsl_tess_control_shader:
      return EShLangTessControl;
    case shaderc_glsl_tess_evaluation_shader:
      return EShLangTessEvaluation;
    case shaderc_glsl_infer_from_source:
    case shaderc_glsl_default_vertex_shader:
    case shaderc_glsl_default_fragment_shader:
    case shaderc_glsl_default_compute_shader:
    case shaderc_glsl_default_geometry_shader:
    case shaderc_glsl_default_tess_control_shader:
    case shaderc_glsl_default_tess_evaluation_shader:
    case shaderc_spirv_assembly:
      return EShLangCount;
  }
  assert(0 && "Unhandled shaderc_shader_kind");
  return EShLangCount;
}

// Converts shaderc_target_env to EShMessages
EShMessages GetMessageRules(shaderc_target_env target) {
  EShMessages msgs = EShMsgDefault;

  switch (target) {
    case shaderc_target_env_opengl_compat:
      break;
    case shaderc_target_env_opengl:
      msgs = EShMsgSpvRules;
      break;
    case shaderc_target_env_vulkan:
      msgs = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
      break;
  }

  return msgs;
}

// A wrapper functor class to be used as stage deducer for libshaderc_util
// Compile() interface. When the given shader kind is one of the default shader
// kinds, this functor will be called if #pragma is not found in the source
// code. And it returns the corresponding shader stage. When the shader kind is
// a forced shader kind, this functor won't be called and it simply returns
// EShLangCount to make the syntax correct. When the shader kind is set to
// shaderc_glsl_deduce_from_pragma, this functor also returns EShLangCount, but
// the compiler should emit error if #pragma annotation is not found in this
// case.
class StageDeducer {
 public:
  explicit StageDeducer(
      shaderc_shader_kind kind = shaderc_glsl_infer_from_source)
      : kind_(kind), error_(false){};
  // The method that underlying glslang will call to determine the shader stage
  // to be used in current compilation. It is called only when there is neither
  // forced shader kind (or say stage, in the view of glslang), nor #pragma
  // annotation in the source code. This method transforms an user defined
  // 'default' shader kind to the corresponding shader stage. As this is the
  // last trial to determine the shader stage, failing to find the corresponding
  // shader stage will record an error.
  // Note that calling this method more than once during one compilation will
  // have the error recorded for the previous call been overwriten by the next
  // call.
  EShLanguage operator()(std::ostream* /*error_stream*/,
                         const shaderc_util::string_piece& /*error_tag*/) {
    EShLanguage stage = GetDefaultStage(kind_);
    if (stage == EShLangCount) {
      error_ = true;
    } else {
      error_ = false;
    }
    return stage;
  };

  // Returns true if there is error during shader stage deduction.
  bool error() const { return error_; }

 private:
  // Gets the corresponding shader stage for a given 'default' shader kind. All
  // other kinds are mapped to EShLangCount which should not be used.
  EShLanguage GetDefaultStage(shaderc_shader_kind kind) const {
    switch (kind) {
      case shaderc_glsl_vertex_shader:
      case shaderc_glsl_fragment_shader:
      case shaderc_glsl_compute_shader:
      case shaderc_glsl_geometry_shader:
      case shaderc_glsl_tess_control_shader:
      case shaderc_glsl_tess_evaluation_shader:
      case shaderc_glsl_infer_from_source:
        return EShLangCount;
      case shaderc_glsl_default_vertex_shader:
        return EShLangVertex;
      case shaderc_glsl_default_fragment_shader:
        return EShLangFragment;
      case shaderc_glsl_default_compute_shader:
        return EShLangCompute;
      case shaderc_glsl_default_geometry_shader:
        return EShLangGeometry;
      case shaderc_glsl_default_tess_control_shader:
        return EShLangTessControl;
      case shaderc_glsl_default_tess_evaluation_shader:
        return EShLangTessEvaluation;
    case shaderc_spirv_assembly:
        return EShLangCount;
    }
    assert(0 && "Unhandled shaderc_shader_kind");
    return EShLangCount;
  }

  shaderc_shader_kind kind_;
  bool error_;
};

// A bridge between the libshaderc includer and libshaderc_util includer.
class InternalFileIncluder : public shaderc_util::CountingIncluder {
 public:
  InternalFileIncluder(const shaderc_include_resolve_fn resolver,
                       const shaderc_include_result_release_fn result_releaser,
                       void* user_data)
      : resolver_(resolver),
        result_releaser_(result_releaser),
        user_data_(user_data){};
  InternalFileIncluder()
      : resolver_(nullptr), result_releaser_(nullptr), user_data_(nullptr){};

 private:
  // Check the validity of the callbacks.
  bool AreValidCallbacks() const {
    return resolver_ != nullptr && result_releaser_ != nullptr;
  }

  // Maps a shaderc_include_type to the correpsonding Glslang include type.
  shaderc_include_type GetIncludeType(
      glslang::TShader::Includer::IncludeType type) {
    switch (type) {
      case glslang::TShader::Includer::EIncludeRelative:
        return shaderc_include_type_relative;
      case glslang::TShader::Includer::EIncludeStandard:
        return shaderc_include_type_standard;
      default:
        break;
    }
    assert(0 && "Unhandled shaderc_include_type");
    return shaderc_include_type_relative;
  }

  // Resolves an include request for the requested source of the given
  // type in the context of the specified requesting source.  On success,
  // returns a newly allocated IncludeResponse containing the fully resolved
  // name of the requested source and the contents of that source.
  // On failure, returns a newly allocated IncludeResponse where the
  // resolved name member is an empty string, and the contents members
  // contains error details.
  virtual glslang::TShader::Includer::IncludeResult* include_delegate(
      const char* requested_source,
      glslang::TShader::Includer::IncludeType type,
      const char* requesting_source, size_t include_depth) override {
    if (!AreValidCallbacks()) {
      const char kUnexpectedIncludeError[] =
          "#error unexpected include directive";
      return new glslang::TShader::Includer::IncludeResult{
          "", kUnexpectedIncludeError, strlen(kUnexpectedIncludeError),
          nullptr};
    }
    shaderc_include_result* include_result =
        resolver_(user_data_, requested_source, GetIncludeType(type),
                  requesting_source, include_depth);
    // Make a glslang IncludeResult from a shaderc_include_result.  The
    // user_data member of the IncludeResult is a pointer to the
    // shaderc_include_result object, so we can later release the latter.
    return new glslang::TShader::Includer::IncludeResult{
        std::string(include_result->source_name,
                    include_result->source_name_length),
        include_result->content, include_result->content_length,
        include_result};
  }

  // Releases the given IncludeResult.
  virtual void release_delegate(
      glslang::TShader::Includer::IncludeResult* result) override {
    if (result && result_releaser_) {
      result_releaser_(user_data_,
                       static_cast<shaderc_include_result*>(result->user_data));
    }
    delete result;
  }

  const shaderc_include_resolve_fn resolver_;
  const shaderc_include_result_release_fn result_releaser_;
  void* user_data_;
};

}  // anonymous namespace

struct shaderc_compile_options {
  shaderc_compile_options() : include_user_data(nullptr) {}
  shaderc_util::Compiler compiler;
  shaderc_include_resolve_fn include_resolver;
  shaderc_include_result_release_fn include_result_releaser;
  void* include_user_data;
};

shaderc_compile_options_t shaderc_compile_options_initialize() {
  return new (std::nothrow) shaderc_compile_options;
}

shaderc_compile_options_t shaderc_compile_options_clone(
    const shaderc_compile_options_t options) {
  if (!options) {
    return shaderc_compile_options_initialize();
  }
  return new (std::nothrow) shaderc_compile_options(*options);
}

void shaderc_compile_options_release(shaderc_compile_options_t options) {
  delete options;
}

void shaderc_compile_options_add_macro_definition(
    shaderc_compile_options_t options, const char* name, size_t name_length,
    const char* value, size_t value_length) {
  options->compiler.AddMacroDefinition(name, name_length, value, value_length);
}

void shaderc_compile_options_set_generate_debug_info(
    shaderc_compile_options_t options) {
  options->compiler.SetGenerateDebugInfo();
}

void shaderc_compile_options_set_forced_version_profile(
    shaderc_compile_options_t options, int version, shaderc_profile profile) {
  // Transfer the profile parameter from public enum type to glslang internal
  // enum type. No default case here so that compiler will complain if new enum
  // member is added later but not handled here.
  switch (profile) {
    case shaderc_profile_none:
      options->compiler.SetForcedVersionProfile(version, ENoProfile);
      break;
    case shaderc_profile_core:
      options->compiler.SetForcedVersionProfile(version, ECoreProfile);
      break;
    case shaderc_profile_compatibility:
      options->compiler.SetForcedVersionProfile(version, ECompatibilityProfile);
      break;
    case shaderc_profile_es:
      options->compiler.SetForcedVersionProfile(version, EEsProfile);
      break;
  }
}

void shaderc_compile_options_set_include_callbacks(
    shaderc_compile_options_t options, shaderc_include_resolve_fn resolver,
    shaderc_include_result_release_fn result_releaser, void* user_data) {
  options->include_resolver = resolver;
  options->include_result_releaser = result_releaser;
  options->include_user_data = user_data;
}

void shaderc_compile_options_set_suppress_warnings(
    shaderc_compile_options_t options) {
  options->compiler.SetSuppressWarnings();
}

void shaderc_compile_options_set_target_env(shaderc_compile_options_t options,
                                            shaderc_target_env target,
                                            uint32_t version) {
  // "version" reserved for future use, intended to distinguish between
  // different versions of a target environment
  options->compiler.SetMessageRules(GetMessageRules(target));
}

void shaderc_compile_options_set_warnings_as_errors(
    shaderc_compile_options_t options) {
  options->compiler.SetWarningsAsErrors();
}

shaderc_compiler_t shaderc_compiler_initialize() {
  static shaderc_util::GlslInitializer* initializer =
      new shaderc_util::GlslInitializer;
  shaderc_compiler_t compiler = new (std::nothrow) shaderc_compiler;
  compiler->initializer = initializer;
  return compiler;
}

void shaderc_compiler_release(shaderc_compiler_t compiler) { delete compiler; }

namespace {
shaderc_compilation_result_t CompileToSpecifiedOutputType(
    const shaderc_compiler_t compiler, const char* source_text,
    size_t source_text_size, shaderc_shader_kind shader_kind,
    const char* input_file_name, const char* entry_point_name,
    const shaderc_compile_options_t additional_options,
    shaderc_util::Compiler::OutputType output_type) {
  auto* result = new (std::nothrow) shaderc_compilation_result_vector;
  if (!result) return nullptr;

  if (!input_file_name) {
    result->messages = "Input file name string was null.";
    result->num_errors = 1;
    result->compilation_status = shaderc_compilation_status_compilation_error;
    return result;
  }
  result->compilation_status = shaderc_compilation_status_invalid_stage;
  bool compilation_succeeded = false;  // In case we exit early.
  std::vector<uint32_t> compilation_output_data;
  size_t compilation_output_data_size_in_bytes = 0u;
  if (!compiler->initializer) return result;
  TRY_IF_EXCEPTIONS_ENABLED {
    std::stringstream errors;
    size_t total_warnings = 0;
    size_t total_errors = 0;
    std::string input_file_name_str(input_file_name);
    EShLanguage forced_stage = GetForcedStage(shader_kind);
    shaderc_util::string_piece source_string =
        shaderc_util::string_piece(source_text, source_text + source_text_size);
    StageDeducer stage_deducer(shader_kind);
    if (additional_options) {
      InternalFileIncluder includer(additional_options->include_resolver,
                                    additional_options->include_result_releaser,
                                    additional_options->include_user_data);
      // Depends on return value optimization to avoid extra copy.
      std::tie(compilation_succeeded, compilation_output_data,
               compilation_output_data_size_in_bytes) =
          additional_options->compiler.Compile(
              source_string, forced_stage, input_file_name_str,
              // stage_deducer has a flag: error_, which we need to check later.
              // We need to make this a reference wrapper, so that std::function
              // won't make a copy for this callable object.
              std::ref(stage_deducer), includer, output_type, &errors,
              &total_warnings, &total_errors, compiler->initializer);
    } else {
      // Compile with default options.
      InternalFileIncluder includer;
      std::tie(compilation_succeeded, compilation_output_data,
               compilation_output_data_size_in_bytes) =
          shaderc_util::Compiler().Compile(
              source_string, forced_stage, input_file_name_str,
              std::ref(stage_deducer), includer, output_type, &errors,
              &total_warnings, &total_errors, compiler->initializer);
    }

    result->messages = errors.str();
    result->SetOutputData(std::move(compilation_output_data));
    result->output_data_size = compilation_output_data_size_in_bytes;
    result->num_warnings = total_warnings;
    result->num_errors = total_errors;
    if (compilation_succeeded) {
      result->compilation_status = shaderc_compilation_status_success;
    } else {
      // Check whether the error is caused by failing to deduce the shader
      // stage. If it is the case, set the error type to shader kind error.
      // Otherwise, set it to compilation error.
      result->compilation_status =
          stage_deducer.error() ? shaderc_compilation_status_invalid_stage
                                : shaderc_compilation_status_compilation_error;
    }
  }
  CATCH_IF_EXCEPTIONS_ENABLED(...) {
    result->compilation_status = shaderc_compilation_status_internal_error;
  }
  return result;
}
}  // anonymous namespace

shaderc_compilation_result_t shaderc_compile_into_spv(
    const shaderc_compiler_t compiler, const char* source_text,
    size_t source_text_size, shaderc_shader_kind shader_kind,
    const char* input_file_name, const char* entry_point_name,
    const shaderc_compile_options_t additional_options) {
  return CompileToSpecifiedOutputType(
      compiler, source_text, source_text_size, shader_kind, input_file_name,
      entry_point_name, additional_options,
      shaderc_util::Compiler::OutputType::SpirvBinary);
}

shaderc_compilation_result_t shaderc_compile_into_spv_assembly(
    const shaderc_compiler_t compiler, const char* source_text,
    size_t source_text_size, shaderc_shader_kind shader_kind,
    const char* input_file_name, const char* entry_point_name,
    const shaderc_compile_options_t additional_options) {
  return CompileToSpecifiedOutputType(
      compiler, source_text, source_text_size, shader_kind, input_file_name,
      entry_point_name, additional_options,
      shaderc_util::Compiler::OutputType::SpirvAssemblyText);
}

shaderc_compilation_result_t shaderc_compile_into_preprocessed_text(
    const shaderc_compiler_t compiler, const char* source_text,
    size_t source_text_size, shaderc_shader_kind shader_kind,
    const char* input_file_name, const char* entry_point_name,
    const shaderc_compile_options_t additional_options) {
  return CompileToSpecifiedOutputType(
      compiler, source_text, source_text_size, shader_kind, input_file_name,
      entry_point_name, additional_options,
      shaderc_util::Compiler::OutputType::PreprocessedText);
}

shaderc_compilation_result_t shaderc_assemble_into_spv(
    const shaderc_compiler_t compiler, const char* source_assembly,
    size_t source_assembly_size) {
  auto* result = new (std::nothrow) shaderc_compilation_result_spv_binary;
  if (!result) return nullptr;
  result->compilation_status = shaderc_compilation_status_invalid_assembly;
  if (!compiler->initializer) return result;
  if (source_assembly == nullptr) return result;

  TRY_IF_EXCEPTIONS_ENABLED {
    spv_binary assembling_output_data = nullptr;
    std::string errors;
    const bool assembling_succeeded = shaderc_util::SpirvToolsAssemble(
        {source_assembly, source_assembly + source_assembly_size},
        &assembling_output_data, &errors);
    result->num_errors = !assembling_succeeded;
    if (assembling_succeeded) {
      result->SetOutputData(assembling_output_data);
      result->output_data_size =
          assembling_output_data->wordCount * sizeof(uint32_t);
      result->compilation_status = shaderc_compilation_status_success;
    } else {
      result->messages = std::move(errors);
      result->compilation_status = shaderc_compilation_status_invalid_assembly;
    }
  }
  CATCH_IF_EXCEPTIONS_ENABLED(...) {
    result->compilation_status = shaderc_compilation_status_internal_error;
  }

  return result;
}

size_t shaderc_result_get_length(const shaderc_compilation_result_t result) {
  return result->output_data_size;
}

size_t shaderc_result_get_num_warnings(
    const shaderc_compilation_result_t result) {
  return result->num_warnings;
}

size_t shaderc_result_get_num_errors(
    const shaderc_compilation_result_t result) {
  return result->num_errors;
}

const char* shaderc_result_get_bytes(
    const shaderc_compilation_result_t result) {
  return result->GetBytes();
}

void shaderc_result_release(shaderc_compilation_result_t result) {
  delete result;
}

const char* shaderc_result_get_error_message(
    const shaderc_compilation_result_t result) {
  return result->messages.c_str();
}

shaderc_compilation_status shaderc_result_get_compilation_status(
    const shaderc_compilation_result_t result) {
  return result->compilation_status;
}

void shaderc_get_spv_version(unsigned int* version, unsigned int* revision) {
  *version = spv::Version;
  *revision = spv::Revision;
}

bool shaderc_parse_version_profile(const char* str, int* version,
                                   shaderc_profile* profile) {
  EProfile glslang_profile;
  bool success = shaderc_util::ParseVersionProfile(
      std::string(str, strlen(str)), version, &glslang_profile);
  if (!success) return false;

  switch (glslang_profile) {
    case EEsProfile:
      *profile = shaderc_profile_es;
      return true;
    case ECoreProfile:
      *profile = shaderc_profile_core;
      return true;
    case ECompatibilityProfile:
      *profile = shaderc_profile_compatibility;
      return true;
    case ENoProfile:
      *profile = shaderc_profile_none;
      return true;
    case EBadProfile:
      return false;
  }

  // Shouldn't reach here, all profile enum should be handled above.
  // Be strict to return false.
  return false;
}
