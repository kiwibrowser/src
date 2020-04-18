# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Makes an existing per-user Chrome appear to be multi-install Chrome.

This script makes minimal mutations to the Windows registry to make ordinary
single-install Chrome appear to be multi-install for purposes of testing multi-
to single- migrations.
"""

import _winreg
import argparse
import sys


def MakeChromeMulti(chrome_long_name, chrome_clients_key,
                    chrome_client_state_key, binaries_clients_key):
  # Update the control panel's uninstall string.
  key = _winreg.OpenKey(_winreg.HKEY_CURRENT_USER,
                        ('Software\\Microsoft\\Windows\\CurrentVersion\\'
                         'Uninstall\\%s' % chrome_long_name), 0,
                        _winreg.KEY_QUERY_VALUE | _winreg.KEY_SET_VALUE |
                        _winreg.KEY_WOW64_32KEY)
  string = _winreg.QueryValueEx(key, 'UninstallString')[0]
  string = string.replace('--uninstall', '--uninstall --multi-install --chrome')
  _winreg.SetValueEx(key, 'UninstallString', 0, _winreg.REG_SZ, string)

  # Read Chrome's version number.
  key = _winreg.OpenKey(_winreg.HKEY_CURRENT_USER,
                        chrome_clients_key, 0,
                        _winreg.KEY_QUERY_VALUE | _winreg.KEY_WOW64_32KEY)
  pv = _winreg.QueryValueEx(key, 'pv')[0]
  _winreg.CloseKey(key)

  # Write that version for the binaries.
  key = _winreg.CreateKeyEx(_winreg.HKEY_CURRENT_USER,
                            binaries_clients_key, 0,
                            _winreg.KEY_SET_VALUE | _winreg.KEY_WOW64_32KEY)
  _winreg.SetValueEx(key, 'pv', 0, _winreg.REG_SZ, pv)
  _winreg.CloseKey(key)

  # Add "--multi-install --chrome" to Chrome's UninstallArguments.
  key = _winreg.OpenKey(_winreg.HKEY_CURRENT_USER,
                        chrome_client_state_key, 0,
                        _winreg.KEY_QUERY_VALUE | _winreg.KEY_SET_VALUE |
                        _winreg.KEY_WOW64_32KEY)
  args = _winreg.QueryValueEx(key, 'UninstallArguments')[0]
  args += ' --multi-install --chrome'
  _winreg.SetValueEx(key, 'UninstallArguments', 0, _winreg.REG_SZ, args)


def main():
  parser = argparse.ArgumentParser(
    description='Transforms single-install Chrome into multi-install.')
  parser.add_argument('--chrome-long-name', default='Google Chrome',
                      help='The full name of the product.')
  parser.add_argument('--chrome-clients-key',
                      default='Software\\Google\\Update\\Clients\\'
                      '{8A69D345-D564-463c-AFF1-A69D9E530F96}',
                      help='Chrome\'s Clients registry key path.')
  parser.add_argument('--chrome-client-state-key',
                      default='Software\\Google\\Update\\ClientState\\'
                      '{8A69D345-D564-463c-AFF1-A69D9E530F96}',
                      help='Chrome\'s ClientState registry key path.')
  parser.add_argument('--binaries-clients-key',
                      default='Software\\Google\\Update\\Clients\\'
                      '{4DC8B4CA-1BDA-483e-B5FA-D3C12E15B62D}',
                      help='Chrome Binaries\' Clients registry key path.')
  args = parser.parse_args()
  MakeChromeMulti(args.chrome_long_name,
                  args.chrome_clients_key,
                  args.chrome_client_state_key,
                  args.binaries_clients_key)
  return 0


if __name__ == '__main__':
  sys.exit(main())
