# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import tempfile
import unittest

import chrome_cache


LOADING_DIR = os.path.dirname(os.path.abspath(__file__))
THIS_BASEMAME = os.path.basename(__file__)


class CacheDirectoryTest(unittest.TestCase):
  def setUp(self):
    self._temp_dir = tempfile.mkdtemp()

  def tearDown(self):
    shutil.rmtree(self._temp_dir)

  def GetTempPath(self, temp_name):
    return os.path.join(self._temp_dir, temp_name)

  def CreateNewGarbageFile(self, file_path):
    assert not os.path.exists(file_path)
    with open(file_path, 'w') as f:
      f.write('garbage content')
    assert os.path.isfile(file_path)

  @classmethod
  def CompareDirectories(cls, reference_path, generated_path):
    def CompareNode(relative_path):
      reference_stat = os.stat(os.path.join(reference_path, relative_path))
      generated_stat = os.stat(os.path.join(generated_path, relative_path))
      assert int(reference_stat.st_mtime) == int(generated_stat.st_mtime), \
          "{}: invalid mtime.".format(relative_path)
    for reference_parent_path, dir_names, file_names in os.walk(reference_path):
      parent_path = os.path.relpath(reference_parent_path, reference_path)
      reference_nodes = sorted(dir_names + file_names)
      generated_nodes = sorted(os.listdir(
          os.path.join(generated_path, parent_path)))
      assert reference_nodes == generated_nodes, \
          '{}: directory entries don\'t match.'.format(parent_path)
      for node in file_names:
        CompareNode(os.path.join(parent_path, node))
      CompareNode(parent_path)

  def testCompareDirectories(self):
    generated_path = self.GetTempPath('dir0')
    shutil.copytree(LOADING_DIR, generated_path)
    self.CompareDirectories(LOADING_DIR, generated_path)

    generated_path = self.GetTempPath('dir1')
    shutil.copytree(LOADING_DIR, generated_path)
    self.CreateNewGarbageFile(os.path.join(generated_path, 'garbage'))
    assert 'garbage' in os.listdir(generated_path)
    with self.assertRaisesRegexp(AssertionError, r'^.* match\.$'):
      self.CompareDirectories(LOADING_DIR, generated_path)

    generated_path = self.GetTempPath('dir2')
    shutil.copytree(LOADING_DIR, generated_path)
    self.CreateNewGarbageFile(os.path.join(generated_path, 'testdata/garbage'))
    with self.assertRaisesRegexp(AssertionError, r'^.* match\.$'):
      self.CompareDirectories(LOADING_DIR, generated_path)

    generated_path = self.GetTempPath('dir3')
    shutil.copytree(LOADING_DIR, generated_path)
    os.remove(os.path.join(generated_path, THIS_BASEMAME))
    with self.assertRaisesRegexp(AssertionError, r'^.* match\.$'):
      self.CompareDirectories(LOADING_DIR, generated_path)
    self.CreateNewGarbageFile(os.path.join(generated_path, 'garbage'))
    with self.assertRaisesRegexp(AssertionError, r'^.* match\.$'):
      self.CompareDirectories(LOADING_DIR, generated_path)

    def TouchHelper(temp_name, relative_name, timestamps):
      generated_path = self.GetTempPath(temp_name)
      shutil.copytree(LOADING_DIR, generated_path)
      os.utime(os.path.join(generated_path, relative_name), timestamps)
      with self.assertRaisesRegexp(AssertionError, r'^.* invalid mtime\.$'):
        self.CompareDirectories(LOADING_DIR, generated_path)

    TouchHelper('dir4', THIS_BASEMAME, (1256925858, 1256463122))
    TouchHelper('dir5', 'testdata', (1256918318, 1256568641))
    TouchHelper('dir6', 'trace_test/test_server.py', (1255116211, 1256156632))
    TouchHelper('dir7', './', (1255115332, 1256251864))

  def testCacheArchive(self):
    zip_dest = self.GetTempPath('cache.zip')
    chrome_cache.ZipDirectoryContent(LOADING_DIR, zip_dest)

    unzip_dest = self.GetTempPath('cache')
    chrome_cache.UnzipDirectoryContent(zip_dest, unzip_dest)
    self.CompareDirectories(LOADING_DIR, unzip_dest)

    self.CreateNewGarbageFile(os.path.join(unzip_dest, 'garbage'))
    chrome_cache.UnzipDirectoryContent(zip_dest, unzip_dest)
    self.CompareDirectories(LOADING_DIR, unzip_dest)

    unzip_dest = self.GetTempPath('foo/bar/cache')
    chrome_cache.UnzipDirectoryContent(zip_dest, unzip_dest)
    self.CompareDirectories(LOADING_DIR, unzip_dest)

  def testCopyCacheDirectory(self):
    copy_dest = self.GetTempPath('cache')
    chrome_cache.CopyCacheDirectory(LOADING_DIR, copy_dest)
    self.CompareDirectories(LOADING_DIR, copy_dest)

    self.CreateNewGarbageFile(os.path.join(copy_dest, 'garbage'))
    chrome_cache.CopyCacheDirectory(LOADING_DIR, copy_dest)
    self.CompareDirectories(LOADING_DIR, copy_dest)

    copy_dest = self.GetTempPath('foo/bar/cache')
    chrome_cache.CopyCacheDirectory(LOADING_DIR, copy_dest)
    self.CompareDirectories(LOADING_DIR, copy_dest)


if __name__ == '__main__':
  unittest.main()
