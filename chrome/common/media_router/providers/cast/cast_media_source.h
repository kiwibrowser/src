// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_MEDIA_ROUTER_PROVIDERS_CAST_CAST_MEDIA_SOURCE_H_
#define CHROME_COMMON_MEDIA_ROUTER_PROVIDERS_CAST_CAST_MEDIA_SOURCE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/optional.h"
#include "chrome/common/media_router/media_source.h"
#include "components/cast_channel/cast_message_util.h"
#include "components/cast_channel/cast_socket.h"

namespace media_router {

// Represents a Cast app and its capabilitity requirements.
struct CastAppInfo {
  explicit CastAppInfo(const std::string& app_id);
  ~CastAppInfo();

  CastAppInfo(const CastAppInfo& other);

  std::string app_id;

  // A bitset of capabilities required by the app.
  int required_capabilities = cast_channel::CastDeviceCapability::NONE;
};

// Represents a MediaSource parsed into structured, Cast specific data. The
// following MediaSources can be parsed into CastMediaSource:
// - Cast Presentation URLs
// - HTTP(S) Presentation URLs
// - Desktop / tab mirroring URNs
class CastMediaSource {
 public:
  // Returns the parsed form of |source|, or nullptr if it cannot be parsed.
  static std::unique_ptr<CastMediaSource> From(const MediaSource::Id& source);

  explicit CastMediaSource(const MediaSource::Id& source_id,
                           const CastAppInfo& app_info);
  explicit CastMediaSource(const MediaSource::Id& source_id,
                           const std::vector<CastAppInfo>& app_infos);
  CastMediaSource(const CastMediaSource& other);
  ~CastMediaSource();

  // Returns |true| if |app_infos| contain |app_id|.
  bool ContainsApp(const std::string& app_id) const;
  bool ContainsAnyAppFrom(const std::vector<std::string>& app_ids) const;

  // Returns a list of App IDs in this CastMediaSource.
  std::vector<std::string> GetAppIds() const;

  const MediaSource::Id& source_id() const { return source_id_; }
  const std::vector<CastAppInfo>& app_infos() const { return app_infos_; }
  const base::Optional<cast_channel::BroadcastRequest>& broadcast_request()
      const {
    return broadcast_request_;
  }

  void set_broadcast_request(const cast_channel::BroadcastRequest& request) {
    broadcast_request_ = request;
  }

 private:
  // TODO(imcheng): Fill in other parameters.
  MediaSource::Id source_id_;
  std::vector<CastAppInfo> app_infos_;
  base::Optional<cast_channel::BroadcastRequest> broadcast_request_;
};

}  // namespace media_router

#endif  // CHROME_COMMON_MEDIA_ROUTER_PROVIDERS_CAST_CAST_MEDIA_SOURCE_H_
