// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/stl_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/runtime_deps.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/target.h"
#include "tools/gn/test_with_scheduler.h"
#include "tools/gn/test_with_scope.h"

namespace {

void InitTargetWithType(TestWithScope& setup,
                        Target* target,
                        Target::OutputType type) {
  target->set_output_type(type);
  target->visibility().SetPublic();
  target->SetToolchain(setup.toolchain());
}

// Convenience function to make the correct kind of pair.
std::pair<OutputFile, const Target*> MakePair(const char* str,
                                              const Target* t) {
  return std::pair<OutputFile, const Target*>(OutputFile(str), t);
}

std::string GetVectorDescription(
    const std::vector<std::pair<OutputFile, const Target*>>& v) {
  std::string result;
  for (size_t i = 0; i < v.size(); i++) {
    if (i != 0)
      result.append(", ");
    result.append("\"" + v[i].first.value() + "\"");
  }
  return result;
}

}  // namespace

using RuntimeDeps = TestWithScheduler;

// Tests an exe depending on different types of libraries.
TEST_F(RuntimeDeps, Libs) {
  TestWithScope setup;
  Err err;

  // Dependency hierarchy: main(exe) -> static library
  //                                 -> shared library
  //                                 -> loadable module
  //                                 -> source set

  Target stat(setup.settings(), Label(SourceDir("//"), "stat"));
  InitTargetWithType(setup, &stat, Target::STATIC_LIBRARY);
  stat.data().push_back("//stat.dat");
  ASSERT_TRUE(stat.OnResolved(&err));

  Target shared(setup.settings(), Label(SourceDir("//"), "shared"));
  InitTargetWithType(setup, &shared, Target::SHARED_LIBRARY);
  shared.data().push_back("//shared.dat");
  ASSERT_TRUE(shared.OnResolved(&err));

  Target loadable(setup.settings(), Label(SourceDir("//"), "loadable"));
  InitTargetWithType(setup, &loadable, Target::LOADABLE_MODULE);
  loadable.data().push_back("//loadable.dat");
  ASSERT_TRUE(loadable.OnResolved(&err));

  Target set(setup.settings(), Label(SourceDir("//"), "set"));
  InitTargetWithType(setup, &set, Target::SOURCE_SET);
  set.data().push_back("//set.dat");
  ASSERT_TRUE(set.OnResolved(&err));

  Target main(setup.settings(), Label(SourceDir("//"), "main"));
  InitTargetWithType(setup, &main, Target::EXECUTABLE);
  main.private_deps().push_back(LabelTargetPair(&stat));
  main.private_deps().push_back(LabelTargetPair(&shared));
  main.private_deps().push_back(LabelTargetPair(&loadable));
  main.private_deps().push_back(LabelTargetPair(&set));
  main.data().push_back("//main.dat");
  ASSERT_TRUE(main.OnResolved(&err));

  std::vector<std::pair<OutputFile, const Target*>> result =
      ComputeRuntimeDeps(&main);

  // The result should have deps of main, all 5 dat files, libshared.so, and
  // libloadable.so.
  ASSERT_EQ(8u, result.size()) << GetVectorDescription(result);

  // The first one should always be the main exe.
  EXPECT_TRUE(MakePair("./main", &main) == result[0]);

  // The rest of the ordering is undefined. First the data files.
  EXPECT_TRUE(base::ContainsValue(result, MakePair("../../stat.dat", &stat)))
      << GetVectorDescription(result);
  EXPECT_TRUE(
      base::ContainsValue(result, MakePair("../../shared.dat", &shared)))
      << GetVectorDescription(result);
  EXPECT_TRUE(
      base::ContainsValue(result, MakePair("../../loadable.dat", &loadable)))
      << GetVectorDescription(result);
  EXPECT_TRUE(base::ContainsValue(result, MakePair("../../set.dat", &set)))
      << GetVectorDescription(result);
  EXPECT_TRUE(base::ContainsValue(result, MakePair("../../main.dat", &main)))
      << GetVectorDescription(result);

  // Check the static library and loadable module.
  EXPECT_TRUE(base::ContainsValue(result, MakePair("./libshared.so", &shared)))
      << GetVectorDescription(result);
  EXPECT_TRUE(
      base::ContainsValue(result, MakePair("./libloadable.so", &loadable)))
      << GetVectorDescription(result);
}

