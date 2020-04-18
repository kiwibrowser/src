# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json

from extensions_paths import CHROME_EXTENSIONS
from third_party.json_schema_compiler.json_parse import OrderedDict
from test_file_system import MoveAllTo, MoveTo


CANNED_CHANNELS = OrderedDict([
  ('master', 'master'),
  ('dev', 31),
  ('beta', 30),
  ('stable', 29)
])


CANNED_BRANCHES = OrderedDict([
  ('master', 'master'),
  (31, '1612'),
  (30, '1599'),
  (29, '1547'),
  (28, '1500'),
  (27, '1453'),
  (26, '1410'),
  (25, '1364'),
  (24, '1312'),
  (23, '1271'),
  (22, '1229'),
  (21, '1180'),
  (20, '1132'),
  (19, '1084'),
  (18, '1025'),
  (17, '963'),
  (16, '912'),
  (15, '874'),
  (14, '835'),
  (13, '782'),
  (12, '742'),
  (11, '696'),
  (10, '648'),
  ( 9, '597'),
  ( 8, '552'),
  ( 7, '544'),
  ( 6, '495'),
  ( 5, '396'),
])


CANNED_TEST_FILE_SYSTEM_DATA = MoveTo(CHROME_EXTENSIONS, {
  'api': {
    '_api_features.json': json.dumps({
      'ref_test': { 'dependencies': ['permission:ref_test'] },
      'tester': { 'dependencies': ['permission:tester', 'manifest:tester'] }
    }),
    '_manifest_features.json': '{}',
    '_permission_features.json': '{}'
  },
  'docs': {
    'templates': {
      'articles': {
        'test_article.html':
            '<h1>hi</h1>you<h2>first</h2><h3>inner</h3><h2>second</h2>'
      },
      'intros': {
        'test_intro.html':
            'you<h2>first</h2><h3>inner</h3><h2>second</h2>'
      },
      'json': {
        'api_availabilities.json': json.dumps({
          'master_api': {
            'channel': 'master'
          },
          'dev_api': {
            'channel': 'dev'
          },
          'beta_api': {
            'channel': 'beta'
          },
          'stable_api': {
            'channel': 'stable',
            'version': 20
          }
        }),
        'intro_tables.json': json.dumps({
          'tester': {
            'Permissions': [
              {
                'class': 'override',
                'text': '"tester"'
              },
              {
                'text': 'is an API for testing things.'
              }
            ],
            'Learn More': [
              {
                'link': 'https://tester.test.com/welcome.html',
                'text': 'Welcome!'
              }
            ]
          }
        }),
        'manifest.json': '{}',
        'permissions.json': '{}'
      },
      'private': {
        'intro_tables': {
          'master_message.html': 'available on master'
        },
        'table_of_contents.html': '<table-of-contents>',
      }
    }
  }
})


_TEST_WHATS_NEW_JSON = {
  "backgroundpages.to-be-non-persistent": {
    "type": "additionsToExistingApis",
    "description": "backgrounds to be non persistent",
    "version": 22
  },
  "chromeSetting.set-regular-only-scope": {
    "type": "additionsToExistingApis",
    "description": "ChromeSetting.set now has a regular_only scope.",
    "version": 21
  },
  "manifest-v1-deprecated": {
    "type": "manifestChanges",
    "description": "Manifest version 1 was deprecated in Chrome 18",
    "version": 20
  }
}


