// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_OPTIONS_WIMAX_CONFIG_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_OPTIONS_WIMAX_CONFIG_VIEW_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "chrome/browser/chromeos/options/network_config_view.h"
#include "chrome/browser/chromeos/options/wifi_config_view.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"

namespace views {
class Checkbox;
class Label;
class ToggleImageButton;
}

namespace chromeos {

// A dialog box for showing a password textfield.
class WimaxConfigView : public ChildNetworkConfigView,
                        public views::TextfieldController,
                        public views::ButtonListener {
 public:
  // Configuration dialog for a WiMax network. If |service_path| is not empty
  // it identifies the network to be configured.
  WimaxConfigView(NetworkConfigView* parent, const std::string& service_path);
  ~WimaxConfigView() override;

  // views::TextfieldController:
  void ContentsChanged(views::Textfield* sender,
                       const base::string16& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // ChildNetworkConfigView:
  base::string16 GetTitle() const override;
  views::View* GetInitiallyFocusedView() override;
  bool CanLogin() override;
  bool Login() override;
  void Cancel() override;
  void InitFocus() override;

 private:
  // Initializes UI.
  void Init();

  // Callback to initialize fields from uncached network properties.
  void InitFromProperties(const std::string& service_path,
                          const base::DictionaryValue& dictionary);

  // Get input values.
  std::string GetEapIdentity() const;
  std::string GetEapPassphrase() const;
  bool GetSaveCredentials() const;
  bool GetShareNetwork(bool share_default) const;

  // Updates state of the Login button.
  void UpdateDialogButtons();

  // Updates the error text label.
  void UpdateErrorLabel();

  NetworkPropertyUIData identity_ui_data_;
  NetworkPropertyUIData passphrase_ui_data_;
  NetworkPropertyUIData save_credentials_ui_data_;

  views::Label* identity_label_;
  views::Textfield* identity_textfield_;
  views::Checkbox* save_credentials_checkbox_;
  views::Checkbox* share_network_checkbox_;
  views::Label* passphrase_label_;
  views::Textfield* passphrase_textfield_;
  views::ToggleImageButton* passphrase_visible_button_;
  views::Label* error_label_;

  base::WeakPtrFactory<WimaxConfigView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WimaxConfigView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_OPTIONS_WIMAX_CONFIG_VIEW_H_
