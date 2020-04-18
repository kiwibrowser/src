# -*- python -*-
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class DisconnectTest(gdb_test.GdbTest):
  """Test that graceful disconnect and gdb restart works."""

  def Hangup(self):
    self.gdb.Disconnect()
    self.gdb.Quit()
    self.gdb = None

  def test_disconnect(self):
    # Confirm that when you connect and disconnect the debugger that you stay
    # at the same place and that breakpoints continue to be preserved.
    for breakpoint, expected_value in [
        ['test_disconnect', '0'],
        ['test_disconnect_layer2', '100001'],
        ['test_disconnect_layer3', '100002']]:
      # Break on the next location.
      self.gdb.Command('break %s' % breakpoint)
      # Run to that breakpoint.
      self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
      # Check for the expected value set at that point.
      self.assertEquals(self.gdb.Eval('global_var'), expected_value)
      # Confirm you can use the gdb disconnect and reconnect commands.
      self.gdb.Disconnect()
      self.gdb.Reconnect()
      self.assertEquals(self.gdb.Eval('global_var'), expected_value)
      # Confirm you can hangup and reconnect with a new gdb session.
      self.Hangup()
      self.LaunchGdb()
      self.assertEquals(self.gdb.Eval('global_var'), expected_value)


class DisconnectAbruptlyTest(DisconnectTest):
  """Test that an abrupt disconnect and gdb restart works."""

  def Hangup(self):
    self.gdb.KillProcess()
    self.gdb = None


if __name__ == '__main__':
  gdb_test.Main()
