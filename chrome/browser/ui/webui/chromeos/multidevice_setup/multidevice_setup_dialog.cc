// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/multidevice_setup/multidevice_setup_dialog.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/multidevice_setup_resources.h"
#include "chrome/grit/multidevice_setup_resources_map.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/l10n/l10n_util.h"

namespace chromeos {

namespace multidevice_setup {

namespace {

constexpr int kDialogHeightPx = 640;
constexpr int kDialogWidthPx = 768;

void AddMultiDeviceSetupStrings(content::WebUIDataSource* html_source) {
  // TODO(jordynass): Add translations for other strings appearing in the
  // dialog by adding new name/translation pairs to |kLocalizedStrings| below.
  // String definitions belong in //chrome/app/chromeos_strings.grdp.
  static constexpr struct {
    const char* name;
    int id;
  } kLocalizedStrings[] = {
      {"accept", IDS_MULTIDEVICE_SETUP_ACCEPT_LABEL},
      {"cancel", IDS_CANCEL},
      {"done", IDS_DONE},
      {"startSetupPageMultipleDeviceHeader",
       IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_MULTIPLE_DEVICE_HEADER},
      {"startSetupPageSingleDeviceHeader",
       IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_SINGLE_DEVICE_HEADER},
      {"setupFailedPageHeader", IDS_MULTIDEVICE_SETUP_SETUP_FAILED_PAGE_HEADER},
      {"setupFailedPageMessage",
       IDS_MULTIDEVICE_SETUP_SETUP_FAILED_PAGE_MESSAGE},
      {"setupSucceededPageHeader",
       IDS_MULTIDEVICE_SETUP_SETUP_SUCCEEDED_PAGE_HEADER},
      {"setupSucceededPageMessage",
       IDS_MULTIDEVICE_SETUP_SETUP_SUCCEEDED_PAGE_MESSAGE},
      {"startSetupPageHeader", IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_HEADER},
      {"startSetupPageMessage", IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_MESSAGE},
      {"title", IDS_MULTIDEVICE_SETUP_DIALOG_TITLE},
      {"tryAgain", IDS_MULTIDEVICE_SETUP_TRY_AGAIN_LABEL},
  };

  for (const auto& entry : kLocalizedStrings)
    html_source->AddLocalizedString(entry.name, entry.id);
}

}  // namespace

// static
MultiDeviceSetupDialog* MultiDeviceSetupDialog::current_instance_ = nullptr;

// static
void MultiDeviceSetupDialog::Show() {
  // The dialog is already showing, so there is nothing to do.
  if (current_instance_)
    return;

  current_instance_ = new MultiDeviceSetupDialog();
  current_instance_->ShowSystemDialog();
}

MultiDeviceSetupDialog::MultiDeviceSetupDialog()
    : SystemWebDialogDelegate(
          GURL(chrome::kChromeUIMultiDeviceSetupUrl),
          l10n_util::GetStringUTF16(IDS_MULTIDEVICE_SETUP_DIALOG_TITLE)) {}

MultiDeviceSetupDialog::~MultiDeviceSetupDialog() = default;

void MultiDeviceSetupDialog::GetDialogSize(gfx::Size* size) const {
  size->SetSize(kDialogWidthPx, kDialogHeightPx);
}

void MultiDeviceSetupDialog::OnDialogClosed(const std::string& json_retval) {
  DCHECK(this == current_instance_);
  current_instance_ = nullptr;

  // Note: The call below deletes |this|, so there is no further need to keep
  // track of the pointer.
  SystemWebDialogDelegate::OnDialogClosed(json_retval);
}

MultiDeviceSetupDialogUI::MultiDeviceSetupDialogUI(content::WebUI* web_ui)
    : ui::WebDialogUI(web_ui) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIMultiDeviceSetupHost);

  AddMultiDeviceSetupStrings(source);
  source->SetJsonPath("strings.js");
  source->SetDefaultResource(
      IDR_MULTIDEVICE_SETUP_MULTIDEVICE_SETUP_DIALOG_HTML);

  // Note: The |kMultiDeviceSetupResourcesSize| and |kMultideviceSetupResources|
  // fields are defined in the generated file
  // chrome/grit/multidevice_setup_resources_map.h.
  for (size_t i = 0; i < kMultideviceSetupResourcesSize; ++i) {
    source->AddResourcePath(kMultideviceSetupResources[i].name,
                            kMultideviceSetupResources[i].value);
  }

  content::WebUIDataSource::Add(Profile::FromWebUI(web_ui), source);
}

MultiDeviceSetupDialogUI::~MultiDeviceSetupDialogUI() = default;

}  // namespace multidevice_setup

}  // namespace chromeos
