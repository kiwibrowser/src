# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the cbuildbot_run module."""

from __future__ import print_function

import cPickle
import mock
import time

from chromite.cbuildbot import chromeos_config
from chromite.cbuildbot import cbuildbot_run
from chromite.lib.const import waterfall
from chromite.lib import config_lib
from chromite.lib import config_lib_unittest
from chromite.lib import cros_test_lib
from chromite.lib import parallel


DEFAULT_ARCHIVE_GS_PATH = 'bogus_bucket/TheArchiveBase'
DEFAULT_ARCHIVE_BASE = 'gs://%s' % DEFAULT_ARCHIVE_GS_PATH
DEFAULT_BUILDROOT = '/tmp/foo/bar/buildroot'
DEFAULT_BUILDNUMBER = 12345
DEFAULT_BRANCH = 'TheBranch'
DEFAULT_CHROME_BRANCH = 'TheChromeBranch'
DEFAULT_VERSION_STRING = 'TheVersionString'
DEFAULT_BOARD = 'TheBoard'
DEFAULT_BOT_NAME = 'TheCoolBot'

# pylint: disable=protected-access

DEFAULT_OPTIONS = cros_test_lib.EasyAttr(
    archive_base=DEFAULT_ARCHIVE_BASE,
    buildroot=DEFAULT_BUILDROOT,
    buildnumber=DEFAULT_BUILDNUMBER,
    buildbot=True,
    branch=DEFAULT_BRANCH,
    remote_trybot=False,
    debug=False,
    postsync_patch=True,
)
DEFAULT_CONFIG = config_lib.BuildConfig(
    name=DEFAULT_BOT_NAME,
    master=True,
    boards=[DEFAULT_BOARD],
    postsync_patch=True,
    child_configs=[
        config_lib.BuildConfig(
            name='foo', postsync_patch=False, boards=[]),
        config_lib.BuildConfig(
            name='bar', postsync_patch=False, boards=[]),
    ],
)

DEFAULT_VERSION = '6543.2.1'


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


class ExceptionsTest(cros_test_lib.TestCase):
  """Test that the exceptions in the module are sane."""

  def _TestException(self, err, expected_startswith):
    """Test that str and pickle behavior of |err| are as expected."""
    err2 = cPickle.loads(cPickle.dumps(err, cPickle.HIGHEST_PROTOCOL))

    self.assertTrue(str(err).startswith(expected_startswith))
    self.assertEqual(str(err), str(err2))

  def testParallelAttributeError(self):
    """Test ParallelAttributeError message and pickle behavior."""
    err1 = cbuildbot_run.ParallelAttributeError('SomeAttr')
    self._TestException(err1, 'No such parallel run attribute')

    err2 = cbuildbot_run.ParallelAttributeError('SomeAttr', 'SomeBoard',
                                                'SomeTarget')
    self._TestException(err2, 'No such board-specific parallel run attribute')

  def testAttrSepCountError(self):
    """Test AttrSepCountError message and pickle behavior."""
    err1 = cbuildbot_run.AttrSepCountError('SomeAttr')
    self._TestException(err1, 'Attribute name has an unexpected number')

  def testAttrNotPickleableError(self):
    """Test AttrNotPickleableError message and pickle behavior."""
    err1 = cbuildbot_run.AttrNotPickleableError('SomeAttr', 'SomeValue')
    self._TestException(err1, 'Run attribute "SomeAttr" value cannot')


