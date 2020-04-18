// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_MEDIA_ROUTER_CAST_DIALOG_MODEL_H_
#define CHROME_BROWSER_UI_MEDIA_ROUTER_CAST_DIALOG_MODEL_H_

#include "base/strings/string16.h"
#include "chrome/browser/ui/media_router/ui_media_sink.h"

namespace media_router {

// Holds data needed to populate a Cast dialog.
struct CastDialogModel {
 public:
  CastDialogModel();
  CastDialogModel(const CastDialogModel& other);
  ~CastDialogModel();

  // The tab ID that the dialog is associated with.
  // TODO(takumif): Set |tab_id| in the ctor.
  int tab_id = -1;

  // The header to use at the top of the dialog.
  // This reflects the current activity associated with the tab.
  base::string16 dialog_header;

  // Sink data in the order they should be shown in the dialog.
  std::vector<UIMediaSink> media_sinks;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_MEDIA_ROUTER_CAST_DIALOG_MODEL_H_
