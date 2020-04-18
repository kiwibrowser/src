// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_MEDIA_ROUTER_MEDIA_SOURCE_HELPER_H_
#define CHROME_COMMON_MEDIA_ROUTER_MEDIA_SOURCE_HELPER_H_

#include <string>
#include <vector>

#include "chrome/common/media_router/media_source.h"

class GURL;

namespace media_router {

// URL schemes used by Presentation URLs for Cast and DIAL.
constexpr char kCastPresentationUrlScheme[] = "cast";
constexpr char kCastDialPresentationUrlScheme[] = "cast-dial";
constexpr char kDialPresentationUrlScheme[] = "dial";
constexpr char kRemotePlaybackPresentationUrlScheme[] = "remote-playback";

// URL prefix used by legacy Cast presentations.
constexpr char kLegacyCastPresentationUrlPrefix[] =
    "https://google.com/cast#__castAppId__=";

// Strings used in presentation IDs by the Cast SDK implementation.
// TODO(takumif): Move them out of media_source_helper, since they are not
// directly related to MediaSource.
//
// This value must be the same as |chrome.cast.AUTO_JOIN_PRESENTATION_ID| in the
// component extension.
constexpr char kAutoJoinPresentationId[] = "auto-join";
// This value must be the same as |chrome.cast.PRESENTATION_ID_PREFIX| in the
// component extension.
constexpr char kCastPresentationIdPrefix[] = "cast-session_";

// Helper library for protocol-specific media source object creation.
// Returns MediaSource URI depending on the type of source.
MediaSource MediaSourceForTab(int tab_id);
MediaSource MediaSourceForTabContentRemoting(int tab_id);
MediaSource MediaSourceForDesktop();
MediaSource MediaSourceForPresentationUrl(const GURL& presentation_url);

// Converts multiple Presentation URLs into MediaSources.
std::vector<MediaSource> MediaSourcesForPresentationUrls(
    const std::vector<GURL>& presentation_urls);

// Returns true if |source| outputs its content via mirroring.
bool IsDesktopMirroringMediaSource(const MediaSource& source);
bool IsTabMirroringMediaSource(const MediaSource& source);
bool IsMirroringMediaSource(const MediaSource& source);

// Returns true if |source| is represents a Cast Presentation URL.
bool IsCastPresentationUrl(const MediaSource& source);

// Parses the |source| and returns the SessionTabHelper tab ID referencing a
// source tab. Returns a non-positive value on error.
int TabIdFromMediaSource(const MediaSource& source);

// Checks that |source| is a parseable URN and is of a known type.
// Does not deeper protocol-level syntax checks.
bool IsValidMediaSource(const MediaSource& source);

// Returns true if |url| represents a legacy Cast presentation URL, i.e., it
// starts with |kLegacyCastPresentationUrlPrefix|.
bool IsLegacyCastPresentationUrl(const GURL& url);

// Returns true if |url| is a valid presentation URL.
bool IsValidPresentationUrl(const GURL& url);

// Returns true if |presentation_id| is an ID used by auto-join requests.
bool IsAutoJoinPresentationId(const std::string& presentation_id);

// Returns true if |source| outputs its content via DIAL.
// TODO(crbug.com/804419): Move this to in-browser DIAL/Cast MRP when we have
// one.
bool IsDialMediaSource(const MediaSource& source);

// Returns empty string if |source| is not DIAL media source, or is not a valid
// DIAL media source.
std::string AppNameFromDialMediaSource(const MediaSource& source);

}  // namespace media_router

#endif  // CHROME_COMMON_MEDIA_ROUTER_MEDIA_SOURCE_HELPER_H_
