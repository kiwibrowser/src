// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/net_internals/net_internals_ui_browsertest.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/prerender/prerender_manager.h"
#include "chrome/browser/prerender/prerender_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/net_internals/net_internals_ui.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/net_log/chrome_net_log.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "net/base/address_list.h"
#include "net/base/net_errors.h"
#include "net/base/network_change_notifier.h"
#include "net/dns/host_cache.h"
#include "net/dns/host_resolver.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_network_session.h"
#include "net/http/http_transaction_factory.h"
#include "net/log/file_net_log_observer.h"
#include "net/log/net_log.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/request_handler_util.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using content::BrowserThread;
using content::WebUIMessageHandler;

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleExpectCTReportPreflight(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->AddCustomHeader("Access-Control-Allow-Origin", "*");
  http_response->AddCustomHeader("Access-Control-Allow-Methods", "POST");
  http_response->AddCustomHeader("Access-Control-Allow-Headers",
                                 "Content-Type");
  return http_response;
}

// Called on IO thread.  Adds an entry to the cache for the specified hostname.
// Either |net_error| must be net::OK, or |address| must be NULL.
void AddCacheEntryOnIOThread(net::URLRequestContextGetter* context_getter,
                             const std::string& hostname,
                             const std::string& ip_literal,
                             int net_error,
                             int expire_days_from_now) {
  ASSERT_TRUE(BrowserThread::CurrentlyOn(BrowserThread::IO));
  net::URLRequestContext* context = context_getter->GetURLRequestContext();
  net::HostCache* cache = context->host_resolver()->GetHostCache();
  ASSERT_TRUE(cache);

  net::HostCache::Key key(hostname, net::ADDRESS_FAMILY_UNSPECIFIED, 0);
  base::TimeDelta ttl = base::TimeDelta::FromDays(expire_days_from_now);

  net::AddressList address_list;
  if (net_error == net::OK) {
    // If |net_error| does not indicate an error, convert |ip_literal| to a
    // net::AddressList, so it can be used with the cache.
    int rv = net::ParseAddressList(ip_literal, hostname, &address_list);
    ASSERT_EQ(net::OK, rv);
  } else {
    ASSERT_TRUE(ip_literal.empty());
  }

  // Add entry to the cache.
  cache->Set(net::HostCache::Key(hostname, net::ADDRESS_FAMILY_UNSPECIFIED, 0),
             net::HostCache::Entry(net_error, address_list,
                                   net::HostCache::Entry::SOURCE_UNKNOWN),
             base::TimeTicks::Now(), ttl);
}

struct WriteNetLogState {
  base::ScopedTempDir temp_directory;
  base::FilePath log_path;
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// NetInternalsTest::MessageHandler
////////////////////////////////////////////////////////////////////////////////

// Class to handle messages from the renderer needed by certain tests.
class NetInternalsTest::MessageHandler : public content::WebUIMessageHandler {
 public:
  explicit MessageHandler(NetInternalsTest* net_internals_test);

 private:
  void RegisterMessages() override;

  // Runs NetInternalsTest.callback with the given value.
  void RunJavascriptCallback(base::Value* value);

  // Takes a string and provides the corresponding URL from the test server,
  // which must already have been started.
  void GetTestServerURL(const base::ListValue* list_value);

  // Called on UI thread.  Adds an entry to the cache for the specified
  // hostname by posting a task to the IO thread.  Takes the host name,
  // ip address, net error code, and expiration time in days from now
  // as parameters.  If the error code indicates failure, the ip address
  // must be an empty string.
  void AddCacheEntry(const base::ListValue* list_value);

  // Simulates a network change.
  void ChangeNetwork(const base::ListValue* list_value);

  // Opens the given URL in a new tab.
  void LoadPage(const base::ListValue* list_value);

  // Opens a page in a new tab that prerenders the given URL.
  void PrerenderPage(const base::ListValue* list_value);

