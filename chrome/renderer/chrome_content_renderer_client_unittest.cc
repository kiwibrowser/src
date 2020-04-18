// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/chrome_content_renderer_client.h"

#include <stddef.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/metrics/histogram_samples.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/renderer/searchbox/search_bouncer.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h"
#include "content/public/common/webplugininfo.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/manifest_constants.h"
#endif

#if BUILDFLAG(ENABLE_NACL)
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_plugin_params.h"
#endif

#if BUILDFLAG(ENABLE_NACL)
using blink::WebPluginParams;
using blink::WebString;
using blink::WebVector;
#endif

using content::WebPluginInfo;
using content::WebPluginMimeType;

namespace {

#if BUILDFLAG(ENABLE_NACL)
const bool kNaClRestricted = false;
const bool kNaClUnrestricted = true;
const bool kExtensionNotFromWebStore = false;
const bool kExtensionFromWebStore = true;
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
const bool kNotHostedApp = false;
const bool kHostedApp = true;
#endif

#if BUILDFLAG(ENABLE_NACL)
const char kExtensionUrl[] = "chrome-extension://extension_id/background.html";

const char kChatManifestFS[] = "filesystem:https://talkgadget.google.com/foo";
#endif

const char kChatAppURL[] = "https://talkgadget.google.com/hangouts/foo";

void AddContentTypeHandler(content::WebPluginInfo* info,
                           const char* mime_type,
                           const char* manifest_url) {
  content::WebPluginMimeType mime_type_info;
  mime_type_info.mime_type = mime_type;
  mime_type_info.additional_params.emplace_back(
      base::UTF8ToUTF16("nacl"), base::UTF8ToUTF16(manifest_url));
  info->mime_types.push_back(mime_type_info);
}

}  // namespace

typedef testing::Test ChromeContentRendererClientTest;


#if BUILDFLAG(ENABLE_EXTENSIONS)
scoped_refptr<const extensions::Extension> CreateTestExtension(
    extensions::Manifest::Location location, bool is_from_webstore,
    bool is_hosted_app, const std::string& app_url) {
  int flags = is_from_webstore ?
      extensions::Extension::FROM_WEBSTORE:
      extensions::Extension::NO_FLAGS;

  base::DictionaryValue manifest;
  manifest.SetString("name", "NaCl Extension");
  manifest.SetString("version", "1");
  manifest.SetInteger("manifest_version", 2);
  if (is_hosted_app) {
    auto url_list = std::make_unique<base::ListValue>();
    url_list->AppendString(app_url);
    manifest.Set(extensions::manifest_keys::kWebURLs, std::move(url_list));
    manifest.SetString(extensions::manifest_keys::kLaunchWebURL, app_url);
  }
  std::string error;
  return extensions::Extension::Create(base::FilePath(), location, manifest,
                                       flags, &error);
}

scoped_refptr<const extensions::Extension> CreateExtension(
    bool is_from_webstore) {
  return CreateTestExtension(
      extensions::Manifest::INTERNAL, is_from_webstore, kNotHostedApp,
      std::string());
}

scoped_refptr<const extensions::Extension> CreateExtensionWithLocation(
    extensions::Manifest::Location location, bool is_from_webstore) {
  return CreateTestExtension(
      location, is_from_webstore, kNotHostedApp, std::string());
}

scoped_refptr<const extensions::Extension> CreateHostedApp(
    bool is_from_webstore, const std::string& app_url) {
  return CreateTestExtension(extensions::Manifest::INTERNAL,
                             is_from_webstore,
                             kHostedApp,
                             app_url);
}
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

