/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2014 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/paint/compositing/graphics_layer_tree_builder.h"

#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_layer_mapping.h"
#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"

namespace blink {

GraphicsLayerTreeBuilder::GraphicsLayerTreeBuilder() = default;

GraphicsLayerTreeBuilder::~GraphicsLayerTreeBuilder() = default;

static bool ShouldAppendLayer(const PaintLayer& layer) {
  Node* node = layer.GetLayoutObject().GetNode();
  if (node && IsHTMLVideoElement(*node)) {
    HTMLVideoElement* element = ToHTMLVideoElement(node);
    if (element->IsFullscreen() && element->UsesOverlayFullscreenVideo())
      return false;
  }
  return true;
}

void GraphicsLayerTreeBuilder::Rebuild(PaintLayer& layer,
                                       GraphicsLayerVector& child_layers) {
  // Make the layer compositing if necessary, and set up clipping and content
  // layers.  Note that we can only do work here that is independent of whether
  // the descendant layers have been processed. computeCompositingRequirements()
  // will already have done the paint invalidation if necessary.

  layer.StackingNode()->UpdateLayerListsIfNeeded();

  const bool has_composited_layer_mapping = layer.HasCompositedLayerMapping();
  CompositedLayerMapping* current_composited_layer_mapping =
      layer.GetCompositedLayerMapping();

  // If this layer has a compositedLayerMapping, then that is where we place
  // subsequent children GraphicsLayers.  Otherwise children continue to append
  // to the child list of the enclosing layer.
  GraphicsLayerVector this_layer_children;
  GraphicsLayerVector* layer_vector_for_children =
      has_composited_layer_mapping ? &this_layer_children : &child_layers;

#if DCHECK_IS_ON()
  LayerListMutationDetector mutation_checker(layer.StackingNode());
#endif

  if (layer.StackingNode()->IsStackingContext()) {
    PaintLayerStackingNodeIterator iterator(*layer.StackingNode(),
                                            kNegativeZOrderChildren);
    while (PaintLayerStackingNode* cur_node = iterator.Next())
      Rebuild(*cur_node->Layer(), *layer_vector_for_children);

    // If a negative z-order child is compositing, we get a foreground layer
    // which needs to get parented.
    if (has_composited_layer_mapping &&
        current_composited_layer_mapping->ForegroundLayer()) {
      layer_vector_for_children->push_back(
          current_composited_layer_mapping->ForegroundLayer());
    }
  }

  PaintLayerStackingNodeIterator iterator(
      *layer.StackingNode(), kNormalFlowChildren | kPositiveZOrderChildren);
  while (PaintLayerStackingNode* cur_node = iterator.Next())
    Rebuild(*cur_node->Layer(), *layer_vector_for_children);

  if (has_composited_layer_mapping) {
    bool parented = false;
    if (layer.GetLayoutObject().IsLayoutEmbeddedContent()) {
      parented = PaintLayerCompositor::AttachFrameContentLayersToIframeLayer(
          ToLayoutEmbeddedContent(layer.GetLayoutObject()));
    }

    if (!parented)
      current_composited_layer_mapping->SetSublayers(this_layer_children);

    if (ShouldAppendLayer(layer)) {
      child_layers.push_back(
          current_composited_layer_mapping->ChildForSuperlayers());
    }
  }

  if (layer.ScrollParent() &&
      layer.ScrollParent()->HasCompositedLayerMapping() &&
      layer.ScrollParent()
          ->GetCompositedLayerMapping()
          ->NeedsToReparentOverflowControls() &&
      layer.ScrollParent()->GetScrollableArea()->TopmostScrollChild() ==
          &layer) {
    child_layers.push_back(layer.ScrollParent()
                               ->GetCompositedLayerMapping()
                               ->DetachLayerForOverflowControls());
  }
}

}  // namespace blink
