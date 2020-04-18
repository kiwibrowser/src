// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/pdf/pdf_extension_util.h"

#include "base/strings/string_util.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/grit/browser_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace pdf_extension_util {

namespace {

// Tags in the manifest to be replaced.
const char kNameTag[] = "<NAME>";

}  // namespace

std::string GetManifest() {
  std::string manifest_contents = ui::ResourceBundle::GetSharedInstance()
                                      .GetRawDataResource(IDR_PDF_MANIFEST)
                                      .as_string();
  DCHECK(manifest_contents.find(kNameTag) != std::string::npos);
  base::ReplaceFirstSubstringAfterOffset(
      &manifest_contents, 0, kNameTag,
      ChromeContentClient::kPDFExtensionPluginName);

  return manifest_contents;
}

}  // namespace pdf_extension_util
