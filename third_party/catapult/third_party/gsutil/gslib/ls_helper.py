# -*- coding: utf-8 -*-
# Copyright 2014 Google Inc. All Rights Reserved.
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
"""Utility functions and class for listing commands such as ls and du."""

from __future__ import absolute_import

import fnmatch

from gslib.cloud_api import EncryptionException
from gslib.exception import CommandException
from gslib.plurality_checkable_iterator import PluralityCheckableIterator
from gslib.util import IS_WINDOWS
from gslib.util import UTF8
from gslib.wildcard_iterator import StorageUrlFromString


ENCRYPTED_FIELDS = ['md5Hash', 'crc32c']
UNENCRYPTED_FULL_LISTING_FIELDS = [
    'acl', 'cacheControl', 'componentCount', 'contentDisposition',
    'contentEncoding', 'contentLanguage', 'contentType', 'kmsKeyName',
    'customerEncryption', 'etag', 'generation', 'metadata',
    'metageneration', 'size', 'storageClass', 'timeCreated', 'timeDeleted',
    'updated', 'timeStorageClassUpdated']


def PrintNewLine():
  """Default function for printing new lines between directories."""
  print


def PrintDirHeader(bucket_listing_ref):
  """Default function for printing headers for prefixes.

  Header is printed prior to listing the contents of the prefix.

  Args:
    bucket_listing_ref: BucketListingRef of type PREFIX.
  """
  print '%s:' % bucket_listing_ref.url_string.encode(UTF8)


def PrintBucketHeader(bucket_listing_ref):  # pylint: disable=unused-argument
  """Default function for printing headers for buckets.

  Header is printed prior to listing the contents of the bucket.

  Args:
    bucket_listing_ref: BucketListingRef of type BUCKET.
  """
  pass


def PrintDir(bucket_listing_ref):
  """Default function for printing buckets or prefixes.

  Args:
    bucket_listing_ref: BucketListingRef of type BUCKET or PREFIX.
  """
  print bucket_listing_ref.url_string.encode(UTF8)


# pylint: disable=unused-argument
def PrintDirSummary(num_bytes, bucket_listing_ref):
  """Off-by-default function for printing buckets or prefix size summaries.

  Args:
    num_bytes: Number of bytes contained in the directory.
    bucket_listing_ref: BucketListingRef of type BUCKET or PREFIX.
  """
  pass


def PrintObject(bucket_listing_ref):
  """Default printing function for objects.

  Args:
    bucket_listing_ref: BucketListingRef of type OBJECT.

  Returns:
    (num_objects, num_bytes).
  """
  try:
    print bucket_listing_ref.url_string.encode(UTF8)
  except IOError as e:
    # Windows throws an IOError 0 here for object names containing Unicode
    # chars. Ignore it.
    if not (IS_WINDOWS and e.errno == 0):
      raise
  return (1, 0)


