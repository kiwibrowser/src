/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/inspector/inspector_layer_tree_agent.h"

#include <memory>

#include "cc/base/region.h"
#include "third_party/blink/public/platform/web_float_point.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/inspector/identifiers_factory.h"
#include "third_party/blink/renderer/core/inspector/inspected_frames.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_layer_mapping.h"
#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/graphics/compositing_reasons.h"
#include "third_party/blink/renderer/platform/graphics/compositor_element_id.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/picture_snapshot.h"
#include "third_party/blink/renderer/platform/transforms/transformation_matrix.h"
#include "third_party/blink/renderer/platform/wtf/text/base64.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "ui/gfx/geometry/rect.h"

namespace blink {

using protocol::Array;
using protocol::Maybe;
using protocol::Response;
unsigned InspectorLayerTreeAgent::last_snapshot_id_;

inline String IdForLayer(const GraphicsLayer* graphics_layer) {
  return String::Number(graphics_layer->CcLayer()->id());
}

static std::unique_ptr<protocol::DOM::Rect> BuildObjectForRect(
    const gfx::Rect& rect) {
  return protocol::DOM::Rect::create()
      .setX(rect.x())
      .setY(rect.y())
      .setHeight(rect.height())
      .setWidth(rect.width())
      .build();
}

static std::unique_ptr<protocol::LayerTree::ScrollRect> BuildScrollRect(
    const gfx::Rect& rect,
    const String& type) {
  std::unique_ptr<protocol::DOM::Rect> rect_object = BuildObjectForRect(rect);
  std::unique_ptr<protocol::LayerTree::ScrollRect> scroll_rect_object =
      protocol::LayerTree::ScrollRect::create()
          .setRect(std::move(rect_object))
          .setType(type)
          .build();
  return scroll_rect_object;
}

static std::unique_ptr<Array<protocol::LayerTree::ScrollRect>>
BuildScrollRectsForLayer(GraphicsLayer* graphics_layer,
                         bool report_wheel_scrollers) {
  std::unique_ptr<Array<protocol::LayerTree::ScrollRect>> scroll_rects =
      Array<protocol::LayerTree::ScrollRect>::create();
  cc::Layer* cc_layer = graphics_layer->CcLayer();
  const cc::Region& non_fast_scrollable_rects =
      cc_layer->non_fast_scrollable_region();
  for (const gfx::Rect& rect : non_fast_scrollable_rects) {
    scroll_rects->addItem(BuildScrollRect(
        IntRect(rect),
        protocol::LayerTree::ScrollRect::TypeEnum::RepaintsOnScroll));
  }
  const cc::Region& touch_event_handler_region =
      cc_layer->touch_action_region().region();

  for (const gfx::Rect& rect : touch_event_handler_region) {
    scroll_rects->addItem(BuildScrollRect(
        IntRect(rect),
        protocol::LayerTree::ScrollRect::TypeEnum::TouchEventHandler));
  }
  if (report_wheel_scrollers) {
    scroll_rects->addItem(BuildScrollRect(
        // TODO(yutak): This truncates the floating point position to integers.
        gfx::Rect(cc_layer->position().x(), cc_layer->position().y(),
                  cc_layer->bounds().width(), cc_layer->bounds().height()),
        protocol::LayerTree::ScrollRect::TypeEnum::WheelEventHandler));
  }
  return scroll_rects->length() ? std::move(scroll_rects) : nullptr;
}

// TODO(flackr): We should be getting the sticky position constraints from the
// property tree once blink is able to access them. https://crbug.com/754339
static GraphicsLayer* FindLayerByElementId(GraphicsLayer* root,
                                           CompositorElementId element_id) {
  if (root->CcLayer()->element_id() == element_id)
    return root;
  for (size_t i = 0, size = root->Children().size(); i < size; ++i) {
    if (GraphicsLayer* layer =
            FindLayerByElementId(root->Children()[i], element_id))
      return layer;
  }
  return nullptr;
}

static std::unique_ptr<protocol::LayerTree::StickyPositionConstraint>
BuildStickyInfoForLayer(GraphicsLayer* root, cc::Layer* layer) {
  cc::LayerStickyPositionConstraint constraints =
      layer->sticky_position_constraint();
  if (!constraints.is_sticky)
    return nullptr;

  std::unique_ptr<protocol::DOM::Rect> sticky_box_rect =
      BuildObjectForRect(constraints.scroll_container_relative_sticky_box_rect);

  std::unique_ptr<protocol::DOM::Rect> containing_block_rect =
      BuildObjectForRect(
          constraints.scroll_container_relative_containing_block_rect);

  std::unique_ptr<protocol::LayerTree::StickyPositionConstraint>
      constraints_obj =
          protocol::LayerTree::StickyPositionConstraint::create()
              .setStickyBoxRect(std::move(sticky_box_rect))
              .setContainingBlockRect(std::move(containing_block_rect))
              .build();
  if (constraints.nearest_element_shifting_sticky_box) {
    constraints_obj->setNearestLayerShiftingStickyBox(String::Number(
        FindLayerByElementId(root,
                             constraints.nearest_element_shifting_sticky_box)
            ->CcLayer()
            ->id()));
  }
  if (constraints.nearest_element_shifting_containing_block) {
    constraints_obj->setNearestLayerShiftingContainingBlock(String::Number(
        FindLayerByElementId(
            root, constraints.nearest_element_shifting_containing_block)
            ->CcLayer()
            ->id()));
  }

  return constraints_obj;
}

static std::unique_ptr<protocol::LayerTree::Layer> BuildObjectForLayer(
    GraphicsLayer* root,
    GraphicsLayer* graphics_layer,
    int node_id,
    bool report_wheel_event_listeners) {
  cc::Layer* cc_layer = graphics_layer->CcLayer();
  std::unique_ptr<protocol::LayerTree::Layer> layer_object =
      protocol::LayerTree::Layer::create()
          .setLayerId(IdForLayer(graphics_layer))
          .setOffsetX(cc_layer->position().x())
          .setOffsetY(cc_layer->position().y())
          .setWidth(cc_layer->bounds().width())
          .setHeight(cc_layer->bounds().height())
          .setPaintCount(graphics_layer->PaintCount())
          .setDrawsContent(cc_layer->DrawsContent())
          .build();

  if (node_id)
    layer_object->setBackendNodeId(node_id);

  GraphicsLayer* parent = graphics_layer->Parent();
  if (parent)
    layer_object->setParentLayerId(IdForLayer(parent));
  if (!graphics_layer->ContentsAreVisible())
    layer_object->setInvisible(true);
  const TransformationMatrix& transform = graphics_layer->Transform();
  if (!transform.IsIdentity()) {
    TransformationMatrix::FloatMatrix4 flattened_matrix;
    transform.ToColumnMajorFloatArray(flattened_matrix);
    std::unique_ptr<Array<double>> transform_array = Array<double>::create();
    for (size_t i = 0; i < arraysize(flattened_matrix); ++i)
      transform_array->addItem(flattened_matrix[i]);
    layer_object->setTransform(std::move(transform_array));
    const FloatPoint3D& transform_origin = graphics_layer->TransformOrigin();
    // FIXME: rename these to setTransformOrigin*
    if (cc_layer->bounds().width() > 0) {
      layer_object->setAnchorX(transform_origin.X() /
                               cc_layer->bounds().width());
    } else {
      layer_object->setAnchorX(0.f);
    }
    if (cc_layer->bounds().height() > 0) {
      layer_object->setAnchorY(transform_origin.Y() /
                               cc_layer->bounds().height());
    } else {
      layer_object->setAnchorY(0.f);
    }
    layer_object->setAnchorZ(transform_origin.Z());
  }
  std::unique_ptr<Array<protocol::LayerTree::ScrollRect>> scroll_rects =
      BuildScrollRectsForLayer(graphics_layer, report_wheel_event_listeners);
  if (scroll_rects)
    layer_object->setScrollRects(std::move(scroll_rects));
  std::unique_ptr<protocol::LayerTree::StickyPositionConstraint> sticky_info =
      BuildStickyInfoForLayer(root, cc_layer);
  if (sticky_info)
    layer_object->setStickyPositionConstraint(std::move(sticky_info));
  return layer_object;
}

InspectorLayerTreeAgent::InspectorLayerTreeAgent(
    InspectedFrames* inspected_frames,
    Client* client)
    : inspected_frames_(inspected_frames),
      client_(client),
      suppress_layer_paint_events_(false) {}

InspectorLayerTreeAgent::~InspectorLayerTreeAgent() = default;

void InspectorLayerTreeAgent::Trace(blink::Visitor* visitor) {
  visitor->Trace(inspected_frames_);
  InspectorBaseAgent::Trace(visitor);
}

void InspectorLayerTreeAgent::Restore() {
  // We do not re-enable layer agent automatically after navigation. This is
  // because it depends on DOMAgent and node ids in particular, so we let
  // front-end request document and re-enable the agent manually after this.
}

Response InspectorLayerTreeAgent::enable() {
  instrumenting_agents_->addInspectorLayerTreeAgent(this);
  Document* document = inspected_frames_->Root()->GetDocument();
  if (document &&
      document->Lifecycle().GetState() >= DocumentLifecycle::kCompositingClean)
    LayerTreeDidChange();
  return Response::OK();
}

Response InspectorLayerTreeAgent::disable() {
  instrumenting_agents_->removeInspectorLayerTreeAgent(this);
  snapshot_by_id_.clear();
  return Response::OK();
}

void InspectorLayerTreeAgent::LayerTreeDidChange() {
  GetFrontend()->layerTreeDidChange(BuildLayerTree());
}

void InspectorLayerTreeAgent::DidPaint(const GraphicsLayer* graphics_layer,
                                       GraphicsContext&,
                                       const LayoutRect& rect) {
  if (suppress_layer_paint_events_)
    return;
  // Should only happen for LocalFrameView paints when compositing is off.
  // Consider different instrumentation method for that.
  if (!graphics_layer)
    return;

  std::unique_ptr<protocol::DOM::Rect> dom_rect = protocol::DOM::Rect::create()
                                                      .setX(rect.X())
                                                      .setY(rect.Y())
                                                      .setWidth(rect.Width())
                                                      .setHeight(rect.Height())
                                                      .build();
  GetFrontend()->layerPainted(IdForLayer(graphics_layer), std::move(dom_rect));
}

std::unique_ptr<Array<protocol::LayerTree::Layer>>
InspectorLayerTreeAgent::BuildLayerTree() {
  PaintLayerCompositor* compositor = GetPaintLayerCompositor();
  if (!compositor || !compositor->InCompositingMode())
    return nullptr;

  LayerIdToNodeIdMap layer_id_to_node_id_map;
  std::unique_ptr<Array<protocol::LayerTree::Layer>> layers =
      Array<protocol::LayerTree::Layer>::create();
  BuildLayerIdToNodeIdMap(compositor->RootLayer(), layer_id_to_node_id_map);
  auto* layer_for_scrolling = inspected_frames_->Root()
                                  ->View()
                                  ->LayoutViewportScrollableArea()
                                  ->LayerForScrolling();
  int scrolling_layer_id =
      layer_for_scrolling ? layer_for_scrolling->CcLayer()->id() : 0;
  bool have_blocking_wheel_event_handlers =
      inspected_frames_->Root()->GetChromeClient().EventListenerProperties(
          inspected_frames_->Root(), WebEventListenerClass::kMouseWheel) ==
      WebEventListenerProperties::kBlocking;

  GatherGraphicsLayers(RootGraphicsLayer(), layer_id_to_node_id_map, layers,
                       have_blocking_wheel_event_handlers, scrolling_layer_id);
  return layers;
}

void InspectorLayerTreeAgent::BuildLayerIdToNodeIdMap(
    PaintLayer* root,
    LayerIdToNodeIdMap& layer_id_to_node_id_map) {
  if (root->HasCompositedLayerMapping()) {
    if (Node* node = root->GetLayoutObject().GeneratingNode()) {
      GraphicsLayer* graphics_layer =
          root->GetCompositedLayerMapping()->ChildForSuperlayers();
      layer_id_to_node_id_map.Set(graphics_layer->CcLayer()->id(),
                                  IdForNode(node));
    }
  }
  for (PaintLayer* child = root->FirstChild(); child;
       child = child->NextSibling())
    BuildLayerIdToNodeIdMap(child, layer_id_to_node_id_map);
  if (!root->GetLayoutObject().IsLayoutIFrame())
    return;
  FrameView* child_frame_view =
      ToLayoutEmbeddedContent(root->GetLayoutObject()).ChildFrameView();
  if (!child_frame_view || !child_frame_view->IsLocalFrameView())
    return;
  LayoutView* child_layout_view =
      ToLocalFrameView(child_frame_view)->GetLayoutView();
  if (!child_layout_view)
    return;
  PaintLayerCompositor* child_compositor = child_layout_view->Compositor();
  if (!child_compositor)
    return;
  BuildLayerIdToNodeIdMap(child_compositor->RootLayer(),
                          layer_id_to_node_id_map);
}

void InspectorLayerTreeAgent::GatherGraphicsLayers(
    GraphicsLayer* layer,
    HashMap<int, int>& layer_id_to_node_id_map,
    std::unique_ptr<Array<protocol::LayerTree::Layer>>& layers,
    bool has_wheel_event_handlers,
    int scrolling_layer_id) {
  if (client_->IsInspectorLayer(layer))
    return;
  int layer_id = layer->CcLayer()->id();
  layers->addItem(BuildObjectForLayer(
      RootGraphicsLayer(), layer, layer_id_to_node_id_map.at(layer_id),
      has_wheel_event_handlers && layer_id == scrolling_layer_id));
  for (size_t i = 0, size = layer->Children().size(); i < size; ++i)
    GatherGraphicsLayers(layer->Children()[i], layer_id_to_node_id_map, layers,
                         has_wheel_event_handlers, scrolling_layer_id);
}

int InspectorLayerTreeAgent::IdForNode(Node* node) {
  return DOMNodeIds::IdForNode(node);
}

PaintLayerCompositor* InspectorLayerTreeAgent::GetPaintLayerCompositor() {
  auto* layout_view = inspected_frames_->Root()->ContentLayoutObject();
  PaintLayerCompositor* compositor =
      layout_view ? layout_view->Compositor() : nullptr;
  return compositor;
}

GraphicsLayer* InspectorLayerTreeAgent::RootGraphicsLayer() {
  return inspected_frames_->Root()
      ->GetPage()
      ->GetVisualViewport()
      .RootGraphicsLayer();
}

static GraphicsLayer* FindLayerById(GraphicsLayer* root, int layer_id) {
  if (root->CcLayer()->id() == layer_id)
    return root;
  for (size_t i = 0, size = root->Children().size(); i < size; ++i) {
    if (GraphicsLayer* layer = FindLayerById(root->Children()[i], layer_id))
      return layer;
  }
  return nullptr;
}

Response InspectorLayerTreeAgent::LayerById(const String& layer_id,
                                            GraphicsLayer*& result) {
  bool ok;
  int id = layer_id.ToInt(&ok);
  if (!ok)
    return Response::Error("Invalid layer id");
  PaintLayerCompositor* compositor = GetPaintLayerCompositor();
  if (!compositor)
    return Response::Error("Not in compositing mode");

  result = FindLayerById(RootGraphicsLayer(), id);
  if (!result)
    return Response::Error("No layer matching given id found");
  return Response::OK();
}

Response InspectorLayerTreeAgent::compositingReasons(
    const String& layer_id,
    std::unique_ptr<Array<String>>* reason_strings) {
  GraphicsLayer* graphics_layer = nullptr;
  Response response = LayerById(layer_id, graphics_layer);
  if (!response.isSuccess())
    return response;
  CompositingReasons reasons_bitmask = graphics_layer->GetCompositingReasons();
  *reason_strings = Array<String>::create();
  for (const char* name : CompositingReason::ShortNames(reasons_bitmask))
    (*reason_strings)->addItem(name);
  return Response::OK();
}

Response InspectorLayerTreeAgent::makeSnapshot(const String& layer_id,
                                               String* snapshot_id) {
  GraphicsLayer* layer = nullptr;
  Response response = LayerById(layer_id, layer);
  if (!response.isSuccess())
    return response;
  if (!layer->DrawsContent())
    return Response::Error("Layer does not draw content");

  IntSize size = ExpandedIntSize(layer->Size());
  IntRect interest_rect(IntPoint(0, 0), size);
  suppress_layer_paint_events_ = true;

  // If we hit a devtool break point in the middle of document lifecycle, for
  // example, https://crbug.com/788219, this will prevent crash when clicking
  // the "layer" panel.
  if (inspected_frames_->Root()->GetDocument() && inspected_frames_->Root()
                                                      ->GetDocument()
                                                      ->Lifecycle()
                                                      .LifecyclePostponed())
    return Response::Error("Layer does not draw content");

  inspected_frames_->Root()->View()->UpdateAllLifecyclePhasesExceptPaint();
  for (auto frame = inspected_frames_->begin();
       frame != inspected_frames_->end(); ++frame) {
    frame->GetDocument()->Lifecycle().AdvanceTo(DocumentLifecycle::kInPaint);
  }
  layer->Paint(&interest_rect);
  for (auto frame = inspected_frames_->begin();
       frame != inspected_frames_->end(); ++frame) {
    frame->GetDocument()->Lifecycle().AdvanceTo(DocumentLifecycle::kPaintClean);
  }

  suppress_layer_paint_events_ = false;

  auto snapshot = base::AdoptRef(new PictureSnapshot(
      ToSkPicture(layer->CapturePaintRecord(), interest_rect)));

  *snapshot_id = String::Number(++last_snapshot_id_);
  bool new_entry = snapshot_by_id_.insert(*snapshot_id, snapshot).is_new_entry;
  DCHECK(new_entry);
  return Response::OK();
}

Response InspectorLayerTreeAgent::loadSnapshot(
    std::unique_ptr<Array<protocol::LayerTree::PictureTile>> tiles,
    String* snapshot_id) {
  if (!tiles->length())
    return Response::Error("Invalid argument, no tiles provided");
  Vector<scoped_refptr<PictureSnapshot::TilePictureStream>> decoded_tiles;
  decoded_tiles.Grow(tiles->length());
  for (size_t i = 0; i < tiles->length(); ++i) {
    protocol::LayerTree::PictureTile* tile = tiles->get(i);
    decoded_tiles[i] = base::AdoptRef(new PictureSnapshot::TilePictureStream());
    decoded_tiles[i]->layer_offset.Set(tile->getX(), tile->getY());
    if (!Base64Decode(tile->getPicture(), decoded_tiles[i]->data))
      return Response::Error("Invalid base64 encoding");
  }
  scoped_refptr<PictureSnapshot> snapshot =
      PictureSnapshot::Load(decoded_tiles);
  if (!snapshot)
    return Response::Error("Invalid snapshot format");
  if (snapshot->IsEmpty())
    return Response::Error("Empty snapshot");

  *snapshot_id = String::Number(++last_snapshot_id_);
  bool new_entry = snapshot_by_id_.insert(*snapshot_id, snapshot).is_new_entry;
  DCHECK(new_entry);
  return Response::OK();
}

Response InspectorLayerTreeAgent::releaseSnapshot(const String& snapshot_id) {
  SnapshotById::iterator it = snapshot_by_id_.find(snapshot_id);
  if (it == snapshot_by_id_.end())
    return Response::Error("Snapshot not found");
  snapshot_by_id_.erase(it);
  return Response::OK();
}

Response InspectorLayerTreeAgent::GetSnapshotById(
    const String& snapshot_id,
    const PictureSnapshot*& result) {
  SnapshotById::iterator it = snapshot_by_id_.find(snapshot_id);
  if (it == snapshot_by_id_.end())
    return Response::Error("Snapshot not found");
  result = it->value.get();
  return Response::OK();
}

Response InspectorLayerTreeAgent::replaySnapshot(const String& snapshot_id,
                                                 Maybe<int> from_step,
                                                 Maybe<int> to_step,
                                                 Maybe<double> scale,
                                                 String* data_url) {
  const PictureSnapshot* snapshot = nullptr;
  Response response = GetSnapshotById(snapshot_id, snapshot);
  if (!response.isSuccess())
    return response;
  std::unique_ptr<Vector<char>> base64_data = snapshot->Replay(
      from_step.fromMaybe(0), to_step.fromMaybe(0), scale.fromMaybe(1.0));
  if (!base64_data)
    return Response::Error("Image encoding failed");
  StringBuilder url;
  url.Append("data:image/png;base64,");
  url.ReserveCapacity(url.length() + base64_data->size());
  url.Append(base64_data->begin(), base64_data->size());
  *data_url = url.ToString();
  return Response::OK();
}

static void ParseRect(protocol::DOM::Rect* object, FloatRect* rect) {
  *rect = FloatRect(object->getX(), object->getY(), object->getWidth(),
                    object->getHeight());
}

Response InspectorLayerTreeAgent::profileSnapshot(
    const String& snapshot_id,
    Maybe<int> min_repeat_count,
    Maybe<double> min_duration,
    Maybe<protocol::DOM::Rect> clip_rect,
    std::unique_ptr<protocol::Array<protocol::Array<double>>>* out_timings) {
  const PictureSnapshot* snapshot = nullptr;
  Response response = GetSnapshotById(snapshot_id, snapshot);
  if (!response.isSuccess())
    return response;
  FloatRect rect;
  if (clip_rect.isJust())
    ParseRect(clip_rect.fromJust(), &rect);
  std::unique_ptr<PictureSnapshot::Timings> timings = snapshot->Profile(
      min_repeat_count.fromMaybe(1), min_duration.fromMaybe(0),
      clip_rect.isJust() ? &rect : nullptr);
  *out_timings = Array<Array<double>>::create();
  for (size_t i = 0; i < timings->size(); ++i) {
    const Vector<double>& row = (*timings)[i];
    std::unique_ptr<Array<double>> out_row = Array<double>::create();
    for (size_t j = 0; j < row.size(); ++j)
      out_row->addItem(row[j]);
    (*out_timings)->addItem(std::move(out_row));
  }
  return Response::OK();
}

Response InspectorLayerTreeAgent::snapshotCommandLog(
    const String& snapshot_id,
    std::unique_ptr<Array<protocol::DictionaryValue>>* command_log) {
  const PictureSnapshot* snapshot = nullptr;
  Response response = GetSnapshotById(snapshot_id, snapshot);
  if (!response.isSuccess())
    return response;
  protocol::ErrorSupport errors;
  std::unique_ptr<protocol::Value> log_value = protocol::StringUtil::parseJSON(
      snapshot->SnapshotCommandLog()->ToJSONString());
  *command_log =
      Array<protocol::DictionaryValue>::fromValue(log_value.get(), &errors);
  if (errors.hasErrors())
    return Response::Error(errors.errors());
  return Response::OK();
}

}  // namespace blink
