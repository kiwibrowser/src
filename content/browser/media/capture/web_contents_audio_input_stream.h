// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// An AudioInputStream which provides a loop-back of all audio output generated
// by the entire RenderFrame tree associated with a WebContents instance.  The
// single stream of data is produced by format-converting and mixing all audio
// output streams.  As the RenderFrameHost tree mutates (e.g., due to page
// navigations, or crashes/reloads), the stream will continue without
// interruption.  In other words, WebContentsAudioInputStream provides tab-level
// audio mirroring.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_WEB_CONTENTS_AUDIO_INPUT_STREAM_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_WEB_CONTENTS_AUDIO_INPUT_STREAM_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "media/audio/audio_io.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {
class AudioParameters;
class VirtualAudioInputStream;
}

namespace content {

class AudioMirroringManager;
class WebContentsTracker;

class CONTENT_EXPORT WebContentsAudioInputStream
    : public media::AudioInputStream {
 public:
  // media::AudioInputStream implementation
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

  // Create a new audio mirroring session, or return NULL on error.  |device_id|
  // should be in the format accepted by
  // WebContentsCaptureUtil::ExtractTabCaptureTarget().  The caller must
  // guarantee Close() is called on the returned object so that it may
  // self-destruct.
  // |worker_task_runner| is the task runner on which AudioInputCallback methods
  // are called and may or may not be the single thread that invokes the
  // AudioInputStream methods.
  static WebContentsAudioInputStream* Create(
      const std::string& device_id,
      const media::AudioParameters& params,
      const scoped_refptr<base::SingleThreadTaskRunner>& worker_task_runner,
      AudioMirroringManager* audio_mirroring_manager);

 private:
  friend class WebContentsAudioInputStreamTest;

  // Maintain most state and functionality in an internal ref-counted
  // implementation class.  This object must outlive a call to Close(), until
  // the shutdown tasks running on other threads complete: The
  // AudioMirroringManager on the IO thread, the WebContentsTracker on the UI
  // thread, and the VirtualAudioOuputStreams on the audio thread.
  class Impl;

  WebContentsAudioInputStream(int render_process_id,
                              int main_render_frame_id,
                              AudioMirroringManager* mirroring_manager,
                              const scoped_refptr<WebContentsTracker>& tracker,
                              media::VirtualAudioInputStream* mixer_stream,
                              bool is_duplication);

  ~WebContentsAudioInputStream() override;

  scoped_refptr<Impl> impl_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsAudioInputStream);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_WEB_CONTENTS_AUDIO_INPUT_STREAM_H_