# TODO(mtennant): Turn this into a PartialMock.
class _BuilderRunTestCase(cros_test_lib.MockTestCase):
  """Provide methods for creating BuilderRun or ChildBuilderRun."""

  def setUp(self):
    self._manager = parallel.Manager()

    # Mimic entering a 'with' statement.
    self._manager.__enter__()

  def tearDown(self):
    # Mimic exiting a 'with' statement.
    self._manager.__exit__(None, None, None)

  def _NewRunAttributes(self):
    return cbuildbot_run.RunAttributes(self._manager)

  def _NewBuilderRun(self, options=None, config=None):
    """Create a BuilderRun objection from options and config values.

    Args:
      options: Specify options or default to DEFAULT_OPTIONS.
      config: Specify build config or default to DEFAULT_CONFIG.

    Returns:
      BuilderRun object.
    """
    options = options or DEFAULT_OPTIONS
    config = config or DEFAULT_CONFIG
    site_config = config_lib_unittest.MockSiteConfig()
    site_config[config.name] = config

    return cbuildbot_run.BuilderRun(options, site_config, config, self._manager)

  def _NewChildBuilderRun(self, child_index, options=None, config=None):
    """Create a ChildBuilderRun objection from options and config values.

    Args:
      child_index: Index of child config to use within config.
      options: Specify options or default to DEFAULT_OPTIONS.
      config: Specify build config or default to DEFAULT_CONFIG.

    Returns:
      ChildBuilderRun object.
    """
    run = self._NewBuilderRun(options, config)
    return cbuildbot_run.ChildBuilderRun(run, child_index)


class BuilderRunPickleTest(_BuilderRunTestCase):
  """Make sure BuilderRun objects can be pickled."""

  def setUp(self):
    self.real_config = chromeos_config.GetConfig()['test-ap-group']
    self.PatchObject(cbuildbot_run._BuilderRunBase, 'GetVersion',
                     return_value=DEFAULT_VERSION)

  def _TestPickle(self, run1):
    self.assertEquals(DEFAULT_VERSION, run1.GetVersion())
    run1.attrs.release_tag = 'TheReleaseTag'

    # Accessing a method on BuilderRun has special behavior, so access and
    # use one before pickling.
    patch_after_sync = run1.ShouldPatchAfterSync()

    # Access the archive object before pickling, too.
    upload_url = run1.GetArchive().upload_url

    # Pickle and unpickle run1 into run2.
    run2 = cPickle.loads(cPickle.dumps(run1, cPickle.HIGHEST_PROTOCOL))

    self.assertEquals(run1.buildnumber, run2.buildnumber)
    self.assertEquals(run1.config.boards, run2.config.boards)
    self.assertEquals(run1.options.branch, run2.options.branch)
    self.assertEquals(run1.attrs.release_tag, run2.attrs.release_tag)
    self.assertRaises(AttributeError, getattr, run1.attrs, 'manifest_manager')
    self.assertRaises(AttributeError, getattr, run2.attrs, 'manifest_manager')
    self.assertEquals(patch_after_sync, run2.ShouldPatchAfterSync())
    self.assertEquals(upload_url, run2.GetArchive().upload_url)

    # The attrs objects should be identical.
    self.assertIs(run1.attrs, run2.attrs)

    # And the run objects themselves are different.
    self.assertIsNot(run1, run2)

  def testPickleBuilderRun(self):
    self._TestPickle(self._NewBuilderRun(config=self.real_config))

  def testPickleChildBuilderRun(self):
    self._TestPickle(self._NewChildBuilderRun(0, config=self.real_config))


