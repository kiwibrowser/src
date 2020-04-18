// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/browser_state.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/guid.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/process/process_handle.h"
#include "ios/web/public/certificate_policy_cache.h"
#include "ios/web/public/network_context_owner.h"
#include "ios/web/public/service_manager_connection.h"
#include "ios/web/public/service_names.mojom.h"
#include "ios/web/public/web_client.h"
#include "ios/web/public/web_thread.h"
#include "ios/web/webui/url_data_manager_ios_backend.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_context_getter_observer.h"
#include "services/network/network_context.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/mojom/service.mojom.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {
namespace {

// Maps service userIds to associated BrowserState instances.
base::LazyInstance<std::map<std::string, BrowserState*>>::DestructorAtExit
    g_user_id_to_browser_state = LAZY_INSTANCE_INITIALIZER;

// Private key used for safe conversion of base::SupportsUserData to
// web::BrowserState in web::BrowserState::FromSupportsUserData.
const char kBrowserStateIdentifierKey[] = "BrowserStateIdentifierKey";

// Data key names.
const char kCertificatePolicyCacheKeyName[] = "cert_policy_cache";
const char kMojoWasInitialized[] = "mojo-was-initialized";
const char kServiceManagerConnection[] = "service-manager-connection";
const char kServiceUserId[] = "service-user-id";

// Wraps a CertificatePolicyCache as a SupportsUserData::Data; this is necessary
// since reference counted objects can't be user data.
struct CertificatePolicyCacheHandle : public base::SupportsUserData::Data {
  explicit CertificatePolicyCacheHandle(CertificatePolicyCache* cache)
      : policy_cache(cache) {}

  scoped_refptr<CertificatePolicyCache> policy_cache;
};

// Container for a service userId to support association between BrowserStates
// and service UserIds.
class ServiceUserIdHolder : public base::SupportsUserData::Data {
 public:
  explicit ServiceUserIdHolder(const std::string& user_id)
      : user_id_(user_id) {}
  ~ServiceUserIdHolder() override {}

  const std::string& user_id() const { return user_id_; }

 private:
  std::string user_id_;

  DISALLOW_COPY_AND_ASSIGN(ServiceUserIdHolder);
};

// Eliminates the mapping from |browser_state|'s associated userId (if any) to
// |browser_state|.
void RemoveBrowserStateFromUserIdMap(BrowserState* browser_state) {
  ServiceUserIdHolder* holder = static_cast<ServiceUserIdHolder*>(
      browser_state->GetUserData(kServiceUserId));
  if (holder) {
    g_user_id_to_browser_state.Get().erase(holder->user_id());
  }
}

// Container for a ServiceManagerConnection to support association between
// a BrowserState and the ServiceManagerConnection initiated on behalf of that
// BrowserState.
class BrowserStateServiceManagerConnectionHolder
    : public base::SupportsUserData::Data {
 public:
  explicit BrowserStateServiceManagerConnectionHolder(
      service_manager::mojom::ServiceRequest request)
      : service_manager_connection_(ServiceManagerConnection::Create(
            std::move(request),
            WebThread::GetTaskRunnerForThread(WebThread::IO))) {}
  ~BrowserStateServiceManagerConnectionHolder() override {}

  ServiceManagerConnection* service_manager_connection() {
    return service_manager_connection_.get();
  }

 private:
  std::unique_ptr<ServiceManagerConnection> service_manager_connection_;

  DISALLOW_COPY_AND_ASSIGN(BrowserStateServiceManagerConnectionHolder);
};

}  // namespace

// static
scoped_refptr<CertificatePolicyCache> BrowserState::GetCertificatePolicyCache(
    BrowserState* browser_state) {
  DCHECK_CURRENTLY_ON(WebThread::UI);
  if (!browser_state->GetUserData(kCertificatePolicyCacheKeyName)) {
    browser_state->SetUserData(kCertificatePolicyCacheKeyName,
                               std::make_unique<CertificatePolicyCacheHandle>(
                                   new CertificatePolicyCache()));
  }

  CertificatePolicyCacheHandle* handle =
      static_cast<CertificatePolicyCacheHandle*>(
          browser_state->GetUserData(kCertificatePolicyCacheKeyName));
  return handle->policy_cache;
}

BrowserState::BrowserState() : url_data_manager_ios_backend_(nullptr) {
  // (Refcounted)?BrowserStateKeyedServiceFactories needs to be able to convert
  // a base::SupportsUserData to a BrowserState. Moreover, since the factories
  // may be passed a content::BrowserContext instead of a BrowserState, attach
  // an empty object to this via a private key.
  SetUserData(kBrowserStateIdentifierKey,
              std::make_unique<SupportsUserData::Data>());
}

BrowserState::~BrowserState() {
  CHECK(GetUserData(kMojoWasInitialized))
      << "Attempting to destroy a BrowserState that never called "
      << "Initialize()";

  if (network_context_) {
    web::WebThread::DeleteSoon(web::WebThread::IO, FROM_HERE,
                               network_context_owner_.release());
  }

  RemoveBrowserStateFromUserIdMap(this);

  // Delete the URLDataManagerIOSBackend instance on the IO thread if it has
  // been created. Note that while this check can theoretically race with a
  // call to |GetURLDataManagerIOSBackendOnIOThread()|, if any clients of this
  // BrowserState are still accessing it on the IO thread at this point,
  // they're going to have a bad time anyway.
  if (url_data_manager_ios_backend_) {
    bool posted = web::WebThread::DeleteSoon(web::WebThread::IO, FROM_HERE,
                                             url_data_manager_ios_backend_);
    if (!posted)
      delete url_data_manager_ios_backend_;
  }
}

