// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_ANDROID_AUDIO_RECORD_INPUT_H_
#define MEDIA_AUDIO_ANDROID_AUDIO_RECORD_INPUT_H_

#include <stdint.h>

#include <memory>

#include "base/android/jni_android.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "media/audio/audio_io.h"
#include "media/base/audio_parameters.h"

namespace media {

class AudioBus;
class AudioManagerAndroid;

// Implements PCM audio input support for Android using the Java AudioRecord
// interface. Most of the work is done by its Java counterpart in
// AudioRecordInput.java. This class is created and lives on the Audio Manager
// thread but recorded audio buffers are delivered on a thread managed by
// the Java class.
class MEDIA_EXPORT AudioRecordInputStream : public AudioInputStream {
 public:
  AudioRecordInputStream(AudioManagerAndroid* manager,
                         const AudioParameters& params);

  ~AudioRecordInputStream() override;

  // Implementation of AudioInputStream.
  bool Open() override;
  void Start(AudioInputCallback* callback) override;
  void Stop() override;
  void Close() override;
  double GetMaxVolume() override;
  void SetVolume(double volume) override;
  double GetVolume() override;
  bool SetAutomaticGainControl(bool enabled) override;
  bool GetAutomaticGainControl() override;
  bool IsMuted() override;
  void SetOutputDeviceForAec(const std::string& output_device_id) override;

  // Called from Java when data is available.
  void OnData(JNIEnv* env,
              const base::android::JavaParamRef<jobject>& obj,
              jint size,
              jint hardware_delay_ms);

  // Called from Java so that we can cache the address of the Java-managed
  // |byte_buffer| in |direct_buffer_address_|.
  void CacheDirectBufferAddress(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& byte_buffer);

 private:
  base::ThreadChecker thread_checker_;
  AudioManagerAndroid* audio_manager_;

  // Java AudioRecordInput instance.
  base::android::ScopedJavaGlobalRef<jobject> j_audio_record_;

  // This is the only member accessed by both the Audio Manager and Java
  // threads. Explanations for why we do not require explicit synchronization
  // are given in the implementation.
  AudioInputCallback* callback_;

  // Owned by j_audio_record_.
  uint8_t* direct_buffer_address_;

  std::unique_ptr<media::AudioBus> audio_bus_;
  int bytes_per_sample_;

  DISALLOW_COPY_AND_ASSIGN(AudioRecordInputStream);
};

}  // namespace media

#endif  // MEDIA_AUDIO_ANDROID_AUDIO_RECORD_INPUT_H_
