# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from core import path_util
from devil.android.sdk import intent  # pylint: disable=import-error
path_util.AddAndroidPylibToPath()
from pylib.utils import shared_preference_utils
from telemetry.core import android_platform
from telemetry.core import platform
from telemetry.core import util
from telemetry.internal.platform import android_device
from telemetry.page import shared_page_state


CARDBOARD_PATH = os.path.join('chrome', 'android', 'shared_preference_files',
                              'test', 'vr_cardboard_skipdon_setupcomplete.json')
FAKE_TRACKER_COMPONENT = ('com.google.vr.vrcore/'
                          '.tracking.HeadTrackingService')
SUPPORTED_POSE_TRACKER_MODES = [
    'frozen',          # Static pose looking straight forward.
    'sweep',           # Moves head back and forth horizontally.
    'rotate',          # Moves head continuously in a circle.
    'circle_strafe',   # Moves head continuously in a circle (also changes
                       # position if 6DoF supported?).
    'motion_sickness', # Moves head in a sort of figure-eight pattern.
]
SUPPORTED_POSE_TRACKER_TYPES = [
    'sensor',    # Standard sensor-fusion-based pose tracker.
    'tango',     # Tango-based pose tracker.
    'platform',  # ?
    'fake',      # Fake pose tracker that can provide pre-defined pose sets.
]


