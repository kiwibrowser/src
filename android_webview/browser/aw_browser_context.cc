// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_browser_context.h"

#include <memory>
#include <utility>

#include "android_webview/browser/aw_browser_policy_connector.h"
#include "android_webview/browser/aw_download_manager_delegate.h"
#include "android_webview/browser/aw_form_database_service.h"
#include "android_webview/browser/aw_metrics_service_client.h"
#include "android_webview/browser/aw_permission_manager.h"
#include "android_webview/browser/aw_quota_manager_bridge.h"
#include "android_webview/browser/aw_resource_context.h"
#include "android_webview/browser/aw_safe_browsing_whitelist_manager.h"
#include "android_webview/browser/aw_web_ui_controller_factory.h"
#include "android_webview/browser/net/aw_url_request_context_getter.h"
#include "android_webview/common/aw_content_client.h"
#include "base/base_paths_android.h"
#include "base/bind.h"
#include "base/path_service.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/metrics/metrics_service.h"
#include "components/policy/core/browser/browser_policy_connector_base.h"
#include "components/policy/core/browser/configuration_policy_pref_store.h"
#include "components/policy/core/browser/url_blacklist_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/triggers/trigger_manager.h"
#include "components/url_formatter/url_fixer.h"
#include "components/user_prefs/user_prefs.h"
#include "components/visitedlink/browser/visitedlink_master.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/ssl_host_state_delegate.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/proxy_resolution/proxy_config_service_android.h"
#include "net/proxy_resolution/proxy_resolution_service.h"

using base::FilePath;
using content::BrowserThread;

namespace android_webview {

namespace prefs {

// String that specifies the Android account type to use for Negotiate
// authentication.
const char kAuthAndroidNegotiateAccountType[] =
    "auth.android_negotiate_account_type";

// Whitelist containing servers for which Integrated Authentication is enabled.
const char kAuthServerWhitelist[] = "auth.server_whitelist";

const char kWebRestrictionsAuthority[] = "web_restrictions_authority";

}  // namespace prefs

namespace {

const base::FilePath::CharType kChannelIDFilename[] = "Origin Bound Certs";

const void* const kDownloadManagerDelegateKey = &kDownloadManagerDelegateKey;

// Shows notifications which correspond to PersistentPrefStore's reading errors.
void HandleReadError(PersistentPrefStore::PrefReadError error) {
}

AwBrowserContext* g_browser_context = NULL;

std::unique_ptr<net::ProxyConfigService> CreateProxyConfigService() {
  std::unique_ptr<net::ProxyConfigService> config_service =
      net::ProxyResolutionService::CreateSystemProxyConfigService(
          BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));

  // TODO(csharrison) Architect the wrapper better so we don't need a cast for
  // android ProxyConfigServices.
  net::ProxyConfigServiceAndroid* android_config_service =
      static_cast<net::ProxyConfigServiceAndroid*>(config_service.get());
  android_config_service->set_exclude_pac_url(true);
  return config_service;
}

std::unique_ptr<AwSafeBrowsingWhitelistManager>
CreateSafeBrowsingWhitelistManager() {
  // Should not be called until the end of PreMainMessageLoopRun,
  scoped_refptr<base::SequencedTaskRunner> background_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND});
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner =
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
  return std::make_unique<AwSafeBrowsingWhitelistManager>(
      background_task_runner, io_task_runner);
}

}  // namespace

AwBrowserContext::AwBrowserContext(const FilePath path)
    : context_storage_path_(path) {
  DCHECK(!g_browser_context);
  g_browser_context = this;
  BrowserContext::Initialize(this, path);

  // This constructor is entered during the creation of ContentBrowserClient,
  // before browser threads are created. Therefore any checks to enforce
  // threading (such as BrowserThread::CurrentlyOn()) will fail here.
}

AwBrowserContext::~AwBrowserContext() {
  DCHECK_EQ(this, g_browser_context);
  g_browser_context = NULL;
}

// static
AwBrowserContext* AwBrowserContext::GetDefault() {
  // TODO(joth): rather than store in a global here, lookup this instance
  // from the Java-side peer.
  return g_browser_context;
}

// static
AwBrowserContext* AwBrowserContext::FromWebContents(
    content::WebContents* web_contents) {
  // This is safe; this is the only implementation of the browser context.
  return static_cast<AwBrowserContext*>(web_contents->GetBrowserContext());
}

