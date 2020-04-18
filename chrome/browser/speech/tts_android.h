// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPEECH_TTS_ANDROID_H_
#define CHROME_BROWSER_SPEECH_TTS_ANDROID_H_

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/speech/tts_platform.h"

class TtsPlatformImplAndroid : public TtsPlatformImpl {
 public:
  // TtsPlatformImpl implementation.
  bool PlatformImplAvailable() override;
  bool Speak(int utterance_id,
             const std::string& utterance,
             const std::string& lang,
             const VoiceData& voice,
             const UtteranceContinuousParameters& params) override;
  bool StopSpeaking() override;
  void Pause() override;
  void Resume() override;
  bool IsSpeaking() override;
  void GetVoices(std::vector<VoiceData>* out_voices) override;

  // Methods called from Java via JNI.
  void VoicesChanged(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& obj);
  void OnEndEvent(JNIEnv* env,
                  const base::android::JavaParamRef<jobject>& obj,
                  jint utterance_id);
  void OnErrorEvent(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj,
                    jint utterance_id);
  void OnStartEvent(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj,
                    jint utterance_id);

  // Static functions.
  static TtsPlatformImplAndroid* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<TtsPlatformImplAndroid>;

  TtsPlatformImplAndroid();
  ~TtsPlatformImplAndroid() override;

  void SendFinalTtsEvent(
      int utterance_id, TtsEventType event_type, int char_index);

  base::android::ScopedJavaGlobalRef<jobject> java_ref_;
  int utterance_id_;
  std::string utterance_;

  DISALLOW_COPY_AND_ASSIGN(TtsPlatformImplAndroid);
};

#endif  // CHROME_BROWSER_SPEECH_TTS_ANDROID_H_
