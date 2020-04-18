// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/options/wifi_config_view.h"

#include <stddef.h>

#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/net/shill_error.h"
#include "chrome/browser/chromeos/options/passphrase_textfield.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/ash/network/enrollment_dialog_view.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "chromeos/login/login_state.h"
#include "chromeos/network/client_cert_util.h"
#include "chromeos/network/network_configuration_handler.h"
#include "chromeos/network/network_connect.h"
#include "chromeos/network/network_event_log.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_ui_data.h"
#include "chromeos/network/onc/onc_utils.h"
#include "chromeos/network/shill_property_util.h"
#include "components/onc/onc_constants.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/events/event.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"

namespace chromeos {

namespace {

// Combobox that supports a preferred width.  Used by Server CA combobox
// because the strings inside it are too wide.
class ComboboxWithWidth : public views::Combobox {
 public:
  ComboboxWithWidth(ui::ComboboxModel* model, int width)
      : Combobox(model),
        width_(width) {
  }
  ~ComboboxWithWidth() override {}
  gfx::Size CalculatePreferredSize() const override {
    gfx::Size size = Combobox::CalculatePreferredSize();
    size.set_width(width_);
    return size;
  }
 private:
  int width_;
  DISALLOW_COPY_AND_ASSIGN(ComboboxWithWidth);
};

enum SecurityComboboxIndex {
  SECURITY_INDEX_NONE  = 0,
  SECURITY_INDEX_WEP   = 1,
  SECURITY_INDEX_PSK   = 2,
  SECURITY_INDEX_COUNT = 3
};

// Methods in alphabetical order.
enum EAPMethodComboboxIndex {
  EAP_METHOD_INDEX_NONE  = 0,
  EAP_METHOD_INDEX_LEAP  = 1,
  EAP_METHOD_INDEX_PEAP  = 2,
  EAP_METHOD_INDEX_TLS   = 3,
  EAP_METHOD_INDEX_TTLS  = 4,
  EAP_METHOD_INDEX_COUNT = 5
};

enum Phase2AuthComboboxIndex {
  PHASE_2_AUTH_INDEX_AUTO     = 0,  // LEAP, EAP-TLS have only this auth.
  PHASE_2_AUTH_INDEX_MD5      = 1,
  PHASE_2_AUTH_INDEX_MSCHAPV2 = 2,  // PEAP has up to this auth.
  PHASE_2_AUTH_INDEX_MSCHAP   = 3,
  PHASE_2_AUTH_INDEX_PAP      = 4,
  PHASE_2_AUTH_INDEX_CHAP     = 5,  // EAP-TTLS has up to this auth.
  PHASE_2_AUTH_INDEX_COUNT    = 6
};

void ShillError(const std::string& function,
                const std::string& error_name,
                std::unique_ptr<base::DictionaryValue> error_data) {
  NET_LOG_ERROR("Shill Error from WifiConfigView: " + error_name, function);
}

}  // namespace

namespace internal {

class SecurityComboboxModel : public ui::ComboboxModel {
 public:
  SecurityComboboxModel();
  ~SecurityComboboxModel() override;

  // Overridden from ui::ComboboxModel:
  int GetItemCount() const override;
  base::string16 GetItemAt(int index) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SecurityComboboxModel);
};

class EAPMethodComboboxModel : public ui::ComboboxModel {
 public:
  EAPMethodComboboxModel();
  ~EAPMethodComboboxModel() override;

  // Overridden from ui::ComboboxModel:
  int GetItemCount() const override;
  base::string16 GetItemAt(int index) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(EAPMethodComboboxModel);
};

class Phase2AuthComboboxModel : public ui::ComboboxModel {
 public:
  explicit Phase2AuthComboboxModel(views::Combobox* eap_method_combobox);
  ~Phase2AuthComboboxModel() override;

  // Overridden from ui::ComboboxModel:
  int GetItemCount() const override;
  base::string16 GetItemAt(int index) override;

 private:
  views::Combobox* eap_method_combobox_;

  DISALLOW_COPY_AND_ASSIGN(Phase2AuthComboboxModel);
};

class ServerCACertComboboxModel : public ui::ComboboxModel {
 public:
  ServerCACertComboboxModel();
  ~ServerCACertComboboxModel() override;

  // Overridden from ui::ComboboxModel:
  int GetItemCount() const override;
  base::string16 GetItemAt(int index) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ServerCACertComboboxModel);
};

class UserCertComboboxModel : public ui::ComboboxModel {
 public:
  explicit UserCertComboboxModel(WifiConfigView* owner);
  ~UserCertComboboxModel() override;

  // Overridden from ui::ComboboxModel:
  int GetItemCount() const override;
  base::string16 GetItemAt(int index) override;

 private:
  WifiConfigView* owner_;

  DISALLOW_COPY_AND_ASSIGN(UserCertComboboxModel);
};

// SecurityComboboxModel -------------------------------------------------------

SecurityComboboxModel::SecurityComboboxModel() {
}

SecurityComboboxModel::~SecurityComboboxModel() {
}

int SecurityComboboxModel::GetItemCount() const {
    return SECURITY_INDEX_COUNT;
  }
base::string16 SecurityComboboxModel::GetItemAt(int index) {
  if (index == SECURITY_INDEX_NONE)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_SECURITY_NONE);
  else if (index == SECURITY_INDEX_WEP)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_SECURITY_WEP);
  else if (index == SECURITY_INDEX_PSK)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_SECURITY_PSK);
  NOTREACHED();
  return base::string16();
}

// EAPMethodComboboxModel ------------------------------------------------------

EAPMethodComboboxModel::EAPMethodComboboxModel() {
}

EAPMethodComboboxModel::~EAPMethodComboboxModel() {
}

int EAPMethodComboboxModel::GetItemCount() const {
  return EAP_METHOD_INDEX_COUNT;
}
base::string16 EAPMethodComboboxModel::GetItemAt(int index) {
  if (index == EAP_METHOD_INDEX_NONE)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_EAP_METHOD_NONE);
  else if (index == EAP_METHOD_INDEX_LEAP)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_EAP_METHOD_LEAP);
  else if (index == EAP_METHOD_INDEX_PEAP)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_EAP_METHOD_PEAP);
  else if (index == EAP_METHOD_INDEX_TLS)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_EAP_METHOD_TLS);
  else if (index == EAP_METHOD_INDEX_TTLS)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_EAP_METHOD_TTLS);
  NOTREACHED();
  return base::string16();
}

// Phase2AuthComboboxModel -----------------------------------------------------

Phase2AuthComboboxModel::Phase2AuthComboboxModel(
    views::Combobox* eap_method_combobox)
    : eap_method_combobox_(eap_method_combobox) {
}

Phase2AuthComboboxModel::~Phase2AuthComboboxModel() {
}

int Phase2AuthComboboxModel::GetItemCount() const {
  switch (eap_method_combobox_->selected_index()) {
    case EAP_METHOD_INDEX_NONE:
    case EAP_METHOD_INDEX_TLS:
    case EAP_METHOD_INDEX_LEAP:
      return PHASE_2_AUTH_INDEX_AUTO + 1;
    case EAP_METHOD_INDEX_PEAP:
      return PHASE_2_AUTH_INDEX_MSCHAPV2 + 1;
    case EAP_METHOD_INDEX_TTLS:
      return PHASE_2_AUTH_INDEX_CHAP + 1;
  }
  NOTREACHED();
  return 0;
}

