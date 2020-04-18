// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_ICON_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_ICON_VIEW_H_

#include <memory>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/controls/focus_ring.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/widget/widget_observer.h"

class CommandUpdater;

namespace content {
class WebContents;
}

namespace gfx {
struct VectorIcon;
}

namespace views {
class BubbleDialogDelegateView;
}

// Represents an inbuilt (as opposed to an extension) page action icon that
// shows a bubble when clicked.
// TODO(spqchan): Convert this to subclass Button.
class PageActionIconView : public views::InkDropHostView {
 public:
  class Delegate {
   public:
    virtual content::WebContents* GetWebContentsForPageActionIconView() = 0;
    virtual OmniboxTint GetTint() = 0;
  };

  void Init();

  // Invoked when a bubble for this icon is created. The PageActionIconView
  // changes highlights based on this widget's visibility.
  void OnBubbleWidgetCreated(views::Widget* bubble_widget);

  // Returns the bubble instance for the icon.
  virtual views::BubbleDialogDelegateView* GetBubble() const = 0;

  // Updates the icon state and associated bubble when the WebContents changes.
  // Returns true if there was a change.
  virtual bool Refresh();

 protected:
  enum ExecuteSource {
    EXECUTE_SOURCE_MOUSE,
    EXECUTE_SOURCE_KEYBOARD,
    EXECUTE_SOURCE_GESTURE,
  };

  PageActionIconView(CommandUpdater* command_updater,
                     int command_id,
                     Delegate* delegate);
  ~PageActionIconView() override;

  // Returns true if a related bubble is showing.
  bool IsBubbleShowing() const;

  // Enables or disables the associated command.
  // Returns true if the command is enabled.
  bool SetCommandEnabled(bool enabled) const;

  // Sets the image that should be displayed in |image_|.
  void SetImage(const gfx::ImageSkia* image_skia);

  // Returns the image currently displayed, which can be empty if not set.
  // The returned image is owned by |image_|.
  const gfx::ImageSkia& GetImage() const;

  // Sets the tooltip text.
  void SetTooltipText(const base::string16& tooltip);

  // Invoked prior to executing the command.
  virtual void OnExecuting(ExecuteSource execute_source) = 0;

  // Invoked after the icon is pressed.
  virtual void OnPressed(bool activated) {}

  // views::InkDropHostView:
  void OnFocus() override;
  void OnBlur() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool GetTooltipText(const gfx::Point& p,
                      base::string16* tooltip) const override;
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  bool OnKeyReleased(const ui::KeyEvent& event) override;
  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override;
  void OnNativeThemeChanged(const ui::NativeTheme* theme) override;
  void OnThemeChanged() override;
  void AddInkDropLayer(ui::Layer* ink_drop_layer) override;
  void RemoveInkDropLayer(ui::Layer* ink_drop_layer) override;
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;
  std::unique_ptr<views::InkDropHighlight> CreateInkDropHighlight()
      const override;
  SkColor GetInkDropBaseColor() const override;
  std::unique_ptr<views::InkDropMask> CreateInkDropMask() const override;

  // ui::EventHandler:
  void OnGestureEvent(ui::GestureEvent* event) override;

 protected:
  // Calls OnExecuting and runs |command_id_| with a valid |command_updater_|.
  virtual void ExecuteCommand(ExecuteSource source);

  // Gets the given vector icon in the correct color and size based on |active|.
  virtual const gfx::VectorIcon& GetVectorIcon() const = 0;

  // views::View:
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;

  // Updates the icon after some state has changed.
  void UpdateIcon();

  // Sets the active state of the icon. An active icon will be displayed in a
  // "call to action" color.
  void SetActiveInternal(bool active);

  // Returns the associated web contents from the delegate.
  content::WebContents* GetWebContents() const;

  bool active() const { return active_; }

  // Retrieve the text to be used for a tooltip or accessible name.
  virtual base::string16 GetTextForTooltipAndAccessibleName() const = 0;

 private:
  class WidgetObserver : public views::WidgetObserver {
   public:
    explicit WidgetObserver(PageActionIconView* parent);
    ~WidgetObserver() override;

    void SetWidget(views::Widget* widget);

   private:
    // views::WidgetObserver:
    void OnWidgetDestroying(views::Widget* widget) override;
    void OnWidgetVisibilityChanged(views::Widget* widget,
                                   bool visible) override;

    PageActionIconView* const parent_;
    ScopedObserver<views::Widget, views::WidgetObserver> scoped_observer_;
    DISALLOW_COPY_AND_ASSIGN(WidgetObserver);
  };

  // Highlights the ink drop for the icon, used when the corresponding widget
  // is visible.
  void SetHighlighted(bool bubble_visible);

  WidgetObserver widget_observer_;

  // The image shown in the button.
  views::ImageView* image_;

  // The CommandUpdater for the Browser object that owns the location bar.
  CommandUpdater* command_updater_;

  // Delegate for access to associated state.
  Delegate* delegate_;

  // The command ID executed when the user clicks this icon.
  const int command_id_;

  // The active node_data. The precise definition of "active" is unique to each
  // subclass, but generally indicates that the associated feature is acting on
  // the web page.
  bool active_;

  // This is used to check if the bookmark bubble was showing during the mouse
  // pressed event. If this is true then the mouse released event is ignored to
  // prevent the bubble from reshowing.
  bool suppress_mouse_released_action_;

  // The focus ring used for this view.
  std::unique_ptr<views::FocusRing> focus_ring_;

  DISALLOW_COPY_AND_ASSIGN(PageActionIconView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_ICON_VIEW_H_
