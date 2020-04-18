// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/options/vpn_config_view.h"

#include <stddef.h>

#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/net/shill_error.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/ash/network/enrollment_dialog_view.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/login/login_state.h"
#include "chromeos/network/network_configuration_handler.h"
#include "chromeos/network/network_connect.h"
#include "chromeos/network/network_event_log.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_ui_data.h"
#include "chromeos/network/onc/onc_utils.h"
#include "components/onc/onc_constants.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/combobox_model.h"
#include "ui/events/event.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"

namespace {

enum ProviderTypeIndex {
  PROVIDER_TYPE_INDEX_L2TP_IPSEC_PSK = 0,
  PROVIDER_TYPE_INDEX_L2TP_IPSEC_USER_CERT = 1,
  PROVIDER_TYPE_INDEX_OPEN_VPN = 2,
  PROVIDER_TYPE_INDEX_MAX = 3,
};

base::string16 ProviderTypeIndexToString(int index) {
  switch (index) {
    case PROVIDER_TYPE_INDEX_L2TP_IPSEC_PSK:
      return l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_L2TP_IPSEC_PSK);
    case PROVIDER_TYPE_INDEX_L2TP_IPSEC_USER_CERT:
      return l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_L2TP_IPSEC_USER_CERT);
    case PROVIDER_TYPE_INDEX_OPEN_VPN:
      return l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_OPEN_VPN);
  }
  NOTREACHED();
  return base::string16();
}

int ProviderTypeToIndex(const std::string& provider_type,
                        const std::string& client_cert_id) {
  if (provider_type == shill::kProviderL2tpIpsec) {
    if (!client_cert_id.empty())
      return PROVIDER_TYPE_INDEX_L2TP_IPSEC_USER_CERT;
    else
      return PROVIDER_TYPE_INDEX_L2TP_IPSEC_PSK;
  } else {
    DCHECK(provider_type == shill::kProviderOpenVpn);
    return PROVIDER_TYPE_INDEX_OPEN_VPN;
  }
}

// Translates the provider type to the name of the respective ONC dictionary
// containing configuration data for the type.
std::string ProviderTypeIndexToONCDictKey(int provider_type_index) {
  switch (provider_type_index) {
    case PROVIDER_TYPE_INDEX_L2TP_IPSEC_PSK:
    case PROVIDER_TYPE_INDEX_L2TP_IPSEC_USER_CERT:
      return onc::vpn::kIPsec;
    case PROVIDER_TYPE_INDEX_OPEN_VPN:
      return onc::vpn::kOpenVPN;
  }
  NOTREACHED() << "Unhandled provider type index " << provider_type_index;
  return std::string();
}

std::string GetPemFromDictionary(
    const base::DictionaryValue* provider_properties,
    const std::string& key) {
  const base::ListValue* pems = NULL;
  if (!provider_properties->GetListWithoutPathExpansion(key, &pems))
    return std::string();
  std::string pem;
  pems->GetString(0, &pem);
  return pem;
}

}  // namespace

namespace chromeos {

namespace internal {

class ProviderTypeComboboxModel : public ui::ComboboxModel {
 public:
  ProviderTypeComboboxModel();
  ~ProviderTypeComboboxModel() override;

  // Overridden from ui::ComboboxModel:
  int GetItemCount() const override;
  base::string16 GetItemAt(int index) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProviderTypeComboboxModel);
};

class VpnServerCACertComboboxModel : public ui::ComboboxModel {
 public:
  VpnServerCACertComboboxModel();
  ~VpnServerCACertComboboxModel() override;

  // Overridden from ui::ComboboxModel:
  int GetItemCount() const override;
  base::string16 GetItemAt(int index) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(VpnServerCACertComboboxModel);
};

class VpnUserCertComboboxModel : public ui::ComboboxModel {
 public:
  VpnUserCertComboboxModel();
  ~VpnUserCertComboboxModel() override;

  // Overridden from ui::ComboboxModel:
  int GetItemCount() const override;
  base::string16 GetItemAt(int index) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(VpnUserCertComboboxModel);
};

// ProviderTypeComboboxModel ---------------------------------------------------

ProviderTypeComboboxModel::ProviderTypeComboboxModel() {
}

ProviderTypeComboboxModel::~ProviderTypeComboboxModel() {
}

int ProviderTypeComboboxModel::GetItemCount() const {
  return PROVIDER_TYPE_INDEX_MAX;
}

base::string16 ProviderTypeComboboxModel::GetItemAt(int index) {
  return ProviderTypeIndexToString(index);
}

// VpnServerCACertComboboxModel ------------------------------------------------

VpnServerCACertComboboxModel::VpnServerCACertComboboxModel() {
}

VpnServerCACertComboboxModel::~VpnServerCACertComboboxModel() {
}

int VpnServerCACertComboboxModel::GetItemCount() const {
  if (CertLibrary::Get()->CertificatesLoading())
    return 1;  // "Loading"
  // "Default" + certs.
  return CertLibrary::Get()->NumCertificates(
      CertLibrary::CERT_TYPE_SERVER_CA) + 1;
}

base::string16 VpnServerCACertComboboxModel::GetItemAt(int index) {
  if (CertLibrary::Get()->CertificatesLoading())
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_CERT_LOADING);
  if (index == 0)
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_CERT_SERVER_CA_DEFAULT);
  int cert_index = index - 1;
  return CertLibrary::Get()->GetCertDisplayStringAt(
      CertLibrary::CERT_TYPE_SERVER_CA, cert_index);
}

