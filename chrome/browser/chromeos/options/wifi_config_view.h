// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_OPTIONS_WIFI_CONFIG_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_OPTIONS_WIFI_CONFIG_VIEW_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "chrome/browser/chromeos/options/cert_library.h"
#include "chrome/browser/chromeos/options/network_config_view.h"
#include "chrome/browser/chromeos/options/network_property_ui_data.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/base/models/combobox_model.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/combobox/combobox_listener.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"

namespace views {
class Checkbox;
class Label;
class ToggleImageButton;
}

namespace chromeos {

class NetworkState;
class PassphraseTextfield;

namespace internal {
class EAPMethodComboboxModel;
class Phase2AuthComboboxModel;
class SecurityComboboxModel;
class ServerCACertComboboxModel;
class UserCertComboboxModel;
}

// A dialog box for configuring Wifi and Ethernet networks

class WifiConfigView : public ChildNetworkConfigView,
                       public views::TextfieldController,
                       public views::ButtonListener,
                       public views::ComboboxListener,
                       public CertLibrary::Observer,
                       public NetworkStateHandlerObserver {
 public:
  // If |service_path| is not empty it identifies the network to be configured.
  // Otherwise |show_8021x| determines whether or not to show the 'advanced'
  // 8021x configuration UI for a hidden WiFi network.
  WifiConfigView(NetworkConfigView* parent,
                 const std::string& service_path,
                 bool show_8021x);
  ~WifiConfigView() override;

  // views::TextfieldController
  void ContentsChanged(views::Textfield* sender,
                       const base::string16& new_contents) override;
  bool HandleKeyEvent(views::Textfield* sender,
                      const ui::KeyEvent& key_event) override;

  // views::ButtonListener
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // views::ComboboxListener
  void OnPerformAction(views::Combobox* combobox) override;

  // CertLibrary::Observer
  void OnCertificatesLoaded() override;

  // ChildNetworkConfigView
  base::string16 GetTitle() const override;
  views::View* GetInitiallyFocusedView() override;
  bool CanLogin() override;
  bool Login() override;
  void Cancel() override;
  void InitFocus() override;
  bool IsConfigureDialog() override;

  // NetworkStateHandlerObserver
  void NetworkPropertiesUpdated(const NetworkState* network) override;

  // Parses a UI |property| from the ONC associated with |network|. |key|
  // is the property name within the ONC dictionary.
  static void ParseUIProperty(NetworkPropertyUIData* property_ui_data,
                              const NetworkState* network,
                              const std::string& key);

  // Parses an EAP UI |property| from the ONC associated with |network|.
  // |key| is the property name within the ONC EAP dictionary.
  static void ParseEAPUIProperty(NetworkPropertyUIData* property_ui_data,
                                 const NetworkState* network,
                                 const std::string& key);

 private:
  friend class internal::UserCertComboboxModel;

  // This will initialize the view depending on whether an existing network
  // is being configured, the type of network, and the security model (i.e.
  // simple password encryption or 802.1x).
  void Init(bool show_8021x);

  // Callback to initialize fields from uncached network properties.
  void InitFromProperties(bool show_8021x,
                          const std::string& service_path,
                          const base::DictionaryValue& dictionary);

  // Get input values.
  std::string GetSsid() const;
  std::string GetPassphrase() const;
  bool GetSaveCredentials() const;
  bool GetShareNetwork(bool share_default) const;

  // Get various 802.1X EAP values from the widgets.
  std::string GetEapMethod() const;
  std::string GetEapPhase2Auth() const;
  std::string GetEapServerCaCertPEM() const;
  bool GetEapUseSystemCas() const;
  std::string GetEapSubjectMatch() const;
  std::string GetEapClientCertPkcs11Id() const;
  std::string GetEapIdentity() const;
  std::string GetEapAnonymousIdentity() const;

  // Fill in |properties| with the properties for the selected client
  // certificate or empty properties if no client cert is required.
  void SetEapClientCertProperties(base::DictionaryValue* properties) const;

  // Fill in |properties| with the appropriate values. If |configured| is
  // true then this is for an already configured network.
  void SetEapProperties(base::DictionaryValue* properties,
                        bool configured);

  // Returns true if the EAP method requires a user certificate.
  bool UserCertRequired() const;

  // Returns true if at least one user certificate is installed.
  bool HaveUserCerts() const;

  // Returns true if there is a selected user certificate and it is valid.
  bool IsUserCertValid() const;

  // Returns true if the phase 2 auth is relevant.
  bool Phase2AuthActive() const;

  // Returns whether the current configuration requires a passphrase.
  bool PassphraseActive() const;

  // Returns true if a user cert should be selected.
  bool UserCertActive() const;

  // Returns true if the user cert is managed but could not be found.
  bool ManagedUserCertNotFound() const { return managed_user_cert_not_found_; }

  // Returns true if a CA cert selection should be allowed.
  bool CaCertActive() const;

  // Updates state of the Login button.
  void UpdateDialogButtons();

  // Enable/Disable EAP fields as appropriate based on selected EAP method.
  void RefreshEapFields();

  // Enable/Disable "share this network" checkbox.
  void RefreshShareCheckbox();

  // Updates the error text label.
  void UpdateErrorLabel();

  // Helper method, returns NULL if |service_path_| is empty, otherwise returns
  // the NetworkState* associated with |service_path_| or NULL if none exists.
  const NetworkState* GetNetworkState() const;

  NetworkPropertyUIData eap_method_ui_data_;
  NetworkPropertyUIData phase_2_auth_ui_data_;
  NetworkPropertyUIData user_cert_ui_data_;
  NetworkPropertyUIData server_ca_cert_ui_data_;
  NetworkPropertyUIData identity_ui_data_;
  NetworkPropertyUIData identity_anonymous_ui_data_;
  NetworkPropertyUIData save_credentials_ui_data_;
  NetworkPropertyUIData passphrase_ui_data_;

  views::Textfield* ssid_textfield_;
  std::unique_ptr<internal::EAPMethodComboboxModel> eap_method_combobox_model_;
  views::Combobox* eap_method_combobox_;
  views::Label* phase_2_auth_label_;
  std::unique_ptr<internal::Phase2AuthComboboxModel>
      phase_2_auth_combobox_model_;
  views::Combobox* phase_2_auth_combobox_;
  views::Label* user_cert_label_;
  std::unique_ptr<internal::UserCertComboboxModel> user_cert_combobox_model_;
  views::Combobox* user_cert_combobox_;
  views::Label* server_ca_cert_label_;
  std::unique_ptr<internal::ServerCACertComboboxModel>
      server_ca_cert_combobox_model_;
  views::Combobox* server_ca_cert_combobox_;
  views::Label* subject_match_label_;
  views::Textfield* subject_match_textfield_;
  views::Label* identity_label_;
  views::Textfield* identity_textfield_;
  views::Label* identity_anonymous_label_;
  views::Textfield* identity_anonymous_textfield_;
  views::Checkbox* save_credentials_checkbox_;
  views::Checkbox* share_network_checkbox_;
  views::Label* shared_network_label_;
  std::unique_ptr<internal::SecurityComboboxModel> security_combobox_model_;
  views::Combobox* security_combobox_;
  views::Label* passphrase_label_;
  PassphraseTextfield* passphrase_textfield_;
  views::ToggleImageButton* passphrase_visible_button_;
  views::Label* error_label_;

  // Indicates that the user certificate is managed by policy (not editable),
  // but the policy-specified certificate has not been found.
  bool managed_user_cert_not_found_ = false;

  base::WeakPtrFactory<WifiConfigView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WifiConfigView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_OPTIONS_WIFI_CONFIG_VIEW_H_
