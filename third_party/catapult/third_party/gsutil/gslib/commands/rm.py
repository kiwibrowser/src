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
"""Implementation of Unix-like rm command for cloud storage providers."""

from __future__ import absolute_import

import time

from gslib.cloud_api import BucketNotFoundException
from gslib.cloud_api import NotEmptyException
from gslib.cloud_api import NotFoundException
from gslib.cloud_api import ServiceException
from gslib.command import Command
from gslib.command import DecrementFailureCount
from gslib.command_argument import CommandArgument
from gslib.cs_api_map import ApiSelector
from gslib.exception import CommandException
from gslib.exception import NO_URLS_MATCHED_GENERIC
from gslib.exception import NO_URLS_MATCHED_TARGET
from gslib.name_expansion import NameExpansionIterator
from gslib.name_expansion import SeekAheadNameExpansionIterator
from gslib.parallelism_framework_util import PutToQueueWithTimeout
from gslib.storage_url import StorageUrlFromString
from gslib.thread_message import MetadataMessage
from gslib.translation_helper import PreconditionsFromHeaders
from gslib.util import GetCloudApiInstance
from gslib.util import NO_MAX
from gslib.util import Retry
from gslib.util import StdinIterator


_SYNOPSIS = """
  gsutil rm [-f] [-r] url...
  gsutil rm [-f] [-r] -I
"""

_DETAILED_HELP_TEXT = ("""
<B>SYNOPSIS</B>
""" + _SYNOPSIS + """


<B>DESCRIPTION</B>
  The gsutil rm command removes objects.
  For example, the command:

    gsutil rm gs://bucket/subdir/*

  will remove all objects in gs://bucket/subdir, but not in any of its
  sub-directories. In contrast:

    gsutil rm gs://bucket/subdir/**

  will remove all objects under gs://bucket/subdir or any of its
  subdirectories.

  You can also use the -r option to specify recursive object deletion. Thus, for
  example, either of the following two commands will remove gs://bucket/subdir
  and all objects and subdirectories under it:

    gsutil rm gs://bucket/subdir**
    gsutil rm -r gs://bucket/subdir

  The -r option will also delete all object versions in the subdirectory for
  versioning-enabled buckets, whereas the ** command will only delete the live
  version of each object in the subdirectory.

  Running gsutil rm -r on a bucket will delete all versions of all objects in
  the bucket, and then delete the bucket:

    gsutil rm -r gs://bucket

  If you want to delete all objects in the bucket, but not the bucket itself,
  this command will work:

    gsutil rm gs://bucket/**

  If you have a large number of objects to remove you might want to use the
  gsutil -m option, to perform parallel (multi-threaded/multi-processing)
  removes:

    gsutil -m rm -r gs://my_bucket/subdir

  You can pass a list of URLs (one per line) to remove on stdin instead of as
  command line arguments by using the -I option. This allows you to use gsutil
  in a pipeline to remove objects identified by a program, such as:

    some_program | gsutil -m rm -I

  The contents of stdin can name cloud URLs and wildcards of cloud URLs.

  Note that gsutil rm will refuse to remove files from the local
  file system. For example this will fail:

    gsutil rm *.txt

  WARNING: Object removal cannot be undone. Google Cloud Storage is designed
  to give developers a high amount of flexibility and control over their data,
  and Google maintains strict controls over the processing and purging of
  deleted data. To protect yourself from mistakes, you can configure object
  versioning on your bucket(s). See 'gsutil help versions' for details.


<B>DATA RESTORATION FROM ACCIDENTAL DELETION OR OVERWRITES</B>
Google Cloud Storage does not provide support for restoring data lost
or overwritten due to customer errors. If you have concerns that your
application software (or your users) may at some point erroneously delete or
overwrite data, you can protect yourself from that risk by enabling Object
Versioning (see "gsutil help versioning"). Doing so increases storage costs,
which can be partially mitigated by configuring Lifecycle Management to delete
older object versions (see "gsutil help lifecycle").


<B>OPTIONS</B>
  -f          Continues silently (without printing error messages) despite
              errors when removing multiple objects. If some of the objects
              could not be removed, gsutil's exit status will be non-zero even
              if this flag is set. Execution will still halt if an inaccessible
              bucket is encountered. This option is implicitly set when running
              "gsutil -m rm ...".

  -I          Causes gsutil to read the list of objects to remove from stdin.
              This allows you to run a program that generates the list of
              objects to remove.

  -R, -r      The -R and -r options are synonymous. Causes bucket or bucket
              subdirectory contents (all objects and subdirectories that it
              contains) to be removed recursively. If used with a bucket-only
              URL (like gs://bucket), after deleting objects and subdirectories
              gsutil will delete the bucket. This option implies the -a option
              and will delete all object versions.

  -a          Delete all versions of an object.
""")


