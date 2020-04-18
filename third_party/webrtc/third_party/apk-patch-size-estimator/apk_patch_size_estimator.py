#!/usr/bin/python
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Estimates the size of Google Play patches and the new gzipped APK.

From two APKs it estimates the size of new patches as well as
the size of a gzipped version of the APK, which would be used in
cases where patches are unexpectedly large, unavailable, or unsuitable.
Google Play uses multiple techniques to generate patches and generally picks
the best match for the device. The best match is usually, but not always, the
smallest patch file produced. The numbers that this script produces are
ESTIMATES that can be used to characterize the impact of arbitrary changes to
APKs. There is NO GUARANTEE that this tool produces the same patches or patch
sizes that Google Play generates, stores or transmits, and the actual
implementation within Google Play may change at any time, without notice.

"""

import sys
import argparse
import locale
import math
import os
import subprocess

_FILEBYFILE_JAR_PATH = os.path.abspath(
    os.path.join(os.path.dirname(__file__), 'lib', 'file-by-file-tools.jar'))

bsdiff_path = None
gzip_path = None
head_path = None
tail_path = None
bunzip2_path = None
java_path = None


def find_bins_or_die():
  """Checks that all the binaries needed are available.

  The script needs bsdiff, gzip, head, tail and bunzip2
  binaries availables in the system.
  """

  global bsdiff_path
  if not bsdiff_path:
    bsdiff_path = find_binary('bsdiff')
  global gzip_path
  if not gzip_path:
    gzip_path = find_binary('gzip')
  global head_path
  if not head_path:
    head_path = find_binary('head')
  global tail_path
  if not tail_path:
    tail_path = find_binary('tail')
  global bunzip2_path
  if not bunzip2_path:
    bunzip2_path = find_binary('bunzip2')
  global java_path
  if not java_path:
    java_path = find_binary('java')


def find_binary(binary_name):
  """Finds the path of a binary."""

  try:
    return subprocess.check_output(['which', binary_name]).strip()
  except subprocess.CalledProcessError:
    raise Exception(
        'No "' + binary_name + '" on PATH, please install or fix PATH.')


def human_file_size(size):
  """Converts a byte size number into a human readable value."""

  size = abs(size)
  if size == 0:
    return '0B'
  units = ['B', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB']
  p = math.floor(math.log(size, 2) / 10)
  return '%.3g%s' % (size/math.pow(1024, p), units[int(p)])


def calculate_bsdiff(old_file, new_file, save_patch_path, temp_path):
  """Estimates the size the Bsdiff patch gzipped.

  Args:
    old_file: the old APK file
    new_file: the new APK file
    save_patch_path: the path including filename to save the generated patch.
    temp_path: the directory to use for the process

  Returns:
    a dictionary with:
      'gzipped_new_file_size': the estimated size of the new gzipped APK
      'bsdiff_patch_size': the estimated size of the patch from the two APKs

  Raises:
    Exception: if there is a problem calling the binaries needed in the process
  """

  # Oddities:
  # Bsdiff forces bzip2 compression, which starts after byte 32. Bzip2 isn't
  # necessarily the best choice in all cases, and isn't necessarily what Google
  # Play uses, so it has to be uncompressed and rewritten with gzip.

  # Checks that the OS binaries needed are available
  find_bins_or_die()
  # Clean temp files
  if os.path.exists(temp_path): os.remove(temp_path)

  # Create the bsdiff of the two APKs
  subprocess.check_output(
      [bsdiff_path, old_file, new_file, temp_path])

  # bsdiff paths
  raw_bsdiff_path = temp_path + '.raw_bsdiff'
  bzipped_bsdiff_path = raw_bsdiff_path + '.bz2'
  gzipped_bsdiff_path = raw_bsdiff_path + '.gz'
  bsdiff_header_path = temp_path + '.raw_bsdiff_header'
  if os.path.exists(raw_bsdiff_path): os.remove(raw_bsdiff_path)
  if os.path.exists(bzipped_bsdiff_path): os.remove(bzipped_bsdiff_path)
  if os.path.exists(gzipped_bsdiff_path): os.remove(gzipped_bsdiff_path)
  if os.path.exists(bsdiff_header_path): os.remove(bsdiff_header_path)

  # Strip the first 32 bytes the bsdiff file, which is a bsdiff-specific header.
  bsdiff_header = open(bsdiff_header_path, 'w')
  p = subprocess.Popen(
      [head_path, '-c', '32', bsdiff_header_path],
      shell=False, stdout=bsdiff_header)
  ret_code = p.wait()
  if ret_code != 0:
    raise Exception('Problem at the bsdiff step, returned code: %s' % ret_code)
  bsdiff_header.flush()
  bsdiff_header.close()

  # Take the remainder of the file to gain an uncompressed copy.
  bzipped_bsdiff_patch = open(bzipped_bsdiff_path, 'w')
  p = subprocess.Popen(
      [tail_path, '-c', '+33', temp_path],
      shell=False, stdout=bzipped_bsdiff_patch)
  ret_code = p.wait()
  if ret_code != 0:
    raise Exception('Problem at the tail step, returned code: %s' % ret_code)
  bzipped_bsdiff_patch.flush()
  bzipped_bsdiff_patch.close()
  subprocess.check_output([bunzip2_path, '-d', '-q', bzipped_bsdiff_path])

  # Prepend the 32 bytes of bsdiff header back onto the uncompressed file.
  if save_patch_path:
    rebuilt_bsdiff_path = save_patch_path + '-bsdiff-patch'
  else:
    rebuilt_bsdiff_path = raw_bsdiff_path + '.rebuilt'
  gzipped_rebuilt_bsdiff_path = rebuilt_bsdiff_path + '.gz'
  if os.path.exists(rebuilt_bsdiff_path): os.remove(rebuilt_bsdiff_path)
  if os.path.exists(gzipped_rebuilt_bsdiff_path):
    os.remove(gzipped_rebuilt_bsdiff_path)
  rebuilt_bsdiff = open(rebuilt_bsdiff_path, 'w')
  p = subprocess.Popen(
      ['cat', bsdiff_header_path, raw_bsdiff_path],
      shell=False, stdout=rebuilt_bsdiff)
  ret_code = p.wait()
  if ret_code != 0:
    raise Exception('Problem at the cat step, returned code: %s' % ret_code)
  rebuilt_bsdiff.flush()
  rebuilt_bsdiff.close()

  # gzip the patch and get its size.
  subprocess.check_output([gzip_path, '-9', rebuilt_bsdiff_path])
  bsdiff_patch_size = os.stat(gzipped_rebuilt_bsdiff_path).st_size

  # Clean up.
  if os.path.exists(temp_path): os.remove(temp_path)
  if os.path.exists(raw_bsdiff_path): os.remove(raw_bsdiff_path)
  if os.path.exists(bsdiff_header_path): os.remove(bsdiff_header_path)
  if os.path.exists(gzipped_bsdiff_path): os.remove(gzipped_bsdiff_path)
  if not save_patch_path and os.path.exists(gzipped_rebuilt_bsdiff_path):
    os.remove(gzipped_rebuilt_bsdiff_path)

  return bsdiff_patch_size


def calculate_new_apk(new_file, temp_path):
  """Estimates the size the new APK gzipped.

  Args:
    new_file: the new APK file
    temp_path: the directory to use for the process

  Returns:
    the size of the new APK gzipped

  Raises:
    Exception: if there is a problem calling the binaries needed in the process
  """

  # Checks that the OS binaries needed are available
  find_bins_or_die()
  # Clean temp files
  if os.path.exists(temp_path + '.gz'): os.remove(temp_path + '.gz')

  # gzip new APK and get its size
  gzipped_new_file = open(temp_path, 'w')
  p = subprocess.Popen(
      [gzip_path, '--keep', '-c', '-9', new_file],
      shell=False, stdout=gzipped_new_file)
  ret_code = p.wait()
  if ret_code != 0: raise Exception(
      'Problem gzipping the new APK, returned code: %s' % ret_code)
  gzipped_new_file.flush()
  gzipped_new_file.close()
  gzipped_size = os.stat(temp_path).st_size
  # Clean up
  if os.path.exists(temp_path + '.gz'): os.remove(temp_path + '.gz')
  return gzipped_size


def calculate_filebyfile(old_file, new_file, save_patch_path, temp_path):
  """Estimates the size the File-by-File patch gzipped.

  Args:
    old_file: the old APK file
    new_file: the new APK file
    save_patch_path: the path including filename to save the generated patch.
    temp_path: the directory to use for the process

  Returns:
    the size the File-by-File patch gzipped

  Raises:
    Exception: if there is a problem calling the binaries needed in the process
  """

  # Checks that the OS binaries needed are available
  find_bins_or_die()
  # Clean temp files
  if os.path.exists(temp_path): os.remove(temp_path)

  if save_patch_path:
    filebyfile_patch_path = save_patch_path + '-file-by-file-patch'
  else:
    filebyfile_patch_path = temp_path + '.filebyfile'
  gzipped_filebyfile_patch_path = filebyfile_patch_path + '.gz'
  if os.path.exists(gzipped_filebyfile_patch_path):
    os.remove(gzipped_filebyfile_patch_path)

  # file by file patch
  # We use a jar from https://github.com/andrewhayden/archive-patcher
  if os.path.exists(filebyfile_patch_path): os.remove(filebyfile_patch_path)
  p = subprocess.Popen(
      [java_path, '-jar', _FILEBYFILE_JAR_PATH, '--generate', '--old', old_file,
       '--new', new_file, '--patch', filebyfile_patch_path],
      shell=False)
  ret_code = p.wait()
  if ret_code != 0: raise Exception(
      'Problem creating file by file patch, returned code: %s' % ret_code)

  # gzip file by file patch and get its size
  subprocess.check_output([gzip_path, '-9', filebyfile_patch_path])
  gzipped_filebyfile_patch_size = os.stat(gzipped_filebyfile_patch_path).st_size
  # Clean temp files
  if os.path.exists(temp_path): os.remove(temp_path)
  if not save_patch_path and os.path.exists(gzipped_filebyfile_patch_path):
    os.remove(gzipped_filebyfile_patch_path)
  return gzipped_filebyfile_patch_size


def main():
  locale.setlocale(locale.LC_ALL, '')

  parser = argparse.ArgumentParser(
      description='Estimate the sizes of APK patches for Google Play')
  parser.add_argument(
      '--old-file', default=None, required=True,
      help='the path to the "old" file to generate patches from.')
  parser.add_argument(
      '--new-file', default=None, required=True,
      help='the path to the "new" file to generate patches from.')
  parser.add_argument(
      '--save-patch', default=None,
      help='the path prefix to save the generated patches.')
  parser.add_argument(
      '--temp-dir', default='/tmp',
      help='the temp directory to use for patch generation; defaults to /tmp')
  if not sys.argv[1:]:
    parser.print_help()
    parser.exit()
  args = parser.parse_args()

  if not os.path.isfile(args.old_file):
    raise Exception('File does not exist: %s' % args.old_file)
  if not os.path.isfile(args.new_file):
    raise Exception('File does not exist: %s' % args.new_file)
  if args.save_patch and not os.access(
      os.path.dirname(os.path.abspath(args.save_patch)), os.W_OK):
    raise Exception('The save patch path is not writable: %s' % args.save_patch)
  if args.save_patch and os.path.isdir(args.save_patch):
    raise Exception('Please include the filename in the path: %s'
                    % args.save_patch)
  save_patch_path = args.save_patch
  if not os.path.isdir(args.temp_dir):
    raise Exception('Temp directory does not exist: %s' % args.temp_dir)
  temp_path = args.temp_dir + '/patch.tmp'

  new_file_size = os.stat(args.new_file).st_size

  bsdiff_size = calculate_bsdiff(
      args.old_file, args.new_file, save_patch_path, temp_path)

  gzipped_size = calculate_new_apk(args.new_file, temp_path)

  # Calculate the size of the File-by-File patch gzipped
  gzipped_filebyfile_patch_size = calculate_filebyfile(
      args.old_file, args.new_file, save_patch_path, temp_path)

  print ('\nNew APK size on disk: %s bytes [%s]'
         % (locale.format('%d', new_file_size, grouping=True),
            human_file_size(new_file_size)))

  print '\nEstimated download size for new installs:'
  print ('   Full new APK (gzipped) size:'
         ' %s bytes [%s]'
         % (locale.format('%d', gzipped_size, grouping=True),
            human_file_size(gzipped_size)))

  print '\nEstimated download size for updates from the old APK, using Bsdiff:'
  print ('   Bsdiff patch (gzipped) size: %s bytes [%s]'
         % (locale.format('%d', bsdiff_size, grouping=True),
            human_file_size(bsdiff_size)))

  print '\nEstimated download size for updates from the old APK,'
  print ' using File-by-File:'
  print ('   File-by-File patch (gzipped) size: %s bytes [%s]\n'
         % (locale.format('%d', gzipped_filebyfile_patch_size, grouping=True),
            human_file_size(gzipped_filebyfile_patch_size)))


if __name__ == '__main__':
  main()
