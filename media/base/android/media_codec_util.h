// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ANDROID_MEDIA_CODEC_UTIL_H_
#define MEDIA_BASE_ANDROID_MEDIA_CODEC_UTIL_H_

#include <jni.h>
#include <set>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "media/base/android/media_codec_direction.h"
#include "media/base/audio_codecs.h"
#include "media/base/media_export.h"
#include "media/base/video_codecs.h"

class GURL;

namespace media {

class MediaCodecBridge;

class MEDIA_EXPORT MediaCodecUtil {
 public:
  static std::string CodecToAndroidMimeType(AudioCodec codec);
  static std::string CodecToAndroidMimeType(VideoCodec codec);

  // Returns true if MediaCodec is available on the device.
  // All other static methods check IsAvailable() internally. There's no need
  // to check IsAvailable() explicitly before calling them.
  static bool IsMediaCodecAvailable();

  // Returns true if MediaCodec is available, with |sdk| as the sdk version and
  // |model| as the model.  This is provided for unit tests; you probably want
  // IsMediaCodecAvailable() otherwise.
  // TODO(liberato): merge this with IsMediaCodecAvailable, and provide a way
  // to mock BuildInfo instead.
  static bool IsMediaCodecAvailableFor(int sdk, const char* model);

  // Returns true if MediaCodec.setParameters() is available on the device.
  static bool SupportsSetParameters();

  // Returns true if MediaCodec supports CBCS Encryption.
  static bool PlatformSupportsCbcsEncryption(int sdk);

  // Returns whether it's possible to create a MediaCodec for the given codec
  // and secureness.
  static bool CanDecode(VideoCodec codec, bool is_secure);
  static bool CanDecode(AudioCodec codec);

  // Returns a vector of supported codecs profiles and levels.
  static bool AddSupportedCodecProfileLevels(
      std::vector<CodecProfileLevel>* out);

  // Get a list of encoder supported color formats for |mime_type|.
  // The mapping of color format name and its value refers to
  // MediaCodecInfo.CodecCapabilities.
  static std::set<int> GetEncoderColorFormats(const std::string& mime_type);

  // Returns true if |mime_type| is known to be unaccelerated (i.e. backed by a
  // software codec instead of a hardware one).
  static bool IsKnownUnaccelerated(VideoCodec codec,
                                   MediaCodecDirection direction);

  // Test whether a URL contains "m3u8".
  static bool IsHLSURL(const GURL& url);

  // Test whether the path of a URL ends with ".m3u8".
  static bool IsHLSPath(const GURL& url);

  // Indicates if the vp8 decoder or encoder is available on this device.
  static bool IsVp8DecoderAvailable();
  static bool IsVp8EncoderAvailable();

  // Indicates if the vp9 decoder is available on this device.
  static bool IsVp9DecoderAvailable();

  // Indicates if the h264 encoder is available on this device.
  static bool IsH264EncoderAvailable();

  // Indicates if SurfaceView and MediaCodec work well together on this device.
  static bool IsSurfaceViewOutputSupported();

  // Indicates if MediaCodec.setOutputSurface() works on this device.
  static bool IsSetOutputSurfaceSupported();

  // Return true if the compressed audio |codec| will pass through the media
  // pipelines without decompression.
  static bool IsPassthroughAudioFormat(AudioCodec codec);

  // Indicates if the decoder is known to fail when flushed. (b/8125974,
  // b/8347958)
  // When true, the client should work around the issue by releasing the
  // decoder and instantiating a new one rather than flushing the current one.
  static bool CodecNeedsFlushWorkaround(MediaCodecBridge* codec);

 private:
  static bool CanDecodeInternal(const std::string& mime, bool is_secure);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_MEDIA_CODEC_UTIL_H_
