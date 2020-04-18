#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import binascii
import cStringIO
import gzip
import hashlib
import json
import os
import urllib2
import xml.etree.ElementTree

PACKAGE_FILTER = [
    "ld-linux-x86-64.so",
    "libX11-xcb.so",
    "libX11.so",
    "libXcomposite.so",
    "libXcursor.so",
    "libXdamage.so",
    "libXext.so",
    "libXfixes.so",
    "libXi.so",
    "libXrandr.so",
    "libXrender.so",
    "libXss.so",
    "libXtst.so",
    "libappindicator3.so",
    "libasound.so",
    "libatk-1.0.so",
    "libatk-bridge-2.0.so",
    "libc.so",
    "libcairo.so",
    "libcups.so",
    "libdbus-1.so",
    "libdl.so",
    "libexpat.so",
    "libgcc_s.so",
    "libgdk-3.so",
    "libgdk_pixbuf-2.0.so",
    "libgio-2.0.so",
    "libglib-2.0.so",
    "libgmodule-2.0.so",
    "libgobject-2.0.so",
    "libgtk-3.so",
    "libm.so",
    "libnspr4.so",
    "libnss3.so",
    "libnssutil3.so",
    "libpango-1.0.so",
    "libpangocairo-1.0.so",
    "libpthread.so",
    "librt.so",
    "libsmime3.so",
    "libstdc++.so",
    "libxcb.so",
    "rtld(GNU_HASH)",
]

SUPPORTED_FEDORA_RELEASES = ['25', '26', '27']
SUPPORTED_OPENSUSE_LEAP_RELEASES = ['42.2', '42.3']

COMMON_NS = "http://linux.duke.edu/metadata/common"
RPM_NS = "http://linux.duke.edu/metadata/rpm"
REPO_NS = "http://linux.duke.edu/metadata/repo"

rpm_sources = {}
for version in SUPPORTED_FEDORA_RELEASES:
  rpm_sources['Fedora ' + version] = [
      "https://download.fedoraproject.org/pub/fedora/linux/releases/%s/Everything/x86_64/os/" % version,
      # 'updates' must appear after 'releases' since its entries
      # overwrite the originals.
      "https://download.fedoraproject.org/pub/fedora/linux/updates/%s/x86_64/" % version,
  ]
for version in SUPPORTED_OPENSUSE_LEAP_RELEASES:
    rpm_sources['openSUSE Leap ' + version] = [
        "https://download.opensuse.org/distribution/leap/%s/repo/oss/suse/" % version,
        # 'update' must appear after 'distribution' since its entries
        # overwrite the originals.
        "https://download.opensuse.org/update/leap/%s/oss/" % version,
  ]

provides = {}
for distro in rpm_sources:
  distro_provides = {}
  for source in rpm_sources[distro]:
    # |source| may redirect to a real download mirror.  However, these
    # mirrors may be out-of-sync with each other.  Follow the redirect
    # to ensure the file-references from the metadata file are valid.
    source = urllib2.urlopen(source).geturl()

    response = urllib2.urlopen(source + "repodata/repomd.xml")
    repomd = xml.etree.ElementTree.fromstring(response.read())
    primary = source + repomd.find("./{%s}data[@type='primary']/{%s}location" %
                                   (REPO_NS, REPO_NS)).attrib['href']
    expected_checksum = repomd.find(
        "./{%s}data[@type='primary']/{%s}checksum[@type='sha256']" %
        (REPO_NS, REPO_NS)).text

    response = urllib2.urlopen(primary)
    gz_data = response.read()

    sha = hashlib.sha256()
    sha.update(gz_data)
    actual_checksum = binascii.hexlify(sha.digest())
    assert expected_checksum == actual_checksum

    zipped_file = cStringIO.StringIO()
    zipped_file.write(gz_data)
    zipped_file.seek(0)
    contents = gzip.GzipFile(fileobj=zipped_file, mode='rb').read()
    metadata = xml.etree.ElementTree.fromstring(contents)
    for package in metadata.findall('./{%s}package' % COMMON_NS):
      if package.find('./{%s}arch' % COMMON_NS).text != 'x86_64':
        continue
      package_name = package.find('./{%s}name' % COMMON_NS).text
      package_provides = []
      for entry in package.findall('./{%s}format/{%s}provides/{%s}entry' %
                                   (COMMON_NS, RPM_NS, RPM_NS)):
        name = entry.attrib['name']
        for prefix in PACKAGE_FILTER:
          if name.startswith(prefix):
            package_provides.append(name)
      distro_provides[package_name] = package_provides
  provides[distro] = sorted(list(set(
      [package_provides for package in distro_provides
       for package_provides in distro_provides[package]])))


script_dir = os.path.dirname(os.path.realpath(__file__))
with open(os.path.join(script_dir, 'dist_package_provides.json'), 'w') as f:
  f.write(json.dumps(provides, sort_keys=True, indent=4,
                     separators=(',', ': ')))
  f.write('\n')
