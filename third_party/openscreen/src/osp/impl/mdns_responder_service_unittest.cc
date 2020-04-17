// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/mdns_responder_service.h"

#include <cstdint>
#include <memory>

#include "osp/impl/service_listener_impl.h"
#include "osp/impl/testing/fake_mdns_platform_service.h"
#include "osp/impl/testing/fake_mdns_responder_adapter.h"
#include "third_party/googletest/src/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace {

using ::testing::_;

constexpr char kTestServiceInstance[] = "turtle";
constexpr char kTestServiceName[] = "_foo";
constexpr char kTestServiceProtocol[] = "_udp";
constexpr char kTestHostname[] = "hostname";
constexpr uint16_t kTestPort = 12345;

class FakeMdnsResponderAdapterFactory final
    : public MdnsResponderAdapterFactory,
      public FakeMdnsResponderAdapter::LifetimeObserver {
 public:
  ~FakeMdnsResponderAdapterFactory() override = default;

  std::unique_ptr<mdns::MdnsResponderAdapter> Create() override {
    auto mdns = std::make_unique<FakeMdnsResponderAdapter>();
    mdns->SetLifetimeObserver(this);
    last_mdns_responder_ = mdns.get();
    ++instances_;
    return mdns;
  }

  void OnDestroyed() override {
    last_running_ = last_mdns_responder_->running();
    last_registered_services_size_ =
        last_mdns_responder_->registered_services().size();
    last_mdns_responder_ = nullptr;
  }

  FakeMdnsResponderAdapter* last_mdns_responder() {
    return last_mdns_responder_;
  }

  int32_t instances() const { return instances_; }
  bool last_running() const { return last_running_; }
  size_t last_registered_services_size() const {
    return last_registered_services_size_;
  }

 private:
  FakeMdnsResponderAdapter* last_mdns_responder_ = nullptr;
  int32_t instances_ = 0;
  bool last_running_ = false;
  size_t last_registered_services_size_ = 0;
};

class MockServiceListenerObserver final : public ServiceListener::Observer {
 public:
  ~MockServiceListenerObserver() override = default;

  MOCK_METHOD0(OnStarted, void());
  MOCK_METHOD0(OnStopped, void());
  MOCK_METHOD0(OnSuspended, void());
  MOCK_METHOD0(OnSearching, void());

  MOCK_METHOD1(OnReceiverAdded, void(const ServiceInfo&));
  MOCK_METHOD1(OnReceiverChanged, void(const ServiceInfo&));
  MOCK_METHOD1(OnReceiverRemoved, void(const ServiceInfo&));
  MOCK_METHOD0(OnAllReceiversRemoved, void());

  MOCK_METHOD1(OnError, void(ServiceListenerError));
  MOCK_METHOD1(OnMetrics, void(ServiceListener::Metrics));
};

class MockServicePublisherObserver final : public ServicePublisher::Observer {
 public:
  ~MockServicePublisherObserver() override = default;

  MOCK_METHOD0(OnStarted, void());
  MOCK_METHOD0(OnStopped, void());
  MOCK_METHOD0(OnSuspended, void());
  MOCK_METHOD1(OnError, void(ServicePublisherError));
  MOCK_METHOD1(OnMetrics, void(ServicePublisher::Metrics));
};

platform::UdpSocket* const kDefaultSocket =
    reinterpret_cast<platform::UdpSocket*>(static_cast<uintptr_t>(16));
platform::UdpSocket* const kSecondSocket =
    reinterpret_cast<platform::UdpSocket*>(static_cast<uintptr_t>(24));

class MdnsResponderServiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto mdns_responder_factory =
        std::make_unique<FakeMdnsResponderAdapterFactory>();
    mdns_responder_factory_ = mdns_responder_factory.get();
    auto platform_service = std::make_unique<FakeMdnsPlatformService>();
    fake_platform_service_ = platform_service.get();
    fake_platform_service_->set_interfaces(bound_interfaces_);
    mdns_service_ = std::make_unique<MdnsResponderService>(
        kTestServiceName, kTestServiceProtocol,
        std::move(mdns_responder_factory), std::move(platform_service));
    service_listener_ =
        std::make_unique<ServiceListenerImpl>(mdns_service_.get());
    service_listener_->AddObserver(&observer_);

    mdns_service_->SetServiceConfig(kTestHostname, kTestServiceInstance,
                                    kTestPort, {}, {{"model", "shifty"}});
    service_publisher_ = std::make_unique<ServicePublisherImpl>(
        &publisher_observer_, mdns_service_.get());
  }

  MockServiceListenerObserver observer_;
  FakeMdnsPlatformService* fake_platform_service_;
  FakeMdnsResponderAdapterFactory* mdns_responder_factory_;
  std::unique_ptr<MdnsResponderService> mdns_service_;
  std::unique_ptr<ServiceListenerImpl> service_listener_;
  MockServicePublisherObserver publisher_observer_;
  std::unique_ptr<ServicePublisherImpl> service_publisher_;
  const uint8_t default_mac_[6] = {0, 11, 22, 33, 44, 55};
  const uint8_t second_mac_[6] = {55, 33, 22, 33, 44, 77};
  std::vector<MdnsPlatformService::BoundInterface> bound_interfaces_{
      MdnsPlatformService::BoundInterface{
          platform::InterfaceInfo{1, default_mac_, "eth0",
                                  platform::InterfaceInfo::Type::kEthernet},
          platform::IPSubnet{IPAddress{192, 168, 3, 2}, 24}, kDefaultSocket},
      MdnsPlatformService::BoundInterface{
          platform::InterfaceInfo{2, second_mac_, "eth1",
                                  platform::InterfaceInfo::Type::kEthernet},
          platform::IPSubnet{IPAddress{10, 0, 0, 3}, 24}, kSecondSocket},
  };
};

}  // namespace

TEST_F(MdnsResponderServiceTest, BasicServiceStates) {
  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
  ASSERT_TRUE(mdns_responder);
  ASSERT_TRUE(mdns_responder->running());

  AddEventsForNewService(mdns_responder, kTestServiceInstance, kTestServiceName,
                         kTestServiceProtocol, "gigliorononomicon", kTestPort,
                         {"model=shifty", "id=asdf"}, IPAddress{192, 168, 3, 7},
                         kDefaultSocket);

  std::string service_id;
  EXPECT_CALL(observer_, OnReceiverAdded(_))
      .WillOnce(::testing::Invoke([&service_id](const ServiceInfo& info) {
        service_id = info.service_id;
        EXPECT_EQ(kTestServiceInstance, info.friendly_name);
        EXPECT_EQ((IPEndpoint{{192, 168, 3, 7}, kTestPort}), info.v4_endpoint);
        EXPECT_FALSE(info.v6_endpoint.address);
      }));
  mdns_service_->HandleNewEvents({});

  mdns_responder->AddAEvent(MakeAEvent(
      "gigliorononomicon", IPAddress{192, 168, 3, 8}, kDefaultSocket));

  EXPECT_CALL(observer_, OnReceiverChanged(_))
      .WillOnce(::testing::Invoke([&service_id](const ServiceInfo& info) {
        EXPECT_EQ(service_id, info.service_id);
        EXPECT_EQ(kTestServiceInstance, info.friendly_name);
        EXPECT_EQ((IPEndpoint{{192, 168, 3, 8}, kTestPort}), info.v4_endpoint);
        EXPECT_FALSE(info.v6_endpoint.address);
      }));
  mdns_service_->HandleNewEvents({});

  auto ptr_remove = MakePtrEvent(kTestServiceInstance, kTestServiceName,
                                 kTestServiceProtocol, kDefaultSocket);
  ptr_remove.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddPtrEvent(std::move(ptr_remove));

  EXPECT_CALL(observer_, OnReceiverRemoved(_))
      .WillOnce(::testing::Invoke([&service_id](const ServiceInfo& info) {
        EXPECT_EQ(service_id, info.service_id);
      }));
  mdns_service_->HandleNewEvents({});
}

