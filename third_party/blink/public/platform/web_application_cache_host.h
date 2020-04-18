/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_APPLICATION_CACHE_HOST_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_APPLICATION_CACHE_HOST_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace blink {

class WebString;
class WebURL;
class WebURLResponse;

// This interface is used by webkit to call out to the embedder. Webkit uses
// the WebFrameClient::CreateApplicationCacheHost method to create instances,
// and calls delete when the instance is no longer needed.
class WebApplicationCacheHost {
 public:
  // These values must match blink::ApplicationCacheHost::Status values
  enum Status {
    kUncached,
    kIdle,
    kChecking,
    kDownloading,
    kUpdateReady,
    kObsolete
  };

  // These values must match blink::ApplicationCacheHost::EventID values
  enum EventID {
    kCheckingEvent,
    kErrorEvent,
    kNoUpdateEvent,
    kDownloadingEvent,
    kProgressEvent,
    kUpdateReadyEvent,
    kCachedEvent,
    kObsoleteEvent
  };

  enum ErrorReason {
    kManifestError,
    kSignatureError,
    kResourceError,
    kChangedError,
    kAbortError,
    kQuotaError,
    kPolicyError,
    kUnknownError
  };

  static const int kAppCacheNoHostId = 0;

  virtual ~WebApplicationCacheHost() = default;

  // Called for every request made within the context.
  virtual void WillStartMainResourceRequest(
      const WebURL& url,
      const WebString& method,
      const WebApplicationCacheHost* spawning_host) {}

  // One or the other selectCache methods is called after having parsed the
  // <html> tag.  The latter returns false if the current document has been
  // identified as a "foreign" entry, in which case the frame navigation will be
  // restarted by webkit.
  virtual void SelectCacheWithoutManifest() {}
  virtual bool SelectCacheWithManifest(const WebURL& manifest_url) {
    return true;
  }

  // Called as the main resource is retrieved.
  virtual void DidReceiveResponseForMainResource(const WebURLResponse&) {}
  virtual void DidReceiveDataForMainResource(const char* data, unsigned len) {}
  virtual void DidFinishLoadingMainResource(bool success) {}

  // Called on behalf of the scriptable interface.
  virtual Status GetStatus() { return kUncached; }
  virtual bool StartUpdate() { return false; }
  virtual bool SwapCache() { return false; }
  virtual void Abort() {}

  // Structures and methods to support inspecting Application Caches.
  struct CacheInfo {
    WebURL manifest_url;  // Empty if there is no associated cache.
    double creation_time;
    double update_time;
    long long total_size;
    CacheInfo() : creation_time(0), update_time(0), total_size(0) {}
  };
  struct ResourceInfo {
    WebURL url;
    long long size;
    bool is_master;
    bool is_manifest;
    bool is_explicit;
    bool is_foreign;
    bool is_fallback;
    ResourceInfo()
        : size(0),
          is_master(false),
          is_manifest(false),
          is_explicit(false),
          is_foreign(false),
          is_fallback(false) {}
  };
  virtual void GetAssociatedCacheInfo(CacheInfo*) {}
  virtual void GetResourceList(WebVector<ResourceInfo>*) {}
  virtual void DeleteAssociatedCacheGroup() {}
  virtual int GetHostID() const { return kAppCacheNoHostId; }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_APPLICATION_CACHE_HOST_H_
