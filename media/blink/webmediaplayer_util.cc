// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webmediaplayer_util.h"

#include <math.h>
#include <stddef.h>
#include <string>
#include <utility>

#include "base/metrics/histogram_macros.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_log.h"
#include "third_party/blink/public/platform/url_conversion.h"
#include "third_party/blink/public/platform/web_media_player_encrypted_media_client.h"

namespace media {

blink::WebTimeRanges ConvertToWebTimeRanges(
    const Ranges<base::TimeDelta>& ranges) {
  blink::WebTimeRanges result(ranges.size());
  for (size_t i = 0; i < ranges.size(); ++i) {
    result[i].start = ranges.start(i).InSecondsF();
    result[i].end = ranges.end(i).InSecondsF();
  }
  return result;
}

blink::WebMediaPlayer::NetworkState PipelineErrorToNetworkState(
    PipelineStatus error) {
  switch (error) {
    case PIPELINE_ERROR_NETWORK:
    case PIPELINE_ERROR_READ:
    case CHUNK_DEMUXER_ERROR_EOS_STATUS_NETWORK_ERROR:
      return blink::WebMediaPlayer::kNetworkStateNetworkError;

    case PIPELINE_ERROR_INITIALIZATION_FAILED:
    case PIPELINE_ERROR_COULD_NOT_RENDER:
    case PIPELINE_ERROR_EXTERNAL_RENDERER_FAILED:
    case DEMUXER_ERROR_COULD_NOT_OPEN:
    case DEMUXER_ERROR_COULD_NOT_PARSE:
    case DEMUXER_ERROR_NO_SUPPORTED_STREAMS:
    case DEMUXER_ERROR_DETECTED_HLS:
    case DECODER_ERROR_NOT_SUPPORTED:
      return blink::WebMediaPlayer::kNetworkStateFormatError;

    case PIPELINE_ERROR_DECODE:
    case PIPELINE_ERROR_ABORT:
    case PIPELINE_ERROR_INVALID_STATE:
    case CHUNK_DEMUXER_ERROR_APPEND_FAILED:
    case CHUNK_DEMUXER_ERROR_EOS_STATUS_DECODE_ERROR:
    case AUDIO_RENDERER_ERROR:
      return blink::WebMediaPlayer::kNetworkStateDecodeError;

    case PIPELINE_OK:
      NOTREACHED() << "Unexpected status! " << error;
  }
  return blink::WebMediaPlayer::kNetworkStateFormatError;
}

namespace {

// Helper enum for reporting scheme histograms.
enum URLSchemeForHistogram {
  kUnknownURLScheme,
  kMissingURLScheme,
  kHttpURLScheme,
  kHttpsURLScheme,
  kFtpURLScheme,
  kChromeExtensionURLScheme,
  kJavascriptURLScheme,
  kFileURLScheme,
  kBlobURLScheme,
  kDataURLScheme,
  kFileSystemScheme,
  kMaxURLScheme = kFileSystemScheme  // Must be equal to highest enum value.
};

URLSchemeForHistogram URLScheme(const GURL& url) {
  if (!url.has_scheme()) return kMissingURLScheme;
  if (url.SchemeIs("http")) return kHttpURLScheme;
  if (url.SchemeIs("https")) return kHttpsURLScheme;
  if (url.SchemeIs("ftp")) return kFtpURLScheme;
  if (url.SchemeIs("chrome-extension")) return kChromeExtensionURLScheme;
  if (url.SchemeIs("javascript")) return kJavascriptURLScheme;
  if (url.SchemeIs("file")) return kFileURLScheme;
  if (url.SchemeIs("blob")) return kBlobURLScheme;
  if (url.SchemeIs("data")) return kDataURLScheme;
  if (url.SchemeIs("filesystem")) return kFileSystemScheme;

  return kUnknownURLScheme;
}

std::string LoadTypeToString(blink::WebMediaPlayer::LoadType load_type) {
  switch (load_type) {
    case blink::WebMediaPlayer::kLoadTypeURL:
      return "SRC";
    case blink::WebMediaPlayer::kLoadTypeMediaSource:
      return "MSE";
    case blink::WebMediaPlayer::kLoadTypeMediaStream:
      return "MS";
  }

  NOTREACHED();
  return "Unknown";
}

}  // namespace

void ReportMetrics(blink::WebMediaPlayer::LoadType load_type,
                   const GURL& url,
                   const blink::WebSecurityOrigin& security_origin,
                   MediaLog* media_log) {
  DCHECK(media_log);

  // Report URL scheme, such as http, https, file, blob etc.
  UMA_HISTOGRAM_ENUMERATION("Media.URLScheme", URLScheme(url),
                            kMaxURLScheme + 1);

  // Report load type, such as URL, MediaSource or MediaStream.
  UMA_HISTOGRAM_ENUMERATION("Media.LoadType", load_type,
                            blink::WebMediaPlayer::kLoadTypeMax + 1);

  // Report the origin from where the media player is created.
  media_log->RecordRapporWithSecurityOrigin("Media.OriginUrl." +
                                            LoadTypeToString(load_type));

  // For MSE, also report usage by secure/insecure origin.
  if (load_type == blink::WebMediaPlayer::kLoadTypeMediaSource) {
    if (security_origin.IsPotentiallyTrustworthy()) {
      media_log->RecordRapporWithSecurityOrigin("Media.OriginUrl.MSE.Secure");
    } else {
      media_log->RecordRapporWithSecurityOrigin("Media.OriginUrl.MSE.Insecure");
    }
  }
}

void ReportPipelineError(blink::WebMediaPlayer::LoadType load_type,
                         PipelineStatus error,
                         MediaLog* media_log) {
  DCHECK_NE(PIPELINE_OK, error);

  // Report the origin from where the media player is created.
  media_log->RecordRapporWithSecurityOrigin(
      "Media.OriginUrl." + LoadTypeToString(load_type) + ".PipelineError");
}

EmeInitDataType ConvertToEmeInitDataType(
    blink::WebEncryptedMediaInitDataType init_data_type) {
  switch (init_data_type) {
    case blink::WebEncryptedMediaInitDataType::kWebm:
      return EmeInitDataType::WEBM;
    case blink::WebEncryptedMediaInitDataType::kCenc:
      return EmeInitDataType::CENC;
    case blink::WebEncryptedMediaInitDataType::kKeyids:
      return EmeInitDataType::KEYIDS;
    case blink::WebEncryptedMediaInitDataType::kUnknown:
      return EmeInitDataType::UNKNOWN;
  }

  NOTREACHED();
  return EmeInitDataType::UNKNOWN;
}

blink::WebEncryptedMediaInitDataType ConvertToWebInitDataType(
    EmeInitDataType init_data_type) {
  switch (init_data_type) {
    case EmeInitDataType::WEBM:
      return blink::WebEncryptedMediaInitDataType::kWebm;
    case EmeInitDataType::CENC:
      return blink::WebEncryptedMediaInitDataType::kCenc;
    case EmeInitDataType::KEYIDS:
      return blink::WebEncryptedMediaInitDataType::kKeyids;
    case EmeInitDataType::UNKNOWN:
      return blink::WebEncryptedMediaInitDataType::kUnknown;
  }

  NOTREACHED();
  return blink::WebEncryptedMediaInitDataType::kUnknown;
}

namespace {
// This class wraps a scoped blink::WebSetSinkIdCallbacks pointer such that
// copying objects of this class actually performs moving, thus
// maintaining clear ownership of the blink::WebSetSinkIdCallbacks pointer.
// The rationale for this class is that the SwichOutputDevice method
// can make a copy of its base::Callback parameter, which implies
// copying its bound parameters.
// SwitchOutputDevice actually wants to move its base::Callback
// parameter since only the final copy will be run, but base::Callback
// does not support move semantics and there is no base::MovableCallback.
// Since scoped pointers are not copyable, we cannot bind them directly
// to a base::Callback in this case. Thus, we use this helper class,
// whose copy constructor transfers ownership of the scoped pointer.

class SetSinkIdCallback {
 public:
  explicit SetSinkIdCallback(blink::WebSetSinkIdCallbacks* web_callback)
      : web_callback_(web_callback) {}
  SetSinkIdCallback(const SetSinkIdCallback& other)
      : web_callback_(std::move(other.web_callback_)) {}
  ~SetSinkIdCallback() = default;
  friend void RunSetSinkIdCallback(const SetSinkIdCallback& callback,
                                   OutputDeviceStatus result);

