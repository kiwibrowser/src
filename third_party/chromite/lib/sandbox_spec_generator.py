# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Logic to generate a SandboxSpec from an appc pod manifest.

https://github.com/appc/spec/blob/master/SPEC.md
"""

from __future__ import print_function

import collections
import copy
import json
import os
import re

from chromite.lib import cros_logging as logging
from chromite.lib import json_lib
from chromite.lib import osutils
from chromite.lib import remote_access
from chromite.lib import user_db

KEY_ANNOTATIONS_LIST = 'annotations'
KEY_ANNOTATION_NAME = 'name'
KEY_ANNOTATION_VALUE = 'value'
ENDPOINT_NAME_ANNOTATION_PREFIX = 'bruteus-endpoint-'

KEY_APPS_LIST = 'apps'
KEY_APP_NAME = 'name'
KEY_APP_IMAGE = 'image'
KEY_APP_IMAGE_NAME = 'name'
KEY_APP_ISOLATORS = 'isolators'

KEY_APP_SUB_APP = 'app'
KEY_SUB_APP_USER = 'user'
KEY_SUB_APP_GROUP = 'group'
KEY_SUB_APP_EXEC = 'exec'
KEY_SUB_APP_PORTS = 'ports'

PORT_SPEC_COUNT = 'count'
PORT_SPEC_NAME = 'name'
PORT_SPEC_PORT = 'port'
PORT_SPEC_PROTOCOL = 'protocol'
PORT_SPEC_SOCKET_ACTIVATED = 'socketActivated'

PROTOCOL_TCP = 'tcp'
PROTOCOL_UDP = 'udp'
VALID_PROTOCOLS = (PROTOCOL_TCP, PROTOCOL_UDP)

ISOLATOR_KEY_NAME = 'name'
ISOLATOR_KEY_VALUE = 'value'
ISOLATOR_KEY_VALUE_SET = 'set'

ISOLATOR_NAME_PREFIX = 'os/linux/capabilities-'
ISOLATOR_NAME_RETAIN_SET = 'os/linux/capabilities-retain-set'

PortSpec = collections.namedtuple('PortSpec', ('allow_all', 'port_list'))


def IsValidAcName(name):
  """Returns true if |name| adheres to appc's AC Name Type.

  This roughly means that a string looks like a protocol-less
  URL (e.g. foo-foo/bar/bar).

  https://github.com/appc/spec/blob/master/SPEC.md#ac-name-type

  Args:
    name: string to validate.

  Returns:
    True iff |name| is a valid AC Name.
  """
  return bool(re.match(r'^[a-z0-9]+([-./][a-z0-9]+)*$', name))


class SandboxSpecWrapper(object):
  """Wrapper that knows how to set fields in a protocol buffer.

  This makes mocking out our protocol buffer interface much simpler.
  """

  def __init__(self):
    # In the context of unittests run from outside the chroot, this import
    # will fail.  Tests will mock out this entire class.
    # pylint: disable=import-error
    from generated import soma_sandbox_spec_pb2
    self.sandbox_spec = soma_sandbox_spec_pb2.SandboxSpec()

  def SetName(self, name):
    """Set the name of the runnable brick."""
    self.sandbox_spec.name = name
    self.sandbox_spec.overlay_path = '/bricks/%s' % name

  def AddExecutable(self, uid, gid, command_line, tcp_ports, udp_ports,
                    linux_caps):
    """Add an executable to the wrapped SandboxSpec.

    Args:
      uid: integer UID of the user to run this executable.
      gid: integer GID of the group to run this executable.
      command_line: list of strings to run.
      tcp_ports: list of PortSpec tuples.
      udp_ports: list of PortSpec tuples.
      linux_caps: list of string names of capabilities (e.g. 'CAP_CHOWN').
    """
    executable = self.sandbox_spec.executables.add()
    executable.uid = uid
    executable.gid = gid
    executable.command_line.extend(command_line)
    for listen_ports, ports in ((executable.tcp_listen_ports, tcp_ports),
                                (executable.udp_listen_ports, udp_ports)):
      if ports.allow_all:
        listen_ports.allow_all = True
      else:
        listen_ports.allow_all = False
        listen_ports.ports.extend(ports.port_list)

    # Map the names of caps to the appropriate protobuffer values.
    caps = [self.sandbox_spec.LinuxCaps.Value('LINUX_' + cap_name)
            for cap_name in linux_caps]
    executable.capabilities.extend(caps)

  def AddEndpointName(self, endpoint_name):
    """Adds the name of an endpoint that'll run inside this sandbox."""
    self.sandbox_spec.endpoint_names.append(endpoint_name)


