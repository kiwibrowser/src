#!/usr/bin/env python
# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import json
import os
import sys
import unittest
import StringIO

ROOT_DIR = os.path.dirname(os.path.abspath(os.path.join(
    __file__.decode(sys.getfilesystemencoding()),
    os.pardir, os.pardir, os.pardir)))
sys.path.insert(0, ROOT_DIR)

from libs.logdog import stream, streamname, varint


class StreamParamsTestCase(unittest.TestCase):

  def setUp(self):
    self.params = stream.StreamParams(
        'name',
        type=stream.StreamParams.TEXT,
        content_type='content-type',
        tags={
            'foo': 'bar',
            'baz': 'qux',
        },
        tee=stream.StreamParams.TEE_STDOUT,
        binary_file_extension='ext')

  def testParamsToJson(self):
    self.assertEqual(self.params.to_json(),
        ('{"binaryFileExtension": "ext", "contentType": "content-type", '
         '"name": "name", "tags": {"baz": "qux", "foo": "bar"}, '
         '"tee": "stdout", "type": "text"}'))

  def testParamsToJsonWithEmpties(self):
    params = self.params._replace(
        content_type=None,
        tags=None,
        tee=None,
        binary_file_extension=None,
    )
    self.assertEqual(params.to_json(), '{"name": "name", "type": "text"}')

  def testParamsWithInvalidTypeRaisesValueError(self):
    params = self.params._replace(type=None)
    self.assertRaises(ValueError, params.to_json)

  def testParamsWithInvalidTeeTypeRaisesValueError(self):
    params = self.params._replace(tee='somewhere')
    self.assertRaises(ValueError, params.to_json)

  def testParamsWithInvalidTagRaisesValueError(self):
    params = self.params._replace(tags='foo')
    self.assertRaises(ValueError, params.to_json)

    params = self.params._replace(tags={'!!! invalid tag key !!!': 'bar'})
    self.assertRaises(ValueError, params.to_json)


