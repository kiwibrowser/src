// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/values_test_util.h"
#include "chrome/browser/domain_reliability/service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/domain_reliability/service.h"
#include "net/base/net_errors.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/url_request/url_request_failed_job.h"
#include "url/gurl.h"

namespace domain_reliability {

class DomainReliabilityBrowserTest : public InProcessBrowserTest {
 public:
  DomainReliabilityBrowserTest() {
    net::URLRequestFailedJob::AddUrlHandler();
  }

  ~DomainReliabilityBrowserTest() override {}

  // Note: In an ideal world, instead of appending the command-line switch and
  // manually setting discard_uploads to false, Domain Reliability would
  // continuously monitor the metrics reporting pref, and the test could just
  // set the pref.

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kEnableDomainReliability);
  }

  void SetUpOnMainThread() override {
    DomainReliabilityService* service = GetService();
    if (service)
      service->SetDiscardUploadsForTesting(false);
  }

  DomainReliabilityService* GetService() {
    return DomainReliabilityServiceFactory::GetForBrowserContext(
        browser()->profile());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DomainReliabilityBrowserTest);
};

class DomainReliabilityDisabledBrowserTest
    : public DomainReliabilityBrowserTest {
 protected:
  DomainReliabilityDisabledBrowserTest() {}

  ~DomainReliabilityDisabledBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kDisableDomainReliability);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DomainReliabilityDisabledBrowserTest);
};

IN_PROC_BROWSER_TEST_F(DomainReliabilityDisabledBrowserTest,
                       ServiceNotCreated) {
  EXPECT_FALSE(GetService());
}

IN_PROC_BROWSER_TEST_F(DomainReliabilityBrowserTest, ServiceCreated) {
  EXPECT_TRUE(GetService());
}

static const char kUploadPath[] = "/domainreliability/upload";

std::unique_ptr<net::test_server::HttpResponse> TestRequestHandler(
    int* request_count_out,
    std::string* last_request_content_out,
    const base::Closure& quit_closure,
    const net::test_server::HttpRequest& request) {
  if (request.relative_url != kUploadPath)
    return std::unique_ptr<net::test_server::HttpResponse>();

  ++*request_count_out;
  *last_request_content_out = request.has_content ? request.content : "";

  quit_closure.Run();

  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  response->set_content("");
  response->set_content_type("text/plain");
  return std::move(response);
}

IN_PROC_BROWSER_TEST_F(DomainReliabilityBrowserTest, Upload) {
  DomainReliabilityService* service = GetService();

  base::RunLoop run_loop;

  net::test_server::EmbeddedTestServer test_server(
      (net::test_server::EmbeddedTestServer::TYPE_HTTPS));

  // This is cribbed from //chrome/test/ppapi/ppapi_test.cc; it shouldn't
  // matter, as we don't actually use any of the handlers that access the
  // filesystem.
  base::FilePath document_root;
  ASSERT_TRUE(ui_test_utils::GetRelativeBuildDirectory(&document_root));
  test_server.AddDefaultHandlers(document_root);

  // Register a same-origin collector to receive report uploads so we can check
  // the full path. (Domain Reliability elides the path for privacy reasons when
  // uploading to non-same-origin collectors.)
  int request_count = 0;
  std::string last_request_content;
  test_server.RegisterRequestHandler(
      base::Bind(&TestRequestHandler, &request_count, &last_request_content,
                 run_loop.QuitClosure()));

  ASSERT_TRUE(test_server.Start());

  GURL error_url = test_server.GetURL("/close-socket");
  GURL upload_url = test_server.GetURL(kUploadPath);

  auto config = std::make_unique<DomainReliabilityConfig>();
  config->origin = test_server.base_url().GetOrigin();
  config->include_subdomains = false;
  config->collectors.push_back(std::make_unique<GURL>(upload_url));
  config->success_sample_rate = 1.0;
  config->failure_sample_rate = 1.0;
  service->AddContextForTesting(std::move(config));

  // Trigger an error.

  ui_test_utils::NavigateToURL(browser(), error_url);

  service->ForceUploadsForTesting();

  run_loop.Run();

  EXPECT_EQ(1, request_count);
  EXPECT_NE("", last_request_content);

  auto body = base::JSONReader::Read(last_request_content);
  ASSERT_TRUE(body);

  const base::DictionaryValue* dict;
  ASSERT_TRUE(body->GetAsDictionary(&dict));

  const base::ListValue* entries;
  ASSERT_TRUE(dict->GetList("entries", &entries));
  ASSERT_EQ(1u, entries->GetSize());

  const base::DictionaryValue* entry;
  ASSERT_TRUE(entries->GetDictionary(0u, &entry));

  std::string url;
  ASSERT_TRUE(entry->GetString("url", &url));
  EXPECT_EQ(url, error_url);
}

IN_PROC_BROWSER_TEST_F(DomainReliabilityBrowserTest, UploadAtShutdown) {
  DomainReliabilityService* service = GetService();

  auto config = std::make_unique<DomainReliabilityConfig>();
  config->origin = GURL("https://localhost/");
  config->include_subdomains = false;
  config->collectors.push_back(std::make_unique<GURL>(
      net::URLRequestFailedJob::GetMockHttpsUrl(net::ERR_IO_PENDING)));
  config->success_sample_rate = 1.0;
  config->failure_sample_rate = 1.0;
  service->AddContextForTesting(std::move(config));

  ui_test_utils::NavigateToURL(browser(), GURL("https://localhost/"));

  service->ForceUploadsForTesting();

  // At this point, there is an upload pending. If everything goes well, the
  // test will finish, destroy the profile, and Domain Reliability will shut
  // down properly. If things go awry, it may crash as terminating the pending
  // upload calls into already-destroyed parts of the component.
}

}  // namespace domain_reliability
