# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import abc
import sys
import traceback
import time

from selenium.common.exceptions import TimeoutException
from selenium.webdriver.common.by import By
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.support.ui import WebDriverWait, Select as SelectElement
from selenium.webdriver.support import expected_conditions as EC


class _ElementSelector(object):
  """Base class for all element selectors."""
  key_type = None

  def __init__(self, key, index=0):
    """Constructor.

    Args:
      key: A string which acts as the selector for the element.
      # index: A selector for which instance of the key should be returned.

    Returns:
      A webdriver element if found, or None.
    """

    self._key = key
    # self._index = index

  def __str__(self):
    return '\'%s\' by %s' % (self._key, self.key_type)

  def tuple(self):
    return (self.key_type, self._key)


class ByID(_ElementSelector):
  """Select an element by its id."""
  key_type = By.ID


class ByClassName(_ElementSelector):
  """Select an element by its class name."""
  key_type = By.CLASS_NAME


class ByCssSelector(_ElementSelector):
  """Select an element by its class name."""
  key_type = By.CSS_SELECTOR


class ByXPath(_ElementSelector):
  """Select an element by its xpath."""
  key_type = By.XPATH


class _Action(object):
  """Base class for all actions."""

  __metaclass__ = abc.ABCMeta

  def Apply(self, driver, test, debug=False):
    self._debug = debug

    self._dprint('Trying to %s ' % self)

    if self._ignorable:
      assert_function = test.expect_expression
    else:
      assert_function = test.assert_expression

    try:
      response = self._Apply(driver, test)
      if response:
        self._dprint('  Success')
      else:
        self._dprint('  Failure')
      assert_function(response, 'Failed to %s' % self)
    except TimeoutException:
      self._dprint('  Failure: TimeoutException')
      stack = traceback.extract_tb(sys.exc_info()[2])
      assert_function(False, 'Failed to %s\nTimeoutException' % self, stack)

  def _dprint(self, msg):
    if self._debug:
      print(msg)

  @abc.abstractmethod
  def _Apply(self, driver, test):
    """The type-specific action execution implementation.

    Note: Subclasses must implement this method.

    Raises:
      NotImplementedError: Subclass did not implement the method
    """
    raise NotImplementedError()


class SetContext(_Action):
  """Sets an element (iframe) as the current driver context.

  If element_selector is None then it will be reset to root.
  """

  def __init__(self, element_selector, ignorable=False):
    self._element_selector = element_selector
    self._ignorable = ignorable

  def __str__(self):
    if self._element_selector is None:
      return 'Set context to default'
    else:
      return 'Set context to %s' % self._element_selector

  def _Apply(self, driver, test):
    if self._element_selector is None:
      driver.switch_to_default_content()
    else:
      wt = WebDriverWait(driver, 30) # seconds
      locator = self._element_selector.tuple()
      element = wt.until(EC.frame_to_be_available_and_switch_to_it(locator))

    return True


class Open(_Action):
  """Open a URL."""

  def __init__(self, url):
    self._url = url
    self._ignorable = False

  def __str__(self):
    return 'Open %s' % self._url

  def _Apply(self, driver, unused_test):
    try:
      driver.get(self._url)
    except TimeoutException:
      self._dprint(('  TimeoutException raised when loading %s\n'
                    '  Proceeding regardless') % (self._url))
    # Return true regardless as a slow-loading page probably means that there's
    # an non-critical script
    return True


class Type(_Action):
  """Type some text into a field."""

  def __init__(self, element_selector, text, ignorable=False):
    self._element_selector = element_selector
    self._text = text
    self._ignorable = ignorable

  def __str__(self):
    return 'Type \'%s\' into %s' % (self._text, self._element_selector)

  def _Apply(self, driver, test):
    wt = WebDriverWait(driver, 30) # seconds
    locator = self._element_selector.tuple()
    element = wt.until(EC.presence_of_element_located(locator))
    ActionChains(driver).move_to_element(element).perform()
    time.sleep(0.1)
    element = wt.until(EC.element_to_be_clickable(locator))
    ActionChains(driver).click(element).perform()
    element.send_keys(self._text)
    return True


class Select(_Action):
  """Select a value from a select input field."""

  def __init__(self, element_selector, value, ignorable=False, by_label=False):
    self._element_selector = element_selector
    self._value = value
    self._ignorable = ignorable
    self._by_label = by_label

  def __str__(self):
    return 'Select \'%s\' from %s%s' % (self._value,
                                        self._element_selector,
                                        (' by label' if self._by_label else ''))

  def _Apply(self, driver, test):
    wt = WebDriverWait(driver, 30)  # seconds
    locator = self._element_selector.tuple()
    element = wt.until(EC.presence_of_element_located(locator))
    ActionChains(driver).move_to_element(element).perform()
    element = wt.until(EC.element_to_be_clickable(locator))
    select = SelectElement(element)
    if self._by_label:
      select.select_by_visible_text(self._value)
    else:
      select.select_by_value(self._value)
    return True


class Wait(_Action):
  """Wait for a specified number of seconds."""

  def __init__(self, seconds, ignorable=False):
    self._seconds = seconds
    self._ignorable = ignorable

  def __str__(self):
    return 'Wait %d seconds' % self._seconds

  def _Apply(self, driver, test):
    time.sleep(self._seconds)
    return True