TEST_F(MdnsResponderServiceTest, NetworkNetworkInterfaceIndex) {
  constexpr uint8_t mac[6] = {12, 34, 56, 78, 90};
  bound_interfaces_.emplace_back(
      platform::InterfaceInfo{2, mac, "wlan0",
                              platform::InterfaceInfo::Type::kWifi},
      platform::IPSubnet{IPAddress{10, 0, 0, 2}, 24}, kSecondSocket);
  fake_platform_service_->set_interfaces(bound_interfaces_);
  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
  ASSERT_TRUE(mdns_responder);
  ASSERT_TRUE(mdns_responder->running());

  AddEventsForNewService(mdns_responder, kTestServiceInstance, kTestServiceName,
                         kTestServiceProtocol, "gigliorononomicon", kTestPort,
                         {"model=shifty", "id=asdf"}, IPAddress{192, 168, 3, 7},
                         kSecondSocket);

  EXPECT_CALL(observer_, OnReceiverAdded(_))
      .WillOnce(::testing::Invoke([](const ServiceInfo& info) {
        EXPECT_EQ(2, info.network_interface_index);
      }));
  mdns_service_->HandleNewEvents({});
}

TEST_F(MdnsResponderServiceTest, SimultaneousFieldChanges) {
  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
  ASSERT_TRUE(mdns_responder);
  ASSERT_TRUE(mdns_responder->running());

  AddEventsForNewService(mdns_responder, kTestServiceInstance, kTestServiceName,
                         kTestServiceProtocol, "gigliorononomicon", kTestPort,
                         {"model=shifty", "id=asdf"}, IPAddress{192, 168, 3, 7},
                         kDefaultSocket);

  EXPECT_CALL(observer_, OnReceiverAdded(_));
  mdns_service_->HandleNewEvents({});

  mdns_responder->AddSrvEvent(
      MakeSrvEvent(kTestServiceInstance, kTestServiceName, kTestServiceProtocol,
                   "gigliorononomicon", 54321, kDefaultSocket));
  auto a_remove = MakeAEvent("gigliorononomicon", IPAddress{192, 168, 3, 7},
                             kDefaultSocket);
  a_remove.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddAEvent(std::move(a_remove));
  mdns_responder->AddAEvent(MakeAEvent(
      "gigliorononomicon", IPAddress{192, 168, 3, 8}, kDefaultSocket));

  EXPECT_CALL(observer_, OnReceiverChanged(_))
      .WillOnce(::testing::Invoke([](const ServiceInfo& info) {
        EXPECT_EQ((IPAddress{192, 168, 3, 8}), info.v4_endpoint.address);
        EXPECT_EQ(54321, info.v4_endpoint.port);
        EXPECT_FALSE(info.v6_endpoint.address);
      }));
  mdns_service_->HandleNewEvents({});
}

TEST_F(MdnsResponderServiceTest, SimultaneousHostAndAddressChange) {
  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
  ASSERT_TRUE(mdns_responder);
  ASSERT_TRUE(mdns_responder->running());

  AddEventsForNewService(mdns_responder, kTestServiceInstance, kTestServiceName,
                         kTestServiceProtocol, "gigliorononomicon", kTestPort,
                         {"model=shifty", "id=asdf"}, IPAddress{192, 168, 3, 7},
                         kDefaultSocket);

  EXPECT_CALL(observer_, OnReceiverAdded(_));
  mdns_service_->HandleNewEvents({});

  auto srv_remove =
      MakeSrvEvent(kTestServiceInstance, kTestServiceName, kTestServiceProtocol,
                   "gigliorononomicon", kTestPort, kDefaultSocket);
  srv_remove.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddSrvEvent(std::move(srv_remove));
  mdns_responder->AddSrvEvent(
      MakeSrvEvent(kTestServiceInstance, kTestServiceName, kTestServiceProtocol,
                   "alpha", kTestPort, kDefaultSocket));
  mdns_responder->AddAEvent(MakeAEvent(
      "gigliorononomicon", IPAddress{192, 168, 3, 8}, kDefaultSocket));
  mdns_responder->AddAEvent(
      MakeAEvent("alpha", IPAddress{192, 168, 3, 10}, kDefaultSocket));

  EXPECT_CALL(observer_, OnReceiverChanged(_))
      .WillOnce(::testing::Invoke([](const ServiceInfo& info) {
        EXPECT_EQ((IPAddress{192, 168, 3, 10}), info.v4_endpoint.address);
        EXPECT_FALSE(info.v6_endpoint.address);
      }));
  mdns_service_->HandleNewEvents({});
}

