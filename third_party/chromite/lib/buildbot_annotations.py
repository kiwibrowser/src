# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Functions and classes for formatting buildbot stage annotations."""

from __future__ import print_function

import abc
import itertools
import json


class Annotation(object):
  """Formatted annotation for buildbot."""

  def __init__(self, name, args):
    """Initialize instance.

    Args:
      name: Annotation name.
      args: A sequence of string arguments.
    """
    self.name = name
    self.args = args

  def __str__(self):
    inner_text = '@'.join(
        _EscapeArgText(text)
        for text in itertools.chain([self.name], self.args)
    )
    return '@@@%s@@@' % (inner_text,)

  @property
  def human_friendly(self):
    """Human-friendly format."""
    if self.args:
      return '%s: %s' % (self.name, '; '.join(self.args))
    else:
      return self.name


class _NamedAnnotation(Annotation):
  """Abstract subclass for creating named annotations.

  Concrete subclasses should define the ANNOTATION_NAME class attribute.
  """

  __metaclass__ = abc.ABCMeta

  def __init__(self, *args):
    super(_NamedAnnotation, self).__init__(self.ANNOTATION_NAME, args)

  @abc.abstractproperty
  def ANNOTATION_NAME(self):
    raise NotImplementedError()


class StepLink(_NamedAnnotation):
  """STEP_LINK annotation."""
  ANNOTATION_NAME = 'STEP_LINK'

  def __init__(self, text, url):
    super(StepLink, self).__init__(text, url)


class StepText(_NamedAnnotation):
  """STEP_TEXT annotation."""
  ANNOTATION_NAME = 'STEP_TEXT'

  def __init__(self, text):
    super(StepText, self).__init__(text)


class StepWarnings(_NamedAnnotation):
  """STEP_WARNINGS annotation."""
  ANNOTATION_NAME = 'STEP_WARNINGS'

  def __init__(self):
    super(StepWarnings, self).__init__()


class StepFailure(_NamedAnnotation):
  """STEP_FAILURE annotation."""
  ANNOTATION_NAME = 'STEP_FAILURE'

  def __init__(self):
    super(StepFailure, self).__init__()


class BuildStep(_NamedAnnotation):
  """BUILD_STEP annotation."""
  ANNOTATION_NAME = 'BUILD_STEP'

  def __init__(self, name):
    super(BuildStep, self).__init__(name)


class SetBuildProperty(_NamedAnnotation):
  """SET_BUILD_PROPERTY annotation."""
  ANNOTATION_NAME = 'SET_BUILD_PROPERTY'

  def __init__(self, name, value):
    super(SetBuildProperty, self).__init__(name, json.dumps(value))


def _EscapeArgText(text):
  """Escape annotation argument text.

  Args:
    text: String to escape.
  """
  return text.replace('@', '-AT-')