void AwBrowserContext::PreMainMessageLoopRun(net::NetLog* net_log) {
  FilePath cache_path;
  base::PathService::Get(base::DIR_CACHE, &cache_path);
  cache_path =
      cache_path.Append(FILE_PATH_LITERAL("org.chromium.android_webview"));

  browser_policy_connector_.reset(new AwBrowserPolicyConnector());

  InitUserPrefService();

  url_request_context_getter_ = new AwURLRequestContextGetter(
      cache_path, context_storage_path_.Append(kChannelIDFilename),
      CreateProxyConfigService(), user_pref_service_.get(), net_log);

  scoped_refptr<base::SequencedTaskRunner> db_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  visitedlink_master_.reset(
      new visitedlink::VisitedLinkMaster(this, this, false));
  visitedlink_master_->Init();

  form_database_service_.reset(
      new AwFormDatabaseService(context_storage_path_));

  EnsureResourceContextInitialized(this);

  AwMetricsServiceClient::GetInstance()->Initialize(
      user_pref_service_.get(),
      content::BrowserContext::GetDefaultStoragePartition(this)
          ->GetURLRequestContext());

  web_restriction_provider_.reset(
      new web_restrictions::WebRestrictionsClient());
  pref_change_registrar_.Add(
      prefs::kWebRestrictionsAuthority,
      base::BindRepeating(&AwBrowserContext::OnWebRestrictionsAuthorityChanged,
                          base::Unretained(this)));
  web_restriction_provider_->SetAuthority(
      user_pref_service_->GetString(prefs::kWebRestrictionsAuthority));

  safe_browsing_ui_manager_ = new AwSafeBrowsingUIManager(
      GetAwURLRequestContext(), user_pref_service_.get());
  safe_browsing_db_manager_ =
      new safe_browsing::RemoteSafeBrowsingDatabaseManager();
  safe_browsing_trigger_manager_ =
      std::make_unique<safe_browsing::TriggerManager>(
          safe_browsing_ui_manager_.get(),
          /*referrer_chain_provider=*/nullptr,
          /*local_state_prefs=*/nullptr);
  safe_browsing_whitelist_manager_ = CreateSafeBrowsingWhitelistManager();

  content::WebUIControllerFactory::RegisterFactory(
      AwWebUIControllerFactory::GetInstance());
}

void AwBrowserContext::OnWebRestrictionsAuthorityChanged() {
  web_restriction_provider_->SetAuthority(
      user_pref_service_->GetString(prefs::kWebRestrictionsAuthority));
}

void AwBrowserContext::AddVisitedURLs(const std::vector<GURL>& urls) {
  DCHECK(visitedlink_master_);
  visitedlink_master_->AddURLs(urls);
}

AwQuotaManagerBridge* AwBrowserContext::GetQuotaManagerBridge() {
  if (!quota_manager_bridge_.get()) {
    quota_manager_bridge_ = AwQuotaManagerBridge::Create(this);
  }
  return quota_manager_bridge_.get();
}

AwFormDatabaseService* AwBrowserContext::GetFormDatabaseService() {
  return form_database_service_.get();
}

AwURLRequestContextGetter* AwBrowserContext::GetAwURLRequestContext() {
  return url_request_context_getter_.get();
}

// Create user pref service
void AwBrowserContext::InitUserPrefService() {
  user_prefs::PrefRegistrySyncable* pref_registry =
      new user_prefs::PrefRegistrySyncable();
  // We only use the autocomplete feature of Autofill, which is controlled via
  // the manager_delegate. We don't use the rest of Autofill, which is why it is
  // hardcoded as disabled here.
  pref_registry->RegisterBooleanPref(autofill::prefs::kAutofillEnabled, false);
  policy::URLBlacklistManager::RegisterProfilePrefs(pref_registry);

  pref_registry->RegisterStringPref(prefs::kWebRestrictionsAuthority,
                                    std::string());

  AwURLRequestContextGetter::RegisterPrefs(pref_registry);
  metrics::MetricsService::RegisterPrefs(pref_registry);
  safe_browsing::RegisterProfilePrefs(pref_registry);

  PrefServiceFactory pref_service_factory;
  pref_service_factory.set_user_prefs(
      base::MakeRefCounted<InMemoryPrefStore>());
  pref_service_factory.set_managed_prefs(
      base::MakeRefCounted<policy::ConfigurationPolicyPrefStore>(
          browser_policy_connector_.get(),
          browser_policy_connector_->GetPolicyService(),
          browser_policy_connector_->GetHandlerList(),
          policy::POLICY_LEVEL_MANDATORY));
  pref_service_factory.set_read_error_callback(
      base::BindRepeating(&HandleReadError));
  user_pref_service_ = pref_service_factory.Create(pref_registry);
  pref_change_registrar_.Init(user_pref_service_.get());

  user_prefs::UserPrefs::Set(this, user_pref_service_.get());
}

base::FilePath AwBrowserContext::GetPath() const {
  return context_storage_path_;
}

bool AwBrowserContext::IsOffTheRecord() const {
  // Android WebView does not support off the record profile yet.
  return false;
}

