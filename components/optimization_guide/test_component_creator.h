// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OPTIMIZATION_GUIDE_TEST_COMPONENT_CREATOR_H_
#define COMPONENTS_OPTIMIZATION_GUIDE_TEST_COMPONENT_CREATOR_H_

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "components/optimization_guide/optimization_guide_service.h"

namespace optimization_guide {
namespace testing {

// Helper class to create test OptimizationHints components for testing.
//
// All temporary files and paths are cleaned up when this instance goes out of
// scope.
class TestComponentCreator {
 public:
  TestComponentCreator();
  ~TestComponentCreator();

  // Creates component data based on |whitelisted_host_suffixes| to a temp file
  // and returns the ComponentInfo for it.
  optimization_guide::ComponentInfo CreateComponentInfoWithNoScriptWhitelist(
      std::vector<std::string> whitelisted_host_suffixes);

 private:
  // Returns the scoped temp directory path with the |file_path_suffix| that is
  // valid for the lifetime of this instance.
  // The file itself will not be automatically created.
  base::FilePath GetFilePath(std::string file_path_suffix);

  // Writes a configuration of hints with the
  // |whitelisted_noscript_host_suffixes| to the file path.
  void WriteConfigToFile(
      base::FilePath file_path,
      std::vector<std::string> whitelisted_noscript_host_suffixes);

  std::unique_ptr<base::ScopedTempDir> scoped_temp_dir_;
  int next_component_version_;

  DISALLOW_COPY_AND_ASSIGN(TestComponentCreator);
};

}  // namespace testing
}  // namespace optimization_guide

#endif  // COMPONENTS_OPTIMIZATION_GUIDE_TEST_COMPONENT_CREATOR_H_
