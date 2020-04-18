# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for net_metrics."""

# pylint: disable=protected-access

from __future__ import absolute_import
from __future__ import print_function

import mock
import socket

import psutil

from chromite.lib import cros_test_lib
from chromite.scripts.sysmon import net_metrics


snetio = psutil._common.snetio
snicstats = psutil._common.snicstats
snic = psutil._common.snic


class TestNetMetrics(cros_test_lib.TestCase):
  """Tests for net_metrics."""

  def setUp(self):
    patcher = mock.patch('infra_libs.ts_mon.common.interface.state.store',
                         autospec=True)
    self.store = patcher.start()
    self.addCleanup(patcher.stop)

  def test_collect(self):
    with mock.patch('psutil.net_io_counters', autospec=True) \
             as net_io_counters, \
         mock.patch('psutil.net_if_stats', autospec=True) as net_if_stats, \
         mock.patch('socket.getfqdn', autospec=True) as getfqdn, \
         mock.patch('psutil.net_if_addrs', autospec=True) as net_if_addrs:
      net_io_counters.return_value = {
          'lo': snetio(
              bytes_sent=17247495681, bytes_recv=172474956,
              packets_sent=109564455, packets_recv=1095644,
              errin=10, errout=1, dropin=20, dropout=2),
      }
      net_if_stats.return_value = {
          'lo': snicstats(isup=True, duplex=0, speed=0, mtu=65536),
      }
      getfqdn.return_value = 'foo.example.com'
      net_if_addrs.return_value = {
          'lo': [
              snic(family=psutil.AF_LINK, address='11:22:33:44:55:66',
                   netmask=None, broadcast=None, ptp=None),
              snic(family=socket.AF_INET, address='10.1.1.1', netmask=None,
                   broadcast=None, ptp=None),
              snic(family=socket.AF_INET6,
                   address='fc00:0000:0000:0000:0000:0000:0000:0001',
                   netmask=None, broadcast=None, ptp=None),
          ],
      }
      net_metrics.collect_net_info()

    setter = self.store.set
    calls = [
        mock.call('dev/net/bytes', ('up', 'lo'), None,
                  17247495681, enforce_ge=mock.ANY),
        mock.call('dev/net/bytes', ('down', 'lo'), None,
                  172474956, enforce_ge=mock.ANY),
        mock.call('dev/net/packets', ('up', 'lo'), None,
                  109564455, enforce_ge=mock.ANY),
        mock.call('dev/net/packets', ('down', 'lo'), None,
                  1095644, enforce_ge=mock.ANY),
        mock.call('dev/net/errors', ('up', 'lo'), None,
                  1, enforce_ge=mock.ANY),
        mock.call('dev/net/errors', ('down', 'lo'), None,
                  10, enforce_ge=mock.ANY),
        mock.call('dev/net/dropped', ('up', 'lo'), None,
                  2, enforce_ge=mock.ANY),
        mock.call('dev/net/dropped', ('down', 'lo'), None,
                  20, enforce_ge=mock.ANY),
        mock.call('dev/net/isup', ('lo',), None,
                  True, enforce_ge=mock.ANY),
        mock.call('dev/net/duplex', ('lo',), None,
                  0, enforce_ge=mock.ANY),
        mock.call('dev/net/speed', ('lo',), None,
                  0, enforce_ge=mock.ANY),
        mock.call('dev/net/mtu', ('lo',), None,
                  65536, enforce_ge=mock.ANY),
        mock.call('net/fqdn', (), None,
                  'foo.example.com', enforce_ge=mock.ANY),
        mock.call('dev/net/address', ('AF_LINK', 'lo'), None,
                  '11:22:33:44:55:66', enforce_ge=mock.ANY),
        mock.call('dev/net/address', ('AF_INET', 'lo',), None,
                  '10.1.1.1', enforce_ge=mock.ANY),
        mock.call('dev/net/address', ('AF_INET6', 'lo'), None,
                  'fc00:0000:0000:0000:0000:0000:0000:0001',
                  enforce_ge=mock.ANY),
    ]
    setter.assert_has_calls(calls)
    self.assertEqual(len(setter.mock_calls), len(calls))
