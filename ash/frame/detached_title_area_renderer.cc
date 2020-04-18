// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/frame/detached_title_area_renderer.h"

#include <memory>

#include "ash/frame/caption_buttons/caption_button_model.h"
#include "ash/frame/header_view.h"
#include "ash/wm/property_util.h"
#include "ash/wm/window_state.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/transient_window_client.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/property_utils.h"
#include "ui/aura/window.h"
#include "ui/base/class_property.h"
#include "ui/views/view.h"
#include "ui/views/widget/native_widget_aura.h"
#include "ui/views/widget/widget.h"

DEFINE_UI_CLASS_PROPERTY_TYPE(ash::DetachedTitleAreaRendererForClient*);

namespace ash {
namespace {

DEFINE_UI_CLASS_PROPERTY_KEY(DetachedTitleAreaRendererForClient*,
                             kDetachedTitleAreaRendererKey,
                             nullptr);

// Used to indicate why this is being created. See header for description of
// types.
enum class Source {
  CLIENT,
  INTERNAL,
};

// Configures the common InitParams.
std::unique_ptr<views::Widget::InitParams> CreateInitParams(
    const char* debug_name) {
  std::unique_ptr<views::Widget::InitParams> params =
      std::make_unique<views::Widget::InitParams>(
          views::Widget::InitParams::TYPE_POPUP);
  params->name = debug_name;
  params->activatable = views::Widget::InitParams::ACTIVATABLE_NO;
  return params;
}

// Configures properties common to both types.
void ConfigureCommonWidgetProperties(views::Widget* widget) {
  aura::Window* window = widget->GetNativeView();
  // Default animations conflict with the reveal animation, so turn off the
  // default animation.
  window->SetProperty(aura::client::kAnimationsDisabledKey, true);
  wm::GetWindowState(window)->set_ignored_by_shelf(true);
}

void CreateHeaderView(views::Widget* frame,
                      views::Widget* detached_widget,
                      Source source) {
  HeaderView* header_view = new HeaderView(frame);
  if (source == Source::CLIENT) {
    // HeaderView behaves differently when the widget it is associated with is
    // fullscreen (HeaderView is normally the
    // ImmersiveFullscreenControllerDelegate). Set this as when creating for
    // the client HeaderView is not the ImmersiveFullscreenControllerDelegate.
    header_view->set_is_immersive_delegate(false);
  }
  header_view->set_is_immersive_delegate(false);
  detached_widget->SetContentsView(header_view);
}

}  // namespace

DetachedTitleAreaRendererForInternal::DetachedTitleAreaRendererForInternal(
    views::Widget* frame)
    : widget_(std::make_unique<views::Widget>()) {
  std::unique_ptr<views::Widget::InitParams> params =
      CreateInitParams("DetachedTitleAreaRendererForInternal");
  params->ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params->parent = frame->GetNativeView()->parent();
  widget_->Init(*params);
  aura::client::GetTransientWindowClient()->AddTransientChild(
      frame->GetNativeView(), widget_->GetNativeView());
  CreateHeaderView(frame, widget_.get(), Source::INTERNAL);
  ConfigureCommonWidgetProperties(widget_.get());
}

DetachedTitleAreaRendererForInternal::~DetachedTitleAreaRendererForInternal() =
    default;

DetachedTitleAreaRendererForClient::DetachedTitleAreaRendererForClient(
    aura::Window* parent,
    aura::PropertyConverter* property_converter,
    std::map<std::string, std::vector<uint8_t>>* properties)
    : widget_(new views::Widget) {
  std::unique_ptr<views::Widget::InitParams> params =
      CreateInitParams("DetachedTitleAreaRendererForClient");
  views::NativeWidgetAura* native_widget =
      new views::NativeWidgetAura(widget_, true);
  native_widget->GetNativeView()->SetProperty(
      aura::client::kEmbedType, aura::client::WindowEmbedType::TOP_LEVEL_IN_WM);
  aura::SetWindowType(native_widget->GetNativeWindow(),
                      ui::mojom::WindowType::POPUP);
  ApplyProperties(native_widget->GetNativeWindow(), property_converter,
                  *properties);
  native_widget->GetNativeView()->SetProperty(kDetachedTitleAreaRendererKey,
                                              this);
  params->delegate = this;
  params->native_widget = native_widget;
  params->parent = parent;
  widget_->Init(*params);
  ConfigureCommonWidgetProperties(widget_);
}

// static
DetachedTitleAreaRendererForClient*
DetachedTitleAreaRendererForClient::ForWindow(aura::Window* window) {
  return window->GetProperty(kDetachedTitleAreaRendererKey);
}

void DetachedTitleAreaRendererForClient::Attach(views::Widget* frame) {
  DCHECK(!is_attached_);
  is_attached_ = true;
  CreateHeaderView(frame, widget_, Source::CLIENT);
  frame->GetNativeView()->parent()->AddChild(widget_->GetNativeView());
}

void DetachedTitleAreaRendererForClient::Detach() {
  is_attached_ = false;
  widget_->SetContentsView(new views::View());
}

views::Widget* DetachedTitleAreaRendererForClient::GetWidget() {
  return widget_;
}

const views::Widget* DetachedTitleAreaRendererForClient::GetWidget() const {
  return widget_;
}

void DetachedTitleAreaRendererForClient::DeleteDelegate() {
  delete this;
}

DetachedTitleAreaRendererForClient::~DetachedTitleAreaRendererForClient() =
    default;

}  // namespace ash
