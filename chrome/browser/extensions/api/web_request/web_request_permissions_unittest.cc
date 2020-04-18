// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "chrome/common/extensions/extension_test_util.h"
#include "chromeos/login/scoped_test_public_session_login_state.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/previews_state.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "extensions/browser/api/web_request/web_request_permissions.h"
#include "extensions/browser/info_map.h"
#include "extensions/common/constants.h"
#include "extensions/common/permissions/permissions_data.h"
#include "ipc/ipc_message.h"
#include "net/base/request_priority.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_CHROMEOS)
#include "chromeos/login/login_state.h"
#endif  // defined(OS_CHROMEOS)

using content::ResourceRequestInfo;
using extensions::Extension;
using extensions::Manifest;
using extensions::PermissionsData;
using extension_test_util::LoadManifestUnchecked;

class ExtensionWebRequestHelpersTestWithThreadsTest : public testing::Test {
 public:
  ExtensionWebRequestHelpersTestWithThreadsTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {}

 protected:
  void SetUp() override;

 private:
  content::TestBrowserThreadBundle thread_bundle_;

 protected:
  net::TestURLRequestContext context;

  // This extension has Web Request permissions, but no host permission.
  scoped_refptr<Extension> permissionless_extension_;
  // This extension has Web Request permissions, and *.com a host permission.
  scoped_refptr<Extension> com_extension_;
  // This extension is the same as com_extension, except it's installed from
  // Manifest::EXTERNAL_POLICY_DOWNLOAD.
  scoped_refptr<Extension> com_policy_extension_;
  scoped_refptr<extensions::InfoMap> extension_info_map_;
};

void ExtensionWebRequestHelpersTestWithThreadsTest::SetUp() {
  testing::Test::SetUp();

  std::string error;
  permissionless_extension_ = LoadManifestUnchecked("permissions",
                                                    "web_request_no_host.json",
                                                    Manifest::INVALID_LOCATION,
                                                    Extension::NO_FLAGS,
                                                    "ext_id_1",
                                                    &error);
  ASSERT_TRUE(permissionless_extension_.get()) << error;
  com_extension_ =
      LoadManifestUnchecked("permissions",
                            "web_request_com_host_permissions.json",
                            Manifest::INVALID_LOCATION,
                            Extension::NO_FLAGS,
                            "ext_id_2",
                            &error);
  ASSERT_TRUE(com_extension_.get()) << error;
  com_policy_extension_ =
      LoadManifestUnchecked("permissions",
                            "web_request_com_host_permissions.json",
                            Manifest::EXTERNAL_POLICY_DOWNLOAD,
                            Extension::NO_FLAGS,
                            "ext_id_3",
                            &error);
  ASSERT_TRUE(com_policy_extension_.get()) << error;
  extension_info_map_ = new extensions::InfoMap;
  extension_info_map_->AddExtension(permissionless_extension_.get(),
                                    base::Time::Now(),
                                    false, // incognito_enabled
                                    false); // notifications_disabled
  extension_info_map_->AddExtension(
      com_extension_.get(),
      base::Time::Now(),
      false, // incognito_enabled
      false); // notifications_disabled
  extension_info_map_->AddExtension(
      com_policy_extension_.get(),
      base::Time::Now(),
      false, // incognito_enabled
      false); // notifications_disabled
}

