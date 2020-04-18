// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/visual_studio_writer.h"

#include <memory>

#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/test_with_scope.h"
#include "tools/gn/visual_studio_utils.h"

namespace {

class VisualStudioWriterTest : public testing::Test {
 protected:
  TestWithScope setup_;
};

std::string MakeTestPath(const std::string& path) {
#if defined(OS_WIN)
  return "C:" + path;
#else
  return path;
#endif
}

}  // namespace

TEST_F(VisualStudioWriterTest, ResolveSolutionFolders) {
  VisualStudioWriter writer(setup_.build_settings(), "Win32",
                            VisualStudioWriter::Version::Vs2015,
                            "10.0.17134.0");

  std::string path =
      MakeTestPath("/foo/chromium/src/out/Debug/obj/base/base.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "base", path, MakeGuid(path, "project"),
          MakeTestPath("/foo/chromium/src/base"), "Win32"));

  path = MakeTestPath("/foo/chromium/src/out/Debug/obj/tools/gn/gn.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "gn", path, MakeGuid(path, "project"),
          MakeTestPath("/foo/chromium/src/tools/gn"), "Win32"));

  path = MakeTestPath("/foo/chromium/src/out/Debug/obj/chrome/chrome.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "chrome", path, MakeGuid(path, "project"),
          MakeTestPath("/foo/chromium/src/chrome"), "Win32"));

  path = MakeTestPath("/foo/chromium/src/out/Debug/obj/base/bar.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "bar", path, MakeGuid(path, "project"),
          MakeTestPath("/foo/chromium/src/base"), "Win32"));

  writer.ResolveSolutionFolders();

  ASSERT_EQ(MakeTestPath("/foo/chromium/src"), writer.root_folder_path_);

  ASSERT_EQ(4u, writer.folders_.size());

  ASSERT_EQ("base", writer.folders_[0]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/base"), writer.folders_[0]->path);
  ASSERT_EQ(nullptr, writer.folders_[0]->parent_folder);

  ASSERT_EQ("chrome", writer.folders_[1]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/chrome"), writer.folders_[1]->path);
  ASSERT_EQ(nullptr, writer.folders_[1]->parent_folder);

  ASSERT_EQ("tools", writer.folders_[2]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/tools"), writer.folders_[2]->path);
  ASSERT_EQ(nullptr, writer.folders_[2]->parent_folder);

  ASSERT_EQ("gn", writer.folders_[3]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/tools/gn"),
            writer.folders_[3]->path);
  ASSERT_EQ(writer.folders_[2].get(), writer.folders_[3]->parent_folder);

  ASSERT_EQ(writer.folders_[0].get(), writer.projects_[0]->parent_folder);
  ASSERT_EQ(writer.folders_[3].get(), writer.projects_[1]->parent_folder);
  ASSERT_EQ(writer.folders_[1].get(), writer.projects_[2]->parent_folder);
  ASSERT_EQ(writer.folders_[0].get(), writer.projects_[3]->parent_folder);
}

TEST_F(VisualStudioWriterTest, ResolveSolutionFolders_AbsPath) {
  VisualStudioWriter writer(setup_.build_settings(), "Win32",
                            VisualStudioWriter::Version::Vs2015,
                            "10.0.17134.0");

  std::string path =
      MakeTestPath("/foo/chromium/src/out/Debug/obj/base/base.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "base", path, MakeGuid(path, "project"),
          MakeTestPath("/foo/chromium/src/base"), "Win32"));

  path = MakeTestPath("/foo/chromium/src/out/Debug/obj/tools/gn/gn.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "gn", path, MakeGuid(path, "project"),
          MakeTestPath("/foo/chromium/src/tools/gn"), "Win32"));

  path = MakeTestPath(
      "/foo/chromium/src/out/Debug/obj/ABS_PATH/C/foo/bar/bar.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "bar", path, MakeGuid(path, "project"), MakeTestPath("/foo/bar"),
          "Win32"));

  std::string baz_label_dir_path = MakeTestPath("/foo/bar/baz");
#if defined(OS_WIN)
  // Make sure mixed lower and upper-case drive letters are handled properly.
  baz_label_dir_path[0] = base::ToLowerASCII(baz_label_dir_path[0]);
#endif
  path = MakeTestPath(
      "/foo/chromium/src/out/Debug/obj/ABS_PATH/C/foo/bar/baz/baz.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "baz", path, MakeGuid(path, "project"), baz_label_dir_path, "Win32"));

  writer.ResolveSolutionFolders();

  ASSERT_EQ(MakeTestPath("/foo"), writer.root_folder_path_);

  ASSERT_EQ(7u, writer.folders_.size());

  ASSERT_EQ("bar", writer.folders_[0]->name);
  ASSERT_EQ(MakeTestPath("/foo/bar"), writer.folders_[0]->path);
  ASSERT_EQ(nullptr, writer.folders_[0]->parent_folder);

  ASSERT_EQ("baz", writer.folders_[1]->name);
  ASSERT_EQ(MakeTestPath("/foo/bar/baz"), writer.folders_[1]->path);
  ASSERT_EQ(writer.folders_[0].get(), writer.folders_[1]->parent_folder);

  ASSERT_EQ("chromium", writer.folders_[2]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium"), writer.folders_[2]->path);
  ASSERT_EQ(nullptr, writer.folders_[2]->parent_folder);

  ASSERT_EQ("src", writer.folders_[3]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src"), writer.folders_[3]->path);
  ASSERT_EQ(writer.folders_[2].get(), writer.folders_[3]->parent_folder);

  ASSERT_EQ("base", writer.folders_[4]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/base"), writer.folders_[4]->path);
  ASSERT_EQ(writer.folders_[3].get(), writer.folders_[4]->parent_folder);

  ASSERT_EQ("tools", writer.folders_[5]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/tools"), writer.folders_[5]->path);
  ASSERT_EQ(writer.folders_[3].get(), writer.folders_[5]->parent_folder);

  ASSERT_EQ("gn", writer.folders_[6]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/tools/gn"),
            writer.folders_[6]->path);
  ASSERT_EQ(writer.folders_[5].get(), writer.folders_[6]->parent_folder);

  ASSERT_EQ(writer.folders_[4].get(), writer.projects_[0]->parent_folder);
  ASSERT_EQ(writer.folders_[6].get(), writer.projects_[1]->parent_folder);
  ASSERT_EQ(writer.folders_[0].get(), writer.projects_[2]->parent_folder);
  ASSERT_EQ(writer.folders_[1].get(), writer.projects_[3]->parent_folder);
}
