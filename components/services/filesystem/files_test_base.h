// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_FILESYSTEM_FILES_TEST_BASE_H_
#define COMPONENTS_SERVICES_FILESYSTEM_FILES_TEST_BASE_H_

#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "components/services/filesystem/public/interfaces/file_system.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/service_test.h"

namespace filesystem {

template <typename... Args>
void IgnoreAllArgs(Args&&... args) {}

template <typename... Args>
void DoCaptures(Args*... out_args, Args... in_args) {
  IgnoreAllArgs((*out_args = std::move(in_args))...);
}

template <typename T1>
base::Callback<void(T1)> Capture(T1* t1) {
  return base::Bind(&DoCaptures<T1>, t1);
}

template <typename T1, typename T2>
base::Callback<void(T1, T2)> Capture(T1* t1, T2* t2) {
  return base::Bind(&DoCaptures<T1, T2>, t1, t2);
}

template <typename T1, typename T2, typename T3>
base::Callback<void(T1, T2, T3)> Capture(T1* t1, T2* t2, T3* t3) {
  return base::Bind(&DoCaptures<T1, T2, T3>, t1, t2, t3);
}

class FilesTestBase : public service_manager::test::ServiceTest {
 public:
  FilesTestBase();
  ~FilesTestBase() override;

  // Overridden from service_manager::test::ServiceTest:
  void SetUp() override;

 protected:
  // Note: This has an out parameter rather than returning the |DirectoryPtr|,
  // since |ASSERT_...()| doesn't work with return values.
  void GetTemporaryRoot(mojom::DirectoryPtr* directory);

  mojom::FileSystemPtr& files() { return files_; }

 private:
  mojom::FileSystemPtr files_;

  DISALLOW_COPY_AND_ASSIGN(FilesTestBase);
};

}  // namespace filesystem

#endif  // COMPONENTS_SERVICES_FILESYSTEM_FILES_TEST_BASE_H_
