// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cctype>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/test/histogram_tester.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/metrics/subprocess_metrics_provider.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/public/test/url_loader_interceptor.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_request_headers.h"
#include "net/nqe/effective_connection_type.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "services/network/public/cpp/cors/cors.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/network_switches.h"
#include "third_party/blink/public/common/client_hints/client_hints.h"

namespace {

// An interceptor that records count of fetches and client hint headers for
// requests to https://foo.com/non-existing-image.jpg.
class ThirdPartyURLLoaderInterceptor {
 public:
  explicit ThirdPartyURLLoaderInterceptor(const GURL intercepted_url)
      : intercepted_url_(intercepted_url),
        interceptor_(base::BindRepeating(
            &ThirdPartyURLLoaderInterceptor::InterceptURLRequest,
            base::Unretained(this))) {}

  ~ThirdPartyURLLoaderInterceptor() = default;

  size_t request_count_seen() const { return request_count_seen_; }

  size_t client_hints_count_seen() const { return client_hints_count_seen_; }

 private:
  bool InterceptURLRequest(
      content::URLLoaderInterceptor::RequestParams* params) {
    if (params->url_request.url != intercepted_url_)
      return false;

    request_count_seen_++;
    for (size_t i = 0; i < blink::kClientHintsHeaderMappingCount; ++i) {
      if (params->url_request.headers.HasHeader(
              blink::kClientHintsHeaderMapping[i])) {
        client_hints_count_seen_++;
      }
    }
    return false;
  }

  GURL intercepted_url_;

  size_t request_count_seen_ = 0u;

  size_t client_hints_count_seen_ = 0u;

  content::URLLoaderInterceptor interceptor_;

  DISALLOW_COPY_AND_ASSIGN(ThirdPartyURLLoaderInterceptor);
};

// Returns true only if |header_value| satisfies ABNF: 1*DIGIT [ "." 1*DIGIT ]
bool IsSimilarToDoubleABNF(const std::string& header_value) {
  if (header_value.empty())
    return false;
  char first_char = header_value.at(0);
  if (!isdigit(first_char))
    return false;

  bool period_found = false;
  bool digit_found_after_period = false;
  for (char ch : header_value) {
    if (isdigit(ch)) {
      if (period_found) {
        digit_found_after_period = true;
      }
      continue;
    }
    if (ch == '.') {
      if (period_found)
        return false;
      period_found = true;
      continue;
    }
    return false;
  }
  if (period_found)
    return digit_found_after_period;
  return true;
}

// Returns true only if |header_value| satisfies ABNF: 1*DIGIT
bool IsSimilarToIntABNF(const std::string& header_value) {
  if (header_value.empty())
    return false;

  for (char ch : header_value) {
    if (!isdigit(ch))
      return false;
  }
  return true;
}

}  // namespace

class ClientHintsBrowserTest : public InProcessBrowserTest {
 public:
  ClientHintsBrowserTest()
      : http_server_(net::EmbeddedTestServer::TYPE_HTTP),
        https_server_(net::EmbeddedTestServer::TYPE_HTTPS),
        expect_client_hints_on_main_frame_(false),
        expect_client_hints_on_subresources_(false),
        count_client_hints_headers_seen_(0),
        request_interceptor_(nullptr) {
    http_server_.ServeFilesFromSourceDirectory("chrome/test/data/client_hints");
    https_server_.ServeFilesFromSourceDirectory(
        "chrome/test/data/client_hints");

    http_server_.RegisterRequestMonitor(
        base::Bind(&ClientHintsBrowserTest::MonitorResourceRequest,
                   base::Unretained(this)));
    https_server_.RegisterRequestMonitor(
        base::Bind(&ClientHintsBrowserTest::MonitorResourceRequest,
                   base::Unretained(this)));

    EXPECT_TRUE(http_server_.Start());
    EXPECT_TRUE(https_server_.Start());

    accept_ch_with_lifetime_http_local_url_ =
        http_server_.GetURL("/accept_ch_with_lifetime.html");
    EXPECT_TRUE(accept_ch_with_lifetime_http_local_url_.SchemeIsHTTPOrHTTPS());
    EXPECT_FALSE(
        accept_ch_with_lifetime_http_local_url_.SchemeIsCryptographic());

    accept_ch_with_lifetime_url_ =
        https_server_.GetURL("/accept_ch_with_lifetime.html");
    EXPECT_TRUE(accept_ch_with_lifetime_url_.SchemeIsHTTPOrHTTPS());
    EXPECT_TRUE(accept_ch_with_lifetime_url_.SchemeIsCryptographic());

    accept_ch_without_lifetime_url_ =
        https_server_.GetURL("/accept_ch_without_lifetime.html");
    EXPECT_TRUE(accept_ch_with_lifetime_url_.SchemeIsHTTPOrHTTPS());
    EXPECT_TRUE(accept_ch_with_lifetime_url_.SchemeIsCryptographic());

    without_accept_ch_without_lifetime_url_ =
        https_server_.GetURL("/without_accept_ch_without_lifetime.html");
    EXPECT_TRUE(without_accept_ch_without_lifetime_url_.SchemeIsHTTPOrHTTPS());
    EXPECT_TRUE(
        without_accept_ch_without_lifetime_url_.SchemeIsCryptographic());

    without_accept_ch_without_lifetime_local_url_ =
        http_server_.GetURL("/without_accept_ch_without_lifetime.html");
    EXPECT_TRUE(
        without_accept_ch_without_lifetime_local_url_.SchemeIsHTTPOrHTTPS());
    EXPECT_FALSE(
        without_accept_ch_without_lifetime_local_url_.SchemeIsCryptographic());

    without_accept_ch_without_lifetime_img_localhost_ = https_server_.GetURL(
        "/without_accept_ch_without_lifetime_img_localhost.html");
    without_accept_ch_without_lifetime_img_foo_com_ = https_server_.GetURL(
        "/without_accept_ch_without_lifetime_img_foo_com.html");
    accept_ch_without_lifetime_with_iframe_url_ =
        https_server_.GetURL("/accept_ch_without_lifetime_with_iframe.html");
    accept_ch_without_lifetime_img_localhost_ =
        https_server_.GetURL("/accept_ch_without_lifetime_img_localhost.html");
  }

