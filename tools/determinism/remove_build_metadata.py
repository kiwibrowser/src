#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Remove the build metadata embedded in the artifacts of a build."""

import json
import multiprocessing
import optparse
import os
import platform
import Queue
import shutil
import subprocess
import sys
import tempfile
import threading
import zipfile


BASE_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.dirname(os.path.dirname(BASE_DIR))

# Files that can't be processed by zap_timestamp.exe.
_ZAP_TIMESTAMP_BLACKLIST = {
  'mini_installer.exe',
}


def build_bitness(build_dir):
  # This function checks whether the target (not host) word size is 64-bits.
  # Since 64-bit hosts can cross-compile 32-bit binaries, check the GN args to
  # see what CPU we're targetting.
  try:
    args_gn = os.path.join(build_dir, 'args.gn')
    with open(args_gn) as f:
      for line in f:
        decommented = line.split('#', 1)[0]
        key_and_value = decommented.split('=', 1)
        if len(key_and_value) != 2:
          continue
        key = key_and_value[0].strip()
        value = key_and_value[1].strip()
        if key == 'target_cpu':
          if value.endswith('64'):
            return 64
          else:
            return 32
  except:
    pass
  # If we don't find anything, or if there is no GN args file, default to the
  # host architecture.
  if platform.machine().endswith('64'):
    return 64
  return 32


def get_files_to_clean(build_dir, recursive=False):
  """Get the list of files to clean."""
  allowed = frozenset(
      ('', '.app', '.dll', '.dylib', '.exe', '.nexe', '.so'))
  non_x_ok_exts = frozenset(('.isolated', '.jar'))
  min_timestamp = 0
  if os.path.exists(os.path.join(build_dir, 'build.ninja')):
    min_timestamp = os.path.getmtime(os.path.join(build_dir, 'build.ninja'))

  def check(f):
    if not os.path.isfile(f) or os.path.basename(f).startswith('.'):
      return False
    if os.path.getmtime(os.path.join(build_dir, f)) < min_timestamp:
      return False
    ext = os.path.splitext(f)[1]
    return (ext in non_x_ok_exts) or (ext in allowed and os.access(f, os.X_OK))

  ret_files = set()
  for root, dirs, files in os.walk(build_dir):
    if not recursive:
      dirs[:] = [d for d in dirs if d.endswith(('_apk', 'lib.java'))]
    for f in (f for f in files if check(os.path.join(root, f))):
      ret_files.add(os.path.relpath(os.path.join(root, f), build_dir))
  return ret_files


def run_zap_timestamp(filepath):
  """Run zap_timestamp.exe on a PE binary."""
  assert sys.platform == 'win32'
  syzygy_dir = os.path.join(
      SRC_DIR, 'third_party', 'syzygy', 'binaries', 'exe')
  zap_timestamp_exe = os.path.join(syzygy_dir, 'zap_timestamp.exe')
  sys.stdout.write('Processing: %s\n' % os.path.basename(filepath))
  proc = subprocess.Popen(
      [zap_timestamp_exe, '--input-image=%s' % filepath, '--overwrite'],
      stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
  log, _ = proc.communicate()
  if proc.returncode != 0:
    sys.stderr.write('%s failed:\n%s\n' % (os.path.basename(filepath), log))
  return proc.returncode


def remove_pe_metadata(filename, bitness):
  """Remove the build metadata from a PE file."""
  # Only run zap_timestamp on the 32-bit PE files for which we have a PDB.
  ret = 0
  if ((not os.path.basename(filename) in _ZAP_TIMESTAMP_BLACKLIST) and
      os.path.exists(filename + '.pdb') and bitness != 64):
    ret = run_zap_timestamp(filename)
  return ret


def remove_zip_timestamps(filename):
  """Remove the timestamps embedded in a zip archive."""
  sys.stdout.write('Processing: %s\n' % os.path.basename(filename))
  with zipfile.ZipFile(filename, 'r') as zf:
    # Creates a temporary file.
    out_file, out_filename = tempfile.mkstemp(prefix='remote_apk_timestamp')
    os.close(out_file)
    try:
      with zipfile.ZipFile(out_filename, 'w') as zf_o:
        # Copy the data from the original file to the new one.
        for info in zf.infolist():
          # Overwrite the timestamp with a constant value.
          info.date_time = (1980, 1, 1, 0, 0, 0)
          zf_o.writestr(info, zf.read(info.filename))
      # Remove the original file and replace it by the modified one.
      os.remove(filename)
      shutil.move(out_filename, filename)
    finally:
      if os.path.isfile(out_filename):
        os.remove(out_filename)


def remove_metadata_worker(file_queue, failed_queue, build_dir, bitness):
  """Worker thread for the remove_metadata function."""
  while True:
    f = file_queue.get()
    if f.endswith(('.dll', '.exe')):
      if remove_pe_metadata(os.path.join(build_dir, f), bitness):
        failed_queue.put(f)
    elif f.endswith('.jar'):
      remove_zip_timestamps(os.path.join(build_dir, f))
    file_queue.task_done()


def remove_metadata(build_dir, recursive, bitness):
  """Remove the build metadata from the artifacts of a build."""
  with open(os.path.join(BASE_DIR, 'deterministic_build_blacklist.json')) as f:
    blacklist = frozenset(json.load(f))
  files = Queue.Queue()
  for f in get_files_to_clean(build_dir, recursive) - blacklist:
    files.put(f)
  failed_files = Queue.Queue()

  for _ in xrange(multiprocessing.cpu_count()):
    worker = threading.Thread(target=remove_metadata_worker,
                              args=(files,
                                    failed_files,
                                    build_dir,
                                    bitness))
    worker.daemon = True
    worker.start()

  files.join()
  if not failed_files.empty():
    print >> sys.stderr, 'Failed for the following files:'
    failed_files_list = []
    while not failed_files.empty():
      failed_files_list.append(failed_files.get())
    print >> sys.stderr, '\n'.join('  ' + i for i in sorted(failed_files_list))
    return 1

  return 0


def main():
  parser = optparse.OptionParser(usage='%prog [options]')
  # TODO(sebmarchand): Add support for reading the list of artifact from a
  # .isolated file.
  parser.add_option('--build-dir', help='The build directory.')
  parser.add_option('-r', '--recursive', action='store_true', default=False,
                    help='Indicates if the script should be recursive.')
  options, _ = parser.parse_args()

  if not options.build_dir:
    parser.error('--build-dir is required')

  bitness = build_bitness(options.build_dir)

  return remove_metadata(options.build_dir, options.recursive, bitness)


if __name__ == '__main__':
  sys.exit(main())
