# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Tests for endpoints.api_backend_service."""

import logging
import unittest

import endpoints.api_backend as api_backend
import endpoints.api_backend_service as api_backend_service
import endpoints.api_exceptions as api_exceptions
import mox
import test_util


class ModuleInterfaceTest(test_util.ModuleInterfaceTest,
                          unittest.TestCase):

  MODULE = api_backend_service


class ApiConfigRegistryTest(unittest.TestCase):

  def setUp(self):
    super(ApiConfigRegistryTest, self).setUp()
    self.registry = api_backend_service.ApiConfigRegistry()

  def testApiMethodsMapped(self):
    self.registry.register_backend(
        '{"methods": {"method1": {"rosyMethod": "foo"}}}')
    self.assertEquals('foo', self.registry.lookup_api_method('method1'))

  def testAllApiConfigsWithTwoConfigs(self):
    config1 = '{"methods": {"method1": {"rosyMethod": "c1.foo"}}}'
    config2 = '{"methods": {"method2": {"rosyMethod": "c2.bar"}}}'
    self.registry.register_backend(config1)
    self.registry.register_backend(config2)
    self.assertEquals('c1.foo', self.registry.lookup_api_method('method1'))
    self.assertEquals('c2.bar', self.registry.lookup_api_method('method2'))
    self.assertItemsEqual([config1, config2], self.registry.all_api_configs())

  def testNoneApiConfigContent(self):
    self.registry.register_backend(None)
    self.assertIsNone(self.registry.lookup_api_method('method'))

  def testUnparseableApiConfigContent(self):
    config = '{"methods": {"method": {"rosyMethod": "foo"'  # Unclosed {s
    self.assertRaises(ValueError, self.registry.register_backend, config)
    self.assertIsNone(self.registry.lookup_api_method('method'))

  def testEmptyApiConfig(self):
    config = '{}'
    self.registry.register_backend(config)
    self.assertIsNone(self.registry.lookup_api_method('method'))

  def testApiConfigContentWithNoMethods(self):
    config = '{"methods": {}}'
    self.registry.register_backend(config)
    self.assertIsNone(self.registry.lookup_api_method('method'))

  def testApiConfigContentWithNoRosyMethod(self):
    config = '{"methods": {"method": {}}}'
    self.registry.register_backend(config)
    self.assertIsNone(self.registry.lookup_api_method('method'))

  def testRegisterSpiRootRepeatedError(self):
    config1 = '{"methods": {"method1": {"rosyMethod": "MyClass.Func1"}}}'
    config2 = '{"methods": {"method2": {"rosyMethod": "MyClass.Func2"}}}'
    self.registry.register_backend(config1)
    self.assertRaises(api_exceptions.ApiConfigurationError,
                      self.registry.register_backend, config2)
    self.assertEquals('MyClass.Func1',
                      self.registry.lookup_api_method('method1'))
    self.assertIsNone(self.registry.lookup_api_method('method2'))
    self.assertEqual([config1], self.registry.all_api_configs())

  def testRegisterSpiDifferentClasses(self):
    """This can happen when multiple classes implement an API."""
    config1 = ('{"methods": {'
               '  "method1": {"rosyMethod": "MyClass.Func1"},'
               '  "method2": {"rosyMethod": "OtherClass.Func2"}}}')
    self.registry.register_backend(config1)
    self.assertEquals('MyClass.Func1',
                      self.registry.lookup_api_method('method1'))
    self.assertEquals('OtherClass.Func2',
                      self.registry.lookup_api_method('method2'))
    self.assertEqual([config1], self.registry.all_api_configs())


class BackedServiceImplTest(unittest.TestCase):

  def setUp(self):
    self.service = api_backend_service.BackendServiceImpl(
        api_backend_service.ApiConfigRegistry(), '1')
    self.mox = mox.Mox()

  def tearDown(self):
    self.mox.UnsetStubs()

  def testGetApiConfigsWithEmptyRequest(self):
    request = api_backend.GetApiConfigsRequest()
    self.assertEqual([], self.service.getApiConfigs(request).items)

  def testGetApiConfigsWithCorrectRevision(self):
    # TODO: there currently exists a bug in protorpc where non-unicode strings
    #     aren't validated correctly and so their values aren't set correctly.
    #     Remove 'u' this once that's fixed. This shouldn't affect production.
    request = api_backend.GetApiConfigsRequest(appRevision=u'1')
    self.assertEqual([], self.service.getApiConfigs(request).items)

  def testGetApiConfigsWithIncorrectRevision(self):
    # TODO: there currently exists a bug in protorpc where non-unicode strings
    #     aren't validated correctly and so their values aren't set correctly.
    #     Remove 'u' this once that's fixed. This shouldn't affect production.
    request = api_backend.GetApiConfigsRequest(appRevision=u'2')
    self.assertRaises(
        api_exceptions.BadRequestException, self.service.getApiConfigs, request)

  # pylint: disable=g-bad-name
  def verifyLogLevels(self, levels):
    Level = api_backend.LogMessagesRequest.LogMessage.Level
    message = 'Test message.'
    logger_name = api_backend_service.__name__

    log = mox.MockObject(logging.Logger)
    self.mox.StubOutWithMock(logging, 'getLogger')
    logging.getLogger(logger_name).AndReturn(log)

    for level in levels:
      if level is None:
        level = 'info'
      record = logging.LogRecord(name=logger_name,
                                 level=getattr(logging, level.upper()),
                                 pathname='', lineno='', msg=message,
                                 args=None, exc_info=None)
      log.handle(record)
    self.mox.ReplayAll()

    requestMessages = []
    for level in levels:
      if level:
        requestMessage = api_backend.LogMessagesRequest.LogMessage(
            level=getattr(Level, level), message=message)
      else:
        requestMessage = api_backend.LogMessagesRequest.LogMessage(
            message=message)
      requestMessages.append(requestMessage)

    request = api_backend.LogMessagesRequest(messages=requestMessages)
    self.service.logMessages(request)
    self.mox.VerifyAll()

  def testLogMessagesUnspecifiedLevel(self):
    self.verifyLogLevels([None])

  def testLogMessagesDebug(self):
    self.verifyLogLevels(['debug'])

  def testLogMessagesInfo(self):
    self.verifyLogLevels(['info'])

  def testLogMessagesWarning(self):
    self.verifyLogLevels(['warning'])

  def testLogMessagesError(self):
    self.verifyLogLevels(['error'])

  def testLogMessagesCritical(self):
    self.verifyLogLevels(['critical'])

  def testLogMessagesAll(self):
    self.verifyLogLevels([None, 'debug', 'info', 'warning', 'error',
                          'critical'])

  def testLogMessagesRandom(self):
    self.verifyLogLevels(['info', 'debug', 'info', 'info', 'warning', 'info',
                          'error', 'error', None, 'info', None, None,
                          'critical', 'critical', 'info', 'info', None])


if __name__ == '__main__':
  unittest.main()