TEST_F(MdnsResponderServiceTest, ListenerStateTransitions) {
  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
  ASSERT_TRUE(mdns_responder);
  ASSERT_TRUE(mdns_responder->running());

  EXPECT_CALL(observer_, OnSuspended());
  service_listener_->Suspend();
  ASSERT_EQ(mdns_responder, mdns_responder_factory_->last_mdns_responder());
  EXPECT_FALSE(mdns_responder->running());

  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Resume();
  ASSERT_EQ(mdns_responder, mdns_responder_factory_->last_mdns_responder());
  EXPECT_TRUE(mdns_responder->running());

  EXPECT_CALL(observer_, OnStopped());
  service_listener_->Stop();
  ASSERT_FALSE(mdns_responder_factory_->last_mdns_responder());

  EXPECT_CALL(observer_, OnSuspended());
  auto instances = mdns_responder_factory_->instances();
  service_listener_->StartAndSuspend();
  EXPECT_EQ(instances + 1, mdns_responder_factory_->instances());
  mdns_responder = mdns_responder_factory_->last_mdns_responder();
  EXPECT_FALSE(mdns_responder->running());

  EXPECT_CALL(observer_, OnStopped());
  service_listener_->Stop();
  ASSERT_FALSE(mdns_responder_factory_->last_mdns_responder());
}

TEST_F(MdnsResponderServiceTest, BasicServicePublish) {
  EXPECT_CALL(publisher_observer_, OnStarted());
  service_publisher_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
  ASSERT_TRUE(mdns_responder);
  ASSERT_TRUE(mdns_responder->running());

  const auto& services = mdns_responder->registered_services();
  ASSERT_EQ(1u, services.size());
  EXPECT_EQ(kTestServiceInstance, services[0].service_instance);
  EXPECT_EQ(kTestServiceName, services[0].service_name);
  EXPECT_EQ(kTestServiceProtocol, services[0].service_protocol);
  auto host_labels = services[0].target_host.GetLabels();
  ASSERT_EQ(2u, host_labels.size());
  EXPECT_EQ(kTestHostname, host_labels[0]);
  EXPECT_EQ("local", host_labels[1]);
  EXPECT_EQ(kTestPort, services[0].target_port);

  EXPECT_CALL(publisher_observer_, OnStopped());
  service_publisher_->Stop();

  EXPECT_FALSE(mdns_responder_factory_->last_mdns_responder());
  EXPECT_EQ(0u, mdns_responder_factory_->last_registered_services_size());
}

TEST_F(MdnsResponderServiceTest, PublisherStateTransitions) {
  EXPECT_CALL(publisher_observer_, OnStarted());
  service_publisher_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
  ASSERT_TRUE(mdns_responder);
  ASSERT_TRUE(mdns_responder->running());
  EXPECT_EQ(1u, mdns_responder->registered_services().size());

  EXPECT_CALL(publisher_observer_, OnSuspended());
  service_publisher_->Suspend();
  EXPECT_EQ(0u, mdns_responder->registered_services().size());

  EXPECT_CALL(publisher_observer_, OnStarted());
  service_publisher_->Resume();
  EXPECT_EQ(1u, mdns_responder->registered_services().size());

  EXPECT_CALL(publisher_observer_, OnStopped());
  service_publisher_->Stop();
  EXPECT_EQ(0u, mdns_responder_factory_->last_registered_services_size());

  EXPECT_CALL(publisher_observer_, OnStarted());
  service_publisher_->Start();
  mdns_responder = mdns_responder_factory_->last_mdns_responder();
  ASSERT_TRUE(mdns_responder);
  ASSERT_TRUE(mdns_responder->running());
  EXPECT_EQ(1u, mdns_responder->registered_services().size());
  EXPECT_CALL(publisher_observer_, OnSuspended());
  service_publisher_->Suspend();
  EXPECT_EQ(0u, mdns_responder->registered_services().size());
  EXPECT_CALL(publisher_observer_, OnStopped());
  service_publisher_->Stop();
  EXPECT_FALSE(mdns_responder_factory_->last_mdns_responder());
  EXPECT_EQ(0u, mdns_responder_factory_->last_registered_services_size());
}

