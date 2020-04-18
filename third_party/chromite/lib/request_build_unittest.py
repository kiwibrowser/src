# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for request_build.py."""

from __future__ import print_function

import json
import mock

from chromite.lib import auth
from chromite.lib import buildbucket_lib
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import request_build

# Tests need internal access.
# pylint: disable=protected-access

class RequestBuildHelperTestsBase(cros_test_lib.MockTestCase):
  """Tests for RequestBuild."""
  BRANCH = 'test-branch'
  PATCHES = ('5555', '6666')
  BUILD_CONFIG_MIN = 'amd64-generic-paladin-tryjob'
  BUILD_CONFIG_MAX = 'amd64-generic-paladin'
  UNKNOWN_CONFIG = 'unknown-config'
  LUCI_BUILDER = 'luci_build'
  DISPLAY_LABEL = 'display'
  PASS_THROUGH_ARGS = ['funky', 'cold', 'medina']
  TEST_EMAIL = 'explicit_email'
  MASTER_CIDB_ID = 'master_cidb_id'
  MASTER_BUILDBUCKET_ID = 'master_bb_id'
  TEST_BUCKET = 'test_bucket'

  def setUp(self):
    self.maxDiff = None

  def _CreateJobMin(self):
    return request_build.RequestBuild(build_config=self.BUILD_CONFIG_MIN)

  def _CreateJobMax(self):
    return request_build.RequestBuild(
        build_config=self.BUILD_CONFIG_MAX,
        luci_builder=self.LUCI_BUILDER,
        display_label=self.DISPLAY_LABEL,
        branch=self.BRANCH,
        extra_args=self.PASS_THROUGH_ARGS,
        user_email=self.TEST_EMAIL,
        master_cidb_id=self.MASTER_CIDB_ID,
        master_buildbucket_id=self.MASTER_BUILDBUCKET_ID,
        bucket=self.TEST_BUCKET)

  def _CreateJobUnknown(self):
    return request_build.RequestBuild(
        build_config=self.UNKNOWN_CONFIG,
        display_label=self.DISPLAY_LABEL,
        branch='master',
        extra_args=(),
        user_email='default_email',
        master_buildbucket_id=None)


class RequestBuildHelperTestsMock(RequestBuildHelperTestsBase):
  """Perform real buildbucket requests against a fake instance."""

  def setUp(self):
    # This mocks out the class, then creates a return_value for a function on
    # instances of it. We do this instead of just mocking out the function to
    # ensure not real network requests are made in other parts of the class.
    client_mock = self.PatchObject(buildbucket_lib, 'BuildbucketClient')
    client_mock().PutBuildRequest.return_value = {
        'build': {'id': 'fake_buildbucket_id'}
    }

  def testMinRequestBody(self):
    """Verify our request body with min options."""
    job = self._CreateJobMin()

    self.assertEqual(job.bucket, constants.INTERNAL_SWARMING_BUILDBUCKET_BUCKET)
    self.assertEqual(job.luci_builder, config_lib.LUCI_BUILDER_TRY)
    self.assertEqual(job.display_label, config_lib.DISPLAY_LABEL_TRYJOB)

    body = job._GetRequestBody()

    self.assertEqual(body, {
        'parameters_json': mock.ANY,
        'bucket': 'luci.chromeos.general',
        'tags': [
            'cbb_branch:master',
            'cbb_config:amd64-generic-paladin-tryjob',
            'cbb_display_label:tryjob',
        ]
    })

    parameters_parsed = json.loads(body['parameters_json'])

    self.assertEqual(parameters_parsed, {
        u'builder_name': u'Try',
        u'properties': {
            u'cbb_branch': u'master',
            u'cbb_config': u'amd64-generic-paladin-tryjob',
            u'cbb_display_label': u'tryjob',
            u'cbb_extra_args': [],
        }
    })


  def testMaxRequestBody(self):
    """Verify our request body with max options."""
    job = self._CreateJobMax()

    self.assertEqual(job.bucket, self.TEST_BUCKET)
    self.assertEqual(job.luci_builder, self.LUCI_BUILDER)
    self.assertEqual(job.display_label, 'display')

    body = job._GetRequestBody()

    self.assertEqual(body, {
        'parameters_json': mock.ANY,
        'bucket': self.TEST_BUCKET,
        'tags': [
            'buildset:cros/master_buildbucket_id/master_bb_id',
            'cbb_branch:test-branch',
            'cbb_config:amd64-generic-paladin',
            'cbb_display_label:display',
            'cbb_email:explicit_email',
            'cbb_master_build_id:master_cidb_id',
            'cbb_master_buildbucket_id:master_bb_id',
            'master:False',
        ]
    })

    parameters_parsed = json.loads(body['parameters_json'])

    self.assertEqual(parameters_parsed, {
        u'builder_name': u'luci_build',
        u'email_notify': [{u'email': u'explicit_email'}],
        u'properties': {
            u'buildset': u'cros/master_buildbucket_id/master_bb_id',
            u'cbb_branch': u'test-branch',
            u'cbb_config': u'amd64-generic-paladin',
            u'cbb_display_label': u'display',
            u'cbb_email': u'explicit_email',
            u'cbb_extra_args': [u'funky', u'cold', u'medina'],
            u'cbb_master_build_id': u'master_cidb_id',
            u'cbb_master_buildbucket_id': u'master_bb_id',
            u'master': u'False',
        }
    })

  def testUnknownRequestBody(self):
    """Verify our request body with max options."""
    self.maxDiff = None
    body = self._CreateJobUnknown()._GetRequestBody()

    self.assertEqual(body, {
        'parameters_json': mock.ANY,
        'bucket': 'luci.chromeos.general',
        'tags': [
            'cbb_branch:master',
            'cbb_config:unknown-config',
            'cbb_display_label:display',
            'cbb_email:default_email',
        ]
    })

    parameters_parsed = json.loads(body['parameters_json'])

    self.assertEqual(parameters_parsed, {
        u'builder_name': u'Try',
        u'email_notify': [{u'email': u'default_email'}],
        u'properties': {
            u'cbb_branch': u'master',
            u'cbb_config': u'unknown-config',
            u'cbb_display_label': u'display',
            u'cbb_email': u'default_email',
            u'cbb_extra_args': [],
        }
    })

  def testMinDryRun(self):
    """Do a dryrun of posting the request, min options."""
    job = self._CreateJobMin()
    job.Submit(testjob=True, dryrun=True)

  def testMaxDryRun(self):
    """Do a dryrun of posting the request, max options."""
    job = self._CreateJobMax()
    job.Submit(testjob=True, dryrun=True)


