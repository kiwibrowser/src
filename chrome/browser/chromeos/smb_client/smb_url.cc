// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_url.h"

#include "base/strings/string_util.h"
#include "chrome/browser/chromeos/smb_client/smb_constants.h"

namespace chromeos {
namespace smb_client {

namespace {

const char kDoubleBackslash[] = "\\\\";

// Returns true if |url| starts with "smb://" or "\\".
bool ShouldProcessUrl(const std::string& url) {
  return base::StartsWith(url, kSmbSchemePrefix,
                          base::CompareCase::INSENSITIVE_ASCII) ||
         base::StartsWith(url, kDoubleBackslash,
                          base::CompareCase::INSENSITIVE_ASCII);
}

// Adds "smb://" to the beginning of |url| if not present.
std::string AddSmbSchemeIfMissing(const std::string& url) {
  DCHECK(ShouldProcessUrl(url));

  if (base::StartsWith(url, kSmbSchemePrefix,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    return url;
  }

  return std::string(kSmbSchemePrefix) + url;
}

}  // namespace

SmbUrl::SmbUrl() = default;
SmbUrl::~SmbUrl() = default;

bool SmbUrl::InitializeWithUrl(const std::string& url) {
  // Only process |url| if it starts with "smb://" or "\\".
  if (ShouldProcessUrl(url)) {
    // Add "smb://" if |url| starts with "\\".
    url_ = GURL(AddSmbSchemeIfMissing(url));
  }

  return IsValid();
}

std::string SmbUrl::GetHost() const {
  DCHECK(IsValid());

  return url_.host();
}

const std::string& SmbUrl::ToString() const {
  DCHECK(IsValid());

  return url_.spec();
}

std::string SmbUrl::ReplaceHost(const std::string& new_host) const {
  DCHECK(IsValid());

  GURL::Replacements replace_host;
  replace_host.SetHostStr(new_host);
  return url_.ReplaceComponents(replace_host).spec();
}

bool SmbUrl::IsValid() const {
  return url_.is_valid();
}

}  // namespace smb_client
}  // namespace chromeos
