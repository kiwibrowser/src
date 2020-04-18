{
  'TARGETS': [
    {
      'NAME' : 'file_io',
      'TYPE' : 'main',
      'SOURCES' : ['file_io.cc'],
      'LIBS' : ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/api',
  'NAME': 'file_io',
  'TITLE': 'File I/O',
  'GROUP': 'API',
  'PERMISSIONS': [
    'unlimitedStorage'
  ]
}

