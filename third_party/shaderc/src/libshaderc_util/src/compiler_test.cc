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

#include "libshaderc_util/compiler.h"

#include <sstream>

#include <gtest/gtest.h>

#include "death_test.h"
#include "libshaderc_util/counting_includer.h"

namespace {

using shaderc_util::Compiler;

// These are the flag combinations Glslang uses to set language
// rules based on the target environment.
const EShMessages kOpenGLCompatibilityRules = EShMsgDefault;
const EShMessages kOpenGLRules = EShMsgSpvRules;
const EShMessages kVulkanRules =
    static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

// A trivial vertex shader
const char kVertexShader[] =
    "#version 140\n"
    "void main() {}";

// A shader that compiles under OpenGL compatibility profile rules,
// but not OpenGL core profile rules.
const char kOpenGLCompatibilityFragShader[] =
    R"(#version 140
       uniform highp sampler2D tex;
       void main() {
         gl_FragColor = texture2D(tex, vec2(0.0,0.0));
       })";

// A shader that compiles under OpenGL compatibility profile rules,
// but not OpenGL core profile rules, even when deducing the stage.
const char kOpenGLCompatibilityFragShaderDeducibleStage[] =
    R"(#version 140
       #pragma shader_stage(fragment)
       uniform highp sampler2D tex;
       void main() {
         gl_FragColor = texture2D(tex, vec2(0.0,0.0));
       })";

// A shader that compiles under OpenGL core profile rules.
const char kOpenGLVertexShader[] =
    R"(#version 150
       void main() { int t = gl_VertexID; })";

// A shader that compiles under OpenGL core profile rules, even when
// deducing the stage.
const char kOpenGLVertexShaderDeducibleStage[] =
    R"(#version 150
       #pragma shader_stage(vertex)
       void main() { int t = gl_VertexID; })";

// A shader that compiles under Vulkan rules.
// See the GL_KHR_vuklan_glsl extension to GLSL.
const char kVulkanVertexShader[] =
    R"(#version 310 es
       void main() { int t = gl_VertexIndex; })";

// A shader that needs valueless macro predefinition E, to be compiled
// successfully.
const std::string kValuelessPredefinitionShader =
    "#version 140\n"
    "#ifdef E\n"
    "void main(){}\n"
    "#else\n"
    "#error\n"
    "#endif";

// A CountingIncluder that never returns valid content for a requested
// file inclusion.
class DummyCountingIncluder : public shaderc_util::CountingIncluder {
 private:
  // Returns a pair of empty strings.
  virtual glslang::TShader::Includer::IncludeResult* include_delegate(
      const char*, glslang::TShader::Includer::IncludeType,
      const char*,
      size_t) override {
    return nullptr;
  }
  virtual void release_delegate(
      glslang::TShader::Includer::IncludeResult*) override {}
};

// A test fixture for compiling GLSL shaders.
class CompilerTest : public testing::Test {
 public:
  // Returns true if the given compiler successfully compiles the given shader
  // source for the given shader stage to the specified output type.  No
  // includes are permitted, and shader stage deduction falls back to an invalid
  // shader stage.
  bool SimpleCompilationSucceedsForOutputType(
      std::string source, EShLanguage stage, Compiler::OutputType output_type) {
    std::function<EShLanguage(std::ostream*, const shaderc_util::string_piece&)>
        stage_callback = [](std::ostream*, const shaderc_util::string_piece&) {
          return EShLangCount;
        };
    std::stringstream errors;
    size_t total_warnings = 0;
    size_t total_errors = 0;
    shaderc_util::GlslInitializer initializer;
    bool result = false;
    DummyCountingIncluder dummy_includer;
    std::tie(result, std::ignore, std::ignore) = compiler_.Compile(
        source, stage, "shader", stage_callback, dummy_includer,
        Compiler::OutputType::SpirvBinary, &errors, &total_warnings,
        &total_errors, &initializer);
    errors_ = errors.str();
    return result;
  }

  // Returns the result of SimpleCompilationSucceedsForOutputType, where
  // the output type is a SPIR-V binary module.
  bool SimpleCompilationSucceeds(std::string source, EShLanguage stage) {
    return SimpleCompilationSucceedsForOutputType(
        source, stage, Compiler::OutputType::SpirvBinary);
  }
 protected:
  Compiler compiler_;
  // The error string from the most recent compilation.
  std::string errors_;
};

TEST_F(CompilerTest, SimpleVertexShaderCompilesSuccessfullyToBinary) {
  EXPECT_TRUE(SimpleCompilationSucceeds(kVertexShader, EShLangVertex));
}