class StreamClientTestCase(unittest.TestCase):

  class _TestStreamClientConnection(object):

    def __init__(self):
      self.buffer = StringIO.StringIO()
      self.closed = False

    def _assert_not_closed(self):
      if self.closed:
        raise Exception('Connection is closed.')

    def write(self, v):
      self._assert_not_closed()
      self.buffer.write(v)

    def close(self):
      self._assert_not_closed()
      self.closed = True

    def interpret(self):
      data = StringIO.StringIO(self.buffer.getvalue())
      magic = data.read(len(stream.BUTLER_MAGIC))
      if magic != stream.BUTLER_MAGIC:
        raise ValueError('Invalid magic value ([%s] != [%s])' % (
            magic, stream.BUTLER_MAGIC))
      length, _ = varint.read_uvarint(data)
      header = data.read(length)
      return json.loads(header), data.read()

  class _TestStreamClient(stream.StreamClient):
    def __init__(self, value, **kwargs):
      super(StreamClientTestCase._TestStreamClient, self).__init__(**kwargs)
      self.value = value
      self.last_conn = None

    @classmethod
    def _create(cls, value, **kwargs):
      return cls(value, **kwargs)

    def _connect_raw(self):
      conn = StreamClientTestCase._TestStreamClientConnection()
      self.last_conn = conn
      return conn

  def setUp(self):
    self._registry = stream.StreamProtocolRegistry()
    self._registry.register_protocol('test', self._TestStreamClient)

  @staticmethod
  def _split_datagrams(value):
    sio = StringIO.StringIO(value)
    while sio.pos < sio.len:
      size_prefix, _ = varint.read_uvarint(sio)
      data = sio.read(size_prefix)
      if len(data) != size_prefix:
        raise ValueError('Expected %d bytes, but only got %d' % (
            size_prefix, len(data)))
      yield data

  def testClientInstantiation(self):
    client = self._registry.create('test:value')
    self.assertIsInstance(client, self._TestStreamClient)
    self.assertEqual(client.value, 'value')

  def testTextStream(self):
    client = self._registry.create('test:value',
                                   project='test',
                                   prefix='foo/bar',
                                   coordinator_host='example.appspot.com')
    with client.text('mystream') as fd:
      self.assertEqual(
          fd.path,
          streamname.StreamPath(prefix='foo/bar', name='mystream'))
      self.assertEqual(
          fd.get_viewer_url(),
          'https://example.appspot.com/v/?s=test%2Ffoo%2Fbar%2F%2B%2Fmystream')
      fd.write('text\nstream\nlines')

    conn = client.last_conn
    self.assertTrue(conn.closed)

    header, data = conn.interpret()
    self.assertEqual(header, {'name': 'mystream', 'type': 'text'})
    self.assertEqual(data, 'text\nstream\nlines')

  def testTextStreamWithParams(self):
    client = self._registry.create('test:value')
    with client.text('mystream', content_type='foo/bar',
                     tee=stream.StreamParams.TEE_STDOUT,
                     tags={'foo': 'bar', 'baz': 'qux'}) as fd:
      self.assertEqual(
          fd.params,
          stream.StreamParams.make(
              name='mystream',
              type=stream.StreamParams.TEXT,
              content_type='foo/bar',
              tee=stream.StreamParams.TEE_STDOUT,
              tags={'foo': 'bar', 'baz': 'qux'}))
      fd.write('text!')

    conn = client.last_conn
    self.assertTrue(conn.closed)

    header, data = conn.interpret()
    self.assertEqual(header, {
        'name': 'mystream',
        'type': 'text',
        'contentType': 'foo/bar',
         'tee': 'stdout',
         'tags': {'foo': 'bar', 'baz': 'qux'},
    })
    self.assertEqual(data, 'text!')

  def testBinaryStream(self):
    client = self._registry.create('test:value',
                                   project='test',
                                   prefix='foo/bar',
                                   coordinator_host='example.appspot.com')
    with client.binary('mystream') as fd:
      self.assertEqual(
          fd.path,
          streamname.StreamPath(prefix='foo/bar', name='mystream'))
      self.assertEqual(
          fd.get_viewer_url(),
          'https://example.appspot.com/v/?s=test%2Ffoo%2Fbar%2F%2B%2Fmystream')
      fd.write('\x60\x0d\xd0\x65')

    conn = client.last_conn
    self.assertTrue(conn.closed)

    header, data = conn.interpret()
    self.assertEqual(header, {'name': 'mystream', 'type': 'binary'})
    self.assertEqual(data, '\x60\x0d\xd0\x65')

  def testDatagramStream(self):
    client = self._registry.create('test:value',
                                   project='test',
                                   prefix='foo/bar',
                                   coordinator_host='example.appspot.com')
    with client.datagram('mystream') as fd:
      self.assertEqual(
          fd.path,
          streamname.StreamPath(prefix='foo/bar', name='mystream'))
      self.assertEqual(
          fd.get_viewer_url(),
          'https://example.appspot.com/v/?s=test%2Ffoo%2Fbar%2F%2B%2Fmystream')
      fd.send('datagram0')
      fd.send('dg1')
      fd.send('')
      fd.send('dg3')

    conn = client.last_conn
    self.assertTrue(conn.closed)

    header, data = conn.interpret()
    self.assertEqual(header, {'name': 'mystream', 'type': 'datagram'})
    self.assertEqual(list(self._split_datagrams(data)),
        ['datagram0', 'dg1', '', 'dg3'])

  def testStreamWithoutPrefixCannotGenerateUrls(self):
    client = self._registry.create('test:value',
                                   coordinator_host='example.appspot.com')
    with client.text('mystream') as fd:
      self.assertRaises(KeyError, fd.get_viewer_url)

  def testStreamWithoutInvalidPrefixCannotGenerateUrls(self):
    client = self._registry.create('test:value',
                                   project='test',
                                   prefix='!!! invalid !!!',
                                   coordinator_host='example.appspot.com')
    with client.text('mystream') as fd:
      self.assertRaises(ValueError, fd.get_viewer_url)

  def testStreamWithoutProjectCannotGenerateUrls(self):
    client = self._registry.create('test:value',
                                   prefix='foo/bar',
                                   coordinator_host='example.appspot.com')
    with client.text('mystream') as fd:
      self.assertRaises(KeyError, fd.get_viewer_url)

  def testStreamWithoutCoordinatorHostCannotGenerateUrls(self):
    client = self._registry.create('test:value',
                                   project='test',
                                   prefix='foo/bar')
    with client.text('mystream') as fd:
      self.assertRaises(KeyError, fd.get_viewer_url)


  def testCreatingDuplicateStreamNameRaisesValueError(self):
    client = self._registry.create('test:value')
    with client.text('mystream') as fd:
      fd.write('Using a text stream.')

    with self.assertRaises(ValueError):
      with client.text('mystream') as fd:
        fd.write('Should not work.')

    conn = client.last_conn
    self.assertTrue(conn.closed)

    header, data = conn.interpret()
    self.assertEqual(header, {'name': 'mystream', 'type': 'text'})
    self.assertEqual(data, 'Using a text stream.')


if __name__ == '__main__':
  unittest.main()
