# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Bootstrap Chrome Telemetry by downloading all its files from SVN servers.

Requires a DEPS file to specify which directories on which SVN servers
are required to run Telemetry. Format of that DEPS file is a subset of the
normal DEPS file format[1]; currently only only the "deps" dictionary is
supported and nothing else.

Fetches all files in the specified directories using WebDAV (SVN is WebDAV under
the hood).

[1] http://dev.chromium.org/developers/how-tos/depottools#TOC-DEPS-file
"""

import imp
import logging
import os
import urllib
import urlparse

# Dummy module for DAVclient.
davclient = None


# TODO(eakuefner): Switch this link to tools/perf version after verifying.
# Link to file containing the 'davclient' WebDAV client library.
_DAVCLIENT_URL = ('https://src.chromium.org/chrome/trunk/src/tools/'
                  'telemetry/third_party/davclient/davclient.py')


def _DownloadAndImportDAVClientModule():
  """Dynamically import davclient helper library."""
  global davclient
  davclient_src = urllib.urlopen(_DAVCLIENT_URL).read()
  davclient = imp.new_module('davclient')
  exec davclient_src in davclient.__dict__  # pylint: disable=exec-used


class DAVClientWrapper(object):
  """Knows how to retrieve subdirectories and files from WebDAV/SVN servers."""

  def __init__(self, root_url):
    """Initialize SVN server root_url, save files to local dest_dir.

    Args:
      root_url: string url of SVN/WebDAV server
    """
    self.root_url = root_url
    self.client = davclient.DAVClient(root_url)

  @staticmethod
  def __norm_path_keys(dict_with_path_keys):
    """Returns a dictionary with os.path.normpath called on every key."""
    return dict((os.path.normpath(k), v) for (k, v) in
                dict_with_path_keys.items())

  def GetDirList(self, path):
    """Returns string names of all files and subdirs of path on the server."""
    props = self.__norm_path_keys(self.client.propfind(path, depth=1))
    # remove this path
    del props[os.path.normpath(path)]
    return [os.path.basename(p) for p in props.keys()]

  def IsFile(self, path):
    """Returns True if the path is a file on the server, False if directory."""
    props = self.__norm_path_keys(self.client.propfind(path, depth=1))
    return props[os.path.normpath(path)]['resourcetype'] is None

  def Traverse(self, src_path, dst_path):
    """Walks the directory hierarchy pointed to by src_path download all files.

    Recursively walks src_path and saves all files and subfolders into
    dst_path.

    Args:
      src_path: string path on SVN server to save (absolute path on server).
      dest_path: string local path (relative or absolute) to save to.
    """
    if self.IsFile(src_path):
      if not os.path.exists(os.path.dirname(dst_path)):
        logging.info('Creating %s', os.path.dirname(dst_path))
        os.makedirs(os.path.dirname(dst_path))
      if os.path.isfile(dst_path):
        logging.info('Skipping %s', dst_path)
      else:
        logging.info('Saving %s to %s', self.root_url + src_path, dst_path)
        urllib.urlretrieve(self.root_url + src_path, dst_path)
      return
    else:
      for subdir in self.GetDirList(src_path):
        self.Traverse(os.path.join(src_path, subdir),
                      os.path.join(dst_path, subdir))


def ListAllDepsPaths(deps_file):
  """Recursively returns a list of all paths indicated in this deps file.

  Note that this discards information about where path dependencies come from,
  so this is only useful in the context of a Chromium source checkout that has
  already fetched all dependencies.

  Args:
    deps_file: File containing deps information to be evaluated, in the
               format given in the header of this file.
  Returns:
    A list of string paths starting under src that are required by the
    given deps file, and all of its sub-dependencies. This amounts to
    the keys of the 'deps' dictionary.
  """
  deps = {}
  deps_includes = {}

  chrome_root = os.path.dirname(__file__)
  while os.path.basename(chrome_root) != 'src':
    chrome_root = os.path.abspath(os.path.join(chrome_root, '..'))

  exec open(deps_file).read()  # pylint: disable=exec-used

  deps_paths = deps.keys()

  for path in deps_includes.keys():
    # Need to localize the paths.
    path = os.path.join(chrome_root, '..', path)
    deps_paths += ListAllDepsPaths(path)

  return deps_paths


def DownloadDeps(destination_dir, url):
  """Saves all the dependencies in deps_path.

  Opens and reads url, assuming the contents are in the simple DEPS-like file
  format specified in the header of this file, then download all
  files/directories listed to the destination_dir.

  Args:
    destination_dir: String path to directory to download files into.
    url: URL containing deps information to be evaluated.
  """
  logging.warning('Downloading deps from %s...', url)
  # TODO(wiltzius): Add a parameter for which revision to pull.
  _DownloadAndImportDAVClientModule()

  deps = {}
  deps_includes = {}

  exec urllib.urlopen(url).read()  # pylint: disable=exec-used

  for dst_path, src_path in deps.iteritems():
    full_dst_path = os.path.join(destination_dir, dst_path)
    parsed_url = urlparse.urlparse(src_path)
    root_url = parsed_url.scheme + '://' + parsed_url.netloc

    dav_client = DAVClientWrapper(root_url)
    dav_client.Traverse(parsed_url.path, full_dst_path)

  for url in deps_includes.values():
    DownloadDeps(destination_dir, url)
