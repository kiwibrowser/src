// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/loader/ftp_directory_listing_response_delegate.h"

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/i18n/encoding_detection.h"
#include "base/i18n/icu_string_conversions.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "content/renderer/loader/weburlresponse_extradata_impl.h"
#include "net/base/directory_listing.h"
#include "net/base/escape.h"
#include "net/base/net_errors.h"
#include "net/ftp/ftp_directory_listing_parser.h"
#include "net/net_buildflags.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_loader_client.h"

using blink::WebURLLoader;
using blink::WebURLLoaderClient;
using blink::WebURLResponse;
using net::FtpDirectoryListingEntry;

namespace {

base::string16 ConvertPathToUTF16(const std::string& path) {
  // Per RFC 2640, FTP servers should use UTF-8 or its proper subset ASCII,
  // but many old FTP servers use legacy encodings. Try UTF-8 first.
  if (base::IsStringUTF8(path))
    return base::UTF8ToUTF16(path);

  // Try detecting the encoding. The sample is rather small though, so it may
  // fail.
  std::string encoding;
  if (base::DetectEncoding(path, &encoding) && encoding != "US-ASCII") {
    base::string16 path_utf16;
    if (base::CodepageToUTF16(path, encoding.c_str(),
                              base::OnStringConversionError::SUBSTITUTE,
                              &path_utf16)) {
      return path_utf16;
    }
  }

  // Use system native encoding as the last resort.
  return base::WideToUTF16(base::SysNativeMBToWide(path));
}

}  // namespace

namespace content {

FtpDirectoryListingResponseDelegate::FtpDirectoryListingResponseDelegate(
    WebURLLoaderClient* client,
    WebURLLoader* loader,
    const WebURLResponse& response)
    : client_(client),
      loader_(loader) {
  if (response.GetExtraData()) {
    // |extra_data| can be NULL during tests.
    WebURLResponseExtraDataImpl* extra_data =
        static_cast<WebURLResponseExtraDataImpl*>(response.GetExtraData());
    extra_data->set_is_ftp_directory_listing(true);
  }
  Init(response.Url());
}

void FtpDirectoryListingResponseDelegate::Cancel() {
  client_ = nullptr;
  loader_ = nullptr;
}

void FtpDirectoryListingResponseDelegate::OnReceivedData(const char* data,
                                                         int data_len) {
  buffer_.append(data, data_len);
}

void FtpDirectoryListingResponseDelegate::OnCompletedRequest() {
  std::vector<FtpDirectoryListingEntry> entries;
  int rv = net::ERR_NOT_IMPLEMENTED;
#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
  rv = net::ParseFtpDirectoryListing(buffer_, base::Time::Now(), &entries);
#endif
  if (rv != net::OK) {
    SendDataToClient("<script>onListingParsingError();</script>\n");
    return;
  }
  for (const FtpDirectoryListingEntry& entry : entries) {
    // Skip the current and parent directory entries in the listing.
    // net::GetParentDirectoryLink() takes care of them.
    if (base::EqualsASCII(entry.name, ".") ||
        base::EqualsASCII(entry.name, ".."))
      continue;

    bool is_directory = (entry.type == FtpDirectoryListingEntry::DIRECTORY);
    int64_t size =
        entry.type == FtpDirectoryListingEntry::FILE ? entry.size : 0;
    SendDataToClient(net::GetDirectoryListingEntry(
        entry.name, entry.raw_name, is_directory, size, entry.last_modified));
  }
}

void FtpDirectoryListingResponseDelegate::Init(const GURL& response_url) {
  net::UnescapeRule::Type unescape_rules =
      net::UnescapeRule::SPACES |
      net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS;
  std::string unescaped_path = net::UnescapeURLComponent(response_url.path(),
                                                         unescape_rules);
  SendDataToClient(net::GetDirectoryListingHeader(
      ConvertPathToUTF16(unescaped_path)));

  // If this isn't top level directory (i.e. the path isn't "/",)
  // add a link to the parent directory.
  if (response_url.path().length() > 1) {
    SendDataToClient(net::GetParentDirectoryLink());
  }
}

void FtpDirectoryListingResponseDelegate::SendDataToClient(
    const std::string& data) {
  if (client_) {
    client_->DidReceiveData(data.data(), data.length());
  }
}

}  // namespace content