// Tests that executables that aren't listed as data deps aren't included in
// the output, but executables that are data deps are included.
TEST_F(RuntimeDeps, ExeDataDep) {
  TestWithScope setup;
  Err err;

  // Dependency hierarchy: main(exe) -> datadep(exe) -> final_in(source set)
  //                                 -> dep(exe) -> final_out(source set)
  // The final_in/out targets each have data files. final_in's should be
  // included, final_out's should not be.

  Target final_in(setup.settings(), Label(SourceDir("//"), "final_in"));
  InitTargetWithType(setup, &final_in, Target::SOURCE_SET);
  final_in.data().push_back("//final_in.dat");
  ASSERT_TRUE(final_in.OnResolved(&err));

  Target datadep(setup.settings(), Label(SourceDir("//"), "datadep"));
  InitTargetWithType(setup, &datadep, Target::EXECUTABLE);
  datadep.private_deps().push_back(LabelTargetPair(&final_in));
  ASSERT_TRUE(datadep.OnResolved(&err));

  Target final_out(setup.settings(), Label(SourceDir("//"), "final_out"));
  InitTargetWithType(setup, &final_out, Target::SOURCE_SET);
  final_out.data().push_back("//final_out.dat");
  ASSERT_TRUE(final_out.OnResolved(&err));

  Target dep(setup.settings(), Label(SourceDir("//"), "dep"));
  InitTargetWithType(setup, &dep, Target::EXECUTABLE);
  dep.private_deps().push_back(LabelTargetPair(&final_out));
  ASSERT_TRUE(dep.OnResolved(&err));

  Target main(setup.settings(), Label(SourceDir("//"), "main"));
  InitTargetWithType(setup, &main, Target::EXECUTABLE);
  main.private_deps().push_back(LabelTargetPair(&dep));
  main.data_deps().push_back(LabelTargetPair(&datadep));
  ASSERT_TRUE(main.OnResolved(&err));

  std::vector<std::pair<OutputFile, const Target*>> result =
      ComputeRuntimeDeps(&main);

  // The result should have deps of main, datadep, final_in.dat
  ASSERT_EQ(3u, result.size()) << GetVectorDescription(result);

  // The first one should always be the main exe.
  EXPECT_TRUE(MakePair("./main", &main) == result[0]);

  // The rest of the ordering is undefined.
  EXPECT_TRUE(base::ContainsValue(result, MakePair("./datadep", &datadep)))
      << GetVectorDescription(result);
  EXPECT_TRUE(
      base::ContainsValue(result, MakePair("../../final_in.dat", &final_in)))
      << GetVectorDescription(result);
}

TEST_F(RuntimeDeps, ActionSharedLib) {
  TestWithScope setup;
  Err err;

  // Dependency hierarchy: main(exe) -> action -> datadep(shared library)
  //                                           -> dep(shared library)
  // Datadep should be included, dep should not be.

  Target dep(setup.settings(), Label(SourceDir("//"), "dep"));
  InitTargetWithType(setup, &dep, Target::SHARED_LIBRARY);
  ASSERT_TRUE(dep.OnResolved(&err));

  Target datadep(setup.settings(), Label(SourceDir("//"), "datadep"));
  InitTargetWithType(setup, &datadep, Target::SHARED_LIBRARY);
  ASSERT_TRUE(datadep.OnResolved(&err));

  Target action(setup.settings(), Label(SourceDir("//"), "action"));
  InitTargetWithType(setup, &action, Target::ACTION);
  action.private_deps().push_back(LabelTargetPair(&dep));
  action.data_deps().push_back(LabelTargetPair(&datadep));
  action.action_values().outputs() =
      SubstitutionList::MakeForTest("//action.output");
  ASSERT_TRUE(action.OnResolved(&err));

  Target main(setup.settings(), Label(SourceDir("//"), "main"));
  InitTargetWithType(setup, &main, Target::EXECUTABLE);
  main.private_deps().push_back(LabelTargetPair(&action));
  ASSERT_TRUE(main.OnResolved(&err));

  std::vector<std::pair<OutputFile, const Target*>> result =
      ComputeRuntimeDeps(&main);

  // The result should have deps of main and data_dep.
  ASSERT_EQ(2u, result.size()) << GetVectorDescription(result);

  // The first one should always be the main exe.
  EXPECT_TRUE(MakePair("./main", &main) == result[0]);
  EXPECT_TRUE(MakePair("./libdatadep.so", &datadep) == result[1]);
}

