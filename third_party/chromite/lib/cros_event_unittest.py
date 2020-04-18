# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for chros_event"""

from __future__ import print_function

from StringIO import StringIO
import json

from chromite.lib import cros_event as ce
from chromite.lib import cros_test_lib


class EventIdGeneratorTest(cros_test_lib.TestCase):
  """Test EventIdGenerator fuction"""
  def testEventIdGenerator(self):
    expected_ids = {1, 2, 3, 4, 5}
    id_gen = ce.EventIdGenerator()
    for expected_id in expected_ids:
      self.assertEquals(expected_id, id_gen.next())


class FailTest(cros_test_lib.TestCase):
  """Test Failure class"""
  def testInit(self):
    f1 = ce.Failure()
    self.assertTrue(isinstance(f1, Exception))
    self.assertEqual(f1.status, ce.EVENT_STATUS_FAIL)

    f2_msg = "This is the message for the failure"
    f2 = ce.Failure(f2_msg)
    self.assertEqual(f2.status, ce.EVENT_STATUS_FAIL)
    self.assertEqual(f2.message, f2_msg)


class EventTest(cros_test_lib.TestCase):
  """Test Event class"""
  # pylint: disable=attribute-defined-outside-init
  def setUp(self):
    self._resetEmit()

    self.id1 = 1
    self.data1 = {1: "a", 2: "b", 3: "c"}
    self.event1 = ce.Event(eid=self.id1,
                           data=self.data1,
                           emit_func=self.emitHook)

  def emitHook(self, event):
    self.emitCalled = True
    self.emitEvent = event

  def _resetEmit(self):
    self.emitCalled = False
    self.emitEvent = None

  def testInit(self):
    self.assertEquals(self.event1[ce.EVENT_ID], self.id1)
    self.assertDictContainsSubset(self.data1, self.event1)

    self.assertTrue(isinstance(self.event1, dict))

  def testWithSuccess(self):
    """test success case"""
    with self.event1 as e:
      self.assertEqual(self.event1, e)
      self.assertEqual(e[ce.EVENT_STATUS], ce.EVENT_STATUS_RUNNING)

    self.assertTrue(self.emitCalled)
    self.assertEqual(self.emitEvent[ce.EVENT_STATUS], ce.EVENT_STATUS_PASS)

  def testWithFailureCall(self):
    """test with fail() call"""
    failMsg = "failed, as it should correctly"

    with self.event1 as e:
      e.fail(message=failMsg)

    self.assertEqual(self.emitEvent[ce.EVENT_STATUS], ce.EVENT_STATUS_FAIL)
    self.assertEqual(self.emitEvent[ce.EVENT_FAIL_MSG], failMsg)

  def testWithFailureCallWithStatus(self):
    """test with fail() and custom status call"""
    failMsg = "failed, as it should correctly"
    customStatus = "UnitTestFailure"

    with self.event1 as e:
      e.fail(message=failMsg, status=customStatus)

    self.assertEqual(self.emitEvent[ce.EVENT_STATUS], customStatus)
    self.assertEqual(self.emitEvent[ce.EVENT_FAIL_MSG], failMsg)

  def testWithFailure(self):
    """test with raising failure exception"""
    failMsg = "failed, as it should correctly"

    with self.event1:
      raise ce.Failure(failMsg)

    self.assertEqual(self.emitEvent[ce.EVENT_STATUS], ce.EVENT_STATUS_FAIL)
    self.assertEqual(self.emitEvent[ce.EVENT_FAIL_MSG], failMsg)

  def testWithExceptionFail(self):
    """test with raised non-Failure exception"""
    try:
      with self.event1:
        raise NameError
    except NameError:
      self.assertEqual(self.emitEvent[ce.EVENT_STATUS], ce.EVENT_STATUS_FAIL)


class EventLoggerTest(cros_test_lib.TestCase):
  """Test EventLogger class"""
  # pylint: disable=attribute-defined-outside-init

  def emitHook(self, event):
    self.emitCalled = True
    self.emitEvent = event

  def _resetEmit(self):
    self.emitCalled = False
    self.emitEvent = None

  def setUp(self):
    self._resetEmit()

    self.data1 = {1:2, 3:4}

    self.events = []
    self.log1 = ce.EventLogger(self.emitHook, data=self.data1)

  def testEvent(self):
    e_data = {"one": "two", "three":"four"}

    e = self.log1.Event(data=e_data)

    self.assertDictContainsSubset(e_data, e)
    self.assertDictContainsSubset(self.data1, e)

  def testEventWithKind(self):
    kind = 'testStep'
    e = self.log1.Event(kind=kind)
    self.assertEqual(e['id'][0], kind)


class EventFileLoggerTest(cros_test_lib.TestCase):
  """Test EventFileLogger class"""
  # pylint: disable=attribute-defined-outside-init

  def encode_func(self, event):
    self.emitEvent = event
    return json.dumps(event)

  def get_event_from_file(self):
    event_str = self.file_out.getvalue()
    self.file_out.buf = "" # Clear out StringIO buffer
    return json.loads(event_str)

  def setUp(self):
    self.data1 = {"one":2, "two":4}
    self.emitEvent = None
    self.file_out = StringIO()
    self.log = ce.EventFileLogger(self.file_out, data=self.data1,
                                  encoder_func=self.encode_func)

  def testInit(self):
    self.assertTrue(isinstance(self.log, ce.EventLogger))

  def testEvents(self):
    with self.log.Event():
      pass

    self.assertDictEqual(self.emitEvent, self.get_event_from_file())

  def testEventFail(self):
    with self.log.Event():
      raise ce.Failure("always fail")

    self.assertDictEqual(self.emitEvent, self.get_event_from_file())

  def testShutdown(self):
    self.log.shutdown()


class EventDummyLogger(cros_test_lib.TestCase):
  """Test EventDummyLogger class"""

  def setUp(self):
    self.log = ce.EventDummyLogger()

  def testInit(self):
    self.assertTrue(isinstance(self.log, ce.EventLogger))


class FunctionTest(cros_test_lib.TestCase):
  """Test Module Tests"""

  def setUp(self):
    self._last_root = ce.root

  def tearDown(self):
    if hasattr(self, "_last_root") and self._last_root:
      ce.setEventLogger(self._last_root)

  def SetEventLoggerTest(self):
    new_log = ce.EventDummyLogger()
    ce.setEventLogger(new_log)
    self.assertEqual(new_log, ce.root)

  def newEventTest(self):
    e1 = ce.newEvent()
    self.assertTrue(isinstance(e1, ce.Event))

    e2 = ce.newEvent(foo="bar")
    self.assertTrue(isinstance(e2, ce.Event))
    self.assertEqual("bar", e2["foo"])

    test_kind = 'testKind'
    e3 = ce.NewEvent(kind=test_kind)
    self.assertEqual(e3['id'][0], test_kind)
