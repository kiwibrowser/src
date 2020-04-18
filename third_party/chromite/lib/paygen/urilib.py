# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities for standard operations on URIs of different kinds."""

from __future__ import print_function

import re
import sys
import urllib
import urllib2

from chromite.lib.paygen import filelib
from chromite.lib.paygen import gslib


# This module allows files from different storage types to be handled
# in a common way, for supported operations.


PROTOCOL_GS = gslib.PROTOCOL
PROTOCOL_HTTP = 'http'
PROTOCOL_HTTPS = 'https'

PROTOCOLS = (PROTOCOL_GS,
             PROTOCOL_HTTP,
             PROTOCOL_HTTPS)

PROTOCOL_SEP = '://'

EXTRACT_PROTOCOL_RE = re.compile(r'^(\w+)%s' % PROTOCOL_SEP)
SPLIT_URI_RE = re.compile(r'^(\w+)%s(.*)$' % PROTOCOL_SEP)

TYPE_GS = PROTOCOL_GS
TYPE_HTTP = PROTOCOL_HTTP
TYPE_HTTPS = PROTOCOL_HTTPS
TYPE_LOCAL = 'file'


class NotSupportedForType(RuntimeError):
  """Raised when operation is not supported for a particular file type"""

  def __init__(self, uri_type, extra_msg=None):
    # pylint: disable=protected-access
    function = sys._getframe(1).f_code.co_name
    msg = 'Function %s not supported for %s URIs' % (function, uri_type)
    if extra_msg:
      msg += ', ' + extra_msg

    RuntimeError.__init__(self, msg)


class NotSupportedForTypes(RuntimeError):
  """Raised when operation is not supported for all particular file type"""

  def __init__(self, extra_msg=None, *uri_types):
    # pylint: disable=protected-access
    function = sys._getframe(1).f_code.co_name
    msg = ('Function %s not supported for set of URIs with types: %s' %
           (function, ', '.join(uri_types)))
    if extra_msg:
      msg += ', ' + extra_msg

    RuntimeError.__init__(self, msg)


class NotSupportedBetweenTypes(RuntimeError):
  """Raised when operation is not supported between particular file types"""

  def __init__(self, uri_type1, uri_type2, extra_msg=None):
    # pylint: disable=protected-access
    function = sys._getframe(1).f_code.co_name
    msg = ('Function %s not supported between %s and %s URIs' %
           (function, uri_type1, uri_type2))
    if extra_msg:
      msg += ', ' + extra_msg

    RuntimeError.__init__(self, msg)


class MissingURLError(RuntimeError):
  """Raised when nothing exists at URL."""


def ExtractProtocol(uri):
  """Take a URI and return the protocol it is using, if any.

  Examples:
  'gs://some/path' ==> 'gs'
  'file:///some/path' ==> 'file'
  '/some/path' ==> None
  '/cns/some/colossus/path' ==> None

  Args:
    uri: The URI to get protocol from.

  Returns:
    Protocol string that is found, or None.
  """
  match = EXTRACT_PROTOCOL_RE.search(uri)
  if match:
    return match.group(1)

  return None


def GetUriType(uri):
  """Get the type of a URI.

  See the TYPE_* constants for examples.  This is mostly based
  on URI protocols, with Colossus and local files as exceptions.

  Args:
    uri: The URI to consider

  Returns:
    The URI type.
  """
  protocol = ExtractProtocol(uri)
  if protocol:
    return protocol

  return TYPE_LOCAL


def SplitURI(uri):
  """Get the protocol and path from a URI

  Examples:
  'gs://some/path' ==> ('gs', 'some/path')
  'file:///some/path' ==> ('file', '/some/path')
  '/some/path' ==> (None, '/some/path')
  '/cns/some/colossus/path' ==> (None, '/cns/some/colossus/path')

  Args:
    uri: The uri to get protocol and path from.

  Returns;
    Tuple (protocol, path)
  """
  match = SPLIT_URI_RE.search(uri)
  if match:
    return (match.group(1), match.group(2))

  return (None, uri)


def IsGsURI(uri):
  """Returns True if given uri uses Google Storage protocol."""
  return PROTOCOL_GS == ExtractProtocol(uri)


def IsFileURI(uri):
  """Return True if given uri is a file URI (or path).

  If uri uses the file protocol or it is a plain non-Colossus path
  then return True.

  Args:
    uri: Any URI or path.

  Returns:
    True or False as described above.
  """
  return TYPE_LOCAL == GetUriType(uri)


