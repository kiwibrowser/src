// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_LAYERED_API_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_LAYERED_API_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// Implements Layered API.
// Spec: https://github.com/drufball/layered-apis/blob/master/spec.md
// Implementation Design Doc:
// https://docs.google.com/document/d/1V-WaCZQbBcQJRSYSYBb8Y6p0DOdDpiNDSmD41ui_73s/edit
namespace layered_api {

// <spec
// href="https://github.com/drufball/layered-apis/blob/master/spec.md#user-content-layered-api-fetching-url">
// This operation maps URLs of the form std:x|y to either std:x or y
// URLs.</spec>
CORE_EXPORT KURL ResolveFetchingURL(const KURL&, const KURL& base_url);

// Returns std-internal://x/index.js if the URL is Layered API, or null URL
// otherwise (not specced).
CORE_EXPORT KURL GetInternalURL(const KURL&);

// Gets source text for std-internal://x/index.js.
CORE_EXPORT String GetSourceText(const KURL&);

}  // namespace layered_api

}  // namespace blink

#endif