class BuilderRunTest(_BuilderRunTestCase):
  """Test the BuilderRun class."""

  def testInit(self):
    with mock.patch.object(cbuildbot_run._BuilderRunBase, 'GetVersion') as m:
      m.return_value = DEFAULT_VERSION

      run = self._NewBuilderRun()
      self.assertEquals(DEFAULT_BUILDROOT, run.buildroot)
      self.assertEquals(DEFAULT_BUILDNUMBER, run.buildnumber)
      self.assertEquals(DEFAULT_BRANCH, run.manifest_branch)
      self.assertEquals(DEFAULT_OPTIONS, run.options)
      self.assertEquals(DEFAULT_CONFIG, run.config)
      self.assertTrue(isinstance(run.attrs, cbuildbot_run.RunAttributes))
      self.assertTrue(isinstance(run.GetArchive(),
                                 cbuildbot_run.archive_lib.Archive))

      # Make sure methods behave normally, since BuilderRun messes with them.
      meth1 = run.GetVersionInfo
      meth2 = run.GetVersionInfo
      self.assertEqual(meth1.__name__, meth2.__name__)

      # We actually do not support identity and equality checks right now.
      self.assertNotEqual(meth1, meth2)
      self.assertIsNot(meth1, meth2)

  def testOptions(self):
    options = _ExtendDefaultOptions(foo=True, bar=10)
    run = self._NewBuilderRun(options=options)

    self.assertEquals(True, run.options.foo)
    self.assertEquals(10, run.options.__getattr__('bar'))
    self.assertRaises(AttributeError, run.options.__getattr__, 'baz')

  def testConfig(self):
    config = _ExtendDefaultConfig(foo=True, bar=10)
    run = self._NewBuilderRun(config=config)

    self.assertEquals(True, run.config.foo)
    self.assertEquals(10, run.config.__getattr__('bar'))
    self.assertRaises(AttributeError, run.config.__getattr__, 'baz')

  def testAttrs(self):
    run = self._NewBuilderRun()

    # manifest_manager is a valid run attribute.  It gives Attribute error
    # if accessed before being set, but thereafter works fine.
    self.assertRaises(AttributeError, run.attrs.__getattribute__,
                      'manifest_manager')
    run.attrs.manifest_manager = 'foo'
    self.assertEquals('foo', run.attrs.manifest_manager)
    self.assertEquals('foo', run.attrs.__getattribute__('manifest_manager'))

    # foobar is not a valid run attribute.  It gives AttributeError when
    # accessed or changed.
    self.assertRaises(AttributeError, run.attrs.__getattribute__, 'foobar')
    self.assertRaises(AttributeError, run.attrs.__setattr__, 'foobar', 'foo')

  def testArchive(self):
    run = self._NewBuilderRun()

    with mock.patch.object(cbuildbot_run._BuilderRunBase, 'GetVersion') as m:
      m.return_value = DEFAULT_VERSION

      archive = run.GetArchive()

      # Check archive.archive_path.
      expected = ('%s/%s/%s/%s' %
                  (DEFAULT_BUILDROOT,
                   cbuildbot_run.archive_lib.Archive._BUILDBOT_ARCHIVE,
                   DEFAULT_BOT_NAME, DEFAULT_VERSION))
      self.assertEqual(expected, archive.archive_path)

      # Check archive.upload_url.
      expected = '%s/%s/%s' % (DEFAULT_ARCHIVE_BASE, DEFAULT_BOT_NAME,
                               DEFAULT_VERSION)
      self.assertEqual(expected, archive.upload_url)

      # Check archive.download_url.
      expected = '%s%s/%s/%s' % (
          cbuildbot_run.archive_lib.gs.PRIVATE_BASE_HTTPS_DOWNLOAD_URL,
          DEFAULT_ARCHIVE_GS_PATH, DEFAULT_BOT_NAME, DEFAULT_VERSION)
      self.assertEqual(expected, archive.download_url)

  def testShouldUploadPrebuilts(self):
    # Enabled
    options = _ExtendDefaultOptions(prebuilts=True)
    config = _ExtendDefaultConfig(prebuilts=True)
    run = self._NewBuilderRun(options=options, config=config)
    self.assertTrue(run.ShouldUploadPrebuilts())

    # Config disabled.
    options = _ExtendDefaultOptions(prebuilts=True)
    config = _ExtendDefaultConfig(prebuilts=False)
    run = self._NewBuilderRun(options=options, config=config)
    self.assertFalse(run.ShouldUploadPrebuilts())

    # Option disabled.
    options = _ExtendDefaultOptions(prebuilts=False)
    config = _ExtendDefaultConfig(prebuilts=True)
    run = self._NewBuilderRun(options=options, config=config)
    self.assertFalse(run.ShouldUploadPrebuilts())

  def testShouldPatchAfterSync(self):
    # Enabled
    options = _ExtendDefaultOptions(postsync_patch=True)
    config = _ExtendDefaultConfig(postsync_patch=True)
    run = self._NewBuilderRun(options=options, config=config)
    self.assertTrue(run.ShouldPatchAfterSync())

    # Config disabled.
    options = _ExtendDefaultOptions(postsync_patch=True)
    config = _ExtendDefaultConfig(postsync_patch=False)
    run = self._NewBuilderRun(options=options, config=config)
    self.assertFalse(run.ShouldPatchAfterSync())

    # Option disabled.
    options = _ExtendDefaultOptions(postsync_patch=False)
    config = _ExtendDefaultConfig(postsync_patch=True)
    run = self._NewBuilderRun(options=options, config=config)
    self.assertFalse(run.ShouldPatchAfterSync())

  def testShouldReexecAfterSync(self):
    # Normal Execution
    options = _ExtendDefaultOptions(postsync_reexec=True, resume=False)
    config = _ExtendDefaultConfig(postsync_reexec=True)
    run = self._NewBuilderRun(options=options, config=config)
    self.assertTrue(run.ShouldReexecAfterSync())

    # Normal after Rexec
    options = _ExtendDefaultOptions(postsync_reexec=True, resume=True)
    config = _ExtendDefaultConfig(postsync_reexec=True)
    run = self._NewBuilderRun(options=options, config=config)
    self.assertFalse(run.ShouldReexecAfterSync())

    # Turned off in config.
    options = _ExtendDefaultOptions(postsync_reexec=True, resume=False)
    config = _ExtendDefaultConfig(postsync_reexec=False)
    run = self._NewBuilderRun(options=options, config=config)
    self.assertFalse(run.ShouldReexecAfterSync())

    # Turned off on command line.
    options = _ExtendDefaultOptions(postsync_reexec=False, resume=False)
    config = _ExtendDefaultConfig(postsync_reexec=True)
    run = self._NewBuilderRun(options=options, config=config)
    self.assertFalse(run.ShouldReexecAfterSync())

  def testInProduction(self):
    run = self._NewBuilderRun()
    self.assertFalse(run.InProduction())

  def testInEmailReportingEnvironment(self):
    run = self._NewBuilderRun()
    self.assertFalse(run.InEmailReportingEnvironment())

    run.attrs.metadata.UpdateWithDict(
        {'buildbot-master-name': waterfall.WATERFALL_INTERNAL})
    self.assertTrue(run.InEmailReportingEnvironment())


