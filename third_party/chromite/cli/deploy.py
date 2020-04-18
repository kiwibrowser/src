# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Deploy packages onto a target device."""

from __future__ import print_function

import fnmatch
import functools
import json
import os

from chromite.cli import command
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import operation
from chromite.lib import portage_util
from chromite.lib import remote_access
try:
  import portage
except ImportError:
  if cros_build_lib.IsInsideChroot():
    raise


_DEVICE_BASE_DIR = '/usr/local/tmp/cros-deploy'
# This is defined in src/platform/dev/builder.py
_STRIPPED_PACKAGES_DIR = 'stripped-packages'

_MAX_UPDATES_NUM = 10
_MAX_UPDATES_WARNING = (
    'You are about to update a large number of installed packages, which '
    'might take a long time, fail midway, or leave the target in an '
    'inconsistent state. It is highly recommended that you flash a new image '
    'instead.')


class DeployError(Exception):
  """Thrown when an unrecoverable error is encountered during deploy."""


class BrilloDeployOperation(operation.ProgressBarOperation):
  """ProgressBarOperation specific for brillo deploy."""
  MERGE_EVENTS = ['NOTICE: Copying', 'NOTICE: Installing',
                  'Calculating dependencies', '... done!', 'Extracting info',
                  'Installing (1 of 1)', 'has been installed.']
  UNMERGE_EVENTS = ['NOTICE: Unmerging', 'has been uninstalled.']

  def __init__(self, pkg_count, emerge):
    """Construct BrilloDeployOperation object.

    Args:
      pkg_count: number of packages being built.
      emerge: True if emerge, False is unmerge.
    """
    super(BrilloDeployOperation, self).__init__()
    if emerge:
      self._events = self.MERGE_EVENTS
    else:
      self._events = self.UNMERGE_EVENTS
    self._total = pkg_count * len(self._events)
    self._completed = 0

  def ParseOutput(self, output=None):
    """Parse the output of brillo deploy to update a progress bar."""
    stdout = self._stdout.read()
    stderr = self._stderr.read()
    output = stdout + stderr
    for event in self._events:
      self._completed += output.count(event)
    self.ProgressBar(float(self._completed) / self._total)


