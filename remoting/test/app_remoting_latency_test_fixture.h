// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_APP_REMOTING_LATENCY_TEST_FIXTURE_H_
#define REMOTING_TEST_APP_REMOTING_LATENCY_TEST_FIXTURE_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "remoting/test/remote_connection_observer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_geometry.h"
#include "ui/events/keycodes/dom/dom_code.h"

namespace base {
class RunLoop;
class Timer;
class TimeDelta;
}

namespace webrtc {
class DesktopRect;
}

namespace remoting {
namespace test {

struct RemoteApplicationDetails;
struct RGBValue;
class AppRemotingConnectionHelper;
class TestVideoRenderer;

// Allows for custom handling of ExtensionMessage messages.
typedef base::Callback<void(const protocol::ExtensionMessage& message)>
    HostMessageReceivedCallback;

// Called to wait for expected image pattern to be matched within up to a max
// wait time.
typedef base::Callback<bool(const base::TimeDelta& max_wait_time)>
    WaitForImagePatternMatchCallback;

// Creates a connection to a remote host which is available for tests to use.
// Provides convenient methods to create test cases to measure the input and
// rendering latency between client and the remote host.
// NOTE: This is an abstract class. To use it, please derive from this class
// and implement GetApplicationDetails to specify the application details.
class AppRemotingLatencyTestFixture : public testing::Test,
                                      public RemoteConnectionObserver {
 public:
  AppRemotingLatencyTestFixture();
  ~AppRemotingLatencyTestFixture() override;

 protected:
  // Set expected image pattern for comparison.
  // A WaitForImagePatternMatchCallback is returned to allow waiting for the
  // expected image pattern to be matched.
  WaitForImagePatternMatchCallback SetExpectedImagePattern(
      const webrtc::DesktopRect& expected_rect,
      const RGBValue& expected_avg_color);

  // Turn on/off saving video frames to disk.
  void SaveFrameDataToDisk(bool save_frame_data_to_disk);

  // Inject press & release key event.
  void PressAndReleaseKey(ui::DomCode usb_keycode);

  // Inject press & release a combination of key events.
  void PressAndReleaseKeyCombination(
      const std::vector<ui::DomCode>& usb_keycodes);

  // Setter for |host_message_received_callback_|.
  void SetHostMessageReceivedCallback(
      const HostMessageReceivedCallback& host_message_received_callback);

  // Reset |host_message_received_callback_| to null.
  void ResetHostMessageReceivedCallback();

  // Get the details of the application to be run.
  virtual const RemoteApplicationDetails& GetApplicationDetails() = 0;

  // Used to ensure the application under test is ready for testing.
  virtual bool PrepareApplicationForTesting() = 0;

  // Clean up the running application to initial state.
  virtual void ResetApplicationState();

  // Creates and manages the connection to the remote host.
  std::unique_ptr<AppRemotingConnectionHelper> connection_helper_;

 private:
  // testing::Test interface.
  void SetUp() override;
  void TearDown() override;

  // RemoteConnectionObserver interface.
  void HostMessageReceived(const protocol::ExtensionMessage& message) override;

  // Inject press key event.
  void PressKey(ui::DomCode usb_keycode, bool pressed);

  // Waits for an image pattern matched reply up to |max_wait_time|. Returns
  // true if we received a response within the maximum time limit.
  // NOTE: This method should only be run when as a returned callback by
  // SetExpectedImagePattern.
  bool WaitForImagePatternMatch(std::unique_ptr<base::RunLoop> run_loop,
                                const base::TimeDelta& max_wait_time);

  // Used to run the thread's message loop.
  std::unique_ptr<base::RunLoop> run_loop_;

  // Used for setting timeouts and delays.
  std::unique_ptr<base::Timer> timer_;

  // Used to maintain a reference to the TestVideoRenderer instance while it
  // exists.
  base::WeakPtr<TestVideoRenderer> test_video_renderer_;

  // Called when an ExtensionMessage is received from the host.  Used to
  // override default message handling.
  HostMessageReceivedCallback host_message_received_callback_;

  DISALLOW_COPY_AND_ASSIGN(AppRemotingLatencyTestFixture);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_APP_REMOTING_LATENCY_TEST_FIXTURE_H_
