// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/mojo/media_router_struct_traits.h"

#include <utility>

#include "base/message_loop/message_loop.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "chrome/common/media_router/mojo/media_router.mojom.h"
#include "chrome/common/media_router/mojo/media_router_traits_test_service.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media_router {

class MediaRouterStructTraitsTest
    : public testing::Test,
      public media_router::mojom::MediaRouterTraitsTestService {
 public:
  MediaRouterStructTraitsTest() {}

 protected:
  mojom::MediaRouterTraitsTestServicePtr GetTraitsTestProxy() {
    mojom::MediaRouterTraitsTestServicePtr proxy;
    traits_test_bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  // MediaRouterTraitsTestService Impl
  void EchoMediaSink(const MediaSinkInternal& sink,
                     EchoMediaSinkCallback callback) override {
    std::move(callback).Run(sink);
  }

  base::MessageLoop loop_;
  mojo::BindingSet<MediaRouterTraitsTestService> traits_test_bindings_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterStructTraitsTest);
};

TEST_F(MediaRouterStructTraitsTest, DialMediaSink) {
  MediaSink::Id sink_id("sinkId123");
  std::string sink_name("The sink");
  SinkIconType icon_type(SinkIconType::CAST);
  MediaRouteProviderId provider_id(MediaRouteProviderId::EXTENSION);
  std::string ip_address("192.168.1.2");
  std::string model_name("model name");
  GURL app_url("https://example.com");

  MediaSink sink(sink_id, sink_name, icon_type, provider_id);
  DialSinkExtraData extra_data;
  EXPECT_TRUE(extra_data.ip_address.AssignFromIPLiteral(ip_address));
  extra_data.model_name = model_name;
  extra_data.app_url = app_url;

  MediaSinkInternal dial_sink(sink, extra_data);

  mojom::MediaRouterTraitsTestServicePtr proxy = GetTraitsTestProxy();
  MediaSinkInternal output;
  proxy->EchoMediaSink(dial_sink, &output);

  EXPECT_EQ(dial_sink, output);
}

TEST_F(MediaRouterStructTraitsTest, CastMediaSink) {
  MediaSink::Id sink_id("sinkId123");
  std::string sink_name("The sink");
  SinkIconType icon_type(SinkIconType::CAST);
  MediaRouteProviderId provider_id(MediaRouteProviderId::EXTENSION);
  std::string model_name("model name");

  MediaSink sink(sink_id, sink_name, icon_type, provider_id);
  CastSinkExtraData extra_data;
  extra_data.ip_endpoint = net::IPEndPoint(net::IPAddress(192, 168, 1, 2), 0);
  extra_data.model_name = model_name;
  extra_data.capabilities = 2;
  extra_data.cast_channel_id = 3;

  MediaSinkInternal cast_sink(sink, extra_data);

  mojom::MediaRouterTraitsTestServicePtr proxy = GetTraitsTestProxy();
  MediaSinkInternal output;
  proxy->EchoMediaSink(cast_sink, &output);

  EXPECT_EQ(cast_sink, output);
}

TEST_F(MediaRouterStructTraitsTest, GenericMediaSink) {
  MediaSink::Id sink_id("sinkId123");
  std::string sink_name("The sink");
  SinkIconType icon_type(SinkIconType::CAST);
  MediaRouteProviderId provider_id(MediaRouteProviderId::EXTENSION);

  MediaSink sink(sink_id, sink_name, icon_type, provider_id);
  MediaSinkInternal generic_sink;
  generic_sink.set_sink(sink);

  mojom::MediaRouterTraitsTestServicePtr proxy = GetTraitsTestProxy();
  MediaSinkInternal output;
  proxy->EchoMediaSink(generic_sink, &output);

  EXPECT_EQ(generic_sink, output);
}

}  // namespace media_router
