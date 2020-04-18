// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/browser/extensions/api/browser/browser_api.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "extensions/common/extension_builder.h"

namespace extensions {

namespace utils = extension_function_test_utils;

namespace {

class BrowserApiTest : public InProcessBrowserTest {
};

}

IN_PROC_BROWSER_TEST_F(BrowserApiTest, OpenTab) {
  std::string url = "about:blank";

  scoped_refptr<api::BrowserOpenTabFunction> function =
      new api::BrowserOpenTabFunction();
  scoped_refptr<Extension> extension(ExtensionBuilder("Test").Build());
  function->set_extension(extension.get());
  base::Value* result = utils::RunFunctionAndReturnSingleResult(
      function.get(),
      base::StringPrintf("[{\"url\":\"%s\"}]", url.c_str()),
      browser());

  // result is currently NULL on success.
  EXPECT_FALSE(result);
}

}  // namespace extensions
