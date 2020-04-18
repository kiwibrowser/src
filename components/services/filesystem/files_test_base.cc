// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/filesystem/files_test_base.h"

#include <utility>

#include "components/services/filesystem/public/interfaces/directory.mojom.h"
#include "components/services/filesystem/public/interfaces/types.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace filesystem {

FilesTestBase::FilesTestBase() : ServiceTest("filesystem_service_unittests") {}

FilesTestBase::~FilesTestBase() {}

void FilesTestBase::SetUp() {
  ServiceTest::SetUp();
  connector()->BindInterface("filesystem", &files_);
}

void FilesTestBase::GetTemporaryRoot(mojom::DirectoryPtr* directory) {
  base::File::Error error = base::File::Error::FILE_ERROR_FAILED;
  bool handled = files()->OpenTempDirectory(MakeRequest(directory), &error);
  ASSERT_TRUE(handled);
  ASSERT_EQ(base::File::Error::FILE_OK, error);
}

}  // namespace filesystem