TEST_F(MdnsResponderServiceTest, PublisherObeysInterfaceWhitelist) {
  {
    mdns_service_->SetServiceConfig(kTestHostname, kTestServiceInstance,
                                    kTestPort, {}, {{"model", "shifty"}});

    EXPECT_CALL(publisher_observer_, OnStarted());
    service_publisher_->Start();

    auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
    ASSERT_TRUE(mdns_responder);
    ASSERT_TRUE(mdns_responder->running());
    auto interfaces = mdns_responder->registered_interfaces();
    ASSERT_EQ(2u, interfaces.size());
    EXPECT_EQ(kDefaultSocket, interfaces[0].socket);
    EXPECT_EQ(kSecondSocket, interfaces[1].socket);

    EXPECT_CALL(publisher_observer_, OnStopped());
    service_publisher_->Stop();
  }
  {
    mdns_service_->SetServiceConfig(kTestHostname, kTestServiceInstance,
                                    kTestPort, {1, 2}, {{"model", "shifty"}});

    EXPECT_CALL(publisher_observer_, OnStarted());
    service_publisher_->Start();

    auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
    ASSERT_TRUE(mdns_responder);
    ASSERT_TRUE(mdns_responder->running());
    auto interfaces = mdns_responder->registered_interfaces();
    ASSERT_EQ(2u, interfaces.size());
    EXPECT_EQ(kDefaultSocket, interfaces[0].socket);
    EXPECT_EQ(kSecondSocket, interfaces[1].socket);

    EXPECT_CALL(publisher_observer_, OnStopped());
    service_publisher_->Stop();
  }
  {
    mdns_service_->SetServiceConfig(kTestHostname, kTestServiceInstance,
                                    kTestPort, {2}, {{"model", "shifty"}});

    EXPECT_CALL(publisher_observer_, OnStarted());
    service_publisher_->Start();

    auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
    ASSERT_TRUE(mdns_responder);
    ASSERT_TRUE(mdns_responder->running());
    auto interfaces = mdns_responder->registered_interfaces();
    ASSERT_EQ(1u, interfaces.size());
    EXPECT_EQ(kSecondSocket, interfaces[0].socket);

    EXPECT_CALL(publisher_observer_, OnStopped());
    service_publisher_->Stop();
  }
}

TEST_F(MdnsResponderServiceTest, ListenAndPublish) {
  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
  ASSERT_TRUE(mdns_responder);
  ASSERT_TRUE(mdns_responder->running());

  {
    auto interfaces = mdns_responder->registered_interfaces();
    ASSERT_EQ(2u, interfaces.size());
    EXPECT_EQ(kDefaultSocket, interfaces[0].socket);
    EXPECT_EQ(kSecondSocket, interfaces[1].socket);
  }

  mdns_service_->SetServiceConfig(kTestHostname, kTestServiceInstance,
                                  kTestPort, {2}, {{"model", "shifty"}});

  auto instances = mdns_responder_factory_->instances();
  EXPECT_CALL(publisher_observer_, OnStarted());
  service_publisher_->Start();

  EXPECT_EQ(instances, mdns_responder_factory_->instances());
  ASSERT_TRUE(mdns_responder->running());
  {
    auto interfaces = mdns_responder->registered_interfaces();
    ASSERT_EQ(1u, interfaces.size());
    EXPECT_EQ(kSecondSocket, interfaces[0].socket);
  }

  EXPECT_CALL(observer_, OnStopped());
  service_listener_->Stop();
  ASSERT_TRUE(mdns_responder->running());
  EXPECT_EQ(1u, mdns_responder->registered_interfaces().size());

  EXPECT_CALL(publisher_observer_, OnStopped());
  service_publisher_->Stop();
  EXPECT_FALSE(mdns_responder_factory_->last_mdns_responder());
  EXPECT_EQ(0u, mdns_responder_factory_->last_registered_services_size());
}

TEST_F(MdnsResponderServiceTest, PublishAndListen) {
  mdns_service_->SetServiceConfig(kTestHostname, kTestServiceInstance,
                                  kTestPort, {2}, {{"model", "shifty"}});

  EXPECT_CALL(publisher_observer_, OnStarted());
  service_publisher_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
  ASSERT_TRUE(mdns_responder);
  ASSERT_TRUE(mdns_responder->running());
  {
    auto interfaces = mdns_responder->registered_interfaces();
    ASSERT_EQ(1u, interfaces.size());
    EXPECT_EQ(kSecondSocket, interfaces[0].socket);
  }

  auto instances = mdns_responder_factory_->instances();
  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Start();

  EXPECT_EQ(instances, mdns_responder_factory_->instances());
  ASSERT_TRUE(mdns_responder->running());
  {
    auto interfaces = mdns_responder->registered_interfaces();
    ASSERT_EQ(1u, interfaces.size());
    EXPECT_EQ(kSecondSocket, interfaces[0].socket);
  }

  EXPECT_CALL(publisher_observer_, OnStopped());
  service_publisher_->Stop();
  ASSERT_TRUE(mdns_responder->running());
  EXPECT_EQ(1u, mdns_responder->registered_interfaces().size());

  EXPECT_CALL(observer_, OnStopped());
  service_listener_->Stop();
  EXPECT_FALSE(mdns_responder_factory_->last_mdns_responder());
  EXPECT_EQ(0u, mdns_responder_factory_->last_registered_services_size());
}