  // Navigates to the prerender in the background tab. This assumes that
  // there is a "Click()" function in the background tab which will navigate
  // there, and that the background tab exists at slot 1.
  void NavigateToPrerender(const base::ListValue* list_value);

  // Creates an incognito browser.  Once creation is complete, passes a
  // message to the Javascript test harness.
  void CreateIncognitoBrowser(const base::ListValue* list_value);

  // Closes an incognito browser created with CreateIncognitoBrowser.
  void CloseIncognitoBrowser(const base::ListValue* list_value);

  // Creates a simple NetLog and returns it to the Javascript callback.
  void GetNetLogFileContents(const base::ListValue* list_value);

  // Sets up the test server to receive test Expect-CT reports. Calls the
  // Javascript callback to return the test server URI.
  void SetUpTestReportURI(const base::ListValue* list_value);

  // Changes the data reduction proxy mode. A boolean is assumed to exist at
  // index 0 which enables the proxy is set to true.
  void EnableDataReductionProxy(const base::ListValue* list_value);

  // Called after the NetLog started by GetNetLogFileContents() has been written
  // to disk. Responds to the Javascript caller with the log contents.
  void OnFinishedWritingNetLog(std::unique_ptr<WriteNetLogState> state);

  Browser* browser() { return net_internals_test_->browser(); }

  NetInternalsTest* net_internals_test_;
  Browser* incognito_browser_;