base::string16 Phase2AuthComboboxModel::GetItemAt(int index) {
  if (index == PHASE_2_AUTH_INDEX_AUTO)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PHASE_2_AUTH_AUTO);
  else if (index == PHASE_2_AUTH_INDEX_MD5)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PHASE_2_AUTH_MD5);
  else if (index == PHASE_2_AUTH_INDEX_MSCHAPV2)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PHASE_2_AUTH_MSCHAPV2);
  else if (index == PHASE_2_AUTH_INDEX_MSCHAP)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PHASE_2_AUTH_MSCHAP);
  else if (index == PHASE_2_AUTH_INDEX_PAP)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PHASE_2_AUTH_PAP);
  else if (index == PHASE_2_AUTH_INDEX_CHAP)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PHASE_2_AUTH_CHAP);
  NOTREACHED();
  return base::string16();
}

// ServerCACertComboboxModel ---------------------------------------------------

ServerCACertComboboxModel::ServerCACertComboboxModel() {
}

ServerCACertComboboxModel::~ServerCACertComboboxModel() {
}

int ServerCACertComboboxModel::GetItemCount() const {
  if (CertLibrary::Get()->CertificatesLoading())
    return 1;  // "Loading"
  // First "Default", then the certs, then "Do not check".
  return CertLibrary::Get()->NumCertificates(
      CertLibrary::CERT_TYPE_SERVER_CA) + 2;
}

base::string16 ServerCACertComboboxModel::GetItemAt(int index) {
  if (CertLibrary::Get()->CertificatesLoading())
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_CERT_LOADING);
  if (index == 0)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_CERT_SERVER_CA_DEFAULT);
  if (index == GetItemCount() - 1)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_CERT_SERVER_CA_DO_NOT_CHECK);
  int cert_index = index - 1;
  return CertLibrary::Get()->GetCertDisplayStringAt(
      CertLibrary::CERT_TYPE_SERVER_CA, cert_index);
}

// UserCertComboboxModel -------------------------------------------------------

UserCertComboboxModel::UserCertComboboxModel(WifiConfigView* owner)
    : owner_(owner) {
}

UserCertComboboxModel::~UserCertComboboxModel() {
}

int UserCertComboboxModel::GetItemCount() const {
  if (!owner_->UserCertActive())
    return 1;  // "None installed" (combobox must have at least 1 entry)
  if (CertLibrary::Get()->CertificatesLoading())
    return 1;  // "Loading"
  int num_certs =
      CertLibrary::Get()->NumCertificates(CertLibrary::CERT_TYPE_USER);
  if (num_certs == 0)
    return 1;  // "None installed"
  if (owner_->ManagedUserCertNotFound())
    return 1;  // Empty: no user cert found, but managed (not editable).
  return num_certs;
}

base::string16 UserCertComboboxModel::GetItemAt(int index) {
  if (!owner_->UserCertActive())
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_USER_CERT_NONE_INSTALLED);
  if (CertLibrary::Get()->CertificatesLoading())
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_CERT_LOADING);
  if (CertLibrary::Get()->NumCertificates(CertLibrary::CERT_TYPE_USER) == 0)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_USER_CERT_NONE_INSTALLED);
  if (owner_->ManagedUserCertNotFound())
    return base::string16();  // Empty: no user cert found, but managed (not
                              // editable).
  return CertLibrary::Get()->GetCertDisplayStringAt(
      CertLibrary::CERT_TYPE_USER, index);
}

}  // namespace internal

WifiConfigView::WifiConfigView(NetworkConfigView* parent,
                               const std::string& service_path,
                               bool show_8021x)
    : ChildNetworkConfigView(parent, service_path),
      ssid_textfield_(NULL),
      eap_method_combobox_(NULL),
      phase_2_auth_label_(NULL),
      phase_2_auth_combobox_(NULL),
      user_cert_label_(NULL),
      user_cert_combobox_(NULL),
      server_ca_cert_label_(NULL),
      server_ca_cert_combobox_(NULL),
      subject_match_label_(NULL),
      subject_match_textfield_(NULL),
      identity_label_(NULL),
      identity_textfield_(NULL),
      identity_anonymous_label_(NULL),
      identity_anonymous_textfield_(NULL),
      save_credentials_checkbox_(NULL),
      share_network_checkbox_(NULL),
      shared_network_label_(NULL),
      security_combobox_(NULL),
      passphrase_label_(NULL),
      passphrase_textfield_(NULL),
      passphrase_visible_button_(NULL),
      error_label_(NULL),
      weak_ptr_factory_(this) {
  Init(show_8021x);
  NetworkHandler::Get()->network_state_handler()->AddObserver(this, FROM_HERE);
}

WifiConfigView::~WifiConfigView() {
  RemoveAllChildViews(true);  // Destroy children before models
  if (NetworkHandler::IsInitialized()) {
    NetworkHandler::Get()->network_state_handler()->RemoveObserver(
        this, FROM_HERE);
  }
  CertLibrary::Get()->RemoveObserver(this);
}

base::string16 WifiConfigView::GetTitle() const {
  const NetworkState* network = GetNetworkState();
  if (network && network->type() == shill::kTypeEthernet)
    return l10n_util::GetStringUTF16(IDS_OPTIONS_SETTINGS_CONFIGURE_ETHERNET);
  return l10n_util::GetStringUTF16(IDS_OPTIONS_SETTINGS_JOIN_WIFI_NETWORKS);
}

views::View* WifiConfigView::GetInitiallyFocusedView() {
  // Return a reasonable widget for initial focus,
  // depending on what we're showing.
  if (ssid_textfield_)
    return ssid_textfield_;
  else if (eap_method_combobox_)
    return eap_method_combobox_;
  else if (passphrase_textfield_ && passphrase_textfield_->enabled())
    return passphrase_textfield_;
  else
    return NULL;
}

bool WifiConfigView::CanLogin() {
  static const size_t kMinPSKPasswordLen = 5;

  // We either have an existing network or the user entered an SSID.
  if (service_path_.empty() && GetSsid().empty())
    return false;

  // If a non-EAP network requires a passphrase, ensure it is the right length.
  if (passphrase_textfield_ != NULL &&
      passphrase_textfield_->enabled() &&
      !passphrase_textfield_->show_fake() &&
      !eap_method_combobox_ &&
      passphrase_textfield_->text().length() < kMinPSKPasswordLen)
    return false;

  // If we're using EAP, we must have a method.
  if (eap_method_combobox_ &&
      eap_method_combobox_->selected_index() == EAP_METHOD_INDEX_NONE)
    return false;

  // Block login if certs are required but user has none.
  if (UserCertRequired() && (!HaveUserCerts() || !IsUserCertValid()))
      return false;

  return true;
}

bool WifiConfigView::UserCertRequired() const {
  return UserCertActive();
}

bool WifiConfigView::HaveUserCerts() const {
  return CertLibrary::Get()->NumCertificates(CertLibrary::CERT_TYPE_USER) > 0;
}

