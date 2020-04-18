{
  'TARGETS': [
    {
      'NAME' : 'messaging',
      'TYPE' : 'main',
      'SOURCES' : ['messaging.cc'],
      'LIBS' : ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/api',
  'NAME': 'messaging',
  'TITLE': 'Messaging',
  'GROUP': 'API'
}

