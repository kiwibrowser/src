# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for the deploy module."""

from __future__ import print_function

import json
import multiprocessing
import os

from chromite.cli import command
from chromite.cli import deploy
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import portage_util
from chromite.lib import remote_access
try:
  import portage
except ImportError:
  if cros_build_lib.IsInsideChroot():
    raise


# pylint: disable=protected-access


class ChromiumOSDeviceFake(object):
  """Fake for device."""

  def __init__(self):
    self.board = 'board'
    self.hostname = None
    self.username = None
    self.port = None
    self.lsb_release = None

  def IsDirWritable(self, _):
    return True


class ChromiumOSDeviceHandlerFake(object):
  """Fake for chromite.lib.remote_access.ChomiumOSDeviceHandler."""

  class RemoteAccessFake(object):
    """Fake for chromite.lib.remote_access.RemoteAccess."""

    def __init__(self):
      self.remote_sh_output = None

    def RemoteSh(self, *_args, **_kwargs):
      return cros_build_lib.CommandResult(output=self.remote_sh_output)

  def __init__(self, *_args, **_kwargs):
    self._agent = self.RemoteAccessFake()
    self.device = ChromiumOSDeviceFake()

  # TODO(dpursell): Mock remote access object in cros_test_lib (brbug.com/986).
  def GetAgent(self):
    return self._agent

  def __exit__(self, _type, _value, _traceback):
    pass

  def __enter__(self):
    return ChromiumOSDeviceFake()


class BrilloDeployOperationFake(deploy.BrilloDeployOperation):
  """Fake for deploy.BrilloDeployOperation."""
  def __init__(self, pkg_count, emerge, queue):
    super(BrilloDeployOperationFake, self).__init__(pkg_count, emerge)
    self._queue = queue

  def ParseOutput(self, output=None):
    super(BrilloDeployOperationFake, self).ParseOutput(output)
    self._queue.put('advance')


class DbApiFake(object):
  """Fake for Portage dbapi."""

  def __init__(self, pkgs):
    self.pkg_db = {}
    for cpv, slot, rdeps_raw, build_time in pkgs:
      self.pkg_db[cpv] = {
          'SLOT': slot, 'RDEPEND': rdeps_raw, 'BUILD_TIME': build_time}

  def cpv_all(self):
    return self.pkg_db.keys()

  def aux_get(self, cpv, keys):
    pkg_info = self.pkg_db[cpv]
    return [pkg_info[key] for key in keys]


class PackageScannerFake(object):
  """Fake for PackageScanner."""

  def __init__(self, packages):
    self.pkgs = packages
    self.listed = []
    self.num_updates = None

  def Run(self, _device, _root, _packages, _update, _deep, _deep_rev):
    return self.pkgs, self.listed, self.num_updates


class PortageTreeFake(object):
  """Fake for Portage tree."""

  def __init__(self, dbapi):
    self.dbapi = dbapi