  ~ClientHintsBrowserTest() override {}

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");

    request_interceptor_ = std::make_unique<ThirdPartyURLLoaderInterceptor>(
        GURL("https://foo.com/non-existing-image.jpg"));
    base::RunLoop().RunUntilIdle();
  }

  void TearDownOnMainThread() override { request_interceptor_.reset(); }

  void SetUpCommandLine(base::CommandLine* cmd) override {
    cmd->AppendSwitch(switches::kEnableExperimentalWebPlatformFeatures);
    cmd->AppendSwitchASCII(network::switches::kForceEffectiveConnectionType,
                           net::kEffectiveConnectionType2G);
  }

  void SetClientHintExpectationsOnMainFrame(bool expect_client_hints) {
    expect_client_hints_on_main_frame_ = expect_client_hints;
  }

  void SetClientHintExpectationsOnSubresources(bool expect_client_hints) {
    expect_client_hints_on_subresources_ = expect_client_hints;
  }

  // Verify that the user is not notified that cookies or JavaScript were
  // blocked on the webpage due to the checks done by client hints.
  void VerifyContentSettingsNotNotified() const {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    EXPECT_FALSE(TabSpecificContentSettings::FromWebContents(web_contents)
                     ->IsContentBlocked(CONTENT_SETTINGS_TYPE_COOKIES));

    EXPECT_FALSE(TabSpecificContentSettings::FromWebContents(web_contents)
                     ->IsContentBlocked(CONTENT_SETTINGS_TYPE_JAVASCRIPT));
  }

  const GURL& accept_ch_with_lifetime_http_local_url() const {
    return accept_ch_with_lifetime_http_local_url_;
  }

  // A URL whose response headers include Accept-CH and Accept-CH-Lifetime
  // headers.
  const GURL& accept_ch_with_lifetime_url() const {
    return accept_ch_with_lifetime_url_;
  }

  // A URL whose response headers include only Accept-CH header.
  const GURL& accept_ch_without_lifetime_url() const {
    return accept_ch_without_lifetime_url_;
  }

  // A URL whose response headers do not include either Accept-CH or
  // Accept-CH-Lifetime headers. Navigating to this URL also fetches an image.
  const GURL& without_accept_ch_without_lifetime_url() const {
    return without_accept_ch_without_lifetime_url_;
  }

  // A URL whose response headers do not include either Accept-CH or
  // Accept-CH-Lifetime headers. Navigating to this URL also fetches an image.
  const GURL& without_accept_ch_without_lifetime_local_url() const {
    return without_accept_ch_without_lifetime_local_url_;
  }

  // A URL whose response headers do not include either Accept-CH or
  // Accept-CH-Lifetime headers. Navigating to this URL also fetches an image
  // from localhost.
  const GURL& without_accept_ch_without_lifetime_img_localhost() const {
    return without_accept_ch_without_lifetime_img_localhost_;
  }

  // A URL whose response headers do not include either Accept-CH or
  // Accept-CH-Lifetime headers. Navigating to this URL also fetches an image
  // from foo.com.
  const GURL& without_accept_ch_without_lifetime_img_foo_com() const {
    return without_accept_ch_without_lifetime_img_foo_com_;
  }

  // A URL whose response does not include Accept-CH or Accept-CH-Lifetime
  // headers. The response loads accept_ch_with_lifetime_url() in an iframe.
  const GURL& accept_ch_without_lifetime_with_iframe_url() const {
    return accept_ch_without_lifetime_with_iframe_url_;
  }

  // A URL whose response headers includes only Accept-CH header. Navigating to
  // this URL also fetches two images: One from the localhost, and one from
  // foo.com.
  const GURL& accept_ch_without_lifetime_img_localhost() const {
    return accept_ch_without_lifetime_img_localhost_;
  }

  size_t count_client_hints_headers_seen() const {
    return count_client_hints_headers_seen_;
  }

  size_t third_party_request_count_seen() const {
    return request_interceptor_->request_count_seen();
  }

  size_t third_party_client_hints_count_seen() const {
    return request_interceptor_->client_hints_count_seen();
  }

  base::test::ScopedFeatureList scoped_feature_list_;

 private:
  // Called by |https_server_|.
  void MonitorResourceRequest(const net::test_server::HttpRequest& request) {
    bool is_main_frame_navigation =
        request.GetURL().spec().find(".html") != std::string::npos;

    if (is_main_frame_navigation) {
      VerifyClientHintsReceived(expect_client_hints_on_main_frame_, request);
      if (expect_client_hints_on_main_frame_) {
        double value = 0.0;
        EXPECT_TRUE(base::StringToDouble(
            request.headers.find("device-memory")->second, &value));
        EXPECT_LT(0.0, value);
        EXPECT_TRUE(IsSimilarToDoubleABNF(
            request.headers.find("device-memory")->second));
        main_frame_device_memory_observed_ = value;

        EXPECT_TRUE(
            base::StringToDouble(request.headers.find("dpr")->second, &value));
        EXPECT_LT(0.0, value);
        EXPECT_TRUE(IsSimilarToDoubleABNF(request.headers.find("dpr")->second));
        main_frame_dpr_observed_ = value;

        EXPECT_TRUE(base::StringToDouble(
            request.headers.find("viewport-width")->second, &value));
        EXPECT_TRUE(
            IsSimilarToIntABNF(request.headers.find("viewport-width")->second));
#if !defined(OS_ANDROID)
        EXPECT_LT(0.0, value);
#else
        EXPECT_EQ(980, value);
#endif
        main_frame_viewport_width_observed_ = value;
        VerifyNetworkQualityClientHints(request);
      }
    }

    if (!is_main_frame_navigation) {
      VerifyClientHintsReceived(expect_client_hints_on_subresources_, request);

      if (expect_client_hints_on_subresources_) {
        double value = 0.0;
        EXPECT_TRUE(base::StringToDouble(
            request.headers.find("device-memory")->second, &value));
        EXPECT_LT(0.0, value);
        EXPECT_TRUE(IsSimilarToDoubleABNF(
            request.headers.find("device-memory")->second));
        if (main_frame_device_memory_observed_ > 0) {
          EXPECT_EQ(main_frame_device_memory_observed_, value);
        }

        EXPECT_TRUE(
            base::StringToDouble(request.headers.find("dpr")->second, &value));
        EXPECT_LT(0.0, value);
        EXPECT_TRUE(IsSimilarToDoubleABNF(request.headers.find("dpr")->second));
        if (main_frame_dpr_observed_ > 0) {
          EXPECT_EQ(main_frame_dpr_observed_, value);
        }

        EXPECT_TRUE(base::StringToDouble(
            request.headers.find("viewport-width")->second, &value));
        EXPECT_TRUE(
            IsSimilarToIntABNF(request.headers.find("viewport-width")->second));
#if !defined(OS_ANDROID)
        EXPECT_LT(0.0, value);
#else
        EXPECT_EQ(980, value);
#endif
#if defined(OS_ANDROID)
        // TODO(tbansal): https://crbug.com/825892: Viewport width on main
        // frame requests may be incorrect when the Chrome window is not
        // maximized.
        if (main_frame_viewport_width_observed_ > 0) {
          EXPECT_EQ(main_frame_viewport_width_observed_, value);
        }
#endif
        VerifyNetworkQualityClientHints(request);
      }
    }

    for (size_t i = 0; i < blink::kClientHintsHeaderMappingCount; ++i) {
      if (base::ContainsKey(request.headers,
                            blink::kClientHintsHeaderMapping[i])) {
        count_client_hints_headers_seen_++;
      }
    }
  }

  void VerifyClientHintsReceived(bool expect_client_hints,
                                 const net::test_server::HttpRequest& request) {
    for (size_t i = 0; i < blink::kClientHintsHeaderMappingCount; ++i) {
      // Resource width client hint is only attached on image subresources.
      if (std::string(blink::kClientHintsHeaderMapping[i]) == "width") {
        continue;
      }
      EXPECT_EQ(expect_client_hints,
                base::ContainsKey(request.headers,
                                  blink::kClientHintsHeaderMapping[i]));
    }
  }

  void VerifyNetworkQualityClientHints(
      const net::test_server::HttpRequest& request) const {
    // Effective connection type is forced to 2G using command line in these
    // tests.
    int rtt_value = 0.0;
    EXPECT_TRUE(
        base::StringToInt(request.headers.find("rtt")->second, &rtt_value));
    EXPECT_LE(0, rtt_value);
    EXPECT_TRUE(IsSimilarToIntABNF(request.headers.find("rtt")->second));
    // Verify that RTT value is a multiple of 50 milliseconds.
    EXPECT_EQ(0, rtt_value % 50);
    EXPECT_GE(3000, rtt_value);

    double mbps_value = 0.0;
    EXPECT_TRUE(base::StringToDouble(request.headers.find("downlink")->second,
                                     &mbps_value));
    EXPECT_LE(0, mbps_value);
    EXPECT_TRUE(
        IsSimilarToDoubleABNF(request.headers.find("downlink")->second));
    // Verify that the mbps value is a multiple of 0.050 mbps.
    // Allow for small amount of noise due to double to integer conversions.
    EXPECT_NEAR(0, (static_cast<int>(mbps_value * 1000)) % 50, 1);
    EXPECT_GE(10.0, mbps_value);

    EXPECT_FALSE(request.headers.find("ect")->second.empty());

    // TODO(tbansal): https://crbug.com/819244: When network servicification is
    // enabled, the UI thread NQE observers do not receive notifications on
    // change in the network quality.
    if (!base::FeatureList::IsEnabled(network::features::kNetworkService)) {
      // Effective connection type is forced to 2G using command line in these
      // tests. RTT is expected to be 1800 msec but leave some gap to account
      // for added noise and randomization.
      EXPECT_NEAR(1800, rtt_value, 360);

      // Effective connection type is forced to 2G using command line in these
      // tests. downlink is expected to be 0.075 Mbps but leave some gap to
      // account for added noise and randomization.
      EXPECT_NEAR(0.075, mbps_value, 0.05);

      EXPECT_EQ("2g", request.headers.find("ect")->second);
    }
  }

  net::EmbeddedTestServer http_server_;
  net::EmbeddedTestServer https_server_;
  GURL accept_ch_with_lifetime_http_local_url_;
  GURL accept_ch_with_lifetime_url_;
  GURL accept_ch_without_lifetime_url_;
  GURL without_accept_ch_without_lifetime_url_;
  GURL without_accept_ch_without_lifetime_local_url_;
  GURL accept_ch_without_lifetime_with_iframe_url_;
  GURL without_accept_ch_without_lifetime_img_foo_com_;
  GURL without_accept_ch_without_lifetime_img_localhost_;
  GURL accept_ch_without_lifetime_img_localhost_;

  double main_frame_dpr_observed_ = -1;
  double main_frame_viewport_width_observed_ = -1;
  double main_frame_device_memory_observed_ = -1;

  // Expect client hints on all the main frame request.
  bool expect_client_hints_on_main_frame_;
  // Expect client hints on all the subresource requests.
  bool expect_client_hints_on_subresources_;

  size_t count_client_hints_headers_seen_;

  std::unique_ptr<ThirdPartyURLLoaderInterceptor> request_interceptor_;

  DISALLOW_COPY_AND_ASSIGN(ClientHintsBrowserTest);
};

