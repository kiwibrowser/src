// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_SERVICES_PROXY_RESOLUTION_SERVICE_PROVIDER_H_
#define CHROMEOS_DBUS_SERVICES_PROXY_RESOLUTION_SERVICE_PROVIDER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/services/cros_dbus_service.h"
#include "dbus/exported_object.h"
#include "net/base/completion_callback.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace dbus {
class MethodCall;
}

namespace net {
class URLRequestContextGetter;
}

namespace chromeos {

// This class processes proxy resolution requests for Chrome OS clients.
//
// The following method is exported:
//
// Interface: org.chromium.NetworkProxyServiceInterface
//            (kNetworkProxyServiceInterface)
// Method: ResolveProxy (kNetworkProxyServiceResolveProxyMethod)
// Parameters: string:source_url
//
//   Resolves the proxy for |source_url| and returns proxy information via an
//   asynchronous response containing two values:
//
//   - string:proxy_info - proxy info for the source URL in PAC format
//                         like "PROXY cache.example.com:12345"
//   - string:error_message - error message. Empty if successful.
//
// This service can be manually tested using dbus-send:
//
//   % dbus-send --system --type=method_call --print-reply
//       --dest=org.chromium.NetworkProxyService
//       /org/chromium/NetworkProxyService
//       org.chromium.NetworkProxyServiceInterface.ResolveProxy
//       string:https://www.google.com/
//
class CHROMEOS_EXPORT ProxyResolutionServiceProvider
    : public CrosDBusService::ServiceProviderInterface {
 public:
  // Delegate interface providing additional resources to
  // ProxyResolutionServiceProvider.
  class CHROMEOS_EXPORT Delegate {
   public:
    virtual ~Delegate() {}

    // Returns the request context used to perform proxy resolution.
    // Always called on UI thread.
    virtual scoped_refptr<net::URLRequestContextGetter> GetRequestContext() = 0;
  };

  explicit ProxyResolutionServiceProvider(std::unique_ptr<Delegate> delegate);
  ~ProxyResolutionServiceProvider() override;

  // CrosDBusService::ServiceProviderInterface:
  void Start(scoped_refptr<dbus::ExportedObject> exported_object) override;

 private:
  // Data used for a single proxy resolution.
  struct Request;

  // Returns true if called on |origin_thread_|.
  bool OnOriginThread();

  // Called when ResolveProxy() is exported as a D-Bus method.
  void OnExported(const std::string& interface_name,
                  const std::string& method_name,
                  bool success);

  // Callback invoked when Chrome OS clients send network proxy resolution
  // requests to the service. Called on UI thread.
  void ResolveProxy(dbus::MethodCall* method_call,
                    dbus::ExportedObject::ResponseSender response_sender);

  // Callback passed to network thread static methods to run
  // NotifyProxyResolved() on |origin_thread_|. This callback can be bound to a
  // WeakPtr from |weak_ptr_factory_| (since the pointer will be dereferenced on
  // |origin_thread_|), but the network methods can't (since WeakPtr disallows
  // use on threads besides the one where it was created) and are static as a
  // result.
  using NotifyCallback = base::Callback<void(std::unique_ptr<Request>)>;

  // Helper method for ResolveProxy() that runs on network thread.
  static void ResolveProxyOnNetworkThread(
      std::unique_ptr<Request> request,
      scoped_refptr<base::SingleThreadTaskRunner> notify_thread,
      NotifyCallback notify_callback);

  // Callback on network thread for when
  // net::ProxyResolutionService::ResolveProxy() completes, synchronously or
  // asynchronously.
  static void OnResolutionComplete(
      std::unique_ptr<Request> request,
      scoped_refptr<base::SingleThreadTaskRunner> notify_thread,
      NotifyCallback notify_callback,
      int result);

  // Called on UI thread from OnResolutionComplete() to pass the resolved proxy
  // information to the client over D-Bus.
  void NotifyProxyResolved(std::unique_ptr<Request> request);

  std::unique_ptr<Delegate> delegate_;
  scoped_refptr<dbus::ExportedObject> exported_object_;
  scoped_refptr<base::SingleThreadTaskRunner> origin_thread_;
  base::WeakPtrFactory<ProxyResolutionServiceProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProxyResolutionServiceProvider);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_SERVICES_PROXY_RESOLUTION_SERVICE_PROVIDER_H_