 private:
  // Mutable is required so that Pass() can be called in the copy
  // constructor.
  mutable std::unique_ptr<blink::WebSetSinkIdCallbacks> web_callback_;
};

void RunSetSinkIdCallback(const SetSinkIdCallback& callback,
                          OutputDeviceStatus result) {
  if (!callback.web_callback_)
    return;

  switch (result) {
    case OUTPUT_DEVICE_STATUS_OK:
      callback.web_callback_->OnSuccess();
      break;
    case OUTPUT_DEVICE_STATUS_ERROR_NOT_FOUND:
      callback.web_callback_->OnError(blink::WebSetSinkIdError::kNotFound);
      break;
    case OUTPUT_DEVICE_STATUS_ERROR_NOT_AUTHORIZED:
      callback.web_callback_->OnError(blink::WebSetSinkIdError::kNotAuthorized);
      break;
    case OUTPUT_DEVICE_STATUS_ERROR_INTERNAL:
      callback.web_callback_->OnError(blink::WebSetSinkIdError::kAborted);
      break;
    default:
      NOTREACHED();
  }

  callback.web_callback_ = nullptr;
}

}  // namespace

OutputDeviceStatusCB ConvertToOutputDeviceStatusCB(
    blink::WebSetSinkIdCallbacks* web_callbacks) {
  return media::BindToCurrentLoop(
      base::Bind(RunSetSinkIdCallback, SetSinkIdCallback(web_callbacks)));
}

}  // namespace media
