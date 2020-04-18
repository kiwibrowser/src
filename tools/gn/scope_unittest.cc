// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/scope.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/input_file.h"
#include "tools/gn/parse_tree.h"
#include "tools/gn/source_file.h"
#include "tools/gn/template.h"
#include "tools/gn/test_with_scope.h"

namespace {

bool HasStringValueEqualTo(const Scope* scope,
                           const char* name,
                           const char* expected_value) {
  const Value* value = scope->GetValue(name);
  if (!value)
    return false;
  if (value->type() != Value::STRING)
    return false;
  return value->string_value() == expected_value;
}

bool ContainsBuildDependencyFile(const Scope* scope,
                                 const SourceFile& source_file) {
  const auto& build_dependency_files = scope->build_dependency_files();
  return build_dependency_files.end() !=
         build_dependency_files.find(source_file);
}

}  // namespace

TEST(Scope, InheritBuildDependencyFilesFromParent) {
  TestWithScope setup;
  SourceFile source_file = SourceFile("//a/BUILD.gn");
  setup.scope()->AddBuildDependencyFile(source_file);

  Scope new_scope(setup.scope());
  EXPECT_EQ(1U, new_scope.build_dependency_files().size());
  EXPECT_TRUE(ContainsBuildDependencyFile(&new_scope, source_file));
}

