// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_DOM_STORAGE_LOCAL_STORAGE_NAMESPACE_H_
#define CONTENT_RENDERER_DOM_STORAGE_LOCAL_STORAGE_NAMESPACE_H_

#include "base/macros.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_storage_namespace.h"

namespace content {
class LocalStorageCachedAreas;

class LocalStorageNamespace : public blink::WebStorageNamespace {
 public:
  // |local_storage_cached_areas| is guaranteed to outlive this object.
  explicit LocalStorageNamespace(
      LocalStorageCachedAreas* local_storage_cached_areas);
  ~LocalStorageNamespace() override;

  // blink::WebStorageNamespace:
  blink::WebStorageArea* CreateStorageArea(
      const blink::WebSecurityOrigin& origin) override;
  bool IsSameNamespace(const WebStorageNamespace&) const override;

 private:
  LocalStorageCachedAreas* const local_storage_cached_areas_;

  DISALLOW_COPY_AND_ASSIGN(LocalStorageNamespace);
};

}  // namespace content

#endif  // CONTENT_RENDERER_DOM_STORAGE_LOCAL_STORAGE_NAMESPACE_H_
