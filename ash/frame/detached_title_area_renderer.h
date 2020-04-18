// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_FRAME_DETACHED_TITLE_AREA_RENDERER_H_
#define ASH_FRAME_DETACHED_TITLE_AREA_RENDERER_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "ui/views/widget/widget_delegate.h"

namespace aura {
class PropertyConverter;
class Window;
}

namespace ash {

// This class is used to support immersive fullscreen mode.
//
// Immersive fullscreen is a specialized fullscreen mode. In it the user can
// move the mouse to the top of the screen and the window header will slide down
// on top of the fullscreen window contents.
//
// There are two distinct ways for immersive mode to work in mash:
//
// 1. Mash handles it all. This is the default. In this mode a separate
// ui::Window is created for the reveal of the title area. HeaderView is used to
// render the title area of the reveal in the separate window. The client does
// nothing special here.
//
// 2. The client takes control of it all (as happens in chrome). To enable this
// the client sets kDisableImmersive_InitProperty on the window. In this mode
// the client creates a separate window for the reveal (similar to 1). The
// reveal window is a child of the window going into immersive mode. Mash knows
// to paint window decorations to the reveal window by way of the property
// kRenderParentTitleArea_Property set on the parent window. The client runs all
// the immersive logic, including positioning of the reveal window.
//
// DetachedTitleAreaRenderer comes in two variants.
// DetachedTitleAreaRendererForClient is used for clients that need to draw
// into the non-client area of the widget (case 2). For example, Chrome browser
// windows draw into the non-client area of tabbed browser widgets (the tab
// strip is in the non-client area). In such a case
// DetachedTitleAreaRendererForClient is used.
// If the client does not need to draw to the non-client area (case 1) then
// DetachedTitleAreaRendererInternal is used (and ash controls the whole
// immersive experience). Which is used is determined by
// |kRenderParentTitleArea_Property|. If |kRenderParentTitleArea_Property| is
// false DetachedTitleAreaRendererInternal is used.

// DetachedTitleAreaRendererInternal owns the widget it creates.
class DetachedTitleAreaRendererForInternal {
 public:
  // |frame| is the Widget the decorations are configured from.
  explicit DetachedTitleAreaRendererForInternal(views::Widget* frame);
  ~DetachedTitleAreaRendererForInternal();

  views::Widget* widget() { return widget_.get(); }

 private:
  std::unique_ptr<views::Widget> widget_;

  DISALLOW_COPY_AND_ASSIGN(DetachedTitleAreaRendererForInternal);
};

// Used when the client wants to control, and possibly render to, the widget
// hosting the frame decorations. In this mode the client owns the window
// backing the widget and controls the lifetime of the window.
class DetachedTitleAreaRendererForClient : public views::WidgetDelegate {
 public:
  DetachedTitleAreaRendererForClient(
      aura::Window* parent,
      aura::PropertyConverter* property_converter,
      std::map<std::string, std::vector<uint8_t>>* properties);

  static DetachedTitleAreaRendererForClient* ForWindow(aura::Window* window);

  void Attach(views::Widget* frame);
  void Detach();

  bool is_attached() const { return is_attached_; }

  views::Widget* widget() { return widget_; }

  // views::WidgetDelegate:
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;
  void DeleteDelegate() override;

 private:
  ~DetachedTitleAreaRendererForClient() override;

  views::Widget* widget_;

  // Has Attach() been called?
  bool is_attached_ = false;

  DISALLOW_COPY_AND_ASSIGN(DetachedTitleAreaRendererForClient);
};

}  // namespace ash

#endif  // ASH_FRAME_DETACHED_TITLE_AREA_RENDERER_H_
