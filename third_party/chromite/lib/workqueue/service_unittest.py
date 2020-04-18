# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for workqueue server and client classes."""

#pylint: disable=protected-access

from __future__ import print_function

import mock
import os
import pickle
import shutil
import tempfile
import unittest

from chromite.lib.workqueue import service
from chromite.lib.workqueue import tasks


REQUESTED = service._BaseWorkQueue._REQUESTED
PENDING = service._BaseWorkQueue._PENDING
RUNNING = service._BaseWorkQueue._RUNNING
COMPLETE = service._BaseWorkQueue._COMPLETE
ABORTING = service._BaseWorkQueue._ABORTING
STATES = service._BaseWorkQueue._STATES


class _MonitorTermination(Exception):
  """Exeception raised to stop `ProcessRequests` during testing.

  `service.WorkQueueServer.ProcessRequests` can only terminate
  if an exception is raised during its execution.  Unit tests that
  call the method raise this exception to end the calls.
  """
  pass


class _Task(object):
  """Information about an enqueued task."""
  def __init__(self, request_id, duration):
    self.request_id = request_id
    self.duration = duration
    self.start_time = None
    self.age = 0
    self.aborted = False


class _TestTaskManager(tasks.TaskManager):
  """A concrete subclass of `TaskManager` for testing purposes.

  This is a test double class used to track usage during tests
  of `WorkQueueServer.ProcessRequests()`.  It also serves a
  very limited role in is some other specific test cases.

  This version of `TaskManager` keeps track of the sequence of events
  that trigger operations on the various requests that are expected to
  make their way through the test cases.  These are the events
  supported:
    * Enqueue(task_id, duration) - enqueue a request for a given
      `task_id`.  The task will be marked ready for `Reap()` after
      `duration` ticks.
    * SetCapacity(capacity) - `self.HasCapacity()` will return true
      until `capacity` tasks have been started by `self.StartTask()`.
    * Abort(task_id) - Abort the request associated with the given
      `task_id`.
    * Execute(tick_count) - Resume processing events after `tick_count`
      ticks have elapsed.

  The `SetSequence()` method is used to initialize the sequence of
  events before a call to `ProcessRequests()`.  For example:

    sequence = [(task_manager.SetCapacity, 1),
                (task_manager.Enqueue, 'a', 1),
                (task_manager.Execute, 1),
                (task_manager.Abort, 'a')]
    task_manager.SetSequence(sequence)

  The sequence above has the following effect:
    * The `SetCapacity` event means that one enqueue request will be
      allowed to start.
    * The `Enqueue` request will actually enqueue a request for
      execution.
    * The `Execute` event will cause `ProcessRequests()` to execute for
      one tick.  This will allow task 'a' to begin executing.
    * The `Abort` event will execute 1 tick after task 'a' is enqueued,
      which will cause the request to be aborted while running..

  This class is responsible for the following assertions:
    * `ProcessRequests()` will not call `time.sleep()` without an
      intervening call to `manager.StartTick()`.
    * `ProcessRequests()` will not call without an `manager.StartTick()`
      intervening call to `time.sleep()`.
    * Calls to `time.sleep()` use the value of `self.sample_interval`.
    * Requests passed to `StartTask()` use the correct argument for the
      given request.
  """

  def __init__(self, test_cases=None):
    super(_TestTaskManager, self).__init__(None, 1e6)
    self.requested_tasks = {}
    self.running_tasks = set()
    self._test_cases = test_cases
    self._sequence_data = None
    self._delay_count = 0
    self._capacity_count = 0
    self._start_tick_allowed = True
    self._sleep_allowed = True
    self._current_tick = 0

  def __len__(self):
    return len(self.running_tasks)

  def SetSequence(self, sequence_data):
    """Initialize a sequence of events for `ProcessRequests()`."""
    self._start_tick_allowed = True
    self._sleep_allowed = True
    self._sequence_data = list(sequence_data)

  def Enqueue(self, task_id, duration):
    """Perform handling for an `Enqueue` event."""
    request_id = self._test_cases.client.EnqueueRequest(task_id)
    self.requested_tasks[task_id] = _Task(request_id, duration)

  def SetCapacity(self, capacity):
    """Perform handling for a `SetCapacity` event."""
    self._capacity_count = capacity

  def Abort(self, task_id):
    """Perform handling for an `Abort` event."""
    request_id = self.requested_tasks[task_id].request_id
    self._test_cases.client.AbortRequest(request_id)
    self.requested_tasks[task_id].aborted = True

  def Execute(self, tick_count):
    """Perform handling for an `Execute` event."""
    self._delay_count = tick_count

  def _SetTickReady(self, start_ready):
    """Set whether `StartTick()` or `Sleep()` should be called next."""
    self._start_tick_allowed = start_ready
    self._sleep_allowed = not start_ready

  def Sleep(self, interval):
    """Replacement for `time.sleep()` in `ProcessRequests()`."""
    self._test_cases.assertTrue(self._sleep_allowed,
                                'time.sleep() called twice without '
                                'intervening call to TaskManager.StartTick()')
    self._SetTickReady(True)
    self._test_cases.assertEqual(interval, self.sample_interval)
    self._current_tick += 1

  def StartTick(self):
    self._test_cases.assertTrue(self._start_tick_allowed,
                                'TaskManager.StartTick() called twice '
                                'without intervening call to time.sleep()')
    self._SetTickReady(False)
    # Execute event from our sequence until an `Execute` event says to
    # proceed.
    while self._delay_count <= 0:
      if not self._sequence_data:
        raise _MonitorTermination()
      next_event = self._sequence_data.pop(0)
      next_event[0](*next_event[1:])
    self._delay_count -= 1
    # Advance the age of all running tasks by one tick.
    for _, task_id in self.running_tasks:
      self.requested_tasks[task_id].age += 1

  def HasCapacity(self):
    return bool(self._capacity_count)

  def StartTask(self, request_id, request_data):
    if self._test_cases:
      task = self.requested_tasks[request_data]
      task.start_time = self._current_tick
      self._test_cases.assertEqual(request_id, task.request_id)
    self._capacity_count -= 1
    self.running_tasks.add((request_id, request_data))

  def TerminateTask(self, request_id):
    for entry in self.running_tasks.copy():
      if request_id == entry[0]:
        self.running_tasks.remove(entry)

  def Reap(self):
    for entry in self.running_tasks.copy():
      task = self.requested_tasks[entry[1]]
      if task.age >= task.duration:
        self.running_tasks.remove(entry)
        yield entry


