# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import subprocess
import sys
import time

# Add the telemetry directory to Python's search paths.
current_directory = os.path.dirname(os.path.realpath(__file__))
perf_dir = os.path.realpath(
    os.path.join(current_directory, '..', '..', '..', 'tools', 'perf'))
if perf_dir not in sys.path:
  sys.path.append(perf_dir)
from chrome_telemetry_build import chromium_config
telemetry_dir = chromium_config.GetTelemetryDir()
if telemetry_dir not in sys.path:
  sys.path.append(telemetry_dir)

from telemetry.internal.browser import browser_options
from telemetry.internal.browser import browser_finder
from telemetry.core import exceptions
from telemetry.core import util
from telemetry.core import cros_interface
from telemetry.internal.browser import extension_to_load

logger = logging.getLogger('proximity_auth.%s' % __name__)

class AccountPickerScreen(object):
  """ Wrapper for the ChromeOS account picker screen.

  The account picker screen is the WebContents page used for both the lock
  screen and signin screen.

  Note: This class assumes the account picker screen only has one user. If there
  are multiple user pods, the first one will be used.
  """

  class AuthType:
    """ The authentication type expected for a user pod. """
    OFFLINE_PASSWORD = 0
    ONLINE_SIGN_IN = 1
    NUMERIC_PIN = 2
    USER_CLICK = 3
    EXPAND_THEN_USER_CLICK = 4
    FORCE_OFFLINE_PASSWORD = 5

  class SmartLockState:
    """ The state of the Smart Lock icon on a user pod.
    """
    NOT_SHOWN = 'not_shown'
    AUTHENTICATED = 'authenticated'
    LOCKED = 'locked'
    HARD_LOCKED = 'hardlocked'
    TO_BE_ACTIVATED = 'to_be_activated'
    SPINNER = 'spinner'

  # JavaScript expression for getting the user pod on the page
  _GET_POD_JS = 'document.getElementById("pod-row").pods[0]'

  def __init__(self, oobe, chromeos):
    """
    Args:
      oobe: Inspector page of the OOBE WebContents.
      chromeos: The parent Chrome wrapper.
    """
    self._oobe = oobe
    self._chromeos = chromeos

  @property
  def is_lockscreen(self):
    return self._oobe.EvaluateJavaScript(
        '!document.getElementById("sign-out-user-item").hidden')

  @property
  def auth_type(self):
    return self._oobe.EvaluateJavaScript(
        '{{ @pod }}.authType', pod=self._GET_POD_JS)

  @property
  def smart_lock_state(self):
    icon_shown = self._oobe.EvaluateJavaScript(
        '!{{ @pod }}.customIconElement.hidden', pod=self._GET_POD_JS)
    if not icon_shown:
      return self.SmartLockState.NOT_SHOWN
    class_list_dict = self._oobe.EvaluateJavaScript(
        '{{ @pod }}.customIconElement.querySelector(".custom-icon").classList',
        pod=self._GET_POD_JS)
    class_list = [v for k,v in class_list_dict.items() if k != 'length']

    if 'custom-icon-unlocked' in class_list:
      return self.SmartLockState.AUTHENTICATED
    if 'custom-icon-locked' in class_list:
      return self.SmartLockState.LOCKED
    if 'custom-icon-hardlocked' in class_list:
      return self.SmartLockState.HARD_LOCKED
    if 'custom-icon-locked-to-be-activated' in class_list:
      return self.SmartLockState.TO_BE_ACTIVATED
    if 'custom-icon-spinner' in class_list:
      return self.SmartLockState.SPINNER

  def WaitForSmartLockState(self, state, wait_time_secs=60):
    """ Waits for the Smart Lock icon to reach the given state.

    Args:
      state: A value in AccountPickerScreen.SmartLockState
      wait_time_secs: The time to wait
    Returns:
      True if the state is reached within the wait time, else False.
    """
    try:
      util.WaitFor(lambda: self.smart_lock_state == state, wait_time_secs)
      return True
    except exceptions.TimeoutException:
      return False

  def EnterPassword(self):
    """ Enters the password to unlock or sign-in.

    Raises:
      TimeoutException: entering the password fails to enter/resume the user
      session.
    """
    assert(self.auth_type == self.AuthType.OFFLINE_PASSWORD or
           self.auth_type == self.AuthType.FORCE_OFFLINE_PASSWORD)
    oobe = self._oobe
    oobe.EvaluateJavaScript(
        '{{ @pod }}.passwordElement.value = {{ password }}',
        pod=self._GET_POD_JS, password=self._chromeos.password)
    oobe.EvaluateJavaScript(
        '{{ @pod }}.activate()', pod=self._GET_POD_JS)
    util.WaitFor(lambda: (self._chromeos.session_state ==
                          ChromeOS.SessionState.IN_SESSION),
                 5)

  def UnlockWithClick(self):
    """ Clicks the user pod to unlock or sign-in. """
    assert(self.auth_type == self.AuthType.USER_CLICK)
    self._oobe.EvaluateJavaScript(
        '{{ @pod }}.activate()', pod=self._GET_POD_JS)


