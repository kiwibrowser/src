#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import unittest

from future import Future
from reference_resolver import ReferenceResolver
from test_object_store import TestObjectStore
from test_util import Server2Path
from third_party.json_schema_compiler.model import Namespace


_TEST_DATA = {
  'baz': {
    'namespace': 'baz',
    'description': '',
    'types': [
      {
        'id': 'baz_t1',
        'type': 'any',
      },
      {
        'id': 'baz_t2',
        'type': 'any',
      },
      {
        'id': 'baz_t3',
        'type': 'any',
      }
    ],
    'functions': [
      {
        'name': 'baz_f1',
        'type': 'function'
      },
      {
        'name': 'baz_f2',
        'type': 'function'
      },
      {
        'name': 'baz_f3',
        'type': 'function'
      }
    ],
    'events': [
      {
        'name': 'baz_e1',
        'type': 'function'
      },
      {
        'name': 'baz_e2',
        'type': 'function'
      },
      {
        'name': 'baz_e3',
        'type': 'function'
      }
    ],
    'properties': {
      'baz_p1': {'type': 'any'},
      'baz_p2': {'type': 'any'},
      'baz_p3': {'type': 'any'}
    }
  },
  'bar.bon': {
    'namespace': 'bar.bon',
    'description': '',
    'types': [
      {
        'id': 'bar_bon_t1',
        'type': 'any',
      },
      {
        'id': 'bar_bon_t2',
        'type': 'any',
      },
      {
        'id': 'bar_bon_t3',
        'type': 'any',
      }
    ],
    'functions': [
      {
        'name': 'bar_bon_f1',
        'type': 'function'
      },
      {
        'name': 'bar_bon_f2',
        'type': 'function'
      },
      {
        'name': 'bar_bon_f3',
        'type': 'function'
      }
    ],
    'events': [
      {
        'name': 'bar_bon_e1',
        'type': 'function'
      },
      {
        'name': 'bar_bon_e2',
        'type': 'function'
      },
      {
        'name': 'bar_bon_e3',
        'type': 'function'
      }
    ],
    'properties': {
      'bar_bon_p1': {'type': 'any'},
      'bar_bon_p2': {'type': 'any'},
      'bar_bon_p3': {'type': 'any'}
    }
  },
  'bar': {
    'namespace': 'bar',
    'description': '',
    'types': [
      {
        'id': 'bar_t1',
        'type': 'any',
        'properties': {
          'bar_t1_p1': {
            'type': 'any'
          }
        }
      },
      {
        'id': 'bar_t2',
        'type': 'any',
        'properties': {
          'bar_t2_p1': {
            'type': 'any'
          }
        }
      },
      {
        'id': 'bar_t3',
        'type': 'any',
      },
      {
        'id': 'bon',
        'type': 'any'
      }
    ],
    'functions': [
      {
        'name': 'bar_f1',
        'type': 'function'
      },
      {
        'name': 'bar_f2',
        'type': 'function'
      },
      {
        'name': 'bar_f3',
        'type': 'function'
      }
    ],
    'events': [
      {
        'name': 'bar_e1',
        'type': 'function'
      },
      {
        'name': 'bar_e2',
        'type': 'function'
      },
      {
        'name': 'bar_e3',
        'type': 'function'
      }
    ],
    'properties': {
      'bar_p1': {'type': 'any'},
      'bar_p2': {'type': 'any'},
      'bar_p3': {'$ref': 'bar_t1'}
    }
  },
  'foo': {
    'namespace': 'foo',
    'description': '',
    'types': [
      {
        'id': 'foo_t1',
        'type': 'any',
      },
      {
        'id': 'foo_t2',
        'type': 'any',
      },
      {
        'id': 'foo_t3',
        'type': 'any',
        'events': [
          {
            'name': 'foo_t3_e1',
            'type': 'function'
          }
        ]
      }
    ],
    'functions': [
      {
        'name': 'foo_f1',
        'type': 'function'
      },
      {
        'name': 'foo_f2',
        'type': 'function'
      },
      {
        'name': 'foo_f3',
        'type': 'function'
      }
    ],
    'events': [
      {
        'name': 'foo_e1',
        'type': 'function'
      },
      {
        'name': 'foo_e2',
        'type': 'function'
      },
      {
        'name': 'foo_e3',
        'type': 'function'
      }
    ],
    'properties': {
      'foo_p1': {'$ref': 'foo_t3'},
      'foo_p2': {'type': 'any'},
      'foo_p3': {'type': 'any'}
    }
  }
}


class _FakePlatformBundle(object):
  def __init__(self):
    self.platforms = ('apps', 'extensions')

  def GetAPIModels(self, platform):
    if platform == 'apps':
      return _FakeAPIModels(_TEST_DATA)
    # Only includes some of the data in the 'extensions' APIModels.
    # ReferenceResolver will have to look at other platforms to resolve 'foo'.
    return _FakeAPIModels({
      'bar': _TEST_DATA['bar'],
      'bar.bon': _TEST_DATA['bar.bon'],
      'baz': _TEST_DATA['baz']
    })


class _FakeAPIModels(object):
  def __init__(self, apis):
    self._apis = apis

  def GetNames(self):
    return self._apis.keys()

  def GetModel(self, name):
    return Future(value=Namespace(self._apis[name], 'fake/path.json'))


