{
  'TARGETS': [
    {
      'NAME' : 'graphics_2d',
      'TYPE' : 'main',
      'SOURCES' : [
        'graphics_2d.cc',
      ],
      'LIBS': ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DEST': 'examples/api',
  'NAME': 'graphics_2d',
  'TITLE': 'Graphics 2D',
  'GROUP': 'API'
}
