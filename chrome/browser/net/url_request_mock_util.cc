// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/url_request_mock_util.h"

#include <string>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/browser_thread.h"
#include "net/test/url_request/url_request_failed_job.h"
#include "net/test/url_request/url_request_mock_http_job.h"
#include "net/test/url_request/url_request_slow_download_job.h"
#include "net/url_request/url_request_filter.h"
#include "net/url_request/url_request_test_job.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::BrowserThread;

namespace chrome_browser_net {

void SetUrlRequestMocksEnabled(bool enabled) {
  // Since this involves changing the net::URLRequest ProtocolFactory, we need
  // to run on the IO thread.
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (enabled) {
    // We have to look around for our helper files, but we only use
    // this from tests, so allow these IO operations to happen
    // anywhere.
    base::ThreadRestrictions::ScopedAllowIO allow_io;

    net::URLRequestFilter::GetInstance()->ClearHandlers();

    net::URLRequestFailedJob::AddUrlHandler();
    net::URLRequestSlowDownloadJob::AddUrlHandler();

    base::FilePath root_http;
    base::PathService::Get(chrome::DIR_TEST_DATA, &root_http);
    net::URLRequestMockHTTPJob::AddUrlHandlers(root_http);
  } else {
    // Revert to the default handlers.
    net::URLRequestFilter::GetInstance()->ClearHandlers();
  }
}

bool WriteFileToURLLoader(net::EmbeddedTestServer* test_server,
                          content::URLLoaderInterceptor::RequestParams* params,
                          std::string path) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  if (path[0] == '/')
    path.erase(0, 1);

  if (path == "favicon.ico")
    return false;

  base::FilePath file_path;
  base::PathService::Get(chrome::DIR_TEST_DATA, &file_path);
  file_path = file_path.AppendASCII(path);

  std::string contents;
  const bool result = base::ReadFileToString(file_path, &contents);
  EXPECT_TRUE(result);

  if (path == "mock-link-doctor.json") {
    GURL url = test_server->GetURL("mock.http", "/title2.html");

    std::string placeholder = "http://mock.http/title2.html";
    contents.replace(contents.find(placeholder), placeholder.length(),
                     url.spec());
  }

  content::URLLoaderInterceptor::WriteResponse(
      net::URLRequestTestJob::test_headers(), contents, params->client.get());
  return true;
}

}  // namespace chrome_browser_net
