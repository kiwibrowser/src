// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/open_from_clipboard/clipboard_recent_content_generic.h"

#include "base/strings/string_util.h"
#include "ui/base/clipboard/clipboard.h"

namespace {
// Schemes appropriate for suggestion by ClipboardRecentContent.
const char* kAuthorizedSchemes[] = {
    url::kAboutScheme, url::kDataScheme, url::kHttpScheme, url::kHttpsScheme,
    // TODO(mpearson): add support for chrome:// URLs.  Right now the scheme
    // for that lives in content and is accessible via
    // GetEmbedderRepresentationOfAboutScheme() or content::kChromeUIScheme
    // TODO(mpearson): when adding desktop support, add kFileScheme, kFtpScheme,
    // and kGopherScheme.
};

}  // namespace

ClipboardRecentContentGeneric::ClipboardRecentContentGeneric() {}

bool ClipboardRecentContentGeneric::GetRecentURLFromClipboard(GURL* url) {
  if (GetClipboardContentAge() > MaximumAgeOfClipboard())
    return false;

  // Get and clean up the clipboard before processing.
  std::string gurl_string;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadAsciiText(ui::CLIPBOARD_TYPE_COPY_PASTE, &gurl_string);
  base::TrimWhitespaceASCII(gurl_string, base::TrimPositions::TRIM_ALL,
                            &gurl_string);

  // Interpret the clipboard as a URL if possible.
  DCHECK(url);
  // If there is mid-string whitespace, don't attempt to interpret the string
  // as a URL.  (Otherwise gurl will happily try to convert
  // "http://example.com extra words" into "http://example.com%20extra%20words",
  // which is not likely to be a useful or intended destination.)
  if (gurl_string.find_first_of(base::kWhitespaceASCII) != std::string::npos)
    return false;
  if (!gurl_string.empty()) {
    *url = GURL(gurl_string);
  } else {
    // Fall back to unicode / UTF16, as some URLs may use international domain
    // names, not punycode.
    base::string16 gurl_string16;
    clipboard->ReadText(ui::CLIPBOARD_TYPE_COPY_PASTE, &gurl_string16);
    base::TrimWhitespace(gurl_string16, base::TrimPositions::TRIM_ALL,
                         &gurl_string16);
    if (gurl_string16.find_first_of(base::kWhitespaceUTF16) !=
        std::string::npos)
      return false;
    if (!gurl_string16.empty())
      *url = GURL(gurl_string16);
  }
  return url->is_valid() && IsAppropriateSuggestion(*url);
}

base::TimeDelta ClipboardRecentContentGeneric::GetClipboardContentAge() const {
  const base::Time last_modified_time =
      ui::Clipboard::GetForCurrentThread()->GetLastModifiedTime();
  const base::Time now = base::Time::Now();
  // In case of system clock change, assume the last modified time is now.
  // (Don't return a negative age, i.e., a time in the future.)
  if (last_modified_time > now)
    return base::TimeDelta();
  return now - last_modified_time;
}

void ClipboardRecentContentGeneric::SuppressClipboardContent() {
  // User cleared the user data.  The pasteboard entry must be removed from the
  // omnibox list.  Do this by pretending the current clipboard is ancient,
  // not recent.
  ui::Clipboard::GetForCurrentThread()->ClearLastModifiedTime();
}

// static
bool ClipboardRecentContentGeneric::IsAppropriateSuggestion(const GURL& url) {
  // Check to make sure it's a scheme we're willing to suggest.
  for (const auto* authorized_scheme : kAuthorizedSchemes) {
    if (url.SchemeIs(authorized_scheme))
      return true;
  }

  // Not a scheme we're allowed to return.
  return false;
}
