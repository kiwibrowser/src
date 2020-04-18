{
  'TOOLS': ['clang-newlib', 'glibc', 'pnacl'],
  'TARGETS': [
    {
      'NAME' : 'flock',
      'TYPE' : 'main',
      'SOURCES' : [
        'flock.cc',
        'goose.cc',
        'goose.h',
        'sprite.cc',
        'sprite.h',
        'vector2.h'
      ],
      'DEPS': ['ppapi_simple', 'nacl_io'],
      'LIBS': ['ppapi_simple_cpp', 'nacl_io', 'ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'images/flock_green.raw'
  ],
  'DEST': 'examples/demo',
  'NAME': 'flock',
  'TITLE': 'Flocking Geese',
  'GROUP': 'Demo'
}