class ClientHintsAllowThirdPartyBrowserTest : public ClientHintsBrowserTest {
  void SetUpCommandLine(base::CommandLine* cmd) override {
    scoped_feature_list_.InitFromCommandLine("AllowClientHintsToThirdParty",
                                             "");
    ClientHintsBrowserTest::SetUpCommandLine(cmd);
  }
};

IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest, CorsChecks) {
  for (size_t i = 0; i < blink::kClientHintsHeaderMappingCount; ++i) {
    // Do not test for headers that have not been enabled on the blink "stable"
    // yet.
    if (std::string(blink::kClientHintsHeaderMapping[i]) == "rtt" ||
        std::string(blink::kClientHintsHeaderMapping[i]) == "downlink" ||
        std::string(blink::kClientHintsHeaderMapping[i]) == "ect") {
      continue;
    }
    EXPECT_TRUE(network::cors::IsCORSSafelistedHeader(
        blink::kClientHintsHeaderMapping[i], "42" /* value */));
  }
  EXPECT_FALSE(network::cors::IsCORSSafelistedHeader("not-a-client-hint-header",
                                                     "" /* value */));
  EXPECT_TRUE(
      network::cors::IsCORSSafelistedHeader("save-data", "on" /* value */));
}

// Loads a webpage that requests persisting of client hints. Verifies that
// the browser receives the mojo notification from the renderer and persists the
// client hints to the disk.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest, ClientHintsHttps) {
  base::HistogramTester histogram_tester;
  ui_test_utils::NavigateToURL(browser(), accept_ch_with_lifetime_url());

  histogram_tester.ExpectUniqueSample("ClientHints.UpdateEventCount", 1, 1);

  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  // client_hints_url() sets six client hints.
  histogram_tester.ExpectUniqueSample("ClientHints.UpdateSize", 6, 1);
  // accept_ch_with_lifetime_url() sets client hints persist duration to 3600
  // seconds.
  histogram_tester.ExpectUniqueSample("ClientHints.PersistDuration",
                                      3600 * 1000, 1);
}

