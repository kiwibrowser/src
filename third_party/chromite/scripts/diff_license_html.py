# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Compares the packages between 2 images by parsing the license file output."""

from __future__ import print_function

import re

from chromite.lib import commandline


def GetPackagesLicensesFromHtml(html_file):
  """Get the list of packages and licenses in a ChromeOS license file.

  Args:
    html_file: which html license file to scan for packages.

  Returns:
    tuple of dictionary of packages and version numbers and set of licenses.

  Raises:
    AssertionError: if regex failed.
  """

  packages = {}
  licenses = set()

  pkg_rgx = re.compile(r'<span class="title">(.+)-(.+)</span>')
  # Do not add <pre> in the regex or it would only show the first entry on
  # a package that has multiple hits.
  license_rgx1 = re.compile(r'Scanned (Source License .+):', re.IGNORECASE)
  license_rgx2 = re.compile(r'(Custom License .+):', re.IGNORECASE)
  license_rgx3 = re.compile(r'(Copyright Attribution .+):', re.IGNORECASE)
  # This regex isn't as tight because it has to match these:
  # Gentoo Package Stock License BZIP2:
  # <a ... class="title">Gentoo Package Provided Stock License public-domain</a>
  # <a ... class="title">Gentoo Package Stock License public-domain</a>
  license_rgx4 = re.compile(r'(Stock License [^<:]+)', re.IGNORECASE)
  license_rgx5 = re.compile(r'class="title">(Custom License .+)</a>',
                            re.IGNORECASE)
  with open(html_file, 'r') as f:
    for line in f:
      # Grep and turn
      # <span class="title">ath6k-34</span>
      # into
      # ath6k 34
      match = pkg_rgx.search(line)
      if match:
        packages[match.group(1)] = match.group(2)

      match = license_rgx1.search(line)
      if match:
        # Turn Source license simplejson-2.5.0/LICENSE.txt
        # into Source license simplejson/LICENSE.txt
        # (we don't want to create diffs based on version numbers)
        lic = re.sub(r'(.+)-([^/]+)/(.+)', r'\1/\3', match.group(1))
        # Old files had this lowercased.
        lic = re.sub(r'Source license', r'Source License', lic)
        licenses.add(lic)

      for rgx in (license_rgx2, license_rgx3, license_rgx4, license_rgx5):
        match = rgx.search(line)
        if match:
          licenses.add(match.group(1))

  return (packages, licenses)


def ComparePkgLists(pkg_list1, pkg_list2):
  """Compare the package list in 2 dictionaries and output the differences.

  Args:
    pkg_list1: dict from GetPackagesLicensesFromHtml.
    pkg_list2: dict from GetPackagesLicensesFromHtml.

  Returns:
    N/A (outputs result on stdout).
  """

  for removed_package in sorted(set(pkg_list1) - set(pkg_list2)):
    print('Package removed: %s-%s' % (
        removed_package, pkg_list1[removed_package]))

  print()
  for added_package in sorted(set(pkg_list2) - set(pkg_list1)):
    print('Package added: %s-%s' % (
        added_package, pkg_list2[added_package]))

  print()
  for changed_package in sorted(set(pkg_list1) & set(pkg_list2)):
    ver1 = pkg_list1[changed_package]
    ver2 = pkg_list2[changed_package]
    if ver1 != ver2:
      print('Package updated: %s from %s to %s' % (changed_package, ver1, ver2))


def CompareLicenseSets(set1, set2):
  """Compare the license list in 2 sets and output the differences.

  Args:
    set1: set from GetPackagesLicensesFromHtml.
    set2: set from GetPackagesLicensesFromHtml.

  Returns:
    N/A (outputs result on stdout).
  """

  for removed_license in sorted(set1 - set2):
    print('License removed: %s' % (removed_license))

  print()
  for added_license in sorted(set2 - set1):
    print('License added: %s' % (added_license))


def main(args):
  parser = commandline.ArgumentParser(usage=__doc__)
  parser.add_argument('html1', metavar='license1.html', type='path',
                      help='old html file')
  parser.add_argument('html2', metavar='license2.html', type='path',
                      help='new html file')
  opts = parser.parse_args(args)

  pkg_list1 = GetPackagesLicensesFromHtml(opts.html1)
  pkg_list2 = GetPackagesLicensesFromHtml(opts.html2)
  ComparePkgLists(pkg_list1[0], pkg_list2[0])
  print()
  CompareLicenseSets(pkg_list1[1], pkg_list2[1])
