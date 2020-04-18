// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_browser_impl.h"
#include "headless/lib/browser/headless_content_browser_client.h"
#include "headless/lib/headless_content_main_delegate.h"

#if defined(CHROME_MULTIPLE_DLL_BROWSER)
#include "headless/lib/renderer/headless_content_renderer_client.h"
#endif

namespace headless {

#if defined(CHROME_MULTIPLE_DLL_CHILD)
int HeadlessContentMainDelegate::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  return -1;
}

content::ContentBrowserClient*
HeadlessContentMainDelegate::CreateContentBrowserClient() {
  return nullptr;
}
#endif  // defined(CHROME_MULTIPLE_DLL_CHILD)

#if defined(CHROME_MULTIPLE_DLL_BROWSER)
content::ContentRendererClient*
HeadlessContentMainDelegate::CreateContentRendererClient() {
  return nullptr;
}

content::ContentUtilityClient*
HeadlessContentMainDelegate::CreateContentUtilityClient() {
  return nullptr;
}
#endif  // defined(CHROME_MULTIPLE_DLL_BROWSER)

}  // namespace headless
