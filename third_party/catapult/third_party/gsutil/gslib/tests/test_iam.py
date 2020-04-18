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
"""Integration tests for the iam command."""
from __future__ import absolute_import

from collections import defaultdict
import json
from gslib.exception import CommandException
from gslib.iamhelpers import BindingsToDict
from gslib.iamhelpers import BindingStringToTuple as bstt
from gslib.iamhelpers import BindingsTuple
from gslib.iamhelpers import DiffBindings
from gslib.iamhelpers import IsEqualBindings
from gslib.iamhelpers import PatchBindings
import gslib.tests.testcase as testcase
from gslib.tests.testcase.integration_testcase import SkipForS3
from gslib.tests.testcase.integration_testcase import SkipForXML
from gslib.tests.util import GenerationFromURI as urigen
from gslib.tests.util import SetBotoConfigForTest
from gslib.third_party.storage_apitools import storage_v1_messages as apitools_messages
from gslib.util import Retry

bvle = apitools_messages.Policy.BindingsValueListEntry

# Feature iam_bucket_roles must be turned on in bigstore dev config for setting
# the new IAM policies on buckets.
IAM_BUCKET_READ_ROLE_ABBREV = 'legacyBucketReader'
IAM_BUCKET_READ_ROLE = 'roles/storage.%s' % IAM_BUCKET_READ_ROLE_ABBREV
# GCS IAM does not currently support new object-level roles.
IAM_OBJECT_READ_ROLE = 'roles/storage.legacyObjectReader'


def gen_binding(role, members=None):
  """Generate an IAM Policy object dictionary.

  Generates Python dictionary representation of a storage_v1_messages.Policy
  object with a single storage_v1_messages.Policy.BindingsValueListEntry.

  Args:
    role: An IAM policy role (e.g. "roles/storage.objectViewer"). Fully
          specified in BindingsValueListEntry.
    members: A list of members (e.g. ["user:foo@bar.com"]). If None,
             bind to ["allUsers"]. Fully specified in BindingsValueListEntry.

  Returns:
    A Python dict representation of an IAM Policy object.
  """
  if members is None:
    members = ['allUsers']
  return [
      {
          'members': members,
          'role': role,
      }
  ]


class TestIamIntegration(testcase.GsUtilIntegrationTestCase):
  """Superclass for iam integration test cases."""

  def assertEqualsPoliciesString(self, a, b):
    """Asserts two serialized policy bindings are equal."""
    expected = [
        bvle(
            members=binding_dict['members'],
            role=binding_dict['role'])
        for binding_dict in json.loads(a)['bindings']]
    result = [
        bvle(
            members=binding_dict['members'],
            role=binding_dict['role'])
        for binding_dict in json.loads(b)['bindings']]
    self.assertTrue(IsEqualBindings(expected, result))


