// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/request_header/offline_page_header.h"

#include "base/base64.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"

namespace offline_pages {

const char kOfflinePageHeader[] = "X-Chrome-offline";
const char kOfflinePageHeaderReasonKey[] = "reason";
const char kOfflinePageHeaderReasonValueDueToNetError[] = "error";
const char kOfflinePageHeaderReasonValueFromDownload[] = "download";
const char kOfflinePageHeaderReasonValueReload[] = "reload";
const char kOfflinePageHeaderReasonValueFromNotification[] = "notification";
const char kOfflinePageHeaderReasonFileUrlIntent[] = "file_url_intent";
const char kOfflinePageHeaderReasonContentUrlIntent[] = "content_url_intent";
const char kOfflinePageHeaderPersistKey[] = "persist";
const char kOfflinePageHeaderIDKey[] = "id";
const char kOfflinePageHeaderIntentUrlKey[] = "intent_url";

namespace {

bool ParseOfflineHeaderValue(const std::string& header_value,
                             bool* need_to_persist,
                             OfflinePageHeader::Reason* reason,
                             std::string* id,
                             GURL* intent_url) {
  // If the offline header is not present, treat it as not parsed successfully.
  if (header_value.empty())
    return false;

  bool token_found = false;
  base::StringTokenizer tokenizer(header_value, ", ");
  while (tokenizer.GetNext()) {
    token_found = true;
    std::string pair = tokenizer.token();
    std::size_t pos = pair.find('=');
    if (pos == std::string::npos)
      return false;
    std::string key = base::ToLowerASCII(pair.substr(0, pos));
    std::string value = pair.substr(pos + 1);
    std::string lower_value = base::ToLowerASCII(value);
    if (key == kOfflinePageHeaderPersistKey) {
      if (lower_value == "1")
        *need_to_persist = true;
      else if (lower_value == "0")
        *need_to_persist = false;
      else
        return false;
    } else if (key == kOfflinePageHeaderReasonKey) {
      if (lower_value == kOfflinePageHeaderReasonValueDueToNetError)
        *reason = OfflinePageHeader::Reason::NET_ERROR;
      else if (lower_value == kOfflinePageHeaderReasonValueFromDownload)
        *reason = OfflinePageHeader::Reason::DOWNLOAD;
      else if (lower_value == kOfflinePageHeaderReasonValueReload)
        *reason = OfflinePageHeader::Reason::RELOAD;
      else if (lower_value == kOfflinePageHeaderReasonValueFromNotification)
        *reason = OfflinePageHeader::Reason::NOTIFICATION;
      else if (lower_value == kOfflinePageHeaderReasonFileUrlIntent)
        *reason = OfflinePageHeader::Reason::FILE_URL_INTENT;
      else if (lower_value == kOfflinePageHeaderReasonContentUrlIntent)
        *reason = OfflinePageHeader::Reason::CONTENT_URL_INTENT;
      else
        return false;
    } else if (key == kOfflinePageHeaderIDKey) {
      *id = value;
    } else if (key == kOfflinePageHeaderIntentUrlKey) {
      std::string decoded_url;
      if (!base::Base64Decode(value, &decoded_url))
        return false;
      GURL url = GURL(decoded_url);
      if (!url.is_valid())
        return false;
      *intent_url = url;
    } else {
      return false;
    }
  }

  return token_found;
}

std::string ReasonToString(OfflinePageHeader::Reason reason) {
  switch (reason) {
    case OfflinePageHeader::Reason::NET_ERROR:
      return kOfflinePageHeaderReasonValueDueToNetError;
    case OfflinePageHeader::Reason::DOWNLOAD:
      return kOfflinePageHeaderReasonValueFromDownload;
    case OfflinePageHeader::Reason::RELOAD:
      return kOfflinePageHeaderReasonValueReload;
    case OfflinePageHeader::Reason::NOTIFICATION:
      return kOfflinePageHeaderReasonValueFromNotification;
    case OfflinePageHeader::Reason::FILE_URL_INTENT:
      return kOfflinePageHeaderReasonFileUrlIntent;
    case OfflinePageHeader::Reason::CONTENT_URL_INTENT:
      return kOfflinePageHeaderReasonContentUrlIntent;
    default:
      NOTREACHED();
      return "";
  }
}

}  // namespace

OfflinePageHeader::OfflinePageHeader()
    : did_fail_parsing_for_test(false),
      need_to_persist(false),
      reason(Reason::NONE) {}

OfflinePageHeader::OfflinePageHeader(const std::string& header_value)
    : did_fail_parsing_for_test(false),
      need_to_persist(false),
      reason(Reason::NONE) {
  if (!ParseOfflineHeaderValue(header_value, &need_to_persist, &reason, &id,
                               &intent_url)) {
    did_fail_parsing_for_test = true;
    Clear();
  }
}

OfflinePageHeader::~OfflinePageHeader() {}

std::string OfflinePageHeader::GetCompleteHeaderString() const {
  if (reason == Reason::NONE)
    return std::string();

  std::string value(kOfflinePageHeader);
  value += ": ";

  value += kOfflinePageHeaderPersistKey;
  value += "=";
  value += need_to_persist ? "1" : "0";

  value += " ";
  value += kOfflinePageHeaderReasonKey;
  value += "=";
  value += ReasonToString(reason);

  if (!id.empty()) {
    value += " ";
    value += kOfflinePageHeaderIDKey;
    value += "=";
    value += id;
  }

  // Base64-encode the intent URL value because unlike http/https URLs, the
  // content:// URL can include any arbitrary unescaped characters in its path,
  // i.e., derived from filename or title which may contain SPACE, QUOTE,
  // BACKSLASH or other unsafe characters.
  if (!intent_url.is_empty()) {
    value += " ";
    value += kOfflinePageHeaderIntentUrlKey;
    value += "=";
    std::string encoded_intent_url;
    base::Base64Encode(intent_url.spec(), &encoded_intent_url);
    value += encoded_intent_url;
  }

  return value;
}

void OfflinePageHeader::Clear() {
  reason = Reason::NONE;
  need_to_persist = false;
  id.clear();
}

}  // namespace offline_pages
