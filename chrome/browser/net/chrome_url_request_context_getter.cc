// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/chrome_url_request_context_getter.h"

#include <utility>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/browser/profiles/storage_partition_descriptor.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

class ChromeURLRequestContextFactory {
 public:
  ChromeURLRequestContextFactory() {}
  virtual ~ChromeURLRequestContextFactory() {}

  // Called to create a new instance (will only be called once).
  virtual net::URLRequestContext* Create() = 0;

 protected:
  DISALLOW_COPY_AND_ASSIGN(ChromeURLRequestContextFactory);
};

namespace {

// ----------------------------------------------------------------------------
// Helper factories
// ----------------------------------------------------------------------------

// Factory that creates the main URLRequestContext.
class FactoryForMain : public ChromeURLRequestContextFactory {
 public:
  FactoryForMain(
      const ProfileIOData* profile_io_data,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors)
      : profile_io_data_(profile_io_data),
        request_interceptors_(std::move(request_interceptors)) {
    std::swap(protocol_handlers_, *protocol_handlers);
  }

  net::URLRequestContext* Create() override {
    profile_io_data_->Init(&protocol_handlers_,
                           std::move(request_interceptors_));
    return profile_io_data_->GetMainRequestContext();
  }

 private:
  const ProfileIOData* const profile_io_data_;
  content::ProtocolHandlerMap protocol_handlers_;
  content::URLRequestInterceptorScopedVector request_interceptors_;
};

// Factory that creates the URLRequestContext for extensions.
class FactoryForExtensions : public ChromeURLRequestContextFactory {
 public:
  explicit FactoryForExtensions(const ProfileIOData* profile_io_data)
      : profile_io_data_(profile_io_data) {}

  net::URLRequestContext* Create() override {
    return profile_io_data_->GetExtensionsRequestContext();
  }

 private:
  const ProfileIOData* const profile_io_data_;
};

// Factory that creates the URLRequestContext for a given isolated app.
class FactoryForIsolatedApp : public ChromeURLRequestContextFactory {
 public:
  FactoryForIsolatedApp(
      const ProfileIOData* profile_io_data,
      const StoragePartitionDescriptor& partition_descriptor,
      ChromeURLRequestContextGetter* main_context,
      std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
          protocol_handler_interceptor,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors)
      : profile_io_data_(profile_io_data),
        partition_descriptor_(partition_descriptor),
        main_request_context_getter_(main_context),
        protocol_handler_interceptor_(std::move(protocol_handler_interceptor)),
        request_interceptors_(std::move(request_interceptors)) {
    std::swap(protocol_handlers_, *protocol_handlers);
  }

  net::URLRequestContext* Create() override {
    // We will copy most of the state from the main request context.
    //
    // Note that this factory is one-shot.  After Create() is called once, the
    // factory is actually destroyed. Thus it is safe to destructively pass
    // state onwards.
    return profile_io_data_->GetIsolatedAppRequestContext(
        main_request_context_getter_->GetURLRequestContext(),
        partition_descriptor_, std::move(protocol_handler_interceptor_),
        &protocol_handlers_, std::move(request_interceptors_));
  }

 private:
  const ProfileIOData* const profile_io_data_;
  const StoragePartitionDescriptor partition_descriptor_;
  scoped_refptr<ChromeURLRequestContextGetter>
      main_request_context_getter_;
  std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
      protocol_handler_interceptor_;
  content::ProtocolHandlerMap protocol_handlers_;
  content::URLRequestInterceptorScopedVector request_interceptors_;
};

// Factory that creates the media URLRequestContext for a given isolated
// app.  The media context is based on the corresponding isolated app's context.
class FactoryForIsolatedMedia : public ChromeURLRequestContextFactory {
 public:
  FactoryForIsolatedMedia(
      const ProfileIOData* profile_io_data,
      const StoragePartitionDescriptor& partition_descriptor,
      ChromeURLRequestContextGetter* app_context)
    : profile_io_data_(profile_io_data),
      partition_descriptor_(partition_descriptor),
      app_context_getter_(app_context) {}

