# -*- coding: utf-8 -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the archive_lib module."""

from __future__ import print_function

import mock

from chromite.cbuildbot import archive_lib
from chromite.cbuildbot import cbuildbot_run
from chromite.lib import config_lib
from chromite.lib import config_lib_unittest
from chromite.lib import cros_test_lib
from chromite.lib import parallel_unittest


DEFAULT_ARCHIVE_PREFIX = 'bogus_bucket/TheArchiveBase'
DEFAULT_ARCHIVE_BASE = 'gs://%s' % DEFAULT_ARCHIVE_PREFIX
DEFAULT_BUILDROOT = '/tmp/foo/bar/buildroot'
DEFAULT_BUILDNUMBER = 12345
DEFAULT_BRANCH = 'TheBranch'
DEFAULT_CHROME_BRANCH = 'TheChromeBranch'
DEFAULT_VERSION_STRING = 'TheVersionString'
DEFAULT_BOARD = 'TheBoard'
DEFAULT_BOT_NAME = 'TheCoolBot'

# Access to protected member.
# pylint: disable=W0212

DEFAULT_OPTIONS = cros_test_lib.EasyAttr(
    archive_base=DEFAULT_ARCHIVE_BASE,
    buildroot=DEFAULT_BUILDROOT,
    buildnumber=DEFAULT_BUILDNUMBER,
    buildbot=True,
    branch=DEFAULT_BRANCH,
    remote_trybot=False,
    debug=False,
)
DEFAULT_CONFIG = config_lib.BuildConfig(
    name=DEFAULT_BOT_NAME,
    master=True,
    boards=[DEFAULT_BOARD],
    child_configs=[config_lib.BuildConfig(name='foo'),
                   config_lib.BuildConfig(name='bar'),
                  ],
    gs_path=config_lib.GS_PATH_DEFAULT
)


def _ExtendDefaultOptions(**kwargs):
  """Extend DEFAULT_OPTIONS with keys/values in kwargs."""
  options_kwargs = DEFAULT_OPTIONS.copy()
  options_kwargs.update(kwargs)
  return cros_test_lib.EasyAttr(**options_kwargs)


def _ExtendDefaultConfig(**kwargs):
  """Extend DEFAULT_CONFIG with keys/values in kwargs."""
  config_kwargs = DEFAULT_CONFIG.copy()
  config_kwargs.update(kwargs)
  return config_lib.BuildConfig(**config_kwargs)


def _NewBuilderRun(options=None, config=None):
  """Create a BuilderRun objection from options and config values.

  Args:
    options: Specify options or default to DEFAULT_OPTIONS.
    config: Specify build config or default to DEFAULT_CONFIG.

  Returns:
    BuilderRun object.
  """
  manager = parallel_unittest.FakeMultiprocessManager()
  options = options or DEFAULT_OPTIONS
  config = config or DEFAULT_CONFIG
  site_config = config_lib_unittest.MockSiteConfig()
  site_config[config.name] = config

  return cbuildbot_run.BuilderRun(options, site_config, config, manager)


