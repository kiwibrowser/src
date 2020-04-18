#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A simple script for downloading latest dictionaries."""

import glob
import os
import sys
import urllib
from zipfile import ZipFile


def main():
  if not os.getcwd().endswith("hunspell_dictionaries"):
    print "Please run this file from the hunspell_dictionaries directory"
  dictionaries = (
      ("https://sourceforge.net/projects/wordlist/files/speller/2017.01.22/"
       "hunspell-en_US-2017.01.22.zip",
       "en_US.zip"),
      ("https://sourceforge.net/projects/wordlist/files/speller/2017.01.22/"
       "hunspell-en_CA-2017.01.22.zip",
       "en_CA.zip"),
      ("https://sourceforge.net/projects/wordlist/files/speller/2017.01.22/"
       "hunspell-en_GB-ise-2017.01.22.zip",
       "en_GB.zip"),
      ("https://sourceforge.net/projects/wordlist/files/speller/2017.01.22/"
       "hunspell-en_AU-2017.01.22.zip",
       "en_AU.zip"),
      ("http://downloads.sourceforge.net/lilak/"
       "lilak_fa-IR_2-1.zip",
       "fa_IR.zip")
  )
  for pair in dictionaries:
    url = pair[0]
    file_name = pair[1]

    urllib.urlretrieve(url, file_name)
    ZipFile(file_name).extractall()
    for name in glob.glob("*en_GB-ise*"):
      os.rename(name, name.replace("-ise", ""))
    os.remove(file_name)
  return 0

if __name__ == "__main__":
  sys.exit(main())
