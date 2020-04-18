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
"""Implementation of Unix-like du command for cloud storage providers."""

from __future__ import absolute_import

import sys

from gslib.boto_translation import S3_DELETE_MARKER_GUID
from gslib.bucket_listing_ref import BucketListingObject
from gslib.command import Command
from gslib.command_argument import CommandArgument
from gslib.cs_api_map import ApiSelector
from gslib.exception import CommandException
from gslib.ls_helper import LsHelper
from gslib.storage_url import ContainsWildcard
from gslib.storage_url import StorageUrlFromString
from gslib.util import MakeHumanReadable
from gslib.util import NO_MAX
from gslib.util import UTF8

_SYNOPSIS = """
  gsutil du url...
"""

_DETAILED_HELP_TEXT = ("""
<B>SYNOPSIS</B>
""" + _SYNOPSIS + """


<B>DESCRIPTION</B>
  The du command displays the amount of space (in bytes) being used by the
  objects in the file or object hierarchy under a given URL. The syntax emulates
  the Linux du command (which stands for disk usage). For example, the command:

  gsutil du -s gs://your-bucket/dir

  will report the total space used by all objects under gs://your-bucket/dir and
  any sub-directories.


<B>OPTIONS</B>
  -0          Ends each output line with a 0 byte rather than a newline. This
              can be useful to make the output more easily machine-readable.

  -a          Includes non-current object versions / generations in the listing
              (only useful with a versioning-enabled bucket). Also prints
              generation and metageneration for each listed object.

  -c          Includes a grand total at the end of the output.

  -e          A pattern to exclude from reporting. Example: -e "*.o" would
              exclude any object that ends in ".o". Can be specified multiple
              times.

  -h          Prints object sizes in human-readable format (e.g., 1 KiB,
              234 MiB, 2GiB, etc.)

  -s          Displays only the grand total for each argument.

  -X          Similar to -e, but excludes patterns from the given file. The
              patterns to exclude should be one per line.


<B>EXAMPLES</B>
  To list the size of all objects in a bucket:

    gsutil du gs://bucketname

  To list the size of all objects underneath a prefix:

    gsutil du gs://bucketname/prefix/*

  To print the total number of bytes in a bucket, in human-readable form:

    gsutil du -ch gs://bucketname

  To see a summary of the total bytes in the two given buckets:

    gsutil du -s gs://bucket1 gs://bucket2

  To list the size of all objects in a versioned bucket, including objects that
  are not the latest:

    gsutil du -a gs://bucketname

  To list all objects in a bucket, except objects that end in ".bak",
  with each object printed ending in a null byte:

    gsutil du -e "*.bak" -0 gs://bucketname

  To get a total of all buckets in a project with a grand total for an entire
  project:

      gsutil -o GSUtil:default_project_id=project-name du -shc
""")


