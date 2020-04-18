{
  'TARGETS': [
    {
      'NAME' : 'video_encode',
      'TYPE' : 'main',
      'SOURCES' : [
        'video_encode.cc',
      ],
      'LIBS': ['ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js'
  ],
  'DEST': 'examples/api',
  'NAME': 'video_encode',
  'TITLE': 'Video Encode',
  'GROUP': 'API',
  'PERMISSIONS': ['videoCapture'],
  'MIN_CHROME_VERSION': '43.0.0.0'
}
