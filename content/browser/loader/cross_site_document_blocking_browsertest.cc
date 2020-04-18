// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ostream>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/strings/pattern.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "content/browser/loader/cross_site_document_resource_handler.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/resource_type.h"
#include "content/public/common/web_preferences.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "content/public/test/url_loader_interceptor.h"
#include "content/shell/browser/shell.h"
#include "content/test/test_content_browser_client.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"
#include "services/network/test/test_url_loader_client.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace content {

using testing::Not;
using testing::HasSubstr;

namespace {

enum HistogramExpectations {
  kShouldBeBlocked = 1 << 0,
  kShouldBeSniffed = 1 << 1,
  kShouldHaveContentLength = 1 << 2,

  kShouldBeAllowedWithoutSniffing = 0,
  kShouldBeBlockedWithoutSniffing = kShouldBeBlocked,
  kShouldBeSniffedAndAllowed = kShouldBeSniffed,
  kShouldBeSniffedAndBlocked = kShouldBeSniffed | kShouldBeBlocked,
};

HistogramExpectations operator|(HistogramExpectations a,
                                HistogramExpectations b) {
  return static_cast<HistogramExpectations>(static_cast<int>(a) |
                                            static_cast<int>(b));
}

std::ostream& operator<<(std::ostream& os, const HistogramExpectations& value) {
  if (value == 0) {
    os << "(none)";
    return os;
  }

  os << "( ";
  if (0 != (value & kShouldBeBlocked))
    os << "kShouldBeBlocked ";
  if (0 != (value & kShouldBeSniffed))
    os << "kShouldBeSniffed ";
  if (0 != (value & kShouldHaveContentLength))
    os << "kShouldHaveContentLength ";
  os << ")";
  return os;
}

// Ensure the correct histograms are incremented for blocking events.
// Assumes the resource type is XHR.
void InspectHistograms(const base::HistogramTester& histograms,
                       const HistogramExpectations& expectations,
                       const std::string& resource_name,
                       ResourceType resource_type) {
  std::string bucket;
  if (base::MatchPattern(resource_name, "*.html")) {
    bucket = "HTML";
  } else if (base::MatchPattern(resource_name, "*.xml")) {
    bucket = "XML";
  } else if (base::MatchPattern(resource_name, "*.json")) {
    bucket = "JSON";
  } else if (base::MatchPattern(resource_name, "*.txt")) {
    bucket = "Plain";
  } else {
    bucket = "Others";
  }

  // Determine the appropriate histograms, including a start and end action
  // (which are verified in unit tests), a read size if it was sniffed, and
  // additional blocked metrics if it was blocked.
  base::HistogramTester::CountsMap expected_counts;
  std::string base = "SiteIsolation.XSD.Browser";
  expected_counts[base + ".Action"] = 2;
  if ((base::MatchPattern(resource_name, "*prefixed*") || bucket == "Others") &&
      (0 != (expectations & kShouldBeBlocked))) {
    expected_counts[base + ".BlockedForParserBreaker"] = 1;
  }
  if (0 != (expectations & kShouldBeSniffed))
    expected_counts[base + ".BytesReadForSniffing"] = 1;
  if (0 != (expectations & kShouldBeBlocked)) {
    expected_counts[base + ".Blocked"] = 1;
    expected_counts[base + ".Blocked." + bucket] = 1;
    expected_counts[base + ".Blocked.ContentLength.WasAvailable"] = 1;
    if (0 != (expectations & kShouldHaveContentLength))
      expected_counts[base + ".Blocked.ContentLength.ValueIfAvailable"] = 1;
  }

  // Make sure that the expected metrics, and only those metrics, were
  // incremented.
  EXPECT_THAT(histograms.GetTotalCountsForPrefix("SiteIsolation.XSD.Browser"),
              testing::ContainerEq(expected_counts))
      << "For resource_name=" << resource_name
      << ", expectations=" << expectations;

  // Determine if the bucket for the resource type (XHR) was incremented.
  if (0 != (expectations & kShouldBeBlocked)) {
    EXPECT_THAT(histograms.GetAllSamples(base + ".Blocked"),
                testing::ElementsAre(base::Bucket(resource_type, 1)))
        << "The wrong Blocked bucket was incremented.";
    EXPECT_THAT(histograms.GetAllSamples(base + ".Blocked." + bucket),
                testing::ElementsAre(base::Bucket(resource_type, 1)))
        << "The wrong Blocked bucket was incremented.";
  }
}

// Helper for intercepting a resource request to the given URL and capturing the
// response headers and body.
//
// Note that after the request completes, the original requestor (e.g. the
// renderer) will see an injected request failure (this is easier to accomplish
// than forwarding the intercepted response to the original requestor),
class RequestInterceptor {
 public:
  // Start intercepting requests to |url_to_intercept|.
  explicit RequestInterceptor(const GURL& url_to_intercept)
      : url_to_intercept_(url_to_intercept),
        interceptor_(
            base::BindRepeating(&RequestInterceptor::InterceptorCallback,
                                base::Unretained(this))) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DCHECK(url_to_intercept.is_valid());
  }

