// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "cc/layers/append_quads_data.h"
#include "cc/layers/heads_up_display_layer_impl.h"
#include "cc/test/fake_impl_task_runner_provider.h"
#include "cc/test/fake_layer_tree_frame_sink.h"
#include "cc/test/fake_layer_tree_host_impl.h"
#include "cc/test/test_task_graph_runner.h"
#include "cc/trees/layer_tree_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

void CheckDrawLayer(HeadsUpDisplayLayerImpl* layer,
                    LayerTreeFrameSink* frame_sink,
                    LayerTreeResourceProvider* resource_provider,
                    viz::ContextProvider* context_provider,
                    DrawMode draw_mode) {
  std::unique_ptr<viz::RenderPass> render_pass = viz::RenderPass::Create();
  AppendQuadsData data;
  bool will_draw = layer->WillDraw(draw_mode, resource_provider);
  if (will_draw)
    layer->AppendQuads(render_pass.get(), &data);
  viz::RenderPassList pass_list;
  pass_list.push_back(std::move(render_pass));
  layer->UpdateHudTexture(draw_mode, frame_sink, resource_provider,
                          context_provider, pass_list);
  if (will_draw)
    layer->DidDraw(resource_provider);

  size_t expected_quad_list_size = will_draw ? 1 : 0;
  EXPECT_EQ(expected_quad_list_size, pass_list.back()->quad_list.size());
  EXPECT_EQ(0u, data.num_missing_tiles);
  EXPECT_EQ(0u, data.num_incomplete_tiles);
}

TEST(HeadsUpDisplayLayerImplTest, ResourcelessSoftwareDrawAfterResourceLoss) {
  FakeImplTaskRunnerProvider task_runner_provider;
  TestTaskGraphRunner task_graph_runner;
  std::unique_ptr<LayerTreeFrameSink> layer_tree_frame_sink =
      FakeLayerTreeFrameSink::Create3d();
  FakeLayerTreeHostImpl host_impl(&task_runner_provider, &task_graph_runner);
  host_impl.CreatePendingTree();
  host_impl.SetVisible(true);
  host_impl.InitializeRenderer(layer_tree_frame_sink.get());
  std::unique_ptr<HeadsUpDisplayLayerImpl> layer_ptr =
      HeadsUpDisplayLayerImpl::Create(host_impl.pending_tree(), 1);
  layer_ptr->SetBounds(gfx::Size(100, 100));

  HeadsUpDisplayLayerImpl* layer = layer_ptr.get();

  host_impl.pending_tree()->SetRootLayerForTesting(std::move(layer_ptr));
  host_impl.pending_tree()->BuildLayerListAndPropertyTreesForTesting();

  // Check regular hardware draw is ok.
  CheckDrawLayer(layer, host_impl.layer_tree_frame_sink(),
                 host_impl.resource_provider(),
                 layer_tree_frame_sink->context_provider(), DRAW_MODE_HARDWARE);

  // Simulate a resource loss on transitioning to resourceless software mode.
  layer->ReleaseResources();

  // Should skip resourceless software draw and not crash in UpdateHudTexture.
  CheckDrawLayer(layer, host_impl.layer_tree_frame_sink(),
                 host_impl.resource_provider(),
                 layer_tree_frame_sink->context_provider(),
                 DRAW_MODE_RESOURCELESS_SOFTWARE);
}

TEST(HeadsUpDisplayLayerImplTest, CPUAndGPURasterCanvas) {
  FakeImplTaskRunnerProvider task_runner_provider;
  TestTaskGraphRunner task_graph_runner;
  std::unique_ptr<LayerTreeFrameSink> layer_tree_frame_sink =
      FakeLayerTreeFrameSink::Create3d();
  FakeLayerTreeHostImpl host_impl(&task_runner_provider, &task_graph_runner);
  host_impl.CreatePendingTree();
  host_impl.SetVisible(true);
  host_impl.InitializeRenderer(layer_tree_frame_sink.get());
  std::unique_ptr<HeadsUpDisplayLayerImpl> layer_ptr =
      HeadsUpDisplayLayerImpl::Create(host_impl.pending_tree(), 1);
  layer_ptr->SetBounds(gfx::Size(100, 100));

  HeadsUpDisplayLayerImpl* layer = layer_ptr.get();

  host_impl.pending_tree()->SetRootLayerForTesting(std::move(layer_ptr));
  host_impl.pending_tree()->BuildLayerListAndPropertyTreesForTesting();

  // Check Ganesh canvas drawing is ok.
  CheckDrawLayer(layer, host_impl.layer_tree_frame_sink(),
                 host_impl.resource_provider(),
                 layer_tree_frame_sink->context_provider(), DRAW_MODE_HARDWARE);

  host_impl.ReleaseLayerTreeFrameSink();
  layer_tree_frame_sink = FakeLayerTreeFrameSink::CreateSoftware();
  host_impl.InitializeRenderer(layer_tree_frame_sink.get());

  // Check SW canvas drawing is ok.
  CheckDrawLayer(layer, host_impl.layer_tree_frame_sink(),
                 host_impl.resource_provider(), nullptr, DRAW_MODE_SOFTWARE);
}

}  // namespace
}  // namespace cc