class SmartLockSettings(object):
  """ Wrapper for the Smart Lock settings in chromeos://settings.
  """
  def __init__(self, tab, chromeos):
    """
    Args:
      tab: Inspector page of the chromeos://settings tag.
      chromeos: The parent Chrome wrapper.
    """
    self._tab = tab
    self._chromeos = chromeos

  @property
  def is_smart_lock_enabled(self):
    ''' Returns true if the settings show that Smart Lock is enabled. '''
    return self._tab.EvaluateJavaScript(
        '!document.getElementById("easy-unlock-enabled").hidden')

  def TurnOffSmartLock(self):
    """ Turns off Smart Lock.

    Smart Lock is turned off by clicking the turn-off button and navigating
    through the resulting overlay.

    Raises:
      TimeoutException: Timed out waiting for Smart Lock to be turned off.
    """
    assert(self.is_smart_lock_enabled)
    tab = self._tab
    tab.EvaluateJavaScript(
        'document.getElementById("easy-unlock-turn-off-button").click()')
    tab.WaitForJavaScriptCondition(
        '!document.getElementById("easy-unlock-turn-off-overlay").hidden && '
        'document.getElementById("easy-unlock-turn-off-confirm") != null',
        timeout=10)
    tab.EvaluateJavaScript(
        'document.getElementById("easy-unlock-turn-off-confirm").click()')
    tab.WaitForJavaScriptCondition(
        '!document.getElementById("easy-unlock-disabled").hidden', timeout=15)

  def StartSetup(self):
    """ Starts the Smart Lock setup flow by clicking the button.
    """
    assert(not self.is_smart_lock_enabled)
    self._tab.EvaluateJavaScript(
        'document.getElementById("easy-unlock-setup-button").click()')

  def StartSetupAndReturnApp(self):
    """ Runs the setup and returns the wrapper to the setup app.

    After clicking the setup button in the settings page, enter the password to
    reauthenticate the user before the app launches.

    Returns:
      A SmartLockApp object of the app that was launched.

    Raises:
      TimeoutException: Timed out waiting for app.
    """
    self.StartSetup()
    util.WaitFor(lambda: (self._chromeos.session_state ==
                          ChromeOS.SessionState.LOCK_SCREEN),
                 5)
    lock_screen = self._chromeos.GetAccountPickerScreen()
    lock_screen.EnterPassword()
    util.WaitFor(lambda: self._chromeos.GetSmartLockApp() is not None, 10)
    return self._chromeos.GetSmartLockApp()