  // Waits until a request gets intercepted and completed.
  void WaitForRequestCompletion() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DCHECK(!request_completed_);
    test_client_.RunUntilComplete();

    // Read the intercepted response body into |body_|.
    if (test_client_.completion_status().error_code == net::OK) {
      char buffer[128];
      while (true) {
        uint32_t num_bytes = sizeof(buffer);
        auto result = test_client_.response_body().ReadData(
            buffer, &num_bytes, MOJO_READ_DATA_FLAG_NONE);
        if (result != MOJO_RESULT_OK)
          break;

        if (num_bytes == 0)
          break;

        body_ += std::string(buffer, num_bytes);
      }
    }

    // Wait until IO cleanup completes.
    base::RunLoop run_loop;
    BrowserThread::PostTaskAndReply(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&RequestInterceptor::CleanUpOnIOThread,
                       base::Unretained(this)),
        run_loop.QuitClosure());
    run_loop.Run();

    // Mark the request as completed (for DCHECK purposes).
    request_completed_ = true;
  }

  const network::URLLoaderCompletionStatus& completion_status() const {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DCHECK(request_completed_);
    return test_client_.completion_status();
  }

  const network::ResourceResponseHead& response_head() const {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DCHECK(request_completed_);
    return test_client_.response_head();
  }

  const std::string& response_body() const {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DCHECK(request_completed_);
    return body_;
  }

 private:
  bool InterceptorCallback(URLLoaderInterceptor::RequestParams* params) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    DCHECK(params);

    if (url_to_intercept_ != params->url_request.url)
      return false;

    // Prevent more than one intercept.
    if (request_intercepted_)
      return false;
    request_intercepted_ = true;

    // Inject |test_client_| into the request.
    DCHECK(!original_client_);
    original_client_ = std::move(params->client);
    params->client = test_client_.CreateInterfacePtr();

    // Forward the request to the original URLLoaderFactory.
    return false;
  }

  void CleanUpOnIOThread() {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);

    // Tell the |original_client_| that the request has completed (and that it
    // can release its URLLoaderClient.
    original_client_->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_NOT_IMPLEMENTED));

    // Reset all temporary mojo bindings.
    original_client_.reset();
    test_client_.Unbind();
  }

  const GURL url_to_intercept_;
  URLLoaderInterceptor interceptor_;
  network::TestURLLoaderClient test_client_;

  // UI thread state:
  std::string body_;
  bool request_completed_ = false;

  // IO thread state:
  network::mojom::URLLoaderClientPtr original_client_;
  bool request_intercepted_ = false;

  DISALLOW_COPY_AND_ASSIGN(RequestInterceptor);
};

// Custom ContentBrowserClient that disables web security in the renderer
// process without actually using --disable-web-security (which disables CORB).
// This disables the same origin policy to let the renderer see cross-origin
// fetches if they are received.
class DisableWebSecurityContentBrowserClient : public TestContentBrowserClient {
 public:
  DisableWebSecurityContentBrowserClient() : TestContentBrowserClient() {}

  ~DisableWebSecurityContentBrowserClient() override {}

  // ContentBrowserClient overrides:
  void OverrideWebkitPrefs(RenderViewHost* render_view_host,
                           WebPreferences* prefs) override {
    prefs->web_security_enabled = false;
  }
};

}  // namespace

