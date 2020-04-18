{
  'TARGETS': [
    {
      'NAME' : 'load_progress',
      'TYPE' : 'main',
      'SOURCES' : ['load_progress.cc'],
      'LIBS': ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/tutorial',
  'NAME': 'load_progress',
  'TITLE': 'Load Progress',
  'GROUP': 'Tutorial'
}

