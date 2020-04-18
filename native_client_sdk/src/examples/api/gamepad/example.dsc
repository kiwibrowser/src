{
  'TARGETS': [
    {
      'NAME' : 'gamepad',
      'TYPE' : 'main',
      'SOURCES' : ['gamepad.cc'],
      'LIBS': ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DEST': 'examples/api',
  'NAME': 'gamepad',
  'TITLE': 'Gamepad',
  'GROUP': 'API'
}