// VpnUserCertComboboxModel ----------------------------------------------------

VpnUserCertComboboxModel::VpnUserCertComboboxModel() {
}

VpnUserCertComboboxModel::~VpnUserCertComboboxModel() {
}

int VpnUserCertComboboxModel::GetItemCount() const {
  if (CertLibrary::Get()->CertificatesLoading())
    return 1;  // "Loading"
  int num_certs =
      CertLibrary::Get()->NumCertificates(CertLibrary::CERT_TYPE_USER);
  if (num_certs == 0)
    return 1;  // "None installed"
  return num_certs;
}

base::string16 VpnUserCertComboboxModel::GetItemAt(int index) {
  if (CertLibrary::Get()->CertificatesLoading()) {
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_CERT_LOADING);
  }
  if (CertLibrary::Get()->NumCertificates(CertLibrary::CERT_TYPE_USER) == 0) {
    return l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_USER_CERT_NONE_INSTALLED);
  }
  return CertLibrary::Get()->GetCertDisplayStringAt(
      CertLibrary::CERT_TYPE_USER, index);
}

}  // namespace internal

VPNConfigView::VPNConfigView(NetworkConfigView* parent,
                             const std::string& service_path)
    : ChildNetworkConfigView(parent, service_path),
      service_text_modified_(false),
      enable_psk_passphrase_(false),
      enable_user_cert_(false),
      enable_server_ca_cert_(false),
      enable_otp_(false),
      enable_group_name_(false),
      user_passphrase_required_(false),
      title_(0),
      server_textfield_(NULL),
      service_text_(NULL),
      service_textfield_(NULL),
      provider_type_combobox_(NULL),
      provider_type_text_label_(NULL),
      psk_passphrase_label_(NULL),
      psk_passphrase_textfield_(NULL),
      user_cert_label_(NULL),
      user_cert_combobox_(NULL),
      server_ca_cert_label_(NULL),
      server_ca_cert_combobox_(NULL),
      username_textfield_(NULL),
      user_passphrase_textfield_(NULL),
      otp_label_(NULL),
      otp_textfield_(NULL),
      group_name_label_(NULL),
      group_name_textfield_(NULL),
      save_credentials_checkbox_(NULL),
      error_label_(NULL),
      provider_type_index_(PROVIDER_TYPE_INDEX_MAX),
      weak_ptr_factory_(this) {
  Init();
}

VPNConfigView::~VPNConfigView() {
  RemoveAllChildViews(true);  // Destroy children before models
  CertLibrary::Get()->RemoveObserver(this);
}

base::string16 VPNConfigView::GetTitle() const {
  DCHECK_NE(title_, 0);
  return l10n_util::GetStringUTF16(title_);
}

views::View* VPNConfigView::GetInitiallyFocusedView() {
  if (service_path_.empty()) {
    // Put focus in the first editable field.
    if (server_textfield_)
      return server_textfield_;
    else if (service_textfield_)
      return service_textfield_;
    else if (provider_type_combobox_)
      return provider_type_combobox_;
    else if (psk_passphrase_textfield_ && psk_passphrase_textfield_->enabled())
      return psk_passphrase_textfield_;
    else if (user_cert_combobox_ && user_cert_combobox_->enabled())
      return user_cert_combobox_;
    else if (server_ca_cert_combobox_ && server_ca_cert_combobox_->enabled())
      return server_ca_cert_combobox_;
  }
  if (user_passphrase_textfield_ && user_passphrase_required_)
    return user_passphrase_textfield_;
  else if (otp_textfield_)
    return otp_textfield_;
  return NULL;
}

bool VPNConfigView::CanLogin() {
  // Username is always required.
  if (GetUsername().empty())
    return false;

  // TODO(stevenjb): min kMinPassphraseLen length?
  if (service_path_.empty() &&
      (GetService().empty() || GetServer().empty()))
    return false;

  // Block login if certs are required but user has none.
  bool cert_required =
      GetProviderTypeIndex() == PROVIDER_TYPE_INDEX_L2TP_IPSEC_USER_CERT;
  if (cert_required && (!HaveUserCerts() || !IsUserCertValid()))
    return false;

  return true;
}

