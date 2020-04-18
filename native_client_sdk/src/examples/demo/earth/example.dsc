{
  'TOOLS': ['clang-newlib', 'glibc', 'pnacl'],
  'TARGETS': [
    {
      'NAME' : 'earth',
      'TYPE' : 'main',
      'SOURCES' : [
        'earth.cc'
      ],
     'LIBS': ['ppapi_simple_cpp', 'nacl_io', 'sdk_util', 'ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'earth.jpg',
    'earthnight.jpg',
    'example.js',
  ],
  'DEST': 'examples/demo',
  'NAME': 'earth',
  'TITLE': 'Multi-Threaded Earth Demo',
  'GROUP': 'Demo'
}
