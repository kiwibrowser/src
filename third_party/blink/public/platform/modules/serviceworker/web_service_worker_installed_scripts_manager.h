// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_H_

#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_vector.h"

#if INSIDE_BLINK
#include <memory>
#include "third_party/blink/renderer/platform/network/http_header_map.h"
#endif  // INSIDE_BLINK

namespace blink {

// WebServiceWorkerInstalledScriptsManager provides installed main script and
// imported scripts to the service worker thread. This is used by multiple
// threads, so implementation needs to be thread safe.
class WebServiceWorkerInstalledScriptsManager {
 public:
  using BytesChunk = WebVector<char>;
  // Container of a script. All the fields of this class need to be
  // cross-thread-transfer-safe.
  class BLINK_PLATFORM_EXPORT RawScriptData {
   public:
    static std::unique_ptr<RawScriptData> Create(
        WebString encoding,
        WebVector<BytesChunk> script_text,
        WebVector<BytesChunk> meta_data);
    static std::unique_ptr<RawScriptData> CreateInvalidInstance();

    // Implementation of the destructor should be in the Blink side because only
    // Blink can know all of members.
    ~RawScriptData();

    void AddHeader(const WebString& key, const WebString& value);

    // Returns false if it fails to receive the script from the browser.
    bool IsValid() const { return is_valid_; }
    // The encoding of the script text.
    const WebString& Encoding() const {
      DCHECK(is_valid_);
      return encoding_;
    }
    // An array of raw byte chunks of the script text.
    const WebVector<BytesChunk>& ScriptTextChunks() const {
      DCHECK(is_valid_);
      return script_text_;
    }
    // An array of raw byte chunks of the scripts's meta data from the script's
    // V8 code cache.
    const WebVector<BytesChunk>& MetaDataChunks() const {
      DCHECK(is_valid_);
      return meta_data_;
    }

#if INSIDE_BLINK
    // The HTTP headers of the script.
    std::unique_ptr<CrossThreadHTTPHeaderMapData> TakeHeaders() {
      DCHECK(is_valid_);
      return std::move(headers_);
    }
#endif  // INSIDE_BLINK

   private:
    // This instance must be created on the Blink side because only Blink can
    // know the exact size of this instance.
    RawScriptData(WebString encoding,
                  WebVector<BytesChunk> script_text,
                  WebVector<BytesChunk> meta_data,
                  bool is_valid);
    const bool is_valid_;
    WebString encoding_;
    WebVector<BytesChunk> script_text_;
    WebVector<BytesChunk> meta_data_;
#if INSIDE_BLINK
    std::unique_ptr<CrossThreadHTTPHeaderMapData> headers_;
#endif  // INSIDE_BLINK
  };

  virtual ~WebServiceWorkerInstalledScriptsManager() = default;

  // Called on the main or worker thread.
  virtual bool IsScriptInstalled(const blink::WebURL& script_url) const = 0;
  // Returns the script data for |script_url|. When an error happened during
  // receiving the script, returns an invalid instance.
  // Called on the worker thread.
  virtual std::unique_ptr<RawScriptData> GetRawScriptData(
      const WebURL& script_url) = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_H_
