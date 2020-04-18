#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Build NativeClient toolchain packages."""

import logging
import optparse
import os
import subprocess
import sys
import textwrap

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.file_tools
import pynacl.gsd_storage
import pynacl.log_tools
import pynacl.local_storage_cache

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(SCRIPT_DIR)
ROOT_DIR = os.path.dirname(NACL_DIR)
BUILD_DIR = os.path.join(NACL_DIR, 'build')
PKG_VER_DIR = os.path.join(BUILD_DIR, 'package_version')
sys.path.append(PKG_VER_DIR)
import archive_info
import package_info

import once
import command_options

DEFAULT_CACHE_DIR = os.path.join(SCRIPT_DIR, 'cache')
DEFAULT_GIT_CACHE_DIR = os.path.join(SCRIPT_DIR, 'git_cache')
DEFAULT_SRC_DIR = os.path.join(SCRIPT_DIR, 'src')
DEFAULT_OUT_DIR = os.path.join(SCRIPT_DIR, 'out')


def PrintAnnotatorURL(cloud_item):
  """Print an URL in buildbot annotator form.

  Args:
    cloud_item: once.CloudStorageItem representing a memoized item in the cloud.
  """
  if cloud_item.dir_item:
    url = cloud_item.dir_item.url
    pynacl.log_tools.WriteAnnotatorLine('@@@STEP_LINK@download@%s@@@' % url)

    if cloud_item.log_url:
      log_url = cloud_item.log_url
      pynacl.log_tools.WriteAnnotatorLine('@@@STEP_LINK@log@%s@@@' % log_url)


class BuildError(Exception):
    pass

