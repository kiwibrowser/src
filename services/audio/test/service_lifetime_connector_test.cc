// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "services/audio/in_process_audio_manager_accessor.h"
#include "services/audio/public/mojom/constants.mojom.h"
#include "services/audio/service.h"
#include "services/audio/system_info.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Exactly;
using testing::StrictMock;

namespace {
constexpr base::TimeDelta kQuitTimeout = base::TimeDelta::FromMilliseconds(100);
}  // namespace

namespace audio {

// Connector-based tests for Audio service "quit with timeout" logic
class AudioServiceLifetimeConnectorTest : public testing::Test {
 public:
  AudioServiceLifetimeConnectorTest() {}
  ~AudioServiceLifetimeConnectorTest() override {}

  void SetUp() override {
    audio_manager_ = std::make_unique<media::MockAudioManager>(
        std::make_unique<media::TestAudioThread>(
            false /*not using a separate audio thread*/));
    std::unique_ptr<Service> service_impl = std::make_unique<Service>(
        std::make_unique<InProcessAudioManagerAccessor>(audio_manager_.get()),
        kQuitTimeout, false /* device_notifications_enabled */);
    service_ = service_impl.get();
    service_->SetQuitClosureForTesting(quit_request_.Get());
    connector_factory_ =
        service_manager::TestConnectorFactory::CreateForUniqueService(
            std::move(service_impl));
    connector_ = connector_factory_->CreateConnector();
  }

  void TearDown() override { audio_manager_->Shutdown(); }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_{
      base::test::ScopedTaskEnvironment::MainThreadType::MOCK_TIME};

  StrictMock<base::MockCallback<base::RepeatingClosure>> quit_request_;
  std::unique_ptr<media::MockAudioManager> audio_manager_;
  std::unique_ptr<service_manager::TestConnectorFactory> connector_factory_;
  std::unique_ptr<service_manager::Connector> connector_;
  Service* service_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioServiceLifetimeConnectorTest);
};

TEST_F(AudioServiceLifetimeConnectorTest, ServiceNotQuitWhenClientConnected) {
  EXPECT_CALL(quit_request_, Run()).Times(Exactly(0));

  mojom::SystemInfoPtr info;
  connector_->BindInterface(mojom::kServiceName, &info);
  EXPECT_TRUE(info.is_bound());

  scoped_task_environment_.FastForwardBy(kQuitTimeout * 2);
  EXPECT_TRUE(info.is_bound());
}

TEST_F(AudioServiceLifetimeConnectorTest,
       ServiceQuitAfterTimeoutWhenClientDisconnected) {
  mojom::SystemInfoPtr info;
  connector_->BindInterface(mojom::kServiceName, &info);

  {
    // Make sure the service does not disconnect before a timeout.
    EXPECT_CALL(quit_request_, Run()).Times(Exactly(0));
    info.reset();
    scoped_task_environment_.FastForwardBy(kQuitTimeout / 2);
  }
  // Now wait for what is left from of the timeout: the service should
  // disconnect.
  EXPECT_CALL(quit_request_, Run()).Times(Exactly(1));
  scoped_task_environment_.FastForwardBy(kQuitTimeout / 2);
}

TEST_F(AudioServiceLifetimeConnectorTest,
       ServiceNotQuitWhenAnotherClientQuicklyConnects) {
  EXPECT_CALL(quit_request_, Run()).Times(Exactly(0));

  mojom::SystemInfoPtr info1;
  connector_->BindInterface(mojom::kServiceName, &info1);
  EXPECT_TRUE(info1.is_bound());

  info1.reset();

  mojom::SystemInfoPtr info2;
  connector_->BindInterface(mojom::kServiceName, &info2);
  EXPECT_TRUE(info2.is_bound());

  scoped_task_environment_.FastForwardBy(kQuitTimeout);
  EXPECT_TRUE(info2.is_bound());
}

TEST_F(AudioServiceLifetimeConnectorTest,
       ServiceNotQuitWhenOneClientRemainsConnected) {
  mojom::SystemInfoPtr info1;
  mojom::SystemInfoPtr info2;
  {
    EXPECT_CALL(quit_request_, Run()).Times(Exactly(0));

    connector_->BindInterface(mojom::kServiceName, &info1);
    EXPECT_TRUE(info1.is_bound());
    connector_->BindInterface(mojom::kServiceName, &info2);
    EXPECT_TRUE(info2.is_bound());

    scoped_task_environment_.FastForwardBy(kQuitTimeout);
    EXPECT_TRUE(info1.is_bound());
    EXPECT_TRUE(info2.is_bound());

    info1.reset();
    EXPECT_TRUE(info2.is_bound());

    scoped_task_environment_.FastForwardBy(kQuitTimeout);
    EXPECT_FALSE(info1.is_bound());
    EXPECT_TRUE(info2.is_bound());
  }
  // Now disconnect the last client and wait for service quit request.
  EXPECT_CALL(quit_request_, Run()).Times(Exactly(1));
  info2.reset();
  scoped_task_environment_.FastForwardBy(kQuitTimeout);
}

TEST_F(AudioServiceLifetimeConnectorTest,
       QuitTimeoutIsNotShortenedAfterDelayedReconnect) {
  mojom::SystemInfoPtr info1;
  mojom::SystemInfoPtr info2;
  {
    EXPECT_CALL(quit_request_, Run()).Times(Exactly(0));

    connector_->BindInterface(mojom::kServiceName, &info1);
    EXPECT_TRUE(info1.is_bound());
    info1.reset();
    scoped_task_environment_.FastForwardBy(kQuitTimeout * 0.75);

    connector_->BindInterface(mojom::kServiceName, &info2);
    EXPECT_TRUE(info2.is_bound());
    info2.reset();
    scoped_task_environment_.FastForwardBy(kQuitTimeout * 0.75);
  }
  EXPECT_CALL(quit_request_, Run()).Times(Exactly(1));
  scoped_task_environment_.FastForwardBy(kQuitTimeout * 0.25);
}

}  // namespace audio