class GetVersionTest(_BuilderRunTestCase):
  """Test the GetVersion and GetVersionInfo methods of BuilderRun class."""

  # pylint: disable=protected-access

  def testGetVersionInfoNotSet(self):
    """Verify we throw an error when the version hasn't been set."""
    run = self._NewBuilderRun()
    self.assertRaises(RuntimeError, run.GetVersionInfo)

  def testGetVersionInfo(self):
    """Verify we return the right version info value."""
    # Prepare a real BuilderRun object with a version_info tag.
    run = self._NewBuilderRun()
    verinfo = object()
    run.attrs.version_info = verinfo
    result = run.GetVersionInfo()
    self.assertEquals(verinfo, result)

  def _TestGetVersionReleaseTag(self, release_tag, cidb_id=None):
    with mock.patch.object(cbuildbot_run._BuilderRunBase,
                           'GetVersionInfo') as m:
      verinfo_mock = mock.Mock()
      verinfo_mock.chrome_branch = DEFAULT_CHROME_BRANCH
      verinfo_mock.VersionString = mock.Mock(return_value='VS')
      m.return_value = verinfo_mock

      # Prepare a real BuilderRun object with a release tag.
      run = self._NewBuilderRun()
      run.attrs.release_tag = release_tag
      if cidb_id:
        run.attrs.metadata.UpdateWithDict(
            {'build_id': cidb_id})

      # Run the test return the result.
      result = run.GetVersion()
      m.assert_called_once_with()
      if release_tag is None:
        verinfo_mock.VersionString.assert_called_once()

      return result

  def testGetVersionReleaseTag(self):
    result = self._TestGetVersionReleaseTag('RT')
    self.assertEquals('R%s-%s' % (DEFAULT_CHROME_BRANCH, 'RT'), result)

  def testGetVersionNoReleaseTag(self):
    cidb_id = 12345678
    result = self._TestGetVersionReleaseTag(None, cidb_id)
    expected_result = ('R%s-%s-b%s' %
                       (DEFAULT_CHROME_BRANCH, 'VS', cidb_id))
    self.assertEquals(result, expected_result)

  def testGetVersionNoReleaseTagNoCidb(self):
    result = self._TestGetVersionReleaseTag(None)
    expected_result = ('R%s-%s-b%s' %
                       (DEFAULT_CHROME_BRANCH, 'VS', 0))
    self.assertEquals(result, expected_result)