// Tests that action and copy outputs are considered if they're data deps, but
// not if they're regular deps. Action and copy "data" files are always
// included.
TEST_F(RuntimeDeps, ActionOutputs) {
  TestWithScope setup;
  Err err;

  // Dependency hierarchy: main(exe) -> datadep (action)
  //                                 -> datadep_copy (copy)
  //                                 -> dep (action)
  //                                 -> dep_copy (copy)

  Target datadep(setup.settings(), Label(SourceDir("//"), "datadep"));
  InitTargetWithType(setup, &datadep, Target::ACTION);
  datadep.data().push_back("//datadep.data");
  datadep.action_values().outputs() =
      SubstitutionList::MakeForTest("//datadep.output");
  ASSERT_TRUE(datadep.OnResolved(&err));

  Target datadep_copy(setup.settings(), Label(SourceDir("//"), "datadep_copy"));
  InitTargetWithType(setup, &datadep_copy, Target::COPY_FILES);
  datadep_copy.sources().push_back(SourceFile("//input"));
  datadep_copy.data().push_back("//datadep_copy.data");
  datadep_copy.action_values().outputs() =
      SubstitutionList::MakeForTest("//datadep_copy.output");
  ASSERT_TRUE(datadep_copy.OnResolved(&err));

  Target dep(setup.settings(), Label(SourceDir("//"), "dep"));
  InitTargetWithType(setup, &dep, Target::ACTION);
  dep.data().push_back("//dep.data");
  dep.action_values().outputs() =
      SubstitutionList::MakeForTest("//dep.output");
  ASSERT_TRUE(dep.OnResolved(&err));

  Target dep_copy(setup.settings(), Label(SourceDir("//"), "dep_copy"));
  InitTargetWithType(setup, &dep_copy, Target::COPY_FILES);
  dep_copy.sources().push_back(SourceFile("//input"));
  dep_copy.data().push_back("//dep_copy/data/");  // Tests a directory.
  dep_copy.action_values().outputs() =
      SubstitutionList::MakeForTest("//dep_copy.output");
  ASSERT_TRUE(dep_copy.OnResolved(&err));

  Target main(setup.settings(), Label(SourceDir("//"), "main"));
  InitTargetWithType(setup, &main, Target::EXECUTABLE);
  main.private_deps().push_back(LabelTargetPair(&dep));
  main.private_deps().push_back(LabelTargetPair(&dep_copy));
  main.data_deps().push_back(LabelTargetPair(&datadep));
  main.data_deps().push_back(LabelTargetPair(&datadep_copy));
  ASSERT_TRUE(main.OnResolved(&err));

  std::vector<std::pair<OutputFile, const Target*>> result =
      ComputeRuntimeDeps(&main);

  // The result should have deps of main, both datadeps files, but only
  // the data file from dep.
  ASSERT_EQ(7u, result.size()) << GetVectorDescription(result);

  // The first one should always be the main exe.
  EXPECT_TRUE(MakePair("./main", &main) == result[0]);

  // The rest of the ordering is undefined.
  EXPECT_TRUE(
      base::ContainsValue(result, MakePair("../../datadep.data", &datadep)))
      << GetVectorDescription(result);
  EXPECT_TRUE(base::ContainsValue(
      result, MakePair("../../datadep_copy.data", &datadep_copy)))
      << GetVectorDescription(result);
  EXPECT_TRUE(
      base::ContainsValue(result, MakePair("../../datadep.output", &datadep)))
      << GetVectorDescription(result);
  EXPECT_TRUE(base::ContainsValue(
      result, MakePair("../../datadep_copy.output", &datadep_copy)))
      << GetVectorDescription(result);
  EXPECT_TRUE(base::ContainsValue(result, MakePair("../../dep.data", &dep)))
      << GetVectorDescription(result);
  EXPECT_TRUE(
      base::ContainsValue(result, MakePair("../../dep_copy/data/", &dep_copy)))
      << GetVectorDescription(result);

  // Explicitly asking for the runtime deps of an action target only includes
  // the data and not all outputs.
  result = ComputeRuntimeDeps(&dep);
  ASSERT_EQ(1u, result.size());
  EXPECT_TRUE(MakePair("../../dep.data", &dep) == result[0]);
}