  net::URLRequestContext* Create() override {
    // We will copy most of the state from the corresopnding app's
    // request context. We expect to have the same lifetime as
    // the associated |app_context_getter_| so we can just reuse
    // all its backing objects, including the
    // |protocol_handler_interceptor|.  This is why the API
    // looks different from FactoryForIsolatedApp's.
    return profile_io_data_->GetIsolatedMediaRequestContext(
        app_context_getter_->GetURLRequestContext(), partition_descriptor_);
  }

 private:
  const ProfileIOData* const profile_io_data_;
  const StoragePartitionDescriptor partition_descriptor_;
  scoped_refptr<ChromeURLRequestContextGetter> app_context_getter_;
};

// Factory that creates the URLRequestContext for media.
class FactoryForMedia : public ChromeURLRequestContextFactory {
 public:
  explicit FactoryForMedia(const ProfileIOData* profile_io_data)
      : profile_io_data_(profile_io_data) {
  }

  net::URLRequestContext* Create() override {
    return profile_io_data_->GetMediaRequestContext();
  }

 private:
  const ProfileIOData* const profile_io_data_;
};

}  // namespace

// ----------------------------------------------------------------------------
// ChromeURLRequestContextGetter
// ----------------------------------------------------------------------------

ChromeURLRequestContextGetter::ChromeURLRequestContextGetter(
    ChromeURLRequestContextFactory* factory)
    : factory_(factory),
      url_request_context_(nullptr) {
  DCHECK(factory);
}

ChromeURLRequestContextGetter::~ChromeURLRequestContextGetter() {
  // NotifyContextShuttingDown() must have been called.
  DCHECK(!factory_.get());
  DCHECK(!url_request_context_);
}

// Lazily create a URLRequestContext using our factory.
net::URLRequestContext*
ChromeURLRequestContextGetter::GetURLRequestContext() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (factory_.get()) {
    DCHECK(!url_request_context_);
    url_request_context_ = factory_->Create();
    factory_.reset();
  }

  return url_request_context_;
}

void ChromeURLRequestContextGetter::NotifyContextShuttingDown() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  factory_.reset();
  url_request_context_ = nullptr;
  URLRequestContextGetter::NotifyContextShuttingDown();
}

scoped_refptr<base::SingleThreadTaskRunner>
ChromeURLRequestContextGetter::GetNetworkTaskRunner() const {
  return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
}

// static
ChromeURLRequestContextGetter* ChromeURLRequestContextGetter::Create(
    Profile* profile,
    const ProfileIOData* profile_io_data,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  return new ChromeURLRequestContextGetter(new FactoryForMain(
      profile_io_data, protocol_handlers, std::move(request_interceptors)));
}

// static
ChromeURLRequestContextGetter*
ChromeURLRequestContextGetter::CreateForMedia(
    Profile* profile, const ProfileIOData* profile_io_data) {
  return new ChromeURLRequestContextGetter(
      new FactoryForMedia(profile_io_data));
}

// static
ChromeURLRequestContextGetter*
ChromeURLRequestContextGetter::CreateForExtensions(
    Profile* profile, const ProfileIOData* profile_io_data) {
  return new ChromeURLRequestContextGetter(
      new FactoryForExtensions(profile_io_data));
}

// static
ChromeURLRequestContextGetter*
ChromeURLRequestContextGetter::CreateForIsolatedApp(
    Profile* profile,
    const ProfileIOData* profile_io_data,
    const StoragePartitionDescriptor& partition_descriptor,
    std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
        protocol_handler_interceptor,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  ChromeURLRequestContextGetter* main_context =
      static_cast<ChromeURLRequestContextGetter*>(profile->GetRequestContext());
  return new ChromeURLRequestContextGetter(new FactoryForIsolatedApp(
      profile_io_data, partition_descriptor, main_context,
      std::move(protocol_handler_interceptor), protocol_handlers,
      std::move(request_interceptors)));
}

// static
ChromeURLRequestContextGetter*
ChromeURLRequestContextGetter::CreateForIsolatedMedia(
    Profile* profile,
    ChromeURLRequestContextGetter* app_context,
    const ProfileIOData* profile_io_data,
    const StoragePartitionDescriptor& partition_descriptor) {
  return new ChromeURLRequestContextGetter(
      new FactoryForIsolatedMedia(
          profile_io_data, partition_descriptor, app_context));
}
