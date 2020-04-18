// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_OPTIONS_VPN_CONFIG_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_OPTIONS_VPN_CONFIG_VIEW_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/chromeos/options/cert_library.h"
#include "chrome/browser/chromeos/options/network_config_view.h"
#include "chrome/browser/chromeos/options/network_property_ui_data.h"
#include "chrome/browser/chromeos/options/passphrase_textfield.h"
#include "chromeos/network/client_cert_util.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/combobox/combobox_listener.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"

namespace base {
class DictionaryValue;
}

namespace views {
class Checkbox;
class Label;
}

namespace chromeos {

class NetworkState;

namespace internal {
class ProviderTypeComboboxModel;
class VpnServerCACertComboboxModel;
class VpnUserCertComboboxModel;
}

// A dialog box to allow configuration of VPN connection.
class VPNConfigView : public ChildNetworkConfigView,
                      public views::TextfieldController,
                      public views::ButtonListener,
                      public views::ComboboxListener,
                      public CertLibrary::Observer {
 public:
  VPNConfigView(NetworkConfigView* parent, const std::string& service_path);
  ~VPNConfigView() override;

  // views::TextfieldController:
  void ContentsChanged(views::Textfield* sender,
                       const base::string16& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // views::ComboboxListener:
  void OnPerformAction(views::Combobox* combobox) override;

  // CertLibrary::Observer:
  void OnCertificatesLoaded() override;

  // ChildNetworkConfigView:
  base::string16 GetTitle() const override;
  views::View* GetInitiallyFocusedView() override;
  bool CanLogin() override;
  bool Login() override;
  void Cancel() override;
  void InitFocus() override;

 private:
  // Initializes data members and create UI controls.
  void Init();

  // Callback to initialize fields from uncached network properties.
  void InitFromProperties(const std::string& service_path,
                          const base::DictionaryValue& dictionary);
  void ParseUIProperties(const NetworkState* vpn);
  void GetPropertiesError(const std::string& error_name,
                          std::unique_ptr<base::DictionaryValue> error_data);

  // Fill in |properties| with the properties for the selected client (user)
  // certificate or empty properties if no client cert is required.
  void SetUserCertProperties(chromeos::client_cert::ConfigType client_cert_type,
                             base::DictionaryValue* properties) const;

  // Helper function to set configurable properties.
  void SetConfigProperties(base::DictionaryValue* properties);

  // Set and update all control values.
  void Refresh();

  // Update various controls.
  void UpdateControlsToEnable();
  void UpdateControls();
  void UpdateErrorLabel();

  // Update state of the Login button.
  void UpdateCanLogin();

  // Returns true if there is at least one user certificate installed.
  bool HaveUserCerts() const;

  // Returns true if there is a selected user certificate and it is valid.
  bool IsUserCertValid() const;

  // Get text from input field.
  const std::string GetTextFromField(views::Textfield* textfield,
                                     bool trim_whitespace) const;

  // Get passphrase from input field.
  const std::string GetPassphraseFromField(
      PassphraseTextfield* textfield) const;

  // Convenience methods to get text from input field or cached VirtualNetwork.
  std::string GetService() const;
  std::string GetServer() const;
  std::string GetPSKPassphrase() const;
  std::string GetUsername() const;
  std::string GetUserPassphrase() const;
  std::string GetOTP() const;
  std::string GetGroupName() const;
  std::string GetServerCACertPEM() const;
  bool GetSaveCredentials() const;
  int GetProviderTypeIndex() const;
  std::string GetProviderTypeString() const;

  // Parses a VPN UI |property| from the given |network|. |key| is the property
  // name within the type-specific VPN subdictionary named |dict_key|.
  void ParseVPNUIProperty(const NetworkState* network,
                          const std::string& dict_key,
                          const std::string& key,
                          NetworkPropertyUIData* property_ui_data);

  base::string16 service_name_from_server_;
  bool service_text_modified_;

  // Initialized in Init():

  bool enable_psk_passphrase_;
  bool enable_user_cert_;
  bool enable_server_ca_cert_;
  bool enable_otp_;
  bool enable_group_name_;
  bool user_passphrase_required_;

  NetworkPropertyUIData ca_cert_ui_data_;
  NetworkPropertyUIData psk_passphrase_ui_data_;
  NetworkPropertyUIData user_cert_ui_data_;
  NetworkPropertyUIData username_ui_data_;
  NetworkPropertyUIData user_passphrase_ui_data_;
  NetworkPropertyUIData group_name_ui_data_;
  NetworkPropertyUIData save_credentials_ui_data_;

  int title_;

  views::Textfield* server_textfield_;
  views::Label* service_text_;
  views::Textfield* service_textfield_;
  std::unique_ptr<internal::ProviderTypeComboboxModel>
      provider_type_combobox_model_;
  views::Combobox* provider_type_combobox_;
  views::Label* provider_type_text_label_;
  views::Label* psk_passphrase_label_;
  PassphraseTextfield* psk_passphrase_textfield_;
  views::Label* user_cert_label_;
  std::unique_ptr<internal::VpnUserCertComboboxModel> user_cert_combobox_model_;
  views::Combobox* user_cert_combobox_;
  views::Label* server_ca_cert_label_;
  std::unique_ptr<internal::VpnServerCACertComboboxModel>
      server_ca_cert_combobox_model_;
  views::Combobox* server_ca_cert_combobox_;
  views::Textfield* username_textfield_;
  PassphraseTextfield* user_passphrase_textfield_;
  views::Label* otp_label_;
  views::Textfield* otp_textfield_;
  views::Label* group_name_label_;
  views::Textfield* group_name_textfield_;
  views::Checkbox* save_credentials_checkbox_;
  views::Label* error_label_;

  // Cached VPN properties, only set when configuring an existing network.
  int provider_type_index_;
  std::string ca_cert_pem_;
  std::string client_cert_id_;

  base::WeakPtrFactory<VPNConfigView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VPNConfigView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_OPTIONS_VPN_CONFIG_VIEW_H_