network::mojom::URLLoaderFactory* BrowserState::GetURLLoaderFactory() {
  if (!url_loader_factory_) {
    DCHECK(!network_context_);
    DCHECK(!network_context_owner_);

    network_context_owner_ = std::make_unique<NetworkContextOwner>(
        GetRequestContext(), &network_context_);
    auto url_loader_factory_params =
        network::mojom::URLLoaderFactoryParams::New();
    url_loader_factory_params->process_id = network::mojom::kBrowserProcessId;
    url_loader_factory_params->is_corb_enabled = false;
    network_context_->CreateURLLoaderFactory(
        mojo::MakeRequest(&url_loader_factory_),
        std::move(url_loader_factory_params));
  }

  return url_loader_factory_.get();
}

URLDataManagerIOSBackend*
BrowserState::GetURLDataManagerIOSBackendOnIOThread() {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  if (!url_data_manager_ios_backend_)
    url_data_manager_ios_backend_ = new URLDataManagerIOSBackend();
  return url_data_manager_ios_backend_;
}

// static
BrowserState* BrowserState::FromSupportsUserData(
    base::SupportsUserData* supports_user_data) {
  if (!supports_user_data ||
      !supports_user_data->GetUserData(kBrowserStateIdentifierKey)) {
    return nullptr;
  }
  return static_cast<BrowserState*>(supports_user_data);
}

// static
void BrowserState::Initialize(BrowserState* browser_state,
                              const base::FilePath& path) {
  std::string new_id = base::GenerateGUID();

  // Note: If the file service is ever used on iOS, code needs to be added here
  // to have the file service associate |path| as the user dir of the user Id
  // of |browser_state| (see corresponding code in
  // content::BrowserContext::Initialize). crbug.com/739450

  RemoveBrowserStateFromUserIdMap(browser_state);
  g_user_id_to_browser_state.Get()[new_id] = browser_state;
  browser_state->SetUserData(kServiceUserId,
                             std::make_unique<ServiceUserIdHolder>(new_id));

  browser_state->SetUserData(kMojoWasInitialized,
                             std::make_unique<base::SupportsUserData::Data>());

  ServiceManagerConnection* service_manager_connection =
      ServiceManagerConnection::Get();
  if (service_manager_connection && base::ThreadTaskRunnerHandle::IsSet()) {
    // NOTE: Many unit tests create a TestBrowserState without initializing
    // Mojo or the global service manager connection.

    // Have the global service manager connection start an instance of the
    // web_browser service that is associated with this BrowserState (via
    // |new_id|).
    service_manager::mojom::ServicePtr service;
    auto service_request = mojo::MakeRequest(&service);

    service_manager::mojom::PIDReceiverPtr pid_receiver;
    service_manager::Identity identity(mojom::kBrowserServiceName, new_id);
    service_manager_connection->GetConnector()->StartService(
        identity, std::move(service), mojo::MakeRequest(&pid_receiver));
    pid_receiver->SetPID(base::GetCurrentProcId());

    service_manager_connection->GetConnector()->StartService(identity);
    auto connection_holder =
        std::make_unique<BrowserStateServiceManagerConnectionHolder>(
            std::move(service_request));

    ServiceManagerConnection* connection =
        connection_holder->service_manager_connection();

    browser_state->SetUserData(kServiceManagerConnection,
                               std::move(connection_holder));

    // New embedded service factories should be added to |connection| here.
    WebClient::StaticServiceMap services;
    browser_state->RegisterServices(&services);
    for (const auto& entry : services) {
      connection->AddEmbeddedService(entry.first, entry.second);
    }

    connection->Start();
  }
}

// static
const std::string& BrowserState::GetServiceUserIdFor(
    BrowserState* browser_state) {
  CHECK(browser_state->GetUserData(kMojoWasInitialized))
      << "Attempting to get the mojo user id for a BrowserState that was "
      << "never Initialize()ed.";

  ServiceUserIdHolder* holder = static_cast<ServiceUserIdHolder*>(
      browser_state->GetUserData(kServiceUserId));
  return holder->user_id();
}

// static
service_manager::Connector* BrowserState::GetConnectorFor(
    BrowserState* browser_state) {
  ServiceManagerConnection* connection =
      GetServiceManagerConnectionFor(browser_state);
  return connection ? connection->GetConnector() : nullptr;
}

// static
ServiceManagerConnection* BrowserState::GetServiceManagerConnectionFor(
    BrowserState* browser_state) {
  BrowserStateServiceManagerConnectionHolder* connection_holder =
      static_cast<BrowserStateServiceManagerConnectionHolder*>(
          browser_state->GetUserData(kServiceManagerConnection));
  return connection_holder ? connection_holder->service_manager_connection()
                           : nullptr;
}

}  // namespace web