def _GetPortList(desired_protocol, appc_port_list):
  """Get the list of ports opened for |desired_protocol| from |appc_port_list|.

  Args:
    desired_protocol: one of VALID_PROTOCOLS.
    appc_port_list: list of port specifications from a appc pod manifest.

  Returns:
    Instance of PortSpec.
  """
  # The port specification is optional.
  if appc_port_list is None:
    return PortSpec(False, [])

  json_lib.AssertIsInstance(appc_port_list, list, 'port specification list')

  allow_all = False
  port_list = []
  for port_dict in appc_port_list:
    json_lib.AssertIsInstance(port_dict, dict, 'port specification')
    port_dict = copy.deepcopy(port_dict)

    # By default, we open a single specified port.
    port_dict.setdefault(PORT_SPEC_COUNT, 1)
    # By default, don't set socket activated.
    port_dict.setdefault(PORT_SPEC_SOCKET_ACTIVATED, False)

    # We don't actually use the port name, but it's handy for documentation
    # and standard adherence to enforce its existence.
    port_name = json_lib.PopValueOfType(
        port_dict, PORT_SPEC_NAME, unicode, 'port name')
    logging.debug('Validating appc specifcation of "%s"', port_name)
    port = json_lib.PopValueOfType(port_dict, PORT_SPEC_PORT, int, 'port')
    protocol = json_lib.PopValueOfType(
        port_dict, PORT_SPEC_PROTOCOL, unicode, 'protocol')

    count = json_lib.PopValueOfType(
        port_dict, PORT_SPEC_COUNT, int, 'port range count')

    # We also don't use the socketActivated flag, but we should tolerate safe
    # values.
    socket_activated = json_lib.PopValueOfType(
        port_dict, PORT_SPEC_SOCKET_ACTIVATED, bool, 'socket activated flag')

    # Validate everything before acting on it.
    if protocol not in VALID_PROTOCOLS:
      raise ValueError('Port protocol must be in %r, not "%s"' %
                       (VALID_PROTOCOLS, protocol))
    if protocol != desired_protocol:
      continue

    if socket_activated != False:
      raise ValueError('No support for socketActivated==True in %s' % port_name)

    if port_dict:
      raise ValueError('Unknown keys found in port spec %s: %r' %
                       (port_name, port_dict.keys()))

    if port == -1:
      # Remember that we're going to return that all ports are opened, but
      # continue validating all the remaining specifications.
      allow_all = True
      continue

    # Now we know it's not the wildcard port, and that we've never declared
    # a wildcard for this protocol.
    port = remote_access.NormalizePort(port)

    if count < 1:
      raise ValueError('May only specify positive port ranges for %s' %
                       port_name)
    if port + count >= 65536:
      raise ValueError('Port range extends past max port number for %s' %
                       port_name)

    for effective_port in xrange(port, port + count):
      port_list.append(effective_port)

  return PortSpec(allow_all, port_list)


