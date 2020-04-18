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
"""File and Cloud URL representation classes."""

from __future__ import absolute_import

import os
import re
import stat

from gslib.exception import InvalidUrlError

# Matches provider strings of the form 'gs://'
PROVIDER_REGEX = re.compile(r'(?P<provider>[^:]*)://$')
# Matches bucket strings of the form 'gs://bucket'
BUCKET_REGEX = re.compile(r'(?P<provider>[^:]*)://(?P<bucket>[^/]*)/{0,1}$')
# Matches object strings of the form 'gs://bucket/obj'
OBJECT_REGEX = re.compile(
    r'(?P<provider>[^:]*)://(?P<bucket>[^/]*)/(?P<object>.*)')
# Matches versioned object strings of the form 'gs://bucket/obj#1234'
GS_GENERATION_REGEX = re.compile(r'(?P<object>.+)#(?P<generation>[0-9]+)$')
# Matches versioned object strings of the form 's3://bucket/obj#NULL'
S3_VERSION_REGEX = re.compile(r'(?P<object>.+)#(?P<version_id>.+)$')
# Matches file strings of the form 'file://dir/filename'
FILE_OBJECT_REGEX = re.compile(r'([^:]*://)(?P<filepath>.*)')
# Regex to determine if a string contains any wildcards.
WILDCARD_REGEX = re.compile(r'[*?\[\]]')


class StorageUrl(object):
  """Abstract base class for file and Cloud Storage URLs."""

  def Clone(self):
    raise NotImplementedError('Clone not overridden')

  def IsFileUrl(self):
    raise NotImplementedError('IsFileUrl not overridden')

  def IsCloudUrl(self):
    raise NotImplementedError('IsCloudUrl not overridden')

  def IsStream():
    raise NotImplementedError('IsStream not overridden')

  def IsFifo(self):
    raise NotImplementedError('IsFifo not overridden')

  def CreatePrefixUrl(self, wildcard_suffix=None):
    """Returns a prefix of this URL that can be used for iterating.

    Args:
      wildcard_suffix: If supplied, this wildcard suffix will be appended to the
                       prefix with a trailing slash before being returned.

    Returns:
      A prefix of this URL that can be used for iterating.

    If this URL contains a trailing slash, it will be stripped to create the
    prefix. This helps avoid infinite looping when prefixes are iterated, but
    preserves other slashes so that objects with '/' in the name are handled
    properly.

    For example, when recursively listing a bucket with the following contents:
      gs://bucket// <-- object named slash
      gs://bucket//one-dir-deep
    a top-level expansion with '/' as a delimiter will result in the following
    URL strings:
      'gs://bucket//' : OBJECT
      'gs://bucket//' : PREFIX
    If we right-strip all slashes from the prefix entry and add a wildcard
    suffix, we will get 'gs://bucket/*' which will produce identical results
    (and infinitely recurse).

    Example return values:
      ('gs://bucket/subdir/', '*') becomes 'gs://bucket/subdir/*'
      ('gs://bucket/', '*') becomes 'gs://bucket/*'
      ('gs://bucket/', None) becomes 'gs://bucket'
      ('gs://bucket/subdir//', '*') becomes 'gs://bucket/subdir//*'
      ('gs://bucket/subdir///', '**') becomes 'gs://bucket/subdir///**'
      ('gs://bucket/subdir/', '*') where 'subdir/' is an object becomes
           'gs://bucket/subdir/*', but iterating on this will return 'subdir/'
           as a BucketListingObject, so we will not recurse on it as a subdir
           during listing.
    """
    raise NotImplementedError('CreatePrefixUrl not overridden')

  @property
  def url_string(self):
    raise NotImplementedError('url_string not overridden')

  @property
  def versionless_url_string(self):
    raise NotImplementedError('versionless_url_string not overridden')

  def __eq__(self, other):
    return isinstance(other, StorageUrl) and self.url_string == other.url_string

  def __hash__(self):
    return hash(self.url_string)


class _FileUrl(StorageUrl):
  """File URL class providing parsing and convenience methods.

    This class assists with usage and manipulation of an
    (optionally wildcarded) file URL string.  Depending on the string
    contents, this class represents one or more directories or files.

    For File URLs, scheme is always file, bucket_name is always blank,
    and object_name contains the file/directory path.
  """

  def __init__(self, url_string, is_stream=False, is_fifo=False):
    self.scheme = 'file'
    self.bucket_name = ''
    match = FILE_OBJECT_REGEX.match(url_string)
    if match and match.lastindex == 2:
      self.object_name = match.group(2)
    else:
      self.object_name = url_string
    self.generation = None
    self.is_stream = is_stream
    self.is_fifo = is_fifo
    self.delim = os.sep

  def Clone(self):
    return _FileUrl(self.url_string)

  def IsFileUrl(self):
    return True

  def IsCloudUrl(self):
    return False

  def IsStream(self):
    return self.is_stream

  def IsFifo(self):
    return self.is_fifo

  def IsDirectory(self):
    return (not self.IsStream() and
            not self.IsFifo() and
            os.path.isdir(self.object_name))

  def CreatePrefixUrl(self, wildcard_suffix=None):
    return self.url_string

  @property
  def url_string(self):
    return '%s://%s' % (self.scheme, self.object_name)

  @property
  def versionless_url_string(self):
    return self.url_string

  def __str__(self):
    return self.url_string


