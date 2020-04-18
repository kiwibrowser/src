# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Contains classes used among bisect modules.

* Score: stores a list of scores and its statistics.
* CommitInfo: stores commit info.
* OptionsChecker: performs sanity check that every required option are given.
"""

from __future__ import print_function

import math

from chromite.lib import cros_logging as logging

class Score(object):
  """Stores a list of scores and calculates its statistics.

  Scores can be assigned by constructor or Update().

  Attr:
    values: A list of scores.
    mean: Mean of the scores.
    variance: Variance of the scores.
    std: Standard deviation of the scores.
  """

  def __init__(self, values=None):
    """Constructor.

    If values is not assigned, e.g. Score(), it returns a default Score object.
    And bool(Score()) == False.

    Args:
      values: A list of scores.
    """
    self.values = []
    self.mean = 0.0
    self.variance = 0.0
    self.std = 0.0
    if values is not None:
      self.Update(values)

  def __repr__(self):
    return 'Score(values=%r)' % self.values

  def __str__(self):
    return 'Score(values=%r, mean=%.3f, var=%.3f, std=%.3f)' % (
        self.values, self.mean, self.variance, self.std)

  def __len__(self):
    """Returns number of scores.

    Returns:
      Number of values.
    """
    return len(self.values)

  def __eq__(self, other):
    return sorted(self.values) == sorted(other.values)

  def __ne__(self, other):
    return not self.__eq__(other)

  def Clear(self):
    """Clears values and statistics."""
    self.values = []
    self.mean = 0.0
    self.variance = 0.0
    self.std = 0.0

  def Update(self, values):
    """Updates scores and their corresponding statistics.

    If values is empty or ill-formed, it calls Clear().

    Args:
      values: a list of scores.
    """
    if not values:
      self.Clear()
      return
    try:
      values = [float(x) for x in values]
    except ValueError as e:
      logging.error('Invalid literal of score: %s' % e)
      self.Clear()
      return
    except TypeError:
      logging.error('Invalid type of score: %s' % values)
      self.Clear()
      return

    self.values = values
    num_values = len(values)
    if num_values == 1:
      self.mean = values[0]
      self.variance = 0.0
      self.std = 0.0
    else:
      self.mean = sum(values) / num_values
      differences_from_mean = [x - self.mean for x in values]
      squared_differences = [x * x for x in differences_from_mean]
      self.variance = sum(squared_differences) / (num_values - 1)
      self.std = math.sqrt(self.variance)


class CommitInfo(object):
  """Stores commit info.

  Attr:
    sha1: Commit SHA1.
    title: Commit title.
    score: Evaluation score of the commit.
    label: 'good' or 'bad'.
    timestamp: Commit timestamp.
  """
  def __init__(self, sha1=None, title=None, score=None, label=None,
               timestamp=None):
    """Constructor.

    All arguments are optional. CommitInfo() creates default CommitInfo object
    and bool(CommitInfo()) == False.

    Args:
      sha1: Commit SHA1.
      title: Commit title.
      score: Evaluation score of the commit.
      label: 'good' or 'bad'.
      timestamp: Commit timestamp.
    """
    self.sha1 = '' if sha1 is None else sha1
    self.title = '' if title is None else title
    self.score = Score() if score is None else score
    self.label = '' if label is None else label
    self.timestamp = 0 if timestamp is None else timestamp

  def __repr__(self):
    return 'CommitInfo(sha1=%r, title=%r, score=%r, label=%r, timestamp=%r)' % (
        self.sha1, self.title, self.score, self.label, self.timestamp)

  def __eq__(self, other):
    return (self.sha1 == other.sha1 and self.title == other.title and
            self.score == other.score and self.label == other.label and
            self.timestamp == other.timestamp)

  def __ne__(self, other):
    return not self.__eq__(other)

  def __nonzero__(self):
    return bool(self.sha1 or self.timestamp or self.title or self.label or
                self.score)


class MissingRequiredOptionsException(Exception):
  """Exception raised for missing required options."""
  pass


class OptionsChecker(object):
  """Makes sure that __init__'s 'options' contains all required arguments.

  Its derived class should just update class attribute "REQUIRED_ARGS" and
  invokes __init__ to get options checked. For example:
    class ClassA(OptionsChecker):
      REQUIRED_ARGS = ['a']
      def __init__(self, options):
        super(ClassA, self).__init__(options)

    class ClassB(ClassA):
      REQUIRED_ARGS = ClassA.REQUIRED_ARGS + ['b']
      def __init__(self, options):
        super(ClassB, self).__init__(options)

    then calling
      ClassA(argparse.Namespace(a='a'))
      ClassB(argparse.Namespace(a='a', b='b'))
    is okay, but calling
      ClassA(argparse.Namespace(c='c'))
      ClassB(argparse.Namespace(b='b'))
    raises MissingRequiredOptionsException telling you that argument a is
    missing.
  """
  REQUIRED_ARGS = ()

  def __init__(self, options):
    """Constructor.

    Args:
      options: An argparse.Namespace to hold command line arguments.

    Raises:
      MissingRequiredOptionsException if any required argument is missing.
    """
    self.SanityCheckOptions(options)

  @classmethod
  def SanityCheckOptions(cls, options):
    """Performs sanity check on command line arguments.

    Args:
      options: An argparse.Namespace to hold command line arguments.

    Returns:
      True if sanity check passed.

    Raises:
      MissingRequiredOptionsException if any required argument is missing.
    """
    missing_args = [arg for arg in cls.REQUIRED_ARGS if arg not in options]
    if missing_args:
      raise MissingRequiredOptionsException(
          'Missing command line argument(s) %s required for class %s' %
          (missing_args, cls.__name__))
    return True