// Test that client hints are attached to subresources only if they belong
// to the same host as document host.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       ClientHintsHttpsSubresourceDifferentOrigin) {
  base::HistogramTester histogram_tester;

  // Add client hints for the embedded test server.
  ui_test_utils::NavigateToURL(browser(), accept_ch_with_lifetime_url());
  histogram_tester.ExpectUniqueSample("ClientHints.UpdateEventCount", 1, 1);

  // Verify that the client hints settings for localhost have been saved.
  ContentSettingsForOneType client_hints_settings;
  HostContentSettingsMap* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  host_content_settings_map->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
      &client_hints_settings);
  ASSERT_EQ(1U, client_hints_settings.size());

  // Copy the client hints setting for localhost to foo.com.
  host_content_settings_map->SetWebsiteSettingDefaultScope(
      GURL("https://foo.com/"), GURL(), CONTENT_SETTINGS_TYPE_CLIENT_HINTS,
      std::string(),
      std::make_unique<base::Value>(
          client_hints_settings.at(0).setting_value.Clone()));

  // Verify that client hints for the two hosts has been saved.
  host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  host_content_settings_map->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
      &client_hints_settings);
  ASSERT_EQ(2U, client_hints_settings.size());

  // Navigating to without_accept_ch_without_lifetime_img_localhost() should
  // attach client hints to the image subresouce contained in that page since
  // the image is located on the same server as the document origin.
  SetClientHintExpectationsOnMainFrame(true);
  SetClientHintExpectationsOnSubresources(true);
  ui_test_utils::NavigateToURL(
      browser(), without_accept_ch_without_lifetime_img_localhost());
  base::RunLoop().RunUntilIdle();
  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  // Six client hints are attached to the image request, and six to the main
  // frame request.
  EXPECT_EQ(12u, count_client_hints_headers_seen());

  // Navigating to without_accept_ch_without_lifetime_img_foo_com() should not
  // attach client hints to the image subresouce contained in that page since
  // the image is located on a different server as the document origin.
  ui_test_utils::NavigateToURL(
      browser(), without_accept_ch_without_lifetime_img_foo_com());
  base::RunLoop().RunUntilIdle();
  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  // The device-memory and dprheader is attached to the main frame request.
