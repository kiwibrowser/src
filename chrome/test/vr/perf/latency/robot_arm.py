# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import serial
import time


class RobotArm():
  """Handles the serial communication with the servos/arm used for movement."""
  def __init__(self, device_name, num_tries=5, baud=115200, timeout=3.0):
    self._connection = None
    connected = False
    for _ in xrange(num_tries):
      try:
        self._connection = serial.Serial('/dev/' + device_name,
                                         baud,
                                         timeout=timeout)
      except serial.SerialException as e:
        pass
      if self._connection and 'Enter parameters' in self._connection.read(1024):
        connected = True
        break
    if not connected:
      raise serial.SerialException('Failed to connect to the robot arm.')

  def ResetPosition(self):
    if not self._connection:
      return
    # If the servo stopped very close to the desired position, it can just
    # vibrate instead of moving, so move away before going to the reset
    # position.
    self._connection.write('5 300 0 5\n')
    time.sleep(0.5)
    self._connection.write('5 250 0 5\n')
    time.sleep(0.5)

  def StartMotophoMovement(self):
    if not self._connection:
      return
    self._connection.write('9\n')

  def StopAllMovement(self):
    if not self._connection:
      return
    self._connection.write('0\n')
    # The manual usage instructions are printed over the serial connection
    # every time we send a command - long test runs can result in the buffer
    # filling up and causing the arm to hang, so periodically clear the buffer.
    self._connection.flushInput()
