// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_LOFI_DECIDER_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_LOFI_DECIDER_H_

#include "base/macros.h"

class GURL;

namespace net {
class HttpRequestHeaders;
class URLRequest;
}

namespace previews {
class PreviewsDecider;
}

namespace data_reduction_proxy {

// Interface to determine if a request should be made for a low fidelity version
// of the resource.
class LoFiDecider {
 public:
  virtual ~LoFiDecider() {}

  // Adds a previews-specific directive to the Chrome-Proxy-Accept-Transform
  // header if needed.
  virtual void MaybeSetAcceptTransformHeader(
      const net::URLRequest& request,
      net::HttpRequestHeaders* headers) const = 0;

  // Returns true if |headers| contains the Chrome-Proxy-Accept-Transform
  // header and a slow page previews directive ("lite-page" or "empty-image")
  // is present and not conditioned on "if-heavy".
  virtual bool IsSlowPagePreviewRequested(
      const net::HttpRequestHeaders& headers) const = 0;

  // Returns true if |headers| contains the Chrome-Proxy-Accept-Transform
  // header with the "lite-page" directive.
  virtual bool IsLitePagePreviewRequested(
      const net::HttpRequestHeaders& headers) const = 0;

  // Unconditionally removes the Chrome-Proxy-Accept-Transform header from
  // |headers.|
  virtual void RemoveAcceptTransformHeader(
      net::HttpRequestHeaders* headers) const = 0;

  // Returns true if the Lo-Fi specific UMA should be recorded. It is set to
  // true if Lo-Fi is enabled for |request|, Chrome session is in Lo-Fi
  // Enabled or Control field trial, and the network quality was slow.
  virtual bool ShouldRecordLoFiUMA(const net::URLRequest& request) const = 0;

  // Returns whether the request was a client-side Lo-Fi image request.
  virtual bool IsClientLoFiImageRequest(
      const net::URLRequest& request) const = 0;

  // Returns true if the request is for a client-side Lo-Fi image that is being
  // automatically reloaded because of a decoding error.
  virtual bool IsClientLoFiAutoReloadRequest(
      const net::URLRequest& request) const = 0;

  // Applies the AMP redirection preview by changing the |new_url|.
  virtual void MaybeApplyAMPPreview(
      net::URLRequest* request,
      GURL* new_url,
      previews::PreviewsDecider* previews_decider) const = 0;
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_LOFI_DECIDER_H_
