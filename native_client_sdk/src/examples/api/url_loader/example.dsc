{
  'TARGETS': [
    {
      'NAME' : 'url_loader',
      'TYPE' : 'main',
      'SOURCES' : [
        'url_loader.cc',
        'url_loader_handler.cc',
        'url_loader_handler.h'
      ],
      'LIBS': ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js',
    'url_loader_success.html',
  ],
  'DEST': 'examples/api',
  'NAME': 'url_loader',
  'TITLE': 'URL Loader',
  'GROUP': 'API'
}

