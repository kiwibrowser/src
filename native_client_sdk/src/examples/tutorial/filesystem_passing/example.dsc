{
  'TARGETS': [
    {
      'NAME' : 'filesystem_passing',
      'TYPE' : 'main',
      'SOURCES' : ['filesystem_passing.cc'],
      'LIBS' : ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/tutorial',
  'NAME': 'filesystem_passing',
  'TITLE': 'Filesystem Passing',
  'GROUP': 'Tutorial',
  'FILESYSTEM_PERMISSIONS': [
    'write',
    'directory'
  ]
}
