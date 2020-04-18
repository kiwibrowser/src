# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Code related to Remote tryjobs."""

from __future__ import print_function

import collections
import json

from chromite.lib import auth
from chromite.lib import buildbucket_lib
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_logging as logging

site_config = config_lib.GetConfig()


# URL to open a build details page.
BUILD_DETAILS_PATTERN = (
    'http://cros-goldeneye/chromeos/healthmonitoring/buildDetails?'
    'buildbucketId=%(buildbucket_id)s'
)


class RemoteRequestFailure(Exception):
  """Thrown when requesting a tryjob fails."""


# Contains the results of a single scheduled build.
ScheduledBuild = collections.namedtuple(
    'ScheduledBuild', ('buildbucket_id', 'build_config', 'url', 'created_ts'))


def TryJobUrl(buildbucket_id):
  """Get link to the build UI for a given build.

  Returns:
    The URL as a string to view the given build.
  """
  return BUILD_DETAILS_PATTERN % {'buildbucket_id': buildbucket_id}


def SlaveBuildSet(master_buildbucket_id):
  """Compute the buildset id for all slaves of a master builder.

  Args:
    master_buildbucket_id: The buildbucket id of the master build.

  Returns:
    A string to use as a buildset for the slave builders, or None.
  """
  if not master_buildbucket_id:
    return None

  return 'cros/master_buildbucket_id/%s' % master_buildbucket_id

class RequestBuild(object):
  """Request a builder via buildbucket."""
  # Buildbucket_put response must contain 'buildbucket_bucket:bucket]',
  # '[config:config_name] and '[buildbucket_id:id]'.
  BUILDBUCKET_PUT_RESP_FORMAT = ('Successfully sent PUT request to '
                                 '[buildbucket_bucket:%s] '
                                 'with [config:%s] [buildbucket_id:%s].')

  def __init__(self,
               build_config,
               luci_builder=None,
               display_label=None,
               branch='master',
               extra_args=(),
               user_email=None,
               master_cidb_id=None,
               master_buildbucket_id=None,
               bucket=constants.INTERNAL_SWARMING_BUILDBUCKET_BUCKET):
    """Construct the object.

    Args:
      build_config: A build config name to schedule.
      luci_builder: Name of builder to execute the build, or None.
                    For waterfall builds, this is the name of the build column.
                    For swarming builds, this is the LUCI builder name.
      display_label: String describing how build group on waterfall, or None.
      branch: Name of branch to build for.
      extra_args: Command line arguments to pass to cbuildbot in job.
      user_email: Email address of person requesting job, or None.
      master_cidb_id: CIDB id of scheduling builder, or None.
      master_buildbucket_id: buildbucket id of scheduling builder, or None.
      bucket: Which bucket do we request the build in?
    """
    self.bucket = bucket

    if build_config in site_config:
      # Extract from build_config, if possible.
      self.luci_builder = site_config[build_config].luci_builder
      self.display_label = site_config[build_config].display_label
    else:
      # Use generic defaults if needed (lowest priority)
      self.luci_builder = config_lib.LUCI_BUILDER_TRY
      self.display_label = config_lib.DISPLAY_LABEL_TRYJOB

    # But allow an explicit overrides.
    if luci_builder:
      self.luci_builder = luci_builder

    if display_label:
      self.display_label = display_label

    self.build_config = build_config
    self.branch = branch
    self.extra_args = extra_args
    self.user_email = user_email
    self.master_cidb_id = master_cidb_id
    self.master_buildbucket_id = master_buildbucket_id

  def _GetRequestBody(self):
    """Generate the request body for a swarming buildbucket request.

    Returns:
      buildbucket request properties as a python dict.
    """
    tags = {
        # buildset identifies a group of related builders.
        'buildset': SlaveBuildSet(self.master_buildbucket_id),
        'cbb_display_label': self.display_label,
        'cbb_branch': self.branch,
        'cbb_config': self.build_config,
        'cbb_email': self.user_email,
        'cbb_master_build_id': self.master_cidb_id,
        'cbb_master_buildbucket_id': self.master_buildbucket_id,
    }

    if self.master_cidb_id or self.master_buildbucket_id:
      # Used by Legoland as part of grouping slave builds. Set to False for
      # slave builds, not set otherwise.
      tags['master'] = 'False'

    # Don't include tags with no value, there is no point.
    tags = {k: v for k, v in tags.iteritems() if v}

    # All tags should also be listed as properties.
    properties = tags.copy()
    properties['cbb_extra_args'] = self.extra_args

    parameters = {
        'builder_name': self.luci_builder,
        'properties': properties,
    }

    if self.user_email:
      parameters['email_notify'] = [{'email': self.user_email}]

    return {
        'bucket': self.bucket,
        'parameters_json': json.dumps(parameters, sort_keys=True),
        # These tags are indexed and searchable in buildbucket.
        'tags': ['%s:%s' % (k, tags[k]) for k in sorted(tags.keys())],
    }

  def _PutConfigToBuildBucket(self, buildbucket_client, dryrun):
    """Put the tryjob request to buildbucket.

    Args:
      buildbucket_client: The buildbucket client instance.
      dryrun: bool controlling dryrun behavior.

    Returns:
      ScheduledBuild describing the scheduled build.

    Raises:
      RemoteRequestFailure.
    """
    request_body = self._GetRequestBody()
    content = buildbucket_client.PutBuildRequest(
        json.dumps(request_body), dryrun)

    if buildbucket_lib.GetNestedAttr(content, ['error']):
      raise RemoteRequestFailure(
          'buildbucket error.\nReason: %s\n Message: %s' %
          (buildbucket_lib.GetErrorReason(content),
           buildbucket_lib.GetErrorMessage(content)))

    buildbucket_id = buildbucket_lib.GetBuildId(content)
    url = TryJobUrl(buildbucket_id)
    created_ts = buildbucket_lib.GetBuildCreated_ts(content)

    result = ScheduledBuild(buildbucket_id, self.build_config, url, created_ts)

    logging.info(self.BUILDBUCKET_PUT_RESP_FORMAT, result)

    return result

  def Submit(self, testjob=False, dryrun=False):
    """Submit the tryjob through Git.

    Args:
      testjob: Submit job to the test branch of the tryjob repo.  The tryjob
               will be ignored by production master.
      dryrun: Setting to true will run everything except the final submit step.

    Returns:
      A ScheduledBuild instance.
    """
    host = (buildbucket_lib.BUILDBUCKET_TEST_HOST if testjob
            else buildbucket_lib.BUILDBUCKET_HOST)
    buildbucket_client = buildbucket_lib.BuildbucketClient(
        auth.GetAccessToken, host,
        service_account_json=buildbucket_lib.GetServiceAccount(
            constants.CHROMEOS_SERVICE_ACCOUNT))

    return self._PutConfigToBuildBucket(buildbucket_client, dryrun)
