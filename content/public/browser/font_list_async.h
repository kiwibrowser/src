// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_FONT_LIST_ASYNC_H_
#define CONTENT_PUBLIC_BROWSER_FONT_LIST_ASYNC_H_

#include <memory>

#include "base/callback.h"
#include "content/common/content_export.h"

namespace base {
class ListValue;
}

namespace content {

// Retrieves the list of fonts on the system as a list of strings. It provides
// a non-blocking interface to GetFontList_SlowBlocking in common/.
//
// This function will run asynchronously on a background thread since getting
// the font list from the system can be slow. The callback will be executed on
// the calling sequence.
CONTENT_EXPORT void GetFontListAsync(
    base::OnceCallback<void(std::unique_ptr<base::ListValue>)> callback);

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_FONT_LIST_ASYNC_H_
