{
  'TOOLS': ['clang-newlib', 'glibc', 'pnacl', 'linux', 'mac'],
  'TARGETS': [
    {
      'NAME' : 'nacl_io_demo',
      'TYPE' : 'main',
      'SOURCES' : [
        'handlers.c',
        'handlers.h',
        'nacl_io_demo.c',
        'nacl_io_demo.h',
        'queue.c',
        'queue.h',
      ],
      'DEPS': ['nacl_io'],
      'LIBS': ['nacl_io', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js'
  ],
  'DEST': 'examples/demo',
  'NAME': 'nacl_io_demo',
  'TITLE': 'NaCl IO Demo',
  'GROUP': 'Demo',
  'PERMISSIONS': [
    'unlimitedStorage'
  ],
  'SOCKET_PERMISSIONS': [
    'resolve-host'
  ]
}