class TestInstallPackageScanner(cros_test_lib.MockOutputTestCase):
  """Test the update package scanner."""
  _BOARD = 'foo_board'
  _BUILD_ROOT = '/build/%s' % _BOARD
  _VARTREE = [
      ('foo/app1-1.2.3-r4', '0', 'foo/app2 !foo/app3', '1413309336'),
      ('foo/app2-4.5.6-r7', '0', '', '1413309336'),
      ('foo/app4-2.0.0-r1', '0', 'foo/app1 foo/app5', '1413309336'),
      ('foo/app5-3.0.7-r3', '0', '', '1413309336'),
  ]

  def setUp(self):
    """Patch imported modules."""
    self.PatchObject(cros_build_lib, 'GetChoice', return_value=0)
    self.device = ChromiumOSDeviceHandlerFake()
    self.scanner = deploy._InstallPackageScanner(self._BUILD_ROOT)

  def SetupVartree(self, vartree_pkgs):
    self.device.GetAgent().remote_sh_output = json.dumps(vartree_pkgs)

  def SetupBintree(self, bintree_pkgs):
    bintree = PortageTreeFake(DbApiFake(bintree_pkgs))
    build_root = os.path.join(self._BUILD_ROOT, '')
    portage_db = {build_root: {'bintree': bintree}}
    self.PatchObject(portage, 'create_trees', return_value=portage_db)

  def ValidatePkgs(self, actual, expected, constraints=None):
    # Containing exactly the same packages.
    self.assertEquals(sorted(expected), sorted(actual))
    # Packages appear in the right order.
    if constraints is not None:
      for needs, needed in constraints:
        self.assertGreater(actual.index(needs), actual.index(needed))

  def testRunUpdatedVersion(self):
    self.SetupVartree(self._VARTREE)
    app1 = 'foo/app1-1.2.5-r4'
    self.SetupBintree([
        (app1, '0', 'foo/app2 !foo/app3', '1413309336'),
        ('foo/app2-4.5.6-r7', '0', '', '1413309336'),
    ])
    installs, listed, num_updates = self.scanner.Run(
        self.device, '/', ['app1'], True, True, True)
    self.ValidatePkgs(installs, [app1])
    self.ValidatePkgs(listed, [app1])
    self.assertEquals(num_updates, 1)

  def testRunUpdatedBuildTime(self):
    self.SetupVartree(self._VARTREE)
    app1 = 'foo/app1-1.2.3-r4'
    self.SetupBintree([
        (app1, '0', 'foo/app2 !foo/app3', '1413309350'),
        ('foo/app2-4.5.6-r7', '0', '', '1413309336'),
    ])
    installs, listed, num_updates = self.scanner.Run(
        self.device, '/', ['app1'], True, True, True)
    self.ValidatePkgs(installs, [app1])
    self.ValidatePkgs(listed, [app1])
    self.assertEquals(num_updates, 1)

  def testRunExistingDepUpdated(self):
    self.SetupVartree(self._VARTREE)
    app1 = 'foo/app1-1.2.5-r2'
    app2 = 'foo/app2-4.5.8-r3'
    self.SetupBintree([
        (app1, '0', 'foo/app2 !foo/app3', '1413309350'),
        (app2, '0', '', '1413309350'),
    ])
    installs, listed, num_updates = self.scanner.Run(
        self.device, '/', ['app1'], True, True, True)
    self.ValidatePkgs(installs, [app1, app2], constraints=[(app1, app2)])
    self.ValidatePkgs(listed, [app1])
    self.assertEquals(num_updates, 2)

  def testRunMissingDepUpdated(self):
    self.SetupVartree(self._VARTREE)
    app1 = 'foo/app1-1.2.5-r2'
    app6 = 'foo/app6-1.0.0-r1'
    self.SetupBintree([
        (app1, '0', 'foo/app2 !foo/app3 foo/app6', '1413309350'),
        ('foo/app2-4.5.6-r7', '0', '', '1413309336'),
        (app6, '0', '', '1413309350'),
    ])
    installs, listed, num_updates = self.scanner.Run(
        self.device, '/', ['app1'], True, True, True)
    self.ValidatePkgs(installs, [app1, app6], constraints=[(app1, app6)])
    self.ValidatePkgs(listed, [app1])
    self.assertEquals(num_updates, 1)

  def testRunExistingRevDepUpdated(self):
    self.SetupVartree(self._VARTREE)
    app1 = 'foo/app1-1.2.5-r2'
    app4 = 'foo/app4-2.0.1-r3'
    self.SetupBintree([
        (app1, '0', 'foo/app2 !foo/app3', '1413309350'),
        (app4, '0', 'foo/app1 foo/app5', '1413309350'),
        ('foo/app5-3.0.7-r3', '0', '', '1413309336'),
    ])
    installs, listed, num_updates = self.scanner.Run(
        self.device, '/', ['app1'], True, True, True)
    self.ValidatePkgs(installs, [app1, app4], constraints=[(app4, app1)])
    self.ValidatePkgs(listed, [app1])
    self.assertEquals(num_updates, 2)

  def testRunMissingRevDepNotUpdated(self):
    self.SetupVartree(self._VARTREE)
    app1 = 'foo/app1-1.2.5-r2'
    app6 = 'foo/app6-1.0.0-r1'
    self.SetupBintree([
        (app1, '0', 'foo/app2 !foo/app3', '1413309350'),
        (app6, '0', 'foo/app1', '1413309350'),
    ])
    installs, listed, num_updates = self.scanner.Run(
        self.device, '/', ['app1'], True, True, True)
    self.ValidatePkgs(installs, [app1])
    self.ValidatePkgs(listed, [app1])
    self.assertEquals(num_updates, 1)

  def testRunTransitiveDepsUpdated(self):
    self.SetupVartree(self._VARTREE)
    app1 = 'foo/app1-1.2.5-r2'
    app2 = 'foo/app2-4.5.8-r3'
    app4 = 'foo/app4-2.0.0-r1'
    app5 = 'foo/app5-3.0.8-r2'
    self.SetupBintree([
        (app1, '0', 'foo/app2 !foo/app3', '1413309350'),
        (app2, '0', '', '1413309350'),
        (app4, '0', 'foo/app1 foo/app5', '1413309350'),
        (app5, '0', '', '1413309350'),
    ])
    installs, listed, num_updates = self.scanner.Run(
        self.device, '/', ['app1'], True, True, True)
    self.ValidatePkgs(installs, [app1, app2, app4, app5],
                      constraints=[(app1, app2), (app4, app1), (app4, app5)])
    self.ValidatePkgs(listed, [app1])
    self.assertEquals(num_updates, 4)

  def testRunDisjunctiveDepsExistingUpdated(self):
    self.SetupVartree(self._VARTREE)
    app1 = 'foo/app1-1.2.5-r2'
    self.SetupBintree([
        (app1, '0', '|| ( foo/app6 foo/app2 ) !foo/app3', '1413309350'),
        ('foo/app2-4.5.6-r7', '0', '', '1413309336'),
    ])
    installs, listed, num_updates = self.scanner.Run(
        self.device, '/', ['app1'], True, True, True)
    self.ValidatePkgs(installs, [app1])
    self.ValidatePkgs(listed, [app1])
    self.assertEquals(num_updates, 1)

  def testRunDisjunctiveDepsDefaultUpdated(self):
    self.SetupVartree(self._VARTREE)
    app1 = 'foo/app1-1.2.5-r2'
    app7 = 'foo/app7-1.0.0-r1'
    self.SetupBintree([
        (app1, '0', '|| ( foo/app6 foo/app7 ) !foo/app3', '1413309350'),
        (app7, '0', '', '1413309350'),
    ])
    installs, listed, num_updates = self.scanner.Run(
        self.device, '/', ['app1'], True, True, True)
    self.ValidatePkgs(installs, [app1, app7], constraints=[(app1, app7)])
    self.ValidatePkgs(listed, [app1])
    self.assertEquals(num_updates, 1)


