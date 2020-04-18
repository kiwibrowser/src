// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/fake_layer_tree_host.h"

#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/animation/animation_host.h"
#include "cc/layers/layer.h"
#include "cc/test/test_task_graph_runner.h"
#include "cc/trees/mutator_host.h"

namespace cc {

FakeLayerTreeHost::FakeLayerTreeHost(FakeLayerTreeHostClient* client,
                                     LayerTreeHost::InitParams* params,
                                     CompositorMode mode)
    : LayerTreeHost(params, mode),
      client_(client),
      host_impl_(*params->settings,
                 &task_runner_provider_,
                 params->task_graph_runner),
      needs_commit_(false) {
  scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner =
      mode == CompositorMode::THREADED ? base::ThreadTaskRunnerHandle::Get()
                                       : nullptr;
  SetTaskRunnerProviderForTesting(TaskRunnerProvider::Create(
      base::ThreadTaskRunnerHandle::Get(), impl_task_runner));
  client_->SetLayerTreeHost(this);
}

std::unique_ptr<FakeLayerTreeHost> FakeLayerTreeHost::Create(
    FakeLayerTreeHostClient* client,
    TestTaskGraphRunner* task_graph_runner,
    MutatorHost* mutator_host) {
  LayerTreeSettings settings;
  return Create(client, task_graph_runner, mutator_host, settings);
}

std::unique_ptr<FakeLayerTreeHost> FakeLayerTreeHost::Create(
    FakeLayerTreeHostClient* client,
    TestTaskGraphRunner* task_graph_runner,
    MutatorHost* mutator_host,
    const LayerTreeSettings& settings) {
  return Create(client, task_graph_runner, mutator_host, settings,
                CompositorMode::SINGLE_THREADED);
}

std::unique_ptr<FakeLayerTreeHost> FakeLayerTreeHost::Create(
    FakeLayerTreeHostClient* client,
    TestTaskGraphRunner* task_graph_runner,
    MutatorHost* mutator_host,
    const LayerTreeSettings& settings,
    CompositorMode mode) {
  LayerTreeHost::InitParams params;
  params.client = client;
  params.settings = &settings;
  params.task_graph_runner = task_graph_runner;
  params.mutator_host = mutator_host;
  return base::WrapUnique(new FakeLayerTreeHost(client, &params, mode));
}

FakeLayerTreeHost::~FakeLayerTreeHost() {
  client_->SetLayerTreeHost(nullptr);
}

void FakeLayerTreeHost::SetNeedsCommit() { needs_commit_ = true; }

LayerImpl* FakeLayerTreeHost::CommitAndCreateLayerImplTree() {
  // TODO(pdr): Update the LayerTreeImpl lifecycle states here so lifecycle
  // violations can be caught.
  TreeSynchronizer::SynchronizeTrees(root_layer(), active_tree());
  active_tree()->SetPropertyTrees(property_trees());
  TreeSynchronizer::PushLayerProperties(root_layer()->layer_tree_host(),
                                        active_tree());
  mutator_host()->PushPropertiesTo(host_impl_.mutator_host());

  active_tree()->property_trees()->scroll_tree.PushScrollUpdatesFromMainThread(
      property_trees(), active_tree());

  if (page_scale_layer() && inner_viewport_scroll_layer()) {
    LayerTreeImpl::ViewportLayerIds ids;
    if (overscroll_elasticity_layer())
      ids.overscroll_elasticity = overscroll_elasticity_layer()->id();
    ids.page_scale = page_scale_layer()->id();
    if (inner_viewport_container_layer())
      ids.inner_viewport_container = inner_viewport_container_layer()->id();
    if (outer_viewport_container_layer())
      ids.outer_viewport_container = outer_viewport_container_layer()->id();
    ids.inner_viewport_scroll = inner_viewport_scroll_layer()->id();
    if (outer_viewport_scroll_layer())
      ids.outer_viewport_scroll = outer_viewport_scroll_layer()->id();
    active_tree()->SetViewportLayersFromIds(ids);
  }

  return active_tree()->root_layer_for_testing();
}

LayerImpl* FakeLayerTreeHost::CommitAndCreatePendingTree() {
  TreeSynchronizer::SynchronizeTrees(root_layer(), pending_tree());
  pending_tree()->SetPropertyTrees(property_trees());
  TreeSynchronizer::PushLayerProperties(root_layer()->layer_tree_host(),
                                        pending_tree());
  mutator_host()->PushPropertiesTo(host_impl_.mutator_host());

  pending_tree()->property_trees()->scroll_tree.PushScrollUpdatesFromMainThread(
      property_trees(), pending_tree());
  return pending_tree()->root_layer_for_testing();
}

}  // namespace cc
