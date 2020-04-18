// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_browser_context_impl.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "base/path_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "headless/grit/headless_lib_resources.h"
#include "headless/lib/browser/headless_browser_context_options.h"
#include "headless/lib/browser/headless_browser_impl.h"
#include "headless/lib/browser/headless_browser_main_parts.h"
#include "headless/lib/browser/headless_net_log.h"
#include "headless/lib/browser/headless_permission_manager.h"
#include "headless/lib/browser/headless_url_request_context_getter.h"
#include "headless/public/util/black_hole_protocol_handler.h"
#include "headless/public/util/in_memory_protocol_handler.h"
#include "net/url_request/url_request_context.h"
#include "ui/base/resource/resource_bundle.h"

namespace headless {

// Contains net::URLRequestContextGetter required for resource loading.
// Must be destructed on the IO thread as per content::ResourceContext
// requirements.
class HeadlessResourceContext : public content::ResourceContext {
 public:
  HeadlessResourceContext();
  ~HeadlessResourceContext() override;

  // ResourceContext implementation:
  net::HostResolver* GetHostResolver() override;
  net::URLRequestContext* GetRequestContext() override;

  // Configure the URL request context getter to be used for resource fetching.
  // Must be called before any of the other methods of this class are used. Must
  // be called on the browser UI thread.
  void set_url_request_context_getter(
      scoped_refptr<net::URLRequestContextGetter> url_request_context_getter) {
    DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
    url_request_context_getter_ = std::move(url_request_context_getter);
  }

  net::URLRequestContextGetter* url_request_context_getter() {
    return url_request_context_getter_.get();
  }

 private:
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  DISALLOW_COPY_AND_ASSIGN(HeadlessResourceContext);
};

HeadlessResourceContext::HeadlessResourceContext() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
}

HeadlessResourceContext::~HeadlessResourceContext() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
}

net::HostResolver* HeadlessResourceContext::GetHostResolver() {
  CHECK(url_request_context_getter_);
  return url_request_context_getter_->GetURLRequestContext()->host_resolver();
}

net::URLRequestContext* HeadlessResourceContext::GetRequestContext() {
  CHECK(url_request_context_getter_);
  return url_request_context_getter_->GetURLRequestContext();
}

HeadlessBrowserContextImpl::HeadlessBrowserContextImpl(
    HeadlessBrowserImpl* browser,
    std::unique_ptr<HeadlessBrowserContextOptions> context_options)
    : browser_(browser),
      context_options_(std::move(context_options)),
      resource_context_(std::make_unique<HeadlessResourceContext>()),
      should_remove_headers_(true),
      permission_manager_(std::make_unique<HeadlessPermissionManager>(this)) {
  InitWhileIOAllowed();
}

HeadlessBrowserContextImpl::~HeadlessBrowserContextImpl() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  NotifyWillBeDestroyed(this);

  // Inform observers that we're going away.
  {
    base::AutoLock lock(observers_lock_);
    for (auto& observer : observers_)
      observer.OnHeadlessBrowserContextDestruct();
  }

  // Destroy all web contents before shutting down storage partitions.
  web_contents_map_.clear();

  if (resource_context_) {
    content::BrowserThread::DeleteSoon(content::BrowserThread::IO, FROM_HERE,
                                       resource_context_.release());
  }

  ShutdownStoragePartitions();

  if (url_request_getter_) {
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            &HeadlessURLRequestContextGetter::NotifyContextShuttingDown,
            url_request_getter_));
  }
}

// static
HeadlessBrowserContextImpl* HeadlessBrowserContextImpl::From(
    HeadlessBrowserContext* browser_context) {
  return static_cast<HeadlessBrowserContextImpl*>(browser_context);
}

// static
HeadlessBrowserContextImpl* HeadlessBrowserContextImpl::From(
    content::BrowserContext* browser_context) {
  return static_cast<HeadlessBrowserContextImpl*>(browser_context);
}

// static
std::unique_ptr<HeadlessBrowserContextImpl> HeadlessBrowserContextImpl::Create(
    HeadlessBrowserContext::Builder* builder) {
  return base::WrapUnique(new HeadlessBrowserContextImpl(
      builder->browser_, std::move(builder->options_)));
}

HeadlessWebContents::Builder
HeadlessBrowserContextImpl::CreateWebContentsBuilder() {
  DCHECK(browser_->BrowserMainThread()->BelongsToCurrentThread());
  return HeadlessWebContents::Builder(this);
}