class TestDeploy(cros_test_lib.ProgressBarTestCase):
  """Test deploy.Deploy."""

  @staticmethod
  def FakeGetPackagesByCPV(cpvs, _strip, _sysroot):
    return ['/path/to/%s.tbz2' % cpv.pv for cpv in cpvs]

  def setUp(self):
    self.PatchObject(remote_access, 'ChromiumOSDeviceHandler',
                     side_effect=ChromiumOSDeviceHandlerFake)
    self.PatchObject(cros_build_lib, 'GetBoard', return_value=None)
    self.PatchObject(cros_build_lib, 'GetSysroot', return_value='sysroot')
    self.package_scanner = self.PatchObject(deploy, '_InstallPackageScanner')
    self.get_packages_paths = self.PatchObject(
        deploy, '_GetPackagesByCPV', side_effect=self.FakeGetPackagesByCPV)
    self.emerge = self.PatchObject(deploy, '_Emerge', return_value=None)
    self.unmerge = self.PatchObject(deploy, '_Unmerge', return_value=None)

  def testDeployEmerge(self):
    """Test that deploy._Emerge is called for each package."""

    _BINPKG = '/path/to/bar-1.2.5.tbz2'
    def FakeIsFile(fname):
      return fname == _BINPKG

    packages = ['some/foo-1.2.3', _BINPKG, 'some/foobar-2.0']
    self.package_scanner.return_value = PackageScannerFake(packages)
    self.PatchObject(os.path, 'isfile', side_effect=FakeIsFile)

    deploy.Deploy(None, ['package'], force=True, clean_binpkg=False)

    # Check that package names were correctly resolved into binary packages.
    self.get_packages_paths.assert_called_once_with(
        [portage_util.SplitCPV(p) for p in packages if p != _BINPKG],
        True, 'sysroot')
    # Check that deploy._Emerge is called the right number of times.
    self.assertEqual(self.emerge.call_count, len(packages))
    self.assertEqual(self.unmerge.call_count, 0)

  def testDeployUnmerge(self):
    """Test that deploy._Unmerge is called for each package."""
    packages = ['foo', 'bar', 'foobar']
    self.package_scanner.return_value = PackageScannerFake(packages)

    deploy.Deploy(None, ['package'], force=True, clean_binpkg=False,
                  emerge=False)

    # Check that deploy._Unmerge is called the right number of times.
    self.assertEqual(self.emerge.call_count, 0)
    self.assertEqual(self.unmerge.call_count, len(packages))

  def testDeployMergeWithProgressBar(self):
    """Test that BrilloDeployOperation.Run() is called for merge."""
    packages = ['foo', 'bar', 'foobar']
    self.package_scanner.return_value = PackageScannerFake(packages)

    run = self.PatchObject(deploy.BrilloDeployOperation, 'Run',
                           return_value=None)

    self.PatchObject(command, 'UseProgressBar', return_value=True)
    deploy.Deploy(None, ['package'], force=True, clean_binpkg=False)

    # Check that BrilloDeployOperation.Run was called.
    self.assertTrue(run.called)

  def testDeployUnmergeWithProgressBar(self):
    """Test that BrilloDeployOperation.Run() is called for unmerge."""
    packages = ['foo', 'bar', 'foobar']
    self.package_scanner.return_value = PackageScannerFake(packages)

    run = self.PatchObject(deploy.BrilloDeployOperation, 'Run',
                           return_value=None)

    self.PatchObject(command, 'UseProgressBar', return_value=True)
    deploy.Deploy(None, ['package'], force=True, clean_binpkg=False,
                  emerge=False)

    # Check that BrilloDeployOperation.Run was called.
    self.assertTrue(run.called)

  def testBrilloDeployMergeOperation(self):
    """Test that BrilloDeployOperation works for merge."""
    def func(queue):
      for event in op.MERGE_EVENTS:
        queue.get()
        print(event)

    queue = multiprocessing.Queue()
    # Emerge one package.
    op = BrilloDeployOperationFake(1, True, queue)

    with self.OutputCapturer():
      op.Run(func, queue)

    # Check that the progress bar prints correctly.
    self.AssertProgressBarAllEvents(len(op.MERGE_EVENTS))

  def testBrilloDeployUnmergeOperation(self):
    """Test that BrilloDeployOperation works for unmerge."""
    def func(queue):
      for event in op.UNMERGE_EVENTS:
        queue.get()
        print(event)

    queue = multiprocessing.Queue()
    # Unmerge one package.
    op = BrilloDeployOperationFake(1, False, queue)

    with self.OutputCapturer():
      op.Run(func, queue)

    # Check that the progress bar prints correctly.
    self.AssertProgressBarAllEvents(len(op.UNMERGE_EVENTS))