#if defined(OS_ANDROID)
  EXPECT_EQ(6u, count_client_hints_headers_seen());
#else
  EXPECT_EQ(18u, count_client_hints_headers_seen());
#endif
  // Requests to third party servers should not have client hints attached.
  EXPECT_EQ(1u, third_party_request_count_seen());
  EXPECT_EQ(0u, third_party_client_hints_count_seen());
}

// Test that client hints are attached to third party subresources if
// AllowClientHintsToThirdParty feature is enabled.
IN_PROC_BROWSER_TEST_F(ClientHintsAllowThirdPartyBrowserTest,
                       ClientHintsThirdPartyAllowed) {
  base::HistogramTester histogram_tester;

  SetClientHintExpectationsOnMainFrame(false);
  SetClientHintExpectationsOnSubresources(true);

  // Add client hints for the embedded test server.
  ui_test_utils::NavigateToURL(browser(),
                               accept_ch_without_lifetime_img_localhost());
  histogram_tester.ExpectTotalCount("ClientHints.UpdateEventCount", 0);

  EXPECT_EQ(6u, count_client_hints_headers_seen());

  // Requests to third party servers should not have client hints attached.
  EXPECT_EQ(1u, third_party_request_count_seen());

  // Device memory, viewport width and DRP client hints should be sent to the
  // third-party when feature "AllowClientHintsToThirdParty" is enabled.
  EXPECT_EQ(3u, third_party_client_hints_count_seen());
}

// Test that client hints are not attached to third party subresources if
// AllowClientHintsToThirdParty feature is not enabled.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       ClientHintsThirdPartyNotAllowed) {
  base::HistogramTester histogram_tester;

  SetClientHintExpectationsOnMainFrame(false);
  SetClientHintExpectationsOnSubresources(true);

  // Add client hints for the embedded test server.
  ui_test_utils::NavigateToURL(browser(),
                               accept_ch_without_lifetime_img_localhost());
  histogram_tester.ExpectTotalCount("ClientHints.UpdateEventCount", 0);

  EXPECT_EQ(6u, count_client_hints_headers_seen());

  // Requests to third party servers should not have client hints attached.
  EXPECT_EQ(1u, third_party_request_count_seen());

  // Client hints should not be sent to the third-party when feature
  // "AllowClientHintsToThirdParty" is not enabled.
  EXPECT_EQ(0u, third_party_client_hints_count_seen());
}

// Loads a HTTPS webpage that does not request persisting of client hints.
// An iframe loaded by the webpage requests persistence of client hints.
// Verify that the request from the iframe is not honored, and client hints
// preference is not persisted.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       DisregardPersistenceRequestIframe) {
  base::HistogramTester histogram_tester;
  ContentSettingsForOneType host_settings;

  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(0u, host_settings.size());

  ui_test_utils::NavigateToURL(browser(),
                               accept_ch_without_lifetime_with_iframe_url());

  histogram_tester.ExpectTotalCount("ClientHints.UpdateEventCount", 0);

  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  // accept_ch_without_lifetime_with_iframe_url() loads
  // accept_ch_with_lifetime() in an iframe. The request to persist client
  // hints from accept_ch_with_lifetime() should be disregarded.
  histogram_tester.ExpectTotalCount("ClientHints.UpdateSize", 0);
  histogram_tester.ExpectTotalCount("ClientHints.PersistDuration", 0);
}

// Loads a HTTP local webpage (which qualifies as a secure context) that
// requests persisting of client hints. Verifies that the browser receives the
// mojo notification from the renderer and persists the client hints to the
// disk.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       ClientHintsLifetimeFollowedByNoClientHintHttpLocal) {
  base::HistogramTester histogram_tester;
  ContentSettingsForOneType host_settings;

  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(0u, host_settings.size());

  ui_test_utils::NavigateToURL(browser(),
                               accept_ch_with_lifetime_http_local_url());

  histogram_tester.ExpectUniqueSample("ClientHints.UpdateEventCount", 1, 1);

  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  histogram_tester.ExpectUniqueSample("ClientHints.UpdateSize", 6, 1);
  // accept_ch_with_lifetime_http_local_url() sets client hints persist duration
  // to 3600 seconds.
  histogram_tester.ExpectUniqueSample("ClientHints.PersistDuration",
                                      3600 * 1000, 1);

  base::RunLoop().RunUntilIdle();

  // Clients hints preferences for one origin should be persisted.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(1u, host_settings.size());

  SetClientHintExpectationsOnMainFrame(true);
  SetClientHintExpectationsOnSubresources(true);
  ui_test_utils::NavigateToURL(browser(),
                               without_accept_ch_without_lifetime_local_url());

  // Six client hints are attached to the image request, and six to the main
  // frame request.
  EXPECT_EQ(12u, count_client_hints_headers_seen());
}

