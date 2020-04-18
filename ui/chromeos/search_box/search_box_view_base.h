// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHOMEOS_SEARCH_BOX_SEARCH_BOX_VIEW_BASE_H_
#define UI_CHOMEOS_SEARCH_BOX_SEARCH_BOX_VIEW_BASE_H_

#include <vector>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/chromeos/search_box/search_box_constants.h"
#include "ui/chromeos/search_box/search_box_export.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/widget/widget_delegate.h"

namespace gfx {
class ImageSkia;
}  // namespace gfx

namespace views {
class BoxLayout;
class ImageView;
class Textfield;
class View;
}  // namespace views

namespace search_box {

class SearchBoxViewDelegate;
class SearchBoxBackground;
class SearchBoxImageButton;

// TODO(wutao): WidgetDelegateView owns itself and cannot be deleted from the
// views hierarchy automatically. Make SearchBoxViewBase a subclass of View
// instead of WidgetDelegateView.
// SearchBoxViewBase consists of icons and a Textfield. The Textfiled is for
// inputting queries and triggering callbacks. The icons include a search icon,
// a close icon and a back icon for different functionalities. This class
// provides common functions for the search box view across Chrome OS.
class SEARCH_BOX_EXPORT SearchBoxViewBase : public views::WidgetDelegateView,
                                            public views::TextfieldController,
                                            public views::ButtonListener {
 public:
  explicit SearchBoxViewBase(SearchBoxViewDelegate* delegate);
  ~SearchBoxViewBase() override;

  void Init();

  bool HasSearch() const;

  // Returns the bounds to use for the view (including the shadow) given the
  // desired bounds of the search box contents.
  gfx::Rect GetViewBoundsForSearchBoxContentsBounds(
      const gfx::Rect& rect) const;

  views::ImageButton* back_button();
  views::ImageButton* close_button();
  views::Textfield* search_box() { return search_box_; }

  void set_contents_view(views::View* contents_view) {
    contents_view_ = contents_view;
  }

  // Swaps the google icon with the back button.
  void ShowBackOrGoogleIcon(bool show_back_button);

  // Setting the search box active left aligns the placeholder text, changes
  // the color of the placeholder text, and enables cursor blink. Setting the
  // search box inactive center aligns the placeholder text, sets the color, and
  // disables cursor blink.
  void SetSearchBoxActive(bool active);

  // Handles Gesture and Mouse Events sent from |search_box_|.
  bool OnTextfieldEvent();

  // Overridden from views::View:
  gfx::Size CalculatePreferredSize() const override;
  bool OnMouseWheel(const ui::MouseWheelEvent& event) override;
  void OnEnabledChanged() override;
  const char* GetClassName() const override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;

  // Overridden from views::WidgetDelegate:
  ax::mojom::Role GetAccessibleWindowRole() const override;
  bool ShouldAdvanceFocusToTopLevelWidget() const override;

  // Overridden from views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // Called when tablet mode starts and ends.
  void OnTabletModeChanged(bool started);

  // Used only in the tests to get the current search icon.
  views::ImageView* get_search_icon_for_test() { return search_icon_; }

  // Whether the search box is active.
  bool is_search_box_active() const { return is_search_box_active_; }

  void OnOnSearchBoxFocusedChanged();

  // Whether the trimmed query in the search box is empty.
  bool IsSearchBoxTrimmedQueryEmpty() const;

  virtual void ClearSearch();

  // Returns selected view in contents view.
  virtual views::View* GetSelectedViewInContentsView();

 protected:
  // Fires query change notification.
  void NotifyQueryChanged();

  // Nofifies the active status change.
  void NotifyActiveChanged();

  // Sets the background color.
  void SetBackgroundColor(SkColor light_vibrant);
  SkColor background_color() const { return background_color_; }

  // Sets the search box color.
  void SetSearchBoxColor(SkColor color);
  SkColor search_box_color() const { return search_box_color_; }

  // Updates the visibility of close button.
  void UpdateCloseButtonVisisbility();

  // Overridden from views::TextfieldController:
  void ContentsChanged(views::Textfield* sender,
                       const base::string16& new_contents) override;
  bool HandleMouseEvent(views::Textfield* sender,
                        const ui::MouseEvent& mouse_event) override;
  bool HandleGestureEvent(views::Textfield* sender,
                          const ui::GestureEvent& gesture_event) override;

  views::BoxLayout* box_layout() { return box_layout_; }

  views::View* contents_view() { return contents_view_; }

  void set_is_tablet_mode(bool is_tablet_mode) {
    is_tablet_mode_ = is_tablet_mode;
  }
  bool is_tablet_mode() const { return is_tablet_mode_; }

  void SetSearchBoxBackgroundCornerRadius(int corner_radius);
  void SetSearchBoxBackgroundColor(SkColor color);

  void SetSearchIconImage(gfx::ImageSkia image);

  // Detects |ET_MOUSE_PRESSED| and |ET_GESTURE_TAP| events on the white
  // background of the search box.
  virtual void HandleSearchBoxEvent(ui::LocatedEvent* located_event);

  // Updates the search box's background color.
  virtual void UpdateBackgroundColor(SkColor color);

 private:
  virtual void ModelChanged() = 0;

  // Shows/hides the virtual keyboard if the search box is active.
  virtual void UpdateKeyboardVisibility() = 0;

  // Updates model text and selection model with current Textfield info.
  virtual void UpdateModel(bool initiated_by_user) = 0;

  // Updates the search icon.
  virtual void UpdateSearchIcon() = 0;

  // Update search box border based on whether the search box is activated.
  virtual void UpdateSearchBoxBorder() = 0;

  // Setup button's image, accessible name, and tooltip text etc.
  virtual void SetupCloseButton() = 0;
  virtual void SetupBackButton() = 0;

  // Gets the search box background.
  SearchBoxBackground* GetSearchBoxBackground() const;

  SearchBoxViewDelegate* delegate_;  // Not owned.

  // Owned by views hierarchy.
  views::View* content_container_;
  views::ImageView* search_icon_ = nullptr;
  SearchBoxImageButton* back_button_ = nullptr;
  SearchBoxImageButton* close_button_ = nullptr;
  views::Textfield* search_box_;
  views::View* search_box_right_space_ = nullptr;
  views::View* contents_view_ = nullptr;

  // Owned by |content_container_|. It is deleted when the view is deleted.
  views::BoxLayout* box_layout_ = nullptr;

  // Whether the search box is active.
  bool is_search_box_active_ = false;
  // Whether tablet mode is active.
  bool is_tablet_mode_ = false;
  // The current background color.
  SkColor background_color_ = kSearchBoxBackgroundDefault;
  // The current search box color.
  SkColor search_box_color_ = kDefaultSearchboxColor;

  DISALLOW_COPY_AND_ASSIGN(SearchBoxViewBase);
};

}  // namespace search_box

#endif  // UI_CHOMEOS_SEARCH_BOX_SEARCH_BOX_VIEW_BASE_H_
