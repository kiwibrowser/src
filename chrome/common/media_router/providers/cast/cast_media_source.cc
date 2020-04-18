// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/providers/cast/cast_media_source.h"

#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/url_util.h"

using cast_channel::BroadcastRequest;

namespace media_router {

namespace {

constexpr char kMirroringAppId[] = "0F5096E8";
constexpr char kAudioMirroringAppId[] = "85CDB22F";

// Parameter keys used by new Cast URLs.
constexpr char kCapabilitiesKey[] = "capabilities";
constexpr char kBroadcastNamespaceKey[] = "broadcastNamespace";
constexpr char kBroadcastMessageKey[] = "broadcastMessage";

// Parameter keys used by legacy Cast URLs.
constexpr char kLegacyAppIdKey[] = "__castAppId__";
constexpr char kLegacyBroadcastNamespaceKey[] = "__castBroadcastNamespace__";
constexpr char kLegacyBroadcastMessageKey[] = "__castBroadcastMessage__";

// TODO(imcheng): Move to common utils?
std::string DecodeURLComponent(const std::string& encoded) {
  url::RawCanonOutputT<base::char16> unescaped;
  std::string output;
  url::DecodeURLEscapeSequences(encoded.data(), encoded.size(), &unescaped);
  if (base::UTF16ToUTF8(unescaped.data(), unescaped.length(), &output))
    return output;

  return std::string();
}

cast_channel::CastDeviceCapability CastDeviceCapabilityFromString(
    const base::StringPiece& s) {
  if (s == "video_out")
    return cast_channel::CastDeviceCapability::VIDEO_OUT;
  if (s == "video_in")
    return cast_channel::CastDeviceCapability::VIDEO_IN;
  if (s == "audio_out")
    return cast_channel::CastDeviceCapability::AUDIO_OUT;
  if (s == "audio_in")
    return cast_channel::CastDeviceCapability::AUDIO_IN;
  if (s == "multizone_group")
    return cast_channel::CastDeviceCapability::MULTIZONE_GROUP;

  return cast_channel::CastDeviceCapability::NONE;
}

std::unique_ptr<CastMediaSource> CastMediaSourceForTabMirroring(
    const MediaSource::Id& source_id) {
  return std::make_unique<CastMediaSource>(
      source_id, std::vector<CastAppInfo>({CastAppInfo(kMirroringAppId),
                                           CastAppInfo(kAudioMirroringAppId)}));
}

std::unique_ptr<CastMediaSource> CastMediaSourceForDesktopMirroring(
    const MediaSource::Id& source_id) {
// Desktop audio mirroring is only supported on some platforms.
#if defined(OS_WIN) || defined(OS_CHROMEOS)
  return std::make_unique<CastMediaSource>(
      source_id, std::vector<CastAppInfo>({CastAppInfo(kMirroringAppId),
                                           CastAppInfo(kAudioMirroringAppId)}));
#else
  return std::make_unique<CastMediaSource>(
      source_id, std::vector<CastAppInfo>({CastAppInfo(kMirroringAppId)}));
#endif
}

std::unique_ptr<CastMediaSource> CreateFromURLParams(
    const MediaSource::Id& source_id,
    const std::vector<CastAppInfo>& app_infos,
    const std::string& broadcast_namespace,
    const std::string& broadcast_message) {
  if (app_infos.empty())
    return nullptr;

  auto cast_source = std::make_unique<CastMediaSource>(source_id, app_infos);
  if (!broadcast_namespace.empty() && !broadcast_message.empty()) {
    cast_source->set_broadcast_request(
        BroadcastRequest(broadcast_namespace, broadcast_message));
  }

  return cast_source;
}

std::unique_ptr<CastMediaSource> ParseCastUrl(const MediaSource::Id& source_id,
                                              const GURL& url) {
  std::string app_id = url.path();
  // App ID must be non-empty.
  if (app_id.empty())
    return nullptr;

  std::string broadcast_namespace, broadcast_message, capabilities;
  for (net::QueryIterator query_it(url); !query_it.IsAtEnd();
       query_it.Advance()) {
    std::string key = query_it.GetKey();
    std::string value = query_it.GetValue();
    if (key.empty() || value.empty())
      continue;
    if (key == kBroadcastNamespaceKey) {
      broadcast_namespace = value;
    } else if (key == kBroadcastMessageKey) {
      // The broadcast message is URL-encoded.
      broadcast_message = DecodeURLComponent(value);
    } else if (key == kCapabilitiesKey) {
      capabilities = value;
    }
  }

  CastAppInfo app_info(app_id);
  if (!capabilities.empty()) {
    for (const auto& capability :
         base::SplitStringPiece(capabilities, ",", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY)) {
      app_info.required_capabilities |=
          CastDeviceCapabilityFromString(capability);
    }
  }

  return CreateFromURLParams(source_id, {app_info}, broadcast_namespace,
                             broadcast_message);
}

std::unique_ptr<CastMediaSource> ParseLegacyCastUrl(
    const MediaSource::Id& source_id,
    const GURL& url) {
  base::StringPairs parameters;
  base::SplitStringIntoKeyValuePairs(url.ref(), '=', '/', &parameters);
  // Legacy URLs can specify multiple apps.
  std::vector<std::string> app_id_params;
  std::string broadcast_namespace, broadcast_message;
  for (const auto& key_value : parameters) {
    const auto& key = key_value.first;
    const auto& value = key_value.second;
    if (key == kLegacyAppIdKey) {
      app_id_params.push_back(value);
    } else if (key == kLegacyBroadcastNamespaceKey) {
      broadcast_namespace = value;
    } else if (key == kLegacyBroadcastMessageKey) {
      // The broadcast message is URL-encoded.
      broadcast_message = DecodeURLComponent(value);
    }
  }

  std::vector<CastAppInfo> app_infos;
  for (const auto& app_id_param : app_id_params) {
    std::string app_id;
    std::string capabilities;
    auto cap_start_index = app_id_param.find('(');
    // If |cap_start_index| is |npos|, then this will return the entire string.
    app_id = app_id_param.substr(0, cap_start_index);
    if (cap_start_index != std::string::npos) {
      auto cap_end_index = app_id_param.find(')', cap_start_index);
      if (cap_end_index != std::string::npos &&
          cap_end_index > cap_start_index) {
        capabilities = app_id_param.substr(cap_start_index + 1,
                                           cap_end_index - cap_start_index - 1);
      }
    }

    if (app_id.empty())
      continue;

    CastAppInfo app_info(app_id);
    for (const auto& capability :
         base::SplitStringPiece(capabilities, ",", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY)) {
      app_info.required_capabilities |=
          CastDeviceCapabilityFromString(capability);
    }

    app_infos.push_back(app_info);
  }

  if (app_infos.empty())
    return nullptr;

  return CreateFromURLParams(source_id, app_infos, broadcast_namespace,
                             broadcast_message);
}

}  // namespace

CastAppInfo::CastAppInfo(const std::string& app_id) : app_id(app_id) {}
CastAppInfo::~CastAppInfo() = default;

CastAppInfo::CastAppInfo(const CastAppInfo& other) = default;

// static
std::unique_ptr<CastMediaSource> CastMediaSource::From(
    const MediaSource::Id& source_id) {
  MediaSource source(source_id);
  if (IsTabMirroringMediaSource(source))
    return CastMediaSourceForTabMirroring(source_id);

  if (IsDesktopMirroringMediaSource(source))
    return CastMediaSourceForDesktopMirroring(source_id);

  const GURL& url = source.url();
  if (!url.is_valid())
    return nullptr;

  if (url.SchemeIs(kCastPresentationUrlScheme)) {
    return ParseCastUrl(source_id, url);
  } else if (IsLegacyCastPresentationUrl(url)) {
    return ParseLegacyCastUrl(source_id, url);
  } else if (url.SchemeIsHTTPOrHTTPS()) {
    // Arbitrary https URLs are supported via 1-UA mode which uses tab
    // mirroring.
    return CastMediaSourceForTabMirroring(source_id);
  }

  return nullptr;
}

CastMediaSource::CastMediaSource(const MediaSource::Id& source_id,
                                 const CastAppInfo& app_info)
    : source_id_(source_id), app_infos_({app_info}) {}
CastMediaSource::CastMediaSource(const MediaSource::Id& source_id,
                                 const std::vector<CastAppInfo>& app_infos)
    : source_id_(source_id), app_infos_(app_infos) {}
CastMediaSource::CastMediaSource(const CastMediaSource& other) = default;
CastMediaSource::~CastMediaSource() = default;

bool CastMediaSource::ContainsApp(const std::string& app_id) const {
  for (const auto& info : app_infos_) {
    if (info.app_id == app_id)
      return true;
  }
  return false;
}

bool CastMediaSource::ContainsAnyAppFrom(
    const std::vector<std::string>& app_ids) const {
  return std::any_of(
      app_ids.begin(), app_ids.end(),
      [this](const std::string& app_id) { return ContainsApp(app_id); });
}

std::vector<std::string> CastMediaSource::GetAppIds() const {
  std::vector<std::string> app_ids;
  for (const auto& info : app_infos_)
    app_ids.push_back(info.app_id);

  return app_ids;
}

}  // namespace media_router
