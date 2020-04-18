// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_impl_io_data.h"

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/sequenced_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/browser/data_use_measurement/chrome_data_use_ascriber.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/net/chrome_network_delegate.h"
#include "chrome/browser/net/predictor.h"
#include "chrome/browser/net/quota_policy_channel_id_store.h"
#include "chrome/browser/net/reporting_permissions_checker.h"
#include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_io_data.h"
#include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings.h"
#include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings_factory.h"
#include "chrome/browser/previews/previews_service.h"
#include "chrome/browser/previews/previews_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/cookie_config/cookie_store_util.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_config.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_io_data.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_settings.h"
#include "components/data_reduction_proxy/core/browser/data_store_impl.h"
#include "components/domain_reliability/monitor.h"
#include "components/net_log/chrome_net_log.h"
#include "components/network_session_configurator/browser/network_session_configurator.h"
#include "components/offline_pages/buildflags/buildflags.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "components/prefs/pref_member.h"
#include "components/prefs/pref_service.h"
#include "components/previews/content/previews_io_data.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/browser/extension_protocols.h"
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/constants.h"
#include "net/base/cache_type.h"
#include "net/cookies/cookie_store.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_session.h"
#include "net/http/http_server_properties.h"
#include "net/http/http_server_properties_manager.h"
#include "net/net_buildflags.h"
#include "net/ssl/channel_id_service.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "storage/browser/quota/special_storage_policy.h"

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
#include "chrome/browser/offline_pages/offline_page_request_interceptor.h"
#endif

#if BUILDFLAG(ENABLE_REPORTING)
#include "net/network_error_logging/network_error_logging_delegate.h"
#include "net/network_error_logging/network_error_logging_service.h"
#include "net/reporting/reporting_policy.h"
#include "net/reporting/reporting_service.h"
#endif  // BUILDFLAG(ENABLE_REPORTING)

namespace {

// Returns the BackendType that the disk cache should use.
// TODO(mmenke): Once all URLRequestContexts are set up using
// URLRequestContextBuilders, and the media URLRequestContext is take care of
// (In one way or another), this should be removed.
net::BackendType ChooseCacheBackendType(const base::CommandLine& command_line) {
  switch (network_session_configurator::ChooseCacheType(command_line)) {
    case net::URLRequestContextBuilder::HttpCacheParams::DISK_BLOCKFILE:
      return net::CACHE_BACKEND_BLOCKFILE;
    case net::URLRequestContextBuilder::HttpCacheParams::DISK_SIMPLE:
      return net::CACHE_BACKEND_SIMPLE;
    case net::URLRequestContextBuilder::HttpCacheParams::DISK:
      return net::CACHE_BACKEND_DEFAULT;
    case net::URLRequestContextBuilder::HttpCacheParams::IN_MEMORY:
      NOTREACHED();
      break;
  }
  return net::CACHE_BACKEND_DEFAULT;
}

}  // namespace

using content::BrowserThread;

ProfileImplIOData::Handle::Handle(Profile* profile)
    : io_data_(new ProfileImplIOData),
      profile_(profile),
      initialized_(false) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(profile);
}

ProfileImplIOData::Handle::~Handle() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (io_data_->predictor_ != NULL) {
    // io_data_->predictor_ might be NULL if Init() was never called
    // (i.e. we shut down before ProfileImpl::DoFinalInit() got called).
    io_data_->predictor_->ShutdownOnUIThread();
  }

  // io_data_->data_reduction_proxy_io_data() might be NULL if Init() was
  // never called.
  if (io_data_->data_reduction_proxy_io_data())
    io_data_->data_reduction_proxy_io_data()->ShutdownOnUIThread();

  io_data_->ShutdownOnUIThread(GetAllContextGetters());
}