@SkipForS3('Tests use GS IAM model.')
@SkipForXML('XML IAM control is not supported.')
class TestIamHelpers(testcase.GsUtilUnitTestCase):
  """Unit tests for iam command helper."""

  def test_convert_bindings_simple(self):
    """Tests that Policy.bindings lists are converted to dicts properly."""
    self.assertEquals(BindingsToDict([]), defaultdict(set))
    expected = defaultdict(set, {'x': set(['y'])})
    self.assertEquals(
        BindingsToDict([bvle(role='x', members=['y'])]), expected)

  def test_convert_bindings_duplicates(self):
    """Test that role and member duplication are converted correctly."""
    expected = defaultdict(set, {'x': set(['y', 'z'])})
    duplicate_roles = [
        bvle(role='x', members=['y']),
        bvle(role='x', members=['z'])]
    duplicate_members = [
        bvle(role='x', members=['z', 'y']),
        bvle(role='x', members=['z'])]
    self.assertEquals(BindingsToDict(duplicate_roles), expected)
    self.assertEquals(BindingsToDict(duplicate_members), expected)

  def test_equality_bindings_literal(self):
    """Tests an easy case of identical bindings."""
    bindings = [bvle(role='x', members=['y'])]
    self.assertTrue(IsEqualBindings([], []))
    self.assertTrue(IsEqualBindings(bindings, bindings))

  def test_equality_bindings_extra_roles(self):
    """Tests bindings equality when duplicate roles are added."""
    bindings = [bvle(role='x', members=['x', 'y'])]
    bindings2 = bindings * 2
    bindings3 = [
        bvle(role='x', members=['y']),
        bvle(role='x', members=['x']),
    ]
    self.assertTrue(IsEqualBindings(bindings, bindings2))
    self.assertTrue(IsEqualBindings(bindings, bindings3))

  def test_diff_bindings_add_role(self):
    """Tests simple grant behavior of Policy.bindings diff."""
    expected = [bvle(role='x', members=['y'])]
    (granted, removed) = DiffBindings([], expected)
    self.assertEquals(granted.bindings, expected)
    self.assertEquals(removed.bindings, [])

  def test_diff_bindings_drop_role(self):
    """Tests simple remove behavior of Policy.bindings diff."""
    expected = [bvle(role='x', members=['y'])]
    (granted, removed) = DiffBindings(expected, [])
    self.assertEquals(granted.bindings, [])
    self.assertEquals(removed.bindings, expected)

  def test_diff_bindings_swap_role(self):
    """Tests expected behavior of switching a role."""
    old = [bvle(role='x', members=['y'])]
    new = [bvle(role='a', members=['b'])]
    (granted, removed) = DiffBindings(old, new)
    self.assertEquals(granted.bindings, new)
    self.assertEquals(removed.bindings, old)

  def test_diff_bindings_add_member(self):
    """Tests expected behavior of adding a member to a role."""
    old = [bvle(role='x', members=['y'])]
    new = [bvle(role='x', members=['z', 'y'])]
    expected = [bvle(role='x', members=['z'])]
    (granted, removed) = DiffBindings(old, new)
    self.assertEquals(granted.bindings, expected)
    self.assertEquals(removed.bindings, [])

  def test_diff_bindings_drop_member(self):
    """Tests expected behavior of dropping a member from a role."""
    old = [bvle(role='x', members=['z', 'y'])]
    new = [bvle(role='x', members=['y'])]
    expected = [bvle(role='x', members=['z'])]
    (granted, removed) = DiffBindings(old, new)
    self.assertEquals(granted.bindings, [])
    self.assertEquals(removed.bindings, expected)

  def test_diff_bindings_swap_member(self):
    """Tests expected behavior of switching a member in a role."""
    old = [bvle(role='x', members=['z'])]
    new = [bvle(role='x', members=['y'])]
    (granted, removed) = DiffBindings(old, new)
    self.assertEquals(granted.bindings, new)
    self.assertEquals(removed.bindings, old)

  def test_patch_bindings_grant(self):
    """Tests patching a grant binding."""
    base = [
        bvle(role='a', members=['user:foo@bar.com']),
        bvle(role='b', members=['user:foo@bar.com']),
        bvle(role='c', members=['user:foo@bar.com']),
    ]
    diff = [
        bvle(role='d', members=['user:foo@bar.com']),
    ]
    expected = base + diff
    res = PatchBindings(base, BindingsTuple(True, diff))
    self.assertTrue(IsEqualBindings(res, expected))

  def test_patch_bindings_remove(self):
    """Tests patching a remove binding."""
    base = [
        bvle(members=['user:foo@bar.com'], role='a'),
        bvle(members=['user:foo@bar.com'], role='b'),
        bvle(members=['user:foo@bar.com'], role='c'),
    ]
    diff = [
        bvle(members=['user:foo@bar.com'], role='a'),
    ]
    expected = [
        bvle(members=['user:foo@bar.com'], role='b'),
        bvle(members=['user:foo@bar.com'], role='c'),
    ]

    res = PatchBindings(base, BindingsTuple(False, diff))
    self.assertTrue(IsEqualBindings(res, expected))

  def test_patch_bindings_remove_all(self):
    """Tests removing all roles from a member."""
    base = [
        bvle(members=['user:foo@bar.com'], role='a'),
        bvle(members=['user:foo@bar.com'], role='b'),
        bvle(members=['user:foo@bar.com'], role='c'),
    ]
    diff = [
        bvle(members=['user:foo@bar.com'], role=''),
    ]
    res = PatchBindings(base, BindingsTuple(False, diff))
    self.assertEquals(res, [])

    diff = [
        bvle(members=['user:foo@bar.com'], role='a'),
        bvle(members=['user:foo@bar.com'], role='b'),
        bvle(members=['user:foo@bar.com'], role='c'),
    ]

    res = PatchBindings(base, BindingsTuple(False, diff))
    self.assertEquals(res, [])

  def test_patch_bindings_multiple_users(self):
    """Tests expected behavior when multiple users exist."""
    expected = [
        bvle(members=['user:fii@bar.com'], role='b'),
    ]
    base = [
        bvle(members=['user:foo@bar.com'], role='a'),
        bvle(members=['user:foo@bar.com', 'user:fii@bar.com'], role='b'),
        bvle(members=['user:foo@bar.com'], role='c'),
    ]
    diff = [
        bvle(members=['user:foo@bar.com'], role='a'),
        bvle(members=['user:foo@bar.com'], role='b'),
        bvle(members=['user:foo@bar.com'], role='c'),
    ]
    res = PatchBindings(base, BindingsTuple(False, diff))
    self.assertTrue(IsEqualBindings(res, expected))

  def test_patch_bindings_grant_all_users(self):
    """Tests a public member grant."""
    base = [
        bvle(role='a', members=['user:foo@bar.com']),
        bvle(role='b', members=['user:foo@bar.com']),
        bvle(role='c', members=['user:foo@bar.com']),
    ]
    diff = [
        bvle(role='a', members=['allUsers']),
    ]
    expected = [
        bvle(role='a', members=['allUsers', 'user:foo@bar.com']),
        bvle(role='b', members=['user:foo@bar.com']),
        bvle(role='c', members=['user:foo@bar.com']),
    ]

    res = PatchBindings(base, BindingsTuple(True, diff))
    self.assertTrue(IsEqualBindings(res, expected))

  def test_patch_bindings_public_member_overwrite(self):
    """Tests public member vs. public member interaction."""
    base = [
        bvle(role='a', members=['allUsers']),
    ]
    diff = [
        bvle(role='a', members=['allAuthenticatedUsers']),
    ]

    res = PatchBindings(base, BindingsTuple(True, diff))
    self.assertTrue(IsEqualBindings(res, base + diff))

  def test_valid_public_member_single_role(self):
    """Tests parsing single role."""
    (_, bindings) = bstt(True, 'allUsers:admin')
    self.assertEquals(len(bindings), 1)
    self.assertIn(
        bvle(members=['allUsers'], role='roles/storage.admin'),
        bindings)

  def test_grant_no_role_error(self):
    """Tests that an error is raised when no role is specified for a grant."""
    with self.assertRaises(CommandException):
      bstt(True, 'allUsers')
    with self.assertRaises(CommandException):
      bstt(True, 'user:foo@bar.com')
    with self.assertRaises(CommandException):
      bstt(True, 'user:foo@bar.com:')

  def test_remove_all_roles(self):
    """Tests parsing a -d allUsers or -d user:foo@bar.com request."""
    # Input specifies remove all roles from allUsers.
    (is_grant, bindings) = bstt(False, 'allUsers')
    self.assertEquals(len(bindings), 1)
    self.assertIn(bvle(members=['allUsers'], role=''), bindings)
    self.assertEquals((is_grant, bindings), bstt(False, 'allUsers:'))

    # Input specifies remove all roles from a user.
    (_, bindings) = bstt(False, 'user:foo@bar.com')
    self.assertEquals(len(bindings), 1)

  def test_valid_multiple_roles(self):
    """Tests parsing of multiple roles bound to one user."""
    (_, bindings) = bstt(True, 'allUsers:a,b,c')
    self.assertEquals(len(bindings), 3)
    self.assertIn(bvle(members=['allUsers'], role='roles/storage.a'), bindings)
    self.assertIn(bvle(members=['allUsers'], role='roles/storage.b'), bindings)
    self.assertIn(bvle(members=['allUsers'], role='roles/storage.c'), bindings)

  def test_valid_member(self):
    """Tests member parsing."""
    (_, bindings) = bstt(True, 'user:foo@bar.com:admin')
    self.assertEquals(len(bindings), 1)
    self.assertIn(
        bvle(
            members=['user:foo@bar.com'],
            role='roles/storage.admin'),
        bindings)

  def test_duplicate_roles(self):
    """Tests that duplicate roles are ignored."""
    (_, bindings) = bstt(True, 'allUsers:a,a')
    self.assertEquals(len(bindings), 1)
    self.assertIn(bvle(members=['allUsers'], role='roles/storage.a'), bindings)

  def test_invalid_input(self):
    """Tests invalid input handling."""
    with self.assertRaises(CommandException):
      bstt(True, 'non_valid_public_member:role')
    with self.assertRaises(CommandException):
      bstt(True, 'non_valid_type:id:role')
    with self.assertRaises(CommandException):
      bstt(True, 'user:r')
    with self.assertRaises(CommandException):
      bstt(True, 'projectViewer:123424:admin')

  def test_invalid_n_args(self):
    """Tests invalid input due to too many colons."""
    with self.assertRaises(CommandException):
      bstt(True, 'allUsers:some_id:some_role')
    with self.assertRaises(CommandException):
      bstt(True, 'user:foo@bar.com:r:nonsense')