class DuCommand(Command):
  """Implementation of gsutil du command."""

  # Command specification. See base class for documentation.
  command_spec = Command.CreateCommandSpec(
      'du',
      command_name_aliases=[],
      usage_synopsis=_SYNOPSIS,
      min_args=0,
      max_args=NO_MAX,
      supported_sub_args='0ace:hsX:',
      file_url_ok=False,
      provider_url_ok=True,
      urls_start_arg=0,
      gs_api_support=[ApiSelector.XML, ApiSelector.JSON],
      gs_default_api=ApiSelector.JSON,
      argparse_arguments=[
          CommandArgument.MakeZeroOrMoreCloudURLsArgument()
      ]
  )
  # Help specification. See help_provider.py for documentation.
  help_spec = Command.HelpSpec(
      help_name='du',
      help_name_aliases=[],
      help_type='command_help',
      help_one_line_summary='Display object size usage',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )

  def _PrintSummaryLine(self, num_bytes, name):
    size_string = (MakeHumanReadable(num_bytes)
                   if self.human_readable else str(num_bytes))
    sys.stdout.write('%(size)-10s  %(name)s%(ending)s' % {
        'size': size_string, 'name': name, 'ending': self.line_ending})

  def _PrintInfoAboutBucketListingRef(self, bucket_listing_ref):
    """Print listing info for given bucket_listing_ref.

    Args:
      bucket_listing_ref: BucketListing being listed.

    Returns:
      Tuple (number of objects, object size)

    Raises:
      Exception: if calling bug encountered.
    """
    obj = bucket_listing_ref.root_object
    url_str = bucket_listing_ref.url_string
    if (obj.metadata and S3_DELETE_MARKER_GUID in
        obj.metadata.additionalProperties):
      size_string = '0'
      num_bytes = 0
      num_objs = 0
      url_str += '<DeleteMarker>'
    else:
      size_string = (MakeHumanReadable(obj.size)
                     if self.human_readable else str(obj.size))
      num_bytes = obj.size
      num_objs = 1

    if not self.summary_only:
      sys.stdout.write('%(size)-10s  %(url)s%(ending)s' % {
          'size': size_string,
          'url': url_str.encode(UTF8),
          'ending': self.line_ending})

    return (num_objs, num_bytes)

  def RunCommand(self):
    """Command entry point for the du command."""
    self.line_ending = '\n'
    self.all_versions = False
    self.produce_total = False
    self.human_readable = False
    self.summary_only = False
    self.exclude_patterns = []
    if self.sub_opts:
      for o, a in self.sub_opts:
        if o == '-0':
          self.line_ending = '\0'
        elif o == '-a':
          self.all_versions = True
        elif o == '-c':
          self.produce_total = True
        elif o == '-e':
          self.exclude_patterns.append(a)
        elif o == '-h':
          self.human_readable = True
        elif o == '-s':
          self.summary_only = True
        elif o == '-X':
          if a == '-':
            f = sys.stdin
          else:
            f = open(a, 'r')
          try:
            for line in f:
              line = line.strip().decode(UTF8)
              if line:
                self.exclude_patterns.append(line)
          finally:
            f.close()

    if not self.args:
      # Default to listing all gs buckets.
      self.args = ['gs://']

    total_bytes = 0
    got_nomatch_errors = False

    def _PrintObjectLong(blr):
      return self._PrintInfoAboutBucketListingRef(blr)

    def _PrintNothing(unused_blr=None):
      pass

    def _PrintDirectory(num_bytes, blr):
      if not self.summary_only:
        self._PrintSummaryLine(num_bytes, blr.url_string.encode(UTF8))

    for url_arg in self.args:
      top_level_storage_url = StorageUrlFromString(url_arg)
      if top_level_storage_url.IsFileUrl():
        raise CommandException('Only cloud URLs are supported for %s'
                               % self.command_name)
      bucket_listing_fields = ['size']

      ls_helper = LsHelper(
          self.WildcardIterator, self.logger,
          print_object_func=_PrintObjectLong, print_dir_func=_PrintNothing,
          print_dir_header_func=_PrintNothing,
          print_dir_summary_func=_PrintDirectory,
          print_newline_func=_PrintNothing, all_versions=self.all_versions,
          should_recurse=True, exclude_patterns=self.exclude_patterns,
          fields=bucket_listing_fields)

      # ls_helper expands to objects and prefixes, so perform a top-level
      # expansion first.
      if top_level_storage_url.IsProvider():
        # Provider URL: use bucket wildcard to iterate over all buckets.
        top_level_iter = self.WildcardIterator(
            '%s://*' % top_level_storage_url.scheme).IterBuckets(
                bucket_fields=['id'])
      elif top_level_storage_url.IsBucket():
        top_level_iter = self.WildcardIterator(
            '%s://%s' % (top_level_storage_url.scheme,
                         top_level_storage_url.bucket_name)).IterBuckets(
                             bucket_fields=['id'])
      else:
        top_level_iter = [BucketListingObject(top_level_storage_url)]

      for blr in top_level_iter:
        storage_url = blr.storage_url
        if storage_url.IsBucket() and self.summary_only:
          storage_url = StorageUrlFromString(
              storage_url.CreatePrefixUrl(wildcard_suffix='**'))
        _, exp_objs, exp_bytes = ls_helper.ExpandUrlAndPrint(storage_url)
        if (storage_url.IsObject() and exp_objs == 0 and
            ContainsWildcard(url_arg) and not self.exclude_patterns):
          got_nomatch_errors = True
        total_bytes += exp_bytes

        if self.summary_only:
          self._PrintSummaryLine(exp_bytes,
                                 blr.url_string.rstrip('/').encode(UTF8))

    if self.produce_total:
      self._PrintSummaryLine(total_bytes, 'total')

    if got_nomatch_errors:
      raise CommandException('One or more URLs matched no objects.')

    return 0