def IsHttpURI(uri, https_ok=False):
  """Returns True if given uri uses http, or optionally https, protocol.

  Args:
    uri: The URI to check.
    https_ok: If True, then accept https protocol as well.

  Returns:
    Boolean
  """
  uri_type = GetUriType(uri)
  return TYPE_HTTP == uri_type or (https_ok and TYPE_HTTPS == uri_type)


def IsHttpsURI(uri):
  """Returns True if given uri uses https protocol."""
  return TYPE_HTTPS == GetUriType(uri)


def MD5Sum(uri):
  """Compute or retrieve MD5 sum of uri.

  Supported for: local files, GS files.

  Args:
    uri: The /unix/path or gs:// uri to compute the md5sum on.

  Returns:
    A string representing the md5sum of the file/uri passed in.
    None if we do not understand the uri passed in or cannot compute
    the md5sum.
  """

  uri_type = GetUriType(uri)

  if uri_type == TYPE_LOCAL:
    return filelib.MD5Sum(uri)
  elif uri_type == TYPE_GS:
    try:
      return gslib.MD5Sum(uri)
    except gslib.GSLibError:
      return None

  # Colossus does not have a command for getting MD5 sum.  We could
  # copy the file to local disk and calculate it, but it seems better
  # to explicitly say it is not supported.

  raise NotSupportedForType(uri_type)


def Cmp(uri1, uri2):
  """Return True if paths hold identical files.

  If either file is missing then always return False.

  Args:
    uri1: URI to a file.
    uri2: URI to a file.

  Returns:
    True if files are the same, False otherwise.

  Raises:
    NotSupportedBetweenTypes if Cmp cannot be done between the two
      URIs provided.
  """
  uri_type1 = GetUriType(uri1)
  uri_type2 = GetUriType(uri2)
  uri_types = set([uri_type1, uri_type2])

  if TYPE_GS in uri_types:
    # GS only supported between other GS files or local files.
    if len(uri_types) == 1 or TYPE_LOCAL in uri_types:
      return gslib.Cmp(uri1, uri2)

  if TYPE_LOCAL in uri_types and len(uri_types) == 1:
    return filelib.Cmp(uri1, uri2)

  raise NotSupportedBetweenTypes(uri_type1, uri_type2)


class URLopener(urllib.FancyURLopener):
  """URLopener that will actually complain when download fails."""
  # The urllib.urlretrieve function, which seems like a good fit for this,
  # does not give access to error code.

  def http_error_default(self, *args, **kwargs):
    urllib.URLopener.http_error_default(self, *args, **kwargs)


def URLRetrieve(src_url, dest_path):
  """Download file from given URL to given local file path.

  Args:
    src_url: URL to download from.
    dest_path: Path to download to.

  Raises:
    MissingURLError if URL cannot be downloaded.
  """
  opener = URLopener()

  try:
    opener.retrieve(src_url, dest_path)
  except IOError as e:
    # If the domain is valid but download failed errno shows up as None.
    if e.errno is None:
      raise MissingURLError('Unable to download %s' % src_url)

    # If the domain is invalid the errno shows up as 'socket error', weirdly.
    try:
      int(e.errno)

      # This means there was some normal error writing to the dest_path.
      raise
    except ValueError:
      raise MissingURLError('Unable to download %s (bad domain?)' % src_url)


def Copy(src_uri, dest_uri):
  """Copy one uri to another.

  Args:
    src_uri: URI to copy from.
    dest_uri: Path to copy to.

  Raises:
    NotSupportedBetweenTypes if Cmp cannot be done between the two
      URIs provided.
  """
  uri_type1 = GetUriType(src_uri)
  uri_type2 = GetUriType(dest_uri)
  uri_types = set([uri_type1, uri_type2])

  if TYPE_GS in uri_types:
    # GS only supported between other GS files or local files.
    if len(uri_types) == 1 or TYPE_LOCAL in uri_types:
      return gslib.Copy(src_uri, dest_uri)

  if TYPE_LOCAL in uri_types and len(uri_types) == 1:
    return filelib.Copy(src_uri, dest_uri)

  if uri_type1 in (TYPE_HTTP, TYPE_HTTPS) and uri_type2 == TYPE_LOCAL:
    # Download file from URL.
    return URLRetrieve(src_uri, dest_uri)

  raise NotSupportedBetweenTypes(uri_type1, uri_type2)


