// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/media_router/media_router_views_ui.h"

#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/media_router/media_sink_with_cast_modes.h"
#include "chrome/browser/ui/media_router/ui_media_sink.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace media_router {

namespace {

UIMediaSink ConvertToUISink(const MediaSinkWithCastModes& sink,
                            const MediaRoute* route) {
  UIMediaSink ui_sink;
  ui_sink.id = sink.sink.id();
  ui_sink.friendly_name = base::UTF8ToUTF16(sink.sink.name());
  ui_sink.icon_type = sink.sink.icon_type();

  if (route) {
    ui_sink.status_text = base::UTF8ToUTF16(route->description());
    ui_sink.route_id = route->media_route_id();
    ui_sink.state = UIMediaSinkState::CONNECTED;
    ui_sink.allowed_actions = static_cast<int>(UICastAction::STOP);
  } else {
    ui_sink.state = UIMediaSinkState::AVAILABLE;
    if (base::ContainsKey(sink.cast_modes, PRESENTATION) ||
        base::ContainsKey(sink.cast_modes, TAB_MIRROR)) {
      ui_sink.allowed_actions |= static_cast<int>(UICastAction::CAST_TAB);
    }
    if (base::ContainsKey(sink.cast_modes, DESKTOP_MIRROR))
      ui_sink.allowed_actions |= static_cast<int>(UICastAction::CAST_DESKTOP);
    // TODO(takumif): Add support for local media casting.
  }
  DCHECK(ui_sink.allowed_actions);
  return ui_sink;
}

}  // namespace

MediaRouterViewsUI::MediaRouterViewsUI() = default;

MediaRouterViewsUI::~MediaRouterViewsUI() {
  for (CastDialogController::Observer& observer : observers_)
    observer.OnControllerInvalidated();
}

void MediaRouterViewsUI::AddObserver(CastDialogController::Observer* observer) {
  observers_.AddObserver(observer);
  observer->OnModelUpdated(model_);
}

void MediaRouterViewsUI::RemoveObserver(
    CastDialogController::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void MediaRouterViewsUI::StartCasting(const std::string& sink_id,
                                      MediaCastMode cast_mode) {
  CreateRoute(sink_id, cast_mode);
}

void MediaRouterViewsUI::StopCasting(const std::string& route_id) {
  TerminateRoute(route_id);
}

void MediaRouterViewsUI::OnRoutesUpdated(
    const std::vector<MediaRoute>& routes,
    const std::vector<MediaRoute::Id>& joinable_route_ids) {
  MediaRouterUIBase::OnRoutesUpdated(routes, joinable_route_ids);
  UpdateSinks();
}

void MediaRouterViewsUI::UpdateSinks() {
  model_.dialog_header =
      l10n_util::GetStringUTF16(IDS_MEDIA_ROUTER_AUTO_CAST_MODE);
  model_.media_sinks.clear();
  for (const MediaSinkWithCastModes& sink : GetEnabledSinks()) {
    auto route_it = std::find_if(
        routes().begin(), routes().end(), [&sink](const MediaRoute& route) {
          return route.media_sink_id() == sink.sink.id();
        });
    const MediaRoute* route = route_it == routes().end() ? nullptr : &*route_it;
    model_.media_sinks.push_back(ConvertToUISink(sink, route));
  }
  for (CastDialogController::Observer& observer : observers_)
    observer.OnModelUpdated(model_);
}

}  // namespace media_router