TEST_F(CompilerTest, SimpleVertexShaderCompilesSuccessfullyToAssembly) {
  EXPECT_TRUE(SimpleCompilationSucceedsForOutputType(
      kVertexShader, EShLangVertex, Compiler::OutputType::SpirvAssemblyText));
}

TEST_F(CompilerTest, SimpleVertexShaderPreprocessesSuccessfully) {
  EXPECT_TRUE(SimpleCompilationSucceedsForOutputType(
      kVertexShader, EShLangVertex, Compiler::OutputType::PreprocessedText));
}

TEST_F(CompilerTest, BadVertexShaderFailsCompilation) {
  EXPECT_FALSE(SimpleCompilationSucceeds(" bogus ", EShLangVertex));
}

TEST_F(CompilerTest, SimpleVulkanShaderCompilesWithDefaultCompilerSettings) {
  EXPECT_TRUE(SimpleCompilationSucceeds(kVulkanVertexShader, EShLangVertex));
}

TEST_F(CompilerTest, RespectTargetEnvOnOpenGLCompatibilityShader) {
  const EShLanguage stage = EShLangFragment;

  compiler_.SetMessageRules(kOpenGLCompatibilityRules);
  EXPECT_TRUE(SimpleCompilationSucceeds(kOpenGLCompatibilityFragShader, stage));
  compiler_.SetMessageRules(kOpenGLRules);
  EXPECT_FALSE(
      SimpleCompilationSucceeds(kOpenGLCompatibilityFragShader, stage));
  compiler_.SetMessageRules(kVulkanRules);
  EXPECT_FALSE(
      SimpleCompilationSucceeds(kOpenGLCompatibilityFragShader, stage));
  // Default compiler.
  compiler_ = Compiler();
  EXPECT_FALSE(
      SimpleCompilationSucceeds(kOpenGLCompatibilityFragShader, stage));
}

TEST_F(CompilerTest,
       RespectTargetEnvOnOpenGLCompatibilityShaderWhenDeducingStage) {
  const EShLanguage stage = EShLangCount;

  compiler_.SetMessageRules(kOpenGLCompatibilityRules);
  EXPECT_TRUE(SimpleCompilationSucceeds(
      kOpenGLCompatibilityFragShaderDeducibleStage, stage))
      << errors_;
  compiler_.SetMessageRules(kOpenGLRules);
  EXPECT_FALSE(SimpleCompilationSucceeds(
      kOpenGLCompatibilityFragShaderDeducibleStage, stage))
      << errors_;
  compiler_.SetMessageRules(kVulkanRules);
  EXPECT_FALSE(SimpleCompilationSucceeds(
      kOpenGLCompatibilityFragShaderDeducibleStage, stage))
      << errors_;
  // Default compiler.
  compiler_ = Compiler();
  EXPECT_FALSE(SimpleCompilationSucceeds(
      kOpenGLCompatibilityFragShaderDeducibleStage, stage))
      << errors_;
}

TEST_F(CompilerTest, RespectTargetEnvOnOpenGLShader) {
  const EShLanguage stage = EShLangVertex;

  compiler_.SetMessageRules(kOpenGLCompatibilityRules);
  EXPECT_TRUE(SimpleCompilationSucceeds(kOpenGLVertexShader, stage));

  compiler_.SetMessageRules(kOpenGLRules);
  EXPECT_TRUE(SimpleCompilationSucceeds(kOpenGLVertexShader, stage));
}

TEST_F(CompilerTest, RespectTargetEnvOnOpenGLShaderWhenDeducingStage) {
  const EShLanguage stage = EShLangCount;

  compiler_.SetMessageRules(kOpenGLCompatibilityRules);
  EXPECT_TRUE(
      SimpleCompilationSucceeds(kOpenGLVertexShaderDeducibleStage, stage));

  compiler_.SetMessageRules(kOpenGLRules);
  EXPECT_TRUE(
      SimpleCompilationSucceeds(kOpenGLVertexShaderDeducibleStage, stage));
}

TEST_F(CompilerTest, RespectTargetEnvOnVulkanShader) {
  compiler_.SetMessageRules(kVulkanRules);
  EXPECT_TRUE(SimpleCompilationSucceeds(kVulkanVertexShader, EShLangVertex));
}

TEST_F(CompilerTest, VulkanSpecificShaderFailsUnderOpenGLCompatibilityRules) {
  compiler_.SetMessageRules(kOpenGLCompatibilityRules);
  EXPECT_FALSE(SimpleCompilationSucceeds(kVulkanVertexShader, EShLangVertex));
}