TEST_F(ExtensionWebRequestHelpersTestWithThreadsTest, TestHideRequestForURL) {
  net::TestURLRequestContext context;
  const char* const sensitive_urls[] = {
      "http://clients2.google.com",
      "http://clients22.google.com",
      "https://clients2.google.com",
      "http://clients2.google.com/service/update2/crx",
      "https://clients.google.com",
      "https://test.clients.google.com",
      "https://clients2.google.com/service/update2/crx",
      "http://www.gstatic.com/chrome/extensions/blacklist",
      "https://www.gstatic.com/chrome/extensions/blacklist",
      "notregisteredscheme://www.foobar.com",
      "https://chrome.google.com/webstore/",
      "https://chrome.google.com/webstore/"
          "inlineinstall/detail/kcnhkahnjcbndmmehfkdnkjomaanaooo"
  };
  const char* const non_sensitive_urls[] = {
      "http://www.google.com/"
  };

  // Check that requests are rejected based on the destination
  for (size_t i = 0; i < arraysize(sensitive_urls); ++i) {
    GURL sensitive_url(sensitive_urls[i]);
    std::unique_ptr<net::URLRequest> request(
        context.CreateRequest(sensitive_url, net::DEFAULT_PRIORITY, NULL,
                              TRAFFIC_ANNOTATION_FOR_TESTS));
    extensions::WebRequestInfo request_info(request.get());
    EXPECT_TRUE(WebRequestPermissions::HideRequest(extension_info_map_.get(),
                                                   request_info))
        << sensitive_urls[i];
  }
  // Check that requests are accepted if they don't touch sensitive urls.
  for (size_t i = 0; i < arraysize(non_sensitive_urls); ++i) {
    GURL non_sensitive_url(non_sensitive_urls[i]);
    std::unique_ptr<net::URLRequest> request(
        context.CreateRequest(non_sensitive_url, net::DEFAULT_PRIORITY, NULL,
                              TRAFFIC_ANNOTATION_FOR_TESTS));
    extensions::WebRequestInfo request_info(request.get());
    EXPECT_FALSE(WebRequestPermissions::HideRequest(extension_info_map_.get(),
                                                    request_info))
        << non_sensitive_urls[i];
  }

  // Check protection of requests originating from the frame showing the Chrome
  // WebStore.
  // Normally this request is not protected:
  GURL non_sensitive_url("http://www.google.com/test.js");
  std::unique_ptr<net::URLRequest> non_sensitive_request(
      context.CreateRequest(non_sensitive_url, net::DEFAULT_PRIORITY, NULL,
                            TRAFFIC_ANNOTATION_FOR_TESTS));
  extensions::WebRequestInfo non_sensitive_request_info(
      non_sensitive_request.get());
  EXPECT_FALSE(WebRequestPermissions::HideRequest(extension_info_map_.get(),
                                                  non_sensitive_request_info));
  // If the origin is labeled by the WebStoreAppId, it becomes protected.
  {
    int process_id = 42;
    int site_instance_id = 23;
    int view_id = 17;
    std::unique_ptr<net::URLRequest> sensitive_request(
        context.CreateRequest(non_sensitive_url, net::DEFAULT_PRIORITY, NULL,
                              TRAFFIC_ANNOTATION_FOR_TESTS));
    ResourceRequestInfo::AllocateForTesting(
        sensitive_request.get(), content::RESOURCE_TYPE_SCRIPT, NULL,
        process_id, view_id, MSG_ROUTING_NONE,
        /*is_main_frame=*/false,
        /*allow_download=*/true,
        /*is_async=*/false, content::PREVIEWS_OFF,
        /*navigation_ui_data*/ nullptr);
    extension_info_map_->RegisterExtensionProcess(extensions::kWebStoreAppId,
                                                  process_id, site_instance_id);
    extensions::WebRequestInfo sensitive_request_info(sensitive_request.get());
    EXPECT_TRUE(WebRequestPermissions::HideRequest(extension_info_map_.get(),
                                                   sensitive_request_info));
  }

  // Check that requests are for a non-sensitive URL is rejected if it's a PAC
  // script fetch.
  std::unique_ptr<net::URLRequest> request(
      context.CreateRequest(non_sensitive_url, net::DEFAULT_PRIORITY, NULL,
                            TRAFFIC_ANNOTATION_FOR_TESTS));
  request->set_is_pac_request(true);
  extensions::WebRequestInfo request_info(request.get());
  EXPECT_TRUE(WebRequestPermissions::HideRequest(extension_info_map_.get(),
                                                 request_info));
}