bool WifiConfigView::IsUserCertValid() const {
  if (!UserCertActive())
    return false;
  int index = user_cert_combobox_->selected_index();
  if (index < 0)
    return false;
  // Currently only hardware-backed user certificates are valid.
  if (!CertLibrary::Get()->IsCertHardwareBackedAt(CertLibrary::CERT_TYPE_USER,
                                                  index)) {
    return false;
  }
  return true;
}

bool WifiConfigView::Phase2AuthActive() const {
  if (phase_2_auth_combobox_)
    return phase_2_auth_combobox_->model()->GetItemCount() > 1;
  return false;
}

bool WifiConfigView::PassphraseActive() const {
  if (eap_method_combobox_) {
    // No password for EAP-TLS.
    int index = eap_method_combobox_->selected_index();
    return index != EAP_METHOD_INDEX_NONE && index != EAP_METHOD_INDEX_TLS;
  } else if (security_combobox_) {
    return security_combobox_->selected_index() != SECURITY_INDEX_NONE;
  }
  return false;
}

bool WifiConfigView::UserCertActive() const {
  // User certs only for EAP-TLS.
  if (eap_method_combobox_)
    return eap_method_combobox_->selected_index() == EAP_METHOD_INDEX_TLS;

  return false;
}

bool WifiConfigView::CaCertActive() const {
  // No server CA certs for LEAP.
  if (eap_method_combobox_) {
    int index = eap_method_combobox_->selected_index();
    return index != EAP_METHOD_INDEX_NONE && index != EAP_METHOD_INDEX_LEAP;
  }
  return false;
}

void WifiConfigView::UpdateDialogButtons() {
  parent_->DialogModelChanged();
}

void WifiConfigView::RefreshEapFields() {
  // If EAP method changes, the phase 2 auth choices may have changed also.
  phase_2_auth_combobox_->ModelChanged();
  phase_2_auth_combobox_->SetSelectedIndex(0);
  bool phase_2_auth_enabled = Phase2AuthActive();
  phase_2_auth_combobox_->SetEnabled(phase_2_auth_enabled &&
                                     phase_2_auth_ui_data_.IsEditable());
  phase_2_auth_label_->SetEnabled(phase_2_auth_enabled);

  // Passphrase.
  bool passphrase_enabled = PassphraseActive();
  passphrase_textfield_->SetEnabled(passphrase_enabled &&
                                    passphrase_ui_data_.IsEditable());
  passphrase_label_->SetEnabled(passphrase_enabled);
  if (!passphrase_enabled)
    passphrase_textfield_->SetText(base::string16());

  // User cert.
  bool certs_loading = CertLibrary::Get()->CertificatesLoading();
  bool user_cert_enabled = UserCertActive();
  user_cert_label_->SetEnabled(user_cert_enabled);
  bool have_user_certs = !certs_loading && HaveUserCerts();
  user_cert_combobox_->SetEnabled(user_cert_enabled &&
                                  have_user_certs &&
                                  user_cert_ui_data_.IsEditable());
  user_cert_combobox_->ModelChanged();
  user_cert_combobox_->SetSelectedIndex(0);

  // Server CA.
  bool ca_cert_enabled = CaCertActive();
  server_ca_cert_label_->SetEnabled(ca_cert_enabled);
  server_ca_cert_combobox_->SetEnabled(ca_cert_enabled &&
                                       !certs_loading &&
                                       server_ca_cert_ui_data_.IsEditable());
  server_ca_cert_combobox_->ModelChanged();
  server_ca_cert_combobox_->SetSelectedIndex(0);

  // Subject Match
  bool subject_match_enabled =
      ca_cert_enabled && eap_method_combobox_ &&
      eap_method_combobox_->selected_index() == EAP_METHOD_INDEX_TLS;
  subject_match_label_->SetEnabled(subject_match_enabled);
  subject_match_textfield_->SetEnabled(subject_match_enabled);
  if (!subject_match_enabled)
    subject_match_textfield_->SetText(base::string16());

  // No anonymous identity if no phase 2 auth.
  bool identity_anonymous_enabled = phase_2_auth_enabled;
  identity_anonymous_textfield_->SetEnabled(
      identity_anonymous_enabled && identity_anonymous_ui_data_.IsEditable());
  identity_anonymous_label_->SetEnabled(identity_anonymous_enabled);
  if (!identity_anonymous_enabled)
    identity_anonymous_textfield_->SetText(base::string16());

  RefreshShareCheckbox();
}

void WifiConfigView::RefreshShareCheckbox() {
  if (!share_network_checkbox_)
    return;

  if (security_combobox_ &&
      security_combobox_->selected_index() == SECURITY_INDEX_NONE) {
    share_network_checkbox_->SetEnabled(false);
    share_network_checkbox_->SetChecked(true);
  } else if (eap_method_combobox_ &&
             (eap_method_combobox_->selected_index() == EAP_METHOD_INDEX_TLS ||
              user_cert_combobox_->selected_index() != 0)) {
    // Can not share TLS network (requires certificate), or any network where
    // user certificates are enabled.
    share_network_checkbox_->SetEnabled(false);
    share_network_checkbox_->SetChecked(false);
  } else {
    bool value = false;
    bool enabled = false;
    ChildNetworkConfigView::GetShareStateForLoginState(&value, &enabled);

    share_network_checkbox_->SetChecked(value);
    share_network_checkbox_->SetEnabled(enabled);
  }
}

void WifiConfigView::UpdateErrorLabel() {
  base::string16 error_msg;
  if (UserCertRequired() && CertLibrary::Get()->CertificatesLoaded()) {
    if (!HaveUserCerts()) {
      if (!LoginState::Get()->IsUserLoggedIn() ||
          LoginState::Get()->IsGuestSessionUser()) {
        error_msg = l10n_util::GetStringUTF16(
            IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_LOGIN_FOR_USER_CERT);
      } else {
        error_msg = l10n_util::GetStringUTF16(
            IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PLEASE_INSTALL_USER_CERT);
      }
    } else if (!IsUserCertValid()) {
      error_msg = l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_REQUIRE_HARDWARE_BACKED);
    }
  }
  if (error_msg.empty() && !service_path_.empty()) {
    const NetworkState* network = GetNetworkState();
    if (network && network->connection_state() == shill::kStateFailure) {
      error_msg = shill_error::GetShillErrorString(network->last_error(),
                                                   network->guid());
    }
  }
  if (!error_msg.empty()) {
    error_label_->SetText(error_msg);
    error_label_->SetVisible(true);
  } else {
    error_label_->SetVisible(false);
  }
}

void WifiConfigView::ContentsChanged(views::Textfield* sender,
                                     const base::string16& new_contents) {
  UpdateDialogButtons();
}

bool WifiConfigView::HandleKeyEvent(views::Textfield* sender,
                                    const ui::KeyEvent& key_event) {
  if (sender == passphrase_textfield_ &&
      key_event.type() == ui::ET_KEY_PRESSED &&
      key_event.key_code() == ui::VKEY_RETURN) {
    parent_->GetDialogClientView()->AcceptWindow();
  }
  return false;
}

void WifiConfigView::ButtonPressed(views::Button* sender,
                                   const ui::Event& event) {
  if (sender == passphrase_visible_button_ && passphrase_textfield_) {
    if (passphrase_textfield_->GetTextInputType() == ui::TEXT_INPUT_TYPE_TEXT) {
      passphrase_textfield_->SetTextInputType(ui::TEXT_INPUT_TYPE_PASSWORD);
      passphrase_visible_button_->SetToggled(false);
    } else {
      passphrase_textfield_->SetTextInputType(ui::TEXT_INPUT_TYPE_TEXT);
      passphrase_visible_button_->SetToggled(true);
    }
  } else {
    NOTREACHED();
  }
}

