// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/net/aw_cookie_store_wrapper.h"

#include <stdint.h>

#include <memory>

#include "android_webview/browser/net/init_native_callback.h"
#include "base/run_loop.h"
#include "net/cookies/cookie_store.h"
#include "net/cookies/cookie_store_change_unittest.h"
#include "net/cookies/cookie_store_test_callbacks.h"
#include "net/cookies/cookie_store_unittest.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace android_webview {

struct AwCookieStoreWrapperTestTraits {
  static std::unique_ptr<net::CookieStore> Create() {
    auto cookie_store = std::make_unique<AwCookieStoreWrapper>();

    // Android Webview can run multiple tests without restarting the binary,
    // so have to delete any cookies the global store may have from an earlier
    // test.
    net::ResultSavingCookieCallback<uint32_t> callback;
    cookie_store->DeleteAllAsync(
        base::BindOnce(&net::ResultSavingCookieCallback<uint32_t>::Run,
                       base::Unretained(&callback)));
    callback.WaitUntilDone();

    return cookie_store;
  }

  static void DeliverChangeNotifications() {
    base::RunLoop run_loop;
    GetCookieStoreTaskRunner()->PostTaskAndReply(
        FROM_HERE, base::BindOnce([] { base::RunLoop().RunUntilIdle(); }),
        base::BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                       base::Unretained(&run_loop)));
    run_loop.Run();

    base::RunLoop().RunUntilIdle();
  }

  static const bool supports_http_only = true;
  static const bool supports_non_dotted_domains = true;
  static const bool preserves_trailing_dots = true;
  static const bool filters_schemes = true;
  static const bool has_path_prefix_bug = false;
  static const bool forbids_setting_empty_name = false;
  static const bool supports_global_cookie_tracking = false;
  static const bool supports_url_cookie_tracking = false;
  static const bool supports_named_cookie_tracking = true;
  static const bool supports_multiple_tracking_callbacks = false;
  static const bool has_exact_change_cause = true;
  static const bool has_exact_change_ordering = true;
  static const int creation_time_granularity_in_ms = 0;
};

}  // namespace android_webview

// Run the standard cookie tests with AwCookieStoreWrapper. Macro must be in
// net namespace.
namespace net {
INSTANTIATE_TYPED_TEST_CASE_P(AwCookieStoreWrapper,
                              CookieStoreTest,
                              android_webview::AwCookieStoreWrapperTestTraits);
INSTANTIATE_TYPED_TEST_CASE_P(AwCookieStoreWrapper,
                              CookieStoreChangeGlobalTest,
                              android_webview::AwCookieStoreWrapperTestTraits);
INSTANTIATE_TYPED_TEST_CASE_P(AwCookieStoreWrapper,
                              CookieStoreChangeUrlTest,
                              android_webview::AwCookieStoreWrapperTestTraits);
INSTANTIATE_TYPED_TEST_CASE_P(AwCookieStoreWrapper,
                              CookieStoreChangeNamedTest,
                              android_webview::AwCookieStoreWrapperTestTraits);
}  // namespace net
