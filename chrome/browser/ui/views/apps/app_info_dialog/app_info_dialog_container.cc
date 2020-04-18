// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/apps/app_info_dialog/app_info_dialog_container.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/common/buildflags.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_types.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/client_view.h"
#include "ui/views/window/dialog_delegate.h"
#include "ui/views/window/native_frame_view.h"
#include "ui/views/window/non_client_view.h"

#if BUILDFLAG(ENABLE_APP_LIST)
#include "ash/public/cpp/app_list/app_list_constants.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "ui/views/background.h"
#endif

namespace {

#if defined(OS_MACOSX)
const ui::ModalType kModalType = ui::MODAL_TYPE_CHILD;
const views::BubbleBorder::Shadow kShadowType = views::BubbleBorder::NO_ASSETS;
#else
const ui::ModalType kModalType = ui::MODAL_TYPE_WINDOW;
const views::BubbleBorder::Shadow kShadowType =
    views::BubbleBorder::SMALL_SHADOW;
#endif

#if BUILDFLAG(ENABLE_APP_LIST)
// The background for App List dialogs, which appears as a rounded rectangle
// with the same border radius and color as the app list contents.
class AppListOverlayBackground : public views::Background {
 public:
  AppListOverlayBackground() {}
  ~AppListOverlayBackground() override {}

  // Overridden from views::Background:
  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    // The radius of the app list overlay (the dialog's background).
    // TODO(sashab): Using SupportsShadow() from app_list_view.cc, make this
    // 1px smaller on platforms that support shadows.
    const int kAppListOverlayBorderRadius = 3;

    cc::PaintFlags flags;
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setColor(app_list::kContentsBackgroundColor);
    canvas->DrawRoundRect(view->GetContentsBounds(),
                          kAppListOverlayBorderRadius, flags);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AppListOverlayBackground);
};
#endif  // ENABLE_APP_LIST

// Base container for modal dialogs. Encases a content view in a modal dialog
// with an accelerator to close on escape.
class BaseDialogContainer : public views::DialogDelegateView {
 public:
  BaseDialogContainer(views::View* dialog_body,
                      const base::Closure& close_callback)
      : dialog_body_(dialog_body), close_callback_(close_callback) {
    AddChildView(dialog_body_);
    // Since we are using a ClientView instead of a DialogClientView, we need to
    // manually bind the escape key to close the dialog.
    ui::Accelerator escape(ui::VKEY_ESCAPE, ui::EF_NONE);
    AddAccelerator(escape);
  }
  ~BaseDialogContainer() override {}

 protected:
  views::View* dialog_body() { return dialog_body_; }

 private:
  // Overridden from views::View:
  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override {
    views::DialogDelegateView::ViewHierarchyChanged(details);
    if (details.is_add && details.child == this)
      GetFocusManager()->AdvanceFocus(false);
  }

  bool AcceleratorPressed(const ui::Accelerator& accelerator) override {
    DCHECK_EQ(accelerator.key_code(), ui::VKEY_ESCAPE);
    GetWidget()->Close();
    return true;
  }

  // Overridden from views::DialogDelegate:
  int GetDialogButtons() const override { return ui::DIALOG_BUTTON_NONE; }

  // Overridden from views::WidgetDelegate:
  ui::ModalType GetModalType() const override { return kModalType; }
  void WindowClosing() override {
    if (!close_callback_.is_null())
      close_callback_.Run();
  }
  views::ClientView* CreateClientView(views::Widget* widget) override {
    return new views::ClientView(widget, GetContentsView());
  }

  views::View* dialog_body_;
  const base::Closure close_callback_;

  DISALLOW_COPY_AND_ASSIGN(BaseDialogContainer);
};

#if BUILDFLAG(ENABLE_APP_LIST)

