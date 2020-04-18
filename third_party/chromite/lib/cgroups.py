# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A class for managing the Linux cgroup subsystem."""

from __future__ import print_function

import errno
import os
import signal
import time

from chromite.lib import cros_build_lib
from chromite.lib import locking
from chromite.lib import osutils
from chromite.lib import signals
from chromite.lib import sudo


# Rough hierarchy sketch:
# - all cgroup aware cros code should nest here.
# - No cros code should modify this namespace- this is user/system configurable
# - only.  A release_agent can be specified, although we won't use it.
# cros/
#
# - cbuildbot instances land here only when they're cleaning their task pool.
# - this root namespace is *not* auto-removed; it's left so that user/system
# - configuration is persistant.
# cros/%(process-name)s/
# cros/cbuildbot/
#
# - a cbuildbot job pool, owned by pid.  These are autocleaned.
# cros/cbuildbot/%(pid)i/
#
# - a job pool using process that was invoked by cbuildbot.
# - for example, cros/cbuildbot/42/cros_sdk:34
# - this pattern continues arbitrarily deep, and is autocleaned.
# cros/cbuildbot/%(pid1)i/%(basename_of_pid2)s:%(pid2)i/
#
# An example for cros_sdk (pid 552) would be:
# cros/cros_sdk/552/
# and it's children would be accessible in 552/tasks, or
# would create their own namespace w/in and assign themselves to it.


class _GroupWasRemoved(Exception):
  """Exception representing when a group was unexpectedly removed.

  Via design, this should only be possible when instantiating a new
  pool, but the parent pool has been removed- this means effectively that
  we're supposed to shutdown (either we've been sigterm'd and ignored it,
  or it's imminent).
  """


def _FileContains(filename, strings):
  """Greps a group of expressions, returns whether all were found."""
  contents = osutils.ReadFile(filename)
  return all(s in contents for s in strings)


def EnsureInitialized(functor):
  """Decorator for Cgroup methods to ensure the method is ran only if inited"""

  def f(self, *args, **kwargs):
    # pylint: disable=W0212
    self.Instantiate()
    return functor(self, *args, **kwargs)

  # Dummy up our wrapper to make it look like what we're wrapping,
  # and expose the underlying docstrings.
  f.__name__ = functor.__name__
  f.__doc__ = functor.__doc__
  f.__module__ = functor.__module__
  return f