void VPNConfigView::ContentsChanged(views::Textfield* sender,
                                    const base::string16& new_contents) {
  if (sender == server_textfield_ && !service_text_modified_) {
    // Set the service name to the server name up to '.', unless it has
    // been explicitly set by the user.
    base::string16 server = server_textfield_->text();
    base::string16::size_type n = server.find_first_of(L'.');
    service_name_from_server_ = server.substr(0, n);
    service_textfield_->SetText(service_name_from_server_);
  }
  if (sender == service_textfield_) {
    if (new_contents.empty())
      service_text_modified_ = false;
    else if (new_contents != service_name_from_server_)
      service_text_modified_ = true;
  }
  UpdateCanLogin();
}

bool VPNConfigView::HandleKeyEvent(views::Textfield* sender,
                                   const ui::KeyEvent& key_event) {
  if ((sender == psk_passphrase_textfield_ ||
       sender == user_passphrase_textfield_) &&
      key_event.type() == ui::ET_KEY_PRESSED &&
      key_event.key_code() == ui::VKEY_RETURN) {
    parent_->GetDialogClientView()->AcceptWindow();
  }
  return false;
}

void VPNConfigView::ButtonPressed(views::Button* sender,
                                  const ui::Event& event) {
}

void VPNConfigView::OnPerformAction(views::Combobox* combobox) {
  UpdateControls();
  UpdateErrorLabel();
  UpdateCanLogin();
}

void VPNConfigView::OnCertificatesLoaded() {
  Refresh();
}

bool VPNConfigView::Login() {
  if (service_path_.empty()) {
    base::DictionaryValue properties;
    // Identifying properties
    properties.SetKey(shill::kTypeProperty, base::Value(shill::kTypeVPN));
    properties.SetKey(shill::kNameProperty, base::Value(GetService()));
    properties.SetKey(shill::kProviderHostProperty, base::Value(GetServer()));
    properties.SetKey(shill::kProviderTypeProperty,
                      base::Value(GetProviderTypeString()));

    SetConfigProperties(&properties);
    bool shared = false;
    bool modifiable = false;
    ChildNetworkConfigView::GetShareStateForLoginState(&shared, &modifiable);

    bool only_policy_autoconnect =
        onc::PolicyAllowsOnlyPolicyNetworksToAutoconnect(!shared);
    if (only_policy_autoconnect) {
      properties.SetKey(shill::kAutoConnectProperty, base::Value(false));
    }

    NetworkConnect::Get()->CreateConfigurationAndConnect(&properties, shared);
  } else {
    const NetworkState* vpn = NetworkHandler::Get()->network_state_handler()->
        GetNetworkState(service_path_);
    if (!vpn) {
      // Shill no longer knows about this network (edge case).
      // TODO(stevenjb): Add notification for this.
      NET_LOG_ERROR("Network not found", service_path_);
      return true;  // Close dialog
    }
    base::DictionaryValue properties;
    SetConfigProperties(&properties);
    NetworkConnect::Get()->ConfigureNetworkIdAndConnect(vpn->guid(), properties,
                                                        false /* not shared */);
  }
  return true;  // Close dialog.
}

void VPNConfigView::Cancel() {
}

void VPNConfigView::InitFocus() {
  views::View* view_to_focus = GetInitiallyFocusedView();
  if (view_to_focus)
    view_to_focus->RequestFocus();
}

std::string VPNConfigView::GetService() const {
  if (service_textfield_ != NULL)
    return GetTextFromField(service_textfield_, true);
  return service_path_;
}

std::string VPNConfigView::GetServer() const {
  if (server_textfield_ != NULL)
    return GetTextFromField(server_textfield_, true);
  return std::string();
}

std::string VPNConfigView::GetPSKPassphrase() const {
  if (psk_passphrase_textfield_ &&
      enable_psk_passphrase_ &&
      psk_passphrase_textfield_->visible())
    return GetPassphraseFromField(psk_passphrase_textfield_);
  return std::string();
}

std::string VPNConfigView::GetUsername() const {
  return GetTextFromField(username_textfield_, true);
}

std::string VPNConfigView::GetUserPassphrase() const {
  return GetPassphraseFromField(user_passphrase_textfield_);
}

std::string VPNConfigView::GetGroupName() const {
  return GetTextFromField(group_name_textfield_, false);
}

std::string VPNConfigView::GetOTP() const {
  return GetTextFromField(otp_textfield_, true);
}

std::string VPNConfigView::GetServerCACertPEM() const {
  int index = server_ca_cert_combobox_ ?
      server_ca_cert_combobox_->selected_index() : 0;
  if (index == 0) {
    // First item is "Default".
    return std::string();
  } else {
    int cert_index = index - 1;
    return CertLibrary::Get()->GetServerCACertPEMAt(cert_index);
  }
}

void VPNConfigView::SetUserCertProperties(
    chromeos::client_cert::ConfigType client_cert_type,
    base::DictionaryValue* properties) const {
  if (!HaveUserCerts()) {
    // No certificate selected or not required.
    chromeos::client_cert::SetEmptyShillProperties(client_cert_type,
                                                   properties);
  } else {
    // Certificates are listed in the order they appear in the model.
    int index = user_cert_combobox_ ? user_cert_combobox_->selected_index() : 0;
    int slot_id = -1;
    const std::string pkcs11_id =
        CertLibrary::Get()->GetUserCertPkcs11IdAt(index, &slot_id);
    chromeos::client_cert::SetShillProperties(
        client_cert_type, slot_id, pkcs11_id, properties);
  }
}

