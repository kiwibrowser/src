#!/usr/bin/python
# Copyright 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import tempfile
import re
import shutil

def FixupAsmjs(filename):
  """Applies various fixups as necessary to an Asm.js file, including:
  1. Initialize argv from the URL,
     e.g. http://host/asmjs.html?arg1&arg2&arg3
  2. (currently disabled) Try to close the tab/window when main() completes
     to facilitate scripting.
  """
  with open(filename, "r") as infile:
    with tempfile.NamedTemporaryFile(delete=False) as tmpfile:
      tmpfilename = tmpfile.name
      for line in infile:
        # This fixup executes window.close() when main() completes,
        # in the hope that Firefox will exit and a testing script can
        # move to the next test.  Unfortunately this is unreliable
        # in Firefox, so it is disabled for now.
        if (False and re.search("if \(Module\['postRun'\]\)", line)):
          tmpfile.write("window.open('', '_self', ''); " +
                        "window.close();\n");
        tmpfile.write(line)
        # Initialize argv from the URL arguments
        if re.search("args = args .. Module\['arguments'\];", line):
          tmpfile.write("args = document.location.search" +
                        ".substring(1).split('&');\n")
  shutil.move(tmpfilename, filename)

if __name__ == '__main__':
  import sys
  sys.exit(FixupAsmjs(sys.argv[1]))
