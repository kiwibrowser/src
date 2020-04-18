// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_FONT_LOADER_DISPATCHER_MAC_H_
#define CONTENT_COMMON_FONT_LOADER_DISPATCHER_MAC_H_

#include "content/common/font_loader_mac.mojom.h"

namespace service_manager {
struct BindSourceInfo;
}

namespace content {

// Dispatches message used for font loading on Mac. This is needed because
// Mac can't load fonts outside its conventional font locations in sandboxed
// processes. So the sandboxed process asks the browser process to do this
// for it.
class FontLoaderDispatcher : public mojom::FontLoaderMac {
 public:
  FontLoaderDispatcher();
  ~FontLoaderDispatcher() override;

  static void Create(mojom::FontLoaderMacRequest request,
                     const service_manager::BindSourceInfo& source_info);

 private:
  // mojom::FontLoaderMac
  void LoadFont(const base::string16& font_name,
                float font_point_size,
                mojom::FontLoaderMac::LoadFontCallback callback) override;

  DISALLOW_COPY_AND_ASSIGN(FontLoaderDispatcher);
};

}  // namespace content

#endif  // CONTENT_COMMON_FONT_LOADER_DISPATCHER_MAC_H_
