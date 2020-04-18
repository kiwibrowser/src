# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging

from dashboard.pinpoint.models import attempt as attempt_module
from dashboard.pinpoint.models import change as change_module
from dashboard.pinpoint.models import kolmogorov_smirnov
from dashboard.pinpoint.models import mann_whitney_u


# The questionable significance levels are determined by first picking two
# representative samples of size 10. Take their p-value. Then repeat for each i,
# multiplying the sample size by i. To calculate these values:
# import math
# from dashboard.pinpoint.models import mann_whitney_u
# a = [0] * 10
# b = [0] * 9 + [1]
# print 1
# for i in xrange(1, 10):
#   pvalue = mann_whitney_u.MannWhitneyU(a * i, b * i)
#   print math.ceil(pvalue * 10000) / 10000
_QUESTIONABLE_SIGNIFICANCE_LEVELS = (
    1.0000, 0.3682, 0.1625, 0.0815, 0.0428, 0.0230,
    0.0126, 0.0070, 0.0039, 0.0022, 0.0013, 0.0007,
)
_SIGNIFICANCE_LEVEL = 0.001
_REPEAT_COUNT_INCREASE = 10


_DIFFERENT = 'different'
_PENDING = 'pending'
_SAME = 'same'
_UNKNOWN = 'unknown'


FUNCTIONAL = 'functional'
PERFORMANCE = 'performance'
COMPARISON_MODES = (FUNCTIONAL, PERFORMANCE)


class JobState(object):
  """The internal state of a Job.

  Wrapping the entire internal state of a Job in a PickleProperty allows us to
  use regular Python objects, with constructors, dicts, and object references.

  We lose the ability to index and query the fields, but it's all internal
  anyway. Everything queryable should be on the Job object."""

  def __init__(self, quests, comparison_mode=None, pin=None):
    """Create a JobState.

    Args:
      comparison_mode: Either 'functional' or 'performance', which the Job uses
          to figure out whether to perform a functional or performance bisect.
          If None, the Job will not automatically add any Attempts or Changes.
      quests: A sequence of quests to run on each Change.
      pin: A Change (Commits + Patch) to apply to every Change in this Job.
    """
    # _quests is mutable. Any modification should mutate the existing list
    # in-place rather than assign a new list, because every Attempt references
    # this object and will be updated automatically if it's mutated.
    self._quests = list(quests)

    self._comparison_mode = comparison_mode

    self._pin = pin

    # _changes can be in arbitrary order. Client should not assume that the
    # list of Changes is sorted in any particular order.
    self._changes = []

    # A mapping from a Change to a list of Attempts on that Change.
    self._attempts = {}

  @property
  def comparison_mode(self):
    return self._comparison_mode

  def AddAttempts(self, change):
    if not hasattr(self, '_pin'):
      # TODO: Remove after data migration.
      self._pin = None

    if self._pin:
      change_with_pin = change.Update(self._pin)
    else:
      change_with_pin = change

    for _ in xrange(_REPEAT_COUNT_INCREASE):
      attempt = attempt_module.Attempt(self._quests, change_with_pin)
      self._attempts[change].append(attempt)

  def AddChange(self, change, index=None):
    if index:
      self._changes.insert(index, change)
    else:
      self._changes.append(change)

    self._attempts[change] = []
    self.AddAttempts(change)

  def Explore(self):
    """Compare Changes and bisect by adding additional Changes as needed.

    For every pair of adjacent Changes, compare their results as probability
    distributions. If the results are different, find the midpoint of the
    Changes and add it to the Job. If the results are the same, do nothing.
    If the results are inconclusive, add more Attempts to the Change with fewer
    Attempts until we decide they are the same or different.

    The midpoint can only be added if the second Change represents a commit that
    comes after the first Change. Otherwise, this method won't explore further.
    For example, if Change A is repo@abc, and Change B is repo@abc + patch,
    there's no way to pick additional Changes to try.
    """
    # This loop adds Changes to the _changes list while looping through it.
    # The Change insertion simultaneously uses and modifies the list indices.
    # However, the loop index goes in reverse order and Changes are only added
    # after the loop index, so the loop never encounters the modified items.
    for index in xrange(len(self._changes) - 1, 0, -1):
      change_a = self._changes[index - 1]
      change_b = self._changes[index]
      comparison = self._Compare(change_a, change_b)

      if comparison == _DIFFERENT:
        try:
          midpoint = change_module.Change.Midpoint(change_a, change_b)
        except change_module.NonLinearError:
          continue

        logging.info('Adding Change %s.', midpoint)
        self.AddChange(midpoint, index)

      elif comparison == _UNKNOWN:
        if len(self._attempts[change_a]) <= len(self._attempts[change_b]):
          self.AddAttempts(change_a)
        else:
          self.AddAttempts(change_b)

  def ScheduleWork(self):
    work_left = False
    for attempts in self._attempts.itervalues():
      for attempt in attempts:
        if attempt.completed:
          continue

        attempt.ScheduleWork()
        work_left = True

    # TODO: Skip this for functional jobs.
    if not work_left and self._attempts and all(
        a.failed for attempts in self._attempts.itervalues() for a in attempts):
      raise Exception('All of the attempts failed. See the individual '
                      'attempts for details on each error.')

    return work_left

  def Differences(self):
    """Compares every pair of Changes and yields ones with different results.

    This method loops through every pair of adjacent Changes. If they have
    statistically different results, this method yields the latter one (which is
    assumed to have caused the difference).

    Yields:
      Tuples of (change_index, Change).
    """
    for index in xrange(1, len(self._changes)):
      change_a = self._changes[index - 1]
      change_b = self._changes[index]
      if self._Compare(change_a, change_b) == _DIFFERENT:
        yield index, change_b

  def AsDict(self):
    state = []
    quest_index = len(self._quests) - 1
    for change in self._changes:
      result_values = []

      if self._comparison_mode == 'functional':
        pass_fails = []
        for attempt in self._attempts[change]:
          if attempt.completed:
            pass_fails.append(int(attempt.failed))
        if pass_fails:
          result_values.append(_Mean(pass_fails))

      elif self._comparison_mode == 'performance':
        for attempt in self._attempts[change]:
          if quest_index < len(attempt.executions):
            result_values += attempt.executions[quest_index].result_values

      state.append({
          'attempts': [attempt.AsDict() for attempt in self._attempts[change]],
          'change': change.AsDict(),
          'comparisons': {},
          'result_values': result_values,
      })

    for index in xrange(1, len(self._changes)):
      change_a = self._changes[index - 1]
      change_b = self._changes[index]
      comparison = self._Compare(change_a, change_b)

      state[index - 1]['comparisons']['next'] = comparison
      state[index]['comparisons']['prev'] = comparison

    return {
        'comparison_mode': self._comparison_mode,
        'quests': map(str, self._quests),
        'state': state,
    }

  def _Compare(self, change_a, change_b):
    """Compare the results of two Changes in this Job.

    Aggregate the exceptions and result_values across every Quest for both
    Changes. Then, compare all the results for each Quest. If any of them are
    different, return _DIFFERENT. Otherwise, if any of them are inconclusive,
    return _UNKNOWN.  Otherwise, they are the _SAME.

    Arguments:
      change_a: The first Change whose results to compare.
      change_b: The second Change whose results to compare.

    Returns:
      _PENDING: If either Change has an incomplete Attempt.
      _DIFFERENT: If the two Changes (very likely) have different results.
      _SAME: If the two Changes (probably) have the same result.
      _UNKNOWN: If we'd like more data to make a decision.
    """
    attempts_a = self._attempts[change_a]
    attempts_b = self._attempts[change_b]

    if any(not attempt.completed for attempt in attempts_a + attempts_b):
      return _PENDING

    attempt_count = len(attempts_a) + len(attempts_b)

    executions_by_quest_a = _ExecutionsPerQuest(attempts_a)
    executions_by_quest_b = _ExecutionsPerQuest(attempts_b)

    any_unknowns = False
    for quest in self._quests:
      executions_a = executions_by_quest_a[quest]
      executions_b = executions_by_quest_b[quest]

      # Compare exceptions.
      values_a = tuple(bool(execution.exception) for execution in executions_a)
      values_b = tuple(bool(execution.exception) for execution in executions_b)
      if values_a and values_b:
        comparison = _CompareValues(values_a, values_b, attempt_count)
        if comparison == _DIFFERENT:
          return _DIFFERENT
        elif comparison == _UNKNOWN:
          any_unknowns = True

      # Compare result values.
      values_a = tuple(_Mean(execution.result_values)
                       for execution in executions_a if execution.result_values)
      values_b = tuple(_Mean(execution.result_values)
                       for execution in executions_b if execution.result_values)
      if values_a and values_b:
        comparison = _CompareValues(values_a, values_b, attempt_count)
        if comparison == _DIFFERENT:
          return _DIFFERENT
        elif comparison == _UNKNOWN:
          any_unknowns = True

    if any_unknowns:
      return _UNKNOWN

    return _SAME


