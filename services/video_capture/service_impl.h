// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIDEO_CAPTURE_SERVICE_IMPL_H_
#define SERVICES_VIDEO_CAPTURE_SERVICE_IMPL_H_

#include <memory>

#include "base/threading/thread_checker.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "services/video_capture/device_factory_provider_impl.h"
#include "services/video_capture/public/mojom/device_factory_provider.mojom.h"
#include "services/video_capture/public/mojom/testing_controls.mojom.h"

#if defined(OS_WIN)
#include "base/win/scoped_com_initializer.h"
#endif

namespace video_capture {

class ServiceImpl : public service_manager::Service {
 public:
  ServiceImpl();
  ~ServiceImpl() override;

  static std::unique_ptr<service_manager::Service> Create();

  // service_manager::Service implementation.
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;
  bool OnServiceManagerConnectionLost() override;

  void SetFactoryProviderClientDisconnectedObserver(
      const base::RepeatingClosure& observer_cb);

 private:
  void OnDeviceFactoryProviderRequest(
      mojom::DeviceFactoryProviderRequest request);
  void OnTestingControlsRequest(mojom::TestingControlsRequest request);
  void SetShutdownDelayInSeconds(float seconds);
  void MaybeRequestQuitDelayed();
  void MaybeRequestQuit();
  void LazyInitializeDeviceFactoryProvider();
  void OnProviderClientDisconnected();

#if defined(OS_WIN)
  // COM must be initialized in order to access the video capture devices.
  base::win::ScopedCOMInitializer com_initializer_;
#endif
  float shutdown_delay_in_seconds_;
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<mojom::DeviceFactoryProvider> factory_provider_bindings_;
  std::unique_ptr<DeviceFactoryProviderImpl> device_factory_provider_;
  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  // Callback to be invoked when a provider client is disconnected. Mainly used
  // for testing.
  base::RepeatingClosure factory_provider_client_disconnected_cb_;
  base::ThreadChecker thread_checker_;
  base::WeakPtrFactory<ServiceImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceImpl);
};

}  // namespace video_capture

#endif  // SERVICES_VIDEO_CAPTURE_SERVICE_IMPL_H_