class _InstallPackageScanner(object):
  """Finds packages that need to be installed on a target device.

  Scans the sysroot bintree, beginning with a user-provided list of packages,
  to find all packages that need to be installed. If so instructed,
  transitively scans forward (mandatory) and backward (optional) dependencies
  as well. A package will be installed if missing on the target (mandatory
  packages only), or it will be updated if its sysroot version and build time
  are different from the target. Common usage:

    pkg_scanner = _InstallPackageScanner(sysroot)
    pkgs = pkg_scanner.Run(...)
  """

  class VartreeError(Exception):
    """An error in the processing of the installed packages tree."""

  class BintreeError(Exception):
    """An error in the processing of the source binpkgs tree."""

  class PkgInfo(object):
    """A record containing package information."""

    __slots__ = ('cpv', 'build_time', 'rdeps_raw', 'rdeps', 'rev_rdeps')

    def __init__(self, cpv, build_time, rdeps_raw, rdeps=None, rev_rdeps=None):
      self.cpv = cpv
      self.build_time = build_time
      self.rdeps_raw = rdeps_raw
      self.rdeps = set() if rdeps is None else rdeps
      self.rev_rdeps = set() if rev_rdeps is None else rev_rdeps

  # Python snippet for dumping vartree info on the target. Instantiate using
  # _GetVartreeSnippet().
  _GET_VARTREE = """
import portage
import json
trees = portage.create_trees(target_root='%(root)s', config_root='/')
vartree = trees['%(root)s']['vartree']
pkg_info = []
for cpv in vartree.dbapi.cpv_all():
  slot, rdep_raw, build_time = vartree.dbapi.aux_get(
      cpv, ('SLOT', 'RDEPEND', 'BUILD_TIME'))
  pkg_info.append((cpv, slot, rdep_raw, build_time))

print(json.dumps(pkg_info))
"""

  def __init__(self, sysroot):
    self.sysroot = sysroot
    # Members containing the sysroot (binpkg) and target (installed) package DB.
    self.target_db = None
    self.binpkgs_db = None
    # Members for managing the dependency resolution work queue.
    self.queue = None
    self.seen = None
    self.listed = None

  @staticmethod
  def _GetCP(cpv):
    """Returns the CP value for a given CPV string."""
    attrs = portage_util.SplitCPV(cpv, strict=False)
    if not (attrs.category and attrs.package):
      raise ValueError('Cannot get CP value for %s' % cpv)
    return os.path.join(attrs.category, attrs.package)

  @staticmethod
  def _InDB(cp, slot, db):
    """Returns whether CP and slot are found in a database (if provided)."""
    cp_slots = db.get(cp) if db else None
    return cp_slots is not None and (not slot or slot in cp_slots)

  @staticmethod
  def _AtomStr(cp, slot):
    """Returns 'CP:slot' if slot is non-empty, else just 'CP'."""
    return '%s:%s' % (cp, slot) if slot else cp

  @classmethod
  def _GetVartreeSnippet(cls, root='/'):
    """Returns a code snippet for dumping the vartree on the target.

    Args:
      root: The installation root.

    Returns:
      The said code snippet (string) with parameters filled in.
    """
    return cls._GET_VARTREE % {'root': root}

  @classmethod
  def _StripDepAtom(cls, dep_atom, installed_db=None):
    """Strips a dependency atom and returns a (CP, slot) pair."""
    # TODO(garnold) This is a gross simplification of ebuild dependency
    # semantics, stripping and ignoring various qualifiers (versions, slots,
    # USE flag, negation) and will likely need to be fixed. chromium:447366.

    # Ignore unversioned blockers, leaving them for the user to resolve.
    if dep_atom[0] == '!' and dep_atom[1] not in '<=>~':
      return None, None

    cp = dep_atom
    slot = None
    require_installed = False

    # Versioned blockers should be updated, but only if already installed.
    # These are often used for forcing cascaded updates of multiple packages,
    # so we're treating them as ordinary constraints with hopes that it'll lead
    # to the desired result.
    if cp.startswith('!'):
      cp = cp.lstrip('!')
      require_installed = True

    # Remove USE flags.
    if '[' in cp:
      cp = cp[:cp.index('[')] + cp[cp.index(']') + 1:]

    # Separate the slot qualifier and strip off subslots.
    if ':' in cp:
      cp, slot = cp.split(':')
      for delim in ('/', '='):
        slot = slot.split(delim, 1)[0]

    # Strip version wildcards (right), comparators (left).
    cp = cp.rstrip('*')
    cp = cp.lstrip('<=>~')

    # Turn into CP form.
    cp = cls._GetCP(cp)

    if require_installed and not cls._InDB(cp, None, installed_db):
      return None, None

    return cp, slot

  @classmethod
  def _ProcessDepStr(cls, dep_str, installed_db, avail_db):
    """Resolves and returns a list of dependencies from a dependency string.

    This parses a dependency string and returns a list of package names and
    slots. Other atom qualifiers (version, sub-slot, block) are ignored. When
    resolving disjunctive deps, we include all choices that are fully present
    in |installed_db|. If none is present, we choose an arbitrary one that is
    available.

    Args:
      dep_str: A raw dependency string.
      installed_db: A database of installed packages.
      avail_db: A database of packages available for installation.

    Returns:
      A list of pairs (CP, slot).

    Raises:
      ValueError: the dependencies string is malformed.
    """
    def ProcessSubDeps(dep_exp, disjunct):
      """Parses and processes a dependency (sub)expression."""
      deps = set()
      default_deps = set()
      sub_disjunct = False
      for dep_sub_exp in dep_exp:
        sub_deps = set()

        if isinstance(dep_sub_exp, (list, tuple)):
          sub_deps = ProcessSubDeps(dep_sub_exp, sub_disjunct)
          sub_disjunct = False
        elif sub_disjunct:
          raise ValueError('Malformed disjunctive operation in deps')
        elif dep_sub_exp == '||':
          sub_disjunct = True
        elif dep_sub_exp.endswith('?'):
          raise ValueError('Dependencies contain a conditional')
        else:
          cp, slot = cls._StripDepAtom(dep_sub_exp, installed_db)
          if cp:
            sub_deps = set([(cp, slot)])
          elif disjunct:
            raise ValueError('Atom in disjunct ignored')

        # Handle sub-deps of a disjunctive expression.
        if disjunct:
          # Make the first available choice the default, for use in case that
          # no option is installed.
          if (not default_deps and avail_db is not None and
              all([cls._InDB(cp, slot, avail_db) for cp, slot in sub_deps])):
            default_deps = sub_deps

          # If not all sub-deps are installed, then don't consider them.
          if not all([cls._InDB(cp, slot, installed_db)
                      for cp, slot in sub_deps]):
            sub_deps = set()

        deps.update(sub_deps)

      return deps or default_deps

    try:
      return ProcessSubDeps(portage.dep.paren_reduce(dep_str), False)
    except portage.exception.InvalidDependString as e:
      raise ValueError('Invalid dep string: %s' % e)
    except ValueError as e:
      raise ValueError('%s: %s' % (e, dep_str))

  def _BuildDB(self, cpv_info, process_rdeps, process_rev_rdeps,
               installed_db=None):
    """Returns a database of packages given a list of CPV info.

    Args:
      cpv_info: A list of tuples containing package CPV and attributes.
      process_rdeps: Whether to populate forward dependencies.
      process_rev_rdeps: Whether to populate reverse dependencies.
      installed_db: A database of installed packages for filtering disjunctive
        choices against; if None, using own built database.

    Returns:
      A map from CP values to another dictionary that maps slots to package
      attribute tuples. Tuples contain a CPV value (string), build time
      (string), runtime dependencies (set), and reverse dependencies (set,
      empty if not populated).

    Raises:
      ValueError: If more than one CPV occupies a single slot.
    """
    db = {}
    logging.debug('Populating package DB...')
    for cpv, slot, rdeps_raw, build_time in cpv_info:
      cp = self._GetCP(cpv)
      cp_slots = db.setdefault(cp, dict())
      if slot in cp_slots:
        raise ValueError('More than one package found for %s' %
                         self._AtomStr(cp, slot))
      logging.debug(' %s -> %s, built %s, raw rdeps: %s',
                    self._AtomStr(cp, slot), cpv, build_time, rdeps_raw)
      cp_slots[slot] = self.PkgInfo(cpv, build_time, rdeps_raw)

    avail_db = db
    if installed_db is None:
      installed_db = db
      avail_db = None

    # Add approximate forward dependencies.
    if process_rdeps:
      logging.debug('Populating forward dependencies...')
      for cp, cp_slots in db.iteritems():
        for slot, pkg_info in cp_slots.iteritems():
          pkg_info.rdeps.update(self._ProcessDepStr(pkg_info.rdeps_raw,
                                                    installed_db, avail_db))
          logging.debug(' %s (%s) processed rdeps: %s',
                        self._AtomStr(cp, slot), pkg_info.cpv,
                        ' '.join([self._AtomStr(rdep_cp, rdep_slot)
                                  for rdep_cp, rdep_slot in pkg_info.rdeps]))

    # Add approximate reverse dependencies (optional).
    if process_rev_rdeps:
      logging.debug('Populating reverse dependencies...')
      for cp, cp_slots in db.iteritems():
        for slot, pkg_info in cp_slots.iteritems():
          for rdep_cp, rdep_slot in pkg_info.rdeps:
            to_slots = db.get(rdep_cp)
            if not to_slots:
              continue

            for to_slot, to_pkg_info in to_slots.iteritems():
              if rdep_slot and to_slot != rdep_slot:
                continue
              logging.debug(' %s (%s) added as rev rdep for %s (%s)',
                            self._AtomStr(cp, slot), pkg_info.cpv,
                            self._AtomStr(rdep_cp, to_slot), to_pkg_info.cpv)
              to_pkg_info.rev_rdeps.add((cp, slot))

    return db

  def _InitTargetVarDB(self, device, root, process_rdeps, process_rev_rdeps):
    """Initializes a dictionary of packages installed on |device|."""
    get_vartree_script = self._GetVartreeSnippet(root)
    try:
      result = device.GetAgent().RemoteSh(['python'], remote_sudo=True,
                                          input=get_vartree_script)
    except cros_build_lib.RunCommandError as e:
      logging.error('Cannot get target vartree:\n%s', e.result.error)
      raise

    try:
      self.target_db = self._BuildDB(json.loads(result.output),
                                     process_rdeps, process_rev_rdeps)
    except ValueError as e:
      raise self.VartreeError(str(e))

  def _InitBinpkgDB(self, process_rdeps):
    """Initializes a dictionary of binary packages for updating the target."""
    # Get build root trees; portage indexes require a trailing '/'.
    build_root = os.path.join(self.sysroot, '')
    trees = portage.create_trees(target_root=build_root, config_root=build_root)
    bintree = trees[build_root]['bintree']
    binpkgs_info = []
    for cpv in bintree.dbapi.cpv_all():
      slot, rdep_raw, build_time = bintree.dbapi.aux_get(
          cpv, ['SLOT', 'RDEPEND', 'BUILD_TIME'])
      binpkgs_info.append((cpv, slot, rdep_raw, build_time))

    try:
      self.binpkgs_db = self._BuildDB(binpkgs_info, process_rdeps, False,
                                      installed_db=self.target_db)
    except ValueError as e:
      raise self.BintreeError(str(e))

  def _InitDepQueue(self):
    """Initializes the dependency work queue."""
    self.queue = set()
    self.seen = {}
    self.listed = set()

  def _EnqDep(self, dep, listed, optional):
    """Enqueues a dependency if not seen before or if turned non-optional."""
    if dep in self.seen and (optional or not self.seen[dep]):
      return False

    self.queue.add(dep)
    self.seen[dep] = optional
    if listed:
      self.listed.add(dep)
    return True

  def _DeqDep(self):
    """Dequeues and returns a dependency, its listed and optional flags.

    This returns listed packages first, if any are present, to ensure that we
    correctly mark them as such when they are first being processed.
    """
    if self.listed:
      dep = self.listed.pop()
      self.queue.remove(dep)
      listed = True
    else:
      dep = self.queue.pop()
      listed = False

    return dep, listed, self.seen[dep]

  def _FindPackageMatches(self, cpv_pattern):
    """Returns list of binpkg (CP, slot) pairs that match |cpv_pattern|.

    This is breaking |cpv_pattern| into its C, P and V components, each of
    which may or may not be present or contain wildcards. It then scans the
    binpkgs database to find all atoms that match these components, returning a
    list of CP and slot qualifier. When the pattern does not specify a version,
    or when a CP has only one slot in the binpkgs database, we omit the slot
    qualifier in the result.

    Args:
      cpv_pattern: A CPV pattern, potentially partial and/or having wildcards.

    Returns:
      A list of (CPV, slot) pairs of packages in the binpkgs database that
      match the pattern.
    """
    attrs = portage_util.SplitCPV(cpv_pattern, strict=False)
    cp_pattern = os.path.join(attrs.category or '*', attrs.package or '*')
    matches = []
    for cp, cp_slots in self.binpkgs_db.iteritems():
      if not fnmatch.fnmatchcase(cp, cp_pattern):
        continue

      # If no version attribute was given or there's only one slot, omit the
      # slot qualifier.
      if not attrs.version or len(cp_slots) == 1:
        matches.append((cp, None))
      else:
        cpv_pattern = '%s-%s' % (cp, attrs.version)
        for slot, pkg_info in cp_slots.iteritems():
          if fnmatch.fnmatchcase(pkg_info.cpv, cpv_pattern):
            matches.append((cp, slot))

    return matches

  def _FindPackage(self, pkg):
    """Returns the (CP, slot) pair for a package matching |pkg|.

    Args:
      pkg: Path to a binary package or a (partial) package CPV specifier.

    Returns:
      A (CP, slot) pair for the given package; slot may be None (unspecified).

    Raises:
      ValueError: if |pkg| is not a binpkg file nor does it match something
      that's in the bintree.
    """
    if pkg.endswith('.tbz2') and os.path.isfile(pkg):
      package = os.path.basename(os.path.splitext(pkg)[0])
      category = os.path.basename(os.path.dirname(pkg))
      return self._GetCP(os.path.join(category, package)), None

    matches = self._FindPackageMatches(pkg)
    if not matches:
      raise ValueError('No package found for %s' % pkg)

    idx = 0
    if len(matches) > 1:
      # Ask user to pick among multiple matches.
      idx = cros_build_lib.GetChoice('Multiple matches found for %s: ' % pkg,
                                     ['%s:%s' % (cp, slot) if slot else cp
                                      for cp, slot in matches])

    return matches[idx]

  def _NeedsInstall(self, cpv, slot, build_time, optional):
    """Returns whether a package needs to be installed on the target.

    Args:
      cpv: Fully qualified CPV (string) of the package.
      slot: Slot identifier (string).
      build_time: The BUILT_TIME value (string) of the binpkg.
      optional: Whether package is optional on the target.

    Returns:
      A tuple (install, update) indicating whether to |install| the package and
      whether it is an |update| to an existing package.

    Raises:
      ValueError: if slot is not provided.
    """
    # If not checking installed packages, always install.
    if not self.target_db:
      return True, False

    cp = self._GetCP(cpv)
    target_pkg_info = self.target_db.get(cp, dict()).get(slot)
    if target_pkg_info is not None:
      if cpv != target_pkg_info.cpv:
        attrs = portage_util.SplitCPV(cpv)
        target_attrs = portage_util.SplitCPV(target_pkg_info.cpv)
        logging.debug('Updating %s: version (%s) different on target (%s)',
                      cp, attrs.version, target_attrs.version)
        return True, True

      if build_time != target_pkg_info.build_time:
        logging.debug('Updating %s: build time (%s) different on target (%s)',
                      cpv, build_time, target_pkg_info.build_time)
        return True, True

      logging.debug('Not updating %s: already up-to-date (%s, built %s)',
                    cp, target_pkg_info.cpv, target_pkg_info.build_time)
      return False, False

    if optional:
      logging.debug('Not installing %s: missing on target but optional', cp)
      return False, False

    logging.debug('Installing %s: missing on target and non-optional (%s)',
                  cp, cpv)
    return True, False

  def _ProcessDeps(self, deps, reverse):
    """Enqueues dependencies for processing.

    Args:
      deps: List of dependencies to enqueue.
      reverse: Whether these are reverse dependencies.
    """
    if not deps:
      return

    logging.debug('Processing %d %s dep(s)...', len(deps),
                  'reverse' if reverse else 'forward')
    num_already_seen = 0
    for dep in deps:
      if self._EnqDep(dep, False, reverse):
        logging.debug(' Queued dep %s', dep)
      else:
        num_already_seen += 1

    if num_already_seen:
      logging.debug('%d dep(s) already seen', num_already_seen)

  def _ComputeInstalls(self, process_rdeps, process_rev_rdeps):
    """Returns a dictionary of packages that need to be installed on the target.

    Args:
      process_rdeps: Whether to trace forward dependencies.
      process_rev_rdeps: Whether to trace backward dependencies as well.

    Returns:
      A dictionary mapping CP values (string) to tuples containing a CPV
      (string), a slot (string), a boolean indicating whether the package
      was initially listed in the queue, and a boolean indicating whether this
      is an update to an existing package.
    """
    installs = {}
    while self.queue:
      dep, listed, optional = self._DeqDep()
      cp, required_slot = dep
      if cp in installs:
        logging.debug('Already updating %s', cp)
        continue

      cp_slots = self.binpkgs_db.get(cp, dict())
      logging.debug('Checking packages matching %s%s%s...', cp,
                    ' (slot: %s)' % required_slot if required_slot else '',
                    ' (optional)' if optional else '')
      num_processed = 0
      for slot, pkg_info in cp_slots.iteritems():
        if required_slot and slot != required_slot:
          continue

        num_processed += 1
        logging.debug(' Checking %s...', pkg_info.cpv)

        install, update = self._NeedsInstall(pkg_info.cpv, slot,
                                             pkg_info.build_time, optional)
        if not install:
          continue

        installs[cp] = (pkg_info.cpv, slot, listed, update)

        # Add forward and backward runtime dependencies to queue.
        if process_rdeps:
          self._ProcessDeps(pkg_info.rdeps, False)
        if process_rev_rdeps:
          target_pkg_info = self.target_db.get(cp, dict()).get(slot)
          if target_pkg_info:
            self._ProcessDeps(target_pkg_info.rev_rdeps, True)

      if num_processed == 0:
        logging.warning('No qualified bintree package corresponding to %s', cp)

    return installs

  def _SortInstalls(self, installs):
    """Returns a sorted list of packages to install.

    Performs a topological sort based on dependencies found in the binary
    package database.

    Args:
      installs: Dictionary of packages to install indexed by CP.

    Returns:
      A list of package CPVs (string).

    Raises:
      ValueError: If dependency graph contains a cycle.
    """
    not_visited = set(installs.keys())
    curr_path = []
    sorted_installs = []

    def SortFrom(cp):
      """Traverses dependencies recursively, emitting nodes in reverse order."""
      cpv, slot, _, _ = installs[cp]
      if cpv in curr_path:
        raise ValueError('Dependencies contain a cycle: %s -> %s' %
                         (' -> '.join(curr_path[curr_path.index(cpv):]), cpv))
      curr_path.append(cpv)
      for rdep_cp, _ in self.binpkgs_db[cp][slot].rdeps:
        if rdep_cp in not_visited:
          not_visited.remove(rdep_cp)
          SortFrom(rdep_cp)

      sorted_installs.append(cpv)
      curr_path.pop()

    # So long as there's more packages, keep expanding dependency paths.
    while not_visited:
      SortFrom(not_visited.pop())

    return sorted_installs

  def _EnqListedPkg(self, pkg):
    """Finds and enqueues a listed package."""
    cp, slot = self._FindPackage(pkg)
    if cp not in self.binpkgs_db:
      raise self.BintreeError('Package %s not found in binpkgs tree' % pkg)
    self._EnqDep((cp, slot), True, False)

  def _EnqInstalledPkgs(self):
    """Enqueues all available binary packages that are already installed."""
    for cp, cp_slots in self.binpkgs_db.iteritems():
      target_cp_slots = self.target_db.get(cp)
      if target_cp_slots:
        for slot in cp_slots.iterkeys():
          if slot in target_cp_slots:
            self._EnqDep((cp, slot), True, False)

  def Run(self, device, root, listed_pkgs, update, process_rdeps,
          process_rev_rdeps):
    """Computes the list of packages that need to be installed on a target.

    Args:
      device: Target handler object.
      root: Package installation root.
      listed_pkgs: Package names/files listed by the user.
      update: Whether to read the target's installed package database.
      process_rdeps: Whether to trace forward dependencies.
      process_rev_rdeps: Whether to trace backward dependencies as well.

    Returns:
      A tuple (sorted, listed, num_updates) where |sorted| is a list of package
      CPVs (string) to install on the target in an order that satisfies their
      inter-dependencies, |listed| the subset that was requested by the user,
      and |num_updates| the number of packages being installed over preexisting
      versions. Note that installation order should be reversed for removal.
    """
    if process_rev_rdeps and not process_rdeps:
      raise ValueError('Must processing forward deps when processing rev deps')
    if process_rdeps and not update:
      raise ValueError('Must check installed packages when processing deps')

    if update:
      logging.info('Initializing target intalled packages database...')
      self._InitTargetVarDB(device, root, process_rdeps, process_rev_rdeps)

    logging.info('Initializing binary packages database...')
    self._InitBinpkgDB(process_rdeps)

    logging.info('Finding listed package(s)...')
    self._InitDepQueue()
    for pkg in listed_pkgs:
      if pkg == '@installed':
        if not update:
          raise ValueError(
              'Must check installed packages when updating all of them.')
        self._EnqInstalledPkgs()
      else:
        self._EnqListedPkg(pkg)

    logging.info('Computing set of packages to install...')
    installs = self._ComputeInstalls(process_rdeps, process_rev_rdeps)

    num_updates = 0
    listed_installs = []
    for cpv, _, listed, update in installs.itervalues():
      if listed:
        listed_installs.append(cpv)
      if update:
        num_updates += 1

    logging.info('Processed %d package(s), %d will be installed, %d are '
                 'updating existing packages',
                 len(self.seen), len(installs), num_updates)

    sorted_installs = self._SortInstalls(installs)
    return sorted_installs, listed_installs, num_updates