std::vector<HeadlessWebContents*>
HeadlessBrowserContextImpl::GetAllWebContents() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::vector<HeadlessWebContents*> result;
  result.reserve(web_contents_map_.size());

  for (const auto& web_contents_pair : web_contents_map_) {
    result.push_back(web_contents_pair.second.get());
  }

  return result;
}

void HeadlessBrowserContextImpl::SetDevToolsFrameToken(
    int render_process_id,
    int render_frame_routing_id,
    const base::UnguessableToken& devtools_frame_token,
    int frame_tree_node_id) {
  base::AutoLock lock(devtools_frame_token_map_lock_);
  devtools_frame_token_map_[std::make_pair(
      render_process_id, render_frame_routing_id)] = devtools_frame_token;
  frame_tree_node_id_to_devtools_frame_token_map_[frame_tree_node_id] =
      devtools_frame_token;
}

void HeadlessBrowserContextImpl::RemoveDevToolsFrameToken(
    int render_process_id,
    int render_frame_routing_id,
    int frame_tree_node_id) {
  base::AutoLock lock(devtools_frame_token_map_lock_);
  devtools_frame_token_map_.erase(
      std::make_pair(render_process_id, render_frame_routing_id));
  frame_tree_node_id_to_devtools_frame_token_map_.erase(frame_tree_node_id);
}

const base::UnguessableToken* HeadlessBrowserContextImpl::GetDevToolsFrameToken(
    int render_process_id,
    int render_frame_id) const {
  base::AutoLock lock(devtools_frame_token_map_lock_);
  const auto& find_it = devtools_frame_token_map_.find(
      std::make_pair(render_process_id, render_frame_id));
  if (find_it == devtools_frame_token_map_.end())
    return nullptr;
  return &find_it->second;
}

const base::UnguessableToken*
HeadlessBrowserContextImpl::GetDevToolsFrameTokenForFrameTreeNodeId(
    int frame_tree_node_id) const {
  base::AutoLock lock(devtools_frame_token_map_lock_);
  const auto& find_it =
      frame_tree_node_id_to_devtools_frame_token_map_.find(frame_tree_node_id);
  if (find_it == frame_tree_node_id_to_devtools_frame_token_map_.end())
    return nullptr;
  return &find_it->second;
}

void HeadlessBrowserContextImpl::Close() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  browser_->DestroyBrowserContext(this);
}

void HeadlessBrowserContextImpl::InitWhileIOAllowed() {
  if (!context_options_->user_data_dir().empty()) {
    path_ = context_options_->user_data_dir().Append(kDefaultProfileName);
  } else {
    base::PathService::Get(base::DIR_EXE, &path_);
  }
  BrowserContext::Initialize(this, path_);
}

std::unique_ptr<content::ZoomLevelDelegate>
HeadlessBrowserContextImpl::CreateZoomLevelDelegate(
    const base::FilePath& partition_path) {
  return std::unique_ptr<content::ZoomLevelDelegate>();
}

base::FilePath HeadlessBrowserContextImpl::GetPath() const {
  return path_;
}

bool HeadlessBrowserContextImpl::IsOffTheRecord() const {
  return context_options_->incognito_mode();
}

content::ResourceContext* HeadlessBrowserContextImpl::GetResourceContext() {
  return resource_context_.get();
}

content::DownloadManagerDelegate*
HeadlessBrowserContextImpl::GetDownloadManagerDelegate() {
  return nullptr;
}

content::BrowserPluginGuestManager*
HeadlessBrowserContextImpl::GetGuestManager() {
  // TODO(altimin): Should be non-null? (is null in content/shell).
  return nullptr;
}

storage::SpecialStoragePolicy*
HeadlessBrowserContextImpl::GetSpecialStoragePolicy() {
  return nullptr;
}

content::PushMessagingService*
HeadlessBrowserContextImpl::GetPushMessagingService() {
  return nullptr;
}

content::SSLHostStateDelegate*
HeadlessBrowserContextImpl::GetSSLHostStateDelegate() {
  return nullptr;
}

content::PermissionManager* HeadlessBrowserContextImpl::GetPermissionManager() {
  return permission_manager_.get();
}

content::BackgroundFetchDelegate*
HeadlessBrowserContextImpl::GetBackgroundFetchDelegate() {
  return nullptr;
}

