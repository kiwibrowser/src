# -*- coding: utf-8 -*-
# Copyright 2011 Google Inc. All Rights Reserved.
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
"""Implementation of mb command for creating cloud storage buckets."""

from __future__ import absolute_import

import re
import textwrap

from gslib.cloud_api import BadRequestException
from gslib.command import Command
from gslib.command_argument import CommandArgument
from gslib.cs_api_map import ApiSelector
from gslib.exception import CommandException
from gslib.exception import InvalidUrlError
from gslib.storage_url import StorageUrlFromString
from gslib.third_party.storage_apitools import storage_v1_messages as apitools_messages
from gslib.util import InsistAscii
from gslib.util import NO_MAX
from gslib.util import NormalizeStorageClass


_SYNOPSIS = """
  gsutil mb [-c class] [-l location] [-p proj_id] url...
"""

_DETAILED_HELP_TEXT = ("""
<B>SYNOPSIS</B>
""" + _SYNOPSIS + """


<B>DESCRIPTION</B>
  The mb command creates a new bucket. Google Cloud Storage has a single
  namespace, so you are not allowed to create a bucket with a name already
  in use by another user. You can, however, carve out parts of the bucket name
  space corresponding to your company's domain name (see "gsutil help naming").

  If you don't specify a project ID using the -p option, the bucket is created
  using the default project ID specified in your gsutil configuration file
  (see "gsutil help config"). For more details about projects see "gsutil help
  projects".

  The -c and -l options specify the storage class and location, respectively,
  for the bucket. Once a bucket is created in a given location and with a
  given storage class, it cannot be moved to a different location, and the
  storage class cannot be changed. Instead, you would need to create a new
  bucket and move the data over and then delete the original bucket.


<B>BUCKET STORAGE CLASSES</B>
  You can specify one of the `storage classes
  <https://cloud.google.com/storage/docs/storage-classes>`_ for a bucket
  with the -c option.

  Example:

    gsutil mb -c nearline gs://some-bucket

  See online documentation for
  `pricing <https://cloud.google.com/storage/pricing>`_ and
  `SLA <https://cloud.google.com/storage/sla>`_ details.

  If you don't specify a -c option, the bucket is created with the
  default storage class Standard Storage, which is equivalent to Multi-Regional
  Storage or Regional Storage, depending on whether the bucket was created in
  a multi-regional location or regional location, respectively.

<B>BUCKET LOCATIONS</B>
  You can specify one of the 'available locations
  <https://cloud.google.com/storage/docs/bucket-locations>`_ for a bucket
  with the -l option.

  Examples:

    gsutil mb -l asia gs://some-bucket

    gsutil mb -c regional -l us-east1 gs://some-bucket

  If you don't specify a -l option, the bucket is created in the default
  location (US).

<B>OPTIONS</B>
  -c class          Specifies the default storage class. Default is "Standard".

  -l location       Can be any multi-regional or regional location. See
                    https://cloud.google.com/storage/docs/bucket-locations
                    for a discussion of this distinction. Default is US.
                    Locations are case insensitive.

  -p proj_id        Specifies the project ID under which to create the bucket.

  -s class          Same as -c.
""")


# Regex to disallow buckets violating charset or not [3..255] chars total.
BUCKET_NAME_RE = re.compile(r'^[a-zA-Z0-9][a-zA-Z0-9\._-]{1,253}[a-zA-Z0-9]$')
# Regex to disallow buckets with individual DNS labels longer than 63.
TOO_LONG_DNS_NAME_COMP = re.compile(r'[-_a-z0-9]{64}')


class MbCommand(Command):
  """Implementation of gsutil mb command."""

  # Command specification. See base class for documentation.
  command_spec = Command.CreateCommandSpec(
      'mb',
      command_name_aliases=['makebucket', 'createbucket', 'md', 'mkdir'],
      usage_synopsis=_SYNOPSIS,
      min_args=1,
      max_args=NO_MAX,
      supported_sub_args='c:l:p:s:',
      file_url_ok=False,
      provider_url_ok=False,
      urls_start_arg=0,
      gs_api_support=[ApiSelector.XML, ApiSelector.JSON],
      gs_default_api=ApiSelector.JSON,
      argparse_arguments=[
          CommandArgument.MakeZeroOrMoreCloudBucketURLsArgument()
      ]
  )
  # Help specification. See help_provider.py for documentation.
  help_spec = Command.HelpSpec(
      help_name='mb',
      help_name_aliases=[
          'createbucket', 'makebucket', 'md', 'mkdir', 'location', 'dra',
          'dras', 'reduced_availability', 'durable_reduced_availability', 'rr',
          'reduced_redundancy', 'standard', 'storage class', 'nearline', 'nl'],
      help_type='command_help',
      help_one_line_summary='Make buckets',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )

  def RunCommand(self):
    """Command entry point for the mb command."""
    location = None
    storage_class = None
    if self.sub_opts:
      for o, a in self.sub_opts:
        if o == '-l':
          location = a
        elif o == '-p':
          # Project IDs are sent as header values when using gs and s3 XML APIs.
          InsistAscii(a, 'Invalid non-ASCII character found in project ID')
          self.project_id = a
        elif o == '-c' or o == '-s':
          storage_class = NormalizeStorageClass(a)

    bucket_metadata = apitools_messages.Bucket(location=location,
                                               storageClass=storage_class)

    for bucket_url_str in self.args:
      bucket_url = StorageUrlFromString(bucket_url_str)
      if not bucket_url.IsBucket():
        raise CommandException('The mb command requires a URL that specifies a '
                               'bucket.\n"%s" is not valid.' % bucket_url)
      if (not BUCKET_NAME_RE.match(bucket_url.bucket_name) or
          TOO_LONG_DNS_NAME_COMP.search(bucket_url.bucket_name)):
        raise InvalidUrlError(
            'Invalid bucket name in URL "%s"' % bucket_url.bucket_name)

      self.logger.info('Creating %s...', bucket_url)
      # Pass storage_class param only if this is a GCS bucket. (In S3 the
      # storage class is specified on the key object.)
      try:
        self.gsutil_api.CreateBucket(
            bucket_url.bucket_name, project_id=self.project_id,
            metadata=bucket_metadata, provider=bucket_url.scheme)
      except BadRequestException as e:
        if (e.status == 400 and e.reason == 'DotfulBucketNameNotUnderTld' and
            bucket_url.scheme == 'gs'):
          bucket_name = bucket_url.bucket_name
          final_comp = bucket_name[bucket_name.rfind('.')+1:]
          raise CommandException('\n'.join(textwrap.wrap(
              'Buckets with "." in the name must be valid DNS names. The bucket'
              ' you are attempting to create (%s) is not a valid DNS name,'
              ' because the final component (%s) is not currently a valid part'
              ' of the top-level DNS tree.' % (bucket_name, final_comp))))
        else:
          raise

    return 0