class Cgroup(object):

  """Class representing a group in cgroups hierarchy.

  Note the instance may not exist on disk; it will be created as necessary.
  Additionally, because cgroups is kernel maintained (and mutated on the fly
  by processes using it), chunks of this class are /explicitly/ designed to
  always go back to disk and recalculate values.

  Attributes:
    path: Absolute on disk pathway to the cgroup directory.
    tasks: Pids contained in this immediate cgroup, and the owning pids of
      any first level groups nested w/in us.
    all_tasks: All Pids, and owners of nested groups w/in this point in
      the hierarchy.
    nested_groups: The immediate cgroups nested w/in this one.  If this
      cgroup is 'cbuildbot/buildbot', 'cbuildbot' would have a nested_groups
      of [Cgroup('cbuildbot/buildbot')] for example.
    all_nested_groups: All cgroups nested w/in this one, regardless of depth.
    pid_owner: Which pid owns this cgroup, if the cgroup is following cros
      conventions for group naming.
  """

  NEEDED_SUBSYSTEMS = ('cpuset',)
  PROC_PATH = '/proc/cgroups'
  _MOUNT_ROOT_POTENTIALS = ('/sys/fs/cgroup/cpuset', '/sys/fs/cgroup')
  _MOUNT_ROOT_FALLBACK = '/dev/cgroup'
  CGROUP_ROOT = None
  MOUNT_ROOT = None
  # Whether or not the cgroup implementation does auto inheritance via
  # cgroup.clone_children
  _SUPPORTS_AUTOINHERIT = False

  @classmethod
  @cros_build_lib.MemoizedSingleCall
  def InitSystem(cls):
    """If cgroups are supported, initialize the system state"""
    if not cls.IsSupported():
      return False

    def _EnsureMounted(mnt, args):
      for mtab in osutils.IterateMountPoints():
        if mtab.destination == mnt:
          return True

      # Grab a lock so in the off chance we have multiple programs (like two
      # cros_sdk launched in parallel) running this init logic, we don't end
      # up mounting multiple times.
      lock_path = '/tmp/.chromite.cgroups.lock'
      with locking.FileLock(lock_path, 'cgroup lock') as lock:
        lock.write_lock()
        for mtab in osutils.IterateMountPoints():
          if mtab.destination == mnt:
            return True

        # Not all distros mount cgroup_root to sysfs.
        osutils.SafeMakedirs(mnt, sudo=True)
        cros_build_lib.SudoRunCommand(['mount'] + args + [mnt], print_cmd=False)

      return True

    mount_root_args = ['-t', 'tmpfs', 'cgroup_root']

    opts = ','.join(cls.NEEDED_SUBSYSTEMS)
    cgroup_root_args = ['-t', 'cgroup', '-o', opts, 'cros']

    return _EnsureMounted(cls.MOUNT_ROOT, mount_root_args) and \
        _EnsureMounted(cls.CGROUP_ROOT, cgroup_root_args)

  @classmethod
  @cros_build_lib.MemoizedSingleCall
  def IsUsable(cls):
    """Function to sanity check if everything is setup to use cgroups"""
    if not cls.InitSystem():
      return False
    cls._SUPPORTS_AUTOINHERIT = os.path.exists(
        os.path.join(cls.CGROUP_ROOT, 'cgroup.clone_children'))
    return True

  @classmethod
  @cros_build_lib.MemoizedSingleCall
  def IsSupported(cls):
    """Sanity check as to whether or not cgroups are supported."""
    # Is the cgroup subsystem even enabled?

    if not os.path.exists(cls.PROC_PATH):
      return False

    # Does it support the subsystems we want?
    if not _FileContains(cls.PROC_PATH, cls.NEEDED_SUBSYSTEMS):
      return False

    for potential in cls._MOUNT_ROOT_POTENTIALS:
      if os.path.exists(potential):
        cls.MOUNT_ROOT = potential
        break
    else:
      cls.MOUNT_ROOT = cls._MOUNT_ROOT_FALLBACK
    cls.MOUNT_ROOT = os.path.realpath(cls.MOUNT_ROOT)

    cls.CGROUP_ROOT = os.path.join(cls.MOUNT_ROOT, 'cros')
    return True

  def __init__(self, namespace, autoclean=True, lazy_init=False, parent=None,
               _is_root=False, _overwrite=True):
    """Initalize a cgroup instance.

    Args:
      namespace: What cgroup namespace is this in?  cbuildbot/1823 for example.
      autoclean: Should this cgroup be removed once unused?
      lazy_init: Should we create the cgroup immediately, or when needed?
      parent: A Cgroup instance; if the namespace is cbuildbot/1823, then the
        parent *must* be the cgroup instance for namespace cbuildbot.
      _is_root:  Internal option, shouldn't be used by consuming code.
      _overwrite: Internal option, shouldn't be used by consuming code.
    """
    self._inited = None
    self._overwrite = bool(_overwrite)
    if _is_root:
      namespace = '.'
      self._inited = True
    else:
      namespace = os.path.normpath(namespace)
      if parent is None:
        raise ValueError("Either _is_root must be set to True, or parent must "
                         "be non null")
      if namespace in ('.', ''):
        raise ValueError("Invalid namespace %r was given" % (namespace,))

    self.namespace = namespace
    self.autoclean = autoclean
    self.parent = parent

    if not lazy_init:
      self.Instantiate()

  def _LimitName(self, name, for_path=False, multilevel=False):
    """Translation function doing sanity checks on derivative namespaces

    If you're extending this class, you should be using this for any namespace
    operations that pass through a nested group.
    """
    # We use a fake pathway here, and this code must do so.  To calculate the
    # real pathway requires knowing CGROUP_ROOT, which requires sudo
    # potentially.  Since this code may be invoked just by loading the module,
    # no execution/sudo should occur.  However, if for_path is set, we *do*
    # require CGROUP_ROOT- which is fine, since we sort that on the way out.
    fake_path = os.path.normpath(os.path.join('/fake-path', self.namespace))
    path = os.path.normpath(os.path.join(fake_path, name))

    # Ensure that the requested pathway isn't trying to sidestep what we
    # expect, and in the process it does internal validation checks.
    if not path.startswith(fake_path + '/'):
      raise ValueError("Name %s tried descending through this namespace into"
                       " another; this isn't allowed." % (name,))
    elif path == self.namespace:
      raise ValueError("Empty name %s" % (name,))
    elif os.path.dirname(path) != fake_path and not multilevel:
      raise ValueError("Name %s is multilevel, but disallowed." % (name,))

    # Get the validated/normalized name.
    name = path[len(fake_path):].strip('/')
    if for_path:
      return os.path.join(self.path, name)
    return name

  @property
  def path(self):
    return os.path.abspath(os.path.join(self.CGROUP_ROOT, self.namespace))

  @property
  def tasks(self):
    s = set(x.strip() for x in self.GetValue('tasks', '').splitlines())
    s.update(x.pid_owner for x in self.nested_groups)
    s.discard(None)
    return s

  @property
  def all_tasks(self):
    s = self.tasks
    for group in self.all_nested_groups:
      s.update(group.tasks)
    return s

  @property
  def nested_groups(self):
    targets = []
    path = self.path
    try:
      targets = [x for x in os.listdir(path)
                 if os.path.isdir(os.path.join(path, x))]
    except EnvironmentError as e:
      if e.errno != errno.ENOENT:
        raise

    targets = [self.AddGroup(x, lazy_init=True, _overwrite=False)
               for x in targets]

    # Suppress initialization checks- if it exists on disk, we know it
    # is already initialized.
    for x in targets:
      # pylint: disable=protected-access
      x._inited = True
    return targets

  @property
  def all_nested_groups(self):
    # Do a depth first traversal.
    def walk(groups):
      for group in groups:
        for subgroup in walk(group.nested_groups):
          yield subgroup
        yield group
    return list(walk(self.nested_groups))

  @property
  @cros_build_lib.MemoizedSingleCall
  def pid_owner(self):
    # Ensure it's in cros namespace- if it is outside of the cros namespace,
    # we shouldn't make assumptions about the naming convention used.
    if not self.GroupIsAParent(_cros_node):
      return None
    # See documentation at the top of the file for the naming scheme.
    # It's basically "%(program_name)s:%(owning_pid)i" if the group
    # is nested.
    return os.path.basename(self.namespace).rsplit(':', 1)[-1]

  def GroupIsAParent(self, node):
    """Is the given node a parent of us?"""
    parent_path = node.path + '/'
    return self.path.startswith(parent_path)

  def GetValue(self, key, default=None):
    """Query a cgroup configuration key from disk.

    If the file doesn't exist, return the given default.
    """
    try:
      return osutils.ReadFile(os.path.join(self.path, key))
    except EnvironmentError as e:
      if e.errno != errno.ENOENT:
        raise
      return default

  def _AddSingleGroup(self, name, **kwargs):
    """Method for creating a node nested within this one.

    Derivative classes should override this method rather than AddGroup;
    see __init__ for the supported keywords.
    """
    return self.__class__(os.path.join(self.namespace, name), **kwargs)

  def AddGroup(self, name, **kwargs):
    """Add and return a cgroup nested in this one.

    See __init__ for the supported keywords.  If this isn't a direct child
    (for example this instance is cbuildbot, and the name is 1823/x), it'll
    create the intermediate groups as lazy_init=True, setting autoclean to
    via the logic described for autoclean_parents below.

    Args:
      name: Name of group to add.
      autoclean_parents: Optional keyword argument; if unspecified, it takes
        the value of autoclean (or True if autoclean isn't specified).  This
        controls whether any intermediate nodes that must be created for
        multilevel groups are autocleaned.
    """
    name = self._LimitName(name, multilevel=True)

    autoclean = kwargs.pop('autoclean', True)
    autoclean_parents = kwargs.pop('autoclean_parents', autoclean)
    chunks = name.split('/', 1)
    node = self
    # pylint: disable=W0212
    for chunk in chunks[:-1]:
      node = node._AddSingleGroup(chunk, parent=node,
                                  autoclean=autoclean_parents, **kwargs)
    return node._AddSingleGroup(chunks[-1], parent=node,
                                autoclean=autoclean, **kwargs)

  @cros_build_lib.MemoizedSingleCall
  def Instantiate(self):
    """Ensure this group exists on disk in the cgroup hierarchy"""

    if self.namespace == '.':
      # If it's the root of the hierarchy, leave it alone.
      return True

    if self.parent is not None:
      self.parent.Instantiate()
    osutils.SafeMakedirs(self.path, sudo=True)

    force_inheritance = True
    if self.parent.GetValue('cgroup.clone_children', '').strip() == '1':
      force_inheritance = False

    if force_inheritance:
      if self._SUPPORTS_AUTOINHERIT:
        # If the cgroup version supports it, flip the auto-inheritance setting
        # on so that cgroups nested here don't have to manually transfer
        # settings
        self._SudoSet('cgroup.clone_children', '1')

      # Deal with noprefix mount option.  Because Android.
      # https://crbug.com/647994 & https://crbug.com/786506
      name_prefix = 'cpuset.'
      if os.path.exists(os.path.join(self.path, 'cpus')):
        name_prefix = ''

      try:
        # TODO(ferringb): sort out an appropriate filter/list for using:
        # for name in os.listdir(parent):
        # rather than just transfering these two values.
        for name in ('cpus', 'mems'):
          name = name_prefix + name
          if not self._overwrite:
            # Top level nodes like cros/cbuildbot we don't want to overwrite-
            # users/system may've leveled configuration.  If it's empty,
            # overwrite it in those cases.
            val = self.GetValue(name, '').strip()
            if val:
              continue
          self._SudoSet(name, self.parent.GetValue(name, ''))
      except (EnvironmentError, cros_build_lib.RunCommandError):
        # Do not leave half created cgroups hanging around-
        # it makes compatibility a pain since we have to rewrite
        # the cgroup each time.  If instantiation fails, we know
        # the group is screwed up, or the instantiaton code is-
        # either way, no reason to leave it alive.
        self.RemoveThisGroup()
        raise

    return True

  # Since some of this code needs to check/reset this function to be ran,
  # we use a more developer friendly variable name.
  Instantiate._cache_key = '_inited'  # pylint: disable=protected-access

  def _SudoSet(self, key, value):
    """Set a cgroup file in this namespace to a specific value"""
    name = self._LimitName(key, True)
    try:
      return sudo.SetFileContents(name, value, cwd=os.path.dirname(name))
    except cros_build_lib.RunCommandError as e:
      if e.exception is not None:
        # Command failed before the exec itself; convert ENOENT
        # appropriately.
        exc = e.exception
        if isinstance(exc, EnvironmentError) and exc.errno == errno.ENOENT:
          raise _GroupWasRemoved(self.namespace, e)
      raise

  def RemoveThisGroup(self, strict=False):
    """Remove this specific cgroup

    If strict is True, then we must be removed.
    """
    if self._RemoveGroupOnDisk(self.path, strict=strict):
      self._inited = None
      return True
    return False

  @classmethod
  def _RemoveGroupOnDisk(cls, path, strict, sudo_strict=True):
    """Perform the actual group removal.

    Args:
      path: The cgroup's location on disk.
      strict: Boolean; if true, then it's an error if the group can't be
        removed.  This can occur if there are still processes in it, or in
        a nested group.
      sudo_strict: See SudoRunCommand's strict option.
    """
    # Depth first recursively remove our children cgroups, then ourselves.
    # Allow this to fail since currently it's possible for the cleanup code
    # to not fully kill the hierarchy.  Note that we must do just rmdirs,
    # rm -rf cannot be used- it tries to remove files which are unlinkable
    # in cgroup (only namespaces can be removed via rmdir).
    # See Documentation/cgroups/ for further details.
    path = os.path.normpath(path) + '/'
    # Do a sanity check to ensure that we're not touching anything we
    # shouldn't.
    if not path.startswith(cls.CGROUP_ROOT):
      raise RuntimeError("cgroups.py: Was asked to wipe path %s, refusing. "
                         "strict was %r, sudo_strict was %r"
                         % (path, strict, sudo_strict))

    result = cros_build_lib.SudoRunCommand(
        ['find', path, '-depth', '-type', 'd', '-exec', 'rmdir', '{}', '+'],
        redirect_stderr=True, error_code_ok=not strict,
        print_cmd=False, strict=sudo_strict)
    if result.returncode == 0:
      return True
    elif not os.path.isdir(path):
      # We were invoked against a nonexistant path.
      return True
    return False

  def TransferCurrentProcess(self, threads=True):
    """Move the current process into this cgroup.

    If threads is True, we move our threads into the group in addition.
    Note this must be called in a threadsafe manner; it primarily exists
    as a helpful default since python stdlib generates some background
    threads (even when the code is operated synchronously).  While we
    try to handle that scenario, it's implicitly racy since python
    gives no clean/sane way to control/stop thread creation; thus it's
    on the invokers head to ensure no new threads are being generated
    while this is ran.
    """
    if not threads:
      return self.TransferPid(os.getpid())

    seen = set()
    while True:
      force_run = False
      threads = set(self._GetCurrentProcessThreads())
      for tid in threads:
        # Track any failures; a failure means the thread died under
        # feet, implying we shouldn't trust the current state.
        force_run |= not self.TransferPid(tid, True)
      if not force_run and threads == seen:
        # We got two runs of this code seeing the same threads; assume
        # we got them all since the first run moved those threads into
        # our cgroup, and the second didn't see any new threads.  While
        # there may have been new threads between run1/run2, we do run2
        # purely to snag threads we missed in run1; anything split by
        # a thread from run1 would auto inherit our cgroup.
        return
      seen = threads

  def _GetCurrentProcessThreads(self):
    """Lookup the given tasks (pids fundamentally) for our process."""
    # Note that while we could try doing tricks like threading.enumerate,
    # that's not guranteed to pick up background c/ffi threads; generally
    # that's ultra rare, but the potential exists thus we ask the kernel
    # instead.  What sucks however is that python releases the GIL; thus
    # consuming code has to know of this, and protect against it.
    return map(int, os.listdir('/proc/self/task'))

  @EnsureInitialized
  def TransferPid(self, pid, allow_missing=False):
    """Assigns a given process to this cgroup."""
    # Assign this root process to the new cgroup.
    try:
      self._SudoSet('tasks', '%d' % int(pid))
      return True
    except cros_build_lib.RunCommandError:
      if not allow_missing:
        raise
      return False

  # TODO(ferringb): convert to snakeoil.weakref.WeakRefFinalizer
  def __del__(self):
    if self.autoclean and self._inited and self.CGROUP_ROOT:
      # Suppress any sudo_strict behaviour, since we may be invoked
      # during interpreter shutdown.
      self._RemoveGroupOnDisk(self.path, False, sudo_strict=False)

  def KillProcesses(self, poll_interval=0.05, remove=False, sigterm_timeout=10):
    """Kill all processes in this namespace."""

    my_pids = set(map(str, self._GetCurrentProcessThreads()))

    def _SignalPids(pids, signum):
      cros_build_lib.SudoRunCommand(
          ['kill', '-%i' % signum] + sorted(pids),
          print_cmd=False, error_code_ok=True, redirect_stdout=True,
          combine_stdout_stderr=True)

    # First sigterm what we can, exiting after 2 runs w/out seeing pids.
    # Let this phase run for a max of 10 seconds; afterwards, switch to
    # sigkilling.
    time_end = time.time() + sigterm_timeout
    saw_pids, pids = True, set()
    while time.time() < time_end:
      previous_pids = pids
      pids = self.tasks

      self_kill = my_pids.intersection(pids)
      if self_kill:
        raise Exception("Bad API usage: asked to kill cgroup %s, but "
                        "current pid %s is in that group.  Effectively "
                        "asked to kill ourselves."
                        % (self.namespace, self_kill))

      if not pids:
        if not saw_pids:
          break
        saw_pids = False
      else:
        saw_pids = True
        new_pids = pids.difference(previous_pids)
        if new_pids:
          _SignalPids(new_pids, signal.SIGTERM)
          # As long as new pids keep popping up, skip sleeping and just keep
          # stomping them as quickly as possible (whack-a-mole is a good visual
          # analogy of this).  We do this to ensure that fast moving spawns
          # are dealt with as quickly as possible.  When considering this code,
          # it's best to think about forkbomb scenarios- shouldn't occur, but
          # synthetic fork-bombs can occur, thus this code being aggressive.
          continue

      time.sleep(poll_interval)

    # Next do a sigkill scan.  Again, exit only after no pids have been seen
    # for two scans, and all groups are removed.
    groups_existed = True
    while True:
      pids = self.all_tasks

      if pids:
        self_kill = my_pids.intersection(pids)
        if self_kill:
          raise Exception("Bad API usage: asked to kill cgroup %s, but "
                          "current pid %i is in that group.  Effectively "
                          "asked to kill ourselves."
                          % (self.namespace, self_kill))

        _SignalPids(pids, signal.SIGKILL)
        saw_pids = True
      elif not (saw_pids or groups_existed):
        break
      else:
        saw_pids = False

      time.sleep(poll_interval)

      # Note this is done after the sleep; try to give the kernel time to
      # shutdown the processes.  They may still be transitioning to defunct
      # kernel side by when we hit this scan, but that's fine- the next will
      # get it.
      # This needs to be nonstrict; it's possible the kernel is currently
      # killing the pids we've just sigkill'd, thus the group isn't removable
      # yet.  Additionally, it's possible a child got forked we didn't see.
      # Ultimately via our killing/removal attempts, it will be removed,
      # just not necessarily on the first run.
      if remove:
        if self.RemoveThisGroup(strict=False):
          # If we successfully removed this group, then there can be no pids,
          # sub groups, etc, within it.  No need to scan further.
          return True
        groups_existed = True
      else:
        groups_existed = [group.RemoveThisGroup(strict=False)
                          for group in self.nested_groups]
        groups_existed = not all(groups_existed)



  @classmethod
  def _FindCurrentCrosGroup(cls, pid=None):
    """Find and return the cros namespace a pid is currently in.

    If no pid is given, os.getpid() is substituted.
    """
    if pid is None:
      pid = 'self'
    elif not isinstance(pid, (long, int)):
      raise ValueError("pid must be None, or an integer/long.  Got %r" % (pid,))

    cpuset = None
    try:
      # See the kernels Documentation/filesystems/proc.txt if you're unfamiliar
      # w/ procfs, and keep in mind that we have to work across multiple kernel
      # versions.
      cpuset = osutils.ReadFile('/proc/%s/cpuset' % (pid,)).rstrip('\n')
    except EnvironmentError as e:
      if e.errno != errno.ENOENT:
        raise
      with open('/proc/%s/cgroup' % pid) as f:
        for line in f:
          # First digit is the hierachy index, 2nd is subsytem, 3rd is space.
          # 2:cpuset:/
          # 2:cpuset:/cros/cbuildbot/1234

          line = line.rstrip('\n')
          if not line:
            continue
          line = line.split(':', 2)
          if line[1] == 'cpuset':
            cpuset = line[2]
            break

    if not cpuset or not cpuset.startswith("/cros/"):
      return None
    return cpuset[len("/cros/"):].strip("/")

  @classmethod
  def FindStartingGroup(cls, process_name, nesting=True):
    """Create and return the starting cgroup for ourselves nesting if allowed.

    Note that the node returned is either a generic process pool (e.g.
    cros/cbuildbot), or the parent pool we're nested within; processes
    generated in this group are the responsibility of this process to
    deal with- nor should this process ever try triggering a kill w/in this
    portion of the tree since they don't truly own it.

    Args:
      process_name: See the hierarchy comments at the start of this module.
        This should basically be the process name- cros_sdk for example,
        cbuildbot, etc.
      nesting: If we're invoked by another cros cgroup aware process,
        should we nest ourselves in their hierarchy?  Generally speaking,
        client code should never have a reason to disable nesting.
    """
    if not cls.IsUsable():
      return None

    target = None
    if nesting:
      target = cls._FindCurrentCrosGroup()
    if target is None:
      target = process_name

    return _cros_node.AddGroup(target, autoclean=False)


