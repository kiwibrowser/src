// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_source.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/test/scoped_task_environment.h"
#include "components/exo/data_source_delegate.h"
#include "components/exo/test/exo_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace exo {
namespace {

constexpr char kTestData[] = "Test Data";

class DataSourceTest : public testing::Test {
 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_ = {
      base::test::ScopedTaskEnvironment::MainThreadType::DEFAULT,
      base::test::ScopedTaskEnvironment::ExecutionMode::QUEUED};
};

class TestDataSourceDelegate : public DataSourceDelegate {
 public:
  TestDataSourceDelegate() {}
  ~TestDataSourceDelegate() override {}

  // Overridden from DataSourceDelegate:
  void OnDataSourceDestroying(DataSource* source) override {}
  void OnTarget(const std::string& mime_type) override {}
  void OnSend(const std::string& mime_type, base::ScopedFD fd) override {
    ASSERT_TRUE(
        base::WriteFileDescriptor(fd.get(), kTestData, strlen(kTestData)));
  }
  void OnCancelled() override {}
  void OnDndDropPerformed() override {}
  void OnDndFinished() override {}
  void OnAction(DndAction dnd_action) override {}
};

TEST_F(DataSourceTest, ReadData) {
  TestDataSourceDelegate delegate;
  DataSource data_source(&delegate);
  data_source.Offer("text/plain;charset=utf-8");

  data_source.ReadData(base::BindOnce([](const std::vector<uint8_t>& data) {
    std::string string_data(data.begin(), data.end());
    EXPECT_EQ(std::string(kTestData), string_data);
  }));
  scoped_task_environment_.RunUntilIdle();
}

TEST_F(DataSourceTest, ReadData_UnknwonMimeType) {
  TestDataSourceDelegate delegate;
  DataSource data_source(&delegate);
  data_source.Offer("text/unknown");

  data_source.ReadData(base::BindOnce([](const std::vector<uint8_t>& data) {
    FAIL()
        << "Callback should not be invoked when known mimetype is not offerred";
  }));
  scoped_task_environment_.RunUntilIdle();
}

TEST_F(DataSourceTest, ReadData_Destroyed) {
  TestDataSourceDelegate delegate;
  {
    DataSource data_source(&delegate);
    data_source.Offer("text/plain;charset=utf-8");

    data_source.ReadData(base::BindOnce([](const std::vector<uint8_t>& data) {
      FAIL() << "Callback should not be invoked after data source is destroyed";
    }));
  }
  scoped_task_environment_.RunUntilIdle();
}

TEST_F(DataSourceTest, ReadData_Cancelled) {
  TestDataSourceDelegate delegate;
  DataSource data_source(&delegate);
  data_source.Offer("text/plain;charset=utf-8");

  data_source.ReadData(base::BindOnce([](const std::vector<uint8_t>& data) {
    FAIL() << "Callback should not be invoked after cancelled";
  }));
  data_source.Cancelled();
  scoped_task_environment_.RunUntilIdle();
}

}  // namespace
}  // namespace exo
