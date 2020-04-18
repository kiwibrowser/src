# -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class SyscallThreadTest(gdb_test.GdbTest):

  def CheckBacktrace(self, backtrace, functions):
    all_functions = [frame['frame']['func'] for frame in backtrace]
    # Check that 'functions' is a subsequence of 'all_functions'
    s1 = '|' + '|'.join(all_functions) + '|'
    s2 = '|' + '|'.join(functions) + '|'
    self.assertIn(s2, s1, '%s not in %s' % (functions, all_functions))

  def test_syscall_thread(self):
    self.gdb.Command('break inside_f3')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    # Check we stopped in inside_f3
    backtrace = self.gdb.Command('-stack-list-frames')
    self.CheckBacktrace(backtrace['stack'], ['inside_f3', 'f3'])
    # Check we have one more thread
    thread_info = self.gdb.Command('-thread-info')
    self.assertEquals(len(thread_info['threads']), 2)
    # Select another thread
    syscall_thread_id = thread_info['threads'][0]['id']
    if syscall_thread_id == thread_info['current-thread-id']:
      syscall_thread_id = thread_info['threads'][1]['id']
    self.gdb.Command('-thread-select %s' % syscall_thread_id)
    # Check that thread waits in usleep
    backtrace = self.gdb.Command('-stack-list-frames')
    self.CheckBacktrace(
        backtrace['stack'], ['pthread_join', 'test_syscall_thread'])


if __name__ == '__main__':
  gdb_test.Main()
