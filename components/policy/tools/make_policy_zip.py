#!/usr/bin/env python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Creates a zip archive with policy template files.
"""

import optparse
import sys
import zipfile


def add_files_to_zip(zip_file, base_dir, file_list):
  """Pack a list of files into a zip archive, that is already opened for
  writing.

  Args:
    zip_file: An object representing the zip archive.
    base_dir: Base path of all the files in the real file system.
    file_list: List of absolute file paths to add. Must start with base_dir.
        The base_dir is stripped in the zip file entries.
  """
  if (base_dir[-1] != '/'):
    base_dir += '/'
  for file_path in file_list:
    assert file_path.startswith(base_dir)
    zip_file.write(file_path, file_path[len(base_dir):])
  return 0


def main(argv):
  """Pack a list of files into a zip archive.

  Args:
    output: The file path of the zip archive.
    base_dir: Base path of input files.
    languages: Comma-separated list of languages, e.g. en-US,de.
    add: List of files to include in the archive. The language placeholder
         ${lang} is expanded into one file for each language.
  """
  parser = optparse.OptionParser()
  parser.add_option("--output", dest="output")
  parser.add_option("--base_dir", dest="base_dir")
  parser.add_option("--languages", dest="languages")
  parser.add_option("--add", action="append", dest="files", default=[])
  options, args = parser.parse_args(argv[1:])

  # Process file list, possibly expanding language placeholders.
  _LANG_PLACEHOLDER = "${lang}";
  languages = filter(bool, options.languages.split(','))
  file_list = []
  for file_to_add in options.files:
    if (_LANG_PLACEHOLDER in file_to_add):
      for lang in languages:
        file_list.append(file_to_add.replace(_LANG_PLACEHOLDER, lang))
    else:
      file_list.append(file_to_add)

  zip_file = zipfile.ZipFile(options.output, 'w', zipfile.ZIP_DEFLATED)
  try:
    return add_files_to_zip(zip_file, options.base_dir, file_list)
  finally:
    zip_file.close()


if '__main__' == __name__:
  sys.exit(main(sys.argv))