def _ExtractLinuxCapNames(app_dict):
  """Parses the set of Linux capabilities for an executable.

  Args:
    app_dict: dictionary defining an executable.

  Returns:
    List of names of Linux capabilities (e.g. ['CAP_CHOWN']).
  """
  if KEY_APP_ISOLATORS not in app_dict:
    return []

  isolator_list = json_lib.GetValueOfType(
      app_dict, KEY_APP_ISOLATORS, list,
      'list of isolators for application')
  linux_cap_isolators = []

  # Look for any isolators related to capability sets.
  for isolator in isolator_list:
    json_lib.AssertIsInstance(isolator, dict, 'isolator instance')
    isolator_name = json_lib.GetValueOfType(
        isolator, ISOLATOR_KEY_NAME, unicode, 'isolator name')
    if not isolator_name.startswith(ISOLATOR_NAME_PREFIX):
      continue
    if isolator_name != ISOLATOR_NAME_RETAIN_SET:
      raise ValueError('Capabilities may only be specified as %s' %
                       ISOLATOR_NAME_RETAIN_SET)
    linux_cap_isolators.append(isolator)

  # We may have only a single isolator.
  if len(linux_cap_isolators) > 1:
    raise ValueError('Found two lists of Linux caps for an executable')
  if not linux_cap_isolators:
    return []

  value = json_lib.GetValueOfType(
      linux_cap_isolators[0], ISOLATOR_KEY_VALUE, dict,
      'Linux cap isolator value')
  caps = json_lib.GetValueOfType(
      value, ISOLATOR_KEY_VALUE_SET, list, 'Linux cap isolator set')
  for cap in caps:
    json_lib.AssertIsInstance(cap, unicode, 'Linux capability in set.')

  return caps


