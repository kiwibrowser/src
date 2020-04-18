// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/proxy_resolver_factory_mojo.h"

#include <set>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/sequence_checker.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/base/load_states.h"
#include "net/base/net_errors.h"
#include "net/dns/mojo_host_resolver_impl.h"
#include "net/interfaces/host_resolver_service.mojom.h"
#include "net/log/net_log.h"
#include "net/log/net_log_capture_mode.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_with_source.h"
#include "net/proxy_resolution/pac_file_data.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/proxy_resolution/proxy_resolver.h"
#include "net/proxy_resolution/proxy_resolver_error_observer.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"

namespace network {

namespace {

std::unique_ptr<base::Value> NetLogErrorCallback(
    int line_number,
    const std::string* message,
    net::NetLogCaptureMode /* capture_mode */) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetInteger("line_number", line_number);
  dict->SetString("message", *message);
  return std::move(dict);
}

// A mixin that forwards logging to (Bound)NetLog and ProxyResolverErrorObserver
// and DNS requests to a MojoHostResolverImpl, which is implemented in terms of
// a HostResolver.
template <typename ClientInterface>
class ClientMixin : public ClientInterface {
 public:
  ClientMixin(net::HostResolver* host_resolver,
              net::ProxyResolverErrorObserver* error_observer,
              net::NetLog* net_log,
              const net::NetLogWithSource& net_log_with_source)
      : host_resolver_(host_resolver, net_log_with_source),
        error_observer_(error_observer),
        net_log_(net_log),
        net_log_with_source_(net_log_with_source) {}

  // Overridden from ClientInterface:
  void Alert(const std::string& message) override {
    auto callback = net::NetLog::StringCallback("message", &message);
    net_log_with_source_.AddEvent(net::NetLogEventType::PAC_JAVASCRIPT_ALERT,
                                  callback);
    if (net_log_)
      net_log_->AddGlobalEntry(net::NetLogEventType::PAC_JAVASCRIPT_ALERT,
                               callback);
  }

  void OnError(int32_t line_number, const std::string& message) override {
    auto callback = base::Bind(&NetLogErrorCallback, line_number, &message);
    net_log_with_source_.AddEvent(net::NetLogEventType::PAC_JAVASCRIPT_ERROR,
                                  callback);
    if (net_log_)
      net_log_->AddGlobalEntry(net::NetLogEventType::PAC_JAVASCRIPT_ERROR,
                               callback);
    if (error_observer_) {
      error_observer_->OnPACScriptError(line_number,
                                        base::UTF8ToUTF16(message));
    }
  }

  void ResolveDns(
      std::unique_ptr<net::HostResolver::RequestInfo> request_info,
      net::interfaces::HostResolverRequestClientPtr client) override {
    host_resolver_.Resolve(std::move(request_info), std::move(client));
  }

 protected:
  bool dns_request_in_progress() {
    return host_resolver_.request_in_progress();
  }

 private:
  net::MojoHostResolverImpl host_resolver_;
  net::ProxyResolverErrorObserver* const error_observer_;
  net::NetLog* const net_log_;
  const net::NetLogWithSource net_log_with_source_;
};

// Implementation of ProxyResolver that connects to a Mojo service to evaluate
// PAC scripts. This implementation only knows about Mojo services, and
// therefore that service may live in or out of process.
//
// This implementation reports disconnections from the Mojo service (i.e. if the
// service is out-of-process and that process crashes) using the error code
// ERR_PAC_SCRIPT_TERMINATED.
class ProxyResolverMojo : public net::ProxyResolver {
 public:
  // Constructs a ProxyResolverMojo that connects to a mojo proxy resolver
  // implementation using |resolver_ptr|. The implementation uses
  // |host_resolver| as the DNS resolver, using |host_resolver_binding| to
  // communicate with it.
  ProxyResolverMojo(
      proxy_resolver::mojom::ProxyResolverPtr resolver_ptr,
      net::HostResolver* host_resolver,
      std::unique_ptr<net::ProxyResolverErrorObserver> error_observer,
      net::NetLog* net_log);
  ~ProxyResolverMojo() override;

  // ProxyResolver implementation:
  int GetProxyForURL(const GURL& url,
                     net::ProxyInfo* results,
                     const net::CompletionCallback& callback,
                     std::unique_ptr<Request>* request,
                     const net::NetLogWithSource& net_log) override;

 private:
  class Job;

  SEQUENCE_CHECKER(sequence_checker_);

  // Mojo error handler.
  void OnConnectionError();

