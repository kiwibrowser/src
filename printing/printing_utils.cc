// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/printing_utils.h"

#include <stddef.h>

#include <algorithm>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/icu/source/common/unicode/uchar.h"
#include "ui/gfx/text_elider.h"

namespace printing {

namespace {

const size_t kMaxDocumentTitleLength = 80;

}  // namespace

base::string16 SimplifyDocumentTitleWithLength(const base::string16& title,
                                               size_t length) {
  base::string16 no_controls(title);
  no_controls.erase(
      std::remove_if(no_controls.begin(), no_controls.end(), &u_iscntrl),
      no_controls.end());
  base::ReplaceChars(no_controls, base::ASCIIToUTF16("\\"),
                     base::ASCIIToUTF16("_"), &no_controls);
  base::string16 result;
  gfx::ElideString(no_controls, length, &result);
  return result;
}

base::string16 FormatDocumentTitleWithOwnerAndLength(
    const base::string16& owner,
    const base::string16& title,
    size_t length) {
  const base::string16 separator = base::ASCIIToUTF16(": ");
  DCHECK_LT(separator.size(), length);

  base::string16 short_title =
      SimplifyDocumentTitleWithLength(owner, length - separator.size());
  short_title += separator;
  if (short_title.size() < length) {
    short_title +=
        SimplifyDocumentTitleWithLength(title, length - short_title.size());
  }

  return short_title;
}

base::string16 SimplifyDocumentTitle(const base::string16& title) {
  return SimplifyDocumentTitleWithLength(title, kMaxDocumentTitleLength);
}

base::string16 FormatDocumentTitleWithOwner(const base::string16& owner,
                                            const base::string16& title) {
  return FormatDocumentTitleWithOwnerAndLength(owner, title,
                                               kMaxDocumentTitleLength);
}

}  // namespace printing
