// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_RENDERER_EXTENSION_REGISTRY_H_
#define EXTENSIONS_RENDERER_RENDERER_EXTENSION_REGISTRY_H_

#include <stddef.h>

#include <string>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "extensions/common/extension_set.h"

class GURL;

namespace extensions {

// Thread safe container for all loaded extensions in this process,
// essentially the renderer counterpart to ExtensionRegistry.
class RendererExtensionRegistry {
 public:
  RendererExtensionRegistry();
  ~RendererExtensionRegistry();

  static RendererExtensionRegistry* Get();

  // Returns the ExtensionSet that underlies this RenderExtensionRegistry.
  //
  // This is not thread-safe and must only be called on the RenderThread, but
  // even so, it's not thread safe because other threads may decide to
  // modify this. Don't persist a reference to this.
  //
  // TODO(annekao): remove or make thread-safe and callback-based.
  const ExtensionSet* GetMainThreadExtensionSet() const;

  size_t size() const;
  bool is_empty() const;

  // Forwards to the ExtensionSet methods by the same name.
  bool Contains(const std::string& id) const;
  bool Insert(const scoped_refptr<const Extension>& extension);
  bool Remove(const std::string& id);
  std::string GetExtensionOrAppIDByURL(const GURL& url) const;
  const Extension* GetExtensionOrAppByURL(const GURL& url) const;
  const Extension* GetHostedAppByURL(const GURL& url) const;
  const Extension* GetByID(const std::string& id) const;
  ExtensionIdSet GetIDs() const;
  bool ExtensionBindingsAllowed(const GURL& url) const;

 private:
  ExtensionSet extensions_;

  mutable base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(RendererExtensionRegistry);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_RENDERER_EXTENSION_REGISTRY_H_