  // Connection to the Mojo proxy resolver.
  proxy_resolver::mojom::ProxyResolverPtr mojo_proxy_resolver_ptr_;

  net::HostResolver* host_resolver_;

  std::unique_ptr<net::ProxyResolverErrorObserver> error_observer_;

  net::NetLog* net_log_;

  DISALLOW_COPY_AND_ASSIGN(ProxyResolverMojo);
};

class ProxyResolverMojo::Job
    : public ProxyResolver::Request,
      public ClientMixin<proxy_resolver::mojom::ProxyResolverRequestClient> {
 public:
  Job(ProxyResolverMojo* resolver,
      const GURL& url,
      net::ProxyInfo* results,
      const net::CompletionCallback& callback,
      const net::NetLogWithSource& net_log);
  ~Job() override;

  // Returns the LoadState of this job.
  net::LoadState GetLoadState() override;

 private:
  // Mojo error handler.
  void OnConnectionError();

  // Overridden from proxy_resolver::mojom::ProxyResolverRequestClient:
  void ReportResult(int32_t error, const net::ProxyInfo& proxy_info) override;

  // Completes a request with a result code.
  void CompleteRequest(int result);

  const GURL url_;
  net::ProxyInfo* results_;
  net::CompletionCallback callback_;

  SEQUENCE_CHECKER(sequence_checker_);
  mojo::Binding<proxy_resolver::mojom::ProxyResolverRequestClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(Job);
};

ProxyResolverMojo::Job::Job(ProxyResolverMojo* resolver,
                            const GURL& url,
                            net::ProxyInfo* results,
                            const net::CompletionCallback& callback,
                            const net::NetLogWithSource& net_log)
    : ClientMixin<proxy_resolver::mojom::ProxyResolverRequestClient>(
          resolver->host_resolver_,
          resolver->error_observer_.get(),
          resolver->net_log_,
          net_log),
      url_(url),
      results_(results),
      callback_(callback),
      binding_(this) {
  proxy_resolver::mojom::ProxyResolverRequestClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  resolver->mojo_proxy_resolver_ptr_->GetProxyForUrl(url_, std::move(client));
  binding_.set_connection_error_handler(base::Bind(
      &ProxyResolverMojo::Job::OnConnectionError, base::Unretained(this)));
}

ProxyResolverMojo::Job::~Job() {}

net::LoadState ProxyResolverMojo::Job::GetLoadState() {
  return dns_request_in_progress() ? net::LOAD_STATE_RESOLVING_HOST_IN_PAC_FILE
                                   : net::LOAD_STATE_RESOLVING_PROXY_FOR_URL;
}

void ProxyResolverMojo::Job::OnConnectionError() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << "ProxyResolverMojo::Job::OnConnectionError";
  CompleteRequest(net::ERR_PAC_SCRIPT_TERMINATED);
}

void ProxyResolverMojo::Job::CompleteRequest(int result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  net::CompletionCallback callback = base::ResetAndReturn(&callback_);
  binding_.Close();
  callback.Run(result);
}

void ProxyResolverMojo::Job::ReportResult(int32_t error,
                                          const net::ProxyInfo& proxy_info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << "ProxyResolverMojo::Job::ReportResult: " << error;

  if (error == net::OK) {
    *results_ = proxy_info;
    DVLOG(1) << "Servers: " << results_->ToPacString();
  }

  CompleteRequest(error);
}

ProxyResolverMojo::ProxyResolverMojo(
    proxy_resolver::mojom::ProxyResolverPtr resolver_ptr,
    net::HostResolver* host_resolver,
    std::unique_ptr<net::ProxyResolverErrorObserver> error_observer,
    net::NetLog* net_log)
    : mojo_proxy_resolver_ptr_(std::move(resolver_ptr)),
      host_resolver_(host_resolver),
      error_observer_(std::move(error_observer)),
      net_log_(net_log) {
  mojo_proxy_resolver_ptr_.set_connection_error_handler(base::Bind(
      &ProxyResolverMojo::OnConnectionError, base::Unretained(this)));
}

ProxyResolverMojo::~ProxyResolverMojo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void ProxyResolverMojo::OnConnectionError() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << "ProxyResolverMojo::OnConnectionError";

  // Disconnect from the Mojo proxy resolver service.
  mojo_proxy_resolver_ptr_.reset();
}

int ProxyResolverMojo::GetProxyForURL(const GURL& url,
                                      net::ProxyInfo* results,
                                      const net::CompletionCallback& callback,
                                      std::unique_ptr<Request>* request,
                                      const net::NetLogWithSource& net_log) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!mojo_proxy_resolver_ptr_)
    return net::ERR_PAC_SCRIPT_TERMINATED;

  *request = std::make_unique<Job>(this, url, results, callback, net_log);

  return net::ERR_IO_PENDING;
}

}  // namespace

