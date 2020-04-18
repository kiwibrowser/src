# Copyright 2017 The PDFium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes that draw conclusions out of a comparison and represent them."""

from collections import Counter


FORMAT_RED = '\033[01;31m{0}\033[00m'
FORMAT_GREEN = '\033[01;32m{0}\033[00m'
FORMAT_MAGENTA = '\033[01;35m{0}\033[00m'
FORMAT_CYAN = '\033[01;36m{0}\033[00m'
FORMAT_NORMAL = '{0}'

RATING_FAILURE = 'failure'
RATING_REGRESSION = 'regression'
RATING_IMPROVEMENT = 'improvement'
RATING_NO_CHANGE = 'no_change'
RATING_SMALL_CHANGE = 'small_change'

RATINGS = [
    RATING_FAILURE,
    RATING_REGRESSION,
    RATING_IMPROVEMENT,
    RATING_NO_CHANGE,
    RATING_SMALL_CHANGE
]

RATING_TO_COLOR = {
    RATING_FAILURE: FORMAT_MAGENTA,
    RATING_REGRESSION: FORMAT_RED,
    RATING_IMPROVEMENT: FORMAT_CYAN,
    RATING_NO_CHANGE: FORMAT_GREEN,
    RATING_SMALL_CHANGE: FORMAT_NORMAL,
}


class ComparisonConclusions(object):
  """All conclusions drawn from a comparison.

  This is initialized empty and then processes pairs of results for each test
  case, determining the rating for that case, which can be:
  "failure" if either or both runs for the case failed.
  "regression" if there is a significant increase in time for the test case.
  "improvement" if there is a significant decrease in time for the test case.
  "no_change" if the time for the test case did not change at all.
  "small_change" if the time for the test case changed but within the threshold.
  """

  def __init__(self, threshold_significant):
    """Initializes an empty ComparisonConclusions.

    Args:
      threshold_significant: Float with the tolerance beyond which changes in
          measurements are considered significant.

          The change is considered as a multiplication rather than an addition
          of a fraction of the previous measurement, that is, a
          threshold_significant of 1.0 will flag test cases that became over
          100% slower (> 200% of the previous time measured) or over 100% faster
          (< 50% of the previous time measured).

          threshold_significant 0.02 -> 98.04% to 102% is not significant
          threshold_significant 0.1 -> 90.9% to 110% is not significant
          threshold_significant 0.25 -> 80% to 125% is not significant
          threshold_significant 1 -> 50% to 200% is not significant
          threshold_significant 4 -> 20% to 500% is not significant

    """
    self.threshold_significant = threshold_significant
    self.threshold_significant_negative = (1 / (1 + threshold_significant)) - 1

    self.params = {'threshold': threshold_significant}
    self.summary = ComparisonSummary()
    self.case_results = {}

  def ProcessCase(self, case_name, before, after):
    """Feeds a test case results to the ComparisonConclusions.

    Args:
      case_name: String identifying the case.
      before: Measurement for the "before" version of the code.
      after: Measurement for the "after" version of the code.
    """

    # Switch 0 to None to simplify the json dict output. All zeros are
    # considered failed runs, so they will be represented by "null".
    if not before:
      before = None
    if not after:
      after = None

    if not before or not after:
      ratio = None
      rating = RATING_FAILURE
    else:
      ratio = (float(after) / before) - 1.0
      if ratio > self.threshold_significant:
        rating = RATING_REGRESSION
      elif ratio < self.threshold_significant_negative:
        rating = RATING_IMPROVEMENT
      elif ratio == 0:
        rating = RATING_NO_CHANGE
      else:
        rating = RATING_SMALL_CHANGE

    case_result = CaseResult(case_name, before, after, ratio, rating)

    self.summary.ProcessCaseResult(case_result)
    self.case_results[case_name] = case_result

  def GetSummary(self):
    """Gets the ComparisonSummary with consolidated totals."""
    return self.summary

  def GetCaseResults(self):
    """Gets a dict mapping each test case identifier to its CaseResult."""
    return self.case_results

  def GetOutputDict(self):
    """Returns a conclusions dict with all the conclusions drawn.

    Returns:
      A serializable dict with the format illustrated below:
      {
        "version": 1,
        "params": {
          "threshold": 0.02
        },
        "summary": {
          "total": 123,
          "failure": 1,
          "regression": 2,
          "improvement": 1,
          "no_change": 100,
          "small_change": 19
        },
        "comparison_by_case": {
          "testing/resources/new_test.pdf": {
            "before": None,
            "after": 1000,
            "ratio": None,
            "rating": "failure"
          },
          "testing/resources/test1.pdf": {
            "before": 100,
            "after": 120,
            "ratio": 0.2,
            "rating": "regression"
          },
          "testing/resources/test2.pdf": {
            "before": 100,
            "after": 2000,
            "ratio": 19.0,
            "rating": "regression"
          },
          "testing/resources/test3.pdf": {
            "before": 1000,
            "after": 1005,
            "ratio": 0.005,
            "rating": "small_change"
          },
          "testing/resources/test4.pdf": {
            "before": 1000,
            "after": 1000,
            "ratio": 0.0,
            "rating": "no_change"
          },
          "testing/resources/test5.pdf": {
            "before": 1000,
            "after": 600,
            "ratio": -0.4,
            "rating": "improvement"
          }
        }
      }
    """
    output_dict = {}
    output_dict['version'] = 1
    output_dict['params'] = {'threshold': self.threshold_significant}
    output_dict['summary'] = self.summary.GetOutputDict()
    output_dict['comparison_by_case'] = {
        cr.case_name.decode('utf-8'): cr.GetOutputDict()
        for cr in self.GetCaseResults().values()
    }
    return output_dict