TEST(Scope, NonRecursiveMergeTo) {
  TestWithScope setup;

  // Make a pretend parse node with proper tracking that we can blame for the
  // given value.
  InputFile input_file(SourceFile("//foo"));
  Token assignment_token(Location(&input_file, 1, 1, 1), Token::STRING,
      "\"hello\"");
  LiteralNode assignment;
  assignment.set_value(assignment_token);

  // Add some values to the scope.
  Value old_value(&assignment, "hello");
  setup.scope()->SetValue("v", old_value, &assignment);
  base::StringPiece private_var_name("_private");
  setup.scope()->SetValue(private_var_name, old_value, &assignment);

  // Add some templates to the scope.
  FunctionCallNode templ_definition;
  scoped_refptr<Template> templ(new Template(setup.scope(), &templ_definition));
  setup.scope()->AddTemplate("templ", templ.get());
  scoped_refptr<Template> private_templ(
      new Template(setup.scope(), &templ_definition));
  setup.scope()->AddTemplate("_templ", private_templ.get());

  // Detect collisions of values' values.
  {
    Scope new_scope(setup.settings());
    Value new_value(&assignment, "goodbye");
    new_scope.SetValue("v", new_value, &assignment);

    Err err;
    EXPECT_FALSE(setup.scope()->NonRecursiveMergeTo(
        &new_scope, Scope::MergeOptions(),
        &assignment, "error", &err));
    EXPECT_TRUE(err.has_error());
  }

  // Template name collisions.
  {
    Scope new_scope(setup.settings());

    scoped_refptr<Template> new_templ(
        new Template(&new_scope, &templ_definition));
    new_scope.AddTemplate("templ", new_templ.get());

    Err err;
    EXPECT_FALSE(setup.scope()->NonRecursiveMergeTo(
        &new_scope, Scope::MergeOptions(), &assignment, "error", &err));
    EXPECT_TRUE(err.has_error());
  }

  // The clobber flag should just overwrite colliding values.
  {
    Scope new_scope(setup.settings());
    Value new_value(&assignment, "goodbye");
    new_scope.SetValue("v", new_value, &assignment);

    Err err;
    Scope::MergeOptions options;
    options.clobber_existing = true;
    EXPECT_TRUE(setup.scope()->NonRecursiveMergeTo(
        &new_scope, options, &assignment, "error", &err));
    EXPECT_FALSE(err.has_error());

    const Value* found_value = new_scope.GetValue("v");
    ASSERT_TRUE(found_value);
    EXPECT_TRUE(old_value == *found_value);
  }

  // Clobber flag for templates.
  {
    Scope new_scope(setup.settings());

    scoped_refptr<Template> new_templ(
        new Template(&new_scope, &templ_definition));
    new_scope.AddTemplate("templ", new_templ.get());
    Scope::MergeOptions options;
    options.clobber_existing = true;

    Err err;
    EXPECT_TRUE(setup.scope()->NonRecursiveMergeTo(
        &new_scope, options, &assignment, "error", &err));
    EXPECT_FALSE(err.has_error());

    const Template* found_value = new_scope.GetTemplate("templ");
    ASSERT_TRUE(found_value);
    EXPECT_TRUE(templ.get() == found_value);
  }

  // Don't flag values that technically collide but have the same value.
  {
    Scope new_scope(setup.settings());
    Value new_value(&assignment, "hello");
    new_scope.SetValue("v", new_value, &assignment);

    Err err;
    EXPECT_TRUE(setup.scope()->NonRecursiveMergeTo(
        &new_scope, Scope::MergeOptions(), &assignment, "error", &err));
    EXPECT_FALSE(err.has_error());
  }

  // Templates that technically collide but are the same.
  {
    Scope new_scope(setup.settings());

    scoped_refptr<Template> new_templ(
        new Template(&new_scope, &templ_definition));
    new_scope.AddTemplate("templ", templ.get());

    Err err;
    EXPECT_TRUE(setup.scope()->NonRecursiveMergeTo(
        &new_scope, Scope::MergeOptions(), &assignment, "error", &err));
    EXPECT_FALSE(err.has_error());
  }

  // Copy private values and templates.
  {
    Scope new_scope(setup.settings());

    Err err;
    EXPECT_TRUE(setup.scope()->NonRecursiveMergeTo(
        &new_scope, Scope::MergeOptions(), &assignment, "error", &err));
    EXPECT_FALSE(err.has_error());
    EXPECT_TRUE(new_scope.GetValue(private_var_name));
    EXPECT_TRUE(new_scope.GetTemplate("_templ"));
  }

  // Skip private values and templates.
  {
    Scope new_scope(setup.settings());

    Err err;
    Scope::MergeOptions options;
    options.skip_private_vars = true;
    EXPECT_TRUE(setup.scope()->NonRecursiveMergeTo(
        &new_scope, options, &assignment, "error", &err));
    EXPECT_FALSE(err.has_error());
    EXPECT_FALSE(new_scope.GetValue(private_var_name));
    EXPECT_FALSE(new_scope.GetTemplate("_templ"));
  }

  // Don't mark used.
  {
    Scope new_scope(setup.settings());

    Err err;
    Scope::MergeOptions options;
    EXPECT_TRUE(setup.scope()->NonRecursiveMergeTo(
        &new_scope, options, &assignment, "error", &err));
    EXPECT_FALSE(err.has_error());
    EXPECT_FALSE(new_scope.CheckForUnusedVars(&err));
    EXPECT_TRUE(err.has_error());
  }

  // Mark dest used.
  {
    Scope new_scope(setup.settings());

    Err err;
    Scope::MergeOptions options;
    options.mark_dest_used = true;
    EXPECT_TRUE(setup.scope()->NonRecursiveMergeTo(
        &new_scope, options, &assignment, "error", &err));
    EXPECT_FALSE(err.has_error());
    EXPECT_TRUE(new_scope.CheckForUnusedVars(&err));
    EXPECT_FALSE(err.has_error());
  }

  // Build dependency files are merged.
  {
    Scope from_scope(setup.settings());
    SourceFile source_file = SourceFile("//a/BUILD.gn");
    from_scope.AddBuildDependencyFile(source_file);

    Scope to_scope(setup.settings());
    EXPECT_FALSE(ContainsBuildDependencyFile(&to_scope, source_file));

    Scope::MergeOptions options;
    Err err;
    EXPECT_TRUE(from_scope.NonRecursiveMergeTo(&to_scope, options, &assignment,
                                               "error", &err));
    EXPECT_FALSE(err.has_error());
    EXPECT_EQ(1U, to_scope.build_dependency_files().size());
    EXPECT_TRUE(ContainsBuildDependencyFile(&to_scope, source_file));
  }
}

