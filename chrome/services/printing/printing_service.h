// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_PRINTING_PRINTING_SERVICE_H_
#define CHROME_SERVICES_PRINTING_PRINTING_SERVICE_H_

#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace printing {

class PrintingService : public service_manager::Service {
 public:
  PrintingService();
  ~PrintingService() override;

  // Factory method for creating the service.
  static std::unique_ptr<service_manager::Service> CreateService();

  // Lifescycle events that occur after the service has started to spinup.
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

 private:
  // State needed to manage service lifecycle and lifecycle of bound clients.
  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(PrintingService);
};

}  // namespace printing

#endif  // CHROME_SERVICES_PRINTING_PRINTING_SERVICE_H_
