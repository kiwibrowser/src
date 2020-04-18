// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ANDROID_MEDIA_CODEC_BRIDGE_IMPL_H_
#define MEDIA_BASE_ANDROID_MEDIA_CODEC_BRIDGE_IMPL_H_

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "media/base/android/media_codec_bridge.h"
#include "media/base/android/media_codec_direction.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/media_export.h"
#include "media/base/video_decoder_config.h"
#include "ui/gfx/geometry/size.h"

namespace media {

class VideoColorSpace;
struct HDRMetadata;

// A bridge to a Java MediaCodec.
class MEDIA_EXPORT MediaCodecBridgeImpl : public MediaCodecBridge {
 public:
  // Creates and starts a new MediaCodec configured for decoding. Returns
  // nullptr on failure.
  static std::unique_ptr<MediaCodecBridge> CreateVideoDecoder(
      VideoCodec codec,
      CodecType codec_type,
      const gfx::Size& size,  // Output frame size.
      const base::android::JavaRef<jobject>&
          surface,  // Output surface, optional.
      const base::android::JavaRef<jobject>&
          media_crypto,  // MediaCrypto object, optional.
      // Codec specific data. See MediaCodec docs.
      const std::vector<uint8_t>& csd0,
      const std::vector<uint8_t>& csd1,
      const VideoColorSpace& color_space,
      const base::Optional<HDRMetadata>& hdr_metadata,
      // Should adaptive playback be allowed if supported.
      bool allow_adaptive_playback = true);

  // Creates and starts a new MediaCodec configured for encoding. Returns
  // nullptr on failure.
  static std::unique_ptr<MediaCodecBridge> CreateVideoEncoder(
      VideoCodec codec,       // e.g. media::kCodecVP8
      const gfx::Size& size,  // input frame size
      int bit_rate,           // bits/second
      int frame_rate,         // frames/second
      int i_frame_interval,   // count
      int color_format);      // MediaCodecInfo.CodecCapabilities.

  // Creates and starts a new MediaCodec configured for decoding. Returns
  // nullptr on failure.
  static std::unique_ptr<MediaCodecBridge> CreateAudioDecoder(
      const AudioDecoderConfig& config,
      const base::android::JavaRef<jobject>& media_crypto);

  ~MediaCodecBridgeImpl() override;

  // MediaCodecBridge implementation.
  void Stop() override;
  MediaCodecStatus Flush() override;
  MediaCodecStatus GetOutputSize(gfx::Size* size) override;
  MediaCodecStatus GetOutputSamplingRate(int* sampling_rate) override;
  MediaCodecStatus GetOutputChannelCount(int* channel_count) override;
  MediaCodecStatus QueueInputBuffer(int index,
                                    const uint8_t* data,
                                    size_t data_size,
                                    base::TimeDelta presentation_time) override;
  MediaCodecStatus QueueSecureInputBuffer(
      int index,
      const uint8_t* data,
      size_t data_size,
      const std::string& key_id,
      const std::string& iv,
      const std::vector<SubsampleEntry>& subsamples,
      const EncryptionScheme& encryption_scheme,
      base::TimeDelta presentation_time) override;
  void QueueEOS(int input_buffer_index) override;
  MediaCodecStatus DequeueInputBuffer(base::TimeDelta timeout,
                                      int* index) override;
  MediaCodecStatus DequeueOutputBuffer(base::TimeDelta timeout,
                                       int* index,
                                       size_t* offset,
                                       size_t* size,
                                       base::TimeDelta* presentation_time,
                                       bool* end_of_stream,
                                       bool* key_frame) override;

  void ReleaseOutputBuffer(int index, bool render) override;
  MediaCodecStatus GetInputBuffer(int input_buffer_index,
                                  uint8_t** data,
                                  size_t* capacity) override;
  MediaCodecStatus CopyFromOutputBuffer(int index,
                                        size_t offset,
                                        void* dst,
                                        size_t num) override;
  std::string GetName() override;
  bool SetSurface(const base::android::JavaRef<jobject>& surface) override;
  void SetVideoBitrate(int bps, int frame_rate) override;
  void RequestKeyFrameSoon() override;

 private:
  MediaCodecBridgeImpl(base::android::ScopedJavaGlobalRef<jobject> j_bridge);

  // Fills the given input buffer. Returns false if |data_size| exceeds the
  // input buffer's capacity (and doesn't touch the input buffer in that case).
  bool FillInputBuffer(int index,
                       const uint8_t* data,
                       size_t data_size) WARN_UNUSED_RESULT;

  // Gets the address of the data in the given output buffer given by |index|
  // and |offset|. The number of bytes available to read is written to
  // |*capacity| and the address is written to |*addr|. Returns
  // MEDIA_CODEC_ERROR if an error occurs, or MEDIA_CODEC_OK otherwise.
  MediaCodecStatus GetOutputBufferAddress(int index,
                                          size_t offset,
                                          const uint8_t** addr,
                                          size_t* capacity);

  // The Java MediaCodecBridge instance.
  base::android::ScopedJavaGlobalRef<jobject> j_bridge_;

  DISALLOW_COPY_AND_ASSIGN(MediaCodecBridgeImpl);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_MEDIA_CODEC_BRIDGE_IMPL_H_