content::BackgroundSyncController*
HeadlessBrowserContextImpl::GetBackgroundSyncController() {
  return nullptr;
}

content::BrowsingDataRemoverDelegate*
HeadlessBrowserContextImpl::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}

net::URLRequestContextGetter* HeadlessBrowserContextImpl::CreateRequestContext(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  url_request_getter_ = base::MakeRefCounted<HeadlessURLRequestContextGetter>(
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::IO),
      protocol_handlers, context_options_->TakeProtocolHandlers(),
      std::move(request_interceptors), context_options_.get(),
      browser_->net_log(), this);
  resource_context_->set_url_request_context_getter(url_request_getter_);
  return url_request_getter_.get();
}

net::URLRequestContextGetter*
HeadlessBrowserContextImpl::CreateRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  return nullptr;
}

net::URLRequestContextGetter*
HeadlessBrowserContextImpl::CreateMediaRequestContext() {
  return resource_context_->url_request_context_getter();
}

net::URLRequestContextGetter*
HeadlessBrowserContextImpl::CreateMediaRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory) {
  return nullptr;
}

HeadlessWebContents* HeadlessBrowserContextImpl::CreateWebContents(
    HeadlessWebContents::Builder* builder) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::unique_ptr<HeadlessWebContentsImpl> headless_web_contents =
      HeadlessWebContentsImpl::Create(builder);

  if (!headless_web_contents) {
    return nullptr;
  }

  HeadlessWebContents* result = headless_web_contents.get();

  RegisterWebContents(std::move(headless_web_contents));

  return result;
}

void HeadlessBrowserContextImpl::RegisterWebContents(
    std::unique_ptr<HeadlessWebContentsImpl> web_contents) {
  DCHECK(web_contents);
  web_contents_map_[web_contents->GetDevToolsAgentHostId()] =
      std::move(web_contents);
}

void HeadlessBrowserContextImpl::DestroyWebContents(
    HeadlessWebContentsImpl* web_contents) {
  auto it = web_contents_map_.find(web_contents->GetDevToolsAgentHostId());
  DCHECK(it != web_contents_map_.end());
  web_contents_map_.erase(it);
}

HeadlessWebContents*
HeadlessBrowserContextImpl::GetWebContentsForDevToolsAgentHostId(
    const std::string& devtools_agent_host_id) {
  auto find_it = web_contents_map_.find(devtools_agent_host_id);
  if (find_it == web_contents_map_.end())
    return nullptr;
  return find_it->second.get();
}

HeadlessBrowserImpl* HeadlessBrowserContextImpl::browser() const {
  return browser_;
}

const HeadlessBrowserContextOptions* HeadlessBrowserContextImpl::options()
    const {
  return context_options_.get();
}

void HeadlessBrowserContextImpl::SetRemoveHeaders(bool should_remove_headers) {
  should_remove_headers_ = should_remove_headers;
}

bool HeadlessBrowserContextImpl::ShouldRemoveHeaders() const {
  return should_remove_headers_;
}

const std::string& HeadlessBrowserContextImpl::Id() const {
  return UniqueId();
}

void HeadlessBrowserContextImpl::AddObserver(Observer* obs) {
  base::AutoLock lock(observers_lock_);
  observers_.AddObserver(obs);
}

void HeadlessBrowserContextImpl::RemoveObserver(Observer* obs) {
  base::AutoLock lock(observers_lock_);
  observers_.RemoveObserver(obs);
}

void HeadlessBrowserContextImpl::NotifyChildContentsCreated(
    HeadlessWebContentsImpl* parent,
    HeadlessWebContentsImpl* child) {
  base::AutoLock lock(observers_lock_);
  for (auto& observer : observers_)
    observer.OnChildContentsCreated(parent, child);
}

void HeadlessBrowserContextImpl::NotifyUrlRequestFailed(
    net::URLRequest* request,
    int net_error,
    DevToolsStatus devtools_status) {
  base::AutoLock lock(observers_lock_);
  for (auto& observer : observers_)
    observer.UrlRequestFailed(request, net_error, devtools_status);
}

void HeadlessBrowserContextImpl::NotifyMetadataForResource(const GURL& url,
                                                           net::IOBuffer* buf,
                                                           int buf_len) {
  base::AutoLock lock(observers_lock_);
  for (auto& observer : observers_)
    observer.OnMetadataForResource(url, buf, buf_len);
}

