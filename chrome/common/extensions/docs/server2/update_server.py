#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import getpass
import os
import subprocess
import sys

import build_server

if __name__ == '__main__':
  additional_args = []
  if len(sys.argv) > 1 and sys.argv[1].endswith('appcfg.py'):
    appcfg_path = sys.argv[1]
    additional_args = sys.argv[2:]
  else:
    appcfg_path = None
    additional_args = sys.argv[1:]
    for path in ['.',
                 os.path.join(os.environ['HOME'], 'local', 'google_appengine'),
                 os.path.join(os.environ['HOME'], 'google_appengine'),
                 os.getcwd()] + sys.path:
      full_path = os.path.join(path, 'appcfg.py')
      if os.path.exists(full_path):
        appcfg_path = full_path
        break
    if appcfg_path is None:
      print 'appcfg.py could not be found in default paths.'
      print 'usage: update_server.py <path_to_appcfg.py> <appcfg_options>'
      exit(1)

  def run_appcfg():
    server2_path = os.path.dirname(sys.argv[0])
    subprocess.call([appcfg_path, 'update', server2_path] + additional_args)

  build_server.main()
  username = raw_input(
      'Update github username/password (empty to skip)? ')
  if username:
    password = getpass.getpass()
    with open('github_file_system.py') as f:
      contents = f.read()
    if 'USERNAME = None' not in contents:
      print 'Error: Can\'t find "USERNAME = None" in github_file_system.py.'
      exit(1)
    if 'PASSWORD = None' not in contents:
      print 'Error: Can\'t find "PASSWORD = None" in github_file_system.py.'
      exit(1)
    try:
      with open('github_file_system.py', 'w+') as f:
        f.write(
            contents.replace('PASSWORD = None', 'PASSWORD = \'%s\'' % password)
                    .replace('USERNAME = None', 'USERNAME = \'%s\'' % username))
      run_appcfg()
    finally:
      with open('github_file_system.py', 'w+') as f:
        f.write(contents)
  else:
    run_appcfg()
