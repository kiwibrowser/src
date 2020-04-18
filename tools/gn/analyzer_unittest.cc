// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/analyzer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/build_settings.h"
#include "tools/gn/builder.h"
#include "tools/gn/config.h"
#include "tools/gn/loader.h"
#include "tools/gn/pool.h"
#include "tools/gn/settings.h"
#include "tools/gn/source_file.h"
#include "tools/gn/substitution_list.h"
#include "tools/gn/target.h"
#include "tools/gn/tool.h"
#include "tools/gn/toolchain.h"

namespace gn_analyzer_unittest {

class MockLoader : public Loader {
 public:
  MockLoader() = default;

  void Load(const SourceFile& file,
            const LocationRange& origin,
            const Label& toolchain_name) override {}
  void ToolchainLoaded(const Toolchain* toolchain) override {}
  Label GetDefaultToolchain() const override {
    return Label(SourceDir("//tc/"), "default");
  }
  const Settings* GetToolchainSettings(const Label& label) const override {
    return nullptr;
  }

 private:
  ~MockLoader() override = default;
};

class AnalyzerTest : public testing::Test {
 public:
  AnalyzerTest()
      : loader_(new MockLoader),
        builder_(loader_.get()),
        settings_(&build_settings_, std::string()) {
    build_settings_.SetBuildDir(SourceDir("//out/"));
    settings_.set_toolchain_label(Label(SourceDir("//tc/"), "default"));
    settings_.set_default_toolchain_label(settings_.toolchain_label());
    tc_dir_ = settings_.toolchain_label().dir();
    tc_name_ = settings_.toolchain_label().name();
  }

  // Ownership of the target will be transfered to the builder, so no leaks.
  Target* MakeTarget(const std::string& dir, const std::string& name) {
    Label label(SourceDir(dir), name, tc_dir_, tc_name_);
    Target* target = new Target(&settings_, label);

    return target;
  }

  // Ownership of the config will be transfered to the builder, so no leaks.
  Config* MakeConfig(const std::string& dir, const std::string& name) {
    Label label(SourceDir(dir), name, tc_dir_, tc_name_);
    Config* config = new Config(&settings_, label);

    return config;
  }

  // Ownership of the pool will be transfered to the builder, so no leaks.
  Pool* MakePool(const std::string& dir, const std::string& name) {
    Label label(SourceDir(dir), name, tc_dir_, tc_name_);
    Pool* pool = new Pool(&settings_, label);

    return pool;
  }

  void RunAnalyzerTest(const std::string& input,
                       const std::string& expected_output) {
    Analyzer analyzer(builder_, SourceFile("//build/config/BUILDCONFIG.gn"),
                      SourceFile("//.gn"),
                      {SourceFile("//out/debug/args.gn"),
                       SourceFile("//build/default_args.gn")});
    Err err;
    std::string actual_output = analyzer.Analyze(input, &err);
    EXPECT_EQ(err.has_error(), false);
    EXPECT_EQ(expected_output, actual_output);
  }

