# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Service for tracking isolates and looking them up by builder and commit.

An isolate is a way to describe the dependencies of a specific build.

More about isolates:
https://github.com/luci/luci-py/blob/master/appengine/isolate/doc/client/Design.md
"""

import json

import webapp2

from dashboard.common import utils
from dashboard.pinpoint.models import change as change_module
from dashboard.pinpoint.models import isolate


# TODO: Use Cloud Endpoints to make a proper API with a proper response.
class Isolate(webapp2.RequestHandler):
  """Handler for managing isolates.

  A post request adds new isolate information.
  A get request looks up an isolate hash from the builder, commit, and target.
  """

  def get(self):
    """Look up an isolate hash.

    Args:
      builder_name: The name of the builder that produced the isolate.
      change: The Change the isolate is for, as a JSON string.
      target: The isolate target.
    """
    # Get parameters.
    parameters = (
        ('builder_name', str),
        ('change', lambda x: change_module.Change.FromDict(json.loads(x))),
        ('target', str),
    )
    try:
      # pylint: disable=unbalanced-tuple-unpacking
      builder_name, change, target = self._ValidateParameters(parameters)
    except (KeyError, TypeError, ValueError) as e:
      self.response.set_status(400)
      self.response.write(e)
      return

    # Get.
    try:
      isolate_server, isolate_hash = isolate.Get(builder_name, change, target)
    except KeyError as e:
      self.response.set_status(404)
      self.response.write(e)
      return

    self.response.write(json.dumps({
        'isolate_server': isolate_server,
        'isolate_hash': isolate_hash,
    }))

  def post(self):
    """Add new isolate information.

    Args:
      builder_name: The name of the builder that produced the isolate.
      change: The Change the isolate is for, as a JSON string.
      isolate_server: The hostname of the server where the isolates are stored.
      isolate_map: A JSON dict mapping the target names to the isolate hashes.
    """
    # Check permissions.
    if self.request.remote_addr not in utils.GetIpWhitelist():
      self.response.set_status(403)
      self.response.write('Permission denied')
      return

    # Get parameters.
    parameters = (
        ('builder_name', str),
        ('change', lambda x: change_module.Change.FromDict(json.loads(x))),
        ('isolate_server', str),
        ('isolate_map', json.loads),
    )
    try:
      # pylint: disable=unbalanced-tuple-unpacking
      builder_name, change, isolate_server, isolate_map = (
          self._ValidateParameters(parameters))
    except (KeyError, TypeError, ValueError) as e:
      self.response.set_status(400)
      self.response.write(e)
      return

    # Put information into the datastore.
    isolate_infos = {(builder_name, change, target, isolate_hash)
                     for target, isolate_hash in isolate_map.iteritems()}
    isolate.Put(isolate_server, isolate_infos)

  def _ValidateParameters(self, parameters):
    """Ensure the right parameters are present and valid.

    Args:
      parameters: Iterable of (name, converter) tuples where name is the
                  parameter name and converter is a function used to validate
                  and convert that parameter into its internal representation.

    Returns:
      A list of parsed parameter values.

    Raises:
      TypeError: The wrong parameters are present.
      ValueError: The parameters have invalid values.
    """
    parameter_names = tuple(parameter_name for parameter_name, _ in parameters)
    for given_parameter in self.request.params:
      if given_parameter not in parameter_names:
        raise TypeError('Unknown parameter: %s' % given_parameter)

    parameter_values = []

    for parameter_name, parameter_converter in parameters:
      if parameter_name not in self.request.params:
        raise TypeError('Missing parameter: %s' % parameter_name)

      parameter_value = self.request.get(parameter_name)
      if not parameter_value:
        raise ValueError('Empty parameter: %s' % parameter_name)

      parameter_value = parameter_converter(parameter_value)

      parameter_values.append(parameter_value)

    return parameter_values
