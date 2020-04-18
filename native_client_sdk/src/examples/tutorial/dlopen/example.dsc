{
  'TOOLS': ['glibc'],
  'TARGETS': [
    {
      'NAME': 'dlopen',
      'TYPE': 'main',
      'SOURCES': ['dlopen.cc'],
      'DEPS': ['nacl_io', 'ppapi_cpp'],
      'LIBS': ['nacl_io', 'ppapi_cpp', 'ppapi', 'dl', 'pthread']
    },
    {
      'NAME' : 'eightball',
      'TYPE' : 'so',
      'SOURCES' : ['eightball.cc', 'eightball.h'],
      'LIBS' : ['ppapi_cpp', 'ppapi', 'pthread']
    },
    {
      'NAME' : 'reverse',
      # This .so file is manually loaded by dlopen; we don't want to include it
      # in the .nmf, or it will be automatically loaded on startup.
      'TYPE' : 'so-standalone',
      'SOURCES' : ['reverse.cc', 'reverse.h'],
      'LIBS' : ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/tutorial',
  'NAME': 'dlopen',
  'TITLE': 'Dynamic Library Open',
  'GROUP': 'Tutorial'
}

