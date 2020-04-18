// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/test/mojo_test_with_file_service.h"

#include "services/file/file_service.h"
#include "services/file/public/mojom/constants.mojom.h"
#include "services/file/user_id_map.h"

namespace content {
namespace test {

MojoTestWithFileService::MojoTestWithFileService() {
  ResetFileServiceAndConnector(file::CreateFileService());
}
MojoTestWithFileService::~MojoTestWithFileService() = default;

void MojoTestWithFileService::ResetFileServiceAndConnector(
    std::unique_ptr<service_manager::Service> service) {
  if (!temp_path_.IsValid())
    CHECK(temp_path_.CreateUniqueTempDir());
  test_connector_ =
      service_manager::TestConnectorFactory::CreateForUniqueService(
          std::move(service));
  connector_ = test_connector_->CreateConnector();
  file::AssociateServiceUserIdWithUserDir(test_connector_->test_user_id(),
                                          temp_path_.GetPath());
}

}  // namespace test
}  // namespace content
