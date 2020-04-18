// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/media_source_helper.h"

#include <stdio.h>

#include <algorithm>
#include <array>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/common/media_router/media_source.h"
#include "url/gurl.h"

namespace media_router {

namespace {

// Prefixes used to format and detect various protocols' media source URNs.
// See: https://www.ietf.org/rfc/rfc3406.txt
constexpr char kTabMediaUrnFormat[] = "urn:x-org.chromium.media:source:tab:%d";
constexpr char kDesktopMediaUrn[] = "urn:x-org.chromium.media:source:desktop";
constexpr char kTabRemotingUrnFormat[] =
    "urn:x-org.chromium.media:source:tab_content_remoting:%d";

// List of non-http(s) schemes that are allowed in a Presentation URL.
constexpr std::array<const char* const, 5> kAllowedSchemes{
    {kCastPresentationUrlScheme, kCastDialPresentationUrlScheme,
     kDialPresentationUrlScheme, kRemotePlaybackPresentationUrlScheme, "test"}};

bool IsSchemeAllowed(const GURL& url) {
  return url.SchemeIsHTTPOrHTTPS() ||
         std::any_of(
             kAllowedSchemes.begin(), kAllowedSchemes.end(),
             [&url](const char* const scheme) { return url.SchemeIs(scheme); });
}

}  // namespace

MediaSource MediaSourceForTab(int tab_id) {
  return MediaSource(base::StringPrintf(kTabMediaUrnFormat, tab_id));
}

MediaSource MediaSourceForTabContentRemoting(int tab_id) {
  return MediaSource(base::StringPrintf(kTabRemotingUrnFormat, tab_id));
}

MediaSource MediaSourceForDesktop() {
  return MediaSource(std::string(kDesktopMediaUrn));
}

MediaSource MediaSourceForPresentationUrl(const GURL& presentation_url) {
  return MediaSource(presentation_url);
}

std::vector<MediaSource> MediaSourcesForPresentationUrls(
    const std::vector<GURL>& presentation_urls) {
  std::vector<MediaSource> sources;
  for (const auto& presentation_url : presentation_urls)
    sources.push_back(MediaSourceForPresentationUrl(presentation_url));

  return sources;
}

bool IsDesktopMirroringMediaSource(const MediaSource& source) {
  return base::StartsWith(source.id(), kDesktopMediaUrn,
                          base::CompareCase::SENSITIVE);
}

bool IsTabMirroringMediaSource(const MediaSource& source) {
  int tab_id;
  return sscanf(source.id().c_str(), kTabMediaUrnFormat, &tab_id) == 1 &&
         tab_id > 0;
}

bool IsMirroringMediaSource(const MediaSource& source) {
  return IsDesktopMirroringMediaSource(source) ||
         IsTabMirroringMediaSource(source);
}

bool IsCastPresentationUrl(const MediaSource& source) {
  const GURL& url = source.url();
  return url.SchemeIs(kCastPresentationUrlScheme) ||
         IsLegacyCastPresentationUrl(url);
}

int TabIdFromMediaSource(const MediaSource& source) {
  int tab_id;
  if (sscanf(source.id().c_str(), kTabMediaUrnFormat, &tab_id) == 1)
    return tab_id;
  else if (sscanf(source.id().c_str(), kTabRemotingUrnFormat, &tab_id) == 1)
    return tab_id;
  else
    return -1;
}

bool IsValidMediaSource(const MediaSource& source) {
  return TabIdFromMediaSource(source) > 0 ||
         IsDesktopMirroringMediaSource(source) ||
         IsValidPresentationUrl(GURL(source.id()));
}

bool IsLegacyCastPresentationUrl(const GURL& url) {
  return base::StartsWith(url.spec(), kLegacyCastPresentationUrlPrefix,
                          base::CompareCase::INSENSITIVE_ASCII);
}

bool IsValidPresentationUrl(const GURL& url) {
  return url.is_valid() && IsSchemeAllowed(url);
}

bool IsAutoJoinPresentationId(const std::string& presentation_id) {
  return presentation_id == kAutoJoinPresentationId;
}

bool IsDialMediaSource(const MediaSource& source) {
  return source.url().SchemeIs(kCastDialPresentationUrlScheme);
}

std::string AppNameFromDialMediaSource(const MediaSource& source) {
  return IsDialMediaSource(source) ? source.url().path() : "";
}

}  // namespace media_router