void ProfileImplIOData::Handle::Init(
    const base::FilePath& media_cache_path,
    int media_cache_max_size,
    const base::FilePath& extensions_cookie_path,
    const base::FilePath& profile_path,
    chrome_browser_net::Predictor* predictor,
    storage::SpecialStoragePolicy* special_storage_policy,
    std::unique_ptr<ReportingPermissionsChecker> reporting_permissions_checker,
    std::unique_ptr<domain_reliability::DomainReliabilityMonitor>
        domain_reliability_monitor) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!io_data_->lazy_params_);
  DCHECK(predictor);

  LazyParams* lazy_params = new LazyParams();

  lazy_params->media_cache_path = media_cache_path;
  lazy_params->media_cache_max_size = media_cache_max_size;
  lazy_params->extensions_cookie_path = extensions_cookie_path;
  lazy_params->restore_old_session_cookies =
      profile_->ShouldRestoreOldSessionCookies();
  lazy_params->persist_session_cookies =
      profile_->ShouldPersistSessionCookies();
  lazy_params->special_storage_policy = special_storage_policy;
  lazy_params->reporting_permissions_checker =
      std::move(reporting_permissions_checker);
  lazy_params->domain_reliability_monitor =
      std::move(domain_reliability_monitor);

  io_data_->lazy_params_.reset(lazy_params);

  PrefService* pref_service = profile_->GetPrefs();
  // Keep track of profile path and cache sizes separately so we can use them
  // on demand when creating storage isolated URLRequestContextGetters.
  io_data_->profile_path_ = profile_path;
  io_data_->app_cache_max_size_ =
      pref_service->GetInteger(prefs::kDiskCacheSize);
  io_data_->app_media_cache_max_size_ = media_cache_max_size;

  io_data_->predictor_.reset(predictor);

  io_data_->InitializeMetricsEnabledStateOnUIThread();
  if (io_data_->lazy_params_->domain_reliability_monitor)
    io_data_->lazy_params_->domain_reliability_monitor->MoveToNetworkThread();

  io_data_->set_previews_io_data(std::make_unique<previews::PreviewsIOData>(
      BrowserThread::GetTaskRunnerForThread(BrowserThread::UI),
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO)));
  PreviewsServiceFactory::GetForProfile(profile_)->Initialize(
      io_data_->previews_io_data(),
      g_browser_process->optimization_guide_service(),
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO), profile_path);

  io_data_->set_data_reduction_proxy_io_data(
      CreateDataReductionProxyChromeIOData(
          g_browser_process->io_thread()->net_log(), profile_->GetPrefs(),
          BrowserThread::GetTaskRunnerForThread(BrowserThread::IO),
          BrowserThread::GetTaskRunnerForThread(BrowserThread::UI)));

#if defined(OS_CHROMEOS)
  // Set a task runner for the get network id call in DataReductionProxyConfig
  // to work around the bug that recv() in AddressTrackerLinux blocks IO thread
  // and freezes the screen. Using SingleThreadTaskRunner so that task scheduler
  // does not create too many worker threads when https://crbug.com/821607
  // happens.
  // TODO(https://crbug.com/821607): Remove after the bug is resolved.
  io_data_->data_reduction_proxy_io_data()
      ->config()
      ->set_get_network_id_task_runner(
          base::CreateSingleThreadTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::BACKGROUND,
               base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN}));
#endif
}

content::ResourceContext*
    ProfileImplIOData::Handle::GetResourceContext() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  LazyInitialize();
  return GetResourceContextNoInit();
}

content::ResourceContext*
ProfileImplIOData::Handle::GetResourceContextNoInit() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Don't call LazyInitialize here, since the resource context is created at
  // the beginning of initalization and is used by some members while they're
  // being initialized (i.e. AppCacheService).
  return io_data_->GetResourceContext();
}

