#!/usr/bin/env python
# Copyright 2014 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import binascii
import os
import re
import sys
import time
import unittest

# Somehow this lets us find isolate_storage
import net_utils

from depot_tools import auto_stub
import isolate_storage
import test_utils

class ByteStreamStubMock(object):
  """Replacement for real gRPC stub

  We can't mock *within* the real stub to replace individual functions, plus
  we'd have to mock __init__ every time anyway. So this class replaces the
  entire stub. As for the functions, they implement default happy path
  behaviour where possible, and are not implemented otherwise.
  """
  def __init__(self, _channel):
    self._push_requests = []
    self._contains_requests = []

  def Read(self, request, timeout=None):
    del request, timeout
    raise NotImplementedError()

  def Write(self, requests, timeout=None):
    del timeout
    nb = 0
    for r in requests:
      nb += len(r.data)
      self._push_requests.append(r.__deepcopy__())
    resp = isolate_storage.bytestream_pb2.WriteResponse()
    resp.committed_size = nb
    return resp

  def popContainsRequests(self):
    cr = self._contains_requests
    self._contains_requests = []
    return cr

  def popPushRequests(self):
    pr = self._push_requests
    self._push_requests = []
    return pr

def raiseError(code):
  raise isolate_storage.grpc.RpcError(
      'cannot turn this into a real code yet: %s' % code)

