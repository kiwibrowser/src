// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/bluetooth_pairing_dialog.h"

#include "base/json/json_writer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/chromeos/bluetooth_dialog_localized_strings_provider.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/l10n/l10n_util.h"

namespace chromeos {

namespace {

constexpr int kBluetoothPairingDialogHeight = 350;

void AddBluetoothStrings(content::WebUIDataSource* html_source) {
  struct {
    const char* name;
    int id;
  } localized_strings[] = {
      {"ok", IDS_OK}, {"cancel", IDS_CANCEL}, {"close", IDS_CLOSE},
  };
  for (const auto& entry : localized_strings)
    html_source->AddLocalizedString(entry.name, entry.id);
  chromeos::bluetooth_dialog::AddLocalizedStrings(html_source);
}

}  // namespace

// static
BluetoothPairingDialog* BluetoothPairingDialog::ShowDialog(
    const std::string& address,
    const base::string16& name_for_display,
    bool paired,
    bool connected) {
  BluetoothPairingDialog* dialog =
      new BluetoothPairingDialog(address, name_for_display, paired, connected);
  dialog->ShowSystemDialog();
  return dialog;
}

BluetoothPairingDialog::BluetoothPairingDialog(
    const std::string& address,
    const base::string16& name_for_display,
    bool paired,
    bool connected)
    : SystemWebDialogDelegate(
          GURL(chrome::kChromeUIBluetoothPairingURL),
          l10n_util::GetStringUTF16(IDS_SETTINGS_BLUETOOTH_PAIR_DEVICE_TITLE)) {
  device_data_.SetString("address", address);
  device_data_.SetString("name", name_for_display);
  device_data_.SetBoolean("paired", paired);
  device_data_.SetBoolean("connected", connected);
}

BluetoothPairingDialog::~BluetoothPairingDialog() = default;

void BluetoothPairingDialog::GetDialogSize(gfx::Size* size) const {
  size->SetSize(SystemWebDialogDelegate::kDialogWidth,
                kBluetoothPairingDialogHeight);
}

std::string BluetoothPairingDialog::GetDialogArgs() const {
  std::string data;
  base::JSONWriter::Write(device_data_, &data);
  return data;
}

// BluetoothPairingUI

BluetoothPairingDialogUI::BluetoothPairingDialogUI(content::WebUI* web_ui)
    : ui::WebDialogUI(web_ui) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIBluetoothPairingHost);

  AddBluetoothStrings(source);
  source->AddLocalizedString("title", IDS_SETTINGS_BLUETOOTH_PAIR_DEVICE_TITLE);
  source->SetJsonPath("strings.js");
#if BUILDFLAG(OPTIMIZE_WEBUI)
  source->UseGzip();
  source->SetDefaultResource(IDR_BLUETOOTH_PAIRING_DIALOG_VULCANIZED_HTML);
  source->AddResourcePath("crisper.js",
                          IDR_BLUETOOTH_PAIRING_DIALOG_CRISPER_JS);
#else
  source->SetDefaultResource(IDR_BLUETOOTH_PAIRING_DIALOG_HTML);
  source->AddResourcePath("bluetooth_pairing_dialog.js",
                          IDR_BLUETOOTH_PAIRING_DIALOG_JS);
#endif
  content::WebUIDataSource::Add(Profile::FromWebUI(web_ui), source);
}

BluetoothPairingDialogUI::~BluetoothPairingDialogUI() = default;

}  // namespace chromeos
