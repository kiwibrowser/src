# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import mimetypes
import posixpath
import traceback

from compiled_file_system import SingleFile
from directory_zipper import DirectoryZipper
from docs_server_utils import ToUnicode
from file_system import FileNotFoundError
from future import All, Future
from path_canonicalizer import PathCanonicalizer
from path_util import AssertIsValid, IsDirectory, Join, ToDirectory
from special_paths import SITE_VERIFICATION_FILE
from third_party.markdown import markdown
from third_party.motemplate import Motemplate


_MIMETYPE_OVERRIDES = {
  # SVG is not supported by mimetypes.guess_type on AppEngine.
  '.svg': 'image/svg+xml',
}


class ContentAndType(object):
  '''Return value from ContentProvider.GetContentAndType.
  '''

  def __init__(self, content, content_type, version):
    self.content = content
    self.content_type = content_type
    self.version = version


class ContentProvider(object):
  '''Returns file contents correctly typed for their content-types (in the HTTP
  sense). Content-type is determined from Python's mimetype library which
  guesses based on the file extension.

  Typically the file contents will be either str (for binary content) or
  unicode (for text content). However, HTML files *may* be returned as
  Motemplate templates (if |supports_templates| is True on construction), in
  which case the caller will presumably want to Render them.

  Zip file are automatically created and returned for .zip file extensions if
  |supports_zip| is True.

  |default_extensions| is a list of file extensions which are queried when no
  file extension is given to GetCanonicalPath/GetContentAndType.  Typically
  this will include .html.
  '''

  def __init__(self,
               name,
               compiled_fs_factory,
               file_system,
               object_store_creator,
               default_extensions=(),
               supports_templates=False,
               supports_zip=False):
    # Public.
    self.name = name
    self.file_system = file_system
    # Private.
    self._content_cache = compiled_fs_factory.Create(file_system,
                                                     self._CompileContent,
                                                     ContentProvider)
    self._path_canonicalizer = PathCanonicalizer(file_system,
                                                 object_store_creator,
                                                 default_extensions)
    self._default_extensions = default_extensions
    self._supports_templates = supports_templates
    if supports_zip:
      self._directory_zipper = DirectoryZipper(compiled_fs_factory, file_system)
    else:
      self._directory_zipper = None

  @SingleFile
  def _CompileContent(self, path, text):
    assert text is not None, path
    try:
      _, ext = posixpath.splitext(path)
      mimetype = _MIMETYPE_OVERRIDES.get(ext, mimetypes.guess_type(path)[0])
      if ext == '.md':
        # See http://pythonhosted.org/Markdown/extensions
        # for details on "extensions=".
        content = markdown(ToUnicode(text),
                           extensions=('extra', 'headerid', 'sane_lists'))
        mimetype = 'text/html'
        if self._supports_templates:
          content = Motemplate(content, name=path)
      elif mimetype is None:
        content = text
        mimetype = 'text/plain'
      elif mimetype == 'text/html':
        content = ToUnicode(text)
        if self._supports_templates:
          content = Motemplate(content, name=path)
      elif (mimetype.startswith('text/') or
            mimetype in ('application/javascript', 'application/json')):
        content = ToUnicode(text)
      else:
        content = text
      return ContentAndType(content,
                            mimetype,
                            self.file_system.Stat(path).version)
    except Exception as e:
      logging.warn('In file %s: %s' % (path, e.message))
      return ContentAndType('', mimetype, self.file_system.Stat(path).version)

  def GetCanonicalPath(self, path):
    '''Gets the canonical location of |path|. This class is tolerant of
    spelling errors and missing files that are in other directories, and this
    returns the correct/canonical path for those.

    For example, the canonical path of "browseraction" is probably
    "extensions/browserAction.html".

    Note that the canonical path is relative to this content provider i.e.
    given relative to |path|. It does not add the "serveFrom" prefix which
    would have been pulled out in ContentProviders, callers must do that
    themselves.
    '''
    AssertIsValid(path)
    base, ext = posixpath.splitext(path)
    if self._directory_zipper and ext == '.zip':
      # The canonical location of zip files is the canonical location of the
      # directory to zip + '.zip'.
      return self._path_canonicalizer.Canonicalize(base + '/').rstrip('/') + ext
    return self._path_canonicalizer.Canonicalize(path)

  def GetContentAndType(self, path):
    '''Returns a Future to the ContentAndType of the file at |path|.
    '''
    AssertIsValid(path)
    base, ext = posixpath.splitext(path)
    if self._directory_zipper and ext == '.zip':
      return (self._directory_zipper.Zip(ToDirectory(base))
              .Then(lambda zipped: ContentAndType(zipped,
                                                  'application/zip',
                                                  None)))
    return self._FindFileForPath(path).Then(self._content_cache.GetFromFile)

  def GetVersion(self, path):
    '''Returns a Future to the version of the file at |path|.
    '''
    AssertIsValid(path)
    base, ext = posixpath.splitext(path)
    if self._directory_zipper and ext == '.zip':
      stat_future = self.file_system.StatAsync(ToDirectory(base))
    else:
      stat_future = self._FindFileForPath(path).Then(self.file_system.StatAsync)
    return stat_future.Then(lambda stat: stat.version)

  def _FindFileForPath(self, path):
    '''Finds the real file backing |path|. This may require looking for the
    correct file extension, or looking for an 'index' file if it's a directory.
    Returns None if no path is found.
    '''
    AssertIsValid(path)
    _, ext = posixpath.splitext(path)

    if ext:
      # There was already an extension, trust that it's a path. Elsewhere
      # up the stack this will be caught if it's not.
      return Future(value=path)

    def find_file_with_name(name):
      '''Tries to find a file in the file system called |name| with one of the
      default extensions of this content provider.
      If none is found, returns None.
      '''
      paths = [name + ext for ext in self._default_extensions]
      def get_first_path_which_exists(existence):
        for exists, path in zip(existence, paths):
          if exists:
            return path
        return None
      return (All(self.file_system.Exists(path) for path in paths)
              .Then(get_first_path_which_exists))

    def find_index_file():
      '''Tries to find an index file in |path|, if |path| is a directory.
      If not, or if there is no index file, returns None.
      '''
      def get_index_if_directory_exists(directory_exists):
        if not directory_exists:
          return None
        return find_file_with_name(Join(path, 'index'))
      return (self.file_system.Exists(ToDirectory(path))
              .Then(get_index_if_directory_exists))

    # Try to find a file with the right name. If not, and it's a directory,
    # look for an index file in that directory. If nothing at all is found,
    # return the original |path| - its nonexistence will be caught up the stack.
    return (find_file_with_name(path)
            .Then(lambda found: found or find_index_file())
            .Then(lambda found: found or path))

  def Refresh(self):
    futures = [self._path_canonicalizer.Refresh()]
    for root, _, files in self.file_system.Walk(''):
      for f in files:
        futures.append(self.GetContentAndType(Join(root, f)))
        # Also cache the extension-less version of the file if needed.
        base, ext = posixpath.splitext(f)
        if f != SITE_VERIFICATION_FILE and ext in self._default_extensions:
          futures.append(self.GetContentAndType(Join(root, base)))
      # TODO(kalman): Cache .zip files for each directory (if supported).
    return All(futures, except_pass=Exception, except_pass_log=True)

  def __repr__(self):
    return 'ContentProvider of <%s>' % repr(self.file_system)