// Tests that the search for dependencies terminates at a bundle target,
// ignoring any shared libraries or loadable modules that get copied into the
// bundle.
TEST_F(RuntimeDeps, CreateBundle) {
  TestWithScope setup;
  Err err;

  // Dependency hierarchy:
  // main(exe) -> dep(bundle) -> dep(shared_library) -> dep(source set)
  //                          -> dep(bundle_data) -> dep(loadable_module)
  //                                                      -> data(lm.data)
  //                          -> datadep(datadep) -> data(dd.data)

  const SourceDir source_dir("//");
  const std::string& build_dir = setup.build_settings()->build_dir().value();

  Target loadable_module(setup.settings(),
                         Label(source_dir, "loadable_module"));
  InitTargetWithType(setup, &loadable_module, Target::LOADABLE_MODULE);
  loadable_module.data().push_back("//lm.data");
  ASSERT_TRUE(loadable_module.OnResolved(&err));

  Target module_data(setup.settings(), Label(source_dir, "module_data"));
  InitTargetWithType(setup, &module_data, Target::BUNDLE_DATA);
  module_data.private_deps().push_back(LabelTargetPair(&loadable_module));
  module_data.bundle_data().file_rules().push_back(BundleFileRule(
      nullptr,
      std::vector<SourceFile>{SourceFile(build_dir + "loadable_module.so")},
      SubstitutionPattern::MakeForTest("{{bundle_resources_dir}}")));
  ASSERT_TRUE(module_data.OnResolved(&err));

  Target source_set(setup.settings(), Label(source_dir, "sources"));
  InitTargetWithType(setup, &source_set, Target::SOURCE_SET);
  source_set.sources().push_back(SourceFile(source_dir.value() + "foo.cc"));
  ASSERT_TRUE(source_set.OnResolved(&err));

  Target dylib(setup.settings(), Label(source_dir, "dylib"));
  dylib.set_output_prefix_override(true);
  dylib.set_output_extension("");
  dylib.set_output_name("Bundle");
  InitTargetWithType(setup, &dylib, Target::SHARED_LIBRARY);
  dylib.private_deps().push_back(LabelTargetPair(&source_set));
  ASSERT_TRUE(dylib.OnResolved(&err));

  Target dylib_data(setup.settings(), Label(source_dir, "dylib_data"));
  InitTargetWithType(setup, &dylib_data, Target::BUNDLE_DATA);
  dylib_data.private_deps().push_back(LabelTargetPair(&dylib));
  dylib_data.bundle_data().file_rules().push_back(BundleFileRule(
      nullptr, std::vector<SourceFile>{SourceFile(build_dir + "dylib")},
      SubstitutionPattern::MakeForTest("{{bundle_executable_dir}}")));
  ASSERT_TRUE(dylib_data.OnResolved(&err));

  Target data_dep(setup.settings(), Label(source_dir, "datadep"));
  InitTargetWithType(setup, &data_dep, Target::EXECUTABLE);
  data_dep.data().push_back("//dd.data");
  ASSERT_TRUE(data_dep.OnResolved(&err));

  Target bundle(setup.settings(), Label(source_dir, "bundle"));
  InitTargetWithType(setup, &bundle, Target::CREATE_BUNDLE);
  const std::string root_dir(build_dir + "Bundle.framework/");
  const std::string contents_dir(root_dir + "Versions/A/");
  bundle.bundle_data().root_dir() = SourceDir(root_dir);
  bundle.bundle_data().contents_dir() = SourceDir(contents_dir);
  bundle.bundle_data().resources_dir() = SourceDir(contents_dir + "Resources");
  bundle.bundle_data().executable_dir() = SourceDir(contents_dir + "MacOS");
  bundle.private_deps().push_back(LabelTargetPair(&dylib_data));
  bundle.private_deps().push_back(LabelTargetPair(&module_data));
  bundle.data_deps().push_back(LabelTargetPair(&data_dep));
  bundle.data().push_back("//b.data");
  ASSERT_TRUE(bundle.OnResolved(&err));

  Target main(setup.settings(), Label(source_dir, "main"));
  InitTargetWithType(setup, &main, Target::EXECUTABLE);
  main.data_deps().push_back(LabelTargetPair(&bundle));
  ASSERT_TRUE(main.OnResolved(&err));

  std::vector<std::pair<OutputFile, const Target*>> result =
      ComputeRuntimeDeps(&main);

  // The result should have deps of main, datadep, final_in.dat
  ASSERT_EQ(5u, result.size()) << GetVectorDescription(result);

  // The first one should always be the main exe.
  EXPECT_EQ(MakePair("./main", &main), result[0]);

  // The rest of the ordering is undefined.

  // The framework bundle's internal dependencies should not be incldued.
  EXPECT_TRUE(
      base::ContainsValue(result, MakePair("Bundle.framework/", &bundle)))
      << GetVectorDescription(result);
  // But direct data and data dependencies should be.
  EXPECT_TRUE(base::ContainsValue(result, MakePair("./datadep", &data_dep)))
      << GetVectorDescription(result);
  EXPECT_TRUE(base::ContainsValue(result, MakePair("../../dd.data", &data_dep)))
      << GetVectorDescription(result);
  EXPECT_TRUE(base::ContainsValue(result, MakePair("../../b.data", &bundle)))
      << GetVectorDescription(result);
}