class _BaseServiceTestCases(unittest.TestCase):
  """Base class for testing workqueue server and client methods."""

  def setUp(self):
    self._spool_tmp = tempfile.mkdtemp()
    spool_dir = os.path.join(self._spool_tmp, 'spool')
    self.server = service.WorkQueueServer(spool_dir)
    self.client = service.WorkQueueClient(spool_dir)

  def tearDown(self):
    shutil.rmtree(self._spool_tmp)

  def ValidateState(self, request_id, *args):
    """Check that a given request is in an expected state.

    Assert the following:
      * The spool file for `request_id` exists for all states in
        `args`.
      * No other spool files exist for `request_id`.

    Args:
      request_id: The request id to be tested.
      args: Allowed states of the request.
    """
    actual_states = set()
    for state in STATES:
      if self.server._RequestInState(request_id, state):
        actual_states.add(state)
    self.assertEqual(actual_states, set(args))


class ServiceInternalsTestCases(_BaseServiceTestCases):
  """Test cases for workqueue state transitions.

  These test cases exercise part of all of the life-cycle of a single
  request.  As tested, the full life-cycle follows these steps:
   1. Client creates a request via `EnqueueRequest()`.
   2. Server sees the request via `_GetNewRequests()`.
   3. Server starts a task for a request via `_StartRequest()`.
   4. Server reports request completion via `_CompleteRequest()`.
   5. Client removes completed request via `Wait()`.

  In methods and comments below, the first four stages are identified by
  the names `Enqueue`, `GetNew`, `Start`, and `Complete`, respectively.
  """

  def setUp(self):
    super(ServiceInternalsTestCases, self).setUp()
    self.server._CreateSpool()

  def _ValidateRequestState(self, request_id, value, expected_state):
    """Check that a given request is in an expected state.

    Assert the following:
      * The spool file associated with `request_id` exists for
        `expected_state`.
      * The spool file for the expected state contains `value`.
      * There is no spool file for `request_id` for any state
        other than `expected_state`.

    Args:
      request_id: The request id to be tested.
      value: The value expected to be stored in the request's state
        file.
      expected_state: The expected state of the request.
    """
    self.ValidateState(request_id, expected_state)
    request_path = self.server._GetRequestPathname(request_id,
                                                   expected_state)
    with open(request_path, 'r') as f:
      self.assertEqual(pickle.load(f), value)

  def _ValidateAborted(self, request_id, expected_state):
    """Check that an abort request has been properly recorded.

    The code calls `_GetAbortRequests()` and asserts that the returned
    value matches the given `request_id`.

    Prior to the call, assert the following:
      * The abort request file exists for `request_id`.
      * The request file remains in `expected_state`.

    After to the call, assert the following:
      * The abort request file no longer exists.
      * The request file remains in `expected_state`.

    Args:
      request_id: The request to be aborted.
      expected_state: The state of the request at the time of abort.
    """
    self.ValidateState(request_id, expected_state, ABORTING)
    abort_list = list(self.server._GetAbortRequests())
    self.assertEqual([request_id], abort_list)
    self.ValidateState(request_id, expected_state)

  def _ExecuteAbort(self, request_id, expected_state):
    """Execute a call to `AbortRequest()`.

    Makes a call to `self.client.AbortRequest()`, and then asserts the
    the conditions required for `_ValidateAborted()`, above.

    Args:
      request_id: The request to be aborted.
      expected_state: The state of the request at the time of abort.
    """
    self.client.AbortRequest(request_id)
    self._ValidateAborted(request_id, expected_state)

  def _ExecuteTimeout(self, request_id, expected_state):
    """Make a call to `Wait()` that is expected to time out.

    Assert the following:
      * A timeout exception is raised by `Wait()`.
      * After the call, the request meets the conditions required for
        `_ValidateAborted()`, above.

    Args:
      request_id: The request that will time out.
      expected_state: The state of the request at timeout.
    """
    with self.assertRaises(service.WorkQueueTimeout):
      self.client.Wait(request_id, 0)
    self._ValidateAborted(request_id, expected_state)

  def _ExecuteEnqueue(self, request_value):
    """Walk a request to completion of the `Enqueue` stage.

    At each stage of execution, and at the end, asserts that
    the request is in the state expected for the stage.

    Args:
      request_value: Initial value to pass to `EnqueueRequest`.
    """
    request_id = self.client.EnqueueRequest(request_value)
    self._ValidateRequestState(request_id, request_value,
                               self.client._REQUESTED)
    return request_id

  def _ExecuteGetNew(self, request_value):
    """Walk a request to completion of the `GetNew` stage.

    At each stage of execution, and at the end, asserts that the request
    is in the state expected for the stage.

    Args:
      request_value: Initial value to pass to `EnqueueRequest`.
    """
    request_id = self._ExecuteEnqueue(request_value)
    new_requests = self.server._GetNewRequests()
    self.assertEqual([request_id], new_requests)
    self._ValidateRequestState(request_id, request_value, PENDING)
    return request_id

  def _ExecuteStart(self, request_value):
    """Walk a request to completion of the `Start` stage.

    At each stage of execution, and at the end, asserts that the request
    is in the state expected for the stage.

    After the call to `_StartRequest()` assert that the expected
    side-effects occurred on the method's parameters.

    Args:
      request_value: Initial value to pass to `EnqueueRequest`.
    """
    request_id = self._ExecuteGetNew(request_value)
    manager = _TestTaskManager()
    self.server._StartRequest(request_id, manager)
    self.assertEqual(manager.running_tasks,
                     set([(request_id, request_value)]))
    self._ValidateRequestState(request_id, request_value, RUNNING)
    return request_id

  def _ExecuteComplete(self, request_value, result_value):
    """Walk a request to completion of the `Complete` stage.

    At each stage of execution, but not at the end, asserts that the
    request is in the state expected for the stage.

    Args:
      request_value: Initial value to pass to `EnqueueRequest`.
      result_value: the return value for the request.
    """
    request_id = self._ExecuteStart(request_value)
    self.server._CompleteRequest(request_id, result_value)
    return request_id

  def _ExecuteLifecycle(self, request_value, result_value):
    """Walk a request through the complete request life cycle.

    This creates a request, and processes all standard state
    transitions, ending with a call to `Wait()`.  The initial
    request object is passed in as `request_value`, and the
    result returned should be `result_value`.

    Args:
      request_value: the initial value to be passed to
        `EnqueueRequest()`.
      result_value: The value to be given the result in the call to
        `_CompleteRequest()`.
    """
    request_id = self._ExecuteComplete(request_value, result_value)
    return self.client.Wait(request_id, 0)

  def _EnqueueMultiple(self, count):
    """Make sequential calls to `EnqueueRequest()`.

    Make `count` calls to `EnqueueRequest()`, then assert the following:
      * The returned request id values are unique and in sorted order.
      * After all calls are complete, all requests are in the
        `_REQUESTED` state.
    """
    request_value = 'blurbity-blurb'
    request_ids = []
    for _ in range(0, count):
      request_ids.append(self.client.EnqueueRequest(request_value))
    for index in range(0, len(request_ids) - 1):
      self.assertLess(request_ids[index], request_ids[index+1],
                      'request id out of order at index '
                      '{:d}'.format(index))
    for request_id in request_ids:
      self._ValidateRequestState(request_id, request_value,
                                 self.client._REQUESTED)

  def testEnqueueMultiple(self):
    """Test sequential calls to `EnqueueRequest()`.

    Make multiple calls to `EnqueueRequest()` in sequence, and assert
    the conditions detailed in `_EnqueueMultiple()`, above.

    Test once with the standard value of `_REQUEST_ID_FORMAT`, and once
    with a patched value that by design forces id collisions between
    successive calls to `EnqueueRequest()`.
    """
    self._EnqueueMultiple(5)
    with mock.patch.object(self.client,
                           '_REQUEST_ID_FORMAT', new='{:.2f}'):
      self._EnqueueMultiple(5)

  def testAbortEnqueue(self):
    """Test aborting a request after `Enqueue`."""
    request_id = self._ExecuteEnqueue('To be or not to be')
    self._ExecuteAbort(request_id, self.client._REQUESTED)

  def testAbortGetNew(self):
    """Test aborting a request after `GetNew`."""
    request_id = self._ExecuteGetNew('To be or not to be')
    self._ExecuteAbort(request_id, self.client._PENDING)

  def testAbortStart(self):
    """Test aborting a request after `Start`."""
    request_id = self._ExecuteStart('To be or not to be')
    self._ExecuteAbort(request_id, self.client._RUNNING)

  def testAbortComplete(self):
    """Test aborting a request after `Complete`."""
    request_id = self._ExecuteComplete('To be or not to be',
                                       'That is the question')
    self._ExecuteAbort(request_id, self.client._COMPLETE)

  def testTimeoutEnqueue(self):
    """Test a call to `Wait()` that times out after `Enqueue`."""
    request_id = self._ExecuteEnqueue('To be or not to be')
    self._ExecuteTimeout(request_id, self.client._REQUESTED)

  def testTimeoutGetNew(self):
    """Test a call to `Wait()` that times out after `GetNew`."""
    request_id = self._ExecuteGetNew('To be or not to be')
    self._ExecuteTimeout(request_id, PENDING)

  def testTimeoutStart(self):
    """Test a call to `Wait()` that times out after `Start`."""
    request_id = self._ExecuteStart('To be or not to be')
    self._ExecuteTimeout(request_id, RUNNING)

  def testTransitionRequest(self):
    """Test `WorkQueueServer._TransitionRequest()`."""
    request_value = 'blurbity-blurb'
    request_id = self.client.EnqueueRequest(request_value)
    oldstate = REQUESTED
    newstate = PENDING
    self.server._TransitionRequest(request_id, oldstate, newstate)
    self._ValidateRequestState(request_id, request_value, newstate)

  def testGetNewMultiple(self):
    """Test `_GetNewRequests()` with multiple requests pending."""
    request_values = [str(i) for i in range(0, 4)]
    request_list = []
    for rq in request_values:
      request_list.append(self.client.EnqueueRequest(rq))
    new_requests = self.server._GetNewRequests()
    self.assertEqual(request_list, new_requests)
    for request_id, value in zip(request_list, request_values):
      self._ValidateRequestState(request_id, value,
                                 PENDING)

  def testRequestLifecycle(self):
    """Test the complete life cycle of a successful request."""
    request_value = 'blurbity-blurb'
    expected_result = 'not blurbity at all'
    actual_result = self._ExecuteLifecycle(request_value,
                                           expected_result)
    self.assertEqual(actual_result, expected_result)
    for state_dir in self.server._spool_state_dirs.itervalues():
      self.assertFalse(os.listdir(state_dir))

  def testRequestLifecycleFailure(self):
    """Test the complete life cycle of a request that fails."""
    request_value = 'blurbity-blurb'
    expected_result = Exception('still blurbity!')
    with self.assertRaises(Exception) as exc:
      self._ExecuteLifecycle(request_value,
                             expected_result)
    # Different exceptions won't compare equal even if constructed with
    # the same arguments, so compare the string values instead.
    self.assertEqual(str(exc.exception), str(expected_result))


