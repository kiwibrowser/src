/*
 *  Copyright 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// FakePeriodicVideoCapturer implements a fake cricket::VideoCapturer that
// creates video frames periodically after it has been started.

#ifndef PC_TEST_FAKEPERIODICVIDEOCAPTURER_H_
#define PC_TEST_FAKEPERIODICVIDEOCAPTURER_H_

#include <memory>
#include <vector>

#include "media/base/fakevideocapturer.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/thread_checker.h"

namespace webrtc {

class FakePeriodicVideoCapturer
    : public cricket::FakeVideoCapturerWithTaskQueue {
 public:
  FakePeriodicVideoCapturer() {
    worker_thread_checker_.DetachFromThread();
    using cricket::VideoFormat;
    static const VideoFormat formats[] = {
        {1280, 720, VideoFormat::FpsToInterval(30), cricket::FOURCC_I420},
        {640, 480, VideoFormat::FpsToInterval(30), cricket::FOURCC_I420},
        {640, 360, VideoFormat::FpsToInterval(30), cricket::FOURCC_I420},
        {320, 240, VideoFormat::FpsToInterval(30), cricket::FOURCC_I420},
        {160, 120, VideoFormat::FpsToInterval(30), cricket::FOURCC_I420},
    };

    ResetSupportedFormats({&formats[0], &formats[arraysize(formats)]});
  }

  ~FakePeriodicVideoCapturer() override {
    RTC_DCHECK(main_thread_checker_.CalledOnValidThread());
    StopFrameDelivery();
  }

  // Workaround method for tests to allow stopping frame delivery directly.
  // The |Start()| method expects to be called from the "worker thread" that
  // is owned by the factory (think OrtcFactoryInterface or
  // PeerConnectionFactoryInterface). That thread is not directly exposed,
  // which can make it tricky for tests to inject calls on.
  // So, in order to allow tests to stop frame delivery directly from the
  // test thread, we expose this method publicly.
  void StopFrameDelivery() {
    task_queue_.SendTask([this]() {
      RTC_DCHECK_RUN_ON(&task_queue_);
      deliver_frames_ = false;
    });
  }

 private:
  cricket::CaptureState Start(const cricket::VideoFormat& format) override {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    cricket::CaptureState state = FakeVideoCapturer::Start(format);
    if (state != cricket::CS_FAILED) {
      task_queue_.PostTask([this]() {
        RTC_DCHECK_RUN_ON(&task_queue_);
        deliver_frames_ = true;
        DeliverFrame();
      });
    }
    return state;
  }

  void Stop() override {
    RTC_DCHECK(worker_thread_checker_.CalledOnValidThread());
    StopFrameDelivery();
  }

  void DeliverFrame() {
    RTC_DCHECK_RUN_ON(&task_queue_);
    if (IsRunning() && deliver_frames_) {
      CaptureFrame();
      task_queue_.PostDelayedTask(
          [this]() { DeliverFrame(); },
          static_cast<int>(GetCaptureFormat()->interval /
                           rtc::kNumNanosecsPerMillisec));
    }
  }

  rtc::ThreadChecker main_thread_checker_;
  rtc::ThreadChecker worker_thread_checker_;
  bool deliver_frames_ RTC_GUARDED_BY(task_queue_) = false;
};

}  // namespace webrtc

#endif  //  PC_TEST_FAKEPERIODICVIDEOCAPTURER_H_
