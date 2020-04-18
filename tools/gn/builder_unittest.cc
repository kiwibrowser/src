// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/builder.h"
#include "tools/gn/config.h"
#include "tools/gn/loader.h"
#include "tools/gn/target.h"
#include "tools/gn/test_with_scope.h"
#include "tools/gn/toolchain.h"

namespace gn_builder_unittest {

class MockLoader : public Loader {
 public:
  MockLoader() = default;

  // Loader implementation:
  void Load(const SourceFile& file,
            const LocationRange& origin,
            const Label& toolchain_name) override {
    files_.push_back(file);
  }
  void ToolchainLoaded(const Toolchain* toolchain) override {}
  Label GetDefaultToolchain() const override { return Label(); }
  const Settings* GetToolchainSettings(const Label& label) const override {
    return nullptr;
  }

  bool HasLoadedNone() const {
    return files_.empty();
  }

  // Returns true if one/two loads have been requested and they match the given
  // file(s). This will clear the records so it will be empty for the next call.
  bool HasLoadedOne(const SourceFile& file) {
    if (files_.size() != 1u) {
      files_.clear();
      return false;
    }
    bool match = (files_[0] == file);
    files_.clear();
    return match;
  }
  bool HasLoadedTwo(const SourceFile& a, const SourceFile& b) {
    if (files_.size() != 2u) {
      files_.clear();
      return false;
    }

    bool match = (
        (files_[0] == a && files_[1] == b) ||
        (files_[0] == b && files_[1] == a));
    files_.clear();
    return match;
  }

 private:
  ~MockLoader() override = default;

  std::vector<SourceFile> files_;
};

class BuilderTest : public testing::Test {
 public:
  BuilderTest()
      : loader_(new MockLoader),
        builder_(loader_.get()),
        settings_(&build_settings_, std::string()),
        scope_(&settings_) {
    build_settings_.SetBuildDir(SourceDir("//out/"));
    settings_.set_toolchain_label(Label(SourceDir("//tc/"), "default"));
    settings_.set_default_toolchain_label(settings_.toolchain_label());
  }

  Toolchain* DefineToolchain() {
    Toolchain* tc = new Toolchain(&settings_, settings_.toolchain_label());
    TestWithScope::SetupToolchain(tc);
    builder_.ItemDefined(std::unique_ptr<Item>(tc));
    return tc;
  }

