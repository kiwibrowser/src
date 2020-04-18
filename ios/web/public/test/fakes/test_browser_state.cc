// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/test/fakes/test_browser_state.h"

#include "base/files/file_path.h"
#include "base/single_thread_task_runner.h"
#include "ios/web/public/web_thread.h"
#include "ios/web/test/test_url_constants.h"
#include "ios/web/webui/url_data_manager_ios_backend.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "net/url_request/url_request_test_util.h"

namespace web {

namespace {

class TestContextURLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  TestContextURLRequestContextGetter(web::BrowserState* browser_state) {
    job_factory_.SetProtocolHandler(
        kTestWebUIScheme,
        web::URLDataManagerIOSBackend::CreateProtocolHandler(browser_state));
    context_.set_job_factory(&job_factory_);
  }

  net::URLRequestContext* GetURLRequestContext() override { return &context_; }

  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override {
    return web::WebThread::GetTaskRunnerForThread(web::WebThread::IO);
  }

 private:
  ~TestContextURLRequestContextGetter() override {}

  net::TestURLRequestContext context_;
  net::URLRequestJobFactoryImpl job_factory_;
};

}  // namespace

TestBrowserState::TestBrowserState() : is_off_the_record_(false) {
  BrowserState::Initialize(this, GetStatePath());
}

TestBrowserState::~TestBrowserState() {}

bool TestBrowserState::IsOffTheRecord() const {
  return is_off_the_record_;
}

base::FilePath TestBrowserState::GetStatePath() const {
  return base::FilePath();
}

net::URLRequestContextGetter* TestBrowserState::GetRequestContext() {
  if (!request_context_)
    request_context_ = new TestContextURLRequestContextGetter(this);
  return request_context_.get();
}

void TestBrowserState::SetOffTheRecord(bool flag) {
  is_off_the_record_ = flag;
}

}  // namespace web
