# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Updates the ShouldAllowOpenURLFailureScheme enum in histograms.xml file with
values read from chrome_content_browser_client_extensions_part.cc.

If the file was pretty-printed, the updated version is pretty-printed too.
"""

import os
import sys

from update_histogram_enum import UpdateHistogramEnum

if __name__ == '__main__':
  if len(sys.argv) > 1:
    print >>sys.stderr, 'No arguments expected!'
    sys.stderr.write(__doc__)
    sys.exit(1)

  source_file = 'chrome/browser/extensions/' \
                'chrome_content_browser_client_extensions_part.cc'
  UpdateHistogramEnum(histogram_enum_name='ShouldAllowOpenURLFailureScheme',
                      source_enum_path=source_file,
                      start_marker='^enum ShouldAllowOpenURLFailureScheme {',
                      end_marker='^SCHEME_LAST')