 protected:
  scoped_refptr<MockLoader> loader_;
  Builder builder_;
  BuildSettings build_settings_;
  Settings settings_;
  Scope scope_;
};

TEST_F(BuilderTest, BasicDeps) {
  SourceDir toolchain_dir = settings_.toolchain_label().dir();
  std::string toolchain_name = settings_.toolchain_label().name();

  // Construct a dependency chain: A -> B -> C. Define A first with a
  // forward-reference to B, then C, then B to test the different orders that
  // the dependencies are hooked up.
  Label a_label(SourceDir("//a/"), "a", toolchain_dir, toolchain_name);
  Label b_label(SourceDir("//b/"), "b", toolchain_dir, toolchain_name);
  Label c_label(SourceDir("//c/"), "c", toolchain_dir, toolchain_name);

  // The builder will take ownership of the pointers.
  Target* a = new Target(&settings_, a_label);
  a->public_deps().push_back(LabelTargetPair(b_label));
  a->set_output_type(Target::EXECUTABLE);
  builder_.ItemDefined(std::unique_ptr<Item>(a));

  // Should have requested that B and the toolchain is loaded.
  EXPECT_TRUE(loader_->HasLoadedTwo(SourceFile("//tc/BUILD.gn"),
                                    SourceFile("//b/BUILD.gn")));

  // Define the toolchain.
  DefineToolchain();
  BuilderRecord* toolchain_record =
      builder_.GetRecord(settings_.toolchain_label());
  ASSERT_TRUE(toolchain_record);
  EXPECT_EQ(BuilderRecord::ITEM_TOOLCHAIN, toolchain_record->type());

  // A should be unresolved with an item
  BuilderRecord* a_record = builder_.GetRecord(a_label);
  EXPECT_TRUE(a_record->item());
  EXPECT_FALSE(a_record->resolved());
  EXPECT_FALSE(a_record->can_resolve());

  // B should be unresolved, have no item, and no deps.
  BuilderRecord* b_record = builder_.GetRecord(b_label);
  EXPECT_FALSE(b_record->item());
  EXPECT_FALSE(b_record->resolved());
  EXPECT_FALSE(b_record->can_resolve());
  EXPECT_TRUE(b_record->all_deps().empty());

  // A should have two deps: B and the toolchain. Only B should be unresolved.
  EXPECT_EQ(2u, a_record->all_deps().size());
  EXPECT_EQ(1u, a_record->unresolved_deps().size());
  EXPECT_NE(a_record->all_deps().end(),
            a_record->all_deps().find(toolchain_record));
  EXPECT_NE(a_record->all_deps().end(),
            a_record->all_deps().find(b_record));
  EXPECT_NE(a_record->unresolved_deps().end(),
            a_record->unresolved_deps().find(b_record));

  // B should be marked as having A waiting on it.
  EXPECT_EQ(1u, b_record->waiting_on_resolution().size());
  EXPECT_NE(b_record->waiting_on_resolution().end(),
            b_record->waiting_on_resolution().find(a_record));

  // Add the C target.
  Target* c = new Target(&settings_, c_label);
  c->set_output_type(Target::STATIC_LIBRARY);
  c->visibility().SetPublic();
  builder_.ItemDefined(std::unique_ptr<Item>(c));

  // C only depends on the already-loaded toolchain so we shouldn't have
  // requested anything else.
  EXPECT_TRUE(loader_->HasLoadedNone());

  // Add the B target.
  Target* b = new Target(&settings_, b_label);
  a->public_deps().push_back(LabelTargetPair(c_label));
  b->set_output_type(Target::SHARED_LIBRARY);
  b->visibility().SetPublic();
  builder_.ItemDefined(std::unique_ptr<Item>(b));

  // B depends only on the already-loaded C and toolchain so we shouldn't have
  // requested anything else.
  EXPECT_TRUE(loader_->HasLoadedNone());

  // All targets should now be resolved.
  BuilderRecord* c_record = builder_.GetRecord(c_label);
  EXPECT_TRUE(a_record->resolved());
  EXPECT_TRUE(b_record->resolved());
  EXPECT_TRUE(c_record->resolved());

  EXPECT_TRUE(a_record->unresolved_deps().empty());
  EXPECT_TRUE(b_record->unresolved_deps().empty());
  EXPECT_TRUE(c_record->unresolved_deps().empty());

  EXPECT_TRUE(a_record->waiting_on_resolution().empty());
  EXPECT_TRUE(b_record->waiting_on_resolution().empty());
  EXPECT_TRUE(c_record->waiting_on_resolution().empty());
}

// Tests that the "should generate" flag is set and propagated properly.
TEST_F(BuilderTest, ShouldGenerate) {
  DefineToolchain();

  // Define a secondary toolchain.
  Settings settings2(&build_settings_, "secondary/");
  Label toolchain_label2(SourceDir("//tc/"), "secondary");
  settings2.set_toolchain_label(toolchain_label2);
  Toolchain* tc2 = new Toolchain(&settings2, toolchain_label2);
  TestWithScope::SetupToolchain(tc2);
  builder_.ItemDefined(std::unique_ptr<Item>(tc2));

  // Construct a dependency chain: A -> B. A is in the default toolchain, B
  // is not.
  Label a_label(SourceDir("//foo/"), "a",
                settings_.toolchain_label().dir(), "a");
  Label b_label(SourceDir("//foo/"), "b",
                toolchain_label2.dir(), toolchain_label2.name());

  // First define B.
  Target* b = new Target(&settings2, b_label);
  b->visibility().SetPublic();
  b->set_output_type(Target::EXECUTABLE);
  builder_.ItemDefined(std::unique_ptr<Item>(b));

  // B should not be marked generated by default.
  BuilderRecord* b_record = builder_.GetRecord(b_label);
  EXPECT_FALSE(b_record->should_generate());

  // Define A with a dependency on B.
  Target* a = new Target(&settings_, a_label);
  a->public_deps().push_back(LabelTargetPair(b_label));
  a->set_output_type(Target::EXECUTABLE);
  builder_.ItemDefined(std::unique_ptr<Item>(a));

  // A should have the generate bit set since it's in the default toolchain.
  BuilderRecord* a_record = builder_.GetRecord(a_label);
  EXPECT_TRUE(a_record->should_generate());

  // It should have gotten pushed to B.
  EXPECT_TRUE(b_record->should_generate());
}

// Tests that configs applied to a config get loaded (bug 536844).
TEST_F(BuilderTest, ConfigLoad) {
  SourceDir toolchain_dir = settings_.toolchain_label().dir();
  std::string toolchain_name = settings_.toolchain_label().name();

  // Construct a dependency chain: A -> B -> C. Define A first with a
  // forward-reference to B, then C, then B to test the different orders that
  // the dependencies are hooked up.
  Label a_label(SourceDir("//a/"), "a", toolchain_dir, toolchain_name);
  Label b_label(SourceDir("//b/"), "b", toolchain_dir, toolchain_name);
  Label c_label(SourceDir("//c/"), "c", toolchain_dir, toolchain_name);

  // The builder will take ownership of the pointers.
  Config* a = new Config(&settings_, a_label);
  a->configs().push_back(LabelConfigPair(b_label));
  builder_.ItemDefined(std::unique_ptr<Item>(a));

  // Should have requested that B is loaded.
  EXPECT_TRUE(loader_->HasLoadedOne(SourceFile("//b/BUILD.gn")));
}

}  // namespace gn_builder_unittest