TEST_F(ChromeContentRendererClientTest, NaClRestriction) {
  // Unknown content types have no NaCl module.
  {
    WebPluginInfo info;
    EXPECT_EQ(GURL(),
              ChromeContentRendererClient::GetNaClContentHandlerURL(
                  "application/x-foo", info));
  }
  // Known content types have a NaCl module.
  {
    WebPluginInfo info;
    AddContentTypeHandler(&info, "application/x-foo", "www.foo.com");
    EXPECT_EQ(GURL("www.foo.com"),
              ChromeContentRendererClient::GetNaClContentHandlerURL(
                  "application/x-foo", info));
  }
#if BUILDFLAG(ENABLE_NACL)
  // --enable-nacl allows all NaCl apps.
  {
    WebPluginParams params;
    EXPECT_TRUE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(),
        GURL(),
        kNaClUnrestricted,
        CreateExtension(kExtensionNotFromWebStore).get(),
        &params));
  }
  // Unpacked extensions are allowed without --enable-nacl.
  {
    WebPluginParams params;
    EXPECT_TRUE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(),
        GURL(kExtensionUrl),
        kNaClRestricted,
        CreateExtensionWithLocation(extensions::Manifest::UNPACKED,
                                    kExtensionNotFromWebStore).get(),
        &params));
  }
  // Component extensions are allowed without --enable-nacl.
  {
    WebPluginParams params;
    EXPECT_TRUE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(),
        GURL(kExtensionUrl),
        kNaClRestricted,
        CreateExtensionWithLocation(extensions::Manifest::COMPONENT,
                                    kExtensionNotFromWebStore).get(),
        &params));
  }
  {
    WebPluginParams params;
    EXPECT_TRUE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(),
        GURL(kExtensionUrl),
        kNaClRestricted,
        CreateExtensionWithLocation(extensions::Manifest::EXTERNAL_COMPONENT,
                                    kExtensionNotFromWebStore).get(),
        &params));
  }
  // Extensions that are force installed by policy are allowed without
  // --enable-nacl.
  {
    WebPluginParams params;
    EXPECT_TRUE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(),
        GURL(kExtensionUrl),
        kNaClRestricted,
        CreateExtensionWithLocation(extensions::Manifest::EXTERNAL_POLICY,
                                    kExtensionNotFromWebStore).get(),
        &params));
    EXPECT_TRUE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(),
        GURL(kExtensionUrl),
        kNaClRestricted,
        CreateExtensionWithLocation(
            extensions::Manifest::EXTERNAL_POLICY_DOWNLOAD,
            kExtensionNotFromWebStore).get(),
        &params));
  }
  // CWS extensions are allowed without --enable-nacl if called from an
  // extension url.
  {
    WebPluginParams params;
    EXPECT_TRUE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(),
        GURL(kExtensionUrl),
        kNaClRestricted,
        CreateExtension(kExtensionFromWebStore).get(),
        &params));
  }

  // Whitelisted URLs are allowed without --enable-nacl. There is a whitelist
  // for the app URL and the manifest URL.
  {
    WebPluginParams params;
    // Whitelisted Chat app is allowed.
    EXPECT_TRUE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(kChatManifestFS),
        GURL(kChatAppURL),
        kNaClRestricted,
        nullptr,
        &params));

    // Whitelisted manifest URL, bad app URLs, NOT allowed.
    EXPECT_FALSE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(kChatManifestFS),
        GURL("http://plus.google.com/foo"),  // http scheme
        kNaClRestricted, nullptr, &params));
    EXPECT_FALSE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(kChatManifestFS),
        GURL("http://plus.sandbox.google.com/foo"),  // http scheme
        kNaClRestricted, nullptr, &params));
    EXPECT_FALSE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(kChatManifestFS),
        GURL("https://plus.google.evil.com/foo"),  // bad host
        kNaClRestricted, nullptr, &params));
    // Whitelisted app URL, bad manifest URL, NOT allowed.
    EXPECT_FALSE(ChromeContentRendererClient::IsNaClAllowed(
        GURL("http://ssl.gstatic.com/s2/oz/nacl/foo"),  // http scheme
        GURL(kChatAppURL), kNaClRestricted, nullptr, &params));
    EXPECT_FALSE(ChromeContentRendererClient::IsNaClAllowed(
        GURL("https://ssl.gstatic.evil.com/s2/oz/nacl/foo"),  // bad host
        GURL(kChatAppURL), kNaClRestricted, nullptr, &params));
    EXPECT_FALSE(ChromeContentRendererClient::IsNaClAllowed(
        GURL("https://ssl.gstatic.com/wrong/s2/oz/nacl/foo"),  // bad path
        GURL(kChatAppURL), kNaClRestricted, nullptr, &params));
  }
  // Non-whitelisted URLs are blocked without --enable-nacl.
  {
    WebPluginParams params;
    EXPECT_FALSE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(),
        GURL("https://plus.google.com.evil.com/foo1"),
        kNaClRestricted,
        nullptr,
        &params));
  }
  // Non chrome-extension:// URLs belonging to hosted apps are allowed for
  // webstore installed hosted apps.
  {
    WebPluginParams params;
    EXPECT_TRUE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(),
        GURL("http://example.com/test.html"),
        kNaClRestricted,
        CreateHostedApp(kExtensionFromWebStore,
                        "http://example.com/").get(),
        &params));
    EXPECT_FALSE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(),
        GURL("http://example.com/test.html"),
        kNaClRestricted,
        CreateHostedApp(kExtensionNotFromWebStore,
                        "http://example.com/").get(),
        &params));
    EXPECT_FALSE(ChromeContentRendererClient::IsNaClAllowed(
        GURL(),
        GURL("http://example.evil.com/test.html"),
        kNaClRestricted,
        CreateHostedApp(kExtensionNotFromWebStore,
                        "http://example.com/").get(),
        &params));
  }
