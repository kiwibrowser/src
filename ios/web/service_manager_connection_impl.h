// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_SERVICE_MANAGER_SERVICE_MANAGER_CONNECTION_IMPL_H_
#define IOS_WEB_SERVICE_MANAGER_SERVICE_MANAGER_CONNECTION_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "ios/web/public/service_manager_connection.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/cpp/identity.h"
#include "services/service_manager/public/mojom/service.mojom.h"

namespace service_manager {
class Connector;
}

namespace web {

class ServiceManagerConnectionImpl : public ServiceManagerConnection {
 public:
  ServiceManagerConnectionImpl(
      service_manager::mojom::ServiceRequest request,
      scoped_refptr<base::SequencedTaskRunner> io_task_runner);
  ~ServiceManagerConnectionImpl() override;

 private:
  class IOThreadContext;

  // ServiceManagerConnection:
  void Start() override;
  service_manager::Connector* GetConnector() override;
  void AddEmbeddedService(
      const std::string& name,
      const service_manager::EmbeddedServiceInfo& info) override;

  // Invoked when the connection to the Service Manager is lost.
  void OnConnectionLost();

  // Binds |request_handle| to an instance of |interface_name| provided by
  // |interface_provider|.
  void GetInterface(service_manager::mojom::InterfaceProvider* provider,
                    const std::string& interface_name,
                    mojo::ScopedMessagePipeHandle request_handle);

  std::unique_ptr<service_manager::Connector> connector_;

  scoped_refptr<IOThreadContext> context_;

  base::WeakPtrFactory<ServiceManagerConnectionImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceManagerConnectionImpl);
};

}  // namespace web

#endif  // IOS_WEB_SERVICE_MANAGER_SERVICE_MANAGER_CONNECTION_IMPL_H_
