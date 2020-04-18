// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/services/proxy_resolution_service_provider.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_with_source.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "url/gurl.h"

namespace chromeos {

struct ProxyResolutionServiceProvider::Request {
 public:
  Request(const std::string& source_url,
          std::unique_ptr<dbus::Response> response,
          const dbus::ExportedObject::ResponseSender& response_sender,
          scoped_refptr<net::URLRequestContextGetter> context_getter)
      : source_url(source_url),
        response(std::move(response)),
        response_sender(response_sender),
        context_getter(context_getter) {
    DCHECK(this->response);
    DCHECK(!response_sender.is_null());
  }
  ~Request() = default;

  // URL being resolved.
  const std::string source_url;

  // D-Bus response and callback for returning data on resolution completion.
  std::unique_ptr<dbus::Response> response;
  const dbus::ExportedObject::ResponseSender response_sender;

  // Used to get the network context associated with the profile used to run
  // this request.
  const scoped_refptr<net::URLRequestContextGetter> context_getter;

  // ProxyInfo resolved for |source_url|.
  net::ProxyInfo proxy_info;

  // Error from proxy resolution.
  std::string error;

 private:
  DISALLOW_COPY_AND_ASSIGN(Request);
};

ProxyResolutionServiceProvider::ProxyResolutionServiceProvider(
    std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)),
      origin_thread_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {}

ProxyResolutionServiceProvider::~ProxyResolutionServiceProvider() {
  DCHECK(OnOriginThread());
}

void ProxyResolutionServiceProvider::Start(
    scoped_refptr<dbus::ExportedObject> exported_object) {
  DCHECK(OnOriginThread());
  exported_object_ = exported_object;
  VLOG(1) << "ProxyResolutionServiceProvider started";
  exported_object_->ExportMethod(
      kNetworkProxyServiceInterface, kNetworkProxyServiceResolveProxyMethod,
      base::Bind(&ProxyResolutionServiceProvider::ResolveProxy,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&ProxyResolutionServiceProvider::OnExported,
                 weak_ptr_factory_.GetWeakPtr()));
}

bool ProxyResolutionServiceProvider::OnOriginThread() {
  return origin_thread_->BelongsToCurrentThread();
}

void ProxyResolutionServiceProvider::OnExported(
    const std::string& interface_name,
    const std::string& method_name,
    bool success) {
  if (success)
    VLOG(1) << "Method exported: " << interface_name << "." << method_name;
  else
    LOG(ERROR) << "Failed to export " << interface_name << "." << method_name;
}

void ProxyResolutionServiceProvider::ResolveProxy(
    dbus::MethodCall* method_call,
    dbus::ExportedObject::ResponseSender response_sender) {
  DCHECK(OnOriginThread());

  VLOG(1) << "Handling method call: " << method_call->ToString();
  dbus::MessageReader reader(method_call);
  std::string source_url;
  if (!reader.PopString(&source_url)) {
    LOG(ERROR) << "Method call lacks source URL: " << method_call->ToString();
    response_sender.Run(dbus::ErrorResponse::FromMethodCall(
        method_call, DBUS_ERROR_INVALID_ARGS, "No source URL string arg"));
    return;
  }

  std::unique_ptr<dbus::Response> response =
      dbus::Response::FromMethodCall(method_call);
  scoped_refptr<net::URLRequestContextGetter> context_getter =
      delegate_->GetRequestContext();

  std::unique_ptr<Request> request = std::make_unique<Request>(
      source_url, std::move(response), response_sender, context_getter);
  NotifyCallback notify_callback =
      base::Bind(&ProxyResolutionServiceProvider::NotifyProxyResolved,
                 weak_ptr_factory_.GetWeakPtr());

  // This would ideally call PostTaskAndReply() instead of PostTask(), but
  // ResolveProxyOnNetworkThread()'s call to
  // net::ProxyResolutionService::ResolveProxy() can result in an asynchronous
  // lookup, in which case the result won't be available immediately.
  context_getter->GetNetworkTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ProxyResolutionServiceProvider::ResolveProxyOnNetworkThread,
          std::move(request), origin_thread_, notify_callback));
}

// static
void ProxyResolutionServiceProvider::ResolveProxyOnNetworkThread(
    std::unique_ptr<Request> request,
    scoped_refptr<base::SingleThreadTaskRunner> notify_thread,
    NotifyCallback notify_callback) {
  DCHECK(request->context_getter->GetNetworkTaskRunner()
             ->BelongsToCurrentThread());

  net::ProxyResolutionService* proxy_resolution_service =
      request->context_getter->GetURLRequestContext()->proxy_resolution_service();
  if (!proxy_resolution_service) {
    request->error = "No proxy service in chrome";
    OnResolutionComplete(std::move(request), notify_thread, notify_callback,
                         net::ERR_UNEXPECTED);
    return;
  }

  Request* request_ptr = request.get();
  net::CompletionCallback callback = base::Bind(
      &ProxyResolutionServiceProvider::OnResolutionComplete,
      base::Passed(std::move(request)), notify_thread, notify_callback);

  VLOG(1) << "Starting network proxy resolution for "
          << request_ptr->source_url;
  const int result = proxy_resolution_service->ResolveProxy(
      GURL(request_ptr->source_url), std::string(), &request_ptr->proxy_info,
      callback, nullptr, nullptr, net::NetLogWithSource());
  if (result != net::ERR_IO_PENDING) {
    VLOG(1) << "Network proxy resolution completed synchronously.";
    callback.Run(result);
  }
}

// static
void ProxyResolutionServiceProvider::OnResolutionComplete(
    std::unique_ptr<Request> request,
    scoped_refptr<base::SingleThreadTaskRunner> notify_thread,
    NotifyCallback notify_callback,
    int result) {
  DCHECK(request->context_getter->GetNetworkTaskRunner()
             ->BelongsToCurrentThread());

  if (request->error.empty() && result != net::OK)
    request->error = net::ErrorToString(result);

  notify_thread->PostTask(FROM_HERE,
                          base::BindOnce(notify_callback, std::move(request)));
}

void ProxyResolutionServiceProvider::NotifyProxyResolved(
    std::unique_ptr<Request> request) {
  DCHECK(OnOriginThread());

  // Reply to the original D-Bus method call.
  dbus::MessageWriter writer(request->response.get());
  writer.AppendString(request->proxy_info.ToPacString());
  writer.AppendString(request->error);
  request->response_sender.Run(std::move(request->response));
}

}  // namespace chromeos