def _Emerge(device, pkg_path, root, extra_args=None):
  """Copies |pkg| to |device| and emerges it.

  Args:
    device: A ChromiumOSDevice object.
    pkg_path: A path to a binary package.
    root: Package installation root path.
    extra_args: Extra arguments to pass to emerge.

  Raises:
    DeployError: Unrecoverable error during emerge.
  """
  pkgroot = os.path.join(device.work_dir, 'packages')
  pkg_name = os.path.basename(pkg_path)
  pkg_dirname = os.path.basename(os.path.dirname(pkg_path))
  pkg_dir = os.path.join(pkgroot, pkg_dirname)
  portage_tmpdir = os.path.join(device.work_dir, 'portage-tmp')
  # Clean out the dirs first if we had a previous emerge on the device so as to
  # free up space for this emerge.  The last emerge gets implicitly cleaned up
  # when the device connection deletes its work_dir.
  device.RunCommand(
      ['rm', '-rf', pkg_dir, portage_tmpdir, '&&',
       'mkdir', '-p', pkg_dir, portage_tmpdir], remote_sudo=True)

  # This message is read by BrilloDeployOperation.
  logging.notice('Copying %s to device.', pkg_name)
  device.CopyToDevice(pkg_path, pkg_dir, mode='rsync', remote_sudo=True)

  logging.info('Use portage temp dir %s', portage_tmpdir)

  # This message is read by BrilloDeployOperation.
  logging.notice('Installing %s.', pkg_name)
  pkg_path = os.path.join(pkg_dir, pkg_name)

  # We set PORTAGE_CONFIGROOT to '/usr/local' because by default all
  # chromeos-base packages will be skipped due to the configuration
  # in /etc/protage/make.profile/package.provided. However, there is
  # a known bug that /usr/local/etc/portage is not setup properly
  # (crbug.com/312041). This does not affect `cros deploy` because
  # we do not use the preset PKGDIR.
  extra_env = {
      'FEATURES': '-sandbox',
      'PKGDIR': pkgroot,
      'PORTAGE_CONFIGROOT': '/usr/local',
      'PORTAGE_TMPDIR': portage_tmpdir,
      'PORTDIR': device.work_dir,
      'CONFIG_PROTECT': '-*',
  }
  cmd = ['emerge', '--usepkg', pkg_path, '--root=%s' % root]
  if extra_args:
    cmd.append(extra_args)

  try:
    device.RunCommand(cmd, extra_env=extra_env, remote_sudo=True,
                      capture_output=False, debug_level=logging.INFO)
  except Exception:
    logging.error('Failed to emerge package %s', pkg_name)
    raise
  else:
    logging.notice('%s has been installed.', pkg_name)


