#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Memoize the data produced by slow operations into Google storage.

Caches computations described in terms of command lines and inputs directories
or files, which yield a set of output file.
"""

import collections
import hashlib
import logging
import os
import platform
import shutil
import subprocess
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.directory_storage
import pynacl.file_tools
import pynacl.gsd_storage
import pynacl.hashing_tools
import pynacl.log_tools
import pynacl.working_directory

import command
import substituter


CloudStorageItem = collections.namedtuple('CloudStorageItem',
                                          ['dir_item', 'log_url'])


class UserError(Exception):
  pass


class HumanReadableSignature(object):
  """Accumator of signature information in human readable form.

  A replacement for hashlib that collects the inputs for later display.
  """
  def __init__(self):
    self._items = []

  def update(self, data):
    """Add an item to the signature."""
    # Drop paranoid nulls for human readable output.
    data = data.replace('\0', '')
    self._items.append(data)

  def hexdigest(self):
    """Fake version of hexdigest that returns the inputs."""
    return ('*' * 30 + ' PACKAGE SIGNATURE ' + '*' * 30 + '\n' +
            '\n'.join(self._items) + '\n' +
            '=' * 70 + '\n')


class Once(object):
  """Class to memoize slow operations."""

  def __init__(self, storage, use_cached_results=True, cache_results=True,
               print_url=None, system_summary=None, extra_paths={}):
    """Constructor.

    Args:
      storage: An storage layer to read/write from (GSDStorage).
      use_cached_results: Flag indicating that cached computation results
                          should be used when possible.
      cache_results: Flag that indicates if successful computations should be
                     written to the cache.
      print_url: Function that accepts a CloudStorageItem for printing URL
                 results, or None if no printing is needed.
      extra_paths: Extra substitution paths that can be used by commands.
    """
    self._storage = storage
    self._directory_storage = pynacl.directory_storage.DirectoryStorageAdapter(
        storage
    )
    self._use_cached_results = use_cached_results
    self._cache_results = cache_results
    self._cached_cloud_items = {}
    self._print_url = print_url
    self._system_summary = system_summary
    self._path_hash_cache = {}
    self._extra_paths = extra_paths

  def KeyForOutput(self, package, output_hash):
    """Compute the key to store a given output in the data-store.

    Args:
      package: Package name.
      output_hash: Stable hash of the package output.
    Returns:
      Key that this instance of the package output should be stored/retrieved.
    """
    return 'object/%s_%s.tgz' % (package, output_hash)

  def KeyForBuildSignature(self, build_signature, extra):
    """Compute the key to store a computation result in the data-store.

    Args:
      build_signature: Stable hash of the computation.
      extra: extra text to be appended to the key.
    Returns:
      Key that this instance of the computation result should be
      stored/retrieved.
    """
    return 'computed/%s%s.txt' % (build_signature, extra if extra else '')

  def KeyForLog(self, package, output_hash):
    """Compute the key to store a given log file in the data-store.

    Args:
      package: Package name.
      output_hash: Stable hash of the package output.
    Returns:
      Key that this instance of the package log should be stored/retrieved.
    """
    return 'log/%s_%s.log' % (package, output_hash)

  def GetLogFile(self, work_dir, package):
    """Returns the local log file for a given package.

    Args:
      work_dir: The work directory for the package.
      package: The package name.
    Returns:
      Path to the local log file within the work directory.
    """
    return os.path.join(work_dir, '%s.log' % package)

  def WriteOutputFromHash(self, work_dir, package, out_hash, output):
    """Write output from the cache.

    Args:
      work_dir: Working directory path.
      package: Package name (for tgz name).
      out_hash: Hash of desired output.
      output: Output path.
    Returns:
      CloudStorageItem on success, None if not.
    """
    key = self.KeyForOutput(package, out_hash)
    dir_item = self._directory_storage.GetDirectory(key, output)
    if not dir_item:
      logging.debug('Failed to retrieve %s' % key)
      return None
    if pynacl.hashing_tools.StableHashPath(output) != out_hash:
      logging.warning('Object does not match expected hash, '
                      'has hashing method changed?')
      return None

    log_key = self.KeyForLog(package, out_hash)
    log_file = self.GetLogFile(work_dir, package)
    pynacl.file_tools.RemoveFile(log_file)
    log_url = self._storage.GetFile(log_key, log_file)

    return CloudStorageItem(dir_item, log_url)

  def _ProcessCloudItem(self, package, cloud_item):
    """Processes cached directory storage items.

    Args:
      package: Package name for the cached directory item.
      cloud_item: CloudStorageItem representing a memoized item in the cloud.
    """
    # Store the cached URL as a tuple for book keeping.
    self._cached_cloud_items[package] = cloud_item

    # If a print URL function has been specified, print the URL now.
    if self._print_url is not None:
      self._print_url(cloud_item)

  def WriteResultToCache(self, work_dir, package, build_signature, bskey_extra,
                         output):
    """Cache a computed result by key.

    Also prints URLs when appropriate.
    Args:
      work_dir: work directory for the package builder.
      package: Package name (for tgz name).
      build_signature: The input hash of the computation.
      bskey_extra: Extra text to append to build signature storage key.
      output: A path containing the output of the computation.
    """
    if not self._cache_results:
      return
    out_hash = pynacl.hashing_tools.StableHashPath(output)
    try:
      output_key = self.KeyForOutput(package, out_hash)
      # Try to get an existing copy in a temporary directory.
      wd = pynacl.working_directory.TemporaryWorkingDirectory()
      with wd as temp_dir:
        temp_output = os.path.join(temp_dir, 'out')
        dir_item = self._directory_storage.GetDirectory(output_key, temp_output)

        log_key = self.KeyForLog(package, out_hash)
        log_file = self.GetLogFile(work_dir, package)
        log_url = None

        if dir_item is None:
          # Isn't present. Cache the computed result instead.
          dir_item = self._directory_storage.PutDirectory(output, output_key)

          if os.path.isfile(log_file):
            log_url = self._storage.PutFile(log_file, log_key)

          logging.info('Computed fresh result and cached it.')
        else:
          # Cached version is present. Replace the current output with that.
          if self._use_cached_results:
            pynacl.file_tools.RemoveDirectoryIfPresent(output)
            shutil.move(temp_output, output)

            pynacl.file_tools.RemoveFile(log_file)
            log_url = self._storage.GetFile(log_key, log_file)

            logging.info('Recomputed result matches cached value, '
                         'using cached value instead.')
          else:
            log_key_exists = self._storage.Exists(log_key)
            if log_key_exists:
              log_url = log_key_exists

      # Upload an entry mapping from computation input to output hash.
      self._storage.PutData(
          out_hash, self.KeyForBuildSignature(build_signature, bskey_extra))

      cloud_item = CloudStorageItem(dir_item, log_url)
      self._ProcessCloudItem(package, cloud_item)
    except pynacl.gsd_storage.GSDStorageError:
      logging.info('Failed to cache result.')
      raise

  def ReadMemoizedResultFromCache(self, work_dir, package,
                                  build_signature, bskey_extra, output):
    """Read a cached result (if it exists) from the cache.

    Also prints URLs when appropriate.
    Args:
      work_dir: Working directory for the build.
      package: Package name (for tgz name).
      build_signature: Build signature of the computation.
      bskey_extra: Extra text to append to build signature storage key.
      output: Output path.
    Returns:
      Boolean indicating successful retrieval.
    """
    # Check if its in the cache.
    if self._use_cached_results:
      out_hash = self._storage.GetData(
          self.KeyForBuildSignature(build_signature, bskey_extra))
      if out_hash is not None:
        cloud_item = self.WriteOutputFromHash(work_dir, package,
                                              out_hash, output)
        if cloud_item is not None:
          logging.info('Retrieved cached result.')
          pynacl.log_tools.WriteAnnotatorLine(
              '@@@STEP_TEXT@(cache hit)@@@')
          self._ProcessCloudItem(package, cloud_item)
          return True
    return False

  def GetCachedCloudItems(self):
    """Returns the complete list of all cached cloud items for this run."""
    return self._cached_cloud_items.values()

  def GetCachedCloudItemForPackage(self, package):
    """Returns cached cloud item for package or None if not processed."""
    return self._cached_cloud_items.get(package, None)

  def Run(self, package, inputs, output, commands, cmd_options=None,
          working_dir=None, memoize=True, signature_file=None, subdir=None,
          bskey_extra=None):
    """Run an operation once, possibly hitting cache.

    Args:
      package: Name of the computation/module.
      inputs: A dict of names mapped to files that are inputs.
      output: An output directory.
      commands: A list of command.Command objects to run.
      working_dir: Working directory to use, or None for a temp dir.
      memoize: Boolean indicating the the result should be memoized.
      signature_file: File to write human readable build signatures to or None.
      subdir: If not None, use this directory instead of the output dir as the
              substituter's output path. Must be a subdirectory of output.
      bskey_extra: Extra text to append to build signature storage key.
    """
    if working_dir is None:
      wdm = pynacl.working_directory.TemporaryWorkingDirectory()
    else:
      wdm = pynacl.working_directory.FixedWorkingDirectory(working_dir)

    pynacl.file_tools.MakeDirectoryIfAbsent(output)

    nonpath_subst = { 'package': package }

    with wdm as work_dir:
      # Compute the build signature with modified inputs.
      build_signature = self.BuildSignature(
          package, inputs=inputs, commands=commands)
      # Optionally write human readable version of signature.
      if signature_file:
        signature_file.write(self.BuildSignature(
            package, inputs=inputs, commands=commands,
            hasher=HumanReadableSignature()))
        signature_file.flush()

      # We're done if it's in the cache.
      if (memoize and self.ReadMemoizedResultFromCache(work_dir, package,
                                                       build_signature,
                                                       bskey_extra, output)):
        return

      if subdir:
        assert subdir.startswith(output)

      # Filter out commands that have a run condition of False.
      # This must be done before any commands are invoked in case the run
      # conditions rely on any pre-existing states.
      commands = [command for command in commands
                  if command.CheckRunCond(cmd_options)]

      # Create a logger that will save the log for each command.
      # This logger will process any messages and then pass the results
      # up to the base logger.
      base_logger = pynacl.log_tools.GetConsoleLogger()
      cmd_logger = base_logger.getChild('OnceCmdLogger')
      cmd_logger.setLevel(logging.DEBUG)

      log_file = self.GetLogFile(work_dir, package)
      file_log_handler = logging.FileHandler(log_file, 'wb')
      file_log_handler.setLevel(logging.DEBUG)
      file_log_handler.setFormatter(
          logging.Formatter(fmt='[%(levelname)s - %(asctime)s] %(message)s'))
      cmd_logger.addHandler(file_log_handler)

      # Log some helpful information
      cmd_logger.propagate = False
      cmd_logger.debug('Hostname: %s', platform.node())
      cmd_logger.debug('Machine: %s', platform.machine())
      cmd_logger.debug('Platform: %s', sys.platform)
      cmd_logger.propagate = True

      for command in commands:
        paths = inputs.copy()
        # Add the extra paths supplied by our caller, and the original working
        # directory
        paths.update(self._extra_paths)
        paths.update({'work_dir': work_dir})
        paths['output'] = subdir if subdir else output
        nonpath_subst['build_signature'] = build_signature
        subst = substituter.Substituter(work_dir, paths, nonpath_subst)
        command.Invoke(cmd_logger, subst)

      # Uninstall the file log handler
      cmd_logger.removeHandler(file_log_handler)
      file_log_handler.close()

      # Confirm that we aren't hitting something we've cached.
      for path in self._path_hash_cache:
        if not os.path.relpath(output, path).startswith(os.pardir + os.sep):
          raise UserError(
              'Package %s outputs to a directory already used as an input: %s' %
              (package, path))

      if memoize:
        self.WriteResultToCache(work_dir, package, build_signature, bskey_extra,
                                output)

  def SystemSummary(self):
    """Gather a string describing intrinsic properties of the current machine.

    Ideally this would capture anything relevant about the current machine that
    would cause build output to vary (other than build recipe + inputs).
    """
    if self._system_summary is not None:
      return self._system_summary

    # Note there is no attempt to canonicalize these values.  If two
    # machines that would in fact produce identical builds differ in
    # these values, it just means that a superfluous build will be
    # done once to get the mapping from new input hash to preexisting
    # output hash into the cache.
    assert len(sys.platform) != 0, len(platform.machine()) != 0
    # Use environment from command so we can access MinGW on windows.
    env = command.PlatformEnvironment([])

    def GetCompilerVersion(compiler_name):
      try:
        compiler_file = pynacl.file_tools.Which(
            compiler_name, paths=env['PATH'].split(os.pathsep))
        p = subprocess.Popen([compiler_file, '-v'], stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, env=env)
        _, compiler_version = p.communicate()
        assert p.returncode == 0
      except pynacl.file_tools.ExecutableNotFound:
        compiler_version = 0
      return compiler_version

    items = [
        ('platform', sys.platform),
        ('machine', platform.machine()),
        ('gcc-v', GetCompilerVersion('gcc')),
        ('arm-gcc-v', GetCompilerVersion('arm-linux-gnueabihf-gcc')),
        ]
    self._system_summary = str(items)
    return self._system_summary

  def BuildSignature(self, package, inputs, commands, hasher=None):
    """Compute a total checksum for a computation.

    The computed hash includes system properties, inputs, and the commands run.
    Args:
      package: The name of the package computed.
      inputs: A dict of names -> files/directories to be included in the
              inputs set.
      commands: A list of command.Command objects describing the commands run
                for this computation.
      hasher: Optional hasher to use.
    Returns:
      A hex formatted sha1 to use as a computation key or a human readable
      signature.
    """
    if hasher is None:
      h = hashlib.sha1()
    else:
      h = hasher

    h.update('package:' + package)
    h.update('summary:' + self.SystemSummary())
    for command in commands:
      h.update('command:')
      h.update(str(command))
    for key in sorted(inputs.keys()):
      h.update('item_name:' + key + '\x00')
      if inputs[key] in self._path_hash_cache:
        path_hash = self._path_hash_cache[inputs[key]]
      else:
        path_hash = 'item:' + pynacl.hashing_tools.StableHashPath(inputs[key])
        self._path_hash_cache[inputs[key]] = path_hash
      h.update(path_hash)
    return h.hexdigest()
