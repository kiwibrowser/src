# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import os
import unittest

from telemetry.internal.results import artifact_results
from telemetry.internal.util import file_handle

from py_utils import tempfile_ext


# splitdrive returns '' on systems which don't have drives, like linux.
ROOT_CHAR = os.path.splitdrive(__file__)[0] + os.sep


def _abs_join(*args):
  """Helper to do a path join that's an absolute path."""
  return ROOT_CHAR + os.path.join(*args)


class ArtifactResultsUnittest(unittest.TestCase):
  def testCreateBasic(self):
    with tempfile_ext.NamedTemporaryDirectory(
        prefix='artifact_tests') as tempdir:
      ar = artifact_results.ArtifactResults(tempdir)
      filenames = []
      with ar.CreateArtifact('bad//story:name', 'logs') as log_file:
        filenames.append(log_file.name)
        log_file.write('hi\n')

      with ar.CreateArtifact('other_name', 'logs') as log_file:
        filenames.append(log_file.name)
        log_file.write('hi\n')

      for filename in filenames:
        with open(filename) as f:
          self.assertEqual(f.read(), 'hi\n')

  def testCreateDuplicateStoryName(self):
    with tempfile_ext.NamedTemporaryDirectory(
        prefix='artifact_tests') as tempdir:
      ar = artifact_results.ArtifactResults(tempdir)
      filenames = []
      with ar.CreateArtifact('story_name', 'logs') as log_file:
        filenames.append(log_file.name)
        log_file.write('hi\n')

      with ar.CreateArtifact('story_name', 'logs') as log_file:
        filenames.append(log_file.name)
        log_file.write('hi\n')

      for filename in filenames:
        with open(filename) as f:
          self.assertEqual(f.read(), 'hi\n')

  @mock.patch('telemetry.internal.results.artifact_results.shutil.move')
  @mock.patch('telemetry.internal.results.artifact_results.os.makedirs')
  def testAddBasic(self, make_patch, move_patch):
    ar = artifact_results.ArtifactResults(_abs_join('foo'))

    ar.AddArtifact(
        'test', 'artifact_name', _abs_join('foo', 'artifacts', 'bar.log'))
    move_patch.assert_not_called()
    make_patch.assert_called_with(_abs_join('foo', 'artifacts'))

    self.assertEqual({k: dict(v) for k, v in ar._test_artifacts.items()}, {
        'test': {
            'artifact_name': ['artifacts/bar.log'],
        }
    })

  @mock.patch('telemetry.internal.results.artifact_results.shutil.move')
  @mock.patch('telemetry.internal.results.artifact_results.os.makedirs')
  def testAddNested(self, make_patch, move_patch):
    ar = artifact_results.ArtifactResults(_abs_join('foo'))

    ar.AddArtifact('test', 'artifact_name', _abs_join(
        'foo', 'artifacts', 'baz', 'bar.log'))
    move_patch.assert_not_called()
    make_patch.assert_called_with(_abs_join('foo', 'artifacts'))

    self.assertEqual({k: dict(v) for k, v in ar._test_artifacts.items()}, {
        'test': {
            'artifact_name': ['artifacts/baz/bar.log'],
        }
    })

  @mock.patch('telemetry.internal.results.artifact_results.shutil.move')
  @mock.patch('telemetry.internal.results.artifact_results.os.makedirs')
  def testAddFileHandle(self, make_patch, move_patch):
    ar = artifact_results.ArtifactResults(_abs_join('foo'))

    ar.AddArtifact('test', 'artifact_name', file_handle.FromFilePath(
        _abs_join('', 'foo', 'artifacts', 'bar.log')))
    move_patch.assert_not_called()
    make_patch.assert_called_with(_abs_join('foo', 'artifacts'))

    self.assertEqual({k: dict(v) for k, v in ar._test_artifacts.items()}, {
        'test': {
            'artifact_name': ['artifacts/bar.log'],
        }
    })

  @mock.patch('telemetry.internal.results.artifact_results.shutil.move')
  @mock.patch('telemetry.internal.results.artifact_results.os.makedirs')
  def testAddAndMove(self, make_patch, move_patch):
    ar = artifact_results.ArtifactResults(_abs_join('foo'))

    ar.AddArtifact('test', 'artifact_name', _abs_join(
        'another', 'directory', 'bar.log'))
    move_patch.assert_called_with(
        _abs_join('another', 'directory', 'bar.log'),
        _abs_join('foo', 'artifacts'))
    make_patch.assert_called_with(_abs_join('foo', 'artifacts'))

    self.assertEqual({k: dict(v) for k, v in ar._test_artifacts.items()}, {
        'test': {
            'artifact_name': ['artifacts/bar.log'],
        }
    })

  @mock.patch('telemetry.internal.results.artifact_results.shutil.move')
  @mock.patch('telemetry.internal.results.artifact_results.os.makedirs')
  def testAddMultiple(self, make_patch, move_patch):
    ar = artifact_results.ArtifactResults(_abs_join('foo'))

    ar.AddArtifact('test', 'artifact_name', _abs_join(
        'foo', 'artifacts', 'bar.log'))
    ar.AddArtifact('test', 'artifact_name', _abs_join(
        'foo', 'artifacts', 'bam.log'))
    move_patch.assert_not_called()
    make_patch.assert_called_with(_abs_join('foo', 'artifacts'))

    self.assertEqual({k: dict(v) for k, v in ar._test_artifacts.items()}, {
        'test': {
            'artifact_name': ['artifacts/bar.log', 'artifacts/bam.log'],
        }
    })

  @mock.patch('telemetry.internal.results.artifact_results.shutil.move')
  @mock.patch('telemetry.internal.results.artifact_results.os.makedirs')
  def testIterTestAndArtifacts(self, make_patch, move_patch):
    del make_patch, move_patch  # unused
    ar = artifact_results.ArtifactResults(_abs_join('foo'))

    ar.AddArtifact('foo', 'log', _abs_join(
        'artifacts', 'foo.log'))
    ar.AddArtifact('bar', 'screenshot', _abs_join(
        'artifacts', 'bar.jpg'))

    test_artifacts = {}

    for test_name, artifacts in ar.IterTestAndArtifacts():
      test_artifacts[test_name] = artifacts

    self.assertEqual({
        'foo': {'log': ['artifacts/foo.log']},
        'bar': {'screenshot': ['artifacts/bar.jpg']}
    }, test_artifacts)