class SandboxSpecGenerator(object):
  """Delegate that knows how to read appc manifests and write SandboxSpecs."""

  def __init__(self, sysroot):
    self._sysroot = sysroot
    self._user_db = user_db.UserDB(sysroot)

  def _CheckAbsPathToExecutable(self, path_to_binary):
    """Raises if there is no exectable at |path_to_binary|."""
    if not os.path.isabs(path_to_binary):
      raise ValueError(
          'Brick executables must be specified by absolute path, not "%s".' %
          path_to_binary)
    return True

  def _FillInEndpointNamesFromAnnotations(self, wrapper, annotations):
    """Fill in the SandboxSpec endpoint_names field from |annotations|.

    An appc pod specification can contain a list of (mostly) arbitrary
    annotations that projects can use to add their own metadata fields.
    |annotations| is a list of dicts that each contain a name and value field,
    and this method looks for 'name' fields that are prefixed with
    ENDPOINT_NAME_ANNOTATION_PREFIX and treats the associated 'value' as the
    name of an endpoint that psyched will expect to be registered from within
    this sandbox.

    Args:
      wrapper: instance of SandboxSpecWrapper.
      annotations: list of dicts, each with a name and value field.
    """
    for annotation in annotations:
      json_lib.AssertIsInstance(annotation, dict, 'a single annotation')
      name = json_lib.GetValueOfType(
          annotation, KEY_ANNOTATION_NAME, unicode, 'annotation name')
      if not IsValidAcName(name):
        raise ValueError('Annotation name "%s" contains illegal characters.' %
                         name)
      if name.startswith(ENDPOINT_NAME_ANNOTATION_PREFIX):
        endpoint_name = json_lib.GetValueOfType(
            annotation, KEY_ANNOTATION_VALUE, unicode, 'endpoint name value')
        if not IsValidAcName(name):
          raise ValueError('Endpoint name "%s" contains illegal characters.' %
                           endpoint_name)
        wrapper.AddEndpointName(endpoint_name)

  def _FillInExecutableFromApp(self, wrapper, app):
    """Fill in the fields of a SandboxSpec.Executable object from |app|.

    Args:
      wrapper: instance of SandboxSpecWrapper.
      app: dictionary of information taken from the appc pod manifest.
    """
    sub_app = json_lib.GetValueOfType(
        app, KEY_APP_SUB_APP, dict, 'per app app dict')
    user = json_lib.GetValueOfType(
        sub_app, KEY_SUB_APP_USER, unicode, 'app dict user')
    group = json_lib.GetValueOfType(
        sub_app, KEY_SUB_APP_GROUP, unicode, 'app dict group')

    if not self._user_db.UserExists(user):
      raise ValueError('Found invalid username "%s"' % user)
    if not self._user_db.GroupExists(group):
      raise ValueError('Found invalid groupname "%s"' % group)

    cmd = json_lib.GetValueOfType(
        sub_app, KEY_SUB_APP_EXEC, list, 'app command line')
    if not cmd:
      raise ValueError('App command line must give the executable to run.')
    self._CheckAbsPathToExecutable(cmd[0])
    for cmd_piece in cmd:
      json_lib.AssertIsInstance(cmd_piece, unicode, 'app.exec fragment')

    port_list = sub_app.get(KEY_SUB_APP_PORTS, None)
    wrapper.AddExecutable(self._user_db.ResolveUsername(user),
                          self._user_db.ResolveGroupname(group),
                          cmd,
                          _GetPortList(PROTOCOL_TCP, port_list),
                          _GetPortList(PROTOCOL_UDP, port_list),
                          _ExtractLinuxCapNames(sub_app))

  def GetSandboxSpec(self, appc_contents, sandbox_spec_name):
    """Create a SandboxSpec encoding the information in an appc pod manifest.

    Args:
      appc_contents: string contents of an appc pod manifest
      sandbox_spec_name: string unique name of this sandbox.

    Returns:
      an instance of SandboxSpec.
    """
    wrapper = SandboxSpecWrapper()
    overlay_name = None

    app_list = json_lib.GetValueOfType(
        appc_contents, KEY_APPS_LIST, list, 'app list')
    for app in app_list:
      json_lib.AssertIsInstance(app, dict, 'app')

      # Aid debugging of problems in specific apps.
      app_name = json_lib.GetValueOfType(
          app, KEY_APP_NAME, unicode, 'app name')
      if not IsValidAcName(app_name):
        raise ValueError('Application name "%s" contains illegal characters.' %
                         app_name)
      logging.debug('Processing application "%s".', app_name)

      # Get the name of the image, check that it's consistent other image names.
      image = json_lib.GetValueOfType(
          app, KEY_APP_IMAGE, dict, 'image specification for app')
      image_name = json_lib.GetValueOfType(
          image, KEY_APP_IMAGE_NAME, unicode, 'image name')
      if not IsValidAcName(image_name):
        raise ValueError('Image name "%s" contains illegal characters.' %
                         image_name)

      if overlay_name and overlay_name != image_name:
        raise ValueError(
            'All elements of "apps" must have the same image.name.')
      overlay_name = image_name

      # Add the executable corresponding to this app to our SandboxSpec.
      self._FillInExecutableFromApp(wrapper, app)

    if not overlay_name:
      raise ValueError('Overlays must declare at least one app')

    annotation_list = json_lib.GetValueOfType(
        appc_contents, KEY_ANNOTATIONS_LIST, list, 'list of all annotations')
    self._FillInEndpointNamesFromAnnotations(wrapper, annotation_list)

    wrapper.SetName(sandbox_spec_name)
    return wrapper.sandbox_spec

  def WriteSandboxSpec(self, appc_pod_manifest_path, output_path):
    """Write a SandboxSpec corresponding to |appc_pod_manifest_path| to disk.

    Args:
      appc_pod_manifest_path: path to an appc pod manifest file.
      output_path: path to file to write serialized SandboxSpec. The
          containing directory must exist, but the file may not.  This is not
          checked atomically.
    """
    if os.path.isfile(output_path):
      raise ValueError(
          'Refusing to write SandboxSpec to file %s which already exists!' %
          output_path)

    appc_contents = json.loads(osutils.ReadFile(appc_pod_manifest_path))
    # Use the file name without extension as the the name of the sandbox spec.
    sandbox_name = os.path.basename(appc_pod_manifest_path).rsplit('.', 1)[0]
    spec = self.GetSandboxSpec(appc_contents, sandbox_name)
    osutils.WriteFile(output_path, spec.SerializeToString())
