// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/resource/text_resource.h"

#include "third_party/blink/renderer/core/html/parser/text_resource_decoder.h"
#include "third_party/blink/renderer/platform/loader/fetch/text_resource_decoder_options.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

TextResource::TextResource(const ResourceRequest& resource_request,
                           Resource::Type type,
                           const ResourceLoaderOptions& options,
                           const TextResourceDecoderOptions& decoder_options)
    : Resource(resource_request, type, options),
      decoder_(TextResourceDecoder::Create(decoder_options)) {}

TextResource::~TextResource() = default;

void TextResource::SetEncoding(const String& chs) {
  decoder_->SetEncoding(WTF::TextEncoding(chs),
                        TextResourceDecoder::kEncodingFromHTTPHeader);
}

WTF::TextEncoding TextResource::Encoding() const {
  return decoder_->Encoding();
}

String TextResource::DecodedText() const {
  DCHECK(Data());

  StringBuilder builder;
  const char* segment;
  size_t position = 0;
  while (size_t length = Data()->GetSomeData(segment, position)) {
    builder.Append(decoder_->Decode(segment, length));
    position += length;
  }
  builder.Append(decoder_->Flush());
  return builder.ToString();
}

}  // namespace blink
