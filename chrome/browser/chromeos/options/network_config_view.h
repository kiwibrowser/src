// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_OPTIONS_NETWORK_CONFIG_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_OPTIONS_NETWORK_CONFIG_VIEW_H_

#include <string>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/gfx/native_widget_types.h"  // gfx::NativeWindow
#include "ui/views/controls/button/button.h"  // views::ButtonListener
#include "ui/views/window/dialog_delegate.h"

namespace views {
class ImageView;
}

namespace chromeos {

class ChildNetworkConfigView;
class NetworkPropertyUIData;
class NetworkState;

// A dialog box for showing a password textfield.
class NetworkConfigView : public views::DialogDelegateView,
                          public views::ButtonListener {
 public:
  class Delegate {
   public:
    // Called when dialog "OK" button is pressed.
    virtual void OnDialogAccepted() = 0;

    // Called when dialog "Cancel" button is pressed.
    virtual void OnDialogCancelled() = 0;

   protected:
     virtual ~Delegate() {}
  };

  // Returns true if a NetworkConfigView instance exists.
  static bool HasInstance();

  // Shows a network connection dialog if none is currently visible. The dialog
  // is placed on the default display for new windows. Returns the dialog or
  // nullptr on error.
  static NetworkConfigView* ShowForNetworkId(const std::string& network_id);

  // Shows a dialog to configure a new network. |type| must be a valid Shill
  // 'Type' property value. Returns the dialog or nullptr on error.
  static NetworkConfigView* ShowForType(const std::string& type);

  // Returns corresponding native window.
  gfx::NativeWindow GetNativeWindow() const;

  // views::DialogDelegate methods.
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  bool IsDialogButtonEnabled(ui::DialogButton button) const override;
  bool Cancel() override;
  bool Accept() override;
  views::View* CreateExtraView() override;
  views::View* GetInitiallyFocusedView() override;

  // views::WidgetDelegate methods.
  base::string16 GetWindowTitle() const override;
  ui::ModalType GetModalType() const override;

  // views::View overrides.
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // views::ButtonListener overrides.
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  void set_delegate(Delegate* delegate) {
    delegate_ = delegate;
  }

 protected:
  // views::View overrides:
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;
  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override;

 private:
  NetworkConfigView();
  ~NetworkConfigView() override;

  // Login dialog for known networks. Returns true if successfully created.
  bool InitWithNetworkState(const NetworkState* network);
  // Login dialog for new/hidden networks. Returns true if successfully created.
  bool InitWithType(const std::string& type);

  // Creates and shows a dialog containing this view.
  void ShowDialog();

  // Resets the underlying view to show advanced options.
  void ShowAdvancedView();

  // There's always only one child view, which will get deleted when
  // NetworkConfigView gets cleaned up.
  ChildNetworkConfigView* child_config_view_;

  Delegate* delegate_;

  // Button in lower-left corner, may be null or hidden.
  views::View* advanced_button_;

  DISALLOW_COPY_AND_ASSIGN(NetworkConfigView);
};

// Children of NetworkConfigView must subclass this and implement the virtual
// methods, which are called by NetworkConfigView.
class ChildNetworkConfigView : public views::View {
 public:
  // If |service_path| is NULL, a dialog for configuring a new network will
  // be created.
  ChildNetworkConfigView(NetworkConfigView* parent,
                         const std::string& service_path);
  ~ChildNetworkConfigView() override;

  // Get the title to show for the dialog.
  virtual base::string16 GetTitle() const = 0;

  // Returns view that should be focused on dialog activation.
  virtual views::View* GetInitiallyFocusedView() = 0;

  // Called to determine if "Connect" button should be enabled.
  virtual bool CanLogin() = 0;

  // Called when "Connect" button is clicked.
  // Should return false if dialog should remain open.
  virtual bool Login() = 0;

  // Called when "Cancel" button is clicked.
  virtual void Cancel() = 0;

  // Called to set focus when view is recreated with the same dialog
  // being active. For example, clicking on "Advanced" button.
  virtual void InitFocus() = 0;

  // Returns 'true' if the dialog is for configuration only (default is false).
  virtual bool IsConfigureDialog();

  // Minimum width of input fields / combo boxes.
  static const int kInputFieldMinWidth;

  // The height of input fields /combo boxes.
  static const int kInputFieldHeight;

 protected:
  // Gets the default network share state for the current login state.
  static void GetShareStateForLoginState(bool* default_value, bool* modifiable);

  NetworkConfigView* parent_;
  std::string service_path_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChildNetworkConfigView);
};

// Shows an icon with tooltip indicating whether a setting is under policy
// control.
class ControlledSettingIndicatorView : public views::View {
 public:
  explicit ControlledSettingIndicatorView(const NetworkPropertyUIData& ui_data);
  ~ControlledSettingIndicatorView() override;

 protected:
  // views::View:
  gfx::Size CalculatePreferredSize() const override;

 private:
  bool managed_;
  views::ImageView* image_view_;

  DISALLOW_COPY_AND_ASSIGN(ControlledSettingIndicatorView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_OPTIONS_NETWORK_CONFIG_VIEW_H_
