// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This is a port of ManifestParser.h from WebKit/WebCore/loader/appcache.

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

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_MANIFEST_PARSER_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_MANIFEST_PARSER_H_

#include <string>
#include <vector>

#include "base/containers/hash_tables.h"
#include "content/common/appcache_interfaces.h"
#include "content/common/content_export.h"

class GURL;

namespace content {

struct CONTENT_EXPORT AppCacheManifest {
  AppCacheManifest();
  ~AppCacheManifest();

  base::hash_set<std::string> explicit_urls;
  AppCacheNamespaceVector intercept_namespaces;
  AppCacheNamespaceVector fallback_namespaces;
  AppCacheNamespaceVector online_whitelist_namespaces;
  bool online_whitelist_all = false;
  bool did_ignore_intercept_namespaces = false;
  bool did_ignore_fallback_namespaces = false;
};

enum ParseMode {
  PARSE_MANIFEST_PER_STANDARD,
  PARSE_MANIFEST_ALLOWING_DANGEROUS_FEATURES
};

CONTENT_EXPORT bool ParseManifest(
    const GURL& manifest_url,
    const char* data,
    int length,
    ParseMode parse_mode,
    AppCacheManifest& manifest);

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_MANIFEST_PARSER_H_
