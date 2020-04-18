// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/proxy_resolver/proxy_resolver_service.h"

#include <memory>

#include "base/test/scoped_task_environment.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace proxy_resolver {

class ProxyResolverServiceTest : public testing::Test {
 public:
  ProxyResolverServiceTest()
      : connector_factory_(
            service_manager::TestConnectorFactory::CreateForUniqueService(
                std::make_unique<ProxyResolverService>())),
        connector_(connector_factory_->CreateConnector()) {}

  ~ProxyResolverServiceTest() override {}

 protected:
  base::test::ScopedTaskEnvironment task_environment_;
  std::unique_ptr<service_manager::TestConnectorFactory> connector_factory_;
  std::unique_ptr<service_manager::Connector> connector_;
};

// Check that destroying the service while there's a live ProxyResolverFactory
// works. This makes sure that the ServiceContextRefFactory is destroyed before
// the underlying ProxyResolverFactoryImpl is.
TEST_F(ProxyResolverServiceTest, ShutdownServiceWithLiveProxyResolverFactory) {
  mojom::ProxyResolverFactoryPtr proxy_resolver_factory;
  connector_->BindInterface(mojom::kProxyResolverServiceName,
                            &proxy_resolver_factory);
  // Wait for the ProxyFactory to be bound.
  task_environment_.RunUntilIdle();

  // Simulate the service being destroyed. No crash should occur.
  connector_.reset();

  // Destroying the ProxyResolverFactory shouldn't result in a crash, either.
  proxy_resolver_factory.reset();
  task_environment_.RunUntilIdle();
}

}  // namespace proxy_resolver
