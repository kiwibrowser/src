// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/font_loader_dispatcher_mac.h"

#include <memory>
#include <utility>

#include "base/strings/string16.h"
#include "content/common/mac/font_loader.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace content {

FontLoaderDispatcher::FontLoaderDispatcher() {}

FontLoaderDispatcher::~FontLoaderDispatcher() {}

// static
void FontLoaderDispatcher::Create(
    mojom::FontLoaderMacRequest request,
    const service_manager::BindSourceInfo& source_info) {
  mojo::MakeStrongBinding(std::make_unique<FontLoaderDispatcher>(),
                          std::move(request));
}

void FontLoaderDispatcher::LoadFont(
    const base::string16& font_name,
    float font_point_size,
    mojom::FontLoaderMac::LoadFontCallback callback) {
  FontLoader::LoadFont(font_name, font_point_size, std::move(callback));
}

}  // namespace content
