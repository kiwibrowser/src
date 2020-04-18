// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/cast_media_route_provider.h"

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/media/router/test/mock_mojo_media_router.h"
#include "chrome/browser/media/router/test/test_helper.h"
#include "chrome/common/media_router/test/test_helper.h"
#include "components/cast_channel/cast_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace media_router {

class CastMediaRouteProviderTest : public testing::Test {
 public:
  CastMediaRouteProviderTest()
      : socket_service_(new base::TestSimpleTaskRunner()),
        message_handler_(&socket_service_) {}
  ~CastMediaRouteProviderTest() override = default;

  void SetUp() override {
    mojom::MediaRouterPtr router_ptr;
    router_binding_ = std::make_unique<mojo::Binding<mojom::MediaRouter>>(
        &mock_router_, mojo::MakeRequest(&router_ptr));

    EXPECT_CALL(mock_router_, OnSinkAvailabilityUpdated(_, _));
    provider_ = std::make_unique<CastMediaRouteProvider>(
        mojo::MakeRequest(&provider_ptr_), router_ptr.PassInterface(),
        &media_sink_service_, &app_discovery_service_, &message_handler_,
        base::SequencedTaskRunnerHandle::Get());

    base::RunLoop().RunUntilIdle();
  }

  void TearDown() override { provider_.reset(); }

 protected:
  base::test::ScopedTaskEnvironment environment_;
  mojom::MediaRouteProviderPtr provider_ptr_;
  MockMojoMediaRouter mock_router_;
  std::unique_ptr<mojo::Binding<mojom::MediaRouter>> router_binding_;

  cast_channel::MockCastSocketService socket_service_;
  cast_channel::MockCastMessageHandler message_handler_;

  TestMediaSinkService media_sink_service_;
  MockCastAppDiscoveryService app_discovery_service_;
  std::unique_ptr<CastMediaRouteProvider> provider_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CastMediaRouteProviderTest);
};

TEST_F(CastMediaRouteProviderTest, StartObservingMediaSinks) {
  MediaSource::Id non_cast_source("not-a-cast-source:foo");
  EXPECT_CALL(app_discovery_service_, DoStartObservingMediaSinks(_)).Times(0);
  provider_->StartObservingMediaSinks(non_cast_source);

  MediaSource::Id cast_source("cast:ABCDEFGH");
  EXPECT_CALL(app_discovery_service_, DoStartObservingMediaSinks(_));
  provider_->StartObservingMediaSinks(cast_source);
  EXPECT_FALSE(app_discovery_service_.callbacks().empty());

  provider_->StopObservingMediaSinks(cast_source);
  EXPECT_TRUE(app_discovery_service_.callbacks().empty());
}

TEST_F(CastMediaRouteProviderTest, BroadcastRequest) {
  media_sink_service_.AddOrUpdateSink(CreateCastSink(1));
  media_sink_service_.AddOrUpdateSink(CreateCastSink(2));
  MediaSource::Id source_id(
      "cast:ABCDEFAB?capabilities=video_out,audio_out"
      "&broadcastNamespace=namespace"
      "&broadcastMessage=message");

  std::vector<std::string> app_ids = {"ABCDEFAB"};
  cast_channel::BroadcastRequest request("namespace", "message");
  EXPECT_CALL(message_handler_, SendBroadcastMessage(1, app_ids, request));
  EXPECT_CALL(message_handler_, SendBroadcastMessage(2, app_ids, request));
  EXPECT_CALL(app_discovery_service_, DoStartObservingMediaSinks(_)).Times(0);
  provider_->StartObservingMediaSinks(source_id);
  EXPECT_TRUE(app_discovery_service_.callbacks().empty());
}

}  // namespace media_router