void WifiConfigView::OnPerformAction(views::Combobox* combobox) {
  if (combobox == security_combobox_) {
    bool passphrase_enabled = PassphraseActive();
    passphrase_label_->SetEnabled(passphrase_enabled);
    passphrase_textfield_->SetEnabled(passphrase_enabled &&
                                      passphrase_ui_data_.IsEditable());
    if (!passphrase_enabled)
      passphrase_textfield_->SetText(base::string16());
    RefreshShareCheckbox();
  } else if (combobox == user_cert_combobox_) {
    RefreshShareCheckbox();
  } else if (combobox == eap_method_combobox_) {
    RefreshEapFields();
  }
  UpdateDialogButtons();
  UpdateErrorLabel();
}

void WifiConfigView::OnCertificatesLoaded() {
  RefreshEapFields();
  UpdateDialogButtons();
  UpdateErrorLabel();
}

bool WifiConfigView::Login() {
  const NetworkState* network = GetNetworkState();

  // Set configuration properties.
  base::DictionaryValue properties;

  // Default shared state for non-private networks is true.
  const bool share_default = !network || !network->IsPrivate();
  bool share_network = GetShareNetwork(share_default);
  bool only_policy_autoconnect =
      onc::PolicyAllowsOnlyPolicyNetworksToAutoconnect(!share_network);
  if (only_policy_autoconnect) {
    properties.SetKey(shill::kAutoConnectProperty, base::Value(false));
  }

  if (service_path_.empty()) {
    // TODO(stevenjb): Support modifying existing EAP configurations.
    // Will probably wait to do this in WebUI instead.
    properties.SetKey(shill::kTypeProperty, base::Value(shill::kTypeWifi));
    shill_property_util::SetSSID(GetSsid(), &properties);
    properties.SetKey(shill::kModeProperty, base::Value(shill::kModeManaged));
    properties.SetKey(shill::kSaveCredentialsProperty,
                      base::Value(GetSaveCredentials()));
    std::string security_class = shill::kSecurityNone;
    if (!eap_method_combobox_) {
      switch (security_combobox_->selected_index()) {
        case SECURITY_INDEX_NONE:
          security_class = shill::kSecurityNone;
          break;
        case SECURITY_INDEX_WEP:
          security_class = shill::kSecurityWep;
          break;
        case SECURITY_INDEX_PSK:
          security_class = shill::kSecurityPsk;
          break;
      }
      std::string passphrase = GetPassphrase();
      if (!passphrase.empty()) {
        properties.SetKey(shill::kPassphraseProperty,
                          base::Value(GetPassphrase()));
      }
    } else {
      security_class = shill::kSecurity8021x;
      SetEapProperties(&properties, false /* not configured */);
    }
    properties.SetKey(shill::kSecurityClassProperty,
                      base::Value(security_class));

    // Configure and connect to network.
    NetworkConnect::Get()->CreateConfigurationAndConnect(&properties,
                                                         share_network);
  } else {
    if (!network) {
      // Shill no longer knows about this network (edge case).
      // TODO(stevenjb): Add notification for this.
      NET_LOG_ERROR("Network not found", service_path_);
      return true;  // Close dialog
    }
    if (eap_method_combobox_) {
      SetEapProperties(&properties, true /* configured */);
      properties.SetKey(shill::kSaveCredentialsProperty,
                        base::Value(GetSaveCredentials()));
    } else {
      const std::string passphrase = GetPassphrase();
      if (!passphrase.empty()) {
        properties.SetKey(shill::kPassphraseProperty, base::Value(passphrase));
      }
    }
    if (network->type() == shill::kTypeEthernet) {
      // When configuring an ethernet service, we actually configure the
      // EthernetEap service, which exists in the Profile only.
      // See crbug.com/126870 for more info.
      properties.SetKey(shill::kTypeProperty,
                        base::Value(shill::kTypeEthernetEap));
      share_network = false;
      NetworkConnect::Get()->CreateConfiguration(&properties, share_network);
    } else {
      NetworkConnect::Get()->ConfigureNetworkIdAndConnect(
          network->guid(), properties, share_network);
    }
  }
  return true;  // dialog will be closed
}

std::string WifiConfigView::GetSsid() const {
  std::string result;
  if (ssid_textfield_ != NULL) {
    std::string untrimmed = base::UTF16ToUTF8(ssid_textfield_->text());
    base::TrimWhitespaceASCII(untrimmed, base::TRIM_ALL, &result);
  }
  return result;
}

std::string WifiConfigView::GetPassphrase() const {
  std::string result;
  if (passphrase_textfield_ != NULL)
    result = base::UTF16ToUTF8(passphrase_textfield_->text());
  return result;
}

bool WifiConfigView::GetSaveCredentials() const {
  if (!save_credentials_checkbox_)
    return true;  // share networks by default (e.g. non 8021x).
  return save_credentials_checkbox_->checked();
}

bool WifiConfigView::GetShareNetwork(bool share_default) const {
  if (!share_network_checkbox_)
    return share_default;
  return share_network_checkbox_->checked();
}

std::string WifiConfigView::GetEapMethod() const {
  DCHECK(eap_method_combobox_);
  switch (eap_method_combobox_->selected_index()) {
    case EAP_METHOD_INDEX_PEAP:
      return shill::kEapMethodPEAP;
    case EAP_METHOD_INDEX_TLS:
      return shill::kEapMethodTLS;
    case EAP_METHOD_INDEX_TTLS:
      return shill::kEapMethodTTLS;
    case EAP_METHOD_INDEX_LEAP:
      return shill::kEapMethodLEAP;
    case EAP_METHOD_INDEX_NONE:
    default:
      return "";
  }
}

std::string WifiConfigView::GetEapPhase2Auth() const {
  DCHECK(phase_2_auth_combobox_);
  bool is_peap = (GetEapMethod() == shill::kEapMethodPEAP);
  switch (phase_2_auth_combobox_->selected_index()) {
    case PHASE_2_AUTH_INDEX_MD5:
      return is_peap ? shill::kEapPhase2AuthPEAPMD5
          : shill::kEapPhase2AuthTTLSMD5;
    case PHASE_2_AUTH_INDEX_MSCHAPV2:
      return is_peap ? shill::kEapPhase2AuthPEAPMSCHAPV2
          : shill::kEapPhase2AuthTTLSMSCHAPV2;
    case PHASE_2_AUTH_INDEX_MSCHAP:
      return shill::kEapPhase2AuthTTLSMSCHAP;
    case PHASE_2_AUTH_INDEX_PAP:
      return shill::kEapPhase2AuthTTLSPAP;
    case PHASE_2_AUTH_INDEX_CHAP:
      return shill::kEapPhase2AuthTTLSCHAP;
    case PHASE_2_AUTH_INDEX_AUTO:
    default:
      return "";
  }
}

