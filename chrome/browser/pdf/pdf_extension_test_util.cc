// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/pdf/pdf_extension_test_util.h"

#include "chrome/grit/component_extension_resources.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/base/resource/resource_bundle.h"

namespace pdf_extension_test_util {

bool EnsurePDFHasLoaded(content::WebContents* web_contents) {
  std::string scripting_api_js =
      ui::ResourceBundle::GetSharedInstance()
          .GetRawDataResource(IDR_PDF_PDF_SCRIPTING_API_JS)
          .as_string();
  CHECK(content::ExecuteScript(web_contents, scripting_api_js));

  bool load_success = false;
  CHECK(content::ExecuteScriptAndExtractBool(
      web_contents,
      "var scriptingAPI = new PDFScriptingAPI(window, "
      "    document.getElementsByTagName('embed')[0]);"
      "scriptingAPI.setLoadCallback(function(success) {"
      "  window.domAutomationController.send(success);"
      "});",
      &load_success));
  return load_success;
}

}  // namespace pdf_extension_test_util