class ReferenceResolverTest(unittest.TestCase):
  def setUp(self):
    self._base_path = Server2Path('test_data', 'test_json')

  def _ReadLocalFile(self, filename):
    with open(os.path.join(self._base_path, filename), 'r') as f:
      return f.read()

  def testGetLink(self):
    apps_resolver = ReferenceResolver(
        _FakePlatformBundle().GetAPIModels('apps'),
        TestObjectStore('apps/test'))
    extensions_resolver = ReferenceResolver(
        _FakePlatformBundle().GetAPIModels('extensions'),
        TestObjectStore('extensions/test'))

    self.assertEqual({
      'href': 'foo',
      'text': 'foo',
      'name': 'foo'
    }, apps_resolver.GetLink('foo', namespace='baz'))
    self.assertEqual({
      'href': 'foo#type-foo_t1',
      'text': 'foo.foo_t1',
      'name': 'foo_t1'
    }, apps_resolver.GetLink('foo.foo_t1', namespace='baz'))
    self.assertEqual({
      'href': 'baz#event-baz_e1',
      'text': 'baz_e1',
      'name': 'baz_e1'
    }, apps_resolver.GetLink('baz.baz_e1', namespace='baz'))
    self.assertEqual({
      'href': 'baz#event-baz_e1',
      'text': 'baz_e1',
      'name': 'baz_e1'
    }, apps_resolver.GetLink('baz_e1', namespace='baz'))
    self.assertEqual({
      'href': 'foo#method-foo_f1',
      'text': 'foo.foo_f1',
      'name': 'foo_f1'
    }, apps_resolver.GetLink('foo.foo_f1', namespace='baz'))
    self.assertEqual({
      'href': 'foo#property-foo_p3',
      'text': 'foo.foo_p3',
      'name': 'foo_p3'
    }, apps_resolver.GetLink('foo.foo_p3', namespace='baz'))
    self.assertEqual({
      'href': 'bar.bon#type-bar_bon_t3',
      'text': 'bar.bon.bar_bon_t3',
      'name': 'bar_bon_t3'
    }, apps_resolver.GetLink('bar.bon.bar_bon_t3', namespace='baz'))
    self.assertEqual({
      'href': 'bar.bon#property-bar_bon_p3',
      'text': 'bar_bon_p3',
      'name': 'bar_bon_p3'
    }, apps_resolver.GetLink('bar_bon_p3', namespace='bar.bon'))
    self.assertEqual({
      'href': 'bar.bon#property-bar_bon_p3',
      'text': 'bar_bon_p3',
      'name': 'bar_bon_p3'
    }, apps_resolver.GetLink('bar.bon.bar_bon_p3', namespace='bar.bon'))
    self.assertEqual({
      'href': 'bar#event-bar_e2',
      'text': 'bar_e2',
      'name': 'bar_e2'
    }, apps_resolver.GetLink('bar.bar_e2', namespace='bar'))
    self.assertEqual({
      'href': 'bar#type-bon',
      'text': 'bon',
      'name': 'bon'
    }, apps_resolver.GetLink('bar.bon', namespace='bar'))
    self.assertEqual({
      'href': 'foo#event-foo_t3-foo_t3_e1',
      'text': 'foo_t3.foo_t3_e1',
      'name': 'foo_t3_e1'
    }, apps_resolver.GetLink('foo_t3.foo_t3_e1', namespace='foo'))
    self.assertEqual({
      'href': 'foo#event-foo_t3-foo_t3_e1',
      'text': 'foo_t3.foo_t3_e1',
      'name': 'foo_t3_e1'
    }, apps_resolver.GetLink('foo.foo_t3.foo_t3_e1', namespace='foo'))
    self.assertEqual({
      'href': 'foo#event-foo_t3-foo_t3_e1',
      'text': 'foo_t3.foo_t3_e1',
      'name': 'foo_t3_e1'
    }, apps_resolver.GetLink('foo.foo_p1.foo_t3_e1', namespace='foo'))
    self.assertEqual({
      'href': 'bar#property-bar_t1-bar_t1_p1',
      'text': 'bar.bar_t1.bar_t1_p1',
      'name': 'bar_t1_p1'
    }, apps_resolver.GetLink('bar.bar_p3.bar_t1_p1', namespace='foo'))
    # Test extensions_resolver.
    self.assertEqual({
      'href': 'bar#property-bar_t1-bar_t1_p1',
      'text': 'bar.bar_t1.bar_t1_p1',
      'name': 'bar_t1_p1'
    }, extensions_resolver.GetLink('bar.bar_p3.bar_t1_p1', namespace='foo'))
    self.assertEqual({
      'href': 'bar#property-bar_t1-bar_t1_p1',
      'text': 'bar_t1.bar_t1_p1',
      'name': 'bar_t1_p1'
    }, apps_resolver.GetLink('bar_p3.bar_t1_p1', namespace='bar'))
    self.assertEqual(
        None,
        apps_resolver.GetLink('bar.bar_p3.bar_t2_p1', namespace='bar'))
    self.assertEqual(
        None,
        apps_resolver.GetLink('bar.bon.bar_e3', namespace='bar'))
    self.assertEqual(
        None,
        apps_resolver.GetLink('bar_p3', namespace='baz.bon'))
    self.assertEqual(
        None,
        apps_resolver.GetLink('falafel.faf', namespace='a'))
    self.assertEqual(
        None,
        apps_resolver.GetLink('bar_p3', namespace='foo'))
    # Exists in apps but not extensions.
    self.assertEqual(
        None,
        extensions_resolver.GetLink('foo.foo_p3', namespace='baz'))

if __name__ == '__main__':
  unittest.main()