class ChildBuilderRunTest(_BuilderRunTestCase):
  """Test the ChildBuilderRun class"""

  def testInit(self):
    with mock.patch.object(cbuildbot_run._BuilderRunBase, 'GetVersion') as m:
      m.return_value = DEFAULT_VERSION

      crun = self._NewChildBuilderRun(0)
      self.assertEquals(DEFAULT_BUILDROOT, crun.buildroot)
      self.assertEquals(DEFAULT_BUILDNUMBER, crun.buildnumber)
      self.assertEquals(DEFAULT_BRANCH, crun.manifest_branch)
      self.assertEquals(DEFAULT_OPTIONS, crun.options)
      self.assertEquals(DEFAULT_CONFIG.child_configs[0], crun.config)
      self.assertEquals('foo', crun.config.name)
      self.assertTrue(isinstance(crun.attrs, cbuildbot_run.RunAttributes))
      self.assertTrue(isinstance(crun.GetArchive(),
                                 cbuildbot_run.archive_lib.Archive))

      # Make sure methods behave normally, since BuilderRun messes with them.
      meth1 = crun.GetVersionInfo
      meth2 = crun.GetVersionInfo
      self.assertEqual(meth1.__name__, meth2.__name__)

      # We actually do not support identity and equality checks right now.
      self.assertNotEqual(meth1, meth2)
      self.assertIsNot(meth1, meth2)