// A Job to create a ProxyResolver instance.
//
// Note: a Job instance is not tied to a particular resolve request, and hence
// there is no per-request logging to be done (any netlog events are only sent
// globally) so this always uses an empty NetLogWithSource.
class ProxyResolverFactoryMojo::Job
    : public ClientMixin<
          proxy_resolver::mojom::ProxyResolverFactoryRequestClient>,
      public ProxyResolverFactory::Request {
 public:
  Job(ProxyResolverFactoryMojo* factory,
      const scoped_refptr<net::PacFileData>& pac_script,
      std::unique_ptr<net::ProxyResolver>* resolver,
      const net::CompletionCallback& callback,
      std::unique_ptr<net::ProxyResolverErrorObserver> error_observer)
      : ClientMixin<proxy_resolver::mojom::ProxyResolverFactoryRequestClient>(
            factory->host_resolver_,
            error_observer.get(),
            factory->net_log_,
            net::NetLogWithSource()),
        factory_(factory),
        resolver_(resolver),
        callback_(callback),
        binding_(this),
        error_observer_(std::move(error_observer)) {
    proxy_resolver::mojom::ProxyResolverFactoryRequestClientPtr client;
    binding_.Bind(mojo::MakeRequest(&client));
    factory_->mojo_proxy_factory_->CreateResolver(
        base::UTF16ToUTF8(pac_script->utf16()),
        mojo::MakeRequest(&resolver_ptr_), std::move(client));
    resolver_ptr_.set_connection_error_handler(
        base::Bind(&ProxyResolverFactoryMojo::Job::OnConnectionError,
                   base::Unretained(this)));
    binding_.set_connection_error_handler(
        base::Bind(&ProxyResolverFactoryMojo::Job::OnConnectionError,
                   base::Unretained(this)));
  }

  void OnConnectionError() { ReportResult(net::ERR_PAC_SCRIPT_TERMINATED); }

 private:
  void ReportResult(int32_t error) override {
    resolver_ptr_.set_connection_error_handler(base::Closure());
    binding_.set_connection_error_handler(base::Closure());
    if (error == net::OK) {
      resolver_->reset(new ProxyResolverMojo(
          std::move(resolver_ptr_), factory_->host_resolver_,
          std::move(error_observer_), factory_->net_log_));
    }
    callback_.Run(error);
  }

  ProxyResolverFactoryMojo* const factory_;
  std::unique_ptr<net::ProxyResolver>* resolver_;
  const net::CompletionCallback callback_;
  proxy_resolver::mojom::ProxyResolverPtr resolver_ptr_;
  mojo::Binding<proxy_resolver::mojom::ProxyResolverFactoryRequestClient>
      binding_;
  std::unique_ptr<net::ProxyResolverErrorObserver> error_observer_;
};

ProxyResolverFactoryMojo::ProxyResolverFactoryMojo(
    proxy_resolver::mojom::ProxyResolverFactoryPtr mojo_proxy_factory,
    net::HostResolver* host_resolver,
    const base::Callback<std::unique_ptr<net::ProxyResolverErrorObserver>()>&
        error_observer_factory,
    net::NetLog* net_log)
    : ProxyResolverFactory(true),
      mojo_proxy_factory_(std::move(mojo_proxy_factory)),
      host_resolver_(host_resolver),
      error_observer_factory_(error_observer_factory),
      net_log_(net_log),
      weak_ptr_factory_(this) {}

ProxyResolverFactoryMojo::~ProxyResolverFactoryMojo() = default;

int ProxyResolverFactoryMojo::CreateProxyResolver(
    const scoped_refptr<net::PacFileData>& pac_script,
    std::unique_ptr<net::ProxyResolver>* resolver,
    const net::CompletionCallback& callback,
    std::unique_ptr<net::ProxyResolverFactory::Request>* request) {
  DCHECK(resolver);
  DCHECK(request);
  if (pac_script->type() != net::PacFileData::TYPE_SCRIPT_CONTENTS ||
      pac_script->utf16().empty()) {
    return net::ERR_PAC_SCRIPT_FAILED;
  }
  request->reset(new Job(this, pac_script, resolver, callback,
                         error_observer_factory_.is_null()
                             ? nullptr
                             : error_observer_factory_.Run()));
  return net::ERR_IO_PENDING;
}

}  // namespace network