def Remove(*args, **kwargs):
  """Delete the file(s) at uris, or directory(s) with recurse set.

  Args:
    args: One or more URIs.
    ignore_no_match: If True, then do not complain if anything was not
      removed because no URI match was found.  Like rm -f.  Defaults to False.
    recurse: Remove recursively starting at path.  Same as rm -R.  Defaults
      to False.
  """
  uri_types = set([GetUriType(u) for u in args])

  if TYPE_GS in uri_types:
    # GS support only allows local files among list.
    if len(uri_types) == 1 or (TYPE_LOCAL in uri_types and len(uri_types) == 2):
      return gslib.Remove(*args, **kwargs)

  if TYPE_LOCAL in uri_types and len(uri_types) == 1:
    return filelib.Remove(*args, **kwargs)

  raise NotSupportedForTypes(*list(uri_types))


def Size(uri):
  """Return size of file at URI in bytes.

  Args:
    uri: URI to consider

  Returns:
    Size of file at given URI in bytes.

  Raises:
    MissingURLError if uri is a URL and cannot be found.
  """

  uri_type = GetUriType(uri)

  if TYPE_GS == uri_type:
    return gslib.FileSize(uri)

  if TYPE_LOCAL == uri_type:
    return filelib.Size(uri)

  if TYPE_HTTP == uri_type or TYPE_HTTPS == uri_type:
    try:
      response = urllib2.urlopen(uri)
      if response.getcode() == 200:
        return int(response.headers.getheader('Content-Length'))

    except urllib2.HTTPError as e:
      # Interpret 4** errors as our own MissingURLError.
      if e.code < 400 or e.code >= 500:
        raise

    raise MissingURLError('No such file at URL %s' % uri)

  raise NotSupportedForType(uri_type)


def Exists(uri, as_dir=False):
  """Return True if file exists at given URI.

  If URI is a directory and as_dir is False then this will return False.

  Args:
    uri: URI to consider
    as_dir: If True then check URI as a directory, otherwise check as a file.

  Returns:
    True if file (or directory) exists at URI, False otherwise.
  """
  uri_type = GetUriType(uri)

  if TYPE_GS == uri_type:
    if as_dir:
      # GS does not contain directories.
      return False

    return gslib.Exists(uri)

  if TYPE_LOCAL == uri_type:
    return filelib.Exists(uri, as_dir=as_dir)

  if TYPE_HTTP == uri_type or TYPE_HTTPS == uri_type:
    if as_dir:
      raise NotSupportedForType(uri_type, extra_msg='with as_dir=True')

    try:
      response = urllib2.urlopen(uri)
      return response.getcode() == 200
    except urllib2.HTTPError:
      return False

  raise NotSupportedForType(uri_type)


def ListFiles(root_path, recurse=False, filepattern=None, sort=False):
  """Return list of file paths under given root path.

  Directories are intentionally excluded from results.  The root_path
  argument can be a local directory path, a Google storage directory URI,
  or a Colossus (/cns) directory path.

  Args:
    root_path: A local path, CNS path, or GS path to directory.
    recurse: Look for files in subdirectories, as well
    filepattern: glob pattern to match against basename of file
    sort: If True then do a default sort on paths

  Returns:
    List of paths to files that matched
  """
  uri_type = GetUriType(root_path)

  if TYPE_GS == uri_type:
    return gslib.ListFiles(root_path, recurse=recurse,
                           filepattern=filepattern, sort=sort)

  if TYPE_LOCAL == uri_type:
    return filelib.ListFiles(root_path, recurse=recurse,
                             filepattern=filepattern, sort=sort)

  raise NotSupportedForType(uri_type)


def CopyFiles(src_dir, dst_dir):
  """Recursively copy all files from src_dir into dst_dir

  This leverages the Copy method, so the restrictions there for what
  copies are supported apply here.

  Args:
    src_dir: A local, CNS, or GS directory to copy from.
    dst_dir: A local, CNS, or GS directory to copy into.

  Returns:
    A list of absolute path files for all copied files.
  """
  dst_paths = []
  src_paths = ListFiles(src_dir, recurse=True)
  for src_path in src_paths:
    dst_path = src_path.replace(src_dir, dst_dir)
    Copy(src_path, dst_path)
    dst_paths.append(dst_path)

  return dst_paths


def RemoveDirContents(base_dir):
  """Remove all contents of a directory.

  Args:
    base_dir: directory to delete contents of.
  """
  uri_type = GetUriType(base_dir)

  if TYPE_GS == uri_type:
    return gslib.RemoveDirContents(base_dir)

  if TYPE_LOCAL == uri_type:
    return filelib.RemoveDirContents(base_dir)

  raise NotSupportedForType(uri_type)
