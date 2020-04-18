# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module implements a simple WSGI server for the memory_inspector Web UI.

The WSGI server essentially handles two kinds of requests:
 - /ajax/foo/bar: The AJAX endpoints which exchange JSON data with the JS.
    Requests routing is achieved using a simple @uri decorator which simply
    performs regex matching on the request path.
 - /static/content: Anything not matching the /ajax/ prefix is treated as a
    static content request (for serving the index.html and JS/CSS resources).

The following HTTP status code are returned by the server:
 - 200 - OK: The request was handled correctly.
 - 404 - Not found: None of the defined handlers did match the /request/path.
 - 410 - Gone: The path was matched but the handler returned an empty response.
    This typically happens when the target device is disconnected.
"""

import cgi
import collections
import datetime
import glob
import json
import memory_inspector
import mimetypes
import os
import posixpath
import re
import traceback
import urlparse
import uuid
import wsgiref.simple_server

from memory_inspector import constants
from memory_inspector.core import backends
from memory_inspector.core import memory_map
from memory_inspector.classification import mmap_classifier
from memory_inspector.classification import native_heap_classifier
from memory_inspector.data import serialization
from memory_inspector.data import file_storage
from memory_inspector.frontends import background_tasks


_HTTP_OK = '200 OK'
_HTTP_GONE = '410 Gone'
_HTTP_NOT_FOUND = '404 Not Found'
_HTTP_INTERNAL_ERROR = '500 Internal Server Error'
_PERSISTENT_STORAGE_PATH = os.path.join(
    os.path.expanduser('~'), '.config', 'memory_inspector')
_CONTENT_DIR = os.path.abspath(os.path.join(
    os.path.dirname(__file__), 'www_content'))
_APP_PROCESS_RE = r'^[\w.:]+$'  # Regex for matching app processes.
_STATS_HIST_SIZE = 120  # Keep at most 120 samples of stats per process.
_CACHE_LEN = 10  # Max length of |_cached_objs|.

# |_cached_objs| keeps the state of short-lived objects that the client needs to
# _cached_objs subsequent AJAX calls.
_cached_objs = collections.OrderedDict()
_persistent_storage = file_storage.Storage(_PERSISTENT_STORAGE_PATH)
_proc_stats_history = {}  # /Android/device/PID -> deque([stats@T=0, stats@T=1])


class UriHandler(object):
  """Base decorator used to automatically route /requests/by/path.

  Each handler is called with the following args:
    args: a tuple of the matching regex groups.
    req_vars: a dictionary of request args (querystring for GET, body for POST).
  Each handler must return a tuple with the following elements:
    http_code: a string with the HTTP status code (e.g., '200 - OK')
    headers: a list of HTTP headers (e.g., [('Content-Type': 'foo/bar')])
    body: the HTTP response body.
  """
  _handlers = []

  def __init__(self, path_regex, verb='GET', output_filter=None):
    self._path_regex = path_regex
    self._verb = verb
    default_output_filter = lambda *x: x  # Just return the same args unchanged.
    self._output_filter = output_filter or default_output_filter

  def __call__(self, handler):
    UriHandler._handlers += [(
        self._verb, self._path_regex, self._output_filter, handler)]

  @staticmethod
  def Handle(method, path, req_vars):
    """Finds a matching handler and calls it (or returns a 404 - Not Found)."""
    cache_headers = [('Cache-Control', 'no-cache'),
                     ('Expires', 'Fri, 19 Sep 1986 05:00:00 GMT')]
    for (match_method, path_regex, output_filter, fn) in UriHandler._handlers:
      if method != match_method:
        continue
      m = re.match(path_regex, path)
      if not m:
        continue
      try:
        (http_code, headers, body) = fn(m.groups(), req_vars)
      except Exception as e:
        traceback.print_exc()
        return _HTTP_INTERNAL_ERROR, [], str(e)
      return output_filter(http_code, cache_headers + headers, body)
    return (_HTTP_NOT_FOUND, [], 'No AJAX handlers found')


class AjaxHandler(UriHandler):
  """Decorator for routing AJAX requests.

  This decorator performs JSON serialization which is shared by most of the
  handlers defined below.
  """
  def __init__(self, path_regex, verb='GET'):
    super(AjaxHandler, self).__init__(
        path_regex, verb, AjaxHandler.AjaxOutputFilter)

  @staticmethod
  def AjaxOutputFilter(http_code, headers, body):
    serialized_content = json.dumps(body, cls=serialization.Encoder)
    return http_code, headers, serialized_content


@AjaxHandler('/ajax/backends')
def _ListBackends(args, req_vars):  # pylint: disable=W0613
  return _HTTP_OK, [], [backend.name for backend in backends.ListBackends()]


@AjaxHandler('/ajax/devices')
def _ListDevices(args, req_vars):  # pylint: disable=W0613
  resp = []
  for device in backends.ListDevices():
    # The device settings must loaded at discovery time (i.e. here), not during
    # startup, because it might have been plugged later.
    for k, v in _persistent_storage.LoadSettings(device.id).iteritems():
      device.settings[k] = v

    resp += [{'backend': device.backend.name,
              'id': device.id,
              'name': device.name}]
  return _HTTP_OK, [], resp


@AjaxHandler(r'/ajax/dump/mmap/([^/]+)/([^/]+)/(\d+)')
def _DumpMmapsForProcess(args, req_vars):  # pylint: disable=W0613
  """Dumps memory maps for a process.

  The response is formatted according to the Google Charts DataTable format.
  """
  process = _GetProcess(args)
  if not process:
    return _HTTP_GONE, [], 'Device not found or process died'
  mmap = process.DumpMemoryMaps()
  table = _ConvertMmapToGTable(mmap)

  # Store the dump in the cache. The client might need it later for profiling.
  cache_id = _CacheObject(mmap)
  return _HTTP_OK, [], {'table': table, 'id': cache_id}


@AjaxHandler('/ajax/initialize/([^/]+)/([^/]+)$', 'POST')
def _InitializeDevice(args, req_vars):  # pylint: disable=W0613
  device = _GetDevice(args)
  if not device:
    return _HTTP_GONE, [], 'Device not found'
  device.Initialize()
  if req_vars['enableNativeTracing']:
    device.EnableNativeTracing(True)
  return _HTTP_OK, [], {
      'isNativeTracingEnabled': device.IsNativeTracingEnabled()}


@AjaxHandler(r'/ajax/profile/create', 'POST')
def _CreateProfile(args, req_vars):  # pylint: disable=W0613
  """Creates (and caches) a profile from a set of dumps.

  The profiling data can be retrieved afterwards using the /profile/{PROFILE_ID}
  endpoints (below).
  """
  classifier = None  # A classifier module (/classification/*_classifier.py).
  dumps = {}  # dump-time -> obj. to classify (e.g., |memory_map.Map|).
  for arg in 'type', 'source', 'ruleset':
    assert(arg in req_vars), 'Expecting %s argument in POST data' % arg

  # Step 1: collect the memory dumps, according to what the client specified in
  # the 'type' and 'source' POST arguments.

  # Case 1a: The client requests to load data from an archive.
  if req_vars['source'] == 'archive':
    archive = _persistent_storage.OpenArchive(req_vars['archive'])
    if not archive:
      return _HTTP_GONE, [], 'Cannot open archive %s' % req_vars['archive']
    first_timestamp = None
    for timestamp_str in req_vars['snapshots']:
      timestamp = file_storage.Archive.StrToTimestamp(timestamp_str)
      first_timestamp = first_timestamp or timestamp
      time_delta = int((timestamp - first_timestamp).total_seconds())
      if req_vars['type'] == 'mmap':
        dumps[time_delta] = archive.LoadMemMaps(timestamp)
      elif req_vars['type'] == 'nheap':
        dumps[time_delta] = archive.LoadNativeHeap(timestamp)

  # Case 1b: Use a dump recently cached (only mmap, via _DumpMmapsForProcess).
  elif req_vars['source'] == 'cache':
    assert(req_vars['type'] == 'mmap'), 'Only cached mmap dumps are supported.'
    dumps[0] = _GetCacheObject(req_vars['id'])

  if not dumps:
    return _HTTP_GONE, [], 'No memory dumps could be retrieved'

  # Initialize the classifier (mmap or nheap) and prepare symbols for nheap.
  if req_vars['type'] == 'mmap':
    classifier = mmap_classifier
  elif req_vars['type'] == 'nheap':
    classifier = native_heap_classifier
    if not archive.HasSymbols():
      return _HTTP_GONE, [], 'No symbols in archive %s' % req_vars['archive']
    symbols = archive.LoadSymbols()
    for nheap in dumps.itervalues():
      nheap.SymbolizeUsingSymbolDB(symbols)

  if not classifier:
    return _HTTP_GONE, [], 'Classifier %s not supported.' % req_vars['type']

  # Step 2: Load the rule-set specified by the client in the 'ruleset' POST arg.
  if req_vars['ruleset'] == 'heuristic':
    assert(req_vars['type'] == 'nheap'), (
        'heuristic rules are supported only for nheap')
    rules = native_heap_classifier.InferHeuristicRulesFromHeap(dumps[0])
  else:
    rules_path = os.path.join(constants.CLASSIFICATION_RULES_PATH,
                              req_vars['ruleset'])
    if not os.path.isfile(rules_path):
      return _HTTP_GONE, [], 'Cannot find the rule-set %s' % rules_path
    with open(rules_path) as f:
      rules = classifier.LoadRules(f.read())

  # Step 3: Aggregate the dump data using the classifier and generate the
  # profile data (which will be kept cached here in the server).
  # The resulting profile will consist of 1+ snapshots (depending on the number
  # dumps the client has requested to process) and a number of 1+ metrics
  # (depending on the buckets' keys returned by the classifier).

  # Converts the {time: dump_obj} dict into a {time: |AggregatedResult|} dict.
  # using the classifier.
  snapshots = collections.OrderedDict((time, classifier.Classify(dump, rules))
     for time, dump in sorted(dumps.iteritems()))

  # Add the profile to the cache (and eventually discard old items).
  # |profile_id| is the key that the client will use in subsequent requests
  # (to the /ajax/profile/{ID}/ endpoints) to refer to this particular profile.
  profile_id = _CacheObject(snapshots)

  first_snapshot = next(snapshots.itervalues())
  return _HTTP_OK, [], {'id': profile_id,
                        'times': snapshots.keys(),
                        'metrics': first_snapshot.keys,
                        'rootBucket': first_snapshot.total.name + '/'}


@AjaxHandler(r'/ajax/profile/([^/]+)/tree/(\d+)/(\d+)')
def _GetProfileTreeDataForSnapshot(args, req_vars):  # pylint: disable=W0613
  """Gets the data for the tree chart for a given time and metric.

  The response is formatted according to the Google Charts DataTable format.
  """
  snapshot_id = args[0]
  metric_index = int(args[1])
  time = int(args[2])
  snapshots = _GetCacheObject(snapshot_id)
  if not snapshots:
    return _HTTP_GONE, [], 'Cannot find the selected profile.'
  if time not in snapshots:
    return _HTTP_GONE, [], 'Cannot find snapshot at T=%d.' % time
  snapshot = snapshots[time]
  if metric_index >= len(snapshot.keys):
    return _HTTP_GONE, [], 'Invalid metric id %d' % metric_index

  resp = {'cols': [{'label': 'bucket', 'type': 'string'},
                   {'label': 'parent', 'type': 'string'}],
          'rows': []}

  def VisitBucketAndAddRows(bucket, parent_id=''):
    """Recursively creates the (node, parent) visiting |ResultTree| in DFS."""
    node_id = parent_id + bucket.name + '/'
    node_label = '<dl><dt>%s</dt><dd>%s</dd></dl>' % (
        bucket.name, _StrMem(bucket.values[metric_index]))
    resp['rows'] += [{'c': [
        {'v': node_id, 'f': node_label},
        {'v': parent_id, 'f': None},
    ]}]
    for child in bucket.children:
      VisitBucketAndAddRows(child, node_id)

  VisitBucketAndAddRows(snapshot.total)
  return _HTTP_OK, [], resp


@AjaxHandler(r'/ajax/profile/([^/]+)/time_serie/(\d+)/(.*)$')
def _GetTimeSerieForSnapshot(args, req_vars):  # pylint: disable=W0613
  """Gets the data for the area chart for a given metric and bucket.

  The response is formatted according to the Google Charts DataTable format.
  """
  snapshot_id = args[0]
  metric_index = int(args[1])
  bucket_path = args[2]
  snapshots = _GetCacheObject(snapshot_id)
  if not snapshots:
    return _HTTP_GONE, [], 'Cannot find the selected profile.'
  if metric_index >= len(next(snapshots.itervalues()).keys):
    return _HTTP_GONE, [], 'Invalid metric id %d' % metric_index

  def FindBucketByPath(bucket, path, parent_path=''):  # Essentially a DFS.
    cur_path = parent_path + bucket.name + '/'
    if cur_path == path:
      return bucket
    for child in bucket.children:
      res = FindBucketByPath(child, path, cur_path)
      if res:
        return res
    return None

  # The resulting data table will look like this (assuming len(metrics) == 2):
  # Time  Ashmem      Dalvik     Other
  # 0    (1024,0)  (4096,1024)  (0,0)
  # 30   (512,512) (1024,1024)  (0,512)
  # 60   (0,512)   (1024,0)     (512,0)
  resp = {'cols': [], 'rows': []}
  for time, aggregated_result in snapshots.iteritems():
    bucket = FindBucketByPath(aggregated_result.total, bucket_path)
    if not bucket:
      return _HTTP_GONE, [], 'Bucket %s not found' % bucket_path

    # If the user selected a non-leaf bucket, display the breakdown of its
    # direct children. Otherwise just the leaf bucket.
    children_buckets = bucket.children if bucket.children else [bucket]

    # Create the columns (form the buckets) when processing the first snapshot.
    if not resp['cols']:
      resp['cols'] += [{'label': 'Time', 'type': 'string'}]
      for child_bucket in children_buckets:
        resp['cols'] += [{'label': child_bucket.name, 'type': 'number'}]

    row = [{'v': str(time), 'f': None}]
    for child_bucket in children_buckets:
      row += [{'v': child_bucket.values[metric_index] / 1024, 'f': None}]
    resp['rows'] += [{'c': row}]

  return _HTTP_OK, [], resp

@AjaxHandler(r'/ajax/profile/rules')
def _ListProfilingRules(args, req_vars):  # pylint: disable=W0613
  """Lists the classification rule files available for profiling."""
  rules = glob.glob(constants.CLASSIFICATION_RULES_PATH +
                    os.sep + '*' + os.sep + '*.py')
  rules = [x.replace(constants.CLASSIFICATION_RULES_PATH, '')[1:]  # Strip /.
           for x in rules]
  resp = {'mmap': filter(lambda x: 'mmap-' in x, rules),
          'nheap': filter(lambda x: 'nheap-' in x, rules)}
  resp['nheap'].insert(0, 'heuristic')
  return _HTTP_OK, [], resp


@AjaxHandler(r'/ajax/ps/([^/]+)/([^/]+)$')  # /ajax/ps/Android/a0b1c2[?all=1]
def _ListProcesses(args, req_vars):  # pylint: disable=W0613
  """Lists processes and their CPU / mem stats.

  The response is formatted according to the Google Charts DataTable format.
  """
  device = _GetDevice(args)
  if not device:
    return _HTTP_GONE, [], 'Device not found'
  resp = {
      'cols': [
          {'label': 'Pid', 'type':'number'},
          {'label': 'Name', 'type':'string'},
          {'label': 'Cpu %', 'type':'number'},
          {'label': 'Mem RSS Kb', 'type':'number'},
          {'label': '# Threads', 'type':'number'},
        ],
      'rows': []}
  for process in device.ListProcesses():
    # Exclude system apps if the request didn't contain the ?all=1 arg.
    if not req_vars.get('all') and not re.match(_APP_PROCESS_RE, process.name):
      continue
    stats = process.GetStats()
    resp['rows'] += [{'c': [
        {'v': process.pid, 'f': None},
        {'v': process.name, 'f': None},
        {'v': stats.cpu_usage, 'f': None},
        {'v': stats.vm_rss, 'f': None},
        {'v': stats.threads, 'f': None},
    ]}]
  return _HTTP_OK, [], resp


@AjaxHandler(r'/ajax/stats/([^/]+)/([^/]+)$')  # /ajax/stats/Android/a0b1c2
def _GetDeviceStats(args, req_vars):  # pylint: disable=W0613
  """Lists device CPU / mem stats.

  The response is formatted according to the Google Charts DataTable format.
  """
  device = _GetDevice(args)
  if not device:
    return _HTTP_GONE, [], 'Device not found'
  device_stats = device.GetStats()

  cpu_stats = {
      'cols': [
          {'label': 'CPU', 'type':'string'},
          {'label': 'Usr %', 'type':'number'},
          {'label': 'Sys %', 'type':'number'},
          {'label': 'Idle %', 'type':'number'},
        ],
      'rows': []}

  for cpu_idx in xrange(len(device_stats.cpu_times)):
    cpu = device_stats.cpu_times[cpu_idx]
    cpu_stats['rows'] += [{'c': [
        {'v': '# %d' % cpu_idx, 'f': None},
        {'v': cpu['usr'], 'f': None},
        {'v': cpu['sys'], 'f': None},
        {'v': cpu['idle'], 'f': None},
    ]}]

  mem_stats = {
      'cols': [
          {'label': 'Section', 'type':'string'},
          {'label': 'MB', 'type':'number',  'pattern': ''},
        ],
      'rows': []}

  for key, value in device_stats.memory_stats.iteritems():
    mem_stats['rows'] += [{'c': [
        {'v': key, 'f': None},
        {'v': value / 1024, 'f': None}
    ]}]

  return _HTTP_OK, [], {'cpu': cpu_stats, 'mem': mem_stats}


@AjaxHandler(r'/ajax/stats/([^/]+)/([^/]+)/(\d+)$')  # /ajax/stats/Android/a0/3
def _GetProcessStats(args, req_vars):  # pylint: disable=W0613
  """Lists CPU / mem stats for a given process (and keeps history).

  The response is formatted according to the Google Charts DataTable format.
  """
  process = _GetProcess(args)
  if not process:
    return _HTTP_GONE, [], 'Device not found'

  proc_uri = '/'.join(args)
  cur_stats = process.GetStats()
  if proc_uri not in _proc_stats_history:
    _proc_stats_history[proc_uri] = collections.deque(maxlen=_STATS_HIST_SIZE)
  history = _proc_stats_history[proc_uri]
  history.append(cur_stats)

  cpu_stats = {
      'cols': [
          {'label': 'T', 'type':'string'},
          {'label': 'CPU %', 'type':'number'},
          {'label': '# Threads', 'type':'number'},
        ],
      'rows': []
  }

  mem_stats = {
      'cols': [
          {'label': 'T', 'type':'string'},
          {'label': 'Mem RSS Kb', 'type':'number'},
          {'label': 'Page faults', 'type':'number'},
        ],
      'rows': []
  }

  for stats in history:
    cpu_stats['rows'] += [{'c': [
          {'v': str(datetime.timedelta(seconds=stats.run_time)), 'f': None},
          {'v': stats.cpu_usage, 'f': None},
          {'v': stats.threads, 'f': None},
    ]}]
    mem_stats['rows'] += [{'c': [
          {'v': str(datetime.timedelta(seconds=stats.run_time)), 'f': None},
          {'v': stats.vm_rss, 'f': None},
          {'v': stats.page_faults, 'f': None},
    ]}]

  return _HTTP_OK, [], {'cpu': cpu_stats, 'mem': mem_stats}


@AjaxHandler(r'/ajax/settings/([^/]+)/?(\w+)?$')  # /ajax/settings/Android[/id]
def _GetDeviceOrBackendSettings(args, req_vars):  # pylint: disable=W0613
  backend = backends.GetBackend(args[0])
  if not backend:
    return _HTTP_GONE, [], 'Backend not found'
  if args[1]:
    device = _GetDevice(args)
    if not device:
      return _HTTP_GONE, [], 'Device not found'
    settings = device.settings
  else:
    settings = backend.settings

  assert(isinstance(settings, backends.Settings))
  resp = {}
  for key  in settings.expected_keys:
    resp[key] = {'description': settings.expected_keys[key],
                 'value': settings.values[key]}
  return _HTTP_OK, [], resp


@AjaxHandler(r'/ajax/settings/([^/]+)/?(\w+)?$', 'POST')
def _SetDeviceOrBackendSettings(args, req_vars):  # pylint: disable=W0613
  backend = backends.GetBackend(args[0])
  if not backend:
    return _HTTP_GONE, [], 'Backend not found'
  if args[1]:
    device = _GetDevice(args)
    if not device:
      return _HTTP_GONE, [], 'Device not found'
    settings = device.settings
    storage_name = device.id
  else:
    settings = backend.settings
    storage_name = backend.name

  for key in req_vars.iterkeys():
    settings[key] = req_vars[key]
  _persistent_storage.StoreSettings(storage_name, settings.values)
  return _HTTP_OK, [], ''


@AjaxHandler(r'/ajax/storage/list')
def _ListStorage(args, req_vars):  # pylint: disable=W0613
  resp = {
      'cols': [
          {'label': 'Archive', 'type':'string'},
          {'label': 'Snapshot', 'type':'string'},
          {'label': 'Mem maps', 'type':'boolean'},
          {'label': 'N. Heap', 'type':'boolean'},
        ],
      'rows': []}
  for archive_name in _persistent_storage.ListArchives():
    archive = _persistent_storage.OpenArchive(archive_name)
    first_timestamp = None
    for timestamp in archive.ListSnapshots():
      first_timestamp = timestamp if not first_timestamp else first_timestamp
      time_delta = '%d s.' % (timestamp - first_timestamp).total_seconds()
      resp['rows'] += [{'c': [
          {'v': archive_name, 'f': None},
          {'v': file_storage.Archive.TimestampToStr(timestamp),
           'f': time_delta},
          {'v': archive.HasMemMaps(timestamp), 'f': None},
          {'v': archive.HasNativeHeap(timestamp), 'f': None},
      ]}]
  return _HTTP_OK, [], resp


@AjaxHandler(r'/ajax/storage/(.+)/(.+)/mmaps')
def _LoadMmapsFromStorage(args, req_vars):  # pylint: disable=W0613
  archive = _persistent_storage.OpenArchive(args[0])
  if not archive:
    return _HTTP_GONE, [], 'Cannot open archive %s' % req_vars['archive']

  timestamp = file_storage.Archive.StrToTimestamp(args[1])
  if not archive.HasMemMaps(timestamp):
    return _HTTP_GONE, [], 'No mmaps for snapshot %s' % timestamp
  mmap = archive.LoadMemMaps(timestamp)
  return _HTTP_OK, [], {'table': _ConvertMmapToGTable(mmap)}


@AjaxHandler(r'/ajax/storage/(.+)/(.+)/nheap')
def _LoadNheapFromStorage(args, req_vars):
  """Returns a Google Charts DataTable dictionary for the nheap."""
  archive = _persistent_storage.OpenArchive(args[0])
  if not archive:
    return _HTTP_GONE, [], 'Cannot open archive %s' % req_vars['archive']

  timestamp = file_storage.Archive.StrToTimestamp(args[1])
  if not archive.HasNativeHeap(timestamp):
    return _HTTP_GONE, [], 'No native heap dump for snapshot %s' % timestamp

  nheap = archive.LoadNativeHeap(timestamp)
  symbols = archive.LoadSymbols()
  nheap.SymbolizeUsingSymbolDB(symbols)

  resp = {
      'cols': [
          {'label': 'Allocated', 'type':'number'},
          {'label': 'Resident', 'type':'number'},
          {'label': 'Flags', 'type':'number'},
          {'label': 'Stack Trace', 'type':'string'},
        ],
      'rows': []}
  for alloc in nheap.allocations:
    strace = '<dl>'
    for frame in alloc.stack_trace.frames:
      # Use the fallback libname.so+0xaddr if symbol info is not available.
      symbol_name = frame.symbol.name if frame.symbol else '??'
      source_info = (str(frame.symbol.source_info[0]) if
          frame.symbol and frame.symbol.source_info else frame.raw_address)
      strace += '<dd title="%s">%s</dd><dt>%s</dt>' % (
          cgi.escape(source_info),
          cgi.escape(posixpath.basename(source_info)),
          cgi.escape(symbol_name))
    strace += '</dl>'

    resp['rows'] += [{'c': [
        {'v': alloc.size, 'f': _StrMem(alloc.size)},
        {'v': alloc.resident_size, 'f': _StrMem(alloc.resident_size)},
        {'v': alloc.flags, 'f': None},
        {'v': strace, 'f': None},
    ]}]
  return _HTTP_OK, [], resp


# /ajax/tracer/start/Android/device-id/pid
@AjaxHandler(r'/ajax/tracer/start/([^/]+)/([^/]+)/(\d+)', 'POST')
def _StartTracer(args, req_vars):
  for arg in 'interval', 'count', 'traceNativeHeap':
    assert(arg in req_vars), 'Expecting %s argument in POST data' % arg
  process = _GetProcess(args)
  if not process:
    return _HTTP_GONE, [], 'Device not found or process died'
  task_id = background_tasks.StartTracer(
      storage_path=_PERSISTENT_STORAGE_PATH,
      process=process,
      interval=int(req_vars['interval']),
      count=int(req_vars['count']),
      trace_native_heap=req_vars['traceNativeHeap'])
  return _HTTP_OK, [], task_id


@AjaxHandler(r'/ajax/tracer/status/(\d+)')  # /ajax/tracer/status/{task_id}
def _GetTracerStatus(args, req_vars):  # pylint: disable=W0613
  task = background_tasks.Get(int(args[0]))
  if not task:
    return _HTTP_GONE, [], 'Task not found'
  return _HTTP_OK, [], task.GetProgress()


@UriHandler(r'^(?!/ajax)/(.*)$')
def _StaticContent(args, req_vars):  # pylint: disable=W0613
  req_path = args[0] if args[0] else 'index.html'
  file_path = os.path.abspath(os.path.join(_CONTENT_DIR, req_path))
  if (os.path.isfile(file_path) and
      os.path.commonprefix([file_path, _CONTENT_DIR]) == _CONTENT_DIR):
    mtype = 'text/plain'
    guessed_mime = mimetypes.guess_type(file_path)
    if guessed_mime and guessed_mime[0]:
      mtype = guessed_mime[0]
    with open(file_path, 'rb') as f:
      body = f.read()
    return _HTTP_OK, [('Content-Type', mtype)], body
  return _HTTP_NOT_FOUND, [],  file_path + ' not found'


def _GetDevice(args):
  """Returns a |backends.Device| instance from a /backend/device URI."""
  assert(len(args) >= 2), 'Malformed request. Expecting /backend/device'
  return backends.GetDevice(backend_name=args[0], device_id=args[1])


def _GetProcess(args):
  """Returns a |backends.Process| instance from a /backend/device/pid URI."""
  assert(len(args) >= 3 and args[2].isdigit()), (
      'Malformed request. Expecting /backend/device/pid')
  device = _GetDevice(args)
  if not device:
    return None
  return device.GetProcess(int(args[2]))

def _ConvertMmapToGTable(mmap):
  """Returns a Google Charts DataTable dictionary for the given mmap."""
  assert(isinstance(mmap, memory_map.Map))
  table = {
      'cols': [
          {'label': 'Start', 'type':'string'},
          {'label': 'End', 'type':'string'},
          {'label': 'Length Kb', 'type':'number'},
          {'label': 'Prot', 'type':'string'},
          {'label': 'RSS Kb', 'type':'number'},
          {'label': 'Priv. Dirty Kb', 'type':'number'},
          {'label': 'Priv. Clean Kb', 'type':'number'},
          {'label': 'Shared Dirty Kb', 'type':'number'},
          {'label': 'Shared Clean Kb', 'type':'number'},
          {'label': 'File', 'type':'string'},
          {'label': 'Offset', 'type':'number'},
          {'label': 'Resident Pages', 'type':'string'},
        ],
      'rows': []}
  for entry in mmap.entries:
    table['rows'] += [{'c': [
        {'v': '%08x' % entry.start, 'f': None},
        {'v': '%08x' % entry.end, 'f': None},
        {'v': entry.len / 1024, 'f': None},
        {'v': entry.prot_flags, 'f': None},
        {'v': entry.rss_bytes / 1024, 'f': None},
        {'v': entry.priv_dirty_bytes / 1024, 'f': None},
        {'v': entry.priv_clean_bytes / 1024, 'f': None},
        {'v': entry.shared_dirty_bytes / 1024, 'f': None},
        {'v': entry.shared_clean_bytes / 1024, 'f': None},
        {'v': entry.mapped_file, 'f': None},
        {'v': entry.mapped_offset, 'f': None},
        {'v': '[%s]' % (','.join(map(str, entry.resident_pages))), 'f': None},
    ]}]
  return table

def _CacheObject(obj_to_store):
  """Stores an object in the server-side cache and returns its unique id."""
  if len(_cached_objs) >= _CACHE_LEN:
    _cached_objs.popitem(last=False)
  obj_id = uuid.uuid4().hex
  _cached_objs[obj_id] = obj_to_store
  return str(obj_id)


def _GetCacheObject(obj_id):
  """Retrieves an object in the server-side cache by its id."""
  return _cached_objs.get(obj_id)


def _StrMem(nbytes):
  """Converts a number (of bytes) into a human readable string (kb, mb)."""
  UNITS = ['B', 'K', 'M', 'G']
  for unit in UNITS:
    if abs(nbytes) < 1024.0 or unit == UNITS[-1]:
      return ('%3.1f' % nbytes).replace('.0','') + ' ' + unit
    nbytes /= 1024.0


def _HttpRequestHandler(environ, start_response):
  """Parses a single HTTP request and delegates the handling through UriHandler.

  This essentially wires up wsgiref.simple_server with our @UriHandler(s).
  """
  path = environ['PATH_INFO']
  method = environ['REQUEST_METHOD']
  if method == 'POST':
    req_body_size = int(environ.get('CONTENT_LENGTH', 0))
    req_body = environ['wsgi.input'].read(req_body_size)
    req_vars = json.loads(req_body)
  else:
    req_vars = urlparse.parse_qs(environ['QUERY_STRING'])
  (http_code, headers, body) = UriHandler.Handle(method, path, req_vars)
  start_response(http_code, headers)
  return [body]


def Start(http_port):
  # Load the saved backends' settings (some of them might be needed to bootstrap
  # as, for instance, the adb path for the Android backend).
  memory_inspector.RegisterAllBackends()
  for backend in backends.ListBackends():
    for k, v in _persistent_storage.LoadSettings(backend.name).iteritems():
      backend.settings[k] = v

  httpd = wsgiref.simple_server.make_server(
      '127.0.0.1', http_port, _HttpRequestHandler)
  try:
    httpd.serve_forever()
  except KeyboardInterrupt:
    pass  # Don't print useless stack traces when the user hits CTRL-C.
