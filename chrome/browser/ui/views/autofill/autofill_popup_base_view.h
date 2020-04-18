// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_BASE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_BASE_VIEW_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/ui/autofill/autofill_popup_view_delegate.h"
#include "ui/views/focus/widget_focus_manager.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

#if defined(OS_MACOSX)
#include "ui/base/cocoa/bubble_closer.h"
#endif

namespace gfx {
class Point;
}

namespace autofill {

// Class that deals with the event handling for Autofill-style popups. This
// class should only be instantiated by sub-classes.
class AutofillPopupBaseView : public views::WidgetDelegateView,
                              public views::WidgetFocusChangeListener,
                              public views::WidgetObserver {
 protected:
  explicit AutofillPopupBaseView(AutofillPopupViewDelegate* delegate,
                                 views::Widget* parent_widget);
  ~AutofillPopupBaseView() override;

  // Show this popup. Idempotent.
  void DoShow();

  // Hide the widget and delete |this|.
  void DoHide();

  // Grows |bounds| to account for the border of the popup.
  void AdjustBoundsForBorder(gfx::Rect* bounds) const;

  virtual void AddExtraInitParams(views::Widget::InitParams* params) {}

  // Returns the widget's contents view.
  virtual std::unique_ptr<views::View> CreateWrapperView();

  // Returns the border to be applied to the popup.
  virtual std::unique_ptr<views::Border> CreateBorder();

  // Update size of popup and paint (virtual for testing).
  virtual void DoUpdateBoundsAndRedrawPopup();

  const AutofillPopupViewDelegate* delegate() { return delegate_; }

 private:
  friend class AutofillPopupBaseViewTest;

  // views::Views implementation.
  void OnMouseCaptureLost() override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

  // views::WidgetFocusChangeListener implementation.
  void OnNativeFocusChanged(gfx::NativeView focused_now) override;

  // views::WidgetObserver implementation.
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

  // Stop observing the widget.
  void RemoveObserver();

  void SetSelection(const gfx::Point& point);
  void AcceptSelection(const gfx::Point& point);
  void ClearSelection();

  // Hide the controller of this view. This assumes that doing so will
  // eventually hide this view in the process.
  void HideController();

  // Compute the space available for the popup. It's the space between its top
  // and the bottom of its parent view, minus some margin space.
  gfx::Rect CalculateClippingBounds() const;

  // Must return the container view for this popup.
  gfx::NativeView container_view();

  // Controller for this popup. Weak reference.
  AutofillPopupViewDelegate* delegate_;

  // The widget of the window that triggered this popup. Weak reference.
  views::Widget* parent_widget_;

  views::ScrollView* scroll_view_;

  // The time when the popup was shown.
  base::Time show_time_;

#if defined(OS_MACOSX)
  // Special handler to close the popup on the Mac Cocoa browser.
  // |parent_widget_| is null on that browser so we can't observe it for
  // window changes.
  std::unique_ptr<ui::BubbleCloser> mac_bubble_closer_;
#endif

  base::WeakPtrFactory<AutofillPopupBaseView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupBaseView);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_BASE_VIEW_H_
