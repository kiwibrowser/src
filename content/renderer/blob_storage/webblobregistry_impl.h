// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_BLOB_STORAGE_WEBBLOBREGISTRY_IMPL_H_
#define CONTENT_RENDERER_BLOB_STORAGE_WEBBLOBREGISTRY_IMPL_H_

#include "base/memory/ref_counted.h"
#include "third_party/blink/public/platform/web_blob_registry.h"

namespace content {
class ThreadSafeSender;

class WebBlobRegistryImpl : public blink::WebBlobRegistry {
 public:
  explicit WebBlobRegistryImpl(scoped_refptr<ThreadSafeSender> sender);
  ~WebBlobRegistryImpl() override;

  void RegisterPublicBlobURL(const blink::WebURL&,
                             const blink::WebString& uuid) override;
  void RevokePublicBlobURL(const blink::WebURL&) override;

 private:

  scoped_refptr<ThreadSafeSender> sender_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_BLOB_STORAGE_WEBBLOBREGISTRY_IMPL_H_