// Tests that a dependency duplicated in regular and data deps is processed
// as a data dep.
TEST_F(RuntimeDeps, Dupe) {
  TestWithScope setup;
  Err err;

  Target action(setup.settings(), Label(SourceDir("//"), "action"));
  InitTargetWithType(setup, &action, Target::ACTION);
  action.action_values().outputs() =
      SubstitutionList::MakeForTest("//action.output");
  ASSERT_TRUE(action.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//"), "foo"));
  InitTargetWithType(setup, &target, Target::EXECUTABLE);
  target.private_deps().push_back(LabelTargetPair(&action));
  target.data_deps().push_back(LabelTargetPair(&action));
  ASSERT_TRUE(target.OnResolved(&err));

  // The results should be the executable and the copy output.
  std::vector<std::pair<OutputFile, const Target*>> result =
      ComputeRuntimeDeps(&target);
  EXPECT_TRUE(
      base::ContainsValue(result, MakePair("../../action.output", &action)))
      << GetVectorDescription(result);
}

// Tests that actions can't have output substitutions.
TEST_F(RuntimeDeps, WriteRuntimeDepsVariable) {
  TestWithScope setup;
  Err err;

  // Should refuse to write files outside of the output dir.
  EXPECT_FALSE(setup.ExecuteSnippet(
      "group(\"foo\") { write_runtime_deps = \"//foo.txt\" }", &err));

  // Should fail for garbage inputs.
  err = Err();
  EXPECT_FALSE(setup.ExecuteSnippet(
      "group(\"foo\") { write_runtime_deps = 0 }", &err));

  // Should be able to write inside the out dir, and shouldn't write the one
  // in the else clause.
  err = Err();
  EXPECT_TRUE(setup.ExecuteSnippet(
      "if (true) {\n"
      "  group(\"foo\") { write_runtime_deps = \"//out/Debug/foo.txt\" }\n"
      "} else {\n"
      "  group(\"bar\") { write_runtime_deps = \"//out/Debug/bar.txt\" }\n"
      "}", &err));
  EXPECT_EQ(1U, setup.items().size());
  EXPECT_EQ(1U, scheduler().GetWriteRuntimeDepsTargets().size());
}
