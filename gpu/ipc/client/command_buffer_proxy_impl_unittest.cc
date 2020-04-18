// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/client/command_buffer_proxy_impl.h"

#include "base/memory/scoped_refptr.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "gpu/ipc/common/surface_handle.h"
#include "ipc/ipc_test_sink.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {
// GpuChannelHost is expected to be created on the IO thread, and posts tasks to
// setup its IPC listener, so it must be created after the thread task runner
// handle is set.  It expects Send to be called on any thread except IO thread,
// and posts tasks to the IO thread to ensure IPCs are sent in order, which is
// important for sync IPCs.  But we override Send, so we can't test sync IPC
// behavior with this setup.
class TestGpuChannelHost : public GpuChannelHost {
 public:
  TestGpuChannelHost(IPC::TestSink* sink)
      : GpuChannelHost(0 /* channel_id */,
                       GPUInfo(),
                       GpuFeatureInfo(),
                       mojo::ScopedMessagePipeHandle(
                           mojo::MessagePipeHandle(mojo::kInvalidHandleValue))),
        sink_(sink) {}

  bool Send(IPC::Message* msg) override { return sink_->Send(msg); }

 protected:
  ~TestGpuChannelHost() override {}

  IPC::TestSink* sink_;
};

class CommandBufferProxyImplTest : public testing::Test {
 public:
  CommandBufferProxyImplTest()
      : task_runner_(base::MakeRefCounted<base::TestSimpleTaskRunner>()),
        thread_task_runner_handle_override_(
            base::ThreadTaskRunnerHandle::OverrideForTesting(task_runner_)),
        channel_(base::MakeRefCounted<TestGpuChannelHost>(&sink_)) {}

  ~CommandBufferProxyImplTest() override {
    // Release channel, and run any cleanup tasks it posts.
    channel_ = nullptr;
    task_runner_->RunUntilIdle();
  }

  std::unique_ptr<CommandBufferProxyImpl> CreateAndInitializeProxy() {
    auto proxy = std::make_unique<CommandBufferProxyImpl>(
        channel_, nullptr /* gpu_memory_buffer_manager */, 0 /* stream_id */,
        task_runner_);
    proxy->Initialize(kNullSurfaceHandle, nullptr, SchedulingPriority::kNormal,
                      ContextCreationAttribs(), GURL());
    // Use an arbitrary valid shm_id. The command buffer doesn't use this
    // directly, but not setting it triggers DCHECKs.
    proxy->SetGetBuffer(1 /* shm_id */);
    sink_.ClearMessages();
    return proxy;
  }

 protected:
  IPC::TestSink sink_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ScopedClosureRunner thread_task_runner_handle_override_;
  scoped_refptr<TestGpuChannelHost> channel_;
};

TEST_F(CommandBufferProxyImplTest, OrderingBarriersAreCoalescedWithFlush) {
  auto proxy1 = CreateAndInitializeProxy();
  auto proxy2 = CreateAndInitializeProxy();

  proxy1->OrderingBarrier(10);
  proxy2->OrderingBarrier(20);
  proxy1->OrderingBarrier(30);
  proxy1->OrderingBarrier(40);
  proxy1->Flush(50);

  EXPECT_EQ(1u, sink_.message_count());
  const IPC::Message* msg =
      sink_.GetFirstMessageMatching(GpuChannelMsg_FlushCommandBuffers::ID);
  ASSERT_TRUE(msg);
  GpuChannelMsg_FlushCommandBuffers::Param params;
  ASSERT_TRUE(GpuChannelMsg_FlushCommandBuffers::Read(msg, &params));
  std::vector<FlushParams> flush_list = std::get<0>(std::move(params));
  EXPECT_EQ(3u, flush_list.size());
  EXPECT_EQ(proxy1->route_id(), flush_list[0].route_id);
  EXPECT_EQ(10, flush_list[0].put_offset);
  EXPECT_EQ(proxy2->route_id(), flush_list[1].route_id);
  EXPECT_EQ(20, flush_list[1].put_offset);
  EXPECT_EQ(proxy1->route_id(), flush_list[2].route_id);
  EXPECT_EQ(50, flush_list[2].put_offset);
}

TEST_F(CommandBufferProxyImplTest, FlushPendingWorkFlushesOrderingBarriers) {
  auto proxy1 = CreateAndInitializeProxy();
  auto proxy2 = CreateAndInitializeProxy();

  proxy1->OrderingBarrier(10);
  proxy2->OrderingBarrier(20);
  proxy1->OrderingBarrier(30);
  proxy2->FlushPendingWork();

  EXPECT_EQ(1u, sink_.message_count());
  const IPC::Message* msg =
      sink_.GetFirstMessageMatching(GpuChannelMsg_FlushCommandBuffers::ID);
  ASSERT_TRUE(msg);
  GpuChannelMsg_FlushCommandBuffers::Param params;
  ASSERT_TRUE(GpuChannelMsg_FlushCommandBuffers::Read(msg, &params));
  std::vector<FlushParams> flush_list = std::get<0>(std::move(params));
  EXPECT_EQ(3u, flush_list.size());
  EXPECT_EQ(proxy1->route_id(), flush_list[0].route_id);
  EXPECT_EQ(10, flush_list[0].put_offset);
  EXPECT_EQ(proxy2->route_id(), flush_list[1].route_id);
  EXPECT_EQ(20, flush_list[1].put_offset);
  EXPECT_EQ(proxy1->route_id(), flush_list[2].route_id);
  EXPECT_EQ(30, flush_list[2].put_offset);
}

TEST_F(CommandBufferProxyImplTest, EnsureWorkVisibleFlushesOrderingBarriers) {
  auto proxy1 = CreateAndInitializeProxy();
  auto proxy2 = CreateAndInitializeProxy();

  proxy1->OrderingBarrier(10);
  proxy2->OrderingBarrier(20);
  proxy1->OrderingBarrier(30);

  proxy2->EnsureWorkVisible();

  EXPECT_EQ(2u, sink_.message_count());
  const IPC::Message* msg = sink_.GetMessageAt(0);
  ASSERT_TRUE(msg);
  EXPECT_EQ(static_cast<uint32_t>(GpuChannelMsg_FlushCommandBuffers::ID),
            msg->type());

  GpuChannelMsg_FlushCommandBuffers::Param params;
  ASSERT_TRUE(GpuChannelMsg_FlushCommandBuffers::Read(msg, &params));
  std::vector<FlushParams> flush_list = std::get<0>(std::move(params));
  EXPECT_EQ(3u, flush_list.size());
  EXPECT_EQ(proxy1->route_id(), flush_list[0].route_id);
  EXPECT_EQ(10, flush_list[0].put_offset);
  EXPECT_EQ(proxy2->route_id(), flush_list[1].route_id);
  EXPECT_EQ(20, flush_list[1].put_offset);
  EXPECT_EQ(proxy1->route_id(), flush_list[2].route_id);
  EXPECT_EQ(30, flush_list[2].put_offset);

  msg = sink_.GetMessageAt(1);
  ASSERT_TRUE(msg);
  EXPECT_EQ(static_cast<uint32_t>(GpuChannelMsg_Nop::ID), msg->type());
}

}  // namespace gpu
