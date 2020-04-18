// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_WEBPUBLICSUFFIXLIST_IMPL_H_
#define CONTENT_RENDERER_WEBPUBLICSUFFIXLIST_IMPL_H_

#include <stddef.h>

#include "base/compiler_specific.h"
#include "third_party/blink/public/platform/web_public_suffix_list.h"

namespace content {

class WebPublicSuffixListImpl : public blink::WebPublicSuffixList {
 public:
  // WebPublicSuffixList methods:
  size_t GetPublicSuffixLength(const blink::WebString&) override;
  ~WebPublicSuffixListImpl() override;
};

}  // namespace content

#endif  // CONTENT_RENDERER_WEBPUBLICSUFFIXLIST_IMPL_H_
