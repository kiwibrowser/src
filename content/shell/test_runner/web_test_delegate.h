// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_WEB_TEST_DELEGATE_H_
#define CONTENT_SHELL_TEST_RUNNER_WEB_TEST_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "third_party/blink/public/common/screen_orientation/web_screen_orientation_type.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_vector.h"

#define WEBTESTRUNNER_NEW_HISTORY_CAPTURE

namespace base {
class DictionaryValue;
}

namespace blink {
struct Manifest;
class WebInputEvent;
class WebLocalFrame;
class WebMediaStream;
class WebPlugin;
struct WebPluginParams;
struct WebSize;
class WebURLRequest;
class WebView;
}

namespace device {
class MotionData;
class OrientationData;
}

namespace test_runner {

class GamepadController;
class WebWidgetTestProxyBase;
struct TestPreferences;

constexpr int kDefaultDatabaseQuota = -1;

class WebTestDelegate {
 public:
  // Set and clear the edit command to execute on the next call to
  // WebViewClient::handleCurrentKeyboardEvent().
  virtual void ClearEditCommand() = 0;
  virtual void SetEditCommand(const std::string& name,
                              const std::string& value) = 0;

  // Sets gamepad provider to be used for tests.
  virtual void SetGamepadProvider(GamepadController* controller) = 0;

  // Set data to return when registering via
  // Platform::setDeviceMotionListener().
  virtual void SetDeviceMotionData(const device::MotionData& data) = 0;
  // Set data to return when registering via
  // Platform::setDeviceOrientationListener().
  virtual void SetDeviceOrientationData(
      const device::OrientationData& data) = 0;

  // Add a message to stderr (not saved to expected output files, for debugging
  // only).
  virtual void PrintMessageToStderr(const std::string& message) = 0;

  // Add a message to the text dump for the layout test.
  virtual void PrintMessage(const std::string& message) = 0;

  virtual void PostTask(base::OnceClosure task) = 0;
  virtual void PostDelayedTask(base::OnceClosure task,
                               base::TimeDelta delay) = 0;

  // Register a new isolated filesystem with the given files, and return the
  // new filesystem id.
  virtual blink::WebString RegisterIsolatedFileSystem(
      const blink::WebVector<blink::WebString>& absolute_filenames) = 0;

  // Gets the current time in milliseconds since the UNIX epoch.
  virtual long long GetCurrentTimeInMillisecond() = 0;

  // Convert the provided relative path into an absolute path.
  virtual blink::WebString GetAbsoluteWebStringFromUTF8Path(
      const std::string& path) = 0;

  // Reads in the given file and returns its contents as data URL.
  virtual blink::WebURL LocalFileToDataURL(const blink::WebURL& file_url) = 0;

  // Replaces file:///tmp/LayoutTests/ with the actual path to the
  // LayoutTests directory, or rewrite URLs generated from absolute
  // path links in web-platform-tests.
  virtual blink::WebURL RewriteLayoutTestsURL(const std::string& utf8_url,
                                              bool is_wpt_mode) = 0;

  // Manages the settings to used for layout tests.
  virtual TestPreferences* Preferences() = 0;
  virtual void ApplyPreferences() = 0;
  virtual void SetPopupBlockingEnabled(bool block_popups) = 0;

  // Enables or disables synchronous resize mode. When enabled, all
  // window-sizing machinery is
  // short-circuited inside the renderer. This mode is necessary for some tests
  // that were written
  // before browsers had multi-process architecture and rely on window resizes
  // to happen synchronously.
  // The function has "unfortunate" it its name because we must strive to remove
  // all tests
  // that rely on this... well, unfortunate behavior. See
  // http://crbug.com/309760 for the plan.
  virtual void UseUnfortunateSynchronousResizeMode(bool enable) = 0;

  // Controls auto resize mode.
  virtual void EnableAutoResizeMode(const blink::WebSize& min_size,
                                    const blink::WebSize& max_size) = 0;
  virtual void DisableAutoResizeMode(const blink::WebSize& new_size) = 0;

  virtual void NavigateSecondaryWindow(const GURL& url) = 0;
  virtual void InspectSecondaryWindow() = 0;

  // Controls WebSQL databases.
  virtual void ClearAllDatabases() = 0;
  // Setting quota to kDefaultDatabaseQuota will reset it to the default value.
  virtual void SetDatabaseQuota(int quota) = 0;

  // Controls Web Notifications.
  virtual void SimulateWebNotificationClick(
      const std::string& title,
      const base::Optional<int>& action_index,
      const base::Optional<base::string16>& reply) = 0;
  virtual void SimulateWebNotificationClose(const std::string& title,
                                            bool by_user) = 0;

  // Controls the device scale factor of the main WebView for hidpi tests.
  virtual void SetDeviceScaleFactor(float factor) = 0;

  // When use-zoom-for-dsf mode is enabled, this returns the scale to
  // convert from window coordinates to viewport coordinates. When
  // use-zoom-for-dsf is disabled, this return always 1.0f.
  virtual float GetWindowToViewportScale() = 0;

  // Converts |event| from screen coordinates used by test_runner::EventSender
  // into coordinates that are understood by the widget associated with
  // |web_widget_test_proxy_base|.  Returns nullptr if no transformation was
  // necessary (e.g. for a keyboard event OR if widget requires no scaling
  // and has coordinates starting at (0,0)).
  virtual std::unique_ptr<blink::WebInputEvent>
  TransformScreenToWidgetCoordinates(
      test_runner::WebWidgetTestProxyBase* web_widget_test_proxy_base,
      const blink::WebInputEvent& event) = 0;