class RunAttributesTest(_BuilderRunTestCase):
  """Test the RunAttributes class."""

  BOARD = 'SomeBoard'
  TARGET = 'SomeConfigName'
  VALUE = 'AnyValueWillDo'

  # Any valid board-specific attribute will work here.
  BATTR = 'breakpad_symbols_generated'

  def testRegisterBoardTarget(self):
    """Test behavior of attributes before and after registering board target."""
    ra = self._NewRunAttributes()

    with self.assertRaises(AssertionError):
      ra.HasBoardParallel(self.BATTR, self.BOARD, self.TARGET)

    ra.RegisterBoardAttrs(self.BOARD, self.TARGET)

    self.assertFalse(ra.HasBoardParallel(self.BATTR, self.BOARD, self.TARGET))

    ra.SetBoardParallel(self.BATTR, 'TheValue', self.BOARD, self.TARGET)

    self.assertTrue(ra.HasBoardParallel(self.BATTR, self.BOARD, self.TARGET))

  def testSetGet(self):
    """Test simple set/get of regular and parallel run attributes."""
    ra = self._NewRunAttributes()
    value = 'foobar'

    # The __slots__ logic above confuses pylint.
    # https://bitbucket.org/logilab/pylint/issue/380/
    # pylint: disable=assigning-non-slot

    # Set/Get a regular run attribute using direct access.
    ra.release_tag = value
    self.assertEqual(value, ra.release_tag)

    # Set/Get of a parallel run attribute using direct access fails.
    self.assertRaises(AttributeError, setattr, ra, 'unittest_value', value)
    self.assertRaises(AttributeError, getattr, ra, 'unittest_value')

    # Set/Get of a parallel run attribute with supported interface.
    ra.SetParallel('unittest_value', value)
    self.assertEqual(value, ra.GetParallel('unittest_value'))

    # Set/Get a board parallel run attribute, testing both the encouraged
    # interface and the underlying interface.
    ra.RegisterBoardAttrs(self.BOARD, self.TARGET)
    ra.SetBoardParallel(self.BATTR, value, self.BOARD, self.TARGET)
    self.assertEqual(value,
                     ra.GetBoardParallel(self.BATTR, self.BOARD, self.TARGET))

  def testSetDefault(self):
    """Test setting default value of parallel run attributes."""
    ra = self._NewRunAttributes()
    value = 'foobar'

    # Attribute starts off not set.
    self.assertFalse(ra.HasParallel('unittest_value'))

    # Use SetParallelDefault to set it.
    ra.SetParallelDefault('unittest_value', value)
    self.assertTrue(ra.HasParallel('unittest_value'))
    self.assertEqual(value, ra.GetParallel('unittest_value'))

    # Calling SetParallelDefault again has no effect.
    ra.SetParallelDefault('unittest_value', 'junk')
    self.assertTrue(ra.HasParallel('unittest_value'))
    self.assertEqual(value, ra.GetParallel('unittest_value'))

    # Run through same sequence for a board-specific attribute.
    with self.assertRaises(AssertionError):
      ra.HasBoardParallel(self.BATTR, self.BOARD, self.TARGET)
    ra.RegisterBoardAttrs(self.BOARD, self.TARGET)
    self.assertFalse(ra.HasBoardParallel(self.BATTR, self.BOARD, self.TARGET))

    # Use SetBoardParallelDefault to set it.
    ra.SetBoardParallelDefault(self.BATTR, value, self.BOARD, self.TARGET)
    self.assertTrue(ra.HasBoardParallel(self.BATTR, self.BOARD, self.TARGET))
    self.assertEqual(value,
                     ra.GetBoardParallel(self.BATTR, self.BOARD, self.TARGET))

    # Calling SetBoardParallelDefault again has no effect.
    ra.SetBoardParallelDefault(self.BATTR, 'junk', self.BOARD, self.TARGET)
    self.assertTrue(ra.HasBoardParallel(self.BATTR, self.BOARD, self.TARGET))
    self.assertEqual(value,
                     ra.GetBoardParallel(self.BATTR, self.BOARD, self.TARGET))

  def testAttributeError(self):
    """Test accessing run attributes that do not exist."""
    ra = self._NewRunAttributes()
    value = 'foobar'

    # Set/Get on made up attribute name.
    self.assertRaises(AttributeError, setattr, ra, 'foo', value)
    self.assertRaises(AttributeError, getattr, ra, 'foo')

    # A board/target value is valid, but only if it is registered first.
    self.assertRaises(AssertionError, ra.GetBoardParallel,
                      self.BATTR, self.BOARD, self.TARGET)
    ra.RegisterBoardAttrs(self.BOARD, self.TARGET)
    self.assertRaises(AttributeError, ra.GetBoardParallel,
                      self.BATTR, self.BOARD, self.TARGET)


