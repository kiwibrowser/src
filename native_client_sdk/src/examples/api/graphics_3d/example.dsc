{
  'TARGETS': [
    {
      'NAME' : 'graphics_3d',
      'TYPE' : 'main',
      'SOURCES' : [
        'graphics_3d.cc',
        'matrix.cc',
        'matrix.h',
        'texture.cc',
      ],
      'LIBS': ['ppapi_gles2', 'ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js'
  ],
  'DEST': 'examples/api',
  'NAME': 'graphics_3d',
  'TITLE': 'Graphics 3D',
  'GROUP': 'API'
}



