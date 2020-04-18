{
  'DISABLE_PACKAGE': True,
  'TARGETS': [
    {
      'NAME' : 'mouse_lock',
      'TYPE' : 'main',
      'SOURCES' : ['mouse_lock.cc', 'mouse_lock.h'],
      'LIBS': ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DEST': 'examples/api',
  'NAME': 'mouse_lock',
  'TITLE': 'Mouse Lock',
  'GROUP': 'API',
  'PERMISSIONS': [
    'fullscreen',
    'pointerLock'
  ]
}

