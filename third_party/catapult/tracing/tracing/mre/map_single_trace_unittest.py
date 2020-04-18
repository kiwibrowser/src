# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import json
import unittest

from tracing.mre import function_handle
from tracing.mre import map_single_trace
from tracing.mre import file_handle
from tracing.mre import failure
from tracing.mre import job as job_module


def _Handle(filename):
  module = function_handle.ModuleToLoad(filename=filename)
  map_handle = function_handle.FunctionHandle(
      modules_to_load=[module], function_name='MyMapFunction')
  return job_module.Job(map_handle, None)


class MapSingleTraceTests(unittest.TestCase):

  def testPassingMapScript(self):
    events = [
        {'pid': 1, 'tid': 2, 'ph': 'X', 'name': 'a', 'cat': 'c',
         'ts': 0, 'dur': 10, 'args': {}},
        {'pid': 1, 'tid': 2, 'ph': 'X', 'name': 'b', 'cat': 'c',
         'ts': 3, 'dur': 5, 'args': {}}
    ]
    trace_handle = file_handle.InMemoryFileHandle(
        '/a.json', json.dumps(events))

    with map_single_trace.TemporaryMapScript("""
      tr.mre.FunctionRegistry.register(
          function MyMapFunction(result, model) {
            var canonicalUrl = model.canonicalUrl;
            result.addPair('result', {
                numProcesses: model.getAllProcesses().length
              });
          });
    """) as map_script:
      result = map_single_trace.MapSingleTrace(trace_handle,
                                               _Handle(map_script.filename))

    self.assertFalse(result.failures)
    r = result.pairs['result']
    self.assertEquals(r['numProcesses'], 1)


  def testProcessingGiantTrace(self):
    # Populate a string trace of 2 million events.
    trace_events = ['[']
    for i in xrange(2000000):
      trace_events.append(
          '{"pid": 1, "tid": %i, "ph": "X", "name": "a", "cat": "c",'
          '"ts": %i, "dur": 1, "args": {}},' %  (i % 5, 2 * i))
    trace_events.append('{}]')
    trace_data = ''.join(trace_events)
    trace_handle = file_handle.InMemoryFileHandle(
        '/a.json', trace_data)

    with map_single_trace.TemporaryMapScript("""
      tr.mre.FunctionRegistry.register(
          function MyMapFunction(result, model) {
            var canonicalUrl = model.canonicalUrl;
            var numEvents = 0;
            for (var e of model.getProcess(1).getDescendantEvents()) {
              numEvents++;
            }
            result.addPair('result', {
                numEvents: numEvents
              });
          });
    """) as map_script:
      result = map_single_trace.MapSingleTrace(trace_handle,
                                               _Handle(map_script.filename))

    self.assertFalse(result.failures,
                     msg='\n'.join(str(f) for f in result.failures))
    r = result.pairs['result']
    self.assertEquals(r['numEvents'], 2000000)



  def testTraceDidntImport(self):
    trace_string = 'This is intentionally not a trace-formatted string.'
    trace_handle = file_handle.InMemoryFileHandle(
        '/a.json', trace_string)

    with map_single_trace.TemporaryMapScript("""
      tr.mre.FunctionRegistry.register(
          function MyMapFunction(results, model) {
          });
    """) as map_script:
      result = map_single_trace.MapSingleTrace(trace_handle,
                                               _Handle(map_script.filename))

    self.assertEquals(len(result.failures), 1)
    self.assertEquals(len(result.pairs), 0)
    f = result.failures[0]
    self.assertIsInstance(f, map_single_trace.TraceImportFailure)

  def testMapFunctionThatThrows(self):
    events = [
        {'pid': 1, 'tid': 2, 'ph': 'X', 'name': 'a', 'cat': 'c',
         'ts': 0, 'dur': 10, 'args': {}},
        {'pid': 1, 'tid': 2, 'ph': 'X', 'name': 'b', 'cat': 'c',
         'ts': 3, 'dur': 5, 'args': {}}
    ]
    trace_handle = file_handle.InMemoryFileHandle(
        '/a.json', json.dumps(events))

    with map_single_trace.TemporaryMapScript("""
      tr.mre.FunctionRegistry.register(
          function MyMapFunction(results, model) {
            throw new Error('Expected error');
          });
    """) as map_script:
      result = map_single_trace.MapSingleTrace(trace_handle,
                                               _Handle(map_script.filename))

    self.assertEquals(len(result.failures), 1)
    self.assertEquals(len(result.pairs), 0)
    f = result.failures[0]
    self.assertIsInstance(f, map_single_trace.MapFunctionFailure)

  def testMapperWithLoadError(self):
    events = [
        {'pid': 1, 'tid': 2, 'ph': 'X', 'name': 'a', 'cat': 'c',
         'ts': 0, 'dur': 10, 'args': {}},
        {'pid': 1, 'tid': 2, 'ph': 'X', 'name': 'b', 'cat': 'c',
         'ts': 3, 'dur': 5, 'args': {}}
    ]
    trace_handle = file_handle.InMemoryFileHandle(
        '/a.json', json.dumps(events))

    with map_single_trace.TemporaryMapScript("""
      throw new Error('Expected load error');
    """) as map_script:
      result = map_single_trace.MapSingleTrace(trace_handle,
                                               _Handle(map_script.filename))

    self.assertEquals(len(result.failures), 1)
    self.assertEquals(len(result.pairs), 0)
    f = result.failures[0]
    self.assertIsInstance(f, map_single_trace.FunctionLoadingFailure)

  def testMapperWithUnexpectedError(self):
    events = [
        {'pid': 1, 'tid': 2, 'ph': 'X', 'name': 'a', 'cat': 'c',
         'ts': 0, 'dur': 10, 'args': {}},
    ]
    trace_handle = file_handle.InMemoryFileHandle(
        '/a.json', json.dumps(events))

    with map_single_trace.TemporaryMapScript("""
          quit(100);
    """) as map_script:
      result = map_single_trace.MapSingleTrace(trace_handle,
                                               _Handle(map_script.filename))

    self.assertEquals(len(result.failures), 1)
    self.assertEquals(len(result.pairs), 0)
    f = result.failures[0]
    self.assertIsInstance(f, failure.Failure)

  def testNoMapper(self):
    events = [
        {'pid': 1, 'tid': 2, 'ph': 'X', 'name': 'a', 'cat': 'c',
         'ts': 0, 'dur': 10, 'args': {}},
        {'pid': 1, 'tid': 2, 'ph': 'X', 'name': 'b', 'cat': 'c',
         'ts': 3, 'dur': 5, 'args': {}}
    ]
    trace_handle = file_handle.InMemoryFileHandle(
        '/a.json', json.dumps(events))

    with map_single_trace.TemporaryMapScript("""
    """) as map_script:
      result = map_single_trace.MapSingleTrace(trace_handle,
                                               _Handle(map_script.filename))

    self.assertEquals(len(result.failures), 1)
    self.assertEquals(len(result.pairs), 0)
    f = result.failures[0]
    self.assertIsInstance(f, map_single_trace.FunctionNotDefinedFailure)

  def testMapperDoesntAddValues(self):
    events = [
        {'pid': 1, 'tid': 2, 'ph': 'X', 'name': 'a', 'cat': 'c',
         'ts': 0, 'dur': 10, 'args': {}},
        {'pid': 1, 'tid': 2, 'ph': 'X', 'name': 'b', 'cat': 'c',
         'ts': 3, 'dur': 5, 'args': {}}
    ]
    trace_handle = file_handle.InMemoryFileHandle(
        '/a.json', json.dumps(events))

    with map_single_trace.TemporaryMapScript("""
      tr.mre.FunctionRegistry.register(
          function MyMapFunction(results, model) {
      });
    """) as map_script:
      result = map_single_trace.MapSingleTrace(trace_handle,
                                               _Handle(map_script.filename))

    self.assertEquals(len(result.failures), 1)
    self.assertEquals(len(result.pairs), 0)
    f = result.failures[0]
    self.assertIsInstance(f, map_single_trace.NoResultsAddedFailure)
