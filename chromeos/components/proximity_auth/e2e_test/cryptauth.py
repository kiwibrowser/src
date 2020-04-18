# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import httplib
import json
import logging
import pprint
import time

logger = logging.getLogger('proximity_auth.%s' % __name__)

_GOOGLE_APIS_URL = 'www.googleapis.com'
_REQUEST_PATH = '/cryptauth/v1/%s?alt=JSON'

class CryptAuthClient(object):
  """ A client for making blocking CryptAuth API calls. """

  def __init__(self, access_token, google_apis_url=_GOOGLE_APIS_URL):
    self._access_token = access_token
    self._google_apis_url = google_apis_url

  def GetMyDevices(self):
    """ Invokes the GetMyDevices API.

    Returns:
      A list of devices or None if the API call fails.
      Each device is a dictionary of the deserialized JSON returned by
      CryptAuth.
    """
    request_data = {
      'approvedForUnlockRequired': False,
      'allowStaleRead': False,
      'invocationReason': 13 # REASON_MANUAL
    }
    response = self._SendRequest('deviceSync/getmydevices', request_data)
    return response['devices'] if response is not None else None

  def GetUnlockKey(self):
    """
    Returns:
      The unlock key registered with CryptAuth if it exists or None.
      The device is a dictionary of the deserialized JSON returned by CryptAuth.
    """
    devices = self.GetMyDevices()
    if devices is None:
      return None

    for device in devices:
      if device['unlockKey']:
        return device
    return None

  def ToggleEasyUnlock(self, enable, public_key=''):
    """ Calls the ToggleEasyUnlock API.
    Args:
      enable: True to designate the device specified by |public_key| as an
          unlock key.
      public_key: The public key of the device to toggle. Ignored if |enable| is
          False, which toggles all unlock keys off.
    Returns:
      True upon success, else False.
    """
    request_data = { 'enable': enable, }
    if not enable:
      request_data['applyToAll'] = True
    else:
      request_data['publicKey'] = public_key
    response = self._SendRequest('deviceSync/toggleeasyunlock', request_data)
    return response is not None

  def FindEligibleUnlockDevices(self, time_delta_millis=None):
    """ Finds devices eligible to be an unlock key.
    Args:
      time_delta_millis: If specified, then only return eligible devices that
          have contacted CryptAuth in the last time delta.
    Returns:
      A tuple containing two lists, one of eligible devices and the other of
      ineligible devices.
      Each device is a dictionary of the deserialized JSON returned by
      CryptAuth.
    """
    request_data = {}
    if time_delta_millis is not None:
      request_data['maxLastUpdateTimeDeltaMillis'] = time_delta_millis * 1000;

    response = self._SendRequest(
        'deviceSync/findeligibleunlockdevices', request_data)
    if response is None:
      return None
    eligibleDevices = (
        response['eligibleDevices'] if 'eligibleDevices' in response else [])
    ineligibleDevices = (
        response['ineligibleDevices'] if (
            'ineligibleDevices' in response) else [])
    return eligibleDevices, ineligibleDevices

  def PingPhones(self, timeout_secs=10):
    """ Asks CryptAuth to ping registered phones and determine their status.
    Args:
      timeout_secs: The number of seconds to wait for phones to respond.
    Returns:
      A tuple containing two lists, one of eligible devices and the other of
      ineligible devices.
      Each device is a dictionary of the deserialized JSON returned by
      CryptAuth.
    """
    response = self._SendRequest(
        'deviceSync/senddevicesynctickle',
        { 'tickleType': 'updateEnrollment' })
    if response is None:
      return None
    # We wait for phones to update their status with CryptAuth.
    logger.info('Waiting for %s seconds for phone status...' % timeout_secs)
    time.sleep(timeout_secs)
    return self.FindEligibleUnlockDevices(time_delta_millis=timeout_secs)

  def _SendRequest(self, function_path, request_data):
    """ Sends an HTTP request to CryptAuth and returns the deserialized
        response.
    """
    conn = httplib.HTTPSConnection(self._google_apis_url)
    path = _REQUEST_PATH % function_path

    headers = {
      'authorization': 'Bearer ' + self._access_token,
      'Content-Type': 'application/json'
    }
    body = json.dumps(request_data)
    logger.info('Making request to %s with body:\n%s' % (
                path, pprint.pformat(request_data)))
    conn.request('POST', path, body, headers)

    response = conn.getresponse()
    if response.status == 204:
      return {}
    if response.status != 200:
      logger.warning('Request to %s failed: %s' % (path, response.status))
      return None
    return json.loads(response.read())