// These tests verify that the browser process blocks cross-site HTML, XML,
// JSON, and some plain text responses when they are not otherwise permitted
// (e.g., by CORS).  This ensures that such responses never end up in the
// renderer process where they might be accessible via a bug.  Careful attention
// is paid to allow other cross-site resources necessary for rendering,
// including cases that may be mislabeled as blocked MIME type.
//
// Many of these tests work by turning off the Same Origin Policy in the
// renderer process via WebPreferences::web_security_enabled, and then trying to
// access the resource via a cross-origin XHR.  If the response is blocked, the
// XHR should see an empty response body.
//
// Note that this BaseTest class does not specify an isolation mode via
// command-line flags.  Most of the tests are in the --site-per-process subclass
// below.
class CrossSiteDocumentBlockingBaseTest : public ContentBrowserTest {
 public:
  CrossSiteDocumentBlockingBaseTest() {}
  ~CrossSiteDocumentBlockingBaseTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // EmbeddedTestServer::InitializeAndListen() initializes its |base_url_|
    // which is required below. This cannot invoke Start() however as that kicks
    // off the "EmbeddedTestServer IO Thread" which then races with
    // initialization in ContentBrowserTest::SetUp(), http://crbug.com/674545.
    ASSERT_TRUE(embedded_test_server()->InitializeAndListen());

    // Add a host resolver rule to map all outgoing requests to the test server.
    // This allows us to use "real" hostnames and standard ports in URLs (i.e.,
    // without having to inject the port number into all URLs), which we can use
    // to create arbitrary SiteInstances.
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP * " + embedded_test_server()->host_port_pair().ToString() +
            ",EXCLUDE localhost");
  }

  void SetUpOnMainThread() override {
    // Complete the manual Start() after ContentBrowserTest's own
    // initialization, ref. comment on InitializeAndListen() above.
    embedded_test_server()->StartAcceptingConnections();

    // Disable web security via the ContentBrowserClient and notify the current
    // renderer process.
    old_client = SetBrowserClientForTesting(&new_client);
    shell()->web_contents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  }

  void TearDown() override { SetBrowserClientForTesting(old_client); }

 private:
  DisableWebSecurityContentBrowserClient new_client;
  ContentBrowserClient* old_client = nullptr;

  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentBlockingBaseTest);
};

// Most tests here use --site-per-process, which enables document blocking
// everywhere.
class CrossSiteDocumentBlockingTest : public CrossSiteDocumentBlockingBaseTest {
 public:
  CrossSiteDocumentBlockingTest() {}
  ~CrossSiteDocumentBlockingTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    IsolateAllSitesForTesting(command_line);
    CrossSiteDocumentBlockingBaseTest::SetUpCommandLine(command_line);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentBlockingTest);
};

IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingTest, BlockDocuments) {
  // Load a page that issues illegal cross-site document requests to bar.com.
  // The page uses XHR to request HTML/XML/JSON documents from bar.com, and
  // inspects if any of them were successfully received. This test is only
  // possible since we run the browser without the same origin policy, allowing
  // it to see the response body if it makes it to the renderer (even if the
  // renderer would normally block access to it).
  GURL foo_url("http://foo.com/cross_site_document_blocking/request.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo_url));

  // The following are files under content/test/data/site_isolation. All
  // should be disallowed for cross site XHR under the document blocking policy.
  //   valid.*        - Correctly labeled HTML/XML/JSON files.
  //   *.txt          - Plain text that sniffs as HTML, XML, or JSON.
  //   htmlN_dtd.*    - Various HTML templates to test.
  //   json-prefixed* - parser-breaking prefixes
  const char* blocked_resources[] = {"valid.html",
                                     "valid.xml",
                                     "valid.json",
                                     "html.txt",
                                     "xml.txt",
                                     "json.txt",
                                     "comment_valid.html",
                                     "html4_dtd.html",
                                     "html4_dtd.txt",
                                     "html5_dtd.html",
                                     "html5_dtd.txt",
                                     "json.js",
                                     "json-prefixed-1.js",
                                     "json-prefixed-2.js",
                                     "json-prefixed-3.js",
                                     "json-prefixed-4.js",
                                     "nosniff.json.js",
                                     "nosniff.json-prefixed.js"};
  for (const char* resource : blocked_resources) {
    SCOPED_TRACE(base::StringPrintf("... while testing page: %s", resource));
    base::HistogramTester histograms;
    bool was_blocked;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(
        shell(), base::StringPrintf("sendRequest('%s');", resource),
        &was_blocked));
    EXPECT_TRUE(was_blocked);
    InspectHistograms(histograms,
                      kShouldBeSniffedAndBlocked | kShouldHaveContentLength,
                      resource, RESOURCE_TYPE_XHR);
  }

  // These files should be disallowed without sniffing.
  //   nosniff.*   - Won't sniff correctly, but blocked because of nosniff.
  const char* nosniff_blocked_resources[] = {"nosniff.html", "nosniff.xml",
                                             "nosniff.json", "nosniff.txt"};
  for (const char* resource : nosniff_blocked_resources) {
    SCOPED_TRACE(base::StringPrintf("... while testing page: %s", resource));
    base::HistogramTester histograms;
    bool was_blocked;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(
        shell(), base::StringPrintf("sendRequest('%s');", resource),
        &was_blocked));
    EXPECT_TRUE(was_blocked);
    InspectHistograms(histograms, kShouldBeBlockedWithoutSniffing, resource,
                      RESOURCE_TYPE_XHR);
  }

  // These files are allowed for XHR under the document blocking policy because
  // the sniffing logic determines they are not actually documents.
  //   *js.*   - JavaScript mislabeled as a document.
  //   jsonp.* - JSONP (i.e., script) mislabeled as a document.
  //   img.*   - Contents that won't match the document label.
  //   valid.* - Correctly labeled responses of non-document types.
  const char* sniff_allowed_resources[] = {"js.html",
                                           "comment_js.html",
                                           "js.xml",
                                           "js.json",
                                           "js.txt",
                                           "jsonp.html",
                                           "jsonp.xml",
                                           "jsonp.json",
                                           "jsonp.txt",
                                           "img.html",
                                           "img.xml",
                                           "img.json",
                                           "img.txt",
                                           "valid.js",
                                           "json-list.js",
                                           "nosniff.json-list.js",
                                           "js-html-polyglot.html",
                                           "js-html-polyglot2.html"};
  for (const char* resource : sniff_allowed_resources) {
    SCOPED_TRACE(base::StringPrintf("... while testing page: %s", resource));
    base::HistogramTester histograms;
    bool was_blocked;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(
        shell(), base::StringPrintf("sendRequest('%s');", resource),
        &was_blocked));
    EXPECT_FALSE(was_blocked);
    InspectHistograms(histograms, kShouldBeSniffedAndAllowed, resource,
                      RESOURCE_TYPE_XHR);
  }

  // These files should be allowed for XHR under the document blocking policy.
  //   cors.*  - Correctly labeled documents with valid CORS headers.
  const char* allowed_resources[] = {"cors.html", "cors.xml", "cors.json",
                                     "cors.txt"};
  for (const char* resource : allowed_resources) {
    SCOPED_TRACE(base::StringPrintf("... while testing page: %s", resource));
    base::HistogramTester histograms;
    bool was_blocked;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(
        shell(), base::StringPrintf("sendRequest('%s');", resource),
        &was_blocked));
    EXPECT_FALSE(was_blocked);
    InspectHistograms(histograms, kShouldBeAllowedWithoutSniffing, resource,
                      RESOURCE_TYPE_XHR);
  }
}

// Verify that range requests disable the sniffing logic, so that attackers
// can't cause sniffing to fail to force a response to be allowed.  This won't
// be a problem for script files mislabeled as HTML/XML/JSON/text (i.e., the
// reason for sniffing), since script tags won't send Range headers.
IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingTest, RangeRequest) {
  GURL foo_url("http://foo.com/cross_site_document_blocking/request.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo_url));

  {
    // Try to skip the first byte using a range request in an attempt to get the
    // response to fail sniffing and be allowed through.  It should still be
    // blocked because sniffing is disabled.
    base::HistogramTester histograms;
    bool was_blocked;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(
        shell(), "sendRequest('valid.html', 'bytes=1-24');", &was_blocked));
    EXPECT_TRUE(was_blocked);
    InspectHistograms(
        histograms, kShouldBeBlockedWithoutSniffing | kShouldHaveContentLength,
        "valid.html", RESOURCE_TYPE_XHR);
  }
  {
    // Verify that a response which would have been allowed by MIME type anyway
    // is still allowed for range requests.
    base::HistogramTester histograms;
    bool was_blocked;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(
        shell(), "sendRequest('valid.js', 'bytes=1-5');", &was_blocked));
    EXPECT_FALSE(was_blocked);
    InspectHistograms(histograms, kShouldBeAllowedWithoutSniffing, "valid.js",
                      RESOURCE_TYPE_XHR);
  }
  {
    // Verify that a response which would have been allowed by CORS anyway is
    // still allowed for range requests.
    base::HistogramTester histograms;
    bool was_blocked;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(
        shell(), "sendRequest('cors.json', 'bytes=2-7');", &was_blocked));
    EXPECT_FALSE(was_blocked);
    InspectHistograms(histograms, kShouldBeAllowedWithoutSniffing, "cors.json",
                      RESOURCE_TYPE_XHR);
  }
}

IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingTest, BlockForVariousTargets) {
  // This webpage loads a cross-site HTML page in different targets such as
  // <img>,<link>,<embed>, etc. Since the requested document is blocked, and one
  // character string (' ') is returned instead, this tests that the renderer
  // does not crash even when it receives a response body which is " ", whose
  // length is different from what's described in "content-length" for such
  // different targets.

  // TODO(nick): Split up these cases, and add positive assertions here about
  // what actually happens in these various resource-block cases.
  GURL foo("http://foo.com/cross_site_document_blocking/request_target.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo));

  // TODO(creis): Wait for all the subresources to load and ensure renderer
  // process is still alive.
}

// Checks to see that CORB blocking applies to processes hosting error pages.
// Regression test for https://crbug.com/814913.
IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingTest,
                       BlockRequestFromErrorPage) {
  GURL error_url = embedded_test_server()->GetURL("bar.com", "/close-socket");
  GURL subresource_url =
      embedded_test_server()->GetURL("foo.com", "/site_isolation/json.js");

  // Load |error_url| and expect a network error page.
  TestNavigationObserver observer(shell()->web_contents());
  EXPECT_FALSE(NavigateToURL(shell(), error_url));
  EXPECT_EQ(error_url, observer.last_navigation_url());
  NavigationEntry* entry =
      shell()->web_contents()->GetController().GetLastCommittedEntry();
  EXPECT_EQ(PAGE_TYPE_ERROR, entry->GetPageType());

  // Add a <script> tag whose src is a CORB-protected resource. Expect no
  // window.onerror to result, because no syntax error is generated by the empty
  // response.
  std::string script = R"((subresource_url => {
    window.onerror = () => domAutomationController.send("CORB BYPASSED");
    var script = document.createElement('script');
    script.src = subresource_url;
    script.onload = () => domAutomationController.send("CORB WORKED");
    document.body.appendChild(script);
    }))";
  std::string result;
  ASSERT_TRUE(ExecuteScriptAndExtractString(
      shell(), script + "('" + subresource_url.spec() + "')", &result));

  EXPECT_EQ("CORB WORKED", result);
}

IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingTest, BlockHeaders) {
  GURL foo_url("http://foo.com/title1.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo_url));

  // Prepare to intercept the network request at the IPC layer.
  //
  // Note: we want to verify that the blocking prevents the data from being sent
  // over IPC.  Testing later (e.g. via Response/Headers Web APIs) might give a
  // false sense of security, since some sanitization happens inside the
  // renderer (e.g. via FetchResponseData::CreateCORSFilteredResponse).
  GURL bar_url("http://bar.com/cross_site_document_blocking/headers-test.json");
  RequestInterceptor interceptor(bar_url);

  // Issue the request that will be intercepted
  EXPECT_TRUE(ExecuteScript(shell(),
                            base::StringPrintf("fetch('%s').catch(error => {})",
                                               bar_url.spec().c_str())));
  interceptor.WaitForRequestCompletion();

  // Verify that the response completed successfully and was blocked.
  ASSERT_EQ(net::OK, interceptor.completion_status().error_code);
  ASSERT_TRUE(interceptor.completion_status().blocked_cross_site_document);

  // Verify that safelisted headers have not been removed by XSDB.
  // See https://fetch.spec.whatwg.org/#cors-safelisted-response-header-name.
  const std::string& headers =
      interceptor.response_head().headers->raw_headers();
  EXPECT_THAT(headers,
              HasSubstr("Cache-Control: no-cache, no-store, must-revalidate"));
  EXPECT_THAT(headers, HasSubstr("Content-Language: TestLanguage"));
  EXPECT_THAT(headers,
              HasSubstr("Content-Type: application/json; charset=utf-8"));
  EXPECT_THAT(headers, HasSubstr("Expires: Wed, 21 Oct 2199 07:28:00 GMT"));
  EXPECT_THAT(headers,
              HasSubstr("Last-Modified: Wed, 07 Feb 2018 13:55:00 PST"));
  EXPECT_THAT(headers, HasSubstr("Pragma: TestPragma"));

  // Make sure the test covers all the safelisted headers known to the product
  // code.
  for (const std::string& safelisted_header :
       network::CrossOriginReadBlocking::GetCorsSafelistedHeadersForTesting()) {
    EXPECT_TRUE(
        interceptor.response_head().headers->HasHeader(safelisted_header));

    std::string value;
    interceptor.response_head().headers->EnumerateHeader(
        nullptr, safelisted_header, &value);
    EXPECT_FALSE(value.empty());
  }

  // Verify that other response headers have been removed by XSDB.
  EXPECT_THAT(headers, Not(HasSubstr("Content-Length")));
  EXPECT_THAT(headers, Not(HasSubstr("X-My-Secret-Header")));
  EXPECT_THAT(headers, Not(HasSubstr("MySecretCookieKey")));
  EXPECT_THAT(headers, Not(HasSubstr("MySecretCookieValue")));

  // Verify that the body is empty.
  EXPECT_EQ("", interceptor.response_body());
  EXPECT_EQ(0, interceptor.completion_status().decoded_body_length);

  // Verify that other response parts have been sanitized.
  EXPECT_EQ(0u, interceptor.response_head().content_length);
}

// This test class sets up a service worker that can be used to try to respond
// to same-origin requests with cross-origin responses.
class CrossSiteDocumentBlockingServiceWorkerTest : public ContentBrowserTest {
 public:
  CrossSiteDocumentBlockingServiceWorkerTest()
      : service_worker_https_server_(net::EmbeddedTestServer::TYPE_HTTPS),
        cross_origin_https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}
  ~CrossSiteDocumentBlockingServiceWorkerTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    IsolateAllSitesForTesting(command_line);
    ContentBrowserTest::SetUpCommandLine(command_line);
  }

  void SetUpOnMainThread() override {
    SetupCrossSiteRedirector(embedded_test_server());

    service_worker_https_server_.ServeFilesFromSourceDirectory(
        "content/test/data");
    ASSERT_TRUE(service_worker_https_server_.Start());

    cross_origin_https_server_.ServeFilesFromSourceDirectory(
        "content/test/data");
    cross_origin_https_server_.SetSSLConfig(
        net::EmbeddedTestServer::CERT_COMMON_NAME_IS_DOMAIN);
    ASSERT_TRUE(cross_origin_https_server_.Start());

    // Sanity check of test setup - the 2 https servers should be cross-site
    // (the second server should have a different hostname because of the call
    // to SetSSLConfig with CERT_COMMON_NAME_IS_DOMAIN argument).
    ASSERT_FALSE(SiteInstance::IsSameWebSite(
        shell()->web_contents()->GetBrowserContext(),
        GetURLOnServiceWorkerServer("/"), GetURLOnCrossOriginServer("/")));

    // Disable web security via the ContentBrowserClient and notify the current
    // renderer process.
    old_client = SetBrowserClientForTesting(&new_client);
    shell()->web_contents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  }

  void TearDown() override { SetBrowserClientForTesting(old_client); }

  GURL GetURLOnServiceWorkerServer(const std::string& path) {
    return service_worker_https_server_.GetURL(path);
  }

  GURL GetURLOnCrossOriginServer(const std::string& path) {
    return cross_origin_https_server_.GetURL(path);
  }

  void StopCrossOriginServer() {
    EXPECT_TRUE(cross_origin_https_server_.ShutdownAndWaitUntilComplete());
  }

  void SetUpServiceWorker() {
    GURL url = GetURLOnServiceWorkerServer(
        "/cross_site_document_blocking/request.html");
    ASSERT_TRUE(NavigateToURL(shell(), url));

    // Register the service worker.
    bool is_script_done;
    std::string script = R"(
        navigator.serviceWorker
            .register('/cross_site_document_blocking/service_worker.js')
            .then(registration => navigator.serviceWorker.ready)
            .then(function(r) { domAutomationController.send(true); })
            .catch(function(e) {
                console.log('error: ' + e);
                domAutomationController.send(false);
            }); )";
    ASSERT_TRUE(ExecuteScriptAndExtractBool(shell(), script, &is_script_done));
    ASSERT_TRUE(is_script_done);

    // Navigate again to the same URL - the service worker should be 1) active
    // at this time (because of waiting for |navigator.serviceWorker.ready|
    // above) and 2) controlling the current page (because of the reload).
    ASSERT_TRUE(NavigateToURL(shell(), url));
    bool is_controlled_by_service_worker;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(
        shell(),
        "domAutomationController.send(!!navigator.serviceWorker.controller)",
        &is_controlled_by_service_worker));
    ASSERT_TRUE(is_controlled_by_service_worker);
  }

 private:
  // The test requires 2 https servers, because:
  // 1. Service workers are only supported on secure origins.
  // 2. One of tests requires fetching cross-origin resources from the
  //    original page and/or service worker - the target of the fetch needs to
  //    be a https server to avoid hitting the mixed content error.
  net::EmbeddedTestServer service_worker_https_server_;
  net::EmbeddedTestServer cross_origin_https_server_;

  DisableWebSecurityContentBrowserClient new_client;
  ContentBrowserClient* old_client = nullptr;

  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentBlockingServiceWorkerTest);
};