class _CloudUrl(StorageUrl):
  """Cloud URL class providing parsing and convenience methods.

    This class assists with usage and manipulation of an
    (optionally wildcarded) cloud URL string.  Depending on the string
    contents, this class represents a provider, bucket(s), or object(s).

    This class operates only on strings.  No cloud storage API calls are
    made from this class.
  """

  def __init__(self, url_string):
    self.scheme = None
    self.bucket_name = None
    self.object_name = None
    self.generation = None
    self.delim = '/'
    provider_match = PROVIDER_REGEX.match(url_string)
    bucket_match = BUCKET_REGEX.match(url_string)
    if provider_match:
      self.scheme = provider_match.group('provider')
    elif bucket_match:
      self.scheme = bucket_match.group('provider')
      self.bucket_name = bucket_match.group('bucket')
    else:
      object_match = OBJECT_REGEX.match(url_string)
      if object_match:
        self.scheme = object_match.group('provider')
        self.bucket_name = object_match.group('bucket')
        self.object_name = object_match.group('object')
        if self.object_name == '.' or self.object_name == '..':
          raise InvalidUrlError(
              '%s is an invalid root-level object name' % self.object_name)
        if self.scheme == 'gs':
          generation_match = GS_GENERATION_REGEX.match(self.object_name)
          if generation_match:
            self.object_name = generation_match.group('object')
            self.generation = generation_match.group('generation')
        elif self.scheme == 's3':
          version_match = S3_VERSION_REGEX.match(self.object_name)
          if version_match:
            self.object_name = version_match.group('object')
            self.generation = version_match.group('version_id')
      else:
        raise InvalidUrlError(
            'CloudUrl: URL string %s did not match URL regex' % url_string)

  def Clone(self):
    return _CloudUrl(self.url_string)

  def IsFileUrl(self):
    return False

  def IsCloudUrl(self):
    return True

  def IsStream(self):
    raise NotImplementedError('IsStream not supported on CloudUrl')

  def IsFifo(self):
    raise NotImplementedError('IsFifo not supported on CloudUrl')

  def IsBucket(self):
    return bool(self.bucket_name and not self.object_name)

  def IsObject(self):
    return bool(self.bucket_name and self.object_name)

  def HasGeneration(self):
    return bool(self.generation)

  def IsProvider(self):
    return bool(self.scheme and not self.bucket_name)

  def CreatePrefixUrl(self, wildcard_suffix=None):
    prefix = StripOneSlash(self.versionless_url_string)
    if wildcard_suffix:
      prefix = '%s/%s' % (prefix, wildcard_suffix)
    return prefix

  @property
  def bucket_url_string(self):
    return '%s://%s/' % (self.scheme, self.bucket_name)

  @property
  def url_string(self):
    url_str = self.versionless_url_string
    if self.HasGeneration():
      url_str += '#%s' % self.generation
    return url_str

  @property
  def versionless_url_string(self):
    if self.IsProvider():
      return '%s://' % self.scheme
    elif self.IsBucket():
      return self.bucket_url_string
    return '%s://%s/%s' % (self.scheme, self.bucket_name, self.object_name)

  def __str__(self):
    return self.url_string


def _GetSchemeFromUrlString(url_str):
  """Returns scheme component of a URL string."""

  end_scheme_idx = url_str.find('://')
  if end_scheme_idx == -1:
    # File is the default scheme.
    return 'file'
  else:
    return url_str[0:end_scheme_idx].lower()


def _GetPathFromUrlString(url_str):
  """Returns path component of a URL string."""

  end_scheme_idx = url_str.find('://')
  if end_scheme_idx == -1:
    return url_str
  else:
    return url_str[end_scheme_idx + 3:]


def IsFileUrlString(url_str):
  """Returns whether a string is a file URL."""

  return _GetSchemeFromUrlString(url_str) == 'file'


def StorageUrlFromString(url_str):
  """Static factory function for creating a StorageUrl from a string."""

  scheme = _GetSchemeFromUrlString(url_str)

  if scheme not in ('file', 's3', 'gs'):
    raise InvalidUrlError('Unrecognized scheme "%s"' % scheme)
  if scheme == 'file':
    path = _GetPathFromUrlString(url_str)
    is_stream = (path == '-')
    is_fifo = False
    try:
      is_fifo = stat.S_ISFIFO(os.stat(path).st_mode)
    except OSError:
      pass
    return _FileUrl(url_str, is_stream=is_stream, is_fifo=is_fifo)
  return _CloudUrl(url_str)


def StripOneSlash(url_str):
  if url_str and url_str.endswith('/'):
    return url_str[:-1]
  return url_str


def ContainsWildcard(url_string):
  """Checks whether url_string contains a wildcard.

  Args:
    url_string: URL string to check.

  Returns:
    bool indicator.
  """
  return bool(WILDCARD_REGEX.search(url_string))
