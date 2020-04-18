# -*- coding: utf-8 -*-
# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Helper module for the IAM command."""
from __future__ import absolute_import

from collections import defaultdict
from collections import namedtuple
from apitools.base.protorpclite import protojson
from gslib.cloud_api import ArgumentException
from gslib.exception import CommandException
from gslib.third_party.storage_apitools import storage_v1_messages as apitools_messages

TYPES = set([
    'user',
    'serviceAccount',
    'group',
    'domain',
])

DISCOURAGED_TYPES = set([
    'projectOwner',
    'projectEditor',
    'projectViewer'
])

DISCOURAGED_TYPES_MSG = (
    'Assigning roles (e.g. objectCreator, legacyBucketOwner) for project '
    'convenience groups is discouraged, as it goes against the principle of '
    'least privilege. Consider creating and using more granular groups with '
    'which to assign permissions. See '
    'https://cloud.google.com/iam/docs/using-iam-securely for more '
    'information. Assigning a role to a project group can be achieved by '
    'setting the IAM policy directly (see gsutil help iam for specifics).')

PUBLIC_MEMBERS = set([
    'allUsers',
    'allAuthenticatedUsers'
])

# This is a convenience class to handle returned results from
# BindingStringToTuple. is_grant is a boolean specifying if the
# bindings are to be granted or removed from a bucket / object,
# and bindings is a list of BindingsValueListEntry instances.
BindingsTuple = namedtuple('BindingsTuple', ['is_grant', 'bindings'])

# This is a special role value assigned to a specific member when all roles
# assigned to the member should be dropped in the policy. A member:DROP_ALL
# binding will be passed from BindingStringToTuple into PatchBindings.
# This role will only ever appear on client-side (i.e. user-generated). It
# will never be returned as a real role from an IAM get request. All roles
# returned by PatchBindings are guaranteed to be "real" roles, i.e. not a
# DROP_ALL role.
DROP_ALL = ''


def SerializeBindingsTuple(bindings_tuple):
  """Serializes the BindingsValueListEntry instances in a BindingsTuple.

  This is necessary when passing instances of BindingsTuple through
  Command.Apply, as apitools_messages classes are not by default pickleable.

  Args:
    bindings_tuple: A BindingsTuple instance to be serialized.

  Returns:
    A serialized BindingsTuple object.
  """
  return (bindings_tuple.is_grant,
          [protojson.encode_message(t) for t in bindings_tuple.bindings])


def DeserializeBindingsTuple(serialized_bindings_tuple):
  (is_grant, bindings) = serialized_bindings_tuple
  return BindingsTuple(
      is_grant=is_grant,
      bindings=[
          protojson.decode_message(
              apitools_messages.Policy.BindingsValueListEntry, t)
          for t in bindings])


def BindingsToDict(bindings):
  """Converts a list of BindingsValueListEntry to a dictionary.

  Args:
    bindings: A list of BindingsValueListEntry instances.

  Returns:
    A {role: set(members)} dictionary.
  """

  tmp_bindings = defaultdict(set)
  for binding in bindings:
    tmp_bindings[binding.role].update(binding.members)
  return tmp_bindings


def IsEqualBindings(a, b):
  (granted, removed) = DiffBindings(a, b)
  return not granted.bindings and not removed.bindings


def DiffBindings(old, new):
  """Computes the difference between two BindingsValueListEntry lists.

  Args:
    old: The original list of BindingValuesListEntry instances
    new: The updated list of BindingValuesListEntry instances

  Returns:
    A pair of BindingsTuple instances, one for roles granted between old and
      new, and one for roles removed between old and new.
  """
  tmp_old = BindingsToDict(old)
  tmp_new = BindingsToDict(new)

  granted = BindingsToDict([])
  removed = BindingsToDict([])

  for (role, members) in tmp_old.iteritems():
    removed[role].update(members.difference(tmp_new[role]))
  for (role, members) in tmp_new.iteritems():
    granted[role].update(members.difference(tmp_old[role]))

  granted = [
      apitools_messages.Policy.BindingsValueListEntry(
          role=r, members=list(m)) for (r, m) in granted.iteritems() if m]
  removed = [
      apitools_messages.Policy.BindingsValueListEntry(
          role=r, members=list(m)) for (r, m) in removed.iteritems() if m]

  return (BindingsTuple(True, granted), BindingsTuple(False, removed))


def PatchBindings(base, diff):
  """Patches a diff list of BindingsValueListEntry to the base.

  Will remove duplicate members for any given role on a grant operation.

  Args:
    base: A list of BindingsValueListEntry instances.
    diff: A BindingsTuple instance of diff to be applied.

  Returns:
    The computed difference, as a list of
    apitools_messages.Policy.BindingsValueListEntry instances.
  """

  # Convert the list of bindings into an {r: [m]} dictionary object.
  tmp_base = BindingsToDict(base)
  tmp_diff = BindingsToDict(diff.bindings)

  # Patch the diff into base
  if diff.is_grant:
    for (role, members) in tmp_diff.iteritems():
      if not role:
        raise CommandException('Role must be specified for a grant request.')
      tmp_base[role].update(members)
  else:
    for role in tmp_base:
      tmp_base[role].difference_update(tmp_diff[role])
      # Drop all members with the DROP_ALL role specifed from input.
      tmp_base[role].difference_update(tmp_diff[DROP_ALL])

  # Construct the BindingsValueListEntry list
  bindings = [
      apitools_messages.Policy.BindingsValueListEntry(
          role=r, members=list(m)) for (r, m) in tmp_base.iteritems() if m]

  return bindings


def BindingStringToTuple(is_grant, input_str):
  """Parses an iam ch bind string to a list of binding tuples.

  Args:
    is_grant: If true, binding is to be appended to IAM policy; else, delete
              this binding from the policy.
    input_str: A string representing a member-role binding.
               e.g. user:foo@bar.com:objectAdmin
                    user:foo@bar.com:objectAdmin,objectViewer
                    user:foo@bar.com

  Raises:
    ArgumentException in the case of invalid input.

  Returns:
    A BindingsTuple instance.
  """

  if not input_str.count(':'):
    input_str += ':'
  if input_str.count(':') == 1:
    tokens = input_str.split(':')
    if tokens[0] in PUBLIC_MEMBERS:
      (member, roles) = tokens
    elif tokens[0] in TYPES:
      member = ':'.join(tokens)
      roles = DROP_ALL
    else:
      raise CommandException(
          'Incorrect public member type for binding %s' % input_str)
  elif input_str.count(':') == 2:
    (member_type, member_id, roles) = input_str.split(':')
    if member_type in DISCOURAGED_TYPES:
      raise CommandException(DISCOURAGED_TYPES_MSG)
    elif member_type not in TYPES:
      raise CommandException('Incorrect member type for binding %s' % input_str)
    member = '%s:%s' % (member_type, member_id)
  else:
    raise CommandException('Invalid ch format %s' % input_str)

  if is_grant and not roles:
    raise CommandException('Must specify a role to grant.')

  roles = ['roles/storage.%s' % r if r else DROP_ALL for r in roles.split(',')]
  bindings = [
      apitools_messages.Policy.BindingsValueListEntry(
          members=[member], role=r) for r in set(roles)]
  return BindingsTuple(is_grant=is_grant, bindings=bindings)