  DISALLOW_COPY_AND_ASSIGN(MessageHandler);
};

NetInternalsTest::MessageHandler::MessageHandler(
    NetInternalsTest* net_internals_test)
    : net_internals_test_(net_internals_test),
      incognito_browser_(NULL) {
}

void NetInternalsTest::MessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getTestServerURL",
      base::BindRepeating(&NetInternalsTest::MessageHandler::GetTestServerURL,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "addCacheEntry",
      base::BindRepeating(&NetInternalsTest::MessageHandler::AddCacheEntry,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "changeNetwork",
      base::BindRepeating(&NetInternalsTest::MessageHandler::ChangeNetwork,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "loadPage",
      base::BindRepeating(&NetInternalsTest::MessageHandler::LoadPage,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "prerenderPage",
      base::BindRepeating(&NetInternalsTest::MessageHandler::PrerenderPage,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "navigateToPrerender",
      base::BindRepeating(
          &NetInternalsTest::MessageHandler::NavigateToPrerender,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "createIncognitoBrowser",
      base::BindRepeating(
          &NetInternalsTest::MessageHandler::CreateIncognitoBrowser,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "closeIncognitoBrowser",
      base::BindRepeating(
          &NetInternalsTest::MessageHandler::CloseIncognitoBrowser,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getNetLogFileContents",
      base::BindRepeating(
          &NetInternalsTest::MessageHandler::GetNetLogFileContents,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setUpTestReportURI",
      base::BindRepeating(&NetInternalsTest::MessageHandler::SetUpTestReportURI,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "enableDataReductionProxy",
      base::BindRepeating(
          &NetInternalsTest::MessageHandler::EnableDataReductionProxy,
          base::Unretained(this)));
}

void NetInternalsTest::MessageHandler::RunJavascriptCallback(
    base::Value* value) {
  web_ui()->CallJavascriptFunctionUnsafe("NetInternalsTest.callback", *value);
}

void NetInternalsTest::MessageHandler::GetTestServerURL(
    const base::ListValue* list_value) {
  ASSERT_TRUE(net_internals_test_->StartTestServer());
  std::string path;
  ASSERT_TRUE(list_value->GetString(0, &path));
  GURL url = net_internals_test_->embedded_test_server()->GetURL(path);
  std::unique_ptr<base::Value> url_value(new base::Value(url.spec()));
  RunJavascriptCallback(url_value.get());
}

void NetInternalsTest::MessageHandler::AddCacheEntry(
    const base::ListValue* list_value) {
  std::string hostname;
  std::string ip_literal;
  double net_error;
  double expire_days_from_now;
  ASSERT_TRUE(list_value->GetString(0, &hostname));
  ASSERT_TRUE(list_value->GetString(1, &ip_literal));
  ASSERT_TRUE(list_value->GetDouble(2, &net_error));
  ASSERT_TRUE(list_value->GetDouble(3, &expire_days_from_now));
  ASSERT_TRUE(browser());

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &AddCacheEntryOnIOThread,
          base::RetainedRef(browser()->profile()->GetRequestContext()),
          hostname, ip_literal, static_cast<int>(net_error),
          static_cast<int>(expire_days_from_now)));
}

void NetInternalsTest::MessageHandler::ChangeNetwork(
    const base::ListValue* list_value) {
  net::NetworkChangeNotifier::NotifyObserversOfIPAddressChangeForTests();
}

void NetInternalsTest::MessageHandler::LoadPage(
    const base::ListValue* list_value) {
  std::string url;
  ASSERT_TRUE(list_value->GetString(0, &url));
  LOG(WARNING) << "url: [" << url << "]";
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(url), WindowOpenDisposition::NEW_BACKGROUND_TAB,
      ui_test_utils::BROWSER_TEST_NONE);
}

void NetInternalsTest::MessageHandler::PrerenderPage(
    const base::ListValue* list_value) {
  std::string prerender_url;
  ASSERT_TRUE(list_value->GetString(0, &prerender_url));
  GURL loader_url =
      net_internals_test_->CreatePrerenderLoaderUrl(GURL(prerender_url));
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(loader_url), WindowOpenDisposition::NEW_BACKGROUND_TAB,
      ui_test_utils::BROWSER_TEST_NONE);
}

void NetInternalsTest::MessageHandler::NavigateToPrerender(
    const base::ListValue* list_value) {
  std::string url;
  ASSERT_TRUE(list_value->GetString(0, &url));
  content::RenderFrameHost* frame =
      browser()->tab_strip_model()->GetWebContentsAt(1)->GetMainFrame();
  frame->ExecuteJavaScriptForTests(
      base::ASCIIToUTF16(base::StringPrintf("Click('%s')", url.c_str())));
}

void NetInternalsTest::MessageHandler::CreateIncognitoBrowser(
    const base::ListValue* list_value) {
  ASSERT_FALSE(incognito_browser_);
  incognito_browser_ = net_internals_test_->CreateIncognitoBrowser();

  // Tell the test harness that creation is complete.
  base::Value command_value("onIncognitoBrowserCreatedForTest");
  web_ui()->CallJavascriptFunctionUnsafe("g_browser.receive", command_value);
}

void NetInternalsTest::MessageHandler::CloseIncognitoBrowser(
    const base::ListValue* list_value) {
  ASSERT_TRUE(incognito_browser_);
  incognito_browser_->tab_strip_model()->CloseAllTabs();
  // Closing all a Browser's tabs will ultimately result in its destruction,
  // thought it may not have been destroyed yet.
  incognito_browser_ = NULL;
}

void NetInternalsTest::MessageHandler::GetNetLogFileContents(
    const base::ListValue* list_value) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  std::unique_ptr<WriteNetLogState> state =
      std::make_unique<WriteNetLogState>();

  ASSERT_TRUE(state->temp_directory.CreateUniqueTempDir());
  ASSERT_TRUE(base::CreateTemporaryFileInDir(state->temp_directory.GetPath(),
                                             &state->log_path));

  std::unique_ptr<base::Value> constants(net_log::ChromeNetLog::GetConstants(
      base::CommandLine::ForCurrentProcess()->GetCommandLineString(),
      chrome::GetChannelName()));

  std::unique_ptr<net::FileNetLogObserver> net_log_logger =
      net::FileNetLogObserver::CreateUnbounded(state->log_path,
                                               std::move(constants));

  net_log_logger->StartObserving(g_browser_process->net_log(),
                                 net::NetLogCaptureMode::Default());

  g_browser_process->net_log()->AddGlobalEntry(
      net::NetLogEventType::NETWORK_IP_ADDRESSES_CHANGED);
  net::NetLogWithSource net_log_with_source = net::NetLogWithSource::Make(
      g_browser_process->net_log(), net::NetLogSourceType::URL_REQUEST);
  net_log_with_source.BeginEvent(net::NetLogEventType::REQUEST_ALIVE);

  // Call OnFinishedWritingNetLog() once net_log_logger has completed writing it
  // to disk.
  net_log_logger->StopObserving(
      nullptr,
      base::Bind(&NetInternalsTest::MessageHandler::OnFinishedWritingNetLog,
                 base::Unretained(this), base::Passed(std::move(state))));
}

void NetInternalsTest::MessageHandler::SetUpTestReportURI(
    const base::ListValue* list_value) {
  net_internals_test_->embedded_test_server()->RegisterRequestHandler(
      base::Bind(&HandleExpectCTReportPreflight));
  ASSERT_TRUE(net_internals_test_->embedded_test_server()->Start());
  base::Value report_uri_value(
      net_internals_test_->embedded_test_server()->GetURL("/").spec());
  RunJavascriptCallback(&report_uri_value);
}

void NetInternalsTest::MessageHandler::EnableDataReductionProxy(
    const base::ListValue* list_value) {
  bool enable;
  ASSERT_TRUE(list_value->GetBoolean(0, &enable));
  browser()->profile()->GetPrefs()->SetBoolean(
      prefs::kDataSaverEnabled, enable);
}

void NetInternalsTest::MessageHandler::OnFinishedWritingNetLog(
    std::unique_ptr<WriteNetLogState> state) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  std::string log_contents;
  ASSERT_TRUE(base::ReadFileToString(state->log_path, &log_contents));
  ASSERT_GT(log_contents.length(), 0u);

  std::unique_ptr<base::Value> log_contents_value(
      new base::Value(log_contents));
  RunJavascriptCallback(log_contents_value.get());

  state.reset();
}

////////////////////////////////////////////////////////////////////////////////
// NetInternalsTest
////////////////////////////////////////////////////////////////////////////////

NetInternalsTest::NetInternalsTest()
    : test_server_started_(false) {
  message_handler_.reset(new MessageHandler(this));
}

NetInternalsTest::~NetInternalsTest() {
}

void NetInternalsTest::SetUpOnMainThread() {
  WebUIBrowserTest::SetUpOnMainThread();
  // Needed to test the prerender view.
  prerender::PrerenderManager::SetMode(
      prerender::PrerenderManager::PRERENDER_MODE_ENABLED);
  // Increase the memory allowed in a prerendered page above normal settings,
  // as debug builds use more memory and often go over the usual limit.
  Profile* profile = browser()->profile();
  prerender::PrerenderManager* prerender_manager =
      prerender::PrerenderManagerFactory::GetForBrowserContext(profile);
  prerender_manager->mutable_config().max_bytes = 1000 * 1024 * 1024;
}

content::WebUIMessageHandler* NetInternalsTest::GetMockMessageHandler() {
  return message_handler_.get();
}

GURL NetInternalsTest::CreatePrerenderLoaderUrl(
    const GURL& prerender_url) {
  EXPECT_TRUE(StartTestServer());
  base::StringPairs replacement_text;
  replacement_text.push_back(
      make_pair("REPLACE_WITH_PRERENDER_URL", prerender_url.spec()));
  std::string replacement_path;
  net::test_server::GetFilePathWithReplacements(
      "/prerender/prerender_loader.html", replacement_text, &replacement_path);
  GURL url_loader = embedded_test_server()->GetURL(replacement_path);
  return url_loader;
}

bool NetInternalsTest::StartTestServer() {
  if (test_server_started_)
    return true;
  test_server_started_ = embedded_test_server()->Start();

  return test_server_started_;
}