// The contents view for an App List Dialog, which covers the entire app list
// and adds a close button.
class AppListDialogContainer : public BaseDialogContainer,
                               public views::ButtonListener {
 public:
  AppListDialogContainer(views::View* dialog_body,
                         const base::Closure& close_callback)
      : BaseDialogContainer(dialog_body, close_callback) {
    SetBackground(std::make_unique<AppListOverlayBackground>());
    close_button_ = views::BubbleFrameView::CreateCloseButton(this);
    AddChildView(close_button_);
  }
  ~AppListDialogContainer() override {}

 private:
  // views::View:
  void Layout() override {
    // Margin of the close button from the top right-hand corner of the dialog.
    const int kCloseButtonDialogMargin = 10;

    close_button_->SetPosition(
        gfx::Point(width() - close_button_->width() - kCloseButtonDialogMargin,
                   kCloseButtonDialogMargin));

    dialog_body()->SetBoundsRect(GetContentsBounds());
    views::DialogDelegateView::Layout();
  }

  // views::WidgetDelegate:
  views::NonClientFrameView* CreateNonClientFrameView(
      views::Widget* widget) override {
    return new views::NativeFrameView(widget);
  }

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override {
    if (sender == close_button_) {
      GetWidget()->Close();
    } else {
      NOTREACHED();
    }
  }

  views::Button* close_button_;

  DISALLOW_COPY_AND_ASSIGN(AppListDialogContainer);
};

#endif  // ENABLE_APP_LIST

// A BubbleFrameView that allows its client view to extend all the way to the
// top of the dialog, overlapping the BubbleFrameView's close button. This
// allows dialog content to appear closer to the top, in place of a title.
// TODO(estade): the functionality here should probably be folded into
// BubbleFrameView.
class FullSizeBubbleFrameView : public views::BubbleFrameView {
 public:
  FullSizeBubbleFrameView()
      : views::BubbleFrameView(gfx::Insets(), gfx::Insets()) {}
  ~FullSizeBubbleFrameView() override {}

 private:
  // Overridden from views::ViewTargeterDelegate:
  bool DoesIntersectRect(const View* target,
                         const gfx::Rect& rect) const override {
    // Make sure click events can still reach the close button, even if the
    // ClientView overlaps it.
    // NOTE: |rect| is in the mirrored coordinate space, so we must use the
    // close button's mirrored bounds to correctly target the close button when
    // in RTL mode.
    if (IsCloseButtonVisible() &&
        GetCloseButtonMirroredBounds().Intersects(rect)) {
      return true;
    }
    return views::BubbleFrameView::DoesIntersectRect(target, rect);
  }

  // Overridden from views::BubbleFrameView:
  bool ExtendClientIntoTitle() const override { return true; }

  // Overridden from views::View:
  gfx::Insets GetInsets() const override { return gfx::Insets(); }

  DISALLOW_COPY_AND_ASSIGN(FullSizeBubbleFrameView);
};

// A container view for a native dialog, which sizes to the given fixed |size|.
class NativeDialogContainer : public BaseDialogContainer {
 public:
  NativeDialogContainer(views::View* dialog_body,
                        const gfx::Size& size,
                        const base::Closure& close_callback)
      : BaseDialogContainer(dialog_body, close_callback) {
    SetLayoutManager(std::make_unique<views::FillLayout>());
    chrome::RecordDialogCreation(chrome::DialogIdentifier::NATIVE_CONTAINER);
    SetPreferredSize(size);
  }
  ~NativeDialogContainer() override {}

 private:
  // Overridden from views::WidgetDelegate:
  views::NonClientFrameView* CreateNonClientFrameView(
      views::Widget* widget) override {
    FullSizeBubbleFrameView* frame = new FullSizeBubbleFrameView();
    std::unique_ptr<views::BubbleBorder> border(new views::BubbleBorder(
        views::BubbleBorder::FLOAT, kShadowType, gfx::kPlaceholderColor));
    border->set_use_theme_background_color(true);
    frame->SetBubbleBorder(std::move(border));
    return frame;
  }

  DISALLOW_COPY_AND_ASSIGN(NativeDialogContainer);
};

}  // namespace

#if BUILDFLAG(ENABLE_APP_LIST)
views::DialogDelegateView* CreateAppListContainerForView(
    views::View* view,
    const base::Closure& close_callback) {
  return new AppListDialogContainer(view, close_callback);
}
#endif  // ENABLE_APP_LIST

views::DialogDelegateView* CreateDialogContainerForView(
    views::View* view,
    const gfx::Size& size,
    const base::Closure& close_callback) {
  return new NativeDialogContainer(view, size, close_callback);
}
