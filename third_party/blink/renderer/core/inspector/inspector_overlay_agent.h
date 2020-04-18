/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_OVERLAY_AGENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_OVERLAY_AGENT_H_

#include <v8-inspector.h>
#include <memory>
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/inspector/inspector_base_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_highlight.h"
#include "third_party/blink/renderer/core/inspector/inspector_overlay_host.h"
#include "third_party/blink/renderer/core/inspector/protocol/Overlay.h"
#include "third_party/blink/renderer/platform/geometry/float_quad.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/graphics/color.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class Color;
class GraphicsLayer;
class InspectedFrames;
class InspectorDOMAgent;
class LocalFrame;
class Node;
class Page;
class PageOverlay;
class WebGestureEvent;
class WebMouseEvent;
class WebLocalFrameImpl;
class WebPointerEvent;

class CORE_EXPORT InspectorOverlayAgent final
    : public InspectorBaseAgent<protocol::Overlay::Metainfo>,
      public InspectorOverlayHost::Listener {
  USING_GARBAGE_COLLECTED_MIXIN(InspectorOverlayAgent);

 public:
  InspectorOverlayAgent(WebLocalFrameImpl*,
                        InspectedFrames*,
                        v8_inspector::V8InspectorSession*,
                        InspectorDOMAgent*);
  ~InspectorOverlayAgent() override;
  void Trace(blink::Visitor*) override;

  // protocol::Dispatcher::OverlayCommandHandler implementation.
  protocol::Response enable() override;
  protocol::Response disable() override;
  protocol::Response setShowPaintRects(bool) override;
  protocol::Response setShowDebugBorders(bool) override;
  protocol::Response setShowFPSCounter(bool) override;
  protocol::Response setShowScrollBottleneckRects(bool) override;
  protocol::Response setShowViewportSizeOnResize(bool) override;
  protocol::Response setPausedInDebuggerMessage(
      protocol::Maybe<String>) override;
  protocol::Response setSuspended(bool) override;
  protocol::Response setInspectMode(
      const String& mode,
      protocol::Maybe<protocol::Overlay::HighlightConfig>) override;
  protocol::Response highlightRect(
      int x,
      int y,
      int width,
      int height,
      protocol::Maybe<protocol::DOM::RGBA> color,
      protocol::Maybe<protocol::DOM::RGBA> outline_color) override;
  protocol::Response highlightQuad(
      std::unique_ptr<protocol::Array<double>> quad,
      protocol::Maybe<protocol::DOM::RGBA> color,
      protocol::Maybe<protocol::DOM::RGBA> outline_color) override;
  protocol::Response highlightNode(
      std::unique_ptr<protocol::Overlay::HighlightConfig>,
      protocol::Maybe<int> node_id,
      protocol::Maybe<int> backend_node_id,
      protocol::Maybe<String> object_id) override;
  protocol::Response hideHighlight() override;
  protocol::Response highlightFrame(
      const String& frame_id,
      protocol::Maybe<protocol::DOM::RGBA> content_color,
      protocol::Maybe<protocol::DOM::RGBA> content_outline_color) override;
  protocol::Response getHighlightObjectForTest(
      int node_id,
      std::unique_ptr<protocol::DictionaryValue>* highlight) override;

  // InspectorBaseAgent overrides.
  void Restore() override;
  void Dispose() override;

  void Inspect(Node*);
  void DispatchBufferedTouchEvents();
  bool HandleInputEvent(const WebInputEvent&);
  void PageLayoutInvalidated(bool resized);
  String EvaluateInOverlayForTest(const String&);
  void PaintOverlay();
  void LayoutOverlay();
  bool IsInspectorLayer(GraphicsLayer*);

 private:
  class InspectorOverlayChromeClient;
  class InspectorPageOverlayDelegate;

  enum SearchMode {
    kNotSearching,
    kSearchingForNormal,
    kSearchingForUAShadow,
  };

  // InspectorOverlayHost::Listener implementation.
  void OverlayResumed() override;
  void OverlaySteppedOver() override;

  bool IsEmpty();
  void DrawNodeHighlight();
  void DrawQuadHighlight();
  void DrawPausedInDebuggerMessage();
  void DrawViewSize();
  void DrawScreenshotBorder();

  float WindowToViewportScale() const;

  Page* OverlayPage();
  LocalFrame* OverlayMainFrame();
  void Reset(const IntSize& viewport_size,
             const IntPoint& document_scroll_offset);
  void EvaluateInOverlay(const String& method, const String& argument);
  void EvaluateInOverlay(const String& method,
                         std::unique_ptr<protocol::Value> argument);
  void OnTimer(TimerBase*);
  void RebuildOverlayPage();
  void Invalidate();
  void ScheduleUpdate();
  void ClearInternal();
  void UpdateAllLifecyclePhases();

  bool HandleMouseDown(const WebMouseEvent&);
  bool HandleMouseUp(const WebMouseEvent&);
  bool HandleGestureEvent(const WebGestureEvent&);
  bool HandlePointerEvent(const WebPointerEvent&);
  bool HandleMouseMove(const WebMouseEvent&);

  protocol::Response CompositingEnabled();

  bool ShouldSearchForNode();
  void NodeHighlightRequested(Node*);
  protocol::Response SetSearchingForNode(
      SearchMode,
      protocol::Maybe<protocol::Overlay::HighlightConfig>);
  protocol::Response HighlightConfigFromInspectorObject(
      protocol::Maybe<protocol::Overlay::HighlightConfig>
          highlight_inspector_object,
      std::unique_ptr<InspectorHighlightConfig>*);
  void InnerHighlightQuad(std::unique_ptr<FloatQuad>,
                          protocol::Maybe<protocol::DOM::RGBA> color,
                          protocol::Maybe<protocol::DOM::RGBA> outline_color);
  void InnerHighlightNode(Node*,
                          Node* event_target,
                          const InspectorHighlightConfig&,
                          bool omit_tooltip);
  void InnerHideHighlight();

  Member<WebLocalFrameImpl> frame_impl_;
  Member<InspectedFrames> inspected_frames_;
  bool enabled_;
  String paused_in_debugger_message_;
  Member<Node> highlight_node_;
  Member<Node> event_target_node_;
  InspectorHighlightConfig node_highlight_config_;
  std::unique_ptr<FloatQuad> highlight_quad_;
  Member<Page> overlay_page_;
  Member<InspectorOverlayChromeClient> overlay_chrome_client_;
  Member<InspectorOverlayHost> overlay_host_;
  Color quad_content_color_;
  Color quad_content_outline_color_;
  bool draw_view_size_;
  bool resize_timer_active_;
  bool omit_tooltip_;
  TaskRunnerTimer<InspectorOverlayAgent> timer_;
  bool suspended_;
  bool disposed_;
  bool in_layout_;
  bool needs_update_;
  v8_inspector::V8InspectorSession* v8_session_;
  Member<InspectorDOMAgent> dom_agent_;
  std::unique_ptr<PageOverlay> page_overlay_;
  Member<Node> hovered_node_for_inspect_mode_;
  bool swallow_next_mouse_up_;
  SearchMode inspect_mode_;
  std::unique_ptr<InspectorHighlightConfig> inspect_mode_highlight_config_;
  int backend_node_id_to_inspect_;
  bool screenshot_mode_ = false;
  IntPoint screenshot_anchor_;
  IntPoint screenshot_position_;
  DISALLOW_COPY_AND_ASSIGN(InspectorOverlayAgent);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTOR_OVERLAY_AGENT_H_
