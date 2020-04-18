# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test gslock library."""

from __future__ import print_function

import multiprocessing

from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import gs

from chromite.lib.paygen import gslock


# We access a lot of protected members during testing.
# pylint: disable=protected-access


def _InProcessAcquire(lock_uri):
  """Acquire a lock in a sub-process, but don't release.

  This helper has to be pickleable, so can't be a member of the test class.

  Args:
    lock_uri: URI of the lock to acquire.

  Returns:
    boolean telling if this method got the lock.
  """
  lock = gslock.Lock(lock_uri)
  try:
    lock.Acquire()
    return True
  except gslock.LockNotAcquired:
    return False


def _InProcessDoubleAcquire(lock_uri):
  """Acquire a lock in a sub-process, and reacquire it a second time.

  Do not release the lock after acquiring.

  This helper has to be pickleable, so can't be a member of the test class.

  Args:
    lock_uri: URI of the lock to acquire.

  Returns:
    int describing how many times it acquired a lock.
  """
  count = 0

  lock = gslock.Lock(lock_uri)
  try:
    lock.Acquire()
    count += 1
    lock.Acquire()
    count += 1
  except gslock.LockNotAcquired:
    pass

  return count


def _InProcessDataUpdate(lock_uri_data_uri):
  """Increment a number in a GS file protected by a lock.

  Keeps looking until the lock is acquired, so effectively, blocking. Stores
  or increments an integer in the data_uri by one, once.

  This helper has to be pickleable, so can't be a member of the test class.

  Args:
    lock_uri_data_uri: Tuple containing (lock_uri, data_uri). Passed as
                       a tuple, since multiprocessing.Pool.map only allows
                       a single argument in.

    lock_uri: URI of the lock to acquire.
    data_uri: URI of the data file to create/increment.

  Returns:
    boolean describing if this method got the lock.
  """
  lock_uri, data_uri = lock_uri_data_uri
  ctx = gs.GSContext()

  # Keep trying until the lock is acquired.
  while True:
    try:
      with gslock.Lock(lock_uri):
        if ctx.Exists(data_uri):
          data = int(ctx.Cat(data_uri)) + 1
        else:
          data = 1

        ctx.CreateWithContents(data_uri, str(data))
        return True

    except gslock.LockNotAcquired:
      pass


