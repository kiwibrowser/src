# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import json
import os
import re
import sys
import tempfile
import types
import platform

import tracing_project
import vinn

from tracing.mre import failure
from tracing.mre import mre_result

_MAP_SINGLE_TRACE_CMDLINE_PATH = os.path.join(
    tracing_project.TracingProject.tracing_src_path, 'mre',
    'map_single_trace_cmdline.html')

class TemporaryMapScript(object):
  def __init__(self, js):
    temp_file = tempfile.NamedTemporaryFile(delete=False)
    temp_file.write("""
<!DOCTYPE html>
<script>
%s
</script>
""" % js)
    temp_file.close()
    self._filename = temp_file.name

  def __enter__(self):
    return self

  def __exit__(self, *args, **kwargs):
    os.remove(self._filename)
    self._filename = None

  @property
  def filename(self):
    return self._filename


class FunctionLoadingFailure(failure.Failure):
  pass

class FunctionNotDefinedFailure(failure.Failure):
  pass

class MapFunctionFailure(failure.Failure):
  pass

class FileLoadingFailure(failure.Failure):
  pass

class TraceImportFailure(failure.Failure):
  pass

class NoResultsAddedFailure(failure.Failure):
  pass

class InternalMapError(Exception):
  pass

_FAILURE_NAME_TO_FAILURE_CONSTRUCTOR = {
    'FileLoadingError': FileLoadingFailure,
    'FunctionLoadingError': FunctionLoadingFailure,
    'FunctionNotDefinedError': FunctionNotDefinedFailure,
    'TraceImportError': TraceImportFailure,
    'MapFunctionError': MapFunctionFailure,
    'NoResultsAddedError': NoResultsAddedFailure
}


def MapSingleTrace(trace_handle,
                   job,
                   extra_import_options=None):
  assert (type(extra_import_options) is types.NoneType or
          type(extra_import_options) is types.DictType), (
              'extra_import_options should be a dict or None.')
  project = tracing_project.TracingProject()

  all_source_paths = list(project.source_paths)
  all_source_paths.append(project.trace_processor_root_path)

  result = mre_result.MreResult()

  with trace_handle.PrepareFileForProcessing() as prepared_trace_handle:
    js_args = [
        json.dumps(prepared_trace_handle.AsDict()),
        json.dumps(job.AsDict()),
    ]
    if extra_import_options:
      js_args.append(json.dumps(extra_import_options))

    # Use 8gb heap space to make sure we don't OOM'ed on big trace, but not
    # on ARM devices since we use 32-bit d8 binary.
    if platform.machine() == 'armv7l' or platform.machine() == 'aarch64':
      v8_args = None
    else:
      v8_args = ['--max-old-space-size=8192']

    res = vinn.RunFile(
        _MAP_SINGLE_TRACE_CMDLINE_PATH,
        source_paths=all_source_paths,
        js_args=js_args,
        v8_args=v8_args)

  if res.returncode != 0:
    sys.stderr.write(res.stdout)
    result.AddFailure(failure.Failure(
        job,
        trace_handle.canonical_url,
        'Error', 'vinn runtime error while mapping trace.',
        'vinn runtime error while mapping trace.', 'Unknown stack'))
    return result

  for line in res.stdout.split('\n'):
    m = re.match('^MRE_RESULT: (.+)', line, re.DOTALL)
    if m:
      found_dict = json.loads(m.group(1))
      failures = [
          failure.Failure.FromDict(f, job, _FAILURE_NAME_TO_FAILURE_CONSTRUCTOR)
          for f in found_dict['failures']]

      for f in failures:
        result.AddFailure(f)

      for k, v in found_dict['pairs'].iteritems():
        result.AddPair(k, v)

    else:
      if len(line) > 0:
        sys.stderr.write(line)
        sys.stderr.write('\n')

  if not (len(result.pairs) or len(result.failures)):
    raise InternalMapError('Internal error: No results were produced!')

  return result
