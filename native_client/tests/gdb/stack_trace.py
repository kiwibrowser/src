# -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gdb_test


class StackTraceTest(gdb_test.GdbTest):

  def test_stack_trace(self):
    self.gdb.Command('break leaf_call')
    self.gdb.ResumeAndExpectStop('continue', 'breakpoint-hit')
    result = self.gdb.Command('-stack-list-frames 0 2')
    self.assertEquals(result['stack'][0]['frame']['func'], 'leaf_call')
    self.assertEquals(result['stack'][1]['frame']['func'], 'nested_calls')
    self.assertEquals(result['stack'][2]['frame']['func'], 'main')

    result = self.gdb.Command('-stack-list-arguments 1 0 1')
    self.assertEquals(result['stack-args'][0]['frame']['args'][0]['value'], '2')
    self.assertEquals(result['stack-args'][1]['frame']['args'][0]['value'], '1')
    self.gdb.Command('return')
    self.gdb.ResumeAndExpectStop('finish', 'function-finished')
    self.assertEquals(self.gdb.Eval('global_var'), '1')


if __name__ == '__main__':
  gdb_test.Main()