#endif  // BUILDFLAG(ENABLE_NACL)
}

TEST_F(ChromeContentRendererClientTest, AllowPepperMediaStreamAPI) {
  ChromeContentRendererClient test;
#if defined(OS_ANDROID)
  EXPECT_FALSE(test.AllowPepperMediaStreamAPI(GURL(kChatAppURL)));
#else
  EXPECT_TRUE(test.AllowPepperMediaStreamAPI(GURL(kChatAppURL)));
#endif
}

// SearchBouncer doesn't exist on Android.
#if !defined(OS_ANDROID)
TEST_F(ChromeContentRendererClientTest, ShouldSuppressErrorPage) {
  ChromeContentRendererClient client;
  SearchBouncer::GetInstance()->SetNewTabPageURL(GURL("http://example.com/n"));
  EXPECT_FALSE(client.ShouldSuppressErrorPage(nullptr,
                                              GURL("http://example.com")));
  EXPECT_TRUE(client.ShouldSuppressErrorPage(nullptr,
                                             GURL("http://example.com/n")));
  SearchBouncer::GetInstance()->SetNewTabPageURL(GURL::EmptyGURL());
}

TEST_F(ChromeContentRendererClientTest, ShouldTrackUseCounter) {
  ChromeContentRendererClient client;
  SearchBouncer::GetInstance()->SetNewTabPageURL(GURL("http://example.com/n"));
  EXPECT_TRUE(client.ShouldTrackUseCounter(GURL("http://example.com")));
  EXPECT_FALSE(client.ShouldTrackUseCounter(GURL("http://example.com/n")));
  SearchBouncer::GetInstance()->SetNewTabPageURL(GURL::EmptyGURL());
}
#endif

TEST_F(ChromeContentRendererClientTest, AddImageContextMenuPropertiesForLoFi) {
  ChromeContentRendererClient client;
  blink::WebURLResponse web_url_response;
  web_url_response.AddHTTPHeaderField(
      blink::WebString::FromUTF8(
          data_reduction_proxy::chrome_proxy_content_transform_header()),
      blink::WebString::FromUTF8(
          data_reduction_proxy::empty_image_directive()));
  std::map<std::string, std::string> properties;
  client.AddImageContextMenuProperties(
      web_url_response, /*is_image_in_context_a_placeholder_image=*/false,
      &properties);
  EXPECT_EQ(
      data_reduction_proxy::empty_image_directive(),
      properties
          [data_reduction_proxy::chrome_proxy_content_transform_header()]);
}

