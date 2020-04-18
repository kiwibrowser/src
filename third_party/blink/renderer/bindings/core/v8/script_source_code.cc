// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/script_source_code.h"

#include "third_party/blink/renderer/core/loader/resource/script_resource.h"

namespace blink {

namespace {

String TreatNullSourceAsEmpty(const String& source) {
  // ScriptSourceCode allows for the representation of the null/not-there-really
  // ScriptSourceCode value.  Encoded by way of a m_source.isNull() being true,
  // with the nullary constructor to be used to construct such a value.
  //
  // Should the other constructors be passed a null string, that is interpreted
  // as representing the empty script. Consequently, we need to disambiguate
  // between such null string occurrences.  Do that by converting the latter
  // case's null strings into empty ones.
  if (source.IsNull())
    return "";

  return source;
}

KURL StripFragmentIdentifier(const KURL& url) {
  if (url.IsEmpty())
    return KURL();

  if (!url.HasFragmentIdentifier())
    return url;

  KURL copy = url;
  copy.RemoveFragmentIdentifier();
  return copy;
}

String SourceMapUrlFromResponse(const ResourceResponse& response) {
  String source_map_url = response.HttpHeaderField(HTTPNames::SourceMap);
  if (!source_map_url.IsEmpty())
    return source_map_url;

  // Try to get deprecated header.
  return response.HttpHeaderField(HTTPNames::X_SourceMap);
}

}  // namespace

ScriptSourceCode::ScriptSourceCode(
    const String& source,
    ScriptSourceLocationType source_location_type,
    SingleCachedMetadataHandler* cache_handler,
    const KURL& url,
    const TextPosition& start_position)
    : source_(TreatNullSourceAsEmpty(source)),
      cache_handler_(cache_handler),
      url_(StripFragmentIdentifier(url)),
      start_position_(start_position),
      source_location_type_(source_location_type) {
  // External files should use a ScriptResource.
  DCHECK(source_location_type != ScriptSourceLocationType::kExternalFile);
}

ScriptSourceCode::ScriptSourceCode(ScriptStreamer* streamer,
                                   ScriptResource* resource)
    : source_(TreatNullSourceAsEmpty(resource->SourceText())),
      cache_handler_(resource->CacheHandler()),
      streamer_(streamer),
      url_(StripFragmentIdentifier(resource->GetResponse().Url())),
      source_map_url_(SourceMapUrlFromResponse(resource->GetResponse())),
      start_position_(TextPosition::MinimumPosition()),
      source_location_type_(ScriptSourceLocationType::kExternalFile) {}

ScriptSourceCode::~ScriptSourceCode() = default;

void ScriptSourceCode::Trace(blink::Visitor* visitor) {
  visitor->Trace(cache_handler_);
  visitor->Trace(streamer_);
}

}  // namespace blink
