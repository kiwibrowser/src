#!/usr/bin/env python2
# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Command-line interface for the Mob* Monitor."""

from __future__ import print_function

import re
import sys

from chromite.lib import commandline
from chromite.lib import remote_access
from chromite.mobmonitor.rpc import rpc


def InputsToArgs(inputs):
  """Convert repair action input string to an args list and kwargs dict.

  Args:
    inputs: A string. A well formed input string is a comma separated
      list of values and/or equal-sign separated key value pairs.
      A valid input string may be the following:
        'arg1,arg2,...,argN,kwarg1=foo,...,kwargN=bar'

  Returns:
    A list of the positional arguments contained in the |inputs| string and
    a dictionary of the key value pairs that made up the |inputs| string.
    All keys and values will be strings.
  """
  args, kwargs = ([], {})
  if not inputs:
    return args, kwargs

  pattern = '([^,]+,)*([^,]+)$'
  if not re.match(pattern, inputs):
    raise ValueError('Action arguments are not well-formed.'
                     ' Expected: "a1,...,aN,kw1=foo,...,kwN=bar".'
                     ' Given: %s', inputs)

  for kv in inputs.split(','):
    try:
      k, v = kv.split('=')
      kwargs[k] = v
    except ValueError:
      args.append(kv)

  return args, kwargs


class MobMonCli(object):
  """Provides command-line functionality for using the Mob* Monitor."""

  def __init__(self, host='localhost', port=9991):
    self.host = host
    self.port = remote_access.NormalizePort(port)

  def ExecuteRequest(self, request, service, healthcheck, action, inputs):
    """Execute the request if an appropriate RPC function is defined.

    Args:
      request: The name of the RPC.
      service: The name of the service involved in the RPC.
      healthcheck: The name of the healthcheck involved in the RPC.
      action: The action to be performed.
      inputs: A string. The inputs of the specified repair action.
    """
    rpcexec = rpc.RpcExecutor(self.host, self.port)

    if not hasattr(rpcexec, request):
      raise rpc.RpcError('The request "%s" is not recognized.' % request)

    args, kwargs = InputsToArgs(inputs)

    if 'GetServiceList' == request:
      return rpcexec.GetServiceList()

    if 'GetStatus' == request:
      return rpcexec.GetStatus(service=service)

    if 'ActionInfo' == request:
      return rpcexec.ActionInfo(service=service, healthcheck=healthcheck,
                                action=action)

    if 'RepairService' == request:
      return rpcexec.RepairService(service=service, healthcheck=healthcheck,
                                   action=action, args=args, kwargs=kwargs)


def ParseArguments(argv):
  parser = commandline.ArgumentParser()
  parser.add_argument('request', choices=rpc.RPC_LIST)
  parser.add_argument('-s', '--service', help='The service to act upon.')
  parser.add_argument('-c', '--healthcheck',
                      help='The healthcheck to act upon.')
  parser.add_argument('-a', '--action', help='The action to execute.')
  parser.add_argument('--host', default='localhost',
                      help='The hostname of the Mob* Monitor.')
  parser.add_argument('-p', '--port', type=int, default=9991,
                      help='The Mob* Monitor port.')
  parser.add_argument('-i', '--inputs',
                      help='Repair action inputs. Inputs are specified'
                           ' as a comma-separated list of values or key'
                           ' value pairs such as: "arg1,arg2,...,argN,'
                           'kwarg1=foo,...,kwargN=bar"')

  return parser.parse_args(argv)


def main(argv):
  """Command line interface for the Mob* Monitor.

  The basic syntax is:
    mobmon <request> [args]
    mobmon --help
  """
  options = ParseArguments(argv)

  cli = MobMonCli(options.host, options.port)
  result = cli.ExecuteRequest(options.request, options.service,
                              options.healthcheck, options.action,
                              options.inputs)

  print(result)


if __name__ == '__main__':
  main(sys.argv[1:])
