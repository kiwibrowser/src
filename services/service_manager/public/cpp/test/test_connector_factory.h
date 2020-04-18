// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_TEST_CONNECTOR_FACTORY_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_TEST_CONNECTOR_FACTORY_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/mojom/connector.mojom.h"

namespace service_manager {

class Service;

// Creates Connector instances which route BindInterface requests directly to
// one or several given Service implementations. Useful for testing production
// code which is parameterized over a Connector, while bypassing all the
// Service Manager machinery. Typical usage should look something like:
//
//     TEST(MyTest, Foo) {
//       // Your implementation of service_manager::Service.
//       auto impl =  std::make_unique<MyServiceImpl>();
//       std::unique_ptr<TestConnectorFactory> connector_factory =
//          TestConnectorFactory::CreateForUniqueService(std::move(impl));
//       std::unique_ptr<service_manager::Connector> connector =
//           connector_factory->CreateConnector();
//       RunSomeClientCode(connector.get());
//     }
//
// Where |RunSomeClientCode()| would typically be some production code that
// expects a functioning Connector and uses it to connect to the service you're
// testing.
// Note that when using CreateForUniqueService(), Connectors created by this
// factory ignores the target service name in BindInterface calls: interface
// requests are always routed to a single target Service instance.
// If you need to set-up more than one service, use CreateForServices(), like:
//   TestConnectorFactory::NameToServiceMap services;
//   services.insert(std::make_pair("data_decoder",
//                                  std::make_unique<DataDecoderService>()));
//   services.insert(std::make_pair("file",
//                                  std::make_unique<file::FileService>()));
//   std::unique_ptr<TestConnectorFactory> connector_factory =
//       TestConnectorFactory::CreateForServices(std::move(services));
//   std::unique_ptr<service_manager::Connector> connector =
//       connector_factory->CreateConnector();
//   ...
class TestConnectorFactory {
 public:
  ~TestConnectorFactory();

  using NameToServiceMap = std::map<std::string, std::unique_ptr<Service>>;

  // Constructs a new TestConnectorFactory which creates Connectors whose
  // requests are routed directly to |service|.
  static std::unique_ptr<TestConnectorFactory> CreateForUniqueService(
      std::unique_ptr<Service> service);

  // Constructs a new TestConnectorFactory which creates Connectors whose
  // requests are routed directly to a service in |services| based on the name
  // they are associated with.
  static std::unique_ptr<TestConnectorFactory> CreateForServices(
      NameToServiceMap services);

  // Creates a new connector which routes BindInterfaces requests directly to
  // the Service instance associated with this factory.
  std::unique_ptr<Connector> CreateConnector();

  const std::string& test_user_id() const { return test_user_id_; }

 private:
  explicit TestConnectorFactory(std::unique_ptr<mojom::Connector> impl,
                                std::string test_user_id);

  NameToServiceMap names_to_services_;

  std::unique_ptr<mojom::Connector> impl_;
  std::string test_user_id_;

  DISALLOW_COPY_AND_ASSIGN(TestConnectorFactory);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_CPP_TEST_TEST_CONNECTOR_FACTORY_H_