TEST_F(MdnsResponderServiceTest, AddressQueryStopped) {
  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();

  AddEventsForNewService(mdns_responder, kTestServiceInstance, kTestServiceName,
                         kTestServiceProtocol, "gigliorononomicon", kTestPort,
                         {"model=shifty", "id=asdf"}, IPAddress{192, 168, 3, 7},
                         kDefaultSocket);

  EXPECT_CALL(observer_, OnReceiverAdded(_));
  mdns_service_->HandleNewEvents({});

  auto srv_remove =
      MakeSrvEvent(kTestServiceInstance, kTestServiceName, kTestServiceProtocol,
                   "gigliorononomicon", kTestPort, kDefaultSocket);
  srv_remove.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddSrvEvent(std::move(srv_remove));

  EXPECT_CALL(observer_, OnReceiverRemoved(_));
  mdns_service_->HandleNewEvents({});

  EXPECT_FALSE(mdns_responder->ptr_queries_empty());
  EXPECT_FALSE(mdns_responder->srv_queries_empty());
  EXPECT_FALSE(mdns_responder->txt_queries_empty());
  EXPECT_TRUE(mdns_responder->a_queries_empty());
  EXPECT_TRUE(mdns_responder->aaaa_queries_empty());
}

TEST_F(MdnsResponderServiceTest, AddressQueryRefCount) {
  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();

  AddEventsForNewService(mdns_responder, kTestServiceInstance, kTestServiceName,
                         kTestServiceProtocol, "gigliorononomicon", kTestPort,
                         {"model=shifty", "id=asdf"}, IPAddress{192, 168, 3, 7},
                         kDefaultSocket);
  AddEventsForNewService(mdns_responder, "instance-2", kTestServiceName,
                         kTestServiceProtocol, "gigliorononomicon", 4321,
                         {"model=shwofty", "id=asdf"},
                         IPAddress{192, 168, 3, 7}, kDefaultSocket);

  EXPECT_CALL(observer_, OnReceiverAdded(_)).Times(2);
  mdns_service_->HandleNewEvents({});

  auto srv_remove =
      MakeSrvEvent(kTestServiceInstance, kTestServiceName, kTestServiceProtocol,
                   "gigliorononomicon", kTestPort, kDefaultSocket);
  srv_remove.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddSrvEvent(std::move(srv_remove));

  EXPECT_CALL(observer_, OnReceiverRemoved(_));
  mdns_service_->HandleNewEvents({});

  EXPECT_FALSE(mdns_responder->ptr_queries_empty());
  EXPECT_FALSE(mdns_responder->srv_queries_empty());
  EXPECT_FALSE(mdns_responder->txt_queries_empty());
  EXPECT_FALSE(mdns_responder->a_queries_empty());
  EXPECT_FALSE(mdns_responder->aaaa_queries_empty());

  srv_remove =
      MakeSrvEvent("instance-2", kTestServiceName, kTestServiceProtocol,
                   "gigliorononomicon", 4321, kDefaultSocket);
  srv_remove.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddSrvEvent(std::move(srv_remove));

  EXPECT_CALL(observer_, OnReceiverRemoved(_));
  mdns_service_->HandleNewEvents({});

  EXPECT_FALSE(mdns_responder->ptr_queries_empty());
  EXPECT_FALSE(mdns_responder->srv_queries_empty());
  EXPECT_FALSE(mdns_responder->txt_queries_empty());
  EXPECT_TRUE(mdns_responder->a_queries_empty());
  EXPECT_TRUE(mdns_responder->aaaa_queries_empty());
}