@SkipForS3('Tests use GS IAM model.')
@SkipForXML('XML IAM control is not supported.')
class TestIamCh(TestIamIntegration):
  """Integration tests for iam ch command."""

  def setUp(self):
    super(TestIamCh, self).setUp()
    self.bucket = self.CreateBucket()
    self.bucket2 = self.CreateBucket()
    self.object = self.CreateObject(bucket_uri=self.bucket, contents='foo')
    self.object2 = self.CreateObject(bucket_uri=self.bucket, contents='bar')

    self.bucket_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.object_iam_string = self.RunGsUtil(
        ['iam', 'get', self.object.uri], return_stdout=True)
    self.object2_iam_string = self.RunGsUtil(
        ['iam', 'get', self.object2.uri], return_stdout=True)

    self.user = 'user:foo@bar.com'
    self.user2 = 'user:bar@foo.com'

  def test_patch_no_role(self):
    """Tests expected failure if no bindings are listed."""
    stderr = self.RunGsUtil(
        ['iam', 'ch', self.bucket.uri], return_stderr=True, expected_status=1)
    self.assertIn('CommandException', stderr)

  def test_patch_single_grant_single_bucket(self):
    """Tests granting single role."""
    self.assertHasNo(
        self.bucket_iam_string, self.user, IAM_BUCKET_READ_ROLE)
    self.RunGsUtil(
        ['iam', 'ch', '%s:%s' % (self.user, IAM_BUCKET_READ_ROLE_ABBREV),
         self.bucket.uri])

    bucket_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.assertHas(bucket_iam_string, self.user, IAM_BUCKET_READ_ROLE)

  def test_patch_repeated_grant(self):
    """Granting multiple times for the same member will have no effect."""
    self.RunGsUtil(
        ['iam', 'ch', '%s:%s' % (self.user, IAM_BUCKET_READ_ROLE_ABBREV),
         self.bucket.uri])
    self.RunGsUtil(
        ['iam', 'ch', '%s:%s' % (self.user, IAM_BUCKET_READ_ROLE_ABBREV),
         self.bucket.uri])

    bucket_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.assertHas(bucket_iam_string, self.user, IAM_BUCKET_READ_ROLE)

  def test_patch_single_remove_single_bucket(self):
    """Tests removing a single role."""
    self.RunGsUtil(
        ['iam', 'ch', '%s:%s' % (self.user, IAM_BUCKET_READ_ROLE_ABBREV),
         self.bucket.uri])
    self.RunGsUtil(
        ['iam', 'ch', '-d', '%s:%s' % (self.user, IAM_BUCKET_READ_ROLE_ABBREV),
         self.bucket.uri])

    bucket_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.assertHasNo(
        bucket_iam_string, self.user, IAM_BUCKET_READ_ROLE)

  def test_patch_null_remove(self):
    """Removing a non-existent binding will have no effect."""
    self.RunGsUtil(
        ['iam', 'ch', '-d', '%s:%s' % (self.user, IAM_BUCKET_READ_ROLE_ABBREV),
         self.bucket.uri])

    bucket_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.assertHasNo(
        bucket_iam_string, self.user, IAM_BUCKET_READ_ROLE)
    self.assertEqualsPoliciesString(bucket_iam_string, self.bucket_iam_string)

  def test_patch_mixed_grant_remove_single_bucket(self):
    """Tests that mixing grant and remove requests will succeed."""
    self.RunGsUtil(
        ['iam', 'ch', '%s:%s' % (self.user2, IAM_BUCKET_READ_ROLE_ABBREV),
         self.bucket.uri])
    self.RunGsUtil(
        ['iam', 'ch', '%s:%s' % (self.user, IAM_BUCKET_READ_ROLE_ABBREV), '-d',
         '%s:%s' % (self.user2, IAM_BUCKET_READ_ROLE_ABBREV), self.bucket.uri])

    bucket_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.assertHas(bucket_iam_string, self.user, IAM_BUCKET_READ_ROLE)
    self.assertHasNo(
        bucket_iam_string, self.user2, IAM_BUCKET_READ_ROLE)

  def test_patch_public_grant_single_bucket(self):
    """Test public grant request interacts properly with existing members."""
    self.RunGsUtil(
        ['iam', 'ch', '%s:%s' % (self.user, IAM_BUCKET_READ_ROLE_ABBREV),
         self.bucket.uri])
    self.RunGsUtil(['iam', 'ch', 'allUsers:%s' % IAM_BUCKET_READ_ROLE_ABBREV,
                    self.bucket.uri])

    bucket_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.assertHas(bucket_iam_string, 'allUsers', IAM_BUCKET_READ_ROLE)
    self.assertHas(
        bucket_iam_string, self.user, IAM_BUCKET_READ_ROLE)

  def test_patch_remove_all_roles(self):
    """Remove with no roles specified will remove member from all bindings."""
    self.RunGsUtil(
        ['iam', 'ch', '%s:%s' % (self.user, IAM_BUCKET_READ_ROLE_ABBREV),
         self.bucket.uri])
    self.RunGsUtil(['iam', 'ch', '-d', self.user, self.bucket.uri])

    bucket_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.assertHasNo(
        bucket_iam_string, self.user, IAM_BUCKET_READ_ROLE)

  def test_patch_single_object(self):
    """Tests object IAM patch behavior."""
    self.assertHasNo(
        self.object_iam_string, self.user, IAM_OBJECT_READ_ROLE)
    self.RunGsUtil(
        ['iam', 'ch', '%s:legacyObjectReader' % self.user, self.object.uri])

    object_iam_string = self.RunGsUtil(
        ['iam', 'get', self.object.uri], return_stdout=True)
    self.assertHas(
        object_iam_string, self.user, IAM_OBJECT_READ_ROLE)

  def test_patch_multithreaded_single_object(self):
    """Tests the edge-case behavior of multithreaded execution."""
    self.assertHasNo(
        self.object_iam_string, self.user, IAM_OBJECT_READ_ROLE)
    self.RunGsUtil(
        ['-m', 'iam', 'ch', '%s:legacyObjectReader' % self.user,
         self.object.uri])

    object_iam_string = self.RunGsUtil(
        ['iam', 'get', self.object.uri], return_stdout=True)
    self.assertHas(
        object_iam_string, self.user, IAM_OBJECT_READ_ROLE)

  def test_patch_invalid_input(self):
    """Tests that listing bindings after a bucket will throw an error."""
    stderr = self.RunGsUtil(
        ['iam', 'ch', '%s:%s' % (self.user, IAM_BUCKET_READ_ROLE_ABBREV),
         self.bucket.uri, '%s:%s' % (self.user2, IAM_BUCKET_READ_ROLE_ABBREV)],
        return_stderr=True, expected_status=1)
    self.assertIn('CommandException', stderr)

    bucket_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.assertHas(bucket_iam_string, self.user, IAM_BUCKET_READ_ROLE)
    self.assertHasNo(
        bucket_iam_string, self.user2, IAM_BUCKET_READ_ROLE)

  def test_patch_multiple_objects(self):
    """Tests IAM patch against multiple objects."""
    self.RunGsUtil(
        ['iam', 'ch', '-r', '%s:legacyObjectReader' % self.user,
         self.bucket.uri])

    object_iam_string = self.RunGsUtil(
        ['iam', 'get', self.object.uri], return_stdout=True)
    object2_iam_string = self.RunGsUtil(
        ['iam', 'get', self.object2.uri], return_stdout=True)
    self.assertHas(
        object_iam_string, self.user, IAM_OBJECT_READ_ROLE)
    self.assertHas(
        object2_iam_string, self.user, IAM_OBJECT_READ_ROLE)

  def test_patch_multithreaded_multiple_objects(self):
    """Tests multithreaded behavior against multiple objects."""
    self.RunGsUtil(
        ['-m', 'iam', 'ch', '-r', '%s:legacyObjectReader' % self.user,
         self.bucket.uri])

    object_iam_string = self.RunGsUtil(
        ['iam', 'get', self.object.uri], return_stdout=True)
    object2_iam_string = self.RunGsUtil(
        ['iam', 'get', self.object2.uri], return_stdout=True)
    self.assertHas(
        object_iam_string, self.user, IAM_OBJECT_READ_ROLE)
    self.assertHas(
        object2_iam_string, self.user, IAM_OBJECT_READ_ROLE)

  def test_patch_error(self):
    """See TestIamSet.test_set_error."""
    stderr = self.RunGsUtil(
        ['iam', 'ch', '%s:%s' % (self.user, IAM_BUCKET_READ_ROLE_ABBREV),
         self.bucket.uri, 'gs://%s' % self.nonexistent_bucket_name,
         self.bucket2.uri],
        return_stderr=True, expected_status=1)
    self.assertIn('BucketNotFoundException', stderr)

    bucket_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    bucket2_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket2.uri], return_stdout=True)

    self.assertHas(bucket_iam_string, self.user, IAM_BUCKET_READ_ROLE)
    self.assertEqualsPoliciesString(bucket2_iam_string, self.bucket_iam_string)

  def test_patch_force_error(self):
    """See TestIamSet.test_set_force_error."""
    stderr = self.RunGsUtil(
        ['iam', 'ch', '-f', '%s:%s' % (self.user, IAM_BUCKET_READ_ROLE_ABBREV),
         self.bucket.uri, 'gs://%s' % self.nonexistent_bucket_name,
         self.bucket2.uri],
        return_stderr=True, expected_status=1)
    self.assertIn('CommandException', stderr)

    bucket_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    bucket2_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket2.uri], return_stdout=True)

    self.assertHas(bucket_iam_string, self.user, IAM_BUCKET_READ_ROLE)
    self.assertHas(bucket2_iam_string, self.user, IAM_BUCKET_READ_ROLE)

  def test_patch_multithreaded_error(self):
    """See TestIamSet.test_set_multithreaded_error."""
    stderr = self.RunGsUtil(
        ['-m', 'iam', 'ch', '-r', '%s:legacyObjectReader' % self.user,
         'gs://%s' % self.nonexistent_bucket_name, self.bucket.uri],
        return_stderr=True, expected_status=1)
    self.assertIn('BucketNotFoundException', stderr)

    object_iam_string = self.RunGsUtil(
        ['iam', 'get', self.object.uri], return_stdout=True)
    object2_iam_string = self.RunGsUtil(
        ['iam', 'get', self.object2.uri], return_stdout=True)

    self.assertEqualsPoliciesString(self.object_iam_string, object_iam_string)
    self.assertEqualsPoliciesString(self.object_iam_string, object2_iam_string)

  def test_assert_has(self):
    test_policy = {
        'bindings': [
            {'members': ['allUsers'], 'role': 'roles/storage.admin'},
            {'members': ['user:foo@bar.com', 'serviceAccount:bar@foo.com'],
             'role': IAM_BUCKET_READ_ROLE}
        ]
    }

    self.assertHas(json.dumps(test_policy), 'allUsers', 'roles/storage.admin')
    self.assertHas(
        json.dumps(test_policy), 'user:foo@bar.com',
        IAM_BUCKET_READ_ROLE)
    self.assertHasNo(
        json.dumps(test_policy), 'allUsers', IAM_BUCKET_READ_ROLE)
    self.assertHasNo(
        json.dumps(test_policy), 'user:foo@bar.com', 'roles/storage.admin')

  def assertHas(self, policy, member, role):
    """Asserts a member has permission for role.

    Given an IAM policy, check if the specified member is bound to the
    specified role. Does not check group inheritence -- that is, if checking
    against the [{'member': ['allUsers'], 'role': X}] policy, this function
    will still raise an exception when testing for any member other than
    'allUsers' against role X.

    This function does not invoke the TestIamPolicy endpoints to smartly check
    IAM policy resolution. This function is simply to assert the expected IAM
    policy is returned, not whether or not the IAM policy is being invoked as
    expected.

    Args:
      policy: Policy object as formatted by IamCommand._GetIam()
      member: A member string (e.g. 'user:foo@bar.com').
      role: A fully specified role (e.g. 'roles/storage.admin')

    Raises:
      AssertionError if member is not bound to role.
    """

    policy = json.loads(policy)
    bindings = dict((p['role'], p) for p in policy.get('bindings', []))
    if role in bindings:
      if member in bindings[role]['members']:
        return
    raise AssertionError('Member \'%s\' does not have permission \'%s\' in '
                         'policy %s' % (member, role, policy))

  def assertHasNo(self, policy, member, role):
    """Functions as logical compliment of TestIamCh.assertHas()."""
    try:
      self.assertHas(policy, member, role)
    except AssertionError:
      pass
    else:
      raise AssertionError('Member \'%s\' has permission \'%s\' in '
                           'policy %s' % (member, role, policy))


