// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/mime_util/mime_util.h"

#include <stddef.h>

#include "base/containers/hash_tables.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "net/base/mime_util.h"

#if !defined(OS_IOS)
// iOS doesn't use and must not depend on //media
#include "media/base/mime_util.h"
#endif

namespace blink {

namespace {

// From WebKit's WebCore/platform/MIMETypeRegistry.cpp:

const char* const kSupportedImageTypes[] = {"image/jpeg",
                                            "image/pjpeg",
                                            "image/jpg",
                                            "image/webp",
                                            "image/png",
                                            "image/apng",
                                            "image/gif",
                                            "image/bmp",
                                            "image/vnd.microsoft.icon",  // ico
                                            "image/x-icon",              // ico
                                            "image/x-xbitmap",           // xbm
                                            "image/x-png"};

//  Support every script type mentioned in the spec, as it notes that "User
//  agents must recognize all JavaScript MIME types." See
//  https://html.spec.whatwg.org/#javascript-mime-type.
const char* const kSupportedJavascriptTypes[] = {
    "application/ecmascript",
    "application/javascript",
    "application/x-ecmascript",
    "application/x-javascript",
    "text/ecmascript",
    "text/javascript",
    "text/javascript1.0",
    "text/javascript1.1",
    "text/javascript1.2",
    "text/javascript1.3",
    "text/javascript1.4",
    "text/javascript1.5",
    "text/jscript",
    "text/livescript",
    "text/x-ecmascript",
    "text/x-javascript",
};

// These types are excluded from the logic that allows all text/ types because
// while they are technically text, it's very unlikely that a user expects to
// see them rendered in text form.
static const char* const kUnsupportedTextTypes[] = {
    "text/calendar",
    "text/x-calendar",
    "text/x-vcalendar",
    "text/vcalendar",
    "text/vcard",
    "text/x-vcard",
    "text/directory",
    "text/ldif",
    "text/qif",
    "text/x-qif",
    "text/x-csv",
    "text/x-vcf",
    "text/rtf",
    "text/comma-separated-values",
    "text/csv",
    "text/tab-separated-values",
    "text/tsv",
    "text/ofx",                         // http://crbug.com/162238
    "text/vnd.sun.j2me.app-descriptor"  // http://crbug.com/176450
};

// Note:
// - does not include javascript types list (see supported_javascript_types)
// - does not include types starting with "text/" (see
//   IsSupportedNonImageMimeType())
static const char* const kSupportedNonImageTypes[] = {
    "image/svg+xml",  // SVG is text-based XML, even though it has an image/
                      // type
    "application/xml", "application/atom+xml", "application/rss+xml",
    "application/xhtml+xml", "application/json",
    "message/rfc822",     // For MHTML support.
    "multipart/related",  // For MHTML support.
    "multipart/x-mixed-replace"
    // Note: ADDING a new type here will probably render it AS HTML. This can
    // result in cross site scripting.
};

// Singleton utility class for mime types
class MimeUtil {
 public:
  bool IsSupportedImageMimeType(const std::string& mime_type) const;
  bool IsSupportedNonImageMimeType(const std::string& mime_type) const;
  bool IsUnsupportedTextMimeType(const std::string& mime_type) const;
  bool IsSupportedJavascriptMimeType(const std::string& mime_type) const;

  bool IsSupportedMimeType(const std::string& mime_type) const;

 private:
  friend struct base::LazyInstanceTraitsBase<MimeUtil>;

  using MimeTypes = base::hash_set<std::string>;

  MimeUtil();

  MimeTypes image_types_;
  MimeTypes non_image_types_;
  MimeTypes unsupported_text_types_;
  MimeTypes javascript_types_;

  DISALLOW_COPY_AND_ASSIGN(MimeUtil);
};

MimeUtil::MimeUtil() {
  for (size_t i = 0; i < arraysize(kSupportedNonImageTypes); ++i)
    non_image_types_.insert(kSupportedNonImageTypes[i]);
  for (size_t i = 0; i < arraysize(kSupportedImageTypes); ++i)
    image_types_.insert(kSupportedImageTypes[i]);
  for (size_t i = 0; i < arraysize(kUnsupportedTextTypes); ++i)
    unsupported_text_types_.insert(kUnsupportedTextTypes[i]);
  for (size_t i = 0; i < arraysize(kSupportedJavascriptTypes); ++i) {
    javascript_types_.insert(kSupportedJavascriptTypes[i]);
    non_image_types_.insert(kSupportedJavascriptTypes[i]);
  }
}

bool MimeUtil::IsSupportedImageMimeType(const std::string& mime_type) const {
  return image_types_.find(base::ToLowerASCII(mime_type)) != image_types_.end();
}

bool MimeUtil::IsSupportedNonImageMimeType(const std::string& mime_type) const {
  return non_image_types_.find(base::ToLowerASCII(mime_type)) !=
             non_image_types_.end() ||
#if !defined(OS_IOS)
         media::IsSupportedMediaMimeType(mime_type) ||
#endif
         (base::StartsWith(mime_type, "text/",
                           base::CompareCase::INSENSITIVE_ASCII) &&
          !IsUnsupportedTextMimeType(mime_type)) ||
         (base::StartsWith(mime_type, "application/",
                           base::CompareCase::INSENSITIVE_ASCII) &&
          net::MatchesMimeType("application/*+json", mime_type));
}

bool MimeUtil::IsUnsupportedTextMimeType(const std::string& mime_type) const {
  return unsupported_text_types_.find(base::ToLowerASCII(mime_type)) !=
         unsupported_text_types_.end();
}

bool MimeUtil::IsSupportedJavascriptMimeType(
    const std::string& mime_type) const {
  return javascript_types_.find(mime_type) != javascript_types_.end();
}

bool MimeUtil::IsSupportedMimeType(const std::string& mime_type) const {
  return (base::StartsWith(mime_type, "image/",
                           base::CompareCase::INSENSITIVE_ASCII) &&
          IsSupportedImageMimeType(mime_type)) ||
         IsSupportedNonImageMimeType(mime_type);
}

// This variable is Leaky because it is accessed from WorkerPool threads.
static base::LazyInstance<MimeUtil>::Leaky g_mime_util =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

bool IsSupportedImageMimeType(const std::string& mime_type) {
  return g_mime_util.Get().IsSupportedImageMimeType(mime_type);
}

bool IsSupportedNonImageMimeType(const std::string& mime_type) {
  return g_mime_util.Get().IsSupportedNonImageMimeType(mime_type);
}

bool IsUnsupportedTextMimeType(const std::string& mime_type) {
  return g_mime_util.Get().IsUnsupportedTextMimeType(mime_type);
}

bool IsSupportedJavascriptMimeType(const std::string& mime_type) {
  return g_mime_util.Get().IsSupportedJavascriptMimeType(mime_type);
}

bool IsSupportedMimeType(const std::string& mime_type) {
  return g_mime_util.Get().IsSupportedMimeType(mime_type);
}

}  // namespace blink
