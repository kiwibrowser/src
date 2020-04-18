// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_SECURITY_ORIGIN_STRUCT_TRAITS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_SECURITY_ORIGIN_STRUCT_TRAITS_H_

#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "url/mojom/origin.mojom-blink.h"

namespace mojo {

template <>
struct StructTraits<url::mojom::blink::Origin::DataView,
                    scoped_refptr<const ::blink::SecurityOrigin>> {
  static WTF::String scheme(
      const scoped_refptr<const ::blink::SecurityOrigin>& origin) {
    return origin->Protocol();
  }
  static WTF::String host(
      const scoped_refptr<const ::blink::SecurityOrigin>& origin) {
    return origin->Host();
  }
  static uint16_t port(
      const scoped_refptr<const ::blink::SecurityOrigin>& origin) {
    return origin->EffectivePort();
  }
  static bool unique(
      const scoped_refptr<const ::blink::SecurityOrigin>& origin) {
    return origin->IsUnique();
  }
  static bool Read(url::mojom::blink::Origin::DataView data,
                   scoped_refptr<const ::blink::SecurityOrigin>* out) {
    if (data.unique()) {
      *out = ::blink::SecurityOrigin::CreateUnique();
    } else {
      WTF::String scheme;
      WTF::String host;
      if (!data.ReadScheme(&scheme) || !data.ReadHost(&host))
        return false;

      *out = ::blink::SecurityOrigin::Create(scheme, host, data.port());
    }

    // If a unique origin was created, but the unique flag wasn't set, then
    // the values provided to 'create' were invalid.
    if (!data.unique() && (*out)->IsUnique())
      return false;

    return true;
  }
};
}

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_SECURITY_ORIGIN_STRUCT_TRAITS_H_
