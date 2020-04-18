#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# A simple native client in python.
# All this client does is echo the text it receives back at the extension.

import os
import platform
import sys
import struct

def WriteMessage(message):
  try:
    sys.stdout.write(struct.pack("I", len(message)))
    sys.stdout.write(message)
    sys.stdout.flush()
    return True
  except IOError:
    return False

def Main():
  message_number = 0

  parent_window = None

  if len(sys.argv) < 2:
    sys.stderr.write("URL of the calling application is not specified.\n")
    return 1
  caller_url = sys.argv[1]

  # TODO(sergeyu): Use argparse module to parse the arguments (not available in
  # Python 2.6).
  for arg in sys.argv[2:]:
    if arg.startswith('--'):
      if arg.startswith('--parent-window='):
        parent_window = long(arg[len('--parent-window='):])

  # Verify that the process was started in the correct directory.
  cwd = os.getcwd()
  script_path = os.path.dirname(os.path.abspath(sys.argv[0]))
  if cwd.lower() != script_path.lower():
    sys.stderr.write('Native messaging host started in a wrong directory.')
    return 1

  # Verify that --parent-window parameter is correct.
  if platform.system() == 'Windows' and parent_window:
    import win32gui
    if not win32gui.IsWindow(parent_window):
      sys.stderr.write('Invalid --parent-window.\n')
      return 1

  while 1:
    # Read the message type (first 4 bytes).
    text_length_bytes = sys.stdin.read(4)

    if len(text_length_bytes) == 0:
      break

    # Read the message length (4 bytes).
    text_length = struct.unpack('i', text_length_bytes)[0]

    # Read the text (JSON object) of the message.
    text = sys.stdin.read(text_length).decode('utf-8')

    # bigMessage() test sends a special message that is sent to verify that
    # chrome rejects messages that are too big. Try sending a message bigger
    # than 1MB after receiving a message that contains 'bigMessageTest'.
    if 'bigMessageTest' in text:
      text = '{"key": "' + ("x" * 1024 * 1024) + '"}'

    # "stopHostTest" verifies that Chrome properly handles the case when the
    # host quits before port is closed. When the test receives response it
    # will try sending second message and it should fail becasue the stdin
    # pipe will be closed at that point.
    if 'stopHostTest' in text:
      # Using os.close() here because sys.stdin.close() doesn't really close
      # the pipe (it just marks it as closed, but doesn't close the file
      # descriptor).
      os.close(sys.stdin.fileno())
      WriteMessage('{"stopped": true }')
      sys.exit(0)

    message_number += 1

    message = '{{"id": {0}, "echo": {1}, "caller_url": "{2}"}}'.format(
          message_number, text, caller_url).encode('utf-8')
    if not WriteMessage(message):
      break

if __name__ == '__main__':
  sys.exit(Main())
