{
  'TARGETS': [
    {
      'NAME' : 'core',
      'TYPE' : 'main',
      'SOURCES' : ['core.cc'],
      'LIBS' : ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/api',
  'NAME': 'core',
  'TITLE': 'Core',
  'GROUP': 'API'
}