def _GetPackagesByCPV(cpvs, strip, sysroot):
  """Returns paths to binary packages corresponding to |cpvs|.

  Args:
    cpvs: List of CPV components given by portage_util.SplitCPV().
    strip: True to run strip_package.
    sysroot: Sysroot path.

  Returns:
    List of paths corresponding to |cpvs|.

  Raises:
    DeployError: If a package is missing.
  """
  packages_dir = None
  if strip:
    try:
      cros_build_lib.RunCommand(
          ['strip_package', '--sysroot', sysroot] +
          [os.path.join(cpv.category, str(cpv.pv)) for cpv in cpvs])
      packages_dir = _STRIPPED_PACKAGES_DIR
    except cros_build_lib.RunCommandError:
      logging.error('Cannot strip packages %s',
                    ' '.join([str(cpv) for cpv in cpvs]))
      raise

  paths = []
  for cpv in cpvs:
    path = portage_util.GetBinaryPackagePath(
        cpv.category, cpv.package, cpv.version, sysroot=sysroot,
        packages_dir=packages_dir)
    if not path:
      raise DeployError('Missing package %s.' % cpv)
    paths.append(path)

  return paths


def _GetPackagesPaths(pkgs, strip, sysroot):
  """Returns paths to binary |pkgs|.

  Each package argument may be specified as a filename, in which case it is
  returned as-is, or it may be a CPV value, in which case it is stripped (if
  instructed) and a path to it is returned.

  Args:
    pkgs: List of package arguments.
    strip: Whether or not to run strip_package for CPV packages.
    sysroot: The sysroot path.

  Returns:
    List of paths corresponding to |pkgs|.
  """
  indexes = []
  cpvs = []
  for i, pkg in enumerate(pkgs):
    if not os.path.isfile(pkg):
      indexes.append(i)
      cpvs.append(portage_util.SplitCPV(pkg))

  cpv_paths = cpvs and _GetPackagesByCPV(cpvs, strip, sysroot)
  paths = list(pkgs)
  for i, cpv_path in zip(indexes, cpv_paths):
    paths[i] = cpv_path
  return paths