 protected:
  scoped_refptr<MockLoader> loader_;
  Builder builder_;
  BuildSettings build_settings_;
  Settings settings_;
  SourceDir tc_dir_;
  std::string tc_name_;
};

// Tests that a target is marked as affected if its sources are modified.
TEST_F(AnalyzerTest, TargetRefersToSources) {
  Target* t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::unique_ptr<Item>(t));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/file_name.cc" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t->sources().push_back(SourceFile("//dir/file_name.cc"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/file_name.cc" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that a target is marked as affected if its public headers are modified.
TEST_F(AnalyzerTest, TargetRefersToPublicHeaders) {
  Target* t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::unique_ptr<Item>(t));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/header_name.h" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t->public_headers().push_back(SourceFile("//dir/header_name.h"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/header_name.h" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that a target is marked as affected if its inputs are modified.
TEST_F(AnalyzerTest, TargetRefersToInputs) {
  Target* t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::unique_ptr<Item>(t));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/extra_input.cc" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  SourceFile extra_input(SourceFile("//dir/extra_input.cc"));
  t->config_values().inputs().push_back(extra_input);
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/extra_input.cc" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");

  t->config_values().inputs().clear();
  Config* c = MakeConfig("//dir", "config_name");
  builder_.ItemDefined(std::unique_ptr<Item>(c));
  c->own_values().inputs().push_back(extra_input);
  t->configs().push_back(LabelConfigPair(c));

  RunAnalyzerTest(
      R"({
       "files": [ "//dir/extra_input.cc" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that a target is marked as affected if its data are modified.
TEST_F(AnalyzerTest, TargetRefersToData) {
  Target* t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::unique_ptr<Item>(t));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/data.html" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t->data().push_back("//dir/data.html");
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/data.html" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that a target is marked as affected if the target is an action and its
// action script is modified.
TEST_F(AnalyzerTest, TargetRefersToActionScript) {
  Target* t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::unique_ptr<Item>(t));
  t->set_output_type(Target::ACTION);
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/script.py" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t->action_values().set_script(SourceFile("//dir/script.py"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/script.py" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that a target is marked as affected if its build dependency files are
// modified.
TEST_F(AnalyzerTest, TargetRefersToBuildDependencyFiles) {
  Target* t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::unique_ptr<Item>(t));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t->build_dependency_files().insert(SourceFile("//dir/BUILD.gn"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that if a target is marked as affected, then it propagates to dependent
// test_targets.
TEST_F(AnalyzerTest, AffectedTargetpropagatesToDependentTargets) {
  Target* t1 = MakeTarget("//dir", "target_name1");
  Target* t2 = MakeTarget("//dir", "target_name2");
  Target* t3 = MakeTarget("//dir", "target_name3");
  t1->private_deps().push_back(LabelTargetPair(t2));
  t2->private_deps().push_back(LabelTargetPair(t3));
  builder_.ItemDefined(std::unique_ptr<Item>(t1));
  builder_.ItemDefined(std::unique_ptr<Item>(t2));
  builder_.ItemDefined(std::unique_ptr<Item>(t3));

  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name1", "//dir:target_name2" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  t3->build_dependency_files().insert(SourceFile("//dir/BUILD.gn"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name1", "//dir:target_name2" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name1","//dir:target_name2"])"
      "}");
}

// Tests that if a config is marked as affected, then it propagates to dependent
// test_targets.
TEST_F(AnalyzerTest, AffectedConfigpropagatesToDependentTargets) {
  Config* c = MakeConfig("//dir", "config_name");
  Target* t = MakeTarget("//dir", "target_name");
  t->configs().push_back(LabelConfigPair(c));
  builder_.ItemDefined(std::unique_ptr<Item>(t));
  builder_.ItemDefined(std::unique_ptr<Item>(c));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  c->build_dependency_files().insert(SourceFile("//dir/BUILD.gn"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that if toolchain is marked as affected, then it propagates to
// dependent test_targets.
TEST_F(AnalyzerTest, AffectedToolchainpropagatesToDependentTargets) {
  Target* target = MakeTarget("//dir", "target_name");
  target->set_output_type(Target::EXECUTABLE);
  Toolchain* toolchain = new Toolchain(&settings_, settings_.toolchain_label());

  // The tool is not used anywhere, but is required to construct the dependency
  // between a target and the toolchain.
  std::unique_ptr<Tool> fake_tool(new Tool());
  fake_tool->set_outputs(
      SubstitutionList::MakeForTest("//out/debug/output.txt"));
  toolchain->SetTool(Toolchain::TYPE_LINK, std::move(fake_tool));
  builder_.ItemDefined(std::unique_ptr<Item>(target));
  builder_.ItemDefined(std::unique_ptr<Item>(toolchain));

  RunAnalyzerTest(
      R"({
         "files": [ "//tc/BUILD.gn" ],
         "additional_compile_targets": [ "all" ],
         "test_targets": [ "//dir:target_name" ]
         })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  toolchain->build_dependency_files().insert(SourceFile("//tc/BUILD.gn"));
  RunAnalyzerTest(
      R"({
       "files": [ "//tc/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that if a pool is marked as affected, then it propagates to dependent
// targets.
TEST_F(AnalyzerTest, AffectedPoolpropagatesToDependentTargets) {
  Target* t = MakeTarget("//dir", "target_name");
  t->set_output_type(Target::ACTION);
  Pool* p = MakePool("//dir", "pool_name");
  t->action_values().set_pool(LabelPtrPair<Pool>(p));

  builder_.ItemDefined(std::unique_ptr<Item>(t));
  builder_.ItemDefined(std::unique_ptr<Item>(p));

  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");

  p->build_dependency_files().insert(SourceFile("//dir/BUILD.gn"));
  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": [ "//dir:target_name" ]
       })",
      "{"
      R"("compile_targets":["all"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Tests that when dependency was found, the "compile_targets" in the output is
// not "all".
TEST_F(AnalyzerTest, CompileTargetsAllWasPruned) {
  Target* t1 = MakeTarget("//dir", "target_name1");
  Target* t2 = MakeTarget("//dir", "target_name2");
  builder_.ItemDefined(std::unique_ptr<Item>(t1));
  builder_.ItemDefined(std::unique_ptr<Item>(t2));
  t2->build_dependency_files().insert(SourceFile("//dir/BUILD.gn"));

  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": []
       })",
      "{"
      R"("compile_targets":["//dir:target_name2"],)"
      R"/("status":"Found dependency",)/"
      R"("test_targets":[])"
      "}");
}

// Tests that output is "No dependency" when no dependency is found.
TEST_F(AnalyzerTest, NoDependency) {
  Target* t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::unique_ptr<Item>(t));

  RunAnalyzerTest(
      R"({
       "files": [ "//dir/BUILD.gn" ],
       "additional_compile_targets": [ "all" ],
       "test_targets": []
       })",
      "{"
      R"("compile_targets":[],)"
      R"/("status":"No dependency",)/"
      R"("test_targets":[])"
      "}");
}

// Tests that output is "No dependency" when no files or targets are provided.
TEST_F(AnalyzerTest, NoFilesNoTargets) {
  RunAnalyzerTest(
      R"({
       "files": [],
       "additional_compile_targets": [],
       "test_targets": []
      })",
      "{"
      R"("compile_targets":[],)"
      R"("status":"No dependency",)"
      R"("test_targets":[])"
      "}");
}

