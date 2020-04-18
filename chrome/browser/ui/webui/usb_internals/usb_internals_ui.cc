// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/usb_internals/usb_internals_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/usb_internals/usb_internals_page_handler.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "content/public/browser/web_ui_data_source.h"

UsbInternalsUI::UsbInternalsUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  // Set up the chrome://usb-internals source.
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIUsbInternalsHost);
  source->AddResourcePath("usb_internals.css", IDR_USB_INTERNALS_CSS);
  source->AddResourcePath("usb_internals.js", IDR_USB_INTERNALS_JS);
  source->AddResourcePath(
      "chrome/browser/ui/webui/usb_internals/usb_internals.mojom.js",
      IDR_USB_INTERNALS_MOJO_JS);
  source->AddResourcePath("url/mojom/origin.mojom.js", IDR_ORIGIN_MOJO_JS);
  source->AddResourcePath("url/mojom/url.mojom.js", IDR_URL_MOJO_JS);
  source->SetDefaultResource(IDR_USB_INTERNALS_HTML);
  source->UseGzip();

  content::WebUIDataSource::Add(Profile::FromWebUI(web_ui), source);
  AddHandlerToRegistry(base::BindRepeating(
      &UsbInternalsUI::BindUsbInternalsPageHandler, base::Unretained(this)));
}

UsbInternalsUI::~UsbInternalsUI() {}

void UsbInternalsUI::BindUsbInternalsPageHandler(
    mojom::UsbInternalsPageHandlerRequest request) {
  page_handler_.reset(new UsbInternalsPageHandler(std::move(request)));
}
