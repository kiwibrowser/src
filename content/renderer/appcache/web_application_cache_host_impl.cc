// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/appcache/web_application_cache_host_impl.h"

#include <stddef.h>

#include "base/compiler_specific.h"
#include "base/containers/id_map.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/platform/web_url_response.h"

using blink::WebApplicationCacheHost;
using blink::WebApplicationCacheHostClient;
using blink::WebString;
using blink::WebURLRequest;
using blink::WebURL;
using blink::WebURLResponse;
using blink::WebVector;

namespace content {

namespace {

// Note: the order of the elements in this array must match those
// of the EventID enum in appcache_interfaces.h.
const char* const kEventNames[] = {
  "Checking", "Error", "NoUpdate", "Downloading", "Progress",
  "UpdateReady", "Cached", "Obsolete"
};

using HostsMap = base::IDMap<WebApplicationCacheHostImpl*>;

HostsMap* all_hosts() {
  static HostsMap* map = new HostsMap;
  return map;
}

GURL ClearUrlRef(const GURL& url) {
  if (!url.has_ref())
    return url;
  GURL::Replacements replacements;
  replacements.ClearRef();
  return url.ReplaceComponents(replacements);
}

}  // anon namespace

WebApplicationCacheHostImpl* WebApplicationCacheHostImpl::FromId(int id) {
  return all_hosts()->Lookup(id);
}

WebApplicationCacheHostImpl::WebApplicationCacheHostImpl(
    WebApplicationCacheHostClient* client,
    AppCacheBackend* backend,
    int appcache_host_id)
    : client_(client),
      backend_(backend),
      status_(AppCacheStatus::APPCACHE_STATUS_UNCACHED),
      is_scheme_supported_(false),
      is_get_method_(false),
      is_new_master_entry_(MAYBE_NEW_ENTRY),
      was_select_cache_called_(false) {
  DCHECK(client && backend);
  // PlzNavigate: The browser passes the ID to be used.
  if (appcache_host_id != kAppCacheNoHostId) {
    DCHECK(IsBrowserSideNavigationEnabled());
    all_hosts()->AddWithID(this, appcache_host_id);
    host_id_ = appcache_host_id;
  } else {
    host_id_ = all_hosts()->Add(this);
  }
  DCHECK(host_id_ != kAppCacheNoHostId);

  backend_->RegisterHost(host_id_);
}

WebApplicationCacheHostImpl::~WebApplicationCacheHostImpl() {
  backend_->UnregisterHost(host_id_);
  all_hosts()->Remove(host_id_);
}

void WebApplicationCacheHostImpl::OnCacheSelected(
    const AppCacheInfo& info) {
  cache_info_ = info;
  client_->DidChangeCacheAssociation();
}

void WebApplicationCacheHostImpl::OnStatusChanged(
    AppCacheStatus status) {
  // TODO(michaeln): delete me, not used
}

void WebApplicationCacheHostImpl::OnEventRaised(
    AppCacheEventID event_id) {
  DCHECK_NE(
      event_id,
      AppCacheEventID::APPCACHE_PROGRESS_EVENT);  // See OnProgressEventRaised.
  DCHECK_NE(event_id,
            AppCacheEventID::APPCACHE_ERROR_EVENT);  // See OnErrorEventRaised.

  // Emit logging output prior to calling out to script as we can get
  // deleted within the script event handler.
  const char kFormatString[] = "Application Cache %s event";
  std::string message = base::StringPrintf(
      kFormatString, kEventNames[static_cast<int>(event_id)]);
  OnLogMessage(APPCACHE_LOG_INFO, message);

  switch (event_id) {
    case AppCacheEventID::APPCACHE_CHECKING_EVENT:
      status_ = AppCacheStatus::APPCACHE_STATUS_CHECKING;
      break;
    case AppCacheEventID::APPCACHE_DOWNLOADING_EVENT:
      status_ = AppCacheStatus::APPCACHE_STATUS_DOWNLOADING;
      break;
    case AppCacheEventID::APPCACHE_UPDATE_READY_EVENT:
      status_ = AppCacheStatus::APPCACHE_STATUS_UPDATE_READY;
      break;
    case AppCacheEventID::APPCACHE_CACHED_EVENT:
    case AppCacheEventID::APPCACHE_NO_UPDATE_EVENT:
      status_ = AppCacheStatus::APPCACHE_STATUS_IDLE;
      break;
    case AppCacheEventID::APPCACHE_OBSOLETE_EVENT:
      status_ = AppCacheStatus::APPCACHE_STATUS_OBSOLETE;
      break;
    default:
      NOTREACHED();
      break;
  }

  client_->NotifyEventListener(static_cast<EventID>(event_id));
}

void WebApplicationCacheHostImpl::OnProgressEventRaised(
    const GURL& url, int num_total, int num_complete) {
  // Emit logging output prior to calling out to script as we can get
  // deleted within the script event handler.
  const char kFormatString[] = "Application Cache Progress event (%d of %d) %s";
  std::string message = base::StringPrintf(kFormatString, num_complete,
                                           num_total, url.spec().c_str());
  OnLogMessage(APPCACHE_LOG_INFO, message);
  status_ = AppCacheStatus::APPCACHE_STATUS_DOWNLOADING;
  client_->NotifyProgressEventListener(url, num_total, num_complete);
}

void WebApplicationCacheHostImpl::OnErrorEventRaised(
    const AppCacheErrorDetails& details) {
  // Emit logging output prior to calling out to script as we can get
  // deleted within the script event handler.
  const char kFormatString[] = "Application Cache Error event: %s";
  std::string full_message =
      base::StringPrintf(kFormatString, details.message.c_str());
  OnLogMessage(APPCACHE_LOG_ERROR, full_message);

  status_ = cache_info_.is_complete ? AppCacheStatus::APPCACHE_STATUS_IDLE
                                    : AppCacheStatus::APPCACHE_STATUS_UNCACHED;
  if (details.is_cross_origin) {
    // Don't leak detailed information to script for cross-origin resources.
    DCHECK_EQ(AppCacheErrorReason::APPCACHE_RESOURCE_ERROR, details.reason);
    client_->NotifyErrorEventListener(static_cast<ErrorReason>(details.reason),
                                      details.url, 0, WebString());
  } else {
    client_->NotifyErrorEventListener(static_cast<ErrorReason>(details.reason),
                                      details.url, details.status,
                                      WebString::FromUTF8(details.message));
  }
}

void WebApplicationCacheHostImpl::WillStartMainResourceRequest(
    const WebURL& url,
    const WebString& method_webstring,
    const WebApplicationCacheHost* spawning_host) {
  original_main_resource_url_ = ClearUrlRef(url);

  std::string method = method_webstring.Utf8();
  is_get_method_ = (method == kHttpGETMethod);
  DCHECK(method == base::ToUpperASCII(method));

  const WebApplicationCacheHostImpl* spawning_host_impl =
      static_cast<const WebApplicationCacheHostImpl*>(spawning_host);
  if (spawning_host_impl && (spawning_host_impl != this) &&
      (spawning_host_impl->status_ !=
       AppCacheStatus::APPCACHE_STATUS_UNCACHED)) {
    backend_->SetSpawningHostId(host_id_, spawning_host_impl->host_id());
  }
}

void WebApplicationCacheHostImpl::SelectCacheWithoutManifest() {
  if (was_select_cache_called_)
    return;
  was_select_cache_called_ = true;

  status_ = (document_response_.AppCacheID() == kAppCacheNoCacheId)
                ? AppCacheStatus::APPCACHE_STATUS_UNCACHED
                : AppCacheStatus::APPCACHE_STATUS_CHECKING;
  is_new_master_entry_ = OLD_ENTRY;
  backend_->SelectCache(host_id_, document_url_,
                        document_response_.AppCacheID(), GURL());
}

bool WebApplicationCacheHostImpl::SelectCacheWithManifest(
    const WebURL& manifest_url) {
  if (was_select_cache_called_)
    return true;
  was_select_cache_called_ = true;

  GURL manifest_gurl(ClearUrlRef(manifest_url));

  // 6.9.6 The application cache selection algorithm
  // Check for new 'master' entries.
  if (document_response_.AppCacheID() == kAppCacheNoCacheId) {
    if (is_scheme_supported_ && is_get_method_ &&
        (manifest_gurl.GetOrigin() == document_url_.GetOrigin())) {
      status_ = AppCacheStatus::APPCACHE_STATUS_CHECKING;
      is_new_master_entry_ = NEW_ENTRY;
    } else {
      status_ = AppCacheStatus::APPCACHE_STATUS_UNCACHED;
      is_new_master_entry_ = OLD_ENTRY;
      manifest_gurl = GURL();
    }
    backend_->SelectCache(
        host_id_, document_url_, kAppCacheNoCacheId, manifest_gurl);
    return true;
  }

  DCHECK_EQ(OLD_ENTRY, is_new_master_entry_);

  // 6.9.6 The application cache selection algorithm
  // Check for 'foreign' entries.
  GURL document_manifest_gurl(document_response_.AppCacheManifestURL());
  if (document_manifest_gurl != manifest_gurl) {
    backend_->MarkAsForeignEntry(host_id_, document_url_,
                                 document_response_.AppCacheID());
    status_ = AppCacheStatus::APPCACHE_STATUS_UNCACHED;
    return false;  // the navigation will be restarted
  }

  status_ = AppCacheStatus::APPCACHE_STATUS_CHECKING;

  // Its a 'master' entry thats already in the cache.
  backend_->SelectCache(host_id_, document_url_,
                        document_response_.AppCacheID(), manifest_gurl);
  return true;
}

void WebApplicationCacheHostImpl::DidReceiveResponseForMainResource(
    const WebURLResponse& response) {
  document_response_ = response;
  document_url_ = ClearUrlRef(document_response_.Url());
  if (document_url_ != original_main_resource_url_)
    is_get_method_ = true;  // A redirect was involved.
  original_main_resource_url_ = GURL();

  is_scheme_supported_ =  IsSchemeSupportedForAppCache(document_url_);
  if ((document_response_.AppCacheID() != kAppCacheNoCacheId) ||
      !is_scheme_supported_ || !is_get_method_)
    is_new_master_entry_ = OLD_ENTRY;
}

void WebApplicationCacheHostImpl::DidReceiveDataForMainResource(
    const char* data,
    unsigned len) {
  if (is_new_master_entry_ == OLD_ENTRY)
    return;
  // TODO(michaeln): write me
}

void WebApplicationCacheHostImpl::DidFinishLoadingMainResource(bool success) {
  if (is_new_master_entry_ == OLD_ENTRY)
    return;
  // TODO(michaeln): write me
}

WebApplicationCacheHost::Status WebApplicationCacheHostImpl::GetStatus() {
  return static_cast<WebApplicationCacheHost::Status>(status_);
}

bool WebApplicationCacheHostImpl::StartUpdate() {
  if (!backend_->StartUpdate(host_id_))
    return false;
  if (status_ == AppCacheStatus::APPCACHE_STATUS_IDLE ||
      status_ == AppCacheStatus::APPCACHE_STATUS_UPDATE_READY)
    status_ = AppCacheStatus::APPCACHE_STATUS_CHECKING;
  else
    status_ = backend_->GetStatus(host_id_);
  return true;
}

bool WebApplicationCacheHostImpl::SwapCache() {
  if (!backend_->SwapCache(host_id_))
    return false;
  status_ = backend_->GetStatus(host_id_);
  return true;
}

void WebApplicationCacheHostImpl::GetAssociatedCacheInfo(
    WebApplicationCacheHost::CacheInfo* info) {
  info->manifest_url = cache_info_.manifest_url;
  if (!cache_info_.is_complete)
    return;
  info->creation_time = cache_info_.creation_time.ToDoubleT();
  info->update_time = cache_info_.last_update_time.ToDoubleT();
  info->total_size = cache_info_.size;
}

int WebApplicationCacheHostImpl::GetHostID() const {
  return host_id_;
}

void WebApplicationCacheHostImpl::GetResourceList(
    WebVector<ResourceInfo>* resources) {
  if (!cache_info_.is_complete)
    return;
  std::vector<AppCacheResourceInfo> resource_infos;
  backend_->GetResourceList(host_id_, &resource_infos);

  WebVector<ResourceInfo> web_resources(resource_infos.size());
  for (size_t i = 0; i < resource_infos.size(); ++i) {
    web_resources[i].size = resource_infos[i].size;
    web_resources[i].is_master = resource_infos[i].is_master;
    web_resources[i].is_explicit = resource_infos[i].is_explicit;
    web_resources[i].is_manifest = resource_infos[i].is_manifest;
    web_resources[i].is_foreign = resource_infos[i].is_foreign;
    web_resources[i].is_fallback = resource_infos[i].is_fallback;
    web_resources[i].url = resource_infos[i].url;
  }
  resources->Swap(web_resources);
}

}  // namespace content