  // Gets WebWidgetTestProxyBase associated with |frame| (associated with either
  // a RenderView or a RenderWidget for the local root).
  virtual test_runner::WebWidgetTestProxyBase* GetWebWidgetTestProxyBase(
      blink::WebLocalFrame* frame) = 0;

  // Enable zoom-for-dsf option.
  virtual void EnableUseZoomForDSF() = 0;

  // Returns whether or not the use-zoom-for-dsf flag is enabled.
  virtual bool IsUseZoomForDSFEnabled() = 0;

  // Change the device color space while running a layout test.
  virtual void SetDeviceColorSpace(const std::string& name) = 0;

  // Set the bluetooth adapter while running a layout test, uses Mojo to
  // communicate with the browser.
  virtual void SetBluetoothFakeAdapter(const std::string& adapter_name,
                                       base::OnceClosure callback) = 0;

  // If |enable| is true makes the Bluetooth chooser record its input and wait
  // for instructions from the test program on how to proceed. Otherwise
  // fall backs to the browser's default chooser.
  virtual void SetBluetoothManualChooser(bool enable) = 0;

  // Returns the events recorded since the last call to this function.
  virtual void GetBluetoothManualChooserEvents(
      base::OnceCallback<void(const std::vector<std::string>& events)>
          callback) = 0;

  // Calls the BluetoothChooser::EventHandler with the arguments here. Valid
  // event strings are:
  //  * "cancel" - simulates the user canceling the chooser.
  //  * "select" - simulates the user selecting a device whose device ID is in
  //               |argument|.
  virtual void SendBluetoothManualChooserEvent(const std::string& event,
                                               const std::string& argument) = 0;

  // Controls which WebView should be focused.
  virtual void SetFocus(blink::WebView* web_view, bool focus) = 0;

  // Controls whether all cookies should be accepted or writing cookies in a
  // third-party context is blocked.
  virtual void SetBlockThirdPartyCookies(bool block) = 0;

  // The same as RewriteLayoutTestsURL unless the resource is a path starting
  // with /tmp/, then return a file URL to a temporary file.
  virtual std::string PathToLocalResource(const std::string& resource) = 0;

  // Sets the POSIX locale of the current process.
  virtual void SetLocale(const std::string& locale) = 0;

  // Invoked when layout test runtime flags change.
  virtual void OnLayoutTestRuntimeFlagsChanged(
      const base::DictionaryValue& changed_values) = 0;

  // Invoked when the test finished.
  virtual void TestFinished() = 0;

  // Invoked when the embedder should close all but the main WebView.
  virtual void CloseRemainingWindows() = 0;

  virtual void DeleteAllCookies() = 0;

  // Returns the length of the back/forward history of the main WebView.
  virtual int NavigationEntryCount() = 0;

  // The following trigger navigations on the main WebView.
  virtual void GoToOffset(int offset) = 0;
  virtual void Reload() = 0;
  virtual void LoadURLForFrame(const blink::WebURL& url,
                               const std::string& frame_name) = 0;

  // Returns true if resource requests to external URLs should be permitted.
  virtual bool AllowExternalPages() = 0;

  // Fetch the manifest for a given WebView from the given url.
  virtual void FetchManifest(
      blink::WebView* view,
      base::OnceCallback<void(const GURL&, const blink::Manifest&)>
          callback) = 0;

  // Sends a message to the LayoutTestPermissionManager in order for it to
  // update its database.
  virtual void SetPermission(const std::string& permission_name,
                             const std::string& permission_value,
                             const GURL& origin,
                             const GURL& embedding_origin) = 0;

  // Clear all the permissions set via SetPermission().
  virtual void ResetPermissions() = 0;

  // Add content MediaStream classes to the Blink MediaStream ones.
  virtual bool AddMediaStreamVideoSourceAndTrack(
      blink::WebMediaStream* stream) = 0;
  virtual bool AddMediaStreamAudioSourceAndTrack(
      blink::WebMediaStream* stream) = 0;

  // Causes the beforeinstallprompt event to be sent to the renderer.
  // |event_platforms| are the platforms to be sent with the event. Once the
  // event listener completes, |callback| will be called with a boolean
  // argument. This argument will be true if the event is canceled, and false
  // otherwise.
  virtual void DispatchBeforeInstallPromptEvent(
      const std::vector<std::string>& event_platforms,
      base::OnceCallback<void(bool)> callback) = 0;

  // Resolves the in-flight beforeinstallprompt event userChoice promise with a
  // platform of |platform|.
  virtual void ResolveBeforeInstallPromptPromise(
      const std::string& platform) = 0;

  virtual blink::WebPlugin* CreatePluginPlaceholder(
      const blink::WebPluginParams& params) = 0;

  virtual float GetDeviceScaleFactor() const = 0;

  // Run all pending idle tasks, and then run callback.
  virtual void RunIdleTasks(base::OnceClosure callback) = 0;

  // Forces a text input state update for the client of WebFrameWidget
  // associated with |frame|.
  virtual void ForceTextInputStateUpdate(blink::WebLocalFrame* frame) = 0;

  // PlzNavigate
  // Indicates if the navigation was initiated by the browser or renderer.
  virtual bool IsNavigationInitiatedByRenderer(
      const blink::WebURLRequest& request) = 0;

 protected:
  virtual ~WebTestDelegate() {}
};

}  // namespace test_runner

#endif  // CONTENT_SHELL_TEST_RUNNER_WEB_TEST_DELEGATE_H_
