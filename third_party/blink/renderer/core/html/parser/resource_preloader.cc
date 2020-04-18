// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/parser/resource_preloader.h"
#include "third_party/blink/renderer/core/loader/network_hints_interface.h"

namespace blink {

void ResourcePreloader::TakeAndPreload(PreloadRequestStream& r) {
  PreloadRequestStream requests;
  NetworkHintsInterfaceImpl network_hints_interface;
  requests.swap(r);

  for (PreloadRequestStream::iterator it = requests.begin();
       it != requests.end(); ++it)
    Preload(std::move(*it), network_hints_interface);
}

}  // namespace blink
