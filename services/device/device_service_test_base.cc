// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/device_service_test_base.h"

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "device/geolocation/public/cpp/location_provider.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/url_request/url_request_test_util.h"
#include "services/device/device_service.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/mojom/service_factory.mojom.h"

namespace device {

namespace {

const char kTestServiceName[] = "device_unittests";

// Simple request context producer that immediately produces a
// TestURLRequestContextGetter.
void TestRequestContextProducer(
    const scoped_refptr<base::SingleThreadTaskRunner>& network_task_runner,
    base::OnceCallback<void(scoped_refptr<net::URLRequestContextGetter>)>
        response_callback) {
  std::move(response_callback)
      .Run(base::MakeRefCounted<net::TestURLRequestContextGetter>(
          network_task_runner));
}

// Simply return a nullptr which means no CustomLocationProvider from embedder.
std::unique_ptr<LocationProvider> GetCustomLocationProviderForTest() {
  return nullptr;
}

// The test service responsible to package Device Service.
class ServiceTestClient : public service_manager::test::ServiceTestClient,
                          public service_manager::mojom::ServiceFactory {
 public:
  explicit ServiceTestClient(
      service_manager::test::ServiceTest* test,
      scoped_refptr<base::SingleThreadTaskRunner> file_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
      : service_manager::test::ServiceTestClient(test),
        file_task_runner_(std::move(file_task_runner)),
        io_task_runner_(std::move(io_task_runner)) {
    registry_.AddInterface<service_manager::mojom::ServiceFactory>(
        base::Bind(&ServiceTestClient::Create, base::Unretained(this)));
  }
  ~ServiceTestClient() override {}

 protected:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  void CreateService(
      service_manager::mojom::ServiceRequest request,
      const std::string& name,
      service_manager::mojom::PIDReceiverPtr pid_receiver) override {
    if (name == device::mojom::kServiceName) {
#if defined(OS_ANDROID)
      device_service_context_.reset(new service_manager::ServiceContext(
          CreateDeviceService(
              file_task_runner_, io_task_runner_,
              base::Bind(&TestRequestContextProducer, io_task_runner_),
              kTestGeolocationApiKey, wake_lock_context_callback_,
              base::Bind(&GetCustomLocationProviderForTest), nullptr),
          std::move(request)));
#else
      device_service_context_.reset(new service_manager::ServiceContext(
          CreateDeviceService(
              file_task_runner_, io_task_runner_,
              base::Bind(&TestRequestContextProducer, io_task_runner_),
              kTestGeolocationApiKey,
              base::Bind(&GetCustomLocationProviderForTest)),
          std::move(request)));
#endif
    }
  }

  void Create(service_manager::mojom::ServiceFactoryRequest request) {
    service_factory_bindings_.AddBinding(this, std::move(request));
  }

 private:
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<service_manager::mojom::ServiceFactory>
      service_factory_bindings_;
  std::unique_ptr<service_manager::ServiceContext> device_service_context_;
  scoped_refptr<base::SingleThreadTaskRunner> file_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  WakeLockContextCallback wake_lock_context_callback_;
};

}  // namespace

DeviceServiceTestBase::DeviceServiceTestBase()
    : ServiceTest(kTestServiceName),
      file_thread_("DeviceServiceTestFileThread"),
      io_thread_("DeviceServiceTestIOThread") {
  file_thread_.Start();
  io_thread_.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
}

DeviceServiceTestBase::~DeviceServiceTestBase() {}

std::unique_ptr<service_manager::Service>
DeviceServiceTestBase::CreateService() {
  return std::make_unique<ServiceTestClient>(this, file_thread_.task_runner(),
                                             io_thread_.task_runner());
}

}  // namespace device