// Issue a cross-origin request that will be handled entirely within a service
// worker (without reaching the network - the cross-origin response will be
// "faked" within the same-origin service worker, because the service worker
// used by the test recognizes the "data_from_service_worker" suffix in the
// URL).  This testcase is designed to hit the case in
// CrossSiteDocumentResourceHandler::ShouldBlockBasedOnHeaders where
// |response_type_via_service_worker| is equal to |kDefault|.  See also
// https://crbug.com/803672.
//
// TODO(lukasza): https://crbug.com/715640: This test might become invalid
// after servicification of service workers.
IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingServiceWorkerTest, NoNetwork) {
  SetUpServiceWorker();

  base::HistogramTester histograms;
  std::string response;
  std::string script = R"(
      // Any cross-origin URL ending with .../data_from_service_worker can be
      // used here - it will be intercepted by the service worker and will never
      // go to the network.
      fetch('https://bar.com/data_from_service_worker')
          .then(response => response.text())
          .then(responseText => {
              domAutomationController.send(responseText);
          })
          .catch(error => {
              var errorMessage = 'error: ' + error;
              console.log(errorMessage);
              domAutomationController.send(errorMessage);
          }); )";
  EXPECT_TRUE(ExecuteScriptAndExtractString(shell(), script, &response));

  // Verify that XSDB didn't block the response (since it was "faked" within the
  // service worker and didn't cross any security boundaries).
  EXPECT_EQ("Response created by service worker", response);
  InspectHistograms(histograms, kShouldBeAllowedWithoutSniffing, "blah.html",
                    RESOURCE_TYPE_XHR);
}

IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingServiceWorkerTest,
                       NetworkToServiceWorkerResponse) {
  SetUpServiceWorker();

  // Build a script for XHR-ing a cross-origin, nosniff HTML document.
  GURL cross_origin_url =
      GetURLOnCrossOriginServer("/site_isolation/nosniff.txt");
  const char* script_template = R"(
      fetch('%s', { mode: 'no-cors' })
          .then(response => response.text())
          .then(responseText => {
              domAutomationController.send(responseText);
          })
          .catch(error => {
              var errorMessage = 'error: ' + error;
              domAutomationController.send(errorMessage);
          }); )";
  std::string script =
      base::StringPrintf(script_template, cross_origin_url.spec().c_str());

  // The service worker will forward the request to the network, but a response
  // will be intercepted by the service worker and replaced with a new,
  // artificial error.
  base::HistogramTester histograms;
  std::string response;
  EXPECT_TRUE(ExecuteScriptAndExtractString(shell(), script, &response));

  // Verify that XSDB blocked the response from the network (from
  // |cross_origin_https_server_|) to the service worker.
  InspectHistograms(histograms, kShouldBeBlockedWithoutSniffing, "network.txt",
                    RESOURCE_TYPE_XHR);

  // Verify that the service worker replied with an expected error.
  // Replying with an error means that XSDB is only active once (for the
  // initial, real network request) and therefore the test doesn't get
  // confused (second successful response would have added noise to the
  // histograms captured by the test).
  EXPECT_EQ("error: TypeError: Failed to fetch", response);
}

