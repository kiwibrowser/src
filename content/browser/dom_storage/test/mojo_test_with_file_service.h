// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_TEST_MOJO_TEST_WITH_FILE_SERVICE_H_
#define CONTENT_BROWSER_DOM_STORAGE_TEST_MOJO_TEST_WITH_FILE_SERVICE_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "services/service_manager/public/mojom/service_factory.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace test {

// Provides a connector that has the file service, and the user directory points
// to the temp_path() location.
class MojoTestWithFileService : public testing::Test {
 public:
  MojoTestWithFileService();
  ~MojoTestWithFileService() override;

 protected:
  void ResetFileServiceAndConnector(
      std::unique_ptr<service_manager::Service> service);

  const base::FilePath& temp_path() { return temp_path_.GetPath(); }

  service_manager::Connector* connector() const { return connector_.get(); }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<service_manager::TestConnectorFactory> test_connector_;
  std::unique_ptr<service_manager::Connector> connector_;
  base::ScopedTempDir temp_path_;

  DISALLOW_COPY_AND_ASSIGN(MojoTestWithFileService);
};

}  // namespace test
}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_TEST_MOJO_TEST_WITH_FILE_SERVICE_H_