std::string WifiConfigView::GetEapServerCaCertPEM() const {
  DCHECK(server_ca_cert_combobox_);
  int index = server_ca_cert_combobox_->selected_index();
  if (index == 0) {
    // First item is "Default".
    return std::string();
  } else if (index == server_ca_cert_combobox_->model()->GetItemCount() - 1) {
    // Last item is "Do not check".
    return std::string();
  } else {
    int cert_index = index - 1;
    return CertLibrary::Get()->GetServerCACertPEMAt(cert_index);
  }
}

bool WifiConfigView::GetEapUseSystemCas() const {
  DCHECK(server_ca_cert_combobox_);
  // Only use system CAs if the first item ("Default") is selected.
  return server_ca_cert_combobox_->selected_index() == 0;
}

std::string WifiConfigView::GetEapSubjectMatch() const {
  DCHECK(subject_match_textfield_);
  return base::UTF16ToUTF8(subject_match_textfield_->text());
}

void WifiConfigView::SetEapClientCertProperties(
    base::DictionaryValue* properties) const {
  DCHECK(user_cert_combobox_);
  if (!HaveUserCerts() || !UserCertActive()) {
    // No certificate selected or not required.
    client_cert::SetEmptyShillProperties(client_cert::CONFIG_TYPE_EAP,
                                         properties);
  } else {
    // Certificates are listed in the order they appear in the model.
    int index = user_cert_combobox_->selected_index();
    int slot_id = -1;
    const std::string pkcs11_id =
        CertLibrary::Get()->GetUserCertPkcs11IdAt(index, &slot_id);
    client_cert::SetShillProperties(
        client_cert::CONFIG_TYPE_EAP, slot_id, pkcs11_id, properties);
  }
}

std::string WifiConfigView::GetEapIdentity() const {
  DCHECK(identity_textfield_);
  return base::UTF16ToUTF8(identity_textfield_->text());
}

std::string WifiConfigView::GetEapAnonymousIdentity() const {
  DCHECK(identity_anonymous_textfield_);
  return base::UTF16ToUTF8(identity_anonymous_textfield_->text());
}

void WifiConfigView::SetEapProperties(base::DictionaryValue* properties,
                                      bool configured) {
  properties->SetKey(shill::kEapIdentityProperty,
                     base::Value(GetEapIdentity()));
  properties->SetKey(shill::kEapMethodProperty, base::Value(GetEapMethod()));
  properties->SetKey(shill::kEapPhase2AuthProperty,
                     base::Value(GetEapPhase2Auth()));
  properties->SetKey(shill::kEapAnonymousIdentityProperty,
                     base::Value(GetEapAnonymousIdentity()));
  properties->SetKey(shill::kEapSubjectMatchProperty,
                     base::Value(GetEapSubjectMatch()));

  SetEapClientCertProperties(properties);

  properties->SetKey(shill::kEapUseSystemCasProperty,
                     base::Value(GetEapUseSystemCas()));
  if (!configured || passphrase_textfield_->changed()) {
    properties->SetKey(shill::kEapPasswordProperty,
                       base::Value(GetPassphrase()));
  }
  auto pem_list = std::make_unique<base::ListValue>();
  std::string ca_cert_pem = GetEapServerCaCertPEM();
  if (!ca_cert_pem.empty())
    pem_list->AppendString(ca_cert_pem);
  properties->SetWithoutPathExpansion(shill::kEapCaCertPemProperty,
                                      std::move(pem_list));
}

void WifiConfigView::Cancel() {
}

