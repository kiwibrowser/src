/*
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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

#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"

#include <math.h>

#include <memory>

#include "base/location.h"
#include "build/build_config.h"
#include "gpu/config/gpu_feature_info.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_messages.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"
#include "third_party/blink/renderer/core/css/css_font_selector.h"
#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/fileapi/file.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_async_blob_creator.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_context_creation_attributes_core.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_draw_listener.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_font_cache.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_rendering_context.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_rendering_context_factory.h"
#include "third_party/blink/renderer/core/html/canvas/image_data.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/html/forms/html_select_element.h"
#include "third_party/blink/renderer/core/html/html_image_element.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/imagebitmap/image_bitmap.h"
#include "third_party/blink/renderer/core/imagebitmap/image_bitmap_options.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/core/layout/hit_test_canvas_result.h"
#include "third_party/blink/renderer/core/layout/layout_html_canvas.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trials.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_timing.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/graphics/canvas_2d_layer_bridge.h"
#include "third_party/blink/renderer/platform/graphics/canvas_heuristic_parameters.h"
#include "third_party/blink/renderer/platform/graphics/canvas_metrics.h"
#include "third_party/blink/renderer/platform/graphics/gpu/shared_gpu_context.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/image_data_buffer.h"
#include "third_party/blink/renderer/platform/graphics/offscreen_canvas_frame_dispatcher.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_canvas.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/image-encoders/image_encoder_utils.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/checked_numeric.h"
#include "v8/include/v8.h"

namespace blink {

using namespace HTMLNames;

namespace {

// These values come from the WhatWG spec.
constexpr int kDefaultCanvasWidth = 300;
constexpr int kDefaultCanvasHeight = 150;

#if defined(OS_ANDROID)
// We estimate that the max limit for android phones is a quarter of that for
// desktops based on local experimental results on Android One.
constexpr int kMaxGlobalAcceleratedResourceCount = 25;
#else
constexpr int kMaxGlobalAcceleratedResourceCount = 100;
#endif

// We estimate the max limit of GPU allocated memory for canvases before Chrome
// becomes laggy by setting the total allocated memory for accelerated canvases
// to be equivalent to memory used by 100 accelerated canvases, each has a size
// of 1000*500 and 2d context.
// Each such canvas occupies 4000000 = 1000 * 500 * 2 * 4 bytes, where 2 is the
// gpuBufferCount in UpdateMemoryUsage() and 4 means four bytes per pixel per
// buffer.
constexpr int kMaxGlobalGPUMemoryUsage =
    4000000 * kMaxGlobalAcceleratedResourceCount;

// A default value of quality argument for toDataURL and toBlob
// It is in an invalid range (outside 0.0 - 1.0) so that it will not be
// misinterpreted as a user-input value
constexpr int kUndefinedQualityValue = -1.0;

}  // namespace

inline HTMLCanvasElement::HTMLCanvasElement(Document& document)
    : HTMLElement(canvasTag, document),
      ContextLifecycleObserver(&document),
      PageVisibilityObserver(document.GetPage()),
      size_(kDefaultCanvasWidth, kDefaultCanvasHeight),
      context_creation_was_blocked_(false),
      ignore_reset_(false),
      origin_clean_(true),
      did_fail_to_create_resource_provider_(false),
      resource_provider_is_clear_(false),
      surface_layer_bridge_(nullptr),
      gpu_memory_usage_(0),
      externally_allocated_memory_(0),
      gpu_readback_invoked_in_current_frame_(false),
      gpu_readback_successive_frames_(0) {
  CanvasMetrics::CountCanvasContextUsage(CanvasMetrics::kCanvasCreated);
  UseCounter::Count(document, WebFeature::kHTMLCanvasElement);
}

DEFINE_NODE_FACTORY(HTMLCanvasElement)

intptr_t HTMLCanvasElement::global_gpu_memory_usage_ = 0;
unsigned HTMLCanvasElement::global_accelerated_context_count_ = 0;

HTMLCanvasElement::~HTMLCanvasElement() {
  if (surface_layer_bridge_ && surface_layer_bridge_->GetCcLayer()) {
    GraphicsLayer::UnregisterContentsLayer(surface_layer_bridge_->GetCcLayer());
  }
  v8::Isolate::GetCurrent()->AdjustAmountOfExternalAllocatedMemory(
      -externally_allocated_memory_);
}

void HTMLCanvasElement::Dispose() {
  if (PlaceholderFrame())
    ReleasePlaceholderFrame();

  if (context_) {
    context_->DetachHost();
    context_ = nullptr;
  }

  if (canvas2d_bridge_) {
    canvas2d_bridge_->SetCanvasResourceHost(nullptr);
    canvas2d_bridge_ = nullptr;
  }

  if (gpu_memory_usage_) {
    DCHECK_GT(global_accelerated_context_count_, 0u);
    global_accelerated_context_count_--;
  }
  global_gpu_memory_usage_ -= gpu_memory_usage_;
}

void HTMLCanvasElement::ParseAttribute(
    const AttributeModificationParams& params) {
  if (params.name == widthAttr || params.name == heightAttr)
    Reset();
  HTMLElement::ParseAttribute(params);
}

LayoutObject* HTMLCanvasElement::CreateLayoutObject(
    const ComputedStyle& style) {
  LocalFrame* frame = GetDocument().GetFrame();
  if (frame && GetDocument().CanExecuteScripts(kNotAboutToExecuteScript))
    return new LayoutHTMLCanvas(this);
  return HTMLElement::CreateLayoutObject(style);
}

Node::InsertionNotificationRequest HTMLCanvasElement::InsertedInto(
    ContainerNode* node) {
  SetIsInCanvasSubtree(true);
  return HTMLElement::InsertedInto(node);
}

void HTMLCanvasElement::setHeight(unsigned value,
                                  ExceptionState& exception_state) {
  if (IsPlaceholderRegistered()) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "Cannot resize canvas after call to transferControlToOffscreen().");
    return;
  }
  SetUnsignedIntegralAttribute(heightAttr, value, kDefaultCanvasHeight);
}

void HTMLCanvasElement::setWidth(unsigned value,
                                 ExceptionState& exception_state) {
  if (IsPlaceholderRegistered()) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "Cannot resize canvas after call to transferControlToOffscreen().");
    return;
  }
  SetUnsignedIntegralAttribute(widthAttr, value, kDefaultCanvasWidth);
}

void HTMLCanvasElement::SetSize(const IntSize& new_size) {
  if (new_size == Size())
    return;
  ignore_reset_ = true;
  SetIntegralAttribute(widthAttr, new_size.Width());
  SetIntegralAttribute(heightAttr, new_size.Height());
  ignore_reset_ = false;
  Reset();
}

HTMLCanvasElement::ContextFactoryVector&
HTMLCanvasElement::RenderingContextFactories() {
  DCHECK(IsMainThread());
  DEFINE_STATIC_LOCAL(ContextFactoryVector, context_factories,
                      (CanvasRenderingContext::kContextTypeCount));
  return context_factories;
}

CanvasRenderingContextFactory* HTMLCanvasElement::GetRenderingContextFactory(
    int type) {
  DCHECK(type < CanvasRenderingContext::kContextTypeCount);
  return RenderingContextFactories()[type].get();
}

void HTMLCanvasElement::RegisterRenderingContextFactory(
    std::unique_ptr<CanvasRenderingContextFactory> rendering_context_factory) {
  CanvasRenderingContext::ContextType type =
      rendering_context_factory->GetContextType();
  DCHECK(type < CanvasRenderingContext::kContextTypeCount);
  DCHECK(!RenderingContextFactories()[type]);
  RenderingContextFactories()[type] = std::move(rendering_context_factory);
}

CanvasRenderingContext* HTMLCanvasElement::GetCanvasRenderingContext(
    const String& type,
    const CanvasContextCreationAttributesCore& attributes) {
  CanvasRenderingContext::ContextType context_type =
      CanvasRenderingContext::ContextTypeFromId(type);

  // Unknown type.
  if (context_type == CanvasRenderingContext::kContextTypeCount ||
      (context_type == CanvasRenderingContext::kContextXRPresent &&
       !OriginTrials::webXREnabled(&GetDocument())))
    return nullptr;

  // Log the aliased context type used.
  if (!context_) {
    DEFINE_STATIC_LOCAL(
        EnumerationHistogram, context_type_histogram,
        ("Canvas.ContextType", CanvasRenderingContext::kContextTypeCount));
    context_type_histogram.Count(context_type);
  }

  context_type =
      CanvasRenderingContext::ResolveContextTypeAliases(context_type);

  CanvasRenderingContextFactory* factory =
      GetRenderingContextFactory(context_type);
  if (!factory)
    return nullptr;

  // FIXME - The code depends on the context not going away once created, to
  // prevent JS from seeing a dangling pointer. So for now we will disallow the
  // context from being changed once it is created.
  if (context_) {
    if (context_->GetContextType() == context_type)
      return context_.Get();

    factory->OnError(this,
                     "Canvas has an existing context of a different type");
    return nullptr;
  }

  context_ = factory->Create(this, attributes);
  if (!context_)
    return nullptr;

  context_creation_was_blocked_ = false;

  probe::didCreateCanvasContext(&GetDocument());

  if (Is3d()) {
    UpdateMemoryUsage();
  }

  LayoutObject* layout_object = GetLayoutObject();
  if (layout_object && Is2d() && !context_->CreationAttributes().alpha) {
    // In the alpha false case, canvas is initially opaque, so we need to
    // trigger an invalidation.
    DidDraw();
  }

  if (attributes.low_latency &&
      OriginTrials::lowLatencyCanvasEnabled(&GetDocument())) {
    CreateLayer();
    SetNeedsUnbufferedInputEvents(true);
    // TODO(fserb): rename to CanvasFrameDispatcher
    frame_dispatcher_ = std::make_unique<OffscreenCanvasFrameDispatcher>(
        nullptr, surface_layer_bridge_->GetFrameSinkId().client_id(),
        surface_layer_bridge_->GetFrameSinkId().sink_id(),
        OffscreenCanvasFrameDispatcher::kInvalidPlaceholderCanvasId, size_);
  }

  SetNeedsCompositingUpdate();

  return context_.Get();
}

bool HTMLCanvasElement::ShouldBeDirectComposited() const {
  return (context_ && context_->IsComposited()) || (!!surface_layer_bridge_);
}

bool HTMLCanvasElement::IsAccelerated() const {
  return context_ && context_->IsAccelerated();
}

bool HTMLCanvasElement::IsWebGL1Enabled() const {
  Document& document = GetDocument();
  LocalFrame* frame = document.GetFrame();
  if (frame) {
    Settings* settings = frame->GetSettings();
    if (settings && settings->GetWebGL1Enabled())
      return true;
  }
  return false;
}

bool HTMLCanvasElement::IsWebGL2Enabled() const {
  Document& document = GetDocument();
  LocalFrame* frame = document.GetFrame();
  if (frame) {
    Settings* settings = frame->GetSettings();
    if (settings && settings->GetWebGL2Enabled())
      return true;
  }
  return false;
}

bool HTMLCanvasElement::IsWebGLBlocked() const {
  Document& document = GetDocument();
  LocalFrame* frame = document.GetFrame();
  if (frame && frame->Client()->ShouldBlockWebGL())
    return true;
  return false;
}

void HTMLCanvasElement::DidDraw(const FloatRect& rect) {
  if (rect.IsEmpty())
    return;
  resource_provider_is_clear_ = false;
  ClearCopiedImage();
  if (GetLayoutObject() && !LowLatencyEnabled())
    GetLayoutObject()->SetMayNeedPaintInvalidation();
  if (Is2d() && context_->ShouldAntialias() && GetPage() &&
      GetPage()->DeviceScaleFactorDeprecated() > 1.0f) {
    FloatRect inflated_rect = rect;
    inflated_rect.Inflate(1);
    dirty_rect_.Unite(inflated_rect);
  } else {
    dirty_rect_.Unite(rect);
  }
  if (Is2d() && canvas2d_bridge_)
    canvas2d_bridge_->DidDraw(rect);
}

void HTMLCanvasElement::DidDraw() {
  DidDraw(FloatRect(0, 0, Size().Width(), Size().Height()));
}

void HTMLCanvasElement::FinalizeFrame() {
  TRACE_EVENT0("blink", "HTMLCanvasElement::FinalizeFrame");

  // FinalizeFrame indicates the end of a script task that may have rendered
  // into the canvas, now is a good time to unlock cache entries.
  auto* resource_provider = canvas2d_bridge_
                                ? canvas2d_bridge_->GetResourceProvider()
                                : ResourceProviderForWebGL();
  if (resource_provider)
    resource_provider->ReleaseLockedImages();

  if (canvas2d_bridge_) {
    // Compute to determine whether disable accleration is needed
    if (IsAccelerated() &&
        CanvasHeuristicParameters::kGPUReadbackForcesNoAcceleration &&
        !RuntimeEnabledFeatures::Canvas2dFixedRenderingModeEnabled()) {
      if (gpu_readback_invoked_in_current_frame_) {
        gpu_readback_successive_frames_++;
        gpu_readback_invoked_in_current_frame_ = false;
      } else {
        gpu_readback_successive_frames_ = 0;
      }

      if (gpu_readback_successive_frames_ >=
          CanvasHeuristicParameters::kGPUReadbackMinSuccessiveFrames) {
        DisableAcceleration();
      }
    }

    if (!LowLatencyEnabled())
      canvas2d_bridge_->FinalizeFrame();

    if (LowLatencyEnabled() && !dirty_rect_.IsEmpty()) {
      // Push a frame
      double start_time = WTF::CurrentTimeTicksInSeconds();
      scoped_refptr<StaticBitmapImage> image =
          canvas2d_bridge_->NewImageSnapshot(kPreferAcceleration);
      FloatRect src_rect(0, 0, Size().Width(), Size().Height());
      dirty_rect_.Intersect(src_rect);
      IntRect int_dirty = EnclosingIntRect(dirty_rect_);
      SkIRect damage_rect = SkIRect::MakeXYWH(
          int_dirty.X(), int_dirty.Y(), int_dirty.Width(), int_dirty.Height());
      frame_dispatcher_->DispatchFrame(image, start_time, damage_rect);
      (void)start_time;
      (void)damage_rect;
      dirty_rect_ = FloatRect();
    }
  }

  // If the canvas is visible, notifying listeners is taken
  // care of in the in doDeferredPaintInvalidation, which allows
  // the frame to be grabbed prior to compositing, which is
  // critically important because compositing may clear the canvas's
  // image. (e.g. WebGL context with preserveDrawingBuffer=false).
  // If the canvas is not visible, doDeferredPaintInvalidation
  // will not get called, so we need to take care of business here.
  if (!did_notify_listeners_for_current_frame_)
    NotifyListenersCanvasChanged();
  did_notify_listeners_for_current_frame_ = false;
}

void HTMLCanvasElement::DisableAcceleration(
    std::unique_ptr<Canvas2DLayerBridge>
        unaccelerated_bridge_used_for_testing) {
  // Create and configure an unaccelerated Canvas2DLayerBridge.
  std::unique_ptr<Canvas2DLayerBridge> bridge;
  if (unaccelerated_bridge_used_for_testing) {
    bridge = std::move(unaccelerated_bridge_used_for_testing);
  } else {
    bridge = CreateUnaccelerated2dBuffer();
  }

  if (bridge && canvas2d_bridge_) {
    ReplaceExisting2dLayerBridge(std::move(bridge));
    UpdateMemoryUsage();
  }

  // We must force a paint invalidation on the canvas even if it's
  // content did not change because it layer was destroyed.
  DidDraw();
  SetNeedsCompositingUpdate();
}

void HTMLCanvasElement::SetNeedsCompositingUpdate() {
  Element::SetNeedsCompositingUpdate();
}

void HTMLCanvasElement::DoDeferredPaintInvalidation() {
  DCHECK(!dirty_rect_.IsEmpty());
  if (LowLatencyEnabled()) {
    // Low latency canvas handles dirty propagation in FinalizeFrame();
    return;
  }
  LayoutBox* layout_box = GetLayoutBox();
  if (Is2d()) {
    FloatRect src_rect(0, 0, Size().Width(), Size().Height());
    dirty_rect_.Intersect(src_rect);

    FloatRect invalidation_rect;
    if (layout_box) {
      FloatRect content_rect(layout_box->ContentBoxRect());
      FloatRect mapped_dirty_rect =
          MapRect(dirty_rect_, src_rect, content_rect);
      if (context_->IsComposited()) {
        // Accelerated 2D canvases need the dirty rect to be expressed relative
        // to the content box, as opposed to the layout box.
        mapped_dirty_rect.MoveBy(-content_rect.Location());
      }
      invalidation_rect = mapped_dirty_rect;
    } else {
      invalidation_rect = dirty_rect_;
    }

    if (dirty_rect_.IsEmpty())
      return;

    if (canvas2d_bridge_) {
      canvas2d_bridge_->DoPaintInvalidation(invalidation_rect);
    }
  }

  if (context_ && HasImageBitmapContext() && context_->CcLayer()) {
    context_->CcLayer()->SetNeedsDisplay();
  }

  NotifyListenersCanvasChanged();
  did_notify_listeners_for_current_frame_ = true;

  // Propagate the |dirty_rect_| accumulated so far to the compositor
  // before restarting with a blank dirty rect.
  // Canvas content updates do not need to be propagated as
  // paint invalidations if the canvas is composited separately, since
  // the canvas contents are sent separately through a texture layer.
  if (layout_box && (!context_ || !context_->IsComposited())) {
    // If the content box is larger than |src_rect|, the canvas's image is
    // being stretched, so we need to account for color bleeding caused by the
    // interpolation filter.
    FloatRect src_rect(0, 0, Size().Width(), Size().Height());
    FloatRect content_rect(layout_box->ContentBoxRect());
    if (content_rect.Width() > src_rect.Width() ||
        content_rect.Height() > src_rect.Height()) {
      dirty_rect_.Inflate(0.5);
    }

    dirty_rect_.Intersect(src_rect);
    LayoutRect mapped_dirty_rect(
        EnclosingIntRect(MapRect(dirty_rect_, src_rect, content_rect)));
    // For querying PaintLayer::GetCompositingState()
    // FIXME: is this invalidation using the correct compositing state?
    DisableCompositingQueryAsserts disabler;
    layout_box->InvalidatePaintRectangle(mapped_dirty_rect);
  }
  dirty_rect_ = FloatRect();

  DCHECK(dirty_rect_.IsEmpty());
}

void HTMLCanvasElement::Reset() {
  if (ignore_reset_)
    return;

  dirty_rect_ = FloatRect();

  bool had_resource_provider = HasResourceProvider();

  unsigned w = 0;
  AtomicString value = getAttribute(widthAttr);
  if (value.IsEmpty() || !ParseHTMLNonNegativeInteger(value, w) ||
      w > 0x7fffffffu)
    w = kDefaultCanvasWidth;

  unsigned h = 0;
  value = getAttribute(heightAttr);
  if (value.IsEmpty() || !ParseHTMLNonNegativeInteger(value, h) ||
      h > 0x7fffffffu)
    h = kDefaultCanvasHeight;

  if (Is2d()) {
    context_->Reset();
    origin_clean_ = true;
  }

  IntSize old_size = Size();
  IntSize new_size(w, h);

  // If the size of an existing buffer matches, we can just clear it instead of
  // reallocating.  This optimization is only done for 2D canvases for now.
  if (had_resource_provider && old_size == new_size && Is2d()) {
    if (!resource_provider_is_clear_) {
      resource_provider_is_clear_ = true;
      context_->ClearRect(0, 0, width(), height());
    }
    return;
  }

  SetSurfaceSize(new_size);

  if (Is3d() && old_size != Size())
    context_->Reshape(width(), height());

  if (LayoutObject* layout_object = GetLayoutObject()) {
    if (layout_object->IsCanvas()) {
      if (old_size != Size()) {
        ToLayoutHTMLCanvas(layout_object)->CanvasSizeChanged();
        if (GetLayoutBox() && GetLayoutBox()->HasAcceleratedCompositing())
          GetLayoutBox()->ContentChanged(kCanvasChanged);
      }
      if (had_resource_provider)
        layout_object->SetShouldDoFullPaintInvalidation();
    }
  }
}

bool HTMLCanvasElement::PaintsIntoCanvasBuffer() const {
  if (PlaceholderFrame())
    return false;
  DCHECK(context_);
  if (!context_->IsComposited())
    return true;
  if (GetLayoutBox() && GetLayoutBox()->HasAcceleratedCompositing())
    return false;

  return true;
}

CanvasColorParams HTMLCanvasElement::ColorParams() const {
  if (context_)
    return context_->ColorParams();
  return CanvasColorParams();
}

void HTMLCanvasElement::NotifyListenersCanvasChanged() {
  if (listeners_.size() == 0)
    return;

  if (!OriginClean()) {
    listeners_.clear();
    return;
  }

  bool listener_needs_new_frame_capture = false;
  for (const CanvasDrawListener* listener : listeners_) {
    if (listener->NeedsNewFrame()) {
      listener_needs_new_frame_capture = true;
    }
  }

  if (listener_needs_new_frame_capture) {
    SourceImageStatus status;
    scoped_refptr<Image> source_image =
        GetSourceImageForCanvas(&status, kPreferNoAcceleration, FloatSize());
    if (status != kNormalSourceImageStatus)
      return;
    sk_sp<SkImage> image =
        source_image->PaintImageForCurrentFrame().GetSkImage();
    for (CanvasDrawListener* listener : listeners_) {
      if (listener->NeedsNewFrame()) {
        listener->SendNewFrame(image, source_image->ContextProviderWrapper());
      }
    }
  }
}

void HTMLCanvasElement::Paint(GraphicsContext& context, const LayoutRect& r) {
  if (context_creation_was_blocked_ ||
      (context_ && context_->isContextLost())) {
    float device_scale_factor =
        blink::DeviceScaleFactorDeprecated(GetDocument().GetFrame());
    std::pair<Image*, float> broken_canvas_and_image_scale_factor =
        ImageResourceContent::BrokenCanvas(device_scale_factor);
    Image* broken_canvas = broken_canvas_and_image_scale_factor.first;
    context.Save();
    context.FillRect(FloatRect(r), Color(), SkBlendMode::kClear);
    // Place the icon near the upper left, like the missing image icon
    // for image elements. Offset it a bit from the upper corner.
    FloatSize icon_size(broken_canvas->Size());
    FloatPoint upper_left =
        FloatPoint(r.PixelSnappedLocation()) + icon_size.ScaledBy(0.5f);
    context.DrawImage(broken_canvas, Image::kSyncDecode,
                      FloatRect(upper_left, icon_size));
    context.Restore();
    return;
  }

  // FIXME: crbug.com/438240; there is a bug with the new CSS blending and
  // compositing feature.
  if (!context_ && !PlaceholderFrame())
    return;

  const ComputedStyle* style = EnsureComputedStyle();
  SkFilterQuality filter_quality =
      (style && style->ImageRendering() == EImageRendering::kPixelated)
          ? kNone_SkFilterQuality
          : kLow_SkFilterQuality;

  if (Is3d()) {
    context_->SetFilterQuality(filter_quality);
  } else if (canvas2d_bridge_) {
    canvas2d_bridge_->SetFilterQuality(filter_quality);
  }

  if (HasResourceProvider() && !resource_provider_is_clear_)
    PaintTiming::From(GetDocument()).MarkFirstContentfulPaint();

  if (!PaintsIntoCanvasBuffer() && !GetDocument().Printing())
    return;

  if (PlaceholderFrame()) {
    DCHECK(GetDocument().Printing());
    context.DrawImage(PlaceholderFrame().get(), Image::kSyncDecode,
                      PixelSnappedIntRect(r));
    return;
  }

  context_->PaintRenderingResultsToCanvas(kFrontBuffer);
  if (HasResourceProvider()) {
    if (!context.ContextDisabled()) {
      SkBlendMode composite_operator =
          !context_ || context_->CreationAttributes().alpha
              ? SkBlendMode::kSrcOver
              : SkBlendMode::kSrc;
      FloatRect src_rect = FloatRect(FloatPoint(), FloatSize(Size()));
      scoped_refptr<StaticBitmapImage> snapshot =
          canvas2d_bridge_
              ? canvas2d_bridge_->NewImageSnapshot(kPreferAcceleration)
              : (webgl_resource_provider_ ? webgl_resource_provider_->Snapshot()
                                          : nullptr);
      if (snapshot) {
        // GraphicsContext cannot handle gpu resource serialization.
        snapshot = snapshot->MakeUnaccelerated();
        DCHECK(!snapshot->IsTextureBacked());
        context.DrawImage(snapshot.get(), Image::kSyncDecode,
                          PixelSnappedIntRect(r), &src_rect,
                          composite_operator);
      }
    }
  } else {
    // When alpha is false, we should draw to opaque black.
    if (!context_->CreationAttributes().alpha)
      context.FillRect(FloatRect(r), Color(0, 0, 0));
  }

  if (Is3d() && PaintsIntoCanvasBuffer())
    context_->MarkLayerComposited();
}

bool HTMLCanvasElement::Is3d() const {
  return context_ && context_->Is3d();
}

bool HTMLCanvasElement::Is2d() const {
  return context_ && context_->Is2d();
}

bool HTMLCanvasElement::IsAnimated2d() const {
  return Is2d() && canvas2d_bridge_ &&
         canvas2d_bridge_->WasDrawnToAfterSnapshot();
}

void HTMLCanvasElement::SetSurfaceSize(const IntSize& size) {
  size_ = size;
  did_fail_to_create_resource_provider_ = false;
  DiscardResourceProvider();
  ClearCopiedImage();
  if (Is2d() && context_->isContextLost()) {
    context_->DidSetSurfaceSize();
  }
  if (frame_dispatcher_)
    frame_dispatcher_->Reshape(size_);
}

const AtomicString HTMLCanvasElement::ImageSourceURL() const {
  return AtomicString(
      ToDataURLInternal(ImageEncoderUtils::kDefaultMimeType, 0, kFrontBuffer));
}

scoped_refptr<StaticBitmapImage> HTMLCanvasElement::ToStaticBitmapImage(
    SourceDrawingBuffer source_buffer,
    AccelerationHint hint) const {
  if (size_.IsEmpty())
    return nullptr;
  scoped_refptr<StaticBitmapImage> image_bitmap = nullptr;
  if (Is3d()) {
    if (context_->CreationAttributes().premultiplied_alpha) {
      context_->PaintRenderingResultsToCanvas(source_buffer);
      if (webgl_resource_provider_)
        image_bitmap = webgl_resource_provider_->Snapshot();
    } else {
      scoped_refptr<Uint8Array> data_array =
          context_->PaintRenderingResultsToDataArray(source_buffer);
      if (data_array) {
        // If the accelerated canvas is too big, there is a logic in WebGL code
        // path that scales down the drawing buffer to the maximum supported
        // size. Hence, we need to query the adjusted size of DrawingBuffer.
        IntSize adjusted_size = context_->DrawingBufferSize();
        SkImageInfo info =
            SkImageInfo::Make(adjusted_size.Width(), adjusted_size.Height(),
                              kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
        info = info.makeColorSpace(ColorParams().GetSkColorSpace());
        if (ColorParams().GetSkColorType() != kN32_SkColorType)
          info = info.makeColorType(kRGBA_F16_SkColorType);
        image_bitmap = StaticBitmapImage::Create(std::move(data_array), info);
      }
    }
  } else if (context_ || PlaceholderFrame()) {
    DCHECK(Is2d() || PlaceholderFrame());
    if (canvas2d_bridge_) {
      image_bitmap = canvas2d_bridge_->NewImageSnapshot(hint);
    } else if (PlaceholderFrame()) {
      DCHECK(PlaceholderFrame()->OriginClean());
      image_bitmap = PlaceholderFrame();
    }
  }
  if (!image_bitmap)
    image_bitmap = CreateTransparentImage(size_);
  return image_bitmap;
}

String HTMLCanvasElement::ToDataURLInternal(
    const String& mime_type,
    const double& quality,
    SourceDrawingBuffer source_buffer) const {
  if (!IsPaintable())
    return String("data:,");

  String encoding_mime_type = ImageEncoderUtils::ToEncodingMimeType(
      mime_type, ImageEncoderUtils::kEncodeReasonToDataURL);

  base::Optional<ScopedUsHistogramTimer> timer;
  if (encoding_mime_type == "image/png") {
    DEFINE_THREAD_SAFE_STATIC_LOCAL(
        CustomCountHistogram, scoped_us_counter_png,
        ("Blink.Canvas.ToDataURL.PNG", 0, 10000000, 50));
    timer.emplace(scoped_us_counter_png);
  } else if (encoding_mime_type == "image/jpeg") {
    DEFINE_THREAD_SAFE_STATIC_LOCAL(
        CustomCountHistogram, scoped_us_counter_jpeg,
        ("Blink.Canvas.ToDataURL.JPEG", 0, 10000000, 50));
    timer.emplace(scoped_us_counter_jpeg);
  } else if (encoding_mime_type == "image/webp") {
    DEFINE_THREAD_SAFE_STATIC_LOCAL(
        CustomCountHistogram, scoped_us_counter_webp,
        ("Blink.Canvas.ToDataURL.WEBP", 0, 10000000, 50));
    timer.emplace(scoped_us_counter_webp);
  } else {
    // Currently we only support three encoding types.
    NOTREACHED();
  }

  scoped_refptr<StaticBitmapImage> image_bitmap =
      ToStaticBitmapImage(source_buffer, kPreferNoAcceleration);
  if (image_bitmap) {
    std::unique_ptr<ImageDataBuffer> data_buffer =
        ImageDataBuffer::Create(image_bitmap);
    if (data_buffer)
      return data_buffer->ToDataURL(encoding_mime_type, quality);
  }
  return String("data:,");
}

String HTMLCanvasElement::toDataURL(const String& mime_type,
                                    const ScriptValue& quality_argument,
                                    ExceptionState& exception_state) const {
  if (!OriginClean()) {
    exception_state.ThrowSecurityError("Tainted canvases may not be exported.");
    return String();
  }

  double quality = kUndefinedQualityValue;
  if (!quality_argument.IsEmpty()) {
    v8::Local<v8::Value> v8_value = quality_argument.V8Value();
    if (v8_value->IsNumber()) {
      quality = v8_value.As<v8::Number>()->Value();
    }
  }
  return ToDataURLInternal(mime_type, quality, kBackBuffer);
}

void HTMLCanvasElement::toBlob(V8BlobCallback* callback,
                               const String& mime_type,
                               const ScriptValue& quality_argument,
                               ExceptionState& exception_state) {
  if (!OriginClean()) {
    exception_state.ThrowSecurityError("Tainted canvases may not be exported.");
    return;
  }

  if (!IsPaintable()) {
    // If the canvas element's bitmap has no pixels
    GetDocument()
        .GetTaskRunner(TaskType::kCanvasBlobSerialization)
        ->PostTask(
            FROM_HERE,
            WTF::Bind(&V8PersistentCallbackFunction<
                          V8BlobCallback>::InvokeAndReportException,
                      WrapPersistent(ToV8PersistentCallbackFunction(callback)),
                      nullptr, nullptr));
    return;
  }

  double start_time = WTF::CurrentTimeTicksInSeconds();
  double quality = kUndefinedQualityValue;
  if (!quality_argument.IsEmpty()) {
    v8::Local<v8::Value> v8_value = quality_argument.V8Value();
    if (v8_value->IsNumber()) {
      quality = v8_value.As<v8::Number>()->Value();
    }
  }

  String encoding_mime_type = ImageEncoderUtils::ToEncodingMimeType(
      mime_type, ImageEncoderUtils::kEncodeReasonToBlobCallback);

  CanvasAsyncBlobCreator* async_creator = nullptr;
  scoped_refptr<StaticBitmapImage> image_bitmap =
      ToStaticBitmapImage(kBackBuffer, kPreferNoAcceleration);
  if (image_bitmap) {
    async_creator = CanvasAsyncBlobCreator::Create(
        image_bitmap, encoding_mime_type, callback, start_time, &GetDocument());
  }

  if (async_creator) {
    async_creator->ScheduleAsyncBlobCreation(quality);
  } else {
    GetDocument()
        .GetTaskRunner(TaskType::kCanvasBlobSerialization)
        ->PostTask(
            FROM_HERE,
            WTF::Bind(&V8PersistentCallbackFunction<
                          V8BlobCallback>::InvokeAndReportException,
                      WrapPersistent(ToV8PersistentCallbackFunction(callback)),
                      nullptr, nullptr));
    return;
  }
}

void HTMLCanvasElement::AddListener(CanvasDrawListener* listener) {
  listeners_.insert(listener);
}

void HTMLCanvasElement::RemoveListener(CanvasDrawListener* listener) {
  listeners_.erase(listener);
}

bool HTMLCanvasElement::OriginClean() const {
  if (GetDocument().GetSettings() &&
      GetDocument().GetSettings()->GetDisableReadingFromCanvas())
    return false;
  if (PlaceholderFrame())
    return PlaceholderFrame()->OriginClean();
  return origin_clean_;
}

bool HTMLCanvasElement::ShouldAccelerate(AccelerationCriteria criteria) const {
  if (context_ && !Is2d())
    return false;

  // TODO(crbug.com/789232): Make low latency mode work with GPU acceleration
  if (LowLatencyEnabled())
    return false;

  if (!RuntimeEnabledFeatures::Accelerated2dCanvasEnabled())
    return false;

  // The following is necessary for handling the special case of canvases in the
  // dev tools overlay, which run in a process that supports accelerated 2d
  // canvas but in a special compositing context that does not.
  if (GetLayoutBox() && !GetLayoutBox()->HasAcceleratedCompositing())
    return false;

  CheckedNumeric<int> checked_canvas_pixel_count = Size().Width();
  checked_canvas_pixel_count *= Size().Height();
  if (!checked_canvas_pixel_count.IsValid())
    return false;
  int canvas_pixel_count = checked_canvas_pixel_count.ValueOrDie();

  // Do not use acceleration for small canvas.
  if (criteria != kIgnoreResourceLimitCriteria) {
    Settings* settings = GetDocument().GetSettings();
    if (!settings ||
        canvas_pixel_count < settings->GetMinimumAccelerated2dCanvasSize())
      return false;

    // When GPU allocated memory runs low (due to having created too many
    // accelerated canvases), the compositor starves and browser becomes laggy.
    // Thus, we should stop allocating more GPU memory to new canvases created
    // when the current memory usage exceeds the threshold.
    if (global_gpu_memory_usage_ >= kMaxGlobalGPUMemoryUsage)
      return false;

    // Allocating too many GPU resources can makes us run into the driver's
    // resource limits. So we need to keep the number of texture resources
    // under tight control
    if (global_accelerated_context_count_ >= kMaxGlobalAcceleratedResourceCount)
      return false;
  }

  // Don't use accelerated canvas if compositor is in software mode.
  if (!SharedGpuContext::IsGpuCompositingEnabled())
    return false;

  // Avoid creating |contextProvider| until we're sure we want to try use it,
  // since it costs us GPU memory.
  base::WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper =
      SharedGpuContext::ContextProviderWrapper();
  if (!context_provider_wrapper) {
    CanvasMetrics::CountCanvasContextUsage(
        CanvasMetrics::kAccelerated2DCanvasGPUContextLost);
    return false;
  }

  const gpu::GpuFeatureInfo& gpu_feature_info =
      context_provider_wrapper->ContextProvider()->GetGpuFeatureInfo();
  if (gpu::kGpuFeatureStatusEnabled !=
      gpu_feature_info
          .status_values[gpu::GPU_FEATURE_TYPE_ACCELERATED_2D_CANVAS]) {
    // Accelerated 2D canvas is blacklisted.
    return false;
  }

  return true;
}

std::unique_ptr<Canvas2DLayerBridge>
HTMLCanvasElement::CreateAccelerated2dBuffer(int* msaa_sample_count) {
  if (GetDocument().GetSettings()) {
    *msaa_sample_count =
        GetDocument().GetSettings()->GetAccelerated2dCanvasMSAASampleCount();
  }

  auto surface = std::make_unique<Canvas2DLayerBridge>(
      Size(), *msaa_sample_count, Canvas2DLayerBridge::kEnableAcceleration,
      ColorParams());
  if (!surface->IsValid()) {
    CanvasMetrics::CountCanvasContextUsage(
        CanvasMetrics::kGPUAccelerated2DCanvasImageBufferCreationFailed);
    return nullptr;
  }

  if (MemoryCoordinator::IsLowEndDevice())
    surface->DisableDeferral(kDisableDeferralReasonLowEndDevice);

  CanvasMetrics::CountCanvasContextUsage(
      CanvasMetrics::kGPUAccelerated2DCanvasImageBufferCreated);
  return surface;
}

std::unique_ptr<Canvas2DLayerBridge>
HTMLCanvasElement::CreateUnaccelerated2dBuffer() {
  auto surface = std::make_unique<Canvas2DLayerBridge>(
      Size(), 0, Canvas2DLayerBridge::kDisableAcceleration, ColorParams());
  if (surface->IsValid()) {
    CanvasMetrics::CountCanvasContextUsage(
        CanvasMetrics::kUnaccelerated2DCanvasImageBufferCreated);
    return surface;
  }

  CanvasMetrics::CountCanvasContextUsage(
      CanvasMetrics::kUnaccelerated2DCanvasImageBufferCreationFailed);
  return nullptr;
}

void HTMLCanvasElement::CreateCanvas2DLayerBridgeInternal(
    std::unique_ptr<Canvas2DLayerBridge> external_canvas2d_bridge) {
  DCHECK(Is2d() && !canvas2d_bridge_);

  did_fail_to_create_resource_provider_ = true;
  resource_provider_is_clear_ = true;

  if (!IsValidImageSize(Size()))
    return;

  int msaa_sample_count = 0;
  if (external_canvas2d_bridge) {
    if (external_canvas2d_bridge->IsValid())
      canvas2d_bridge_ = std::move(external_canvas2d_bridge);
  } else {
    if (ShouldAccelerate(kNormalAccelerationCriteria)) {
      canvas2d_bridge_ = CreateAccelerated2dBuffer(&msaa_sample_count);
    }
    if (!canvas2d_bridge_) {
      canvas2d_bridge_ = CreateUnaccelerated2dBuffer();
    }
  }

  if (canvas2d_bridge_) {
    canvas2d_bridge_->SetCanvasResourceHost(this);
  } else {
    return;
  }

  did_fail_to_create_resource_provider_ = false;
  UpdateMemoryUsage();

  // Enabling MSAA overrides a request to disable antialiasing. This is true
  // regardless of whether the rendering mode is accelerated or not. For
  // consistency, we don't want to apply AA in accelerated canvases but not in
  // unaccelerated canvases.
  if (!msaa_sample_count && GetDocument().GetSettings() &&
      !GetDocument().GetSettings()->GetAntialiased2dCanvasEnabled())
    context_->SetShouldAntialias(false);

  if (context_)
    SetNeedsCompositingUpdate();
}

void HTMLCanvasElement::NotifyGpuContextLost() {
  if (Is2d())
    context_->LoseContext(CanvasRenderingContext::kRealLostContext);
}

void HTMLCanvasElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(listeners_);
  visitor->Trace(context_);
  ContextLifecycleObserver::Trace(visitor);
  PageVisibilityObserver::Trace(visitor);
  HTMLElement::Trace(visitor);
}

void HTMLCanvasElement::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(context_);
  HTMLElement::TraceWrappers(visitor);
}

void HTMLCanvasElement::DisableDeferral(DisableDeferralReason reason) {
  if (canvas2d_bridge_)
    canvas2d_bridge_->DisableDeferral(reason);
}

Canvas2DLayerBridge* HTMLCanvasElement::GetOrCreateCanvas2DLayerBridge() {
  DCHECK(Is2d());
  if (!canvas2d_bridge_ && !did_fail_to_create_resource_provider_) {
    CreateCanvas2DLayerBridgeInternal(nullptr);
    if (did_fail_to_create_resource_provider_ && !Size().IsEmpty()) {
      context_->LoseContext(CanvasRenderingContext::kSyntheticLostContext);
    }
  }
  return canvas2d_bridge_.get();
}

CanvasResourceProvider*
HTMLCanvasElement::GetOrCreateCanvasResourceProviderForWebGL() {
  DCHECK(Is3d());
  if (!webgl_resource_provider_ && !did_fail_to_create_resource_provider_) {
    resource_provider_is_clear_ = true;
    if (IsValidImageSize(Size())) {
      webgl_resource_provider_ = CanvasResourceProvider::Create(
          size_,
          Platform::Current()->IsGpuCompositingDisabled()
              ? CanvasResourceProvider::kSoftwareResourceUsage
              : CanvasResourceProvider::kAcceleratedResourceUsage,
          SharedGpuContext::ContextProviderWrapper(), 0, ColorParams());
    }
    if (!webgl_resource_provider_) {
      did_fail_to_create_resource_provider_ = true;
    } else {
      UpdateMemoryUsage();
    }
  }
  return webgl_resource_provider_.get();
}

void HTMLCanvasElement::CreateCanvas2DLayerBridgeForTesting(
    std::unique_ptr<Canvas2DLayerBridge> bridge,
    const IntSize& size) {
  DiscardResourceProvider();
  SetIntegralAttribute(widthAttr, size.Width());
  SetIntegralAttribute(heightAttr, size.Height());
  CreateCanvas2DLayerBridgeInternal(std::move(bridge));
}

scoped_refptr<Image> HTMLCanvasElement::CopiedImage(
    SourceDrawingBuffer source_buffer,
    AccelerationHint hint) {
  if (SurfaceLayerBridge())
    return PlaceholderFrame();

  if (!IsPaintable())
    return nullptr;

  if (!context_)
    return CreateTransparentImage(Size());

  if (HasImageBitmapContext()) {
    scoped_refptr<Image> image = context_->GetImage(hint);
    // TODO(fserb): return image?
    if (image)
      return context_->GetImage(hint);
    // Special case: transferFromImageBitmap is not yet called.
    sk_sp<SkSurface> surface =
        SkSurface::MakeRasterN32Premul(width(), height());
    return StaticBitmapImage::Create(surface->makeImageSnapshot());
  }

  bool need_to_update = !copied_image_;
  // The concept of SourceDrawingBuffer is valid on only WebGL.
  if (context_->Is3d())
    need_to_update |= context_->PaintRenderingResultsToCanvas(source_buffer);
  if (need_to_update) {
    if (Is2d() && GetOrCreateCanvas2DLayerBridge()) {
      copied_image_ = canvas2d_bridge_->NewImageSnapshot(hint);
    } else if (Is3d() && GetOrCreateCanvasResourceProviderForWebGL()) {
      copied_image_ = webgl_resource_provider_->Snapshot();
    }
    UpdateMemoryUsage();
  }
  return copied_image_;
}

void HTMLCanvasElement::DiscardResourceProvider() {
  canvas2d_bridge_.reset();
  webgl_resource_provider_.reset();
  dirty_rect_ = FloatRect();
  UpdateMemoryUsage();
}

void HTMLCanvasElement::ClearCopiedImage() {
  if (copied_image_) {
    copied_image_ = nullptr;
    UpdateMemoryUsage();
  }
}

void HTMLCanvasElement::PageVisibilityChanged() {
  bool hidden = !GetPage()->IsPageVisible();
  SetSuspendOffscreenCanvasAnimation(hidden);

  if (!context_)
    return;

  context_->SetIsHidden(hidden);
  if (hidden) {
    ClearCopiedImage();
    if (Is3d()) {
      DiscardResourceProvider();
    }
  }
}

void HTMLCanvasElement::ContextDestroyed(ExecutionContext*) {
  if (context_)
    context_->Stop();
}

void HTMLCanvasElement::StyleDidChange(const ComputedStyle* old_style,
                                       const ComputedStyle& new_style) {
  if (context_)
    context_->StyleDidChange(old_style, new_style);
}

void HTMLCanvasElement::DidMoveToNewDocument(Document& old_document) {
  ContextLifecycleObserver::SetContext(&GetDocument());
  PageVisibilityObserver::SetContext(GetDocument().GetPage());
  HTMLElement::DidMoveToNewDocument(old_document);
}

void HTMLCanvasElement::WillDrawImageTo2DContext(CanvasImageSource* source) {
  if (CanvasHeuristicParameters::kEnableAccelerationToAvoidReadbacks &&
      SharedGpuContext::AllowSoftwareToAcceleratedCanvasUpgrade() &&
      source->IsAccelerated() && GetOrCreateCanvas2DLayerBridge() &&
      !canvas2d_bridge_->IsAccelerated() &&
      ShouldAccelerate(kIgnoreResourceLimitCriteria)) {
    int msaa_sample_count = 0;
    std::unique_ptr<Canvas2DLayerBridge> surface =
        CreateAccelerated2dBuffer(&msaa_sample_count);
    if (surface) {
      ReplaceExisting2dLayerBridge(std::move(surface));
      UpdateMemoryUsage();
      SetNeedsCompositingUpdate();
    }
  }
}

scoped_refptr<Image> HTMLCanvasElement::GetSourceImageForCanvas(
    SourceImageStatus* status,
    AccelerationHint hint,
    const FloatSize&) {
  if (!width() || !height()) {
    *status = kZeroSizeCanvasSourceImageStatus;
    return nullptr;
  }

  if (!IsPaintable()) {
    *status = kInvalidSourceImageStatus;
    return nullptr;
  }

  if (PlaceholderFrame()) {
    *status = kNormalSourceImageStatus;
    return PlaceholderFrame();
  }

  if (!context_) {
    scoped_refptr<Image> result = CreateTransparentImage(Size());
    *status = result ? kNormalSourceImageStatus : kInvalidSourceImageStatus;
    return result;
  }

  if (HasImageBitmapContext()) {
    *status = kNormalSourceImageStatus;
    scoped_refptr<Image> result = context_->GetImage(hint);
    if (!result)
      result = CreateTransparentImage(Size());
    *status = result ? kNormalSourceImageStatus : kInvalidSourceImageStatus;
    return result;
  }

  scoped_refptr<Image> image;
  // TODO(ccameron): Canvas should produce sRGB images.
  // https://crbug.com/672299
  if (Is3d()) {
    // Because WebGL sources always require making a copy of the back buffer, we
    // use paintRenderingResultsToCanvas instead of getImage in order to keep a
    // cached copy of the backing in the canvas's resource provider.
    RenderingContext()->PaintRenderingResultsToCanvas(kBackBuffer);
    if (webgl_resource_provider_) {
      image = webgl_resource_provider_->Snapshot();
    } else {
      image = CreateTransparentImage(Size());
    }
  } else {
    if (CanvasHeuristicParameters::kDisableAccelerationToAvoidReadbacks &&
        !RuntimeEnabledFeatures::Canvas2dFixedRenderingModeEnabled() &&
        hint == kPreferNoAcceleration && canvas2d_bridge_ &&
        canvas2d_bridge_->IsAccelerated()) {
      DisableAcceleration();
    }
    image = RenderingContext()->GetImage(hint);
    if (!image) {
      image = CreateTransparentImage(Size());
    }
  }

  if (image) {
    *status = kNormalSourceImageStatus;
  } else {
    *status = kInvalidSourceImageStatus;
  }
  return image;
}

bool HTMLCanvasElement::WouldTaintOrigin(const SecurityOrigin*) const {
  return !OriginClean();
}

FloatSize HTMLCanvasElement::ElementSize(const FloatSize&) const {
  if (context_ && HasImageBitmapContext()) {
    scoped_refptr<Image> image = context_->GetImage(kPreferNoAcceleration);
    if (image)
      return FloatSize(image->width(), image->height());
    return FloatSize(0, 0);
  }
  if (PlaceholderFrame())
    return FloatSize(PlaceholderFrame()->Size());
  return FloatSize(width(), height());
}

IntSize HTMLCanvasElement::BitmapSourceSize() const {
  return IntSize(width(), height());
}

ScriptPromise HTMLCanvasElement::CreateImageBitmap(
    ScriptState* script_state,
    EventTarget& event_target,
    base::Optional<IntRect> crop_rect,
    const ImageBitmapOptions& options) {
  DCHECK(event_target.ToLocalDOMWindow());

  return ImageBitmapSource::FulfillImageBitmap(
      script_state, ImageBitmap::Create(this, crop_rect, options));
}

void HTMLCanvasElement::SetPlaceholderFrame(
    scoped_refptr<StaticBitmapImage> image,
    base::WeakPtr<OffscreenCanvasFrameDispatcher> dispatcher,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    unsigned resource_id) {
  OffscreenCanvasPlaceholder::SetPlaceholderFrame(
      std::move(image), std::move(dispatcher), std::move(task_runner),
      resource_id);
  IntSize new_size(PlaceholderFrame()->width(), PlaceholderFrame()->height());
  SetSize(new_size);
  NotifyListenersCanvasChanged();
}

bool HTMLCanvasElement::IsOpaque() const {
  return context_ && !context_->CreationAttributes().alpha;
}

bool HTMLCanvasElement::IsSupportedInteractiveCanvasFallback(
    const Element& element) {
  if (!element.IsDescendantOf(this))
    return false;

  // An element is a supported interactive canvas fallback element if it is one
  // of the following:
  // https://html.spec.whatwg.org/multipage/scripting.html#supported-interactive-canvas-fallback-element

  // An a element that represents a hyperlink and that does not have any img
  // descendants.
  if (IsHTMLAnchorElement(element))
    return !Traversal<HTMLImageElement>::FirstWithin(element);

  // A button element
  if (IsHTMLButtonElement(element))
    return true;

  // An input element whose type attribute is in one of the Checkbox or Radio
  // Button states.  An input element that is a button but its type attribute is
  // not in the Image Button state.
  if (auto* input_element = ToHTMLInputElementOrNull(element)) {
    if (input_element->type() == InputTypeNames::checkbox ||
        input_element->type() == InputTypeNames::radio ||
        input_element->IsTextButton())
      return true;
  }

  // A select element with a "multiple" attribute or with a display size greater
  // than 1.
  if (auto* select_element = ToHTMLSelectElementOrNull(element)) {
    if (select_element->IsMultiple() || select_element->size() > 1)
      return true;
  }

  // An option element that is in a list of options of a select element with a
  // "multiple" attribute or with a display size greater than 1.
  if (IsHTMLOptionElement(element) && element.parentNode() &&
      IsHTMLSelectElement(*element.parentNode())) {
    const HTMLSelectElement& select_element =
        ToHTMLSelectElement(*element.parentNode());
    if (select_element.IsMultiple() || select_element.size() > 1)
      return true;
  }

  // An element that would not be interactive content except for having the
  // tabindex attribute specified.
  if (element.FastHasAttribute(HTMLNames::tabindexAttr))
    return true;

  // A non-interactive table, caption, thead, tbody, tfoot, tr, td, or th
  // element.
  if (IsHTMLTableElement(element) ||
      element.HasTagName(HTMLNames::captionTag) ||
      element.HasTagName(HTMLNames::theadTag) ||
      element.HasTagName(HTMLNames::tbodyTag) ||
      element.HasTagName(HTMLNames::tfootTag) ||
      element.HasTagName(HTMLNames::trTag) ||
      element.HasTagName(HTMLNames::tdTag) ||
      element.HasTagName(HTMLNames::thTag))
    return true;

  return false;
}

HitTestCanvasResult* HTMLCanvasElement::GetControlAndIdIfHitRegionExists(
    const LayoutPoint& location) {
  if (Is2d())
    return context_->GetControlAndIdIfHitRegionExists(location);
  return HitTestCanvasResult::Create(String(), nullptr);
}

String HTMLCanvasElement::GetIdFromControl(const Element* element) {
  if (context_)
    return context_->GetIdFromControl(element);
  return String();
}

void HTMLCanvasElement::CreateLayer() {
  DCHECK(!surface_layer_bridge_);
  LocalFrame* frame = GetDocument().GetFrame();
  WebLayerTreeView* layer_tree_view = nullptr;
  // We do not design transferControlToOffscreen() for frame-less HTML canvas.
  if (frame) {
    layer_tree_view =
        frame->GetPage()->GetChromeClient().GetWebLayerTreeView(frame);
    surface_layer_bridge_ =
        std::make_unique<::blink::SurfaceLayerBridge>(layer_tree_view, this);
    // Creates a placeholder layer first before Surface is created.
    surface_layer_bridge_->CreateSolidColorLayer();
  }
}

void HTMLCanvasElement::OnWebLayerUpdated() {
  SetNeedsCompositingUpdate();
}

void HTMLCanvasElement::RegisterContentsLayer(cc::Layer* layer) {
  GraphicsLayer::RegisterContentsLayer(layer);
}

void HTMLCanvasElement::UnregisterContentsLayer(cc::Layer* layer) {
  GraphicsLayer::UnregisterContentsLayer(layer);
}

FontSelector* HTMLCanvasElement::GetFontSelector() {
  return GetDocument().GetStyleEngine().GetFontSelector();
}

void HTMLCanvasElement::UpdateMemoryUsage() {
  int non_gpu_buffer_count = 0;
  int gpu_buffer_count = 0;
  if (Is2d() && canvas2d_bridge_) {
    non_gpu_buffer_count++;
    if (canvas2d_bridge_->IsAccelerated()) {
      // The number of internal GPU buffers vary between one (stable
      // non-displayed state) and three (triple-buffered animations).
      // Adding 2 is a pessimistic but relevant estimate.
      // Note: These buffers might be allocated in GPU memory.
      gpu_buffer_count += 2;
    }
  }

  if (copied_image_)
    non_gpu_buffer_count++;

  if (Is3d()) {
    if (webgl_resource_provider_) {
      non_gpu_buffer_count++;
      if (webgl_resource_provider_->IsAccelerated()) {
        gpu_buffer_count += 2;
      }
    }
    non_gpu_buffer_count += context_->ExternallyAllocatedBufferCountPerPixel();
  }

  int bytes_per_pixel = ColorParams().BytesPerPixel();

  // Re-computation of gpu memory usage is only carried out when there is a
  // a change from acceleration to non-accleration or vice versa.
  if (gpu_buffer_count && !gpu_memory_usage_) {
    // Switch from non-acceleration mode to acceleration mode
    CheckedNumeric<intptr_t> checked_usage = gpu_buffer_count * bytes_per_pixel;
    checked_usage *= width();
    checked_usage *= height();
    intptr_t gpu_memory_usage =
        checked_usage.ValueOrDefault(std::numeric_limits<intptr_t>::max());

    global_gpu_memory_usage_ += (gpu_memory_usage - gpu_memory_usage_);
    gpu_memory_usage_ = gpu_memory_usage;
    global_accelerated_context_count_++;
  } else if (!gpu_buffer_count && gpu_memory_usage_) {
    // Switch from acceleration mode to non-acceleration mode
    DCHECK_GT(global_accelerated_context_count_, 0u);
    global_accelerated_context_count_--;
    global_gpu_memory_usage_ -= gpu_memory_usage_;
    gpu_memory_usage_ = 0;
  }

  // Recomputation of externally memory usage computation is carried out
  // in all cases.
  CheckedNumeric<intptr_t> checked_usage =
      non_gpu_buffer_count * bytes_per_pixel;
  checked_usage *= width();
  checked_usage *= height();
  checked_usage += gpu_memory_usage_;
  intptr_t externally_allocated_memory =
      checked_usage.ValueOrDefault(std::numeric_limits<intptr_t>::max());
  // Subtracting two intptr_t that are known to be positive will never
  // underflow.
  v8::Isolate::GetCurrent()->AdjustAmountOfExternalAllocatedMemory(
      externally_allocated_memory - externally_allocated_memory_);
  externally_allocated_memory_ = externally_allocated_memory;
}

void HTMLCanvasElement::ReplaceExisting2dLayerBridge(
    std::unique_ptr<Canvas2DLayerBridge> new_buffer) {
  if (canvas2d_bridge_) {
    scoped_refptr<StaticBitmapImage> image =
        canvas2d_bridge_->NewImageSnapshot(kPreferNoAcceleration);

    // image can be null if alloaction failed in which case we should just
    // abort the surface switch to reatain the old surface which is still
    // functional.
    if (!image)
      return;
    new_buffer->DrawFullImage(image->PaintImageForCurrentFrame());
  }

  RestoreCanvasMatrixClipStack(new_buffer->Canvas());
  new_buffer->DidRestoreCanvasMatrixClipStack(new_buffer->Canvas());
  canvas2d_bridge_ = std::move(new_buffer);
  canvas2d_bridge_->SetCanvasResourceHost(this);
}

bool HTMLCanvasElement::HasImageBitmapContext() const {
  if (!context_)
    return false;
  CanvasRenderingContext::ContextType type = context_->GetContextType();
  return (type == CanvasRenderingContext::kContextImageBitmap ||
          type == CanvasRenderingContext::kContextXRPresent);
}

}  // namespace blink
