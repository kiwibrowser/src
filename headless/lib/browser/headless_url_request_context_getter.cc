// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_url_request_context_getter.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "components/cookie_config/cookie_store_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "content/public/browser/devtools_network_transaction_factory.h"
#include "headless/app/headless_shell_switches.h"
#include "headless/lib/browser/headless_browser_context_impl.h"
#include "headless/lib/browser/headless_browser_context_options.h"
#include "headless/lib/browser/headless_network_delegate.h"
#include "headless/lib/browser/headless_network_transaction_factory.h"
#include "net/cookies/cookie_store.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_preferences.h"
#include "net/http/http_auth_scheme.h"
#include "net/http/http_transaction_factory.h"
#include "net/http/http_util.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
#include "base/command_line.h"
#include "components/os_crypt/key_storage_config_linux.h"
#include "components/os_crypt/os_crypt.h"
#endif

namespace headless {

HeadlessURLRequestContextGetter::HeadlessURLRequestContextGetter(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    content::ProtocolHandlerMap* protocol_handlers,
    content::ProtocolHandlerMap context_protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors,
    HeadlessBrowserContextOptions* options,
    net::NetLog* net_log,
    HeadlessBrowserContextImpl* headless_browser_context)
    : io_task_runner_(std::move(io_task_runner)),
      accept_language_(options->accept_language()),
      user_agent_(options->user_agent()),
      host_resolver_rules_(options->host_resolver_rules()),
      proxy_config_(options->proxy_config()),
      request_interceptors_(std::move(request_interceptors)),
      net_log_(net_log),
      capture_resource_metadata_(options->capture_resource_metadata()),
      headless_browser_context_(headless_browser_context) {
  // Must first be created on the UI thread.
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::swap(protocol_handlers_, *protocol_handlers);
  for (auto& pair : context_protocol_handlers) {
    protocol_handlers_[pair.first] = std::move(pair.second);
  }

  // We must create the proxy config service on the UI loop on Linux because it
  // must synchronously run on the glib message loop. This will be passed to
  // the URLRequestContextStorage on the IO thread in GetURLRequestContext().
  if (!proxy_config_) {
    proxy_config_service_ =
        net::ProxyResolutionService::CreateSystemProxyConfigService(
            io_task_runner_);
  }
  base::AutoLock lock(lock_);
  headless_browser_context_->AddObserver(this);
}

HeadlessURLRequestContextGetter::~HeadlessURLRequestContextGetter() {
  base::AutoLock lock(lock_);
  if (headless_browser_context_)
    headless_browser_context_->RemoveObserver(this);
}

net::URLRequestContext*
HeadlessURLRequestContextGetter::GetURLRequestContext() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (shut_down_)
    return nullptr;