class GetBaseUploadURITest(cros_test_lib.TestCase):
  """Test the GetBaseUploadURI function."""

  ARCHIVE_BASE = '/tmp/the/archive/base'
  BOT_ID = 'TheNewBotId'

  def setUp(self):
    self.cfg = DEFAULT_CONFIG

  def _GetBaseUploadURI(self, *args, **kwargs):
    """Test GetBaseUploadURI with archive_base and no bot_id."""
    return archive_lib.GetBaseUploadURI(self.cfg, *args, **kwargs)

  def testArchiveBase(self):
    expected_result = '%s/%s' % (self.ARCHIVE_BASE, DEFAULT_BOT_NAME)
    result = self._GetBaseUploadURI(archive_base=self.ARCHIVE_BASE)
    self.assertEqual(expected_result, result)

  def testArchiveBaseBotId(self):
    expected_result = '%s/%s' % (self.ARCHIVE_BASE, self.BOT_ID)
    result = self._GetBaseUploadURI(archive_base=self.ARCHIVE_BASE,
                                    bot_id=self.BOT_ID)
    self.assertEqual(expected_result, result)

  def testBotId(self):
    expected_result = ('%s/%s' %
                       (config_lib.GetConfig().params.ARCHIVE_URL,
                        self.BOT_ID))
    result = self._GetBaseUploadURI(bot_id=self.BOT_ID)
    self.assertEqual(expected_result, result)

  def testDefaultGSPath(self):
    """Test GetBaseUploadURI with default gs_path value in config."""
    self.cfg = _ExtendDefaultConfig(gs_path=config_lib.GS_PATH_DEFAULT)

    # Test without bot_id.
    expected_result = ('%s/%s' %
                       (config_lib.GetConfig().params.ARCHIVE_URL,
                        DEFAULT_BOT_NAME))
    result = self._GetBaseUploadURI()
    self.assertEqual(expected_result, result)

    # Test with bot_id.
    expected_result = ('%s/%s' %
                       (config_lib.GetConfig().params.ARCHIVE_URL,
                        self.BOT_ID))
    result = self._GetBaseUploadURI(bot_id=self.BOT_ID)
    self.assertEqual(expected_result, result)

  def testOverrideGSPath(self):
    """Test GetBaseUploadURI with default gs_path value in config."""
    self.cfg = _ExtendDefaultConfig(gs_path='gs://funkytown/foo/bar')

    # Test without bot_id.
    expected_result = 'gs://funkytown/foo/bar/TheCoolBot'
    result = self._GetBaseUploadURI()
    self.assertEqual(expected_result, result)

    # Test with bot_id.
    expected_result = 'gs://funkytown/foo/bar/TheNewBotId'
    result = self._GetBaseUploadURI(bot_id=self.BOT_ID)
    self.assertEqual(expected_result, result)


class ArchiveTest(cros_test_lib.TestCase):
  """Test the Archive class."""
  _VERSION = '6543.2.1'

  def _GetAttributeValue(self, attr, options=None, config=None):
    with mock.patch.object(cbuildbot_run._BuilderRunBase, 'GetVersion') as m:
      m.return_value = self._VERSION

      run = _NewBuilderRun(options, config)
      return getattr(run.GetArchive(), attr)

  def testVersion(self):
    value = self._GetAttributeValue('version')
    self.assertEqual(self._VERSION, value)

  def testVersionNotReady(self):
    run = _NewBuilderRun()
    self.assertRaises(AttributeError, getattr, run, 'version')

  def testArchivePathTrybot(self):
    options = _ExtendDefaultOptions(buildbot=False)
    value = self._GetAttributeValue('archive_path', options=options)
    expected_value = ('%s/%s/%s/%s' %
                      (DEFAULT_BUILDROOT,
                       archive_lib.Archive._TRYBOT_ARCHIVE,
                       DEFAULT_BOT_NAME,
                       self._VERSION))
    self.assertEqual(expected_value, value)

  def testArchivePathBuildbot(self):
    value = self._GetAttributeValue('archive_path')
    expected_value = ('%s/%s/%s/%s' %
                      (DEFAULT_BUILDROOT,
                       archive_lib.Archive._BUILDBOT_ARCHIVE,
                       DEFAULT_BOT_NAME,
                       self._VERSION))
    self.assertEqual(expected_value, value)

  def testUploadUri(self):
    value = self._GetAttributeValue('upload_url')
    expected_value = '%s/%s/%s' % (DEFAULT_ARCHIVE_BASE,
                                   DEFAULT_BOT_NAME,
                                   self._VERSION)
    self.assertEqual(expected_value, value)

  def testDownloadURLBuildbot(self):
    value = self._GetAttributeValue('download_url')
    expected_value = ('%s%s/%s/%s' %
                      (archive_lib.gs.PRIVATE_BASE_HTTPS_DOWNLOAD_URL,
                       DEFAULT_ARCHIVE_PREFIX,
                       DEFAULT_BOT_NAME,
                       self._VERSION))
    self.assertEqual(expected_value, value)

  def testDownloadURLFileBuildbot(self):
    value = self._GetAttributeValue('download_url_file')
    expected_value = ('%s%s/%s/%s' %
                      (archive_lib.gs.PRIVATE_BASE_HTTPS_URL,
                       DEFAULT_ARCHIVE_PREFIX,
                       DEFAULT_BOT_NAME,
                       self._VERSION))
    self.assertEqual(expected_value, value)
