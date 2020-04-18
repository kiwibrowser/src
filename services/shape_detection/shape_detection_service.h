// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_SHAPE_DETECTION_SERVICE_H_
#define SERVICES_SHAPE_DETECTION_SHAPE_DETECTION_SERVICE_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace shape_detection {

class ShapeDetectionService : public service_manager::Service {
 public:
  // Factory function for use as an embedded service.
  static std::unique_ptr<service_manager::Service> Create();

  ShapeDetectionService();
  ~ShapeDetectionService() override;

  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;
 private:
#if defined(OS_ANDROID)
  // Binds |java_interface_provider_| to an interface registry that exposes
  // factories for the interfaces that are provided via Java on Android.
  service_manager::InterfaceProvider* GetJavaInterfaces();

  // InterfaceProvider that is bound to the Java-side interface registry.
  std::unique_ptr<service_manager::InterfaceProvider> java_interface_provider_;
#endif

  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(ShapeDetectionService);
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_SHAPE_DETECTION_SERVICE_H_
