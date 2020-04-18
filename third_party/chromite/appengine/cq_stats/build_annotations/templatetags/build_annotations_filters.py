# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Custom template tags for the build_annotations app."""

from __future__ import print_function

from django import template
from django.template import defaultfilters
from django.utils import safestring

register = template.Library()


@register.filter(needs_autoescape=True, is_safe=True)
@defaultfilters.stringfilter
def crosurlize(value, autoescape=None):
  """URLize strings.

  This builds on top of the url'ize function from django. In addition, it
  creates links for cros specific regexs.

  TODO(pprabhu) This should be merged with the (much more thorough) urlize
  functionality in the chromium_status AE app.
  """
  words = value.split(' ')
  for i in xrange(len(words)):
    is_url = False
    word = words[i]
    if (word.startswith('crbug.com/') or word.startswith('crosreview.com/') or
        word.startswith('b/')):
      parts = word.split('/')
      if len(parts) == 2:
        try:
          int(parts[1])
          is_url = True
        except ValueError:
          pass

    if is_url:
      # In-place urlize.
      words[i] = '<a href="http://%s" rel="nofollow">%s</a>' % (word, word)

  value = safestring.mark_safe(' '.join(words))
  return defaultfilters.urlize(value, autoescape)