TEST_F(ExtensionWebRequestHelpersTestWithThreadsTest,
       TestCanExtensionAccessURL_HostPermissions) {
  // Request with empty initiator.
  std::unique_ptr<net::URLRequest> request(
      context.CreateRequest(GURL("http://example.com"), net::DEFAULT_PRIORITY,
                            NULL, TRAFFIC_ANNOTATION_FOR_TESTS));

  EXPECT_EQ(
      PermissionsData::PageAccess::kAllowed,
      WebRequestPermissions::CanExtensionAccessURL(
          extension_info_map_.get(), permissionless_extension_->id(),
          request->url(),
          -1,     // No tab id.
          false,  // crosses_incognito
          WebRequestPermissions::DO_NOT_CHECK_HOST, request->initiator()));
  EXPECT_EQ(PermissionsData::PageAccess::kDenied,
            WebRequestPermissions::CanExtensionAccessURL(
                extension_info_map_.get(), permissionless_extension_->id(),
                request->url(),
                -1,     // No tab id.
                false,  // crosses_incognito
                WebRequestPermissions::REQUIRE_HOST_PERMISSION_FOR_URL,
                request->initiator()));
  EXPECT_EQ(PermissionsData::PageAccess::kAllowed,
            WebRequestPermissions::CanExtensionAccessURL(
                extension_info_map_.get(), com_extension_->id(), request->url(),
                -1,     // No tab id.
                false,  // crosses_incognito
                WebRequestPermissions::REQUIRE_HOST_PERMISSION_FOR_URL,
                request->initiator()));
  EXPECT_EQ(
      PermissionsData::PageAccess::kAllowed,
      WebRequestPermissions::CanExtensionAccessURL(
          extension_info_map_.get(), com_extension_->id(), request->url(),
          -1,     // No tab id.
          false,  // crosses_incognito
          WebRequestPermissions::REQUIRE_HOST_PERMISSION_FOR_URL_AND_INITIATOR,
          request->initiator()));
  EXPECT_EQ(PermissionsData::PageAccess::kDenied,
            WebRequestPermissions::CanExtensionAccessURL(
                extension_info_map_.get(), com_extension_->id(), request->url(),
                -1,     // No tab id.
                false,  // crosses_incognito
                WebRequestPermissions::REQUIRE_ALL_URLS, request->initiator()));

  std::unique_ptr<net::URLRequest> request_with_initiator(
      context.CreateRequest(GURL("http://example.com"), net::DEFAULT_PRIORITY,
                            nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));
  request_with_initiator->set_initiator(
      url::Origin::Create(GURL("http://www.example.org")));

  EXPECT_EQ(PermissionsData::PageAccess::kAllowed,
            WebRequestPermissions::CanExtensionAccessURL(
                extension_info_map_.get(), permissionless_extension_->id(),
                request_with_initiator->url(),
                -1,     // No tab id.
                false,  // crosses_incognito
                WebRequestPermissions::DO_NOT_CHECK_HOST,
                request_with_initiator->initiator()));
  EXPECT_EQ(PermissionsData::PageAccess::kDenied,
            WebRequestPermissions::CanExtensionAccessURL(
                extension_info_map_.get(), permissionless_extension_->id(),
                request_with_initiator->url(),
                -1,     // No tab id.
                false,  // crosses_incognito
                WebRequestPermissions::REQUIRE_HOST_PERMISSION_FOR_URL,
                request_with_initiator->initiator()));
  EXPECT_EQ(PermissionsData::PageAccess::kAllowed,
            WebRequestPermissions::CanExtensionAccessURL(
                extension_info_map_.get(), com_extension_->id(),
                request_with_initiator->url(),
                -1,     // No tab id.
                false,  // crosses_incognito
                WebRequestPermissions::REQUIRE_HOST_PERMISSION_FOR_URL,
                request_with_initiator->initiator()));
  EXPECT_EQ(
      PermissionsData::PageAccess::kDenied,
      WebRequestPermissions::CanExtensionAccessURL(
          extension_info_map_.get(), com_extension_->id(),
          request_with_initiator->url(),
          -1,     // No tab id.
          false,  // crosses_incognito
          WebRequestPermissions::REQUIRE_HOST_PERMISSION_FOR_URL_AND_INITIATOR,
          request_with_initiator->initiator()));
  EXPECT_EQ(PermissionsData::PageAccess::kDenied,
            WebRequestPermissions::CanExtensionAccessURL(
                extension_info_map_.get(), com_extension_->id(),
                request_with_initiator->url(),
                -1,     // No tab id.
                false,  // crosses_incognito
                WebRequestPermissions::REQUIRE_ALL_URLS,
                request_with_initiator->initiator()));

  // Public Sessions tests.
#if defined(OS_CHROMEOS)
  std::unique_ptr<net::URLRequest> org_request(context.CreateRequest(
      GURL("http://example.org"), net::DEFAULT_PRIORITY, nullptr));

  // com_extension_ doesn't have host permission for .org URLs.
  EXPECT_EQ(PermissionsData::PageAccess::kDenied,
            WebRequestPermissions::CanExtensionAccessURL(
                extension_info_map_.get(), com_policy_extension_->id(),
                org_request->url(),
                -1,     // No tab id.
                false,  // crosses_incognito
                WebRequestPermissions::REQUIRE_HOST_PERMISSION_FOR_URL,
                org_request->initiator()));

  chromeos::ScopedTestPublicSessionLoginState login_state;

  // Host permission checks are disabled in Public Sessions, instead all URLs
  // are whitelisted.
  EXPECT_EQ(PermissionsData::PageAccess::kAllowed,
            WebRequestPermissions::CanExtensionAccessURL(
                extension_info_map_.get(), com_policy_extension_->id(),
                org_request->url(),
                -1,     // No tab id.
                false,  // crosses_incognito
                WebRequestPermissions::REQUIRE_HOST_PERMISSION_FOR_URL,
                org_request->initiator()));

  EXPECT_EQ(
      PermissionsData::PageAccess::kAllowed,
      WebRequestPermissions::CanExtensionAccessURL(
          extension_info_map_.get(), com_policy_extension_->id(),
          org_request->url(),
          -1,     // No tab id.
          false,  // crosses_incognito
          WebRequestPermissions::REQUIRE_ALL_URLS, org_request->initiator()));

  // Make sure that chrome:// URLs cannot be accessed.
  std::unique_ptr<net::URLRequest> chrome_request(
      context.CreateRequest(GURL("chrome://version/"), net::DEFAULT_PRIORITY,
                            nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));

  EXPECT_EQ(PermissionsData::PageAccess::kDenied,
            WebRequestPermissions::CanExtensionAccessURL(
                extension_info_map_.get(), com_policy_extension_->id(),
                chrome_request->url(),
                -1,     // No tab id.
                false,  // crosses_incognito
                WebRequestPermissions::REQUIRE_HOST_PERMISSION_FOR_URL,
                chrome_request->initiator()));
#endif
}
