# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This script (intended to be invoked by autoninja or autoninja.bat) detects
whether a build is using goma. If so it runs with a large -j value, and
otherwise it chooses a small one. This auto-adjustment makes using goma simpler
and safer, and avoids errors that can cause slow goma builds or swap-storms
on non-goma builds.
"""

import multiprocessing
import os
import re
import sys

# The -t tools are incompatible with -j and -l
t_specified = False
j_specified = False
output_dir = '.'
input_args = sys.argv
# On Windows the autoninja.bat script passes along the arguments enclosed in
# double quotes. This prevents multiple levels of parsing of the special '^'
# characters needed when compiling a single file but means that this script gets
# called with a single argument containing all of the actual arguments,
# separated by spaces. When this case is detected we need to do argument
# splitting ourselves. This means that arguments containing actual spaces are
# not supported by autoninja, but that is not a real limitation.
if (sys.platform.startswith('win') and len(sys.argv) == 2 and
    input_args[1].count(' ') > 0):
  input_args = sys.argv[:1] + sys.argv[1].split()
for index, arg in enumerate(input_args[1:]):
  if arg == '-j':
    j_specified = True
  if arg == '-t':
    t_specified = True
  if arg == '-C':
    # + 1 to get the next argument and +1 because we trimmed off input_args[0]
    output_dir = input_args[index + 2]

use_goma = False
try:
  # If GOMA_DISABLED is set (to anything) then gomacc will use the local
  # compiler instead of doing a goma compile. This is convenient if you want
  # to briefly disable goma. It avoids having to rebuild the world when
  # transitioning between goma/non-goma builds. However, it is not as fast as
  # doing a "normal" non-goma build because an extra process is created for each
  # compile step. Checking this environment variable ensures that autoninja uses
  # an appropriate -j value in this situation.
  if 'GOMA_DISABLED' not in os.environ:
    with open(os.path.join(output_dir, 'args.gn')) as file_handle:
      for line in file_handle:
        # This regex pattern copied from create_installer_archive.py
        m = re.match('^\s*use_goma\s*=\s*true(\s*$|\s*#.*$)', line)
        if m:
          use_goma = True
except IOError:
  pass

if sys.platform.startswith('win'):
  # Specify ninja.exe on Windows so that ninja.bat can call autoninja and not
  # be called back.
  args = ['ninja.exe'] + input_args[1:]
else:
  args = ['ninja'] + input_args[1:]

num_cores = multiprocessing.cpu_count()
if not j_specified and not t_specified:
  if use_goma:
    args.append('-j')
    core_multiplier = int(os.environ.get("NINJA_CORE_MULTIPLIER", "20"))
    args.append('%d' % (num_cores * core_multiplier))
  else:
    core_addition = os.environ.get("NINJA_CORE_ADDITION")
    if core_addition:
      core_addition = int(core_addition)
      args.append('-j')
      args.append('%d' % (num_cores + core_addition))

if not t_specified:
  # Specify a maximum CPU load so that running builds in two different command
  # prompts won't overload the system too much. This is not reliable enough to
  # be used to auto-adjust between goma/non-goma loads, but it is a nice
  # fallback load balancer.
  args.append('-l')
  args.append('%d' % num_cores)

print ' '.join(args)
