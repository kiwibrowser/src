# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Parses logcat output for VrCore performance logs and computes stats.

Can be used either as a standalone manual script or as part of the automated
VR performance tests.

For manual use, simply pipe logcat output into the script.
"""

import logging
import math
import sys

LINE_SESSION_START = 'PerfMon: Start of session'
LINE_SESSION_END = 'PerfMon: End of session'

LINE_ASYNC_FPS = 'Async reprojection thread FPS: '
LINE_APP_FPS = 'Application FPS: '
LINE_BLOCKED_FRAME = 'Application frame submits blocked on GPU in FPS window: '
LINE_MISSED_VSYNC = 'Async reprojection thread missed vsync (late by '

APP_FPS_KEY ='app_fps_data'
ASYNC_FPS_KEY = 'async_fps_data'
ASYNC_MISSED_KEY = 'asymc_missed_data'
BLOCKED_SUBMISSION_KEY = 'blocked_submission_key'

def CalculateAverage(inputs):
  return sum(inputs) / len(inputs)

# Implement own standard deviation function instead of using numpy so that
# script has no dependencies on third-party libraries.
def CalculateStandardDeviation(inputs):
  average = sum(inputs) / len(inputs)
  sum_of_squares = 0
  for element in inputs:
    sum_of_squares += math.pow(element - average, 2)
  sum_of_squares /= len(inputs)
  return math.sqrt(sum_of_squares)

def GetInputFromStdin():
  """Turns stdin input into a single string.

  Returns:
    A single string containing all lines input via Stdin
  """
  print 'Waiting for stdin input.'
  lines = []
  while True:
    try:
      line = sys.stdin.readline()
    except KeyboardInterrupt:
      break
    if not line:
      break
    lines.append(line)

  return '\n'.join(lines)

def ParseLinesIntoSessions(lines):
  """Parses a string for VR performance logs.

  lines: A string containing lines of logcat output.

  Returns:
    A list of strings, where each element contains all the lines of interest
    for a single session
  """
  logging_sessions = []
  lines_of_interest = [LINE_ASYNC_FPS, LINE_APP_FPS, LINE_BLOCKED_FRAME,
                       LINE_MISSED_VSYNC]
  for line in lines.split('\n'):
    if LINE_SESSION_START in line:
      logging_sessions.append("")
    elif LINE_SESSION_END in line:
      continue
    for loi in lines_of_interest:
      if loi in line:
        logging_sessions[-1] += (line + "\n")
        break
  return logging_sessions

def ComputeSessionStatistics(session):
  """Extracts raw statistical data from a session string.

  session: A string containing all the performance logging lines from
    a single VR session

  Returns:
    A dictionary containing the raw statistics
  """
  app_fps_data = []
  async_fps_data = []
  async_missed_data = []
  blocked_submission_data = []

  for line in session.split("\n"):
    if LINE_ASYNC_FPS in line: # Async thread FPS.
      async_fps_data.append( float(line.split(LINE_ASYNC_FPS)[1]) )
    elif LINE_APP_FPS in line: # Application FPS.
      app_fps_data.append( float(line.split(LINE_APP_FPS)[1]) )
    elif LINE_BLOCKED_FRAME in line: # Application frame blocked.
      blocked_submission_data.append(int(line.split(LINE_BLOCKED_FRAME)[1]))
    elif LINE_MISSED_VSYNC in line: # Async thread missed vsync.
      # Convert to milliseconds as well.
      async_missed_data.append( float(
          line.split(LINE_MISSED_VSYNC)[1].split("us")[0]) / 1000)

  return {
      APP_FPS_KEY: app_fps_data,
      ASYNC_FPS_KEY: async_fps_data,
      ASYNC_MISSED_KEY: async_missed_data,
      BLOCKED_SUBMISSION_KEY: blocked_submission_data,
  }

def StringifySessionStatistics(statistics):
  """Turns a raw statistics dictionary into a formatted string.

  statistics: A raw statistics dictionary

  Returns:
    A string with min/max/average calculations
  """
  app_fps_data = statistics[APP_FPS_KEY]
  async_fps_data = statistics[ASYNC_FPS_KEY]
  async_missed_data = statistics[ASYNC_MISSED_KEY]
  blocked_submission_data = statistics[BLOCKED_SUBMISSION_KEY]

  output = """
  Min application FPS: %(min_app_fps).4f
  Max application FPS: %(max_app_fps).4f
  Average application FPS: %(avg_app_fps).4f +/- %(std_app_fps).4f
  %(blocked)d total application frame submissions blocked on GPU
  ---
  Min application FPS after first second: %(min_app_fps_after).4f
  Max application FPS after first second: %(max_app_fps_after).4f
  Average application FPS after first second: %(avg_app_fps_after).4f +/- %(std_app_fps_after).4f
  ---
  Min async reprojection thread FPS: %(min_async_fps).4f
  Max async reprojection thread FPS: %(max_async_fps).4f
  Average async reprojection thread FPS: %(avg_async_fps).4f +/- %(std_async_fps).4f
  Async reprojection thread missed %(async_missed)d vsyncs
  """ % ({
      'min_app_fps': min(app_fps_data),
      'max_app_fps': max(app_fps_data),
      'avg_app_fps': CalculateAverage(app_fps_data),
      'std_app_fps': CalculateStandardDeviation(app_fps_data),
      'blocked': sum(blocked_submission_data),
      'min_app_fps_after': min(app_fps_data[1:]),
      'max_app_fps_after': max(app_fps_data[1:]),
      'avg_app_fps_after': CalculateAverage(app_fps_data[1:]),
      'std_app_fps_after': CalculateStandardDeviation(app_fps_data[1:]),
      'min_async_fps': min(async_fps_data),
      'max_async_fps': max(async_fps_data),
      'avg_async_fps': CalculateAverage(async_fps_data),
      'std_async_fps': CalculateStandardDeviation(async_fps_data),
      'async_missed': len(async_missed_data)})


  if len(async_missed_data) > 0:
    output += 'Average vsync miss time: %.4f +/- %.4f ms' % (
        CalculateAverage(async_missed_data),
        CalculateStandardDeviation(async_missed_data))
  return output


def ComputeAndPrintStatistics(session):
  if len(session) == 0:
    logging.warning('No data collected for session')
    return
  logging.warning(StringifySessionStatistics(ComputeSessionStatistics(session)))


def main():
  logging_sessions = ParseLinesIntoSessions(GetInputFromStdin())
  logging.warning('Found %d sessions', len(logging_sessions))
  counter = 1
  for session in logging_sessions:
    logging.warning('\n#### Session %d ####', counter)
    ComputeAndPrintStatistics(session)
    counter += 1

if __name__ == '__main__':
  main()