// Tests that output displays proper error message when given files aren't
// source-absolute or absolute path.
TEST_F(AnalyzerTest, FilesArentSourceAbsolute) {
  RunAnalyzerTest(
      R"({
       "files": [ "a.cc" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name" ]
      })",
      "{"
      R"("error":)"
      R"("\"a.cc\" is not a source-absolute or absolute path.",)"
      R"("invalid_targets":[])"
      "}");
}

// Tests that output displays proper error message when input is illy-formed.
TEST_F(AnalyzerTest, WrongInputFields) {
  RunAnalyzerTest(
      R"({
       "files": [ "//a.cc" ],
       "compile_targets": [],
       "test_targets": [ "//dir:target_name" ]
      })",
      "{"
      R"("error":)"
      R"("Input does not have a key named )"
      R"(\"additional_compile_targets\" with a list value.",)"
      R"("invalid_targets":[])"
      "}");
}

// Bails out early with "Found dependency (all)" if dot file is modified.
TEST_F(AnalyzerTest, DotFileWasModified) {
  Target* t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::unique_ptr<Item>(t));

  RunAnalyzerTest(
      R"({
       "files": [ "//.gn" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name" ]
      })",
      "{"
      R"("compile_targets":["//dir:target_name"],)"
      R"/("status":"Found dependency (all)",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Bails out early with "Found dependency (all)" if master build config file is
// modified.
TEST_F(AnalyzerTest, BuildConfigFileWasModified) {
  Target* t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::unique_ptr<Item>(t));

  RunAnalyzerTest(
      R"({
       "files": [ "//build/config/BUILDCONFIG.gn" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name" ]
      })",
      "{"
      R"("compile_targets":["//dir:target_name"],)"
      R"/("status":"Found dependency (all)",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

// Bails out early with "Found dependency (all)" if a build args dependency file
// is modified.
TEST_F(AnalyzerTest, BuildArgsDependencyFileWasModified) {
  Target* t = MakeTarget("//dir", "target_name");
  builder_.ItemDefined(std::unique_ptr<Item>(t));

  RunAnalyzerTest(
      R"({
       "files": [ "//build/default_args.gn" ],
       "additional_compile_targets": [],
       "test_targets": [ "//dir:target_name" ]
      })",
      "{"
      R"("compile_targets":["//dir:target_name"],)"
      R"/("status":"Found dependency (all)",)/"
      R"("test_targets":["//dir:target_name"])"
      "}");
}

}  // namespace gn_analyzer_unittest