def _Unmerge(device, pkg, root):
  """Unmerges |pkg| on |device|.

  Args:
    device: A RemoteDevice object.
    pkg: A package name.
    root: Package installation root path.
  """
  pkg_name = os.path.basename(pkg)
  # This message is read by BrilloDeployOperation.
  logging.notice('Unmerging %s.', pkg_name)
  cmd = ['qmerge', '--yes']
  # Check if qmerge is available on the device. If not, use emerge.
  if device.RunCommand(
      ['qmerge', '--version'], error_code_ok=True).returncode != 0:
    cmd = ['emerge']

  cmd.extend(['--unmerge', pkg, '--root=%s' % root])
  try:
    # Always showing the emerge output for clarity.
    device.RunCommand(cmd, capture_output=False, remote_sudo=True,
                      debug_level=logging.INFO)
  except Exception:
    logging.error('Failed to unmerge package %s', pkg_name)
    raise
  else:
    logging.notice('%s has been uninstalled.', pkg_name)


def _ConfirmDeploy(num_updates):
  """Returns whether we can continue deployment."""
  if num_updates > _MAX_UPDATES_NUM:
    logging.warning(_MAX_UPDATES_WARNING)
    return cros_build_lib.BooleanPrompt(default=False)

  return True


def _EmergePackages(pkgs, device, strip, sysroot, root, emerge_args):
  """Call _Emerge for each packge in pkgs."""
  for pkg_path in _GetPackagesPaths(pkgs, strip, sysroot):
    _Emerge(device, pkg_path, root, extra_args=emerge_args)


