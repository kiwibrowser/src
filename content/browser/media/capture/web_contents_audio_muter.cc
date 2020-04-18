// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/web_contents_audio_muter.h"

#include <memory>
#include <set>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "content/browser/media/capture/audio_mirroring_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "media/audio/audio_io.h"
#include "media/audio/audio_manager.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/fake_audio_worker.h"

namespace content {

namespace {

// An AudioOutputStream that pumps audio data, but does nothing with it.
// Pumping the audio data is necessary because video playback is synchronized to
// the audio stream and will freeze otherwise.
//
// TODO(miu): media::FakeAudioOutputStream does pretty much the same thing as
// this class, but requires construction/destruction via media::AudioManagerBase
// on the audio thread.  Once that's fixed, this class will no longer be needed.
// http://crbug.com/416278
class AudioDiscarder : public media::AudioOutputStream {
 public:
  explicit AudioDiscarder(const media::AudioParameters& params)
      : worker_(media::AudioManager::Get()->GetWorkerTaskRunner(), params),
        audio_bus_(media::AudioBus::Create(params)) {}

  // AudioOutputStream implementation.
  bool Open() override { return true; }
  void Start(AudioSourceCallback* callback) override {
    worker_.Start(base::Bind(&AudioDiscarder::FetchAudioData,
                             base::Unretained(this),
                             callback));
  }
  void Stop() override { worker_.Stop(); }
  void SetVolume(double volume) override {}
  void GetVolume(double* volume) override { *volume = 0; }
  void Close() override { delete this; }

 private:
  ~AudioDiscarder() override {}

  void FetchAudioData(AudioSourceCallback* callback) {
    callback->OnMoreData(base::TimeDelta(), base::TimeTicks::Now(), 0,
                         audio_bus_.get());
  }

  // Calls FetchAudioData() at regular intervals and discards the data.
  media::FakeAudioWorker worker_;
  std::unique_ptr<media::AudioBus> audio_bus_;

  DISALLOW_COPY_AND_ASSIGN(AudioDiscarder);
};

}  // namespace

// A simple AudioMirroringManager::MirroringDestination implementation that
// identifies the audio streams rendered by a WebContents and provides
// AudioDiscarders to AudioMirroringManager.
class WebContentsAudioMuter::MuteDestination
    : public base::RefCountedThreadSafe<MuteDestination>,
      public AudioMirroringManager::MirroringDestination {
 public:
  explicit MuteDestination(WebContents* web_contents)
      : web_contents_(web_contents) {}

 private:
  friend class base::RefCountedThreadSafe<MuteDestination>;

  typedef AudioMirroringManager::SourceFrameRef SourceFrameRef;

  ~MuteDestination() override {}

  void QueryForMatches(const std::set<SourceFrameRef>& candidates,
                       const MatchesCallback& results_callback) override {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&MuteDestination::QueryForMatchesOnUIThread, this,
                       candidates, media::BindToCurrentLoop(results_callback)));
  }

  void QueryForMatchesOnUIThread(const std::set<SourceFrameRef>& candidates,
                                 const MatchesCallback& results_callback) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    std::set<SourceFrameRef> matches;
    // Add each ID to |matches| if it maps to a RenderFrameHost that maps to the
    // WebContents being muted.
    for (std::set<SourceFrameRef>::const_iterator i = candidates.begin();
         i != candidates.end(); ++i) {
      WebContents* const contents_containing_frame =
          WebContents::FromRenderFrameHost(
              RenderFrameHost::FromID(i->first, i->second));
      if (contents_containing_frame == web_contents_)
        matches.insert(*i);
    }
    results_callback.Run(matches, false);
  }

  media::AudioOutputStream* AddInput(
      const media::AudioParameters& params) override {
    return new AudioDiscarder(params);
  }

  media::AudioPushSink* AddPushInput(
      const media::AudioParameters& params) override {
    NOTREACHED();
    return nullptr;
  }

  WebContents* const web_contents_;

  DISALLOW_COPY_AND_ASSIGN(MuteDestination);
};

WebContentsAudioMuter::WebContentsAudioMuter(WebContents* web_contents)
    : destination_(new MuteDestination(web_contents)), is_muting_(false) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

WebContentsAudioMuter::~WebContentsAudioMuter() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  StopMuting();
}

void WebContentsAudioMuter::StartMuting() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (is_muting_)
    return;
  is_muting_ = true;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&AudioMirroringManager::StartMirroring,
                     base::Unretained(AudioMirroringManager::GetInstance()),
                     base::RetainedRef(destination_)));
}

void WebContentsAudioMuter::StopMuting() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!is_muting_)
    return;
  is_muting_ = false;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&AudioMirroringManager::StopMirroring,
                     base::Unretained(AudioMirroringManager::GetInstance()),
                     base::RetainedRef(destination_)));
}

}  // namespace content