class SharedAndroidVrPageState(shared_page_state.SharedPageState):
  """SharedPageState for VR Telemetry tests.

  Performs the same functionality as SharedPageState, but with three main
  differences:
  1. It is currently restricted to Android
  2. It performs VR-specific setup such as installing and configuring
     additional APKs that are necessary for testing
  3. It cycles the screen off then on before each story, similar to how
     AndroidScreenRestorationSharedState ensures that the screen is on. See
     _CycleScreen() for an explanation on the reasoning behind this.
  """
  def __init__(self, test, finder_options, story_set):
    # TODO(bsheedy): See about making this a cross-platform SharedVrPageState -
    # Seems like we should be able to use SharedPageState's default platform
    # property instead of specifying AndroidPlatform, and then just perform
    # different setup based off the platform type
    device = android_device.GetDevice(finder_options)
    assert device, 'Android device is required for this story'
    self._platform = platform.GetPlatformForDevice(device, finder_options)
    assert self._platform, 'Unable to create Android platform'
    assert isinstance(self._platform, android_platform.AndroidPlatform)

    super(SharedAndroidVrPageState, self).__init__(test, finder_options,
                                                   story_set)
    self._story_set = story_set
    # Optimization so we're not doing redundant service starts before every
    # story.
    self._did_set_tracker = False
    self._PerformAndroidVrSetup()

  def _PerformAndroidVrSetup(self):
    self._InstallVrCore()
    self._ConfigureVrCore(os.path.join(path_util.GetChromiumSrcDir(),
                                       self._finder_options.shared_prefs_file))
    self._InstallNfcApk()
    self._InstallKeyboardApk()

  def _InstallVrCore(self):
    """Installs the VrCore APK."""
    # TODO(bsheedy): Add support for temporarily replacing it if it's still
    # installed as a system app on the test device
    self._platform.InstallApplication(
        os.path.join(path_util.GetChromiumSrcDir(), 'third_party',
                     'gvr-android-sdk', 'test-apks', 'vr_services',
                     'vr_services_current.apk'))

  def _ConfigureVrCore(self, filepath):
    """Configures VrCore using the provided settings file."""
    settings = shared_preference_utils.ExtractSettingsFromJson(filepath)
    for setting in settings:
      shared_pref = self._platform.GetSharedPrefs(
          setting['package'], setting['filename'],
          use_encrypted_path=setting.get('supports_encrypted_path', False))
      shared_preference_utils.ApplySharedPreferenceSetting(
          shared_pref, setting)

  def _InstallNfcApk(self):
    """Installs the APK that allows VR tests to simulate a headset NFC scan."""
    chromium_root = path_util.GetChromiumSrcDir()
    # Find the most recently build APK
    candidate_apks = []
    for build_path in util.GetBuildDirectories(chromium_root):
      apk_path = os.path.join(build_path, 'apks', 'VrNfcSimulator.apk')
      if os.path.exists(apk_path):
        last_changed = os.path.getmtime(apk_path)
        candidate_apks.append((last_changed, apk_path))

    if not candidate_apks:
      raise RuntimeError(
          'Could not find VrNfcSimulator.apk in a build output directory')
    newest_apk_path = sorted(candidate_apks)[-1][1]
    self._platform.InstallApplication(
        os.path.join(chromium_root, newest_apk_path))

  def _InstallKeyboardApk(self):
    """Installs the VR Keyboard APK."""
    self._platform.InstallApplication(
        os.path.join(path_util.GetChromiumSrcDir(), 'third_party',
                     'gvr-android-sdk', 'test-apks', 'vr_keyboard',
                     'vr_keyboard_current.apk'))

  def _SetFakePoseTrackerIfNotSet(self):
    if self._story_set.use_fake_pose_tracker and not self._did_set_tracker:
      self.SetPoseTrackerType('fake')
      self.SetPoseTrackerMode('sweep')
      self._did_set_tracker = True

  def SetPoseTrackerType(self, tracker_type):
    """Sets the VrCore pose tracker to the given type.

    Only works if VrCore has been configured to use the VrCore-side tracker
    by setting EnableVrCoreHeadTracking to true. This setting persists between
    VR sessions and Chrome restarts.

    Args:
      tracker_type: A string corresponding to the tracker type to set.

    Raises:
      RuntimeError if the given |tracker_type| is not in the supported list.
    """
    if tracker_type not in SUPPORTED_POSE_TRACKER_TYPES:
      raise RuntimeError('Given tracker %s is not supported.' % tracker_type)
    self.platform.StartAndroidService(start_intent=intent.Intent(
        action='com.google.vr.vrcore.SET_TRACKER_TYPE',
        component=FAKE_TRACKER_COMPONENT,
        extras={'com.google.vr.vrcore.TRACKER_TYPE': tracker_type}))

  def SetPoseTrackerMode(self, tracker_mode):
    """Sets the fake VrCore pose tracker to provide poses in the given mode.

    Only works after SetPoseTrackerType has been set to 'fake'. This setting
    persists between VR sessions and Chrome restarts.

    Args:
      tracker_mode: A string corresponding to the tracker mode to set.

    Raises:
      RuntimeError if the given |tracker_mode| is not in the supported list.
    """
    if tracker_mode not in SUPPORTED_POSE_TRACKER_MODES:
      raise RuntimeError('Given mode %s is not supported.' % tracker_mode)
    self.platform.StartAndroidService(start_intent=intent.Intent(
        action='com.google.vr.vrcore.SET_FAKE_TRACKER_MODE',
        component=FAKE_TRACKER_COMPONENT,
        extras={'com.google.vr.vrcore.FAKE_TRACKER_MODE': tracker_mode}))

  def WillRunStory(self, page):
    super(SharedAndroidVrPageState, self).WillRunStory(page)
    if not self._finder_options.disable_screen_reset:
      self._CycleScreen()
    self._SetFakePoseTrackerIfNotSet()

  def TearDownState(self):
    super(SharedAndroidVrPageState, self).TearDownState()
    # Reset the tracker type to use the actual sensor if it's been changed. When
    # run on the bots, this shouldn't matter since the service will be killed
    # during the automatic restart, but this could persist when run locally.
    if self._did_set_tracker:
      self.SetPoseTrackerType('sensor')
    # Re-apply Cardboard as the viewer to leave the device in a consistent
    # state after a benchmark run
    # TODO(bsheedy): Remove this after crbug.com/772969 is fixed
    self._ConfigureVrCore(os.path.join(path_util.GetChromiumSrcDir(),
                                       CARDBOARD_PATH))

  def _CycleScreen(self):
    """Cycles the screen off then on.

    This is because VR test devices are set to have normal screen brightness and
    automatically turn off after several minutes instead of the usual approach
    of having the screen always on at minimum brightness. This is due to the
    motion-to-photon latency test being sensitive to screen brightness, and min
    brightness does not work well for it.

    Simply using TurnScreenOn does not actually reset the timer for turning off
    the screen, so instead cycle the screen to refresh it periodically.
    """
    self.platform.android_action_runner.TurnScreenOff()
    self.platform.android_action_runner.TurnScreenOn()

  @property
  def platform(self):
    return self._platform

  @property
  def recording_wpr(self):
    return self._finder_options.recording_wpr