TEST_F(ChromeContentRendererClientTest,
       AddImageContextMenuPropertiesForPlaceholder) {
  ChromeContentRendererClient client;
  std::map<std::string, std::string> properties;
  client.AddImageContextMenuProperties(
      blink::WebURLResponse(), /*is_image_in_context_a_placeholder_image=*/true,
      &properties);
  EXPECT_EQ(
      data_reduction_proxy::empty_image_directive(),
      properties
          [data_reduction_proxy::chrome_proxy_content_transform_header()]);
}

TEST_F(ChromeContentRendererClientTest, RewriteEmbed) {
  struct TestData {
    std::string original;
    std::string expected;
  } test_data[] = {
      // { original, expected }
      {"youtube.com", ""},
      {"www.youtube.com", ""},
      {"http://www.youtube.com", ""},
      {"https://www.youtube.com", ""},
      {"http://www.foo.youtube.com", ""},
      {"https://www.foo.youtube.com", ""},
      // Non-YouTube domains shouldn't be modified
      {"http://www.plus.google.com", ""},
      // URL isn't using Flash
      {"http://www.youtube.com/embed/deadbeef", ""},
      // URL isn't using Flash, no www
      {"http://youtube.com/embed/deadbeef", ""},
      // URL isn't using Flash, invalid parameter construct
      {"http://www.youtube.com/embed/deadbeef&start=4", ""},
      // URL is using Flash, no www
      {"http://youtube.com/v/deadbeef", "http://youtube.com/embed/deadbeef"},
      // URL is using Flash, is valid, https
      {"https://www.youtube.com/v/deadbeef",
       "https://www.youtube.com/embed/deadbeef"},
      // URL is using Flash, is valid, http
      {"http://www.youtube.com/v/deadbeef",
       "http://www.youtube.com/embed/deadbeef"},
      // URL is using Flash, is valid, not a complete URL, no www or protocol
      {"youtube.com/v/deadbeef", ""},
      // URL is using Flash, is valid, not a complete URL,or protocol
      {"www.youtube.com/v/deadbeef", ""},
      // URL is using Flash, valid
      {"https://www.foo.youtube.com/v/deadbeef",
       "https://www.foo.youtube.com/embed/deadbeef"},
      // URL is using Flash, is valid, has one parameter
      {"http://www.youtube.com/v/deadbeef?start=4",
       "http://www.youtube.com/embed/deadbeef?start=4"},
      // URL is using Flash, is valid, has multiple parameters
      {"http://www.youtube.com/v/deadbeef?start=4&fs=1",
       "http://www.youtube.com/embed/deadbeef?start=4&fs=1"},
      // URL is using Flash, invalid parameter construct, has one parameter
      {"http://www.youtube.com/v/deadbeef&start=4",
       "http://www.youtube.com/embed/deadbeef?start=4"},
      // URL is using Flash, invalid parameter construct, has multiple
      // parameters
      {"http://www.youtube.com/v/deadbeef&start=4&fs=1?foo=bar",
       "http://www.youtube.com/embed/deadbeef?start=4&fs=1&foo=bar"},
      // URL is using Flash, invalid parameter construct, has multiple
      // parameters
      {"http://www.youtube.com/v/deadbeef&start=4&fs=1",
       "http://www.youtube.com/embed/deadbeef?start=4&fs=1"},
      // Invalid parameter construct
      {"http://www.youtube.com/abcd/v/deadbeef", ""},
      // Invalid parameter construct
      {"http://www.youtube.com/v/abcd/", "http://www.youtube.com/embed/abcd/"},
      // Invalid parameter construct
      {"http://www.youtube.com/v/123/", "http://www.youtube.com/embed/123/"},
      // youtube-nocookie.com
      {"http://www.youtube-nocookie.com/v/123/",
       "http://www.youtube-nocookie.com/embed/123/"},
      // youtube-nocookie.com, isn't using flash
      {"http://www.youtube-nocookie.com/embed/123/", ""},
      // youtube-nocookie.com, has one parameter
      {"http://www.youtube-nocookie.com/v/123?start=foo",
       "http://www.youtube-nocookie.com/embed/123?start=foo"},
      // youtube-nocookie.com, has multiple parameters
      {"http://www.youtube-nocookie.com/v/123?start=foo&bar=baz",
       "http://www.youtube-nocookie.com/embed/123?start=foo&bar=baz"},
      // youtube-nocookie.com, invalid parameter construct, has one parameter
      {"http://www.youtube-nocookie.com/v/123&start=foo",
       "http://www.youtube-nocookie.com/embed/123?start=foo"},
      // youtube-nocookie.com, invalid parameter construct, has multiple
      // parameters
      {"http://www.youtube-nocookie.com/v/123&start=foo&bar=baz",
       "http://www.youtube-nocookie.com/embed/123?start=foo&bar=baz"},
      // youtube-nocookie.com, https
      {"https://www.youtube-nocookie.com/v/123/",
       "https://www.youtube-nocookie.com/embed/123/"},
      // URL isn't using Flash, has JS API enabled
      {"http://www.youtube.com/embed/deadbeef?enablejsapi=1", ""},
      // URL is using Flash, has JS API enabled
      {"http://www.youtube.com/v/deadbeef?enablejsapi=1",
       "http://www.youtube.com/embed/deadbeef?enablejsapi=1"},
      // youtube-nocookie.com, has JS API enabled
      {"http://www.youtube-nocookie.com/v/123?enablejsapi=1",
       "http://www.youtube-nocookie.com/embed/123?enablejsapi=1"},
      // URL is using Flash, has JS API enabled, invalid parameter construct
      {"http://www.youtube.com/v/deadbeef&enablejsapi=1",
       "http://www.youtube.com/embed/deadbeef?enablejsapi=1"},
      // URL is using Flash, has JS API enabled, invalid parameter construct,
      // has multiple parameters
      {"http://www.youtube.com/v/deadbeef&start=4&enablejsapi=1",
       "http://www.youtube.com/embed/deadbeef?start=4&enablejsapi=1"},
  };

  ChromeContentRendererClient client;

  for (const auto& data : test_data)
    EXPECT_EQ(GURL(data.expected),
              client.OverrideFlashEmbedWithHTML(GURL(data.original)));
}