void WifiConfigView::Init(bool show_8021x) {
  views::LayoutProvider* provider = views::LayoutProvider::Get();
  SetBorder(views::CreateEmptyBorder(
      provider->GetDialogInsetsForContentType(views::TEXT, views::TEXT)));

  const NetworkState* network = GetNetworkState();
  if (network) {
    if (network->type() == shill::kTypeWifi) {
      if (network->security_class() == shill::kSecurity8021x ||
          network->IsDynamicWep()) {
        show_8021x = true;
      }
    } else if (network->type() == shill::kTypeEthernet) {
      show_8021x = true;
    } else {
      NOTREACHED() << "Unexpected network type for WifiConfigView: "
                   << network->type() << " Path: " << service_path_;
    }
    ParseEAPUIProperty(&eap_method_ui_data_, network, ::onc::eap::kOuter);
    ParseEAPUIProperty(&phase_2_auth_ui_data_, network, ::onc::eap::kInner);
    ParseEAPUIProperty(
        &user_cert_ui_data_, network, ::onc::client_cert::kClientCertRef);
    ParseEAPUIProperty(
        &server_ca_cert_ui_data_, network, ::onc::eap::kServerCARef);
    if (server_ca_cert_ui_data_.IsManaged()) {
      ParseEAPUIProperty(
          &server_ca_cert_ui_data_, network, ::onc::eap::kUseSystemCAs);
    }
    ParseEAPUIProperty(&identity_ui_data_, network, ::onc::eap::kIdentity);
    ParseEAPUIProperty(
        &identity_anonymous_ui_data_, network, ::onc::eap::kAnonymousIdentity);
    ParseEAPUIProperty(
        &save_credentials_ui_data_, network, ::onc::eap::kSaveCredentials);
    if (show_8021x)
      ParseEAPUIProperty(&passphrase_ui_data_, network, ::onc::eap::kPassword);
    else
      ParseUIProperty(&passphrase_ui_data_, network, ::onc::wifi::kPassphrase);
  }

  views::GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));

  const int column_view_set_id = 0;
  views::ColumnSet* column_set = layout->AddColumnSet(column_view_set_id);
  const int kPasswordVisibleWidth = 20;
  // Label
  if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
    // This constant ensures the minimum width of the label column and ensures
    // the width of the Wifi dialog equals 512.
    const int kLabelMinWidth = 158;
    column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::FILL,
                          1, views::GridLayout::USE_PREF, 0, kLabelMinWidth);
  } else {
    column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::FILL,
                          1, views::GridLayout::USE_PREF, 0, 0);
  }
  column_set->AddPaddingColumn(
      0,
      provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_HORIZONTAL));
  // Textfield, combobox.
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1,
                        views::GridLayout::USE_PREF, 0,
                        ChildNetworkConfigView::kInputFieldMinWidth);
  column_set->AddPaddingColumn(
      0,
      provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_HORIZONTAL));
  // Password visible button / policy indicator.
  column_set->AddColumn(views::GridLayout::CENTER, views::GridLayout::FILL, 0,
                        views::GridLayout::FIXED, kPasswordVisibleWidth, 0);

  // SSID input
  if (!network || network->type() != shill::kTypeEthernet) {
    layout->StartRow(0, column_view_set_id);
    layout->AddView(new views::Label(l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_NETWORK_ID)));
    if (!network) {
      ssid_textfield_ = new views::Textfield();
      ssid_textfield_->set_controller(this);
      ssid_textfield_->SetAccessibleName(l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_NETWORK_ID));
      if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
        layout->AddView(ssid_textfield_, 1, 1, views::GridLayout::FILL,
                        views::GridLayout::FILL, 0,
                        ChildNetworkConfigView::kInputFieldHeight);
      } else {
        layout->AddView(ssid_textfield_);
      }
    } else {
      views::Label* label =
          new views::Label(base::UTF8ToUTF16(network->name()));
      label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      layout->AddView(label);
    }
    layout->AddPaddingRow(0, provider->GetDistanceMetric(
                                 views::DISTANCE_RELATED_CONTROL_VERTICAL));
  }

  // Security select
  if (!network && !show_8021x) {
    layout->StartRow(0, column_view_set_id);
    base::string16 label_text = l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_SECURITY);
    layout->AddView(new views::Label(label_text));
    security_combobox_model_.reset(new internal::SecurityComboboxModel);
    security_combobox_ = new views::Combobox(security_combobox_model_.get());
    security_combobox_->SetAccessibleName(label_text);
    security_combobox_->set_listener(this);
    if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
      layout->AddView(security_combobox_, 1, 1, views::GridLayout::FILL,
                      views::GridLayout::FILL, 0,
                      ChildNetworkConfigView::kInputFieldHeight);
    } else {
      layout->AddView(security_combobox_);
    }
    layout->AddView(security_combobox_);
    layout->AddPaddingRow(0, provider->GetDistanceMetric(
                                 views::DISTANCE_RELATED_CONTROL_VERTICAL));
  }

  // Only enumerate certificates in the data model for 802.1X networks.
  if (show_8021x) {
    // Observer any changes to the certificate list.
    CertLibrary::Get()->AddObserver(this);

    // EAP method
    layout->StartRow(0, column_view_set_id);
    base::string16 eap_label_text = l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_EAP_METHOD);
    layout->AddView(new views::Label(eap_label_text));
    eap_method_combobox_model_.reset(new internal::EAPMethodComboboxModel);
    eap_method_combobox_ = new views::Combobox(
        eap_method_combobox_model_.get());
    eap_method_combobox_->SetAccessibleName(eap_label_text);
    eap_method_combobox_->set_listener(this);
    eap_method_combobox_->SetEnabled(eap_method_ui_data_.IsEditable());
    if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
      layout->AddView(eap_method_combobox_, 1, 1, views::GridLayout::FILL,
                      views::GridLayout::FILL, 0,
                      ChildNetworkConfigView::kInputFieldHeight);
    } else {
      layout->AddView(eap_method_combobox_);
    }
    layout->AddView(new ControlledSettingIndicatorView(eap_method_ui_data_));
    layout->AddPaddingRow(0, provider->GetDistanceMetric(
                                 views::DISTANCE_RELATED_CONTROL_VERTICAL));

    // Phase 2 authentication
    layout->StartRow(0, column_view_set_id);
    base::string16 phase_2_label_text = l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PHASE_2_AUTH);
    phase_2_auth_label_ = new views::Label(phase_2_label_text);
    layout->AddView(phase_2_auth_label_);
    phase_2_auth_combobox_model_.reset(
        new internal::Phase2AuthComboboxModel(eap_method_combobox_));
    phase_2_auth_combobox_ = new views::Combobox(
        phase_2_auth_combobox_model_.get());
    phase_2_auth_combobox_->SetAccessibleName(phase_2_label_text);
    phase_2_auth_label_->SetEnabled(false);
    phase_2_auth_combobox_->SetEnabled(false);
    phase_2_auth_combobox_->set_listener(this);
    if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
      layout->AddView(phase_2_auth_combobox_, 1, 1, views::GridLayout::FILL,
                      views::GridLayout::FILL, 0,
                      ChildNetworkConfigView::kInputFieldHeight);
    } else {
      layout->AddView(phase_2_auth_combobox_);
    }
    layout->AddView(new ControlledSettingIndicatorView(phase_2_auth_ui_data_));
    layout->AddPaddingRow(0, provider->GetDistanceMetric(
                                 views::DISTANCE_RELATED_CONTROL_VERTICAL));

    // Server CA certificate
    layout->StartRow(0, column_view_set_id);
    base::string16 server_ca_cert_label_text = l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_CERT_SERVER_CA);
    server_ca_cert_label_ = new views::Label(server_ca_cert_label_text);
    layout->AddView(server_ca_cert_label_);
    server_ca_cert_combobox_model_.reset(
        new internal::ServerCACertComboboxModel());
    server_ca_cert_combobox_ = new ComboboxWithWidth(
        server_ca_cert_combobox_model_.get(),
        ChildNetworkConfigView::kInputFieldMinWidth);
    server_ca_cert_combobox_->SetAccessibleName(server_ca_cert_label_text);
    server_ca_cert_label_->SetEnabled(false);
    server_ca_cert_combobox_->SetEnabled(false);
    server_ca_cert_combobox_->set_listener(this);
    if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
      layout->AddView(server_ca_cert_combobox_, 1, 1, views::GridLayout::FILL,
                      views::GridLayout::FILL, 0,
                      ChildNetworkConfigView::kInputFieldHeight);
    } else {
      layout->AddView(server_ca_cert_combobox_);
    }
    layout->AddView(
        new ControlledSettingIndicatorView(server_ca_cert_ui_data_));
    layout->AddPaddingRow(0, provider->GetDistanceMetric(
                                 views::DISTANCE_RELATED_CONTROL_VERTICAL));

    // Subject Match
    layout->StartRow(0, column_view_set_id);
    base::string16 subject_match_label_text = l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_EAP_SUBJECT_MATCH);
    subject_match_label_ = new views::Label(subject_match_label_text);
    layout->AddView(subject_match_label_);
    subject_match_textfield_ = new views::Textfield();
    subject_match_textfield_->SetAccessibleName(subject_match_label_text);
    subject_match_textfield_->set_controller(this);
    if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
      layout->AddView(subject_match_textfield_, 1, 1, views::GridLayout::FILL,
                      views::GridLayout::FILL, 0,
                      ChildNetworkConfigView::kInputFieldHeight);
    } else {
      layout->AddView(subject_match_textfield_);
    }
    layout->AddPaddingRow(0, provider->GetDistanceMetric(
                                 views::DISTANCE_RELATED_CONTROL_VERTICAL));

    // User certificate
    managed_user_cert_not_found_ = false;
    layout->StartRow(0, column_view_set_id);
    base::string16 user_cert_label_text = l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_CERT);
    user_cert_label_ = new views::Label(user_cert_label_text);
    layout->AddView(user_cert_label_);
    user_cert_combobox_model_.reset(new internal::UserCertComboboxModel(this));
    user_cert_combobox_ = new views::Combobox(user_cert_combobox_model_.get());
    user_cert_combobox_->SetAccessibleName(user_cert_label_text);
    user_cert_label_->SetEnabled(false);
    user_cert_combobox_->SetEnabled(false);
    user_cert_combobox_->set_listener(this);
    if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
      layout->AddView(user_cert_combobox_, 1, 1, views::GridLayout::FILL,
                      views::GridLayout::FILL, 0,
                      ChildNetworkConfigView::kInputFieldHeight);
    } else {
      layout->AddView(user_cert_combobox_);
    }
    layout->AddView(new ControlledSettingIndicatorView(user_cert_ui_data_));
    layout->AddPaddingRow(0, provider->GetDistanceMetric(
                                 views::DISTANCE_RELATED_CONTROL_VERTICAL));

    // Identity
    layout->StartRow(0, column_view_set_id);
    base::string16 identity_label_text = l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_CERT_IDENTITY);
    identity_label_ = new views::Label(identity_label_text);
    layout->AddView(identity_label_);
    identity_textfield_ = new views::Textfield();
    identity_textfield_->SetAccessibleName(identity_label_text);
    identity_textfield_->set_controller(this);
    identity_textfield_->SetEnabled(identity_ui_data_.IsEditable());
    if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
      layout->AddView(identity_textfield_, 1, 1, views::GridLayout::FILL,
                      views::GridLayout::FILL, 0,
                      ChildNetworkConfigView::kInputFieldHeight);
    } else {
      layout->AddView(identity_textfield_);
    }
    layout->AddView(new ControlledSettingIndicatorView(identity_ui_data_));
    layout->AddPaddingRow(0, provider->GetDistanceMetric(
                                 views::DISTANCE_RELATED_CONTROL_VERTICAL));
  }

  // Passphrase input
  layout->StartRow(0, column_view_set_id);
  int label_text_id = IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PASSPHRASE;
  base::string16 passphrase_label_text =
      l10n_util::GetStringUTF16(label_text_id);
  passphrase_label_ = new views::Label(passphrase_label_text);
  layout->AddView(passphrase_label_);
  passphrase_textfield_ = new PassphraseTextfield();
  passphrase_textfield_->set_controller(this);
  // Disable passphrase input initially for other network.
  passphrase_label_->SetEnabled(network);
  passphrase_textfield_->SetEnabled(network &&
                                    passphrase_ui_data_.IsEditable());
  passphrase_textfield_->SetAccessibleName(passphrase_label_text);
  if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
    layout->AddView(passphrase_textfield_, 1, 1, views::GridLayout::FILL,
                    views::GridLayout::FILL, 0,
                    ChildNetworkConfigView::kInputFieldHeight);
  } else {
    layout->AddView(passphrase_textfield_);
  }

  if (passphrase_ui_data_.IsManaged()) {
    layout->AddView(new ControlledSettingIndicatorView(passphrase_ui_data_));
  } else {
    // Password visible button.
    passphrase_visible_button_ = new views::ToggleImageButton(this);
    passphrase_visible_button_->SetFocusForPlatform();
    passphrase_visible_button_->set_request_focus_on_press(true);
    passphrase_visible_button_->SetTooltipText(
        l10n_util::GetStringUTF16(
            IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PASSPHRASE_SHOW));
    passphrase_visible_button_->SetToggledTooltipText(
        l10n_util::GetStringUTF16(
            IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PASSPHRASE_HIDE));
    passphrase_visible_button_->SetImage(
        views::ImageButton::STATE_NORMAL,
        *ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
            IDR_SHOW_PASSWORD));
    passphrase_visible_button_->SetImage(
        views::ImageButton::STATE_HOVERED,
        *ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
            IDR_SHOW_PASSWORD_HOVER));
    passphrase_visible_button_->SetToggledImage(
        views::ImageButton::STATE_NORMAL,
        ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
            IDR_HIDE_PASSWORD));
    passphrase_visible_button_->SetToggledImage(
        views::ImageButton::STATE_HOVERED,
        ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
            IDR_HIDE_PASSWORD_HOVER));
    passphrase_visible_button_->SetImageAlignment(
        views::ImageButton::ALIGN_CENTER, views::ImageButton::ALIGN_MIDDLE);
    layout->AddView(passphrase_visible_button_);
  }

  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));

  if (show_8021x) {
    // Anonymous identity
    layout->StartRow(0, column_view_set_id);
    identity_anonymous_label_ =
        new views::Label(l10n_util::GetStringUTF16(
            IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_CERT_IDENTITY_ANONYMOUS));
    layout->AddView(identity_anonymous_label_);
    identity_anonymous_textfield_ = new views::Textfield();
    identity_anonymous_label_->SetEnabled(false);
    identity_anonymous_textfield_->SetEnabled(false);
    identity_anonymous_textfield_->set_controller(this);
    if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
      layout->AddView(identity_anonymous_textfield_, 1, 1,
                      views::GridLayout::FILL, views::GridLayout::FILL, 0,
                      ChildNetworkConfigView::kInputFieldHeight);
    } else {
      layout->AddView(identity_anonymous_textfield_);
    }
    layout->AddView(
        new ControlledSettingIndicatorView(identity_anonymous_ui_data_));
    layout->AddPaddingRow(0, provider->GetDistanceMetric(
                                 views::DISTANCE_RELATED_CONTROL_VERTICAL));
  }

  if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
    // We need a little bit more padding above Checkboxes.
    layout->AddPaddingRow(0, provider->GetDistanceMetric(
                                 views::DISTANCE_RELATED_CONTROL_VERTICAL));
  }

  // Checkboxes.

  // Save credentials
  if (show_8021x) {
    layout->StartRow(0, column_view_set_id);
    save_credentials_checkbox_ = new views::Checkbox(
        l10n_util::GetStringUTF16(
            IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_SAVE_CREDENTIALS));
    save_credentials_checkbox_->SetEnabled(
        save_credentials_ui_data_.IsEditable());
    layout->SkipColumns(1);
    layout->AddView(save_credentials_checkbox_);
    layout->AddView(
        new ControlledSettingIndicatorView(save_credentials_ui_data_));
  }

  // Share network
  if (!network || network->profile_path().empty()) {
    layout->StartRow(0, column_view_set_id);
    share_network_checkbox_ = new views::Checkbox(
        l10n_util::GetStringUTF16(
            IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_SHARE_NETWORK));
    layout->SkipColumns(1);
    layout->AddView(share_network_checkbox_);
  }
  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));

  // Create an error label.
  layout->StartRow(0, column_view_set_id);
  layout->SkipColumns(1);
  error_label_ = new views::Label();
  error_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  error_label_->SetEnabledColor(SK_ColorRED);
  layout->AddView(error_label_);

  // Initialize the field and checkbox values.

  if (!network && show_8021x)
    RefreshEapFields();

  RefreshShareCheckbox();
  UpdateErrorLabel();

  if (network) {
    NetworkHandler::Get()->network_configuration_handler()->GetShillProperties(
        service_path_, base::Bind(&WifiConfigView::InitFromProperties,
                                  weak_ptr_factory_.GetWeakPtr(), show_8021x),
        base::Bind(&ShillError, "GetProperties"));
  }
}