class GSLockTest(cros_test_lib.MockTestCase):
  """This test suite covers the GSLock file."""

  # For contention tests, how many parallel workers to spawn.  To really
  # stress test, you can bump it up to 200, but 20 seems to provide good
  # coverage w/out sucking up too many resources.
  NUM_THREADS = 20

  @cros_test_lib.NetworkTest()
  def setUp(self):
    self.ctx = gs.GSContext()

  @cros_test_lib.NetworkTest()
  def testLock(self):
    """Test getting a lock."""
    # Force a known host name.
    self.PatchObject(cros_build_lib, 'MachineDetails', return_value='TestHost')

    with gs.TemporaryURL('gslock') as lock_uri:
      lock = gslock.Lock(lock_uri)

      self.assertFalse(self.ctx.Exists(lock_uri))
      lock.Acquire()
      self.assertTrue(self.ctx.Exists(lock_uri))

      contents = self.ctx.Cat(lock_uri)
      self.assertEqual(contents, 'TestHost')

      lock.Release()
      self.assertFalse(self.ctx.Exists(lock_uri))

  @cros_test_lib.NetworkTest()
  def testLockRepetition(self):
    """Test aquiring same lock multiple times."""
    # Force a known host name.
    self.PatchObject(cros_build_lib, 'MachineDetails', return_value='TestHost')

    with gs.TemporaryURL('gslock') as lock_uri:
      lock = gslock.Lock(lock_uri)

      self.assertFalse(self.ctx.Exists(lock_uri))
      lock.Acquire()
      self.assertTrue(self.ctx.Exists(lock_uri))

      lock.Acquire()
      self.assertTrue(self.ctx.Exists(lock_uri))

      lock.Release()
      self.assertFalse(self.ctx.Exists(lock_uri))

      lock.Acquire()
      self.assertTrue(self.ctx.Exists(lock_uri))

      lock.Release()
      self.assertFalse(self.ctx.Exists(lock_uri))

  @cros_test_lib.NetworkTest()
  def testLockConflict(self):
    """Test lock conflict."""
    with gs.TemporaryURL('gslock') as lock_uri:
      lock1 = gslock.Lock(lock_uri)
      lock2 = gslock.Lock(lock_uri)

      # Manually lock 1, and ensure lock2 can't lock.
      lock1.Acquire()
      self.assertRaises(gslock.LockNotAcquired, lock2.Acquire)
      lock1.Release()

      # Use a with clause on 2, and ensure 1 can't lock.
      with lock2:
        self.assertRaises(gslock.LockNotAcquired, lock1.Acquire)

      # Ensure we can renew a given lock.
      lock1.Acquire()
      lock1.Renew()
      lock1.Release()

      # Ensure we get an error renewing a lock we don't hold.
      self.assertRaises(gslock.LockNotAcquired, lock1.Renew)

  @cros_test_lib.NetworkTest()
  def testLockTimeout(self):
    """Test getting a lock when an old timed out one is present."""
    with gs.TemporaryURL('gslock') as lock_uri:
      # Both locks are always timed out.
      lock1 = gslock.Lock(lock_uri, lock_timeout_mins=-1)
      lock2 = gslock.Lock(lock_uri, lock_timeout_mins=-1)

      lock1.Acquire()
      lock2.Acquire()

  @cros_test_lib.NetworkTest()
  def testRaceToAcquire(self):
    """Have lots of processes race to acquire the same lock."""
    count = self.NUM_THREADS
    pool = multiprocessing.Pool(processes=count)
    with gs.TemporaryURL('gslock') as lock_uri:
      results = pool.map(_InProcessAcquire, [lock_uri] * count)

      # Clean up the lock since the processes explicitly only acquire.
      self.ctx.Remove(lock_uri)

      # Ensure that only one of them got the lock.
      self.assertEqual(results.count(True), 1)

  @cros_test_lib.NetworkTest()
  def testRaceToDoubleAcquire(self):
    """Have lots of processes race to double acquire the same lock."""
    count = self.NUM_THREADS
    pool = multiprocessing.Pool(processes=count)
    with gs.TemporaryURL('gslock') as lock_uri:
      results = pool.map(_InProcessDoubleAcquire, [lock_uri] * count)

      # Clean up the lock sinc the processes explicitly only acquire.
      self.ctx.Remove(lock_uri)

      # Ensure that only one of them got the lock (and got it twice).
      self.assertEqual(results.count(0), count - 1)
      self.assertEqual(results.count(2), 1)

  @cros_test_lib.NetworkTest()
  def testMultiProcessDataUpdate(self):
    """Have lots of processes update a GS file proctected by a lock."""
    count = self.NUM_THREADS
    pool = multiprocessing.Pool(processes=count)
    with gs.TemporaryURL('gslock') as lock_uri:
      data_uri = lock_uri + '.data'
      results = pool.map(_InProcessDataUpdate,
                         [(lock_uri, data_uri)] * count)

      self.assertEqual(self.ctx.Cat(data_uri), str(count))

      # Ensure that all report success
      self.assertEqual(results.count(True), count)

  @cros_test_lib.NetworkTest()
  def testDryrunLock(self):
    """Ensure that lcok can be obtained and released in dry-run mode."""
    with gs.TemporaryURL('gslock') as lock_uri:
      lock = gslock.Lock(lock_uri, dry_run=True)
      self.assertIsNone(lock.Acquire())
      self.assertFalse(self.ctx.Exists(lock_uri))
      self.assertIsNone(lock.Release())

  @cros_test_lib.NetworkTest()
  def testDryrunLockRepetition(self):
    """Test aquiring same lock multiple times in dry-run mode."""
    with gs.TemporaryURL('gslock') as lock_uri:
      lock = gslock.Lock(lock_uri, dry_run=True)
      self.assertIsNone(lock.Acquire())
      self.assertIsNone(lock.Acquire())
      self.assertIsNone(lock.Release())
      self.assertIsNone(lock.Acquire())
      self.assertIsNone(lock.Release())
