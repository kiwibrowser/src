// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/referrer_struct_traits.h"

#include "url/mojom/url_gurl_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<::blink::mojom::ReferrerDataView, content::Referrer>::Read(
    ::blink::mojom::ReferrerDataView data,
    content::Referrer* out) {
  return data.ReadUrl(&out->url) && data.ReadPolicy(&out->policy);
}

}  // namespace mojo