class SmartLockApp(object):
  """ Wrapper for the Smart Lock setup dialog.

  Note: This does not include the app's background page.
  """

  class PairingState:
    """ The current state of the setup flow. """
    SCAN = 'scan'
    PAIR = 'pair'
    CLICK_FOR_TRIAL_RUN = 'click_for_trial_run'
    TRIAL_RUN_COMPLETED = 'trial_run_completed'
    PROMOTE_SMARTLOCK_FOR_ANDROID = 'promote-smart-lock-for-android'

  def __init__(self, app_page, chromeos):
    """
    Args:
      app_page: Inspector page of the app window.
      chromeos: The parent Chrome wrapper.
    """
    self._app_page = app_page
    self._chromeos = chromeos

  @property
  def pairing_state(self):
    ''' Returns the state the app is currently in.

    Raises:
      ValueError: The current state is unknown.
    '''
    state = self._app_page.EvaluateJavaScript(
        'document.body.getAttribute("step")')
    if state == 'scan':
      return SmartLockApp.PairingState.SCAN
    elif state == 'pair':
      return SmartLockApp.PairingState.PAIR
    elif state == 'promote-smart-lock-for-android':
      return SmartLockApp.PairingState.PROMOTE_SMARTLOCK_FOR_ANDROID
    elif state == 'complete':
      button_text = self._app_page.EvaluateJavaScript(
          'document.getElementById("pairing-button").textContent')
      button_text = button_text.strip().lower()
      if button_text == 'try it out':
        return SmartLockApp.PairingState.CLICK_FOR_TRIAL_RUN
      elif button_text == 'done':
        return SmartLockApp.PairingState.TRIAL_RUN_COMPLETED
      else:
        raise ValueError('Unknown button text: %s', button_text)
    else:
      raise ValueError('Unknown pairing state: %s' % state)

  def FindPhone(self, retries=3):
    """ Starts the initial step to find nearby phones.

    The app must be in the SCAN state.

    Args:
      retries: The number of times to retry if no phones are found.
    Returns:
      True if a phone is found, else False.
    """
    assert(self.pairing_state == self.PairingState.SCAN)
    for _ in xrange(retries):
      self._ClickPairingButton()
      if self.pairing_state == self.PairingState.PAIR:
        return True
      # Wait a few seconds before retrying.
      time.sleep(10)
    return False

  def PairPhone(self):
    """ Starts the step of finding nearby phones.

    The app must be in the PAIR state.

    Returns:
      True if pairing succeeded, else False.
    """
    assert(self.pairing_state == self.PairingState.PAIR)
    self._ClickPairingButton()
    if self.pairing_state == self.PairingState.PROMOTE_SMARTLOCK_FOR_ANDROID:
      self._ClickPairingButton()
    return self.pairing_state == self.PairingState.CLICK_FOR_TRIAL_RUN

  def StartTrialRun(self):
    """ Starts the trial run.

    The app must be in the CLICK_FOR_TRIAL_RUN state.

    Raises:
      TimeoutException: Timed out starting the trial run.
    """
    assert(self.pairing_state == self.PairingState.CLICK_FOR_TRIAL_RUN)
    self._app_page.EvaluateJavaScript(
        'document.getElementById("pairing-button").click()')
    util.WaitFor(lambda: (self._chromeos.session_state ==
                          ChromeOS.SessionState.LOCK_SCREEN),
                 10)

  def DismissApp(self):
    """ Dismisses the app after setup is completed.

    The app must be in the TRIAL_RUN_COMPLETED state.
    """
    assert(self.pairing_state == self.PairingState.TRIAL_RUN_COMPLETED)
    self._app_page.EvaluateJavaScript(
        'document.getElementById("pairing-button").click()')

  def _ClickPairingButton(self):
    # Waits are needed because the clicks occur before the button label changes.
    time.sleep(1)
    self._app_page.EvaluateJavaScript(
        'document.getElementById("pairing-button").click()')
    time.sleep(1)
    self._app_page.WaitForJavaScriptCondition(
        '!document.getElementById("pairing-button").disabled', timeout=60)
    time.sleep(1)
    self._app_page.WaitForJavaScriptCondition(
        '!document.getElementById("pairing-button-title")'
        '.classList.contains("animated-fade-out")', timeout=5)
    self._app_page.WaitForJavaScriptCondition(
        '!document.getElementById("pairing-button-title")'
        '.classList.contains("animated-fade-in")', timeout=5)


