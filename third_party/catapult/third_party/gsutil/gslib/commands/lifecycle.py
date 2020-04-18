# -*- coding: utf-8 -*-
# Copyright 2013 Google Inc. All Rights Reserved.
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
"""Implementation of lifecycle configuration command for GCS buckets."""

from __future__ import absolute_import

import sys

from gslib import metrics
from gslib.command import Command
from gslib.command_argument import CommandArgument
from gslib.cs_api_map import ApiSelector
from gslib.exception import CommandException
from gslib.exception import NO_URLS_MATCHED_TARGET
from gslib.help_provider import CreateHelpText
from gslib.third_party.storage_apitools import storage_v1_messages as apitools_messages
from gslib.translation_helper import LifecycleTranslation
from gslib.util import NO_MAX
from gslib.util import UrlsAreForSingleProvider


_GET_SYNOPSIS = """
  gsutil lifecycle get url
"""

_SET_SYNOPSIS = """
  gsutil lifecycle set config-json-file url...
"""

_SYNOPSIS = _GET_SYNOPSIS + _SET_SYNOPSIS.lstrip('\n') + '\n'

_GET_DESCRIPTION = """
<B>GET</B>
  Gets the lifecycle configuration for a given bucket. You can get the
  lifecycle configuration for only one bucket at a time. The output can be
  redirected into a file, edited and then updated via the set sub-command.

"""

_SET_DESCRIPTION = """
<B>SET</B>
  Sets the lifecycle configuration on one or more buckets. The config-json-file
  specified on the command line should be a path to a local file containing
  the lifecycle configuration JSON document.

"""

_DESCRIPTION = """
  The lifecycle command can be used to get or set lifecycle management policies
  for the given bucket(s). This command is supported for buckets only, not
  objects. For more information on object lifecycle management, please see the
  `Google Cloud Storage docs <https://cloud.google.com/storage/docs/lifecycle>`_.

  The lifecycle command has two sub-commands:
""" + _GET_DESCRIPTION + _SET_DESCRIPTION + """
<B>EXAMPLES</B>
  The following lifecycle configuration JSON document specifies that all objects
  in this bucket that are more than 365 days old will be deleted automatically:

    {
      "rule":
      [
        {
          "action": {"type": "Delete"},
          "condition": {"age": 365}
        }
      ]
    }

  The following (empty) lifecycle configuration JSON document removes all
  lifecycle configuration for a bucket:

    {}

"""

_DETAILED_HELP_TEXT = CreateHelpText(_SYNOPSIS, _DESCRIPTION)

_get_help_text = CreateHelpText(_GET_SYNOPSIS, _GET_DESCRIPTION)
_set_help_text = CreateHelpText(_SET_SYNOPSIS, _SET_DESCRIPTION)


class LifecycleCommand(Command):
  """Implementation of gsutil lifecycle command."""

  # Command specification. See base class for documentation.
  command_spec = Command.CreateCommandSpec(
      'lifecycle',
      command_name_aliases=['lifecycleconfig'],
      usage_synopsis=_SYNOPSIS,
      min_args=2,
      max_args=NO_MAX,
      supported_sub_args='',
      file_url_ok=True,
      provider_url_ok=False,
      urls_start_arg=1,
      gs_api_support=[
          ApiSelector.JSON,
          ApiSelector.XML
      ],
      gs_default_api=ApiSelector.JSON,
      argparse_arguments={
          'set': [
              CommandArgument.MakeNFileURLsArgument(1),
              CommandArgument.MakeZeroOrMoreCloudBucketURLsArgument()
          ],
          'get': [
              CommandArgument.MakeNCloudBucketURLsArgument(1)
          ]
      }
  )
  # Help specification. See help_provider.py for documentation.
  help_spec = Command.HelpSpec(
      help_name='lifecycle',
      help_name_aliases=['getlifecycle', 'setlifecycle'],
      help_type='command_help',
      help_one_line_summary=(
          'Get or set lifecycle configuration for a bucket'),
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={'get': _get_help_text, 'set': _set_help_text},
  )

  def _SetLifecycleConfig(self):
    """Sets lifecycle configuration for a Google Cloud Storage bucket."""
    lifecycle_arg = self.args[0]
    url_args = self.args[1:]
    # Disallow multi-provider 'lifecycle set' requests.
    if not UrlsAreForSingleProvider(url_args):
      raise CommandException('"%s" command spanning providers not allowed.' %
                             self.command_name)

    # Open, read and parse file containing JSON document.
    lifecycle_file = open(lifecycle_arg, 'r')
    lifecycle_txt = lifecycle_file.read()
    lifecycle_file.close()

    # Iterate over URLs, expanding wildcards and setting the lifecycle on each.
    some_matched = False
    for url_str in url_args:
      bucket_iter = self.GetBucketUrlIterFromArg(url_str,
                                                 bucket_fields=['lifecycle'])
      for blr in bucket_iter:
        url = blr.storage_url
        some_matched = True
        self.logger.info('Setting lifecycle configuration on %s...', blr)
        if url.scheme == 's3':
          self.gsutil_api.XmlPassThroughSetLifecycle(
              lifecycle_txt, url, provider=url.scheme)
        else:
          lifecycle = LifecycleTranslation.JsonLifecycleToMessage(lifecycle_txt)
          bucket_metadata = apitools_messages.Bucket(lifecycle=lifecycle)
          self.gsutil_api.PatchBucket(url.bucket_name, bucket_metadata,
                                      provider=url.scheme, fields=['id'])
    if not some_matched:
      raise CommandException(NO_URLS_MATCHED_TARGET % list(url_args))
    return 0

  def _GetLifecycleConfig(self):
    """Gets lifecycle configuration for a Google Cloud Storage bucket."""
    bucket_url, bucket_metadata = self.GetSingleBucketUrlFromArg(
        self.args[0], bucket_fields=['lifecycle'])

    if bucket_url.scheme == 's3':
      sys.stdout.write(self.gsutil_api.XmlPassThroughGetLifecycle(
          bucket_url, provider=bucket_url.scheme))
    else:
      if bucket_metadata.lifecycle and bucket_metadata.lifecycle.rule:
        sys.stdout.write(LifecycleTranslation.JsonLifecycleFromMessage(
            bucket_metadata.lifecycle))
      else:
        sys.stdout.write('%s has no lifecycle configuration.\n' % bucket_url)

    return 0

  def RunCommand(self):
    """Command entry point for the lifecycle command."""
    subcommand = self.args.pop(0)
    if subcommand == 'get':
      metrics.LogCommandParams(subcommands=[subcommand])
      return self._GetLifecycleConfig()
    elif subcommand == 'set':
      metrics.LogCommandParams(subcommands=[subcommand])
      return self._SetLifecycleConfig()
    else:
      raise CommandException('Invalid subcommand "%s" for the %s command.' %
                             (subcommand, self.command_name))
