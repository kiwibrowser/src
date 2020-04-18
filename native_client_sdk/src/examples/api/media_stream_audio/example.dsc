{
  'TARGETS': [
    {
      'NAME' : 'media_stream_audio',
      'TYPE' : 'main',
      'SOURCES' : [
        'media_stream_audio.cc',
      ],
      'LIBS': ['ppapi_gles2', 'ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js'
  ],
  'DEST': 'examples/api',
  'NAME': 'media_stream_audio',
  'TITLE': 'MediaStream Audio',
  'GROUP': 'API',
  'MIN_CHROME_VERSION': '35.0.0.0',
  'PERMISSIONS': ['audioCapture']
}
