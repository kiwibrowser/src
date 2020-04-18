// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "gpu/ipc/common/gpu_messages.h"
#include "gpu/ipc/service/gpu_channel.h"
#include "gpu/ipc/service/gpu_channel_manager.h"
#include "gpu/ipc/service/gpu_channel_test_common.h"

namespace gpu {

class GpuChannelManagerTest : public GpuChannelTestCommon {
 public:
  GpuChannelManagerTest() : GpuChannelTestCommon() {}
  ~GpuChannelManagerTest() override = default;

#if defined(OS_ANDROID)
  void TestApplicationBackgrounded(ContextType type,
                                   bool should_destroy_channel) {
    ASSERT_TRUE(channel_manager());

    int32_t kClientId = 1;
    GpuChannel* channel = CreateChannel(kClientId, true);
    EXPECT_TRUE(channel);

    int32_t kRouteId = 1;
    const SurfaceHandle kFakeSurfaceHandle = 1;
    SurfaceHandle surface_handle = kFakeSurfaceHandle;
    GPUCreateCommandBufferConfig init_params;
    init_params.surface_handle = surface_handle;
    init_params.share_group_id = MSG_ROUTING_NONE;
    init_params.stream_id = 0;
    init_params.stream_priority = SchedulingPriority::kNormal;
    init_params.attribs = ContextCreationAttribs();
    init_params.attribs.context_type = type;
    init_params.active_url = GURL();
    gpu::ContextResult result = gpu::ContextResult::kFatalFailure;
    gpu::Capabilities capabilities;
    HandleMessage(channel, new GpuChannelMsg_CreateCommandBuffer(
                               init_params, kRouteId, GetSharedHandle(),
                               &result, &capabilities));
    EXPECT_EQ(result, gpu::ContextResult::kSuccess);

    CommandBufferStub* stub = channel->LookupCommandBuffer(kRouteId);
    EXPECT_TRUE(stub);

    channel_manager()->OnApplicationBackgrounded();

    channel = channel_manager()->LookupChannel(kClientId);
    if (should_destroy_channel) {
      EXPECT_FALSE(channel);
    } else {
      EXPECT_TRUE(channel);
    }
  }
#endif
};

TEST_F(GpuChannelManagerTest, EstablishChannel) {
  int32_t kClientId = 1;
  uint64_t kClientTracingId = 1;

  ASSERT_TRUE(channel_manager());
  GpuChannel* channel =
      channel_manager()->EstablishChannel(kClientId, kClientTracingId, false);
  EXPECT_TRUE(channel);
  EXPECT_EQ(channel_manager()->LookupChannel(kClientId), channel);
}

#if defined(OS_ANDROID)
TEST_F(GpuChannelManagerTest, OnBackgroundedWithoutWebGL) {
  TestApplicationBackgrounded(CONTEXT_TYPE_OPENGLES2, true);
}

TEST_F(GpuChannelManagerTest, OnBackgroundedWithWebGL) {
  TestApplicationBackgrounded(CONTEXT_TYPE_WEBGL2, false);
}

#endif

}  // namespace gpu
