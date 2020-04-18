# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Script that exercises the Smart Lock setup flow, testing that a nearby phone
can be found and used to unlock a Chromebook.

Note: This script does not currently automate Android phones, so make sure that
a phone is properly configured and online before starting the test.

Usage:
  python setup_test.py --remote_address REMOTE_ADDRESS
                       --username USERNAME
                       --password PASSWORD
                       [--app_path APP_PATH]
                       [--ssh_port SSH_PORT]
                       [--cryptauth_staging_url STAGING_URL]
  If |--app_path| is provided, then a copy of the Smart Lock app on the local
  machine will be used instead of the app on the ChromeOS device.
"""

import argparse
import cros
import cryptauth
import logging
import os
import subprocess
import sys
import tempfile

logger = logging.getLogger('proximity_auth.%s' % __name__)

class SmartLockSetupError(Exception):
  pass

def pingable_address(address):
  try:
    subprocess.check_output(['ping', '-c', '1', '-W', '1', address])
  except subprocess.CalledProcessError:
    raise argparse.ArgumentError('%s cannot be reached.' % address)
  return address

def email(arg):
  tokens = arg.lower().split('@')
  if len(tokens) != 2 or '.' not in tokens[1]:
    raise argparse.ArgumentError('%s is not a valid email address' % arg)
  name, domain = tokens
  if domain == 'gmail.com':
    name = name.replace('.', '')
  return '@'.join([name, domain])

def directory(path):
  if not os.path.isdir(path):
    raise argparse.ArgumentError('%s is not a directory' % path)
  return path

def ParseArgs():
  parser = argparse.ArgumentParser(prog='python setup_test.py')
  parser.add_argument('--remote_address', required=True, type=pingable_address)
  parser.add_argument('--username', required=True, type=email)
  parser.add_argument('--password', required=True)
  parser.add_argument('--ssh_port', type=int)
  parser.add_argument('--app_path', type=directory)
  parser.add_argument('--cryptauth_staging_url', type=str)
  args = parser.parse_args()
  return args

def CheckCryptAuthState(access_token):
  cryptauth_client = cryptauth.CryptAuthClient(access_token)

  # Check if we can make CryptAuth requests.
  if cryptauth_client.GetMyDevices() is None:
    logger.error('Cannot reach CryptAuth on test machine.')
    return False

  if cryptauth_client.GetUnlockKey() is not None:
    logger.info('Smart Lock currently enabled, turning off on Cryptauth...')
    if not cryptauth_client.ToggleEasyUnlock(False):
      logger.error('ToggleEasyUnlock request failed.')
      return False

  result = cryptauth_client.FindEligibleUnlockDevices()
  if result is None:
    logger.error('FindEligibleUnlockDevices request failed')
    return False
  eligibleDevices, _ = result
  if len(eligibleDevices) == 0:
    logger.warn('No eligible phones found, trying to ping phones...')
    result = cryptauth_client.PingPhones()
    if result is None or not len(result[0]):
      logger.error('Pinging phones failed :(')
      return False
    else:
      logger.info('Pinging phones succeeded!')
  else:
    logger.info('Found eligible device: %s' % (
                eligibleDevices[0]['friendlyDeviceName']))
  return True

def _NavigateSetupDialog(chromeos, app):
  logger.info('Scanning for nearby phones...')
  btmon = chromeos.RunBtmon()
  find_phone_success = app.FindPhone()
  btmon.terminate()

  if not find_phone_success:
    fd, filepath = tempfile.mkstemp(prefix='btmon-')
    os.write(fd, btmon.stdout.read())
    os.close(fd)
    logger.info('Logs for btmon can be found at %s' % filepath)
    raise SmartLockSetupError("Failed to find nearby phone.")

  logger.info('Phone found! Starting pairing...')
  if not app.PairPhone():
    raise SmartLockSetupError("Failed to pair with phone.")
  logger.info('Pairing success! Starting trial run...')
  app.StartTrialRun()

  logger.info('Unlocking for trial run...')
  lock_screen = chromeos.GetAccountPickerScreen()
  lock_screen.WaitForSmartLockState(
      lock_screen.SmartLockState.AUTHENTICATED)
  lock_screen.UnlockWithClick()

  logger.info('Trial run success! Dismissing app...')
  app.DismissApp()

def RunSetupTest(args):
  logger.info('Starting test for %s at %s' % (
              args.username, args.remote_address))
  if args.app_path is not None:
    logger.info('Replacing Smart Lock app with %s' % args.app_path)

  chromeos = cros.ChromeOS(
      args.remote_address, args.username, args.password, ssh_port=args.ssh_port)
  with chromeos.Start(local_app_path=args.app_path):
    logger.info('Chrome initialized')

    # TODO(tengs): The access token is currently fetched from the Smart Lock
    # app's background page. To be more robust, we should instead mint the
    # access token ourselves.
    if not CheckCryptAuthState(chromeos.cryptauth_access_token):
      raise SmartLockSetupError('Failed to check CryptAuth state')

    logger.info('Opening Smart Lock settings...')
    settings = chromeos.GetSmartLockSettings()
    assert(not settings.is_smart_lock_enabled)

    if args.cryptauth_staging_url is not None:
      chromeos.SetCryptAuthStaging(args.cryptauth_staging_url)

    logger.info('Starting Smart Lock setup flow...')
    app = settings.StartSetupAndReturnApp()

    if app is None:
      raise SmartLockSetupError('Failed to obtain set up app window')

    _NavigateSetupDialog(chromeos, app)

def main():
  logging.basicConfig()
  logging.getLogger('proximity_auth').setLevel(logging.INFO)
  args = ParseArgs()
  RunSetupTest(args)

if __name__ == '__main__':
  main()
