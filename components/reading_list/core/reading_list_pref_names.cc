// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/reading_list/core/reading_list_pref_names.h"

namespace reading_list {
namespace prefs {

// Boolean to track if some reading list entries have never been seen on this
// device. Not synced.
const char kReadingListHasUnseenEntries[] = "reading_list.has_unseen_entries";

}  // namespace prefs
}  // namespace reading_list