scoped_refptr<ChromeURLRequestContextGetter>
ProfileImplIOData::Handle::CreateMainRequestContextGetter(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors,
    IOThread* io_thread) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  LazyInitialize();
  DCHECK(!main_request_context_getter_.get());
  main_request_context_getter_ = ChromeURLRequestContextGetter::Create(
      profile_, io_data_, protocol_handlers, std::move(request_interceptors));

  scoped_refptr<base::SequencedTaskRunner> db_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  std::unique_ptr<data_reduction_proxy::DataStore> store(
      new data_reduction_proxy::DataStoreImpl(io_data_->profile_path_));
  DataReductionProxyChromeSettingsFactory::GetForBrowserContext(profile_)
      ->InitDataReductionProxySettings(
          io_data_->data_reduction_proxy_io_data(), profile_->GetPrefs(),
          main_request_context_getter_.get(), std::move(store),
          BrowserThread::GetTaskRunnerForThread(BrowserThread::UI),
          db_task_runner);

  io_data_->predictor_
      ->InitNetworkPredictor(profile_->GetPrefs(),
                             io_thread,
                             main_request_context_getter_.get(),
                             io_data_);

  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_PROFILE_URL_REQUEST_CONTEXT_GETTER_INITIALIZED,
      content::Source<Profile>(profile_),
      content::NotificationService::NoDetails());
  return main_request_context_getter_;
}

scoped_refptr<ChromeURLRequestContextGetter>
ProfileImplIOData::Handle::GetMediaRequestContextGetter() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  LazyInitialize();
  if (!media_request_context_getter_.get()) {
    media_request_context_getter_ =
        ChromeURLRequestContextGetter::CreateForMedia(profile_, io_data_);
  }
  return media_request_context_getter_;
}

scoped_refptr<ChromeURLRequestContextGetter>
ProfileImplIOData::Handle::GetExtensionsRequestContextGetter() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  LazyInitialize();
  if (!extensions_request_context_getter_.get()) {
    extensions_request_context_getter_ =
        ChromeURLRequestContextGetter::CreateForExtensions(profile_, io_data_);
  }
  return extensions_request_context_getter_;
}

scoped_refptr<ChromeURLRequestContextGetter>
ProfileImplIOData::Handle::CreateIsolatedAppRequestContextGetter(
    const base::FilePath& partition_path,
    bool in_memory,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Check that the partition_path is not the same as the base profile path. We
  // expect isolated partition, which will never go to the default profile path.
  CHECK(partition_path != profile_->GetPath());
  LazyInitialize();

  // Keep a map of request context getters, one per requested storage partition.
  StoragePartitionDescriptor descriptor(partition_path, in_memory);
  ChromeURLRequestContextGetterMap::iterator iter =
      app_request_context_getter_map_.find(descriptor);
  if (iter != app_request_context_getter_map_.end())
    return iter->second;

  std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
      protocol_handler_interceptor(
          ProtocolHandlerRegistryFactory::GetForBrowserContext(profile_)
              ->CreateJobInterceptorFactory());
  ChromeURLRequestContextGetter* context =
      ChromeURLRequestContextGetter::CreateForIsolatedApp(
          profile_, io_data_, descriptor,
          std::move(protocol_handler_interceptor), protocol_handlers,
          std::move(request_interceptors));
  app_request_context_getter_map_[descriptor] = context;

  return context;
}

scoped_refptr<ChromeURLRequestContextGetter>
ProfileImplIOData::Handle::GetIsolatedMediaRequestContextGetter(
    const base::FilePath& partition_path,
    bool in_memory) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // We must have a non-default path, or this will act like the default media
  // context.
  CHECK(partition_path != profile_->GetPath());
  LazyInitialize();

  // Keep a map of request context getters, one per requested storage partition.
  StoragePartitionDescriptor descriptor(partition_path, in_memory);
  ChromeURLRequestContextGetterMap::iterator iter =
      isolated_media_request_context_getter_map_.find(descriptor);
  if (iter != isolated_media_request_context_getter_map_.end())
    return iter->second;

  // Get the app context as the starting point for the media context, so that
  // it uses the app's cookie store.
  ChromeURLRequestContextGetterMap::const_iterator app_iter =
      app_request_context_getter_map_.find(descriptor);
  DCHECK(app_iter != app_request_context_getter_map_.end());
  ChromeURLRequestContextGetter* app_context = app_iter->second.get();
  ChromeURLRequestContextGetter* context =
      ChromeURLRequestContextGetter::CreateForIsolatedMedia(
          profile_, app_context, io_data_, descriptor);
  isolated_media_request_context_getter_map_[descriptor] = context;

  return context;
}