class RequestBuildHelperTestsNetork(RequestBuildHelperTestsBase):
  """Perform real buildbucket requests against a test instance."""

  def verifyBuildbucketRequest(self,
                               buildbucket_id,
                               expected_bucket,
                               expected_tags,
                               expected_parameters):
    """Verify the contents of a push to the TEST buildbucket instance.

    Args:
      buildbucket_id: Id to verify.
      expected_bucket: Bucket the push was supposed to go to as a string.
      expected_tags: List of buildbucket tags as strings.
      expected_parameters: Python dict equivalent to json string in
                           parameters_json.
    """
    buildbucket_client = buildbucket_lib.BuildbucketClient(
        auth.GetAccessToken, buildbucket_lib.BUILDBUCKET_TEST_HOST,
        service_account_json=buildbucket_lib.GetServiceAccount(
            constants.CHROMEOS_SERVICE_ACCOUNT))

    request = buildbucket_client.GetBuildRequest(buildbucket_id, False)

    self.assertEqual(request['build']['id'], buildbucket_id)
    self.assertEqual(request['build']['bucket'], expected_bucket)
    self.assertItemsEqual(request['build']['tags'], expected_tags)

    request_parameters = json.loads(request['build']['parameters_json'])
    self.assertEqual(request_parameters, expected_parameters)

  @cros_test_lib.NetworkTest()
  def testMinTestBucket(self):
    """Talk to a test buildbucket instance with min job settings."""
    job = self._CreateJobMin()
    result = job.Submit(testjob=True)

    self.verifyBuildbucketRequest(
        result.buildbucket_id,
        'luci.chromeos.general',
        [
            'builder:Try',
            'cbb_branch:master',
            'cbb_config:amd64-generic-paladin-tryjob',
            'cbb_display_label:tryjob',
        ],
        {
            u'builder_name': u'Try',
            u'properties': {
                u'cbb_branch': u'master',
                u'cbb_config': u'amd64-generic-paladin-tryjob',
                u'cbb_display_label': u'tryjob',
                u'cbb_extra_args': [],
            },
        })

    self.assertEqual(
        result,
        request_build.ScheduledBuild(
            buildbucket_id=result.buildbucket_id,
            build_config='amd64-generic-paladin-tryjob',
            url=(u'http://cros-goldeneye/chromeos/healthmonitoring/'
                 u'buildDetails?buildbucketId=%s' % result.buildbucket_id),
            created_ts=mock.ANY),
    )

  @cros_test_lib.NetworkTest()
  def testMaxTestBucket(self):
    """Talk to a test buildbucket instance with max job settings."""
    job = self._CreateJobMax()
    result = job.Submit(testjob=True)

    self.verifyBuildbucketRequest(
        result.buildbucket_id,
        self.TEST_BUCKET,
        [
            'builder:luci_build',
            'buildset:cros/master_buildbucket_id/master_bb_id',
            'cbb_branch:test-branch',
            'cbb_display_label:display',
            'cbb_config:amd64-generic-paladin',
            'cbb_email:explicit_email',
            'cbb_master_build_id:master_cidb_id',
            'cbb_master_buildbucket_id:master_bb_id',
            'master:False',
        ],
        {
            u'builder_name': u'luci_build',
            u'email_notify': [{u'email': u'explicit_email'}],
            u'properties': {
                u'buildset': u'cros/master_buildbucket_id/master_bb_id',
                u'cbb_branch': u'test-branch',
                u'cbb_config': u'amd64-generic-paladin',
                u'cbb_display_label': u'display',
                u'cbb_email': u'explicit_email',
                u'cbb_extra_args': [u'funky', u'cold', u'medina'],
                u'cbb_master_build_id': u'master_cidb_id',
                u'cbb_master_buildbucket_id': u'master_bb_id',
                u'master': u'False',
            },
        })

    self.assertEqual(
        result,
        request_build.ScheduledBuild(
            buildbucket_id=result.buildbucket_id,
            build_config='amd64-generic-paladin',
            url=(u'http://cros-goldeneye/chromeos/healthmonitoring/'
                 u'buildDetails?buildbucketId=%s' % result.buildbucket_id),
            created_ts=mock.ANY),
    )

  # pylint: disable=protected-access
  def testPostConfigToBuildBucket(self):
    """Check syntax for PostConfigsToBuildBucket."""
    self.PatchObject(auth, 'Login')
    self.PatchObject(auth, 'Token')
    self.PatchObject(request_build.RequestBuild, '_PutConfigToBuildBucket')

    remote_try_job = request_build.RequestBuild(
        build_config=self.BUILD_CONFIG_MIN,
        display_label=self.DISPLAY_LABEL,
        branch='master',
        extra_args=(),
        user_email='default_email',
        master_buildbucket_id=None)
    remote_try_job.Submit(testjob=True, dryrun=True)