// Loads a webpage that does not request persisting of client hints.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest, NoClientHintsHttps) {
  base::HistogramTester histogram_tester;
  ui_test_utils::NavigateToURL(browser(),
                               without_accept_ch_without_lifetime_url());

  histogram_tester.ExpectTotalCount("ClientHints.UpdateEventCount", 0);

  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  // no_client_hints_url() does not sets the client hints.
  histogram_tester.ExpectTotalCount("ClientHints.UpdateSize", 0);
  histogram_tester.ExpectTotalCount("ClientHints.PersistDuration", 0);
}

IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       ClientHintsLifetimeFollowedByNoClientHint) {
  base::HistogramTester histogram_tester;
  ContentSettingsForOneType host_settings;

  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(0u, host_settings.size());

  // Fetching accept_ch_with_lifetime_url() should persist the request for
  // client hints.
  ui_test_utils::NavigateToURL(browser(), accept_ch_with_lifetime_url());

  histogram_tester.ExpectUniqueSample("ClientHints.UpdateEventCount", 1, 1);

  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  histogram_tester.ExpectUniqueSample("ClientHints.UpdateSize", 6, 1);
  // accept_ch_with_lifetime_url() sets client hints persist duration to 3600
  // seconds.
  histogram_tester.ExpectUniqueSample("ClientHints.PersistDuration",
                                      3600 * 1000, 1);
  base::RunLoop().RunUntilIdle();

  // Clients hints preferences for one origin should be persisted.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(1u, host_settings.size());

  SetClientHintExpectationsOnMainFrame(true);
  SetClientHintExpectationsOnSubresources(true);
  ui_test_utils::NavigateToURL(browser(),
                               without_accept_ch_without_lifetime_url());

  // Six client hints are attached to the image request, and six to the main
  // frame request.
  EXPECT_EQ(12u, count_client_hints_headers_seen());
}

// Ensure that when cookies are blocked, client hint preferences are not
// persisted.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       ClientHintsLifetimeNotPersistedCookiesBlocked) {
  scoped_refptr<content_settings::CookieSettings> cookie_settings_ =
      CookieSettingsFactory::GetForProfile(browser()->profile());
  base::HistogramTester histogram_tester;
  ContentSettingsForOneType host_settings;

  // Block cookies.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(accept_ch_without_lifetime_url(), GURL(),
                                      CONTENT_SETTINGS_TYPE_COOKIES,
                                      std::string(), CONTENT_SETTING_BLOCK);

  // Fetching accept_ch_with_lifetime_url() should not persist the request for
  // client hints since cookies are blocked.
  ui_test_utils::NavigateToURL(browser(), accept_ch_with_lifetime_url());
  histogram_tester.ExpectTotalCount("ClientHints.UpdateEventCount", 0);
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(0u, host_settings.size());
  VerifyContentSettingsNotNotified();

  // Allow cookies.
  cookie_settings_->SetCookieSetting(accept_ch_without_lifetime_url(),
                                     CONTENT_SETTING_ALLOW);
  // Fetching accept_ch_with_lifetime_url() should persist the request for
  // client hints since cookies are allowed.
  ui_test_utils::NavigateToURL(browser(), accept_ch_with_lifetime_url());
  histogram_tester.ExpectTotalCount("ClientHints.UpdateEventCount", 1);
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(1u, host_settings.size());
}

IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       ClientHintsLifetimeNotAttachedCookiesBlocked) {
  base::HistogramTester histogram_tester;
  ContentSettingsForOneType host_settings;

  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(0u, host_settings.size());

  // Fetching accept_ch_with_lifetime_url() should persist the request for
  // client hints.
  ui_test_utils::NavigateToURL(browser(), accept_ch_with_lifetime_url());
  histogram_tester.ExpectUniqueSample("ClientHints.UpdateEventCount", 1, 1);
  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  histogram_tester.ExpectUniqueSample("ClientHints.UpdateSize", 6, 1);
  // accept_ch_with_lifetime_url() tries to set client hints persist duration to
  // 3600 seconds.
  histogram_tester.ExpectUniqueSample("ClientHints.PersistDuration",
                                      3600 * 1000, 1);
  base::RunLoop().RunUntilIdle();

  // Clients hints preferences for one origin should be persisted.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(1u, host_settings.size());

  // Block the cookies: Client hints should not be attached.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(accept_ch_without_lifetime_url(), GURL(),
                                      CONTENT_SETTINGS_TYPE_COOKIES,
                                      std::string(), CONTENT_SETTING_BLOCK);

  ui_test_utils::NavigateToURL(browser(),
                               without_accept_ch_without_lifetime_url());
  EXPECT_EQ(0u, count_client_hints_headers_seen());
  VerifyContentSettingsNotNotified();

  // Allow the cookies: Client hints should now be attached.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(accept_ch_without_lifetime_url(), GURL(),
                                      CONTENT_SETTINGS_TYPE_COOKIES,
                                      std::string(), CONTENT_SETTING_ALLOW);

  SetClientHintExpectationsOnMainFrame(true);
  SetClientHintExpectationsOnSubresources(true);
  ui_test_utils::NavigateToURL(browser(),
                               without_accept_ch_without_lifetime_url());

  // Six client hints are attached to the image request, and six to the main
  // frame request.
  EXPECT_EQ(12u, count_client_hints_headers_seen());

  // Clear settings.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->ClearSettingsForOneType(CONTENT_SETTINGS_TYPE_COOKIES);
}