class CrossSiteDocumentBlockingKillSwitchTest
    : public CrossSiteDocumentBlockingTest {
 public:
  CrossSiteDocumentBlockingKillSwitchTest() {
    // Simulate flipping both of the kill switches.
    std::vector<base::Feature> disabled_features = {
        features::kCrossSiteDocumentBlockingAlways,
        features::kCrossSiteDocumentBlockingIfIsolating,
    };
    scoped_feature_list_.InitWithFeatures({}, disabled_features);
  }

  ~CrossSiteDocumentBlockingKillSwitchTest() override {}

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentBlockingKillSwitchTest);
};

// After the kill switch is flipped, there should be no document blocking.
IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingKillSwitchTest,
                       NoBlockingWithKillSwitch) {
  // Load a page that issues illegal cross-site document requests to bar.com.
  GURL foo_url("http://foo.com/cross_site_document_blocking/request.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo_url));

  bool was_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      shell(), "sendRequest(\"valid.html\");", &was_blocked));
  EXPECT_FALSE(was_blocked);
}

// Test class to verify that --disable-web-security turns off CORB.  This
// inherits from CrossSiteDocumentBlockingTest, so it runs in SitePerProcess.
class CrossSiteDocumentBlockingDisableWebSecurityTest
    : public CrossSiteDocumentBlockingTest {
 public:
  CrossSiteDocumentBlockingDisableWebSecurityTest() {}
  ~CrossSiteDocumentBlockingDisableWebSecurityTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kDisableWebSecurity);
    CrossSiteDocumentBlockingTest::SetUpCommandLine(command_line);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentBlockingDisableWebSecurityTest);
};

IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingDisableWebSecurityTest,
                       DisableBlocking) {
  // Load a page that issues illegal cross-site document requests.
  GURL foo_url("http://foo.com/cross_site_document_blocking/request.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo_url));

  bool was_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      shell(), "sendRequest(\"valid.html\");", &was_blocked));
  EXPECT_FALSE(was_blocked);
}

// Test class to verify that kCrossSiteDocumentBlockingAlways does not take
// precedence over --disable-web-security.  This inherits from
// CrossSiteDocumentBlockingTest, so it runs in SitePerProcess.
class CrossSiteDocumentBlockingDisableVsFeatureTest
    : public CrossSiteDocumentBlockingDisableWebSecurityTest {
 public:
  CrossSiteDocumentBlockingDisableVsFeatureTest() {
    scoped_feature_list_.InitAndEnableFeature(
        features::kCrossSiteDocumentBlockingAlways);
  }
  ~CrossSiteDocumentBlockingDisableVsFeatureTest() override {}

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentBlockingDisableVsFeatureTest);
};

IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingDisableVsFeatureTest,
                       DisableBlocking) {
  // Load a page that issues illegal cross-site document requests.
  GURL foo_url("http://foo.com/cross_site_document_blocking/request.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo_url));

  bool was_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      shell(), "sendRequest(\"valid.html\");", &was_blocked));
  EXPECT_FALSE(was_blocked);
}

// Even without any Site Isolation, document blocking should be turned on by
// default.
IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingBaseTest,
                       BlockDocumentsByDefault) {
  // Load a page that issues illegal cross-site document requests to bar.com.
  GURL foo_url("http://foo.com/cross_site_document_blocking/request.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo_url));

  bool was_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      shell(), "sendRequest(\"valid.html\");", &was_blocked));
  EXPECT_TRUE(was_blocked);
}

// Test class to verify that documents are blocked for isolated origins as well.
class CrossSiteDocumentBlockingIsolatedOriginTest
    : public CrossSiteDocumentBlockingBaseTest {
 public:
  CrossSiteDocumentBlockingIsolatedOriginTest() {}
  ~CrossSiteDocumentBlockingIsolatedOriginTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(switches::kIsolateOrigins,
                                    "http://bar.com");
    CrossSiteDocumentBlockingBaseTest::SetUpCommandLine(command_line);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentBlockingIsolatedOriginTest);
};

IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingIsolatedOriginTest,
                       BlockDocumentsFromIsolatedOrigin) {
  if (AreAllSitesIsolatedForTesting())
    return;

  // Load a page that issues illegal cross-site document requests to the
  // isolated origin.
  GURL foo_url("http://foo.com/cross_site_document_blocking/request.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo_url));

  bool was_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      shell(), "sendRequest(\"valid.html\");", &was_blocked));
  EXPECT_TRUE(was_blocked);
}

}  // namespace content
