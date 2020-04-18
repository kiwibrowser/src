{
  'TARGETS': [
    {
      'NAME' : 'input_event',
      'TYPE' : 'main',
      'SOURCES' : [
        'input_event.cc',
      ],
      'LIBS': ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/api',
  'NAME': 'input_event',
  'TITLE': 'Input Event',
  'GROUP': 'API',
}

