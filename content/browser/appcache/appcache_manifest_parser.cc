// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This is a port of ManifestParser.cc from WebKit/WebCore/loader/appcache.

/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "content/browser/appcache/appcache_manifest_parser.h"

#include <stddef.h>

#include "base/command_line.h"
#include "base/i18n/icu_string_conversions.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "url/gurl.h"

namespace content {

namespace {

// Helper function used to identify 'isPattern' annotations.
bool HasPatternMatchingAnnotation(const wchar_t* line_p,
                                  const wchar_t* line_end) {
  // Skip whitespace separating the resource url from the annotation.
  // Note: trailing whitespace has already been trimmed from the line.
  while (line_p < line_end && (*line_p == '\t' || *line_p == ' '))
    ++line_p;
  if (line_p == line_end)
    return false;
  std::wstring annotation(line_p, line_end - line_p);
  return annotation == L"isPattern";
}

bool ScopeMatches(const GURL& manifest_url, const GURL& namespace_url) {
  return base::StartsWith(namespace_url.spec(),
                          manifest_url.GetWithoutFilename().spec(),
                          base::CompareCase::SENSITIVE);
}

}  // namespace

enum Mode {
  EXPLICIT,
  INTERCEPT,
  FALLBACK,
  ONLINE_WHITELIST,
  UNKNOWN_MODE,
};

enum InterceptVerb {
  RETURN,
  EXECUTE,
  UNKNOWN_VERB,
};

AppCacheManifest::AppCacheManifest() {}

AppCacheManifest::~AppCacheManifest() {}

bool ParseManifest(const GURL& manifest_url, const char* data, int length,
                   ParseMode parse_mode, AppCacheManifest& manifest) {
  // This is an implementation of the parsing algorithm specified in
  // the HTML5 offline web application docs:
  //   http://www.w3.org/TR/html5/offline.html
  // Do not modify it without consulting those docs.
  // Though you might be tempted to convert these wstrings to UTF-8 or
  // base::string16, this implementation seems simpler given the constraints.

  const wchar_t kSignature[] = L"CACHE MANIFEST";
  const size_t kSignatureLength = arraysize(kSignature) - 1;
  const wchar_t kChromiumSignature[] = L"CHROMIUM CACHE MANIFEST";
  const size_t kChromiumSignatureLength = arraysize(kChromiumSignature) - 1;

  DCHECK(manifest.explicit_urls.empty());
  DCHECK(manifest.fallback_namespaces.empty());
  DCHECK(manifest.online_whitelist_namespaces.empty());
  DCHECK(!manifest.online_whitelist_all);
  DCHECK(!manifest.did_ignore_intercept_namespaces);
  DCHECK(!manifest.did_ignore_fallback_namespaces);

  Mode mode = EXPLICIT;

  std::wstring data_string;
  base::UTF8ToWide(data, length, &data_string);
  const wchar_t* p = data_string.c_str();
  const wchar_t* end = p + data_string.length();

  // Look for the magic signature: "^\xFEFF?CACHE MANIFEST[ \t]?"
  // Example: "CACHE MANIFEST #comment" is a valid signature.
  // Example: "CACHE MANIFEST;V2" is not.

  // When the input data starts with a UTF-8 Byte-Order-Mark
  // (0xEF, 0xBB, 0xBF), the UTF8ToWide() function converts it to a
  // Unicode BOM (U+FEFF). Skip a converted Unicode BOM if it exists.
  int bom_offset = 0;
  if (!data_string.empty() && data_string[0] == 0xFEFF) {
    bom_offset = 1;
    ++p;
  }

  if (p >= end)
    return false;

  // Check for a supported signature and skip p past it.
  if (0 == data_string.compare(bom_offset, kSignatureLength,
                               kSignature)) {
    p += kSignatureLength;
  } else if (0 == data_string.compare(bom_offset, kChromiumSignatureLength,
                                      kChromiumSignature)) {
    p += kChromiumSignatureLength;
  } else {
    return false;
  }

  // Character after "CACHE MANIFEST" must be whitespace.
  if (p < end && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
    return false;

  // Skip to the end of the line.
  while (p < end && *p != '\r' && *p != '\n')
    ++p;

  while (1) {
    // Skip whitespace
    while (p < end && (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t'))
      ++p;

    if (p == end)
      break;

    const wchar_t* line_start = p;

    // Find the end of the line
    while (p < end && *p != '\r' && *p != '\n')
      ++p;

    // Check if we have a comment
    if (*line_start == '#')
      continue;

    // Get rid of trailing whitespace
    const wchar_t* tmp = p - 1;
    while (tmp > line_start && (*tmp == ' ' || *tmp == '\t'))
      --tmp;

    std::wstring line(line_start, tmp - line_start + 1);

    if (line == L"CACHE:") {
      mode = EXPLICIT;
    } else if (line == L"FALLBACK:") {
      mode = FALLBACK;
    } else if (line == L"NETWORK:") {
      mode = ONLINE_WHITELIST;
    } else if (line == L"CHROMIUM-INTERCEPT:") {
      mode = INTERCEPT;
    } else if (*(line.end() - 1) == ':') {
      mode = UNKNOWN_MODE;
    } else if (mode == UNKNOWN_MODE) {
      continue;
    } else if (line == L"*" && mode == ONLINE_WHITELIST) {
      manifest.online_whitelist_all = true;
      continue;
    } else if (mode == EXPLICIT || mode == ONLINE_WHITELIST) {
      const wchar_t *line_p = line.c_str();
      const wchar_t *line_end = line_p + line.length();

      // Look for whitespace separating the URL from subsequent ignored tokens.
      while (line_p < line_end && *line_p != '\t' && *line_p != ' ')
        ++line_p;

      base::string16 url16;
      base::WideToUTF16(line.c_str(), line_p - line.c_str(), &url16);
      GURL url = manifest_url.Resolve(url16);
      if (!url.is_valid())
        continue;
      if (url.has_ref()) {
        GURL::Replacements replacements;
        replacements.ClearRef();
        url = url.ReplaceComponents(replacements);
      }

      // Scheme component must be the same as the manifest URL's.
      if (url.scheme() != manifest_url.scheme()) {
        continue;
      }

      // See http://code.google.com/p/chromium/issues/detail?id=69594
      // We willfully violate the HTML5 spec at this point in order
      // to support the appcaching of cross-origin HTTPS resources.
      // Per the spec, EXPLICIT cross-origin HTTS resources should be
      // ignored here. We've opted for a milder constraint and allow
      // caching unless the resource has a "no-store" header. That
      // condition is enforced in AppCacheUpdateJob.

      if (mode == EXPLICIT) {
        manifest.explicit_urls.insert(url.spec());
      } else {
        bool is_pattern = HasPatternMatchingAnnotation(line_p, line_end);
        manifest.online_whitelist_namespaces.push_back(
            AppCacheNamespace(APPCACHE_NETWORK_NAMESPACE, url, GURL(),
                is_pattern));
      }
    } else if (mode == INTERCEPT) {
      if (parse_mode != PARSE_MANIFEST_ALLOWING_DANGEROUS_FEATURES) {
        manifest.did_ignore_intercept_namespaces = true;
        continue;
      }

      // Lines of the form,
      // <urlnamespace> <intercept_type> <targeturl>
      const wchar_t* line_p = line.c_str();
      const wchar_t* line_end = line_p + line.length();

      // Look for first whitespace separating the url namespace from
      // the intercept type.
      while (line_p < line_end && *line_p != '\t' && *line_p != ' ')
        ++line_p;

      if (line_p == line_end)
        continue;  // There was no whitespace separating the URLs.

      base::string16 namespace_url16;
      base::WideToUTF16(line.c_str(), line_p - line.c_str(), &namespace_url16);
      GURL namespace_url = manifest_url.Resolve(namespace_url16);
      if (!namespace_url.is_valid())
        continue;
      if (namespace_url.has_ref()) {
        GURL::Replacements replacements;
        replacements.ClearRef();
        namespace_url = namespace_url.ReplaceComponents(replacements);
      }

      // The namespace URL must have the same scheme, host and port
      // as the manifest's URL.
      if (manifest_url.GetOrigin() != namespace_url.GetOrigin())
        continue;

      // Skip whitespace separating namespace from the type.
      while (line_p < line_end && (*line_p == '\t' || *line_p == ' '))
        ++line_p;

      // Look for whitespace separating the type from the target url.
      const wchar_t* type_start = line_p;
      while (line_p < line_end && *line_p != '\t' && *line_p != ' ')
        ++line_p;

      // Look for a type value we understand, otherwise skip the line.
      std::wstring type(type_start, line_p - type_start);
      if (type != L"return")
        continue;

      // Skip whitespace separating type from the target_url.
      while (line_p < line_end && (*line_p == '\t' || *line_p == ' '))
        ++line_p;

      // Look for whitespace separating the URL from subsequent ignored tokens.
      const wchar_t* target_url_start = line_p;
      while (line_p < line_end && *line_p != '\t' && *line_p != ' ')
        ++line_p;

      base::string16 target_url16;
      base::WideToUTF16(target_url_start, line_p - target_url_start,
                        &target_url16);
      GURL target_url = manifest_url.Resolve(target_url16);
      if (!target_url.is_valid())
        continue;

      if (target_url.has_ref()) {
        GURL::Replacements replacements;
        replacements.ClearRef();
        target_url = target_url.ReplaceComponents(replacements);
      }
      if (manifest_url.GetOrigin() != target_url.GetOrigin())
        continue;

      bool is_pattern = HasPatternMatchingAnnotation(line_p, line_end);
      manifest.intercept_namespaces.push_back(AppCacheNamespace(
          APPCACHE_INTERCEPT_NAMESPACE, namespace_url, target_url, is_pattern));
    } else if (mode == FALLBACK) {
      const wchar_t* line_p = line.c_str();
      const wchar_t* line_end = line_p + line.length();

      // Look for whitespace separating the two URLs
      while (line_p < line_end && *line_p != '\t' && *line_p != ' ')
        ++line_p;

      if (line_p == line_end) {
        // There was no whitespace separating the URLs.
        continue;
      }

      base::string16 namespace_url16;
      base::WideToUTF16(line.c_str(), line_p - line.c_str(), &namespace_url16);
      GURL namespace_url = manifest_url.Resolve(namespace_url16);
      if (!namespace_url.is_valid())
        continue;
      if (namespace_url.has_ref()) {
        GURL::Replacements replacements;
        replacements.ClearRef();
        namespace_url = namespace_url.ReplaceComponents(replacements);
      }

      // Fallback namespace URL must have the same scheme, host and port
      // as the manifest's URL.
      if (manifest_url.GetOrigin() != namespace_url.GetOrigin()) {
        continue;
      }

      if (parse_mode != PARSE_MANIFEST_ALLOWING_DANGEROUS_FEATURES) {
        if (!ScopeMatches(manifest_url, namespace_url)) {
          manifest.did_ignore_fallback_namespaces = true;
          continue;
        }
      }

      // Skip whitespace separating fallback namespace from URL.
      while (line_p < line_end && (*line_p == '\t' || *line_p == ' '))
        ++line_p;

      // Look for whitespace separating the URL from subsequent ignored tokens.
      const wchar_t* fallback_start = line_p;
      while (line_p < line_end && *line_p != '\t' && *line_p != ' ')
        ++line_p;

      base::string16 fallback_url16;
      base::WideToUTF16(fallback_start, line_p - fallback_start,
                        &fallback_url16);
      GURL fallback_url = manifest_url.Resolve(fallback_url16);
      if (!fallback_url.is_valid())
        continue;
      if (fallback_url.has_ref()) {
        GURL::Replacements replacements;
        replacements.ClearRef();
        fallback_url = fallback_url.ReplaceComponents(replacements);
      }

      // Fallback entry URL must have the same scheme, host and port
      // as the manifest's URL.
      if (manifest_url.GetOrigin() != fallback_url.GetOrigin()) {
        continue;
      }

      bool is_pattern = HasPatternMatchingAnnotation(line_p, line_end);

      // Store regardless of duplicate namespace URL. Only first match
      // will ever be used.
      manifest.fallback_namespaces.push_back(
          AppCacheNamespace(APPCACHE_FALLBACK_NAMESPACE, namespace_url,
                    fallback_url, is_pattern));
    } else {
      NOTREACHED();
    }
  }

  return true;
}

}  // namespace content
