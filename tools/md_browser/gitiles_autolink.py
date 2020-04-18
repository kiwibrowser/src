# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Implements Gitiles' simpler auto linking.

This extention auto links basic URLs that aren't bracketed by <...>.

https://gerrit.googlesource.com/gitiles/+/master/gitiles-servlet/src/main/java/com/google/gitiles/Linkifier.java
"""

from markdown.inlinepatterns import (AutolinkPattern, Pattern)
from markdown.extensions import Extension


AUTOLINK_RE = r'([Hh][Tt][Tt][Pp][Ss]?://[^>]*)'


class _GitilesSmartQuotesExtension(Extension):
  """Add Gitiles' simpler linkifier to Markdown."""
  def extendMarkdown(self, md, md_globals):
    md.inlinePatterns.add('gitilesautolink',
                          AutolinkPattern(AUTOLINK_RE, md),
                          '<autolink')


def makeExtension(*args, **kwargs):
  return _GitilesSmartQuotesExtension(*args, **kwargs)
