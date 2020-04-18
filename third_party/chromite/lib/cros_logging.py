# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Logging module to be used by all scripts.

cros_logging is a wrapper around logging with additional support for NOTICE
level. This is to be used instead of the default logging module. The new
logging level can only be used from here.
"""

from __future__ import print_function

import sys
# pylint: disable=unused-wildcard-import, wildcard-import
from logging import *
# pylint: enable=unused-wildcard-import, wildcard-import

# Have to import shutdown explicitly from logging because it is not included
# in logging's __all__.
# pylint: disable=unused-import
from logging import shutdown
# pylint: enable=unused-import

# Import as private to avoid polluting module namespace.
from chromite.lib import buildbot_annotations as _annotations


# Notice Level.
NOTICE = 25
addLevelName(NOTICE, 'NOTICE')


# Notice implementation.
def notice(message, *args, **kwargs):
  """Log 'msg % args' with severity 'NOTICE'."""
  log(NOTICE, message, *args, **kwargs)


# Only buildbot aware entry-points need to spew buildbot specific logs. Require
# user action for the special log lines.
_buildbot_markers_enabled = False
def EnableBuildbotMarkers():
  # pylint: disable=global-statement
  global _buildbot_markers_enabled
  _buildbot_markers_enabled = True


def _PrintForBuildbot(handle, annotation_class, *args):
  """Log a line for buildbot.

  This function dumps a line to log recognizable by buildbot if
  EnableBuildbotMarkers has been called. Otherwise, it dumps the same line in a
  human friendly way that buildbot ignores.

  Args:
    handle: The pipe to dump the log to. If None, log to sys.stderr.
    annotation_class: Annotation subclass for the type of buildbot log.
    buildbot_tag: A tag specifying the type of buildbot log.
    *args: The rest of the str arguments to be dumped to the log.
  """
  if handle is None:
    handle = sys.stderr
  # Cast each argument, because we end up getting all sorts of objects from
  # callers.
  str_args = [str(x) for x in args]
  annotation = annotation_class(*str_args)
  if _buildbot_markers_enabled:
    line = str(annotation)
  else:
    line = annotation.human_friendly
  handle.write('\n' + line + '\n')


def PrintBuildbotLink(text, url, handle=None):
  """Prints out a link to buildbot."""
  _PrintForBuildbot(handle, _annotations.StepLink, text, url)


def PrintBuildbotSetBuildProperty(name, data, handle=None):
  """Prints out a request to set a build property to a JSON value."""
  _PrintForBuildbot(handle, _annotations.SetBuildProperty, name, data)


def PrintBuildbotStepText(text, handle=None):
  """Prints out stage text to buildbot."""
  _PrintForBuildbot(handle, _annotations.StepText, text)


def PrintBuildbotStepWarnings(handle=None):
  """Marks a stage as having warnings."""
  _PrintForBuildbot(handle, _annotations.StepWarnings)


def PrintBuildbotStepFailure(handle=None):
  """Marks a stage as having failures."""
  _PrintForBuildbot(handle, _annotations.StepFailure)


def PrintBuildbotStepName(name, handle=None):
  """Marks a step name for buildbot to display."""
  _PrintForBuildbot(handle, _annotations.BuildStep, name)