def _UnmergePackages(pkgs, device, root):
  """Call _Unmege for each package in pkgs."""
  for pkg in pkgs:
    _Unmerge(device, pkg, root)


def Deploy(device, packages, board=None, emerge=True, update=False, deep=False,
           deep_rev=False, clean_binpkg=True, root='/', strip=True,
           emerge_args=None, ssh_private_key=None, ping=True, force=False,
           dry_run=False):
  """Deploys packages to a device.

  Args:
    device: commandline.Device object; None to use the default device.
    packages: List of packages (strings) to deploy to device.
    board: Board to use; None to automatically detect.
    emerge: True to emerge package, False to unmerge.
    update: Check installed version on device.
    deep: Install dependencies also. Implies |update|.
    deep_rev: Install reverse dependencies. Implies |deep|.
    clean_binpkg: Clean outdated binary packages.
    root: Package installation root path.
    strip: Run strip_package to filter out preset paths in the package.
    emerge_args: Extra arguments to pass to emerge.
    ssh_private_key: Path to an SSH private key file; None to use test keys.
    ping: True to ping the device before trying to connect.
    force: Ignore sanity checks and prompts.
    dry_run: Print deployment plan but do not deploy anything.

  Raises:
    ValueError: Invalid parameter or parameter combination.
    DeployError: Unrecoverable failure during deploy.
  """
  if deep_rev:
    deep = True
  if deep:
    update = True

  if not packages:
    raise DeployError('No packages provided, nothing to deploy.')

  if update and not emerge:
    raise ValueError('Cannot update and unmerge.')

  if device:
    hostname, username, port = device.hostname, device.username, device.port
  else:
    hostname, username, port = None, None, None

  lsb_release = None
  sysroot = None
  try:
    with remote_access.ChromiumOSDeviceHandler(
        hostname, port=port, username=username, private_key=ssh_private_key,
        base_dir=_DEVICE_BASE_DIR, ping=ping) as device:
      lsb_release = device.lsb_release

      board = cros_build_lib.GetBoard(device_board=device.board,
                                      override_board=board)
      if not force and board != device.board:
        raise DeployError('Device (%s) is incompatible with board %s. Use '
                          '--force to deploy anyway.' % (device.board, board))

      sysroot = cros_build_lib.GetSysroot(board=board)

      if clean_binpkg:
        logging.notice('Cleaning outdated binary packages from %s', sysroot)
        portage_util.CleanOutdatedBinaryPackages(sysroot)

      if not device.IsDirWritable(root):
        # Only remounts rootfs if the given root is not writable.
        if not device.MountRootfsReadWrite():
          raise DeployError('Cannot remount rootfs as read-write. Exiting.')

      # Obtain list of packages to upgrade/remove.
      pkg_scanner = _InstallPackageScanner(sysroot)
      pkgs, listed, num_updates = pkg_scanner.Run(
          device, root, packages, update, deep, deep_rev)
      if emerge:
        action_str = 'emerge'
      else:
        pkgs.reverse()
        action_str = 'unmerge'

      if not pkgs:
        logging.notice('No packages to %s', action_str)
        return

      logging.notice('These are the packages to %s:', action_str)
      for i, pkg in enumerate(pkgs):
        logging.notice('%s %d) %s', '*' if pkg in listed else ' ', i + 1, pkg)

      if dry_run or not _ConfirmDeploy(num_updates):
        return

      # Select function (emerge or unmerge) and bind args.
      if emerge:
        func = functools.partial(_EmergePackages, pkgs, device, strip,
                                 sysroot, root, emerge_args)
      else:
        func = functools.partial(_UnmergePackages, pkgs, device, root)

      # Call the function with the progress bar or with normal output.
      if command.UseProgressBar():
        op = BrilloDeployOperation(len(pkgs), emerge)
        op.Run(func, log_level=logging.DEBUG)
      else:
        func()

      logging.warning('Please restart any updated services on the device, '
                      'or just reboot it.')
  except Exception:
    if lsb_release:
      lsb_entries = sorted(lsb_release.items())
      logging.info('Following are the LSB version details of the device:\n%s',
                   '\n'.join('%s=%s' % (k, v) for k, v in lsb_entries))
    raise
