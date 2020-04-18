{
  'TOOLS': ['clang-newlib', 'glibc', 'pnacl', 'linux', 'mac'],
  'TARGETS': [
    {
      'NAME' : 'life',
      'TYPE' : 'main',
      'SOURCES' : [
        'life.c',
      ],
      'DEPS': ['ppapi_simple', 'nacl_io'],
      'LIBS': ['ppapi_simple', 'nacl_io', 'ppapi', 'pthread']
    }
  ],
  'DEST': 'examples/demo',
  'NAME': 'life',
  'TITLE': "Conway's Life",
  'GROUP': 'Demo'
}