class ComparisonSummary(object):
  """Totals computed for a comparison."""

  def __init__(self):
    self.rating_counter = Counter()

  def ProcessCaseResult(self, case_result):
    self.rating_counter[case_result.rating] += 1

  def GetTotal(self):
    """Gets the number of test cases processed."""
    return sum(self.rating_counter.values())

  def GetCount(self, rating):
    """Gets the number of test cases processed with a given rating."""
    return self.rating_counter[rating]

  def GetOutputDict(self):
    """Returns a dict that can be serialized with all the totals."""
    result = {'total': self.GetTotal()}
    for rating in RATINGS:
      result[rating] = self.GetCount(rating)
    return result


class CaseResult(object):
  """The conclusion for the comparison of a single test case."""

  def __init__(self, case_name, before, after, ratio, rating):
    """Initializes an empty ComparisonConclusions.

    Args:
      case_name: String identifying the case.
      before: Measurement for the "before" version of the code.
      after: Measurement for the "after" version of the code.
      ratio: Difference between |after| and |before| as a fraction of |before|.
      rating: Rating for this test case.
    """
    self.case_name = case_name
    self.before = before
    self.after = after
    self.ratio = ratio
    self.rating = rating

  def GetOutputDict(self):
    """Returns a dict with the test case's conclusions."""
    return {'before': self.before,
            'after': self.after,
            'ratio': self.ratio,
            'rating': self.rating}


def PrintConclusionsDictHumanReadable(conclusions_dict, colored, key=None):
  """Prints a conclusions dict in a human-readable way.

  Args:
    conclusions_dict: Dict to print.
    colored: Whether to color the output to highlight significant changes.
    key: String with the CaseResult dictionary key to sort the cases.
  """
  # Print header
  print '=' * 80
  print '{0:>11s} {1:>15s}  {2}' .format(
      '% Change',
      'Time after',
      'Test case')
  print '-' * 80

  color = FORMAT_NORMAL

  # Print cases
  if key is not None:
    case_pairs = sorted(conclusions_dict['comparison_by_case'].iteritems(),
                        key=lambda kv: kv[1][key])
  else:
    case_pairs = sorted(conclusions_dict['comparison_by_case'].iteritems())

  for case_name, case_dict in case_pairs:
    if colored:
      color = RATING_TO_COLOR[case_dict['rating']]

    if case_dict['rating'] == RATING_FAILURE:
      print u'{} to measure time for {}'.format(
          color.format('Failed'),
          case_name).encode('utf-8')
      continue

    print u'{0} {1:15,d}  {2}' .format(
        color.format('{:+11.4%}'.format(case_dict['ratio'])),
        case_dict['after'],
        case_name).encode('utf-8')

  # Print totals
  totals = conclusions_dict['summary']
  print '=' * 80
  print 'Test cases run: %d' % totals['total']

  if colored:
    color = FORMAT_MAGENTA if totals[RATING_FAILURE] else FORMAT_GREEN
  print ('Failed to measure: %s'
         % color.format(totals[RATING_FAILURE]))

  if colored:
    color = FORMAT_RED if totals[RATING_REGRESSION] else FORMAT_GREEN
  print ('Regressions: %s'
         % color.format(totals[RATING_REGRESSION]))

  if colored:
    color = FORMAT_CYAN if totals[RATING_IMPROVEMENT] else FORMAT_GREEN
  print ('Improvements: %s'
         % color.format(totals[RATING_IMPROVEMENT]))
