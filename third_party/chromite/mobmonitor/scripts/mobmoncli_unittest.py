# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the Mob* Monitor CLI script."""

from __future__ import print_function

import mock

from chromite.lib import cros_test_lib
from chromite.mobmonitor.rpc import rpc
from chromite.mobmonitor.scripts import mobmoncli


class MobMonCliHelper(cros_test_lib.MockTestCase):
  """Unittests for MobMonCli helper functions."""

  def testInputsToArgs(self):
    """Test converting string inputs to an args list and kwargs dict."""
    inputs = '1,2,3,4,a=5,b=6,c=7'
    expected_args = ['1', '2', '3', '4']
    expected_kwargs = {'a': '5', 'b': '6', 'c': '7'}
    self.assertEquals((expected_args, expected_kwargs),
                      mobmoncli.InputsToArgs(inputs))

  def testInputsNone(self):
    """Test InputsToArgs when the user does not pass arguments."""
    self.assertEquals(([], {}), mobmoncli.InputsToArgs(None))

  def testInputsToArgMalformed(self):
    """Test InputsToArgs when the inputs are not well-formed."""
    bad_args = [',', 'a=1,', ',1', '1,2,a=1,,b=2']
    for bad_arg in bad_args:
      with self.assertRaises(ValueError):
        mobmoncli.InputsToArgs(bad_arg)


class MobMonCliTest(cros_test_lib.MockTestCase):
  """Unittests for the MobMonCli."""

  def setUp(self):
    """Setup for MobMonCli tests."""
    self.cli = mobmoncli.MobMonCli()

  def testBadRequest(self):
    """Test that we error when an unrecognized request is passed."""
    with self.assertRaises(rpc.RpcError):
      self.cli.ExecuteRequest('InvalidRequest', 'TestService', '', '', '')

  def testGetServiceList(self):
    """Test that we correctly execute a GetServiceList RPC."""
    with mock.patch('chromite.mobmonitor.rpc.rpc.RpcExecutor') as rpc_executor:
      mock_executor = mock.MagicMock()
      rpc_executor.return_value = mock_executor
      self.cli.ExecuteRequest('GetServiceList', 'TestService', '', '', '')
      self.assertTrue(mock_executor.GetServiceList.called)

  def testGetStatus(self):
    """Test that we correctly execute a GetStatus RPC."""
    with mock.patch('chromite.mobmonitor.rpc.rpc.RpcExecutor') as rpc_executor:
      mock_executor = mock.MagicMock()
      rpc_executor.return_value = mock_executor
      self.cli.ExecuteRequest('GetStatus', 'TestService', '', '', '')
      self.assertTrue(mock_executor.GetStatus.called)

  def testActionInfo(self):
    """Test that we correctly execute an ActionInfo RPC."""
    with mock.patch('chromite.mobmonitor.rpc.rpc.RpcExecutor') as rpc_executor:
      mock_executor = mock.MagicMock()
      rpc_executor.return_value = mock_executor
      self.cli.ExecuteRequest('ActionInfo', 'TestService',
                              'healthcheck', 'action', '')
      self.assertTrue(mock_executor.ActionInfo.called)

  def testRepairService(self):
    """Test that we correctly execute a RepairService RPC."""
    with mock.patch('chromite.mobmonitor.rpc.rpc.RpcExecutor') as rpc_executor:
      mock_executor = mock.MagicMock()
      rpc_executor.return_value = mock_executor
      self.cli.ExecuteRequest('RepairService', 'TestService', '', '', '')
      self.assertTrue(mock_executor.RepairService.called)
