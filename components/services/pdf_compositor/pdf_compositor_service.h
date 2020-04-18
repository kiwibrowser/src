// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_PDF_COMPOSITOR_PDF_COMPOSITOR_SERVICE_H_
#define COMPONENTS_SERVICES_PDF_COMPOSITOR_PDF_COMPOSITOR_SERVICE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/discardable_memory/client/client_discardable_shared_memory_manager.h"
#include "components/services/pdf_compositor/public/interfaces/pdf_compositor.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace printing {

class PdfCompositorService : public service_manager::Service {
 public:
  explicit PdfCompositorService(const std::string& creator);
  ~PdfCompositorService() override;

  // Factory function for use as an embedded service.
  static std::unique_ptr<service_manager::Service> Create(
      const std::string& creator);

  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  virtual void PrepareToStart();

 private:
  // The creator of this service.
  // Currently contains the service creator's user agent string if given,
  // otherwise just use string "Chromium".
  const std::string creator_;

  std::unique_ptr<discardable_memory::ClientDiscardableSharedMemoryManager>
      discardable_shared_memory_manager_;
  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  service_manager::BinderRegistry registry_;
  base::WeakPtrFactory<PdfCompositorService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PdfCompositorService);
};

}  // namespace printing

#endif  // COMPONENTS_SERVICES_PDF_COMPOSITOR_PDF_COMPOSITOR_SERVICE_H_
