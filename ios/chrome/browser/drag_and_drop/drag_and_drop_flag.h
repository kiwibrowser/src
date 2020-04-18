// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DRAG_AND_DROP_DROP_DRAG_AND_DROP_FLAG_H_
#define IOS_CHROME_BROWSER_DRAG_AND_DROP_DROP_DRAG_AND_DROP_FLAG_H_

#include "base/feature_list.h"

extern const base::Feature kDragAndDrop;

// Returns whether drag and drop is enabled.
bool DragAndDropIsEnabled();

#endif  // IOS_CHROME_BROWSER_DRAG_AND_DROP_DROP_DRAG_AND_DROP_FLAG_H_