  if (!url_request_context_) {
    net::URLRequestContextBuilder builder;

    {
      base::AutoLock lock(lock_);
      // Don't store cookies in incognito mode or if no user-data-dir was
      // specified
      // TODO: Enable this always once saving/restoring sessions is implemented
      // (https://crbug.com/617931)
      if (headless_browser_context_ &&
          !headless_browser_context_->IsOffTheRecord() &&
          !headless_browser_context_->options()->user_data_dir().empty()) {
#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
        std::unique_ptr<os_crypt::Config> config(new os_crypt::Config());
        base::CommandLine* command_line =
            base::CommandLine::ForCurrentProcess();
        config->store =
            command_line->GetSwitchValueASCII(switches::kPasswordStore);
        config->product_name = "HeadlessChrome";
        // OSCrypt may target keyring, which requires calls from the main
        // thread.
        config->main_thread_runner =
            content::BrowserThread::GetTaskRunnerForThread(
                content::BrowserThread::UI);
        config->should_use_preference = false;
        config->user_data_path = headless_browser_context_->GetPath();
        OSCrypt::SetConfig(std::move(config));
#endif

        content::CookieStoreConfig cookie_config(
            headless_browser_context_->GetPath().Append(
                FILE_PATH_LITERAL("Cookies")),
            false, true, NULL);
        cookie_config.crypto_delegate =
            cookie_config::GetCookieCryptoDelegate();
        std::unique_ptr<net::CookieStore> cookie_store =
            CreateCookieStore(cookie_config);
        std::unique_ptr<net::ChannelIDService> channel_id_service =
            std::make_unique<net::ChannelIDService>(
                new net::DefaultChannelIDStore(nullptr));

        cookie_store->SetChannelIDServiceID(channel_id_service->GetUniqueID());
        builder.SetCookieAndChannelIdStores(std::move(cookie_store),
                                            std::move(channel_id_service));
      }
    }

    builder.set_accept_language(
        net::HttpUtil::GenerateAcceptLanguageHeader(accept_language_));
    builder.set_user_agent(user_agent_);
    // TODO(skyostil): Make these configurable.
    builder.set_data_enabled(true);
    builder.set_file_enabled(true);
    if (proxy_config_) {
      net::NetworkTrafficAnnotationTag traffic_annotation =
          net::DefineNetworkTrafficAnnotation("proxy_config_headless", R"(
        semantics {
          sender: "Proxy Config"
          description:
            "Creates a proxy based on configuration received from headless "
            "command prompt."
          trigger:
            "User starts headless with proxy config."
          data:
            "Proxy configurations."
          destination: OTHER
          destination_other:
            "The proxy server specified in the configuration."
        }
        policy {
          cookies_allowed: NO
          setting:
            "This config is only used for headless mode and provided by user."
          policy_exception_justification:
            "This config is only used for headless mode and provided by user."
        })");

      builder.set_proxy_resolution_service(
          net::ProxyResolutionService::CreateFixed(
              net::ProxyConfigWithAnnotation(*proxy_config_,
                                             traffic_annotation)));
    } else {
      builder.set_proxy_config_service(std::move(proxy_config_service_));
    }

    {
      base::AutoLock lock(lock_);
      builder.set_network_delegate(
          std::make_unique<HeadlessNetworkDelegate>(headless_browser_context_));
    }

    std::unique_ptr<net::HostResolver> host_resolver(
        net::HostResolver::CreateDefaultResolver(net_log_));

    if (!host_resolver_rules_.empty()) {
      std::unique_ptr<net::MappedHostResolver> mapped_host_resolver(
          new net::MappedHostResolver(std::move(host_resolver)));
      mapped_host_resolver->SetRulesFromString(host_resolver_rules_);
      host_resolver = std::move(mapped_host_resolver);
    }

    net::HttpAuthPreferences* prefs(new net::HttpAuthPreferences());
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    prefs->SetServerWhitelist(
        command_line->GetSwitchValueASCII(switches::kAuthServerWhitelist));
    std::unique_ptr<net::HttpAuthHandlerRegistryFactory> factory =
        net::HttpAuthHandlerRegistryFactory::CreateDefault(host_resolver.get());
    factory->SetHttpAuthPreferences(net::kNegotiateAuthScheme,
                                    std::move(prefs));
    builder.SetHttpAuthHandlerFactory(std::move(factory));
    builder.set_host_resolver(std::move(host_resolver));

    // Extra headers are required for network emulation and are removed in
    // DevToolsNetworkTransaction. If a protocol handler is set for http or
    // https, then it is likely that the HttpTransactionFactoryCallback will
    // not be called and DevToolsNetworkTransaction would not remove the header.
    // In that case, the headers should be removed in HeadlessNetworkDelegate.
    bool has_http_handler = false;
    for (auto& pair : protocol_handlers_) {
      builder.SetProtocolHandler(pair.first, std::move(pair.second));
      if (pair.first == url::kHttpScheme || pair.first == url::kHttpsScheme)
        has_http_handler = true;
    }
    protocol_handlers_.clear();
    builder.SetInterceptors(std::move(request_interceptors_));

    if (!has_http_handler && headless_browser_context_) {
      headless_browser_context_->SetRemoveHeaders(false);
      builder.SetCreateHttpTransactionFactoryCallback(
          base::BindOnce(&content::CreateDevToolsNetworkTransactionFactory));
    }
    if (capture_resource_metadata_) {
      builder.SetCreateHttpTransactionFactoryCallback(
          base::BindOnce(&HeadlessNetworkTransactionFactory::Create,
                         headless_browser_context_));
      // We want to use the http cache inside HeadlessNetworkTransactionFactory.
      builder.DisableHttpCache();
    }

    url_request_context_ = builder.Build();
    url_request_context_->set_net_log(net_log_);
  }

  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
HeadlessURLRequestContextGetter::GetNetworkTaskRunner() const {
  return io_task_runner_;
}

net::HostResolver* HeadlessURLRequestContextGetter::host_resolver() const {
  return url_request_context_->host_resolver();
}

void HeadlessURLRequestContextGetter::OnHeadlessBrowserContextDestruct() {
  base::AutoLock lock(lock_);
  headless_browser_context_ = nullptr;
}

void HeadlessURLRequestContextGetter::NotifyContextShuttingDown() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  shut_down_ = true;
  net::URLRequestContextGetter::NotifyContextShuttingDown();
  url_request_context_ = nullptr;  // deletes it
}

}  // namespace headless
