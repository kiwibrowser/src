// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_UNZIP_UNZIPPER_IMPL_H_
#define COMPONENTS_SERVICES_UNZIP_UNZIPPER_IMPL_H_

#include <memory>

#include "base/files/file.h"
#include "base/macros.h"
#include "components/services/unzip/public/interfaces/unzipper.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace unzip {

class UnzipperImpl : public mojom::Unzipper {
 public:
  explicit UnzipperImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~UnzipperImpl() override;

 private:
  // unzip::mojom::Unzipper:
  void Unzip(base::File zip_file,
             filesystem::mojom::DirectoryPtr output_dir,
             UnzipCallback callback) override;

  void UnzipWithFilter(base::File zip_file,
                       filesystem::mojom::DirectoryPtr output_dir,
                       mojom::UnzipFilterPtr filter,
                       UnzipWithFilterCallback callback) override;

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(UnzipperImpl);
};

}  // namespace unzip

#endif  // COMPONENTS_SERVICES_UNZIP_UNZIPPER_IMPL_H_