// Ensure that when the JavaScript is blocked, client hint preferences are not
// persisted.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       ClientHintsLifetimeNotPersistedJavaScriptBlocked) {
  ContentSettingsForOneType host_settings;

  // Start a navigation. This navigation makes it possible to block JavaScript
  // later.
  ui_test_utils::NavigateToURL(browser(),
                               without_accept_ch_without_lifetime_url());

  // Block the JavaScript: Client hint preferences should not be persisted.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(accept_ch_with_lifetime_url(), GURL(),
                                      CONTENT_SETTINGS_TYPE_JAVASCRIPT,
                                      std::string(), CONTENT_SETTING_BLOCK);
  ui_test_utils::NavigateToURL(browser(), accept_ch_with_lifetime_url());
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(0u, host_settings.size());
  VerifyContentSettingsNotNotified();

  // Allow the JavaScript: Client hint preferences should be persisted.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(accept_ch_with_lifetime_url(), GURL(),
                                      CONTENT_SETTINGS_TYPE_JAVASCRIPT,
                                      std::string(), CONTENT_SETTING_ALLOW);
  ui_test_utils::NavigateToURL(browser(), accept_ch_with_lifetime_url());
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(1u, host_settings.size());

  // Clear settings.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->ClearSettingsForOneType(CONTENT_SETTINGS_TYPE_JAVASCRIPT);
}

// Ensure that when the JavaScript is blocked, persisted client hints are not
// attached to the request headers.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       ClientHintsLifetimeNotAttachedJavaScriptBlocked) {
  base::HistogramTester histogram_tester;
  ContentSettingsForOneType host_settings;

  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(0u, host_settings.size());

  // Fetching accept_ch_with_lifetime_url() should persist the request for
  // client hints.
  ui_test_utils::NavigateToURL(browser(), accept_ch_with_lifetime_url());
  histogram_tester.ExpectUniqueSample("ClientHints.UpdateEventCount", 1, 1);
  content::FetchHistogramsFromChildProcesses();
  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  histogram_tester.ExpectUniqueSample("ClientHints.UpdateSize", 6, 1);
  // accept_ch_with_lifetime_url() tries to set client hints persist duration to
  // 3600 seconds.
  histogram_tester.ExpectUniqueSample("ClientHints.PersistDuration",
                                      3600 * 1000, 1);
  base::RunLoop().RunUntilIdle();

  // Clients hints preferences for one origin should be persisted.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(1u, host_settings.size());

  // Block the Javascript: Client hints should not be attached.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(without_accept_ch_without_lifetime_url(),
                                      GURL(), CONTENT_SETTINGS_TYPE_JAVASCRIPT,
                                      std::string(), CONTENT_SETTING_BLOCK);
  ui_test_utils::NavigateToURL(browser(),
                               without_accept_ch_without_lifetime_url());
  EXPECT_EQ(0u, count_client_hints_headers_seen());
  VerifyContentSettingsNotNotified();

  // Allow the Javascript: Client hints should now be attached.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(without_accept_ch_without_lifetime_url(),
                                      GURL(), CONTENT_SETTINGS_TYPE_JAVASCRIPT,
                                      std::string(), CONTENT_SETTING_ALLOW);

  SetClientHintExpectationsOnMainFrame(true);
  SetClientHintExpectationsOnSubresources(true);
  ui_test_utils::NavigateToURL(browser(),
                               without_accept_ch_without_lifetime_url());

  // Six client hints are attached to the image request, and six to the main
  // frame request.
  EXPECT_EQ(12u, count_client_hints_headers_seen());

  // Clear settings.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->ClearSettingsForOneType(CONTENT_SETTINGS_TYPE_JAVASCRIPT);
}

// Ensure that when the JavaScript is blocked, client hints requested using
// Accept-CH are not attached to the request headers for subresources.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       ClientHintsNoLifetimeScriptNotAllowed) {
  base::HistogramTester histogram_tester;
  ContentSettingsForOneType host_settings;

  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(0u, host_settings.size());

  // Block the Javascript: Client hints should not be attached.
  SetClientHintExpectationsOnSubresources(false);
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(
          accept_ch_without_lifetime_img_localhost(), GURL(),
          CONTENT_SETTINGS_TYPE_JAVASCRIPT, std::string(),
          CONTENT_SETTING_BLOCK);
  ui_test_utils::NavigateToURL(browser(),
                               accept_ch_without_lifetime_img_localhost());
  EXPECT_EQ(0u, count_client_hints_headers_seen());
  EXPECT_EQ(1u, third_party_request_count_seen());
  EXPECT_EQ(0u, third_party_client_hints_count_seen());

  // Allow the Javascript: Client hints should now be attached.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(
          accept_ch_without_lifetime_img_localhost(), GURL(),
          CONTENT_SETTINGS_TYPE_JAVASCRIPT, std::string(),
          CONTENT_SETTING_ALLOW);

  SetClientHintExpectationsOnSubresources(true);
  ui_test_utils::NavigateToURL(browser(),
                               accept_ch_without_lifetime_img_localhost());

  EXPECT_EQ(6u, count_client_hints_headers_seen());
  EXPECT_EQ(2u, third_party_request_count_seen());
  EXPECT_EQ(0u, third_party_client_hints_count_seen());
  VerifyContentSettingsNotNotified();

  // Clear settings.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->ClearSettingsForOneType(CONTENT_SETTINGS_TYPE_JAVASCRIPT);

  // Block the Javascript again: Client hints should not be attached.
  SetClientHintExpectationsOnSubresources(false);
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(
          accept_ch_without_lifetime_img_localhost(), GURL(),
          CONTENT_SETTINGS_TYPE_JAVASCRIPT, std::string(),
          CONTENT_SETTING_BLOCK);
  ui_test_utils::NavigateToURL(browser(),
                               accept_ch_without_lifetime_img_localhost());
  EXPECT_EQ(6u, count_client_hints_headers_seen());
  EXPECT_EQ(3u, third_party_request_count_seen());
  EXPECT_EQ(0u, third_party_client_hints_count_seen());

  // Clear settings.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->ClearSettingsForOneType(CONTENT_SETTINGS_TYPE_JAVASCRIPT);
}

