// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/net/ios_chrome_url_request_context_getter.h"

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_io_data.h"
#include "ios/chrome/browser/ios_chrome_io_thread.h"
#include "ios/web/public/web_thread.h"
#include "net/cookies/cookie_store.h"

class IOSChromeURLRequestContextFactory {
 public:
  IOSChromeURLRequestContextFactory() {}
  virtual ~IOSChromeURLRequestContextFactory() {}

  // Called to create a new instance (will only be called once).
  virtual net::URLRequestContext* Create() = 0;

 protected:
  DISALLOW_COPY_AND_ASSIGN(IOSChromeURLRequestContextFactory);
};

namespace {

// ----------------------------------------------------------------------------
// Helper factories
// ----------------------------------------------------------------------------

// Factory that creates the main URLRequestContext.
class FactoryForMain : public IOSChromeURLRequestContextFactory {
 public:
  FactoryForMain(const ChromeBrowserStateIOData* io_data,
                 ProtocolHandlerMap* protocol_handlers)
      : io_data_(io_data) {
    std::swap(protocol_handlers_, *protocol_handlers);
  }

  net::URLRequestContext* Create() override {
    io_data_->Init(&protocol_handlers_);
    return io_data_->GetMainRequestContext();
  }

 private:
  const ChromeBrowserStateIOData* const io_data_;
  ProtocolHandlerMap protocol_handlers_;
};

// Factory that creates the URLRequestContext for a given isolated app.
class FactoryForIsolatedApp : public IOSChromeURLRequestContextFactory {
 public:
  FactoryForIsolatedApp(const ChromeBrowserStateIOData* io_data,
                        const base::FilePath& partition_path,
                        net::URLRequestContextGetter* main_context)
      : io_data_(io_data),
        partition_path_(partition_path),
        main_request_context_getter_(main_context) {}

  net::URLRequestContext* Create() override {
    // We will copy most of the state from the main request context.
    //
    // Note that this factory is one-shot.  After Create() is called once, the
    // factory is actually destroyed. Thus it is safe to destructively pass
    // state onwards.
    return io_data_->GetIsolatedAppRequestContext(
        main_request_context_getter_->GetURLRequestContext(), partition_path_);
  }

 private:
  const ChromeBrowserStateIOData* const io_data_;
  const base::FilePath partition_path_;
  scoped_refptr<net::URLRequestContextGetter> main_request_context_getter_;
};

}  // namespace

// ----------------------------------------------------------------------------
// IOSChromeURLRequestContextGetter
// ----------------------------------------------------------------------------

IOSChromeURLRequestContextGetter::IOSChromeURLRequestContextGetter(
    std::unique_ptr<IOSChromeURLRequestContextFactory> factory)
    : factory_(std::move(factory)), url_request_context_(nullptr) {
  DCHECK(factory_);
}

IOSChromeURLRequestContextGetter::~IOSChromeURLRequestContextGetter() {
  // NotifyContextShuttingDown() must have been called.
  DCHECK(!factory_.get());
  DCHECK(!url_request_context_);
}

// Lazily create a URLRequestContext using our factory.
net::URLRequestContext*
IOSChromeURLRequestContextGetter::GetURLRequestContext() {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);

  if (factory_.get()) {
    DCHECK(!url_request_context_);
    url_request_context_ = factory_->Create();
    factory_.reset();
  }

  return url_request_context_;
}

void IOSChromeURLRequestContextGetter::NotifyContextShuttingDown() {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);

  factory_.reset();
  url_request_context_ = nullptr;
  URLRequestContextGetter::NotifyContextShuttingDown();
}

scoped_refptr<base::SingleThreadTaskRunner>
IOSChromeURLRequestContextGetter::GetNetworkTaskRunner() const {
  return web::WebThread::GetTaskRunnerForThread(web::WebThread::IO);
}

// static
IOSChromeURLRequestContextGetter* IOSChromeURLRequestContextGetter::Create(
    const ChromeBrowserStateIOData* io_data,
    ProtocolHandlerMap* protocol_handlers) {
  return new IOSChromeURLRequestContextGetter(
      std::make_unique<FactoryForMain>(io_data, protocol_handlers));
}

// static
IOSChromeURLRequestContextGetter*
IOSChromeURLRequestContextGetter::CreateForIsolatedApp(
    net::URLRequestContextGetter* main_context,
    const ChromeBrowserStateIOData* io_data,
    const base::FilePath& partition_path) {
  return new IOSChromeURLRequestContextGetter(
      std::make_unique<FactoryForIsolatedApp>(io_data, partition_path,
                                              main_context));
}
