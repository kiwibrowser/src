// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/media/resource_bundle_helper.h"

#include "third_party/zlib/google/compression_utils.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_handle.h"

namespace {

std::string GetResource(int resource_id) {
  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
  return bundle
      .GetRawDataResourceForScale(resource_id, bundle.GetMaxScaleFactor())
      .as_string();
}

}  // namespace.

namespace blink {

String ResourceBundleHelper::GetResourceAsString(int resource_id) {
  return String::FromUTF8(GetResource(resource_id).c_str());
};

String ResourceBundleHelper::UncompressResourceAsString(int resource_id) {
  std::string uncompressed;
  CHECK(compression::GzipUncompress(GetResource(resource_id), &uncompressed));
  return String::FromUTF8(uncompressed.c_str());
}

}  // namespace blink
