#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from api_schema_graph import APISchemaGraph, LookupResult


API_SCHEMA = [{
  'namespace': 'tabs',
  'properties': {
    'lowercase': {
      'properties': {
        'one': { 'value': 1 },
        'two': { 'description': 'just as bad as one' }
      }
    },
    'TAB_PROPERTY_ONE': { 'value': 'magic' },
    'TAB_PROPERTY_TWO': {}
  },
  'types': [
    {
      'id': 'Tab',
      'properties': {
        'id': {},
        'url': {}
      }
    }
  ],
  'functions': [
    {
      'name': 'get',
      'parameters': [ { 'name': 'tab',
                        'type': 'object',
                        'description': 'gets stuff, never complains'
                      },
                      { 'name': 'tabId' }
                    ]
    }
  ],
  'events': [
    {
      'name': 'onActivated',
      'parameters': [ {'name': 'activeInfo'} ]
    },
    {
      'name': 'onUpdated',
      'parameters': [ {'name': 'updateInfo'} ]
    }
  ]
}]


class APISchemaGraphTest(unittest.TestCase):

  def testLookup(self):
    self._testAPISchema(APISchemaGraph(API_SCHEMA))

  def testIsEmpty(self):
    # A few assertions to make sure that Lookup works on empty sets.
    empty_graph = APISchemaGraph({})
    self.assertTrue(empty_graph.IsEmpty())
    self.assertEqual(LookupResult(False, None),
                     empty_graph.Lookup('tabs', 'properties',
                                        'TAB_PROPERTY_ONE'))
    self.assertEqual(LookupResult(False, None),
                     empty_graph.Lookup('tabs', 'functions', 'get',
                                        'parameters', 'tab'))
    self.assertEqual(LookupResult(False, None),
                     empty_graph.Lookup('tabs', 'functions', 'get',
                                        'parameters', 'tabId'))
    self.assertEqual(LookupResult(False, None),
                     empty_graph.Lookup('tabs', 'events', 'onActivated',
                                        'parameters', 'activeInfo'))
    self.assertEqual(LookupResult(False, None),
                     empty_graph.Lookup('tabs', 'events', 'onUpdated',
                                        'parameters', 'updateInfo'))

  def testSubtractEmpty(self):
    self._testAPISchema(
        APISchemaGraph(API_SCHEMA).Subtract(APISchemaGraph({})))

  def _testAPISchema(self, api_schema_graph):
    self.assertEqual(LookupResult(True, None),
                     api_schema_graph.Lookup('tabs', 'properties',
                                             'TAB_PROPERTY_ONE'))
    self.assertEqual(LookupResult(True, None),
                     api_schema_graph.Lookup('tabs', 'types', 'Tab'))
    self.assertEqual(LookupResult(True, None),
                    api_schema_graph.Lookup('tabs', 'functions', 'get',
                                            'parameters', 'tab'))
    self.assertEqual(LookupResult(True, None),
                    api_schema_graph.Lookup('tabs', 'functions', 'get',
                                            'parameters', 'tabId'))
    self.assertEqual(LookupResult(True, None),
                    api_schema_graph.Lookup('tabs', 'functions', 'get',
                                            'parameters', 'tab', 'type'))
    self.assertEqual(LookupResult(True, None),
                    api_schema_graph.Lookup('tabs', 'events',
                                            'onActivated', 'parameters',
                                            'activeInfo'))
    self.assertEqual(LookupResult(True, None),
                    api_schema_graph.Lookup('tabs', 'events', 'onUpdated',
                                            'parameters', 'updateInfo'))
    self.assertEqual(LookupResult(True, None),
                    api_schema_graph.Lookup('tabs', 'properties',
                                            'lowercase', 'properties',
                                            'one', 'value'))
    self.assertEqual(LookupResult(True, None),
                    api_schema_graph.Lookup('tabs', 'properties',
                                            'lowercase', 'properties',
                                            'two', 'description'))
    self.assertEqual(LookupResult(False, None),
                     api_schema_graph.Lookup('windows'))
    self.assertEqual(LookupResult(False, None),
                     api_schema_graph.Lookup('tabs', 'properties',
                                             'TAB_PROPERTY_DEUX'))
    self.assertEqual(LookupResult(False, None),
                     api_schema_graph.Lookup('tabs', 'events', 'onActivated',
                                             'parameters', 'callback'))
    self.assertEqual(LookupResult(False, None),
                     api_schema_graph.Lookup('tabs', 'functions', 'getById',
                                             'parameters', 'tab'))
    self.assertEqual(LookupResult(False, None),
                     api_schema_graph.Lookup('tabs', 'functions', 'get',
                                             'parameters', 'type'))
    self.assertEqual(LookupResult(False, None),
                     api_schema_graph.Lookup('tabs', 'properties',
                                             'lowercase', 'properties',
                                             'two', 'value'))

  def testSubtractSelf(self):
    self.assertTrue(
        APISchemaGraph(API_SCHEMA).Subtract(APISchemaGraph(API_SCHEMA))
            .IsEmpty())


  def testSubtractDisjointSet(self):
    difference = APISchemaGraph(API_SCHEMA).Subtract(APISchemaGraph({
      'contextMenus': {
        'properties': {
          'CONTEXT_MENU_PROPERTY_ONE': {}
        },
        'types': {
          'Menu': {
            'properties': {
              'id': {},
              'width': {}
            }
          }
        },
        'functions': {
          'get': {
            'parameters': {
              'callback': {}
            }
          }
        },
        'events': {
          'onClicked': {
            'parameters': {
              'clickInfo': {}
            }
          },
          'onUpdated': {
            'parameters': {
              'updateInfo': {}
            }
          }
        }
      }
    }))
    self.assertEqual(LookupResult(True, None),
                     difference.Lookup('tabs', 'properties',
                                      'TAB_PROPERTY_ONE'))
    self.assertEqual(LookupResult(True, None),
                    difference.Lookup('tabs', 'functions', 'get',
                                      'parameters', 'tab'))
    self.assertEqual(LookupResult(True, None),
                     difference.Lookup('tabs', 'events', 'onUpdated',
                                      'parameters', 'updateInfo'))
    self.assertEqual(LookupResult(True, None),
                     difference.Lookup('tabs', 'functions', 'get',
                                      'parameters', 'tabId'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('contextMenus', 'properties',
                                       'CONTEXT_MENU_PROPERTY_ONE'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('contextMenus', 'types', 'Menu'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('contextMenus', 'types', 'Menu',
                                       'properties', 'id'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('contextMenus', 'functions'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('contextMenus', 'events', 'onClicked',
                                       'parameters', 'clickInfo'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('contextMenus', 'events', 'onUpdated',
                                       'parameters', 'updateInfo'))

  def testSubtractSubset(self):
    difference = APISchemaGraph(API_SCHEMA).Subtract(APISchemaGraph({
      'tabs': {
        'properties': {
          'TAB_PROPERTY_ONE': { 'value': {} }
        },
        'functions': {
          'get': {
            'parameters': {
              'tab': { 'name': {},
                       'type': {},
                       'description': {}
                     }
            }
          }
        },
        'events': {
          'onUpdated': {
            'parameters': {
              'updateInfo': {
                'name': {},
                'properties': {
                  'tabId': {}
                }
              }
            }
          }
        }
      }
    }))
    self.assertEqual(LookupResult(True, None),
                     difference.Lookup('tabs'))
    self.assertEqual(LookupResult(True, None),
                     difference.Lookup('tabs', 'properties',
                                       'TAB_PROPERTY_TWO'))
    self.assertEqual(LookupResult(True, None),
                     difference.Lookup('tabs', 'properties', 'lowercase',
                                       'properties', 'two', 'description'))
    self.assertEqual(LookupResult(True, None),
                     difference.Lookup('tabs', 'types', 'Tab', 'properties',
                                       'url'))
    self.assertEqual(LookupResult(True, None),
                     difference.Lookup('tabs', 'events', 'onActivated',
                                       'parameters', 'activeInfo'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('tabs', 'events', 'onUpdated',
                                       'parameters', 'updateInfo'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('tabs', 'properties',
                                       'TAB_PROPERTY_ONE'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('tabs', 'properties',
                                       'TAB_PROPERTY_ONE', 'value'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('tabs', 'functions', 'get',
                                       'parameters', 'tab'))

  def testSubtractSuperset(self):
    difference = APISchemaGraph(API_SCHEMA).Subtract(APISchemaGraph({
      'tabs': {
        'namespace': {},
        'properties': {
          'lowercase': {
            'properties': {
              'one': { 'value': {} },
              'two': { 'description': {} }
            }
          },
          'TAB_PROPERTY_ONE': { 'value': {} },
          'TAB_PROPERTY_TWO': {},
          'TAB_PROPERTY_THREE': {}
        },
        'types': {
          'Tab': {
            'id': {},
            'properties': {
              'id': {},
              'url': {}
            }
          },
          'UpdateInfo': {}
        },
        'functions': {
          'get': {
            'name': {},
            'parameters': {
              'tab': { 'name': {},
                       'type': {},
                       'description': {}
                     },
              'tabId': { 'name': {} }
            }
          },
          'getById': {
            'parameters': {
              'tabId': {}
            }
          }
        },
        'events': {
          'onActivated': {
            'name': {},
            'parameters': {
              'activeInfo': { 'name': {} }
            }
          },
          'onUpdated': {
            'name': {},
            'parameters': {
              'updateInfo': { 'name': {} }
            }
          },
          'onClicked': {
            'name': {},
            'parameters': {
              'clickInfo': {}
            }
          }
        }
      }
    }))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('tabs'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('tabs', 'properties',
                                       'TAB_PROPERTY_TWO'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('tabs', 'properties'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('tabs', 'types', 'Tab', 'properties',
                                       'url'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('tabs', 'types', 'Tab', 'properties',
                                       'id'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('tabs', 'events', 'onUpdated',
                                       'parameters', 'updateInfo'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('tabs', 'properties',
                                       'TAB_PROPERTY_ONE'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('tabs', 'functions', 'get',
                                       'parameters', 'tabId'))
    self.assertEqual(LookupResult(False, None),
                     difference.Lookup('events', 'onUpdated', 'parameters',
                                       'updateInfo'))

  def testUpdate(self):
    result = APISchemaGraph(API_SCHEMA)
    to_add = APISchemaGraph({
      'tabs': {
        'properties': {
          'TAB_PROPERTY_THREE': { 'description': 'better than two' },
          'TAB_PROPERTY_FOUR': { 'value': 4 }
        },
        'functions': {
        'get': {
          'name': {},
          'parameters': {
            'tab': {
              'type': {},
              'name': {},
              'description': {},
              'surprise': {}
            }
          }
        },
          'getAllInWindow': {
            'parameters': {
              'windowId': { 'type': 'object' }
            }
          }
        }
      }
    })
    result.Update(to_add, lambda _: 'first')
    # Looking up elements that were originally available in |result|. Because
    # of this, no |annotation| object should be attached to the LookupResult
    # object.
    self.assertEqual(LookupResult(True, None),
                     result.Lookup('tabs'))
    self.assertEqual(LookupResult(True, None),
                     result.Lookup('tabs', 'functions', 'get',
                                   'parameters'))
    self.assertEqual(LookupResult(True, None),
                     result.Lookup('tabs', 'properties',
                                   'TAB_PROPERTY_ONE'))
    self.assertEqual(LookupResult(True, None),
                     result.Lookup('tabs', 'properties',
                                   'TAB_PROPERTY_ONE'))
    self.assertEqual(LookupResult(True, None),
                     result.Lookup('tabs', 'functions', 'get',
                                   'parameters', 'tabId'))

    # Looking up elements that were just added to |result|.
    self.assertEqual(LookupResult(True, 'first'),
                     result.Lookup('tabs', 'properties',
                                   'TAB_PROPERTY_THREE'))
    self.assertEqual(LookupResult(True, 'first'),
                     result.Lookup('tabs', 'properties',
                                   'TAB_PROPERTY_FOUR'))
    self.assertEqual(LookupResult(True, 'first'),
                     result.Lookup('tabs', 'functions', 'getAllInWindow',
                                   'parameters', 'windowId'))
    self.assertEqual(LookupResult(True, 'first'),
                     result.Lookup('tabs', 'functions', 'get', 'parameters',
                                   'tab', 'surprise'))

    to_add = APISchemaGraph({
      'tabs': {
        'properties': {
          'TAB_PROPERTY_FIVE': { 'description': 'stayin\' alive' }
        },
        'functions': {
          'getAllInWindow': {
            'parameters': {
              'callback': { 'type': 'function' }
            }
          }
        }
      }
    })
    result.Update(to_add, lambda _: 'second')
    # Looking up the second group of elements added to |result|.
    self.assertEqual(LookupResult(True, 'first'),
                     result.Lookup('tabs', 'properties',
                                   'TAB_PROPERTY_FOUR'))
    self.assertEqual(LookupResult(True, 'second'),
                     result.Lookup('tabs', 'properties',
                                   'TAB_PROPERTY_FIVE'))
    self.assertEqual(LookupResult(True, 'first'),
                     result.Lookup('tabs', 'functions',
                                   'getAllInWindow', 'parameters',
                                   'windowId'))
    self.assertEqual(LookupResult(True, 'second'),
                     result.Lookup('tabs', 'functions',
                                   'getAllInWindow', 'parameters',
                                   'callback'))
    self.assertEqual(LookupResult(True, 'first'),
                     result.Lookup('tabs', 'functions',
                                   'getAllInWindow'))


if __name__ == '__main__':
  unittest.main()
