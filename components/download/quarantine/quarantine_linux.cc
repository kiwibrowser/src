// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/quarantine/quarantine.h"

#include <stddef.h>
#include <sys/types.h>
#include <sys/xattr.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/threading/thread_restrictions.h"
#include "components/download/quarantine/quarantine.h"
#include "components/download/quarantine/quarantine_constants_linux.h"
#include "url/gurl.h"

namespace download {

const char kSourceURLExtendedAttrName[] = "user.xdg.origin.url";
const char kReferrerURLExtendedAttrName[] = "user.xdg.referrer.url";

namespace {

bool SetExtendedFileAttribute(const char* path,
                              const char* name,
                              const char* value,
                              size_t value_size,
                              int flags) {
  base::AssertBlockingAllowed();
  int result = setxattr(path, name, value, value_size, flags);
  if (result) {
    DPLOG(ERROR) << "Could not set extended attribute " << name << " on file "
                 << path;
    return false;
  }
  return true;
}

std::string GetExtendedFileAttribute(const char* path, const char* name) {
  base::AssertBlockingAllowed();
  ssize_t len = getxattr(path, name, nullptr, 0);
  if (len <= 0)
    return std::string();

  std::vector<char> buffer(len);
  len = getxattr(path, name, buffer.data(), buffer.size());
  if (len < static_cast<ssize_t>(buffer.size()))
    return std::string();
  return std::string(buffer.begin(), buffer.end());
}

}  // namespace

QuarantineFileResult QuarantineFile(const base::FilePath& file,
                                    const GURL& source_url,
                                    const GURL& referrer_url,
                                    const std::string& client_guid) {
  DCHECK(base::PathIsWritable(file));

  bool source_succeeded =
      source_url.is_valid() &&
      SetExtendedFileAttribute(file.value().c_str(), kSourceURLExtendedAttrName,
                               source_url.spec().c_str(),
                               source_url.spec().length(), 0);

  // Referrer being empty is not considered an error. This could happen if the
  // referrer policy resulted in an empty referrer for the download request.
  bool referrer_succeeded =
      !referrer_url.is_valid() ||
      SetExtendedFileAttribute(
          file.value().c_str(), kReferrerURLExtendedAttrName,
          referrer_url.spec().c_str(), referrer_url.spec().length(), 0);
  return source_succeeded && referrer_succeeded
             ? QuarantineFileResult::OK
             : QuarantineFileResult::ANNOTATION_FAILED;
}

bool IsFileQuarantined(const base::FilePath& file,
                       const GURL& source_url,
                       const GURL& referrer_url) {
  if (!base::PathExists(file))
    return false;

  std::string url_value = GetExtendedFileAttribute(file.value().c_str(),
                                                   kSourceURLExtendedAttrName);
  if (source_url.is_empty())
    return !url_value.empty();

  if (source_url != GURL(url_value))
    return false;

  return !referrer_url.is_valid() ||
         GURL(GetExtendedFileAttribute(file.value().c_str(),
                                       kReferrerURLExtendedAttrName)) ==
             referrer_url;
}

}  // namespace download
