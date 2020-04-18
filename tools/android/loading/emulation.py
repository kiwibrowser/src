# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Device and network emulation utilities via devtools."""

import json

# Copied from
# WebKit/Source/devtools/front_end/network/NetworkConditionsSelector.js
# Units:
#   download/upload: byte/s
#   latency: ms
NETWORK_CONDITIONS = {
    'GPRS': {
        'download': 50 * 1024 / 8, 'upload': 20 * 1024 / 8, 'latency': 500},
    'Regular2G': {
        'download': 250 * 1024 / 8, 'upload': 50 * 1024 / 8, 'latency': 300},
    'Good2G': {
        'download': 450 * 1024 / 8, 'upload': 150 * 1024 / 8, 'latency': 150},
    'Regular3G': {
        'download': 750 * 1024 / 8, 'upload': 250 * 1024 / 8, 'latency': 100},
    'Good3G': {
        'download': 1.5 * 1024 * 1024 / 8, 'upload': 750 * 1024 / 8,
        'latency': 40},
    'Regular4G': {
        'download': 4 * 1024 * 1024 / 8, 'upload': 3 * 1024 * 1024 / 8,
        'latency': 20},
    'DSL': {
        'download': 2 * 1024 * 1024 / 8, 'upload': 1 * 1024 * 1024 / 8,
        'latency': 5},
    'WiFi': {
        'download': 30 * 1024 * 1024 / 8, 'upload': 15 * 1024 * 1024 / 8,
        'latency': 2}
}


def LoadEmulatedDevices(registry):
  """Loads a list of emulated devices from the DevTools JSON registry.

  See, for example, third_party/WebKit/Source/devtools/front_end
  /emulated_devices/module.json.

  Args:
    registry: A file-like object for the device registry (should be JSON).

  Returns:
    {'device_name': device}
  """
  json_dict = json.load(registry)
  devices = {}
  for device in json_dict['extensions']:
    device = device['device']
    devices[device['title']] = device
  return devices


def SetUpDeviceEmulationAndReturnMetadata(connection, device):
  """Configures an instance of Chrome for device emulation.

  Args:
    connection: (DevToolsConnection)
    device: (dict) An entry from LoadEmulatedDevices().

  Returns:
    A dict containing the device emulation metadata.
  """
  res = connection.SyncRequest('Emulation.canEmulate')
  assert res['result'], 'Cannot set device emulation.'
  data = _GetDeviceEmulationMetadata(device)
  connection.SyncRequestNoResponse(
      'Emulation.setDeviceMetricsOverride',
      {'width': data['width'],
       'height': data['height'],
       'deviceScaleFactor': data['deviceScaleFactor'],
       'mobile': data['mobile'],
       'fitWindow': True})
  connection.SyncRequestNoResponse('Network.setUserAgentOverride',
                                   {'userAgent': data['userAgent']})
  return data


def SetUpNetworkEmulation(connection, latency, download, upload):
  """Configures an instance of Chrome for network emulation.

  See NETWORK_CONDITIONS for example (or valid?) emulation options.

  Args:
    connection: (DevToolsConnection)
    latency: (float) Latency in ms.
    download: (float) Download speed (Bytes / s).
    upload: (float) Upload speed (Bytes / s).
  """
  res = connection.SyncRequest('Network.canEmulateNetworkConditions')
  assert res['result'], 'Cannot set network emulation.'
  connection.SyncRequestNoResponse(
      'Network.emulateNetworkConditions',
      {'offline': False, 'latency': latency, 'downloadThroughput': download,
       'uploadThroughput': upload})


def BandwidthToString(bandwidth):
  """Converts a bandwidth to string.

  Args:
    bandwidth: The bandwidth to convert in byte/s. Must be a multiple of 1024/8.

  Returns:
    A string compatible with wpr --{up,down} command line flags.
  """
  assert bandwidth % (1024/8) == 0
  bandwidth_kbps = (int(bandwidth) * 8) / 1024
  if bandwidth_kbps % 1024:
    return '{}Kbit/s'.format(bandwidth_kbps)
  return '{}Mbit/s'.format(bandwidth_kbps / 1024)


def _GetDeviceEmulationMetadata(device):
  """Returns the metadata associated with a given device."""
  return {'width': device['screen']['vertical']['width'],
          'height': device['screen']['vertical']['height'],
          'deviceScaleFactor': device['screen']['device-pixel-ratio'],
          'mobile': 'mobile' in device['capabilities'],
          'userAgent': device['user-agent']}
