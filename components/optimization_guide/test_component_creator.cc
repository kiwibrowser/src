// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/optimization_guide/test_component_creator.h"

#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/version.h"
#include "components/optimization_guide/proto/hints.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace optimization_guide {
namespace testing {

TestComponentCreator::TestComponentCreator()
    : scoped_temp_dir_(std::make_unique<base::ScopedTempDir>()),
      next_component_version_(1) {}

TestComponentCreator::~TestComponentCreator() {
  base::ScopedAllowBlockingForTesting allow_blocking;
  scoped_temp_dir_.reset();
}

optimization_guide::ComponentInfo
TestComponentCreator::CreateComponentInfoWithNoScriptWhitelist(
    std::vector<std::string> whitelisted_host_suffixes) {
  std::string version_string = base::IntToString(next_component_version_++);
  base::FilePath hints_path = GetFilePath(version_string);
  WriteConfigToFile(hints_path, whitelisted_host_suffixes);
  return optimization_guide::ComponentInfo(base::Version(version_string),
                                           hints_path);
}

base::FilePath TestComponentCreator::GetFilePath(std::string file_path_suffix) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_TRUE(scoped_temp_dir_->IsValid() ||
              scoped_temp_dir_->CreateUniqueTempDir());
  return scoped_temp_dir_->GetPath().AppendASCII(file_path_suffix);
}

void TestComponentCreator::WriteConfigToFile(
    base::FilePath file_path,
    std::vector<std::string> whitelisted_noscript_host_suffixes) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  optimization_guide::proto::Configuration config;
  for (auto whitelisted_noscript_site : whitelisted_noscript_host_suffixes) {
    optimization_guide::proto::Hint* hint = config.add_hints();
    hint->set_key(whitelisted_noscript_site);
    hint->set_key_representation(optimization_guide::proto::HOST_SUFFIX);
    optimization_guide::proto::Optimization* optimization =
        hint->add_whitelisted_optimizations();
    optimization->set_optimization_type(optimization_guide::proto::NOSCRIPT);
  }

  std::string serialized_config;
  ASSERT_TRUE(config.SerializeToString(&serialized_config));

  ASSERT_EQ(static_cast<int32_t>(serialized_config.length()),
            base::WriteFile(file_path, serialized_config.data(),
                            serialized_config.length()));
}

}  // namespace testing
}  // namespace optimization_guide
