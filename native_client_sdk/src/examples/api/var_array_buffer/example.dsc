{
  'TARGETS': [
    {
      'NAME' : 'var_array_buffer',
      'TYPE' : 'main',
      'SOURCES' : ['var_array_buffer.cc'],
      'LIBS' : ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
  ],
  'DEST': 'examples/api',
  'NAME': 'var_array_buffer',
  'TITLE': 'Var Array Buffer',
  'GROUP': 'API'
}

