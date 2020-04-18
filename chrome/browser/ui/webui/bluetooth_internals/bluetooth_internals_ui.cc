// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/bluetooth_internals/bluetooth_internals_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/bluetooth_internals/bluetooth_internals_handler.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "content/public/browser/web_ui_data_source.h"

BluetoothInternalsUI::BluetoothInternalsUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  // Set up the chrome://bluetooth-internals source.
  content::WebUIDataSource* html_source =
      content::WebUIDataSource::Create(chrome::kChromeUIBluetoothInternalsHost);

  // Add required resources.
  html_source->AddResourcePath("adapter.mojom.js",
                               IDR_BLUETOOTH_ADAPTER_MOJO_JS);
  html_source->AddResourcePath("adapter_broker.js",
                               IDR_BLUETOOTH_INTERNALS_ADAPTER_BROKER_JS);
  html_source->AddResourcePath("adapter_page.js",
                               IDR_BLUETOOTH_INTERNALS_ADAPTER_PAGE_JS);
  html_source->AddResourcePath("bluetooth_internals.css",
                               IDR_BLUETOOTH_INTERNALS_CSS);
  html_source->AddResourcePath("bluetooth_internals.js",
                               IDR_BLUETOOTH_INTERNALS_JS);
  html_source->AddResourcePath("bluetooth_internals.mojom.js",
                               IDR_BLUETOOTH_INTERNALS_MOJO_JS);
  html_source->AddResourcePath("characteristic_list.js",
                               IDR_BLUETOOTH_INTERNALS_CHARACTERISTIC_LIST_JS);
  html_source->AddResourcePath("descriptor_list.js",
                               IDR_BLUETOOTH_INTERNALS_DESCRIPTOR_LIST_JS);
  html_source->AddResourcePath("device.mojom.js", IDR_BLUETOOTH_DEVICE_MOJO_JS);
  html_source->AddResourcePath("device_broker.js",
                               IDR_BLUETOOTH_INTERNALS_DEVICE_BROKER_JS);
  html_source->AddResourcePath("device_collection.js",
                               IDR_BLUETOOTH_INTERNALS_DEVICE_COLLECTION_JS);
  html_source->AddResourcePath("device_details_page.js",
                               IDR_BLUETOOTH_INTERNALS_DEVICE_DETAILS_PAGE_JS);
  html_source->AddResourcePath("device_table.js",
                               IDR_BLUETOOTH_INTERNALS_DEVICE_TABLE_JS);
  html_source->AddResourcePath("devices_page.js",
                               IDR_BLUETOOTH_INTERNALS_DEVICES_PAGE_JS);
  html_source->AddResourcePath("expandable_list.js",
                               IDR_BLUETOOTH_INTERNALS_EXPANDABLE_LIST_JS);
  html_source->AddResourcePath("object_fieldset.js",
                               IDR_BLUETOOTH_INTERNALS_OBJECT_FIELDSET_JS);
  html_source->AddResourcePath("service_list.js",
                               IDR_BLUETOOTH_INTERNALS_SERVICE_LIST_JS);
  html_source->AddResourcePath("sidebar.js",
                               IDR_BLUETOOTH_INTERNALS_SIDEBAR_JS);
  html_source->AddResourcePath("snackbar.js",
                               IDR_BLUETOOTH_INTERNALS_SNACKBAR_JS);
  html_source->AddResourcePath("uuid.mojom.js", IDR_BLUETOOTH_UUID_MOJO_JS);
  html_source->AddResourcePath("value_control.js",
                               IDR_BLUETOOTH_INTERNALS_VALUE_CONTROL_JS);

  html_source->SetDefaultResource(IDR_BLUETOOTH_INTERNALS_HTML);
  html_source->UseGzip();

  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, html_source);
  AddHandlerToRegistry(
      base::BindRepeating(&BluetoothInternalsUI::BindBluetoothInternalsHandler,
                          base::Unretained(this)));
}

BluetoothInternalsUI::~BluetoothInternalsUI() {}

void BluetoothInternalsUI::BindBluetoothInternalsHandler(
    mojom::BluetoothInternalsHandlerRequest request) {
  page_handler_.reset(new BluetoothInternalsHandler(std::move(request)));
}
