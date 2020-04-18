// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/media_router/media_router_localized_strings_provider.h"

#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui_data_source.h"

namespace {

// Note that media_router.html contains a <script> tag which imports a script
// of the following name. These names must be kept in sync.
const char kLocalizedStringsFile[] = "strings.js";

void AddMediaRouterStrings(content::WebUIDataSource* html_source) {
  html_source->AddLocalizedString("mediaRouterTitle", IDS_MEDIA_ROUTER_TITLE);
  html_source->AddLocalizedString("learnMoreText", IDS_LEARN_MORE);
  html_source->AddLocalizedString("backButtonTitle",
                                  IDS_MEDIA_ROUTER_BACK_BUTTON_TITLE);
  html_source->AddLocalizedString("closeButtonTitle",
                                  IDS_MEDIA_ROUTER_CLOSE_BUTTON_TITLE);
  html_source->AddLocalizedString("searchButtonTitle",
                                  IDS_MEDIA_ROUTER_SEARCH_BUTTON_TITLE);
  html_source->AddLocalizedString(
      "viewCastModeListButtonTitle",
      IDS_MEDIA_ROUTER_VIEW_CAST_MODE_LIST_BUTTON_TITLE);
  html_source->AddLocalizedString(
      "viewDeviceListButtonTitle",
      IDS_MEDIA_ROUTER_VIEW_DEVICE_LIST_BUTTON_TITLE);
}

void AddRouteDetailsStrings(content::WebUIDataSource* html_source) {
  html_source->AddLocalizedString("castingActivityStatus",
                                  IDS_MEDIA_ROUTER_CASTING_ACTIVITY_STATUS);
  html_source->AddLocalizedString("stopCastingButtonText",
                                  IDS_MEDIA_ROUTER_STOP_CASTING_BUTTON);
  html_source->AddLocalizedString("startCastingButtonText",
                                  IDS_MEDIA_ROUTER_START_CASTING_BUTTON);
  html_source->AddLocalizedString("playTitle",
                                  IDS_MEDIA_ROUTER_ROUTE_DETAILS_PLAY_TITLE);
  html_source->AddLocalizedString("pauseTitle",
                                  IDS_MEDIA_ROUTER_ROUTE_DETAILS_PAUSE_TITLE);
  html_source->AddLocalizedString("muteTitle",
                                  IDS_MEDIA_ROUTER_ROUTE_DETAILS_MUTE_TITLE);
  html_source->AddLocalizedString("unmuteTitle",
                                  IDS_MEDIA_ROUTER_ROUTE_DETAILS_UNMUTE_TITLE);
  html_source->AddLocalizedString("seekTitle",
                                  IDS_MEDIA_ROUTER_ROUTE_DETAILS_SEEK_TITLE);
  html_source->AddLocalizedString("volumeTitle",
                                  IDS_MEDIA_ROUTER_ROUTE_DETAILS_VOLUME_TITLE);
  html_source->AddLocalizedString(
      "currentTimeLabel", IDS_MEDIA_ROUTER_ROUTE_DETAILS_CURRENT_TIME_LABEL);
  html_source->AddLocalizedString(
      "durationLabel", IDS_MEDIA_ROUTER_ROUTE_DETAILS_DURATION_LABEL);
  html_source->AddLocalizedString(
      "hangoutsLocalPresentTitle",
      IDS_MEDIA_ROUTER_ROUTE_DETAILS_HANGOUTS_LOCAL_PRESENT_TITLE);
  html_source->AddLocalizedString(
      "hangoutsLocalPresentSubtitle",
      IDS_MEDIA_ROUTER_ROUTE_DETAILS_HANGOUTS_LOCAL_PRESENT_SUBTITLE);
  html_source->AddLocalizedString(
      "alwaysUseMirroringTitle",
      IDS_MEDIA_ROUTER_ROUTE_DETAILS_ALWAYS_USE_MIRRORING_TITLE);
  html_source->AddLocalizedString(
      "fullscreenVideosDropdownTitle",
      IDS_MEDIA_ROUTER_ROUTE_DETAILS_FULLSCREEN_VIDEOS_DROPDOWN_TITLE);
  html_source->AddLocalizedString(
      "fullscreenVideosRemoteScreen",
      IDS_MEDIA_ROUTER_ROUTE_DETAILS_FULLSCREEN_VIDEOS_REMOTE_SCREEN);
  html_source->AddLocalizedString(
      "fullscreenVideosBothScreens",
      IDS_MEDIA_ROUTER_ROUTE_DETAILS_FULLSCREEN_VIDEOS_BOTH_SCREENS);
}

void AddIssuesStrings(content::WebUIDataSource* html_source) {
  html_source->AddLocalizedString("dismissButton",
                                  IDS_MEDIA_ROUTER_DISMISS_BUTTON);
  html_source->AddLocalizedString("issueHeaderText",
                                  IDS_MEDIA_ROUTER_ISSUE_HEADER);
}

void AddMediaRouterContainerStrings(content::WebUIDataSource* html_source) {
  html_source->AddLocalizedString("castLocalMediaSubheadingText",
                                  IDS_MEDIA_ROUTER_CAST_LOCAL_MEDIA_SUBHEADING);
  html_source->AddLocalizedString("castLocalMediaSelectedFileTitle",
                                  IDS_MEDIA_ROUTER_CAST_LOCAL_MEDIA_TITLE);
  html_source->AddLocalizedString("firstRunFlowButtonText",
                                  IDS_MEDIA_ROUTER_FIRST_RUN_FLOW_BUTTON);
  html_source->AddLocalizedString("firstRunFlowText",
                                  IDS_MEDIA_ROUTER_FIRST_RUN_FLOW_TEXT);
  html_source->AddLocalizedString("firstRunFlowTitle",
                                  IDS_MEDIA_ROUTER_FIRST_RUN_FLOW_TITLE);
  html_source->AddLocalizedString(
      "firstRunFlowCloudPrefText",
      IDS_MEDIA_ROUTER_FIRST_RUN_FLOW_CLOUD_PREF_TEXT);
  html_source->AddLocalizedString("autoCastMode",
                                  IDS_MEDIA_ROUTER_AUTO_CAST_MODE);
  html_source->AddLocalizedString("destinationMissingText",
                                  IDS_MEDIA_ROUTER_DESTINATION_MISSING);
  html_source->AddLocalizedString("searchInputLabel",
                                  IDS_MEDIA_ROUTER_SEARCH_LABEL);
  html_source->AddLocalizedString("searchNoMatchesText",
                                  IDS_MEDIA_ROUTER_SEARCH_NO_MATCHES);
  html_source->AddLocalizedString("selectCastModeHeaderText",
                                  IDS_MEDIA_ROUTER_SELECT_CAST_MODE_HEADER);
  html_source->AddLocalizedString(
      "shareYourScreenSubheadingText",
      IDS_MEDIA_ROUTER_SHARE_YOUR_SCREEN_SUBHEADING);
}

}  // namespace

namespace media_router {

void AddLocalizedStrings(content::WebUIDataSource* html_source) {
  AddMediaRouterStrings(html_source);
  AddRouteDetailsStrings(html_source);
  AddIssuesStrings(html_source);
  AddMediaRouterContainerStrings(html_source);
  html_source->SetJsonPath(kLocalizedStringsFile);
}

}  // namespace media_router