CANNED_API_FILE_SYSTEM_DATA = MoveAllTo(CHROME_EXTENSIONS, {
  'master': {
    'api': {
      '_api_features.json': json.dumps({
        'alarm': {
          'channel': 'stable'
        },
        'app.window': {
          'channel': 'stable'
        },
        'browserAction': {
          'channel': 'stable'
        },
        'contextMenus': {
          'channel': 'stable'
        },
        'events': {
          'channel': 'stable'
        },
        'extension': {
          'channel': 'stable'
        },
        'signedInDevices': {
          'channel': 'stable'
        },
        'systemInfo.cpu': {
          'channel': 'stable'
        },
        'systemInfo.stuff': {
          'channel': 'dev'
        }
      }),
      '_manifest_features.json': json.dumps({
        'history': {
          'channel': 'beta'
        },
        'notifications': {
          'channel': 'beta'
        },
        'page_action': {
          'channel': 'stable'
        },
        'runtime': {
          'channel': 'stable'
        },
        'storage': {
          'channel': 'beta'
        },
        'sync': {
          'channel': 'master'
        },
        'web_request': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'alarms': {
          'channel': 'stable'
        },
        'bluetooth': {
          'channel': 'dev'
        },
        'bookmarks': {
          'channel': 'stable'
        },
        'cookies': {
          'channel': 'dev'
        },
        'declarativeContent': {
          'channel': 'master'
        },
        'declarativeWebRequest': [
          { 'channel': 'beta',
            'extension_types': ['extension']
          },
          { 'channel': 'stable',
            'extension_types': ['extension'],
            'whitelist': ['aaa']
          },
        ],
        'falseBetaAPI': {
          'channel': 'beta'
        },
        'systemInfo.display': {
          'channel': 'stable'
        },
        'masterAPI': {
          'channel': 'master'
        }
      }),
      'alarm.json': json.dumps([{
        'namespace': 'alarm',
        'description': '<code>alarm</code>'
      }]),
      'app_window.json': json.dumps([{
        'namespace': 'app.window',
        'description': '<code>app.window</code>'
      }]),
      'browser_action.json': json.dumps([{
        'namespace': 'browserAction',
        'description': '<code>browserAction</code>'
      }]),
      'bluetooth.idl': '\n'.join(('//Copyleft Schmopyright',
                                  '',
                                  '//An IDL description, oh my!',
                                  'namespace bluetooth {',
                                  '  dictionary Socket {',
                                  '    long id;',
                                  '  };',
                                  '};')),
      'context_menus.json': json.dumps([{
        'namespace': 'contextMenus',
        'description': ''
      }]),
      'json_stable_api.json': json.dumps([{
        'namespace': 'jsonStableAPI',
        'description': 'An API with a predetermined availability.'
      }]),
      'idle.json': json.dumps([{'namespace': 'idle', 'description': ''}]),
      'input_ime.json': json.dumps([{
        'namespace': 'input.ime',
        'description': 'An API that has the potential to cause some trouble.'
      }]),
      'menus.json': json.dumps([{'namespace': 'menus', 'description': ''}]),
      'signed_in_devices.json': json.dumps([{
        'namespace': 'signedInDevices',
        'description': 'Another API that could cause some trouble.'
      }]),
      'system_info_stuff.json': json.dumps([{
        'namespace': 'systemInfo.stuff',
        'description': 'Yet another API that could wreck havoc...'
      }]),
      'tabs.json': json.dumps([{'namespace': 'tabs', 'description': ''}]),
      'windows.json': json.dumps([{'namespace': 'windows', 'description': ''}])
    },
    'docs': {
      'templates': {
        'json': {
          'api_availabilities.json': json.dumps({
            'jsonMasterAPI': {
              'channel': 'master'
            },
            'jsonDevAPI': {
              'channel': 'dev'
            },
            'jsonBetaAPI': {
              'channel': 'beta'
            },
            'jsonStableAPI': {
              'channel': 'stable',
              'version': 20
            }
          }),
          'intro_tables.json': json.dumps({
            'test': [
              {
                'Permissions': 'probably none'
              }
            ]
          }),
          'manifest.json': '{}',
          'permissions.json': '{}',
          'whats_new.json': json.dumps(_TEST_WHATS_NEW_JSON)
        },
        'public': {
          'apps': {
            'alarm.html': 'alarm.html',
            'app_window.html': 'app_window.html',
            'contextMenus.html': 'contextMenus.html',
          },
          'extensions': {
            'alarm.html': 'alarm.html',
            'browserAction.html': 'browserAction.html',
            'contextMenus.html': 'contextMenus.html',
          }
        }
      }
    }
  },
  '1612': {
    'api': {
      '_api_features.json': json.dumps({
        'alarm': {
          'channel': 'stable'
        },
        'app.window': {
          'channel': 'stable'
        },
        'browserAction': {
          'channel': 'stable'
        },
        'events': {
          'channel': 'master'
        },
        'extension': {
          'channel': 'stable'
        },
        'systemInfo.cpu': {
          'channel': 'stable'
        },
        'systemInfo.stuff': {
          'channel': 'dev'
        }
      }),
      '_manifest_features.json': json.dumps({
        'contextMenus': {
          'channel': 'master'
        },
        'notifications': {
          'channel': 'beta'
        },
        'page_action': {
          'channel': 'stable'
        },
        'runtime': {
          'channel': 'stable'
        },
        'storage': {
          'channel': 'dev'
        },
        'sync': {
          'channel': 'master'
        },
        'system_info_display': {
          'channel': 'stable'
        },
        'web_request': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'alarms': {
          'channel': 'stable'
        },
        'appsFirst': {
          'channel': 'stable',
          'extension_types': ['extension', 'platform_app']
        },
        'bluetooth': {
          'channel': 'dev'
        },
        'bookmarks': {
          'channel': 'stable'
        },
        'cookies': {
          'channel': 'dev'
        },
        'declarativeContent': {
          'channel': 'master'
        },
        'declarativeWebRequest': [
          { 'channel': 'beta' },
          { 'channel': 'stable', 'whitelist': ['aaa'] }
        ],
        'downloads': {
          'channel': 'beta'
        }
      }),
      'alarm.json': json.dumps([{
        'namespace': 'alarm',
        'description': '<code>alarm</code>'
      }]),
      'app_window.json': json.dumps([{
        'namespace': 'app.window',
        'description': '<code>app.window</code>'
      }]),
      'browser_action.json': json.dumps([{
        'namespace': 'browserAction',
        'description': '<code>browserAction</code>'
      }]),
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input_ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
      'windows.json': json.dumps([{'namespace': 'windows'}])
    },
    'docs': {
      'templates': {
        'json': {
          'api_availabilities.json': json.dumps({
            'jsonMasterAPI': {
              'channel': 'master'
            },
            'jsonDevAPI': {
              'channel': 'dev'
            },
            'jsonBetaAPI': {
              'channel': 'beta'
            },
            'jsonStableAPI': {
              'channel': 'stable',
              'version': 20
            }
          }),
          'intro_tables.json': json.dumps({
            'test': [
              {
                'Permissions': 'probably none'
              }
            ]
          }),
          'manifest.json': '{}',
          'permissions.json': '{}',
          'whats_new.json': json.dumps(_TEST_WHATS_NEW_JSON)
        },
        'public': {
          'apps': {
            'alarm.html': 'alarm.html',
            'app_window.html': 'app_window.html',
          },
          'extensions': {
            'alarm.html': 'alarm.html',
            'browserAction.html': 'browserAction.html',
          }
        }
      }
    }
  },
  '1599': {
    'api': {
      '_api_features.json': json.dumps({
        'alarm': {
          'channel': 'stable'
        },
        'app.window': {
          'channel': 'stable'
        },
        'browserAction': {
          'channel': 'stable'
        },
        'events': {
          'channel': 'master'
        },
        'extension': {
          'channel': 'stable'
        },
        'systemInfo.cpu': {
          'channel': 'beta'
        },
        'systemInfo.stuff': {
          'channel': 'dev'
        }
      }),
      '_manifest_features.json': json.dumps({
        'contextMenus': {
          'channel': 'master'
        },
        'notifications': {
          'channel': 'dev'
        },
        'page_action': {
          'channel': 'stable'
        },
        'runtime': {
          'channel': 'stable'
        },
        'storage': {
          'channel': 'dev'
        },
        'sync': {
          'channel': 'master'
        },
        'system_info_display': {
          'channel': 'stable'
        },
        'web_request': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'alarms': {
          'channel': 'stable'
        },
        'appsFirst': {
          'channel': 'stable',
          'extension_types': ['extension', 'platform_app']
        },
        'bluetooth': {
          'channel': 'dev'
        },
        'bookmarks': {
          'channel': 'stable'
        },
        'cookies': {
          'channel': 'dev'
        },
        'declarativeContent': {
          'channel': 'master'
        },
        'declarativeWebRequest': [
          { 'channel': 'beta' },
          { 'channel': 'stable', 'whitelist': ['aaa'] }
        ],
        'downloads': {
          'channel': 'beta'
        }
      }),
      'alarm.json': json.dumps([{
        'namespace': 'alarm',
        'description': '<code>alarm</code>'
      }]),
      'app_window.json': json.dumps([{
        'namespace': 'app.window',
        'description': '<code>app.window</code>'
      }]),
      'browser_action.json': json.dumps([{
        'namespace': 'browserAction',
        'description': '<code>browserAction</code>'
      }]),
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input_ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
      'windows.json': json.dumps([{'namespace': 'windows'}])
    },
    'docs': {
      'templates': {
        'json': {
          'api_availabilities.json': json.dumps({
            'jsonMasterAPI': {
              'channel': 'master'
            },
            'jsonDevAPI': {
              'channel': 'dev'
            },
            'jsonBetaAPI': {
              'channel': 'beta'
            },
            'jsonStableAPI': {
              'channel': 'stable',
              'version': 20
            }
          }),
          'intro_tables.json': json.dumps({
            'test': [
              {
                'Permissions': 'probably none'
              }
            ]
          }),
          'manifest.json': '{}',
          'permissions.json': '{}',
          'whats_new.json': json.dumps(_TEST_WHATS_NEW_JSON)
        },
        'public': {
          'apps': {
            'alarm.html': 'alarm.html',
            'app_window.html': 'app_window.html',
          },
          'extensions': {
            'alarm.html': 'alarm.html',
            'browserAction.html': 'browserAction.html',
          }
        }
      }
    }
  },
  '1547': {
    'api': {
      '_api_features.json': json.dumps({
        'alarm': {
          'channel': 'stable'
        },
        'app.window': {
          'channel': 'stable'
        },
        'browserAction': {
          'channel': 'stable'
        },
        'events': {
          'channel': 'master'
        },
        'extension': {
          'channel': 'stable'
        },
        'systemInfo.stuff': {
          'channel': 'dev'
        }
      }),
      '_manifest_features.json': json.dumps({
        'contextMenus': {
          'channel': 'master'
        },
        'notifications': {
          'channel': 'dev'
        },
        'page_action': {
          'channel': 'stable'
        },
        'runtime': {
          'channel': 'stable'
        },
        'storage': {
          'channel': 'dev'
        },
        'sync': {
          'channel': 'master'
        },
        'system_info_display': {
          'channel': 'stable'
        },
        'web_request': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'alarms': {
          'channel': 'stable'
        },
        'appsFirst': {
          'channel': 'stable',
          'extension_types': ['extension', 'platform_app']
        },
        'bluetooth': {
          'channel': 'dev'
        },
        'bookmarks': {
          'channel': 'stable'
        },
        'cookies': {
          'channel': 'dev'
        },
        'declarativeContent': {
          'channel': 'master'
        },
        'declarativeWebRequest': [
          { 'channel': 'beta' },
          { 'channel': 'stable', 'whitelist': ['aaa'] }
        ],
        'downloads': {
          'channel': 'beta'
        }
      }),
      'alarm.json': json.dumps([{
        'namespace': 'alarm',
        'description': '<code>alarm</code>'
      }]),
      'app_window.json': json.dumps([{
        'namespace': 'app.window',
        'description': '<code>app.window</code>'
      }]),
      'browser_action.json': json.dumps([{
        'namespace': 'browserAction',
        'description': '<code>browserAction</code>'
      }]),
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input_ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
      'windows.json': json.dumps([{'namespace': 'windows'}])
    },
    'docs': {
      'templates': {
        'json': {
          'api_availabilities.json': json.dumps({
            'jsonMasterAPI': {
              'channel': 'master'
            },
            'jsonDevAPI': {
              'channel': 'dev'
            },
            'jsonBetaAPI': {
              'channel': 'beta'
            },
            'jsonStableAPI': {
              'channel': 'stable',
              'version': 20
            }
          }),
          'intro_tables.json': json.dumps({
            'test': [
              {
                'Permissions': 'probably none'
              }
            ]
          }),
          'manifest.json': '{}',
          'permissions.json': '{}',
          'whats_new.json': json.dumps(_TEST_WHATS_NEW_JSON)
        },
        'public': {
          'apps': {
            'alarm.html': 'alarm.html',
            'app_window.html': 'app_window.html',
          },
          'extensions': {
            'alarm.html': 'alarm.html',
            'browserAction.html': 'browserAction.html',
          }
        }
      }
    }
  },
  '1500': {
    'api': {
      '_api_features.json': json.dumps({
        'alarm': {
          'channel': 'stable'
        },
        'app.window': {
          'channel': 'stable'
        },
        'browserAction': {
          'channel': 'stable'
        },
        'events': {
          'channel': 'master'
        },
        'extension': {
          'channel': 'stable'
        },
        'systemInfo.stuff': {
          'channel': 'dev'
        }
      }),
      '_manifest_features.json': json.dumps({
        'contextMenus': {
          'channel': 'master'
        },
        'notifications': {
          'channel': 'dev'
        },
        'page_action': {
          'channel': 'stable'
        },
        'runtime': {
          'channel': 'stable'
        },
        'storage': {
          'channel': 'dev'
        },
        'sync': {
          'channel': 'master'
        },
        'system_info_display': {
          'channel': 'stable'
        },
        'web_request': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'alarms': {
          'channel': 'stable'
        },
        'appsFirst': {
          'channel': 'stable',
          'extension_types': ['extension', 'platform_app']
        },
        'bluetooth': {
          'channel': 'dev'
        },
        'bookmarks': {
          'channel': 'stable'
        },
        'cookies': {
          'channel': 'dev'
        },
        'declarativeContent': {
          'channel': 'master'
        },
        'declarativeWebRequest': [
          { 'channel': 'beta' },
          { 'channel': 'stable', 'whitelist': ['aaa'] }
        ],
        'downloads': {
          'channel': 'beta'
        }
      }),
      'alarm.json': json.dumps([{
        'namespace': 'alarm',
        'description': '<code>alarm</code>'
      }]),
      'app_window.json': json.dumps([{
        'namespace': 'app.window',
        'description': '<code>app.window</code>'
      }]),
      'browser_action.json': json.dumps([{
        'namespace': 'browserAction',
        'description': '<code>browserAction</code>'
      }]),
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input_ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
      'windows.json': json.dumps([{'namespace': 'windows'}])
    },
    'docs': {
      'templates': {
        'json': {
          'api_availabilities.json': json.dumps({
            'jsonMasterAPI': {
              'channel': 'master'
            },
            'jsonDevAPI': {
              'channel': 'dev'
            },
            'jsonBetaAPI': {
              'channel': 'beta'
            },
            'jsonStableAPI': {
              'channel': 'stable',
              'version': 20
            }
          }),
          'intro_tables.json': json.dumps({
            'test': [
              {
                'Permissions': 'probably none'
              }
            ]
          }),
          'manifest.json': '{}',
          'permissions.json': '{}',
          'whats_new.json': json.dumps(_TEST_WHATS_NEW_JSON)
        },
        'public': {
          'apps': {
            'alarm.html': 'alarm.html',
            'app_window.html': 'app_window.html',
          },
          'extensions': {
            'alarm.html': 'alarm.html',
            'browserAction.html': 'browserAction.html',
          }
        }
      }
    }
  },
  '1453': {
    'api': {
      '_api_features.json': json.dumps({
        'alarm': {
          'channel': 'stable'
        },
        'app.window': {
          'channel': 'stable'
        },
        'browserAction': {
          'channel': 'stable'
        },
        'events': {
          'channel': 'dev'
        },
        'extension': {
          'channel': 'stable'
        },
        'systemInfo.stuff': {
          'channel': 'dev'
        }
      }),
      '_manifest_features.json': json.dumps({
        'notifications': {
          'channel': 'dev'
        },
        'page_action': {
          'channel': 'stable'
        },
        'runtime': {
          'channel': 'stable'
        },
        'storage': {
          'channel': 'dev'
        },
        'system_info_display': {
          'channel': 'stable'
        },
        'web_request': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'alarms': {
          'channel': 'stable'
        },
        'appsFirst': {
          'channel': 'stable',
          'extension_types': ['extension', 'platform_app']
        },
        'bluetooth': {
          'channel': 'dev'
        },
        'bookmarks': {
          'channel': 'stable'
        },
        'context_menus': {
          'channel': 'master'
        },
        'declarativeContent': {
          'channel': 'master'
        },
        'declarativeWebRequest': [
          { 'channel': 'beta' },
          { 'channel': 'stable', 'whitelist': ['aaa'] }
        ],
        'downloads': {
          'channel': 'dev'
        }
      }),
      'alarm.json': json.dumps([{
        'namespace': 'alarm',
        'description': '<code>alarm</code>'
      }]),
      'app_window.json': json.dumps([{
        'namespace': 'app.window',
        'description': '<code>app.window</code>'
      }]),
      'browser_action.json': json.dumps([{
        'namespace': 'browserAction',
        'description': '<code>browserAction</code>'
      }]),
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input_ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
      'windows.json': json.dumps([{'namespace': 'windows'}])
    },
    'docs': {
      'templates': {
        'json': {
          'api_availabilities.json': json.dumps({
            'jsonMasterAPI': {
              'channel': 'master'
            },
            'jsonDevAPI': {
              'channel': 'dev'
            },
            'jsonBetaAPI': {
              'channel': 'beta'
            },
            'jsonStableAPI': {
              'channel': 'stable',
              'version': 20
            }
          }),
          'intro_tables.json': json.dumps({
            'test': [
              {
                'Permissions': 'probably none'
              }
            ]
          }),
          'manifest.json': '{}',
          'permissions.json': '{}',
          'whats_new.json': json.dumps(_TEST_WHATS_NEW_JSON)
        },
        'public': {
          'apps': {
            'alarm.html': 'alarm.html',
            'app_window.html': 'app_window.html',
          },
          'extensions': {
            'alarm.html': 'alarm.html',
            'browserAction.html': 'browserAction.html',
          }

        }
      }
    }
  },
  '1410': {
    'api': {
      '_manifest_features.json': json.dumps({
        'alarm': {
          'channel': 'stable'
        },
        'app.window': {
          'channel': 'stable'
        },
        'browserAction': {
          'channel': 'stable'
        },
        'events': {
          'channel': 'beta'
        },
        'notifications': {
          'channel': 'dev'
        },
        'page_action': {
          'channel': 'stable'
        },
        'runtime': {
          'channel': 'stable'
        },
        'web_request': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'alarms': {
          'channel': 'stable'
        },
        'appsFirst': {
          'channel': 'stable',
          'extension_types': ['extension', 'platform_app']
        },
        'bluetooth': {
          'channel': 'dev'
        },
        'bookmarks': {
          'channel': 'stable'
        },
        'context_menus': {
          'channel': 'master'
        },
        'declarativeContent': {
          'channel': 'master'
        },
        'declarativeWebRequest': [
          { 'channel': 'beta' },
          { 'channel': 'stable', 'whitelist': ['aaa'] }
        ],
        'systemInfo.display': {
          'channel': 'stable'
        }
      }),
      'alarm.json': json.dumps([{
        'namespace': 'alarm',
        'description': '<code>alarm</code>'
      }]),
      'app_window.json': json.dumps([{
        'namespace': 'app.window',
        'description': '<code>app.window</code>'
      }]),
      'browser_action.json': json.dumps([{
        'namespace': 'browserAction',
        'description': '<code>browserAction</code>'
      }]),
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input_ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
      'windows.json': json.dumps([{'namespace': 'windows'}])
    }
  },
  '1364': {
    'api': {
      '_manifest_features.json': json.dumps({
        'page_action': {
          'channel': 'stable'
        },
        'runtime': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'alarms': {
          'channel': 'stable'
        },
        'appsFirst': {
          'channel': 'stable',
          'extension_types': ['platform_app']
        },
        'bookmarks': {
          'channel': 'stable'
        },
        'systemInfo.display': {
          'channel': 'stable'
        },
        'webRequest': {
          'channel': 'stable'
        }
      }),
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input_ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
      'windows.json': json.dumps([{'namespace': 'windows'}])
    }
  },
  '1312': {
    'api': {
      '_manifest_features.json': json.dumps({
        'page_action': {
          'channel': 'stable'
        },
        'runtime': {
          'channel': 'stable'
        },
        'web_request': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'alarms': {
          'channel': 'stable'
        },
        'bookmarks': {
          'channel': 'stable'
        },
        'systemInfo.display': {
          'channel': 'stable'
        }
      }),
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input_ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
      'windows.json': json.dumps([{'namespace': 'windows'}])
    }
  },
  '1271': {
    'api': {
      '_manifest_features.json': json.dumps({
        'page_action': {
          'channel': 'stable'
        },
        'runtime': {
          'channel': 'stable'
        },
        'system_info_display': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'alarms': {
          'channel': 'beta'
        },
        'bookmarks': {
          'channel': 'stable'
        },
        'webRequest': {
          'channel': 'stable'
        }
      }),
      'alarms.idl': '//copy\n\n//desc\nnamespace alarms {}',
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input_ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
      'windows.json': json.dumps([{'namespace': 'windows'}])
    }
  },
  '1229': {
    'api': {
      '_manifest_features.json': json.dumps({
        'page_action': {
          'channel': 'stable'
        },
        'runtime': {
          'channel': 'stable'
        },
        'web_request': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'bookmarks': {
          'channel': 'stable'
        },
        'systemInfo.display': {
          'channel': 'beta'
        }
      }),
      'alarms.idl': '//copy\n\n//desc\nnamespace alarms {}',
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input_ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
    }
  },
  '1180': {
    'api': {
      '_manifest_features.json': json.dumps({
        'page_action': {
          'channel': 'stable'
        },
        'runtime': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'bookmarks': {
          'channel': 'stable'
        },
        'webRequest': {
          'channel': 'stable'
        }
      }),
      'bookmarks.json': json.dumps([{'namespace': 'bookmarks'}]),
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input_ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
    }
  },
  '1132': {
    'api': {
      '_manifest_features.json': json.dumps({
        'bookmarks': {
          'channel': 'master'
        },
        'page_action': {
          'channel': 'stable'
        }
      }),
      '_permission_features.json': json.dumps({
        'webRequest': {
          'channel': 'stable'
        }
      }),
      'bookmarks.json': json.dumps([{'namespace': 'bookmarks'}]),
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input.ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
    }
  },
  '1084': {
    'api': {
      '_manifest_features.json': json.dumps({
        'contents': 'nothing of interest here,really'
      }),
      'bookmarks.json': json.dumps([{'namespace': 'bookmarks'}]),
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input.ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
      'pageAction.json': json.dumps([{'namespace': 'pageAction'}]),
      'webRequest.json': json.dumps([{'namespace': 'webRequest'}])
    }
  },
  '1025': {
    'api': {
      'bookmarks.json': json.dumps([{'namespace': 'bookmarks'}]),
      'idle.json': json.dumps([{'namespace': 'idle'}]),
      'input.ime.json': json.dumps([{'namespace': 'input.ime'}]),
      'menus.json': json.dumps([{'namespace': 'menus'}]),
      'tabs.json': json.dumps([{'namespace': 'tabs'}]),
      'pageAction.json': json.dumps([{'namespace': 'pageAction'}]),
      'webRequest.json': json.dumps([{'namespace': 'webRequest'}])
    }
  },
  '963': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'menus'
        },
        {
          'namespace': 'pageAction'
        },
        {
          'namespace': 'webRequest'
        }
      ])
    }
  },
  '912': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'menus'
        },
        {
          'namespace': 'pageAction'
        },
        {
          'namespace': 'experimental.webRequest'
        }
      ])
    }
  },
  '874': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'menus'
        },
        {
          'namespace': 'pageAction'
        }
      ])
    }
  },
  '835': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'menus'
        },
        {
          'namespace': 'pageAction'
        }
      ])
    }
  },
  '782': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'menus'
        },
        {
          'namespace': 'pageAction'
        }
      ])
    }
  },
  '742': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'menus'
        },
        {
          'namespace': 'pageAction'
        }
      ])
    }
  },
  '696': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'menus'
        },
        {
          'namespace': 'pageAction'
        }
      ])
    }
  },
  '648': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'menus'
        },
        {
          'namespace': 'pageAction'
        }
      ])
    }
  },
  '597': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'menus'
        },
        {
          'namespace': 'pageAction'
        }
      ])
    }
  },
  '552': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'menus'
        },
        {
          'namespace': 'pageAction'
        }
      ])
    }
  },
  '544': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'menus'
        }
      ])
    }
  },
  '495': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'menus'
        }
      ])
    }
  },
  '396': {
    'api': {
      'extension_api.json': json.dumps([
        {
          'namespace': 'idle'
        },
        {
          'namespace': 'experimental.menus'
        }
      ])
    }
  }
})
