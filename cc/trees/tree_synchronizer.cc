// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/tree_synchronizer.h"

#include <stddef.h>

#include <set>

#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "cc/layers/layer.h"
#include "cc/layers/layer_collections.h"
#include "cc/layers/layer_impl.h"
#include "cc/trees/layer_tree_host.h"
#include "cc/trees/layer_tree_impl.h"

namespace cc {

template <typename LayerTreeType>
void SynchronizeTreesInternal(LayerTreeType* source_tree,
                              LayerTreeImpl* tree_impl,
                              PropertyTrees* property_trees) {
  DCHECK(tree_impl);

  TRACE_EVENT0("cc", "TreeSynchronizer::SynchronizeTrees");
  std::unique_ptr<OwnedLayerImplList> old_layers(tree_impl->DetachLayers());

  OwnedLayerImplMap old_layer_map;
  for (auto& it : *old_layers) {
    DCHECK(it);
    old_layer_map[it->id()] = std::move(it);
  }

  PushLayerList(&old_layer_map, source_tree, tree_impl);

  for (int id : property_trees->effect_tree.mask_layer_ids()) {
    std::unique_ptr<LayerImpl> layer_impl(ReuseOrCreateLayerImpl(
        &old_layer_map, source_tree->LayerById(id), tree_impl));
    tree_impl->AddLayer(std::move(layer_impl));
  }
}

void TreeSynchronizer::SynchronizeTrees(Layer* layer_root,
                                        LayerTreeImpl* tree_impl) {
  if (!layer_root) {
    tree_impl->DetachLayers();
  } else {
    SynchronizeTreesInternal(layer_root->layer_tree_host(), tree_impl,
                             layer_root->layer_tree_host()->property_trees());
  }
}

void TreeSynchronizer::SynchronizeTrees(LayerTreeImpl* pending_tree,
                                        LayerTreeImpl* active_tree) {
  if (pending_tree->LayerListIsEmpty()) {
    active_tree->DetachLayers();
  } else {
    SynchronizeTreesInternal(pending_tree, active_tree,
                             pending_tree->property_trees());
  }
}

template <typename LayerType>
std::unique_ptr<LayerImpl> ReuseOrCreateLayerImpl(OwnedLayerImplMap* old_layers,
                                                  LayerType* layer,
                                                  LayerTreeImpl* tree_impl) {
  if (!layer)
    return nullptr;
  std::unique_ptr<LayerImpl> layer_impl = std::move((*old_layers)[layer->id()]);
  if (!layer_impl)
    layer_impl = layer->CreateLayerImpl(tree_impl);
  return layer_impl;
}

#if DCHECK_IS_ON()
template <typename LayerType>
static void AssertValidPropertyTreeIndices(LayerType* layer) {
  DCHECK(layer);
  DCHECK_NE(layer->transform_tree_index(), TransformTree::kInvalidNodeId);
  DCHECK_NE(layer->effect_tree_index(), EffectTree::kInvalidNodeId);
  DCHECK_NE(layer->clip_tree_index(), ClipTree::kInvalidNodeId);
  DCHECK_NE(layer->scroll_tree_index(), ScrollTree::kInvalidNodeId);
}

static bool LayerHasValidPropertyTreeIndices(LayerImpl* layer) {
  DCHECK(layer);
  return layer->transform_tree_index() != TransformTree::kInvalidNodeId &&
         layer->effect_tree_index() != EffectTree::kInvalidNodeId &&
         layer->clip_tree_index() != ClipTree::kInvalidNodeId &&
         layer->scroll_tree_index() != ScrollTree::kInvalidNodeId;
}
#endif

template <typename LayerTreeType>
void PushLayerList(OwnedLayerImplMap* old_layers,
                   LayerTreeType* host,
                   LayerTreeImpl* tree_impl) {
  tree_impl->ClearLayerList();
  for (auto* layer : *host) {
    std::unique_ptr<LayerImpl> layer_impl(
        ReuseOrCreateLayerImpl(old_layers, layer, tree_impl));

#if DCHECK_IS_ON()
    // Every layer should have valid property tree indices
    AssertValidPropertyTreeIndices(layer);
    // Every layer_impl should either have valid property tree indices already
    // or the corresponding layer should push them onto layer_impl.
    DCHECK(LayerHasValidPropertyTreeIndices(layer_impl.get()) ||
           host->LayerNeedsPushPropertiesForTesting(layer));
#endif

    tree_impl->AddToLayerList(layer_impl.get());
    tree_impl->AddLayer(std::move(layer_impl));
  }
  tree_impl->OnCanDrawStateChangedForTree();
}

template <typename LayerType>
static void PushLayerPropertiesInternal(
    std::unordered_set<LayerType*> layers_that_should_push_properties,
    LayerTreeImpl* impl_tree) {
  for (auto layer : layers_that_should_push_properties) {
    LayerImpl* layer_impl = impl_tree->LayerById(layer->id());
    DCHECK(layer_impl);
    layer->PushPropertiesTo(layer_impl);
  }
}

void TreeSynchronizer::PushLayerProperties(LayerTreeImpl* pending_tree,
                                           LayerTreeImpl* active_tree) {
  const auto& layers = pending_tree->LayersThatShouldPushProperties();
  TRACE_EVENT1("cc", "TreeSynchronizer::PushLayerPropertiesTo.Impl",
               "layer_count", layers.size());
  PushLayerPropertiesInternal(layers, active_tree);
}

void TreeSynchronizer::PushLayerProperties(LayerTreeHost* host_tree,
                                           LayerTreeImpl* impl_tree) {
  const auto& layers = host_tree->LayersThatShouldPushProperties();
  TRACE_EVENT1("cc", "TreeSynchronizer::PushLayerPropertiesTo.Main",
               "layer_count", layers.size());
  PushLayerPropertiesInternal(layers, impl_tree);
}

}  // namespace cc
