// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/link_header.h"

#include "base/strings/string_util.h"
#include "components/link_header_util/link_header_util.h"
#include "third_party/blink/renderer/platform/wtf/text/parsing_utilities.h"

namespace blink {

// Verify that the parameter is a link-extension which according to spec doesn't
// have to have a value.
static bool IsExtensionParameter(LinkHeader::LinkParameterName name) {
  return name >= LinkHeader::kLinkParameterUnknown;
}

static LinkHeader::LinkParameterName ParameterNameFromString(
    base::StringPiece name) {
  if (base::EqualsCaseInsensitiveASCII(name, "rel"))
    return LinkHeader::kLinkParameterRel;
  if (base::EqualsCaseInsensitiveASCII(name, "anchor"))
    return LinkHeader::kLinkParameterAnchor;
  if (base::EqualsCaseInsensitiveASCII(name, "crossorigin"))
    return LinkHeader::kLinkParameterCrossOrigin;
  if (base::EqualsCaseInsensitiveASCII(name, "title"))
    return LinkHeader::kLinkParameterTitle;
  if (base::EqualsCaseInsensitiveASCII(name, "media"))
    return LinkHeader::kLinkParameterMedia;
  if (base::EqualsCaseInsensitiveASCII(name, "type"))
    return LinkHeader::kLinkParameterType;
  if (base::EqualsCaseInsensitiveASCII(name, "rev"))
    return LinkHeader::kLinkParameterRev;
  if (base::EqualsCaseInsensitiveASCII(name, "hreflang"))
    return LinkHeader::kLinkParameterHreflang;
  if (base::EqualsCaseInsensitiveASCII(name, "as"))
    return LinkHeader::kLinkParameterAs;
  if (base::EqualsCaseInsensitiveASCII(name, "nonce"))
    return LinkHeader::kLinkParameterNonce;
  if (base::EqualsCaseInsensitiveASCII(name, "integrity"))
    return LinkHeader::kLinkParameterIntegrity;
  if (base::EqualsCaseInsensitiveASCII(name, "srcset"))
    return LinkHeader::kLinkParameterSrcset;
  if (base::EqualsCaseInsensitiveASCII(name, "imgsizes"))
    return LinkHeader::kLinkParameterImgsizes;
  return LinkHeader::kLinkParameterUnknown;
}

void LinkHeader::SetValue(LinkParameterName name, const String& value) {
  if (name == kLinkParameterRel && !rel_)
    rel_ = value.DeprecatedLower();
  else if (name == kLinkParameterAnchor)
    is_valid_ = false;
  else if (name == kLinkParameterCrossOrigin)
    cross_origin_ = value;
  else if (name == kLinkParameterAs)
    as_ = value.DeprecatedLower();
  else if (name == kLinkParameterType)
    mime_type_ = value.DeprecatedLower();
  else if (name == kLinkParameterMedia)
    media_ = value.DeprecatedLower();
  else if (name == kLinkParameterNonce)
    nonce_ = value;
  else if (name == kLinkParameterIntegrity)
    integrity_ = value;
  else if (name == kLinkParameterSrcset)
    srcset_ = value;
  else if (name == kLinkParameterImgsizes)
    imgsizes_ = value;
}

template <typename Iterator>
LinkHeader::LinkHeader(Iterator begin, Iterator end) : is_valid_(true) {
  std::string url;
  std::unordered_map<std::string, base::Optional<std::string>> params;
  is_valid_ = link_header_util::ParseLinkHeaderValue(begin, end, &url, &params);
  if (!is_valid_)
    return;

  url_ = String(&url[0], url.length());
  for (const auto& param : params) {
    LinkParameterName name = ParameterNameFromString(param.first);
    if (!IsExtensionParameter(name) && !param.second)
      is_valid_ = false;
    std::string value = param.second.value_or("");
    SetValue(name, String(&value[0], value.length()));
  }
}

LinkHeaderSet::LinkHeaderSet(const String& header) {
  if (header.IsNull())
    return;

  DCHECK(header.Is8Bit()) << "Headers should always be 8 bit";
  std::string header_string(reinterpret_cast<const char*>(header.Characters8()),
                            header.length());
  for (const auto& value : link_header_util::SplitLinkHeader(header_string))
    header_set_.push_back(LinkHeader(value.first, value.second));
}

}  // namespace blink
