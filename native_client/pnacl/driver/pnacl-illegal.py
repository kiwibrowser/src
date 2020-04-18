#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from driver_log import Log, StringifyCommand
from driver_env import env

def main(argv):
  argv = [env.getone('DRIVER_PATH')] + argv
  Log.Fatal('ILLEGAL COMMAND: ' + StringifyCommand(argv))
