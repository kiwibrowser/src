// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/options/wimax_config_view.h"

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/net/shill_error.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/ash/network/enrollment_dialog_view.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "chromeos/login/login_state.h"
#include "chromeos/network/network_configuration_handler.h"
#include "chromeos/network/network_connect.h"
#include "chromeos/network/network_event_log.h"
#include "chromeos/network/network_profile.h"
#include "chromeos/network/network_profile_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/onc/onc_utils.h"
#include "components/onc/onc_constants.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/events/event.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"

namespace chromeos {

namespace {

void ShillError(const std::string& function,
                const std::string& error_name,
                std::unique_ptr<base::DictionaryValue> error_data) {
  NET_LOG_ERROR("Shill Error from WimaxConfigView: " + error_name, function);
}

}  // namespace

WimaxConfigView::WimaxConfigView(NetworkConfigView* parent,
                                 const std::string& service_path)
    : ChildNetworkConfigView(parent, service_path),
      identity_label_(NULL),
      identity_textfield_(NULL),
      save_credentials_checkbox_(NULL),
      share_network_checkbox_(NULL),
      passphrase_label_(NULL),
      passphrase_textfield_(NULL),
      passphrase_visible_button_(NULL),
      error_label_(NULL),
      weak_ptr_factory_(this) {
  Init();
}

WimaxConfigView::~WimaxConfigView() {
  RemoveAllChildViews(true);  // Destroy children before models
}

base::string16 WimaxConfigView::GetTitle() const {
  return l10n_util::GetStringUTF16(IDS_OPTIONS_SETTINGS_JOIN_WIMAX_NETWORKS);
}

views::View* WimaxConfigView::GetInitiallyFocusedView() {
  if (identity_textfield_ && identity_textfield_->enabled())
    return identity_textfield_;
  if (passphrase_textfield_ && passphrase_textfield_->enabled())
    return passphrase_textfield_;
  return NULL;
}

bool WimaxConfigView::CanLogin() {
  // In OOBE it may be valid to log in with no credentials (crbug.com/137776).
  if (!chromeos::StartupUtils::IsOobeCompleted())
    return true;

  // TODO(benchan): Update this with the correct minimum length (don't just
  // check if empty).
  // If the network requires a passphrase, make sure it is the right length.
  return passphrase_textfield_ && !passphrase_textfield_->text().empty();
}

void WimaxConfigView::UpdateDialogButtons() {
  parent_->DialogModelChanged();
}

void WimaxConfigView::UpdateErrorLabel() {
  base::string16 error_msg;
  if (!service_path_.empty()) {
    const NetworkState* wimax = NetworkHandler::Get()->network_state_handler()->
        GetNetworkState(service_path_);
    if (wimax && wimax->connection_state() == shill::kStateFailure)
      error_msg =
          shill_error::GetShillErrorString(wimax->last_error(), wimax->guid());
  }
  if (!error_msg.empty()) {
    error_label_->SetText(error_msg);
    error_label_->SetVisible(true);
  } else {
    error_label_->SetVisible(false);
  }
}

void WimaxConfigView::ContentsChanged(views::Textfield* sender,
                                      const base::string16& new_contents) {
  UpdateDialogButtons();
}

bool WimaxConfigView::HandleKeyEvent(views::Textfield* sender,
                                     const ui::KeyEvent& key_event) {
  if (sender == passphrase_textfield_ &&
      key_event.type() == ui::ET_KEY_PRESSED &&
      key_event.key_code() == ui::VKEY_RETURN) {
    parent_->GetDialogClientView()->AcceptWindow();
  }
  return false;
}

void WimaxConfigView::ButtonPressed(views::Button* sender,
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

bool WimaxConfigView::Login() {
  const NetworkState* wimax = NetworkHandler::Get()->network_state_handler()->
      GetNetworkState(service_path_);
  if (!wimax) {
    // Shill no longer knows about this network (edge case).
    // TODO(stevenjb): Add notification for this.
    NET_LOG_ERROR("Network not found", service_path_);
    return true;  // Close dialog
  }
  base::DictionaryValue properties;
  properties.SetKey(shill::kEapIdentityProperty, base::Value(GetEapIdentity()));
  properties.SetKey(shill::kEapPasswordProperty,
                    base::Value(GetEapPassphrase()));
  properties.SetKey(shill::kSaveCredentialsProperty,
                    base::Value(GetSaveCredentials()));

  const bool share_default = true;
  bool share_network = GetShareNetwork(share_default);

  bool only_policy_autoconnect =
      onc::PolicyAllowsOnlyPolicyNetworksToAutoconnect(!share_network);
  if (only_policy_autoconnect) {
    properties.SetKey(shill::kAutoConnectProperty, base::Value(false));
  }

  NetworkConnect::Get()->ConfigureNetworkIdAndConnect(wimax->guid(), properties,
                                                      share_network);
  return true;  // dialog will be closed
}

std::string WimaxConfigView::GetEapIdentity() const {
  DCHECK(identity_textfield_);
  return base::UTF16ToUTF8(identity_textfield_->text());
}

std::string WimaxConfigView::GetEapPassphrase() const {
  return passphrase_textfield_ ? base::UTF16ToUTF8(
                                     passphrase_textfield_->text()) :
                                 std::string();
}

bool WimaxConfigView::GetSaveCredentials() const {
  return save_credentials_checkbox_ ? save_credentials_checkbox_->checked() :
                                      false;
}

bool WimaxConfigView::GetShareNetwork(bool share_default) const {
  return share_network_checkbox_ ? share_network_checkbox_->checked() :
                                   share_default;
}

void WimaxConfigView::Cancel() {
}

void WimaxConfigView::Init() {
  const views::LayoutProvider* provider = views::LayoutProvider::Get();
  SetBorder(views::CreateEmptyBorder(
      provider->GetDialogInsetsForContentType(views::TEXT, views::TEXT)));

  const NetworkState* wimax = NetworkHandler::Get()->network_state_handler()->
      GetNetworkState(service_path_);
  DCHECK(wimax && wimax->type() == shill::kTypeWimax);

  WifiConfigView::ParseEAPUIProperty(
      &save_credentials_ui_data_, wimax, ::onc::eap::kSaveCredentials);
  WifiConfigView::ParseEAPUIProperty(
      &identity_ui_data_, wimax, ::onc::eap::kIdentity);
  WifiConfigView::ParseUIProperty(
      &passphrase_ui_data_, wimax, ::onc::wifi::kPassphrase);

  views::GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));