void WifiConfigView::InitFromProperties(
    bool show_8021x,
    const std::string& service_path,
    const base::DictionaryValue& properties) {
  if (!show_8021x) {
    std::string passphrase;
    properties.GetStringWithoutPathExpansion(
        shill::kPassphraseProperty, &passphrase);
    passphrase_textfield_->SetText(base::UTF8ToUTF16(passphrase));
    return;
  }

  // EAP Method
  std::string eap_method;
  properties.GetStringWithoutPathExpansion(
      shill::kEapMethodProperty, &eap_method);
  if (eap_method == shill::kEapMethodPEAP)
    eap_method_combobox_->SetSelectedIndex(EAP_METHOD_INDEX_PEAP);
  else if (eap_method == shill::kEapMethodTTLS)
    eap_method_combobox_->SetSelectedIndex(EAP_METHOD_INDEX_TTLS);
  else if (eap_method == shill::kEapMethodTLS)
    eap_method_combobox_->SetSelectedIndex(EAP_METHOD_INDEX_TLS);
  else if (eap_method == shill::kEapMethodLEAP)
    eap_method_combobox_->SetSelectedIndex(EAP_METHOD_INDEX_LEAP);
  RefreshEapFields();

  // Phase 2 authentication and anonymous identity.
  if (Phase2AuthActive()) {
    std::string eap_phase_2_auth;
    properties.GetStringWithoutPathExpansion(
        shill::kEapPhase2AuthProperty, &eap_phase_2_auth);
    if (eap_phase_2_auth == shill::kEapPhase2AuthTTLSMD5)
      phase_2_auth_combobox_->SetSelectedIndex(PHASE_2_AUTH_INDEX_MD5);
    else if (eap_phase_2_auth == shill::kEapPhase2AuthTTLSMSCHAPV2)
      phase_2_auth_combobox_->SetSelectedIndex(PHASE_2_AUTH_INDEX_MSCHAPV2);
    else if (eap_phase_2_auth == shill::kEapPhase2AuthTTLSMSCHAP)
      phase_2_auth_combobox_->SetSelectedIndex(PHASE_2_AUTH_INDEX_MSCHAP);
    else if (eap_phase_2_auth == shill::kEapPhase2AuthTTLSPAP)
      phase_2_auth_combobox_->SetSelectedIndex(PHASE_2_AUTH_INDEX_PAP);
    else if (eap_phase_2_auth == shill::kEapPhase2AuthTTLSCHAP)
      phase_2_auth_combobox_->SetSelectedIndex(PHASE_2_AUTH_INDEX_CHAP);

    std::string eap_anonymous_identity;
    properties.GetStringWithoutPathExpansion(
        shill::kEapAnonymousIdentityProperty, &eap_anonymous_identity);
    identity_anonymous_textfield_->SetText(
        base::UTF8ToUTF16(eap_anonymous_identity));
  }

  // Subject match
  std::string subject_match;
  properties.GetStringWithoutPathExpansion(
      shill::kEapSubjectMatchProperty, &subject_match);
  subject_match_textfield_->SetText(base::UTF8ToUTF16(subject_match));

  // Server CA certificate.
  if (CaCertActive()) {
    std::string eap_ca_cert_pem;
    const base::ListValue* pems = NULL;
    if (properties.GetListWithoutPathExpansion(
            shill::kEapCaCertPemProperty, &pems))
      pems->GetString(0, &eap_ca_cert_pem);
    if (eap_ca_cert_pem.empty()) {
      bool eap_use_system_cas = false;
      properties.GetBooleanWithoutPathExpansion(
          shill::kEapUseSystemCasProperty, &eap_use_system_cas);
      if (eap_use_system_cas) {
        // "Default"
        server_ca_cert_combobox_->SetSelectedIndex(0);
      } else {
        // "Do not check".
        server_ca_cert_combobox_->SetSelectedIndex(
            server_ca_cert_combobox_->model()->GetItemCount() - 1);
      }
    } else {
      // Select the certificate if available.
      int cert_index =
          CertLibrary::Get()->GetServerCACertIndexByPEM(eap_ca_cert_pem);
      if (cert_index >= 0) {
        // Skip item for "Default".
        server_ca_cert_combobox_->SetSelectedIndex(1 + cert_index);
      } else {
        // "Default"
        server_ca_cert_combobox_->SetSelectedIndex(0);
      }
    }
  }

  // User certificate.
  if (UserCertActive()) {
    std::string eap_cert_id;
    properties.GetStringWithoutPathExpansion(
        shill::kEapCertIdProperty, &eap_cert_id);
    int unused_slot_id = 0;
    std::string pkcs11_id = client_cert::GetPkcs11AndSlotIdFromEapCertId(
        eap_cert_id, &unused_slot_id);
    if (!pkcs11_id.empty()) {
      int cert_index =
          CertLibrary::Get()->GetUserCertIndexByPkcs11Id(pkcs11_id);
      if (cert_index >= 0)
        user_cert_combobox_->SetSelectedIndex(cert_index);
    } else if (!user_cert_ui_data_.IsEditable() &&
               CertLibrary::Get()->NumCertificates(
                   CertLibrary::CERT_TYPE_USER) > 0) {
      // The cert is not configured (e.g. policy-provided client cert pattern
      // did not match anything), and the cert selection is not editable
      // (probably because the network is configured by policy). In this case,
      // we don't want to display the first certificate in the list, as that
      // would be misleading.
      managed_user_cert_not_found_ = true;
      user_cert_combobox_->ModelChanged();
      user_cert_combobox_->SetSelectedIndex(0);
    }
  }

  // Identity is always active.
  std::string eap_identity;
  properties.GetStringWithoutPathExpansion(
      shill::kEapIdentityProperty, &eap_identity);
  identity_textfield_->SetText(base::UTF8ToUTF16(eap_identity));

  // Passphrase
  if (PassphraseActive()) {
    std::string eap_password;
    properties.GetStringWithoutPathExpansion(
        shill::kEapPasswordProperty, &eap_password);
    passphrase_textfield_->SetText(base::UTF8ToUTF16(eap_password));
    // If 'Connectable' is True, show a fake passphrase to indicate that it
    // has already been set.
    bool connectable = false;
    properties.GetBooleanWithoutPathExpansion(
        shill::kConnectableProperty, &connectable);
    passphrase_textfield_->SetShowFake(connectable);
  }

  // Save credentials
  bool save_credentials = false;
  properties.GetBooleanWithoutPathExpansion(
      shill::kSaveCredentialsProperty, &save_credentials);
  save_credentials_checkbox_->SetChecked(save_credentials);

  UpdateDialogButtons();
  RefreshShareCheckbox();
  UpdateErrorLabel();
}