// Ensure that when the cookies is blocked, client hints are not attached to the
// request headers.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest,
                       ClientHintsNoLifetimeCookiesNotAllowed) {
  base::HistogramTester histogram_tester;
  ContentSettingsForOneType host_settings;
  scoped_refptr<content_settings::CookieSettings> cookie_settings_ =
      CookieSettingsFactory::GetForProfile(browser()->profile());

  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
                              &host_settings);
  EXPECT_EQ(0u, host_settings.size());

  // Block cookies.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(
          accept_ch_without_lifetime_img_localhost(), GURL(),
          CONTENT_SETTINGS_TYPE_COOKIES, std::string(), CONTENT_SETTING_BLOCK);
  base::RunLoop().RunUntilIdle();

  ui_test_utils::NavigateToURL(browser(),
                               accept_ch_without_lifetime_img_localhost());
  EXPECT_EQ(0u, count_client_hints_headers_seen());
  // Client hints are not attached to third party subresources even though
  // cookies are allowed only for the first party origin.
  EXPECT_EQ(0u, third_party_client_hints_count_seen());
  VerifyContentSettingsNotNotified();

  // Allow cookies.
  cookie_settings_->SetCookieSetting(accept_ch_without_lifetime_img_localhost(),
                                     CONTENT_SETTING_ALLOW);
  base::RunLoop().RunUntilIdle();

  SetClientHintExpectationsOnSubresources(true);
  ui_test_utils::NavigateToURL(browser(),
                               accept_ch_without_lifetime_img_localhost());
  EXPECT_EQ(6u, count_client_hints_headers_seen());
  EXPECT_EQ(2u, third_party_request_count_seen());
  EXPECT_EQ(0u, third_party_client_hints_count_seen());

  // Block cookies again.
  SetClientHintExpectationsOnSubresources(false);
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->SetContentSettingDefaultScope(
          accept_ch_without_lifetime_img_localhost(), GURL(),
          CONTENT_SETTINGS_TYPE_COOKIES, std::string(), CONTENT_SETTING_BLOCK);
  base::RunLoop().RunUntilIdle();

  ui_test_utils::NavigateToURL(browser(),
                               accept_ch_without_lifetime_img_localhost());
  EXPECT_EQ(6u, count_client_hints_headers_seen());
  EXPECT_EQ(3u, third_party_request_count_seen());
  EXPECT_EQ(0u, third_party_client_hints_count_seen());

  // Clear settings.
  HostContentSettingsMapFactory::GetForProfile(browser()->profile())
      ->ClearSettingsForOneType(CONTENT_SETTINGS_TYPE_COOKIES);
}

// Check the client hints for the given URL in an incognito window.
// Start incognito browser twice to ensure that client hints prefs are
// not carried over.
IN_PROC_BROWSER_TEST_F(ClientHintsBrowserTest, ClientHintsHttpsIncognito) {
  for (size_t i = 0; i < 2; ++i) {
    base::HistogramTester histogram_tester;

    Browser* incognito = CreateIncognitoBrowser();
    ui_test_utils::NavigateToURL(incognito, accept_ch_with_lifetime_url());

    histogram_tester.ExpectUniqueSample("ClientHints.UpdateEventCount", 1, 1);

    content::FetchHistogramsFromChildProcesses();
    SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

    // accept_ch_with_lifetime_url() sets six client hints.
    histogram_tester.ExpectUniqueSample("ClientHints.UpdateSize", 6, 1);

    // At least one renderer must have been created. All the renderers created
    // must have read 0 client hints.
    EXPECT_LE(1u,
              histogram_tester.GetAllSamples("ClientHints.CountRulesReceived")
                  .size());
    for (const auto& bucket :
         histogram_tester.GetAllSamples("ClientHints.CountRulesReceived")) {
      EXPECT_EQ(0, bucket.min);
    }
    // |url| sets client hints persist duration to 3600 seconds.
    histogram_tester.ExpectUniqueSample("ClientHints.PersistDuration",
                                        3600 * 1000, 1);

    CloseBrowserSynchronously(incognito);
  }
}
