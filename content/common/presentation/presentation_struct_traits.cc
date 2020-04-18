// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/presentation/presentation_struct_traits.h"

#include "url/mojom/url_gurl_mojom_traits.h"

namespace mojo {

bool UnionTraits<blink::mojom::PresentationConnectionMessageDataView,
                 content::PresentationConnectionMessage>::
    Read(blink::mojom::PresentationConnectionMessageDataView data,
         content::PresentationConnectionMessage* out) {
  if (data.is_message()) {
    if (!data.ReadMessage(&(out->message))) {
      return false;
    }
  } else {
    if (!data.ReadData(&(out->data))) {
      return false;
    }
  }
  return true;
}
}
