# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging

from telemetry.core import exceptions
from telemetry.timeline import model
from tracing.trace_data import trace_data


class InspectorNetworkException(Exception):
  pass


class InspectorNetworkResponseData(object):
  def __init__(self, inspector_network, params, initiator):
    """Creates a new InspectorNetworkResponseData instance.

    Args:
      inspector_network: InspectorNetwork instance.
      params: the 'params' field of the devtools Network.responseReceived event.
      initiator: initiator of the request, as gathered from
                 Network.requestWillBeSent.
    """
    self._inspector_network = inspector_network
    self._request_id = params['requestId']
    self._timestamp = params['timestamp']
    self._initiator = initiator

    self._response = params['response']
    if not self._response:
      raise InspectorNetworkException('response must exist')

    # Response headers.
    headers = self._response['headers']
    self._header_map = {}
    for k, v in headers.iteritems():
      # Camel-case header keys.
      self._header_map[k.title()] = v

    # Request headers.
    self._request_header_map = {}
    if 'requestHeaders' in self._response:
      # Camel-case header keys.
      for k, v in self._response['requestHeaders'].iteritems():
        self._request_header_map[k.title()] = v

    self._body = None
    self._base64_encoded = False
    if self._inspector_network:
      self._served_from_cache = (
          self._inspector_network.HTTPResponseServedFromCache(self._request_id))
    else:
      self._served_from_cache = False

    # Whether constructed from a timeline event.
    self._from_event = False

  @property
  def status(self):
    return self._response['status']

  @property
  def status_text(self):
    return self._response['status_text']

  @property
  def headers(self):
    return self._header_map

  @property
  def request_headers(self):
    return self._request_header_map

  @property
  def timestamp(self):
    return self._timestamp

  @property
  def timing(self):
    if 'timing' in self._response:
      return self._response['timing']
    return None

  @property
  def url(self):
    return self._response['url']

  @property
  def request_id(self):
    return self._request_id

  @property
  def served_from_cache(self):
    return self._served_from_cache

  @property
  def initiator(self):
    return self._initiator

  def GetHeader(self, name):
    if name in self.headers:
      return self.headers[name]
    return None

  def GetBody(self, timeout=60):
    if not self._body and not self._from_event:
      self._body, self._base64_encoded = (
        self._inspector_network.GetHTTPResponseBody(self._request_id, timeout))
    return self._body, self._base64_encoded

  def AsTimelineEvent(self):
    event = {}
    event['type'] = 'HTTPResponse'
    event['startTime'] = self.timestamp
    # There is no end time. Just return the timestamp instead.
    event['endTime'] = self.timestamp
    event['requestId'] = self.request_id
    event['response'] = self._response
    event['body'], event['base64_encoded_body'] = self.GetBody()
    event['served_from_cache'] = self.served_from_cache
    event['initiator'] = self._initiator
    return event

  @staticmethod
  def FromTimelineEvent(event):
    assert event.name == 'HTTPResponse'
    params = {}
    params['timestamp'] = event.start
    params['requestId'] = event.args['requestId']
    params['response'] = event.args['response']
    recorded = InspectorNetworkResponseData(None, params, None)
    # pylint: disable=protected-access
    recorded._body = event.args['body']
    recorded._base64_encoded = event.args['base64_encoded_body']
    recorded._served_from_cache = event.args['served_from_cache']
    recorded._initiator = event.args.get('initiator', None)
    recorded._from_event = True
    return recorded