void ProfileImplIOData::Handle::LazyInitialize() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (initialized_)
    return;

  // Set initialized_ to true at the beginning in case any of the objects
  // below try to get the ResourceContext pointer.
  initialized_ = true;
  PrefService* pref_service = profile_->GetPrefs();
  io_data_->safe_browsing_enabled()->Init(prefs::kSafeBrowsingEnabled,
      pref_service);
  io_data_->safe_browsing_enabled()->MoveToThread(
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));
  io_data_->safe_browsing_whitelist_domains()->Init(
      prefs::kSafeBrowsingWhitelistDomains, pref_service);
  io_data_->safe_browsing_whitelist_domains()->MoveToThread(
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));
#if BUILDFLAG(ENABLE_PLUGINS)
  io_data_->always_open_pdf_externally()->Init(
      prefs::kPluginsAlwaysOpenPdfExternally, pref_service);
  io_data_->always_open_pdf_externally()->MoveToThread(
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));
#endif
  io_data_->InitializeOnUIThread(profile_);
}

std::unique_ptr<ProfileIOData::ChromeURLRequestContextGetterVector>
ProfileImplIOData::Handle::GetAllContextGetters() {
  ChromeURLRequestContextGetterMap::iterator iter;
  std::unique_ptr<ChromeURLRequestContextGetterVector> context_getters(
      new ChromeURLRequestContextGetterVector());

  iter = isolated_media_request_context_getter_map_.begin();
  for (; iter != isolated_media_request_context_getter_map_.end(); ++iter)
    context_getters->push_back(iter->second);

  iter = app_request_context_getter_map_.begin();
  for (; iter != app_request_context_getter_map_.end(); ++iter)
    context_getters->push_back(iter->second);

  if (extensions_request_context_getter_.get())
    context_getters->push_back(extensions_request_context_getter_);

  if (media_request_context_getter_.get())
    context_getters->push_back(media_request_context_getter_);

  if (main_request_context_getter_.get())
    context_getters->push_back(main_request_context_getter_);

  return context_getters;
}

ProfileImplIOData::LazyParams::LazyParams()
    : media_cache_max_size(0),
      restore_old_session_cookies(false),
      persist_session_cookies(false) {}

ProfileImplIOData::LazyParams::~LazyParams() {}

ProfileImplIOData::ProfileImplIOData()
    : ProfileIOData(Profile::REGULAR_PROFILE),
      app_cache_max_size_(0),
      app_media_cache_max_size_(0) {
}

ProfileImplIOData::~ProfileImplIOData() {
  DestroyResourceContext();

  if (media_request_context_)
    media_request_context_->AssertNoURLRequests();
}

std::unique_ptr<net::NetworkDelegate>
ProfileImplIOData::ConfigureNetworkDelegate(
    IOThread* io_thread,
    std::unique_ptr<ChromeNetworkDelegate> chrome_network_delegate) const {
  if (lazy_params_->domain_reliability_monitor) {
    chrome_network_delegate->set_domain_reliability_monitor(
        std::move(lazy_params_->domain_reliability_monitor));
  }

  if (lazy_params_->reporting_permissions_checker) {
    chrome_network_delegate->set_reporting_permissions_checker(
        std::move(lazy_params_->reporting_permissions_checker));
  }

  return data_reduction_proxy_io_data()->CreateNetworkDelegate(
      io_thread->globals()->data_use_ascriber->CreateNetworkDelegate(
          std::move(chrome_network_delegate),
          io_thread->GetMetricsDataUseForwarder()),
      true);
}

