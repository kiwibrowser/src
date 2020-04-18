{
  'TOOLS': ['pnacl'],
  'TARGETS': [
    {
      'NAME' : 'earth_simd',
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
  'NAME': 'earth_simd',
  'TITLE': 'Multi-Threaded SIMD Earth Demo for PNaCl',
  'GROUP': 'Demo'
}
