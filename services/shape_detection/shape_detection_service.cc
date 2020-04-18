// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/shape_detection_service.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/shape_detection/barcode_detection_impl.h"
#if defined(OS_WIN)
#include "services/shape_detection/face_detection_provider_win.h"
#elif defined(OS_MACOSX)
#include "services/shape_detection/face_detection_provider_mac.h"
#else
#include "services/shape_detection/face_detection_provider_impl.h"
#endif
#include "services/shape_detection/text_detection_impl.h"

#if defined(OS_ANDROID)
#include "base/android/jni_android.h"
#include "jni/InterfaceRegistrar_jni.h"
#endif

namespace shape_detection {

std::unique_ptr<service_manager::Service> ShapeDetectionService::Create() {
  return std::make_unique<ShapeDetectionService>();
}

ShapeDetectionService::ShapeDetectionService() = default;

ShapeDetectionService::~ShapeDetectionService() = default;

void ShapeDetectionService::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      context()->CreateQuitClosure()));

#if defined(OS_ANDROID)
  registry_.AddInterface(
      GetJavaInterfaces()->CreateInterfaceFactory<mojom::BarcodeDetection>());
  registry_.AddInterface(
      GetJavaInterfaces()
          ->CreateInterfaceFactory<mojom::FaceDetectionProvider>());
  registry_.AddInterface(
      GetJavaInterfaces()->CreateInterfaceFactory<mojom::TextDetection>());
#elif defined(OS_WIN)
  registry_.AddInterface(base::Bind(&BarcodeDetectionImpl::Create));
  registry_.AddInterface(base::Bind(&TextDetectionImpl::Create));
  registry_.AddInterface(base::Bind(&FaceDetectionProviderWin::Create));
#elif defined(OS_MACOSX)
  registry_.AddInterface(base::Bind(&BarcodeDetectionImpl::Create));
  registry_.AddInterface(base::Bind(&TextDetectionImpl::Create));
  registry_.AddInterface(base::Bind(&FaceDetectionProviderMac::Create));
#else
  registry_.AddInterface(base::Bind(&BarcodeDetectionImpl::Create));
  registry_.AddInterface(base::Bind(&TextDetectionImpl::Create));
  registry_.AddInterface(base::Bind(&FaceDetectionProviderImpl::Create));
#endif
}

void ShapeDetectionService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

#if defined(OS_ANDROID)
service_manager::InterfaceProvider* ShapeDetectionService::GetJavaInterfaces() {
  if (!java_interface_provider_) {
    service_manager::mojom::InterfaceProviderPtr provider;
    Java_InterfaceRegistrar_createInterfaceRegistryForContext(
        base::android::AttachCurrentThread(),
        mojo::MakeRequest(&provider).PassMessagePipe().release().value());
    java_interface_provider_ =
        std::make_unique<service_manager::InterfaceProvider>();
    java_interface_provider_->Bind(std::move(provider));
  }
  return java_interface_provider_.get();
}
#endif

}  // namespace shape_detection