def _ExecutionsPerQuest(attempts):
  executions = collections.defaultdict(list)
  for attempt in attempts:
    for quest, execution in zip(attempt.quests, attempt.executions):
      executions[quest].append(execution)
  return executions


def _CompareValues(values_a, values_b, attempt_count):
  """Decide whether two samples are the same, different, or unknown.

  Arguments:
    values_a: A list of sortable values. They don't need to be numeric.
    values_b: A list of sortable values. They don't need to be numeric.
    attempt_count: The total number of attempts made.

  Returns:
    _DIFFERENT: The samples likely come from different distributions.
        Reject the null hypothesis.
    _SAME: Not enough evidence to say that the samples come from different
        distributions. Fail to reject the null hypothesis.
    _UNKNOWN: Not enough evidence to say that the samples come from different
        distributions, but it looks a little suspicious, and we would like more
        data before making a final decision.
  """
  if not (values_a and values_b):
    # A sample has no values in it.
    return _UNKNOWN

  # MWU is bad at detecting changes in variance, and K-S is bad with discrete
  # distributions. So use both. We want low p-values for the below examples.
  #        a                     b               MWU(a, b)  KS(a, b)
  # [0]*20            [0]*15+[1]*5                0.0097     0.4973
  # range(10, 30)     range(10)+range(30, 40)     0.4946     0.0082
  p_value = min(
      kolmogorov_smirnov.KolmogorovSmirnov(values_a, values_b),
      mann_whitney_u.MannWhitneyU(values_a, values_b))

  if p_value < _SIGNIFICANCE_LEVEL:
    # The p-value is less than the significance level. Reject the null
    # hypothesis.
    return _DIFFERENT

  index = min(attempt_count / 20, len(_QUESTIONABLE_SIGNIFICANCE_LEVELS) - 1)
  questionable_significance_level = _QUESTIONABLE_SIGNIFICANCE_LEVELS[index]
  if p_value < questionable_significance_level:
    # The p-value is not less than the significance level, but it's small enough
    # to be suspicious. We'd like to investigate more closely.
    return _UNKNOWN

  # The p-value is quite large. We're not suspicious that the two samples might
  # come from different distributions, and we don't care to investigate more.
  return _SAME


def _Mean(values):
  return float(sum(values)) / len(values)
