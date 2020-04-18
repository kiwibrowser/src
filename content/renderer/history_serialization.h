// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_HISTORY_SERIALIZATION_H_
#define CONTENT_RENDERER_HISTORY_SERIALIZATION_H_

#include <memory>
#include <string>

#include "content/common/content_export.h"

namespace blink {
class WebHistoryItem;
}

namespace content {
class HistoryEntry;
class PageState;

CONTENT_EXPORT PageState HistoryEntryToPageState(HistoryEntry* entry);
CONTENT_EXPORT PageState SingleHistoryItemToPageState(
    const blink::WebHistoryItem& item);
CONTENT_EXPORT std::unique_ptr<HistoryEntry> PageStateToHistoryEntry(
    const PageState& state);

}  // namespace content

#endif  // CONTENT_RENDERER_HISTORY_SERIALIZATION_H_
