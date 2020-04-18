# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from queuing_lens import QueuingLens
import request_track
import test_utils

class QueuingLensTestCase(unittest.TestCase):
  MILLIS_TO_MICROS = 1000
  MILLIS_TO_SECONDS = 0.001
  URL_1 = 'http://1'
  URL_2 = 'http://2'

  def testRequestQueuing(self):
    # http://1: queued at 5ms, request start at 10ms, ready at 11ms,
    #           done at 12ms; blocked by 2.
    # http://2: queued at 4ms, request start at 4ms, ready at 5 ms, done at 9ms.
    trace_events = [
        {'args': {
            'data': {
                'request_url': self.URL_1,
                'source_id': 1
            }
          },
         'cat': QueuingLens.QUEUING_CATEGORY,
         'id': 1,
         'name': QueuingLens.ASYNC_NAME,
         'ph': 'b',
         'ts': 5 * self.MILLIS_TO_MICROS
        },
        {'args': {
            'data': {
                'source_id': 1
            }
          },
         'cat': QueuingLens.QUEUING_CATEGORY,
         'id': 1,
         'name': QueuingLens.READY_NAME,
         'ph': 'n',
         'ts': 10 * self.MILLIS_TO_MICROS
        },
        {'args': {
            'data': {
                'source_id': 1
            }
          },
         'cat': QueuingLens.QUEUING_CATEGORY,
         'id': 1,
         'name': QueuingLens.ASYNC_NAME,
         'ph': 'e',
         'ts': 12 * self.MILLIS_TO_MICROS
        },

        {'args': {
            'data': {
                'request_url': self.URL_2,
                'source_id': 2
            }
          },
         'cat': QueuingLens.QUEUING_CATEGORY,
         'id': 2,
         'name': QueuingLens.ASYNC_NAME,
         'ph': 'b',
         'ts': 4 * self.MILLIS_TO_MICROS
        },
        {'args': {
            'data': {
                'source_id': 2
            }
          },
         'cat': QueuingLens.QUEUING_CATEGORY,
         'id': 2,
         'name': QueuingLens.READY_NAME,
         'ph': 'n',
         'ts': 5 * self.MILLIS_TO_MICROS
        },
        {'args': {
            'data': {
                'source_id': 2
            }
          },
         'cat': QueuingLens.QUEUING_CATEGORY,
         'id': 2,
         'name': QueuingLens.ASYNC_NAME,
         'ph': 'e',
         'ts': 9 * self.MILLIS_TO_MICROS
        }]
    requests = [
        request_track.Request.FromJsonDict(
            {'url': self.URL_1,
             'request_id': '0.1',
             'timing': {'request_time': 10 * self.MILLIS_TO_SECONDS,
                        'loading_finished': 2}}),
        request_track.Request.FromJsonDict(
            {'url': self.URL_2,
             'request_id': '0.2',
             'timing': {'request_time': 4 * self.MILLIS_TO_SECONDS,
                        'loading_finished': 5}})]
    trace = test_utils.LoadingTraceFromEvents(
        requests=requests, trace_events=trace_events)
    queue_info = QueuingLens(trace).GenerateRequestQueuing()
    self.assertEqual(set(['0.2']),
                     set(rq.request_id
                         for rq in queue_info[requests[0]].blocking))
    self.assertEqual((5., 10., 12.), (queue_info[requests[0]].start_msec,
                                      queue_info[requests[0]].ready_msec,
                                      queue_info[requests[0]].end_msec))
    self.assertEqual(0, len(queue_info[requests[1]].blocking))
    self.assertEqual((4., 5., 9.), (queue_info[requests[1]].start_msec,
                                    queue_info[requests[1]].ready_msec,
                                    queue_info[requests[1]].end_msec))