  const int column_view_set_id = 0;
  views::ColumnSet* column_set = layout->AddColumnSet(column_view_set_id);
  const int kPasswordVisibleWidth = 20;
  // Label
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
  // Password visible button / policy indicator.
  column_set->AddColumn(views::GridLayout::CENTER, views::GridLayout::FILL, 1,
                        views::GridLayout::USE_PREF, 0, kPasswordVisibleWidth);

  // Network name
  layout->StartRow(0, column_view_set_id);
  layout->AddView(new views::Label(l10n_util::GetStringUTF16(
      IDS_OPTIONS_SETTINGS_INTERNET_TAB_NETWORK)));
  views::Label* label = new views::Label(base::UTF8ToUTF16(wimax->name()));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  layout->AddView(label);
  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));

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
  layout->AddView(identity_textfield_);
  layout->AddView(new ControlledSettingIndicatorView(identity_ui_data_));
  layout->AddPaddingRow(
      0, provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL));

  // Passphrase input
  layout->StartRow(0, column_view_set_id);
  base::string16 passphrase_label_text = l10n_util::GetStringUTF16(
      IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_PASSPHRASE);
  passphrase_label_ = new views::Label(passphrase_label_text);
  layout->AddView(passphrase_label_);
  passphrase_textfield_ = new views::Textfield();
  passphrase_textfield_->SetTextInputType(ui::TEXT_INPUT_TYPE_PASSWORD);
  passphrase_textfield_->set_controller(this);
  passphrase_label_->SetEnabled(true);
  passphrase_textfield_->SetEnabled(passphrase_ui_data_.IsEditable());
  passphrase_textfield_->SetAccessibleName(passphrase_label_text);
  layout->AddView(passphrase_textfield_);

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

  // Checkboxes.

  if (LoginState::Get()->IsUserAuthenticated()) {
    // Save credentials
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
  if (wimax->profile_path().empty()) {
    layout->StartRow(0, column_view_set_id);
    share_network_checkbox_ = new views::Checkbox(
        l10n_util::GetStringUTF16(
            IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_SHARE_NETWORK));

    bool share_network_checkbox_value = false;
    bool share_network_checkbox_enabled = false;
    ChildNetworkConfigView::GetShareStateForLoginState(
        &share_network_checkbox_value,
        &share_network_checkbox_enabled);

    share_network_checkbox_->SetChecked(share_network_checkbox_value);
    share_network_checkbox_->SetEnabled(share_network_checkbox_enabled);

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

  UpdateErrorLabel();

  if (wimax) {
    NetworkHandler::Get()->network_configuration_handler()->GetShillProperties(
        service_path_, base::Bind(&WimaxConfigView::InitFromProperties,
                                  weak_ptr_factory_.GetWeakPtr()),
        base::Bind(&ShillError, "GetProperties"));
  }
}

void WimaxConfigView::InitFromProperties(
    const std::string& service_path,
    const base::DictionaryValue& properties) {
  // EapIdentity
  std::string eap_identity;
  properties.GetStringWithoutPathExpansion(
      shill::kEapIdentityProperty, &eap_identity);
  identity_textfield_->SetText(base::UTF8ToUTF16(eap_identity));

  // Save credentials
  if (save_credentials_checkbox_) {
    bool save_credentials = false;
    properties.GetBooleanWithoutPathExpansion(
        shill::kSaveCredentialsProperty, &save_credentials);
    save_credentials_checkbox_->SetChecked(save_credentials);
  }
}

void WimaxConfigView::InitFocus() {
  views::View* view_to_focus = GetInitiallyFocusedView();
  if (view_to_focus)
    view_to_focus->RequestFocus();
}

}  // namespace chromeos