TEST(Scope, MakeClosure) {
  // Create 3 nested scopes [const root from setup] <- nested1 <- nested2.
  TestWithScope setup;

  // Make a pretend parse node with proper tracking that we can blame for the
  // given value.
  InputFile input_file(SourceFile("//foo"));
  Token assignment_token(Location(&input_file, 1, 1, 1), Token::STRING,
      "\"hello\"");
  LiteralNode assignment;
  assignment.set_value(assignment_token);
  setup.scope()->SetValue("on_root", Value(&assignment, "on_root"),
                           &assignment);

  // Root scope should be const from the nested caller's perspective.
  Scope nested1(static_cast<const Scope*>(setup.scope()));
  nested1.SetValue("on_one", Value(&assignment, "on_one"), &assignment);

  Scope nested2(&nested1);
  nested2.SetValue("on_one", Value(&assignment, "on_two"), &assignment);
  nested2.SetValue("on_two", Value(&assignment, "on_two2"), &assignment);

  // Making a closure from the root scope.
  std::unique_ptr<Scope> result = setup.scope()->MakeClosure();
  EXPECT_FALSE(result->containing());  // Should have no containing scope.
  EXPECT_TRUE(result->GetValue("on_root"));  // Value should be copied.

  // Making a closure from the second nested scope.
  result = nested2.MakeClosure();
  EXPECT_EQ(setup.scope(),
            result->containing());  // Containing scope should be the root.
  EXPECT_TRUE(HasStringValueEqualTo(result.get(), "on_root", "on_root"));
  EXPECT_TRUE(HasStringValueEqualTo(result.get(), "on_one", "on_two"));
  EXPECT_TRUE(HasStringValueEqualTo(result.get(), "on_two", "on_two2"));
}

TEST(Scope, GetMutableValue) {
  TestWithScope setup;

  // Make a pretend parse node with proper tracking that we can blame for the
  // given value.
  InputFile input_file(SourceFile("//foo"));
  Token assignment_token(Location(&input_file, 1, 1, 1), Token::STRING,
      "\"hello\"");
  LiteralNode assignment;
  assignment.set_value(assignment_token);

  const char kOnConst[] = "on_const";
  const char kOnMutable1[] = "on_mutable1";
  const char kOnMutable2[] = "on_mutable2";

  Value value(&assignment, "hello");

  // Create a root scope with one value.
  Scope root_scope(setup.settings());
  root_scope.SetValue(kOnConst, value, &assignment);

  // Create a first nested scope with a different value.
  const Scope* const_root_scope = &root_scope;
  Scope mutable_scope1(const_root_scope);
  mutable_scope1.SetValue(kOnMutable1, value, &assignment);

  // Create a second nested scope with a different value.
  Scope mutable_scope2(&mutable_scope1);
  mutable_scope2.SetValue(kOnMutable2, value, &assignment);

  // Check getting root scope values.
  EXPECT_TRUE(mutable_scope2.GetValue(kOnConst, true));
  EXPECT_FALSE(mutable_scope2.GetMutableValue(
      kOnConst, Scope::SEARCH_NESTED, true));

  // Test reading a value from scope 1.
  Value* mutable1_result = mutable_scope2.GetMutableValue(
      kOnMutable1, Scope::SEARCH_NESTED, false);
  ASSERT_TRUE(mutable1_result);
  EXPECT_TRUE(*mutable1_result == value);

  // Make sure CheckForUnusedVars works on scope1 (we didn't mark the value as
  // used in the previous step).
  Err err;
  EXPECT_FALSE(mutable_scope1.CheckForUnusedVars(&err));
  mutable1_result = mutable_scope2.GetMutableValue(
      kOnMutable1, Scope::SEARCH_NESTED, true);
  EXPECT_TRUE(mutable1_result);
  err = Err();
  EXPECT_TRUE(mutable_scope1.CheckForUnusedVars(&err));

  // Test reading a value from scope 2.
  Value* mutable2_result = mutable_scope2.GetMutableValue(
      kOnMutable2, Scope::SEARCH_NESTED, true);
  ASSERT_TRUE(mutable2_result);
  EXPECT_TRUE(*mutable2_result == value);
}

TEST(Scope, RemovePrivateIdentifiers) {
  TestWithScope setup;
  setup.scope()->SetValue("a", Value(nullptr, true), nullptr);
  setup.scope()->SetValue("_b", Value(nullptr, true), nullptr);

  setup.scope()->RemovePrivateIdentifiers();
  EXPECT_TRUE(setup.scope()->GetValue("a"));
  EXPECT_FALSE(setup.scope()->GetValue("_b"));
}