class ChromeOS(object):
  """ Wrapper for a remote ChromeOS device.

  Operations performed through this wrapper are sent through the network to
  Chrome using the Chrome DevTools API. Therefore, any function may throw an
  exception if the communication to the remote device is severed.
  """

  class SessionState:
    """ The state of the user session.
    """
    SIGNIN_SCREEN = 'signin_screen'
    IN_SESSION = 'in_session'
    LOCK_SCREEN = 'lock_screen'

  _SMART_LOCK_SETTINGS_URL = 'chrome://settings/search#Smart%20Lock'

  def __init__(self, remote_address, username, password, ssh_port=None):
    """
    Args:
      remote_address: The remote address of the cros device.
      username: The username of the account to test.
      password: The password of the account to test.
      ssh_port: The ssh port to connect to.
    """
    self._remote_address = remote_address
    self._username = username
    self._password = password
    self._ssh_port = ssh_port
    self._browser_to_create = None
    self._browser = None
    self._cros_interface = None
    self._background_page = None
    self._processes = []

  @property
  def username(self):
    ''' Returns the username of the user to login. '''
    return self._username

  @property
  def password(self):
    ''' Returns the password of the user to login. '''
    return self._password

  @property
  def session_state(self):
    ''' Returns the state of the user session. '''
    assert(self._browser is not None)
    if self._browser.oobe_exists:
      if self._cros_interface.IsCryptohomeMounted(self.username, False):
        return self.SessionState.LOCK_SCREEN
      else:
        return self.SessionState.SIGNIN_SCREEN
    else:
      return self.SessionState.IN_SESSION;

  @property
  def cryptauth_access_token(self):
    try:
      self._background_page.WaitForJavaScriptCondition(
            'var __token = __token || null; '
            'chrome.identity.getAuthToken(function(token) {'
            '  __token = token;'
            '}); '
            '__token != null', timeout=5)
      return self._background_page.EvaluateJavaScript('__token');
    except exceptions.TimeoutException:
      logger.error('Failed to get access token.');
      return ''

  def __enter__(self):
    return self

  def __exit__(self, *args):
    if self._browser is not None:
      self._browser.Close()
    if self._browser_to_create is not None:
      self._browser_to_create.CleanUpEnvironment()
    if self._cros_interface is not None:
      self._cros_interface.CloseConnection()
    for process in self._processes:
      process.terminate()

  def Start(self, local_app_path=None):
    """ Connects to the ChromeOS device and logs in.
    Args:
      local_app_path: A path on the local device containing the Smart Lock app
                      to use instead of the app on the ChromeOS device.
    Return:
      |self| for using in a "with" statement.
    """
    assert(self._browser is None)

    finder_opts = browser_options.BrowserFinderOptions('cros-chrome')
    finder_opts.CreateParser().parse_args(args=[])
    finder_opts.cros_remote = self._remote_address
    if self._ssh_port is not None:
      finder_opts.cros_remote_ssh_port = self._ssh_port
    finder_opts.verbosity = 1

    browser_opts = finder_opts.browser_options
    browser_opts.create_browser_with_oobe = True
    browser_opts.disable_component_extensions_with_background_pages = False
    browser_opts.gaia_login = True
    browser_opts.username = self._username
    browser_opts.password = self._password
    browser_opts.auto_login = True

    self._cros_interface = cros_interface.CrOSInterface(
        finder_opts.cros_remote,
        finder_opts.cros_remote_ssh_port,
        finder_opts.cros_ssh_identity)

    browser_opts.disable_default_apps = local_app_path is not None
    if local_app_path is not None:
      easy_unlock_app = extension_to_load.ExtensionToLoad(
          path=local_app_path,
          browser_type='cros-chrome',
          is_component=True)
      finder_opts.extensions_to_load.append(easy_unlock_app)

    self._browser_to_create = browser_finder.FindBrowser(finder_opts)
    self._browser_to_create.SetUpEnvironment(browser_opts)

    retries = 3
    while self._browser is not None or retries > 0:
      try:
        self._browser = self._browser_to_create.Create()
        break;
      except (exceptions.LoginException) as e:
        logger.error('Timed out logging in: %s' % e);
        if retries == 1:
          raise

    bg_page_path = '/_generated_background_page.html'
    util.WaitFor(
        lambda: self._FindSmartLockAppPage(bg_page_path) is not None,
        10);
    self._background_page = self._FindSmartLockAppPage(bg_page_path)
    return self

  def GetAccountPickerScreen(self):
    """ Returns the wrapper for the lock screen or sign-in screen.

    Return:
      An instance of AccountPickerScreen.
    Raises:
      TimeoutException: Timed out waiting for account picker screen to load.
    """
    assert(self._browser is not None)
    assert(self.session_state == self.SessionState.LOCK_SCREEN or
           self.session_state == self.SessionState.SIGNIN_SCREEN)
    oobe = self._browser.oobe
    def IsLockScreenResponsive():
      return (oobe.EvaluateJavaScript("typeof Oobe == 'function'") and
              oobe.EvaluateJavaScript(
                  "typeof Oobe.authenticateForTesting == 'function'"))
    util.WaitFor(IsLockScreenResponsive, 10)
    oobe.WaitForJavaScriptCondition(
        'document.getElementById("pod-row") && '
        'document.getElementById("pod-row").pods && '
        'document.getElementById("pod-row").pods.length > 0', timeout=10)
    return AccountPickerScreen(oobe, self)

  def GetSmartLockSettings(self):
    """ Returns the wrapper for the Smart Lock settings.
    A tab will be navigated to chrome://settings if it does not exist.

    Return:
      An instance of SmartLockSettings.
    Raises:
      TimeoutException: Timed out waiting for settings page.
    """
    if not len(self._browser.tabs):
      self._browser.New()
    tab = self._browser.tabs[0]
    url = tab.EvaluateJavaScript('document.location.href')
    if url != self._SMART_LOCK_SETTINGS_URL:
      tab.Navigate(self._SMART_LOCK_SETTINGS_URL)

    # Wait for settings page to be responsive.
    tab.WaitForJavaScriptCondition(
        'document.getElementById("easy-unlock-disabled") && '
        'document.getElementById("easy-unlock-enabled") && '
        '(!document.getElementById("easy-unlock-disabled").hidden || '
        ' !document.getElementById("easy-unlock-enabled").hidden)',
        timeout=10)
    settings = SmartLockSettings(tab, self)
    logger.info('Started Smart Lock settings: enabled=%s' %
                 settings.is_smart_lock_enabled)
    return settings

  def GetSmartLockApp(self):
    """ Returns the wrapper for the Smart Lock setup app.

    Return:
      An instance of SmartLockApp or None if the app window does not exist.
    """
    app_page = self._FindSmartLockAppPage('/pairing.html')
    if app_page is not None:
      # Wait for app window to be responsive.
      tab.WaitForJavaScriptCondition(
            'document.getElementById("pairing-button") != null', timeout=10)
      return SmartLockApp(app_page, self)
    return None

  def SetCryptAuthStaging(self, cryptauth_staging_url):
    logger.info('Setting CryptAuth to Staging')
    try:
      self._background_page.ExecuteJavaScript("""
          var key = app.CryptAuthClient.GOOGLE_API_URL_OVERRIDE_;
          var __complete = false;
          chrome.storage.local.set({key: {{ url }}}, function() {
              __complete = true;
          });""",
          url=cryptauth_staging_url)
      self._background_page.WaitForJavaScriptCondition(
          '__complete == true', timeout=10)
    except exceptions.TimeoutException:
      logger.error('Failed to override CryptAuth to staging url.')

  def RunBtmon(self):
    """ Runs the btmon command.
    Return:
      A subprocess.Popen object of the btmon process.
    """
    assert(self._cros_interface)
    cmd = self._cros_interface.FormSSHCommandLine(['btmon'])
    process = subprocess.Popen(args=cmd, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
    self._processes.append(process)
    return process

  def _FindSmartLockAppPage(self, page_name):
    try:
      extensions = self._browser.extensions.GetByExtensionId(
          'mkaemigholebcgchlkbankmihknojeak')
    except KeyError:
      return None
    for extension_page in extensions:
      pathname = extension_page.EvaluateJavaScript(
          'document.location.pathname')
      if pathname == page_name:
        return extension_page
    return None