class Screenshot(_Action):
  """Print a base 64 encoded screenshot to stdout."""

  def __init__(self, filename, ignorable=False):
    self._filename = filename
    self._ignorable = ignorable

  def __str__(self):
    return 'Screenshot'

  def _Apply(self, driver, test):
    sys.stdout.write(driver.get_screenshot_as_base64())
    driver.get_screenshot_as_file(self._filename)
    return True


class Click(_Action):
  """Click on an element."""

  def __init__(self, element_selector, ignorable=False):
    self._element_selector = element_selector
    self._ignorable = ignorable

  def __str__(self):
    return 'Click %s' % self._element_selector

  def _Apply(self, driver, test):
    wt = WebDriverWait(driver, 10)  # seconds
    locator = self._element_selector.tuple()
    element = wt.until(EC.presence_of_element_located(locator))
    ActionChains(driver).move_to_element(element).perform()
    wt = WebDriverWait(driver, 10)  # seconds
    element = wt.until(EC.element_to_be_clickable(locator))
    element.click()
    return True


class TriggerAutofill(_Action):
  """Trigger autofill using a form field."""

  def __init__(self, element_selector, expected_type, trigger_character=None,
               ignorable=False):
    self._element_selector = element_selector
    self._expected_type = expected_type
    self._trigger_character = trigger_character
    self._ignorable = ignorable

  def __str__(self):
    return 'Trigger Autofill %s' % self._element_selector

  def _Apply(self, driver, test):
    expected_value = test.profile_data(self._expected_type)
    if expected_value != '':
      if self._trigger_character is None:
        trigger_character = expected_value[0]
      else:
        trigger_character = self._trigger_character
    else:
      test.expect_expression(
          overall_type,
          'Failure: Cannot trigger autofill, field_type \'%s\' '
          'does not have an expected value' % self._expected_type)
      return

    locator = self._element_selector.tuple()
    wt = WebDriverWait(driver, 15)  # seconds
    element = wt.until(EC.presence_of_element_located(locator))
    ActionChains(driver).move_to_element(element).perform()
    wt = WebDriverWait(driver, 10)  # seconds
    element = wt.until(EC.element_to_be_clickable(locator))
    ActionChains(driver).click(element).perform()
    time.sleep(1)
    actions = ActionChains(driver)
    actions.send_keys(trigger_character)
    actions.perform()
    time.sleep(0.5)
    actions = ActionChains(driver)
    actions.key_down(Keys.ARROW_DOWN)
    actions.key_down(Keys.ENTER)
    actions.perform()
    time.sleep(1)

    return True


class ValidateFields(_Action):
  """Assert that each field has its expected type and has been correctly filled.
  """

  def __init__(self, field_data_tuples, ignorable=False):
    self._field_data_tuples = field_data_tuples
    self._ignorable = ignorable

  def __str__(self):
    return 'Validate Fields:'

  def _VerifyField(self, driver, test, selector, expected_type,
                   custom_expected_value=None):
    try:
      wt = WebDriverWait(driver, 30)  # seconds
      element = wt.until(EC.presence_of_element_located(selector.tuple()))

      overall_type = element.get_attribute('autofill-prediction')

      if overall_type is None:
        test.expect_expression(
            overall_type,
            'Failure: Cannot verify field type, autofill-prediction attribute '
            'not set for \'%s\'' % selector)
        return

      response = True

      self._dprint('    Checking detected field type')
      field_type_response = overall_type == expected_type
      test.expect_expression(field_type_response,
                             'Field type %s does not match %s' %
                             (overall_type, expected_type))
      if field_type_response:
        self._dprint('      Success')
      else:
        response = False
        self._dprint('      Failure: actual overall_type is %s' % overall_type)


      self._dprint('    Checking autofilled data')
      field_value = element.get_attribute('value')

      if custom_expected_value is None:
        expected_value = test.profile_data(expected_type)
        if expected_value == '':
          test.expect_expression(
              overall_type,
              'Failure: Cannot verify field value, expected_type \'%s\' does '
              'not have an expected value' % expected_type)
          return
      else:
        expected_value = custom_expected_value

      field_value_response = field_value == expected_value
      test.expect_expression(field_value_response,
                             'Field value \'%s\' does not match \'%s\'' %
                             (field_value, expected_value))
      if field_value_response:
        self._dprint('      Success')
      else:
        response = False
        self._dprint('      Failure: Actual field value is \'%s\'' %
                     field_value)

      # The same field type.
      return response
    except TimeoutException:
      test.expect_expression(False,
                             'Failure: Cannot verify field type, input element '
                             '\'%s\' unavailable' % selector)
      return False

  def _Apply(self, driver, test):
    response = True

    for field_data_tuple in self._field_data_tuples:
      field_data_size = len(field_data_tuple)
      if field_data_size == 2:
        selector, expected_type = field_data_tuple
        custom_expected_value = None
      elif field_data_size == 3:
        selector, expected_type, custom_expected_value = field_data_tuple
      else:
        raise ValueError('Field data tuples must have either 2 or 3 elements.')

      self._dprint('  %s: %s' % (expected_type, selector))

      response = self._VerifyField(driver, test, selector, expected_type,
                                   custom_expected_value)

      if response:
        self._dprint('    Success')
      else:
        response = False
        self._dprint('    Failure')

    if not self._ignorable:
      test.assert_expectations();

    return response
