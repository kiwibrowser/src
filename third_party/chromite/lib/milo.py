# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helpers for interacting with LUCI Milo service."""

from __future__ import print_function

import base64
import collections
import json

from chromite.lib import prpc
from chromite.cbuildbot import topology


class MiloClient(prpc.PRPCClient):
  """Milo client to interact with the LUCI Milo service."""

  def _GetHost(self):
    """Get LUCI Milo Server host from topology."""
    return topology.topology.get(topology.LUCI_MILO_HOST_KEY)

  def GetBuildbotBuildJSON(self, master, builder, build_num, dryrun=False):
    """Get Buildbot build summary JSON file.

    Args:
      master: waterfall master to query.
      builder: builder to query.
      build_num: build number to query.
      dryrun: Whether a dryrun.

    Returns:
      Dictionary corresponding to parsed JSON file.
    """
    body = json.dumps({
        'master': master,
        'builder': builder,
        'build_num': int(build_num),
    })
    resp = self.SendRequest('prpc/milo.Buildbot', 'GetBuildbotBuildJSON',
                            body, dryrun=dryrun)
    data = base64.b64decode(resp['data'])
    if not data:
      return None
    result = json.loads(data)
    properties = {p[0] : p[1] for p in result['properties']}
    result['properties'] = properties
    steps = {step['name'] : step for step in result['steps']}
    result['steps'] = steps
    result['masterName'] = master
    return result

  def BuildInfoGetBuildbot(self, master, builder, build_num,
                           project_hint=None, dryrun=False):
    """Get BuildInfo corresponding to Buildbot build.

    Args:
      master: waterfall master to query.
      builder: builder to query.
      build_num: build number to query.
      project_hint: Logdog project hint.
      dryrun: Whether a dryrun.

    Returns:
      Dictionary of response protobuf including extra key 'steps'.
      'steps' value is a collections.OrderedDict() keyed by tuple of
      step name path (and a scalar of level 1 steps) to the steps.

      The protobuf allows duplicates but the steps dictionary only
      includes the most recent occurrence.
    """
    request = {
        'buildbot': {
            'masterName': master,
            'builderName': builder,
            'buildNumber': int(build_num),
        },
    }
    if project_hint is not None:
      request['projectHint'] = project_hint

    result = self.SendRequest('prpc/milo.BuildInfo', 'Get',
                              json.dumps(request), dryrun=dryrun)

    def AddSteps(steps, step, root):
      """Recursive helper function to build mapping of step names to steps."""
      if step is not None:
        # Build a tuple of the path to the step.  Root is always unnamed
        # step so start keys at the level 1 substeps.
        # Level 0: root = (), name = (None,), key = (None,)
        # Level 1: root = (None,), name = (xyz,), key = (xyz,)
        # Level 2: root = (xyz,), name = (abc,), key = (xyz,abc)
        name = (step.get('name'),)
        key = (root + name) if root != (None,) else name
        # Add both the full path tuple, and the scalar value for level 1 steps.
        # This allows handling most lookups as buildinfo['steps']['cidb name'].
        steps[key] = step
        if len(key) == 1:
          steps[key[0]] = step
        # Recurse.
        for substep in step.get('substep', []):
          AddSteps(steps, substep.get('step', None), key)

    steps = collections.OrderedDict()
    AddSteps(steps, result.get('step'), ())
    result['steps'] = steps
    return result
