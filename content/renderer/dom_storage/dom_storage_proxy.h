// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_DOM_STORAGE_DOM_STORAGE_PROXY_H_
#define CONTENT_RENDERER_DOM_STORAGE_DOM_STORAGE_PROXY_H_

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/string16.h"
#include "content/common/dom_storage/dom_storage_types.h"
#include "url/gurl.h"

namespace content {

// Abstract interface for cached area, renderer to browser communications.
class DOMStorageProxy : public base::RefCounted<DOMStorageProxy> {
 public:
  typedef base::OnceCallback<void(bool)> CompletionCallback;

  virtual void LoadArea(int connection_id,
                        DOMStorageValuesMap* values,
                        CompletionCallback callback) = 0;

  virtual void SetItem(int connection_id,
                       const base::string16& key,
                       const base::string16& value,
                       const base::NullableString16& old_value,
                       const GURL& page_url,
                       CompletionCallback callback) = 0;

  virtual void RemoveItem(int connection_id,
                          const base::string16& key,
                          const base::NullableString16& old_value,
                          const GURL& page_url,
                          CompletionCallback callback) = 0;

  virtual void ClearArea(int connection_id,
                         const GURL& page_url,
                         CompletionCallback callback) = 0;

 protected:
  friend class base::RefCounted<DOMStorageProxy>;
  virtual ~DOMStorageProxy() {}
};

}  // namespace content

#endif  // CONTENT_RENDERER_DOM_STORAGE_DOM_STORAGE_PROXY_H_