content::ResourceContext* AwBrowserContext::GetResourceContext() {
  if (!resource_context_) {
    resource_context_.reset(
        new AwResourceContext(url_request_context_getter_.get()));
  }
  return resource_context_.get();
}

content::DownloadManagerDelegate*
AwBrowserContext::GetDownloadManagerDelegate() {
  if (!GetUserData(kDownloadManagerDelegateKey)) {
    SetUserData(kDownloadManagerDelegateKey,
                std::make_unique<AwDownloadManagerDelegate>());
  }
  return static_cast<AwDownloadManagerDelegate*>(
      GetUserData(kDownloadManagerDelegateKey));
}

content::BrowserPluginGuestManager* AwBrowserContext::GetGuestManager() {
  return NULL;
}

storage::SpecialStoragePolicy* AwBrowserContext::GetSpecialStoragePolicy() {
  // Intentionally returning NULL as 'Extensions' and 'Apps' not supported.
  return NULL;
}

content::PushMessagingService* AwBrowserContext::GetPushMessagingService() {
  // TODO(johnme): Support push messaging in WebView.
  return NULL;
}

content::SSLHostStateDelegate* AwBrowserContext::GetSSLHostStateDelegate() {
  if (!ssl_host_state_delegate_.get()) {
    ssl_host_state_delegate_.reset(new AwSSLHostStateDelegate());
  }
  return ssl_host_state_delegate_.get();
}

content::PermissionManager* AwBrowserContext::GetPermissionManager() {
  if (!permission_manager_.get())
    permission_manager_.reset(new AwPermissionManager());
  return permission_manager_.get();
}

content::BackgroundFetchDelegate*
AwBrowserContext::GetBackgroundFetchDelegate() {
  // TODO(crbug.com/766077): Resolve whether to support or disable background
  // fetch on WebView.
  return nullptr;
}

content::BackgroundSyncController*
AwBrowserContext::GetBackgroundSyncController() {
  return nullptr;
}

content::BrowsingDataRemoverDelegate*
AwBrowserContext::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}

net::URLRequestContextGetter* AwBrowserContext::CreateRequestContext(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  // This function cannot actually create the request context because
  // there is a reentrant dependency on GetResourceContext() via
  // content::StoragePartitionImplMap::Create(). This is not fixable
  // until http://crbug.com/159193. Until then, assert that the context
  // has already been allocated and just handle setting the protocol_handlers.
  DCHECK(url_request_context_getter_.get());
  url_request_context_getter_->SetHandlersAndInterceptors(
      protocol_handlers, std::move(request_interceptors));
  return url_request_context_getter_.get();
}

net::URLRequestContextGetter*
AwBrowserContext::CreateRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  NOTREACHED();
  return NULL;
}

net::URLRequestContextGetter* AwBrowserContext::CreateMediaRequestContext() {
  return url_request_context_getter_.get();
}

net::URLRequestContextGetter*
AwBrowserContext::CreateMediaRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory) {
  NOTREACHED();
  return NULL;
}

web_restrictions::WebRestrictionsClient*
AwBrowserContext::GetWebRestrictionProvider() {
  DCHECK(web_restriction_provider_);
  return web_restriction_provider_.get();
}

AwSafeBrowsingUIManager* AwBrowserContext::GetSafeBrowsingUIManager() const {
  return safe_browsing_ui_manager_.get();
}

safe_browsing::RemoteSafeBrowsingDatabaseManager*
AwBrowserContext::GetSafeBrowsingDBManager() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!safe_browsing_db_manager_started_) {
    // V4ProtocolConfig is not used. Just create one with empty values..
    safe_browsing::V4ProtocolConfig config("", false, "", "");
    safe_browsing_db_manager_->StartOnIOThread(
        safe_browsing_ui_manager_->GetURLLoaderFactoryOnIOThread(), config);
    safe_browsing_db_manager_started_ = true;
  }
  return safe_browsing_db_manager_.get();
}

safe_browsing::TriggerManager* AwBrowserContext::GetSafeBrowsingTriggerManager()
    const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return safe_browsing_trigger_manager_.get();
}

AwSafeBrowsingWhitelistManager*
AwBrowserContext::GetSafeBrowsingWhitelistManager() const {
  // Should not be called until the end of PreMainMessageLoopRun,
  return safe_browsing_whitelist_manager_.get();
}

void AwBrowserContext::RebuildTable(
    const scoped_refptr<URLEnumerator>& enumerator) {
  // Android WebView rebuilds from WebChromeClient.getVisitedHistory. The client
  // can change in the lifetime of this WebView and may not yet be set here.
  // Therefore this initialization path is not used.
  enumerator->OnComplete(true);
}

}  // namespace android_webview