TEST_F(MdnsResponderServiceTest, ServiceQueriesStoppedSrvFirst) {
  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();

  AddEventsForNewService(mdns_responder, kTestServiceInstance, kTestServiceName,
                         kTestServiceProtocol, "gigliorononomicon", kTestPort,
                         {"model=shifty", "id=asdf"}, IPAddress{192, 168, 3, 7},
                         kDefaultSocket);

  EXPECT_CALL(observer_, OnReceiverAdded(_));
  mdns_service_->HandleNewEvents({});

  auto srv_remove =
      MakeSrvEvent(kTestServiceInstance, kTestServiceName, kTestServiceProtocol,
                   "gigliorononomicon", kTestPort, kDefaultSocket);
  srv_remove.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddSrvEvent(std::move(srv_remove));

  EXPECT_CALL(observer_, OnReceiverRemoved(_));
  mdns_service_->HandleNewEvents({});

  EXPECT_FALSE(mdns_responder->ptr_queries_empty());
  EXPECT_FALSE(mdns_responder->srv_queries_empty());
  EXPECT_FALSE(mdns_responder->txt_queries_empty());
  EXPECT_TRUE(mdns_responder->a_queries_empty());
  EXPECT_TRUE(mdns_responder->aaaa_queries_empty());

  auto ptr_remove = MakePtrEvent(kTestServiceInstance, kTestServiceName,
                                 kTestServiceProtocol, kDefaultSocket);
  ptr_remove.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddPtrEvent(std::move(ptr_remove));
  mdns_service_->HandleNewEvents({});

  EXPECT_FALSE(mdns_responder->ptr_queries_empty());
  EXPECT_TRUE(mdns_responder->srv_queries_empty());
  EXPECT_TRUE(mdns_responder->txt_queries_empty());
  EXPECT_TRUE(mdns_responder->a_queries_empty());
  EXPECT_TRUE(mdns_responder->aaaa_queries_empty());
}

TEST_F(MdnsResponderServiceTest, ServiceQueriesStoppedPtrFirst) {
  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();

  AddEventsForNewService(mdns_responder, kTestServiceInstance, kTestServiceName,
                         kTestServiceProtocol, "gigliorononomicon", kTestPort,
                         {"model=shifty", "id=asdf"}, IPAddress{192, 168, 3, 7},
                         kDefaultSocket);

  EXPECT_CALL(observer_, OnReceiverAdded(_));
  mdns_service_->HandleNewEvents({});

  auto ptr_remove = MakePtrEvent(kTestServiceInstance, kTestServiceName,
                                 kTestServiceProtocol, kDefaultSocket);
  ptr_remove.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddPtrEvent(std::move(ptr_remove));

  EXPECT_CALL(observer_, OnReceiverRemoved(_));
  mdns_service_->HandleNewEvents({});

  EXPECT_FALSE(mdns_responder->ptr_queries_empty());
  EXPECT_FALSE(mdns_responder->srv_queries_empty());
  EXPECT_FALSE(mdns_responder->txt_queries_empty());
  EXPECT_FALSE(mdns_responder->a_queries_empty());
  EXPECT_FALSE(mdns_responder->aaaa_queries_empty());

  auto srv_remove =
      MakeSrvEvent(kTestServiceInstance, kTestServiceName, kTestServiceProtocol,
                   "gigliorononomicon", kTestPort, kDefaultSocket);
  srv_remove.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddSrvEvent(std::move(srv_remove));
  mdns_service_->HandleNewEvents({});

  EXPECT_FALSE(mdns_responder->ptr_queries_empty());
  EXPECT_TRUE(mdns_responder->srv_queries_empty());
  EXPECT_TRUE(mdns_responder->txt_queries_empty());
  EXPECT_TRUE(mdns_responder->a_queries_empty());
  EXPECT_TRUE(mdns_responder->aaaa_queries_empty());
}

