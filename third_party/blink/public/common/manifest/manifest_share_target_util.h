// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_MANIFEST_MANIFEST_SHARE_TARGET_UTIL_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_MANIFEST_MANIFEST_SHARE_TARGET_UTIL_H_

#include <string>

#include "base/strings/string_piece.h"
#include "third_party/blink/common/common_export.h"

class GURL;

namespace blink {

// Determines whether |url_template| is valid; that is, whether
// ReplaceWebShareUrlPlaceholders() would succeed for the given template.
BLINK_COMMON_EXPORT bool ValidateWebShareUrlTemplate(const GURL& url_template);

// Writes to |url_template_filled|, a copy of |url_template| with all
// instances of "{title}", "{text}", and "{url}" in the query and fragment
// parts of the URL replaced with |title|, |text|, and |url| respectively.
// Replaces instances of "{X}" where "X" is any string besides "title",
// "text", and "url", with an empty string, for forwards compatibility.
// Returns false, if there are badly nested placeholders.
// This includes any case in which two "{" occur before a "}", or a "}"
// occurs with no preceding "{".
BLINK_COMMON_EXPORT bool ReplaceWebShareUrlPlaceholders(
    const GURL& url_template,
    base::StringPiece title,
    base::StringPiece text,
    const GURL& share_url,
    GURL* url_template_filled);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_MANIFEST_MANIFEST_SHARE_TARGET_UTIL_H_
