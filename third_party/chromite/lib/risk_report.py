# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Report any risky CLs using CL-Scanner."""

from __future__ import print_function

import requests

from chromite.lib import timeout_util
from chromite.lib import cros_logging as logging


TIMEOUT = 3 * 60
CL_SCANNER_API = (
    'https://chromiumos-cl-scanner.appspot.com/external/risk/build/%d')

# Include all CLs with at least 10% risk.
_MINIMUM_RISK_THRESHOLD = 0.10
# Include CLs at least 80% as risky as the top risky CL
_CLOSE_TO_TOP_RATIO = 0.80
_EXTERNAL_CL_LINK = 'http://crrev.com/c/%s'


def GetCLRiskReport(build_id):
  """Returns a string reporting the riskiest CLs.

  Args:
    build_id: The master build id in cidb.

  Returns:
    A dictionary mapping "CL=risk%" strings to the CLs' URLs.
  """
  try:
    risks = _GetCLRisks(build_id)
  except requests.exceptions.HTTPError:
    logging.exception('Encountered an error reaching CL-Scanner.')
    return {'(encountered exception reaching CL-Scanner)': ''}
  except timeout_util.TimeoutError:
    logging.exception('Timed out reaching CL-Scanner.')
    return {'(timeout reaching CL-Scanner)': ''}

  logging.info('CL-Scanner risks: %s', _PrettyPrintCLRisks(risks))
  if not risks:
    return {}

  # TODO(phobbs) this will need to be changed to handle internal CLs.
  top_risky = _TopRisky(risks)
  return {_PrettyPrintCLRisk(cl, risk): _EXTERNAL_CL_LINK % cl
          for cl, risk in top_risky.iteritems()}


@timeout_util.TimeoutDecorator(TIMEOUT)
def _GetCLRisks(build_id):
  """Gets the CL risks given a cidb build_id.

  Args:
    build_id: The master build id in cidb.

  Returns:
    A mapping of CL numbers to risks
  """
  response = requests.get(CL_SCANNER_API % build_id)
  response.raise_for_status()
  return response.json().get('cls')


def _TopRisky(risks):
  """Returns the top risky commits.

  Ars:
    risks: A dictionary mapping CL numbers to bad CL probability.
  """
  top_key = max(risks, key=risks.get)
  above_threshold = {
      cl: risk for cl, risk in risks.iteritems()
      if risk > _MINIMUM_RISK_THRESHOLD}
  close_to_top = {
      cl: risk for cl, risk in risks.iteritems()
      if risk >= risks[top_key] * _CLOSE_TO_TOP_RATIO
  }

  above_threshold.update(close_to_top)
  return above_threshold


def _PrettyPrintCLRisks(risks):
  """Pretty print a risk map as a string.

  Args:
    risks: The risks to print.
  """
  return ', '.join([
      '%s = %s' % (k, v)
      for k, v in risks.iteritems()])


def _PrettyPrintCLRisk(cl, risk):
  """Pretty print a cl, risk pair as a string.

  Args:
    cl: The Gerrit CL number.
    risk: The risk as a percent.
  """
  return 'Bad CL risk for %s = %s' % (cl, _Percent(risk))


def _Percent(risk):
  """Converts a float to a percent.

  Args:
    risk: A probability.

  Returns:
    A string percent.
  """
  return '{:0.1f}%'.format(risk * 100)
