// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_FETCHERS_MANIFEST_FETCHER_H_
#define CONTENT_RENDERER_FETCHERS_MANIFEST_FETCHER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/platform/web_url_response.h"

class GURL;

namespace blink {
class WebLocalFrame;
}

namespace content {

class AssociatedResourceFetcher;

// Helper class to download a Web Manifest. When an instance is created, the
// caller need to call Start() and wait for the passed callback to be executed.
// If the fetch fails, the callback will be called with two empty objects.
class CONTENT_EXPORT ManifestFetcher {
 public:
  // This will be called asynchronously after the URL has been fetched,
  // successfully or not.  If there is a failure, response and data will both be
  // empty.  |response| and |data| are both valid until the URLFetcher instance
  // is destroyed.
  typedef base::Callback<void(const blink::WebURLResponse& response,
                              const std::string& data)> Callback;

  explicit ManifestFetcher(const GURL& url);
  virtual ~ManifestFetcher();

  void Start(blink::WebLocalFrame* frame,
             bool use_credentials,
             const Callback& callback);
  void Cancel();

 private:
  void OnLoadComplete(const blink::WebURLResponse& response,
                      const std::string& data);

  bool completed_;
  Callback callback_;
  std::unique_ptr<AssociatedResourceFetcher> fetcher_;

  DISALLOW_COPY_AND_ASSIGN(ManifestFetcher);
};

}  // namespace content

#endif  // CONTENT_RENDERER_FETCHERS_MANIFEST_FETCHER_H_
