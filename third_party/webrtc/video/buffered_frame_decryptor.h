/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VIDEO_BUFFERED_FRAME_DECRYPTOR_H_
#define VIDEO_BUFFERED_FRAME_DECRYPTOR_H_

#include <deque>
#include <memory>

#include "api/crypto/crypto_options.h"
#include "api/crypto/frame_decryptor_interface.h"
#include "modules/include/module_common_types.h"
#include "modules/video_coding/frame_object.h"

namespace webrtc {

// This callback is provided during the construction of the
// BufferedFrameDecryptor and is called each time a frame is sucessfully
// decrypted by the buffer.
class OnDecryptedFrameCallback {
 public:
  virtual ~OnDecryptedFrameCallback() = default;
  // Called each time a decrypted frame is returned.
  virtual void OnDecryptedFrame(
      std::unique_ptr<video_coding::RtpFrameObject> frame) = 0;
};

// The BufferedFrameDecryptor is responsible for deciding when to pass
// decrypted received frames onto the OnDecryptedFrameCallback. Frames can be
// delayed when frame encryption is enabled but the key hasn't arrived yet. In
// this case we stash about 1 second of encrypted frames instead of dropping
// them to prevent re-requesting the key frame. This optimization is
// particularly important on low bandwidth networks. Note stashing is only ever
// done if we have never sucessfully decrypted a frame before. After the first
// successful decryption payloads will never be stashed.
class BufferedFrameDecryptor final {
 public:
  // Constructs a new BufferedFrameDecryptor that can hold
  explicit BufferedFrameDecryptor(
      OnDecryptedFrameCallback* decrypted_frame_callback,
      rtc::scoped_refptr<FrameDecryptorInterface> frame_decryptor);
  ~BufferedFrameDecryptor();
  // This object cannot be copied.
  BufferedFrameDecryptor(const BufferedFrameDecryptor&) = delete;
  BufferedFrameDecryptor& operator=(const BufferedFrameDecryptor&) = delete;
  // Determines whether the frame should be stashed, dropped or handed off to
  // the OnDecryptedFrameCallback.
  void ManageEncryptedFrame(
      std::unique_ptr<video_coding::RtpFrameObject> encrypted_frame);

 private:
  // Represents what should be done with a given frame.
  enum class FrameDecision { kStash, kDecrypted, kDrop };

  // Attempts to decrypt the frame, if it fails and no prior frames have been
  // decrypted it will return kStash. Otherwise fail to decrypts will return
  // kDrop. Successful decryptions will always return kDecrypted.
  FrameDecision DecryptFrame(video_coding::RtpFrameObject* frame);
  // Retries all the stashed frames this is triggered each time a kDecrypted
  // event occurs.
  void RetryStashedFrames();

  static const size_t kMaxStashedFrames = 24;

  const bool generic_descriptor_auth_experiment_;
  bool first_frame_decrypted_ = false;
  const rtc::scoped_refptr<FrameDecryptorInterface> frame_decryptor_;
  OnDecryptedFrameCallback* const decrypted_frame_callback_;
  std::deque<std::unique_ptr<video_coding::RtpFrameObject>> stashed_frames_;
};

}  // namespace webrtc

#endif  // VIDEO_BUFFERED_FRAME_DECRYPTOR_H_