TEST_F(CompilerTest, VulkanSpecificShaderFailsUnderOpenGLRules) {
  compiler_.SetMessageRules(kOpenGLRules);
  EXPECT_FALSE(SimpleCompilationSucceeds(kVulkanVertexShader, EShLangVertex));
}

TEST_F(CompilerTest, OpenGLCompatibilitySpecificShaderFailsUnderDefaultRules) {
  EXPECT_FALSE(SimpleCompilationSucceeds(kOpenGLCompatibilityFragShader,
                                         EShLangFragment));
}

TEST_F(CompilerTest, OpenGLSpecificShaderFailsUnderDefaultRules) {
  EXPECT_FALSE(SimpleCompilationSucceeds(kOpenGLVertexShader, EShLangVertex));
}

TEST_F(CompilerTest, OpenGLCompatibilitySpecificShaderFailsUnderVulkanRules) {
  compiler_.SetMessageRules(kVulkanRules);
  EXPECT_FALSE(SimpleCompilationSucceeds(kOpenGLCompatibilityFragShader,
                                         EShLangFragment));
}

TEST_F(CompilerTest, OpenGLSpecificShaderFailsUnderVulkanRules) {
  compiler_.SetMessageRules(kVulkanRules);
  EXPECT_FALSE(SimpleCompilationSucceeds(kOpenGLVertexShader, EShLangVertex));
}

TEST_F(CompilerTest, AddMacroDefinition) {
  const std::string kMinimalExpandedShader = "#version 140\nvoid E(){}";
  compiler_.AddMacroDefinition("E", 1u, "main", 4u);
  EXPECT_TRUE(SimpleCompilationSucceeds(kMinimalExpandedShader, EShLangVertex));
}

TEST_F(CompilerTest, AddValuelessMacroDefinitionNullPointer) {
  compiler_.AddMacroDefinition("E", 1u, nullptr, 100u);
  EXPECT_TRUE(
      SimpleCompilationSucceeds(kValuelessPredefinitionShader, EShLangVertex));
}

TEST_F(CompilerTest, AddValuelessMacroDefinitionZeroLength) {
  compiler_.AddMacroDefinition("E", 1u, "something", 0u);
  EXPECT_TRUE(
      SimpleCompilationSucceeds(kValuelessPredefinitionShader, EShLangVertex));
}

TEST_F(CompilerTest, AddMacroDefinitionNotNullTerminated) {
  const std::string kMinimalExpandedShader = "#version 140\nvoid E(){}";
  compiler_.AddMacroDefinition("EFGH", 1u, "mainnnnnn", 4u);
  EXPECT_TRUE(SimpleCompilationSucceeds(kMinimalExpandedShader, EShLangVertex));
}

// A convert-string-to-vector test case consists of 1) an input string; 2) an
// expected vector after the conversion.
struct ConvertStringToVectorTestCase {
  std::string input_str;
  std::vector<uint32_t> expected_output_vec;
};

// Test the shaderc_util::ConvertStringToVector() function. The content of the
// input string, including the null terminator, should be packed into uint32_t
// cells and stored in the returned vector of uint32_t. In case extra bytes are
// required to complete the ending uint32_t element, bytes with value 0x00
// should be used to fill the space.
using ConvertStringToVectorTestFixture =
    testing::TestWithParam<ConvertStringToVectorTestCase>;

TEST_P(ConvertStringToVectorTestFixture, VariousStringSize) {
  const ConvertStringToVectorTestCase& test_case = GetParam();
  EXPECT_EQ(test_case.expected_output_vec,
            shaderc_util::ConvertStringToVector(test_case.input_str))
      << "test_case.input_str: " << test_case.input_str << std::endl;
}

INSTANTIATE_TEST_CASE_P(
    ConvertStringToVectorTest, ConvertStringToVectorTestFixture,
    testing::ValuesIn(std::vector<ConvertStringToVectorTestCase>{
        {"", {0x00000000}},
        {"1", {0x00000031}},
        {"12", {0x00003231}},
        {"123", {0x00333231}},
        {"1234", {0x34333231, 0x00000000}},
        {"12345", {0x34333231, 0x00000035}},
        {"123456", {0x34333231, 0x00003635}},
        {"1234567", {0x34333231, 0x00373635}},
        {"12345678", {0x34333231, 0x38373635, 0x00000000}},
        {"123456789", {0x34333231, 0x38373635, 0x00000039}},
    }));

}  // anonymous namespace