void HeadlessBrowserContextImpl::SetNetworkConditions(
    HeadlessNetworkConditions conditions) {
  network_conditions_ = conditions;
}

HeadlessNetworkConditions HeadlessBrowserContextImpl::GetNetworkConditions() {
  return network_conditions_;
}

HeadlessBrowserContext::Builder::Builder(HeadlessBrowserImpl* browser)
    : browser_(browser),
      options_(new HeadlessBrowserContextOptions(browser->options())),
      enable_http_and_https_if_mojo_used_(false) {}

HeadlessBrowserContext::Builder::~Builder() = default;

HeadlessBrowserContext::Builder::Builder(Builder&&) = default;

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::SetProtocolHandlers(
    ProtocolHandlerMap protocol_handlers) {
  options_->protocol_handlers_ = std::move(protocol_handlers);
  return *this;
}

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::SetProductNameAndVersion(
    const std::string& product_name_and_version) {
  options_->product_name_and_version_ = product_name_and_version;
  return *this;
}

HeadlessBrowserContext::Builder& HeadlessBrowserContext::Builder::SetUserAgent(
    const std::string& user_agent) {
  options_->user_agent_ = user_agent;
  return *this;
}

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::SetAcceptLanguage(
    const std::string& accept_language) {
  options_->accept_language_ = accept_language;
  return *this;
}

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::SetProxyConfig(
    std::unique_ptr<net::ProxyConfig> proxy_config) {
  options_->proxy_config_ = std::move(proxy_config);
  return *this;
}

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::SetHostResolverRules(
    const std::string& host_resolver_rules) {
  options_->host_resolver_rules_ = host_resolver_rules;
  return *this;
}

HeadlessBrowserContext::Builder& HeadlessBrowserContext::Builder::SetWindowSize(
    const gfx::Size& window_size) {
  options_->window_size_ = window_size;
  return *this;
}

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::SetUserDataDir(
    const base::FilePath& user_data_dir) {
  options_->user_data_dir_ = user_data_dir;
  return *this;
}

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::SetIncognitoMode(bool incognito_mode) {
  options_->incognito_mode_ = incognito_mode;
  return *this;
}

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::SetSitePerProcess(bool site_per_process) {
  options_->site_per_process_ = site_per_process;
  return *this;
}

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::SetBlockNewWebContents(
    bool block_new_web_contents) {
  options_->block_new_web_contents_ = block_new_web_contents;
  return *this;
}

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::SetAllowCookies(bool allow_cookies) {
  options_->allow_cookies_ = allow_cookies;
  return *this;
}

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::EnableUnsafeNetworkAccessWithMojoBindings(
    bool enable_http_and_https_if_mojo_used) {
  enable_http_and_https_if_mojo_used_ = enable_http_and_https_if_mojo_used;
  return *this;
}

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::SetOverrideWebPreferencesCallback(
    base::RepeatingCallback<void(WebPreferences*)> callback) {
  options_->override_web_preferences_callback_ = std::move(callback);
  return *this;
}

HeadlessBrowserContext::Builder&
HeadlessBrowserContext::Builder::SetCaptureResourceMetadata(
    bool capture_resource_metadata) {
  options_->capture_resource_metadata_ = capture_resource_metadata;
  return *this;
}

HeadlessBrowserContext* HeadlessBrowserContext::Builder::Build() {
  if (!mojo_bindings_.empty()) {
    // Unless you know what you're doing it's unsafe to allow http/https for a
    // context with mojo bindings.
    if (!enable_http_and_https_if_mojo_used_) {
      options_->protocol_handlers_[url::kHttpScheme] =
          std::make_unique<BlackHoleProtocolHandler>();
      options_->protocol_handlers_[url::kHttpsScheme] =
          std::make_unique<BlackHoleProtocolHandler>();
    }
  }

  return browser_->CreateBrowserContext(this);
}

HeadlessBrowserContext::Builder::MojoBindings::MojoBindings() = default;

HeadlessBrowserContext::Builder::MojoBindings::MojoBindings(
    const std::string& mojom_name,
    const std::string& js_bindings)
    : mojom_name(mojom_name), js_bindings(js_bindings) {}

HeadlessBrowserContext::Builder::MojoBindings::~MojoBindings() = default;

}  // namespace headless