bool VPNConfigView::GetSaveCredentials() const {
  return save_credentials_checkbox_->checked();
}

int VPNConfigView::GetProviderTypeIndex() const {
  if (provider_type_combobox_)
    return provider_type_combobox_->selected_index();
  return provider_type_index_;
}

std::string VPNConfigView::GetProviderTypeString() const {
  int index = GetProviderTypeIndex();
  switch (index) {
    case PROVIDER_TYPE_INDEX_L2TP_IPSEC_PSK:
    case PROVIDER_TYPE_INDEX_L2TP_IPSEC_USER_CERT:
      return shill::kProviderL2tpIpsec;
    case PROVIDER_TYPE_INDEX_OPEN_VPN:
      return shill::kProviderOpenVpn;
  }
  NOTREACHED();
  return std::string();
}

void VPNConfigView::Init() {
  const views::LayoutProvider* provider = views::LayoutProvider::Get();
  SetBorder(views::CreateEmptyBorder(
      provider->GetDialogInsetsForContentType(views::CONTROL, views::TEXT)));

  const NetworkState* vpn = NULL;
  if (!service_path_.empty()) {
    vpn = NetworkHandler::Get()->network_state_handler()->
        GetNetworkState(service_path_);
    DCHECK(vpn && vpn->type() == shill::kTypeVPN);
  }

  views::GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));

  // Observer any changes to the certificate list.
  CertLibrary::Get()->AddObserver(this);

  views::ColumnSet* column_set = layout->AddColumnSet(0);
  // Label.
  column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::FILL, 1,
                        views::GridLayout::USE_PREF, 0, 0);
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
  // Policy indicator.
  column_set->AddColumn(views::GridLayout::CENTER, views::GridLayout::CENTER, 0,
                        views::GridLayout::USE_PREF, 0, 0);

  // Initialize members.
  service_text_modified_ = false;
  title_ = vpn ? IDS_OPTIONS_SETTINGS_JOIN_VPN : IDS_OPTIONS_SETTINGS_ADD_VPN;

  // By default enable all controls.
  enable_psk_passphrase_ = true;
  enable_user_cert_ = true;
  enable_server_ca_cert_ = true;
  enable_otp_ = true;
  enable_group_name_ = true;

  // Server label and input.
  layout->StartRow(0, 0);
  views::View* server_label =
      new views::Label(l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_VPN_SERVER_HOSTNAME));
  layout->AddView(server_label);
  server_textfield_ = new views::Textfield();
  server_textfield_->set_controller(this);
  layout->AddView(server_textfield_);
  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));
  if (!service_path_.empty()) {
    server_label->SetEnabled(false);
    server_textfield_->SetEnabled(false);
  }

  // Service label and name or input.
  layout->StartRow(0, 0);
  layout->AddView(new views::Label(l10n_util::GetStringUTF16(
      IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_VPN_SERVICE_NAME)));
  if (service_path_.empty()) {
    service_textfield_ = new views::Textfield();
    service_textfield_->set_controller(this);
    layout->AddView(service_textfield_);
    service_text_ = NULL;
  } else {
    service_text_ = new views::Label();
    service_text_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    layout->AddView(service_text_);
    service_textfield_ = NULL;
  }
  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));

  // Provider type label and select.
  layout->StartRow(0, 0);
  layout->AddView(new views::Label(l10n_util::GetStringUTF16(
      IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_VPN_PROVIDER_TYPE)));
  if (service_path_.empty()) {
    provider_type_combobox_model_.reset(
        new internal::ProviderTypeComboboxModel);
    provider_type_combobox_ = new views::Combobox(
        provider_type_combobox_model_.get());
    provider_type_combobox_->set_listener(this);
    layout->AddView(provider_type_combobox_);
    provider_type_text_label_ = NULL;
  } else {
    provider_type_text_label_ = new views::Label();
    provider_type_text_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    layout->AddView(provider_type_text_label_);
    provider_type_combobox_ = NULL;
  }
  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));

  // PSK passphrase label, input and visible button.
  layout->StartRow(0, 0);
  psk_passphrase_label_ =  new views::Label(l10n_util::GetStringUTF16(
      IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_VPN_PSK_PASSPHRASE));
  layout->AddView(psk_passphrase_label_);
  psk_passphrase_textfield_ = new PassphraseTextfield();
  psk_passphrase_textfield_->set_controller(this);
  layout->AddView(psk_passphrase_textfield_);
  layout->AddView(new ControlledSettingIndicatorView(psk_passphrase_ui_data_));
  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));

  // Server CA certificate
  if (service_path_.empty()) {
    layout->StartRow(0, 0);
    server_ca_cert_label_ = new views::Label(l10n_util::GetStringUTF16(
        IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_CERT_SERVER_CA));
    layout->AddView(server_ca_cert_label_);
    server_ca_cert_combobox_model_.reset(
        new internal::VpnServerCACertComboboxModel());
    server_ca_cert_combobox_ = new views::Combobox(
        server_ca_cert_combobox_model_.get());
    layout->AddView(server_ca_cert_combobox_);
    layout->AddView(new ControlledSettingIndicatorView(ca_cert_ui_data_));
    layout->AddPaddingRow(0, provider->GetDistanceMetric(
                                 views::DISTANCE_RELATED_CONTROL_VERTICAL));
  } else {
    server_ca_cert_label_ = NULL;
    server_ca_cert_combobox_ = NULL;
  }

  // User certificate label and input.
  layout->StartRow(0, 0);
  user_cert_label_ = new views::Label(l10n_util::GetStringUTF16(
      IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_VPN_USER_CERT));
  layout->AddView(user_cert_label_);
  user_cert_combobox_model_.reset(
      new internal::VpnUserCertComboboxModel());
  user_cert_combobox_ = new views::Combobox(user_cert_combobox_model_.get());
  user_cert_combobox_->set_listener(this);
  layout->AddView(user_cert_combobox_);
  layout->AddView(new ControlledSettingIndicatorView(user_cert_ui_data_));
  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));

  // Username label and input.
  layout->StartRow(0, 0);
  layout->AddView(new views::Label(l10n_util::GetStringUTF16(
      IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_VPN_USERNAME)));
  username_textfield_ = new views::Textfield();
  username_textfield_->set_controller(this);
  username_textfield_->SetEnabled(username_ui_data_.IsEditable());
  layout->AddView(username_textfield_);
  layout->AddView(new ControlledSettingIndicatorView(username_ui_data_));
  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));

  // User passphrase label, input and visble button.
  layout->StartRow(0, 0);
  layout->AddView(new views::Label(l10n_util::GetStringUTF16(
      IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_VPN_USER_PASSPHRASE)));
  user_passphrase_textfield_ = new PassphraseTextfield();
  user_passphrase_textfield_->set_controller(this);
  user_passphrase_textfield_->SetEnabled(user_passphrase_ui_data_.IsEditable());
  layout->AddView(user_passphrase_textfield_);
  layout->AddView(new ControlledSettingIndicatorView(user_passphrase_ui_data_));
  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));

  // OTP label and input.
  layout->StartRow(0, 0);
  otp_label_ = new views::Label(l10n_util::GetStringUTF16(
      IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_VPN_OTP));
  layout->AddView(otp_label_);
  otp_textfield_ = new views::Textfield();
  otp_textfield_->set_controller(this);
  layout->AddView(otp_textfield_);
  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));

  // Group Name label and input.
  layout->StartRow(0, 0);
  group_name_label_ = new views::Label(l10n_util::GetStringUTF16(
      IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_VPN_GROUP_NAME));
  layout->AddView(group_name_label_);
  group_name_textfield_ =
      new views::Textfield();
  group_name_textfield_->set_controller(this);
  layout->AddView(group_name_textfield_);
  layout->AddView(new ControlledSettingIndicatorView(group_name_ui_data_));
  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));

  // Save credentials
  layout->StartRow(0, 0);
  save_credentials_checkbox_ = new views::Checkbox(
      l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_SAVE_CREDENTIALS));
  save_credentials_checkbox_->SetEnabled(
      save_credentials_ui_data_.IsEditable());
  layout->SkipColumns(1);
  layout->AddView(save_credentials_checkbox_);
  layout->AddView(
      new ControlledSettingIndicatorView(save_credentials_ui_data_));

  // Error label.
  layout->StartRow(0, 0);
  layout->SkipColumns(1);
  error_label_ = new views::Label();
  error_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  error_label_->SetEnabledColor(SK_ColorRED);
  layout->AddView(error_label_);

  // Set or hide the UI, update comboboxes and error labels.
  Refresh();

  if (vpn) {
    NetworkHandler::Get()->network_configuration_handler()->GetShillProperties(
        service_path_, base::Bind(&VPNConfigView::InitFromProperties,
                                  weak_ptr_factory_.GetWeakPtr()),
        base::Bind(&VPNConfigView::GetPropertiesError,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void VPNConfigView::InitFromProperties(
    const std::string& service_path,
    const base::DictionaryValue& service_properties) {
  const NetworkState* vpn = NetworkHandler::Get()->network_state_handler()->
      GetNetworkState(service_path);
  if (!vpn) {
    NET_LOG_ERROR("Shill Error getting properties VpnConfigView", service_path);
    return;
  }

  std::string provider_type, server_hostname, username, group_name;
  bool psk_passphrase_required = false;
  user_passphrase_required_ = true;
  const base::DictionaryValue* provider_properties;
  if (service_properties.GetDictionaryWithoutPathExpansion(
          shill::kProviderProperty, &provider_properties)) {
    provider_properties->GetStringWithoutPathExpansion(
        shill::kTypeProperty, &provider_type);
    provider_properties->GetStringWithoutPathExpansion(
        shill::kHostProperty, &server_hostname);
    if (provider_type == shill::kProviderL2tpIpsec) {
      provider_properties->GetStringWithoutPathExpansion(
          shill::kL2tpIpsecClientCertIdProperty, &client_cert_id_);
      ca_cert_pem_ = GetPemFromDictionary(
          provider_properties, shill::kL2tpIpsecCaCertPemProperty);
      provider_properties->GetBooleanWithoutPathExpansion(
          shill::kL2tpIpsecPskRequiredProperty, &psk_passphrase_required);
      provider_properties->GetStringWithoutPathExpansion(
          shill::kL2tpIpsecUserProperty, &username);
      provider_properties->GetStringWithoutPathExpansion(
          shill::kL2tpIpsecTunnelGroupProperty, &group_name);
    } else if (provider_type == shill::kProviderOpenVpn) {
      provider_properties->GetStringWithoutPathExpansion(
          shill::kOpenVPNClientCertIdProperty, &client_cert_id_);
      ca_cert_pem_ = GetPemFromDictionary(
          provider_properties, shill::kOpenVPNCaCertPemProperty);
      provider_properties->GetStringWithoutPathExpansion(
          shill::kOpenVPNUserProperty, &username);
      provider_properties->GetBooleanWithoutPathExpansion(
          shill::kPassphraseRequiredProperty, &user_passphrase_required_);
    }
  }
  bool save_credentials = false;
  service_properties.GetBooleanWithoutPathExpansion(
      shill::kSaveCredentialsProperty, &save_credentials);

  provider_type_index_ = ProviderTypeToIndex(provider_type, client_cert_id_);

  if (service_text_)
    service_text_->SetText(base::ASCIIToUTF16(vpn->name()));
  if (provider_type_text_label_)
    provider_type_text_label_->SetText(
        ProviderTypeIndexToString(provider_type_index_));

  if (server_textfield_ && !server_hostname.empty())
    server_textfield_->SetText(base::UTF8ToUTF16(server_hostname));
  if (username_textfield_ && !username.empty())
    username_textfield_->SetText(base::UTF8ToUTF16(username));
  if (group_name_textfield_ && !group_name.empty())
    group_name_textfield_->SetText(base::UTF8ToUTF16(group_name));
  if (psk_passphrase_textfield_)
    psk_passphrase_textfield_->SetShowFake(!psk_passphrase_required);
  if (user_passphrase_textfield_)
    user_passphrase_textfield_->SetShowFake(!user_passphrase_required_);
  if (save_credentials_checkbox_)
    save_credentials_checkbox_->SetChecked(save_credentials);

  Refresh();
  UpdateCanLogin();
}

void VPNConfigView::ParseUIProperties(const NetworkState* vpn) {
  std::string type_dict_name =
      ProviderTypeIndexToONCDictKey(provider_type_index_);
  if (provider_type_index_ == PROVIDER_TYPE_INDEX_L2TP_IPSEC_PSK) {
    ParseVPNUIProperty(vpn, type_dict_name, ::onc::ipsec::kServerCARef,
                       &ca_cert_ui_data_);
    ParseVPNUIProperty(vpn, type_dict_name, ::onc::ipsec::kPSK,
                       &psk_passphrase_ui_data_);
    ParseVPNUIProperty(vpn, type_dict_name, ::onc::ipsec::kGroup,
                       &group_name_ui_data_);
  } else if (provider_type_index_ == PROVIDER_TYPE_INDEX_OPEN_VPN) {
    ParseVPNUIProperty(vpn, type_dict_name, ::onc::openvpn::kServerCARef,
                       &ca_cert_ui_data_);
  }
  ParseVPNUIProperty(vpn, type_dict_name, ::onc::client_cert::kClientCertRef,
                     &user_cert_ui_data_);

  const std::string credentials_dict_name(
      provider_type_index_ == PROVIDER_TYPE_INDEX_L2TP_IPSEC_PSK ?
      ::onc::vpn::kL2TP : type_dict_name);
  ParseVPNUIProperty(vpn, credentials_dict_name, ::onc::vpn::kUsername,
                     &username_ui_data_);
  ParseVPNUIProperty(vpn, credentials_dict_name, ::onc::vpn::kPassword,
                     &user_passphrase_ui_data_);
  ParseVPNUIProperty(vpn, credentials_dict_name, ::onc::vpn::kSaveCredentials,
                     &save_credentials_ui_data_);
}

void VPNConfigView::GetPropertiesError(
    const std::string& error_name,
    std::unique_ptr<base::DictionaryValue> error_data) {
  NET_LOG_ERROR("Shill Error from VpnConfigView: " + error_name, "");
}

void VPNConfigView::SetConfigProperties(
    base::DictionaryValue* properties) {
  int provider_type_index = GetProviderTypeIndex();
  std::string user_passphrase = GetUserPassphrase();
  std::string user_name = GetUsername();
  std::string group_name = GetGroupName();
  switch (provider_type_index) {
    case PROVIDER_TYPE_INDEX_L2TP_IPSEC_PSK: {
      std::string psk_passphrase = GetPSKPassphrase();
      if (!psk_passphrase.empty()) {
        properties->SetKey(shill::kL2tpIpsecPskProperty,
                           base::Value(GetPSKPassphrase()));
      }
      if (!group_name.empty()) {
        properties->SetKey(shill::kL2tpIpsecTunnelGroupProperty,
                           base::Value(group_name));
      }
      if (!user_name.empty()) {
        properties->SetKey(shill::kL2tpIpsecUserProperty,
                           base::Value(user_name));
      }
      if (!user_passphrase.empty()) {
        properties->SetKey(shill::kL2tpIpsecPasswordProperty,
                           base::Value(user_passphrase));
      }
      break;
    }
    case PROVIDER_TYPE_INDEX_L2TP_IPSEC_USER_CERT: {
      if (server_ca_cert_combobox_) {
        std::string ca_cert_pem = GetServerCACertPEM();
        auto pem_list = std::make_unique<base::ListValue>();
        if (!ca_cert_pem.empty())
          pem_list->AppendString(ca_cert_pem);
        properties->SetWithoutPathExpansion(shill::kL2tpIpsecCaCertPemProperty,
                                            std::move(pem_list));
      }
      SetUserCertProperties(client_cert::CONFIG_TYPE_IPSEC, properties);
      if (!group_name.empty()) {
        properties->SetKey(shill::kL2tpIpsecTunnelGroupProperty,
                           base::Value(GetGroupName()));
      }
      if (!user_name.empty()) {
        properties->SetKey(shill::kL2tpIpsecUserProperty,
                           base::Value(user_name));
      }
      if (!user_passphrase.empty()) {
        properties->SetKey(shill::kL2tpIpsecPasswordProperty,
                           base::Value(user_passphrase));
      }
      break;
    }
    case PROVIDER_TYPE_INDEX_OPEN_VPN: {
      if (server_ca_cert_combobox_) {
        std::string ca_cert_pem = GetServerCACertPEM();
        auto pem_list = std::make_unique<base::ListValue>();
        if (!ca_cert_pem.empty())
          pem_list->AppendString(ca_cert_pem);
        properties->SetWithoutPathExpansion(shill::kOpenVPNCaCertPemProperty,
                                            std::move(pem_list));
      }
      SetUserCertProperties(client_cert::CONFIG_TYPE_OPENVPN, properties);
      properties->SetKey(shill::kOpenVPNUserProperty,
                         base::Value(GetUsername()));
      if (!user_passphrase.empty()) {
        properties->SetKey(shill::kOpenVPNPasswordProperty,
                           base::Value(user_passphrase));
      }
      std::string otp = GetOTP();
      if (!otp.empty()) {
        properties->SetKey(shill::kOpenVPNOTPProperty, base::Value(otp));
      }
      break;
    }
    case PROVIDER_TYPE_INDEX_MAX:
      NOTREACHED();
      break;
  }
  properties->SetKey(shill::kSaveCredentialsProperty,
                     base::Value(GetSaveCredentials()));
}

void VPNConfigView::Refresh() {
  UpdateControls();

  // Set certificate combo boxes.
  if (server_ca_cert_combobox_) {
    server_ca_cert_combobox_->ModelChanged();
    if (enable_server_ca_cert_ && !ca_cert_pem_.empty()) {
      // Select the current server CA certificate in the combobox.
      int cert_index =
          CertLibrary::Get()->GetServerCACertIndexByPEM(ca_cert_pem_);
      if (cert_index >= 0) {
        // Skip item for "Default"
        server_ca_cert_combobox_->SetSelectedIndex(1 + cert_index);
      } else {
        server_ca_cert_combobox_->SetSelectedIndex(0);
      }
    } else {
      server_ca_cert_combobox_->SetSelectedIndex(0);
    }
  }

  if (user_cert_combobox_) {
    user_cert_combobox_->ModelChanged();
    if (enable_user_cert_ && !client_cert_id_.empty()) {
      int cert_index =
          CertLibrary::Get()->GetUserCertIndexByPkcs11Id(client_cert_id_);
      if (cert_index >= 0)
        user_cert_combobox_->SetSelectedIndex(cert_index);
      else
        user_cert_combobox_->SetSelectedIndex(0);
    } else {
      user_cert_combobox_->SetSelectedIndex(0);
    }
  }

  UpdateErrorLabel();
}

void VPNConfigView::UpdateControlsToEnable() {
  enable_psk_passphrase_ = false;
  enable_user_cert_ = false;
  enable_server_ca_cert_ = false;
  enable_otp_ = false;
  enable_group_name_ = false;
  int provider_type_index = GetProviderTypeIndex();
  if (provider_type_index == PROVIDER_TYPE_INDEX_L2TP_IPSEC_PSK) {
    enable_psk_passphrase_ = true;
    enable_group_name_ = true;
  } else if (provider_type_index == PROVIDER_TYPE_INDEX_L2TP_IPSEC_USER_CERT) {
    enable_server_ca_cert_ = true;
    enable_user_cert_ = HaveUserCerts();
    enable_group_name_ = true;
  } else {  // PROVIDER_TYPE_INDEX_OPEN_VPN (default)
    enable_server_ca_cert_ = true;
    enable_user_cert_ = HaveUserCerts();
    enable_otp_ = true;
  }
}

void VPNConfigView::UpdateControls() {
  UpdateControlsToEnable();

  if (psk_passphrase_label_)
    psk_passphrase_label_->SetEnabled(enable_psk_passphrase_);
  if (psk_passphrase_textfield_)
    psk_passphrase_textfield_->SetEnabled(enable_psk_passphrase_ &&
                                          psk_passphrase_ui_data_.IsEditable());

  if (user_cert_label_)
    user_cert_label_->SetEnabled(enable_user_cert_);
  if (user_cert_combobox_)
    user_cert_combobox_->SetEnabled(enable_user_cert_ &&
                                    user_cert_ui_data_.IsEditable());

  if (server_ca_cert_label_)
    server_ca_cert_label_->SetEnabled(enable_server_ca_cert_);
  if (server_ca_cert_combobox_)
    server_ca_cert_combobox_->SetEnabled(enable_server_ca_cert_ &&
                                         ca_cert_ui_data_.IsEditable());

  if (otp_label_)
    otp_label_->SetEnabled(enable_otp_);
  if (otp_textfield_)
    otp_textfield_->SetEnabled(enable_otp_);

  if (group_name_label_)
    group_name_label_->SetEnabled(enable_group_name_);
  if (group_name_textfield_)
    group_name_textfield_->SetEnabled(enable_group_name_ &&
                                      group_name_ui_data_.IsEditable());
}

void VPNConfigView::UpdateErrorLabel() {
  // Error message.
  base::string16 error_msg;
  bool cert_required =
      GetProviderTypeIndex() == PROVIDER_TYPE_INDEX_L2TP_IPSEC_USER_CERT;
  if (cert_required && CertLibrary::Get()->CertificatesLoaded()) {
    if (!HaveUserCerts()) {
      error_msg = l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PLEASE_INSTALL_USER_CERT);
    } else if (!IsUserCertValid()) {
      error_msg = l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_REQUIRE_HARDWARE_BACKED);
    }
  }
  if (error_msg.empty() && !service_path_.empty()) {
    // TODO(kuan): differentiate between bad psk and user passphrases.
    const NetworkState* vpn = NetworkHandler::Get()->network_state_handler()->
        GetNetworkState(service_path_);
    if (vpn && vpn->connection_state() == shill::kStateFailure)
      error_msg =
          shill_error::GetShillErrorString(vpn->last_error(), vpn->guid());
  }
  if (!error_msg.empty()) {
    error_label_->SetText(error_msg);
    error_label_->SetVisible(true);
  } else {
    error_label_->SetVisible(false);
  }
}

void VPNConfigView::UpdateCanLogin() {
  parent_->DialogModelChanged();
}

bool VPNConfigView::HaveUserCerts() const {
  return CertLibrary::Get()->NumCertificates(CertLibrary::CERT_TYPE_USER) > 0;
}

bool VPNConfigView::IsUserCertValid() const {
  if (!user_cert_combobox_ || !enable_user_cert_)
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

const std::string VPNConfigView::GetTextFromField(views::Textfield* textfield,
                                                  bool trim_whitespace) const {
  if (!textfield)
    return std::string();
  std::string untrimmed = base::UTF16ToUTF8(textfield->text());
  if (!trim_whitespace)
    return untrimmed;
  std::string result;
  base::TrimWhitespaceASCII(untrimmed, base::TRIM_ALL, &result);
  return result;
}

const std::string VPNConfigView::GetPassphraseFromField(
    PassphraseTextfield* textfield) const {
  if (!textfield)
    return std::string();
  return textfield->GetPassphrase();
}

void VPNConfigView::ParseVPNUIProperty(
    const NetworkState* network,
    const std::string& dict_key,
    const std::string& key,
    NetworkPropertyUIData* property_ui_data) {
  ::onc::ONCSource onc_source = ::onc::ONC_SOURCE_NONE;
  const base::DictionaryValue* onc =
      onc::FindPolicyForActiveUser(network->guid(), &onc_source);

  VLOG_IF(1, !onc) << "No ONC found for VPN network " << network->guid();
  property_ui_data->ParseOncProperty(
      onc_source,
      onc,
      base::StringPrintf("%s.%s.%s",
                         ::onc::network_config::kVPN,
                         dict_key.c_str(),
                         key.c_str()));
}

}  // namespace chromeos
