// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WINDOW_DIALOG_DELEGATE_H_
#define UI_VIEWS_WINDOW_DIALOG_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/base/models/dialog_model.h"
#include "ui/base/ui_base_types.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace views {

class DialogClientView;
class DialogObserver;
class LabelButton;

///////////////////////////////////////////////////////////////////////////////
//
// DialogDelegate
//
//  DialogDelegate is an interface implemented by objects that wish to show a
//  dialog box Window. The window that is displayed uses this interface to
//  determine how it should be displayed and notify the delegate object of
//  certain events.
//
///////////////////////////////////////////////////////////////////////////////
class VIEWS_EXPORT DialogDelegate : public ui::DialogModel,
                                    public WidgetDelegate {
 public:
  DialogDelegate();

  // Creates a widget at a default location.
  static Widget* CreateDialogWidget(WidgetDelegate* delegate,
                                    gfx::NativeWindow context,
                                    gfx::NativeView parent);

  // Returns the dialog widget InitParams for a given |context| or |parent|.
  // If |bounds| is not empty, used to initially place the dialog, otherwise
  // a default location is used.
  static Widget::InitParams GetDialogWidgetInitParams(WidgetDelegate* delegate,
                                                      gfx::NativeWindow context,
                                                      gfx::NativeView parent,
                                                      const gfx::Rect& bounds);

  // Override this function to display an extra view adjacent to the buttons.
  // Overrides may construct the view; this will only be called once per dialog.
  virtual View* CreateExtraView();

  // Override this function to adjust the padding between the extra view and
  // the confirm/cancel buttons. Note that if there are no buttons, this will
  // not be used.
  // If a custom padding should be used, returns true and populates |padding|.
  virtual bool GetExtraViewPadding(int* padding);

  // Override this function to display a footnote view below the buttons.
  // Overrides may construct the view; this will only be called once per dialog.
  virtual View* CreateFootnoteView();

  // For Dialog boxes, if there is a "Cancel" button or no dialog button at all,
  // this is called when the user presses the "Cancel" button.
  // It can also be called on a close action if |Close| has not been
  // overridden. This function should return true if the window can be closed
  // after it returns, or false if it must remain open.
  virtual bool Cancel();

  // For Dialog boxes, this is called when the user presses the "OK" button,
  // or the Enter key. It can also be called on a close action if |Close|
  // has not been overridden. This function should return true if the window
  // can be closed after it returns, or false if it must remain open.
  virtual bool Accept();

  // Called when the user closes the window without selecting an option,
  // e.g. by pressing the close button on the window, pressing the Esc key, or
  // using a window manager gesture. By default, this calls Accept() if the only
  // button in the dialog is Accept, Cancel() otherwise. This function should
  // return true if the window can be closed after it returns, or false if it
  // must remain open.
  virtual bool Close();

  // Updates the properties and appearance of |button| which has been created
  // for type |type|. Override to do special initialization above and beyond
  // the typical.
  virtual void UpdateButton(LabelButton* button, ui::DialogButton type);

  // Returns true if this dialog should snap the frame width based on the
  // LayoutProvider's snapping.
  virtual bool ShouldSnapFrameWidth() const;

  // Overridden from ui::DialogModel:
  int GetDialogButtons() const override;
  int GetDefaultDialogButton() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  bool IsDialogButtonEnabled(ui::DialogButton button) const override;

  // Overridden from WidgetDelegate:
  View* GetInitiallyFocusedView() override;
  DialogDelegate* AsDialogDelegate() override;
  ClientView* CreateClientView(Widget* widget) override;
  NonClientFrameView* CreateNonClientFrameView(Widget* widget) override;

  static NonClientFrameView* CreateDialogFrameView(Widget* widget);

  // Returns true if this particular dialog should use a Chrome-styled frame
  // like the one used for bubbles. The alternative is a more platform-native
  // frame.
  virtual bool ShouldUseCustomFrame() const;

  const gfx::Insets& margins() const { return margins_; }
  void set_margins(const gfx::Insets& margins) { margins_ = margins; }

  // A helper for accessing the DialogClientView object contained by this
  // delegate's Window.
  const DialogClientView* GetDialogClientView() const;
  DialogClientView* GetDialogClientView();

  // Add or remove an observer notified by calls to DialogModelChanged().
  void AddObserver(DialogObserver* observer);
  void RemoveObserver(DialogObserver* observer);

  // Notifies observers when the result of the DialogModel overrides changes.
  void DialogModelChanged();

 protected:
  ~DialogDelegate() override;

  // Overridden from WidgetDelegate:
  ax::mojom::Role GetAccessibleWindowRole() const override;

 private:
  // A flag indicating whether this dialog is able to use the custom frame
  // style for dialogs.
  bool supports_custom_frame_;

  // The margins between the content and the inside of the border.
  gfx::Insets margins_;

  // The time the dialog is created.
  base::TimeTicks creation_time_;

  // Observers for DialogModel changes.
  base::ObserverList<DialogObserver> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(DialogDelegate);
};

// A DialogDelegate implementation that is-a View. Used to override GetWidget()
// to call View's GetWidget() for the common case where a DialogDelegate
// implementation is-a View. Note that DialogDelegateView is not owned by
// view's hierarchy and is expected to be deleted on DeleteDelegate call.
class VIEWS_EXPORT DialogDelegateView : public DialogDelegate,
                                        public View {
 public:
  DialogDelegateView();
  ~DialogDelegateView() override;

  // Overridden from DialogDelegate:
  void DeleteDelegate() override;
  Widget* GetWidget() override;
  const Widget* GetWidget() const override;
  View* GetContentsView() override;

  // Overridden from View:
  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DialogDelegateView);
};

}  // namespace views

#endif  // UI_VIEWS_WINDOW_DIALOG_DELEGATE_H_