class ServerProcessRequestsTestCases(_BaseServiceTestCases):
  """Test cases for `WorkQueueServer.ProcessRequests()`."""

  def setUp(self):
    super(ServerProcessRequestsTestCases, self).setUp()
    self._manager = _TestTaskManager(self)

  def _ValidateCompleted(self, task, task_id):
    """Assert that the given `task` completed successfully."""
    value = self.client.Wait(task.request_id, 0)
    self.assertEqual(task_id, value)
    self.assertEqual(task.age, task.duration)

  def _ValidateAborted(self, task):
    """Assert that the given `task` was aborted."""
    request_id = task.request_id
    self.ValidateState(request_id)

  def _RunSequence(self, *args):
    """Run `ProcessRequests()` with the given sequence of events."""
    self._manager.SetSequence(args)
    with self.assertRaises(_MonitorTermination):
      with mock.patch('time.sleep', wraps=self._manager.Sleep):
        self.server.ProcessRequests(self._manager)
    for task_id, task in self._manager.requested_tasks.iteritems():
      if task.aborted:
        self._ValidateAborted(task)
      else:
        self._ValidateCompleted(task, task_id)
    return self._manager.requested_tasks

  def testDoNothing(self):
    for capacity in range(2):
      task_results = self._RunSequence(
          (self._manager.SetCapacity, capacity),
          (self._manager.Execute, 2))
      self.assertFalse(task_results)

  def testImmediateCapacity(self):
    task_results = self._RunSequence(
        (self._manager.SetCapacity, 1),
        (self._manager.Enqueue, 'a', 1),
        (self._manager.Execute, 2))
    task = task_results['a']
    self.assertEqual(task.start_time, 0)
    self.assertEqual(task.age, task.duration)

  def testDelayedCapacity(self):
    task_results = self._RunSequence(
        (self._manager.SetCapacity, 0),
        (self._manager.Enqueue, 'a', 1),
        (self._manager.Execute, 1),
        (self._manager.SetCapacity, 1),
        (self._manager.Execute, 2))
    task = task_results['a']
    self.assertEqual(task.start_time, 1)
    self.assertEqual(task.age, task.duration)

  def testStagedCapacity(self):
    task_results = self._RunSequence(
        (self._manager.SetCapacity, 1),
        (self._manager.Enqueue, 'a', 1),
        (self._manager.Enqueue, 'b', 1),
        (self._manager.Execute, 1),
        (self._manager.SetCapacity, 1),
        (self._manager.Execute, 2))
    for task_id, start in [('a', 0), ('b', 1)]:
      task = task_results[task_id]
      self.assertEqual(task.start_time, start)
      self.assertEqual(task.age, task.duration)

  def testAbortRequested(self):
    task_results = self._RunSequence(
        (self._manager.SetCapacity, 1),
        (self._manager.Enqueue, 'a', 1),
        (self._manager.Abort, 'a'),
        (self._manager.Execute, 2))
    task = task_results['a']
    self.assertTrue(task.aborted)
    self.assertIsNone(task.start_time)

  def testAbortPending(self):
    task_results = self._RunSequence(
        (self._manager.SetCapacity, 0),
        (self._manager.Enqueue, 'a', 1),
        (self._manager.Execute, 1),
        (self._manager.SetCapacity, 1),
        (self._manager.Abort, 'a'),
        (self._manager.Execute, 2))
    task = task_results['a']
    self.assertTrue(task.aborted)
    self.assertIsNone(task.start_time)

  def testAbortRunning(self):
    task_results = self._RunSequence(
        (self._manager.SetCapacity, 1),
        (self._manager.Enqueue, 'a', 1),
        (self._manager.Execute, 1),
        (self._manager.Abort, 'a'),
        (self._manager.Execute, 1))
    task = task_results['a']
    self.assertTrue(task.aborted)
    self.assertEqual(task.start_time, 0)

  def testAbortComplete(self):
    task_results = self._RunSequence(
        (self._manager.SetCapacity, 1),
        (self._manager.Enqueue, 'a', 1),
        (self._manager.Execute, 2),
        (self._manager.Abort, 'a'),
        (self._manager.Execute, 1))
    task = task_results['a']
    self.assertTrue(task.aborted)
    self.assertEqual(task.age, task.duration)


if __name__ == '__main__':
  unittest.main()