TEST_F(MdnsResponderServiceTest, MultipleInterfaceRemove) {
  service_listener_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();

  AddEventsForNewService(mdns_responder, kTestServiceInstance, kTestServiceName,
                         kTestServiceProtocol, "gigliorononomicon", kTestPort,
                         {"model=shifty", "id=asdf"}, IPAddress{192, 168, 3, 7},
                         kDefaultSocket);
  AddEventsForNewService(mdns_responder, kTestServiceInstance, kTestServiceName,
                         kTestServiceProtocol, "gigliorononomicon", kTestPort,
                         {"model=shifty", "id=asdf"}, IPAddress{192, 168, 3, 7},
                         kSecondSocket);

  EXPECT_CALL(observer_, OnReceiverAdded(_));
  mdns_service_->HandleNewEvents({});

  auto srv_remove1 =
      MakeSrvEvent(kTestServiceInstance, kTestServiceName, kTestServiceProtocol,
                   "gigliorononomicon", kTestPort, kSecondSocket);
  srv_remove1.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddSrvEvent(std::move(srv_remove1));
  EXPECT_CALL(observer_, OnReceiverChanged(_)).Times(0);
  EXPECT_CALL(observer_, OnReceiverRemoved(_)).Times(0);
  mdns_service_->HandleNewEvents({});

  auto srv_remove2 =
      MakeSrvEvent(kTestServiceInstance, kTestServiceName, kTestServiceProtocol,
                   "gigliorononomicon", kTestPort, kDefaultSocket);
  srv_remove2.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddSrvEvent(std::move(srv_remove2));
  EXPECT_CALL(observer_, OnReceiverRemoved(_));
  mdns_service_->HandleNewEvents({});
  EXPECT_TRUE(mdns_responder->a_queries_empty());

  auto ptr_remove = MakePtrEvent(kTestServiceInstance, kTestServiceName,
                                 kTestServiceProtocol, kDefaultSocket);
  ptr_remove.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddPtrEvent(std::move(ptr_remove));
  mdns_service_->HandleNewEvents({});

  EXPECT_FALSE(mdns_responder->ptr_queries_empty());
  EXPECT_TRUE(mdns_responder->srv_queries_empty());
  EXPECT_TRUE(mdns_responder->txt_queries_empty());
  EXPECT_TRUE(mdns_responder->a_queries_empty());
  EXPECT_TRUE(mdns_responder->aaaa_queries_empty());
}

TEST_F(MdnsResponderServiceTest, ResumeService) {
  EXPECT_CALL(publisher_observer_, OnStarted());
  service_publisher_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();
  ASSERT_TRUE(mdns_responder);
  ASSERT_TRUE(mdns_responder->running());

  EXPECT_EQ(2u, mdns_responder->registered_interfaces().size());
  ASSERT_EQ(1u, mdns_responder->registered_services().size());

  EXPECT_CALL(publisher_observer_, OnSuspended());
  service_publisher_->Suspend();

  EXPECT_TRUE(mdns_responder_factory_->last_mdns_responder());
  EXPECT_EQ(0u, mdns_responder->registered_services().size());

  EXPECT_CALL(publisher_observer_, OnStarted());
  service_publisher_->Resume();

  EXPECT_EQ(2u, mdns_responder->registered_interfaces().size());
  ASSERT_EQ(1u, mdns_responder->registered_services().size());
}

TEST_F(MdnsResponderServiceTest, RestorePtrNotifiesObserver) {
  EXPECT_CALL(observer_, OnStarted());
  service_listener_->Start();

  auto* mdns_responder = mdns_responder_factory_->last_mdns_responder();

  AddEventsForNewService(mdns_responder, kTestServiceInstance, kTestServiceName,
                         kTestServiceProtocol, "gigliorononomicon", kTestPort,
                         {"model=shifty", "id=asdf"}, IPAddress{192, 168, 3, 7},
                         kDefaultSocket);

  EXPECT_CALL(observer_, OnReceiverAdded(_));
  mdns_service_->HandleNewEvents({});

  auto ptr_remove = MakePtrEvent(kTestServiceInstance, kTestServiceName,
                                 kTestServiceProtocol, kDefaultSocket);
  ptr_remove.header.response_type = mdns::QueryEventHeader::Type::kRemoved;
  mdns_responder->AddPtrEvent(std::move(ptr_remove));

  EXPECT_CALL(observer_, OnReceiverRemoved(_));
  mdns_service_->HandleNewEvents({});

  auto ptr_add = MakePtrEvent(kTestServiceInstance, kTestServiceName,
                              kTestServiceProtocol, kDefaultSocket);
  mdns_responder->AddPtrEvent(std::move(ptr_add));

  EXPECT_CALL(observer_, OnReceiverAdded(_));
  mdns_service_->HandleNewEvents({});
}

}  // namespace openscreen