class ChromeContentRendererClientMetricsTest : public testing::Test {
 public:
  ChromeContentRendererClientMetricsTest() = default;

  std::unique_ptr<base::HistogramSamples> GetHistogramSamples() {
    return histogram_tester_.GetHistogramSamplesSinceCreation(
        internal::kFlashYouTubeRewriteUMA);
  }

  void OverrideFlashEmbed(const GURL& gurl) {
    client_.OverrideFlashEmbedWithHTML(gurl);
  }

 private:
  ChromeContentRendererClient client_;
  base::HistogramTester histogram_tester_;

  DISALLOW_COPY_AND_ASSIGN(ChromeContentRendererClientMetricsTest);
};

TEST_F(ChromeContentRendererClientMetricsTest, RewriteEmbedIneligibleURL) {
  std::unique_ptr<base::HistogramSamples> samples = GetHistogramSamples();
  EXPECT_EQ(0, samples->TotalCount());

  const std::string test_data[] = {
      // HTTP, www, no flash
      "http://www.youtube.com",
      // No flash, subdomain
      "http://www.foo.youtube.com",
      // No flash
      "youtube.com",
      // Not youtube
      "http://www.plus.google.com",
      // Already using HTML5
      "http://youtube.com/embed/deadbeef",
      // Already using HTML5, enablejsapi=1
      "http://www.youtube.com/embed/deadbeef?enablejsapi=1"};

  for (const auto& data : test_data) {
    GURL gurl = GURL(data);
    OverrideFlashEmbed(gurl);
    samples = GetHistogramSamples();
    EXPECT_EQ(0, samples->GetCount(internal::SUCCESS));
    EXPECT_EQ(0, samples->TotalCount());
  }
}

