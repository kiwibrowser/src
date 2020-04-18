# -*- coding: utf-8 -*-
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Terminal utilities

This module handles terminal interaction including ANSI color codes.
"""

from __future__ import print_function

import os

from chromite.lib import cros_build_lib


class Color(object):
  """Conditionally wraps text in ANSI color escape sequences."""
  BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE = range(8)
  BOLD = -1
  COLOR_START = '\033[1;%dm'
  BOLD_START = '\033[1m'
  RESET = '\033[0m'

  def __init__(self, enabled=None):
    """Create a new Color object, optionally disabling color output.

    Args:
      enabled: True if color output should be enabled. If False then this
        class will not add color codes at all.
    """
    self._enabled = self.UserEnabled() if enabled is None else enabled

  def Start(self, color):
    """Returns a start color code.

    Args:
      color: Color to use, .e.g BLACK, RED, etc.

    Returns:
      If color is enabled, returns an ANSI sequence to start the given color,
      otherwise returns empty string
    """
    if self._enabled:
      return self.COLOR_START % (color + 30)
    return ''

  def Stop(self):
    """Returns a stop color code.

    Returns:
      If color is enabled, returns an ANSI color reset sequence, otherwise
      returns empty string
    """
    if self._enabled:
      return self.RESET
    return ''

  def Color(self, color, text):
    """Returns text with conditionally added color escape sequences.

    Keyword arguments:
      color: Text color -- one of the color constants defined in this class.
      text: The text to color.

    Returns:
      If self._enabled is False, returns the original text. If it's True,
      returns text with color escape sequences based on the value of color.
    """
    if not self._enabled:
      return text
    if color == self.BOLD:
      start = self.BOLD_START
    else:
      start = self.COLOR_START % (color + 30)
    return start + text + self.RESET

  @staticmethod
  def UserEnabled():
    """See if the global colorization preference is enabled ($NOCOLOR env)"""
    return not cros_build_lib.BooleanShellValue(
        os.environ.get('NOCOLOR'), msg='$NOCOLOR env var is invalid',
        default=False)
