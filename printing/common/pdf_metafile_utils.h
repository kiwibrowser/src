// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_COMMON_PDF_METAFILE_UTILS_H_
#define PRINTING_COMMON_PDF_METAFILE_UTILS_H_

#include <map>
#include <string>

#include "base/containers/flat_map.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/skia/include/core/SkDocument.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkSerialProcs.h"
#include "third_party/skia/include/core/SkStream.h"

namespace printing {

using ContentToProxyIdMap = std::map<uint32_t, int>;

enum class SkiaDocumentType {
  PDF,
  // MSKP is an experimental, fragile, and diagnostic-only document type.
  MSKP,
  MAX = MSKP
};

// Stores the mapping between a content's unique id and its actual content.
using DeserializationContext = base::flat_map<uint32_t, sk_sp<SkPicture>>;

// Stores the mapping between content's unique id and its corresponding frame
// proxy id.
using SerializationContext = ContentToProxyIdMap;

sk_sp<SkDocument> MakePdfDocument(const std::string& creator,
                                  SkWStream* stream);

SkSerialProcs SerializationProcs(SerializationContext* ctx);

SkDeserialProcs DeserializationProcs(DeserializationContext* ctx);

}  // namespace printing

#endif  // PRINTING_COMMON_PDF_METAFILE_UTILS_H_