@SkipForS3('Tests use GS IAM model.')
@SkipForXML('XML IAM control is not supported.')
class TestIamSet(TestIamIntegration):
  """Integration tests for iam set command."""

  def _patch_binding(self, policy, role, new_policy):
    """Returns a patched Python object representation of a Policy.

    Given replaces the original role:members binding in policy with new_policy.

    Args:
      policy: Python dict representation of a Policy instance.
      role: An IAM policy role (e.g. "roles/storage.objectViewer"). Fully
            specified in BindingsValueListEntry.
      new_policy: A Python dict representation of a Policy instance, with a
                  single BindingsValueListEntry entry.

    Returns:
      A Python dict representation of the patched IAM Policy object.
    """
    bindings = [
        b for b in policy.get('bindings', []) if b.get('role', '') != role]
    bindings.extend(new_policy)
    policy = dict(policy)
    policy['bindings'] = bindings
    return policy

  # TODO(iam-beta): Replace gen_binding, _patch_binding with generators from
  # iamhelpers.
  def setUp(self):
    super(TestIamSet, self).setUp()

    self.public_bucket_read_binding = gen_binding(IAM_BUCKET_READ_ROLE)
    self.public_object_read_binding = gen_binding(IAM_OBJECT_READ_ROLE)

    self.bucket = self.CreateBucket()
    self.versioned_bucket = self.CreateVersionedBucket()

    self.bucket_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.old_bucket_iam_path = self.CreateTempFile(
        contents=self.bucket_iam_string)
    self.new_bucket_iam_policy = self._patch_binding(
        json.loads(self.bucket_iam_string),
        IAM_BUCKET_READ_ROLE,
        self.public_bucket_read_binding)
    self.new_bucket_iam_path = self.CreateTempFile(
        contents=json.dumps(self.new_bucket_iam_policy))

    # Create a temporary object to get the IAM policy.
    tmp_object = self.CreateObject(contents='foobar')
    self.object_iam_string = self.RunGsUtil(
        ['iam', 'get', tmp_object.uri], return_stdout=True)
    self.old_object_iam_path = self.CreateTempFile(
        contents=self.object_iam_string)
    self.new_object_iam_policy = self._patch_binding(
        json.loads(self.object_iam_string), IAM_OBJECT_READ_ROLE,
        self.public_object_read_binding)
    self.new_object_iam_path = self.CreateTempFile(
        contents=json.dumps(self.new_object_iam_policy))

  def test_seek_ahead_iam(self):
    """Ensures that the seek-ahead iterator is being used with iam commands."""

    gsutil_object = self.CreateObject(
        bucket_uri=self.bucket, contents='foobar')

    # This forces the seek-ahead iterator to be utilized.
    with SetBotoConfigForTest([('GSUtil', 'task_estimation_threshold', '1'),
                               ('GSUtil', 'task_estimation_force', 'True')]):
      stderr = self.RunGsUtil(
          ['-m', 'iam', 'set', self.new_object_iam_path, gsutil_object.uri],
          return_stderr=True)
      self.assertIn('Estimated work for this command: objects: 1\n', stderr)

  def test_set_invalid_iam_bucket(self):
    """Ensures invalid content returns error on input check."""
    inpath = self.CreateTempFile(contents='badIam')
    stderr = self.RunGsUtil(['iam', 'set', inpath, self.bucket.uri],
                            return_stderr=True, expected_status=1)
    self.assertIn('ArgumentException', stderr)

    # Tests that setting with a non-existent file will also return error.
    stderr = self.RunGsUtil(
        ['iam', 'set', 'nonexistent/path', self.bucket.uri],
        return_stderr=True, expected_status=1)
    self.assertIn('ArgumentException', stderr)

  def test_get_invalid_bucket(self):
    """Ensures that invalid bucket names returns an error."""
    stderr = self.RunGsUtil(['iam', 'get', self.nonexistent_bucket_name],
                            return_stderr=True, expected_status=1)
    self.assertIn('CommandException', stderr)

    stderr = self.RunGsUtil(['iam', 'get',
                             'gs://%s' % self.nonexistent_bucket_name],
                            return_stderr=True, expected_status=1)
    self.assertIn('BucketNotFoundException', stderr)

    # N.B.: The call to wildcard_iterator.WildCardIterator here will invoke
    # ListBucket, which only promises eventual consistency. We use @Retry here
    # to mitigate errors due to this.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():  # pylint: disable=invalid-name
      # There are at least two buckets in the project
      # due to TestIamSet.setUp().
      stderr = self.RunGsUtil(['iam', 'get', 'gs://*'],
                              return_stderr=True, expected_status=1)
      self.assertIn('CommandException', stderr)
    _Check()

  def test_set_valid_iam_bucket(self):
    """Tests setting a valid IAM on a bucket."""
    self.RunGsUtil(
        ['iam', 'set', '-e', '', self.new_bucket_iam_path, self.bucket.uri])
    set_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.RunGsUtil(
        ['iam', 'set', '-e', '', self.old_bucket_iam_path, self.bucket.uri])
    reset_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)

    self.assertEqualsPoliciesString(self.bucket_iam_string, reset_iam_string)
    self.assertIn(
        self.public_bucket_read_binding[0],
        json.loads(set_iam_string)['bindings'])

  def test_set_blank_etag(self):
    """Tests setting blank etag behaves appropriately."""
    self.RunGsUtil(
        ['iam', 'set', '-e', '', self.new_bucket_iam_path, self.bucket.uri])

    set_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.RunGsUtil(
        ['iam', 'set', '-e', json.loads(set_iam_string)['etag'],
         self.old_bucket_iam_path, self.bucket.uri])

    reset_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)

    self.assertEqualsPoliciesString(self.bucket_iam_string, reset_iam_string)
    self.assertIn(
        self.public_bucket_read_binding[0],
        json.loads(set_iam_string)['bindings'])

  def test_set_valid_etag(self):
    """Tests setting valid etag behaves correctly."""
    get_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.RunGsUtil(
        ['iam', 'set', '-e', json.loads(get_iam_string)['etag'],
         self.new_bucket_iam_path, self.bucket.uri])

    set_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.RunGsUtil(
        ['iam', 'set', '-e', json.loads(set_iam_string)['etag'],
         self.old_bucket_iam_path, self.bucket.uri])

    reset_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)

    self.assertEqualsPoliciesString(self.bucket_iam_string, reset_iam_string)
    self.assertIn(
        self.public_bucket_read_binding[0],
        json.loads(set_iam_string)['bindings'])

  def test_set_invalid_etag(self):
    """Tests setting an invalid etag format raises an error."""
    self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    stderr = self.RunGsUtil(
        ['iam', 'set', '-e', 'some invalid etag',
         self.new_bucket_iam_path, self.bucket.uri],
        return_stderr=True, expected_status=1)
    self.assertIn('ArgumentException', stderr)

  def test_set_mismatched_etag(self):
    """Tests setting mismatched etag raises an error."""
    get_iam_string = self.RunGsUtil(
        ['iam', 'get', self.bucket.uri], return_stdout=True)
    self.RunGsUtil(
        ['iam', 'set', '-e', json.loads(get_iam_string)['etag'],
         self.new_bucket_iam_path, self.bucket.uri])
    stderr = self.RunGsUtil(
        ['iam', 'set', '-e', json.loads(get_iam_string)['etag'],
         self.new_bucket_iam_path, self.bucket.uri],
        return_stderr=True, expected_status=1)
    self.assertIn('PreconditionException', stderr)

  def _create_multiple_objects(self):
    """Creates two versioned objects and return references to all versions.

    Returns:
      A four-tuple (a, b, a*, b*) of storage_uri.BucketStorageUri instances.
    """

    old_gsutil_object = self.CreateObject(
        bucket_uri=self.versioned_bucket, contents='foo')
    old_gsutil_object2 = self.CreateObject(
        bucket_uri=self.versioned_bucket, contents='bar')
    gsutil_object = self.CreateObject(
        bucket_uri=self.versioned_bucket,
        object_name=old_gsutil_object.object_name,
        contents='new_foo', gs_idempotent_generation=urigen(old_gsutil_object))
    gsutil_object2 = self.CreateObject(
        bucket_uri=self.versioned_bucket,
        object_name=old_gsutil_object2.object_name,
        contents='new_bar', gs_idempotent_generation=urigen(old_gsutil_object2))
    return (old_gsutil_object, old_gsutil_object2, gsutil_object,
            gsutil_object2)

  def test_set_valid_iam_multiple_objects(self):
    """Tests setting a valid IAM on multiple objects."""
    (old_gsutil_object, old_gsutil_object2, gsutil_object,
     gsutil_object2) = self._create_multiple_objects()

    # Set IAM policy on newest versions of all objects.
    self.RunGsUtil(['iam', 'set', '-r',
                    self.new_object_iam_path, self.versioned_bucket.uri])
    set_iam_string = self.RunGsUtil(
        ['iam', 'get', gsutil_object.uri], return_stdout=True)
    set_iam_string2 = self.RunGsUtil(
        ['iam', 'get', gsutil_object2.uri], return_stdout=True)
    self.assertEqualsPoliciesString(set_iam_string, set_iam_string2)
    self.assertIn(
        self.public_object_read_binding[0],
        json.loads(set_iam_string)['bindings'])

    # Check that old versions are not affected by the set IAM call.
    iam_string_old = self.RunGsUtil(
        ['iam', 'get', old_gsutil_object.version_specific_uri],
        return_stdout=True)
    iam_string_old2 = self.RunGsUtil(
        ['iam', 'get', old_gsutil_object2.version_specific_uri],
        return_stdout=True)
    self.assertEqualsPoliciesString(iam_string_old, iam_string_old2)
    self.assertEqualsPoliciesString(self.object_iam_string, iam_string_old)

  def test_set_valid_iam_multithreaded_multiple_objects(self):
    """Tests setting a valid IAM on multiple objects."""
    (old_gsutil_object, old_gsutil_object2, gsutil_object,
     gsutil_object2) = self._create_multiple_objects()

    # Set IAM policy on newest versions of all objects.
    self.RunGsUtil(['-m', 'iam', 'set', '-r',
                    self.new_object_iam_path, self.versioned_bucket.uri])
    set_iam_string = self.RunGsUtil(
        ['iam', 'get', gsutil_object.uri], return_stdout=True)
    set_iam_string2 = self.RunGsUtil(
        ['iam', 'get', gsutil_object2.uri], return_stdout=True)
    self.assertEqualsPoliciesString(set_iam_string, set_iam_string2)
    self.assertIn(
        self.public_object_read_binding[0],
        json.loads(set_iam_string)['bindings'])

    # Check that old versions are not affected by the set IAM call.
    iam_string_old = self.RunGsUtil(
        ['iam', 'get', old_gsutil_object.version_specific_uri],
        return_stdout=True)
    iam_string_old2 = self.RunGsUtil(
        ['iam', 'get', old_gsutil_object2.version_specific_uri],
        return_stdout=True)
    self.assertEqualsPoliciesString(iam_string_old, iam_string_old2)
    self.assertEqualsPoliciesString(self.object_iam_string, iam_string_old)

  def test_set_valid_iam_multiple_objects_all_versions(self):
    """Tests set IAM policy on all versions of all objects."""
    (old_gsutil_object, old_gsutil_object2, gsutil_object,
     gsutil_object2) = self._create_multiple_objects()

    self.RunGsUtil(['iam', 'set', '-ra', self.new_object_iam_path,
                    self.versioned_bucket.uri])
    set_iam_string = self.RunGsUtil(
        ['iam', 'get', gsutil_object.version_specific_uri], return_stdout=True)
    set_iam_string2 = self.RunGsUtil(
        ['iam', 'get', gsutil_object2.version_specific_uri],
        return_stdout=True)
    set_iam_string_old = self.RunGsUtil(
        ['iam', 'get', old_gsutil_object.version_specific_uri],
        return_stdout=True)
    set_iam_string_old2 = self.RunGsUtil(
        ['iam', 'get', old_gsutil_object2.version_specific_uri],
        return_stdout=True)
    self.assertEqualsPoliciesString(set_iam_string, set_iam_string2)
    self.assertEqualsPoliciesString(set_iam_string, set_iam_string_old)
    self.assertEqualsPoliciesString(set_iam_string, set_iam_string_old2)
    self.assertIn(
        self.public_object_read_binding[0],
        json.loads(set_iam_string)['bindings'])

  def test_set_error(self):
    """Tests fail-fast behavior of iam set.

    We initialize two buckets (bucket, bucket2) and attempt to set both along
    with a third, non-existent bucket in between, self.nonexistent_bucket_name.

    We want to ensure
      1.) Bucket "bucket" IAM policy has been set appropriately,
      2.) Bucket self.nonexistent_bucket_name has caused an error, and
      3.) gsutil has exited and "bucket2"'s IAM policy is unaltered.
    """

    bucket = self.CreateBucket()
    bucket2 = self.CreateBucket()

    stderr = self.RunGsUtil(['iam', 'set', '-e', '', self.new_bucket_iam_path,
                             bucket.uri,
                             'gs://%s' % self.nonexistent_bucket_name,
                             bucket2.uri],
                            return_stderr=True, expected_status=1)

    # The program has exited due to a bucket lookup 404.
    self.assertIn('BucketNotFoundException', stderr)
    set_iam_string = self.RunGsUtil(
        ['iam', 'get', bucket.uri], return_stdout=True)
    set_iam_string2 = self.RunGsUtil(
        ['iam', 'get', bucket2.uri], return_stdout=True)

    # The IAM policy has been set on Bucket "bucket".
    self.assertIn(
        self.public_bucket_read_binding[0],
        json.loads(set_iam_string)['bindings'])

    # The IAM policy for Bucket "bucket2" remains unchanged.
    self.assertEqualsPoliciesString(self.bucket_iam_string, set_iam_string2)

  def test_set_force_error(self):
    """Tests ignoring failure behavior of iam set.

    Similar to TestIamSet.test_set_error, except here we want to ensure
      1.) Bucket "bucket" IAM policy has been set appropriately,
      2.) Bucket self.nonexistent_bucket_name has caused an error, BUT
      3.) gsutil has continued and "bucket2"'s IAM policy has been set as well.
    """
    bucket = self.CreateBucket()
    bucket2 = self.CreateBucket()

    stderr = self.RunGsUtil(['iam', 'set', '-f', self.new_bucket_iam_path,
                             bucket.uri,
                             'gs://%s' % self.nonexistent_bucket_name,
                             bucket2.uri],
                            return_stderr=True, expected_status=1)

    # The program asserts that an error has occured (due to 404).
    self.assertIn('CommandException', stderr)

    set_iam_string = self.RunGsUtil(
        ['iam', 'get', bucket.uri], return_stdout=True)
    set_iam_string2 = self.RunGsUtil(
        ['iam', 'get', bucket2.uri], return_stdout=True)

    # The IAM policy has been set appropriately on Bucket "bucket".
    self.assertIn(
        self.public_bucket_read_binding[0],
        json.loads(set_iam_string)['bindings'])

    # The IAM policy has also been set on Bucket "bucket2".
    self.assertEqualsPoliciesString(set_iam_string, set_iam_string2)

  def test_set_multithreaded_error(self):
    """Tests fail-fast behavior of multithreaded iam set.

    This is testing gsutil iam set with the -m and -r flags present in
    invocation.

    N.B.: Currently, (-m, -r) behaves identically to (-m, -fr) and (-fr,).
    However, (-m, -fr) and (-fr,) behavior is not as expected due to
    name_expansion.NameExpansionIterator.next raising problematic e.g. 404
    or 403 errors. More details on this issue can be found in comments in
    commands.iam.IamCommand._SetIam.

    Thus, the following command
      gsutil -m iam set -fr <object_policy> gs://bad_bucket gs://good_bucket

    will NOT set policies on objects in gs://good_bucket due to an error when
    iterating over gs://bad_bucket.
    """

    gsutil_object = self.CreateObject(bucket_uri=self.bucket, contents='foobar')
    gsutil_object2 = self.CreateObject(
        bucket_uri=self.bucket, contents='foobar')

    stderr = self.RunGsUtil(['-m', 'iam', 'set', '-r', self.new_object_iam_path,
                             'gs://%s' % self.nonexistent_bucket_name,
                             self.bucket.uri],
                            return_stderr=True, expected_status=1)
    self.assertIn('BucketNotFoundException', stderr)
    set_iam_string = self.RunGsUtil(
        ['iam', 'get', gsutil_object.uri], return_stdout=True)
    set_iam_string2 = self.RunGsUtil(
        ['iam', 'get', gsutil_object2.uri], return_stdout=True)
    self.assertEqualsPoliciesString(set_iam_string, set_iam_string2)
    self.assertEqualsPoliciesString(self.object_iam_string, set_iam_string)

  def test_set_valid_iam_single_unversioned_object(self):
    """Tests setting a valid IAM on an object."""
    gsutil_object = self.CreateObject(bucket_uri=self.bucket, contents='foobar')

    lookup_uri = gsutil_object.uri
    self.RunGsUtil(['iam', 'set', self.new_object_iam_path, lookup_uri])
    set_iam_string = self.RunGsUtil(
        ['iam', 'get', lookup_uri], return_stdout=True)
    self.RunGsUtil(
        ['iam', 'set', '-e', json.loads(set_iam_string)['etag'],
         self.old_object_iam_path, lookup_uri])
    reset_iam_string = self.RunGsUtil(
        ['iam', 'get', lookup_uri], return_stdout=True)

    self.assertEqualsPoliciesString(self.object_iam_string, reset_iam_string)
    self.assertIn(
        self.public_object_read_binding[0],
        json.loads(set_iam_string)['bindings'])

  def test_set_valid_iam_single_versioned_object(self):
    """Tests setting a valid IAM on a versioned object."""
    gsutil_object = self.CreateObject(bucket_uri=self.bucket, contents='foobar')

    lookup_uri = gsutil_object.version_specific_uri
    self.RunGsUtil(['iam', 'set', self.new_object_iam_path, lookup_uri])
    set_iam_string = self.RunGsUtil(
        ['iam', 'get', lookup_uri], return_stdout=True)
    self.RunGsUtil(
        ['iam', 'set', '-e', json.loads(set_iam_string)['etag'],
         self.old_object_iam_path, lookup_uri])
    reset_iam_string = self.RunGsUtil(
        ['iam', 'get', lookup_uri], return_stdout=True)

    self.assertEqualsPoliciesString(self.object_iam_string, reset_iam_string)
    self.assertIn(
        self.public_object_read_binding[0],
        json.loads(set_iam_string)['bindings'])

  def test_set_valid_iam_multithreaded_single_object(self):
    """Tests setting a valid IAM on a single object with multithreading."""
    gsutil_object = self.CreateObject(bucket_uri=self.bucket, contents='foobar')

    lookup_uri = gsutil_object.version_specific_uri
    self.RunGsUtil(
        ['-m', 'iam', 'set', '-e', '', self.new_object_iam_path, lookup_uri])
    set_iam_string = self.RunGsUtil(
        ['iam', 'get', lookup_uri], return_stdout=True)
    self.RunGsUtil(
        ['-m', 'iam', 'set', '-e', '', self.old_object_iam_path, lookup_uri])
    reset_iam_string = self.RunGsUtil(
        ['iam', 'get', lookup_uri], return_stdout=True)

    self.assertEqualsPoliciesString(self.object_iam_string, reset_iam_string)
    self.assertIn(
        self.public_object_read_binding[0],
        json.loads(set_iam_string)['bindings'])

    # Test multithreading on single object, specified with wildcards.
    lookup_uri = '%s*' % self.bucket.uri
    self.RunGsUtil(
        ['-m', 'iam', 'set', '-e', '', self.new_object_iam_path, lookup_uri])
    set_iam_string = self.RunGsUtil(
        ['iam', 'get', lookup_uri], return_stdout=True)
    self.RunGsUtil(
        ['-m', 'iam', 'set', '-e', '', self.old_object_iam_path, lookup_uri])
    reset_iam_string = self.RunGsUtil(
        ['iam', 'get', lookup_uri], return_stdout=True)

    self.assertEqualsPoliciesString(self.object_iam_string, reset_iam_string)
    self.assertIn(
        self.public_object_read_binding[0],
        json.loads(set_iam_string)['bindings'])