class IsolateStorageTest(auto_stub.TestCase):
  def get_server(self):
    return isolate_storage.IsolateServerGrpc('https://luci.appspot.com',
                                             'default-gzip',
                                             'https://luci.com/client/bob')

  def testFetchHappySimple(self):
    """Fetch: if we get a few chunks with the right offset, everything works"""
    def Read(self, request, timeout=None):
      del timeout
      self.request = request
      response = isolate_storage.bytestream_pb2.ReadResponse()
      for i in range(0, 3):
        response.data = str(i)
        yield response
    self.mock(ByteStreamStubMock, 'Read', Read)

    s = self.get_server()
    replies = s.fetch('abc123', 1, 0)
    response = replies.next()
    self.assertEqual('0', response)
    response = replies.next()
    self.assertEqual('1', response)
    response = replies.next()
    self.assertEqual('2', response)

  def testFetchHappyZeroLengthBlob(self):
    """Fetch: if we get a zero-length blob, everything works"""
    def Read(self, request, timeout=None):
      del timeout
      self.request = request
      response = isolate_storage.bytestream_pb2.ReadResponse()
      response.data = ''
      yield response
    self.mock(ByteStreamStubMock, 'Read', Read)

    s = self.get_server()
    replies = s.fetch('abc123', 1, 0)
    reply = replies.next()
    self.assertEqual(0, len(reply))

  def testFetchThrowsOnFailure(self):
    """Fetch: if something goes wrong in Isolate, we throw an exception"""
    def Read(self, request, timeout=None):
      del timeout
      self.request = request
      raiseError(isolate_storage.grpc.StatusCode.INTERNAL)
    self.mock(ByteStreamStubMock, 'Read', Read)

    s = self.get_server()
    replies = s.fetch('abc123', 1, 0)
    with self.assertRaises(IOError):
      _response = replies.next()

  def testFetchThrowsCorrectExceptionOnGrpcFailure(self):
    """Fetch: if something goes wrong in gRPC, we throw an IOError"""
    def Read(_self, _request, timeout=None):
      del timeout
      raise isolate_storage.grpc.RpcError('proxy died during initial fetch :(')
    self.mock(ByteStreamStubMock, 'Read', Read)

    s = self.get_server()
    replies = s.fetch('abc123', 1, 0)
    with self.assertRaises(IOError):
      _response = replies.next()

  def testFetchThrowsCorrectExceptionOnStreamingGrpcFailure(self):
    """Fetch: if something goes wrong in gRPC, we throw an IOError"""
    def Read(self, request, timeout=None):
      del timeout
      self.request = request
      response = isolate_storage.bytestream_pb2.ReadResponse()
      for i in range(0, 3):
        if i is 2:
          raise isolate_storage.grpc.RpcError(
              'proxy died during fetch stream :(')
        response.data = str(i)
        yield response
    self.mock(ByteStreamStubMock, 'Read', Read)

    s = self.get_server()
    with self.assertRaises(IOError):
      for _response in s.fetch('abc123', 1, 0):
        pass

  def testPushHappySingleSmall(self):
    """Push: send one chunk of small data"""
    s = self.get_server()
    i = isolate_storage.Item(digest='abc123', size=4)
    s.push(i, isolate_storage._IsolateServerGrpcPushState(), '1234')
    requests = s._proxy._stub.popPushRequests()
    self.assertEqual(1, len(requests))
    m = re.search('client/bob/uploads/.*/blobs/abc123/4',
                  requests[0].resource_name)
    self.assertTrue(m)
    self.assertEqual('1234', requests[0].data)
    self.assertEqual(0, requests[0].write_offset)
    self.assertTrue(requests[0].finish_write)

  def testPushHappySingleBig(self):
    """Push: send one chunk of big data by splitting it into two"""
    self.mock(isolate_storage, 'NET_IO_FILE_CHUNK', 3)
    s = self.get_server()
    i = isolate_storage.Item(digest='abc123', size=4)
    s.push(i, isolate_storage._IsolateServerGrpcPushState(), '1234')
    requests = s._proxy._stub.popPushRequests()
    self.assertEqual(2, len(requests))
    m = re.search('client/bob/uploads/.*/blobs/abc123/4',
                  requests[0].resource_name)
    self.assertTrue(m)
    self.assertEqual('123', requests[0].data)
    self.assertEqual(0, requests[0].write_offset)
    self.assertFalse(requests[0].finish_write)
    self.assertEqual('4', requests[1].data)
    self.assertEqual(3, requests[1].write_offset)
    self.assertTrue(requests[1].finish_write)

  def testPushHappyMultiSmall(self):
    """Push: sends multiple small chunks"""
    s = self.get_server()
    i = isolate_storage.Item(digest='abc123', size=4)
    s.push(i, isolate_storage._IsolateServerGrpcPushState(), ['12', '34'])
    requests = s._proxy._stub.popPushRequests()
    self.assertEqual(2, len(requests))
    m = re.search('client/bob/uploads/.*/blobs/abc123/4',
                  requests[0].resource_name)
    self.assertTrue(m)
    self.assertEqual('12', requests[0].data)
    self.assertEqual(0, requests[0].write_offset)
    self.assertFalse(requests[0].finish_write)
    self.assertEqual('34', requests[1].data)
    self.assertEqual(2, requests[1].write_offset)
    self.assertTrue(requests[1].finish_write)

  def testPushHappyMultiBig(self):
    """Push: sends multiple chunks, each of which have to be split"""
    self.mock(isolate_storage, 'NET_IO_FILE_CHUNK', 2)
    s = self.get_server()
    i = isolate_storage.Item(digest='abc123', size=6)
    s.push(i, isolate_storage._IsolateServerGrpcPushState(), ['123', '456'])
    requests = s._proxy._stub.popPushRequests()
    self.assertEqual(4, len(requests))
    m = re.search('client/bob/uploads/.*/blobs/abc123/6',
                  requests[0].resource_name)
    self.assertTrue(m)
    self.assertEqual(0, requests[0].write_offset)
    self.assertEqual('12', requests[0].data)
    self.assertFalse(requests[0].finish_write)
    self.assertEqual(2, requests[1].write_offset)
    self.assertEqual('3', requests[1].data)
    self.assertFalse(requests[1].finish_write)
    self.assertEqual(3, requests[2].write_offset)
    self.assertEqual('45', requests[2].data)
    self.assertFalse(requests[2].finish_write)
    self.assertEqual(5, requests[3].write_offset)
    self.assertEqual('6', requests[3].data)
    self.assertTrue(requests[3].finish_write)

  def testPushHappyZeroLengthBlob(self):
    """Push: send a zero-length blob"""
    s = self.get_server()
    i = isolate_storage.Item(digest='abc123', size=0)
    s.push(i, isolate_storage._IsolateServerGrpcPushState(), '')
    requests = s._proxy._stub.popPushRequests()
    self.assertEqual(1, len(requests))
    m = re.search('client/bob/uploads/.*/blobs/abc123/0',
                  requests[0].resource_name)
    self.assertTrue(m)
    self.assertEqual(0, requests[0].write_offset)
    self.assertEqual('', requests[0].data)
    self.assertTrue(requests[0].finish_write)

  def testPushThrowsOnFailure(self):
    """Push: if something goes wrong in Isolate, we throw an exception"""
    def Write(self, request, timeout=None):
      del request, timeout, self
      raiseError(isolate_storage.grpc.StatusCode.INTERNAL_ERROR)
    self.mock(ByteStreamStubMock, 'Write', Write)

    s = self.get_server()
    i = isolate_storage.Item(digest='abc123', size=0)
    with self.assertRaises(IOError):
      s.push(i, isolate_storage._IsolateServerGrpcPushState(), '1234')

  def testPushThrowsCorrectExceptionOnGrpcFailure(self):
    """Push: if something goes wrong in Isolate, we throw an exception"""
    def Write(_self, _request, timeout=None):
      del timeout
      raiseError(isolate_storage.grpc.StatusCode.INTERNAL_ERROR)
    self.mock(ByteStreamStubMock, 'Write', Write)

    s = self.get_server()
    i = isolate_storage.Item(digest='abc123', size=0)
    with self.assertRaises(IOError):
      s.push(i, isolate_storage._IsolateServerGrpcPushState(), '1234')

  def testPushRetriesOnGrpcFailure(self):
    """Push: retry will succeed if the Isolate failure is transient."""
    class IsFirstWrapper:
      is_first = True

    def Write(self, requests, timeout=None):
      del timeout
      if IsFirstWrapper.is_first:
        IsFirstWrapper.is_first = False
        raiseError(isolate_storage.grpc.StatusCode.INTERNAL_ERROR)
      else:
        nb = 0
        for r in requests:
          nb += len(r.data)
          self._push_requests.append(r.__deepcopy__())
        resp = isolate_storage.bytestream_pb2.WriteResponse()
        resp.committed_size = nb
        return resp

    self.mock(ByteStreamStubMock, 'Write', Write)

    s = self.get_server()
    i = isolate_storage.Item(digest='abc123', size=4)
    self.mock(isolate_storage.Item, 'content',
              lambda _: [(yield x) for x in ['12', '34']])
    with self.assertRaises(IOError):
      s.push(i, isolate_storage._IsolateServerGrpcPushState())

    # The retry should succeed.
    s.push(i, isolate_storage._IsolateServerGrpcPushState())
    requests = s._proxy._stub.popPushRequests()
    self.assertEqual(2, len(requests))
    m = re.search('client/bob/uploads/.*/blobs/abc123/4',
                  requests[0].resource_name)
    self.assertTrue(m)
    self.assertEqual('12', requests[0].data)
    self.assertEqual(0, requests[0].write_offset)
    self.assertFalse(requests[0].finish_write)
    self.assertEqual('34', requests[1].data)
    self.assertEqual(2, requests[1].write_offset)
    self.assertTrue(requests[1].finish_write)


if __name__ == '__main__':
  if not isolate_storage.grpc:
    # Don't print to stderr or return error code as this will
    # show up as a warning and fail in presubmit.
    print('gRPC could not be loaded; skipping tests')
    sys.exit(0)
  isolate_storage.bytestream_pb2.ByteStreamStub = ByteStreamStubMock
  test_utils.main()