def _RemoveExceptionHandler(cls, e):
  """Simple exception handler to allow post-completion status."""
  if not cls.continue_on_error:
    cls.logger.error(str(e))
  # TODO: Use shared state to track missing bucket names when we get a
  # BucketNotFoundException. Then improve bucket removal logic and exception
  # messages.
  if isinstance(e, BucketNotFoundException):
    cls.bucket_not_found_count += 1
    cls.logger.error(str(e))
  else:
    if _ExceptionMatchesBucketToDelete(cls.bucket_strings_to_delete, e):
      DecrementFailureCount()
    else:
      cls.op_failure_count += 1


# pylint: disable=unused-argument
def _RemoveFoldersExceptionHandler(cls, e):
  """When removing folders, we don't mind if none exist."""
  if ((isinstance(e, CommandException) and
       NO_URLS_MATCHED_GENERIC in e.reason)
      or isinstance(e, NotFoundException)):
    DecrementFailureCount()
  else:
    raise e


def _RemoveFuncWrapper(cls, name_expansion_result, thread_state=None):
  cls.RemoveFunc(name_expansion_result, thread_state=thread_state)


def _ExceptionMatchesBucketToDelete(bucket_strings_to_delete, e):
  """Returns True if the exception matches a bucket slated for deletion.

  A recursive delete call on an empty bucket will raise an exception when
  listing its objects, but if we plan to delete the bucket that shouldn't
  result in a user-visible error.

  Args:
    bucket_strings_to_delete: Buckets slated for recursive deletion.
    e: Exception to check.

  Returns:
    True if the exception was a no-URLs-matched exception and it matched
    one of bucket_strings_to_delete, None otherwise.
  """
  if bucket_strings_to_delete:
    msg = NO_URLS_MATCHED_TARGET % ''
    if msg in str(e):
      parts = str(e).split(msg)
      return len(parts) == 2 and parts[1] in bucket_strings_to_delete