void ProfileImplIOData::InitializeInternal(
    net::URLRequestContextBuilder* builder,
    ProfileParams* profile_params,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) const {
  IOThread* const io_thread = profile_params->io_thread;
  IOThread::Globals* const io_thread_globals = io_thread->globals();

  // This check is needed because with the network service the cookies are used
  // in a different process. See the bottom of
  // ProfileNetworkContextService::SetUpProfileIODataMainContext.
  if (profile_params->main_network_context_params->cookie_path) {
    // Create a single task runner to use with the CookieStore and
    // ChannelIDStore.
    scoped_refptr<base::SequencedTaskRunner> cookie_background_task_runner =
        base::CreateSequencedTaskRunnerWithTraits(
            {base::MayBlock(), base::TaskPriority::BACKGROUND,
             base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

    // Set up server bound cert service.
    DCHECK(!profile_params->main_network_context_params->channel_id_path.value()
                .empty());
    scoped_refptr<QuotaPolicyChannelIDStore> channel_id_db =
        new QuotaPolicyChannelIDStore(
            profile_params->main_network_context_params->channel_id_path
                .value(),
            cookie_background_task_runner,
            lazy_params_->special_storage_policy.get());
    std::unique_ptr<net::ChannelIDService> channel_id_service(
        std::make_unique<net::ChannelIDService>(
            new net::DefaultChannelIDStore(channel_id_db.get())));

    // Set up cookie store.
    content::CookieStoreConfig cookie_config(
        profile_params->main_network_context_params->cookie_path.value(),
        profile_params->main_network_context_params
            ->restore_old_session_cookies,
        profile_params->main_network_context_params->persist_session_cookies,
        lazy_params_->special_storage_policy.get());
    cookie_config.crypto_delegate = cookie_config::GetCookieCryptoDelegate();
    cookie_config.channel_id_service = channel_id_service.get();
    cookie_config.background_task_runner = cookie_background_task_runner;
    std::unique_ptr<net::CookieStore> cookie_store(
        content::CreateCookieStore(cookie_config));

    cookie_store->SetChannelIDServiceID(channel_id_service->GetUniqueID());

    builder->SetCookieAndChannelIdStores(std::move(cookie_store),
                                         std::move(channel_id_service));
  }

  AddProtocolHandlersToBuilder(builder, protocol_handlers);

  // Install the Offline Page Interceptor.
#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  request_interceptors.push_back(
      std::make_unique<offline_pages::OfflinePageRequestInterceptor>(
          previews_io_data()));
#endif

  // The data reduction proxy interceptor should be as close to the network
  // as possible.
  request_interceptors.insert(
      request_interceptors.begin(),
      data_reduction_proxy_io_data()->CreateInterceptor());
  data_reduction_proxy_io_data()->SetDataUseAscriber(
      io_thread_globals->data_use_ascriber.get());
  data_reduction_proxy_io_data()->SetPreviewsDecider(previews_io_data());
  SetUpJobFactoryDefaultsForBuilder(
      builder, std::move(request_interceptors),
      std::move(profile_params->protocol_handler_interceptor));
}

void ProfileImplIOData::OnMainRequestContextCreated(
    ProfileParams* profile_params) const {
  DCHECK(lazy_params_);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  InitializeExtensionsRequestContext(profile_params);
#endif

  // Create a media request context based on the main context, but using a
  // media cache.  It shares the same job factory as the main context.
  StoragePartitionDescriptor details(profile_path_, false);
  media_request_context_.reset(InitializeMediaRequestContext(
      main_request_context(), details, "main_media"));
  lazy_params_.reset();
}

void ProfileImplIOData::
    InitializeExtensionsRequestContext(ProfileParams* profile_params) const {
  // The extensions context only serves to hold onto the extensions cookie
  // store.
  net::URLRequestContext* extensions_context = extensions_request_context();

  content::CookieStoreConfig cookie_config(
      lazy_params_->extensions_cookie_path,
      lazy_params_->restore_old_session_cookies,
      lazy_params_->persist_session_cookies, NULL);
  cookie_config.crypto_delegate = cookie_config::GetCookieCryptoDelegate();
  // Enable cookies for chrome-extension URLs.
  cookie_config.cookieable_schemes.push_back(extensions::kExtensionScheme);
  cookie_config.channel_id_service = extensions_context->channel_id_service();
  extensions_cookie_store_ = content::CreateCookieStore(cookie_config);
  extensions_context->set_cookie_store(extensions_cookie_store_.get());
}

net::URLRequestContext* ProfileImplIOData::InitializeAppRequestContext(
    net::URLRequestContext* main_context,
    const StoragePartitionDescriptor& partition_descriptor,
    std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
        protocol_handler_interceptor,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) const {
  // Copy most state from the main context.
  AppRequestContext* context = new AppRequestContext();
  context->CopyFrom(main_context);

  base::FilePath cookie_path = partition_descriptor.path.Append(
      chrome::kCookieFilename);
  base::FilePath channel_id_path =
      partition_descriptor.path.Append(chrome::kChannelIDFilename);
  base::FilePath cache_path =
      partition_descriptor.path.Append(chrome::kCacheDirname);

  // Use a separate HTTP disk cache for isolated apps.
  std::unique_ptr<net::HttpCache::BackendFactory> app_backend;
  if (partition_descriptor.in_memory) {
    app_backend = net::HttpCache::DefaultBackend::InMemory(0);
  } else {
    app_backend.reset(new net::HttpCache::DefaultBackend(
        net::DISK_CACHE,
        ChooseCacheBackendType(*base::CommandLine::ForCurrentProcess()),
        cache_path, app_cache_max_size_));
  }

  std::unique_ptr<net::CookieStore> cookie_store;
  scoped_refptr<net::SQLiteChannelIDStore> channel_id_db;
  // Create a single task runner to use with the CookieStore and ChannelIDStore.
  scoped_refptr<base::SequencedTaskRunner> cookie_background_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  if (partition_descriptor.in_memory) {
    cookie_path = base::FilePath();
  }
  content::CookieStoreConfig cookie_config(cookie_path, false, false, nullptr);
  if (!partition_descriptor.in_memory) {
    // Use an app-specific cookie store.
    DCHECK(!cookie_path.empty());

    // TODO(creis): We should have a cookie delegate for notifying the cookie
    // extensions API, but we need to update it to understand isolated apps
    // first.
    cookie_config.crypto_delegate = cookie_config::GetCookieCryptoDelegate();
    channel_id_db = new net::SQLiteChannelIDStore(
        channel_id_path, cookie_background_task_runner);
  }
  std::unique_ptr<net::ChannelIDService> channel_id_service(
      new net::ChannelIDService(
          new net::DefaultChannelIDStore(channel_id_db.get())));
  cookie_config.channel_id_service = channel_id_service.get();
  cookie_config.background_task_runner = cookie_background_task_runner;
  cookie_store = content::CreateCookieStore(cookie_config);
  cookie_store->SetChannelIDServiceID(channel_id_service->GetUniqueID());

  // Build a new HttpNetworkSession that uses the new ChannelIDService.
  // TODO(mmenke):  It's weird to combine state from
  // main_request_context_storage() objects and the argument to this method,
  // |main_context|.  Remove |main_context| as an argument, and just use
  // main_context() instead.
  net::HttpNetworkSession* network_session =
      main_context->http_transaction_factory()->GetSession();
  net::HttpNetworkSession::Context session_context = network_session->context();
  session_context.channel_id_service = channel_id_service.get();
  std::unique_ptr<net::HttpNetworkSession> http_network_session(
      new net::HttpNetworkSession(network_session->params(), session_context));
  std::unique_ptr<net::HttpCache> app_http_cache =
      CreateMainHttpFactory(http_network_session.get(), std::move(app_backend));

  // Transfer ownership of the ChannelIDStore and the HttpNetworkSession to the
  // AppRequestContext.
  context->SetChannelIDService(std::move(channel_id_service));
  context->SetHttpNetworkSession(std::move(http_network_session));

  // Transfer ownership of the cookies and cache to AppRequestContext.
  context->SetCookieStore(std::move(cookie_store));
  context->SetHttpTransactionFactory(std::move(app_http_cache));

  std::unique_ptr<net::URLRequestJobFactoryImpl> job_factory(
      new net::URLRequestJobFactoryImpl());
  InstallProtocolHandlers(job_factory.get(), protocol_handlers);
  // The data reduction proxy interceptor should be as close to the network
  // as possible.
  request_interceptors.insert(
      request_interceptors.begin(),
      data_reduction_proxy_io_data()->CreateInterceptor());

  std::unique_ptr<net::URLRequestJobFactory> top_job_factory(
      SetUpJobFactoryDefaults(
          std::move(job_factory), std::move(request_interceptors),
          std::move(protocol_handler_interceptor), context->network_delegate(),
          context->host_resolver()));
  context->SetJobFactory(std::move(top_job_factory));

#if BUILDFLAG(ENABLE_REPORTING)
  if (context->reporting_service()) {
    context->SetReportingService(net::ReportingService::Create(
        context->reporting_service()->GetPolicy(), context));
  }

  if (context->network_error_logging_service()) {
    context->SetNetworkErrorLoggingService(
        net::NetworkErrorLoggingService::Create(
            net::NetworkErrorLoggingDelegate::Create()));
    context->network_error_logging_service()->SetReportingService(
        context->reporting_service());
  }
#endif  // BUILDFLAG(ENABLE_REPORTING)

  return context;
}

net::URLRequestContext* ProfileImplIOData::InitializeMediaRequestContext(
    net::URLRequestContext* original_context,
    const StoragePartitionDescriptor& partition_descriptor,
    const char* name) const {
  // Copy most state from the original context.
  MediaRequestContext* context = new MediaRequestContext(name);
  context->CopyFrom(original_context);
  if (base::FeatureList::IsEnabled(features::kUseSameCacheForMedia))
    return context;

  // For in-memory context, return immediately after creating the new
  // context before attaching a separate cache. It is important to return
  // a new context rather than just reusing |original_context| because
  // the caller expects to take ownership of the pointer.
  if (partition_descriptor.in_memory)
    return context;

  using content::StoragePartition;
  base::FilePath cache_path;
  int cache_max_size = app_media_cache_max_size_;
  if (partition_descriptor.path == profile_path_) {
    // lazy_params_ is only valid for the default media context creation.
    cache_path = lazy_params_->media_cache_path;
    cache_max_size = lazy_params_->media_cache_max_size;
  } else {
    cache_path = partition_descriptor.path.Append(chrome::kMediaCacheDirname);
  }

  // Use a separate HTTP disk cache for isolated apps.
  std::unique_ptr<net::HttpCache::BackendFactory> media_backend(
      new net::HttpCache::DefaultBackend(
          net::MEDIA_CACHE,
          ChooseCacheBackendType(*base::CommandLine::ForCurrentProcess()),
          cache_path, cache_max_size));
  std::unique_ptr<net::HttpCache> media_http_cache = CreateHttpFactory(
      main_request_context()->http_transaction_factory(),
      std::move(media_backend));

  // Transfer ownership of the cache to MediaRequestContext.
  context->SetHttpTransactionFactory(std::move(media_http_cache));

  // Note that we do not create a new URLRequestJobFactory because
  // the media context should behave exactly like its parent context
  // in all respects except for cache behavior on media subresources.
  // The CopyFrom() step above means that our media context will use
  // the same URLRequestJobFactory instance that our parent context does.

  return context;
}

net::URLRequestContext*
ProfileImplIOData::AcquireMediaRequestContext() const {
  DCHECK(media_request_context_);
  return media_request_context_.get();
}

net::URLRequestContext* ProfileImplIOData::AcquireIsolatedAppRequestContext(
    net::URLRequestContext* main_context,
    const StoragePartitionDescriptor& partition_descriptor,
    std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
        protocol_handler_interceptor,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) const {
  // We create per-app contexts on demand, unlike the others above.
  net::URLRequestContext* app_request_context = InitializeAppRequestContext(
      main_context, partition_descriptor,
      std::move(protocol_handler_interceptor), protocol_handlers,
      std::move(request_interceptors));
  DCHECK(app_request_context);
  return app_request_context;
}

net::URLRequestContext*
ProfileImplIOData::AcquireIsolatedMediaRequestContext(
    net::URLRequestContext* app_context,
    const StoragePartitionDescriptor& partition_descriptor) const {
  // We create per-app media contexts on demand, unlike the others above.
  net::URLRequestContext* media_request_context = InitializeMediaRequestContext(
      app_context, partition_descriptor, "isolated_media");
  DCHECK(media_request_context);
  return media_request_context;
}

chrome_browser_net::Predictor* ProfileImplIOData::GetPredictor() {
  return predictor_.get();
}
