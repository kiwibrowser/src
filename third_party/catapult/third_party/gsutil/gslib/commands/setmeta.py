# -*- coding: utf-8 -*-
# Copyright 2012 Google Inc. All Rights Reserved.
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
"""Implementation of setmeta command for setting cloud object metadata."""

from __future__ import absolute_import

import time

from apitools.base.py import encoding
from gslib.cloud_api import AccessDeniedException
from gslib.cloud_api import PreconditionException
from gslib.cloud_api import Preconditions
from gslib.command import Command
from gslib.command_argument import CommandArgument
from gslib.cs_api_map import ApiSelector
from gslib.exception import CommandException
from gslib.name_expansion import NameExpansionIterator
from gslib.name_expansion import SeekAheadNameExpansionIterator
from gslib.parallelism_framework_util import PutToQueueWithTimeout
from gslib.storage_url import StorageUrlFromString
from gslib.third_party.storage_apitools import storage_v1_messages as apitools_messages
from gslib.thread_message import MetadataMessage
from gslib.translation_helper import CopyObjectMetadata
from gslib.translation_helper import ObjectMetadataFromHeaders
from gslib.translation_helper import PreconditionsFromHeaders
from gslib.util import GetCloudApiInstance
from gslib.util import InsistAsciiHeader
from gslib.util import InsistAsciiHeaderValue
from gslib.util import IsCustomMetadataHeader
from gslib.util import NO_MAX
from gslib.util import Retry

_SYNOPSIS = """
  gsutil setmeta -h [header:value|header] ... url...
"""

_DETAILED_HELP_TEXT = ("""
<B>SYNOPSIS</B>
""" + _SYNOPSIS + """


<B>DESCRIPTION</B>
  The gsutil setmeta command allows you to set or remove the metadata on one
  or more objects. It takes one or more header arguments followed by one or
  more URLs, where each header argument is in one of two forms:

  - if you specify header:value, it will set the given header on all
    named objects.

  - if you specify header (with no value), it will remove the given header
    from all named objects.

  For example, the following command would set the Content-Type and
  Cache-Control and remove the Content-Disposition on the specified objects:

    gsutil setmeta -h "Content-Type:text/html" \\
      -h "Cache-Control:public, max-age=3600" \\
      -h "Content-Disposition" gs://bucket/*.html

  If you have a large number of objects to update you might want to use the
  gsutil -m option, to perform a parallel (multi-threaded/multi-processing)
  update:

    gsutil -m setmeta -h "Content-Type:text/html" \\
      -h "Cache-Control:public, max-age=3600" \\
      -h "Content-Disposition" gs://bucket/*.html

  You can also use the setmeta command to set custom metadata on an object:

    gsutil setmeta -h "x-goog-meta-icecreamflavor:vanilla" gs://bucket/object

  See "gsutil help metadata" for details about how you can set metadata
  while uploading objects, what metadata fields can be set and the meaning of
  these fields, use of custom metadata, and how to view currently set metadata.

  NOTE: By default, publicly readable objects are served with a Cache-Control
  header allowing such objects to be cached for 3600 seconds. For more details
  about this default behavior see the CACHE-CONTROL section of
  "gsutil help metadata". If you need to ensure that updates become visible
  immediately, you should set a Cache-Control header of "Cache-Control:private,
  max-age=0, no-transform" on such objects.  You can do this with the command:

    gsutil setmeta -h "Content-Type:text/html" \\
      -h "Cache-Control:private, max-age=0, no-transform" gs://bucket/*.html

  The setmeta command reads each object's current generation and metageneration
  and uses those as preconditions unless they are otherwise specified by
  top-level arguments. For example:

    gsutil -h "x-goog-if-metageneration-match:2" setmeta
      -h "x-goog-meta-icecreamflavor:vanilla"

  will set the icecreamflavor:vanilla metadata if the current live object has a
  metageneration of 2.

<B>OPTIONS</B>
  -h          Specifies a header:value to be added, or header to be removed,
              from each named object.
""")

# Setmeta assumes a header-like model which doesn't line up with the JSON way
# of doing things. This list comes from functionality that was supported by
# gsutil3 at the time gsutil4 was released.
SETTABLE_FIELDS = ['cache-control', 'content-disposition',
                   'content-encoding', 'content-language',
                   'content-type']