class ContainChildren(cros_build_lib.MasterPidContextManager):
  """Context manager for containing children processes.

  This manager creates a job pool derived from the specified Cgroup |node|
  and transfers the current process into it upon __enter__.

  Any children processes created at that point will inherit our cgroup;
  they can only escape the group if they're running as root and move
  themselves out of this hierarchy.

  Upon __exit__, transfer the current process back to this group, then
  SIGTERM (progressing to SIGKILL) any immediate children in the pool,
  finally removing the pool if possible. After sending SIGTERM, we wait
  |sigterm_timeout| seconds before sending SIGKILL.

  If |pool_name| is given, that name is used rather than os.getpid() for
  the job pool created.

  Finally, note that during cleanup this will suppress all signals
  to ensure that it cleanses any children before returning.
  """

  def __init__(self, node, pool_name=None, sigterm_timeout=10):
    super(ContainChildren, self).__init__()
    self.node = node
    self.child = None
    self.pid = None
    self.pool_name = pool_name
    self.sigterm_timeout = sigterm_timeout
    self.run_kill = False

  def _enter(self):
    self.pid = os.getpid()

    # Note: We use lazy init here so that we cannot trigger a
    # _GroupWasRemoved -- we want that to be contained.
    pool_name = str(self.pid) if self.pool_name is None else self.pool_name
    self.child = self.node.AddGroup(pool_name, autoclean=True, lazy_init=True)
    try:
      self.child.TransferCurrentProcess()
    except _GroupWasRemoved:
      raise SystemExit(
          "Group %s was removed under our feet; pool shutdown is underway"
          % self.child.namespace)
    self.run_kill = True

  def _exit(self, *_args, **_kwargs):
    with signals.DeferSignals():
      self.node.TransferCurrentProcess()
      if self.run_kill:
        self.child.KillProcesses(remove=True,
                                 sigterm_timeout=self.sigterm_timeout)
      else:
        # Non-strict since the group may have failed to be created.
        self.child.RemoveThisGroup(strict=False)


def SimpleContainChildren(process_name, nesting=True, pid=None, **kwargs):
  """Convenience context manager to create a cgroup for children containment

  See Cgroup.FindStartingGroup and Cgroup.ContainChildren for specifics.
  If Cgroups aren't supported on this system, this is a noop context manager.
  """
  node = Cgroup.FindStartingGroup(process_name, nesting=nesting)
  if node is None:
    return cros_build_lib.NoOpContextManager()
  if pid is None:
    pid = os.getpid()
  name = '%s:%i' % (process_name, pid)
  return ContainChildren(node, name, **kwargs)

# This is a generic group, not associated with any specific process id, so
# we shouldn't autoclean it on exit; doing so would delete the group from
# under the feet of any other processes interested in using the group.
_root_node = Cgroup(None, _is_root=True, autoclean=False, lazy_init=True)
_cros_node = _root_node.AddGroup('cros', autoclean=False, lazy_init=True,
                                 _overwrite=False)
