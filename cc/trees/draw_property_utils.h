// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_DRAW_PROPERTY_UTILS_H_
#define CC_TREES_DRAW_PROPERTY_UTILS_H_

#include "cc/cc_export.h"
#include "cc/layers/layer_collections.h"

namespace gfx {
class Transform;
class Vector2dF;
}  // namespace gfx

namespace cc {

class Layer;
class LayerImpl;
class LayerTreeHost;
class LayerTreeImpl;
class RenderSurfaceImpl;
class EffectTree;
class TransformTree;
class PropertyTrees;
struct EffectNode;

namespace draw_property_utils {

void CC_EXPORT ConcatInverseSurfaceContentsScale(const EffectNode* effect_node,
                                                 gfx::Transform* transform);

// Computes combined (screen space) transforms for every node in the transform
// tree. This must be done prior to calling |ComputeClips|.
void CC_EXPORT ComputeTransforms(TransformTree* transform_tree);

// Computes screen space opacity for every node in the opacity tree.
void CC_EXPORT ComputeEffects(EffectTree* effect_tree);

void CC_EXPORT UpdatePropertyTrees(LayerTreeHost* layer_tree_host,
                                   PropertyTrees* property_trees);

void CC_EXPORT
UpdatePropertyTreesAndRenderSurfaces(LayerImpl* root_layer,
                                     PropertyTrees* property_trees,
                                     bool can_adjust_raster_scales);

void CC_EXPORT FindLayersThatNeedUpdates(LayerTreeHost* layer_tree_host,
                                         const PropertyTrees* property_trees,
                                         LayerList* update_layer_list);

void CC_EXPORT
FindLayersThatNeedUpdates(LayerTreeImpl* layer_tree_impl,
                          const PropertyTrees* property_trees,
                          std::vector<LayerImpl*>* visible_layer_list);

void CC_EXPORT
ComputeDrawPropertiesOfVisibleLayers(const LayerImplList* layer_list,
                                     PropertyTrees* property_trees);

void CC_EXPORT ComputeMaskDrawProperties(LayerImpl* mask_layer,
                                         PropertyTrees* property_trees);

void CC_EXPORT ComputeSurfaceDrawProperties(PropertyTrees* property_trees,
                                            RenderSurfaceImpl* render_surface);

bool CC_EXPORT LayerShouldBeSkippedForDrawPropertiesComputation(
    LayerImpl* layer,
    const TransformTree& transform_tree,
    const EffectTree& effect_tree);

bool CC_EXPORT LayerNeedsUpdate(Layer* layer,
                                bool layer_is_drawn,
                                const PropertyTrees* property_trees);

bool CC_EXPORT LayerNeedsUpdate(LayerImpl* layer,
                                bool layer_is_drawn,
                                const PropertyTrees* property_trees);

gfx::Transform CC_EXPORT DrawTransform(const LayerImpl* layer,
                                       const TransformTree& transform_tree,
                                       const EffectTree& effect_tree);

gfx::Transform CC_EXPORT ScreenSpaceTransform(const Layer* layer,
                                              const TransformTree& tree);

gfx::Transform CC_EXPORT ScreenSpaceTransform(const LayerImpl* layer,
                                              const TransformTree& tree);

void CC_EXPORT UpdatePageScaleFactor(PropertyTrees* property_trees,
                                     const LayerImpl* page_scale_layer,
                                     float page_scale_factor,
                                     float device_scale_factor,
                                     const gfx::Transform device_transform);

void CC_EXPORT UpdatePageScaleFactor(PropertyTrees* property_trees,
                                     const Layer* page_scale_layer,
                                     float page_scale_factor,
                                     float device_scale_factor,
                                     const gfx::Transform device_transform);

void CC_EXPORT
UpdateElasticOverscroll(PropertyTrees* property_trees,
                        const LayerImpl* overscroll_elasticity_layer,
                        const gfx::Vector2dF& elastic_overscroll);

void CC_EXPORT
UpdateElasticOverscroll(PropertyTrees* property_trees,
                        const Layer* overscroll_elasticity_layer,
                        const gfx::Vector2dF& elastic_overscroll);

}  // namespace draw_property_utils
}  // namespace cc

#endif  // CC_TREES_DRAW_PROPERTY_UTILS_H_