TEST_F(ChromeContentRendererClientMetricsTest, RewriteEmbedSuccess) {
  ChromeContentRendererClient client;

  std::unique_ptr<base::HistogramSamples> samples = GetHistogramSamples();
  auto total_count = 0;
  EXPECT_EQ(total_count, samples->TotalCount());

  const std::string test_data[] = {
      // HTTP, www, flash
      "http://www.youtube.com/v/deadbeef",
      // HTTP, no www, flash
      "http://youtube.com/v/deadbeef",
      // HTTPS, www, flash
      "https://www.youtube.com/v/deadbeef",
      // HTTPS, no www, flash
      "https://youtube.com/v/deadbeef",
      // Invalid parameter construct
      "http://www.youtube.com/v/abcd/",
      // Invalid parameter construct
      "http://www.youtube.com/v/1234/",
  };

  for (const auto& data : test_data) {
    ++total_count;
    GURL gurl = GURL(data);
    OverrideFlashEmbed(gurl);
    samples = GetHistogramSamples();
    EXPECT_EQ(total_count, samples->GetCount(internal::SUCCESS));
    EXPECT_EQ(total_count, samples->TotalCount());
  }

  // Invalid parameter construct
  GURL gurl = GURL("http://www.youtube.com/abcd/v/deadbeef");
  samples = GetHistogramSamples();
  EXPECT_EQ(total_count, samples->GetCount(internal::SUCCESS));
  EXPECT_EQ(total_count, samples->TotalCount());
}

TEST_F(ChromeContentRendererClientMetricsTest, RewriteEmbedSuccessRewrite) {
  ChromeContentRendererClient client;

  std::unique_ptr<base::HistogramSamples> samples = GetHistogramSamples();
  auto total_count = 0;
  EXPECT_EQ(total_count, samples->TotalCount());

  const std::string test_data[] = {
      // Invalid parameter construct, one parameter
      "http://www.youtube.com/v/deadbeef&start=4",
      // Invalid parameter construct, has multiple parameters
      "http://www.youtube.com/v/deadbeef&start=4&fs=1?foo=bar",
  };

  for (const auto& data : test_data) {
    ++total_count;
    GURL gurl = GURL(data);
    client.OverrideFlashEmbedWithHTML(gurl);
    samples = GetHistogramSamples();
    EXPECT_EQ(total_count, samples->GetCount(internal::SUCCESS_PARAMS_REWRITE));
    EXPECT_EQ(total_count, samples->TotalCount());
  }

  // Invalid parameter construct, not flash
  GURL gurl = GURL("http://www.youtube.com/embed/deadbeef&start=4");
  OverrideFlashEmbed(gurl);
  samples = GetHistogramSamples();
  EXPECT_EQ(total_count, samples->GetCount(internal::SUCCESS_PARAMS_REWRITE));
  EXPECT_EQ(total_count, samples->TotalCount());
}

TEST_F(ChromeContentRendererClientMetricsTest, RewriteEmbedJSAPI) {
  ChromeContentRendererClient client;

  std::unique_ptr<base::HistogramSamples> samples = GetHistogramSamples();
  auto total_count = 0;
  EXPECT_EQ(total_count, samples->TotalCount());

  const std::string test_data[] = {
      // Valid parameter construct, one parameter
      "http://www.youtube.com/v/deadbeef?enablejsapi=1",
      // Valid parameter construct, has multiple parameters
      "http://www.youtube.com/v/deadbeef?enablejsapi=1&foo=2",
      // Invalid parameter construct, one parameter
      "http://www.youtube.com/v/deadbeef&enablejsapi=1",
      // Invalid parameter construct, has multiple parameters
      "http://www.youtube.com/v/deadbeef&enablejsapi=1&foo=2"};

  for (const auto& data : test_data) {
    ++total_count;
    GURL gurl = GURL(data);
    OverrideFlashEmbed(gurl);
    samples = GetHistogramSamples();
    EXPECT_EQ(total_count, samples->GetCount(internal::SUCCESS_ENABLEJSAPI));
    EXPECT_EQ(total_count, samples->TotalCount());
  }
}

