// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/host_resolver_impl_chromeos.h"

#include <memory>

#include "base/location.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/shill_device_client.h"
#include "chromeos/dbus/shill_ipconfig_client.h"
#include "chromeos/dbus/shill_service_client.h"
#include "chromeos/network/device_state.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "dbus/object_path.h"
#include "net/base/net_errors.h"
#include "net/base/network_interfaces.h"
#include "net/log/net_log_with_source.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

const char kTestIPv4Address[] = "1.2.3.4";
const char kTestIPv6Address[] = "1:2:3:4:5:6:7:8";

void ErrorCallbackFunction(const std::string& error_name,
                           const std::string& error_message) {
  LOG(ERROR) << "Shill Error: " << error_name << " : " << error_message;
}
void ResolveCompletionCallback(int result) {}

}  // namespace

class HostResolverImplChromeOSTest : public testing::Test {
 public:
  HostResolverImplChromeOSTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO) {}

  ~HostResolverImplChromeOSTest() override = default;

  void SetUp() override {
    DBusThreadManager::Initialize();

    network_state_handler_ = NetworkStateHandler::InitializeForTest();
    base::RunLoop().RunUntilIdle();

    const NetworkState* default_network =
        network_state_handler_->DefaultNetwork();
    ASSERT_TRUE(default_network);
    const DeviceState* default_device =
        network_state_handler_->GetDeviceState(default_network->device_path());
    ASSERT_TRUE(default_device);
    SetDefaultIPConfigs(default_device->path());

    // Create the host resolver from the main thread loop.
    scoped_task_environment_.GetMainThreadTaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(&HostResolverImplChromeOSTest::InitializeHostResolver,
                       base::Unretained(this)));
    base::RunLoop().RunUntilIdle();
  }

  void TearDown() override {
    network_state_handler_->Shutdown();
    network_state_handler_.reset();
    DBusThreadManager::Shutdown();
  }

 protected:
  int CallResolve(net::HostResolver::RequestInfo& info) {
    scoped_task_environment_.GetMainThreadTaskRunner()->PostTask(
        FROM_HERE, base::BindOnce(&HostResolverImplChromeOSTest::Resolve,
                                  base::Unretained(this), info));
    base::RunLoop().RunUntilIdle();
    return result_;
  }

  net::AddressList addresses_;
  int result_;

 private:
  // Run from the main thread loop.
  void InitializeHostResolver() {
    net::HostResolver::Options options;
    host_resolver_ = HostResolverImplChromeOS::CreateHostResolverForTest(
        base::ThreadTaskRunnerHandle::Get(), network_state_handler_.get());
  }

  // Run from the main thread loop.
  void Resolve(net::HostResolver::RequestInfo info) {
    result_ = host_resolver_->Resolve(info, net::DEFAULT_PRIORITY, &addresses_,
                                      base::Bind(&ResolveCompletionCallback),
                                      &request_, net_log_);
  }

  void SetDefaultIPConfigs(const std::string& default_device_path) {
    const std::string kTestIPv4ConfigPath("test_ip_v4_config_path");
    const std::string kTestIPv6ConfigPath("test_ip_v6_config_path");

    SetIPConfig(kTestIPv4ConfigPath, shill::kTypeIPv4, kTestIPv4Address);
    SetIPConfig(kTestIPv6ConfigPath, shill::kTypeIPv6, kTestIPv6Address);
    base::RunLoop().RunUntilIdle();

    base::ListValue ip_configs;
    ip_configs.AppendString(kTestIPv4ConfigPath);
    ip_configs.AppendString(kTestIPv6ConfigPath);

    DBusThreadManager::Get()->GetShillDeviceClient()->SetProperty(
        dbus::ObjectPath(default_device_path), shill::kIPConfigsProperty,
        ip_configs, base::DoNothing(), base::Bind(&ErrorCallbackFunction));
    base::RunLoop().RunUntilIdle();
  }

  void SetIPConfig(const std::string& path,
                   const std::string& method,
                   const std::string& address) {
    DBusThreadManager::Get()->GetShillIPConfigClient()->SetProperty(
        dbus::ObjectPath(path), shill::kAddressProperty, base::Value(address),
        EmptyVoidDBusMethodCallback());
    DBusThreadManager::Get()->GetShillIPConfigClient()->SetProperty(
        dbus::ObjectPath(path), shill::kMethodProperty, base::Value(method),
        EmptyVoidDBusMethodCallback());
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<NetworkStateHandler> network_state_handler_;
  std::unique_ptr<net::HostResolver> host_resolver_;
  net::NetLogWithSource net_log_;
  std::unique_ptr<net::HostResolver::Request> request_;

  DISALLOW_COPY_AND_ASSIGN(HostResolverImplChromeOSTest);
};

TEST_F(HostResolverImplChromeOSTest, Resolve) {
  net::HostResolver::RequestInfo info(
      net::HostPortPair(net::GetHostName(), 80));
  info.set_address_family(net::ADDRESS_FAMILY_IPV4);
  info.set_is_my_ip_address(true);
  EXPECT_EQ(net::OK, CallResolve(info));
  ASSERT_EQ(1u, addresses_.size());
  std::string expected = base::StringPrintf("%s:%d", kTestIPv4Address, 0);
  EXPECT_EQ(expected, addresses_[0].ToString());

  info.set_address_family(net::ADDRESS_FAMILY_IPV6);
  EXPECT_EQ(net::OK, CallResolve(info));
  ASSERT_EQ(2u, addresses_.size());
  expected = base::StringPrintf("[%s]:%d", kTestIPv6Address, 0);
  EXPECT_EQ(expected, addresses_[0].ToString());
  expected = base::StringPrintf("%s:%d", kTestIPv4Address, 0);
  EXPECT_EQ(expected, addresses_[1].ToString());
}

}  // namespace chromeos
