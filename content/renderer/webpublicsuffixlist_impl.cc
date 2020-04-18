// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/webpublicsuffixlist_impl.h"

#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace content {

WebPublicSuffixListImpl::~WebPublicSuffixListImpl() {
}

size_t WebPublicSuffixListImpl::GetPublicSuffixLength(
    const blink::WebString& host) {
  // Blink passes some things that aren't technically hosts like "*.foo", so
  // use the permissive variant.
  size_t result =
      net::registry_controlled_domains::PermissiveGetHostRegistryLength(
          host.Utf8(),
          net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  return result ? result : host.length();
}

} // namespace content