class InspectorNetwork(object):
  def __init__(self, inspector_websocket):
    self._inspector_websocket = inspector_websocket
    self._http_responses = []
    self._served_from_cache = set()
    self._timeline_recorder = None
    self._initiators = {}
    self._finished = {}

  def ClearCache(self, timeout=60):
    """Clears the browser's disk and memory cache."""
    res = self._inspector_websocket.SyncRequest({
        'method': 'Network.canClearBrowserCache'
        }, timeout)
    assert res['result'], 'Cache clearing is not supported by this browser.'
    self._inspector_websocket.SyncRequest({
        'method': 'Network.clearBrowserCache'
        }, timeout)

  def StartMonitoringNetwork(self, timeout=60):
    """Starts monitoring network notifications and recording HTTP responses."""
    self.ClearResponseData()
    self._inspector_websocket.RegisterDomain(
        'Network',
        self._OnNetworkNotification)
    request = {
        'method': 'Network.enable'
        }
    self._inspector_websocket.SyncRequest(request, timeout)

  def StopMonitoringNetwork(self, timeout=60):
    """Stops monitoring network notifications and recording HTTP responses."""
    request = {
        'method': 'Network.disable'
        }
    self._inspector_websocket.SyncRequest(request, timeout)
    # There may be queued messages that don't appear until the SyncRequest
    # happens. Wait to unregister until after sending the disable command.
    self._inspector_websocket.UnregisterDomain('Network')

  def GetResponseData(self):
    """Returns all recorded HTTP responses."""
    return [self._AugmentResponse(rsp) for rsp in self._http_responses]

  def ClearResponseData(self):
    """Clears recorded HTTP responses."""
    self._http_responses = []
    self._served_from_cache.clear()
    self._initiators.clear()

  def _AugmentResponse(self, response):
    """Augments an InspectorNetworkResponseData for final output.

    Join the loadingFinished timing event to the response. This event is
    timestamped with epoch seconds. In the response timing object, all timing
    aside from requestTime is in millis relative to requestTime, so
    loadingFinished is converted to be consistent.

    Args:
      response: an InspectorNetworkResponseData instance to augment.

    Returns:
      The same response, modifed as described above.

    """
    if response.timing is None:
      return response

    if response.request_id not in self._finished:
      response.timing['loadingFinished'] = -1
    else:
      delta_ms = 1000 * (self._finished[response.request_id] -
                         response.timing['requestTime'])
      if delta_ms < 0:
        delta_ms = -1
      response.timing['loadingFinished'] = delta_ms
    return response

  def _OnNetworkNotification(self, msg):
    if msg['method'] == 'Network.requestWillBeSent':
      self._ProcessRequestWillBeSent(msg['params'])
    if msg['method'] == 'Network.responseReceived':
      self._RecordHTTPResponse(msg['params'])
    elif msg['method'] == 'Network.requestServedFromCache':
      self._served_from_cache.add(msg['params']['requestId'])
    elif msg['method'] == 'Network.loadingFinished':
      assert msg['params']['requestId'] not in self._finished
      self._finished[msg['params']['requestId']] = msg['params']['timestamp']

  def _ProcessRequestWillBeSent(self, params):
    request_id = params['requestId']
    self._initiators[request_id] = params['initiator']

  def _RecordHTTPResponse(self, params):
    required_fields = ['requestId', 'timestamp', 'response']
    for field in required_fields:
      if field not in params:
        logging.warning('HTTP Response missing required field: %s', field)
        return
    request_id = params['requestId']
    if request_id not in self._initiators:
      logging.warning('Dropped a message with no initiator.')
      return
    initiator = self._initiators[request_id]
    self._http_responses.append(
        InspectorNetworkResponseData(self, params, initiator))

  def GetHTTPResponseBody(self, request_id, timeout=60):
    try:
      res = self._inspector_websocket.SyncRequest({
          'method': 'Network.getResponseBody',
          'params': {
              'requestId': request_id,
              }
          }, timeout)
    except exceptions.TimeoutException:
      logging.warning('Timeout during fetching body for %s' % request_id)
      return None, False
    if 'error' in res:
      return None, False
    return res['result']['body'], res['result']['base64Encoded']

  def HTTPResponseServedFromCache(self, request_id):
    return request_id and request_id in self._served_from_cache

  @property
  def timeline_recorder(self):
    if not self._timeline_recorder:
      self._timeline_recorder = TimelineRecorder(self)
    return self._timeline_recorder


class TimelineRecorder(object):
  def __init__(self, inspector_network):
    self._inspector_network = inspector_network
    self._is_recording = False

  def Start(self):
    assert not self._is_recording, 'Start should only be called once.'
    self._is_recording = True
    self._inspector_network.StartMonitoringNetwork()

  def Stop(self):
    if not self._is_recording:
      return None
    responses = self._inspector_network.GetResponseData()
    events = [r.AsTimelineEvent() for r in list(responses)]
    self._inspector_network.StopMonitoringNetwork()
    self._is_recording = False
    if len(events) == 0:
      return None
    builder = trace_data.TraceDataBuilder()
    builder.AddTraceFor(trace_data.INSPECTOR_TRACE_PART, events)
    return model.TimelineModel(builder.AsData(), shift_world_to_zero=False)