class PackageBuilder(object):
  """Module to build a setup of packages."""

  def __init__(self, packages, package_targets, args):
    """Constructor.

    Args:
      packages: A dictionary with the following format. There are two types of
                packages: source and build (described below).
        {
          '<package name>': {
            'type': 'source',
                # Source packages are for sources; in particular remote sources
                # where it is not known whether they have changed until they are
                # synced (it can also or for tarballs which need to be
                # unpacked). Source package commands are run unconditionally
                # unless sync is skipped via the command-line option. Source
                # package contents are not memoized.
            'dependencies':  # optional
              [<list of package depdenencies>],
            'output_dirname': # optional
              '<directory name>', # Name of the directory to checkout sources
              # into (a subdirectory of the global source directory); defaults
              # to the package name.
            'commands':
              [<list of command.Runnable objects to run>],
            'inputs': # optional
              {<mapping whose keys are names, and whose values are files or
                directories (e.g. checked-in tarballs) used as input. Since
                source targets are unconditional, this is only useful as a
                convenience for commands, which may refer to the inputs by their
                key name>},
          },
          '<package name>': {
            'type': 'build', [or 'build_noncanonical']
                # Build packages are memoized, and will build only if their
                # inputs have changed. Their inputs consist of the output of
                # their package dependencies plus any file or directory inputs
                # given by their 'inputs' member
                # build_noncanonical packages are memoized in the same way, but
                # their cache storage keys get the build platform name appended.
                # This means they can be built by multiple bots without
                # collisions, but only one will be canonical.
            'dependencies':  # optional
              [<list of package depdenencies>],
            'inputs': # optional
              {<mapping whose keys are names, and whose values are files or
                directories (e.g. checked-in tarballs) used as input>},
            'output_subdir': # optional
              '<directory name>', # Name of a subdir to be created in the output
               # directory, into which all output will be placed. If not present
               # output will go into the root of the output directory.
            'commands':
              [<list of command.Command objects to run>],
          },
          '<package name>': {
            'type': 'work',
              # Work packages have the same keys as build packages. However,
              # they are intended to be intermediate targets, and are not
              # memoized or included for package_version.py. Therefore they will
              # always run, regardless of whether their inputs have changed or
              # of whether source syncing is skipped via the command line.
            <same keys as build-type packages>
          },
        }
      package_targets: A dictionary with the following format. This is a
                       description of output package targets the packages are
                       built for. Each output package should contain a list of
                       <package_name> referenced in the previous "packages"
                       dictionary. This list of targets is expected to stay
                       the same from build to build, so it should include
                       package names even if they aren't being built. A package
                       target is usually the platform, such as "$OS_$ARCH",
                       while the output package is usually the toolchain name,
                       such as "nacl_arm_glibc".
        {
          '<package_target>': {
            '<output_package>':
              [<list of package names included in output package>]
          }
        }
      args: sys.argv[1:] or equivalent.
    """
    self._packages = packages
    self._package_targets = package_targets
    self.DecodeArgs(packages, args)
    self._build_once = once.Once(
        use_cached_results=self._options.use_cached_results,
        cache_results=self._options.cache_results,
        print_url=PrintAnnotatorURL,
        storage=self.CreateStorage(),
        extra_paths=self.ExtraSubstitutionPaths())
    self._signature_file = None
    if self._options.emit_signatures is not None:
      if self._options.emit_signatures == '-':
        self._signature_file = sys.stdout
      else:
        self._signature_file = open(self._options.emit_signatures, 'w')

  def Main(self):
    """Main entry point."""
    pynacl.file_tools.MakeDirectoryIfAbsent(self._options.source)
    pynacl.file_tools.MakeDirectoryIfAbsent(self._options.output)

    pynacl.log_tools.SetupLogging(
        verbose=self._options.verbose,
        log_file=self._options.log_file,
        quiet=self._options.quiet,
        no_annotator=self._options.no_annotator)
    try:
      self.BuildAll()
      self.OutputPackagesInformation()
    except BuildError as e:
      print e
      return 1
    return 0

  def GetOutputDir(self, package, use_subdir):
    # The output dir of source packages is in the source directory, and can be
    # overridden.
    if self._packages[package]['type'] == 'source':
      dirname = self._packages[package].get('output_dirname', package)
      return os.path.join(self._options.source, dirname)
    else:
      root = os.path.join(self._options.output, package + '_install')
      if use_subdir and 'output_subdir' in self._packages[package]:
        return os.path.join(root, self._packages[package]['output_subdir'])
      return root

  def BuildPackage(self, package):
    """Build a single package.

    Assumes dependencies of the package have been built.
    Args:
      package: Package to build.
    """

    package_info = self._packages[package]

    # Validate the package description.
    if 'type' not in package_info:
      raise BuildError('package %s does not have a type' % package)
    type_text = package_info['type']
    if type_text not in ('source', 'build', 'build_noncanonical', 'work'):
      raise BuildError('package %s has unrecognized type: %s' %
                      (package, type_text))
    is_source_target = type_text == 'source'
    is_build_target = type_text in ('build', 'build_noncanonical')
    build_signature_key_extra = ''
    if type_text == 'build_noncanonical':
      build_signature_key_extra = '_' + pynacl.gsd_storage.LegalizeName(
          pynacl.platform.PlatformTriple())

    if 'commands' not in package_info:
      raise BuildError('package %s does not have any commands' % package)

    # Source targets are the only ones to run when doing sync-only.
    if not is_source_target and self._options.sync_sources_only:
      logging.debug('Build skipped: not running commands for %s' % package)
      return

    # Source targets do not run when skipping sync.
    if is_source_target and not (
        self._options.sync_sources or self._options.sync_sources_only):
      logging.debug('Sync skipped: not running commands for %s' % package)
      return

    if type_text == 'build_noncanonical' and self._options.canonical_only:
      logging.debug('Non-canonical build of %s skipped' % package)
      return

    pynacl.log_tools.WriteAnnotatorLine(
        '@@@BUILD_STEP %s (%s)@@@' % (package, type_text))
    logging.debug('Building %s package %s' % (type_text, package))

    dependencies = package_info.get('dependencies', [])

    # Collect a dict of all the inputs.
    inputs = {}
    # Add in explicit inputs.
    if 'inputs' in package_info:
      for key, value in package_info['inputs'].iteritems():
        if key in dependencies:
          raise BuildError('key "%s" found in both dependencies and inputs of '
                          'package "%s"' % (key, package))
        inputs[key] = value
    elif type_text != 'source':
      # Non-source packages default to a particular input directory.
      inputs['src'] = os.path.join(self._options.source, package)
    # Add in each dependency by package name.
    for dependency in dependencies:
      inputs[dependency] = self.GetOutputDir(dependency, True)

    # Each package generates intermediate into output/<PACKAGE>_work.
    # Clobbered here explicitly.
    work_dir = os.path.join(self._options.output, package + '_work')
    if self._options.clobber:
      logging.debug('Clobbering working directory %s' % work_dir)
      pynacl.file_tools.RemoveDirectoryIfPresent(work_dir)
    pynacl.file_tools.MakeDirectoryIfAbsent(work_dir)

    output = self.GetOutputDir(package, False)
    output_subdir = self.GetOutputDir(package, True)

    if not is_source_target or self._options.clobber_source:
      logging.debug('Clobbering output directory %s' % output)
      pynacl.file_tools.RemoveDirectoryIfPresent(output)
      os.makedirs(output_subdir)

    commands = package_info.get('commands', [])

    # Create a command option object specifying current build.
    cmd_options = command_options.CommandOptions(
        work_dir=work_dir,
        clobber_working=self._options.clobber,
        clobber_source=self._options.clobber_source,
        trybot=self._options.trybot,
        buildbot=self._options.buildbot)

    # Do it.
    try:
      self._build_once.Run(
        package, inputs, output,
        commands=commands,
        cmd_options=cmd_options,
        working_dir=work_dir,
        memoize=is_build_target,
        signature_file=self._signature_file,
        subdir=output_subdir,
        bskey_extra = build_signature_key_extra)
    except subprocess.CalledProcessError as e:
      raise BuildError(
        'Error building %s: %s' % (package, str(e)))

    if not is_source_target and self._options.install:
      install = pynacl.platform.CygPath(self._options.install)
      logging.debug('Installing output to %s' % install)
      pynacl.file_tools.CopyTree(output, install)

  def BuildOrder(self, targets):
    """Find what needs to be built in what order to build all targets.

    Args:
      targets: A list of target packages to build.
    Returns:
      A topologically sorted list of the targets plus their transitive
      dependencies, in an order that will allow things to be built.
    """
    order = []
    order_set = set()
    if self._options.ignore_dependencies:
      return targets
    def Add(target, target_path):
      if target in order_set:
        return
      if target not in self._packages:
        raise Exception('Unknown package %s' % target)
      next_target_path = target_path + [target]
      if target in target_path:
        raise Exception('Dependency cycle: %s' % ' -> '.join(next_target_path))
      for dependency in self._packages[target].get('dependencies', []):
        Add(dependency, next_target_path)
      order.append(target)
      order_set.add(target)
    for target in targets:
      Add(target, [])
    return order

  def BuildAll(self):
    """Build all packages selected and their dependencies."""
    for target in self._targets:
      self.BuildPackage(target)

  def OutputPackagesInformation(self):
    """Outputs packages information for the built data."""
    packages_dir = os.path.join(self._options.output, 'packages')
    pynacl.file_tools.RemoveDirectoryIfPresent(packages_dir)
    os.makedirs(packages_dir)

    built_packages = []
    for target, target_dict in self._package_targets.iteritems():
      target_dir = os.path.join(packages_dir, target)
      pynacl.file_tools.MakeDirectoryIfAbsent(target_dir)
      for output_package, components in target_dict.iteritems():
        package_desc = package_info.PackageInfo()

        include_package = False
        for component in components:
          if '.' in component:
            archive_name = component
          else:
            archive_name = component + '.tgz'
          cache_item = self._build_once.GetCachedCloudItemForPackage(component)
          if cache_item is None:
            archive_desc = archive_info.ArchiveInfo(name=archive_name)
          else:
            if cache_item.dir_item:
              include_package = True
              archive_desc = archive_info.ArchiveInfo(
                  name=archive_name,
                  hash=cache_item.dir_item.hash,
                  url=cache_item.dir_item.url,
                  log_url=cache_item.log_url)

          package_desc.AppendArchive(archive_desc)

        # Only output package file if an archive was actually included.
        if include_package:
          package_file = os.path.join(target_dir, output_package + '.json')
          package_desc.SavePackageFile(package_file)

          built_packages.append(package_file)

    if self._options.packages_file:
      packages_file = pynacl.platform.CygPath(self._options.packages_file)
      pynacl.file_tools.MakeParentDirectoryIfAbsent(packages_file)
      with open(packages_file, 'wt') as f:
        f.write('\n'.join(built_packages))

  def DecodeArgs(self, packages, args):
    """Decode command line arguments to this build.

    Populated self._options and self._targets.
    Args:
      packages: A list of package names to build.
      args: sys.argv[1:] or equivalent.
    """
    package_list = sorted(packages.keys())
    parser = optparse.OptionParser(
        usage='USAGE: %prog [options] [targets...]\n\n'
              'Available targets:\n' +
              '\n'.join(textwrap.wrap(' '.join(package_list))))
    parser.add_option(
        '-v', '--verbose', dest='verbose',
        default=False, action='store_true',
        help='Produce more output.')
    parser.add_option(
        '-q', '--quiet', dest='quiet',
        default=False, action='store_true',
        help='Produce no output.')
    parser.add_option(
        '-c', '--clobber', dest='clobber',
        default=False, action='store_true',
        help='Clobber working directories before building.')
    parser.add_option(
        '--cache', dest='cache',
        default=DEFAULT_CACHE_DIR,
        help='Select directory containing local storage cache.')
    parser.add_option(
        '-s', '--source', dest='source',
        default=DEFAULT_SRC_DIR,
        help='Select directory containing source checkouts.')
    parser.add_option(
        '--git-cache', dest='git_cache',
        default=DEFAULT_GIT_CACHE_DIR,
        help='Select directory containing the git cache for syncing.')
    parser.add_option(
        '-o', '--output', dest='output',
        default=DEFAULT_OUT_DIR,
        help='Select directory containing build output.')
    parser.add_option(
        '--packages-file', dest='packages_file',
        default=None,
        help='Output packages file describing list of package files built.')
    parser.add_option(
        '--no-use-cached-results', dest='use_cached_results',
        default=True, action='store_false',
        help='Do not rely on cached results.')
    parser.add_option(
        '--no-use-remote-cache', dest='use_remote_cache',
        default=True, action='store_false',
        help='Do not rely on non-local cached results.')
    parser.add_option(
        '--no-cache-results', dest='cache_results',
        default=True, action='store_false',
        help='Do not cache results.')
    parser.add_option(
        '--no-pinned', dest='pinned',
        default=True, action='store_false',
        help='Do not use pinned revisions.')
    parser.add_option(
        '--no-annotator', dest='no_annotator',
        default=False, action='store_true',
        help='Do not print annotator headings.')
    parser.add_option(
        '--trybot', dest='trybot',
        default=False, action='store_true',
        help='Clean source dirs, run and cache as if on trybot.')
    parser.add_option(
        '--buildbot', dest='buildbot',
        default=False, action='store_true',
        help='Clean source dirs, run and cache as if on a non-trybot buildbot.')
    parser.add_option(
        '--bot', dest='bot',
        default=False, action='store_true',
        help='Clean source dirs, run and cache as if on bot, ' +
        'but do not upload (unless --trybot or --buildbot).')
    parser.add_option(
        '--clobber-source', dest='clobber_source',
        default=False, action='store_true',
        help='Clobber source directories before building')
    parser.add_option(
        '-y', '--sync', dest='sync_sources',
        default=False, action='store_true',
        help='Run source target commands')
    parser.add_option(
        '--sync-only', dest='sync_sources_only',
        default=False, action='store_true',
        help='Run source target commands only')
    parser.add_option(
        '--disable-git-cache', dest='disable_git_cache',
        default=False, action='store_true',
        help='Disable git cache when syncing sources')
    parser.add_option(
        '--emit-signatures', dest='emit_signatures',
        help='Write human readable build signature for each step to FILE.',
        metavar='FILE')
    parser.add_option(
        '-i', '--ignore-dependencies', dest='ignore_dependencies',
        default=False, action='store_true',
        help='Ignore target dependencies and build only the specified target.')
    parser.add_option(
        '--canonical-only', dest='canonical_only',
        default=False, action='store_true',
        help='Do not build build_noncanonical targets')
    parser.add_option('--install', dest='install',
                      help='After building, copy contents of build packages' +
                      ' to the specified directory')
    parser.add_option('--log-file', dest='log_file',
                      default=None, action='store',
                      help='Log all logging into a log file.')
    options, targets = parser.parse_args(args)
    if options.trybot and options.buildbot:
      print >>sys.stderr, (
          'ERROR: Tried to run with both --trybot and --buildbot.')
      sys.exit(1)
    if options.trybot or options.buildbot:
      options.bot = True
    if options.bot:
      options.verbose = True
      options.quiet = False
      options.no_annotator = False
      options.sync_sources = True
      options.clobber = True
      options.emit_signatures = '-'
    self._options = options
    if not targets:
      if self._options.ignore_dependencies:
        print >>sys.stderr, (
            'ERROR: A target must be specified if ignoring target dependencies')
        sys.exit(1)
      targets = sorted(packages.keys())
    targets = self.BuildOrder(targets)
    self._targets = targets

  def CreateStorage(self):
    """Create a storage object for this build.

    Returns:
      A storage object (GSDStorage).
    """
    if self._options.buildbot:
      return pynacl.gsd_storage.GSDStorage(
          write_bucket='nativeclient-once',
          read_buckets=['nativeclient-once'])
    elif self._options.trybot:
      return pynacl.gsd_storage.GSDStorage(
          write_bucket='nativeclient-once-try',
          read_buckets=['nativeclient-once', 'nativeclient-once-try'])
    else:
      read_buckets = []
      if self._options.use_remote_cache:
        read_buckets += ['nativeclient-once']
      return pynacl.local_storage_cache.LocalStorageCache(
          cache_path=self._options.cache,
          storage=pynacl.gsd_storage.GSDStorage(
              write_bucket=None,
              read_buckets=read_buckets))

  def ExtraSubstitutionPaths(self):
    """Returns a dictionary of extra substitution paths allowed for commands."""
    if self._options.disable_git_cache:
      git_cache_dir = ''
    else:
      git_cache_dir = self._options.git_cache

    return {
        'top_srcdir': NACL_DIR,
        'git_cache_dir': git_cache_dir,
        }
