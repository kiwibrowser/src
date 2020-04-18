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
"""Implementation of compose command for Google Cloud Storage."""

from __future__ import absolute_import

from boto import config

from gslib.bucket_listing_ref import BucketListingObject
from gslib.command import Command
from gslib.command_argument import CommandArgument
from gslib.cs_api_map import ApiSelector
from gslib.encryption_helper import GetEncryptionKeyWrapper
from gslib.exception import CommandException
from gslib.storage_url import ContainsWildcard
from gslib.storage_url import StorageUrlFromString
from gslib.third_party.storage_apitools import storage_v1_messages as apitools_messages
from gslib.translation_helper import PreconditionsFromHeaders

MAX_COMPONENT_COUNT = 1024
MAX_COMPOSE_ARITY = 32
MAX_COMPONENT_RATE = 200

_SYNOPSIS = """
  gsutil compose gs://bucket/obj1 [gs://bucket/obj2 ...] gs://bucket/composite
"""

_DETAILED_HELP_TEXT = ("""
<B>SYNOPSIS</B>
""" + _SYNOPSIS + """


<B>DESCRIPTION</B>
  The compose command creates a new object whose content is the concatenation
  of a given sequence of component objects under the same bucket. gsutil uses
  the content type of the first source object to determine the destination
  object's content type. For more information, please see:
  https://cloud.google.com/storage/docs/composite-objects

  Note also that the gsutil cp command can automatically split uploads for
  large files into multiple component objects, upload them in parallel, and
  compose them into a final object (which will be subject to the component
  count limit). This will still perform all uploads from a single machine. For
  extremely large files and/or very low per-machine bandwidth, you may want to
  split the file and upload it from multiple machines, and later compose these
  parts of the file manually. See the 'PARALLEL COMPOSITE UPLOADS' section under
  'gsutil help cp' for details.

  Appending simply entails uploading your new data to a temporary object,
  composing it with the growing append-target, and deleting the temporary
  object:

    $ echo 'new data' | gsutil cp - gs://bucket/data-to-append
    $ gsutil compose gs://bucket/append-target gs://bucket/data-to-append \\
        gs://bucket/append-target
    $ gsutil rm gs://bucket/data-to-append

  Note that there is a limit (currently %d) to the number of components that can
  be composed in a single operation.

  There is a limit (currently %d) to the total number of components
  for a given composite object. This means you can append to each object at most
  %d times.

  There is a per-project rate limit (currently %d) to the number of components
  you can compose per second. This rate counts both the components being
  appended to a composite object as well as the components being copied when
  the composite object of which they are a part is copied.
""" % (MAX_COMPOSE_ARITY, MAX_COMPONENT_COUNT, MAX_COMPONENT_COUNT - 1,
       MAX_COMPONENT_RATE))


class ComposeCommand(Command):
  """Implementation of gsutil compose command."""

  # Command specification. See base class for documentation.
  command_spec = Command.CreateCommandSpec(
      'compose',
      command_name_aliases=['concat'],
      usage_synopsis=_SYNOPSIS,
      min_args=1,
      max_args=MAX_COMPOSE_ARITY + 1,
      supported_sub_args='',
      # Not files, just object names without gs:// prefix.
      file_url_ok=False,
      provider_url_ok=False,
      urls_start_arg=1,
      gs_api_support=[ApiSelector.XML, ApiSelector.JSON],
      gs_default_api=ApiSelector.JSON,
      argparse_arguments=[
          CommandArgument.MakeZeroOrMoreCloudURLsArgument()
      ]
  )
  # Help specification. See help_provider.py for documentation.
  help_spec = Command.HelpSpec(
      help_name='compose',
      help_name_aliases=['concat'],
      help_type='command_help',
      help_one_line_summary=(
          'Concatenate a sequence of objects into a new composite object.'),
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )

  def CheckProvider(self, url):
    if url.scheme != 'gs':
      raise CommandException(
          '"compose" called on URL with unsupported provider (%s).' % str(url))

  # Command entry point.
  def RunCommand(self):
    """Command entry point for the compose command."""
    target_url_str = self.args[-1]
    self.args = self.args[:-1]
    target_url = StorageUrlFromString(target_url_str)
    self.CheckProvider(target_url)
    if target_url.HasGeneration():
      raise CommandException('A version-specific URL (%s) cannot be '
                             'the destination for gsutil compose - abort.'
                             % target_url)

    dst_obj_metadata = apitools_messages.Object(name=target_url.object_name,
                                                bucket=target_url.bucket_name)

    components = []
    # Remember the first source object so we can get its content type.
    first_src_url = None
    for src_url_str in self.args:
      if ContainsWildcard(src_url_str):
        src_url_iter = self.WildcardIterator(src_url_str).IterObjects()
      else:
        src_url_iter = [BucketListingObject(StorageUrlFromString(src_url_str))]
      for blr in src_url_iter:
        src_url = blr.storage_url
        self.CheckProvider(src_url)

        if src_url.bucket_name != target_url.bucket_name:
          raise CommandException(
              'GCS does not support inter-bucket composing.')

        if not first_src_url:
          first_src_url = src_url
        src_obj_metadata = (
            apitools_messages.ComposeRequest.SourceObjectsValueListEntry(
                name=src_url.object_name))
        if src_url.HasGeneration():
          src_obj_metadata.generation = src_url.generation
        components.append(src_obj_metadata)
        # Avoid expanding too many components, and sanity check each name
        # expansion result.
        if len(components) > MAX_COMPOSE_ARITY:
          raise CommandException('"compose" called with too many component '
                                 'objects. Limit is %d.' % MAX_COMPOSE_ARITY)

    if not components:
      raise CommandException('"compose" requires at least 1 component object.')

    dst_obj_metadata.contentType = self.gsutil_api.GetObjectMetadata(
        first_src_url.bucket_name, first_src_url.object_name,
        provider=first_src_url.scheme, fields=['contentType']).contentType

    preconditions = PreconditionsFromHeaders(self.headers or {})

    self.logger.info(
        'Composing %s from %d component object(s).',
        target_url, len(components))
    self.gsutil_api.ComposeObject(
        components, dst_obj_metadata, preconditions=preconditions,
        provider=target_url.scheme,
        encryption_tuple=GetEncryptionKeyWrapper(config))