class LsHelper(object):
  """Helper class for ls and du."""

  def __init__(self, iterator_func, logger,
               print_object_func=PrintObject,
               print_dir_func=PrintDir,
               print_dir_header_func=PrintDirHeader,
               print_bucket_header_func=PrintBucketHeader,
               print_dir_summary_func=PrintDirSummary,
               print_newline_func=PrintNewLine,
               all_versions=False, should_recurse=False,
               exclude_patterns=None, fields=('name',),
               list_subdir_contents=True):
    """Initializes the helper class to prepare for listing.

    Args:
      iterator_func: Function for instantiating iterator.
                     Inputs-
                       url_string- Url string to iterate on. May include
                                   wildcards.
                       all_versions=False- If true, iterate over all object
                                           versions.
      logger: Logger for outputting warnings / errors.
      print_object_func: Function for printing objects.
      print_dir_func:    Function for printing buckets/prefixes.
      print_dir_header_func: Function for printing header line for buckets
                             or prefixes.
      print_bucket_header_func: Function for printing header line for buckets
                                or prefixes.
      print_dir_summary_func: Function for printing size summaries about
                              buckets/prefixes.
      print_newline_func: Function for printing new lines between dirs.
      all_versions:      If true, list all object versions.
      should_recurse:    If true, recursively listing buckets/prefixes.
      exclude_patterns:  Patterns to exclude when listing.
      fields:            Fields to request from bucket listings; this should
                         include all fields that need to be populated in
                         objects so they can be listed. Can be set to None
                         to retrieve all object fields. Defaults to short
                         listing fields.
      list_subdir_contents: If true, return the directory and any contents,
                            otherwise return only the directory itself.
    """
    self._iterator_func = iterator_func
    self.logger = logger
    self._print_object_func = print_object_func
    self._print_dir_func = print_dir_func
    self._print_dir_header_func = print_dir_header_func
    self._print_bucket_header_func = print_bucket_header_func
    self._print_dir_summary_func = print_dir_summary_func
    self._print_newline_func = print_newline_func
    self.all_versions = all_versions
    self.should_recurse = should_recurse
    self.exclude_patterns = exclude_patterns
    self.bucket_listing_fields = fields
    self.list_subdir_contents = list_subdir_contents

  def ExpandUrlAndPrint(self, url):
    """Iterates over the given URL and calls print functions.

    Args:
      url: StorageUrl to iterate over.

    Returns:
      (num_objects, num_bytes) total number of objects and bytes iterated.
    """
    num_objects = 0
    num_dirs = 0
    num_bytes = 0
    print_newline = False

    if url.IsBucket() or self.should_recurse:
      # IsBucket() implies a top-level listing.
      if url.IsBucket():
        self._print_bucket_header_func(url)
      return self._RecurseExpandUrlAndPrint(url.url_string,
                                            print_initial_newline=False)
    else:
      # User provided a prefix or object URL, but it's impossible to tell
      # which until we do a listing and see what matches.
      top_level_iterator = PluralityCheckableIterator(self._iterator_func(
          url.CreatePrefixUrl(wildcard_suffix=None),
          all_versions=self.all_versions).IterAll(
              expand_top_level_buckets=True,
              bucket_listing_fields=self.bucket_listing_fields))
      plurality = top_level_iterator.HasPlurality()

      try:
        top_level_iterator.PeekException()
      except EncryptionException:
        # Detailed listing on a single object can perform a GetObjectMetadata
        # call, which raises if a matching encryption key isn't found.
        # Re-iterate without requesting encrypted fields.
        top_level_iterator = PluralityCheckableIterator(self._iterator_func(
            url.CreatePrefixUrl(wildcard_suffix=None),
            all_versions=self.all_versions).IterAll(
                expand_top_level_buckets=True,
                bucket_listing_fields=UNENCRYPTED_FULL_LISTING_FIELDS))
        plurality = top_level_iterator.HasPlurality()

      for blr in top_level_iterator:
        if self._MatchesExcludedPattern(blr):
          continue
        if blr.IsObject():
          nd = 0
          no, nb = self._print_object_func(blr)
          print_newline = True
        elif blr.IsPrefix():
          if print_newline:
            self._print_newline_func()
          else:
            print_newline = True
          if plurality and self.list_subdir_contents:
            self._print_dir_header_func(blr)
          elif plurality and not self.list_subdir_contents:
            print_newline = False
          expansion_url_str = StorageUrlFromString(
              blr.url_string).CreatePrefixUrl(
                  wildcard_suffix='*' if self.list_subdir_contents else None)
          nd, no, nb = self._RecurseExpandUrlAndPrint(expansion_url_str)
          self._print_dir_summary_func(nb, blr)
        else:
          # We handle all buckets at the top level, so this should never happen.
          raise CommandException(
              'Sub-level iterator returned a CsBucketListingRef of type Bucket')
        num_objects += no
        num_dirs += nd
        num_bytes += nb
      return num_dirs, num_objects, num_bytes

  def _RecurseExpandUrlAndPrint(self, url_str, print_initial_newline=True):
    """Iterates over the given URL string and calls print functions.

    Args:
      url_str: String describing StorageUrl to iterate over.
               Must be of depth one or higher.
      print_initial_newline: If true, print a newline before recursively
                             expanded prefixes.

    Returns:
      (num_objects, num_bytes) total number of objects and bytes iterated.
    """
    num_objects = 0
    num_dirs = 0
    num_bytes = 0
    for blr in self._iterator_func(
        '%s' % url_str, all_versions=self.all_versions).IterAll(
            expand_top_level_buckets=True,
            bucket_listing_fields=self.bucket_listing_fields):
      if self._MatchesExcludedPattern(blr):
        continue

      if blr.IsObject():
        nd = 0
        no, nb = self._print_object_func(blr)
      elif blr.IsPrefix():
        if self.should_recurse:
          if print_initial_newline:
            self._print_newline_func()
          else:
            print_initial_newline = True
          self._print_dir_header_func(blr)
          expansion_url_str = StorageUrlFromString(
              blr.url_string).CreatePrefixUrl(wildcard_suffix='*')

          nd, no, nb = self._RecurseExpandUrlAndPrint(expansion_url_str)
          self._print_dir_summary_func(nb, blr)
        else:
          nd, no, nb = 1, 0, 0
          self._print_dir_func(blr)
      else:
        # We handle all buckets at the top level, so this should never happen.
        raise CommandException(
            'Sub-level iterator returned a bucketListingRef of type Bucket')
      num_dirs += nd
      num_objects += no
      num_bytes += nb

    return num_dirs, num_objects, num_bytes

  def _MatchesExcludedPattern(self, blr):
    """Checks bucket listing reference against patterns to exclude.

    Args:
      blr: BucketListingRef to check.

    Returns:
      True if reference matches a pattern and should be excluded.
    """
    if self.exclude_patterns:
      tomatch = blr.url_string
      for pattern in self.exclude_patterns:
        if fnmatch.fnmatch(tomatch, pattern):
          return True
    return False
