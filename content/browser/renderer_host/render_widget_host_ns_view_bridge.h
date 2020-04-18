// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_NS_VIEW_BRIDGE_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_NS_VIEW_BRIDGE_H_

@class RenderWidgetHostViewCocoa;

#include <memory>

#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "content/common/mac/attributed_string_coder.h"
#include "third_party/blink/public/web/web_popup_type.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/ime/text_input_type.h"

namespace gfx {
struct CALayerParams;
class Point;
class Range;
class Rect;
}  // namespace gfx

namespace ui {
enum class DomCode;
}  // namespace ui

namespace content {

class RenderWidgetHostNSViewClient;
class WebCursor;

// The interface through which RenderWidgetHostViewMac is to manipulate its
// corresponding NSView (potentially in another process).
class RenderWidgetHostNSViewBridge {
 public:
  RenderWidgetHostNSViewBridge() {}
  virtual ~RenderWidgetHostNSViewBridge() {}

  static std::unique_ptr<RenderWidgetHostNSViewBridge> Create(
      RenderWidgetHostNSViewClient* client);

  // TODO(ccameron): RenderWidgetHostViewMac and other functions currently use
  // this method to communicate directly with RenderWidgetHostViewCocoa. The
  // goal of this class is to eliminate this direct communication (so this
  // method is expected to go away).
  virtual RenderWidgetHostViewCocoa* GetRenderWidgetHostViewCocoa() = 0;

  // Initialize the window as a popup (e.g, date/time picker).
  virtual void InitAsPopup(const gfx::Rect& content_rect,
                           blink::WebPopupType popup_type) = 0;

  // Disable displaying any content (including the background color). This is
  // to be called on views that are to be displayed via a parent ui::Compositor.
  virtual void DisableDisplay() = 0;

  // Make the NSView be the first responder of its NSWindow.
  virtual void MakeFirstResponder() = 0;

  // Set the bounds of the NSView or its enclosing NSWindow (depending on the
  // window type).
  virtual void SetBounds(const gfx::Rect& rect) = 0;

  // Set the contents to display in the NSView.
  virtual void SetCALayerParams(const gfx::CALayerParams& ca_layer_params) = 0;

  // Set the background color of the hosted CALayer.
  virtual void SetBackgroundColor(SkColor color) = 0;

  // Call the -[NSView setHidden:] method.
  virtual void SetVisible(bool visible) = 0;

  // Call the -[NSView setToolTipAtMousePoint] method.
  virtual void SetTooltipText(const base::string16& display_text) = 0;

  // Forward changes in ui::TextInputType.
  virtual void SetTextInputType(ui::TextInputType text_input_type) = 0;

  // Forward the TextInputManager::TextSelection from the renderer.
  virtual void SetTextSelection(const base::string16& text,
                                size_t offset,
                                const gfx::Range& range) = 0;

  // Forward the TextInputManager::CompositionRangeInfo from the renderer.
  virtual void SetCompositionRangeInfo(const gfx::Range& range) = 0;

  // Clear the marked range.
  virtual void CancelComposition() = 0;

  // Indicate if the WebContext is showing a context menu.
  virtual void SetShowingContextMenu(bool showing) = 0;

  // Set the cursor type to display.
  virtual void DisplayCursor(const WebCursor& cursor) = 0;

  // Lock or unlock the cursor.
  virtual void SetCursorLocked(bool locked) = 0;

  // Open the dictionary overlay for the currently selected string. This
  // will roundtrip to the NSView to determine the selected range.
  virtual void ShowDictionaryOverlayForSelection() = 0;

  // Open the dictionary overlay for the specified string at the specified
  // point.
  virtual void ShowDictionaryOverlay(
      const mac::AttributedStringCoder::EncodedString& encoded_string,
      gfx::Point baseline_point) = 0;

  // Start intercepting keyboard events.
  virtual void LockKeyboard(
      base::Optional<base::flat_set<ui::DomCode>> dom_codes) = 0;

  // Stop intercepting keyboard events.
  virtual void UnlockKeyboard() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostNSViewBridge);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_NS_VIEW_BRIDGE_H_
