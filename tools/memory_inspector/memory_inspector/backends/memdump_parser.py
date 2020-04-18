# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This parser turns the am memdump output into a |memory_map.Map| instance."""

import base64
import logging
import re

from memory_inspector.core import memory_map


def Parse(content):
  """Parses the output of memdump.

  memdump (see chrome/src/tools/memdump) is a Linux/Android binary meant to be
  executed on the target device which extracts memory map information about one
  or more processes. In principle is can be seen as an alternative to cat-ing
  /proc/PID/smaps, but with extra features (multiprocess accounting and resident
  pages reporting).

  The expected memdump output looks like this:
  ------------------------------------------------------------------------------
  [ PID=1234]
  1000-2000 r-xp 0 private_unevictable=4096 private=8192 shared_app=[] \
      shared_other_unevictable=4096 shared_other=4096 "/lib/foo.so" [v///fv0D]
  ... other entries like the one above.
  ------------------------------------------------------------------------------
  The output is extremely similar to /proc/PID/smaps, with the following notes:
   - unevictable has pretty much the same meaning of "dirty", in VM terms.
   - private and shared_other are cumulative. This means the the "clean" part
     must be calculated as difference of (private - private_unevictable).
   - The final field [v///fv0D] is a base64 encoded bitmap which contains the
     information about which pages inside the mapping are resident (present).
  See tests/android_backend_test.py for a more complete example.

  Args:
      content: string containing the memdump output.

  Returns:
      An instance of |memory_map.Map|.
  """
  RE = (r'^([0-9a-f]+)-([0-9a-f]+)\s+'
        r'([rwxps-]{4})\s+'
        r'([0-9a-f]+)\s+'
        r'private_unevictable=(\d+) private=(\d+) '
        r'shared_app=(.*?) '
        r'shared_other_unevictable=(\d+) shared_other=(\d+) '
        r'\"(.*)\" '
        r'\[([a-zA-Z0-9+/=-_:]*)\]$')
  map_re = re.compile(RE)
  skip_first_n_lines = 1
  maps = memory_map.Map()

  for line in content.splitlines():
    line = line.rstrip('\r\n')

    if skip_first_n_lines > 0:
      skip_first_n_lines -= 1
      continue

    m = map_re.match(line)
    if not m:
      logging.warning('Skipping unrecognized memdump line "%s"' % line)
      continue

    start = int(m.group(1), 16)
    end = int(m.group(2), 16) - 1 # end addr is inclusive in memdump output.
    if (start > end):
      # Sadly, this actually happened. Probably a kernel bug, see b/17402069.
      logging.warning('Skipping unfeasible mmap "%s"' % line)
      continue
    entry = memory_map.MapEntry(
        start=start,
        end=end,
        prot_flags=m.group(3),
        mapped_file=m.group(10),
        mapped_offset=int(m.group(4), 16))
    entry.priv_dirty_bytes = int(m.group(5))
    entry.priv_clean_bytes = int(m.group(6)) - entry.priv_dirty_bytes
    entry.shared_dirty_bytes = int(m.group(8))
    entry.shared_clean_bytes = int(m.group(9)) - entry.shared_dirty_bytes
    entry.resident_pages = [ord(c) for c in base64.b64decode(m.group(11))]
    maps.Add(entry)

  return maps
