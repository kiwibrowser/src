# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import subprocess
import threading


MIN_VALID_LATENCY = 10
MAX_VALID_LATENCY = 1000


class MotophoThread(threading.Thread):
  """Handles the running of the Motopho script and extracting results."""
  def __init__(self):
    threading.Thread.__init__(self)
    self._latencies = []
    self._correlations = []
    self._thread_can_die = False
    # Threads can't be restarted, so in order to gather multiple samples, we
    # need to either re-create the thread for every iteration or use a loop
    # and locks in a single thread -> use the latter solution.
    self._start_lock = threading.Event()
    self._finish_lock = threading.Event()
    self.BlockNextIteration()

  def run(self):
    while True:
      self._WaitForIterationStart()
      if self._thread_can_die:
        break
      self._ResetEndLock()
      motopho_output = ""
      try:
        motopho_output = subprocess.check_output(["./motophopro_nograph"],
                                                 stderr=subprocess.STDOUT)
      except subprocess.CalledProcessError as e:
        logging.error('Failed to run Motopho script: %s', e.output)
        raise e

      if "FAIL" in motopho_output:
        logging.error('Failed to get latency, logging raw output: %s',
                      motopho_output)
        self._failed_iteration = True
      else:
        current_num_samples = len(self._latencies)
        for line in motopho_output.split("\n"):
          if 'Motion-to-photon latency:' in line:
            self._latencies.append(float(line.split(" ")[-2]))
          if 'Max correlation is' in line:
            self._correlations.append(float(line.split(' ')[-1]))
          if (len(self._latencies) > current_num_samples and
              len(self._correlations) > current_num_samples):
            break;
      if not self._failed_iteration:
        # Rarely, the reported latency will be lower than physically possible
        # (Single digit latency when it should be impossible to get below the
        # scan out latency of 16.7 ms) or impossibly high (tens of seconds when
        # the test lasts < 10 seconds). Clearly, these results are invalid, so
        # repeat as if we failed to get the latency at all.
        # TODO(bsheedy): Figure out the root cause of this instead of working
        # around it crbug.com/755596.
        if self._latencies[-1] < MIN_VALID_LATENCY:
          logging.error('Measured latency of %f lower than min of %d\n'
                        'Logging raw output: %s', self._latencies[-1],
                        MIN_VALID_LATENCY, motopho_output)
          self._failed_iteration = True
        elif self._latencies[-1] > MAX_VALID_LATENCY:
          logging.error('Measured latency of %f higher than max of %d\n'
                        'Logging raw output: %s', self._latencies[-1],
                        MAX_VALID_LATENCY, motopho_output)
          self._failed_iteration = True
        if self._failed_iteration:
          del self._latencies[-1]
          del self._correlations[-1]
      self._EndIteration()

  def _WaitForIterationStart(self):
    self._start_lock.wait()

  def StartIteration(self):
    """Tells the thread to start its next test iteration."""
    self._start_lock.set()

  def BlockNextIteration(self):
    """Blocks the thread from starting the next iteration without a signal."""
    self._start_lock.clear()

  def Terminate(self):
    """Tells the thread that it should self-terminate."""
    self._thread_can_die = True
    self._start_lock.set()

  def _EndIteration(self):
    self._finish_lock.set()

  def _ResetEndLock(self):
    self._finish_lock.clear()
    self._failed_iteration = False

  def WaitForIterationEnd(self, timeout):
    """Waits until the thread says it's finished or times out.

    Returns:
      Whether the iteration ended within the given timeout
    """
    return self._finish_lock.wait(timeout)

  @property
  def latencies(self):
    return self._latencies

  @property
  def correlations(self):
    return self._correlations

  @property
  def failed_iteration(self):
    return self._failed_iteration