void WifiConfigView::InitFocus() {
  views::View* view_to_focus = GetInitiallyFocusedView();
  if (view_to_focus)
    view_to_focus->RequestFocus();
}

bool WifiConfigView::IsConfigureDialog() {
  const NetworkState* network = GetNetworkState();
  return network && network->type() == shill::kTypeEthernet;
}

void WifiConfigView::NetworkPropertiesUpdated(const NetworkState* network) {
  if (network->path() != service_path_)
    return;
  UpdateErrorLabel();
}

const NetworkState* WifiConfigView::GetNetworkState() const {
  if (service_path_.empty())
    return NULL;
  return NetworkHandler::Get()->network_state_handler()->GetNetworkState(
      service_path_);
}

// static
void WifiConfigView::ParseUIProperty(NetworkPropertyUIData* property_ui_data,
                                     const NetworkState* network,
                                     const std::string& key) {
  ::onc::ONCSource onc_source = ::onc::ONC_SOURCE_NONE;
  const base::DictionaryValue* onc =
      onc::FindPolicyForActiveUser(network->guid(), &onc_source);
  std::string onc_tag = network->type() == shill::kTypeEthernet
                            ? ::onc::network_config::kEthernet
                            : ::onc::network_config::kWiFi;
  property_ui_data->ParseOncProperty(onc_source, onc, onc_tag + '.' + key);
}

// static
void WifiConfigView::ParseEAPUIProperty(NetworkPropertyUIData* property_ui_data,
                                        const NetworkState* network,
                                        const std::string& key) {
  std::string onc_tag = network->type() == shill::kTypeEthernet
                            ? ::onc::ethernet::kEAP
                            : ::onc::wifi::kEAP;
  ParseUIProperty(property_ui_data, network, onc_tag + '.' + key);
}

}  // namespace chromeos
