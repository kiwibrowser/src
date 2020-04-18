// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_MEDIA_ROUTER_UI_MEDIA_SINK_H_
#define CHROME_BROWSER_UI_MEDIA_ROUTER_UI_MEDIA_SINK_H_

#include "base/strings/string16.h"
#include "chrome/common/media_router/media_sink.h"
#include "url/gurl.h"

namespace media_router {

enum class UIMediaSinkState {
  // Sink is available to be Cast to.
  AVAILABLE,
  // Sink is starting a new Casting activity. A sink temporarily enters this
  // state when transitioning from AVAILABLE to CONNECTED (or to ERROR_STATE).
  CONNECTING,
  // Sink has a media route.
  CONNECTED,
  // Sink is disconnected/cached (not available right now).
  UNAVAILABLE,
  // Sink is in an error state.
  ERROR_STATE
};

// A bitmask of this enum is used to indicate what actions can be performed on a
// sink.
enum class UICastAction {
  // Start Casting the presentation for the current tab or the tab itself.
  CAST_TAB = 1 << 1,
  // Start Casting the entire desktop.
  CAST_DESKTOP = 1 << 2,
  // Stop Casting.
  STOP = 1 << 3,
};

struct UIMediaSink {
 public:
  UIMediaSink();
  UIMediaSink(const UIMediaSink& other);
  ~UIMediaSink();

  // The unique ID for the media sink.
  std::string id;

  // Name that can be used by the user to identify the sink.
  base::string16 friendly_name;

  // Normally the sink status text is set from |state|. This field allows it
  // to be overridden for error states or to show route descriptions.
  base::string16 status_text;

  // Presentation URL to use when initiating a new casting activity for this
  // sink. For sites that integrate with the Presentation API, this is the
  // top frame presentation URL. Mirroring, Cast apps, and DIAL apps use a
  // non-https scheme.
  GURL presentation_url;

  // Active route ID, or empty string if none.
  std::string route_id;

  // The ID of the tab associated with the media route specified by |route_id|.
  // This is a nullopt if the route is not associated with a tab (e.g. because
  // it is for desktop Casting) or there is no route.
  base::Optional<int> tab_id;

  // The icon to use for the sink.
  SinkIconType icon_type = SinkIconType::GENERIC;

  // The current state of the media sink.
  UIMediaSinkState state = UIMediaSinkState::AVAILABLE;

  // Help center article ID for troubleshooting.
  // This will show an info bubble with a tooltip for the article.
  // This is a nullopt if there are no issues with the sink.
  base::Optional<int> tooltip_article_id;

  // Bitmask of UICastAction values that determine which actions are supported
  // by this sink.
  int allowed_actions = 0;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_MEDIA_ROUTER_UI_MEDIA_SINK_H_
