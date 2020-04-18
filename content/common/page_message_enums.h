// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_PAGE_MESSAGE_ENUMS_H_
#define CONTENT_COMMON_PAGE_MESSAGE_ENUMS_H_

enum class PageMsg_SetZoomLevel_Command {
  SET_TEMPORARY,
  CLEAR_TEMPORARY,
  USE_CURRENT_TEMPORARY_MODE,
  LAST = USE_CURRENT_TEMPORARY_MODE
};

#endif  // CONTENT_COMMON_PAGE_MESSAGE_ENUMS_H_
