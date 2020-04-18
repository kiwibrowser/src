// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INTERNAL_DOCUMENT_STATE_DATA_H_
#define CONTENT_RENDERER_INTERNAL_DOCUMENT_STATE_DATA_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/supports_user_data.h"
#include "third_party/blink/public/platform/modules/fetch/fetch_api_request.mojom-shared.h"
#include "url/gurl.h"

namespace blink {
class WebDocumentLoader;
}

namespace content {

class DocumentState;

// Stores internal state per WebDocumentLoader.
class InternalDocumentStateData : public base::SupportsUserData::Data {
 public:
  InternalDocumentStateData();
  ~InternalDocumentStateData() override;

  static InternalDocumentStateData* FromDocumentLoader(
      blink::WebDocumentLoader* document_loader);
  static InternalDocumentStateData* FromDocumentState(DocumentState* ds);

  int http_status_code() const { return http_status_code_; }
  void set_http_status_code(int http_status_code) {
    http_status_code_ = http_status_code;
  }

  const GURL& searchable_form_url() const { return searchable_form_url_; }
  void set_searchable_form_url(const GURL& url) { searchable_form_url_ = url; }
  const std::string& searchable_form_encoding() const {
    return searchable_form_encoding_;
  }
  void set_searchable_form_encoding(const std::string& encoding) {
    searchable_form_encoding_ = encoding;
  }

  // True if the user agent was overridden for this page.
  bool is_overriding_user_agent() const { return is_overriding_user_agent_; }
  void set_is_overriding_user_agent(bool state) {
    is_overriding_user_agent_ = state;
  }

  // True if we have to reset the scroll and scale state of the page
  // after the provisional load has been committed.
  bool must_reset_scroll_and_scale_state() const {
    return must_reset_scroll_and_scale_state_;
  }
  void set_must_reset_scroll_and_scale_state(bool state) {
    must_reset_scroll_and_scale_state_ = state;
  }

  // Sets the cache policy. The cache policy is only used if explicitly set and
  // by default is not set. You can mark a NavigationState as not having a cache
  // state by way of clear_cache_policy_override.
  void set_cache_policy_override(blink::mojom::FetchCacheMode cache_policy) {
    cache_policy_override_ = cache_policy;
    cache_policy_override_set_ = true;
  }
  blink::mojom::FetchCacheMode cache_policy_override() const {
    return cache_policy_override_;
  }
  void clear_cache_policy_override() {
    cache_policy_override_set_ = false;
    cache_policy_override_ = blink::mojom::FetchCacheMode::kDefault;
  }
  bool is_cache_policy_override_set() const {
    return cache_policy_override_set_;
  }

 private:
  int http_status_code_;
  GURL searchable_form_url_;
  std::string searchable_form_encoding_;
  bool is_overriding_user_agent_;
  bool must_reset_scroll_and_scale_state_;
  bool cache_policy_override_set_;
  blink::mojom::FetchCacheMode cache_policy_override_;

  DISALLOW_COPY_AND_ASSIGN(InternalDocumentStateData);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INTERNAL_DOCUMENT_STATE_DATA_H_
