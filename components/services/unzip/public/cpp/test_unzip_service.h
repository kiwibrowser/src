// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_UNZIP_PUBLIC_CPP_TEST_UNZIP_SERVICE_H_
#define COMPONENTS_SERVICES_UNZIP_PUBLIC_CPP_TEST_UNZIP_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "components/services/unzip/public/interfaces/unzipper.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/service.h"

namespace unzip {

// An implementation of the UnzipService that closes the connection when
// a call is made on the Unzipper interface.
// Can be used with a TestConnectorFactory to simulate crashes in the service
// while processing a call.
class CrashyUnzipService : public service_manager::Service,
                           public mojom::Unzipper {
 public:
  CrashyUnzipService();
  ~CrashyUnzipService() override;

  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  // unzip::mojom::Unzipper:
  void Unzip(base::File zip_file,
             filesystem::mojom::DirectoryPtr output_dir,
             UnzipCallback callback) override;
  void UnzipWithFilter(base::File zip_file,
                       filesystem::mojom::DirectoryPtr output_dir,
                       mojom::UnzipFilterPtr filter,
                       UnzipWithFilterCallback callback) override;

 private:
  std::unique_ptr<mojo::Binding<mojom::Unzipper>> unzipper_binding_;

  DISALLOW_COPY_AND_ASSIGN(CrashyUnzipService);
};

}  // namespace unzip

#endif  // COMPONENTS_SERVICES_UNZIP_PUBLIC_CPP_TEST_UNZIP_SERVICE_H_