class BoardRunAttributesTest(_BuilderRunTestCase):
  """Test the BoardRunAttributes class."""

  BOARD = 'SomeBoard'
  TARGET = 'SomeConfigName'
  VALUE = 'AnyValueWillDo'

  # Any valid board-specific attribute will work here.
  BATTR = 'breakpad_symbols_generated'

  class _SetAttr(object):
    """Stage-like class to set attr on a BoardRunAttributes obj."""
    def __init__(self, bra, attr, value, delay=1):
      self.bra = bra
      self.attr = attr
      self.value = value
      self.delay = delay

    def Run(self):
      if self.delay:
        time.sleep(self.delay)
      self.bra.SetParallel(self.attr, self.value)

  class _WaitForAttr(object):
    """Stage-like class to wait for attr on BoardRunAttributes obj."""
    def __init__(self, bra, attr, expected_value, timeout=10):
      self.bra = bra
      self.attr = attr
      self.expected_value = expected_value
      self.timeout = timeout

    def GetParallel(self):
      return self.bra.GetParallel(self.attr, timeout=self.timeout)

  class _CheckWaitForAttr(_WaitForAttr):
    """Stage-like class to wait for then check attr on BoardRunAttributes."""
    def Run(self):
      value = self.GetParallel()
      assert value == self.expected_value, \
          ('For run attribute %s expected value %r but got %r.' %
           (self.attr, self.expected_value, value))

  class _TimeoutWaitForAttr(_WaitForAttr):
    """Stage-like class to time-out waiting for attr on BoardRunAttributes."""
    def Run(self):
      try:
        self.GetParallel()
        assert False, 'Expected AttrTimeoutError'
      except cbuildbot_run.AttrTimeoutError:
        pass

  def setUp(self):
    self.ra = self._NewRunAttributes()
    self.bra = self.ra.RegisterBoardAttrs(self.BOARD, self.TARGET)

  def _TestParallelSetGet(self, stage_args):
    """Helper to run "stages" in parallel, according to |stage_args|.

    Args:
      stage_args: List of tuples of the form (stage_object, extra_args, ...)
        where stage_object has a Run method which takes a BoardRunAttributes
        object as the first argument and extra_args for the remaining arguments.
    """
    stages = [a[0](self.bra, *a[1:]) for a in stage_args]
    steps = [stage.Run for stage in stages]

    parallel.RunParallelSteps(steps)

  def testParallelSetGetFast(self):
    """Pass the parallel run attribute around with no delay."""
    stage_args = [
        (self._CheckWaitForAttr, self.BATTR, self.VALUE),
        (self._SetAttr, self.BATTR, self.VALUE),
    ]
    self._TestParallelSetGet(stage_args)
    self.assertRaises(AttributeError,
                      getattr, self.bra, self.BATTR)
    self.assertEqual(self.VALUE, self.bra.GetParallel(self.BATTR))

  def testParallelSetGetSlow(self):
    """Pass the parallel run attribute around with a delay."""
    stage_args = [
        (self._SetAttr, self.BATTR, self.VALUE, 10),
        (self._TimeoutWaitForAttr, self.BATTR, self.VALUE, 2),
    ]
    self._TestParallelSetGet(stage_args)
    self.assertEqual(self.VALUE, self.bra.GetParallel(self.BATTR))

  def testParallelSetGetManyGets(self):
    """Set the parallel run attribute in one stage, access in many stages."""
    stage_args = [
        (self._SetAttr, self.BATTR, self.VALUE, 8),
        (self._CheckWaitForAttr, self.BATTR, self.VALUE, 16),
        (self._CheckWaitForAttr, self.BATTR, self.VALUE, 16),
        (self._CheckWaitForAttr, self.BATTR, self.VALUE, 16),
        (self._TimeoutWaitForAttr, self.BATTR, self.VALUE, 1),
    ]
    self._TestParallelSetGet(stage_args)
    self.assertEqual(self.VALUE, self.bra.GetParallel(self.BATTR))

  def testParallelSetGetManySets(self):
    """Set the parallel run attribute in many stages, access in one stage."""
    # Three "stages" set the value, with increasing delays.  The stage that
    # checks the value should get the first value set.
    stage_args = [
        (self._SetAttr, self.BATTR, self.VALUE + '1', 1),
        (self._SetAttr, self.BATTR, self.VALUE + '2', 11),
        (self._CheckWaitForAttr, self.BATTR, self.VALUE + '1', 12),
    ]
    self._TestParallelSetGet(stage_args)
    self.assertEqual(self.VALUE + '2', self.bra.GetParallel(self.BATTR))

  def testSetGet(self):
    """Test that board-specific attrs do not work with set/get directly."""
    self.assertRaises(AttributeError, setattr,
                      self.bra, 'breakpad_symbols_generated', self.VALUE)
    self.assertRaises(AttributeError, getattr,
                      self.bra, 'breakpad_symbols_generated')

  def testAccessRegularRunAttr(self):
    """Test that regular attributes are not known to BoardRunAttributes."""
    self.assertRaises(AttributeError, getattr, self.bra, 'release_tag')
    self.assertRaises(AttributeError, setattr, self.bra, 'release_tag', 'foo')
