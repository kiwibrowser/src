# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Chromite main test runner.

Run the specified tests.  If none are specified, we'll scan the
tree looking for tests to run and then only run the semi-fast ones.

You can add a .testignore file to a dir to disable scanning it.
"""

from __future__ import print_function

import errno
import json
import multiprocessing
import os
import signal
import stat
import sys
import tempfile

from chromite.lib import constants
from chromite.lib import cgroups
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import namespaces
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import proctitle
from chromite.lib import timeout_util


# How long (in minutes) to let a test run before we kill it.
TEST_TIMEOUT = 20
# How long (in minutes) before we send SIGKILL after the timeout above.
TEST_SIG_TIMEOUT = 5

# How long (in seconds) to let tests clean up after CTRL+C is sent.
SIGINT_TIMEOUT = 5
# How long (in seconds) to let all children clean up after CTRL+C is sent.
CTRL_C_TIMEOUT = SIGINT_TIMEOUT + 5


# The cache file holds various timing information.  This is used later on to
# optimistically sort tests so the slowest ones run first.  That way we don't
# wait until all of the fast ones finish before we launch the slow ones.
TIMING_CACHE_FILE = None


# Test has to run inside the chroot.
INSIDE = 'inside'

# Test has to run outside the chroot.
OUTSIDE = 'outside'

# Don't run this test (please add a comment as to why).
SKIP = 'skip'


# List all exceptions, with a token describing what's odd here.
SPECIAL_TESTS = {
    # Tests that need to run inside the chroot.
    'cbuildbot/stages/test_stages_unittest': INSIDE,
    'cli/cros/cros_build_unittest': INSIDE,
    'cli/cros/cros_chroot_unittest': INSIDE,
    'cli/cros/cros_debug_unittest': INSIDE,
    'cli/cros/cros_payload_unittest': INSIDE,
    'cli/cros/lint_unittest': INSIDE,
    'cli/cros/lint_autotest_unittest': INSIDE,
    'cli/deploy_unittest': INSIDE,
    'lib/alerts_unittest': INSIDE,
    'lib/androidbuild_unittest': INSIDE,
    'lib/chroot_util_unittest': INSIDE,
    'lib/filetype_unittest': INSIDE,
    'lib/paygen/paygen_payload_lib_unittest': INSIDE,
    'lib/upgrade_table_unittest': INSIDE,
    'mobmonitor/checkfile/manager_unittest': INSIDE,
    'mobmonitor/scripts/mobmonitor_unittest': INSIDE,
    'scripts/cros_mark_android_as_stable_unittest': INSIDE,
    'scripts/cros_install_debug_syms_unittest': INSIDE,
    'scripts/cros_list_modified_packages_unittest': INSIDE,
    'scripts/cros_mark_as_stable_unittest': INSIDE,
    'scripts/cros_mark_chrome_as_stable_unittest': INSIDE,
    'scripts/sync_package_status_unittest': INSIDE,
    'scripts/cros_portage_upgrade_unittest': INSIDE,
    'scripts/dep_tracker_unittest': INSIDE,
    'scripts/test_image_unittest': INSIDE,
    'scripts/upload_package_status_unittest': INSIDE,

    # Tests that need to run outside the chroot.
    'lib/cgroups_unittest': OUTSIDE,

    # Running cros_sdk_unittest through run_tests is triggering
    # a hang inside lvm.
    # TODO(crbug.com/764335): Change this back to OUTSIDE once
    # the lvm hang is resolved.
    'scripts/cros_sdk_unittest': SKIP,

    # Tests that take >2 minutes to run.  All the slow tests are
    # disabled atm though ...
    #'scripts/cros_portage_upgrade_unittest': SKIP,
}

SLOW_TESTS = {
    # Tests that require network can be really slow.
    'buildbot/manifest_version_unittest': SKIP,
    'buildbot/repository_unittest': SKIP,
    'buildbot/remote_try_unittest': SKIP,
    'lib/cros_build_lib_unittest': SKIP,
    'lib/gce_unittest': SKIP,
    'lib/gerrit_unittest': SKIP,
    'lib/patch_unittest': SKIP,

    # cgroups_unittest runs cros_sdk a lot, so is slow.
    'lib/cgroups_unittest': SKIP,
    # cros_sdk_unittest runs cros_sdk a lot, so is slow.
    'scripts/cros_sdk_unittest': SKIP,
    # This involves multiprocess timeout testing.
    'lib/workqueue/tasks_unittest': SKIP,
}


def RunTest(test, cmd, tmpfile, finished, total):
  """Run |test| with the |cmd| line and save output to |tmpfile|.

  Args:
    test: The human readable name for this test.
    cmd: The full command line to run the test.
    tmpfile: File to write test output to.
    finished: Counter to update when this test finishes running.
    total: Total number of tests to run.

  Returns:
    The exit code of the test.
  """
  logging.info('Starting %s', test)

  with cros_build_lib.TimedSection() as timer:
    ret = cros_build_lib.RunCommand(
        cmd, capture_output=True, error_code_ok=True,
        combine_stdout_stderr=True, debug_level=logging.DEBUG,
        int_timeout=SIGINT_TIMEOUT)

  with finished.get_lock():
    finished.value += 1
    if ret.returncode:
      func = logging.error
      msg = 'Failed'
    else:
      func = logging.info
      msg = 'Finished'
    func('%s [%i/%i] %s (%s)', msg, finished.value, total, test, timer.delta)

    # Save the timing for this test run for future usage.
    seconds = timer.delta.total_seconds()
    try:
      cache = json.load(open(TIMING_CACHE_FILE))
    except (IOError, ValueError):
      cache = {}
    if test in cache:
      seconds = (cache[test] + seconds) / 2
    cache[test] = seconds
    json.dump(cache, open(TIMING_CACHE_FILE, 'w'))

  if ret.returncode:
    tmpfile.write(ret.output)
    if not ret.output:
      tmpfile.write('<no output>\n')
  tmpfile.close()

  return ret.returncode


def SortTests(tests, jobs=1, timing_cache_file=None):
  """Interleave the slowest & fastest

  Hopefully we can pipeline the overall process better by queueing the slowest
  tests first while also using half the slots for fast tests.  We don't need
  the timing info to be exact, just ballpark.

  Args:
    tests: The list of tests to sort.
    jobs: How many jobs will we run in parallel.
    timing_cache_file: Where to read test timing info.

  Returns:
    The tests ordered for best execution timing (we hope).
  """
  if timing_cache_file is None:
    timing_cache_file = TIMING_CACHE_FILE

  # Usually |tests| will be a generator -- break it down.
  tests = list(tests)

  # If we have enough spare cpus to crunch the jobs, just do so.
  if len(tests) <= jobs:
    return tests

  # Create a dict mapping tests to their timing information using the cache.
  try:
    with cros_build_lib.Open(timing_cache_file) as f:
      cache = json.load(f)
  except (IOError, ValueError):
    cache = {}

  # Sort the cached list of tests from slowest to fastest.
  sorted_tests = [test for (test, _timing) in
                  sorted(cache.iteritems(), key=lambda x: x[1], reverse=True)]
  # Then extract the tests from the cache list that we care about -- remember
  # that the cache could be stale and contain tests that no longer exist, or
  # the user only wants to run a subset of tests.
  ret = []
  for test in sorted_tests:
    if test in tests:
      ret.append(test)
      tests.remove(test)
  # Any tests not in the cache we just throw on the end.  No real way to
  # predict their speed ahead of time, and we'll get useful data when they
  # run the test a second time.
  ret += tests

  # Now interleave the fast & slow tests so every other one mixes.
  # On systems with fewer cores, this can help out in two ways:
  # (1) Better utilization of resources when some slow tests are I/O or time
  #     bound, so the other cores can spawn/fork fast tests faster (generally).
  # (2) If there is common code that is broken, we get quicker feedback if we
  #     churn through the fast tests.
  # Worse case, this interleaving doesn't slow things down overall.
  fast = ret[:int(round(len(ret) / 2.0)) - 1:-1]
  slow = ret[:-len(fast)]
  ret[::2] = slow
  ret[1::2] = fast
  return ret


def BuildTestSets(tests, chroot_available, network, jobs=1):
  """Build the tests to execute.

  Take care of special test handling like whether it needs to be inside or
  outside of the sdk, whether the test should be skipped, etc...

  Args:
    tests: List of tests to execute.
    chroot_available: Whether we can execute tests inside the sdk.
    network: Whether to execute network tests.
    jobs: How many jobs will we run in parallel.

  Returns:
    List of tests to execute and their full command line.
  """
  testsets = []
  for test in SortTests(tests, jobs=jobs):
    cmd = [test]

    # See if this test requires special consideration.
    status = SPECIAL_TESTS.get(test)
    if status is SKIP:
      logging.info('Skipping %s', test)
      continue
    elif status is INSIDE:
      if not cros_build_lib.IsInsideChroot():
        if not chroot_available:
          logging.info('Skipping %s: chroot not available', test)
          continue
        cmd = ['cros_sdk', '--', os.path.join('..', '..', 'chromite', test)]
    elif status is OUTSIDE:
      if cros_build_lib.IsInsideChroot():
        logging.info('Skipping %s: must be outside the chroot', test)
        continue
    else:
      mode = os.stat(test).st_mode
      if stat.S_ISREG(mode):
        if not mode & 0o111:
          logging.debug('Skipping %s: not executable', test)
          continue
      else:
        logging.debug('Skipping %s: not a regular file', test)
        continue

    # Build up the final test command.
    cmd.append('--verbose')
    if network:
      cmd.append('--network')
    cmd = ['timeout', '--preserve-status', '-k', '%sm' % TEST_SIG_TIMEOUT,
           '%sm' % TEST_TIMEOUT] + cmd

    testsets.append((test, cmd, tempfile.TemporaryFile()))

  return testsets


def RunTests(tests, jobs=1, chroot_available=True, network=False, dryrun=False,
             failfast=False):
  """Execute |paths| with |jobs| in parallel (including |network| tests).

  Args:
    tests: The tests to run.
    jobs: How many tests to run in parallel.
    chroot_available: Whether we can run tests inside the sdk.
    network: Whether to run network based tests.
    dryrun: Do everything but execute the test.
    failfast: Stop on first failure

  Returns:
    True if all tests pass, else False.
  """
  finished = multiprocessing.Value('i')
  testsets = []
  pids = []
  failed = aborted = False

  def WaitOne():
    (pid, status) = os.wait()
    pids.remove(pid)
    return status

  # Launch all the tests!
  try:
    # Build up the testsets.
    testsets = BuildTestSets(tests, chroot_available, network, jobs=jobs)

    # Fork each test and add it to the list.
    for test, cmd, tmpfile in testsets:
      if failed and failfast:
        logging.error('failure detected; stopping new tests')
        break

      if len(pids) >= jobs:
        if WaitOne():
          failed = True
      pid = os.fork()
      if pid == 0:
        proctitle.settitle(test)
        ret = 1
        try:
          if dryrun:
            logging.info('Would have run: %s', cros_build_lib.CmdToStr(cmd))
            ret = 0
          else:
            ret = RunTest(test, cmd, tmpfile, finished, len(testsets))
        except KeyboardInterrupt:
          pass
        except BaseException:
          logging.error('%s failed', test, exc_info=True)
        # We cannot run clean up hooks in the child because it'll break down
        # things like tempdir context managers.
        os._exit(ret)  # pylint: disable=protected-access
      pids.append(pid)

    # Wait for all of them to get cleaned up.
    while pids:
      if WaitOne():
        failed = True

  except KeyboardInterrupt:
    # If the user wants to stop, reap all the pending children.
    logging.warning('CTRL+C received; cleaning up tests')
    aborted = True
    CleanupChildren(pids)

  # Walk through the results.
  failed_tests = []
  for test, cmd, tmpfile in testsets:
    tmpfile.seek(0)
    output = tmpfile.read()
    if output:
      failed_tests.append(test)
      print()
      logging.error('### LOG: %s', test)
      print(output.rstrip())
      print()

  if failed_tests:
    logging.error('The following %i tests failed:\n  %s', len(failed_tests),
                  '\n  '.join(sorted(failed_tests)))
    return False
  elif aborted or failed:
    return False

  return True


def CleanupChildren(pids):
  """Clean up all the children in |pids|."""
  # Note: SIGINT was already sent due to the CTRL+C via the kernel itself.
  # So this func is just waiting for them to clean up.
  handler = signal.signal(signal.SIGINT, signal.SIG_IGN)

  def _CheckWaitpid(ret):
    (pid, _status) = ret
    if pid:
      try:
        pids.remove(pid)
      except ValueError:
        # We might have reaped a grandchild -- be robust.
        pass
    return len(pids)

  def _Waitpid():
    try:
      return os.waitpid(-1, os.WNOHANG)
    except OSError as e:
      if e.errno == errno.ECHILD:
        # All our children went away!
        pids[:] = []
        return (0, 0)
      else:
        raise

  def _RemainingTime(remaining):
    print('\rwaiting %s for %i tests to exit ... ' % (remaining, len(pids)),
          file=sys.stderr, end='')

  try:
    timeout_util.WaitForSuccess(_CheckWaitpid, _Waitpid,
                                timeout=CTRL_C_TIMEOUT, period=0.1,
                                side_effect_func=_RemainingTime)
    print('All tests cleaned up!')
    return
  except timeout_util.TimeoutError:
    # Let's kill them hard now.
    print('Hard killing %i tests' % len(pids))
    for pid in pids:
      try:
        os.kill(pid, signal.SIGKILL)
      except OSError as e:
        if e.errno != errno.ESRCH:
          raise
  finally:
    signal.signal(signal.SIGINT, handler)


def FindTests(search_paths=('.',)):
  """Find all the tests available in |search_paths|."""
  for search_path in search_paths:
    for root, dirs, files in os.walk(search_path):
      if os.path.exists(os.path.join(root, '.testignore')):
        # Delete the dir list in place.
        dirs[:] = []
        continue

      dirs[:] = [x for x in dirs if x[0] != '.']

      for path in files:
        test = os.path.join(os.path.relpath(root, search_path), path)
        if test.endswith('_unittest'):
          yield test


def ChrootAvailable():
  """See if `cros_sdk` will work at all.

  If we try to run unittests in the buildtools group, we won't be able to
  create one.
  """
  ret = cros_build_lib.RunCommand(
      ['repo', 'list'], capture_output=True, error_code_ok=True,
      combine_stdout_stderr=True, debug_level=logging.DEBUG)
  return 'chromiumos-overlay' in ret.output


def _ReExecuteIfNeeded(argv, network):
  """Re-execute as root so we can unshare resources."""
  if os.geteuid() != 0:
    cmd = ['sudo', '-E', 'HOME=%s' % os.environ['HOME'],
           'PATH=%s' % os.environ['PATH'], '--'] + argv
    os.execvp(cmd[0], cmd)
  else:
    cgroups.Cgroup.InitSystem()
    namespaces.SimpleUnshare(net=not network, pid=True)
    # We got our namespaces, so switch back to the user to run the tests.
    gid = int(os.environ.pop('SUDO_GID'))
    uid = int(os.environ.pop('SUDO_UID'))
    user = os.environ.pop('SUDO_USER')
    os.initgroups(user, gid)
    os.setresgid(gid, gid, gid)
    os.setresuid(uid, uid, uid)
    os.environ['USER'] = user


def GetParser():
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('-f', '--failfast', default=False, action='store_true',
                      help='Stop on first failure')
  parser.add_argument('-q', '--quick', default=False, action='store_true',
                      help='Only run the really quick tests')
  parser.add_argument('-n', '--dry-run', default=False, action='store_true',
                      dest='dryrun',
                      help='Do everything but actually run the test')
  parser.add_argument('-l', '--list', default=False, action='store_true',
                      help='List all the available tests')
  parser.add_argument('-j', '--jobs', type=int,
                      help='Number of tests to run in parallel at a time')
  parser.add_argument('--network', default=False, action='store_true',
                      help='Run tests that depend on good network connectivity')
  parser.add_argument('tests', nargs='*', default=None, help='Tests to run')
  return parser


def main(argv):
  parser = GetParser()
  opts = parser.parse_args(argv)
  opts.Freeze()

  # Process list output quickly as it takes no privileges.
  if opts.list:
    print('\n'.join(sorted(opts.tests or FindTests((constants.CHROMITE_DIR,)))))
    return

  # Many of our tests require a valid chroot to run. Make sure it's created
  # before we block network access.
  chroot = os.path.join(constants.SOURCE_ROOT, constants.DEFAULT_CHROOT_DIR)
  if (not os.path.exists(chroot) and
      ChrootAvailable() and
      not cros_build_lib.IsInsideChroot()):
    cros_build_lib.RunCommand(['cros_sdk', '--create'])

  # This is a cheesy hack to make sure gsutil is populated in the cache before
  # we run tests. This is a partial workaround for crbug.com/468838.
  gs.GSContext.GetDefaultGSUtilBin()

  # Now let's run some tests.
  _ReExecuteIfNeeded([sys.argv[0]] + argv, opts.network)
  # A lot of pieces here expect to be run in the root of the chromite tree.
  # Make them happy.
  os.chdir(constants.CHROMITE_DIR)
  tests = opts.tests or FindTests()

  if opts.quick:
    SPECIAL_TESTS.update(SLOW_TESTS)

  global TIMING_CACHE_FILE  # pylint: disable=global-statement
  TIMING_CACHE_FILE = os.path.join(
      path_util.GetCacheDir(), constants.COMMON_CACHE, 'run_tests.cache.json')

  jobs = opts.jobs or multiprocessing.cpu_count()

  with cros_build_lib.ContextManagerStack() as stack:
    # If we're running outside the chroot, try to contain ourselves.
    if cgroups.Cgroup.IsSupported() and not cros_build_lib.IsInsideChroot():
      stack.Add(cgroups.SimpleContainChildren, 'run_tests')

    # Throw all the tests into a custom tempdir so that if we do CTRL+C, we can
    # quickly clean up all the files they might have left behind.
    stack.Add(osutils.TempDir, prefix='chromite.run_tests.', set_global=True,
              sudo_rm=True)

    with cros_build_lib.TimedSection() as timer:
      result = RunTests(
          tests, jobs=jobs, chroot_available=ChrootAvailable(),
          network=opts.network, dryrun=opts.dryrun, failfast=opts.failfast)

    if result:
      logging.info('All tests succeeded! (%s total)', timer.delta)
    else:
      return 1

  if not opts.network:
    logging.warning('Network tests skipped; use --network to run them')