class RmCommand(Command):
  """Implementation of gsutil rm command."""

  # Command specification. See base class for documentation.
  command_spec = Command.CreateCommandSpec(
      'rm',
      command_name_aliases=['del', 'delete', 'remove'],
      usage_synopsis=_SYNOPSIS,
      min_args=0,
      max_args=NO_MAX,
      supported_sub_args='afIrR',
      file_url_ok=False,
      provider_url_ok=False,
      urls_start_arg=0,
      gs_api_support=[ApiSelector.XML, ApiSelector.JSON],
      gs_default_api=ApiSelector.JSON,
      argparse_arguments=[
          CommandArgument.MakeZeroOrMoreCloudURLsArgument()
      ]
  )
  # Help specification. See help_provider.py for documentation.
  help_spec = Command.HelpSpec(
      help_name='rm',
      help_name_aliases=['del', 'delete', 'remove'],
      help_type='command_help',
      help_one_line_summary='Remove objects',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )

  def RunCommand(self):
    """Command entry point for the rm command."""
    # self.recursion_requested is initialized in command.py (so it can be
    # checked in parent class for all commands).
    self.continue_on_error = self.parallel_operations
    self.read_args_from_stdin = False
    self.all_versions = False
    if self.sub_opts:
      for o, unused_a in self.sub_opts:
        if o == '-a':
          self.all_versions = True
        elif o == '-f':
          self.continue_on_error = True
        elif o == '-I':
          self.read_args_from_stdin = True
        elif o == '-r' or o == '-R':
          self.recursion_requested = True
          self.all_versions = True

    if self.read_args_from_stdin:
      if self.args:
        raise CommandException('No arguments allowed with the -I flag.')
      url_strs = StdinIterator()
    else:
      if not self.args:
        raise CommandException('The rm command (without -I) expects at '
                               'least one URL.')
      url_strs = self.args

    # Tracks number of object deletes that failed.
    self.op_failure_count = 0

    # Tracks if any buckets were missing.
    self.bucket_not_found_count = 0

    # Tracks buckets that are slated for recursive deletion.
    bucket_urls_to_delete = []
    self.bucket_strings_to_delete = []

    if self.recursion_requested:
      bucket_fields = ['id']
      for url_str in url_strs:
        url = StorageUrlFromString(url_str)
        if url.IsBucket() or url.IsProvider():
          for blr in self.WildcardIterator(url_str).IterBuckets(
              bucket_fields=bucket_fields):
            bucket_urls_to_delete.append(blr.storage_url)
            self.bucket_strings_to_delete.append(url_str)

    self.preconditions = PreconditionsFromHeaders(self.headers or {})

    try:
      # Expand wildcards, dirs, buckets, and bucket subdirs in URLs.
      name_expansion_iterator = NameExpansionIterator(
          self.command_name, self.debug, self.logger, self.gsutil_api,
          url_strs, self.recursion_requested, project_id=self.project_id,
          all_versions=self.all_versions,
          continue_on_error=self.continue_on_error or self.parallel_operations)

      seek_ahead_iterator = None
      # Cannot seek ahead with stdin args, since we can only iterate them
      # once without buffering in memory.
      if not self.read_args_from_stdin:
        seek_ahead_iterator = SeekAheadNameExpansionIterator(
            self.command_name, self.debug, self.GetSeekAheadGsutilApi(),
            url_strs, self.recursion_requested,
            all_versions=self.all_versions, project_id=self.project_id)

      # Perform remove requests in parallel (-m) mode, if requested, using
      # configured number of parallel processes and threads. Otherwise,
      # perform requests with sequential function calls in current process.
      self.Apply(_RemoveFuncWrapper, name_expansion_iterator,
                 _RemoveExceptionHandler,
                 fail_on_error=(not self.continue_on_error),
                 shared_attrs=['op_failure_count', 'bucket_not_found_count'],
                 seek_ahead_iterator=seek_ahead_iterator)

    # Assuming the bucket has versioning enabled, url's that don't map to
    # objects should throw an error even with all_versions, since the prior
    # round of deletes only sends objects to a history table.
    # This assumption that rm -a is only called for versioned buckets should be
    # corrected, but the fix is non-trivial.
    except CommandException as e:
      # Don't raise if there are buckets to delete -- it's valid to say:
      #   gsutil rm -r gs://some_bucket
      # if the bucket is empty.
      if _ExceptionMatchesBucketToDelete(self.bucket_strings_to_delete, e):
        DecrementFailureCount()
      else:
        raise
    except ServiceException, e:
      if not self.continue_on_error:
        raise

    if self.bucket_not_found_count:
      raise CommandException('Encountered non-existent bucket during listing')

    if self.op_failure_count and not self.continue_on_error:
      raise CommandException('Some files could not be removed.')

    # If this was a gsutil rm -r command covering any bucket subdirs,
    # remove any dir_$folder$ objects (which are created by various web UI
    # tools to simulate folders).
    if self.recursion_requested:
      folder_object_wildcards = []
      for url_str in url_strs:
        url = StorageUrlFromString(url_str)
        if url.IsObject():
          folder_object_wildcards.append('%s**_$folder$' % url_str)
      if folder_object_wildcards:
        self.continue_on_error = True
        try:
          name_expansion_iterator = NameExpansionIterator(
              self.command_name, self.debug,
              self.logger, self.gsutil_api,
              folder_object_wildcards, self.recursion_requested,
              project_id=self.project_id,
              all_versions=self.all_versions)
          # When we're removing folder objects, always continue on error
          self.Apply(_RemoveFuncWrapper, name_expansion_iterator,
                     _RemoveFoldersExceptionHandler,
                     fail_on_error=False)
        except CommandException as e:
          # Ignore exception from name expansion due to an absent folder file.
          if not e.reason.startswith(NO_URLS_MATCHED_GENERIC):
            raise

    # Now that all data has been deleted, delete any bucket URLs.
    for url in bucket_urls_to_delete:
      self.logger.info('Removing %s...', url)

      @Retry(NotEmptyException, tries=3, timeout_secs=1)
      def BucketDeleteWithRetry():
        self.gsutil_api.DeleteBucket(url.bucket_name, provider=url.scheme)

      BucketDeleteWithRetry()

    if self.op_failure_count:
      plural_str = 's' if self.op_failure_count else ''
      raise CommandException('%d file%s/object%s could not be removed.' % (
          self.op_failure_count, plural_str, plural_str))

    return 0

  def RemoveFunc(self, name_expansion_result, thread_state=None):
    gsutil_api = GetCloudApiInstance(self, thread_state=thread_state)

    exp_src_url = name_expansion_result.expanded_storage_url
    self.logger.info('Removing %s...', exp_src_url)
    gsutil_api.DeleteObject(
        exp_src_url.bucket_name, exp_src_url.object_name,
        preconditions=self.preconditions, generation=exp_src_url.generation,
        provider=exp_src_url.scheme)
    PutToQueueWithTimeout(gsutil_api.status_queue,
                          MetadataMessage(message_time=time.time()))
