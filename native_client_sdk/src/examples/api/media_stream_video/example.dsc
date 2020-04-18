{
  'TARGETS': [
    {
      'NAME' : 'media_stream_video',
      'TYPE' : 'main',
      'SOURCES' : [
        'media_stream_video.cc',
      ],
      'LIBS': ['ppapi_gles2', 'ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js'
  ],
  'DEST': 'examples/api',
  'NAME': 'media_stream_video',
  'TITLE': 'MediaStream Video',
  'GROUP': 'API',
  'MIN_CHROME_VERSION': '35.0.0.0',
  'PERMISSIONS': ['videoCapture']
}
