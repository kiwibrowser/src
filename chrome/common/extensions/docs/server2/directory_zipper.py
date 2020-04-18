# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from io import BytesIO
import posixpath
from zipfile import ZipFile

from compiled_file_system import SingleFile


class DirectoryZipper(object):
  '''Creates zip files of whole directories.
  '''

  def __init__(self, compiled_fs_factory, file_system):
    self._file_system = file_system
    self._zip_cache = compiled_fs_factory.Create(file_system,
                                                 self._MakeZipFile,
                                                 DirectoryZipper)

  # NOTE: It's ok to specify SingleFile here even though this method reads
  # multiple files. All files are underneath |base_dir|. If any file changes its
  # stat will change, so the stat of |base_dir| will also change.
  @SingleFile
  def _MakeZipFile(self, base_dir, files):
    base_dir = base_dir.strip('/')
    zip_bytes = BytesIO()
    with ZipFile(zip_bytes, mode='w') as zip_file:
      futures_with_name = [
          (file_name,
           self._file_system.ReadSingle(posixpath.join(base_dir, file_name)))
          for file_name in files]
      for file_name, future in futures_with_name:
        file_contents = future.Get()
        if isinstance(file_contents, unicode):
          # Data is sometimes already cached as unicode.
          file_contents = file_contents.encode('utf8')
        # We want e.g. basic.zip to expand to basic/manifest.json etc, not
        # chrome/common/extensions/.../basic/manifest.json, so only use the
        # end of the path component when writing into the zip file.
        dir_name = posixpath.basename(base_dir)
        zip_file.writestr(posixpath.join(dir_name, file_name), file_contents)
    return zip_bytes.getvalue()

  def Zip(self, path):
    '''Creates a new zip file from the recursive contents of |path| as returned
    by |_zip_cache|.

    Paths within the zip file will be relative to and include the last
    component of |path|, such that when zipping foo/bar containing baf.txt and
    qux.txt will return a zip file which unzips to bar/baz.txt and bar/qux.txt.
    '''
    return self._zip_cache.GetFromFileListing(path)