def _SetMetadataExceptionHandler(cls, e):
  """Exception handler that maintains state about post-completion status."""
  cls.logger.error(e)
  cls.everything_set_okay = False


def _SetMetadataFuncWrapper(cls, name_expansion_result, thread_state=None):
  cls.SetMetadataFunc(name_expansion_result, thread_state=thread_state)


class SetMetaCommand(Command):
  """Implementation of gsutil setmeta command."""

  # Command specification. See base class for documentation.
  command_spec = Command.CreateCommandSpec(
      'setmeta',
      command_name_aliases=['setheader'],
      usage_synopsis=_SYNOPSIS,
      min_args=1,
      max_args=NO_MAX,
      supported_sub_args='h:rR',
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
      help_name='setmeta',
      help_name_aliases=['setheader'],
      help_type='command_help',
      help_one_line_summary='Set metadata on already uploaded objects',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )

  def RunCommand(self):
    """Command entry point for the setmeta command."""
    headers = []
    if self.sub_opts:
      for o, a in self.sub_opts:
        if o == '-h':
          if 'x-goog-acl' in a or 'x-amz-acl' in a:
            raise CommandException(
                'gsutil setmeta no longer allows canned ACLs. Use gsutil acl '
                'set ... to set canned ACLs.')
          headers.append(a)

    (metadata_minus, metadata_plus) = self._ParseMetadataHeaders(headers)

    self.metadata_change = metadata_plus
    for header in metadata_minus:
      self.metadata_change[header] = ''

    if len(self.args) == 1 and not self.recursion_requested:
      url = StorageUrlFromString(self.args[0])
      if not (url.IsCloudUrl() and url.IsObject()):
        raise CommandException('URL (%s) must name an object' % self.args[0])

    # Used to track if any objects' metadata failed to be set.
    self.everything_set_okay = True

    self.preconditions = PreconditionsFromHeaders(self.headers)

    name_expansion_iterator = NameExpansionIterator(
        self.command_name, self.debug, self.logger, self.gsutil_api,
        self.args, self.recursion_requested, all_versions=self.all_versions,
        continue_on_error=self.parallel_operations,
        bucket_listing_fields=['generation', 'metadata', 'metageneration'])

    seek_ahead_iterator = SeekAheadNameExpansionIterator(
        self.command_name, self.debug, self.GetSeekAheadGsutilApi(),
        self.args, self.recursion_requested,
        all_versions=self.all_versions, project_id=self.project_id)

    try:
      # Perform requests in parallel (-m) mode, if requested, using
      # configured number of parallel processes and threads. Otherwise,
      # perform requests with sequential function calls in current process.
      self.Apply(_SetMetadataFuncWrapper, name_expansion_iterator,
                 _SetMetadataExceptionHandler, fail_on_error=True,
                 seek_ahead_iterator=seek_ahead_iterator)
    except AccessDeniedException as e:
      if e.status == 403:
        self._WarnServiceAccounts()
      raise

    if not self.everything_set_okay:
      raise CommandException('Metadata for some objects could not be set.')

    return 0

  @Retry(PreconditionException, tries=3, timeout_secs=1)
  def SetMetadataFunc(self, name_expansion_result, thread_state=None):
    """Sets metadata on an object.

    Args:
      name_expansion_result: NameExpansionResult describing target object.
      thread_state: gsutil Cloud API instance to use for the operation.
    """
    gsutil_api = GetCloudApiInstance(self, thread_state=thread_state)

    exp_src_url = name_expansion_result.expanded_storage_url
    self.logger.info('Setting metadata on %s...', exp_src_url)

    cloud_obj_metadata = encoding.JsonToMessage(
        apitools_messages.Object, name_expansion_result.expanded_result)

    preconditions = Preconditions(
        gen_match=self.preconditions.gen_match,
        meta_gen_match=self.preconditions.meta_gen_match)
    if preconditions.gen_match is None:
      preconditions.gen_match = cloud_obj_metadata.generation
    if preconditions.meta_gen_match is None:
      preconditions.meta_gen_match = cloud_obj_metadata.metageneration

    # Patch handles the patch semantics for most metadata, but we need to
    # merge the custom metadata field manually.
    patch_obj_metadata = ObjectMetadataFromHeaders(self.metadata_change)

    api = gsutil_api.GetApiSelector(provider=exp_src_url.scheme)
    # For XML we only want to patch through custom metadata that has
    # changed.  For JSON we need to build the complete set.
    if api == ApiSelector.XML:
      pass
    elif api == ApiSelector.JSON:
      CopyObjectMetadata(patch_obj_metadata, cloud_obj_metadata,
                         override=True)
      patch_obj_metadata = cloud_obj_metadata
      # Patch body does not need the object generation and metageneration.
      patch_obj_metadata.generation = None
      patch_obj_metadata.metageneration = None

    gsutil_api.PatchObjectMetadata(
        exp_src_url.bucket_name, exp_src_url.object_name, patch_obj_metadata,
        generation=exp_src_url.generation, preconditions=preconditions,
        provider=exp_src_url.scheme, fields=['id'])
    PutToQueueWithTimeout(gsutil_api.status_queue,
                          MetadataMessage(message_time=time.time()))

  def _ParseMetadataHeaders(self, headers):
    """Validates and parses metadata changes from the headers argument.

    Args:
      headers: Header dict to validate and parse.

    Returns:
      (metadata_plus, metadata_minus): Tuple of header sets to add and remove.
    """
    metadata_minus = set()
    cust_metadata_minus = set()
    metadata_plus = {}
    cust_metadata_plus = {}
    # Build a count of the keys encountered from each plus and minus arg so we
    # can check for dupe field specs.
    num_metadata_plus_elems = 0
    num_cust_metadata_plus_elems = 0
    num_metadata_minus_elems = 0
    num_cust_metadata_minus_elems = 0

    for md_arg in headers:
      # Use partition rather than split, as we should treat all characters past
      # the initial : as part of the header's value.
      parts = md_arg.partition(':')
      (header, _, value) = parts
      InsistAsciiHeader(header)

      # Translate headers to lowercase to match the casing assumed by our
      # sanity-checking operations.
      lowercase_header = header.lower()
      # This check is overly simple; it would be stronger to check, for each
      # URL argument, whether the header starts with the provider
      # metadata_prefix, but here we just parse the spec once, before
      # processing any of the URLs. This means we will not detect if the user
      # tries to set an x-goog-meta- field on an another provider's object,
      # for example.
      is_custom_meta = IsCustomMetadataHeader(lowercase_header)
      if not is_custom_meta and lowercase_header not in SETTABLE_FIELDS:
        raise CommandException(
            'Invalid or disallowed header (%s).\nOnly these fields (plus '
            'x-goog-meta-* fields) can be set or unset:\n%s' % (
                header, sorted(list(SETTABLE_FIELDS))))

      if value:
        if is_custom_meta:
          # Allow non-ASCII data for custom metadata fields.
          cust_metadata_plus[header] = value
          num_cust_metadata_plus_elems += 1
        else:
          # Don't unicode encode other fields because that would perturb their
          # content (e.g., adding %2F's into the middle of a Cache-Control
          # value).
          InsistAsciiHeaderValue(header, value)
          value = str(value)
          metadata_plus[lowercase_header] = value
          num_metadata_plus_elems += 1
      else:
        if is_custom_meta:
          cust_metadata_minus.add(header)
          num_cust_metadata_minus_elems += 1
        else:
          metadata_minus.add(lowercase_header)
          num_metadata_minus_elems += 1

    if (num_metadata_plus_elems != len(metadata_plus)
        or num_cust_metadata_plus_elems != len(cust_metadata_plus)
        or num_metadata_minus_elems != len(metadata_minus)
        or num_cust_metadata_minus_elems != len(cust_metadata_minus)
        or metadata_minus.intersection(set(metadata_plus.keys()))):
      raise CommandException('Each header must appear at most once.')

    metadata_plus.update(cust_metadata_plus)
    metadata_minus.update(cust_metadata_minus)
    return (metadata_minus, metadata_plus)

